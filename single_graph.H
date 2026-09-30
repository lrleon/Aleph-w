
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

# include <ah-graph-concepts.H>

# include <tpl_dynSetTree.H>

namespace Aleph {

/** Determines whether a graph or digraph is simple. A simple graph is one
    that has no duplicate arcs.

    The class takes three type parameters:
    -# GT: the graph
    -# SN: filter class for the node iterator.
    -# SA: filter class for the arc iterator.
    .

    @ingroup Graphs
    @author Leandro R. León
*/
template <AlephGraph GT, 
	  NodeFilter<GT> SN = Dft_Show_Node<GT>,
	  ArcFilter<GT> SA = Dft_Show_Arc<GT> >
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

      /// Returns `true` if `p` has neither multi-arcs nor self-loops
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
