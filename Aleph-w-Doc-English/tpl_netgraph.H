 
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
  @file tpl_netgraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_NETGRAPH_H
# define TPL_NETGRAPH_H

# include <limits>
# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>
# include <tpl_random_queue.H>
# include <Set.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>

/**
  @defgroup Redes Redes de Flujo.

  @{ Redes de flujo

  A flow network is a digraph directed that abstracts a network of
  pipes through which a fluid flows. The arcs represent pipes;
  joints nodes. Each arc attaches two numeric values:
  associated with each arc:
  - # Capacity: Maximum flow value that can flow through the arc.
  - # Flow value: Current value of the flow through the arc.
  .

  A flow network it is also known as skilled network. In a
  trained special network nodes called "sources" are identified
  and "sinks", respectively. A source node is one that has
  archways; consequently, only flow emanates. Similarly,
  a sink node is one that has no output arcs; in the same
  sense, only receives flow.

  Since abstractions come from the physical theory, for each
  non-source node or sink must always meet the
  total amount of inflow must be the same as the output.

  The trained networks are an excellent vehicle for resolving
  problems on graphs. Apart from the problem of maximum flow,
  representable in many real-life situations, a network
  it trained used to solve other problems of optimization
  combinatorial: shortest path, matches, cuts, etc.

  @}
*/
using namespace Aleph;

namespace Aleph {

template <class N> class Res_F;


/** 
  @brief Arc of a network of flow implemented by adjacency lists. 
        
  @details Net_Arc models a arc of a trained network with information of
  Arc_Info type and numerical F_Type type to represent ability
  and flow. 

  The Net_Arc class is only intended to model the network. It did not
  you must be instantiated. Wear insert_arc() of the Net_Graph class
  arches to create a network.

  @see Array_Digraph Net_Graph Net_Node 

  @ingroup Redes
*/
  template <typename Arc_Info, typename F_Type = double>
class Net_Arc : public Graph_Aarc<Arc_Info>
{
public:

/** @brief Type that representing the flow. */
  typedef F_Type Flow_Type;

/** @brief Capacitance value. */
  Flow_Type cap; 

/** @brief Flow value. */
  Flow_Type flow; 

/** @brief Returns true if the values of capacity and flow of arc
satisfy the conditions of flow
(equal to or lower flow or the capacity). */
  bool check_arc() const { return flow >= 0 and flow <= cap; }

/** @brief Points to the reflex arc */
  Net_Arc * img_arc;          

/** @brief It indicates whether the arc is no residual */
  bool is_residual;      

  Net_Arc() : img_arc(NULL), is_residual(false) { /* Empty */ } 

  Net_Arc(Net_Arc * net_arc) 
    : Graph_Aarc<Arc_Info>(*net_arc), cap(net_arc->cap), flow(net_arc->flow), 
      img_arc(NULL), is_residual(false) 
  {
    /* Empty */ 
  }

  Net_Arc(const Arc_Info & info)
    :  Graph_Aarc<Arc_Info>(info), img_arc(NULL), is_residual(false) 
  { /* Empty */ } 

  Net_Arc & operator = (Net_Arc & arc)
  {
    if (this == &arc) 
      return *this;

    this->get_info() = arc.get_info();
    cap             = arc.cap;
    flow            = arc.flow;
    img_arc         = NULL;
    is_residual     = false;

    return *this;
  }
};

/** 
  @brief Node of a network flow implemented by adjacency lists.

  @details Net_Node modeled on a trained network node. The class
  drift of Graph_Node, allowing access methods
  Graph_Node. The class manages two parameterized types:
  -# Node_Info: Information to be included in the node.
  -# F_Type: Integrated numeric type that define the values 
  of capacity and flow of the network. This parameter is default
  type "long" and it is strongly recommended that this type is used
  to model the flow.

  For convenience of implementation, manages several Net_Node
  public attributes should only be used as read. Use the
  observer methods for portability and security.

  The Net_Node class is only intended to model the network. It did not
  you must be instantiated. Wear insert_arc() of the Net_Graph class
  arches to create a network.

  @see Array_Digraph Net_Graph Net_Arc

  @ingroup Redes
*/
    template <typename Node_Info, typename F_Type = double>
class Net_Node : public Graph_Anode<Node_Info>
{
public:

/** @brief Type that representing the flow. */
  typedef F_Type Flow_Type;

/** @brief amount of incoming arcs. */
  size_t in_degree;   

/** @brief Total Output Capacity
(sum of output arcs "caps"). */
  Flow_Type out_cap;  

/** @brief Total input capacity
(sum of input arcs "caps") */
  Flow_Type in_cap;   

/** @brief Outflow value. */
  Flow_Type out_flow; 

/** @brief Inflow value. */
  Flow_Type in_flow;  

  Net_Node() : in_degree(0), out_cap(0), in_cap(0), out_flow(0), in_flow(0) 
  {
    /* Empty */
  }

/** @brief Constructor that receives information of node saved.
The attributes values and flow capacity are initiated to zero. */
  Net_Node(const Node_Info & node_info) 
    : Graph_Anode<Node_Info>(node_info), in_degree(0),
      out_cap(0), in_cap(0), out_flow(0), in_flow(0) 
  {
    /* Empty */ 
  }

/** @brief Node copy constructor. 
The attributes values of capacity and 
flow are initiated to zero. */
  Net_Node(Net_Node * net_node) 
    : Graph_Anode<Node_Info>(*net_node), in_degree(net_node->in_degree),
      out_cap(net_node->out_cap), in_cap(net_node->in_cap), 
      out_flow(net_node->out_flow), in_flow(net_node->in_flow) 
  {
    /* Empty */ 
  }
};

/** 
  @brief Network flow implemented by adjacency lists.

  @details The Net_graph type modeled a trained network, principal
  instrument and maximum flow calculating and vehicle of a
  extensive and important family of graph algorithms. 

  The class takes two type parameters:
  -# NodeT: Type network node, which must be
  descendant of the Net_Node class.
  -# ArcT: Arc type network, which must be
  descendant of the Net_Arc class. 

  @see Array_Digraph Net_Node Net_Arc ford_fulkerson_maximum_flow() edmonds_karp_maximum_flow() generic_preflow_vertex_push_maximum_flow() fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow() generic_preflow_edge_maximum_flow() depth_first_preflow_edge_maximum_flow() breadth_first_preflow_edge_maximum_flow() priority_first_preflow_edge_maximum_flow() random_first_preflow_edge_maximum_flow() 
  
  @ingroup Redes
*/
  template <class NodeT = Net_Node<Empty_Class, double>, 
            class ArcT  = Net_Arc<Empty_Class, double> >
class Net_Graph : public Array_Digraph<NodeT, ArcT>
{
public: 

  typedef Array_Digraph<NodeT, ArcT> Digraph;

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

public:

  mutable Flow_Type Infinity;

/** @brief Returns the input total capacity of the "node" node. */
  Flow_Type get_in_cap(Node * node) const { return node->in_cap; }

/** @brief Returns the total capacity output of "node" node. */
  Flow_Type get_out_cap(Node * node) const { return node->out_cap; }

/** @brief Returns the degree of input node
(number of arcs that affect it) */
  size_t get_in_degree(Node * node) const { return node->in_degree; }

/** @brief Returns the degree of output node
(number of arcs leaving him) */
  size_t get_out_degree(Node * node) const { return get_num_arcs(node); }

/** @brief Returns the value of outflow of the node. */
  Flow_Type get_out_flow(Node * node) const { return node->out_flow; }

/** @brief Returns the value of inflow node. */
  Flow_Type get_in_flow(Node * node) const { return node->in_flow; }

/** @brief Returns true if node is a source. */
  bool is_source(Node * node) { return src_nodes.count(node) > 0; }

/** @brief Returns true if node is sink. */
  bool is_sink(Node * node)  { return sink_nodes.count(node) > 0; }

/** @brief Returns true if the node is connected node. The purpose of this
validation method is as part of a verification
network is connected. */
  bool is_connected(Node * node) const 
  {
    return node->in_degree != 0 or node->num_arcs != 0;
  }

/** @brief Returns true if the "node" node satisfies the conditions of flow;
ie, that the inflow is equal to the output.*/
  bool check_node(Node * node) 
  { 
    if (not is_connected(node)) 
      return false;
    
    if (is_sink(node)) 
      return node->out_flow == 0 and node->in_flow >= 0;

    if (is_source(node))
      return node->in_flow == 0 and node->out_flow >= 0;

    Flow_Type sum_out_cap = 0;
    Flow_Type sum_out_flow = 0;
    for (Node_Arc_Iterator<Net_Graph> it(node); it.has_curr(); it.next())
      {
	Arc * arc = it.get_current();
	if (arc->is_residual)
	  continue;

	if (not arc->check_arc())
	  return false;

	sum_out_cap += arc->cap;
	sum_out_flow += arc->flow;
      }

    if (sum_out_cap != node->out_cap or sum_out_flow != node->out_flow)
      return false;

    if (is_source(node))
      return true;

    return node->out_flow == node->in_flow;
  }

private:

