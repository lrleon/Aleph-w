
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


/** @file tpl_test_path.H
 *  @brief Path existence testing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
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
         @ingroup Graphs
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
