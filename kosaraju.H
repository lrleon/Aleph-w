
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


/** @file kosaraju.H
 *  @brief Kosaraju's algorithm for finding strongly connected components.
 *
 *  This file implements Kosaraju's algorithm for computing the strongly
 *  connected components (SCCs) of a directed graph. The algorithm performs
 *  two depth-first searches: one on the original graph to compute finish
 *  times, and one on the transposed graph in decreasing order of finish
 *  times to identify the SCCs.
 *
 *  Time complexity: O(V + E)
 *  Space complexity: O(V + E) for the transposed graph
 *
 *  @see Tarjan.H for an alternative SCC algorithm that uses only one DFS
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef KOSARAJU_H
# define KOSARAJU_H

# include <ah-graph-concepts.H>

# include <tpl_dfs_stack.H>
# include <tpl_graph_utils.H>

namespace Aleph {

/** @brief Depth-first search that computes finish times in postorder (Phase 1).
 *
 *  Performs a DFS traversal starting from node p, marking visited nodes
 *  and arcs with the Depth_First bit. Nodes are appended to the df array
 *  in postorder (after all descendants have been visited).
 *
 *  This is the first phase of Kosaraju's algorithm.
 *
 *  @tparam GT Graph type.
 *  @param[in] g The directed graph.
 *  @param[in] p Starting node for the DFS.
 *  @param[out] df Array where nodes are stored in postorder (finish time order).
 */
namespace kosaraju_detail
{
  /// A node that waits for the one that it visits, and the next of its
  /// outgoing arcs to look at
  template <class GT>
  struct Frame
  {
    typename GT::Node * p;
    typename GT::Out_Iterator it;

    [[gnu::always_inline, gnu::flatten]]
    Frame(const GT & g, typename GT::Node * n) : p(n), it(g.get_out_it(n)) {}

    [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }

    // Moves the iterator to the next arc not visited whose target was not
    // visited either, and gives that target; it marks the arcs not visited
    // that it passes. nullptr when the arcs are done.
    [[gnu::always_inline, gnu::flatten]] typename GT::Node * next_target() noexcept
    {
      for (; it.has_curr(); it.next_ne())
        {
          auto a = it.get_current_arc_ne();
          if (IS_ARC_VISITED(a, Depth_First))
            continue;

          // the target before the mark, which the compiler takes for a
          // store that could change the iterator
          auto tgt = it.get_tgt_node_ne();
          ARC_BITS(a).set_bit(Depth_First, true);
          if (not IS_NODE_VISITED(tgt, Depth_First))
            return tgt;
        }
      return nullptr;
    }
  };

  // Depth-first search from p, which visits and enters each node that it
  // reaches, and leaves it when its arcs are done. A path can have as many
  // nodes as the graph, too many for recursion, so that the search keeps
  // its own stack; the order of the visits is that of a recursion. The
  // node being visited keeps its iterator in a local frame until it visits
  // a node; then it waits in the stack, where it goes on when it comes
  // back.
  template <class GT, class Enter, class Leave>
  [[gnu::always_inline]] inline void
  search(const GT & g, typename GT::Node * p, Enter enter, Leave leave)
  {
    if (IS_NODE_VISITED(p, Depth_First))
      return;

    NODE_BITS(p).set_bit(Depth_First, true);
    enter(p);

    Dfs_Stack<Frame<GT>> frames;
    Frame<GT> here(g, p);
    while (true)
      {
        if (auto tgt = here.next_target())
          { // the iterator stays on the arc to tgt, as in a recursion
            NODE_BITS(tgt).set_bit(Depth_First, true);
            enter(tgt);
            frames.push(std::move(here));
            here = Frame<GT>(g, tgt);
            continue;
          }

        leave(here.p);
        while (true) // back to the nodes that wait
          {
            if (frames.is_empty())
              return;

            Frame<GT> & f = frames.top();
            f.next_arc(); // past the arc to the node that was visited
            if (auto tgt = f.next_target())
              {
                NODE_BITS(tgt).set_bit(Depth_First, true);
                enter(tgt);
                here = Frame<GT>(g, tgt);
                break;
              }
            leave(f.p);
            frames.pop();
          }
      }
  }
} // namespace kosaraju_detail

template <AlephGraph GT>
inline static void __dfp_phase1(const GT & g,
                                 typename GT::Node * p,
                                 DynArray<typename GT::Node*> & df)
{
  kosaraju_detail::search(g, p, [](typename GT::Node *) {},
                          [&df](typename GT::Node * q)
                          {
                            df.append(q); // Append node in postorder (finish time)
                            NODE_COUNTER(q) = df.size();
                          });
}

