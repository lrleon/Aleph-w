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

/** @file tpl_ca_update_scheme.H
 *  @brief Phase 13 update-scheme strategies for `Aleph::CA`.
 *
 *  An *update scheme* is a strategy that decouples *what to compute*
 *  (the rule and the neighborhood) from *how the cells are visited*
 *  (synchronous double-buffer, sequential in-place, random asynchronous,
 *  block-synchronous, Margolus). Combining `Lattice × Rule × Update_Scheme`
 *  exposes the full spectrum of CA update orders without forcing a new
 *  engine per scheme.
 *
 *  Catalogue:
 *
 *    - `Synchronous_Update<Order>`        — the classical double-buffer
 *      sweep, equivalent to `Synchronous_Engine`. Used as the default.
 *    - `Sequential_Update<Order>`         — in-place sweep (no double
 *      buffer); the natural fit for Bak-Tang-Wiesenfeld sandpile and
 *      similar relaxation models.
 *    - `Random_Asynchronous_Update<Engine>` — pick one cell at random
 *      per sub-step, perform `n` sub-steps per `step()`. Reproducible
 *      via a master seed, even across thread schedulings.
 *    - `Block_Synchronous_Update<BlockSize, Order>` — block-based
 *      double buffering: rotate which block is updated each step.
 *    - `Margolus_Update`                  — alternates the 2×2 block
 *      partition origin between (0,0) and (1,1) on consecutive steps;
 *      enables canonical reversible CAs (Critters, BBM, TM Gas).
 *
 *  Every scheme declares two compile-time tags consumed by the
 *  Phase 13 engine to size buffers correctly:
 *
 *      static constexpr bool requires_double_buffer;
 *      static constexpr bool requires_block_rule;
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_UPDATE_SCHEME_H
#define TPL_CA_UPDATE_SCHEME_H

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_neighborhood.H>

namespace Aleph {
namespace CA {

namespace ca_scheme_detail {

/// Detect whether `O` is a `Tile<W, H>` instantiation.
template <typename O>
struct is_tile : std::false_type
{
};

template <std::size_t W, std::size_t H>
struct is_tile<Tile<W, H>> : std::true_type
{
};

template <typename O>
inline constexpr bool is_tile_v = is_tile<O>::value;

/// Iterate every coordinate of a rank-1 lattice in row order.
template <typename Coord, typename Cb>
inline void for_each_coord_1d(const ca_size_t n0, Cb &&cb)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    {
      c[0] = i;
      cb(c);
    }
}

/// Iterate every coordinate of a rank-2 lattice in row-major order.
template <typename Coord, typename Cb>
inline void for_each_coord_2d_row(const ca_size_t n0, const ca_size_t n1, Cb &&cb)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
      {
        c[0] = i;
        c[1] = j;
        cb(c);
      }
}

/// Iterate every coordinate of a rank-2 lattice in column-major order.
template <typename Coord, typename Cb>
inline void for_each_coord_2d_col(const ca_size_t n0, const ca_size_t n1, Cb &&cb)
{
  Coord c{};
  for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
      {
        c[0] = i;
        c[1] = j;
        cb(c);
      }
}

/// Iterate a rank-2 lattice in `H × W` tiles (cache-friendly).
template <std::size_t W, std::size_t H, typename Coord, typename Cb>
inline void for_each_coord_2d_tile(const ca_size_t n0, const ca_size_t n1, Cb &&cb)
{
  Coord c{};
  for (ca_size_t ti = 0; ti < n0; ti += H)
    for (ca_size_t tj = 0; tj < n1; tj += W)
      {
        const ca_size_t i_end = ti + H < n0 ? ti + H : n0;
        const ca_size_t j_end = tj + W < n1 ? tj + W : n1;
        for (ca_size_t i = ti; i < i_end; ++i)
          for (ca_size_t j = tj; j < j_end; ++j)
            {
              c[0] = static_cast<ca_index_t>(i);
              c[1] = static_cast<ca_index_t>(j);
              cb(c);
            }
      }
}

/// Iterate every coordinate of a rank-3 lattice in row-major order.
template <typename Coord, typename Cb>
inline void for_each_coord_3d_row(const ca_size_t n0, const ca_size_t n1, const ca_size_t n2, Cb &&cb)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
      for (ca_index_t k = 0; k < static_cast<ca_index_t>(n2); ++k)
        {
          c[0] = i;
          c[1] = j;
          c[2] = k;
          cb(c);
        }
}

/// Visit every coordinate of `lat` honouring the chosen `Order`.
template <typename Order, typename Lattice, typename Cb>
inline void for_each_coord(const Lattice &lat, Cb &&cb)
{
  using Coord = typename Lattice::coord_type;
  if constexpr (Lattice::rank == 1)
    {
      for_each_coord_1d<Coord>(lat.size(0), std::forward<Cb>(cb));
    }
  else if constexpr (Lattice::rank == 2)
    {
      if constexpr (is_tile_v<Order>)
        for_each_coord_2d_tile<Order::width, Order::height, Coord>(lat.size(0), lat.size(1),
                                                                   std::forward<Cb>(cb));
      else if constexpr (std::is_same_v<Order, ColumnMajor>)
        for_each_coord_2d_col<Coord>(lat.size(0), lat.size(1), std::forward<Cb>(cb));
      else
        for_each_coord_2d_row<Coord>(lat.size(0), lat.size(1), std::forward<Cb>(cb));
    }
  else if constexpr (Lattice::rank == 3)
    {
      for_each_coord_3d_row<Coord>(lat.size(0), lat.size(1), lat.size(2), std::forward<Cb>(cb));
    }
  else
    {
      static_assert(Lattice::rank <= 3, "Update schemes currently support rank <= 3");
    }
}

/// Total cell count of a lattice (product of extents).
template <typename Lattice>
inline ca_size_t cell_count(const Lattice &lat) noexcept
{
  ca_size_t n = 1;
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    n *= lat.size(d);
  return n;
}

/// Decode a 64-bit hash into a coordinate uniformly drawn over `extents`.
template <std::size_t Rank>
inline Coord_Vec<Rank> coord_from_hash(std::uint64_t h, const std::array<ca_size_t, Rank> &ext) noexcept
{
  Coord_Vec<Rank> c{};
  for (std::size_t d = 0; d < Rank; ++d)
    {
      const ca_size_t n = ext[d] == 0 ? 1 : ext[d];
      c[d] = static_cast<ca_index_t>(h % n);
      h /= n == 0 ? 1 : n;
      h = splitmix64(h ^ (static_cast<std::uint64_t>(d) + 1));
    }
  return c;
}

}  // namespace ca_scheme_detail

// =======================================================================
// Synchronous_Update
// =======================================================================

/** @brief Classical double-buffer synchronous update.
 *
 *  Reads the current frame, computes every cell's next state, writes
 *  into the next-buffer, then swaps the two buffers in O(1). Equivalent
 *  to the Phase 3 `Synchronous_Engine`; provided as a strategy so the
 *  Phase 13 engine can pick it up like any other scheme.
 *
 *  @tparam Order iteration tag (`RowMajor`, `ColumnMajor`, `Tile<W, H>`).
 */
