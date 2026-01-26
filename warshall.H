
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


/** @file warshall.H
 *  @brief Warshall's algorithm for transitive closure.
 *
 *  Computes the transitive closure of a directed graph: determines
 *  reachability between all pairs of vertices.
 *
 *  ## Features
 *  - Boolean reachability matrix
 *  - Works on adjacency matrix representation
 *
 *  ## Complexity: O(V³)
 *
 *  @see Floyd_Warshall.H All-pairs shortest paths (weighted)
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef WARSHALL_H
# define WARSHALL_H

# include <tpl_graph.H>
# include <tpl_matgraph.H>

namespace Aleph {

    /**
        @brief Computes the transitive closure of a graph using Warshall's algorithm.

        This function computes the transitive closure of graph @p g by means of
        Warshall's algorithm. The result is stored in the bit matrix @p mat.

        Each entry @c mat(i,j) indicates whether there exists a path from the
        source node with index @c i to the destination node with index @c j.
        A value of 0 indicates there is no path; a value of 1 indicates there
        exists at least one path.

        The procedure uses two bit matrices: an internal temporary one
        (freed when the procedure ends) and the output matrix @p mat.

        @param[in] g The graph represented by a List_Graph variant.
        @param[out] mat Bit matrix where the result is stored.
        @throw std::bad_alloc if there is not enough memory.
        @see List_Graph Bit_Mat_Graph

        @ingroup Graphs
     */
    template <class GT, class SA = Dft_Show_Arc<GT> >
void warshall_compute_transitive_clausure(GT &                    g, 
                                          Bit_Mat_Graph<GT, SA> & mat)
{
  Bit_Mat_Graph<GT, SA> mat_prev(g);
  if (mat.get_list_graph() != &g)
    mat.set_list_graph(g);
  const size_t & n = mat.get_num_nodes();
  for (int k = 0; k < n; k++) 
    {
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
          mat(i, j) = mat_prev(i, j) or
                      (mat_prev(i, k) and mat_prev(k, j));

      mat_prev = mat;
    }
}

    /**
        @brief Computes the transitive closure of a graph using Warshall's algorithm.

        This functor class computes the transitive closure of a graph using
        Warshall's algorithm. The result is stored in a bit matrix.

        Each entry @c (i,j) in the matrix indicates whether there exists a
        path from the source node with index @c i to the destination node with
        index @c j.

        @see List_Graph Bit_Mat_Graph

        @ingroup Graphs
     */
    template <class GT, class SA = Dft_Show_Arc<GT> >
class Warshall_Compute_Transitive_Clausure
{ 
public:

  /**
      @brief Invokes the computation of the transitive closure of a graph.

      The procedure uses two bit matrices: an internal temporary one
      (freed when the procedure ends) and the output matrix @p mat.

      @param[in] g The graph represented by a List_Graph variant.
      @param[out] mat Bit matrix where the result is stored.
      @throw std::bad_alloc if there is not enough memory.
   */
  void operator () (GT & g, Bit_Mat_Graph<GT> & mat) const
  {
    warshall_compute_transitive_clausure <GT, SA> (g, mat);
  }
};

} // end namespace Aleph
# endif // WARSHALL_H

