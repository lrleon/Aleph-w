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

/** @file tpl_test_acyclique.H
 *  @brief DAG (acyclic) graph testing.
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TEST_ACYCLIQUE_H
# define TPL_TEST_ACYCLIQUE_H

# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph {

      /** Determina si un grafo es o no acíclico (no contiene ciclos).

          Is_Graph_Acyclique explora en profundidad en grafo g, a 
          partir de un nodo y verifica si el grafo es acíclico; es 
          decir, que no contenga ningún ciclo.

          La clase toma dos parámetros tipo:
          -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
          -# SA: clase que se encarga de mostrar el arco. Internamente, la
          función emplea el iterador filtro Node_Arc_Iterator (basado
          en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
          el la clase que determina si el arco debe o no mostrarse al
          recorrido. 

          La clase usa el bit is_acyclique y lo reinicia al principio del 
          algoritmo para marcar los nodos y arcos ya visitados.

          @ingroup Grafos
       */
      template <class GT, class SA = Dft_Show_Arc<GT>> 
class Is_Graph_Acyclique
{
  SA & sa;

  bool is_acyclique(typename GT::Node * curr)
  {
    if (IS_NODE_VISITED(curr, Test_Cycle)) 
      return false;

    NODE_BITS(curr).set_bit(Test_Cycle, true); // marcar nodo

    for (Node_Arc_Iterator<GT, SA> i(curr, sa); i.has_curr(); i.next_ne())
      {
        typename GT::Arc * arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue; 

        ARC_BITS(arc).set_bit(Test_Cycle, true); 

        if (not is_acyclique(i.get_tgt_node())) 
          return false;
      }

    // todos los arcos recorridos sin encontrar ciclo ==>
    // el grafo es acíclico pasando por curr_node
    return true; 
  }

  bool is_acyclique(GT & g, size_t num_arcs)
  {
    ah_domain_error_if(g.is_digraph())
      << "is_graph_acyclique() does not work for digraps";

    if (num_arcs >= g.get_num_nodes()) 
      return false; 

    g.reset_bit_arcs(Test_Cycle);
    g.reset_bit_nodes(Test_Cycle);

    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne()) 
    {
      typename GT::Node * curr = it.get_current_node_ne();
      if (IS_NODE_VISITED(curr, Test_Cycle)) 
	continue; 

      if (not is_acyclique(curr)) 
	return false;
    }

  return true;
}

public:
  
  Is_Graph_Acyclique(SA && __sa = SA()) : sa(__sa) { /* empty */ }

  Is_Graph_Acyclique(SA & __sa) : sa(__sa) { /* empty */ }

      /** Invoca a la prueba de aciclicidad.

        Si g es un grafo (no un digrafo) entonces el algoritmo compara la 
        cantidad de arcos con la de nodos. Si el grafo tiene menos arcos que 
        nodos, entonces se concluye que el grafo es acíclico. 
        Consecuentemente, esta técnica no opera para multigrafos.
          
        @param[in] g el grafo a probar.
	@param num_arcs cantidad de arcos que tiene g. Use este valor si
	está empleando iteradores filtros sobre arcos pintados o
	interpretados según un criterio dado. De este modo, la rutina
	conoce la cantidad de arcos que que cumplen el criterio.
        @return true si el grafo no contiene ningún ciclo; false de lo 
        contrario. 
        @note Debido a la prueba inicial con la cantidad de arcos, esta rutina 
        es preferible que test_for_cycle(), aún bajo el conocimiento de que el
        grafo es conexo, pues esta última rutina siempre realiza la 
        exploración en profundidad, mientras que is_graph_acyclique() no.
     */
  bool operator () (GT & g, size_t num_arcs) 
  {
    return is_acyclique(g, num_arcs);
  }

      /** Invoca a la prueba de aciclicidad.

        Si g es un grafo (no un digrafo) entonces el algoritmo compara la 
        cantidad de arcos con la de nodos. Si el grafo tiene menos arcos que 
        nodos, entonces se concluye que el grafo es acíclico. 
        Consecuentemente, esta técnica no opera para multigrafos.
          
        @param[in] g el grafo a probar.
        @return true si el grafo no contiene ningún ciclo; false de lo 
        contrario. 
        @note Debido a la prueba inicial con la cantidad de arcos, esta rutina 
        es preferible que test_for_cycle(), aún bajo el conocimiento de que el
        grafo es conexo, pues esta última rutina siempre realiza la 
        exploración en profundidad, mientras que is_graph_acyclique() no.
     */
  bool operator () (GT & g) 
  {
    return is_acyclique(g, g.get_num_arcs());
  }
};

     /** Determina si un grafo contiene o no ciclos.

          Has_Cycle explora en profundidad en grafo g, a 
          partir de un nodo y verifica si el grafo es acíclico; es 
          decir, que no contenga ningún ciclo.

          La clase toma dos parámetros tipo:
          -# GT: el tipo de grafo, el cual debe ser derivado de List_Graph.
          -# SA: clase que se encarga de mostrar el arco. Internamente, la
          función emplea el iterador filtro Node_Arc_Iterator (basado
          en Filter_Iterator) para recorrer los arcos de cada nodo. SA es
          el la clase que determina si el arco debe o no mostrarse al
          recorrido. 

          La clase usa el bit is_acyclique y lo reinicia al principio del 
          algoritmo para marcar los nodos y arcos ya visitados.

          @ingroup Grafos
       */
      template <class GT, class SA = Dft_Show_Arc<GT>> 
class Has_Cycle
{
  SA & sa;

public:

  Has_Cycle(SA && __sa = SA()) : sa(__sa) { /* empty */ }

  Has_Cycle(SA & __sa) : sa(__sa) { /* empty */ }

  /** Invoca la prueba de existencia de ciclos.

      Si g es un grafo (no un digrafo) entonces el algoritmo compara la
      cantidad de arcos con la de nodos. Si el grafo tiene igual o más
      arcos que nodos, entonces se concluye que el grafo tiene ciclos.
      Consecuentemente, esta técnica no opera para multigrafos.
          
      @param[in] g el grafo a probar.
      @return true si el grafo no contiene ningún ciclo; false de lo 
      contrario. 
      @note Debido a la prueba inicial con la cantidad de arcos, esta rutina 
      es preferible que test_for_cycle(), aún bajo el conocimiento de que el
      grafo es conexo, pues esta última rutina siempre realiza la 
      exploración en profundidad, mientras que is_graph_acyclique() no.
  */
  bool operator () (GT & g) const
  {
    return not Is_Graph_Acyclique <GT, SA> (sa) (g);
  }

  /** Invoca la prueba de existencia de ciclos.

      Si g es un grafo (no un digrafo) entonces el algoritmo compara la
      cantidad de arcos con la de nodos. Si el grafo tiene igual o más
      arcos que nodos, entonces se concluye que el grafo tiene ciclos.
      Consecuentemente, esta técnica no opera para multigrafos.
          
      @param[in] g el grafo a probar.
      @return true si el grafo no contiene ningún ciclo; false de lo 
      contrario. 
      @note Debido a la prueba inicial con la cantidad de arcos, esta rutina 
      es preferible que test_for_cycle(), aún bajo el conocimiento de que el
      grafo es conexo, pues esta última rutina siempre realiza la 
      exploración en profundidad, mientras que is_graph_acyclique() no.
  */
  bool operator () (GT & g, size_t num_arcs) const
  {
    return not Is_Graph_Acyclique <GT, SA> (sa) (g, num_arcs);
  }
};




} // end namespace Aleph

# endif //  TPL_TEST_ACYCLIQUE_H
