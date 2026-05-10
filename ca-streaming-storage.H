/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file ca-streaming-storage.H
 *  @brief Phase 15 tile-based out-of-core storage for `Aleph::CA`.
 *
 *  `Tile_Cache<State, TileSide>` exposes a logical 2D grid of any
 *  size while keeping only a bounded set of resident tiles in RAM.
 *  Tiles that fall out of the LRU cache are evicted to disk as raw
 *  little-endian binary files; subsequent accesses page them back
 *  in transparently.
 *
 *  The cache is intentionally simple compared to a full mmap-backed
 *  engine: it favours portability (works on any filesystem with no
 *  POSIX dependencies) and inspectability (each tile is a separate
 *  file you can examine with a hex dump). When more aggressive
 *  out-of-core performance is required, the same tile layout can be
 *  consumed by a custom mmap loader without changing client code.
 *
 *  Design contract:
 *
 *    - Tiles are square of side `TileSide` (compile-time constant).
 *    - The grid extents must be multiples of `TileSide`. Out-of-bound
 *      tiles are not allocated.
 *    - Eviction is LRU; capacity (number of resident tiles) is set at
 *      construction.
 *    - Each tile lives on disk as `tile_<row>_<col>.bin`, with
 *      `TileSide * TileSide * sizeof(State)` raw bytes.
 *    - The cache is not thread-safe.
 *
 *  Typical usage:
 *
 *  ```cpp
 *  Tile_Cache<int, 64> tiles({1024, 1024}, "/tmp/sim", 16);
 *  tiles.set(123, 456, 1);
 *  const int v = tiles.at(123, 456);
 *  tiles.flush();          // persist resident tiles to disk
 *  ```
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_STREAMING_STORAGE_H
#define CA_STREAMING_STORAGE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-traits.H>

namespace Aleph {
namespace CA {

namespace ca_stream_detail {

/// Pack `(row, col)` into a stable 64-bit key for the tile map.
[[nodiscard]] inline constexpr std::uint64_t tile_key(const std::uint32_t row,
                                                      const std::uint32_t col) noexcept
{
  return (static_cast<std::uint64_t>(row) << 32) | static_cast<std::uint64_t>(col);
}

}  // namespace ca_stream_detail

/** @brief Statistics surfaced by `Tile_Cache::stats()`.
 *
 *  Useful as a smoke check that the LRU is doing its job in long
 *  runs: the hit ratio should stay high if tile visits respect a
 *  predictable order, and `evictions` should be bounded by the
 *  number of distinct tiles touched minus the cache capacity.
 */
struct Tile_Cache_Stats
{
  std::uint64_t hits = 0;        ///< accesses served from RAM
  std::uint64_t misses = 0;      ///< accesses that paged a tile in
  std::uint64_t evictions = 0;   ///< tiles evicted to disk to make room
  std::uint64_t writes = 0;      ///< tile writes to disk (eviction or flush)
  std::uint64_t reads = 0;       ///< tile reads from disk (load on miss)
};

/** @brief Out-of-core 2D tile cache with LRU eviction.
 *
 *  The cache models a logical `rows × cols` grid divided into
 *  `TileSide × TileSide` tiles. The grid extents must be multiples
 *  of `TileSide`. Up to `capacity` tiles are kept resident in RAM;
 *  cache misses page the tile in from disk (or zero-initialise it
 *  on first touch) and evict the least-recently-used tile if needed.
 *
 *  @tparam State cell value type. Must be trivially copyable.
 *  @tparam TileSide compile-time tile side length.
 */
template <typename State, std::size_t TileSide>
class Tile_Cache
{
  static_assert(std::is_trivially_copyable_v<State>,
                "Tile_Cache requires a trivially copyable State");
  static_assert(TileSide >= 1, "Tile_Cache requires TileSide >= 1");

public:
  /// Cell value type.
  using state_type = State;
  /// Tile side length (in cells).
  static constexpr std::size_t tile_side = TileSide;
  /// Number of cells per tile.
  static constexpr std::size_t tile_cells = TileSide * TileSide;

private:
  using tile_buffer = std::array<State, tile_cells>;

  struct Tile_Entry
  {
    std::uint32_t row;
    std::uint32_t col;
    tile_buffer data{};
    bool dirty = false;
  };

