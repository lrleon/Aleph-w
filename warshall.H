
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

    /** Cálculo de la clausura transitiva de una matriz de adyacencia.

        Esta función calcula la clausura transitiva del grafo g mediante
        el algoritmo de Warshall. El resultado se almacena en la matriz
        de bits mat.

        Cada entrada mat(i,j) indica existencia de un camino entre el
        nodo origen con índice i y el nodo destino con índice j. Un
        valor cero indica que no hay ningún camino; un valor 1 que
        existe al menos un camino.

        El procedimiento utiliza dos matrices de bits; una de uso
        interno que es liberada al término del procedimiento y la propia
        matriz mat.
        
        @param[in] g el grafo representado mediante una variante de
        List_Graph.  
        @param[out] mat matriz de bits donde se coloca el resultado.
        @throw bad_alloc si no hay suficiente memoria. 
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

    /** Cálculo de la clausura transitiva de una matriz de adyacencia.

        Esta clase calcula la clausura transitiva de un grafo mediante
        el algoritmo de Warshall. El resultado se almacena en una matriz
        de bits.

        Cada entrada(i,j) en la matriz indica existencia de un camino
        entre el nodo origen con índice i y el nodo destino con índice
        j. Un valor cero indica que no hay ningún camino; un valor 1 que
        existe al menos un camino.

        @see List_Graph Bit_Mat_Graph

        @ingroup Grafos
     */
    template <class GT, class SA = Dft_Show_Arc<GT> >
class Warshall_Compute_Transitive_Clausure
{ 
public:

  /** Invoca al cálculo de la clausura transitiva de un grafo.

      El procedimiento utiliza dos matrices de bits; una de uso
      interno que es liberada al término del procedimiento y la propia
      matriz mat.
        
      @param[in] g el grafo representado mediante una variante de
      List_Graph.  
      @param[out] mat matriz de bits donde se coloca el resultado.
      @throw bad_alloc si no hay suficiente memoria. 
   */
  void operator () (GT & g, Bit_Mat_Graph<GT> & mat) const
  {
    warshall_compute_transitive_clausure <GT, SA> (g, mat);
  }
};

} // end namespace Aleph
# endif // WARSHALL_H