template <typename Order = RowMajor>
struct Synchronous_Update
{
  /// Engines must allocate a second buffer for this scheme.
  static constexpr bool requires_double_buffer = true;
  /// Operates on cell rules — not on block rules.
  static constexpr bool requires_block_rule = false;

  /** @brief Execute one synchronous step.
   *
   *  @tparam Engine engine type providing `current_buffer()`,
   *                 `next_buffer()`, `swap_buffers()`,
   *                 `neighborhood()` and `step_count()`.
   *  @tparam Rule cell rule satisfying `RuleLike<Rule, Lattice>`.
   *  @param[in,out] e engine reference.
   *  @param[in] r rule reference.
   *  @throws Any exception propagated by the rule or the lattice.
   */
  template <typename Engine, typename Rule>
  void apply(Engine &e, Rule &r) const
  {
    using Lattice = typename Engine::lattice_type;
    using State = typename Lattice::state_type;
    using Coord = typename Lattice::coord_type;
    using Neighborhood = typename Engine::neighborhood_type;
    constexpr std::size_t K = Neighborhood::size_v;

    auto &cur = e.current_buffer();
    auto &nxt = e.next_buffer();
    const auto &nh = e.neighborhood();
    const std::size_t step = e.step_count();

    std::array<State, K == 0 ? 1 : K> nbuf{};
    const std::size_t neigh_sz = (std::min) (nh.size(), static_cast<std::size_t>(nbuf.size()));

    ca_scheme_detail::for_each_coord<Order>(cur, [&](const Coord &c)
    {
      gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
      const Cell_Context<Lattice::rank> ctx{step, c};
      nxt.set(c, apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx));
    });

