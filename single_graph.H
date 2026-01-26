
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
