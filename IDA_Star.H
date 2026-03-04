
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


/** @file IDA_Star.H
 *  @brief IDA* (Iterative Deepening A*) shortest path algorithm.
 *
 *  Implements the IDA* algorithm, a memory-efficient variant of A* that
 *  uses iterative deepening with a cost threshold. Each iteration
 *  performs a depth-first search bounded by f(n) = g(n) + h(n), where
 *  g(n) is the cost from start and h(n) is the heuristic estimate.
 *
 *  ## How it works
 *
 *  1. Set initial threshold = h(start)
 *  2. Perform DFS from start, pruning nodes where f(n) > threshold
 *  3. If goal found, return the path
 *  4. Otherwise, set threshold = minimum f(n) that exceeded the
 *     previous threshold, and repeat from step 2
 *
 *  ## Advantages over A*
 *
 *  - **Linear memory**: O(d) where d is the solution depth, vs O(b^d)
 *    for A*. This makes IDA* practical for very large search spaces.
 *  - **No priority queue overhead**: Uses simple DFS recursion.
 *
 *  ## Disadvantages
 *
 *  - **Redundant work**: Nodes may be revisited across iterations.
 *  - **Not suitable for graphs with many distinct f-values**: Each
 *    distinct f-value causes a new iteration.
 *
 *  ## Complexity
 *
 *  | Aspect | Complexity |
 *  |--------|------------|
 *  | Time | O(b^d) (same as A* asymptotically) |
 *  | Space | O(d) (linear in solution depth) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Grid with (x,y) coordinates
 *  struct Coord { int x, y; };
 *  using G = List_Graph<Graph_Node<Coord>, Graph_Arc<int>>;
 *
 *  struct ManhattanH {
 *    using Distance_Type = int;
 *    int operator()(G::Node* from, G::Node* to) const {
 *      auto& f = from->get_info();
 *      auto& t = to->get_info();
 *      return std::abs(f.x - t.x) + std::abs(f.y - t.y);
 *    }
 *  };
 *
 *  IDA_Star<G, Dft_Dist<G>, ManhattanH> ida;
 *  Path<G> path(g);
 *  int dist = ida(g, start, goal, path);
 *  ```
 *
 *  @see AStar.H For heap-based A* (faster but more memory)
 *  @see Dijkstra.H For shortest paths without heuristic
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef IDA_STAR_H
# define IDA_STAR_H

# include <limits>
# include <cmath>
# include <tpl_graph_utils.H>
# include <ah-errors.H>
# include <AStar.H>

namespace Aleph
{

/** @brief Chebyshev (L-infinity) distance heuristic for 8-connected grids.
 *
 *  For grids where diagonal movement is allowed, the Chebyshev distance
 *  (also called chessboard distance) is an admissible heuristic.
 *  It equals max(|dx|, |dy|).
 *
 *  Assumes node info has `x` and `y` fields.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance accessor functor.
 *
 *  @ingroup Graphs
 */
template <class GT, class Distance = Dft_Dist<GT>>
struct Chebyshev_Heuristic
{
  using Distance_Type = typename Distance::Distance_Type;

  Distance_Type operator()(typename GT::Node * from,
                           typename GT::Node * to) const
  {
    auto & f = from->get_info();
    auto & t = to->get_info();
    auto dx = std::abs(f.x - t.x);
    auto dy = std::abs(f.y - t.y);
    return static_cast<Distance_Type>(dx > dy ? dx : dy);
  }
};

/** @brief IDA* algorithm for memory-efficient shortest path search.
 *
 *  @details This class implements the Iterative Deepening A* (IDA*)
 *  algorithm, which combines the optimality of A* with the linear
 *  memory usage of iterative deepening depth-first search.
 *
 *  The algorithm performs repeated depth-limited DFS searches with
 *  increasing f-cost thresholds. In each iteration, nodes with
 *  f(n) = g(n) + h(n) exceeding the threshold are pruned.
 *
 *  The class receives the following template parameters:
 *  -# `GT`: graph type.
 *  -# `Distance`: weight accessor (default: Dft_Dist<GT>).
 *  -# `Heuristic`: h(n) estimator. Must be admissible for optimality.
 *      Signature: `Distance_Type operator()(Node* from, Node* to)`.
 *  -# `Itor`: arc iterator template (defaults to Node_Arc_Iterator).
 *  -# `SA`: arc filter for internal iterators.
 *
 *  @warning The heuristic must be admissible (never overestimate) for
 *           the algorithm to find optimal paths.
 *
 *  @warning All edge weights must be non-negative.
 *
 *  @note IDA* may revisit nodes across iterations. For graphs with
 *        many distinct f-values, A* may be more efficient.
 *
 *  @see AStar_Min_Path For heap-based A*.
 *  @see Dijkstra_Min_Paths For Dijkstra without heuristic.
 *
 *  @ingroup Graphs
 */
