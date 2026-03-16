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
 *  ## Included Algorithms:
 *
 *  - **Greedy Coloring**: Processes nodes in their natural iteration order.
 *    Fast but highly sensitive to vertex ordering.
 *  - **Welsh-Powell**: An improvement over the greedy algorithm that sorts
 *    nodes by decreasing degree before coloring. Usually yields better results.
 *  - **DSatur (Degree of Saturation)**: An adaptive heuristic that selects the
 *    next node based on its "saturation degree". One of the best general-purpose
 *    heuristics.
 *  - **Chromatic Number**: Computes the exact chromatic number ($\chi(G)$) for
 *    small graphs (up to 64 nodes) using backtracking and DSatur bounds.
 *
 *  ## Implementation Details
 *
 *  All algorithms use the `NODE_COOKIE` field of the graph nodes to temporarily
 *  store colors efficiently ($O(1)$). To guarantee exception safety and
 *  avoid destroying previous data, @ref Cookie_Saver is employed, which
 *  automatically restores the original cookies upon function exit.
 *
 *  Results are returned in a @ref DynMapTree that maps each node pointer to its
 *  color index (0-based).
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

/** @brief Internal implementation details for graph coloring.
 *
 *  Contains utilities for color encoding within cookies and searching
 *  for available colors.
 */
