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

/** @file ca-tiling.H
 *  @brief Domain-decomposition utilities for the parallel CA engine.
 *
 *  This Phase-5 helper centralises the deterministic, balanced
 *  partitioning of a cellular-automaton iteration domain across an
 *  arbitrary number of workers. The strategies are intentionally pure
 *  arithmetic so the parallel engine can split and re-split without any
 *  synchronisation cost: every worker computes its slice from
 *  `(rank, extents, partition_idx, num_partitions)`.
 *
 *  Partitions produced here cover the iteration domain exactly once
 *  with disjoint, contiguous ranges along the chosen axis or tile grid.
 *  This is the minimal contract required for the synchronous CA step,
 *  whose writes go to the next-buffer only and never read back from it
 *  inside the same step.
 *
 *  Catalogue:
 *
 *    - `Range1D`              : `[begin, end)` interval helper.
 *    - `split_range_balanced` : balanced split of `[0, n)` into `parts`.
 *    - `Row_Partition`        : axis-0 row strips for any rank.
 *    - `Column_Partition`     : axis-(rank-1) column strips for any rank.
 *    - `Block_Partition_2D`   : 2D tile grid for `rank == 2` lattices.
 *    - `morton_encode_2d` /
 *      `morton_encode_3d`     : Z-order encoding (cache-friendly schedule
 *                               hint for higher-dimensional CAs).
 *
 *  Every helper is `constexpr` where the standard library allows and
 *  has no run-time allocations. The primitives below are independent of
 *  the engine: they can be reused by hex/triangular variants in Phase 6
 *  or by the IO helpers in Phase 11 as long as the iteration domain is
 *  rectangular.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_TILING_H
#define CA_TILING_H

#include <array>
#include <cstddef>
#include <cstdint>

#include <ah-errors.H>

#include <ca-traits.H>

namespace Aleph {
namespace CA {

/** @brief Half-open integer interval `[begin, end)`.
 *
 *  Used everywhere a "slice along an axis" must be passed around. An
 *  empty range (`begin == end`) is the canonical representation for a
 *  no-op partition (e.g., when the number of partitions exceeds the
 *  number of cells along the split axis).
 */
struct Range1D
{
  /// Inclusive lower bound.
  ca_size_t begin = 0;
  /// Exclusive upper bound.
  ca_size_t end = 0;

  /// @return number of indices in the range.
  [[nodiscard]] constexpr ca_size_t size() const noexcept
  {
    return end > begin ? end - begin : 0;
  }

  /// @return true when the range covers no index.
  [[nodiscard]] constexpr bool empty() const noexcept
  {
    return end <= begin;
  }
};

/** @brief Balanced split of `[0, n)` into `parts` contiguous ranges.
 *
 *  The first `n % parts` ranges receive one extra index, so that the
 *  size disparity between any two ranges never exceeds 1. This is the
 *  classic "block-cyclic" balance used by OpenMP `static`. The function
 *  is total: passing `parts == 0` yields the single full range and
 *  passing `idx >= parts` yields an empty range. This mirrors the
 *  default behaviour of `std::partition_point`-style helpers and is the
 *  cleanest way to handle pool-size-dependent loops at compile time.
 *
 *  @param[in] n     total number of indices to split.
 *  @param[in] parts desired number of partitions (`0` is treated as 1).
 *  @param[in] idx   partition index in `[0, parts)`.
 *  @return the half-open range owned by `idx`.
 *
 *  @par Complexity
 *  O(1).
 */
[[nodiscard]] inline constexpr Range1D split_range_balanced(const ca_size_t n,
                                                            const ca_size_t parts,
                                                            const ca_size_t idx) noexcept
{
  if (parts <= 1 or n == 0)
    {
      if (idx == 0)
        return Range1D{0, n};
      return Range1D{n, n};
    }
  if (idx >= parts)
    return Range1D{n, n};

  const ca_size_t base = n / parts;
  const ca_size_t rem = n % parts;
  const ca_size_t begin = idx * base + (idx < rem ? idx : rem);
  const ca_size_t extra = idx < rem ? 1 : 0;
  const ca_size_t end = begin + base + extra;
  return Range1D{begin, end};
}

/** @brief Decide whether a workload should run sequentially.
 *
 *  Returns `true` when the workload is small enough that the cost of
 *  scheduling work onto a thread pool would dominate the actual cell
 *  updates. The heuristic is intentionally tunable from the call site
 *  rather than baked in: small grids on a deep pool benefit from this
 *  cutoff, and larger grids should always parallelise.
 *
 *  @param[in] cells          total cell count to update.
 *  @param[in] num_partitions number of partitions that would be spawned.
 *  @param[in] min_cells      minimum work per partition before
 *                            parallelisation pays off.
 *
 *  @return `true` when the workload should be executed sequentially.
 */
