
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


/** @file latex_floyd.H
 *  @brief Floyd-Warshall algorithm with LaTeX output generation.
 *
 *  This file provides the Floyd-Warshall all-pairs shortest path algorithm
 *  with optional LaTeX output that shows the distance and path matrices
 *  at each iteration step.
 *
 *  ## Algorithm Overview
 *
 *  Floyd-Warshall computes shortest paths between all pairs of vertices
 *  in O(V³) time and O(V²) space. It works with:
 *  - Directed and undirected graphs
 *  - Positive and negative edge weights (no negative cycles)
 *
 *  ## Key Components
 *
 *  - `floyd_all_shortest_paths()`: Core algorithm
 *  - `floyd_all_shortest_paths_latex()`: Algorithm with LaTeX step-by-step output
 *  - `find_min_path()`: Path reconstruction from the path matrix
 *
 *  ## Arc Type Requirements
 *
 *  The arc type `GT::Arc_Type` must provide:
 *  - Type `Distance_Type` (e.g., int, double)
 *  - Static constant `Max_Distance` (infinity value)
 *  - Static constant `Zero_Distance` (identity for addition)
 *  - Method `get_distance()` returning the arc weight
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Create distance and path matrices
 *  Ady_Mat<Graph, double> dist(g);
 *  Ady_Mat<Graph, long> path(g);
 *
 *  // Compute all shortest paths
 *  floyd_all_shortest_paths(g, dist, path);
 *
 *  // Reconstruct a specific path
 *  Path<Graph> p;
 *  find_min_path(path, src_idx, tgt_idx, p);
 *  ```
 *
 *  @see tpl_matgraph.H For adjacency matrix representation
 *  @see mat_latex.H For LaTeX matrix formatting
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef LATEX_FLOYD_H
#define LATEX_FLOYD_H

#include <ahFunction.H>
#include <tpl_matgraph.H>
#include <mat_latex.H>

namespace Aleph
{

/** @brief Matrix initialization functor for Floyd-Warshall.
 *
 *  Initializes the distance matrix with:
 *  - Zero on the diagonal
 *  - Arc weight for existing edges
 *  - Max_Distance (infinity) for non-existing edges
 *
 *  Also initializes the path matrix for path reconstruction.
 *
 *  @tparam AM Adjacency matrix type
 */
template <class AM>
struct Initialize_Dist
{
  typedef typename AM::Graph_Type::Arc_Type Arc_Type;

  typedef typename AM::Graph_Type GT;

  typedef typename GT::Node Node;

  typedef typename GT::Arc Arc;

  typedef typename AM::Arc_Type::Distance_Type Distance_Type;

  void operator () (AM &            mat,
		    Node *          src,
		    Node *          tgt,
		    const long &    i,
		    const long &    j,
		    Distance_Type & entry,
		    void *          p)
  {
    Ady_Mat<typename AM::Graph_Type, long> & path = 
      * reinterpret_cast<Ady_Mat<typename AM::Graph_Type, long> *>(p);

    if (i == j)
      {
	entry = AM::Graph_Type::Arc_Type::Zero_Distance;
	path(i, j) = j;

	return;
      }

    GT & g = mat.get_list_graph();

    Arc * arc = search_arc(g, src, tgt);

    if (arc == nullptr)
      {
	entry = AM::Graph_Type::Arc_Type::Max_Distance;
	return;
      }

    entry = arc->get_info().get_distance();

    path(i, j) = j;
  }
};

/** @brief Compute all-pairs shortest paths using Floyd-Warshall algorithm.
 *
 *  Computes the shortest path between every pair of vertices in the graph.
 *  The algorithm runs in O(V³) time and O(V²) space.
 *
 *  @tparam GT Graph type (must have Arc_Type with Distance_Type, Max_Distance, Zero_Distance)
 *  @tparam Compare Comparison functor (default: less)
 *  @tparam Plus Addition functor for distances (default: plus)
 *
 *  @param g The input graph
 *  @param[out] dist Distance matrix - dist(i,j) = shortest distance from i to j
 *  @param[out] path Path matrix - path(i,j) = next node on shortest path from i to j
 *
 *  @note The path matrix can be used with find_min_path() to reconstruct paths.
 *  @warning Does not detect negative cycles.
 *
 *  @ingroup Graphs
 */
