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

/** @file tpl_ca_concepts.H
 *  @brief C++20 concepts for the Cellular Automata module.
 *
 *  Defines the lightweight constraints used throughout `Aleph::CA`
 *  to validate adapters that build a cellular automaton:
 *
 *  - **CellState<T>**       — what can live inside a cell.
 *  - **Coord<C, N>**        — N-dimensional coordinate types.
 *  - **LatticeLike<L>**     — storage + topology that holds the cells.
 *  - **NeighborhoodLike<H>**— connectivity pattern around a coordinate.
 *  - **RuleLike<R, L>**     — local transition function.
 *
 *  The concept names use a `Like` suffix so that the class templates
 *  introduced in later phases (`Lattice<Storage, Boundary>`, the
 *  `Moore` / `Von_Neumann` neighborhoods, the rule classes) can keep
 *  their natural unsuffixed names without colliding with concepts.
 *
 *  The concepts are intentionally permissive. They describe the
 *  minimum API the Cellular Automata engine relies on, so any
 *  reasonable user-defined storage/rule pair can be plugged in.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_CONCEPTS_H
#define TPL_CA_CONCEPTS_H

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

#include <ca-traits.H>

namespace Aleph {
namespace CA {

/** @brief A type usable as the value stored inside a cell.
 *
 *  Cell states must be regular: copyable, comparable for equality
 *  and default-constructible. This covers `bool`, integral and
 *  enum types, and any small POD struct used in lattice models.
 *
 *  @tparam T candidate cell state type.
 */
template <typename T>
concept CellState = std::regular<T>;

/** @brief A coordinate type with `N` integral components.
 *
 *  The CA module manipulates coordinates as fixed-size random-access
 *  containers (`std::array<index_t, N>` is the canonical choice). A
 *  coordinate must expose `operator[]` and a compile-time `size()`.
 *
 *  @tparam C coordinate type.
 *  @tparam N expected dimension.
 */
template <typename C, std::size_t N>
concept Coord = std::regular<C> and requires(C c, std::size_t i) {
  { c[i] } -> std::convertible_to<ca_index_t>;
  { C{}.size() } -> std::convertible_to<std::size_t>;
} and (std::tuple_size<C>::value == N);

/** @brief Storage + topology that carries the cell values.
 *
 *  A Lattice exposes the geometric layout of the automaton and a
 *  read/write interface over its cells. The CA engine only relies on:
 *
 *    - `dimension()` — number of axes (compile-time or runtime).
 *    - `size(d)`     — extent along axis `d`.
 *    - `at(coord)`   — read access to a cell.
 *    - `set(coord, v)` — write access to a cell.
 *
 *  The `state_type` and `coord_type` aliases let the rest of the
 *  module deduce the cell value type and coordinate kind without
 *  forcing a particular concrete container.
 *
 *  @tparam L candidate lattice type.
 */
template <typename L>
concept LatticeLike = requires {
  typename L::state_type;
  typename L::coord_type;
} and CellState<typename L::state_type> and requires(L &lat, const L &clat, const typename L::coord_type &c, const typename L::state_type &v, std::size_t d) {
  { clat.dimension() } -> std::convertible_to<std::size_t>;
  { clat.size(d) } -> std::convertible_to<std::size_t>;
  { clat.at(c) } -> std::convertible_to<typename L::state_type>;
  { lat.set(c, v) };
};

/** @brief Connectivity pattern around a coordinate.
 *
 *  A Neighborhood describes how to enumerate the cells that take part
 *  in the local transition for a given coordinate. The engine relies
 *  on the following interface:
 *
 *    - `coord_type`              — coordinate type accepted by enumeration.
 *    - `rank_v`                  — compile-time dimensionality.
 *    - `size_v`                  — compile-time number of offsets.
 *    - `radius()`                — chebyshev or graph radius (≥ 1).
 *    - `size()`                  — number of neighbors enumerated at runtime.
 *    - `for_each_offset(center, f)` — invokes `f(offset)` for each
 *      neighbour offset around `center`.
 *
 *  Concrete neighborhoods (Moore, Von Neumann, custom offsets, ...)
 *  will be added in Phase 2; this concept fixes the contract relied on
 *  by the synchronous engine and neighbour gathering helpers.
 *
 *  @tparam H candidate neighborhood type.
 */