[[nodiscard]] inline constexpr bool should_run_sequential(const ca_size_t cells,
                                                          const ca_size_t num_partitions,
                                                          const ca_size_t min_cells) noexcept
{
  return num_partitions <= 1 or cells == 0 or cells < min_cells;
}

/** @brief Row partitioning along axis 0.
 *
 *  For row-major storage this produces contiguous slabs of memory per
 *  worker and is therefore the friendliest schedule for the L1/L2 cache
 *  hierarchy. It works for every rank (`1`, `2`, `3`).
 *
 *  @tparam Rank lattice dimension (must match the engine's).
 */
template <std::size_t Rank>
struct Row_Partition
{
  static_assert(Rank >= 1, "Row_Partition requires Rank >= 1");

  /// Number of partitions along axis 0 with a balanced split. Each
  /// other axis is iterated in full inside every partition.
  ///
  /// @param[in] extents user-visible extents of the lattice.
  /// @param[in] parts   requested number of partitions.
  /// @param[in] idx     partition index.
  /// @return the half-open row range owned by `idx`.
  [[nodiscard]] static constexpr Range1D slab(const std::array<ca_size_t, Rank> &extents,
                                              const ca_size_t parts,
                                              const ca_size_t idx) noexcept
  {
    return split_range_balanced(extents[0], parts, idx);
  }
};

/** @brief Column partitioning along the last axis.
 *
 *  Columns are *not* contiguous in row-major memory, so this strategy
 *  trades cache locality for finer-grained partitioning when axis 0 is
 *  small (e.g. a 1D very-tall lattice or a 2D lattice with one row).
 *  Provided for completeness; `Row_Partition` is the default in the
 *  engine.
 *
 *  @tparam Rank lattice dimension.
 */
template <std::size_t Rank>
struct Column_Partition
{
  static_assert(Rank >= 1, "Column_Partition requires Rank >= 1");

  /// Half-open column range owned by partition `idx`.
  [[nodiscard]] static constexpr Range1D slab(const std::array<ca_size_t, Rank> &extents,
                                              const ca_size_t parts,
                                              const ca_size_t idx) noexcept
  {
    return split_range_balanced(extents[Rank - 1], parts, idx);
  }
};

/** @brief 2D tile descriptor: a rectangle in axis-0 / axis-1 coordinates.
 */
struct Tile2D_Range
{
  /// Half-open axis-0 range.
  Range1D rows{};
  /// Half-open axis-1 range.
  Range1D cols{};

  /// @return number of cells covered by the tile.
  [[nodiscard]] constexpr ca_size_t size() const noexcept
  {
    return rows.size() * cols.size();
  }

  /// @return true when the tile is degenerate.
  [[nodiscard]] constexpr bool empty() const noexcept
  {
    return rows.empty() or cols.empty();
  }
};

/** @brief 2D blocked partitioning into a grid of `parts_y x parts_x`.
 *
 *  Each tile is balanced along both axes, mirroring the structure of
 *  `Synchronous_Engine`'s `Tile<W,H>` schedule. Useful when a lattice
 *  is wide enough that splitting along axis 0 alone leaves rows so
 *  long that they exceed the L2 working set.
 *
 *  Tiles are enumerated row-major over the tile grid:
 *
 *      tile_index = ty * parts_x + tx
 *
 *  with `ty` in `[0, parts_y)` and `tx` in `[0, parts_x)`.
 */
struct Block_Partition_2D
{
  /// Tile owned by `tile_index` over the `parts_y * parts_x` grid.
  ///
  /// @param[in] extents   2D lattice extents `(rows, cols)`.
  /// @param[in] parts_y   number of partitions along axis 0.
  /// @param[in] parts_x   number of partitions along axis 1.
  /// @param[in] tile_index linear tile index.
  /// @return the rectangle covered by `tile_index`.
  [[nodiscard]] static constexpr Tile2D_Range tile(const std::array<ca_size_t, 2> &extents,
                                                   const ca_size_t parts_y,
                                                   const ca_size_t parts_x,
                                                   const ca_size_t tile_index) noexcept
  {
    const ca_size_t py = parts_y == 0 ? 1 : parts_y;
    const ca_size_t px = parts_x == 0 ? 1 : parts_x;
    const ca_size_t total = py * px;
    if (tile_index >= total)
      return Tile2D_Range{Range1D{extents[0], extents[0]}, Range1D{extents[1], extents[1]}};
    const ca_size_t ty = tile_index / px;
    const ca_size_t tx = tile_index % px;
    return Tile2D_Range{split_range_balanced(extents[0], py, ty),
                        split_range_balanced(extents[1], px, tx)};
  }

