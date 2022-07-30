
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
# ifndef TEST_PATH_H
# define TEST_PATH_H

# include <tpl_graph.H>

namespace Aleph {


      /** verfica si existe un camino entre dos nodos.

          Test_For_Path explora en profundidad el grafo g a partir de un 
          nodo de inicio en búsqueda de un camino que depare en uno destino. 

          La clase toma dos parámetros tipo:
          -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
          -# SA: clase que se encarga de mostrar el arco. Internamente, la
          función emplea el iterador filtro Node_Arc_Iterator (basado
          en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
          el la clase que determina si el arco debe o no mostrarse al
          recorrido. 

          El bit test_path es utilizado para marcar los nodos y arcos
          visitados durante la búsqueda.

         @see find_path_depth_first() find_path_breadth_first()
         @ingroup Grafos
       */
      template <class GT, class SA = Dft_Show_Arc<GT> > 
class Test_For_Path
{
  SA sa;
  typename GT::Node * tgt = nullptr;

  bool test_path(typename GT::Node * curr)  
  {
    if (curr == tgt) 
      return true; // se alcanzó a tgt

    if (IS_NODE_VISITED(curr, Find_Path)) // ¿se visitó curr_node?
      return false; // sí, no explore

    NODE_BITS(curr).set_bit(Find_Path, true); // pintar curr_node

    // buscar recursivamente a través de arcos de curr 
    for (Node_Arc_Iterator<GT, SA> i(curr, sa); i.has_curr(); i.next_ne())
      { 
        typename GT::Arc * arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Find_Path)) 
          continue; 

        ARC_BITS(arc).set_bit(Find_Path, true); // pintar arco
        if (test_path(i.get_tgt_node())) 
          return true;
      }

    // todos los arcos adyacentes de curr_node explorados sin
    // encontrar a end_node ==> no existe camino por curr_node
    return false;
  }

  bool test_path(const GT & g, typename GT::Node * src, typename GT::Node * dest)
  {      // si el grafo es conexo ==> existe camino
    if (not g.is_digraph() and g.get_num_arcs() >= g.get_num_nodes()) 
      return true;

    g.reset_bit_nodes(Find_Path); 
    g.reset_bit_arcs(Find_Path);

    tgt = dest;

        // buscar recursivamente por arcos adyacentes a src
    for (Node_Arc_Iterator<GT, SA> i(src, sa); i.has_curr(); i.next_ne())
      {
        typename GT::Arc * arc = i.get_current_arc_ne();
        ARC_BITS(arc).set_bit(Find_Path, true); // marcar arco
        if (test_path(i.get_tgt_node())) 
          return true; 
      }

    // todos los arcos de start_node han sido explorados sin 
    // encontrar camino hasta end_node ==> no existe camino
    return false;
  }

public:

  Test_For_Path(SA __sa = SA()) : sa(__sa) { /* empty */ }

    /** Invoca a la prueba de existencia de camino entre dos nodos.

         @param[in] g el grafo a buscar camino.
         @param[in] start_node puntero al nodo origen del camino.
         @param[in] end_node puntero a nodo destino del camino.
         @return true si existe un camino entre start_node y end_node.
    */
  bool operator () (const GT& g, typename GT::Node * start_node, 
		    typename GT::Node * end_node) 
  {
    return test_path(g, start_node, end_node);
  }
};



} // end namespace Aleph

# endif // TEST_PAT_H