  Aleph::set<Node*> src_nodes;
  Aleph::set<Node*> sink_nodes;

public:

/** @brief Returns the set of source nodes containing the network. */
  Aleph::set<Node*> & get_src_nodes() { return src_nodes; }

/** @brief Returns the set of sink nodes comprising the network. */
  Aleph::set<Node*> & get_sink_nodes() { return sink_nodes; }

public:

/** @brief Converts a network with multiple sources to a network with a single node
supra-source. Shoot exception if the network does not contain a source
node or if there is insufficient memory. */
  void make_super_source()
  {
    if (src_nodes.size() == 1) 
      return;

    if (src_nodes.size() == 0)
      throw std::domain_error("network has not source node (it has cicles)");

    DynDlist<Node*> src_list;
    for (typename Aleph::set<Node*>::iterator it = src_nodes.begin(); 
	 it != src_nodes.end(); ++it)
      src_list.append(*it);

    Node * super_source = insert_node(); 
    while (not src_list.is_empty())
      {
	Node * p = src_list.remove_first();
	insert_arc(super_source, p, get_out_cap(p));
      }

    with_super_source = true;
  }

/** @brief Restores a network with a supra-source node to the
original source network with multiple nodes. */
  void unmake_super_source()
  {
    if (not with_super_source) 
      return;

    assert(src_nodes.size() == 1);

    remove_node(*src_nodes.begin());
    with_super_source = false;
  }

/** @brief Converts a network with multiple sinks to a network with a single node above-sink.  
Shoot exception if the network does not contain a sink node or if there is insufficient memory. */
  void make_super_sink()
  {
    if (sink_nodes.size() == 1) 
      return;

    if (sink_nodes.size() == 0)
      throw std::domain_error("network has not sink node (it has cicles)");

    DynDlist<Node*> sink_list;
    for (typename Aleph::set<Node*>::iterator it = sink_nodes.begin(); 
	 it != sink_nodes.end(); ++it)
      sink_list.append(*it);

    Node * super_sink = insert_node(); 
    while (not sink_list.is_empty())
      {
	Node * p = sink_list.remove_first();
	insert_arc(p, super_sink, get_in_cap(p));
      }

    with_super_sink = true;
  }

/** @brief Restores a network with a supra-sink node to the original network
several sink nodes. */
  void unmake_super_sink()
  {
    if (not with_super_sink) 
      return;

    assert(sink_nodes.size() == 1);

    remove_node(*sink_nodes.begin());
    with_super_sink = false;
  }

/** @brief Converts a network with multiple nodes source and sink to a
node network with one supra-source and one supra-sink. */
  void make_super_nodes()
  {
    make_super_source();
    try
      {
	make_super_sink();
      }
    catch (std::bad_alloc)
      {
	unmake_super_source();
      }
  }

/** @brief Restores a network with a supra-source node and one
supra-sink to the original network with multiple nodes sources
and sinks. */
  void unmake_super_nodes()
  {
    unmake_super_source();
    unmake_super_sink();
  }

/** @brief Returns a source node of the network. */
  Node * get_source() { return *get_src_nodes().begin(); }

/** @brief Returns a sink node in the network */
  Node * get_sink() { return *get_sink_nodes().begin(); }

/** 
  @brief Insert a new node on the network.

  @details insert_node(node_info) aside memory for a network node, you
  copy information associated node_info and inserts the node in the
  this network.

  @param[in] node_info Information to copy to the node.
  
  @return A pointer to the new node inserted into the network.
  
  @throw bad_alloc If there is insufficient memory.
*/
  Node * insert_node(const Node_Type & node_info)
  {
    Node * p = Digraph::insert_node(node_info); 
    try
      {
	src_nodes.insert(p);
	try
	  {
	    sink_nodes.insert(p);
	    return p;
	  }
	catch (bad_alloc)
	  {
	    src_nodes.erase(p);
	    Digraph::remove_node(p);
	    throw;
	  }
      }
    catch (bad_alloc)
      {
	Digraph::remove_node(p);
	throw; /* Propagar excepción */
      }
  }

/** @brief Insert a new node on the network. 
The value of information is undetermined.
Shoot bad_alloc if there is insufficient memory. */
  Node * insert_node() { return insert_node(Node_Type()); }

/** 
  @brief Inserts a new node in the network copy another p node.

  @details insert_node(p) aside memory for a network node, 
  copy the information contained in "p" and inserts the new node in the
  this network.

  @param[in] p Another node in the same or another network which
  will copy the information associated with the node.

  @return A pointer to the new node inserted into the network.

  @throw bad_alloc If there is insufficient memory.
*/
  Node * insert_node(Node * p)
  {
    Digraph::insert_node(p); 
    try
      {
	src_nodes.insert(p);
	try
	  {
	    sink_nodes.insert(p);
	    return p;
	  }
	catch (bad_alloc)
	  {
	    src_nodes.erase(p);
	    Digraph::remove_node(p);
	    throw;
	  }
      }
    catch (bad_alloc)
      {
	Digraph::remove_node(p);
	throw; /* Propagar excepción */
      }
  }

/**
  @brief Inserts an arc in a flow network.

  @details insert_arc() creates a new arc with attribute value
  arc_info with capacitance value cap and flow "flow",
  from the src_node node to the tgt_node node.

  @param[in] src_node Pointer to the origin node.
  @param[in] tgt_node Pointer to the destination node.
  @param[in] arc_info Attribute value to save in the new arc.

  @param[in] cap Arch capacity value.
  @param[in] flow Arch flow value. 

  @return A pointer to the new arc inserted

  @throw std::bad_alloc if there is not enough memory to create
  the arc.
  @throw std::overflow_error if the value is greater flow
  the capacity.
*/
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node, 
			   const typename Arc::Arc_Type & arc_info,
			   const Flow_Type & cap, const Flow_Type & flow)
  {     
    /* Inserción en clase base */
    Arc * arc = Digraph::insert_arc(src_node, tgt_node, arc_info); 

    src_nodes.erase(tgt_node);  /* Actualización de source/sink */
    sink_nodes.erase(src_node); 

    arc->cap  = cap;            /* Ajuste capacidad y flujo de arco */
    arc->flow = flow;

    if (not arc->check_arc())
      throw std::overflow_error("flow is greater than capacity");

    tgt_node->in_degree++;   /* Actualizar info de control de nodo */
    src_node->out_cap  += cap;
    tgt_node->in_cap   += cap;
    src_node->out_flow += flow;   
    tgt_node->in_flow  += flow;

    return arc;
  }

/** 
  @brief Connects an Network arc previously inserted and disconnected.

  @details This method takes a pointer to an arc "arc" previously
  disconnected by, disconnect_arc(), and re-inserted into the
  graph.

  The arch, of course, must have been previously inserted in
  the graph. In this regard, no verification is not done.

  No verification any prior existence of an arc between the nodes 
  involved (this is necessary to operate with multigraphs) is performed.

  @param[in] arc Pointer to arc to re-insert.
  
  @return A pointer to the arc re-inserted
*/
  Arc * connect_arc(Arc * arc)
  {     
    Digraph::connect_arc(arc); 

    Node * src = this->get_src_node(arc);
    Node * tgt = this->get_tgt_node(arc);

    src_nodes.erase(tgt);        /* Elimina destino de src_nodes */
    sink_nodes.erase(src);       /* Elimina fuente de sink_nodes */

    tgt->in_degree++;            /* Actualiza información de control de nodo */
    src->out_cap  += arc->cap;
    tgt->in_cap   += arc->cap;
    src->out_flow += arc->flow;   
    tgt->in_flow  += arc->flow;
    
    return arc;
  }

/** 
  @brief Inserts an arc in a flow network.

  @details insert_arc() creates a new arc with attribute value
  arc_info with capacitance value cap
  from the src_node node to the tgt_node node.

  The flow value is zero.

  @param[in] src_node Pointer to the origin node.
  @param[in] tgt_node Pointer to the destination node.
  @param[in] cap Arch capacity value.

  @return A pointer to the new arc inserted

  @throw std::bad_alloc If there is insufficient memory to create 
  the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
       virtual Arc * 
  insert_arc(Node * src_node, Node * tgt_node, const Flow_Type & cap)
  {
    return insert_arc(src_node, tgt_node, Arc_Type(), cap, 0);
  }

/** 
  @brief Inserts an arc in a flow network.

  @details insert_arc() creates a arc new with attribute value
  arc_info from the src_node node to the tgt_node node.

  The arc is created with values of capacity and flow of zero.

  @param[in] src_node Pointer to the origin node.
  @param[in] tgt_node Pointer to the destination node.
  @param[in] arc_info Attribute value to save in the new arc.

  @return A pointer to the new arc inserted

  @throw std::bad_alloc If there is insufficient memory to create 
  the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node, 
			   const typename Arc::Arc_Type & arc_info)
  {
    return insert_arc(src_node, tgt_node, arc_info, 0, 0);
  }

/** 
  @brief Inserts an arc in a flow network.

  @details insert_arc() creates a new arc from the src_node node to the
  tgt_node node.

  The information associated with the arc is undetermined. As usuall
  this should be the method used if the arcs of the network does not
  store information associated.

  The arc is created with values of capacity and flow of zero.

  @param[in] src_node Pointer to the origin node.
  @param[in] tgt_node Pointer to the destination node.
  
  @return A pointer to the new arc inserted
  
  @throw std::bad_alloc If there is insufficient memory to create 
  the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
  virtual Arc * insert_arc(Node * src_node, Node * tgt_node)
  {
    return insert_arc(src_node, tgt_node, Arc_Type(), 0, 0);
  }

/** @brief Elimina de la red el arco arc. */
  virtual void remove_arc(Arc * arc)
  {
    Node * src = this->get_src_node(arc);
    Node * tgt = this->get_tgt_node(arc);

    if (--(tgt->in_degree) == 0)
      src_nodes.insert(tgt);  /* tgt deviene un nodo fuente */

    src->out_cap  -= arc->cap; /* Actualizar caps y flujos en src y tgt */
    src->out_flow -= arc->flow;
    tgt->in_cap   -= arc->cap;
    tgt->in_flow  -= arc->flow;

    Digraph::remove_arc(arc); /* Eliminar en clase base */

    if (this->get_num_arcs(src) == 0)
      sink_nodes.insert(src); /* src deviene un nodo sumidero */
  }

/**  
  @brief Disconnected from the network the "arc" arc.

  @details The operation of the graph off the "arc" arc. Eventually
  the arc can be saved and then reinserted by connect_arc().

  The arc must belong to the graph and do not make any
  verification regard.

  @param[in] arc Pointer to arc off
*/
  void disconnect_arc(Arc * arc)
  {
    Node * src = this->get_src_node(arc);
    Node * tgt = this->get_tgt_node(arc);
    if (--(tgt->in_degree) == 0)
      src_nodes.insert(tgt); /* tgt deviene un nodo fuente */

    src->out_cap  -= arc->cap;   /* Actualizar caps y flujos en src y tgt */
    src->out_flow -= arc->flow;
    tgt->in_cap   -= arc->cap;
    tgt->in_flow  -= arc->flow;

    Digraph::disconnect_arc(arc); /* Desconeción en clase base */

    if (this->get_num_arcs(src) == 0)
      sink_nodes.insert(src); /* src deviene un nodo sumidero */
  }

/** @brief Removes a node of a network flow with all their arches. */
  virtual void remove_node(Node * p)          
  {
    Digraph::remove_node(p);  /* Eliminación en clase base */
    src_nodes.erase(p);
    sink_nodes.erase(p);
  }

/** @brief Builder from a digraph.
Shoot bad_alloc if there is insufficient memory. */
  Net_Graph(Digraph & digraph)
  : with_super_source(false), with_super_sink(false), residual_net(false)
  {
    Net_Graph::Net_Graph();     /* inicializa atributos 
    copy_graph(*this, digraph, true);  /* copia mapeada
  }

/** @brief Build a network copy of the "net" network.
Shoot bad_alloc if there is insufficient memory. */
  Net_Graph(const Net_Graph & net) 
    : Array_Digraph<NodeT, ArcT>::Array_Digraph(),
      Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(net.with_super_source),
      with_super_sink(net.with_super_sink), residual_net(net.residual_net)
  {
    copy_graph(*this, net, false);            /* Copia sin mapear */
  }

  ~Net_Graph()
  {
    if (residual_net)
      unmake_residual_net();
  }

/** @brief Place capacitance value to an arc. */
  void set_cap(Arc * arc, const Flow_Type & cap) 
  {
    if (cap < arc->flow)
      throw std::out_of_range("capacity value is smaller than flow");

    const Flow_Type old_cap = arc->cap;
    arc->cap = cap;

    Node * src_node = get_src_node(arc);
    src_node->out_cap -= old_cap;
    src_node->out_cap += cap;

    Node * tgt_node = get_tgt_node(arc);
    tgt_node->in_cap -= old_cap;
    tgt_node->in_cap += cap;
  }

/** @brief Place flow value to an arc.
Shoot exception if the value is greater than capacity. */
  void set_flow(Arc * arc, const Flow_Type & flow)
  {
    if (flow > arc->cap)
      throw std::out_of_range("flow value is greater than capacity");

    const Flow_Type old_flow = arc->flow;
    arc->flow = flow;

    Node * src_node = get_src_node(arc);
    src_node->out_flow -= old_flow;
    src_node->out_flow += flow;

    Node * tgt_node = get_tgt_node(arc);
    tgt_node->in_flow -= old_flow;
    tgt_node->in_flow += flow;
  }

/** @brief Returns the capacity value of the arc. */
  const Flow_Type & get_flow(Arc * arc) const { return arc->flow; }

/** @brief Returns the value of flow of an arc. */
  const Flow_Type & get_cap(Arc * arc) const { return arc->cap; }

  void reset()
  {
    for (Arc_Iterator<Net_Graph> it(*this); it.has_current(); it.next())
      it.get_current()->flow = 0;

    for (Node_Iterator<Net_Graph> it(*this); it.has_current(); it.next())
      {
	Node * p = it.get_current();
	p->in_flow = p->out_flow = 0;
      }
  }

/** 
  @brief Check if a trained network meets conditions of definition.

  @details check_network() recorre todos los nodos y arcos de una red
  capacitada en búsqueda de inconsistencias de
  definición. Básicamente, se verifica que para todo nodo no
  fuente o no sumidero la cantidad de flujo entrante sea igual a
  la saliente. Por cada arco inspeccionado se verifica que el
  flujo sea menor o igual que su capacidad.

  @return true if the network is valid; false otherwise.
*/
  bool check_network() 
  {
    for (Node_Iterator<Net_Graph> it(*this); it.has_current(); it.next())
      if (not check_node(it.get_current()))
	return false;

    return true; /* Todos los nodos son válidos */
  }

/** @brief Returns the value of network flow seen from the source or sink "p" node.
Shoot std::domain_error exception if "p" is neither source nor sink. */
  Flow_Type flow_value() { return get_source()->out_flow; }

/** @brief Returns true if the network has added a source supra. */
  bool with_super_source; 

/** @brief Returns true if the network has added an supra sink. */
  bool with_super_sink;

  bool residual_net;

  Net_Graph() 
    : Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(false), with_super_sink(false), residual_net(false)
  { /* Empty */ }

  void insert_residual_arc(Arc * arc)
  {
    Arc * res_arc = Digraph::insert_arc(this->get_tgt_node(arc), 
					this->get_src_node(arc)); 

    res_arc->is_residual = true;
    res_arc->img_arc     = arc;
    res_arc->cap         = arc->cap;
    res_arc->flow        = arc->cap - arc->flow;

    arc->img_arc = res_arc;
  }

private:

  void remove_residual_arc(Arc * arc)
  {    
    assert(arc->is_residual);
    
    Digraph::remove_arc(arc);
  }

public:

/** 
  @brief Calcula la red residual de una red capacitada.

  @details make_residual_net() calcula el grafo residual de una red. La red
  puede tener múltiples fuentes y sumideros, así como tener
  valores de flujo distintos de cero.

  La red residual es la principal estructura de dato empleada por
  los algoritmos de cálculo de flujo máximo.

  A diferencia de otros enfoques, en esta implantación los arcos
  residuales se colocan sobre la misma 
  red.  Consecuentemente, se puede decir que el método tiene un
  buen ahorre de memoria, pues los arcos normales y los nodos no
  son redundados.

  Para saber si un arco es residual o no puede consultarse su
  atributo is_residual. 

  El arco reflejo puede consultarse mediante el atributo img_arc.

  Para ver sólo los arcos de la red, residuales o no, puede
  emplearse un iterador wrapper con la clase filtro Res_F. De este modo,
  Node_Arc_Iterator o Arc_Iterator sólo verán arcos con capacidad
  disponible; es decir aquellos arcos cuya diferencia entre
  capacidad nominal y el valor de flujo sea diferente de cero. 

  También pueden verse todos los arcos, independientemente de que
  sean o no residuales, con un iterador interno de Net_Graph o con
  uno wrapper sin filtro. En este caso, es responsabilidad del
  usuario distinguir los arcos según el interés; que sean
  residuales o que tengan capacidad según sea el caso.

  Los algoritmos de cálculo de flujo máximo emplean este
  método. Su uso está destinado para otros algoritmos no
  instrumentados en esta biblioteca.

  @throw domain_error If the residual network is already calculated.
  @throw bad_alloc If there is insufficient memory.
  
  @see unmake_residual_net() Res_F
*/
  void make_residual_net()
  {
    try
      {
	if (residual_net)
	  throw std::domain_error("Residual net is currently computed");

	size_t n = this->get_num_arcs(); /* Num arcos residuales a insertar */
	for (Arc_Iterator<Net_Graph> it(*this); it.has_curr() and n > 0; 
	     it.next())
	  {
	    Arc * arc = it.get_current_arc();
	    if (not arc->is_residual)
	      {
		insert_residual_arc(arc);
		--n; 
	      }
	  }

	residual_net = true;
      }
    catch (...)
      {
	unmake_residual_net();
      }
  }

/** @brief Undoes a previously calculated residual network.
Shoot domain_error exception if the residual network has not been previously calculated.  */
  void unmake_residual_net()
  {
    for (Arc_Iterator<Net_Graph> it(*this); it.has_current(); )
      {
	Arc * arc = it.get_current_arc();
	if (arc->is_residual)
	  {
	    it.next();
	    remove_residual_arc(arc);
	  }
	else
	  it.next();
      }
    residual_net = false;
  }

public:

/** @brief Returns true if the residual network is created; false otherwise */
  bool is_there_residual_net() const { return residual_net; }

/** @brief Gets the residual arc of "a" */
  Arc * get_residual_arc(Arc * a) 
  {
    if (not residual_net)
      throw std::domain_error("Residual net is not computed");

    return a->img_arc; 
  }

  void increase_out_flow(Node * p, const Flow_Type & flow)
  {
    if (is_sink(p))
      return;
    
    p->out_flow += flow;
  }

  void decrease_out_flow(Node * p, const Flow_Type & flow)
  {
    if (is_sink(p))
      return;

    p->out_flow -= flow;
  }

  void increase_in_flow(Node * p, const Flow_Type & flow)
  {
    if (is_source(p))
      return;

    p->in_flow += flow;
  }

  void decrease_in_flow(Node * p, const Flow_Type & flow)
  {
    if (is_source(p))
      return;

    p->in_flow -= flow;
  }
};

/** 
  @brief Filter of arches of a residual network.

  @details The Res_F class is used on a network to filter waste
  those arcs whose remaining flow is zero. This class
  algorithms for calculating the maximum flow are based
  library.

  This is the filter to use when you want to watch network
  residual.

  @note It requires that the residual network has been instantiated

  @ingroup Redes
*/
    template <class N> 
class Res_F
{
public:

