
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
# ifndef TPL_TEST_CYCLE_H
# define TPL_TEST_CYCLE_H

# include <tpl_graph.H>

namespace Aleph {


  /** Determina si existe un ciclo a partir de un nodo.

      La clase Test_For_Cycle explora en profundidad el grafo g a partir de
      un nodo start_node y verifica si existe algún ciclo a partir de él. 

      El bit test_cycle es usado e iniciado al principio del algoritmo
      para marcar los nodos y arcos visitados.

      @ingroup Grafos
  */
      template <class GT, class SA = Dft_Show_Arc<GT>> 
class Test_For_Cycle
{
  typename GT::Node * src = nullptr;
  SA sa;

  bool test_cycle(typename GT::Node * curr)
  {
    if (src == curr) 
      return true; // ciclo detectado!

    if (IS_NODE_VISITED(curr, Test_Cycle)) 
      return false;

    NODE_BITS(curr).set_bit(Test_Cycle, true); // marque nodo

      // buscar caminos desde current_node a ver si llega a src_node 
    for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true); // marque arco

        if (test_cycle(it.get_tgt_node()))
          return true; // ciclo encontrado desde el arco actual 
      }

        // En este punto se han explorado caminos desde curr_node
        // sin encontrar src_node ==> no existe ciclo por curr_node  
    return false; 
  }  

  bool test_cycle(GT & g, typename GT::Node * s)
  {
    src = s;

    g.reset_bit_nodes(Test_Cycle); // reiniciar Test_Cycle para nodos
    g.reset_bit_arcs(Test_Cycle);  // reiniciar Test_Cycle para arcos

      // explorar recursivamente por arcos adyacentes a src_node
    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true); // pintar arco

        if (test_cycle(it.get_tgt_node()))
          return true; // ciclo detectado
      }

        // Se han explorado todos los caminos desde src_node 
        // sin encontrar de nuevo a src_node ==> no hay ciclo 
    return false;
  }

public:

  Test_For_Cycle(SA __sa = SA()) : sa(__sa) { /* empty */ }

  /** Invoca a la prueba de existencia de ciclo
      
      @note La rutina sólo verifica existencia de ciclo, no dice nada en 
      absoluto sobre la composición del ciclo.
      
      @param[in] g el grafo a verificar.
      @param[in] src_node el nodo que se quiere verificar.
      @return true si existe un ciclo; false de lo contrario.
  */
  bool operator () (GT & g, typename GT::Node * src) 
  {
    return this->test_cycle(g, src);
  }
};

} // end namespace Aleph


# endif //  TPL_TEST_CYCLE_H
