
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file single_graph.H
 *  @brief Single graph utilities.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef SINGLE_GRAPH_H
# define SINGLE_GRAPH_H

# include <tpl_dynSetTree.H>

namespace Aleph {

/** Determina si un grafo o digrafo es simple. Un grafo simple es uno
    que notiene arcos repetidos.

    La clase recibe tres parámetros tipo:
    -# GT: el grafo
    -# SN: clase filtro para el iterador de nodos.
    -# SA: clase filtro para el iterador de arcos.
    .

    @ingroup Graphs
    @author Leandro R. León
*/
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Single_Graph
{
  SN & sn;
  SA & sa;

  typedef typename GT::Node GT_Node;

public:

  Test_Single_Graph(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

      /// retorna true si p no tiene multiarcos ni lazos
  bool test_node(typename GT::Node * p)
  {
    DynSetAvlTree<GT_Node*> nodes;

    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      {
	GT_Node * q = it.get_tgt_node_ne();
	if (nodes.exist(q))
	  return false;
	else
	  nodes.insert(p);
      }
    
    return true;
  }

  bool operator () (GT & g)
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if (not test_node(it.get_curr()))
	return false;

    return true;
  }
};



} // end namespace Aleph

# endif // SINGLE_GRAPH_H_H
