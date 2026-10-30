
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
 *  Of the parallel arcs between two nodes, the lightest counts. A loop
 *  does not count: the diagonal starts at `Zero_Distance`. The functions
 *  do not detect negative cycles, a negative loop among them: with one,
 *  the matrices do not hold distances.
 *
 *  ## Key Components
 *
 *  - `floyd_all_shortest_paths()`: Core algorithm
 *  - `floyd_all_shortest_paths_latex()`: Algorithm with LaTeX step-by-step output
 *  - `find_min_path()`, from `mat_path.H`, which this header includes:
 *    Path reconstruction from the path matrix
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

# include <ah-graph-concepts.H>

#include <utility>
#include <ahFunction.H>
#include <tpl_array.H>
#include <tpl_matgraph.H>
#include <mat_latex.H>
#include <mat_path.H>

namespace Aleph
{

/** @brief Matrix initialization functor for Floyd-Warshall.
 *
 *  Initializes the distance matrix with:
 *  - Zero on the diagonal, whatever loops the node has
 *  - The weight of the lightest of the arcs from the node of the row to
 *    that of the column (in a graph that is not directed, of the arcs
 *    that link them), by `Compare`; of two as light, the first one found
 *  - Max_Distance (infinity) for non-existing edges
 *
 *  Also initializes the path matrix for path reconstruction: `path(i, j)`
 *  is `j` on the diagonal and where there is an arc, and is left as it
 *  was elsewhere.
 *
 *  floyd_all_shortest_paths() and floyd_all_shortest_paths_latex() set
 *  the matrices in the same way, from one pass over the arcs instead of
 *  a search for each pair of nodes.
 *
 *  @tparam AM Adjacency matrix type
 *  @tparam Compare Comparison of distances; the lighter of two arcs is
 *          the one that it puts first (default: less)
 */
template <class AM,
          class Compare = Aleph::less<typename AM::Arc_Type::Distance_Type>>
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

    // In a graph that is not directed, the arcs of the node with fewer
    // arcs are searched, as search_arc() does
    if (not g.is_digraph() and tgt->num_arcs < src->num_arcs)
      std::swap(src, tgt);

    Arc * lightest = nullptr;
    for (typename GT::Node_Arc_Iterator it(src); it.has_curr(); it.next_ne())
      if (it.get_tgt_node_ne() == tgt and
          (lightest == nullptr or
           Compare () (it.get_current_arc_ne()->get_info().get_distance(),
                       lightest->get_info().get_distance())))
        lightest = it.get_current_arc_ne();

    if (lightest == nullptr)
      {
	entry = AM::Graph_Type::Arc_Type::Max_Distance;
	return;
      }

    entry = lightest->get_info().get_distance();

    path(i, j) = j;
  }
};

namespace latex_floyd_detail
{
  /* Sets the matrices as Initialize_Dist does, but from one pass over the
     arcs of each node instead of a search for each pair of nodes: O(n^2 +
     m log n) instead of O(n m). The first arc from i to j takes the entry,
     and a lighter one, by Compare, replaces it; seen[j] == i says that an
     arc from i to j was already found. */
  template <AlephGraph GT, class Compare>
  void initialize_matrices(
      Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
      Ady_Mat<GT, long> &                                 path)
  {
    using Dist_Type = typename GT::Arc_Type::Distance_Type;
    using Node = typename GT::Node;

    const long n = dist.get_num_nodes();
    Array<long> seen(n, -1L);
    for (long i = 0; i < n; ++i)
      {
        for (long j = 0; j < n; ++j)
          dist(i, j) = i == j ? GT::Arc_Type::Zero_Distance
                              : GT::Arc_Type::Max_Distance;
        path(i, i) = i;

        Node * src = dist(i);
        for (typename GT::Node_Arc_Iterator it(src); it.has_curr(); it.next_ne())
          {
            Node * tgt = it.get_tgt_node_ne();
            if (tgt == src)
              continue; // a loop does not count

            const long j = dist(tgt);
            const Dist_Type w = it.get_current_arc_ne()->get_info().get_distance();
            if (seen[j] != i)
              {
                seen[j] = i;
                dist(i, j) = w;
                path(i, j) = j;
              }
            else if (Compare () (w, dist(i, j)))
              dist(i, j) = w;
          }
      }
  }
} // end namespace latex_floyd_detail

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
 *  Of the parallel arcs between two nodes, the lightest counts, by
 *  `Compare`; a loop does not count. The matrices start as
 *  Initialize_Dist sets them.
 *
 *  @note The path matrix can be used with find_min_path() to reconstruct paths.
 *  @warning Does not detect negative cycles, a negative loop among them:
 *           with one, the matrices do not hold distances.
 *  @throw std::bad_alloc if there is not enough memory.
 *
 *  @ingroup Graphs
 */
template <AlephGraph GT, class Compare, class Plus> 
void floyd_all_shortest_paths(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path)
{
  typedef typename GT::Arc_Type::Distance_Type Dist_Type;

  latex_floyd_detail::initialize_matrices<GT, Compare>(dist, path);

  const Dist_Type & max = GT::Arc_Type::Max_Distance;

  const long & n = g.get_num_nodes();

  for (int i = 0; i < n; ++i)
    for (int s = 0; s < n; ++s)
      if (dist(s, i) < max)
	for (int t = 0; t < n; ++t)
	  {
            if (!(dist(i, t) < max))
              continue;

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
template <AlephGraph GT> 
void floyd_all_shortest_paths(
    GT &                                                g, 
    Ady_Mat<GT, typename GT::Arc_Type::Distance_Type> & dist,
    Ady_Mat<GT, long> &                                 path)
{
  using Dist_T = typename GT::Arc_Type::Distance_Type;
  floyd_all_shortest_paths<GT, Aleph::less<Dist_T>, Aleph::plus<Dist_T>>(g, dist, path);
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
 *  It counts the arcs as floyd_all_shortest_paths() does: of the parallel
 *  arcs between two nodes, the lightest, by `Compare`, and no loop.
 *
 *  @warning Does not detect negative cycles, a negative loop among them:
 *           with one, the matrices do not hold distances.
 *  @throw std::bad_alloc if there is not enough memory.
 *
 *  @see floyd_all_shortest_paths() For algorithm without LaTeX output
 *
 *  @ingroup Graphs
 */
template <AlephGraph GT, class Compare, class Plus, 
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

  latex_floyd_detail::initialize_matrices<GT, Compare>(dist, path);

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
              if (!(dist(i, t) < max))
                continue;

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
template <AlephGraph GT, 
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
