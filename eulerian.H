
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
# ifndef EULERIAN_H
# define EULERIAN_H

# include <tpl_graph.H>

namespace Aleph
{

    /** Determina si un grafo o digrafo es euleriano

	La clase Test_Eulerian efectúa verifica si el grafo o digrafo es
	euleriano; es decir, si contiene un ciclo euleriano.

	La clase recibe tres parámetros tipo:
	-# GT: el grafo
	-# SN: clase filtro para el iterador de nodos.
	-# SA: clase filtro para el iterador de arcos.
	.

	@ingroup Grafos
	@author Leandro R. León
     */
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Eulerian
{
  SN & sn;
  SA & sa;

  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
	return false;

    return true;
  }

  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

      // una primera pasada sobre arcos para contar grados de entrada
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
	typename GT::Node * p = it.get_curr();
	if (g.get_num_arcs(p) != NODE_COUNTER(p))
	  return false;
      }

    return true;
  }

public:

  Test_Eulerian(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

      // retorna true si el grafo el euleriano
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};


} // end namespace Aleph

# endif // EULERIAN_H