/** @brief DFS on transposed graph that builds an SCC subgraph (Phase 2).
 *
 *  Traverses the transposed graph starting from node p, inserting all
 *  reachable nodes into the block subgraph. Each node in the block is
 *  assigned the same color (component ID) and mapped to the original graph.
 *
 *  This is the second phase of Kosaraju's algorithm (subgraph version).
 *
 *  @tparam GT Graph type.
 *  @param[in] g The transposed directed graph.
 *  @param[in] p Starting node for the DFS.
 *  @param[out] blk Subgraph where nodes of this SCC are inserted.
 *  @param[in] color Component ID assigned to all nodes in this SCC.
 */
template <AlephGraph GT>
inline static void __dfp_phase2_subgraph(const GT & g,
                                          typename GT::Node * p,
                                          GT & blk,
                                          const int & color)
{
  kosaraju_detail::search(g, p, [&blk, &color](typename GT::Node * q)
                          {
                            auto b = blk.insert_node(q->get_info());
                            NODE_COUNTER(b) = color;
                            GT::map_nodes(q, b);
                          },
                          [](typename GT::Node *) {});
}

/** @brief DFS on transposed graph that builds an SCC node list (Phase 2).
 *
 *  Traverses the transposed graph starting from node p, appending all
 *  reachable nodes (mapped back to original graph) to the list.
 *
 *  This is the second phase of Kosaraju's algorithm (lightweight version).
 *
 *  @tparam GT Graph type.
 *  @param[in] g The transposed directed graph.
 *  @param[in] p Starting node for the DFS.
 *  @param[out] list List where nodes of this SCC are appended.
 */
template <AlephGraph GT>
inline static void __dfp_phase2_list(const GT & g,
                                      typename GT::Node * p,
                                      DynList<typename GT::Node *> & list)
{
  kosaraju_detail::search(g, p, [&list](typename GT::Node * q)
                          {
                            list.append(mapped_node<GT>(q));
                          },
                          [](typename GT::Node *) {});
}