template <typename H>
concept NeighborhoodLike =
  requires {
    typename H::coord_type;
    { H::rank_v } -> std::convertible_to<std::size_t>;
    { H::size_v } -> std::convertible_to<std::size_t>;
  }
  and requires(const H &h, const typename H::coord_type &center) {
    { h.radius() } -> std::convertible_to<std::size_t>;
    { h.size()   } -> std::convertible_to<std::size_t>;
    h.for_each_offset(center, [](const auto &) {});
  };

/** @brief Pure local transition function (Phase 0–7 signature).
 *
 *  A `BasicRuleLike` rule consumes only `(state, neighbours)` and is
 *  therefore deterministic from the engine's point of view. Every
 *  rule shipped before Phase 8 satisfies this concept.
 *
 *  @tparam R candidate rule type.
 *  @tparam L lattice type whose `state_type` is consumed/produced.
 */
template <typename R, typename L>
concept BasicRuleLike
  = LatticeLike<L>
and requires(const R &r, const typename L::state_type &s, Neighbor_View<typename L::state_type> v) {
      { r(s, v) } -> std::convertible_to<typename L::state_type>;
    };

/** @brief Context-aware local transition function (Phase 8).
 *
 *  A `ContextualRuleLike` rule consumes an extra `Cell_Context<Rank>`
 *  argument exposing the current step and the cell coordinate. Rules
 *  that need to derive a deterministic per-cell pseudo-random number
 *  generator (`Forest_Fire_Rule`, `SIR_Rule`, `Ising_*`, `Schelling_Rule`,
 *  ...) implement this signature so the engine can guarantee bit-for-bit
 *  reproducibility independently of thread scheduling.
 *
 *  @tparam R candidate rule type.
 *  @tparam L lattice type.
 */
template <typename R, typename L>
concept ContextualRuleLike
  = LatticeLike<L>
and requires(const R &r, const typename L::state_type &s,
             Neighbor_View<typename L::state_type> v,
             const Cell_Context<L::rank> &ctx) {
      { r(s, v, ctx) } -> std::convertible_to<typename L::state_type>;
    };

/** @brief Local transition function (any supported signature).
 *
 *  Either a plain `(state, neighbours)` functor (Phase 0–7) or a
 *  context-aware `(state, neighbours, ctx)` functor (Phase 8). The
 *  engines pick the right overload via `apply_rule()`.
 *
 *  @tparam R candidate rule type.
 *  @tparam L lattice type whose `state_type` is consumed/produced.
 */
template <typename R, typename L>
concept RuleLike = BasicRuleLike<R, L> or ContextualRuleLike<R, L>;

/** @brief Invoke a rule, optionally forwarding the per-cell context.
 *
 *  When the rule supports the `(state, neighbours, ctx)` signature
 *  (`ContextualRuleLike`), the context is forwarded; otherwise the
 *  function falls back to the legacy `(state, neighbours)` form. This
 *  helper is the single dispatch point used by every CA engine in the
 *  module so adding new contextual rules never requires touching the
 *  engines.
 *
 *  @tparam Rule rule type satisfying `RuleLike`.
 *  @tparam State cell state type.
 *  @tparam Rank lattice dimension.
 *  @param[in] r rule instance.
 *  @param[in] s current state of the centre cell.
 *  @param[in] v neighbour view (gathered upstream).
 *  @param[in] ctx per-cell context (`step`, `coord`).
 *  @return the next state produced by the rule.
 *  @throws Any exception thrown by the rule.
 */
template <typename Rule, typename State, std::size_t Rank>
[[nodiscard]] inline State apply_rule(const Rule &r,
                                      const State &s,
                                      Neighbor_View<State> v,
                                      const Cell_Context<Rank> &ctx)
{
  if constexpr (requires { r(s, v, ctx); })
    return r(s, v, ctx);
  else
    return r(s, v);
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_CONCEPTS_H
