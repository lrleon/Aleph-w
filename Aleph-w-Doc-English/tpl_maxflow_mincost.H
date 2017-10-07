
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
  @file tpl_maxflow_mincost.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_MAXFLOW_MINCOST_H
# define TPL_MAXFLOW_MINCOST_H

# include <Bellman_Ford.H>
# include <tpl_netgraph.H>

namespace Aleph {


/** 
  @brief Arc definition for network of maximum flow at minimum cost.

  @details Net_Cost_Arc models a arc that belongs to a network flow
  costs associated with their arcs.

  The class manages two type parameters:
  -# Arc_Info: Type representing the attributes associated with the arc.
  -# F_Type: Data type that represents the capacity,
  flow and cost. This standard has the same as for
  accumulated flow of the nodes.

  @ingroup Grafos Redes
*/                               
    template <typename Arc_Info, typename F_Type = double>
class Net_Cost_Arc : public Net_Arc<Arc_Info, F_Type>
{ 
public:

/** @brief Type that representing the flow. */
  typedef F_Type Flow_Type;

/** @brief Cost per unit of flow. */
  Flow_Type cost;

/** @brief Returns the cost of circulating flow. */
  Flow_Type flow_cost() const { return this->flow*cost; }

  Net_Cost_Arc() :  cost(0) { /* Empty */ }

  Net_Cost_Arc(const Arc_Info & info) 
    :  Net_Arc<Arc_Info, F_Type>(info), cost(0) 
  { /* Empty */ }
};

/** 
  @brief Defining of network capable of flow with costs associated with
  flows.  

  @details This type derived from Net_Graph, modeled a trained network
  which each arc is defined a cost per unit of flow. east
  model, network extension skilled expands considerably
  the range of combinatorial optimization problems to raise a
  max-min optimization type. In this case, the maximum cost flow
  minimum.

  Being bypass Net_Graph, its interface is very similar, with the
  addition of parameters and methods for managing the cost of each
  arc.

  @see max_flow_min_cost_by_cycle_canceling Max_Flow_Min_Cost

  @ingroup Redes
*/
    template <class NodeT, class ArcT>
class Net_Max_Flow_Min_Cost : public Net_Graph<NodeT, ArcT>
{
public:

/** @brief The type of network flow on which this model is based cost. */ 
  typedef Net_Graph<NodeT, ArcT> Net;

/** @brief The directed graph type. */
  typedef typename Net::Digraph Digraph;

  typedef Net_Max_Flow_Min_Cost<NodeT, ArcT> Net_MFMC;

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
    Arc * a = Net::insert_arc(src_node, tgt_node, Arc_Type(), cap, 0);
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

/** @brief Calculate the total cost of circulating flow for the net */
  Flow_Type compute_flow_cost()
  {
    Flow_Type total = 0;
    for (Arc_Iterator<Net_MFMC> it(*this); it.has_current(); it.next())
      {
	Arc * a = it.get_current();
	if (not a->is_residual)
	  total += a->flow_cost();
      }

    return total;
  }
};

    /* TODO Documentar para que se usa esta clase. */
    template <class Net> 
class Access_Cost
{
public:

  typedef typename Net::Flow_Type Distance_Type;

  static const Distance_Type Max_Distance;

  static const Distance_Type Zero_Distance;

  typename Net::Flow_Type operator () (typename Net::Arc * a) const
  {
    return a->is_residual ? -((typename Net::Arc *) a->img_arc)->cost : a->cost;
  }

  static void set_zero(typename Net::Arc * a) { a->cost = 0; }
};

  template <class Net> 
const typename Access_Cost<Net>::Distance_Type 
  Access_Cost<Net>::Zero_Distance = 0;


    template <class Net>
  const typename Access_Cost<Net>::Distance_Type 
Access_Cost<Net>::Max_Distance = 
    numeric_limits<typename Access_Cost::Distance_Type>::max();


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
  @param[in] leave_residual It indicates whether or not residual network
  removed. By default, the value is false.
  
  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
    template <class Net, template <class> class Max_Flow_Algo>
void max_flow_min_cost_by_cycle_canceling
  (Net & net, bool leave_residual = false)
{    /* Tipos sinónimos para hacer más simple la lectura */
  typedef Res_F<Net> Filter;
  using BF = Bellman_Ford <Net, Access_Cost<Net>, Arc_Iterator, 
			   Node_Arc_Iterator, Filter> ;


  Max_Flow_Algo <Net> () (net, true); /* Obtains maximum initial flow */

  Path<Net> cycle(net); /* Here negative cycles are stored */
  
    /** Remove negative cycles while they exist */
  while (BF (net)(cycle))
    if (increase_flow <Net> (net, cycle) == 0)
      break;

  if (not leave_residual)
    net.unmake_residual_net();
}

    template <class Net, template <class> class Cost> static 
typename Net::Flow_Type search_max_arc_cost(Net & net)
{
  Arc_Iterator<Net> it(net);
  typename Net::Flow_Type max_cost = Cost <Net> () (it.get_current());
  for (it.next(); it.has_current(); it.next())
    {
      typename Net::Flow_Type curr_cost = Cost <Net> () (it.get_current());
      if (curr_cost > max_cost)
        max_cost = curr_cost;
    }

  return max_cost;
}

