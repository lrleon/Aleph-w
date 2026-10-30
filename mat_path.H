
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


/** @file mat_path.H
 *  @brief Path reconstruction from Floyd-Warshall path matrices.
 *
 *  This file provides utilities for reconstructing minimum paths from
 *  the path matrix computed by the Floyd-Warshall all-pairs shortest
 *  path algorithm.
 *
 *  ## Overview
 *
 *  After running Floyd-Warshall (see latex_floyd.H or Floyd_Warshall.H),
 *  you obtain two matrices:
 *  - **Distance matrix (dist)**: Contains the shortest path distances
 *  - **Path matrix (path)**: Contains intermediate node indices for path reconstruction
 *
 *  This file provides `find_min_path()` to reconstruct the actual path
 *  from the path matrix.
 *
 *  ## Path Matrix Interpretation
 *
 *  For a path from node i to node j:
 *  - `path(i, j)` contains the index of the next node after i on the shortest path
 *  - To reconstruct the full path: start at i, follow path(i, j), path(path(i,j), j), etc.
 *
 *  ## Usage Example
 *
 *  @code{.cpp}
 *  #include <latex_floyd.H> // it includes mat_path.H
 *
 *  // Create the matrices and run Floyd-Warshall
 *  Ady_Mat<Graph, double> dist(graph);
 *  Ady_Mat<Graph, long> path(graph);
 *  floyd_all_shortest_paths(graph, dist, path);
 *
 *  // Reconstruct path from node 0 to node 5
 *  Path<Graph> min_path(graph);
 *  find_min_path(path, 0L, 5L, min_path);
 *
 *  // Or using node pointers
 *  find_min_path(path, src_node, tgt_node, min_path);
 *  @endcode
 *
 *  @see latex_floyd.H Floyd-Warshall algorithm with LaTeX output
 *  @see Floyd_Warshall.H Standard Floyd-Warshall implementation
 *  @see tpl_matgraph.H Matrix-based graph representations
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef MAT_PATH_H
#define MAT_PATH_H

#include <concepts>
#include <functional>
#include <utility>
#include <tpl_matgraph.H>

namespace Aleph
{

namespace mat_path_detail
{
  /// Whether the arcs of `GT` give their weights by
  /// `get_info().get_distance()`, and `Compare` compares them.
  template <class GT, class Compare>
  concept Arc_With_Distance = requires (typename GT::Arc * a, Compare cmp)
  {
    { cmp(a->get_info().get_distance(), a->get_info().get_distance()) }
      -> std::convertible_to<bool>;
  };

  // Appends node to path through the lightest, by Compare, of the arcs
  // from the last node of path to node, if the arcs give their weights;
  // otherwise through the first one, as Path::append(Node *) does
  template <class GT, class Compare>
  void append_node(GT & g, Path<GT> & path, typename GT::Node * node)
  {
    if constexpr (Arc_With_Distance<GT, Compare>)
      {
        typename GT::Node * src = path.get_last_node();
        typename GT::Node * tgt = node;

        // In a graph that is not directed, the arcs of the node with fewer
        // arcs are searched, as search_arc() does
        if (not g.is_digraph() and tgt->num_arcs < src->num_arcs)
          std::swap(src, tgt);

        typename GT::Arc * lightest = nullptr;
        for (typename GT::Node_Arc_Iterator it(src); it.has_curr(); it.next_ne())
          if (it.get_tgt_node_ne() == tgt and
              (lightest == nullptr or
               Compare () (it.get_current_arc_ne()->get_info().get_distance(),
                           lightest->get_info().get_distance())))
            lightest = it.get_current_arc_ne();

        ah_invalid_argument_if(lightest == nullptr)
          << "find_min_path(): there is no arc between two nodes of the path";

        path.append(lightest);
      }
    else
      path.append(node);
  }
} // namespace mat_path_detail

/** @brief Build a minimum path from Floyd-Warshall path matrix using indices.
 *
 *  Takes a path matrix computed by Floyd-Warshall algorithm and two
 *  matrix indices (source and target), then constructs the minimum path
 *  in the adjacency list representation.
 *
 *  The path is reconstructed by following the intermediate node indices
 *  stored in the path matrix until reaching the target.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph (e.g., Ady_Mat<GT, long, ...>)
 *  @tparam Compare Comparison of the weights of the arcs, that of the
 *          floyd_all_shortest_paths() that computed `p` (default:
 *          `std::less<>`, which gives the order of `operator <`, as the
 *          default comparison of floyd_all_shortest_paths() does)
 *
 *  @param[in] p Path index matrix from Floyd-Warshall. Entry p(i,j) contains
 *               the index of the next node after i on the shortest path to j.
 *  @param[in] src_idx Source node index in the path matrix (0-based).
 *  @param[in] tgt_idx Target node index in the path matrix (0-based).
 *  @param[out] path The reconstructed minimum path in the adjacency list graph.
 *                   The path is cleared and rebuilt from scratch.
 *
 *  Between two consecutive nodes, if the arcs give their weights by
 *  `get_info().get_distance()`, the path takes the lightest of the
 *  parallel arcs, by `Compare`: with the comparison that
 *  floyd_all_shortest_paths() used, the arc that the distance counts.
 *  Otherwise, it takes the first arc that it finds.
 *
 *  @pre The path matrix `p` must have been computed by floyd_all_shortest_paths().
 *  @pre `src_idx` and `tgt_idx` must be valid indices (0 <= idx < n).
 *  @pre A path must exist between `src_idx` and `tgt_idx` (no infinite distance).
 *
 *  @warning Results are undefined if:
 *           - The path matrix is invalid or not properly computed
 *           - There is no path between source and target
 *           - The indices are out of range
 *
 *  @throw std::bad_alloc If insufficient memory to build the path.
 *  @throw std::invalid_argument If two consecutive nodes of the path
 *         have no arc between them, which the matrix of
 *         floyd_all_shortest_paths() does not give.
 *
 *  @par Complexity
 *  - Time: O(k + a), where k is the number of nodes in the path and a
 *    the number of arcs of the nodes whose arcs it searches
 *  - Space: O(k) for storing the path
 *
 *  @see floyd_all_shortest_paths()
 *  @see find_min_path(Mat&, typename Mat::Node*, typename Mat::Node*, Path<typename Mat::Graph_Type>&)
 *  @see latex_floyd.H For floyd_all_shortest_paths(), which computes the matrix
 *  @ingroup Graphs
 */
