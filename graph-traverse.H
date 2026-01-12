/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file graph-traverse.H
 *  @brief Graph traversal algorithms (DFS, BFS).
 *
 *  Provides depth-first search (DFS) and breadth-first search (BFS)
 *  traversal algorithms with customizable visitor callbacks.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef GRAPH_TRAVERSE_H
# define GRAPH_TRAVERSE_H

# include <tuple>
# include <cassert>
# include <tpl_agraph.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>

namespace // local constants to avoid collisions with aleph-graph.H
{
  constexpr unsigned char GT_Unprocessed = 0;
  constexpr unsigned char GT_Processing  = 1;
  constexpr unsigned char GT_Processed   = 2;
}

/** Traverse a graph depth-first or breadth-first and execute a visit function.

    This class template provides generic graph traversal capabilities using
    either depth-first search (DFS) or breadth-first search (BFS) depending
    on the queue type Q used.

    @tparam GT Graph type
    @tparam Itor Iterator type for traversing node arcs
    @tparam Q Queue type: DynListStack for DFS, DynListQueue for BFS
    @tparam Show_Arc Arc filter functor

    @ingroup Graphs
 */
template <class GT, class Itor,
          template <typename T> class Q = DynListStack,
          class Show_Arc = Dft_Show_Arc<GT>>
class Graph_Traverse
{
  GT & g;
  Show_Arc sa;

public:
  /// Construct a traverser with a graph and arc filter
  Graph_Traverse(GT & __g, Show_Arc __sa = Show_Arc()) : g(__g), sa(__sa) {}