template <class GT,
          class Distance = Dft_Dist<GT>,
          class Heuristic = Zero_Heuristic<GT, Distance>,
          template <typename, class> class Itor = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>>
class IDA_Star
{
public:
  using Distance_Type = typename Distance::Distance_Type;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

private:
  SA sa;
  Distance distance;
  Heuristic heuristic;

  static constexpr Distance_Type Inf =
    std::numeric_limits<Distance_Type>::max();

  // Sentinel: FOUND means the goal was found
  static constexpr Distance_Type FOUND = Distance_Type(-1);

  /** Recursive DFS bounded by threshold.
   *
   *  @return FOUND if goal was reached, or the minimum f-value that
   *          exceeded the threshold (for next iteration).
   */
  Distance_Type search(const GT & g, Node * curr, Node * end,
                       Distance_Type g_cost, Distance_Type threshold,
                       Path<GT> & path)
  {
    auto f = g_cost + heuristic(curr, end);

    if (f > threshold)
      return f;

    if (curr == end)
      return FOUND;

    NODE_BITS(curr).set_bit(Find_Path, true);

    Distance_Type min_threshold = Inf;

    for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
      {
        auto arc = it.get_current_arc();
        auto tgt = g.get_connected_node(arc, curr);

        if (IS_NODE_VISITED(tgt, Find_Path))
          continue;

        path.append(arc);

        auto w = distance(arc);
        auto t = search(g, tgt, end, g_cost + w, threshold, path);

        if (t == FOUND)
          return FOUND;

        if (t < min_threshold)
          min_threshold = t;

        path.remove_last_node();
      }

    NODE_BITS(curr).set_bit(Find_Path, false);

    return min_threshold;
  }

public:
  /** @brief Constructor.
   *
   *  @param[in] dist Distance functor for arc weights.
   *  @param[in] h Heuristic functor.
   *  @param[in] __sa Arc filter for iterators.
   */
  IDA_Star(Distance dist = Distance(),
           Heuristic h = Heuristic(),
           SA __sa = SA())
    : sa(__sa), distance(dist), heuristic(h)
  {
    // empty
  }

  /** @brief Finds the shortest path from start to end using IDA*.
   *
   *  Performs iterative deepening with f-cost thresholds. Each
   *  iteration runs a depth-first search bounded by the threshold.
   *  The threshold starts at h(start) and increases to the minimum
   *  f-value that exceeded the previous threshold.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path (empty if no path exists).
   *  @return The total cost, or max value if no path exists.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   *  @throw bad_alloc If there is not enough memory.
   *
   *  @note Time complexity: O(b^d) where b is branching factor and
   *        d is solution depth.
   *  @note Space complexity: O(d) (linear in solution depth).
   */
  Distance_Type find_path(const GT & g, Node * start, Node * end,
                          Path<GT> & path)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    path.empty();

    if (start == end)
      {
        path.set_graph(g, start);
        return Distance_Type(0);
      }

    Distance_Type threshold = heuristic(start, end);

    // Initialize path with start node
    path.set_graph(g, start);

    while (true)
      {
        g.reset_bit_nodes(Find_Path);

        auto t = search(g, start, end, Distance_Type(0), threshold, path);

        if (t == FOUND)
          {
            // Compute actual path cost
            Distance_Type total = 0;
            path.for_each_arc([&total, this](Arc * a)
              {
                total += distance(a);
              });
            return total;
          }

        if (t == Inf)
          {
            path.empty();
            return Inf;
          }

        threshold = t;

        // Reset path for next iteration
        path.empty();
        path.set_graph(g, start);
      }
  }

  /** @brief Finds shortest path (operator interface).
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path.
   *  @return The total cost, or max value if no path exists.
   */
  Distance_Type operator()(const GT & g, Node * start, Node * end,
                           Path<GT> & path)
  {
    return find_path(g, start, end, path);
  }
};

} // end namespace Aleph

# endif // IDA_STAR_H