    e.swap_buffers();
  }
};

// =======================================================================
// Sequential_Update
// =======================================================================

/** @brief In-place sequential update (no double buffer).
 *
 *  Visits every cell in `Order` and writes the new value back to the
 *  *same* lattice. Each cell sees the partially-updated neighbourhood,
 *  which is the dynamics needed by sandpile-style relaxation models
 *  and many physics-flavoured CAs. The choice of `Order` materially
 *  changes the qualitative behaviour, on purpose.
 *
 *  @tparam Order iteration tag (`RowMajor`, `ColumnMajor`, `Tile<W, H>`).
 */
template <typename Order = RowMajor>
struct Sequential_Update
{
  /// Operates in place, no second buffer needed.
  static constexpr bool requires_double_buffer = false;
  /// Operates on cell rules.
  static constexpr bool requires_block_rule = false;

  /** @brief Execute one sequential in-place step.
   *
   *  @tparam Engine engine type providing `current_buffer()`,
   *                 `neighborhood()` and `step_count()`.
   *  @tparam Rule cell rule satisfying `RuleLike<Rule, Lattice>`.
   *  @param[in,out] e engine reference.
   *  @param[in] r rule reference.
   *  @throws Any exception propagated by the rule or the lattice.
   */
  template <typename Engine, typename Rule>
  void apply(Engine &e, Rule &r) const
  {
    using Lattice = typename Engine::lattice_type;
    using State = typename Lattice::state_type;
    using Coord = typename Lattice::coord_type;
    using Neighborhood = typename Engine::neighborhood_type;
    constexpr std::size_t K = Neighborhood::size_v;

    auto &cur = e.current_buffer();
    const auto &nh = e.neighborhood();
    const std::size_t step = e.step_count();

    std::array<State, K == 0 ? 1 : K> nbuf{};
    const std::size_t neigh_sz = (std::min) (nh.size(), static_cast<std::size_t>(nbuf.size()));

    ca_scheme_detail::for_each_coord<Order>(cur, [&](const Coord &c)
    {
      gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
      const Cell_Context<Lattice::rank> ctx{step, c};
      const State next = apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx);
      cur.set(c, next);
    });
  }
};

// =======================================================================
// Random_Asynchronous_Update
// =======================================================================

/** @brief Pick one cell at random per sub-step, update it in place.
 *
 *  A `step()` performs `sub_steps_per_step` sub-steps; each sub-step
 *  picks a single cell uniformly at random (using the master seed,
 *  the engine step count and the sub-step index to derive a stable
 *  per-(step, sub) draw) and applies the rule in place.
 *
 *  Both the cell selection and any per-cell randomness in the rule
 *  derive from the same master seed but use disjoint sub-streams,
 *  so changing `sub_steps_per_step` reshuffles the visit pattern but
 *  keeps every other dimension reproducible.
 *
 *  @tparam Engine_Type PRNG used for cell selection. Defaults to
 *          `std::mt19937_64` for its 64-bit output and good quality.
 */
template <typename Engine_Type = std::mt19937_64>
struct Random_Asynchronous_Update
{
  /// In-place updates → no second buffer.
  static constexpr bool requires_double_buffer = false;
  /// Operates on cell rules.
  static constexpr bool requires_block_rule = false;