  bool operator () (typename N::Arc * a) const
  {
    return (a->cap - a->flow) != 0;
  }
};

/** 
  @brief Increases the flow of a network as a road of increase.

  @details increase_flow(net, path) takes the path of increased content in "path"
  and increases the value of "net" flow in the minimum link
  (slack) of the road. 

  The library allows to calculate paths increase over the network
  residual in two ways:
  -# find_path_depth_first<Net, Res_F<Net> >(net, source, sink, path)
  -# find_path_breadth_first<Net, Res_F<Net> >(net, source, sink, path)
  .
  To make sense of any of these ways to find
  ways of increasing the residual network on "net" should be
  instantiated. Stop it, it must have previously called
  make_residual_net(). 

  @param[in] net Trained network where you want to increase
  the flow path increase.
  @param[in] path Road of Path type where you want
  store the road increase.

  @return The value has been increased the flow.

  @see Path find_path_depth_first() find_path_breadth_first() make_residual_net()

  @throw domain_error If the residual network has not been created 

  @ingroup Redes
*/
    template <class Net>
typename Net::Flow_Type increase_flow(Net & net, Path<Net> & path)
{
  if (not net.residual_net)
    throw std::domain_error("Residual net is not created");

  typename Net::Flow_Type slack = net.Infinity; /* Eslabón mínimo */

    /** Calcular el eslabón mínimo del camino de aumento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      typename Net::Arc * arc = it.get_current_arc();
      const typename Net::Flow_Type w = arc->cap - arc->flow;

      assert(w >= 0);

      if (w < slack)
        slack = w;
    }

  if (slack == 0)
    return slack;

    /** Aumentar el flujo de la red por el camino de aumento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      typename Net::Arc * arc = it.get_current_arc();
      typename Net::Arc * img = (typename Net::Arc*) arc->img_arc;
      arc->flow += slack; 
      img->flow -= slack;

      if (not arc->is_residual)
        {
          net.increase_out_flow(net.get_src_node(arc), slack);
          net.increase_in_flow(net.get_tgt_node(arc), slack);
        }
      else
        {
          net.decrease_in_flow(net.get_src_node(arc), slack);
          net.decrease_out_flow(net.get_tgt_node(arc), slack);
        }

      assert(arc->check_arc());
    }

  return slack;
}

/** 
  @brief Determines whether or not there is a way of increase.

  @details has_aumenting_path() returns true if the "this" network contains the
  one way of increasing.

  A way of increasing a way which can increase
  the value of a trained network flow without altering its
  network conditions.

  Normally routine is used to determine whether the value of
  network flow or is not possible. In which case
  has_aumenting_path() you should not find any way.

  The way of increase is wanted in depth.

  The routine requires that the residual network has been calculated.

  @param[in] net Red de flujo.

  @return true if the network contains a path of increasing; false
  otherwise.

  @throw domain_error If the residual network has not been calculated.

  @ingroup Redes
*/
    template <class Net>
bool has_aumenting_path(Net & net) 
{
  if (not net.residual_net)
    throw std::domain_error("Residual net has not been generated");

  return test_for_path<Net, Res_F<Net> >(net, net.get_source(), net.get_sink());
}

/** 
  @brief Maximizes the network flow according searches of paths of increase.

  @details aumenting_path_maximum_flow() receives a trained network and
  maximizes flow value by successive searches of
  increase paths.

  The method takes two type parameters:
  -# Net: the network type whose flow is to be maximized and should
  Net_Graph be derived.
  -# Find_Path: the class of search of path.

  The network can have multiple sources and sinks.

  During the calculation a partial residual network is built, whose
  space consumption is proportional to the amount of edges.

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.
    
  @return The maximized flow value of the network
   
  @throw bad_alloc If there is insufficient memory to the network
  waste and increase internal roads. In this case,
  the network status corresponds to the last increment of
  flow done before it occurs exception.
    
  @throw domain_error If the residual network has already been calculated.
  
  @ingroup Redes
*/
template 
<class Net, 
 template <class, template <class, class> class, class> class Find_Path>
typename Net::Flow_Type 
aumenting_path_maximum_flow(Net & net, const bool & leave_residual = false)
{
  net.make_super_nodes();
  try
    {
      net.make_residual_net();
    }
  catch (std::bad_alloc)
    {
      net.unmake_super_nodes();
      throw;
    }

  typename Net::Node * source = net.get_source();
  typename Net::Node * sink   = net.get_sink();
  try
    {
      while (true) /* Mientras exista un camino de aumento */
	{
	  Path<Net> path(net);
	  if (not Find_Path<Net, Node_Arc_Iterator, Res_F<Net>> () 
		(net, source, sink, path))
	    break;

	  increase_flow <Net> (net, path);
	}
    }
  catch (std::bad_alloc)
    {
      net.unmake_residual_net();
      net.unmake_super_nodes();
      throw;
    }

  const typename Net::Flow_Type ret_val = source->out_flow;
  if (not leave_residual)
    { 
      net.unmake_residual_net();
      net.unmake_super_nodes();
    }

  return ret_val;
}

/** 
  @brief Maximizes the network flow according to the algorithm
  Ford-Fulkerson.

  @details ford_fulkerson_maximum_flow() receives a trained network and
  it maximizes flow value by the Ford-Fulkerson
  algorithm.

  The network can have multiple sources and sinks.

  During the calculation a partial residual network is built, whose
  space consumption is proportional to the amount of edges.

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory to the network
  waste and increase internal roads. In this case,
  the network status corresponds to the last increment of
  flow done before it occurs exception.
  @throw domain_error If the residual network has already been calculated.

  @ingroup Redes
*/
      template <class Net> typename Net::Flow_Type 
ford_fulkerson_maximum_flow(Net & net, const bool & leave_residual = false)
{
  return aumenting_path_maximum_flow<Net, Find_Path_Depth_First>
    (net, leave_residual);
} 

/** 
  @brief Invoking class the Ford Fulkerson algorithm.

  @see ford_fulkerson_maximum_flow()
  
  @ingroup Redes
*/
    template <class Net> class 
Ford_Fulkerson_Maximum_Flow 
{
public:

/** 
  @brief Invoke to ford_fulkerson_maximum_flow().

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
      typename Net::Flow_Type 
  operator () (Net & net, const bool & leave_residual = false) const
  {
    return ford_fulkerson_maximum_flow(net, leave_residual);
  }
};

/** 
  @brief Maximizes network flow according to the algorithm
  Edmonds-Karp.

  @details edmonds_karp_maximum_flow() receives a network
  capable and maximizes their value flow through the algorithm
  Edmonds-Karp.

  The network can have multiple sources and sinks.

  During the calculation a partial residual network is built, whose
  space consumption is proportional to the amount of edges.

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory to the network
  waste and increase internal roads. In this case,
  the network status corresponds to the last increment of
  flow done before it occurs exception.
  @throw domain_error If the residual network has already been calculated.

  @ingroup Redes
*/
      template <class Net> typename Net::Flow_Type
edmonds_karp_maximum_flow(Net & net, const bool & leave_residual = false)
{
  return aumenting_path_maximum_flow<Net, Find_Path_Breadth_First>
    (net, leave_residual);
}

/** 
  @brief Invoking class to the Ford Fulkerson algorithm.

  @see edmonds_karp_maximum_flow()

  @ingroup Redes
*/
    template <class Net> 
class Edmonds_Karp_Maximum_Flow
{
public:

/** 
  @brief Invoke to edmonds_karp_maximum_flow().

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
      typename Net::Flow_Type 
  operator () (Net & net, 
	       const bool & leave_residual = false) const
  {
    return edmonds_karp_maximum_flow(net, leave_residual);
  }
};

    template <class Net> static
bool is_node_active(typename Net::Node * p) 
{
  return p->in_flow > p->out_flow; 
}

    template <class Net> static
long & node_height(typename Net::Node * p) { return NODE_COUNTER(p); }

/** 
  @brief Residual filter arcs.

  @details Res_Arc is a class filter residual to be used arches
  for wrapper iterators to iterate only on arcs
  waste.

  @ingroup Redes
*/
    template <class N> 
class Res_Arc
{
public:

  bool operator () (typename N::Arc * a) const
  {
    return a->is_residual;
  }
};

    template <class Net> static
bool initial_height(Net & net, typename Net::Node * p, typename Net::Arc * a)
{
  node_height<Net>(p) = node_height<Net>(net.get_src_node(a)) + 1;
  return false;
}

    template <class Net> static 
void init_height_in_nodes(Net & net)
{
  breadth_first_traversal <Net, Res_Arc<Net>> 
    (net, net.get_sink(), &initial_height); 
}

    template <class Q_Type> static
void put_in_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
{
  if (NODE_BITS(p).get_bit(Aleph::Maximum_Flow))
    return;

  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, true);
  q.put(p);
}

    template <class Q_Type> static 
typename Q_Type::Item_Type get_from_active_queue(Q_Type & q)
{
  typename Q_Type::Item_Type p = q.get();

  assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));

  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);

