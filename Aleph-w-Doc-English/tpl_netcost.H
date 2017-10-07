
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
  @file tpl_netcost.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_MAXFLOW_MINCOST_H
# define TPL_MAXFLOW_MINCOST_H

# include <Bellman_Ford.H>
# include <tpl_net.H>

using namespace Aleph;

namespace Aleph {

template <typename Node_Info = Empty_Class>
using Net_Cost_Node = Net_Node<Node_Info>;

/** 
  @brief Defining network bow for maximum flow at minimum cost.

  @details Net_Cost_Arc modeliza un arco pertenciente a una red de flujo
  con costes asociados en sus arcos.

  The class manages two type parameters:
  -# Arc_Info: Type representing the attributes associated with the arc.
  -# F_Type: Data type that represents the capacity,
  flow and cost. This standard has the same as for
  accumulated flow of the nodes.

  @ingroup Grafos Redes
*/                               
    template <typename Arc_Info = Empty_Class, typename F_Type = double>
struct Net_Cost_Arc : public Net_Arc<Arc_Info, F_Type>
{ 
  using Base = Net_Arc<Arc_Info, F_Type>;

  using Base::Base; /* Hereda constructores */

/** @brief Type that representing the flow */
  typedef F_Type Flow_Type;

/** @brief Cost per unit of flow
(negative if the arc is residual) */
  Flow_Type cost;

/** @brief Returns the cost of circulating flow. */
  Flow_Type flow_cost() const { return this->flow*cost; }

  Net_Cost_Arc(const Net_Cost_Arc & a) : Base(a), cost(a->cost) {}

  Net_Cost_Arc & operator = (const Net_Cost_Arc & a)
  {
    if (&a == this)
      return *this;
    *(Base*) this = a;
    cost = a.cost;
    return *this;
  }
};

/** 
  @brief Defining of network capable of flow with costs associated with
  flows.  

  @details This type derived from net_graph, modeled a trained network
  which each arc is defined a cost per unit of flow. east
  model, network extension skilled expands considerably
  the range of combinatorial optimization problems to raise a
  max-min optimization type. In this case, the maximum cost flow
  minimum.

  Being bypass net_graph, its interface is very similar, with the
  addition of parameters and methods for managing the cost of each
  arc.

  @see max_flow_min_cost_by_cycle_canceling Max_Flow_Min_Cost

  @ingroup Redes
*/
  template <class NodeT = Net_Cost_Node<Empty_Class>, 
	    class ArcT = Net_Cost_Arc<Empty_Class, double>>
struct Net_Cost_Graph : public Net_Graph<NodeT, ArcT>
{
  using Base = Net_Graph<NodeT, ArcT>;

  Net_Cost_Graph(const Net_Cost_Graph & net) : Base(net)
  {
    using Pair = 
      std::pair<typename Net_Cost_Graph::Arc*, typename Net_Cost_Graph::Arc*>;
    zip(this->arcs(), net.arcs()).for_each([] (const Pair & p)
      {
  	typename Net_Cost_Graph::Arc * atgt = p.first;
  	typename Net_Cost_Graph::Arc * asrc = p.second;
  	atgt->cost = asrc->cost;
      });
  }

  Net_Cost_Graph() {}

/** @brief The type of network flow on which this model is based cost. */
  typedef Net_Graph<NodeT, ArcT> Net;

  typedef Net_Cost_Graph<NodeT, ArcT> Net_MFMC;

/** @brief Type of arc. */
  typedef ArcT Arc;

/** @brief Type of node. */
  typedef NodeT Node;

/** @brief Type representing the capacity and flow. */
  typedef typename Arc::Flow_Type Flow_Type;

/** @brief Attribute type that stores a node. */
  typedef typename Node::Node_Type Node_Type;

/** @brief Attribute type that stores a arc. */
  typedef typename Arc::Arc_Type Arc_Type;

/** @brief Returns a modified reference to the cost of an arc. */
  Flow_Type & get_cost(Arc * a) { return a->cost; }

/** @brief Returns a reference to the cost of an arc. */
  const Flow_Type & flow_cost(Arc * a) const { return a->flow_cost(); }

/** 
  @brief Creates and inserts an arc in a flow network with costs. The arc
  created with zero flow.

  @param[in] src_node Origin node
  @param[in] tgt_node Destination node.
  @param[in] cap Arch capacity value
  @param[in] cost Cost value per unit of arch flow

  @return A pointer to the new arc

  @throw bad_alloc If there is insufficient memory
*/
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node,
			   const Flow_Type & cap, const Flow_Type & cost)
  {
    Arc * a = Net::insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
    a->cost = cost;
    return a;
  }

/** @brief Used by internal algorithm.
DO NOT USE */
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node)
  {
    Arc * a = Net::insert_arc(src_node, tgt_node, Arc_Type());
    a->cost = 0;
    return a;
  }