  std::uint64_t master_seed = 0;
  /// Number of single-cell sub-steps per `step()`. `0` means
  /// "as many sub-steps as there are cells in the lattice".
  std::size_t sub_steps_per_step = 0;

  /** @brief Build a random asynchronous update strategy.
   *
   *  @param[in] seed master seed forwarded to `cell_seed`.
   *  @param[in] sub_steps number of sub-steps per engine step
   *             (`0` → one full sweep per step).
   *  @throws This constructor does not throw.
   */
  constexpr Random_Asynchronous_Update(const std::uint64_t seed = 0,
                                       const std::size_t sub_steps = 0) noexcept
    : master_seed(seed), sub_steps_per_step(sub_steps)
  {}

  /** @brief Execute one randomised step.
   *
   *  @tparam Engine engine type.
   *  @tparam Rule cell rule satisfying `RuleLike<Rule, Lattice>`.
   *  @param[in,out] e engine reference.
   *  @param[in] r rule reference.
   *  @throws Any exception propagated by the rule, the lattice or
   *          the PRNG construction.
   */
  template <typename Engine, typename Rule>
  void apply(Engine &e, Rule &r) const
  {
    using Lattice = typename Engine::lattice_type;
    using State = typename Lattice::state_type;
    using Coord = typename Lattice::coord_type;
    using Neighborhood = typename Engine::neighborhood_type;
    constexpr std::size_t K = Neighborhood::size_v;

    auto &cur = e.current_buffer();
    const auto &nh = e.neighborhood();
    const std::size_t step = e.step_count();

    const ca_size_t n_cells = ca_scheme_detail::cell_count(cur);
    const std::size_t subs
      = sub_steps_per_step == 0 ? static_cast<std::size_t>(n_cells) : sub_steps_per_step;

    std::array<State, K == 0 ? 1 : K> nbuf{};
    const std::size_t neigh_sz = (std::min) (nh.size(), static_cast<std::size_t>(nbuf.size()));

    for (std::size_t sub = 0; sub < subs; ++sub)
      {
        // Stable per-(step, sub) selection seed independent of the
        // rule's own per-cell sub-stream.
        const std::uint64_t pick_seed = mix_seed(
          mix_seed(master_seed, static_cast<std::uint64_t>(step)), static_cast<std::uint64_t>(sub));
        Coord c = ca_scheme_detail::coord_from_hash<Lattice::rank>(pick_seed, cur.extents());

        gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
        const Cell_Context<Lattice::rank> ctx{step, c};
        const State next = apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx);
        cur.set(c, next);
      }
  }
};

// =======================================================================
// Block_Synchronous_Update
// =======================================================================

/** @brief Synchronous update over rotating sub-blocks.
 *
 *  The lattice is split into non-overlapping blocks of side `BlockSize`.
 *  Each `step()` updates all cells inside the *active block index*
 *  using the canonical synchronous rule (read from cur, write to nxt,
 *  swap), then advances the active block index. Cells outside the
 *  active block carry over unchanged.
 *
 *  After `(n_blocks)` steps every cell has been updated exactly once.
 *
 *  @tparam BlockSize side of the square (or cubic) block in cells.
 *  @tparam Order intra-block iteration order.
 */
template <std::size_t BlockSize = 4, typename Order = RowMajor>
struct Block_Synchronous_Update
{
  static_assert(BlockSize >= 1, "Block_Synchronous_Update requires BlockSize >= 1");

  static constexpr bool requires_double_buffer = true;
  static constexpr bool requires_block_rule = false;

  static constexpr std::size_t block_size = BlockSize;

