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
 *  #include <mat_path.H>
 *  #include <latex_floyd.H>
 *
 *  // Create adjacency matrix and run Floyd-Warshall
 *  Ady_Mat<Graph, long, Max_Dist_Val> dist(graph);
 *  Ady_Mat<Graph, long, Max_Dist_Val> path(graph);
 *  floyd_all_shortest_paths(dist, path);
 *
 *  // Reconstruct path from node 0 to node 5
 *  Path<Graph> min_path;
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
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

#ifndef MAT_PATH_H
#define MAT_PATH_H

#include <tpl_matgraph.H>

namespace Aleph
{

/** @brief Build a minimum path from Floyd-Warshall path matrix using indices.
 *
 *  Takes a path matrix p computed by Floyd-Warshall algorithm and two
 *  matrix indices (source and target), then constructs the minimum path
 *  in the adjacency list representation.
 *
 *  The path is reconstructed by following the intermediate node indices
 *  stored in the path matrix until reaching the target.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph (e.g., Ady_Mat<GT, long, ...>)
 *
 *  @param[in] p Path index matrix from Floyd-Warshall. Entry p(i,j) contains
 *               the index of the next node after i on the shortest path to j.
 *  @param[in] src_idx Source node index in the path matrix (0-based)
 *  @param[in] tgt_idx Target node index in the path matrix (0-based)
 *  @param[out] path The reconstructed minimum path in the adjacency list graph.
 *                   The path is cleared and rebuilt from scratch.
 *
 *  @pre The path matrix p must have been computed by floyd_all_shortest_paths().
 *  @pre src_idx and tgt_idx must be valid indices (0 <= idx < n).
 *  @pre A path must exist between src_idx and tgt_idx (no infinite distance).
 *
 *  @warning Results are undefined if:
 *           - The path matrix is invalid or not properly computed
 *           - There is no path between source and target
 *           - The indices are out of range
 *
 *  @throw std::bad_alloc If insufficient memory to build the path.
 *
 *  @par Complexity
 *  - Time: O(k) where k is the number of nodes in the path
 *  - Space: O(k) for storing the path
 *
 *  @see floyd_all_shortest_paths()
 *  @see find_min_path(Mat&, typename Mat::Node*, typename Mat::Node*, Path<typename Mat::Graph_Type>&)
 *  @ingroup Grafos
 */
template <class Mat>
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
      path.append(p(j));
      if (j == tgt_idx)
        break;
    }
}

/** @brief Build a minimum path from Floyd-Warshall path matrix using node pointers.
 *
 *  Takes a path matrix p computed by Floyd-Warshall algorithm and two node
 *  pointers (source and target) from the adjacency list representation,
 *  then constructs the minimum path.
 *
 *  This is a convenience overload that converts node pointers to matrix
 *  indices and delegates to the index-based version.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph
 *
 *  @param[in] p Path index matrix from Floyd-Warshall.
 *  @param[in] src_node Source node pointer in adjacency list representation.
 *  @param[in] tgt_node Target node pointer in adjacency list representation.
 *  @param[out] path The minimum path in the adjacency list graph.
 *
 *  @pre Both src_node and tgt_node must be valid nodes in the graph.
 *  @pre The path matrix p must have been computed by floyd_all_shortest_paths().
 *
 *  @throw std::bad_alloc If insufficient memory to build the path.
 *
 *  @see find_min_path(Mat&, long, long, Path<typename Mat::Graph_Type>&)
 *  @see floyd_all_shortest_paths()
 *  @ingroup Grafos
 */
template <class Mat>
void find_min_path(Mat & p, 
                   typename Mat::Node * src_node, 
                   typename Mat::Node * tgt_node,
                   Path<typename Mat::Graph_Type> & path)
{
  find_min_path(p, p(src_node), p(tgt_node), path);
}

/** @brief Functor wrapper for find_min_path.
 *
 *  Provides a callable object interface for path reconstruction,
 *  useful for generic algorithms and functional programming patterns.
 *
 *  @tparam Mat Matrix type from Map_Matrix_Graph
 *
 *  @par Example
 *  @code{.cpp}
 *  Find_Min_Path<Ady_Mat<Graph, long, Max_Val>> finder;
 *  Path<Graph> path;
 *  finder(path_matrix, 0L, 5L, path);
 *  @endcode
 *
 *  @ingroup Grafos
 */
template <class Mat>
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
    find_min_path<Mat>(p, src_node, tgt_node, path);
  }

  /** @brief Reconstruct path using matrix indices.
   *  @see find_min_path(Mat&, long, long, Path<typename Mat::Graph_Type>&)
   */
  void operator()(Mat & p, 
                  long src_index, long tgt_index,
                  Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path<Mat>(p, src_index, tgt_index, path);
  }
};

} // namespace Aleph

// Global namespace compatibility
using Aleph::find_min_path;
using Aleph::Find_Min_Path;

#endif // MAT_PATH_H