  return p;
}


   template <class Q_Type> static
void remove_from_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
{
  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);
  q.remove(p);
}

/** 
  @brief Maximum flow as generic preflow push algorithm
  oriented to nodes.

  @details generic_preflow_vertex_push_maximum_flow(net) calculates the flow
  maximum of the "net" network as the generic algorithm and Goldberg
  Tarjan. The function takes two type parameters:
  - #Net: a class of derivative network trained of Net_Graph type.
  -# Q_Type: one set of "active nodes" class. Q_Type must
  handle pointers of Net::Node type typename and must support
  the following:
   -# get(): Removing a node.
   -# put(p): Insertion "p" node in the set.
   .
  .

  During the execution of the algorithm used accountants and
  cookies from all network nodes. The bit is Maximum_Flow is
  used for arches.

  @param[in,out] net The network capable to maximize flow. The net
  can have several sources and sinks.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network is already calculated.

  @see fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow()

  @ingroup Redes
*/
    template <class Net, class Q_Type> typename Net::Flow_Type
generic_preflow_vertex_push_maximum_flow
    (Net & net, const bool & leave_residual = false)
{
  net.make_super_nodes();
  try
    {
      net.make_residual_net();
      net.reset_counter_nodes();
    }
  catch (std::bad_alloc)
    {
      net.unmake_super_nodes();
      throw;
    }

  init_height_in_nodes(net); 
  typename Net::Node * source = net.get_source();
  typename Net::Node * sink   = net.get_sink();

  Q_Type q;  /* Instancia el conjunto de nodos activos */
  try
    {
      for (Node_Arc_Iterator<Net, Res_F<Net> > it(source); it.has_curr(); 
	   it.next())
	{
	  typename Net::Arc * arc  = it.get_current_arc();  
	  typename Net::Node * tgt = net.get_tgt_node(arc); 
	  arc->flow = tgt->in_flow = arc->cap - arc->flow; /* Inunde arco */
	  arc->img_arc->flow = 0; 
	  if (tgt != sink)
	    put_in_active_queue(q, tgt); /* tgt deviene activo */
	}

      source->out_flow = source->out_cap;

      while (not q.is_empty()) /* Mientras haya nodos activos */
	{
	  typename Net::Node * src = get_from_active_queue(q);
	  typename Net::Flow_Type excess = src->in_flow - src->out_flow;

	  for (Node_Arc_Iterator<Net, Res_F<Net> > it(src); 
	       it.has_curr() and excess > 0; it.next()) 
	    {
	      typename Net::Arc * arc  = it.get_current_arc();
	      typename Net::Node * tgt = net.get_tgt_node(arc);
	      if (node_height<Net>(src) != node_height<Net>(tgt) + 1)
		continue; /* El nodo no es elegible */

	      const typename Net::Flow_Type flow_avail_in_arc = 
		arc->cap - arc->flow;   
	      typename Net::Flow_Type flow_to_push = 
		std::min(flow_avail_in_arc, excess);

	      arc->flow          += flow_to_push;
	      arc->img_arc->flow -= flow_to_push;
	      if (arc->is_residual)
		{
		  net.decrease_out_flow(tgt, flow_to_push);
		  net.decrease_in_flow(src, flow_to_push);
		}
	      else
		{
		  net.increase_out_flow(src, flow_to_push);
		  net.increase_in_flow(tgt, flow_to_push);
		}

	      excess -= flow_to_push;

	      if (is_node_active<Net>(tgt) and tgt != sink and tgt != source) 
		put_in_active_queue(q, tgt);
        }

    /* ¿src aún sigue activo?
    Si la respuesta es SI, entonces incremente h(src) y re-inserte en q */
	  if (excess > 0) 
	    {
	      node_height<Net>(src)++;
	      put_in_active_queue(q, src);
	    }
	}

      const typename Net::Flow_Type ret_val = source->out_flow; 

      if (not leave_residual)
	{
	  net.unmake_residual_net();
	  net.unmake_super_nodes();
	}

      return ret_val;
    } 
  catch (...)
    {
      net.unmake_residual_net();
      net.unmake_super_nodes();
      throw;
    }
}