  using lru_list = std::list<Tile_Entry>;
  using lru_iter = typename lru_list::iterator;
  using key_map = std::unordered_map<std::uint64_t, lru_iter>;

  ca_size_t rows_ = 0;
  ca_size_t cols_ = 0;
  ca_size_t tiles_per_row_ = 0;
  ca_size_t tiles_per_col_ = 0;
  std::filesystem::path dir_;
  std::size_t capacity_ = 0;
  lru_list cache_;
  key_map index_;
  Tile_Cache_Stats stats_;

  [[nodiscard]] std::filesystem::path tile_path(std::uint32_t row, std::uint32_t col) const
  {
    return dir_
           / ("tile_" + std::to_string(row) + "_" + std::to_string(col) + ".bin");
  }

  /// Read tile from disk into `buf`. Initialises `buf` to State{} when
  /// the file does not yet exist (first touch).
  void read_tile_from_disk(std::uint32_t row, std::uint32_t col, tile_buffer &buf)
  {
    const auto path = tile_path(row, col);
    if (not std::filesystem::exists(path))
      {
        buf.fill(State{});
        return;
      }
    std::ifstream in(path, std::ios::binary);
    ah_runtime_error_if(not in)
      << "Tile_Cache: cannot read tile (" << row << "," << col << ") from '"
      << path.string() << "'";
    in.read(reinterpret_cast<char *>(buf.data()),
            static_cast<std::streamsize>(tile_cells * sizeof(State)));
    ah_runtime_error_if(in.gcount() != static_cast<std::streamsize>(tile_cells * sizeof(State)))
      << "Tile_Cache: short read of tile (" << row << "," << col << ")";
    ++stats_.reads;
  }

  /// Persist tile to disk.
  void write_tile_to_disk(const Tile_Entry &entry)
  {
    const auto path = tile_path(entry.row, entry.col);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ah_runtime_error_if(not out)
      << "Tile_Cache: cannot write tile (" << entry.row << "," << entry.col
      << ") to '" << path.string() << "'";
    out.write(reinterpret_cast<const char *>(entry.data.data()),
              static_cast<std::streamsize>(tile_cells * sizeof(State)));
    ah_runtime_error_if(not out)
      << "Tile_Cache: stream error while writing tile (" << entry.row << ","
      << entry.col << ")";
    ++stats_.writes;
  }

  /// Move `it` to the front of the LRU list (mark as most-recent).
  void touch(lru_iter it)
  {
    if (it != cache_.begin())
      cache_.splice(cache_.begin(), cache_, it);
  }

  /// Page a tile in, evicting the LRU when full.
  lru_iter page_in(std::uint32_t row, std::uint32_t col)
  {
    if (cache_.size() >= capacity_)
      {
        Tile_Entry &victim = cache_.back();
        if (victim.dirty)
          write_tile_to_disk(victim);
        index_.erase(ca_stream_detail::tile_key(victim.row, victim.col));
        cache_.pop_back();
        ++stats_.evictions;
      }
    Tile_Entry entry;
    entry.row = row;
    entry.col = col;
    read_tile_from_disk(row, col, entry.data);
    cache_.push_front(std::move(entry));
    const auto it = cache_.begin();
    index_[ca_stream_detail::tile_key(row, col)] = it;
    return it;
  }

  /// Locate the tile that owns `(r, c)`, paging it in if absent.
  lru_iter resolve(ca_size_t r, ca_size_t c)
  {
    const std::uint32_t tr = static_cast<std::uint32_t>(r / TileSide);
    const std::uint32_t tc = static_cast<std::uint32_t>(c / TileSide);
    const auto key = ca_stream_detail::tile_key(tr, tc);
    if (auto map_it = index_.find(key); map_it != index_.end())
      {
        ++stats_.hits;
        touch(map_it->second);
        return map_it->second;
      }
    ++stats_.misses;
    return page_in(tr, tc);
  }

