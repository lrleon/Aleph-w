

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

  template <class GT, 
	    template <class> class Max_Flow = Random_Preflow_Maximum_Flow,
	    class SA                        = Dft_Show_Arc<GT> > 
long vertex_connectivity(GT & g)
{
  typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class> > Net;
  Net net;
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr_ne();
      NODE_COOKIE(p) = net.insert_node();
    }

  for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne()) 
    {
      auto a   = it.get_curr_ne();
      auto src = mapped_node<GT,Net>(g.get_src_node(a));
      auto tgt = mapped_node<GT,Net>(g.get_tgt_node(a));
      net.insert_arc(tgt, src, 1);
      net.insert_arc(src, tgt, 1);
    }

  long min_k = g.get_num_nodes();
  int i = 1;

  for (Node_Iterator<Net> k(net); k.has_curr() and i < min_k; k.next_ne(), i++)
    {
      auto source = k.get_curr_ne();
      DynDlist<typename Net::Arc*> to_source_list;
      for (Node_Arc_Iterator<Net> it(source); it.has_curr(); it.next_ne())
	{
	  auto from_arc = it.get_curr_ne();
	  auto to_arc = 
	    search_arc<Net>(net, net.get_tgt_node(from_arc), source);
	  assert(to_arc != nullptr);
	  assert(net.get_tgt_node(to_arc) == source);
	  to_source_list.append(to_arc);
	}

      for (typename DynDlist<typename Net::Arc *>::Iterator it(to_source_list); 
	   it.has_curr(); it.next_ne())
	net.disconnect_arc(it.get_curr_ne());

      {
	Node_Iterator<Net> j(k);
	for (j.next(); j.has_curr(); j.next_ne())
	  {
	    auto sink = j.get_curr_ne();
	    if (search_arc <Net> (net, source, sink) != nullptr)
	      continue; // existe arco ==> ignórelo y avance a siguiente

	    DynDlist<typename Net::Arc*> from_sink_arcs;

	    for (Node_Arc_Iterator<Net> it(sink); it.has_curr(); it.next_ne())
	      from_sink_arcs.append(it.get_curr_ne());

	    for (typename DynDlist<typename Net::Arc*>::Iterator 
		   it(from_sink_arcs); it.has_curr(); it.next_ne())
	      net.disconnect_arc(it.get_curr_ne());
	    {
	      Net aux_net;
	      {
		DynMapTreap<typename Net::Node*,typename Net::Arc*> map;
		for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
		  {
		    auto p = it.get_curr_ne();
		    if (p == source or p == sink)
		      {
			NODE_COOKIE(p) = aux_net.insert_node();
			continue;
		      }

		    auto ps = aux_net.insert_node(p->get_info());
		    auto pt = aux_net.insert_node(p->get_info());
		    map.insert(p, aux_net.insert_arc(ps, pt, 1));
		  }

		for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
		  {
		    auto a    = it.get_curr_ne();
		    auto nsrc = net.get_src_node(a);
		    auto ntgt = net.get_tgt_node(a);
		    typename Net::Node * asrc = nullptr;
		    typename Net::Node * atgt = nullptr;

		    if (nsrc == source)
		      asrc = (typename Net::Node *) NODE_COOKIE(nsrc);
		    else
		      {
			auto arc = map.find(nsrc);
			asrc = aux_net.get_tgt_node(arc);
		      }

		    if (ntgt == sink)
		      atgt = (typename Net::Node *) NODE_COOKIE(ntgt);
		    else
		      {
			auto arc = map.find(ntgt);
			atgt = aux_net.get_src_node(arc);
		      }
		    aux_net.insert_arc(asrc, atgt, 1);
		  }
	      } // fin de bloque que crea el mapeo

	      const auto flow = Max_Flow <Net> () (aux_net); 
	      if (flow < min_k)
		min_k = flow;
	    }

	    while (not from_sink_arcs.is_empty())
	      net.connect_arc(from_sink_arcs.get());

	    net.reset(); // colocar flujo en cero
	  }

	while (not to_source_list.is_empty())
	  net.connect_arc(to_source_list.get());
      }
    }

  return min_k;
}


} // end namespace Aleph

# endif // TPL_KGRAPH_H
