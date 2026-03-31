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

/** @file Graph_Coloring.H
 *  @brief Graph coloring algorithms and heuristics.
 *
 *  This file provides a collection of algorithms for the vertex coloring
 *  problem, which aims to assign a color to each node such that no two
 *  adjacent nodes share the same color.
 *
 *  ## Consistency across Directed and Undirected Graphs
 *
 *  Graph coloring is fundamentally an undirected problem. All algorithms in
 *  this module treat directed arcs as bidirectional constraints: if there
 *  is an arc@f$u \to v@f$, then@f$u@f$and@f$v@f$cannot have the same color.
 *
 *  ## Included Algorithms:
 *
 *  - **Greedy Coloring**: Processes nodes in their natural iteration order.
 *    Complexity:@f$O((V+E) \log V)@f$.
 *  - **Welsh-Powell**: Sorts nodes by decreasing degree before coloring.
 *    Complexity:@f$O((V+E) \log V)@f$.
 *  - **DSatur (Degree of Saturation)**: Adaptive heuristic that prioritizes
 *    nodes with the most distinct colors in their neighborhood.
 *    Complexity:@f$O(V^2 + E \log V)@f$.
 *  - **Chromatic Number**: Exact minimum colors (@f$\chi(G)@f$) for small
 *    graphs (up to 64 nodes) via backtracking. Complexity: Exponential.
 *
 *  ## Implementation Details
 *
 *  All algorithms use the `NODE_COOKIE` field of graph nodes for O(1)
 *  color lookups. @ref Aleph::Cookie_Saver ensures pre-existing cookies
 *  are restored upon exit.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef GRAPH_COLORING_H
#define GRAPH_COLORING_H

#include <tpl_graph.H>
#include <tpl_array.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>
#include <tpl_sort_utils.H>
#include <cookie_guard.H>
#include <ah-errors.H>

namespace Aleph {

namespace graph_coloring_detail {

inline void *encode_color(size_t c) noexcept
{
  return reinterpret_cast<void *>(static_cast<uintptr_t>(c + 1));
}

inline bool is_colored(void *cookie) noexcept
{
  return cookie != nullptr;
}

inline size_t decode_color(void *cookie) noexcept
{
  return static_cast<size_t>(reinterpret_cast<uintptr_t>(cookie)) - 1;
}

inline size_t smallest_available(const DynSetTree<size_t> &used)
{
  size_t c = 0;
  while (used.search(c) != nullptr)
    ++c;
  return c;
}

/** @brief Internal helper to pre-build undirected adjacency lists.
 *  Ensures O(degree) iteration even for directed graphs.
 *
 *  Parallel/multi-arcs are deduplicated so that each neighbor appears
 *  at most once in the adjacency list of a node.
 */
template <class GT, class SA>
void build_undirected_adj(const GT                                                      &g,
                          DynMapTree<typename GT::Node *, DynList<typename GT::Node *>> &adj)
{
  using Node = typename GT::Node;

  // Ensure all nodes are present in the adjacency map with empty lists.
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    adj.insert(it.get_curr(), DynList<Node *>());

  // Temporary per-node neighbor sets to deduplicate parallel arcs.
  DynMapTree<Node *, DynSetTree<Node *>> neighbor_sets;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    neighbor_sets.insert(it.get_curr(), DynSetTree<Node *>());

  // Collect undirected neighbors, deduplicating parallel arcs.
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto *arc = it.get_curr();
      if (not SA()(arc))
        continue;
      Node *u = g.get_src_node(arc);
      Node *v = g.get_tgt_node(arc);
      if (u == v)
        continue;
      neighbor_sets[u].insert(v);
      neighbor_sets[v].insert(u);
    }

  // Materialize deduplicated neighbor sets into adjacency lists.
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *u        = it.get_curr();
      auto &adj_list = adj[u];
      auto &nbrs     = neighbor_sets[u];
      for (auto sit = nbrs.get_it(); sit.has_curr(); sit.next_ne())
        adj_list.append(sit.get_curr());
    }
}

template <class GT, class SA = Dft_Show_Arc<GT>>
void validate_no_self_loops(const GT &g)
{
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto *arc = it.get_curr();
      ah_domain_error_if(SA()(arc) and g.get_src_node(arc) == g.get_tgt_node(arc))
        << "graph coloring does not support self-loops";
    }
}

}  // end namespace graph_coloring_detail

