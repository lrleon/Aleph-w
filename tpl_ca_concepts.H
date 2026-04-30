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

# ifndef TPL_CA_CONCEPTS_H
# define TPL_CA_CONCEPTS_H

# include <concepts>
# include <cstddef>
# include <type_traits>

# include <ca-traits.H>

namespace Aleph
{
namespace CA
{

  /** @brief A type usable as the value stored inside a cell.
   *
   *  Cell states must be regular: copyable, comparable for equality
   *  and default-constructible. This covers `bool`, integral and
   *  enum types, and any small POD struct used in lattice models.
   *
   *  @tparam T candidate cell state type.
   */
  template <typename T>
  concept CellState =
      std::regular<T> and
      std::default_initializable<T> and
      std::equality_comparable<T>;

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
  concept Coord =
      std::regular<C> and
      requires(C c, std::size_t i)
        {
          { c[i] } -> std::convertible_to<ca_index_t>;
          { C{}.size() } -> std::convertible_to<std::size_t>;
        } and
      (std::tuple_size<C>::value == N);

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
  concept LatticeLike =
      requires { typename L::state_type; typename L::coord_type; } and
      CellState<typename L::state_type> and
      requires(L & lat, const L & clat,
               const typename L::coord_type & c,
               const typename L::state_type & v,
               std::size_t d)
        {
          { clat.dimension() } -> std::convertible_to<std::size_t>;
          { clat.size(d) }     -> std::convertible_to<std::size_t>;
          { clat.at(c) }       -> std::convertible_to<typename L::state_type>;
          { lat.set(c, v) };
        };

  /** @brief Connectivity pattern around a coordinate.
   *
   *  A Neighborhood describes how to enumerate the cells that take part
   *  in the local transition for a given coordinate. The minimal
   *  interface is:
   *
   *    - `radius()`  — chebyshev or graph radius (≥ 1).
   *    - `size()`    — number of neighbors enumerated by `apply`.
   *
   *  Concrete neighborhoods (Moore, Von Neumann, custom offsets, ...)
   *  will be added in Fase 2; this concept only fixes the scaffolding
   *  needed by the engine in Fase 3.
   *
   *  @tparam H candidate neighborhood type.
   */
  template <typename H>
  concept NeighborhoodLike =
      requires(const H & h)
        {
          { h.radius() } -> std::convertible_to<std::size_t>;
          { h.size()   } -> std::convertible_to<std::size_t>;
        };

  /** @brief Local transition function.
   *
   *  A Rule maps the current state of a cell plus a view over its
   *  neighbours into the next state. The exact shape of the neighbour
   *  view is fixed in Fase 2 (`Neighbor_View`). For Fase 0 we only
   *  require that a Rule be invocable with the cell's current state
   *  and produce a value convertible back to that state type.
   *
   *  @tparam R candidate rule type.
   *  @tparam L lattice type whose `state_type` is consumed/produced.
   */
  template <typename R, typename L>
  concept RuleLike =
      LatticeLike<L> and
      std::invocable<const R &, typename L::state_type> and
      std::convertible_to<
        std::invoke_result_t<const R &, typename L::state_type>,
        typename L::state_type>;

} // namespace CA
} // namespace Aleph

# endif // TPL_CA_CONCEPTS_H