/** @brief Compute strongly connected components using Kosaraju's algorithm.
 *
 *  This function computes the strongly connected components (SCCs) of a
 *  directed graph using Kosaraju's algorithm. The algorithm works in two phases:
 *
 *  1. **First DFS**: Traverse the original graph and compute finish times
 *     (nodes are ordered by when they finish, stored in postorder).
 *
 *  2. **Second DFS**: Create the transposed graph, then process nodes in
 *     decreasing order of finish time. Each DFS tree in this phase forms
 *     one SCC.
 *
 *  The resulting SCCs are returned as subgraphs, with nodes and arcs mapped
 *  to the original graph via cookies. Arcs that cross between different SCCs
 *  are collected in a separate list.
 *
 *  **Time complexity**: O(V + E)
 *  **Space complexity**: O(V + E) for the transposed graph
 *
 *  @tparam GT Graph type (must be a directed graph).
 *
 *  @param[in] g The directed graph. Must not be modified during execution.
 *  @param[out] blk_list List of subgraphs, one per SCC. Each subgraph is
 *              mapped to g via node and arc cookies.
 *  @param[out] arc_list List of arcs that cross between different SCCs
 *              (inter-component arcs).
 *
 *  @note If the graph is strongly connected, blk_list will contain a single
 *        subgraph with all nodes, and arc_list will be empty.
 *
 *  @warning This function modifies the control bits (Depth_First) on nodes
 *           and arcs. The bits are reset at the start.
 *
 *  @see tarjan_connected_components() for an alternative algorithm
 *  @see invert_digraph() used internally to create the transposed graph
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <AlephGraph GT>
inline void kosaraju_connected_components(const GT & g,
                                          DynList<GT> & blk_list,
                                          DynList<typename GT::Arc *> & arc_list)
{
  g.reset_nodes();
  g.reset_arcs();

  DynArray<typename GT::Node*> df; // Array for postorder (finish times)

  // First DFS: compute finish times in postorder
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    __dfp_phase1(g, it.get_curr(), df);

  GT gi = invert_digraph(g); // Transposed graph

  DynArray<GT*> array; // Array of subgraph pointers indexed by color

  // Second DFS: process nodes in decreasing order of finish time
  for (long i = static_cast<long>(df.size()) - 1, color = 0; i >= 0; --i)
    {
      auto gp = df.access(i);
      auto bp = mapped_node<GT>(gp);
      if (not IS_NODE_VISITED(bp, Depth_First))
        {
          GT & blk = blk_list.append(GT());
          array[color] = &blk;

          __dfp_phase2_subgraph(gi, bp, blk, color++); // DFS on transposed graph

          assert(NODE_COLOR(mapped_node<GT>(bp)) == color - 1);
        }

      // gp was mapped to bp, of the transposed graph, which goes; now it
      // maps to the node of its block, which already maps to gp
      NODE_COOKIE(gp) = mapped_node<GT>(bp);
    }

  // Classify arcs: internal to SCC or crossing between SCCs
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto a  = it.get_curr();
      auto gs = g.get_src_node(a);
      auto gt = g.get_tgt_node(a);

      auto bs = mapped_node<GT>(gs); // the nodes of the blocks
      auto bt = mapped_node<GT>(gt);

      const long & color = NODE_COLOR(bs);

      ARC_COOKIE(a) = nullptr; // it was mapped to the transposed graph
      if (color == NODE_COLOR(bt))
        {
          typename GT::Arc * ba = array.access(color)->insert_arc(bs, bt);
          GT::map_arcs(a, ba);
        }
      else
        arc_list.append(a);
    }
}

/** @brief Compute strongly connected components as lists of nodes.
 *
 *  This is a lightweight version of Kosaraju's algorithm that returns
 *  the SCCs as lists of node pointers rather than full subgraphs.
 *  This is more efficient when you only need to know which nodes belong
 *  to which component, without needing the induced subgraphs.
 *
 *  **Time complexity**: O(V + E)
 *  **Space complexity**: O(V + E) for the transposed graph
 *
 *  @tparam GT Graph type (must be a directed graph).
 *
 *  @param[in] g The directed graph.
 *
 *  @return A list of lists, where each inner list contains the nodes
 *          belonging to one SCC. The nodes are pointers to the original
 *          graph's nodes.
 *
 *  @note This version is faster than the subgraph version because it
 *        doesn't need to build and map subgraphs.
 *
 *  @warning This function modifies the control bits (Depth_First) on nodes
 *           and arcs. The bits are reset at the start.
 *
 *  @see kosaraju_connected_components(g, blk_list, arc_list) for the
 *       version that builds mapped subgraphs
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <AlephGraph GT>
inline DynList<DynList<typename GT::Node*>>
kosaraju_connected_components(const GT & g)
{
  DynList<DynList<typename GT::Node*>> list;

  g.reset_nodes();
  g.reset_arcs();

  DynArray<typename GT::Node*> df; // Array for postorder (finish times)

  // First DFS: compute finish times in postorder
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    __dfp_phase1(g, it.get_curr(), df);

  GT gi = invert_digraph(g); // Transposed graph

  // Second DFS: process nodes in decreasing order of finish time
  for (long i = static_cast<long>(df.size()) - 1; i >= 0; --i)
    {
      auto gp = df.access(i);
      auto bp = mapped_node<GT>(gp);
      NODE_COOKIE(gp) = nullptr; // it mapped to bp, which goes with gi
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;

      auto & blk = list.append(DynList<typename GT::Node*>());

      __dfp_phase2_list(gi, bp, blk);
    }

  // The arcs of g were mapped to the transposed graph, which goes now
  g.for_each_arc([](auto a) { ARC_COOKIE(a) = nullptr; });

  return list;
}

/** @brief Functor wrapper for Kosaraju's algorithm.
 *
 *  This struct provides a callable interface for computing strongly
 *  connected components using Kosaraju's algorithm.
 *
 *  @tparam GT Graph type (must be a directed graph).
 *
 *  @see kosaraju_connected_components()
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <AlephGraph GT>
struct Kosaraju_Connected_Components
{
  /** @brief Compute SCCs returning subgraphs and cross-arcs.
   *
   *  @param[in] g The directed graph.
   *  @param[out] blk_list List of SCC subgraphs.
   *  @param[out] arc_list List of inter-component arcs.
   */
  void operator () (const GT & g,
                    DynList<GT> & blk_list,
                    DynList<typename GT::Arc *> & arc_list) const
  {
    kosaraju_connected_components(g, blk_list, arc_list);
  }

  /** @brief Compute SCCs returning lists of nodes.
   *
   *  @param[in] g The directed graph.
   *  @return List of lists of nodes, one list per SCC.
   */
  DynList<DynList<typename GT::Node*>> operator () (const GT & g) const
  {
    return kosaraju_connected_components(g);
  }
};

/** @brief Count the number of strongly connected components.
 *
 *  Computes the number of SCCs in the graph without building the
 *  actual component lists.
 *
 *  @tparam GT Graph type (must be a directed graph).
 *  @param[in] g The directed graph.
 *  @return The number of strongly connected components.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <AlephGraph GT>
inline size_t kosaraju_scc_count(const GT & g)
{
  return kosaraju_connected_components(g).size();
}

/** @brief Check if a directed graph is strongly connected.
 *
 *  A directed graph is strongly connected if there is exactly one SCC
 *  containing all nodes.
 *
 *  @tparam GT Graph type (must be a directed graph).
 *  @param[in] g The directed graph.
 *  @return true if the graph is strongly connected, false otherwise.
 *
 *  @note An empty graph or a graph with a single node is considered
 *        strongly connected.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */
template <AlephGraph GT>
inline bool is_strongly_connected(const GT & g)
{
  if (g.get_num_nodes() <= 1)
    return true;

  return kosaraju_scc_count(g) == 1;
}

} // end namespace Aleph

# endif // KOSARAJU_H