  /** Traverse the graph starting from `start` and execute `op` on each node.

      @param start Starting node for traversal
      @param op Operation to execute on each visited node. Must return bool;
             return false to stop traversal early.
      @return Number of nodes visited
  */
  template <class Node_Op>
  size_t operator ()(typename GT::Node *start, Node_Op & op)
  {
    assert(start != nullptr);
    g.reset_nodes();
    g.reset_arcs();

    size_t count = 1;
    start->set_state(GT_Processed);
    if (not op(start))
      return count;

    Q<typename GT::Arc *> q;
    for (Itor it(start, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        typename GT::Node *tgt = g.get_connected_node(a, start);
        a->set_state(GT_Processing);
        tgt->set_state(GT_Processing);
        q.put(a);
      }

    const size_t n = g.vsize();
    while (not q.is_empty() and count < n)
      {
        typename GT::Arc *arc = q.get();
        assert(arc->state() == GT_Processing);
        arc->set_state(GT_Processed);

        typename GT::Node *s = g.get_src_node(arc);
        typename GT::Node *t = g.get_tgt_node(arc);
        if (s->state() == GT_Processed and t->state() == GT_Processed)
          continue;

        typename GT::Node *curr = s->state() == GT_Processed ? t : s;
        assert(curr->state() == GT_Processing);
        ++count;
        curr->set_state(GT_Processed);
        if (not op(curr))
          return count;

        for (Itor it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc *a = it.get_curr();
            if (a->state() != GT_Unprocessed)
              continue;
            typename GT::Node *tgt = g.get_connected_node(a, curr);
            if (tgt->state() == GT_Unprocessed)
              {
                assert(a->state() != GT_Processing);
                q.put(a);
                a->set_state(GT_Processing);
                tgt->set_state(GT_Processing);
              }
            else
              a->set_state(GT_Processed);
          }
      }

    return count;
  }

  /// \overload
  template <class Node_Op>
  size_t operator ()(typename GT::Node *start, Node_Op && op = Node_Op())
  {
    return (*this)(start, op);
  }

  /** Execute operation op(curr, arc) where curr is the visited node
      and arc is the incoming arc.

      @param start Starting node for traversal
      @param op Operation to execute. Signature: bool op(Node*, Arc*).
             The arc parameter is nullptr for the start node.
             Return false to stop traversal early.
      @return Number of nodes visited
  */
  template <class Op>
  size_t exec(typename GT::Node *start, Op & op)
  {
    assert(start != nullptr);
    g.reset_nodes();
    g.reset_arcs();

    size_t count = 1;
    start->set_state(GT_Processed);
    if (not op(start, nullptr))
      return count;

    using Pair = std::tuple<typename GT::Node *, typename GT::Arc *>;
    Q<Pair> q;
    for (Itor it(start, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        typename GT::Node *tgt = g.get_connected_node(a, start);
        a->set_state(GT_Processing);
        tgt->set_state(GT_Processing);
        q.put(std::make_tuple(start, a));
      }

    const size_t n = g.vsize();
    while (not q.is_empty() and count < n)
      {
        const Pair p = q.get();
        typename GT::Arc *arc = get<1>(p);
        assert(arc->state() == GT_Processing);
        assert(get<0>(p)->state() == GT_Processed);
        arc->set_state(GT_Processed);

        typename GT::Node *curr = g.get_connected_node(arc, get<0>(p));
        assert(curr->state() == GT_Processing);
        ++count;
        curr->set_state(GT_Processed);
        if (not op(curr, arc))
          return count;

        for (Itor it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc *a = it.get_curr();
            if (a->state() != GT_Unprocessed)
              continue;
            if (typename GT::Node *tgt = g.get_connected_node(a, curr); tgt->state() == GT_Unprocessed)
              {
                assert(a->state() != GT_Processing);
                a->set_state(GT_Processing);
                tgt->set_state(GT_Processing);
                q.put(std::make_tuple(curr, a));
              }
            else
              a->set_state(GT_Processed);
          }
      }

    return count;
  }

  /// \overload
  template <class Operation>
  size_t exec(typename GT::Node *start, Operation && op = Operation())
  {
    return exec(start, op);
  }

  /** Traverse the graph executing separate operations on nodes and arcs.

      @param start Starting node for traversal
      @param node_op Operation for nodes. Signature: bool node_op(Node*).
      @param arc_op Operation for arcs. Signature: bool arc_op(Arc*).
      @return Tuple of (nodes_visited, arcs_visited)
  */
  template <class Node_Op, class Arc_Op>
  std::tuple<size_t, size_t> operator ()(typename GT::Node *start,
                                    Node_Op & node_op, Arc_Op & arc_op)
  {
    assert(start != nullptr);
    g.reset_nodes();
    g.reset_arcs();
    Q<typename GT::Arc *> q;

    size_t node_count = 1;
    size_t arc_count = 0;

    start->set_state(GT_Processed);
    if (not node_op(start))
      return std::make_tuple(1, 0);

    for (Itor it(start, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        typename GT::Node *tgt = g.get_connected_node(a, start);
        a->set_state(GT_Processing);
        tgt->set_state(GT_Processing);
        q.put(a);
        ++arc_count;
        if (not arc_op(a))
          return std::make_tuple(node_count, arc_count);
      }

    while (not q.is_empty())
      {
        typename GT::Arc *arc = q.get();
        assert(arc->state() == GT_Processing);
        arc->set_state(GT_Processed);

        typename GT::Node *s = g.get_src_node(arc);
        typename GT::Node *t = g.get_tgt_node(arc);
        if (s->state() == GT_Processed and t->state() == GT_Processed)
          continue;

        typename GT::Node *curr = s->state() == GT_Processed ? t : s;
        assert(curr->state() == GT_Processing);
        ++node_count;
        curr->set_state(GT_Processed);
        if (not node_op(curr))
          return std::make_tuple(node_count, arc_count);

        for (Itor it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc *a = it.get_curr();
            if (a->state() != GT_Unprocessed)
              continue;
            typename GT::Node *tgt = g.get_connected_node(a, curr);
            if (tgt->state() != GT_Processed)
              {
                q.put(a);
                tgt->set_state(GT_Processing);
                a->set_state(GT_Processing);
                ++arc_count;
                if (not arc_op(a))
                  return std::make_tuple(node_count, arc_count);
              }
            else
              a->set_state(GT_Processed);
          }
      }

    return std::make_tuple(node_count, arc_count);
  }

  /// \overload
  template <class Node_Op, class Arc_Op>
  std::tuple<size_t, size_t> operator ()(typename GT::Node *start,
                                    Node_Op && node_op = Node_Op(),
                                    Arc_Op && arc_op = Arc_Op())
  {
    return (*this)(start, node_op, arc_op);
  }
};


template <class GT, class Itor,
          class Show_Arc = Dft_Show_Arc<GT>>
using Graph_Traverse_DFS = Graph_Traverse<GT, Itor, DynListStack, Show_Arc>;

template <class GT, class Itor,
          class Show_Arc = Dft_Show_Arc<GT>>
using Graph_Traverse_BFS = Graph_Traverse<GT, Itor, DynListQueue, Show_Arc>;


# endif // GRAPH_TRAVERSE_H
