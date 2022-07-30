
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef TPL_KGRAPH_H
# define TPL_KGRAPH_H

# include <Set.H>
# include <tpl_net.H>

namespace Aleph {

    /** Calcula la conectividad en arcos de un grafo.

	edge_connectivity(g) calcula la conectividad en arcos de un
	grafo mediante maximizaciones sucesivas de flujo sobre una red
	alterna capacitada unitaria. 

	La rutina recibe dos paráetros tipo:
	-# GT: el tipo grafo sobre al cual se le desea averiguar su
	conectividad. 
	-# Max_Flow el algoritmo de maximización de flujo a emplear para
	averiguar conectividad.
	.

	@param[in] g el grafo.
	@return la conectividad en arcos del grafo g.
	@exception bad_alloc si no hay suficiente memoria.

	@ingroup Grafos 
     */
template <class GT, 
	  template <class> class Max_Flow = Random_Preflow_Maximum_Flow,
	  class SA = Dft_Show_Arc<GT> > 
long edge_connectivity(GT & g)
{
  typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class>> Net;
  Net net;
  typename Net::Node * source = nullptr; 
  long min_degree = numeric_limits<long>::max(); 
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      NODE_COOKIE(p) = net.insert_node();
      if (g.get_num_arcs(p) < min_degree)
	{
	  source = mapped_node<GT, Net> (p);
	  min_degree = g.get_num_arcs(p);
	}
    }

  if (min_degree <= 1)
    return min_degree; 

  for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne()) 
    {
      auto a   = it.get_curr();
      auto src = mapped_node<GT, Net> (g.get_src_node(a));
      auto tgt = mapped_node<GT, Net> (g.get_tgt_node(a));
      if (src != source)
	net.insert_arc(tgt, src, 1);
      if (tgt != source)
	net.insert_arc(src, tgt, 1);
    }

  long min_k = min_degree;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto sink = it.get_curr();
      if (sink == source) 
	continue;

      DynDlist<typename Net::Arc*> from_sink_arcs;

      for (Node_Arc_Iterator<Net> it(sink); it.has_curr(); it.next_ne())
	from_sink_arcs.append(it.get_curr());

      for (typename DynDlist<typename Net::Arc*>::Iterator 
	     it(from_sink_arcs); it.has_curr(); it.next_ne())
	net.disconnect_arc(it.get_curr());

      const typename Net::Flow_Type flow = Max_Flow <Net> () (net); 
      if (flow < min_k)
	min_k = flow;

      while (not from_sink_arcs.is_empty())
	net.connect_arc(from_sink_arcs.get());

      net.reset(); // colocar flujo en cero
    }

  return min_k;
}

   /** Clase de cálculo de conectividad en arcos de un grafo.
       
       La rutina recibe dos paráetros tipo:
       -# GT: el tipo grafo sobre al cual se le desea averiguar su
       conectividad. 
       -# Max_Flow el algoritmo de maximización de flujo a emplear para
       averiguar conectividad. Por omisión, el algoritmo de maximización
       es por empuje de preflujo con heap.
       .

	@param[in] g el grafo.
	@return la conectividad en arcos del grafo g.
	@exception bad_alloc si no hay suficiente memoria.
	
	@ingroup Grafos 
     */
    template <class GT, 
	      template <class> class Max_Flow = Heap_Preflow_Maximum_Flow> 
class Edge_Connectivity
{
public:

  /** Calcula la conectividad en arcos de grafo g.

      Calcula la conectividad en arcos de un
      grafo mediante maximizaciones sucesivas de flujo sobre una red
      alterna capacitada unitaria. 

      @param[in] g el grafo.
      @return la conectividad en arcos del grafo g.
      @exception bad_alloc si no hay suficiente memoria.
   */
  long operator () (GT & g) { return edge_connectivity <GT, Max_Flow> (g); }
};


    /** Calcula el corte mínimo de un grafo.

	compute_min_cut(g, l, r, cut) indaga la conectividad en
	arcos del grafo g y retorna los conjuntos de nodos y arcos que
	conforman el corte.

	El algoritmo construye una red capacitada unitaria equivalente y
	mediante sucesivas maximizaciones de flujo indaga la
	conectividad en arcos. 

	La rutina recibe dos paráetros tipo:
	-# GT: el tipo grafo sobre al cual se le desea averiguar su
	conectividad. 
	-# Max_Flow el algoritmo de maximización de flujo a emplear para
	averiguar conectividad. 
	.
	
	@param[in] g el grafo sobre el cual se desea calcular el corte
	mínimo.
	@param[out] l un conjunto de nodos origen que involucra los
	arcos parte del corte.
	@param[out] r un conjunto de nodos destino que involucra los
	arcos parte del corte y que son destino de los nodos en l.
	@param[out] cut el conjunto de arcos parte del corte.
	@return la conectividad en arcos del grafo.
	@exception bad_alloc si no hay suficiente memoria.
	@ingroup Grafos 
     */
  template <class GT, 
	    template <class> class Max_Flow = Heap_Preflow_Maximum_Flow,
	    class SA = Dft_Show_Arc<GT> > 