/** @brief Used by internal algorithm.
DO NOT USE */
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node, 
			   const Arc_Type & arc_info)
  {
    Arc * a = Net::insert_arc(src_node, tgt_node, arc_info);
    a->cost = 0;
    return a;
  }

/** @brief Calculate the total cost of current flow through the network. */
  Flow_Type flow_cost()
  {
    Flow_Type total = 0;
    for (Arc_Iterator<Net_MFMC> it(*this); it.has_current(); it.next())
      {
	Arc * a = it.get_current();
	total += a->flow_cost();
      }

    return total;
  }

  using Pars = tuple<Flow_Type, Flow_Type, Flow_Type>;
  
/** @brief Returns a triplet of capacity,
outgoing flow and cost. */
  Pars out_pars(Node * p)
  {
    Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
    for (__Out_Iterator<Net> it(p); it.has_curr(); it.next())
      {
	Arc * a = it.get_curr();
	cap_sum += a->cap;
	flow_sum += a->flow;
	cost_sum += a->cost;
      }
    return make_tuple(cap_sum, flow_sum, cost_sum);
  }

/** @brief Returns a triplet of capacity,
outgoing flow and cost. */
  Pars in_pars(Node * p)
  {
    Flow_Type cap_sum = 0, flow_sum = 0, cost_sum = 0;
    for (__In_Iterator<Net> it(p); it.has_curr(); it.next())
      {
	Arc * a = it.get_curr();
	cap_sum += a->cap;
	flow_sum += a->flow;
	cost_sum += a->cost;
      }
    return make_tuple(cap_sum, flow_sum, cost_sum);
  }
};

/** @brief Arches filter for residual network */
    template <class Net>
struct Res_Filt
{
  Res_Filt(typename Net::Node *) {}
  Res_Filt() {}

  bool operator () (typename Net::Arc * a) const
  {
    return a->cap > a->flow;
  }
};

    template <class Net> 
struct Rcost
{
  Rcost () {}

  using Distance_Type = typename Net::Flow_Type;
  
    /* This is the operator to be called when Distance is run ()(a) */
  typename Net::Flow_Type operator () (typename Net::Arc * a) const
  {
    return a->cost;
  }

  static void set_zero(typename Net::Arc * a) 
  { 
    a->cap = numeric_limits<Distance_Type>::max();
    a->flow = 0;
    a->cost = 0; 
  }
};

    
    template <typename Ftype>
struct Res_Arc : public Net_Cost_Arc<Empty_Class, Ftype>
{
  using Base = Net_Cost_Arc<Empty_Class, Ftype>;
  using Base::Base;

  bool is_residual = false;
  Res_Arc * img = NULL;
};


    /* TODO: Remove and then put Empty_Class */
template <typename Ftype>
using Residual_Net = Net_Cost_Graph<Net_Node<string>, Res_Arc<Ftype>>;

/** 
  @brief Inserted in the residual waste arches residual_net network
  for the arc. src and tgt are the source and destination nodes
  respectively of the residual network.

  @return The residual reflection of a bow.
*/
template <class Res_Net> typename Res_Net::Arc * 
create_residual_arc(Res_Net & residual_net, 
		    typename Res_Net::Node * src,
		    typename Res_Net::Node * tgt,
		    const typename Res_Net::Flow_Type cap,
		    const typename Res_Net::Flow_Type flow,
		    const typename Res_Net::Flow_Type cost)
{
  assert(flow <= cap and cost >= 0);

  typename Res_Net::Arc * arc = residual_net.insert_arc(src, tgt, cap, cost);
  typename Res_Net::Arc * rarc = residual_net.insert_arc(tgt, src, cap, -cost);

  arc->is_residual = false;
  arc->flow = flow;
  arc->img = rarc;

  rarc->is_residual = true;
  rarc->img = arc;
  rarc->flow = arc->cap - arc->flow;

  assert(arc->cap == cap and arc->flow == flow and arc->cost == cost);
  assert(rarc->cap == cap and rarc->flow == cap - flow and rarc->cost == -cost);

  return arc;
}