  static std::size_t local_index(ca_size_t r, ca_size_t c) noexcept
  {
    const std::size_t lr = r % TileSide;
    const std::size_t lc = c % TileSide;
    return lr * TileSide + lc;
  }

public:
  /** @brief Build a tile cache.
   *
   *  Creates `dir` if it does not exist. Tile files are written into
   *  this directory on demand.
   *
   *  @param[in] extents `{rows, cols}` of the logical grid. Both
   *             must be > 0 and multiples of `TileSide`.
   *  @param[in] dir directory backing the tiles on disk.
   *  @param[in] capacity maximum number of tiles kept resident.
   *  @throws std::domain_error if `capacity == 0` or extents are not
   *          multiples of `TileSide`.
   *  @throws std::filesystem::filesystem_error if the directory
   *          cannot be created.
   */
  Tile_Cache(const std::array<ca_size_t, 2> &extents,
             std::filesystem::path dir,
             const std::size_t capacity)
    : rows_(extents[0]), cols_(extents[1]), dir_(std::move(dir)), capacity_(capacity)
  {
    ah_domain_error_if(capacity == 0)
      << "Tile_Cache: capacity must be >= 1";
    ah_domain_error_if(rows_ == 0 or cols_ == 0)
      << "Tile_Cache: extents must be positive";
    ah_domain_error_if(rows_ % TileSide != 0 or cols_ % TileSide != 0)
      << "Tile_Cache: extents (" << rows_ << "," << cols_
      << ") must be multiples of TileSide=" << TileSide;
    tiles_per_row_ = rows_ / TileSide;
    tiles_per_col_ = cols_ / TileSide;
    if (not std::filesystem::exists(dir_))
      std::filesystem::create_directories(dir_);
  }

  /// @return logical grid extents.
  [[nodiscard]] std::array<ca_size_t, 2> extents() const noexcept
  {
    return {rows_, cols_};
  }

  /// @return number of tiles along axis 0.
  [[nodiscard]] ca_size_t tiles_rows() const noexcept { return tiles_per_row_; }
  /// @return number of tiles along axis 1.
  [[nodiscard]] ca_size_t tiles_cols() const noexcept { return tiles_per_col_; }
  /// @return resident tile capacity (LRU upper bound).
  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
  /// @return number of tiles currently resident.
  [[nodiscard]] std::size_t resident() const noexcept { return cache_.size(); }
  /// @return cache statistics snapshot.
  [[nodiscard]] Tile_Cache_Stats stats() const noexcept { return stats_; }

  /** @brief Read the cell at `(r, c)`.
   *
   *  Pages the owning tile in if it is not resident. Marks the tile
   *  as most-recently-used.
   *
   *  @param[in] r row index in `[0, rows())`.
   *  @param[in] c column index in `[0, cols())`.
   *  @return cell value.
   *  @throws std::out_of_range if `(r, c)` is outside the grid.
   */
  [[nodiscard]] State at(ca_size_t r, ca_size_t c)
  {
    ah_out_of_range_error_if(r >= rows_ or c >= cols_)
      << "Tile_Cache::at: (" << r << "," << c << ") outside ("
      << rows_ << "," << cols_ << ")";
    auto it = resolve(r, c);
    return it->data[local_index(r, c)];
  }

  /** @brief Write the cell at `(r, c)` and mark its tile dirty.
   *
   *  @param[in] r row index in `[0, rows())`.
   *  @param[in] c column index in `[0, cols())`.
   *  @param[in] v new cell value.
   *  @throws std::out_of_range if `(r, c)` is outside the grid.
   */
  void set(ca_size_t r, ca_size_t c, const State &v)
  {
    ah_out_of_range_error_if(r >= rows_ or c >= cols_)
      << "Tile_Cache::set: (" << r << "," << c << ") outside ("
      << rows_ << "," << cols_ << ")";
    auto it = resolve(r, c);
    it->data[local_index(r, c)] = v;
    it->dirty = true;
  }

  /** @brief Persist every dirty resident tile to disk and clear the
   *         dirty flag.
   *
   *  After `flush` the on-disk image is consistent with what the
   *  cache currently holds. Resident tiles stay in the cache so the
   *  next access remains a hit.
   *
   *  @throws std::runtime_error on stream write failure.
   */
  void flush()
  {
    for (auto &entry : cache_)
      if (entry.dirty)
        {
          write_tile_to_disk(entry);
          entry.dirty = false;
        }
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_STREAMING_STORAGE_H