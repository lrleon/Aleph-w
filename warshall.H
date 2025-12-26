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

        @ingroup Grafos
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

        @ingroup Grafos
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