template <class Mat, class Compare = std::less<>>
void find_min_path(Mat & p, long src_idx, long tgt_idx,
                   Path<typename Mat::Graph_Type> & path)
{
  using GT = typename Mat::Graph_Type;
  GT & g = p.get_list_graph();
  
  // Handle trivial case: source equals target
  if (src_idx == tgt_idx)
    {
      path.set_graph(g, p(src_idx));
      return;
    }
  
  // Initialize path with source node
  typename GT::Node * src = p(src_idx);
  path.set_graph(g, src);
  
  // Follow path matrix to reconstruct the path
  for (long i = src_idx, j = p(i, tgt_idx); ; i = j, j = p(i, tgt_idx))
    {
      mat_path_detail::append_node<GT, Compare>(g, path, p(j));
      if (j == tgt_idx)
        break;
    }
}

/** \overload
 *
 *  Build a minimum path using node pointers instead of indices.
 *
 *  Convenience overload that accepts node pointers and converts them to
 *  matrix indices before calling the index-based version.
 *
 *  @see find_min_path(Mat&, long, long, Path<typename Mat::Graph_Type>&)
 *  @ingroup Graphs
 */
template <class Mat, class Compare = std::less<>>
void find_min_path(Mat & p, 
                   typename Mat::Node * src_node, 
                   typename Mat::Node * tgt_node,
                   Path<typename Mat::Graph_Type> & path)
{
  find_min_path<Mat, Compare>(p, p(src_node), p(tgt_node), path);
}

/** @brief Functor wrapper for find_min_path.
 *
 *  Provides a callable object interface for path reconstruction,
 *  useful for generic algorithms and functional programming patterns.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph
 *  @tparam Compare Comparison of the weights of the arcs, as in
 *          find_min_path() (default: `std::less<>`)
 *
 *  @par Example
 *  @code{.cpp}
 *  Find_Min_Path<Ady_Mat<Graph, long>> finder;
 *  Path<Graph> path(graph);
 *  finder(path_matrix, 0L, 5L, path);
 *  @endcode
 *
 *  @ingroup Graphs
 */
template <class Mat, class Compare = std::less<>>
class Find_Min_Path
{
public:
  /** @brief Reconstruct path using node pointers.
   *  @see find_min_path(Mat&, typename Mat::Node*, typename Mat::Node*, Path<typename Mat::Graph_Type>&)
   */
  void operator()(Mat & p, 
                  typename Mat::Node * src_node, 
                  typename Mat::Node * tgt_node,
                  Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path<Mat, Compare>(p, src_node, tgt_node, path);
  }

  /** @brief Reconstruct path using matrix indices.
   *  @see find_min_path(Mat&, long, long, Path<typename Mat::Graph_Type>&)
   */
  void operator()(Mat & p, 
                  long src_index, long tgt_index,
                  Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path<Mat, Compare>(p, src_index, tgt_index, path);
  }
};

} // namespace Aleph

// Global namespace compatibility
using Aleph::find_min_path;
using Aleph::Find_Min_Path;

#endif // MAT_PATH_H
