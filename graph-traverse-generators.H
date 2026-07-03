/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file graph-traverse-generators.H
 *  @brief Lazy (coroutine-based) graph traversal (DFS, BFS).
 *
 *  Lazy counterpart of `graph-traverse.H`'s `Graph_Traverse`: the exact
 *  same queue-driven algorithm (a `DynListStack` gives DFS order, a
 *  `DynListQueue` gives BFS order — the graph's own arcs decide which
 *  nodes are reachable next), but exposed as an `Aleph::Generator<Node *>`
 *  instead of a visitor callback. The eager `Graph_Traverse`/
 *  `Graph_Traverse_DFS`/`Graph_Traverse_BFS` are **not replaced** — reach
 *  for the lazy version when the caller may stop early, or wants to
 *  compose the traversal with other lazy computation.
 *
 *  @code
 *  using Itor = Node_Arc_Iterator<MyGraph>;
 *  Graph_Traverse_BFS_Generator<MyGraph, Itor> bfs(g);
 *  for (MyGraph::Node *n : bfs.traverse(start))
 *    {
 *      if (found(n))
 *        break;               // stops the search outright
 *      visit(n);
 *    }
 *  @endcode
 *
 *  @note The traverser object (`bfs` above) owns the `Aleph::Generator`'s
 *        captured state (the graph reference and arc filter) via the usual
 *        member-function-coroutine rule: keep it alive for as long as the
 *        generator returned by `traverse()` is being iterated.
 *  @note Like the eager version, traversal mutates the graph's per-node
 *        and per-arc processing state (`reset_nodes()`/`reset_arcs()` are
 *        called at the start of `traverse()`), so only one traversal of a
 *        given graph should be in flight at a time.
 *
 *  @see graph-traverse.H Eager visitor-based DFS/BFS (`Graph_Traverse`).
 *  @see ah-generator.H `Aleph::Generator<T>`, the underlying lazy sequence
 *       type.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

#ifndef GRAPH_TRAVERSE_GENERATORS_H
#define GRAPH_TRAVERSE_GENERATORS_H

#include <cassert>

#include <ah-errors.H>
#include <ah-generator.H>
#include <graph-traverse.H>

namespace Aleph
{

/// @cond INTERNAL
namespace graph_traverse_generator_detail
{
/// Per-node/per-arc processing state during a lazy traversal. Scoped inside
/// a nested `Aleph::` namespace (rather than the file-scope unnamed
/// namespace `graph-traverse.H` uses for the same purpose) so this header
/// can be included alongside `graph-traverse.H` in the same translation
/// unit without a duplicate-symbol clash.
enum class Traverse_State : unsigned char
{
  Unprocessed = 0,
  Processing = 1,
  Processed = 2
};
} // namespace graph_traverse_generator_detail
/// @endcond

/** @brief Lazily traverse a graph depth-first or breadth-first.
 *
 *  Mirrors `Graph_Traverse` (see `graph-traverse.H`): the queue type `Q`
 *  selects the traversal order (`DynListStack` for DFS, `DynListQueue` for
 *  BFS — see the `Graph_Traverse_DFS_Generator`/`Graph_Traverse_BFS_Generator`
 *  aliases below), and `Itor` iterates the arcs incident to a node.
 *
 *  @tparam GT Graph type (e.g. `List_Graph<...>`, `List_Digraph<...>`).
 *  @tparam Itor Arc iterator type for a node (e.g. `Node_Arc_Iterator<GT>`).
 *  @tparam Q Queue template: `DynListStack` for DFS, `DynListQueue` for BFS.
 *  @tparam Show_Arc Arc filter functor; arcs for which it returns `false`
 *          are skipped, exactly as in `Graph_Traverse`.
 *
 *  @ingroup Graphs
 */
template <class GT, class Itor,
          template <typename T> class Q = DynListStack,
          class Show_Arc = Dft_Show_Arc<GT>>
class Graph_Traverse_Generator
{
  using State = graph_traverse_generator_detail::Traverse_State;

  GT &g_;
  Show_Arc sa_;

  static unsigned char raw(State s) noexcept { return static_cast<unsigned char>(s); }

