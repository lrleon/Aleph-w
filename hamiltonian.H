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

/** @file hamiltonian.H
 *  @brief Hamiltonian path and cycle detection.
 *
 *  Provides algorithms to find Hamiltonian paths (visiting every
 *  vertex exactly once) and Hamiltonian cycles in graphs. Uses
 *  backtracking with pruning.
 *
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef HAMILTONIAN_H
# define HAMILTONIAN_H

# include <tpl_graph.H>

namespace Aleph 
{

    /** Determina si un grafo o digrafo satisface las pruebas de
	suficiencia para ser hamiltoniano.

	La clase Test_Hamiltonian_Sufficiency efectúa una prueba de
	suficiencia hamiltoniana para el grafo o digrafo parámetro.

	La clase recibe tres parámetros tipo:
	-# GT: el grafo
	-# SN: clase filtro para el iterador de nodos.
	-# SA: clase filtro para el iterador de arcos.
	.

	La prueba está basada en el teorema de suficiencia de Ore.

	@ingroup Grafos
	@author Leandro R. León
     */
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Hamiltonian_Sufficiency
{
  SN & sn;
  SA & sa;

  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
	typename GT::Node * src = i.get_curr();
	const size_t & nsrc = g.get_num_arcs(src);

	i.next_ne();

	for (Node_Iterator<GT, SN> j(i); j.has_curr(); j.next_ne())
	  if (nsrc + g.get_num_arcs(j.get_curr()) < n)
	    return false;
      }

    return true;
  }

  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

      // una primera pasada sobre arcos para contar grados de entrada
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
	typename GT::Node * src = i.get_curr();

	for (Node_Iterator<GT, SN> j(g, sn); j.has_curr(); j.next_ne())
	  {
	    typename GT::Node * tgt = j.get_curr();

	    if (src == tgt)
	      continue;

	    if (g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n)
	      continue;

	        // buscar si hay arco src-->tgt
	    bool terminate = true;
	    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); 
		 it.next_ne())
	      if (src == it.get_tgt_node_ne())
		{
		  terminate = false;
		  break;
		}
	    
	    if (terminate)
	      return false;
	  }
      } 

    return true;
  }

public:

  Test_Hamiltonian_Sufficiency(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

      // retorna true si el grafo o digrafo satisface la prueba de
      // suficiencia para ser hamiltoniano.
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};



} // end namespace Aleph

# endif // HAMILTONIAN_H
