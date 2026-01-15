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

/** @file tpl_test_connectivity.H
 *  @brief Graph connectivity testing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TEST_CONNECTIVITY_H
# define TPL_TEST_CONNECTIVITY_H

# include <tpl_graph_utils.H>
 # include <ah-errors.H>


       /** Determina si un grafo g es conexo.

           Esta clase realiza una prueba de conectividad del grafo g. La
           prueba apela a un recorrido en profundidad.

           La clase verifica la
           cantidad de arcos. Si esta cantidad es menor que el número de
           nodos, entonces el grafo se considera inconexo.

           La función toma dos parámetros tipo:
           -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
           -# SA: clase que se encarga de mostrar el arco. Internamente, la
           función emplea el iterador filtro Node_Arc_Iterator (basado
           en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
           el la clase que determina si el arco debe o no mostrarse al
           recorrido. 

           @ingroup Graphs
           @see depth_first_traversal() is_reachable()
        */
    template <class GT, class SA = Dft_Show_Arc<GT> > 
class Test_Connectivity
{
public:

        /** Invoca a la prueba de conectividad.

            Esta versión se emplea cuando se requiera que el filtro de arcos
            mantenga un estado.

            @param[in] g el grafo o digrafo a verificar.
            @param[in] sa el filtro de arcos del grafo (default: Dft_Show_Arc<GT>)
            @return true si el grafo es conexo; false de lo contrario.
            @note Debido a la prueba con el número de arcos, esta función
            es incorrecta para multigrafos.
            @throw domain_error si la rutina es invocada sobre un digrafo.
         */
  bool operator () (GT & g, SA && sa = SA()) const
  {
    ah_domain_error_if(g.is_digraph())
      << "test_connectivity() does not work on digraphs";

    if (g.get_num_arcs() < g.get_num_nodes() - 1)
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);

    return traversal(g) == g.get_num_nodes();
  }

/** Invoca a la prueba de conectividad.

    Esta versión se emplea cuando se requiera que el filtro de arcos
    mantenga un estado.

    @param[in] g el grafo o digrafo a verificar.
    @param[in] sa el filtro de arcos del grafo (por referencia)
    @return true si el grafo es conexo; false de lo contrario.
    @note Debido a la prueba con el número de arcos, esta función
    es incorrecta para multigrafos.
    @throw domain_error si la rutina es invocada sobre un digrafo.
 */
  bool operator () (GT & g, SA & sa) const
  {
    ah_domain_error_if(g.is_digraph())
      << "test_connectivity() does not work on digraphs";

    if (g.get_num_arcs() < g.get_num_nodes() - 1) 
      return false;

    Depth_First_Traversal <GT, Default_Visit_Op<GT>, SA> traversal(sa);
 
    return traversal(g) == g.get_num_nodes();
  }
};



# endif // TPL_TEST_CONNECTIVITY_H
