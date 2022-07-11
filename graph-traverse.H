
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# ifndef GRAPH_TRAVERSE_H
# define GRAPH_TRAVERSE_H

# include <tpl_agraph.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>

/** Atraviesa un grafo en profundidad o en amplitud y ejecuta función
    de visita 

   @ingroup Grafos
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
    for (Itor it(start, sa); it.has_curr(); it.next_ne())
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

	for (Itor it(curr, sa); it.has_curr(); it.next_ne()) 
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

  /// Ejecuta la operación op(curr, arc), donde curr es el nodo visitado
  /// y op es el arco de proveniencia
  template <class Op> 
  size_t exec(typename GT::Node * start, Op & op)
  {
    g.reset_nodes();
    g.reset_arcs();

    size_t count = 1;
    start->set_state(Processed);
    if (not op(start, nullptr))
      return count;
    
    using Pair = tuple<typename GT::Node*, typename GT::Arc*>; 
    Q<Pair> q;
    for (Itor it(start, sa); it.has_curr(); it.next_ne())
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

	for (Itor it(curr, sa); it.has_curr(); it.next_ne()) 
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

    for (Itor it(start, sa); it.has_curr(); it.next_ne())
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

	for (Itor it(curr, sa); it.has_curr(); it.next_ne()) 
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
  using Graph_Traverse_BFS = Graph_Traverse<GT, Itor, DynListQueue, Show_Arc>;


# endif // GRAPH_TRAVERSE_H