  /// Pick a balanced `(parts_y, parts_x)` factorisation of `parts`.
  ///
  /// The factorisation prefers shapes whose aspect ratio matches the
  /// lattice aspect ratio: tall lattices get more `parts_y`, wide ones
  /// get more `parts_x`. The result is symmetric around `sqrt(parts)`
  /// when the lattice is roughly square.
  ///
  /// @param[in] extents 2D lattice extents.
  /// @param[in] parts   total number of partitions.
  /// @return `{parts_y, parts_x}` such that `parts_y * parts_x == parts`.
  [[nodiscard]] static constexpr std::array<ca_size_t, 2> factor_partitions(
    const std::array<ca_size_t, 2> &extents, const ca_size_t parts) noexcept
  {
    if (parts <= 1)
      return {1, 1};

    // Find the divisor pair (a, b) of `parts` whose ratio b/a best
    // matches cols/rows. Iterate over divisors only; `parts` is small
    // (a few dozen at most) so the linear search is trivial.
    ca_size_t best_y = 1;
    ca_size_t best_x = parts;
    long long best_score = -1;
    const long long rows = static_cast<long long>(extents[0]);
    const long long cols = static_cast<long long>(extents[1]);

    for (ca_size_t y = 1; y <= parts; ++y)
      {
        if (parts % y != 0)
          continue;
        const ca_size_t x = parts / y;
        // score = - |rows / y - cols / x| (closer to balanced => better)
        const long long ry = rows / static_cast<long long>(y);
        const long long rx = cols / static_cast<long long>(x);
        long long diff = ry - rx;
        if (diff < 0)
          diff = -diff;
        const long long score = -diff;
        if (score > best_score)
          {
            best_score = score;
            best_y = y;
            best_x = x;
          }
      }
    return {best_y, best_x};
  }
};

// ---------------------------------------------------------------------------
// Z-order (Morton) encoding helpers.
//
// They are not used by the default partitioner, but they are documented as
// part of the Phase-5 deliverables and exposed here so that future engines
// (parallel 3D, hashlife streaming) can opt into a cache-friendly visit
// order without having to redefine the bit-twiddling each time.
// ---------------------------------------------------------------------------

namespace ca_tiling_detail {

/// Spread the low `bits` bits of `v` so that they occupy even positions.
/// Iterative implementation; works for any reasonable bit count.
inline constexpr std::uint64_t spread_bits_2(const std::uint32_t v) noexcept
{
  std::uint64_t r = 0;
  for (std::size_t i = 0; i < 32; ++i)
    r |= (static_cast<std::uint64_t>(v >> i) & 1ULL) << (2 * i);
  return r;
}

/// Spread the low `bits` bits of `v` so that they occupy every third
/// position (used for 3D Morton codes).
inline constexpr std::uint64_t spread_bits_3(const std::uint32_t v) noexcept
{
  std::uint64_t r = 0;
  for (std::size_t i = 0; i < 21; ++i)
    r |= (static_cast<std::uint64_t>(v >> i) & 1ULL) << (3 * i);
  return r;
}

}  // namespace ca_tiling_detail

/** @brief Morton (Z-order) encoding of `(x, y)`.
 *
 *  Interleaves the bits of `x` and `y`. Useful as a sort key for
 *  visiting 2D cells in a cache-friendly order (cells close in
 *  Euclidean distance map to close indices). Phase-5 leaves the engine
 *  using row partitioning by default; this helper is exposed as a
 *  building block for higher-dimensional schedulers.
 *
 *  @param[in] x first coordinate (axis 0).
 *  @param[in] y second coordinate (axis 1).
 *  @return interleaved 64-bit Morton code.
 */
[[nodiscard]] inline constexpr std::uint64_t morton_encode_2d(const std::uint32_t x,
                                                              const std::uint32_t y) noexcept
{
  return ca_tiling_detail::spread_bits_2(x) | (ca_tiling_detail::spread_bits_2(y) << 1);
}

/** @brief Morton (Z-order) encoding of `(x, y, z)`.
 *
 *  Same idea as `morton_encode_2d` but interleaving three streams of
 *  bits, suitable for 3D CAs. Inputs are clamped to 21 bits each so
 *  the result fits in `std::uint64_t`.
 *
 *  @param[in] x first coordinate.
 *  @param[in] y second coordinate.
 *  @param[in] z third coordinate.
 *  @return interleaved 64-bit Morton code.
 */
[[nodiscard]] inline constexpr std::uint64_t morton_encode_3d(const std::uint32_t x,
                                                              const std::uint32_t y,
                                                              const std::uint32_t z) noexcept
{
  return ca_tiling_detail::spread_bits_3(x) | (ca_tiling_detail::spread_bits_3(y) << 1)
       | (ca_tiling_detail::spread_bits_3(z) << 2);
}

}  // namespace CA
}  // namespace Aleph

#endif  // CA_TILING_H