/** 
  @brief Build a network for the net residual network.

  @return A tuple, the first
  element is the residual network. The second is a mapping from
  net arches arches towards the arches of the residual network.
*/
template <class Net>
void build_residual_net(const Net & net, 
			Residual_Net<typename Net::Flow_Type> & rnet,
			DynMapTree<void*, void*> & arcs)
{
  using Rnet = Residual_Net<typename Net::Flow_Type>;
  using Node = typename Rnet::Node;
  using Arc = typename Rnet::Arc;

    /* Walk net nodes and inserts copies in the residual network */
  for (typename Net::Node_Iterator it(net); it.has_curr(); it.next())
    {
      typename Net::Node * p = it.get_curr();
      Node * q = rnet.insert_node(p->get_info()); /* TODO: Delete get_info */
      map_nodes<Net, Rnet>(p, q); /* Temporal mapping nodes */
    }

  DynList<Node*> sources; /* Save source nodes */ 
  if (not net.is_single_source())
    sources = const_cast<Net&>(net).get_src_nodes(). 
      template map<Node*>([] (typename Net::Node * p) 
			  {
			    return (Node *) NODE_COOKIE(p); 
			  });

  DynList<Node*> sinks; /* Save sink nodes */
  if (not net.is_single_sink())
     sinks = const_cast<Net&>(net).get_sink_nodes().
       template map<Node*>([] (typename Net::Node * p) 
			   {
			     return (Node*) NODE_COOKIE(p); 
			   });

    /* Net crosses arches and arcs constructed mapping */
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next())
    {
      typename Net::Arc * ga = it.get_curr();
      typename Net::Node * gsrc = (typename Net::Node*) ga->src_node;
      typename Net::Node * gtgt = (typename Net::Node*) ga->tgt_node;

      Node * rsrc = mapped_node<Net, Rnet>(gsrc);
      Node * rtgt = mapped_node<Net, Rnet>(gtgt);
      Arc * ra = 
	create_residual_arc(rnet, rsrc, rtgt, ga->cap, ga->flow, ga->cost);
      arcs.insert(ga, ra);
    }

  if (not sources.is_empty())
    { 
      Node * source = rnet.insert_node();
      rnet.get_src_nodes().for_each([source, &rnet] (Node * p)
        {
	  typename Rnet::Pars pars = rnet.out_pars(p);
	  create_residual_arc(rnet, source, p, get<0>(pars), get<1>(pars), 0);
	});
    }

  if (not sinks.is_empty())
    { 
      Node* sink = rnet.insert_node();
      rnet.get_sink_nodes().for_each([sink, &rnet] (Node * p)
        {
	  typename Rnet::Pars pars = rnet.in_pars(p);
	  create_residual_arc(rnet, p, sink, get<0>(pars), get<1>(pars), 0);
	});
    }

  assert(check_residual_net(rnet));
} 

template <class Res_Net>
bool check_residual_net(const Res_Net & net)
{
  return
    net.all_arc([] (typename Res_Net::Arc * a) { return a->img->img == a; });
}

template <class Res_Net>
void cancel_cycle(const Res_Net & res_net, const Path<Res_Net> & path)
{
  assert(not path.is_empty() and path.is_cycle());
  using Ftype = typename Res_Net::Flow_Type;

    /* Cycle determines the slack. */
  Ftype slack = std::numeric_limits<Ftype>::max();
  path.for_each_arc([&slack] (typename Res_Net::Arc * a)
    {
      assert(a->cap -a->flow > 0);
      slack = std::min(slack, a->cap - a->flow);
    });

    /* Cancel the cycle */
  path.for_each_arc([slack] (typename Res_Net::Arc * a)
    {
      typename Res_Net::Arc * img = a->img;
      assert(img->img == a);
      assert(a->cap == img->cap);
      a->flow += slack;
      img->flow -= slack;
    });
}

/** 
  @brief receive an arcs mapping from the network residual to network
  original.

  @details Go through the arches and updates the values of flow in the
  original network
*/
template <class Net> static 
void residual_to_net(const DynMapTree<void*, void*> & arcs)
{
  using Rnet = Residual_Net<typename Net::Flow_Type>;
  arcs.for_each([] (std::pair<void*, void*> p)
		 {
		   typename Rnet::Arc * a = (typename Rnet::Arc *) p.first;
		   typename Net::Arc * ra = (typename Net::Arc *) p.second;
		   a->flow = ra->flow;
		 });
}

