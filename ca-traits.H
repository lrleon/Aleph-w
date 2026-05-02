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

/** @file ca-traits.H
 *  @brief Common typedefs and tag types for the Cellular Automata module.
 *
 *  Lives in `namespace Aleph::CA`. Provides:
 *
 *    - Index/coordinate aliases (`ca_index_t`, `ca_size_t`).
 *    - Boundary policy tag types (`OpenBoundary`, `ToroidalBoundary`,
 *      `ReflectiveBoundary`, `ConstantBoundary<v>`).
 *    - Iteration order tag types (`RowMajor`, `ColumnMajor`).
 *
 *  No code here depends on a particular storage or rule layout, so it
 *  is the natural shared header for every later phase of the CA module.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_TRAITS_H
#define CA_TRAITS_H

#include <array>
#include <cstddef>
#include <span>

namespace Aleph {
namespace CA {

/// Signed coordinate component used by lattices and neighborhoods.
/// Signed because neighborhoods produce negative offsets.
using ca_index_t = std::ptrdiff_t;

/// Unsigned size component used for extents and counts.
using ca_size_t = std::size_t;

/** @brief Default coordinate vector.
 *  @tparam N number of axes.
 */
template <std::size_t N>
using Coord_Vec = std::array<ca_index_t, N>;

/** @brief Default offset vector (aliases `Coord_Vec`).
 *
 *  Used by neighborhoods to express signed displacements that are
 *  added to a lattice coordinate. The type is identical to
 *  `Coord_Vec<N>`; the alias exists to make intent explicit at
 *  call sites.
 */
template <std::size_t N>
using Offset_Vec = Coord_Vec<N>;

/** @brief Read-only view over a contiguous range of neighbour values.
 *
 *  A `Neighbor_View<T>` is a thin `std::span<const T>` populated by
 *  `gather_neighbors` (see `tpl_ca_neighborhood.H`) on a stack
 *  buffer. Rules consume it without owning the underlying memory.
 *
 *  @tparam T cell state type.
 */
template <typename T>
using Neighbor_View = std::span<const T>;

/** @brief Per-cell context handed to rules that need to know "where"
 *         and "when" they are firing.
 *
 *  Phase 8 introduces stochastic rules that derive deterministic,
 *  per-cell pseudo-random number generators from `(step, coord)`.
 *  The CA engine fills a `Cell_Context<Rank>` once per cell and
 *  forwards it to the rule via `apply_rule()`. Deterministic rules
 *  remain unaffected because `apply_rule()` only forwards the context
 *  when the rule explicitly accepts it.
 *
 *  @tparam Rank lattice dimension (1, 2 or 3 in the bundled lattices).
 */
template <std::size_t Rank>
struct Cell_Context
{
  /// Index of the step that is currently being computed (0-based).
  std::size_t step = 0;
  /// Cell coordinate inside the current frame.
  Coord_Vec<Rank> coord{};
};

// -----------------------------------------------------------------------
// Boundary policies
// -----------------------------------------------------------------------

/// Out-of-range neighbours behave as if the lattice ended.
struct OpenBoundary
{
};

/// The lattice wraps around on every axis.
struct ToroidalBoundary
{
};

/// Out-of-range coordinates mirror back into the lattice.
struct ReflectiveBoundary
{
};

/** @brief Constant-value boundary. Out-of-range cells take value `V`.
 *  @tparam T cell state type.
 *  @tparam V constant value used outside the lattice.
 */
template <typename T, T V>
struct ConstantBoundary
{
  using state_type = T;
  static constexpr T value = V;
};

/** @brief Zero-gradient (Neumann) boundary.
 *
 *  Out-of-range cells replicate the value of the closest in-range cell,
 *  which is the discrete analogue of a zero-derivative condition
 *  `\partial u / \partial n = 0`. Introduced with the Phase 4 ghost
 *  layer; useful for reaction-diffusion CAs and sealed-domain
 *  simulations.
 */
struct NeumannBoundary
{
};

// --- Phase 4 spelling aliases -----------------------------------------
//
// Phase 4 of the CA plan refers to the boundary policies as
// `Periodic`, `Reflective`, `Dirichlet<...>` and `Neumann<...>`. The
// underlying implementations are the Phase 1 tags; these aliases make
// both naming conventions available without duplicating the logic.

/// Alias for `ToroidalBoundary` using the HPC "periodic" terminology.
using PeriodicBoundary = ToroidalBoundary;

/** @brief Alias for `ConstantBoundary<T, V>` using the "Dirichlet" name.
 *  @tparam T cell state type.
 *  @tparam V imposed value on the out-of-range cells.
 */
template <typename T, T V>
using DirichletBoundary = ConstantBoundary<T, V>;

// -----------------------------------------------------------------------
// Iteration order tags
// -----------------------------------------------------------------------

/// Iterate the lattice in row-major (C) order.
struct RowMajor
{
};

/// Iterate the lattice in column-major (Fortran) order.
struct ColumnMajor
{
};

/** @brief Iterate a 2D lattice in `W` x `H` tiles.
 *
 *  Tiles are visited in row-major order over the tile grid, and the
 *  cells inside each tile are visited row-major as well. Useful as
 *  a cache-friendly schedule for the synchronous engine.
 *
 *  Implementations that don't have a meaningful tile interpretation
 *  (1D, 3D) fall back to `RowMajor`.
 *
 *  @tparam W tile width  (cells along the last axis).
 *  @tparam H tile height (cells along the first axis).
 */
template <std::size_t W, std::size_t H>
struct Tile
{
  static_assert(W > 0 and H > 0, "Tile dimensions must be non-zero");

  static constexpr std::size_t width = W;
  static constexpr std::size_t height = H;
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_TRAITS_H
