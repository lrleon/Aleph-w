
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


/** @file Floyd_Warshall.H
 *  @brief Floyd-Warshall algorithm for all-pairs shortest paths.
 *
 *  This file implements the Floyd-Warshall algorithm, a dynamic programming
 *  approach to compute the shortest paths between all pairs of vertices in
 *  a weighted graph.
 *
 *  ## Algorithm Overview
 *
 *  Floyd-Warshall works by iteratively improving path estimates. For each
 *  intermediate vertex k, it checks if going through k provides a shorter
 *  path between any pair (i, j).
 *
 *  ## Key Features
 *
 *  - Computes shortest paths between ALL pairs of vertices
 *  - Handles negative edge weights (but not negative cycles)
 *  - Returns both distance matrix and path reconstruction matrix
 *  - Can detect negative cycles (diagonal becomes negative)
 *
 *  ## Complexity
 *
 *  | Metric | Value |
 *  |--------|-------|
 *  | Time | O(V³) |
 *  | Space | O(V²) for matrices |
 *
 *  ## When to Use
 *
 *  - Dense graphs where you need all-pairs distances
 *  - Small to medium graphs (V < 1000)
 *  - When negative weights are present (but no negative cycles)
 *
 *  For sparse graphs or single-source queries, consider:
 *  - Dijkstra (single-source, no negative weights)
 *  - Bellman-Ford (single-source, with negative weights)
 *  - Johnson (all-pairs, sparse graphs with negative weights)
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build graph ...
 *
 *  Floyd_All_Shortest_Paths<decltype(g)> floyd(g);
 *  floyd.compute();
 *
 *  // Get distance between nodes i and j
 *  auto dist = floyd.get_dist(node_i, node_j);
 *
 *  // Reconstruct path
 *  Path<Graph> path = floyd.get_min_path(node_i, node_j);
 *  ```
 *
 *  @see Dijkstra.H For single-source shortest paths
 *  @see Bellman_Ford.H For negative weight handling
 *  @see Johnson.H For sparse all-pairs with negative weights
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef FLOYD_WARSHALL_H
# define FLOYD_WARSHALL_H

# include <sstream>
# include <iostream>
# include <limits>
# include <ahFunction.H>
# include <ahSort.H>
# include <tpl_indexArc.H>
# include <tpl_dynMat.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  /**
      Compute the all-pairs shortest path distance matrix and the
      path reconstruction matrix for a graph `g` using the
      Floyd-Warshall algorithm.

      This class computes two matrices:
      -# `dist`: matrix of shortest-path costs between all node pairs.
         Each entry `dist(i,j)` stores the minimum total cost to go from
         the node with index `i` to the node with index `j`.
      -# `path`: path reconstruction matrix. Each entry `path(i,j)` stores
         the next node index on a shortest path from `i` to `j`. By
         iterating through `path(k,j)` you can reconstruct the path.
         The helper get_min_path() performs this reconstruction.

      Floyd-Warshall supports negative edge weights, but it does not work
      correctly if the graph contains negative cycles. Use Bellman-Ford
      based algorithms if you suspect negative cycles.

      Template parameters:
      -# `GT`: graph type.
      -# `Distance<GT>`: arc weight accessor that must provide:
         -# `Distance<GT>::Distance_Type`: the weight type.
         -# `Distance_Type operator()(typename GT::Arc *a)`: returns the
            weight of arc `a`.
         -# `Distance<GT>::Max_Distance`: a sentinel maximum value that is
            treated as infinity.
         -# `Distance<GT>::Zero_Distance`: additive identity (usually 0).
      -# `SA`: arc filter.

      @see dijkstra_min_spanning_tree() dijkstra_min_path()
      find_path_depth_first()
      find_min_path() bellman_ford_min_spanning_tree()
      q_bellman_ford_min_spanning_tree()
      @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Floyd_All_Shortest_Paths
  {
    typedef typename GT::Node Node;
    typedef typename GT::Arc Arc;
    typedef typename Distance::Distance_Type Distance_Type;

    DynArray<Node *> nodes;
    GT & g;
    const long n;
    const Distance_Type Inf;
    bool negative_cycle = false;
    DynMatrix<long> path_mat;
    DynMatrix<Distance_Type> dist;
    SA & sa;

  public:
    /**
        Check if the graph contains a negative cycle.

        @return true if a negative cycle was detected, false otherwise.
     */
    [[nodiscard]] constexpr bool has_negative_cycle() const noexcept { return negative_cycle; }

    /**
        Get the path reconstruction matrix.

        The path matrix contains intermediate nodes for shortest path reconstruction.
        Entry (i,j) contains the next node index on the shortest path from i to j.

        @return Const reference to the path reconstruction matrix.
     */
    const DynMatrix<long> &get_path_mat() const noexcept { return path_mat; }

    /**
        Get the distance matrix.

        The distance matrix contains shortest distances between all pairs of nodes.
        Entry (i,j) contains the shortest distance from node i to node j.

        @return Const reference to the distance matrix.
     */
    const DynMatrix<Distance_Type> &get_dist_mat() const noexcept
    {
      return dist;
    }

    /**
        Get the array of graph nodes.

        Returns the sorted array of node pointers used for matrix indexing.

        @return Const copy of the node pointer array.
     */
    DynArray<Node *> get_nodes() const noexcept { return nodes; }

    /**
        Get the graph node at a given matrix index.

        @param[in] i Matrix index (must be in range [0, n)).
        @return Pointer to the node at index i.
     */
    Node * select_node(long i) const noexcept { return nodes(i); }

    /// Return the adjacency-matrix index corresponding to node `p`.
    /**
        Find the matrix index corresponding to a graph node.

        @param[in] p Pointer to the node to find.
        @return Matrix index of the node.
        @throw std::domain_error if the node is not found.
     */
    long index_node(Node *p) const
    {
      ah_invalid_argument_if(p == nullptr) << "Node pointer cannot be null";
      auto i = binary_search(nodes, p);
      ah_domain_error_if(i < 0 or i >= nodes.size() or nodes(i) != p)
        << "Floyd_All_Shortest_Paths::index_node() node not found";
      return i;
    }

  public:
    /**
        Construct Floyd-Warshall solver for all-pairs shortest paths.

        Computes shortest distances and path reconstruction matrices for all
        pairs of nodes in the graph using the Floyd-Warshall algorithm.
        Handles negative weights but detects negative cycles.

        @param[in] __g Input graph (will be treated as const).
        @param[in] __sa Arc filter for considering which arcs to include.
        @throw std::invalid_argument if graph has no nodes.
        @throw std::bad_alloc if memory allocation fails.
     */
    Floyd_All_Shortest_Paths(const GT & __g, SA & __sa)
      : g(const_cast<GT &>(__g)), n(g.get_num_nodes()),
        Inf(std::numeric_limits<Distance_Type>::max()),
        path_mat(n, n), dist(n, n), sa(__sa)
    {
      ah_invalid_argument_if(n <= 0) << "Graph must contain at least one node";

      long i = 0;
      nodes.reserve(n);
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        nodes.access(i++) = it.get_curr();
      in_place_sort(nodes); // sort by node pointer value

      dist.allocate();
      path_mat.allocate(); { // initialize matrices
        IndexArc<GT, Rand_Tree, SA> arcs(g, true, sa);

        for (long i = 0; i < n; ++i)
          for (long j = 0; j < n; ++j)
            path_mat(i, j) = -1;

        for (long i = 0; i < n; ++i)
          {
            Node *src = nodes(i);
            for (long j = 0; j < n; ++j)
              {
                if (i == j)
                  {
                    dist(i, j) = 0;
                    path_mat(i, j) = j;
                    continue;
                  }

                Node *tgt = nodes(j);
                Arc *arc = arcs.search_directed(src, tgt);
                if (arc == nullptr)
                  {
                    dist(i, j) = Inf;
                    continue;
                  }

                dist(i, j) = Distance()(arc);
                path_mat(i, j) = j;
              }
          }
      }

      for (long k = 0; k < n; ++k)
        for (long i = 0; i < n; ++i)
          {
            const Distance_Type & dik = dist(i, k);
            for (long j = 0; j < n; ++j)
              {
                const Distance_Type & dij = dist(i, j);
                if (dik == Inf)
                  continue;

                const Distance_Type & dkj = dist(k, j);
                if (dkj == Inf)
                  continue;

                // Calculate new distance through intermediate node k.
                // Important: avoid undefined behavior on signed overflow
                // (e.g. Inf - negative) and handle negative weights safely.
                bool can_add = true;
                if constexpr (std::numeric_limits<Distance_Type>::is_integer)
                  {
                    if constexpr (std::numeric_limits<Distance_Type>::is_signed)
                      {
                        const Distance_Type maxv = std::numeric_limits<Distance_Type>::max();
                        const Distance_Type minv = std::numeric_limits<Distance_Type>::lowest();
                        if (dkj > 0)
                          can_add = (dik <= maxv - dkj);
                        else if (dkj < 0)
                          can_add = (dik >= minv - dkj);
                      }
                    else
                      {
                        can_add = (dik <= std::numeric_limits<Distance_Type>::max() - dkj);
                      }
                  }

                if (!can_add)
                  continue;

                const Distance_Type new_dist = dik + dkj;
                if (new_dist < dij)
                  {
                    dist(i, j) = new_dist;
                    path_mat(i, j) = path_mat(i, k);
                  }
              }
          }

      // Check for negative cycles on the diagonal
      for (long idx = 0; idx < n; ++idx)
        if (dist(idx, idx) < 0)
          {
            negative_cycle = true;
            break;
          }
    }

    /**
        Construct Floyd-Warshall solver with rvalue arc filter.

        @param[in] g Input graph.
        @param[in] sa Arc filter (moved).
     */
    Floyd_All_Shortest_Paths(const GT & g, SA && sa = SA())
      : Floyd_All_Shortest_Paths(g, sa) {}

    /**
        Convert a distance value to string representation.

        @param[in] e Distance value to convert.
        @return String representation ("Inf" for infinite distances).
     */
    std::string entry(const Distance_Type & e) const
    {
      if (e == Inf)
        return "Inf";

      std::stringstream ss;
      ss << e;
      return ss.str();
    }

    /**
        Print a distance matrix to standard output.

        Prints the distance matrix in a readable format, showing "inf" for
        infinite distances and numeric values otherwise.

        @param[in] dist Distance matrix to print.
     */
    static void print(const DynMatrix<Distance_Type> & dist)
    {
      const Distance_Type Inf = std::numeric_limits<Distance_Type>::max();
      const int n = dist.rows();
      for (int i = 0; i < n; ++i)
        {
          for (int j = 0; j < n; ++j)
            if (dist.access(i, j) == Inf)
              std::cout << "inf ";
            else
              std::cout << dist.access(i, j) << " ";
          std::cout << '\n';
        }
      std::cout << '\n';
    }

    /**
        Reconstruct the shortest path between two nodes by matrix indices.

        Uses the path reconstruction matrix to build the shortest path from
        source to target node.

        @param[in] src_idx Matrix index of source node.
        @param[in] tgt_idx Matrix index of target node.
        @return Path object containing the shortest path.
        @throw std::out_of_range if indices are invalid.
     */
    Path<GT> get_min_path(const long src_idx, const long tgt_idx) const
    {
      ah_out_of_range_error_if(src_idx < 0 or src_idx >= n)
        << "Source index out of range";
      ah_out_of_range_error_if(tgt_idx < 0 or tgt_idx >= n)
        << "Target index out of range";

      if (dist(src_idx, tgt_idx) == Inf)
        return Path<GT>(g);

      auto src = nodes(src_idx);
      Path<GT> path(g, src);

      if (src_idx == tgt_idx)
        return path;

      long i = src_idx;
      while (true)
        {
          const auto & k = path_mat(i, tgt_idx);
          auto p = nodes(k);
          path.append_directed(p);
          if (k == tgt_idx)
            break;
          i = k;
        }

      return path;
    }

    /**
        Reconstruct the shortest path between two nodes.

        Convenience method that accepts node pointers instead of indices.

        @param[in] src Pointer to source node.
        @param[in] tgt Pointer to target node.
        @return Path object containing the shortest path.
        @throw std::invalid_argument if either node pointer is null.
        @throw std::domain_error if either node is not found in the graph.
     */
    Path<GT> get_min_path(typename GT::Node *src,
                          typename GT::Node *tgt) const
    {
      return get_min_path(index_node(src), index_node(tgt));
    }
  };
} // end namespace Aleph

# endif // FLOYD_WARSHALL_H