/** 
  @brief Maximum flow of preflow push with FIFO queue of nodes.

  @details fifo_preflow_vertex_push_maximum_flow(net) calculates the flow
  maximum of the "net" network as the generic algorithm and Goldberg
  Tarjan wherein the active nodes, those whose flow of
  input is greater than the output is stored and processed
  as a FIFO queue. This algorithm processes pushes the preflow
  through the arches as a heuristic in depth.

  During the execution of the algorithm used accountants and
  cookies from all network nodes. The bit is Maximum_Flow is
  used for arches.

  @param[in,out] net The network capable to maximize flow. The net
  can have several sources and sinks.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
    template <class Net> typename Net::Flow_Type
fifo_preflow_maximum_flow
    (Net & net, const bool & leave_residual = false)
{
  return generic_preflow_vertex_push_maximum_flow
    <Net,DynListQueue<typename Net::Node*> >(net, leave_residual);
}

/** 
  @brief Invoking class to fifo_preflow_maximum_flow()

  @see fifo_preflow_maximum_flow()

  @ingroup Redes
*/
    template <class Net> 
class Fifo_Preflow_Maximum_Flow
{
public:

/** 
  @brief Invoke to fifo_preflow_maximum_flow().

  @param[in,out] net Capable network whose flow you want to be 
  maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
       typename Net::Flow_Type 
   operator () (Net & net, 
	     const bool & leave_residual = false) const
  {
    return fifo_preflow_maximum_flow(net, leave_residual);
  }
};

    template <class Net> 
struct Compare_Height
{
  bool operator () (typename Net::Node * n1, typename Net::Node * n2) const
  {
    return node_height<Net>(n1) > node_height<Net>(n2);
  }
};

/** 
  @brief Maximum flow of preflow push with priority queue according
  the height of the nodes. 

  @details heap_preflow_vertex_push_maximum_flow(net) calculates the flow
  maximum "net" in which the active network nodes, those whose
  inflow is larger than the output they are stored in
  a priority queue. Preflow push from the source
  until sump according to a heuristic amplitude.

  heap_preflow_vertex_push_maximum_flow()It is one of the best
  general calculation algorithms of maximum flow. It performance
  the worst case is \f$O(V^3)\f$.

  During the execution of the algorithm used accountants and
  cookies from all network nodes. The bit is Maximum_Flow is
  used for arches.

  @param[in,out] net The network capable to maximize flow. The net
  can have several sources and sinks.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
    template <class Net> typename Net::Flow_Type
heap_preflow_maximum_flow(Net & net, const bool & leave_residual = false)
{
  return generic_preflow_vertex_push_maximum_flow 
    <Net, DynBinHeap<typename Net::Node*, Compare_Height<Net> > > 
     (net, leave_residual);
}

/** 
  @brief Invoking class to heap_preflow_maximum_flow()

  @see heap_preflow_maximum_flow()

  @ingroup Redes
*/
   template <class Net> 
class Heap_Preflow_Maximum_Flow
{
public:

/** 
  @brief Invoke to heap_preflow_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
      typename Net::Flow_Type
  operator () (Net & net, 
	       const bool & leave_residual = false) const
  {
    return heap_preflow_maximum_flow(net, leave_residual);
  }
};

/** 
  @brief Maximum flow of preflow push with tail nodes randomly
  active.

  @details random_preflow_vertex_push_maximum_flow(net) calculates the flow
  maximum "net" in which the active network nodes, those whose
  inflow is larger than the output they are stored in
  a random line. 

  During the execution of the algorithm used accountants and
  cookies from all network nodes. The bit is Maximum_Flow is
  used for arches.

  @param[in,out] net The network capable to maximize flow. The net
  can have several sources and sinks.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.

  @ingroup Redes
*/
    template <class Net> typename Net::Flow_Type
random_preflow_maximum_flow(Net & net, const bool & leave_residual = false)
{
  return generic_preflow_vertex_push_maximum_flow
    <Net, Random_Set<typename Net::Node*> > (net, leave_residual);
}

/** 
  @brief Invoking class to random_preflow_maximum_flow()

  @see random_preflow_maximum_flow()

  @ingroup Redes
*/
template <class Net> class Random_Preflow_Maximum_Flow
{
public:

/** 
  @brief Invoke to random_preflow_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.
  
  @return The maximized flow value of the network
*/
       typename Net::Flow_Type 
   operator () (Net & net,
		const bool & leave_residual = false) const
  {
    return random_preflow_maximum_flow(net, leave_residual);
  }
};


