 
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
  @file tpl_net_sup_dem.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/

#line 620 "reducciones.nw"
# ifndef TPL_NET_SUP_DEM_H
# define TPL_NET_SUP_DEM_H

# include <tpl_netgraph.H>

namespace Aleph {

#line 586 "reducciones.nw"
/** 
  @brief Node with provision of flow.

  @ingroup Grafos Redes
*/
    template <typename Node_Info, typename F_Type = long>
class Net_Sup_Dem_Node : public Net_Node<Node_Info, F_Type>
{

public:

/** @brief Type that representing the flow. */
  typedef F_Type Flow_Type;

/** @brief Providing value (positive) or demand (negative) */
  Flow_Type supply_flow; 

/** @brief Returns the value of providing the node. */
  Flow_Type & get_supply_flow() { return supply_flow; }

  Net_Sup_Dem_Node() : supply_flow(0) { /* Empty */ }

/** @brief Constructor that receives information of node saved.
The attributes values and flow capacity are initiated to zero. */
  Net_Sup_Dem_Node(const Node_Info & node_info) 
    : Net_Node<Node_Info, F_Type>(node_info), supply_flow(0) { /* Empty */ }

/** @brief Node copy constructor. The values
attribute flow capacity and are initiated to zero. */
  Net_Sup_Dem_Node(Net_Sup_Dem_Node * node) 
    : Net_Node<Node_Info, F_Type>(*node), supply_flow(node->supply_flow) { /* Empty */ }
};

#line 629 "reducciones.nw"
  
/** 
  @brief Network flow provisions.

  @details A network flow is a provision in which some nodes
  They have a value of supply flow. If the value is positive,
  it means that the node provides a maximum flow corresponding to
  given value. If the value is negative, it means that the node
  required receive for his arch and a corresponding flow
  the absolute value of the given value.

  In a network of provisions, it is said that the flow is feasible if
  You can meet the demands from the
  provisions. To do this, calculate a trained network
  equivalent and calculate the maximum flow.

  @ingroup Grafos Redes
*/
    template <class NodeT = Net_Sup_Dem_Node<Empty_Class, double>, 
	      class ArcT  = Net_Arc<Empty_Class, double> >
class Net_Sup_Dem_Graph : public Net_Graph<NodeT, ArcT>
{
public:
/** @brief The class of traditional network. */
  typedef Net_Graph<NodeT, ArcT> Net_Class;
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

  
#line 675 "reducciones.nw"
  
/** 
  @brief Creates a provision node and inserts it into a network.

  @param[in] node_info Node attribute value.
  @param[in] supply Providing flow value. If the value is
  positive, then the node is a provider of value flow
  given; otherwise, the node given the quantity demand. For
  default, this parameter is zero.
      
  @return A pointer to node created and inserted.
      
  @throw bad_alloc if there is insufficient memory.
*/
Node * insert_node(const Node_Type & node_info, const Flow_Type & supply = 0)
{
  Node * p = Net_Class::insert_node(node_info);
  p->supply_flow = supply;
  return p;
}

/** 
  @brief Creates a provision node and inserts it into a network.

  @param[in] supply Providing flow value. If the value is
  positive, then the node is a provider of value flow
  given; otherwise, the node given the quantity demand. For
  default, this parameter is zero.

  @return A pointer to node created and inserted.

  @throw bad_alloc If there is insufficient memory.
*/
Node * insert_node(const Flow_Type & supply = 0)
{
  return insert_node(Node_Type(), supply);
}
#line 719 "reducciones.nw"
private:

  Node * super_source;
  Node * super_sink;

public:
  Net_Sup_Dem_Graph() : super_source(NULL), super_sink(NULL) { /* Empty */ }

/** @brief Returns true if the auxiliary network has already been created. */
  bool exist_aux_net() const 
  { 
    return super_source != NULL or super_sink != NULL; 
  }

#line 739 "reducciones.nw"

/** @brief The type of auxiliary network trained equivalent. */
typedef Net_Sup_Dem_Graph Aux_Net;

/** 
  @brief Calculate the equivalent trained network.

	@details Given a supply network nodes (represented in "this"),
  compute_aux_net() calculates the equivalent network trained on
  which can perform calculations of maximizing flow,
  equivalent cut and, in particular, check feasibility
  flow.

	The equivalent trained network is built on the network itself
  of provisions. For that reason, any calculation on the net
  equivalent is immediately reflected on the network
  provisions. 

	@return A pointer to the equivalent network.

	@throw bad_alloc if there is not enough memory to create the
  equivalent network.
	@throw domain_error if the network has already been equivalent
  calculated.
	@throw domain_error if the residual equivalent network and
  it has been calculated.
*/
Net_Sup_Dem_Graph * compute_aux_net()
{
  if (exist_aux_net())
    throw std::domain_error("Auxiliar net is already computed");
  if (this->residual_net)
    throw std::domain_error("Residual net has been already computed");

  super_source = insert_node();   /* Fuente auxiliar */
  super_sink   = insert_node();   /* Sumidero auxiliar */
  
    /* Recorrer nodos y atar arcos a fuente o sumidero según signo provisión */
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_current(); it.next())
    {
      Node * p = it.get_current();
      if (p->supply_flow > 0) /* ¿Nodo proveedor? */
	{
 	  if (p->out_cap < p->supply_flow)
 	    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
						    "greater than out capacity", p));
        insert_arc(super_source, p, p->supply_flow);
	}
      else if (p->supply_flow < 0) /* ¿Nodo demandante? */
	{
 	  if (p->in_cap < -p->supply_flow)
 	    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
						    "smaller than in capacity", p));
        insert_arc(p, super_sink, -p->supply_flow);
	}
    }
  if (this->get_out_degree(super_source) == 0)
    {
      remove_node(super_source);
      super_source = NULL;
    }
   if (this->get_in_degree(super_sink) == 0)
    {
      remove_node(super_sink);
      super_sink = NULL;
    }
  return this;
}
#line 825 "reducciones.nw"
  Net_Sup_Dem_Graph * get_aux_net() { return exist_aux_net() ? this : NULL; }
#line 833 "reducciones.nw"
    
/** 
  @brief Runs the network of supplies and determines whether the flow is
  feasible.

	@return true if the flow is feasible; false otherwise.
	
  @throw domain_error If the network has not trained equivalent
  previously calculated.
*/
bool is_feasible() const
{
  if (not exist_aux_net())
    throw std::domain_error("Auxiliar net has not been computed");
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_current(); it.next())
    {
      Node * p = it.get_current();
      Flow_Type & supply_flow = p->supply_flow;
      if (supply_flow == 0)
	continue;
      if (supply_flow > 0 and p->out_flow < supply_flow)
	return false;
      else if (supply_flow < 0 and p->in_flow < -supply_flow)
	return true;
    }
  return true;
}

/** 
  @brief Estimated node whose flow is not feasible.

  @details non_feasible_nodes() travels the network nodes. For each node,
  by provision value, it is checked if it is satisfied. Yes
  it is not possible to satisfy the provision, then the direction of
  node is stored in a list according to the node demand or
  provision.

  @param[out] supply_list List of nodes whose outflow is
  less than what they can provide.
  @param[out] demand_list List of nodes whose inflow is
  less than they receive.

  @throw bad_alloc If there is insufficient memory to insert
  in one of the lists parameters.
*/
void non_feasible_nodes(DynDlist<Node *> & supply_list, DynDlist<Node *> & demand_list)
{
  for (Node_Iterator<Net_Sup_Dem_Graph> it(*this); it.has_current(); it.next())
    {
      Node * p = it.get_current();
      Flow_Type & supply_flow = p->supply_flow;
      if (supply_flow == 0)
	continue;
      if (supply_flow > 0 and p->out_flow < supply_flow)
	supply_list.append(p);
      else if (supply_flow < 0 and p->in_flow < -supply_flow)
	demand_list.append(p);
    }
}
#line 895 "reducciones.nw"
/** 
  @brief Sets the value of providing a node.

  @details set_supply_flow(p, supply) places the provision value of "supply"
  on the node pointed to by the pointer "p".

  @param[in] p Pointer to the node.
  @param[in] supply Providing a value placed on the node.

  @note once made any adjustments on the provision is
  necessary to recalculate the maximum flow if you want to know the
  flow feasibility.

  @throw range_error If the value is not feasible provision
  the incoming or outgoing node capacity.
*/
void set_supply_flow(Node * p, const Flow_Type & supply)
{
  if (supply > 0 and p->out_cap < supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "greater than out capacity", p));
  else if (supply < 0 and p->in_cap < -supply)
    throw std::range_error(gnu::autosprintf("Supply flow in node at %p is "
					    "smaller than in capacity", p));
  p->supply_flow = supply;
}
#line 922 "reducciones.nw"

/** @brief Frees memory of the extended network. */
void free_aux_net()
{
  if (not exist_aux_net())
    throw std::domain_error("Auxiliar net has not been computed");

  if (this->residual_net)
    {
      this->unmake_residual_net();
      this->unmake_super_nodes();
    } 
  if (super_source != NULL)
    {
      remove_node(super_source);
      super_source = NULL;
    }
  if (super_sink != NULL)
    {
      remove_node(super_sink);
      super_sink = NULL;
    }
}
#line 664 "reducciones.nw"
};

} /* End namespace Aleph */

# endif /* TPL_NET_SUP_DEM_H