template <class GT, class Compare, class Plus> 
void floyd_all_shortest_paths(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path)
{
  typedef Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> Dist_Mat;

  typedef typename GT::Arc_Type::Distance_Type Dist_Type;

  dist.
    template operate_all_arcs_matrix<Initialize_Dist<Dist_Mat> > (&path);

  const Dist_Type & max = GT::Arc_Type::Max_Distance;

  const long & n = g.get_num_nodes();

  for (int i = 0; i < n; ++i)
    for (int s = 0; s < n; ++s)
      if (dist(s, i) < max)
	for (int t = 0; t < n; ++t)
	  {
	    Dist_Type new_dist = Plus () (dist(s, i), dist(i, t));

	    if (Compare () (new_dist, dist(s, t)))
	      {
		path(s, t) = path(s, i);
		dist(s, t) = new_dist;
	      }
	  }
}

/// @brief Overload using default comparison (less) and addition (plus).
/// @see floyd_all_shortest_paths()
template <class GT> 
void floyd_all_shortest_paths(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path)
{
  using Dist_T = typename GT::Arc_Type::Distance_Type;
  floyd_all_shortest_paths<GT, Aleph::less<Dist_T>, Aleph::plus<Dist_T>>(g, dist, path);
}

/** \overload
 *
 *  Variant that accepts const indices.
 *
 *  See mat_path.H for the primary definition and full documentation.
 *  This overload is provided for compatibility with const index values.
 *
 *  @see mat_path.H For the primary definition with non-const indices
 *  @ingroup Graphs
 *  @internal This is an overload; parameters are documented in mat_path.H
 */
template <class Mat>
void find_min_path(Mat &                           p, 
                   const long                      src_index,
                   const long                      tgt_index,
                   Path<typename Mat::Graph_Type>& path)
{
  using GT = typename Mat::Graph_Type;
  using Node = typename GT::Node;

  GT & g = p.get_list_graph();
  Node * src = p(src_index);
  path.set_graph(g, src);
  
  // NOTE: Initialize j properly to avoid undefined behavior
  for (long i = src_index, j = p(i, tgt_index); i != tgt_index; i = j, j = p(i, tgt_index))
  {
    Node * next = p(j);
    path.append(next);
  }
}

/** \overload
 *
 *  Reconstruct shortest path from path matrix using node pointers.
 *
 *  Convenience overload that accepts node pointers instead of indices.
 *
 *  @tparam Mat Path matrix type
 *
 *  @param[in] p The path matrix from Floyd-Warshall
 *  @param[in] src_node Source node pointer
 *  @param[in] tgt_node Target node pointer
 *  @param[out] path The reconstructed path
 *
 *  @see find_min_path(Mat&, const long, const long, Path<GT>&)
 *  @ingroup Graphs
 */
template <class Mat>
void find_min_path(Mat &                           p, 
                   typename Mat::Node *            src_node,
                   typename Mat::Node *            tgt_node,
                   Path<typename Mat::Graph_Type>& path)
{
  const long src_index = p(src_node);
  const long tgt_index = p(tgt_node);
  find_min_path(p, src_index, tgt_index, path);
}


/** @brief Floyd-Warshall algorithm with LaTeX step-by-step output.
 *
 *  Computes all-pairs shortest paths while generating LaTeX output
 *  showing the distance and path matrices at each iteration step.
 *  Useful for educational purposes and algorithm visualization.
 *
 *  ## LaTeX Output Format
 *
 *  Generates figures with side-by-side D_k and P_k matrices for k = 0..n.
 *  Requires LaTeX packages: float (for [H] placement).
 *
 *  @tparam GT Graph type
 *  @tparam Compare Comparison functor
 *  @tparam Plus Addition functor
 *  @tparam P_i Row/column index formatter
 *  @tparam P_ij Path matrix entry formatter
 *  @tparam D_ij Distance matrix entry formatter
 *
 *  @param g The input graph
 *  @param[out] dist Distance matrix
 *  @param[out] path Path matrix
 *  @param[out] output Output stream for LaTeX code
 *
 *  @see floyd_all_shortest_paths() For algorithm without LaTeX output
 *
 *  @ingroup Graphs
 */