/** 
  @brief Calculate the maximum minimum cost flow through technique
  cycle cancellation. 

  @details This primitive uses the typical cancellation algorithm
  cycles to calculate the minimum cost maximum flow of a network
  capable of flow.

  First, the routine calculates a maximum flow by an algorithm
  flow maximization. The algorithm is a type parameter,
  called Max_Flow_Algo, instantiated on a network type
  Net generic. Once calculated maximum initial flow, the 
  algorithm searches for the residual negative cycles network using
  Bellman-Ford algorithm. For each cycle found, is
  increases the flow to remove it.

  The above procedure is repeated until no longer
  find more negative cycles. At which time the flow is
  maximum and minimum cost.

  @param[in] net Network to maximize flow and minimize cost.
  @param[in] it_factor Factor iterations in which the Bellman-Ford 
  algorithm will start searching negative cycles

  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
template 
<class Net, 
 template <class> class Max_Flow_Algo = Ford_Fulkerson_Maximum_Flow>
void max_flow_min_cost_by_cycle_canceling(Net & net, double it_factor = 0.4)
{    
  Max_Flow_Algo <Net> () (net); /* Obtains maximum initial flow */

  typename Net::Flow_Type max_flow = net.flow_value();

  using Rnet = Residual_Net<typename Net::Flow_Type>;
  Rnet rnet;
  DynMapTree<void*, void*> arcs_map;
  build_residual_net(net, rnet, arcs_map);

  assert(check_residual_net(rnet));

  using BF = 
    Bellman_Ford<Rnet, Rcost<Rnet>, Arc_Iterator, Out_Iterator, Res_Filt<Rnet>>;
  while (true)
    {
      Path<Rnet> cycle = BF(rnet).search_negative_cycle(); 
      if (cycle.is_empty())
	break;
      
      cancel_cycle(rnet, cycle);
    }
  residual_to_net<Net>(arcs_map);
}


template <class Net>
void print(const Net & net, ostream & out)
{
  long i = 0;
  net.nodes().for_each([&i] (typename Net::Node* p) { NODE_COUNTER(p) = i++; });

  struct Show_Node
  {
    void operator () (const Net &, typename Net::Node * p, ostream & out)
    {
      out << "label = \"(" << p->get_info() << "," << NODE_COUNTER(p) << ")\"";
    }
  };

  struct Show_Arc
  {
    void operator () (const Net &, typename Net::Arc * a, ostream & out)
    {
      out << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
    }
  };

  To_Graphviz<Net, Show_Node, Show_Arc> ().digraph (net, out);
}

template <class Net>
void print(const Residual_Net<typename Net::Flow_Type> & net, ostream & out)
{
  long i = 0;
  net.nodes().for_each([&i] (typename Net::Node* p) { NODE_COUNTER(p) = i++; });

  struct Show_Node
  {
    void operator () (const Net &, typename Net::Node * p, ostream & out)
    {
      out << "label = \"(" << p->get_info() << "," << NODE_COUNTER(p) << ")\"";
    }
  };

  struct Show_Arc
  {
    void operator () (const Net &, typename Net::Arc * a, ostream & out)
    {
      out << "label = \"" << a->flow << "/" << a->cap << "/" << a->cost << "\"";
      if (a->is_residual)
	out << " color = red";
    }
  };

  To_Graphviz<Net, Show_Node, Show_Arc, Dft_Show_Node<Net>, Res_Filt<Net>>().
    digraph (net, out);
}
  

    template <class Net>
struct SimplexInfo
{
  typename Net::Flow_Type potential;
  long valid = 0;
};

template <class Net> static inline
void init_simplex_info(Net & net)
{

}


    /* 1st field: Empty arcs, 
       2nd field: Full arcs, 
       3rd field: Partial Arcs */
  template <class Net>
using Feasible_Tree = tuple<DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>, 
			    DynList<typename Net::Arc*>>;
								

template <class Net> static inline
Feasible_Tree<Net> build_feasible_spanning_tree(const Net & net)
{
  using Arc = typename Net::Arc;
  DynList<Arc*> empty, full, partial;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next())
    {
      Arc * a = it.get_curr();
      if (a->flow == 0) /* Empty arc? */
	empty.append(a);
      else if (a->flow == a->cap) /* Full arc? */
	full.append(a);
      else
	partial.append(a);
    }
  return make_tuple(move(empty), move(full), move(partial));
}


template <class Net> static inline 
DynList<typename Net::Arc*> get_partial_arcs(const Net & net)
{
  DynList<typename Net::Arc*> ret;
  for (typename Net::Arc_Iterator it(net); it.has_curr(); it.next())
    {
      typename Net::Arc * a = it.get_curr();
      if (a->flow > 0 and a->flow < a->cap)
	ret.append(a);
    }
  return ret;
}


# define POTENTIAL(p) (NODE_COUNTER(p))


template <class Net>
typename Net::Flow_Type rcost(typename Net::Node * src, 
			      typename Net::Node * tgt)
{
  for (Out_Iterator<Net> it(src); it.has_curr(); it.next())
    {
      typename Net::Arc * a = it.get_curr();
      if (a->tgt_node == tgt)
	return a->cost - (POTENTIAL(src) - POTENTIAL(tgt));
    }
  ERROR("Arc not found");
}


template <class Net>
typename Net::Flow_Type rcost(typename Net::Arc * a)
{
  using Node = typename Net::Node*;
  return a->cost - 
  (POTENTIAL((Node*)a->src_node) - POTENTIAL((Node*)a->tgt_node));
}




# undef POTENTIAL

} /* End namespace Aleph */

# endif /* TPL_MAXFLOW_MINCOST_H