    template <class Net> static
bool has_arcs_in_active_queue(typename Net::Node * p)
{
  for (Node_Arc_Iterator<Net, Res_F<Net> > it(p); it.has_current(); it.next()) 
    if (IS_ARC_VISITED(it.get_current(), Aleph::Maximum_Flow))
      return true;

  return false;
}

/** 
  @brief Maximum flow as generic preflow push algorithm
  oriented arcs.

  @details generic_preflow_edge_maximum_flow(net) calculates the flow
  maximum of the "net" network as the generic algorithm and Goldberg
  Tarjan. The function uses three parameters type:
  - #Net: a class of derivative network trained of Net_Graph type.
  -# QN_Type: one set of "active nodes" class. Q_Type must
  handle pointers Net::Node type typename and must support
  the following:
    -# get(): Removing a node.
    -# put(p): Insertion "p" node in the set.
    -# remove(p): Elimination of "p" specific nodes.
    .
  -# QA_Type: one set of "eligible arches" class. QA_Type
  should handle pointer type typename Net::Arc with the
  following:
    -# get(): Removing a arc.
    -# put(p): Inserting the "p" arc in the set.
    .
  .

  During the execution of the algorithm used accountants and
  cookies from all network nodes. The bit is Maximum_Flow is
  used for arches.

  @param[in,out] net The network capable to maximize flow. The net
  can have several sources and sinks.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.
  
  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network is already calculated.
  
  @see fifo_preflow_maximum_flow() heap_preflow_maximum_flow() random_preflow_maximum_flow()
  
  @ingroup Redes
*/
    template <class Net, class QN_Type, class QA_Type> 
    typename Net::Flow_Type
generic_preflow_edge_maximum_flow(Net & net, 
				  const bool & leave_residual = false)
{
  net.make_super_nodes();
  try
    {
      net.make_residual_net();
      net.reset_counter_nodes();
    }
  catch (std::bad_alloc)
    {
       net.unmake_super_nodes();
       throw;
    }

  QA_Type q;  /* Conjunto de arcos elegibles */
  QN_Type p;  /* Conjunto de nodos activos */
  typename Net::Node * source = net.get_source();
  typename Net::Node * sink   = net.get_sink();
  init_height_in_nodes(net); 

  for (Node_Arc_Iterator<Net, Res_F<Net> > i(source); i.has_curr(); i.next())
    {
      typename Net::Arc * arc  = i.get_current_arc();  
      typename Net::Node * tgt = net.get_tgt_node(arc); 
      arc->flow = tgt->in_flow = arc->cap - arc->flow;  /* Inundar arco */
      arc->img_arc->flow       = 0;     

    /* Meter arcos elegibles de tgt */
      for (Node_Arc_Iterator<Net, Res_F<Net> > j(tgt); j.has_curr(); j.next())
        {
          typename Net::Arc * a  = j.get_current_arc(); 
          if (node_height<Net>(tgt) == 
	      node_height<Net>(net.get_tgt_node(a)) + 1)
            put_in_active_queue(q, a); 
        }
      put_in_active_queue(p, tgt); /* Mete en cola de nodos activos */
    }
  source->out_flow = source->out_cap;

  while (true)
    {

    /* Mientras haya arcos elegibles */
      while (not q.is_empty()) 
        {
          typename Net::Arc * arc  = get_from_active_queue(q);
          typename Net::Node * src = net.get_src_node(arc);
          typename Net::Node * tgt = net.get_tgt_node(arc);
          if (node_height<Net>(src) != node_height<Net>(tgt) + 1) 
            continue; /* No ==> ignore arco */

          typename Net::Flow_Type excess = src->in_flow - src->out_flow;

    /** Source downloaded in previous iteration?
    If the answer is YES, then ignore arc */
          if (excess == 0) 
            {
              remove_from_active_queue(p, src); 
              continue; /* Avance al siguiente arco elegible */
            }
          
          const typename Net::Flow_Type push = 
	    std::min(excess, arc->cap - arc->flow);
          arc->flow          += push;
          arc->img_arc->flow -= push;
          if (arc->is_residual)
            {
              net.decrease_out_flow(net.get_tgt_node(arc), push); 
              net.decrease_in_flow(net.get_src_node(arc), push);
            }
          else
            {
              net.increase_in_flow(net.get_tgt_node(arc), push);
              net.increase_out_flow(net.get_src_node(arc), push); 
            }
          excess -= push;

          if (is_node_active<Net>(tgt) and tgt != source and tgt != sink) 
            {
              for (Node_Arc_Iterator<Net, Res_F<Net> > it(tgt); 
		   it.has_curr(); it.next()) 
                {
                  typename Net::Arc * a  = it.get_current_arc();
                  if (node_height<Net>(tgt) == 
		      node_height<Net>(net.get_tgt_node(a)) + 1)
                    put_in_active_queue(q, a);
                }

              put_in_active_queue(p, tgt);
            } 

    /** Is the source was discharged?
    If the answer is YES, the source becomes inactive */
          if (excess == 0)
            {
              remove_from_active_queue(p, src);
              continue;
            }

    /** Dig non-saturating?
    If the answer is YES, increase h(active) and re-insert "arc" in "q" */

          if (src != source and src != sink and   
              not has_arcs_in_active_queue<Net>(src)) 
            {
              remove_from_active_queue(p, src);
              node_height<Net>(src)++;
              put_in_active_queue(p, src);

              for (Node_Arc_Iterator<Net> it(src); it.has_current(); it.next()) 
                {
                  typename Net::Arc * a = it.get_current_arc(); /* Sale de src */
                  if ((node_height<Net>(src) == 
		       node_height<Net>(net.get_tgt_node(a)) + 1) 
                      and (a->cap - a->flow > 0)) 
                    put_in_active_queue(q, a);

                  typename Net::Arc * i = a->img_arc; /* Arco entrante a src */
                  if ((i->cap - i->flow > 0) and /* ¿i es elegible? */
                      (node_height<Net>(net.get_src_node(i)) == 
		       node_height<Net>(src) + 1))
                    put_in_active_queue(q, i);
                }
            }
        }

      if (p.is_empty()) /* ¿Quedan nodos activos? */
        break; /* Ya no hay nodos activos */

      typename Net::Node * src = get_from_active_queue(p);
      node_height<Net>(src)++;
      put_in_active_queue(p, src);

      for (Node_Arc_Iterator<Net> it(src); it.has_current(); it.next()) 
        {
          typename Net::Arc * a = it.get_current_arc(); /* Sale de src*/
          if ((node_height<Net>(src) == 
	       node_height<Net>(net.get_tgt_node(a)) + 1) 
              and (a->cap - a->flow > 0)) 
            put_in_active_queue(q, a);

          typename Net::Arc * i = a->img_arc; /* arco entrante a src */
          if ((i->cap - i->flow > 0) and /* ¿i es elegible? */
              (node_height<Net>(net.get_src_node(i)) == 
	       node_height<Net>(src) + 1))
            put_in_active_queue(q, i);
        }
    } /* End while (true) */

  if (not leave_residual)
    {
      net.unmake_residual_net();
      assert(source->out_flow == sink->in_flow);
      net.unmake_super_nodes();
    }

  return source->out_flow;
}

/** 
  @brief Maximum network flow for preflow push in depth.

  @details depth_first_preflow_edge_maximum_flow() calculates the flow
  maximum of the "net" network according to the preflow push technique. The
  preflow is carried towards the drain, which is
  suitable for low eccentricity graphs.

  @param[in,out] net Capable network whose flow is to be maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @ingroup  Redes
*/
    template <class Net> typename Net::Flow_Type
depth_first_preflow_edge_maximum_flow
  (Net & net, const bool & leave_residual = false)
{
  typedef DynSetTreap<typename Net::Node*> Active;
  typedef DynListStack<typename Net::Arc*> Stack;

  return generic_preflow_edge_maximum_flow <Net,Active,Stack>
    (net, leave_residual);
}

/** 
  @brief Invoking class to depth_first_preflow_maximum_flow()

  @see depth_first_preflow_edge_maximum_flow()

  @ingroup  Redes
*/
template <class Net> class Depth_First_Preflow_Maximum_Flow
{
public:

/** 
  @brief Invoke to depth_first_preflow_edge_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
       typename Net::Flow_Type 
   operator () (Net & net, 
		const bool & leave_residual = false) const
  {
    return depth_first_preflow_edge_maximum_flow(net, leave_residual);
  }
};

/** 
  @brief Maximum network flow preflow push in amplitude.

  @details breadth_first_preflow_edge_maximum_flow() calculates the flow
  maximum of the "net" network according to the preflow push technique. The
  preflow amplitude is taken into the sump, which can be
  suitable for high eccentricity graphs.

  @param[in,out] net Capable network whose flow is to be maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network has already been calculated.

  @ingroup  Redes
*/
    template <class Net> typename Net::Flow_Type
breadth_first_preflow_edge_maximum_flow
   (Net & net, const bool leave_residual = false)
{
  typedef DynSetTreap<typename Net::Node*> Active;
  typedef DynListQueue<typename Net::Arc*> Queue;
  return generic_preflow_edge_maximum_flow<Net, Active, Queue>
    (net, leave_residual);
}

/** 
  @brief Invoking class to breadth_first_preflow_maximum_flow()

  @see breadth_first_preflow_edge_maximum_flow()

  @ingroup  Redes
*/
template <class Net> class Breadth_First_Preflow_Maximum_Flow
{
public:
  
/** 
  @brief Invoke to breadth_first_preflow_edge_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.
  
  @return The maximized flow value of the network
*/
       typename Net::Flow_Type 
   operator () (Net & net, 
		const bool & leave_residual = false) const
  {
    return breadth_first_preflow_edge_maximum_flow(net, leave_residual);
  }
};

template <class Net> struct Compare_Arc
{
  bool operator () (typename Net::Arc * a1, typename Net::Arc * a2) const
  {
    typename Net::Node * src1 = (typename Net::Node *) a1->src_node;
    typename Net::Node * src2 = (typename Net::Node *) a2->src_node;
    if (src1->counter == src2->counter)
      return a1->cap - a1->flow > a2->cap - a2->flow;

    return src1->counter > src2->counter;
  }
};

/** 
  @brief Maximum network flow for preflow push in priority.

  @details priority_first_preflow_edge_maximum_flow() calculates the flow
  maximum "net" network as preflow push technique. The
  preflow is brought into the sump by the arcs emanating
  nodes more distant from the sump to the
  close. When a tie occurs, the arc is chosen more
  capacity under the expectation that it will push for greater
  preflow.

  Overall, this is one of the best algorithms for calculating
  maximum flow.

  @param[in,out] net Capable network whose flow is to be maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network has already been calculated.

  @ingroup  Redes
*/
    template <class Net> typename Net::Flow_Type
priority_first_preflow_edge_maximum_flow
  (Net & net, const bool & leave_residual = false)
{
  typedef DynSetTreap<typename Net::Node*> Active;
  typedef DynBinHeap<typename Net::Arc*, Compare_Arc<Net> > Prio_Queue;
  return generic_preflow_edge_maximum_flow <Net, Active, Prio_Queue> 
    (net, leave_residual);
}

/** 
  @brief Invoking class to priority_first_preflow_maximum_flow()

  @see priority_first_preflow_edge_maximum_flow()

  @ingroup Redes
*/
template <class Net> class Priority_First_Preflow_Maximum_Flow
{
public:
  
/** 
  @brief Invoke to priority_first_preflow_edge_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network
*/
       typename Net::Flow_Type
   operator () (Net & net, 
		const bool & leave_residual = false) const
  {
    return priority_first_preflow_edge_maximum_flow(net, leave_residual);
  }
};

/** 
  @brief Maximum network flow for preflow push random.

  @details random_first_preflow_edge_maximum_flow() calculates the flow
  maximum "net" network as preflow push technique. The
  preflow is carried randomly into the sump.

  @param[in,out] net Capable network whose flow is to be maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network has already been calculated.

  @ingroup Redes
*/
    template <class Net> typename Net::Flow_Type
random_first_preflow_edge_maximum_flow
  (Net & net, const bool & leave_residual = false)
{
  typedef DynSetTreap<typename Net::Node*> Active;
  typedef Random_Set <typename Net::Arc*> Random_Queue;
  return generic_preflow_edge_maximum_flow <Net, Active, Random_Queue> 
    (net, leave_residual);
}

/** 
  @brief Invoking class to random_first_preflow_maximum_flow()

  @see random_first_preflow_edge_maximum_flow()

  @ingroup  Redes
*/
template <class Net> class Random_First_Preflow_Maximum_Flow
{
public:

/** 
  @brief Maximum network flow for preflow push random.

  @details random_first_preflow_edge_maximum_flow() calculates the flow
  maximum "net" network as preflow push technique. The
  preflow is carried randomly into the sump.

  @param[in,out] net Capable network whose flow is to be maximized.
  @param[in] leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. By
  default, the residual network and the supra graph if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory.
  @throw domain_error If the residual network has already been calculated.

  @ingroup  Redes
*/
       typename Net::Flow_Type 
   operator () (Net & net,
		const bool & leave_residual = false) const
  {
    return random_first_preflow_edge_maximum_flow(net, leave_residual);
  }
};


static void * vs_ptr = NULL;

    template <class Net> static bool 
add_vertex_to_vs(Net &, typename Net::Node * node, typename Net::Arc*)
{
  ((Aleph::set<typename Net::Node*>*)vs_ptr)->insert(node);
  return false; /* Indica que debe seguir explorando */
}

/** 
  @brief Class filter for arches not waste for iterators on arches.

  @details This is the class to use if you want to iterate over a graph
  residual and only get the arcs are not residuals.

  @ingroup  Redes 
*/
template <class N> class No_Res_Arc
{
public:

 bool operator () (typename N::Node *, typename N::Arc * a) const
  {
    return not a->is_residual;
  }

  bool operator () (N&, typename N::Arc * a) const
  {
    return not a->is_residual;
  }

 bool operator () (typename N::Arc * a) const
  {
    return not a->is_residual;
  }
};

/** 
  @brief Calculate the maximum flow of a trained network and determines the cut
  minimum.  

  @details min_cut() receives a trained network whose flow is maximized and
  then, from the maximum flow, minimum cut is calculated. The
  type routine receives two parameters:
  -# Net: Network class, which must be derived from the Net_Graph
  class.
  -# Maxflow: Class calculation algorithm maximum flow to be used.

  @param[in,out] net Network to maximize the flow and which
  want to calculate a minimum cut
  @param[out] vs Nodes set \f$V_s\f$.
  @param[out] vt Nodes set \f$V_t\f$.
  @param[out] cuts Set of arches ranging from \f$V_s\f$ to
  \f$V_t\f$. This is the cut.
  @param[out] cutt Set of arches ranging from \f$V_t\f$ to
  \f$V_s\f$.
  @param leave_residual leave_residual Indicates whether the residual network required
  for partial calculations should be left instantiated or not. For
  default, the residual network and the graph above if
  the network has several sources or sinks, they are released into
  end of the calculation. If the parameter is true, then the network
  residual is instantiated and eventually usable for
  other algorithms.

  @return Maximized value of the flow (which is equal to the capacity
  the minimum cut) 

  @see Edmonds_Karp_Maximum_Flow Random_First_Preflow_Maximum_Flow Priority_First_Preflow_Maximum_Flow Breadth_First_Preflow_Maximum_Flow Depth_First_Preflow_Maximum_Flow Random_Preflow_Maximum_Flow Heap_Preflow_Maximum_Flow  Fifo_Preflow_Maximum_Flow Ford_Fulkerson_Maximum_Flow

  @ingroup  Redes
*/
  template <class Net, template <class> class Maxflow>
typename Net::Flow_Type min_cut(Net &                             net, 
                                Aleph::set<typename Net::Node*> & vs, 
                                Aleph::set<typename Net::Node*> & vt,
                                DynDlist<typename Net::Arc *> &   cuts,
                                DynDlist<typename Net::Arc *> &   cutt,
                                const bool leave_residual = false)
{
  Maxflow <Net> () (net, true); /* Calcula flujo máximo y red residual*/

  typename Net::Node * source = net.get_source();

  vs_ptr = &vs;
  depth_first_traversal <Net, Res_F<Net>> 
    (net, source, &add_vertex_to_vs);
  const size_t size_vt = net.get_num_nodes() - vs.size();
  for (Node_Iterator<Net> it(net); it.has_current() and 
         vt.size() < size_vt; it.next())
    {
      typename Net::Node * p = it.get_current();
      if (not IS_NODE_VISITED(p, Aleph::Depth_First))
        vt.insert(p);
    }     

  for (Arc_Iterator<Net, No_Res_Arc<Net> > it(net); it.has_curr(); it.next())
    {
      typename Net::Arc * arc = it.get_current();
      if (arc->flow == 0) /* ¿Candidato a arco de retroceso? */
        if (vt.count(net.get_src_node(arc)) > 0 and /* ¿De vt hacia vs? */
            vs.count(net.get_tgt_node(arc)) > 0)
          {
            cutt.insert(arc);
            continue;
          }

      if (arc->flow == arc->cap) /* ¿Candidato a arco de cruce? */
        if (vs.count(net.get_src_node(arc)) > 0 and /* ¿De vs hacia vt? */
            vt.count(net.get_tgt_node(arc)) > 0)
          cuts.insert(arc); 
    }

  if (not leave_residual)
    { 
      net.unmake_residual_net();
      net.unmake_super_nodes();
    }

  return source->out_flow;
}

/** 
  @brief Invoking class calculation algorithm according to the minimal cut as the
  maximum flow. 

  @details Min_Cut() receives a trained network whose flow is maximized and
  then, from the maximum flow, minimum cut is calculated. The
  type routine receives two parameters:
  -# Net: Network class, which must be derived from the Net_Graph
  class.
  -# Maxflow: Class calculation algorithm maximum flow to be used.

  @see min_cut()

  @ingroup  Redes
*/
  template <class Net, 
            template <class> class Maxflow = Heap_Preflow_Maximum_Flow>
class Min_Cut
{
public:

/** 
  @brief Invoke to min_cut().

  @param[in,out] net Network to maximize the flow and which
  want to calculate a minimum cut
  @param[out] vs Nodes set \f$V_s\f$.
  @param[out] vt Nodes set \f$V_t\f$.
  @param[out] cuts Set of arches ranging from \f$V_s\f$ to
  \f$V_t\f$. This is the cut.
  @param[out] cutt Set of arches ranging from \f$V_t\f$ to
  \f$V_s\f$.
  
  @return Maximized value of the flow (which is equal to the capacity
  the minimum cut) 
*/
  typename Net::Flow_Type operator () (Net &                             net, 
                                       Aleph::set<typename Net::Node*> & vs, 
                                       Aleph::set<typename Net::Node*> & vt,
                                       DynDlist<typename Net::Arc *> &   cuts,
                                       DynDlist<typename Net::Arc *> &   cutt)
  {
    return min_cut <Net, Maxflow> (net, vs, vt, cuts, cutt);
  }
};

/** 
  @brief Class filter of arc as a dimension of remaining flow.

  @details This class, for use with iterators wrappers
  (Wrapper_Iterator), filter arches of a network (which can be
  residual), the flow of value is greater than or equal to attribute
  global Flow_Filter<N>::min.

  @ingroup Redes
*/
template <class N> class Flow_Filter
{

public:

/** @brief Value of dimension flow to filter */
  static typename N::Flow_Type min;

  typename N::Arc * operator () (typename N::Node *, typename N::Arc * arc) 
  { 
    return arc->cap - arc->flow >= min ? arc : NULL; 
  }

  typename N::Arc * operator () (typename N::Arc * arc) 
  { 
    return arc->cap - arc->flow >= min ? arc : NULL; 
  }
};

template <class N> typename N::Flow_Type Flow_Filter<N>::min = 0;

/** 
  @brief Find a way to increase by which you can increase
  flow at the given value min_slack.

  @details It's manage the following parameters type:
  -# Net: Type network trained.
  -# Find_Path: Search class (depth or width) by
  which you want to find the way.
  .

  @param[in] net Network on which you want to find the way
  increase.
  @param[out] path Way of increase (if any) with capacity
  remaining sufficient to increase the flow min_slack
  @param[in] min_slack Minimum value at which you want
  increase the flow.

  @return true if there is a way to increase capacity
  remaining sufficient to increase the flow in min_slack

  @ingroup Redes
*/
template 
<class Net, 
 template <class, template <class, class> class, class> class Find_Path>
bool find_aumenting_path(Net & net, Path<Net> & path, 
                         const typename Net::Flow_Type & min_slack)
{
  Flow_Filter<Net>::min     = min_slack;
  typename Net::Node * src  = net.get_source();
  typename Net::Node * sink = net.get_sink();

  if (not net.residual_net)
    net.make_residual_net();

  return Find_Path<Net, Node_Arc_Iterator, Flow_Filter<Net>> () 
    (net, src, sink, path);
}

/** 
  @brief Class of search of path by which increased can be increased
  flow at the given value min_slack.

  @details It's manage the following parameters type:
  -# Net: Type network trained.
  -# Find_Path: Search class (depth or width) by
  which you want to find the way. By default, search
  it is amplitude.  
  .

  @ingroup  Redes
*/
template <class Net, 
 template <class, template <class, class> class, class> class Find_Path =
          Find_Path_Breadth_First>
class Find_Aumenting_Path
{
public:

/** 
  @brief Invoke to the seeking of way to increase value
  minimum flow.

  @param[in] net Network on which you want to find the way
  increase.
  @param[out] path Way of increase (if any) with capacity
  remaining sufficient to increase the flow min_slack
  @param[in] slack Minimum value at which you want
  increase the flow.

  @return true if there is a way to increase capacity
  remaining sufficient to increase the flow in min_slack
*/
  bool operator () (Net & net, Path<Net> & path, 
                    const typename Net::Flow_Type & slack)
  {
    return find_aumenting_path <Net, Find_Path> (net, path, slack);
  }
};

/** 
  @brief Find a way to increase whereby it can decrease
  the flow at the given value min_slack.

  @details It's manage the following parameters type:
  -# Net: Type network trained.
  -# Find_Path: Search class (depth or width) by
  which you want to find the way.
  .

  @param[in] net Network on which you want to find the way
  decrement.
  @param[out] path The path of increase (if any) with flow
  sufficient to decrease the flow min_slack
  @param[in] min_slack the minimum value at which you want
  flow decrease.

  @return true if there is a way of increase with sufficient flow
  to decrease the flow min_slack.

  @ingroup  Redes
*/
    template <class Net, template <class, class> class Find_Path>
bool find_decrementing_path(Net & net, Path<Net> & path, 
                            const typename Net::Flow_Type & min_slack)
{
  Flow_Filter<Net>::min     = min_slack;
  typename Net::Node * src  = net.get_source();
  typename Net::Node * sink = net.get_sink();

  if (not net.residual_net)
    net.make_residual_net();

  return Find_Path<Net, Flow_Filter<Net> > () (net, sink, src, path);
}

/** 
  @brief Class search of path increase by which to decrease 
  the flow at the given value min_slack.

  @details It's manage the following parameters type:
  -# Net: Type network trained.
  -# Find_Path: Search class (depth or width) by
  which you want to find the way. By default, search
  it is amplitude.  
  .

  @ingroup Redes
*/
template <class Net, 
          template <class, class> class Find_Path = Find_Path_Breadth_First>
class Find_Decrementing_Path
{
public:
      
/** 
  @brief Invoke to the search of way to increase value
  minimum flow.

  @param[in] net Network on which you want to find the way
  increase.
  @param[out] path Way of increase (if any) with capacity
  remaining sufficient to increase the flow min_slack
  @param[in] slack Minimum value at which you want
  increase the flow.

  @return true if there is a way to increase capacity
  remaining sufficient to increase the flow in min_slack
*/
  bool operator () (Net & net, Path<Net> & path, 
                    const typename Net::Flow_Type & slack)
  {
    return find_decrementing_path <Net, Find_Path> (net, path, slack);
  }
};

/** 
  @brief Increases the flow of a network according to a path of increase.

  @details increase_flow(net, path, val) take the road of increase
  content "path" and increases the flow value of the "net" network in 
  the value "val".

  The library allows to calculate paths increase over the network
  residual in two ways:
  -# find_path_depth_first<Net, Res_F<Net> >(net, source, sink, path)
  -# find_path_breadth_first<Net, Res_F<Net> >(net, source, sink, path)
  .
  
  To make sense of any of these ways to find
  ways to increase the residual network on "net" should be
  instantiated. To do this, you must have previously called
  make_residual_net().

  @param[in] net Network which is to increase the flow
  by the way "path".
  @param[in] path Road of Path type where you want
  store the road increase.
  @param val Value at which you want to increase the flow.

  @see Path find_path_depth_first() find_path_breadth_first() make_residual_net()

  @throw domain_error If the residual network has not been created or if the
  minimum road link is less than "val".
  @throw invalid_argument If the increase value "val" is less than
  the minimum link of increase road.

  @ingroup  Redes
*/
    template <class Net>
void increase_flow(Net & net, Path<Net> & path, 
                   const typename Net::Flow_Type & val)
{
  if (not net.residual_net)
    throw std::domain_error("Residual net is not created");

    /* Aumentar flujo de red por el camino de aumento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      typename Net::Arc * arc = it.get_current_arc();
      typename Net::Arc * img = (typename Net::Arc *) arc->img_arc;
      if (arc->cap - arc->flow < val)
        throw std::domain_error("minimum slack of aumenting path es"
                                " smaller than val");

      arc->flow += val; 
      img->flow -= val;
      if (not arc->is_residual)
        {
          net.increase_out_flow(net.get_src_node(arc), val);
          net.increase_in_flow(net.get_tgt_node(arc), val);
        }
      else
        {
          net.decrease_in_flow(net.get_src_node(arc), val);
          net.decrease_out_flow(net.get_tgt_node(arc), val);
        }
    }
}

/** 
  @brief Decreases the flow of a network according to a path of decline.

  @details decrease_flow(net, path, val) take the road of decline
  content "path" and decreases the flow value of the "net" network in 
  the value "val".

  The library allows to calculate paths decrease over the network
  residual in two ways:
  -# find_path_depth_first<Net, Res_F<Net> >(net, sink, source, path)
  -# find_path_breadth_first<Net, Res_F<Net> >(net, sink, source, path)
  .
  To make sense of any of these ways to find
  ways to decrease the residual network on "net" should be
  instantiated. To do this, you must have previously called
  make_residual_net().

  @param[in] net Network which is to decrease the flow
  by the way "path"
  @param[in] path Road of Path type where you want
  store the road increase.
  @param[in] val Value at which you want to decrease the flow.
  
  @see Path find_path_depth_first() find_path_breadth_first() make_residual_net()

  @throw domain_error If the residual network has not been created or if
  by the way there is a decrease arc whose flow is less than "val".
  @throw invalid_argument If the increase value "val" is less than
  the minimum link of increase road.
  
  @ingroup Redes
*/
    template <class Net>
void decrease_flow(Net & net, Path<Net> & path, 
                   const typename Net::Flow_Type & val)
{
  if (not net.residual_net)
    throw std::domain_error("Residual net is not created");

    /* Decrementar flujo de red por camino de decremento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      typename Net::Arc * arc = it.get_current_arc();
      typename Net::Arc * img = (typename Net::Arc *) arc->img_arc;

      if (arc->cap - arc->flow < val)
        throw std::domain_error("minimum slack of decrementing path "
                                " is smaller than val");

      arc->flow -= val; 
      img->flow += val;
      if (not arc->is_residual)
        {
          net.decrease_out_flow(get_src_node(arc), val);
          net.decrease_in_flow(net.get_tgt_node(arc), val);
        }
      else
        {
          net.increase_out_flow(get_tgt_node(arc), val);
          net.increase_in_flow(net.get_src_node(arc), val);
        }
    }
}

} /* End namespace Aleph */

# endif // TPL_NETGRAPH_H