namespace graph_coloring_detail {

/** @brief Encodes a color index into a pointer for storage in a cookie.
 *  @param[in] c Color index (starting from 0).
 *  @return Pointer representing color `c`.
 *  @note Internal encoding: `(void*)(c + 1)`. `nullptr` represents "uncolored".
 */
inline void *encode_color(size_t c) noexcept
{
  return reinterpret_cast<void *>(static_cast<uintptr_t>(c + 1));
}

/** @brief Checks if a node has a color assigned based on its cookie.
 *  @param[in] cookie Node's cookie value.
 *  @return `true` if the node is colored, `false` otherwise.
 */
inline bool is_colored(void *cookie) noexcept
{
  return cookie != nullptr;
}

/** @brief Decodes the color index stored in a cookie.
 *  @param[in] cookie Cookie value.
 *  @return The original color index.
 *  @pre The cookie must represent a colored node (`is_colored(cookie)`).
 */
inline size_t decode_color(void *cookie) noexcept
{
  return static_cast<size_t>(reinterpret_cast<uintptr_t>(cookie)) - 1;
}

/** @brief Finds the smallest non-negative integer (color) not in the `used` set.
 *  @param[in] used Set of colors already used by neighbors.
 *  @return The lowest available color.
 *  @note Complexity: $O(k \log k)$ where $k$ is the number of used colors.
 */
inline size_t smallest_available(const DynSetTree<size_t> &used)
{
  size_t c = 0;
  while (used.search(c) != nullptr)
    ++c;
  return c;
}

/** @brief Iterates over all adjacent nodes in both directions for directed graphs.
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 *  @tparam Op Operation to apply to each adjacent node.
 *  @param[in] g Graph.
 *  @param[in] v Node whose neighbors to iterate.
 *  @param[in] op Operation to apply.
 *  @note For directed graphs (List_Digraph), this correctly handles both incoming
 *        and outgoing arcs by combining Out_Iterator with a full arc scan.
 */
template <class GT, class SA = Dft_Show_Arc<GT>, class Op>
void for_each_adjacent_node(const GT &g, typename GT::Node *v, Op &&op)
{
  DynSetTree<typename GT::Node *> seen;

  // Outgoing arcs from v
  for (Out_Iterator<GT, SA> ait(v); ait.has_curr(); ait.next_ne())
    {
      auto *w = ait.get_node_ne();
      if (seen.search(w) == nullptr)
        {
          seen.insert(w);
          op(w);
        }
    }

  // Incoming arcs to v: iterate all graph arcs
  for (Arc_Iterator<GT, SA> ait(g); ait.has_curr(); ait.next_ne())
    {
      auto *arc = ait.get_curr();
      if (g.get_tgt_node(arc) == v)
        {
          auto *w = g.get_src_node(arc);
          if (seen.search(w) == nullptr)
            {
              seen.insert(w);
              op(w);
            }
        }
    }
}

/** @brief Validates that the graph contains no self-loops.
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 *  @param[in] g Graph to validate.
 *  @throws std::domain_error if any self-loop is detected.
 *  @note Graph coloring algorithms do not support self-loops since a node
 *        cannot be adjacent to itself with a different color.
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
void validate_no_self_loops(const GT &g)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto *v = it.get_curr();
      for_each_adjacent_node<GT, SA>(g, v, [&v](auto *w) {
        ah_domain_error_if(w == v) << "graph coloring does not support self-loops";
      });
    }
}
}  // end namespace graph_coloring_detail

/** @brief Greedy coloring following the iteration order.
 *
 *  Traverses the graph nodes in their natural order and assigns each the
 *  first available color not being used by its already colored neighbors.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter (defaults to @ref Dft_Show_Arc<GT>).
 *  @param[in] g Graph to color.
 *  @param[out] colors Map where the result (Node -> Color) will be saved.
 *  @return The total number of colors used.
 *
 *  @pre The graph must be valid and `colors` must be ready to receive data.
 *  @post `is_valid_coloring(g, colors)` will be `true`.
 *
 *  @note **Complexity**: $O((V+E) \log \Delta)$, where $\Delta$ is the maximum degree.
 *  @note The result critically depends on the iteration order of the nodes.
 *
 *  @see welsh_powell_coloring, dsatur_coloring, is_valid_coloring
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

  // Guarantees that previous cookies are restored on exit, even in case of exception.
  Cookie_Saver<GT> saver(g, true, false);

  // Initialize cookies to nullptr (uncolored)
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    NODE_COOKIE(it.get_curr()) = nullptr;

  size_t num_colors = 0;

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node              *v = it.get_curr();
      DynSetTree<size_t> used;

      // Collect colors from neighbors
      for_each_adjacent_node<GT, SA>(g, v, [&used](Node *w) {
        if (is_colored(NODE_COOKIE(w)))
          used.insert(decode_color(NODE_COOKIE(w)));
      });

      // Assign the smallest available color
      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Welsh-Powell coloring heuristic.
 *
 *  Sorts nodes by decreasing degree and then applies the greedy algorithm.
 *  This strategy usually reduces the number of colors required by processing
 *  the most "difficult" nodes (those with higher degrees) first.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter (defaults to @ref Dft_Show_Arc<GT>).
 *  @param[in] g Graph to color.
 *  @param[out] colors Map with the coloring result.
 *  @return The total number of colors used.
 *
 *  @note **Complexity**: $O((V+E) \log \Delta + V \log V)$ due to sorting.
 *
 *  @see greedy_coloring, dsatur_coloring
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

  Cookie_Saver<GT> saver(g, true, false);

  // Collect nodes and clear cookies
  DynList<Node *> nodes;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *p        = it.get_curr();
      NODE_COOKIE(p) = nullptr;
      nodes.append(p);
    }

  // Sort by decreasing degree
  mergesort(nodes, [&g](Node *a, Node *b) { return g.get_num_arcs(a) > g.get_num_arcs(b); });

  size_t num_colors = 0;

  // Apply greedy coloring in the sorted order
  for (auto nit = nodes.get_it(); nit.has_curr(); nit.next_ne())
    {
      Node              *v = nit.get_curr();
      DynSetTree<size_t> used;

      for_each_adjacent_node<GT, SA>(g, v, [&used](Node *w) {
        if (is_colored(NODE_COOKIE(w)))
          used.insert(decode_color(NODE_COOKIE(w)));
      });

      size_t c       = smallest_available(used);
      NODE_COOKIE(v) = encode_color(c);
      colors.insert(v, c);

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief DSatur (Degree of Saturation) coloring heuristic.
 *
 *  DSatur is an adaptive algorithm that, in each step, selects the uncolored
 *  node with the highest "saturation degree" (the number of distinct colors
 *  assigned to its neighbors). Ties are broken by choosing the node with the
 *  highest degree in the original graph.
 *
 *  It is widely considered one of the best heuristics for general graph
 *  coloring, frequently outperforming Welsh-Powell.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter (defaults to @ref Dft_Show_Arc<GT>).
 *  @param[in] g Graph to color.
 *  @param[out] colors Map with the result.
 *  @return The total number of colors used.
 *
 *  @note **Complexity**: $O(V \cdot (V+E))$ in this implementation.
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

  const size_t n = g.get_num_nodes();
  if (n == 0)
    return 0;

  validate_no_self_loops<GT, SA>(g);

  Cookie_Saver<GT> saver(g, true, false);

  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    NODE_COOKIE(it.get_curr()) = nullptr;

  size_t num_colors    = 0;
  size_t colored_count = 0;

  while (colored_count < n)
    {
      // Adaptive step: find the uncolored node with the highest saturation degree.
      Node  *best     = nullptr;
      size_t best_sat = 0;
      size_t best_deg = 0;

      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        {
          Node *v = it.get_curr();
          if (is_colored(NODE_COOKIE(v)))
            continue;

          // Saturation degree: count of distinct colors among neighbors
          DynSetTree<size_t> neighbor_colors;
          for_each_adjacent_node<GT, SA>(g, v, [&neighbor_colors](Node *w) {
            if (is_colored(NODE_COOKIE(w)))
              neighbor_colors.insert(decode_color(NODE_COOKIE(w)));
          });

          size_t sat = neighbor_colors.size();
          size_t deg = g.get_num_arcs(v);

          // Priority: highest saturation, then highest degree.
          if (best == nullptr or sat > best_sat or (sat == best_sat and deg > best_deg))
            {
              best     = v;
              best_sat = sat;
              best_deg = deg;
            }
        }

      // Color the chosen node
      DynSetTree<size_t> used;
      for_each_adjacent_node<GT, SA>(g, best, [&used](Node *w) {
        if (is_colored(NODE_COOKIE(w)))
          used.insert(decode_color(NODE_COOKIE(w)));
      });

      size_t c          = smallest_available(used);
      NODE_COOKIE(best) = encode_color(c);
      colors.insert(best, c);
      ++colored_count;

      if (c + 1 > num_colors)
        num_colors = c + 1;
    }

  return num_colors;
}

/** @brief Validates if a given coloring for a graph is correct.
 *
 *  A coloring is valid if:
 *  1. All graph nodes have an entry in the `colors` map.
 *  2. No two adjacent nodes share the same color.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter (defaults to @ref Dft_Show_Arc<GT>).
 *  @param[in] g Graph.
 *  @param[in] colors Color map to validate.
 *  @return `true` if the coloring is valid, `false` otherwise.
 *
 *  @note **Complexity**: $O(V + E)$.
 *  @note The presence of self-loops makes any valid coloring impossible, so the 
 *        function returns false if any are detected.
 *
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
[[nodiscard]] bool is_valid_coloring(const GT &g, const DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;

  // Each node must have a color
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (colors.search(it.get_curr()) == nullptr)
      return false;

  // No edge should connect nodes with the same color
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node  *v  = it.get_curr();
      size_t cv = colors.find(v);

      bool valid = true;
      graph_coloring_detail::for_each_adjacent_node<GT, SA>(g, v, [&](Node *w) {
        if (w == v)
          {
            valid = false;
            return;
          }

        size_t cw = colors.find(w);
        if (cv == cw)
          valid = false;
      });

      if (not valid)
        return false;
    }

  return true;
}

/** @brief Computes the exact chromatic number of a small graph.
 *
 *  The chromatic number $\chi(G)$ is the minimum number of colors required to
 *  color the graph. This problem is NP-hard.
 *
 *  The function uses a hybrid approach:
 *  1. Obtains an upper bound using DSatur.
 *  2. Performs a binary search over the number of colors $k \in [2, \text{bound}]$.
 *  3. For each $k$, uses a backtracking algorithm to determine if the graph
 *     is $k$-colorable.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter (defaults to @ref Dft_Show_Arc<GT>).
 *  @param[in] g Graph (maximum 64 nodes).
 *  @param[out] colors Map that will be filled with an optimal coloring.
 *  @return The exact chromatic number.
 *
 *  @throws std::domain_error If the graph has more than 64 nodes.
 *
 *  @note **Complexity**: Exponential in the worst case. It is limited to $V \le 64$
 *        to avoid excessive execution times.
 *
 *  @see dsatur_coloring, is_valid_coloring
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
size_t chromatic_number(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors)
{
  using Node = typename GT::Node;

  graph_coloring_detail::validate_no_self_loops<GT, SA>(g);

  ah_domain_error_if(g.get_num_nodes() > 64)
    << "chromatic_number: graph has " << g.get_num_nodes() << " nodes (max 64)";

  colors = {};

  if (g.get_num_nodes() == 0)
    return 0;

  // Trivial case: graph with no edges, chromatic number is 1.
  if (g.get_num_arcs() == 0)
    {
      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        colors.insert(it.get_curr(), 0);
      return 1;
    }

  // Step 1: Upper bound with DSatur
  DynMapTree<Node *, size_t> best_colors;
  size_t                     upper = dsatur_coloring<GT, SA>(g, best_colors);

  if (upper <= 2)
    {
      colors = std::move(best_colors);
      return upper;
    }

  // Step 2: Exact search via backtracking.
  // Prepare an indexed representation for better speed.
  DynList<Node *> node_list;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    node_list.append(it.get_curr());

  const size_t n = node_list.size();
  Array<Node *> nodes(n, nullptr);
  size_t idx = 0;
  for (auto it = node_list.get_it(); it.has_curr(); it.next_ne())
    nodes[idx++] = it.get_curr();

  // Index-based adjacency list.
  Array<DynList<size_t>> adj(n, DynList<size_t>{});
  {
    Cookie_Saver<GT> saver(g, true, false);
    for (size_t i = 0; i < n; ++i)
      NODE_COOKIE(nodes[i]) = reinterpret_cast<void *>(static_cast<uintptr_t>(i));

    for (size_t i = 0; i < n; ++i)
      graph_coloring_detail::for_each_adjacent_node<GT, SA>(g, nodes[i], [&](Node *w) {
        size_t j = reinterpret_cast<uintptr_t>(NODE_COOKIE(w));
        if (j != i)
          adj[i].append(j);
      });
  }

  // Coloring state for backtracking
  Array<size_t> coloring(n, static_cast<size_t>(0));

  auto reset_coloring = [&]()
    {
      for (size_t i = 0; i < n; ++i)
        coloring[i] = 0;
    };

  // Internal backtracking function to verify k-colorability.
  auto try_k_coloring = [&](size_t k, auto &self, size_t node_idx) -> bool {
    if (node_idx == n)
      return true;

    for (size_t c = 0; c < k; ++c)
      {
        bool feasible = true;
        for (auto jit = adj[node_idx].get_it(); jit.has_curr(); jit.next_ne())
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

  // Binary search to find the minimum k in the range [2, upper).
  size_t lo = 2;
  size_t hi = upper;

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
      // Found value is optimal; reconstruct color map.
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

/** @brief Functor for greedy coloring (@ref greedy_coloring).
 *
 *  Allows using the algorithm in contexts requiring a function object.
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class Greedy_Coloring
{
public:
  /** @brief Executes greedy coloring.
   *  @param[in] g Graph.
   *  @param[out] colors Result.
   *  @return Number of colors.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return greedy_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor for Welsh-Powell coloring (@ref welsh_powell_coloring).
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class Welsh_Powell_Coloring
{
public:
  /** @brief Executes Welsh-Powell.
   *  @param[in] g Graph.
   *  @param[out] colors Result.
   *  @return Number of colors.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return welsh_powell_coloring<GT, SA>(g, colors);
  }
};

/** @brief Functor for DSatur coloring (@ref dsatur_coloring).
 *
 *  @tparam GT Graph type.
 *  @tparam SA Arc filter.
 *  @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> class DSatur_Coloring
{
public:
  /** @brief Executes DSatur.
   *  @param[in] g Graph.
   *  @param[out] colors Result.
   *  @return Number of colors.
   */
  size_t operator()(const GT &g, DynMapTree<typename GT::Node *, size_t> &colors) const
  {
    return dsatur_coloring<GT, SA>(g, colors);
  }
};

}  // end namespace Aleph

#endif  // GRAPH_COLORING_H
