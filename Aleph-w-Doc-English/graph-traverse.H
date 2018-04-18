
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file graph-traverse.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef GRAPH_TRAVERSE_H
# define GRAPH_TRAVERSE_H

# include <tpl_agraph.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>

/** 
  @brief traverses a graph in depth or breadth and
   function executes visit

   @ingroup Graphs
*/
template <class GT, class Itor, 
	  template <typename T> class Q = DynListStack,
	  class Show_Arc = Dft_Show_Arc<GT>>
class Graph_Traverse
{
  GT & g;
  Show_Arc & sa;

public:

  Graph_Traverse(GT & __g, Show_Arc & __sa) : g(__g), sa(__sa) {}

  Graph_Traverse(GT & __g, Show_Arc && __sa = Show_Arc()) : g(__g), sa(__sa) {}

  template <class Node_Op> 
  size_t operator () (typename GT::Node * start, Node_Op & op)
  {
    g.reset_nodes();
    g.reset_arcs();

    size_t count = 1;
    start->set_state(Processed);
    if (not op(start))
      return count;

    Q<typename GT::Arc*> q;
    for (Itor it(start, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * a = it.get_curr();
	typename GT::Node * tgt = g.get_connected_node(a, start);
	a->set_state(Processing);
	tgt->set_state(Processing);
	q.put(a);
      }

    const size_t & n = g.vsize(); 
    while (not q.is_empty() and count < n)
      {
	typename GT::Arc * arc = q.get();
	assert(arc->state() == Processing);
	arc->set_state(Processed);

	typename GT::Node * s = g.get_src_node(arc);
	typename GT::Node * t = g.get_tgt_node(arc);
	if (s->state() == Processed and t->state() == Processed)
	  continue; 

	typename GT::Node * curr = s->state() == Processed ? t : s;
	assert(curr->state() == Processing);
	++count;
	curr->set_state(Processed); 
	if (not op(curr))
	  return count;

	for (Itor it(curr, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * a  = it.get_curr();
	    if (a->state() != Unprocessed) 
	      continue;
	    typename GT::Node * tgt = g.get_connected_node(a, curr);
	    if (tgt->state() == Unprocessed)
	      {
		assert(a->state() != Processing); 
		q.put(a);
		a->set_state(Processing);
		tgt->set_state(Processing);
	      }
	    else
	      a->set_state(Processed);
	  }
      }

    return count;
  }

  template <class Node_Op> 
  size_t operator () (typename GT::Node * start, Node_Op && op = Node_Op())
  {
    return (*this)(start, op); 
  }

/** @brief Run the op (curr, arc) operation,
where curr is visited and op is the bow of origin node. */
  template <class Op> 
  size_t exec(typename GT::Node * start, Op & op)
  {
    g.reset_nodes();
    g.reset_arcs();

    size_t count = 1;
    start->set_state(Processed);
    if (not op(start, NULL))
      return count;
    
    using Pair = tuple<typename GT::Node*, typename GT::Arc*>; 
    Q<Pair> q;
    for (Itor it(start, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * a = it.get_curr();
	typename GT::Node * tgt = g.get_connected_node(a, start);
	a->set_state(Processing);
	tgt->set_state(Processing);
	q.put(make_tuple(start, a));
      }

    const size_t & n = g.vsize(); 
    while (not q.is_empty() and count < n)
      {
	const Pair p = q.get();
	typename GT::Arc * arc = get<1>(p);
	assert(arc->state() == Processing);
	assert(get<0>(p)->state() == Processed); 
	arc->set_state(Processed);

	typename GT::Node * curr = g.get_connected_node(arc, get<0>(p));
	assert(curr->state() == Processing);
	++count;
	curr->set_state(Processed); 
	if (not op(curr, arc))
	  return count;

	for (Itor it(curr, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * a  = it.get_curr();
	    if (a->state() != Unprocessed) 
	      continue;
	    typename GT::Node * tgt = g.get_connected_node(a, curr);
	    if (tgt->state() == Unprocessed)
	      {
		assert(a->state() != Processing); 
		a->set_state(Processing);
		tgt->set_state(Processing);
		q.put(make_tuple(curr, a));
	      }
	    else
	      a->set_state(Processed);
	  }
      }

    return count;
  }

  template <class Operation> 
  size_t exec(typename GT::Node * start, Operation && op = Operation())
  {
    return exec(start, op);
  }

  template <class Node_Op, class Arc_Op> 
  tuple<size_t, size_t> operator () (typename GT::Node * start, 
				     Node_Op & node_op, Arc_Op & arc_op)
  {
    g.reset_nodes();
    g.reset_arcs();
    Q<typename GT::Arc*> q;

    size_t node_count = 1;
    size_t arc_count = 0;

    start->set_state(Processed);
    if (not node_op(start))
      return make_tuple(1, 0);

    for (Itor it(start, sa); it.has_curr(); it.next())
      {
	typename GT::Arc * a = it.get_curr();
	typename GT::Node * tgt = g.get_connected_node(a, start);
	a->set_state(Processing);
	tgt->set_state(Processing);
	q.put(a);
	++arc_count;
	if (not arc_op(a))
	  return make_tuple(node_count, arc_count);
      }

    while (not q.is_empty())
      {
	typename GT::Arc * arc = q.get();
	assert(arc->state() == Processing);
	arc->set_state(Processed);

	typename GT::Node * s = g.get_src_node(arc);
	typename GT::Node * t = g.get_tgt_node(arc);
	if (s->state() == Processed and t->state() == Processed)
	  continue; 

	typename GT::Node * curr = s->state() == Processed ? t : s;
	assert(curr->state() == Processing);
	++node_count;
	curr->set_state(Processed); 
	if (not node_op(curr))
	  return make_tuple(node_count, arc_count);

	for (Itor it(curr, sa); it.has_curr(); it.next()) 
	  {
	    typename GT::Arc * a  = it.get_curr();
	    if (a->state() != Unprocessed) 
	      continue;
	    typename GT::Node * tgt = g.get_connected_node(a, curr);
	    if (tgt->state() != Processed)
	      {
		q.put(a);
		tgt->set_state(Processing);
		a->set_state(Processing);
		++arc_count;
		if (not arc_op(a))
		  return make_tuple(node_count, arc_count); 
	      }
	    else
	      a->set_state(Processed);
	  }
      }

    return make_tuple(node_count, arc_count); 
  }  

  template <class Node_Op, class Arc_Op> 
  tuple<size_t, size_t> operator () (typename GT::Node * start, 
				     Node_Op && node_op = Node_Op(), 
				     Arc_Op && arc_op = Arc_Op())
  {
    return (*this)(start, node_op, arc_op); 
  }
};


template <class GT, class Itor, 
	  class Show_Arc = Dft_Show_Arc<GT>>
  using Graph_Traverse_DFS = Graph_Traverse<GT, Itor, DynListStack, Show_Arc>;

template <class GT, class Itor, 
	  class Show_Arc = Dft_Show_Arc<GT>>
  using Graph_Traverse_DFB = Graph_Traverse<GT, Itor, DynListQueue, Show_Arc>;


# endif // GRAPH_TRAVERSE_H
  