long compute_min_cut(GT &                             g,
		     Aleph::set<typename GT::Node*> & l, 
		     Aleph::set<typename GT::Node*> & r,
		     DynDlist<typename GT::Arc *> &   cut)
{
  typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class> > Net;
  Net net;
  typename Net::Node * source = nullptr; 
  long min_degree = numeric_limits<long>::max(); 
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
    {
      auto p  = it.get_curr();
      auto q = net.insert_node();
      GT::map_nodes (p, q);

      if (g.get_num_arcs(p) < min_degree)
	{
	  source = mapped_node<GT, Net> (p);
	  min_degree = g.get_num_arcs(p);
	}
    }

  if (min_degree <= 1)
    return min_degree; 

  for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne()) 
    {
      auto a   = it.get_curr();
      auto src = mapped_node<GT, Net>(g.get_src_node(a));
      auto tgt = mapped_node<GT, Net>(g.get_tgt_node(a));
      if (src != source)
	{
	  auto arc = net.insert_arc(tgt, src, 1);
	  ARC_COOKIE(arc) = a;
	}

      if (tgt != source)
	{
	  auto arc = net.insert_arc(src, tgt, 1);
	  ARC_COOKIE(arc) = a;
	}
    }

  Aleph::set<typename Net::Node*> tmp_vs, tmp_vt;
  DynDlist<typename Net::Arc*>    tmp_cuts, tmp_cutt;
  long min_k = numeric_limits<long>::max();  
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto sink = it.get_curr();
      if (sink == source) 
	continue;

      DynDlist<typename Net::Arc*> from_sink_arcs;

      for (Node_Arc_Iterator<Net> it(sink); it.has_curr(); it.next_ne())
	from_sink_arcs.append(it.get_curr());

      for (typename DynDlist<typename Net::Arc*>::Iterator 
	     it(from_sink_arcs); it.has_curr(); it.next_ne())
	net.disconnect_arc(it.get_curr());

      Aleph::set<typename Net::Node*> vs, vt;
      DynDlist<typename Net::Arc *> cuts, cutt;
      const auto flow = Min_Cut <Net, Max_Flow> () (net, vs, vt, cuts, cutt); 

      if (flow < min_k)
	{
	  min_k = flow;
	  tmp_vs.swap(vs);
	  tmp_vt.swap(vt);
	  tmp_cuts.swap(cuts);
	  tmp_cutt.swap(cutt);
	}

      net.reset(); // colocar flujo en cero

      while (not from_sink_arcs.is_empty())
	net.connect_arc(from_sink_arcs.get());
    }
  
  for (typename Aleph::set<typename Net::Node*>::iterator
	 it = tmp_vs.begin(); it != tmp_vs.end(); it++)
    l.insert(mapped_node <Net, GT> (*it));

  for (typename Aleph::set<typename Net::Node*>::iterator 
	 it = tmp_vt.begin(); it != tmp_vt.end(); it++)
    r.insert(mapped_node <Net, GT> (*it));

  for (typename DynDlist<typename Net::Arc*>::Iterator it(tmp_cuts); 
       it.has_curr(); it.next_ne())
    {
      typename Net::Arc* arc = it.get_curr();
      cut.append(mapped_arc<Net, GT> (arc));
    }

  return min_k;
}

   /** Clase de cálculo de corte mínimo de un grafo.

	compute_min_cut(g, l, r, cut) indaga la conectividad en
	arcos del grafo g y retorna los conjuntos de nodos y arcos que
	conforman el corte.

	El algoritmo construye una red capacitada unitaria equivalente y
	mediante sucesivas maximizaciones de flujo indaga la
	conectividad en arcos. 

	La rutina recibe dos paráetros tipo:
	-# GT: el tipo grafo sobre al cual se le desea averiguar su
	conectividad. 
	-# Max_Flow el algoritmo de maximización de flujo a emplear para
	averiguar conectividad. Por omisión, el algoritmo de maximización
	es por empuje de preflujo con heap.
	.
	
	@param[in] g el grafo sobre el cual se desea calcular el corte
	mínimo.
	@param[out] l un conjunto de nodos origen que involucra los
	arcos parte del corte.
	@param[out] r un conjunto de nodos destino que involucra los
	arcos parte del corte y que son destino de los nodos en l.
	@param[out] cut el conjunto de arcos parte del corte.
	@return la conectividad en arcos del grafo.
	@exception bad_alloc si no hay suficiente memoria.
	@ingroup Grafos 
     */
    template <class GT, 
	      template <class> class Max_Flow = Heap_Preflow_Maximum_Flow,
	      class SA = Dft_Show_Arc<GT> > 
