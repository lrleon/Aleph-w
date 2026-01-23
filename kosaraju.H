
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
template <class GT>
inline static void __dfp_phase1(const GT & g,
                                 typename GT::Node * p,
                                 DynArray<typename GT::Node*> & df)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true);

  // Traverse outgoing arcs in depth-first order
  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;

      ARC_BITS(a).set_bit(Depth_First, true);

      __dfp_phase1(g, it.get_tgt_node(), df);
    }

  df.append(p); // Append node in postorder (finish time)
  NODE_COUNTER(p) = df.size();
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
template <class GT>
inline static void __dfp_phase2_subgraph(const GT & g,
                                          typename GT::Node * p,
                                          GT & blk,
                                          const int & color)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true);

  auto q = blk.insert_node(p->get_info());
  NODE_COUNTER(q) = color;
  GT::map_nodes(p, q);

  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true);

      __dfp_phase2_subgraph(g, it.get_tgt_node(), blk, color);
    }
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
template <class GT>
inline static void __dfp_phase2_list(const GT & g,
                                      typename GT::Node * p,
                                      DynList<typename GT::Node *> & list)
{
  if (IS_NODE_VISITED(p, Depth_First))
    return;

  NODE_BITS(p).set_bit(Depth_First, true);

  list.append(mapped_node<GT>(p));

  for (auto it = g.get_out_it(p); it.has_curr(); it.next_ne())
    {
      auto a = it.get_current_arc_ne();
      if (IS_ARC_VISITED(a, Depth_First))
        continue;
      ARC_BITS(a).set_bit(Depth_First, true);

      __dfp_phase2_list(g, it.get_tgt_node(), list);
    }
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
template <class GT>
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
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;

      GT & blk = blk_list.append(GT());
      array[color] = &blk;

      __dfp_phase2_subgraph(gi, bp, blk, color++); // DFS on transposed graph

      assert(NODE_COLOR(mapped_node<GT>(bp)) == color - 1);
    }

  // Classify arcs: internal to SCC or crossing between SCCs
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto a  = it.get_curr();
      auto gs = g.get_src_node(a);
      auto gt = g.get_tgt_node(a);

      // Double mapping: original graph → transposed graph → SCC subgraph
      // First mapped_node: original node → transposed graph node
      // Second mapped_node: transposed graph node → SCC subgraph node
      auto bs = mapped_node<GT>(mapped_node<GT>(gs));
      auto bt = mapped_node<GT>(mapped_node<GT>(gt));

      const long & color = NODE_COLOR(bs);

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
template <class GT>
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
      if (IS_NODE_VISITED(bp, Depth_First))
        continue;

      auto & blk = list.append(DynList<typename GT::Node*>());

      __dfp_phase2_list(gi, bp, blk);
    }

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
template <class GT>
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
template <class GT>
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
template <class GT>
inline bool is_strongly_connected(const GT & g)
{
  if (g.get_num_nodes() <= 1)
    return true;

  return kosaraju_scc_count(g) == 1;
}

} // end namespace Aleph

# endif // KOSARAJU_H
