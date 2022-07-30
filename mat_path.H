
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef MAT_PATH_H
# define MAT_PATH_H

# include <tpl_matgraph.H>

    template <class Mat>
void find_min_path(Mat & p, const long & src_index, const long & tgt_index,
                   Path<typename Mat::Graph_Type> & path);
    /** Construye un camino mínimo desde un nodo a otro a partir de una
        matriz de caminos calculada por el algoritmo de Floyd-Warshall.

        Este procedimiento toma una matriz p que contiene los índices de
        caminos mínimos calculados por el algoritmo de Floyd-Warshall,
        dos punteros correspondientes a los nodos origen y destino
        respectivamente en la representación con listas de adyacencia
        usada cuando se invocó a floyd_all_shortest_paths(), y calcula
        el camino mínimo.

        Este procedimiento está concebido para llamarse después de
        ejecutar floyd_all_shortest_paths(), el cual calcula la matriz
        p.

        Resultados son completamente indeterminados si la matriz p es
        inválida. 

        @param[in] p matriz de índices de caminos mínimos.
        @param[in] src_node puntero al nodo origen dentro de la
        representación con listas de adyacencia.
        @param[in] tgt_node puntero al nodo destino dentro de la
        representación con listas de adyacencia.
        @param[out] path el camino mínimo en el grafo representado con
        listas de adyacencia usado cuando se ejecutó
        floyd_all_shortest_paths().
        @throw bad_alloc si no hay suficiente memoria para construir el
        camino. 
        @see floyd_all_shortest_paths()
        @ingroup Grafos
     */
    template <class Mat>
void find_min_path(Mat & p, typename Mat::Node * src_node, 
                   typename Mat::Node * tgt_node,
                   Path<typename Mat::Graph_Type> & path)
{
  find_min_path(p, p(src_node), p(tgt_node, path));
}

    template <class Mat>
class Find_Min_Path
{
public:

  void operator () (Mat &                                 p, 
                    typename Mat::Node *                  src_node, 
                    typename Mat::Node *                  tgt_node,
                    Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path <Mat> (p, src_node, tgt_node, path);
  }

  void operator () (Mat &                                 p, 
                    const long & src_index, const long &  tgt_index,
                    Path<typename Mat::Graph_Type> & path) const
  {
    find_min_path <Mat> (p, src_index, tgt_index, path);
  }
};

    /** Construye un camino mínimo desde un nodo a otro a partir de una
        matriz de caminos calculada por el algoritmo de Floyd-Warshall.

        Este procedimiento toma una matriz p que contiene los índices de
        caminos mínimos calculados por el algoritmo de Floyd-Warshall,
        dos enteros i y j en la matriz correspondientes a los índices del
        nodo origen y destino respectivamente, y calcula el camino
        mínimo en la representación con listas de adyacencia usada en el
        algoritmo de Floyd-Warshall.

        Este procedimiento está concebido para llamarse después de
        ejecutar floyd_all_shortest_paths(), el cual calcula la matriz
        p.

        Resultados son completamente indeterminados si la matriz p es
        inválida. 

        Una versión sobrecargada de este método toma como parámetros
        punteros a nodos, en lugar de índices sobre la matriz.

        @param[in] p matriz de índices de caminos mínimos.
        @param[in] src_idx índice del nodo origen dentro de la matriz de
        caminos mínimos.
        @param[in] tgt_idx índice del nodo destino dentro de la matriz de
        caminos mínimos.
        @param[out] path el camino mínimo en el grafo representado con
        listas de adyacencia usado cuando se ejecutó
        floyd_all_shortest_paths().
        @throw bad_alloc si no hay suficiente memoria para construir el
        camino. 
        @see floyd_all_shortest_paths()
        @ingroup Grafos
     */
    template <class Mat> void 
find_min_path(Mat & p, const long & src_idx, const long & tgt_idx,
              Path<typename Mat::Graph_Type> & path)
{
  typedef typename Mat::Graph_Type GT;
  GT & g                  = p.get_list_graph();
  typename GT::Node * src = p(src_idx);
  path.set_graph(g, src);
  for (int i = src_idx, j; j != tgt_idx; i = j)
    {
      j = p(i, tgt_idx);
      path.append(p(j));
    }
}

# endif // MAT_PATH_H