    template <class Net> static 
typename Net::Flow_Type compute_max_possible_flow(Net & net)
{
  typename Net::Node * source = net.get_source();
  typename Net::Node * sink   = net.get_sink();

  return std::min(net.get_out_cap(source), net.get_in_cap(sink));
}

    template <class Net, template <class> class Cost> static
typename Net::Arc * create_dummy_arc(Net & net)
{
  const typename Net::Flow_Type max_cost = 
    net.get_num_nodes() * search_max_arc_cost <Net, Cost> (net);

  net.make_residual_net();

  typename Net::Node * src = net.get_source();
  typename Net::Node * tgt = net.get_sink();
  const typename Net::Flow_Type max_flow = 
    std::min(net.get_out_cap(src), net.get_in_cap(tgt));
  typename Net::Net::Digraph * digraph = &net;
  typename Net::Arc * a = digraph->insert_arc(src, tgt); 
  net.get_cookie() = a;

  try 
    {
      typename Net::Arc * img = digraph->insert_arc(tgt, src); 

      digraph->disconnect_arc(a); /* No supra-arco, dejarlo por coste */

      a->is_residual = false;
      a->img_arc     = img;
      a->cap         = max_flow;
      a->cost        = max_cost;
      a->flow        = 0;

      img->is_residual = true;
      img->img_arc     = a;
      img->cap         = max_flow;
      img->cost        = max_cost;
      img->flow        = 0;

      return img;

    }
  catch (...)
    {
      digraph->remove_arc(a);
      throw;
    }
}

    template <class Net> static
void destroy_dummy_arc(Net & net)
{
  typename Net::Arc * a = (typename Net::Arc *) net.get_cookie();
  typename Net::Arc * img = (typename Net::Arc *) a->img_arc;
  typename Net::Net::Digraph * digraph = &net;

  digraph->connect_arc(a);
  digraph->remove_arc(img);
  digraph->remove_arc(a);

  net.unmake_residual_net();
}

/** 
  @brief Calculate the maximum minimum cost flow through technique
  cycle cancellation. 

  @details This primitive uses the typical cancellation algorithm
  cycles to calculate the minimum cost maximum flow of a network
  capable of flow.

  The routine creates a fictitious negative cycle by addition of a
  negative arc, with sufficient capacity between the source and the
  sump. From this negative cycle, the algorithm searches for the 
  residual negative cycles network using Bellman-Ford. For each 
  cycle found, is increases the flow to remove it.

  The above procedure is repeated until no longer
  find more negative cycles. At which time the flow is
  maximum and minimum cost.

  @param[in] net Network to maximize flow and minimize cost.

  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
    template <class Net>
void max_flow_min_cost_by_cycle_canceling(Net & net)
{
  typedef Res_F<Net> Res_Filter;
  using BF = Bellman_Ford_Negative_Cycle<Net, Access_Cost<Net>, Arc_Iterator, 
					 Node_Arc_Iterator, Res_Filter> ;

  create_dummy_arc <Net, Access_Cost> (net);

  Path<Net> cycle(net); /* Here negative cycles are stored */
  while (BF () (net, cycle))
    if (increase_flow <Net> (net, cycle) == 0)
      {
	((typename Net::Arc *) net.get_cookie())->cost = 0;
	break;
      }

  destroy_dummy_arc(net);
}

/** 
  @brief Class calculation of maximum flow maximum cost.

  @details This class type parameter used as a network of flow
  and it provides various primitives costs to maximize flow
  minimum cost.

  @ingroup Grafos Redes
*/
    template <class Net>
class Max_Flow_Min_Cost
{
public:

/** 
  @brief Calculate the maximum minimum cost flow through technique
  cycle cancellation. 

  @details This primitive uses the typical cancellation algorithm
  cycles to calculate the minimum cost maximum flow of a network
  capable of flow.

  The routine creates a fictitious negative cycle by addition of a
  negative arc, with sufficient capacity between the source and the
  sump. From this negative cycle, the algorithm searches for the 
  residual negative cycles network using Bellman-Ford. For each 
  cycle found, is increases the flow to remove it.

  The above procedure is repeated until no longer
  find more negative cycles. At which time the flow is
  maximum and minimum cost.

  @param[in] net The network to maximize flow and minimize cost.

  @throw bad_alloc If there is insufficient memory.
*/
  void by_cycle_canceling_and_dummy_arc(Net & net) const
  {
    max_flow_min_cost_by_cycle_canceling(net);
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
  @param[in] leave_residual It indicates whether or not residual network
  removed. By default, the value is false.

  @throw bad_alloc If there is insufficient memory.
*/
      template <template <class> class Max_Flow_Algo>
  void by_cycle_canceling(Net & net, bool leave_residual = false)
  {
    max_flow_min_cost_by_cycle_canceling<Net, Max_Flow_Algo>
      (net, leave_residual);
  }
};


} /* End namespace Aleph */

# endif /* TPL_MAXFLOW_MINCOST_H