  /** Prepare arc `a` (leading to `tgt`) for frontier expansion, and report
      whether the caller should enqueue it. Same semantics as
      `Graph_Traverse::seed_arc` (graph-traverse.H) — kept as a separate
      copy rather than shared because the two classes use different
      state representations (`Traverse_State` enum here vs. the
      `GT_Unprocessed`/`GT_Processing`/`GT_Processed` constants there; see
      the file-level note on why this header defines its own enum). */
  static bool seed_arc(typename GT::Arc *a, typename GT::Node *tgt) noexcept
  {
    if (tgt->state() != raw(State::Unprocessed))
      {
        a->set_state(raw(State::Processed));
        return false;
      }
    a->set_state(raw(State::Processing));
    tgt->set_state(raw(State::Processing));
    return true;
  }

public:
  /// Construct a lazy traverser bound to graph `__g` with arc filter `__sa`.
  Graph_Traverse_Generator(GT &__g, Show_Arc __sa = Show_Arc()) : g_(__g), sa_(__sa) {}

  /** @brief Lazily traverse the graph from `start`.
   *
   *  @param start Starting node for the traversal.
   *  @return A lazy sequence of `GT::Node *`, one per visited node, in the
   *          order determined by `Q` (stack ⇒ DFS, queue ⇒ BFS).
   *  @note Complexity: O(1) amortized per yielded node — same total O(V+E)
   *        work as the eager traversal, just spread across resumes.
   *  @throws std::invalid_argument if `start == nullptr`. Like every
   *          `Aleph::Generator`-returning function, the body doesn't run
   *          until the first resume (`initial_suspend`), so this check
   *          fires on the first `begin()`/`operator++()` of the returned
   *          sequence, not on the `traverse(start)` call itself — catch it
   *          around the iteration, not just around the call.
   *  @note This method is `&`-ref-qualified on purpose: `traverse()` is a
   *        member-function coroutine, so its frame keeps a raw `this` and
   *        doesn't touch `g_`/`sa_` until the first resume — well after a
   *        temporary `Graph_Traverse_Generator` would have been destroyed.
   *        `Graph_Traverse_BFS_Generator<GT,Itor>(g).traverse(start)` is
   *        therefore a **compile error** instead of a dangling-`this` bug:
   *        bind the traverser to a named variable first (as every example
   *        and test in this codebase already does) and keep it alive for
   *        as long as the returned sequence is iterated.
   */
  Aleph::Generator<typename GT::Node *> traverse(typename GT::Node *start) &
  {
    ah_invalid_argument_if(start == nullptr)
      << "Graph_Traverse_Generator::traverse(): null start node";
    g_.reset_nodes();
    g_.reset_arcs();

    size_t count = 1;
    start->set_state(raw(State::Processed));
    co_yield start;

    Q<typename GT::Arc *> q;
    for (Itor it(start, sa_); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        typename GT::Node *tgt = g_.get_connected_node(a, start);
        if (seed_arc(a, tgt))
          q.put(a);
      }

    const size_t n = g_.vsize();
    while (not q.is_empty() and count < n)
      {
        typename GT::Arc *arc = q.get();
        assert(arc->state() == raw(State::Processing));
        arc->set_state(raw(State::Processed));

        typename GT::Node *s = g_.get_src_node(arc);
        typename GT::Node *t = g_.get_tgt_node(arc);
        if (s->state() == raw(State::Processed) and t->state() == raw(State::Processed))
          continue;

        typename GT::Node *curr = s->state() == raw(State::Processed) ? t : s;
        assert(curr->state() == raw(State::Processing));
        ++count;
        curr->set_state(raw(State::Processed));
        co_yield curr;

        for (Itor it(curr, sa_); it.has_curr(); it.next_ne())
          {
            typename GT::Arc *a = it.get_curr();
            if (a->state() != raw(State::Unprocessed))
              continue;
            typename GT::Node *tgt = g_.get_connected_node(a, curr);
            if (seed_arc(a, tgt))
              q.put(a);
          }
      }
  }
};

/// Lazy depth-first traversal: `Graph_Traverse_Generator` with a
/// `DynListStack` frontier. Mirrors `Graph_Traverse_DFS`.
template <class GT, class Itor, class Show_Arc = Dft_Show_Arc<GT>>
using Graph_Traverse_DFS_Generator =
  Graph_Traverse_Generator<GT, Itor, DynListStack, Show_Arc>;

/// Lazy breadth-first traversal: `Graph_Traverse_Generator` with a
/// `DynListQueue` frontier. Mirrors `Graph_Traverse_BFS`.
template <class GT, class Itor, class Show_Arc = Dft_Show_Arc<GT>>
using Graph_Traverse_BFS_Generator =
  Graph_Traverse_Generator<GT, Itor, DynListQueue, Show_Arc>;

} // end namespace Aleph

#endif // GRAPH_TRAVERSE_GENERATORS_H
