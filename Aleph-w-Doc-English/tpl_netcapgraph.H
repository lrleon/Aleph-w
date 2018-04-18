 
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
  @file tpl_netcapgraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_NETCAPGRAPH_H
# define TPL_NETCAPGRAPH_H
# include <tpl_netgraph.H>

namespace Aleph {

/** 
  @brief Node with capacity.

  @details A node capacitance value specifies a maximum value of 
  flow node can have as inflow and outflow.

  @ingroup Grafos Redes
*/
    template <typename Node_Info, typename F_Type = double>
class Net_Cap_Node : public Net_Node<Node_Info, F_Type>
{
public:

/** @brief  Type that representing the flow */
  typedef F_Type Flow_Type;

  Flow_Type max_cap; 

  Net_Cap_Node() : max_cap(numeric_limits<Flow_Type>::max()) {}

/** @brief Constructor receives node information saved. The values
attribute flow capacity and are initiated to zero. */
  Net_Cap_Node(const Node_Info & node_info) 
    : Net_Node<Node_Info, F_Type>(node_info), 
      max_cap(numeric_limits<Flow_Type>::max())
  { /* Empty */ }

/** @brief Node copy constructor. The values
attribute flow capacity and are initiated to zero. */
  Net_Cap_Node(Net_Cap_Node * node) 
    : Net_Node<Node_Info, F_Type>(*node), max_cap(node->max_cap) { /* Empty */ }
}; 

/** 
  @brief Network trained with capacity limits nodes.

  @details Net_Cap_Graph defines a trained network in which nodes
  they limit incoming or outgoing flow.

  The class takes two type parameters:
    -# NodeT: Type network node, which must be
    descendant of the Net_Node class.
    -# ArcT: Arc type network, which must be
    descendant of the Net_Arc class.

  A network is defined by Net_Cap_Graph unusable by
  flow maximization algorithms. Instead, it can be obtained
  a network of Net_Graph equivalent type. The network is equivalent
  Net_Cap_Graph::Aux_Net type, which is fundamentally Net_Graph
  without capacity nodes.

  The equivalent network can be calculated by compute_aux_net().

  @see Net_Graph compute_aux_net()

  @ingroup Redes
*/
    template <class NodeT, class ArcT>
class Net_Cap_Graph : public Net_Graph<NodeT, ArcT>
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

/** 
  @brief Create a new node and inserts trained network.

  @param[in] node_info Information stored in the node as
  as it has been defined when the node specification.

  @param[in] cap Maximum flow value that can receive or stop the
  node. By default this value is set to maximum.
  
  @return A pointer to the node created and inserted into the network.

  @throw bad_alloc If there is insufficient memory.
*/
  Node * insert_node(const Node_Type & node_info, 
                     const Flow_Type & cap = numeric_limits<Flow_Type>::max())
  {
    Node * p = Net_Class::insert_node(node_info);
    p->max_cap = cap;
    return p;
  }

/** 
  @brief The type of traditional network equivalent network nodes capabilities.

  @details Net_Graph::Aux_Net represents the type of network capable
  equivalent network capabilities in nodes
  (Net_Cap_Graph).

  To get the equivalent network is invoked compute_aux_net(). The network
  result is mapped as arches in both nodes.

  Net_Cap_Graph a node is mapped to a value of arc
  node capacity. Given a "p" node in Net_Cap_Graph, the arc in
  Aux_Net is obtained through NODE_COOKIE(p).

  An arch in Net_Cap_Graph is mapped to an arc with the same value
  capacity. An arch in Aux_Net has a single logical attribute. Yes
  the value to true, then the arc mapped to a node, whose value
  capability is the same as the capacity of the node. On the contrary,
  the maps arc to another arc.
*/
    typedef Net_Graph<Net_Node<Empty_Class, Flow_Type>, 
                      Net_Arc<bool, Flow_Type> > Aux_Net;
  private:
    Aux_Net * aux_net;
  public:
    Net_Cap_Graph() : aux_net(NULL) { /* Empty */ }

/** @brief Returns a pointer to the equivalent of "this" network.
It must have been created previously by  */
  Aux_Net * get_aux_net() { return aux_net; }
      
/** 
  @brief Calculates equivalent traditional network to the capabilities in the 
  nodes.

  @details Given a network with capacity in the nodes (displayed in "this"),
  compute_aux_net() calculates the equivalent of traditional network
  which can perform calculations of maximizing flow,
  equivalent cut, etc.

  @return A pointer to the equivalent network.

  @throw bad_alloc If there is insufficient memory para crear la
  red equivalente.
  @throw domain_error If the network equivalent has been calculated.
*/
  Aux_Net * compute_aux_net()
  {
    if (aux_net != NULL)
      throw std::domain_error("aux_net has already computed");
    aux_net = new Aux_Net;
    try
      {
        for (Node_Iterator<Net_Cap_Graph> it(*this); it.has_current(); it.next())
          {
            Node * p = it.get_current();
            typename Aux_Net::Node * src = aux_net->insert_node();
            typename Aux_Net::Node * tgt = aux_net->insert_node();
            typename Aux_Net::Arc  * arc = 
              aux_net->insert_arc(src, tgt, true, p->max_cap, 0);
            NODE_COOKIE(p)  = arc;
            ARC_COOKIE(arc) = p;
          }
        for (Arc_Iterator<Net_Cap_Graph> it(*this); it.has_current(); it.next())
          {
            Arc * arc = it.get_current();
            typename Aux_Net::Arc * src_arc = 
              (typename Aux_Net::Arc *) NODE_COOKIE(get_src_node(arc));
            typename Aux_Net::Arc * tgt_arc = 
              (typename Aux_Net::Arc *) NODE_COOKIE(get_tgt_node(arc));
            typename Aux_Net::Arc * a = 
              aux_net->insert_arc(aux_net->get_tgt_node(src_arc), 
                                  aux_net->get_src_node(tgt_arc), false, 
                                  arc->cap, arc->flow);
            ARC_COOKIE(arc) = a;
            ARC_COOKIE(a)   = arc;
          }
        return aux_net;
      }
    catch (...)
      {
        free_aux_net();
      }
  }
/** 
  @brief Updates the values of flow equal to "this" network.

  @details update() This covers the capacity network equivalent, previously
  calculated by compute_aux_net(), and updates the values of
  flow over the "this" network.

  update() It must be called before set free the equivalent network,
  order to reflect on "this" calculations on the network
  equivalent. For example, if you want to maximize the flow of "this"
  then the equivalent network flow is maximized and then
  values are updated on "this" through update().

  @throw domain_error If the equivalent network has not been created.
*/
  void update()
  {
    if (aux_net == NULL)
      throw std::domain_error("Auxiliar net has not been generated");

    for (Arc_Iterator<Aux_Net, No_Res_Arc<Aux_Net> > it(*aux_net); 
         it.has_current(); it.next())
      {
        typename Aux_Net::Arc * arc = it.get_current();
        if (arc->get_info())
          {
            Node * p = (Node*) ARC_COOKIE(arc);
            p->in_flow = p->out_flow = arc->flow;
          }
        else
          {
            Arc * a = (Arc*) ARC_COOKIE(arc);
            a->flow = arc->flow;
          }
      }
  }

/** @brief Frees the memory of the equivalent network. Shoot exception
domain_error if the equivalent network has not been created. */
  void free_aux_net()
  {
    if (aux_net == NULL)
      throw std::domain_error("Auxiliar net has not been generated");

    clear_graph(*aux_net);
    delete aux_net;
    aux_net = NULL;
  }

  ~Net_Cap_Graph()
  {
    if (aux_net != NULL)
      free_aux_net();
  }
};

} /* End namespace Aleph */

# endif // endif