  /** @brief Execute one block-synchronous step.
   *
   *  @tparam Engine engine type providing `current_buffer()`,
   *                 `next_buffer()`, `swap_buffers()`,
   *                 `neighborhood()` and `step_count()`.
   *  @tparam Rule cell rule satisfying `RuleLike<Rule, Lattice>`.
   *  @param[in,out] e engine reference.
   *  @param[in] r rule reference.
   *  @throws Any exception propagated by the rule or the lattice.
   */
  template <typename Engine, typename Rule>
  void apply(Engine &e, Rule &r) const
  {
    using Lattice = typename Engine::lattice_type;
    using State = typename Lattice::state_type;
    using Coord = typename Lattice::coord_type;
    using Neighborhood = typename Engine::neighborhood_type;
    constexpr std::size_t K = Neighborhood::size_v;

    auto &cur = e.current_buffer();
    auto &nxt = e.next_buffer();
    const auto &nh = e.neighborhood();
    const std::size_t step = e.step_count();

    // Number of blocks along each axis.
    std::array<ca_size_t, Lattice::rank> blocks_per_axis{};
    ca_size_t total_blocks = 1;
    for (std::size_t d = 0; d < Lattice::rank; ++d)
      {
        const ca_size_t n = cur.size(d);
        blocks_per_axis[d] = (n + BlockSize - 1) / BlockSize;
        total_blocks *= (blocks_per_axis[d] == 0 ? 1 : blocks_per_axis[d]);
      }

    if (total_blocks == 0)
      total_blocks = 1;
    const std::size_t active = step % total_blocks;

    // Decompose the active block index into per-axis tile coordinates.
    std::array<ca_size_t, Lattice::rank> tile_coord{};
    {
      std::size_t rem = active;
      for (std::size_t d = Lattice::rank; d-- > 0;)
        {
          const ca_size_t bn = blocks_per_axis[d] == 0 ? 1 : blocks_per_axis[d];
          tile_coord[d] = rem % bn;
          rem /= bn;
        }
    }

    // First, copy current → next so cells outside the active block
    // carry over unchanged after the swap.
    ca_scheme_detail::for_each_coord<Order>(cur, [&](const Coord &c)
    {
      nxt.set(c, cur.at(c));
    });

    std::array<State, K == 0 ? 1 : K> nbuf{};
    const std::size_t neigh_sz = (std::min) (nh.size(), static_cast<std::size_t>(nbuf.size()));

    // Iterate over the active block only.
    if constexpr (Lattice::rank == 1)
      {
        const ca_size_t i0 = tile_coord[0] * BlockSize;
        const ca_size_t i_end = (std::min) (i0 + BlockSize, cur.size(0));
        for (ca_size_t i = i0; i < i_end; ++i)
          {
            Coord c{static_cast<ca_index_t>(i)};
            gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
            const Cell_Context<Lattice::rank> ctx{step, c};
            nxt.set(c, apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx));
          }
      }
    else if constexpr (Lattice::rank == 2)
      {
        const ca_size_t i0 = tile_coord[0] * BlockSize;
        const ca_size_t j0 = tile_coord[1] * BlockSize;
        const ca_size_t i_end = (std::min) (i0 + BlockSize, cur.size(0));
        const ca_size_t j_end = (std::min) (j0 + BlockSize, cur.size(1));
        for (ca_size_t i = i0; i < i_end; ++i)
          for (ca_size_t j = j0; j < j_end; ++j)
            {
              Coord c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
              gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
              const Cell_Context<Lattice::rank> ctx{step, c};
              nxt.set(c, apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx));
            }
      }
    else if constexpr (Lattice::rank == 3)
      {
        const ca_size_t i0 = tile_coord[0] * BlockSize;
        const ca_size_t j0 = tile_coord[1] * BlockSize;
        const ca_size_t k0 = tile_coord[2] * BlockSize;
        const ca_size_t i_end = (std::min) (i0 + BlockSize, cur.size(0));
        const ca_size_t j_end = (std::min) (j0 + BlockSize, cur.size(1));
        const ca_size_t k_end = (std::min) (k0 + BlockSize, cur.size(2));
        for (ca_size_t i = i0; i < i_end; ++i)
          for (ca_size_t j = j0; j < j_end; ++j)
            for (ca_size_t k = k0; k < k_end; ++k)
              {
                Coord c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j),
                        static_cast<ca_index_t>(k)};
                gather_neighbors(nh, cur, c, std::span<State>(nbuf.data(), neigh_sz));
                const Cell_Context<Lattice::rank> ctx{step, c};
                nxt.set(c,
                        apply_rule(r, cur.at(c), Neighbor_View<State>(nbuf.data(), neigh_sz), ctx));
              }
      }

    e.swap_buffers();
  }
};