template <class GT, class Compare, class Plus, 
          template <class> class P_i,   // Row/column index format for dist and path
          template <class> class P_ij,  // Path matrix entry format
          template <class> class D_ij>  // Distance matrix entry format
void floyd_all_shortest_paths_latex(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path,
    std::ofstream &                                     output)
{
  typedef Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> Dist_Mat;
  typedef Ady_Mat<GT, long>                                 Path_Mat;

  typedef typename GT::Arc_Type::Distance_Type Dist_Type;

  dist.
    template operate_all_arcs_matrix<Initialize_Dist<Dist_Mat> > (&path);

  const Dist_Type & max = GT::Arc_Type::Max_Distance;

  const long & n = g.get_num_nodes();

  output << "\\begin{figure}[H]{\\tiny " << std::endl
	 << "\\begin{tabular}{ll}" << std::endl
	 << "\\begin{tabular}{ll}" << std::endl;
  mat_to_latex <Dist_Mat, P_i<Dist_Mat>, P_i<Dist_Mat>, D_ij<Dist_Mat> >
    (dist, n, n, output, "\\hskip -5mm $D_0=$", "\\\\ ");
  output << "\\end{tabular}" << std::endl
	 << " & \\begin{tabular}{ll}" << std::endl;
  mat_to_latex <Path_Mat, P_i<Path_Mat>, P_i<Path_Mat>, P_ij<Path_Mat> >
    (path, n, n, output, "\\hskip -7mm $P_0=$", "\\\\ ");
  output << "\\end{tabular}" << std::endl 
    	 << "\\end{tabular}" << std::endl
	 << "}\\end{figure}" << std::endl;

  for (int i = 0; i < n; ++i)
    {
      for (int s = 0; s < n; ++s)
	if (dist(s, i) < max)
	  for (int t = 0; t < n; ++t)
	    {
	      Dist_Type new_dist = Plus() (dist(s, i), dist(i, t));

	      if (Compare () (new_dist, dist(s, t)))
		{
		  path(s, t) = path(s, i);
		  dist(s, t) = new_dist;
		}
	    }
      char buf[256];

      snprintf(buf, 256, "\\hskip -5mm $D_%d=$ ", i + 1);

      output << "\\begin{figure}[H]{\\tiny " << std::endl
 	     << "\\begin{tabular}{ll}" << std::endl
 	     << "\\begin{tabular}{ll}" << std::endl;
      mat_to_latex <Dist_Mat, P_i<Dist_Mat>, P_i<Dist_Mat>, D_ij<Dist_Mat> >
	(dist, n, n, output, buf, "\\\\ ");
      output << "\\end{tabular}" << std::endl
	     << " & \\begin{tabular}{ll}" << std::endl;

      snprintf(buf, 256, "\\hskip -7mm $P_%d=$ ", i + 1);

      mat_to_latex <Path_Mat, P_i<Path_Mat>, P_i<Path_Mat>, P_ij<Path_Mat> >
	(path, n, n, output, buf, "\\\\");
      output << "\\end{tabular}" << std::endl 
	     << "\\end{tabular}" << std::endl
	     << "}\\end{figure}" << std::endl;
    }
}


/// @brief Overload using default comparison (less) and addition (plus).
/// @see floyd_all_shortest_paths_latex()
template <class GT, 
          template <class> class P_i,   // Row/column index format
          template <class> class P_ij,  // Path matrix entry format
          template <class> class D_ij>  // Distance matrix entry format
void floyd_all_shortest_paths_latex(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path,
    std::ofstream &                                     output)
{
  using Dist_T = typename GT::Arc_Type::Distance_Type;
  floyd_all_shortest_paths_latex<GT, Aleph::less<Dist_T>, Aleph::plus<Dist_T>, 
                                  P_i, P_ij, D_ij>(g, dist, path, output);
}

} // end namespace Aleph

#endif // LATEX_FLOYD_H