/** @brief Greedy graph coloring in iteration order.
 *
 *  Assigns each node the smallest 0-based color index not already used by
 *  any of its colored neighbors, visiting nodes in their natural graph-
 *  iteration order.  Uses at most@f$\Delta+1@f$colors where@f$\Delta@f$
 *  is the maximum degree.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, Array_Graph, …).
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @param[in]  g      Input graph.  Must not contain self-loops.
 *  @param[out] colors Output map from @c GT::Node* to 0-based color index.
 *                     Cleared and overwritten on entry.
 *
 *  @return Number of distinct colors used (@f$\geq 1@f$for non-empty graphs,
 *          0 for an empty graph).
 *
 *  @throws std::domain_error if @p g contains a self-loop.
 *
 *  @pre No self-loops in @p g.
 *
 *  @note **Complexity**:@f$O((V+E) \log V)@f$.
 *
 *  @note **Thread safety**: not thread-safe.  Temporarily writes to
 *        @c NODE_COOKIE on every node during execution; prior cookie values
 *        are saved and restored automatically via @ref Cookie_Saver before
 *        the function returns (even under exceptions).  Do not call
 *        concurrently with any other code that reads or writes node cookies.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t greedy_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};
  if (g.get_num_nodes() == 0)
    return 0;

  validate_no_self_loops<GT, SA>(g);

  DynMapTree<Node *, DynList<Node *>> adj;
  build_undirected_adj<GT, SA>(g, adj);

  Cookie_Saver<GT> saver(g, true, false);
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    NODE_COOKIE(it.get_curr()) = nullptr;

  size_t num_colors = 0;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node              *v = it.get_curr();
      DynSetTree<size_t> used;

      for (auto nit = adj[v].get_it(); nit.has_curr(); nit.next_ne())
        if (Node *w = nit.get_curr(); is_colored(NODE_COOKIE(w)))
          used.insert(decode_color(NODE_COOKIE(w)));

      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Welsh-Powell graph coloring.
 *
 *  Applies first-fit greedy coloring after sorting nodes in decreasing
 *  order of degree.  Typically uses fewer colors than plain greedy but
 *  is still a heuristic.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, Array_Graph, …).
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @param[in]  g      Input graph.  Must not contain self-loops.
 *  @param[out] colors Output map from @c GT::Node* to 0-based color index.
 *                     Cleared and overwritten on entry.
 *
 *  @return Number of distinct colors used (0 for an empty graph).
 *
 *  @throws std::domain_error if @p g contains a self-loop.
 *
 *  @pre No self-loops in @p g.
 *
 *  @note **Complexity**:@f$O((V+E) \log V)@f$(sort step dominates for
 *        dense graphs:@f$O(V \log V)@f$).
 *
 *  @note **Thread safety**: not thread-safe.  Temporarily writes to
 *        @c NODE_COOKIE on every node; saved and restored via
 *        @ref Cookie_Saver before the function returns.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t welsh_powell_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};
  if (g.get_num_nodes() == 0)
    return 0;

  validate_no_self_loops<GT, SA>(g);

  DynMapTree<Node *, DynList<Node *>> adj;
  build_undirected_adj<GT, SA>(g, adj);

  Cookie_Saver<GT> saver(g, true, false);
  DynList<Node *>  nodes;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *p        = it.get_curr();
      NODE_COOKIE(p) = nullptr;
      nodes.append(p);
    }

  mergesort(nodes, [&adj](Node *a, Node *b)
  {
    return adj[a].size() > adj[b].size();
  });

  size_t num_colors = 0;
  for (auto nit = nodes.get_it(); nit.has_curr(); nit.next_ne())
    {
      Node              *v = nit.get_curr();
      DynSetTree<size_t> used;

      for (auto ait = adj[v].get_it(); ait.has_curr(); ait.next_ne())
        {
          Node *w = ait.get_curr();
          if (is_colored(NODE_COOKIE(w)))
            used.insert(decode_color(NODE_COOKIE(w)));
        }

      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief DSatur graph coloring (saturation-degree heuristic).
 *
 *  At each step the uncolored node with the highest saturation degree
 *  (number of distinct colors among already-colored neighbors) is colored
 *  first, using the smallest available color.  Ties are broken by choosing
 *  the node with the highest degree.  This adaptive strategy frequently
 *  produces near-optimal or optimal colorings.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, Array_Graph, …).
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @param[in]  g      Input graph.  Must not contain self-loops.
 *  @param[out] colors Output map from @c GT::Node* to 0-based color index.
 *                     Cleared and overwritten on entry.
 *
 *  @return Number of distinct colors used (0 for an empty graph).
 *
 *  @throws std::domain_error if @p g contains a self-loop.
 *
 *  @pre No self-loops in @p g.
 *
 *  @note **Complexity**:@f$O(V^2 + E \log V)@f$.
 *
 *  @note **Thread safety**: not thread-safe.  Temporarily writes to
 *        @c NODE_COOKIE on every node; saved and restored via
 *        @ref Cookie_Saver before the function returns.
 *
 *  @see chromatic_number, welsh_powell_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t dsatur_coloring(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  colors = {};
  if (const size_t n = g.get_num_nodes(); n == 0)
    return 0;

  validate_no_self_loops<GT, SA>(g);

  DynMapTree<Node *, DynList<Node *>> adj;
  build_undirected_adj<GT, SA>(g, adj);

  Cookie_Saver<GT>                       saver(g, true, false);
  DynMapTree<Node *, DynSetTree<size_t>> saturation_sets;
  DynSetTree<Node *>                     uncolored_nodes;

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *v        = it.get_curr();
      NODE_COOKIE(v) = nullptr;
      uncolored_nodes.insert(v);
      saturation_sets.insert(v, DynSetTree<size_t>());
    }

  size_t num_colors = 0;
  while (not uncolored_nodes.is_empty())
    {
      Node  *best     = nullptr;
      size_t best_sat = 0;
      size_t best_deg = 0;

      // Select best node: O(V)
      for (auto it = uncolored_nodes.get_it(); it.has_curr(); it.next_ne())
        {
          Node  *v   = it.get_curr();
          size_t sat = saturation_sets[v].size();
          size_t deg = adj[v].size();

          if (best == nullptr or sat > best_sat or (sat == best_sat and deg > best_deg))
            {
              best     = v;
              best_sat = sat;
              best_deg = deg;
            }
        }

      size_t c          = smallest_available(saturation_sets[best]);
      NODE_COOKIE(best) = encode_color(c);
      colors.insert(best, c);
      uncolored_nodes.remove(best);

      if (c + 1 > num_colors)
        num_colors = c + 1;

      // Update neighbors: O(deg(best) log colors)
      for (auto it = adj[best].get_it(); it.has_curr(); it.next_ne())
        if (Node *w = it.get_curr(); not is_colored(NODE_COOKIE(w)))
          saturation_sets[w].insert(c);
    }

  return num_colors;
}

/** @brief Validates a graph coloring.
 *
 *  A coloring is valid iff every graph node has a color entry in @p colors
 *  and no arc connects two nodes of the same color.  Self-loops are
 *  considered invalid regardless of color assignment.
 *
 *  A size match alone between @p colors and the graph is insufficient:
 *  the map could be keyed with foreign (non-graph) pointers of the same
 *  count.  This function explicitly verifies that every @c GT::Node* in
 *  @p g has an entry before checking arc conflicts.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, Array_Graph, …).
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @param[in] g      Graph to validate against.
 *  @param[in] colors Map from @c GT::Node* to 0-based color index.
 *
 *  @return @c true iff every node is present in @p colors and no arc
 *          connects same-colored endpoints; @c false otherwise (including
 *          self-loops and missing/foreign map entries).
 *
 *  @note **Complexity**:@f$O((V+E) \log V)@f$.
 *
 *  @note Does not modify any graph state.  Thread-safe provided @p g and
 *        @p colors are not concurrently modified.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] bool is_valid_coloring(const GT                                      &g,
                                     const DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;

  // Explicit per-node check: every graph node must have a color entry.
  // A size match alone is insufficient — the map could be keyed with
  // foreign pointers that coincidentally match the node count.
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (colors.search(it.get_curr()) == nullptr)
      return false;

  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto *arc = it.get_curr();
      if (not SA()(arc))
        continue;
      Node *u = g.get_src_node(arc);
      Node *v = g.get_tgt_node(arc);
      if (u == v)
        return false;
      auto *pu = colors.search(u);
      auto *pv = colors.search(v);
      if (pu == nullptr or pv == nullptr)
        return false;
      if (pu->second == pv->second)
        return false;
    }

  return true;
}

/** @brief Computes the exact chromatic number of a small graph.
 *
 *  Uses DSatur to obtain an upper bound, then performs binary search with
 *  backtracking to find the minimum@f$k@f$for which a valid@f$k@f$-coloring
 *  exists.  Applicable only to graphs with at most 64 nodes.
 *
 *  @tparam GT Graph type (List_Graph, List_Digraph, Array_Graph, …).
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @param[in]  g      Input graph.  Must have at most 64 nodes and no
 *                     self-loops.
 *  @param[out] colors Output map from @c GT::Node* to 0-based color index
 *                     for an optimal coloring.  Cleared and overwritten on
 *                     entry.
 *
 *  @return The chromatic number@f$\chi(G)@f$(0 for an empty graph,
 *          1 if the graph has no edges).
 *
 *  @throws std::domain_error if @p g has more than 64 nodes or contains
 *          a self-loop.
 *
 *  @pre @c g.get_num_nodes() <= 64 and no self-loops in @p g.
 *
 *  @note **Complexity**: Exponential in the worst case (backtracking over
 *        all@f$k@f$-colorings).  Practical for graphs up to ~30–40 nodes.
 *
 *  @note **Thread safety**: not thread-safe.  Temporarily writes to
 *        @c NODE_COOKIE on every node; saved and restored via
 *        @ref Cookie_Saver before the function returns.
 *
 *  @see dsatur_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t chromatic_number(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;
  using namespace graph_coloring_detail;

  ah_domain_error_if(g.get_num_nodes() > 64)
    << "chromatic_number: graph has " << g.get_num_nodes() << " nodes (max 64)";

  colors = {};
  if (g.get_num_nodes() == 0)
    return 0;

  validate_no_self_loops<GT, SA>(g);

  DynMapTree<Node *, size_t> best_colors;
  size_t                     upper = dsatur_coloring<GT, SA>(g, best_colors);

  if (upper <= 2)
    {
      colors = std::move(best_colors);
      return upper;
    }

  DynList<Node *> node_list;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    node_list.append(it.get_curr());

  const size_t  n = node_list.size();
  Array<Node *> nodes(n, nullptr);
  size_t        idx = 0;
  for (auto it = node_list.get_it(); it.has_curr(); it.next_ne())
    nodes[idx++] = it.get_curr();

  Array<DynList<size_t>> adj_idx(n, DynList<size_t>{});
  {
    Cookie_Saver<GT> saver(g, true, false);
    for (size_t i = 0; i < n; ++i)
      NODE_COOKIE(nodes[i]) = reinterpret_cast<void *>(static_cast<uintptr_t>(i));

    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto *arc = it.get_curr();
        if (not SA()(arc))
          continue;
        size_t i = reinterpret_cast<uintptr_t>(NODE_COOKIE(g.get_src_node(arc)));
        size_t j = reinterpret_cast<uintptr_t>(NODE_COOKIE(g.get_tgt_node(arc)));
        if (i != j)
          {
            adj_idx[i].append(j);
            adj_idx[j].append(i);
          }
      }
  }

  Array<size_t> coloring(n, static_cast<size_t>(0));
  auto          reset_coloring = [&]()
  {
    for (size_t i = 0; i < n; ++i)
      coloring[i] = 0;
  };

  auto try_k_coloring = [&](size_t k, auto &self, size_t node_idx) -> bool
  {
    if (node_idx == n)
      return true;
    for (size_t c = 0; c < k; ++c)
      {
        bool feasible = true;
        for (auto jit = adj_idx[node_idx].get_it(); jit.has_curr(); jit.next_ne())
          {
            size_t j = jit.get_curr();
            if (j < node_idx and coloring[j] == c)
              {
                feasible = false;
                break;
              }
          }
        if (feasible)
          {
            coloring[node_idx] = c;
            if (self(k, self, node_idx + 1))
              return true;
          }
      }
    coloring[node_idx] = 0;
    return false;
  };

  size_t lo = 2, hi = upper;
  while (lo < hi)
    {
      size_t mid = (lo + hi) / 2;
      reset_coloring();
      if (try_k_coloring(mid, try_k_coloring, 0))
        hi = mid;
      else
        lo = mid + 1;
    }

  if (lo < upper)
    {
      reset_coloring();
      try_k_coloring(lo, try_k_coloring, 0);
      colors = {};
      for (size_t i = 0; i < n; ++i)
        colors.insert(nodes[i], coloring[i]);
    }
  else
    colors = std::move(best_colors);

  return lo;
}

/** @brief Functor wrapper for @ref greedy_coloring.
 *
 *  Provided for use with higher-order graph utilities that accept coloring
 *  algorithms as template parameters.  Forwards directly to
 *  @ref greedy_coloring; see that function for full semantics,
 *  complexity, and thread-safety notes.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Greedy_Coloring
{
public:
  /** @brief Execute greedy coloring.
   *  @param[in]  g      Input graph (no self-loops).
   *  @param[out] colors Output node→color map (overwritten).
   *  @return Number of distinct colors used.
   *  @throws std::domain_error if @p g contains a self-loop.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return greedy_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor wrapper for @ref welsh_powell_coloring.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Welsh_Powell_Coloring
{
public:
  /** @brief Execute Welsh-Powell coloring.
   *  @param[in]  g      Input graph (no self-loops).
   *  @param[out] colors Output node→color map (overwritten).
   *  @return Number of distinct colors used.
   *  @throws std::domain_error if @p g contains a self-loop.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return welsh_powell_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor wrapper for @ref dsatur_coloring.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc-filter functor (defaults to @ref Dft_Show_Arc<GT>).
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class DSatur_Coloring
{
public:
  /** @brief Execute DSatur coloring.
   *  @param[in]  g      Input graph (no self-loops).
   *  @param[out] colors Output node→color map (overwritten).
   *  @return Number of distinct colors used.
   *  @throws std::domain_error if @p g contains a self-loop.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return dsatur_coloring<GT, SA>(g, colors);
  }
};

}  // end namespace Aleph

#endif  // GRAPH_COLORING_H