// =======================================================================
// Margolus_Update
// =======================================================================

/** @brief Margolus 2×2 partition update for reversible CAs.
 *
 *  The 2D lattice is partitioned into non-overlapping 2×2 blocks.
 *  Even steps anchor the partition at origin `(0, 0)` and odd steps
 *  shift it to `(1, 1)`. A Block_Rule (`tpl_ca_block_rule.H`) is then
 *  applied to every block of the active partition.
 *
 *  Combined with an involution Block_Rule (Critters, BBM, TM Gas),
 *  the resulting CA is bit-exact reversible: undoing step `k` is
 *  obtained by applying the same rule to the partition with the
 *  same parity (since the rule is its own inverse).
 *
 *  Constraints:
 *    - The lattice must be 2D.
 *    - Even-sided lattices are recommended; for odd extents the
 *      partial blocks at the right/bottom edge are skipped on
 *      odd steps to preserve reversibility.
 *    - Toroidal boundaries are recommended for fully wrapping the
 *      partition.
 *
 *  Margolus does not need a neighborhood: the engine should be
 *  instantiated with `Null_Neighborhood<2>`.
 */
struct Margolus_Update
{
  /// Margolus uses a single buffer; reads and writes the same lattice
  /// block by block.
  static constexpr bool requires_double_buffer = false;
  /// Operates on Block_Rule, not on cell rules.
  static constexpr bool requires_block_rule = true;

  /// Origin offset used at step `k`: even → (0,0), odd → (1,1).
  [[nodiscard]] static constexpr ca_index_t origin_for_step(const std::size_t k) noexcept
  {
    return (k % 2 == 0) ? 0 : 1;
  }

  /** @brief Apply the block rule to every 2×2 block of the active partition.
   *
   *  @tparam Engine engine type providing `current_buffer()`.
   *  @tparam Block_Rule type satisfying `Block_Rule_2x2<Block_Rule, State>`.
   *  @param[in,out] e engine reference.
   *  @param[in] r block rule reference.
   *  @param[in] origin offset for the partition origin (usually 0 or 1).
   *  @throws Any exception propagated by the rule or the lattice.
   */
  template <typename Engine, typename Block_Rule>
  void apply_with_origin(Engine &e, Block_Rule &r, const ca_index_t origin) const
  {
    using Lattice = typename Engine::lattice_type;
    using State = typename Lattice::state_type;
    using Coord = typename Lattice::coord_type;
    static_assert(Lattice::rank == 2, "Margolus_Update requires a 2D lattice");

    auto &lat = e.current_buffer();
    const ca_size_t n0 = lat.size(0);
    const ca_size_t n1 = lat.size(1);
    if (n0 < 2 or n1 < 2)
      return;

    // Sweep every 2×2 block of the active partition.
    for (ca_index_t i = origin; i + 1 < static_cast<ca_index_t>(n0); i += 2)
      for (ca_index_t j = origin; j + 1 < static_cast<ca_index_t>(n1); j += 2)
        {
          const Coord nw{i, j};
          const Coord ne{i, j + 1};
          const Coord sw{i + 1, j};
          const Coord se{i + 1, j + 1};

          std::array<State, 4> in{lat.at(nw), lat.at(ne), lat.at(sw), lat.at(se)};
          const std::array<State, 4> out = r(in);

          lat.set(nw, out[0]);
          lat.set(ne, out[1]);
          lat.set(sw, out[2]);
          lat.set(se, out[3]);
        }
  }

  /** @brief Apply the block rule using the parity of the engine step.
   *  @tparam Engine engine type.
   *  @tparam Block_Rule block rule type.
   *  @param[in,out] e engine reference.
   *  @param[in] r block rule reference.
   */
  template <typename Engine, typename Block_Rule>
  void apply(Engine &e, Block_Rule &r) const
  {
    apply_with_origin(e, r, origin_for_step(e.step_count()));
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_UPDATE_SCHEME_H