
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


/** @file ah-graph-concepts.H
 *  @brief C++20 concepts for the protocol shared by graph algorithms.
 *
 *  The graph algorithms have always relied on an implicit protocol for their
 *  template parameters. These concepts write down its minimal, common part:
 *
 *  - **AlephGraph<GT>** — node/arc types and the core queries every graph
 *    algorithm relies on (`List_Graph`, `Array_Graph`, `List_SGraph`, their
 *    `Digraph<>` wrappers, `Net_Graph` and derivatives).
 *  - **NodeFilter<SN, GT>** / **ArcFilter<SA, GT>** — the `SN`/`SA` filters,
 *    invoked the way `Filter_Iterator` invokes them.
 *  - **ArcDistance<D, GT>** — the `Distance` functor: a `Distance_Type` and a
 *    call on an arc. `Zero_Distance`/`Max_Distance`/`set_zero` are not part of
 *    it: no algorithm relies on them generally.
 *  - **FlowNetwork<Net>** — what the flow algorithms need from a network.
 *
 *  Each concept states a condition every algorithm that constrains with it
 *  needs anyway, so it never rejects a type the algorithm would accept; an
 *  algorithm may still need more than its concept states.
 *
 *  @warning Constrain algorithms, never `GraphCommon` or types used while a
 *  graph class is still being defined (iterators, default filters,
 *  `Dft_Dist`): the graph type is incomplete there.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef AH_GRAPH_CONCEPTS_H
# define AH_GRAPH_CONCEPTS_H

# include <concepts>
# include <cstddef>
# include <type_traits>

# include <ah-concepts.H>

namespace Aleph
{
  namespace graph_concepts_detail
  {
    /// Unqualified form of `AlephGraph`, applied to a cv/ref-free type.
    template <class G>
    concept AlephGraphImpl =
        requires
          {
            typename G::Node;
            typename G::Arc;
            typename G::Node_Type;
            typename G::Arc_Type;
          } and
        requires(const G & g, typename G::Arc * a)
          {
            { g.get_num_nodes() } -> std::convertible_to<std::size_t>;
            { g.get_num_arcs() } -> std::convertible_to<std::size_t>;
            { g.get_src_node(a) } -> std::convertible_to<typename G::Node *>;
            { g.get_tgt_node(a) } -> std::convertible_to<typename G::Node *>;
            { g.is_digraph() } -> std::convertible_to<bool>;
          };

    /// `GT` with cv-qualifiers and references removed.
    template <class GT>
    using graph_t = std::remove_cvref_t<GT>;
  } // namespace graph_concepts_detail

  /** @brief A graph usable by the graph algorithms.
   *
   *  Requires the nested `Node`, `Arc`, `Node_Type` and `Arc_Type` types and,
   *  on a `const` graph, `get_num_nodes()`, `get_num_arcs()`,
   *  `get_src_node(arc)`, `get_tgt_node(arc)` and `is_digraph()`. Every graph
   *  class derived from `GraphCommon` satisfies it. cv-qualifiers and
   *  references on `GT` are ignored.
   *
   *  @tparam GT candidate graph type.
   */
  template <class GT>
  concept AlephGraph = graph_concepts_detail::AlephGraphImpl<graph_concepts_detail::graph_t<GT>>;

  /** @brief A node filter (`SN`) for graph `GT`.
   *
   *  Invoked as `Filter_Iterator` does: a non-`const` lvalue called with a
   *  `Node *`, its result used as a condition.
   *
   *  @tparam SN filter type.
   *  @tparam GT graph type.
   */
  template <class SN, class GT>
  concept NodeFilter =
      AlephGraph<GT> and
      PredicateWith<SN &, typename graph_concepts_detail::graph_t<GT>::Node *>;

  /** @brief An arc filter (`SA`) for graph `GT`.
   *
   *  Invoked as `Filter_Iterator` does: a non-`const` lvalue called with an
   *  `Arc *`, its result used as a condition. `set_cookie()` is not required:
   *  no algorithm calls it.
   *
   *  @tparam SA filter type.
   *  @tparam GT graph type.
   */
  template <class SA, class GT>
  concept ArcFilter =
      AlephGraph<GT> and
      PredicateWith<SA &, typename graph_concepts_detail::graph_t<GT>::Arc *>;

  /** @brief An arc distance (`Distance`) for graph `GT`.
   *
   *  Requires a nested `Distance_Type` and a call on an `Arc *`, through a
   *  non-`const` lvalue, whose result converts to it. That is the only part
   *  of the `Dft_Dist` interface the algorithms use in general.
   *
   *  @tparam D distance functor type.
   *  @tparam GT graph type.
   */
  template <class D, class GT>
  concept ArcDistance =
      AlephGraph<GT> and
      requires { typename D::Distance_Type; } and
      requires(D & d, typename graph_concepts_detail::graph_t<GT>::Arc * a)
        {
          { d(a) } -> std::convertible_to<typename D::Distance_Type>;
        };

  /** @brief A flow network usable by the max-flow and min-cost algorithms.
   *
   *  An `AlephGraph` with a nested `Flow_Type`, `const` access to its source
   *  and sink and to whether they are unique, and arcs exposing `cap` and
   *  `flow`. `Net_Graph` and its derivatives satisfy it.
   *
   *  @tparam Net candidate network type.
   */
  template <class Net>
  concept FlowNetwork =
      AlephGraph<Net> and
      requires { typename graph_concepts_detail::graph_t<Net>::Flow_Type; } and
      requires(const graph_concepts_detail::graph_t<Net> & net,
               typename graph_concepts_detail::graph_t<Net>::Arc * a)
        {
          { net.get_source() } -> std::convertible_to<typename graph_concepts_detail::graph_t<Net>::Node *>;
          { net.get_sink() } -> std::convertible_to<typename graph_concepts_detail::graph_t<Net>::Node *>;
          { net.is_single_source() } -> std::convertible_to<bool>;
          { net.is_single_sink() } -> std::convertible_to<bool>;
          { a->cap } -> std::convertible_to<typename graph_concepts_detail::graph_t<Net>::Flow_Type>;
          { a->flow } -> std::convertible_to<typename graph_concepts_detail::graph_t<Net>::Flow_Type>;
        };

} // namespace Aleph

# endif // AH_GRAPH_CONCEPTS_H