class Compute_Min_Cut
{
public:
  long operator () (GT &                             g,
		    Aleph::set<typename GT::Node*> & l, 
		    Aleph::set<typename GT::Node*> & r,
		    DynDlist<typename GT::Arc *> &   cut) 
  { 
    return compute_min_cut <GT, Max_Flow, SA> (g, l, r, cut); 
  }
};


  /** Calcula la conectividad en  nodos o vértices de un grafo.

      Esta rutina se vale de un algoritmo de maximización de flujo para
      calcular la conectividad en nodos de un grafo. Como tal, su
      segundo parámetro plantilla es el algoritmo de maximización de
      flujo. 

      @param[in] g el grafo.

     @ingroup Grafos
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
      auto p = it.get_curr();
      NODE_COOKIE(p) = net.insert_node();
    }

  for (Arc_Iterator<GT, SA> it(g); it.has_curr(); it.next_ne()) 
    {
      auto a   = it.get_curr();
      auto src = mapped_node<GT,Net>(g.get_src_node(a));
      auto tgt = mapped_node<GT,Net>(g.get_tgt_node(a));
      net.insert_arc(tgt, src, 1);
      net.insert_arc(src, tgt, 1);
    }

  long min_k = g.get_num_nodes();
  int i = 1;

  for (Node_Iterator<Net> k(net); k.has_curr() and i < min_k; k.next_ne(), i++)
    {
      auto source = k.get_curr();
      DynDlist<typename Net::Arc*> to_source_list;
      for (Node_Arc_Iterator<Net> it(source); it.has_curr(); it.next_ne())
	{
	  auto from_arc = it.get_curr();
	  auto to_arc = 
	    search_arc<Net>(net, net.get_tgt_node(from_arc), source);
	  assert(to_arc != nullptr);
	  assert(net.get_tgt_node(to_arc) == source);
	  to_source_list.append(to_arc);
	}

      for (typename DynDlist<typename Net::Arc *>::Iterator it(to_source_list); 
	   it.has_curr(); it.next_ne())
	net.disconnect_arc(it.get_curr());

      {
	Node_Iterator<Net> j(k);
	for (j.next(); j.has_curr(); j.next_ne())
	  {
	    auto sink = j.get_curr();
	    if (search_arc <Net> (net, source, sink) != nullptr)
	      continue; // existe arco ==> ignórelo y avance a siguiente

	    DynDlist<typename Net::Arc*> from_sink_arcs;

	    for (Node_Arc_Iterator<Net> it(sink); it.has_curr(); it.next_ne())
	      from_sink_arcs.append(it.get_curr());

	    for (typename DynDlist<typename Net::Arc*>::Iterator 
		   it(from_sink_arcs); it.has_curr(); it.next_ne())
	      net.disconnect_arc(it.get_curr());
	    {
	      Net aux_net;
	      {
		DynMapTreap<typename Net::Node*,typename Net::Arc*> map;
		for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
		  {
		    auto p = it.get_curr();
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
		    auto a    = it.get_curr();
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
