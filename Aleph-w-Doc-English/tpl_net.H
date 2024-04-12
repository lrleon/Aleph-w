
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
  @file tpl_net.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_NET_H
# define TPL_NET_H

# include <limits>
# include <set>

# include <tpl_dynDlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynSetHash.H>
# include <tpl_random_queue.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>
# include <graph-traverse.H>

/**
  @defgroup Redes Redes de Flujo.
  
  @{ Redes de flujo

  A flow network is a digraph directed that abstracts a network of
  pipes through which a fluid flows. The arcs represent pipes;
  joints nodes. Each arc attaches two numeric values:
  associated with
  each arc:
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
  problems on graphs. Apart from the problem of peak flow,
  representable in many real-life situations, a network
  It trained used to solve other problems of optimization
  combinatorial: shortest path, matches, cuts, etc.

  @}
*/
using namespace Aleph;

namespace Aleph {

  template <typename Arc_Info, typename Flow_Type>
struct Net_Arc_Info : public Arc_Info
{
/** @brief Capacitance value. */
  Flow_Type cap; 

/** @brief Flow value. */
  Flow_Type flow; 

  Net_Arc_Info() {}

  Net_Arc_Info(const Arc_Info & info, Flow_Type __cap, Flow_Type __flow)
    : Arc_Info(info), cap(__cap), flow(__flow)
  {
    /* Empty */
  }
};

/** 
  @brief Arc of a network of flow implemented by adjacency lists. 
        
  @details Net_Arc models a arc of a trained network with information of
  Arc_Info type and numerical F_Type type to represent ability
  and flow. 

  The Net_Arc class is only intended to model the network. It did not
  you must be instantiated. Wear insert_arc() of the Net_Graph class
  arches to create a network.

  @see Array_graph Net_Graph Net_Node 

  @ingroup Redes
*/
  template <typename Arc_Info, typename F_Type = double>
struct Net_Arc : public Graph_Aarc<Arc_Info> /* Type that representing the flow */
{
  using Base = Graph_Aarc<Arc_Info>;

  typedef F_Type Flow_Type;

/** @brief Capacitance value. */
  Flow_Type cap; 

/** @brief Flow value. */
  Flow_Type flow; 

/** @brief Returns true if the values of capacity and
arc stream flow conditions met
(equal to or lower flow or the capacity).  */
  bool check_arc() const { return flow >= 0 and flow <= cap; }

  Net_Arc(const Arc_Info & info) : Base(info) { /* Empty */ } 

  Net_Arc(const Net_Arc & arc) 
    : Base(arc.arc_info), cap(arc.cap), flow(arc.flow)
  {
    /* Empty */
  }

  Net_Arc & operator = (const Net_Arc & arc)
  {
    if (this == &arc) 
      return *this;

    *(Base*) this = arc;
    cap             = arc.cap;
    flow            = arc.flow;

    return *this;
  }
};

/** 
  @brief Filter arcs to flow networks.

  @ingroup Redes
*/
  template <class Net>
struct Net_Filt
{
  typename Net::Node * p = NULL;

  void * cookie = NULL;

  void set_cookie(void * __cookie) { cookie = __cookie; }

  Net_Filt(typename Net::Node * s = NULL) : p(s) { /* empty */ }

  bool operator () (typename Net::Arc * a) const
  {
    typename Net::Node * src = (typename Net::Node*) a->src_node;
    if (src == p)
      return a->cap - a->flow > 0;
    return a->flow > 0;
  }

  bool operator () (const Net &, typename Net::Arc * arc)
  {
    return (*this)(arc);
  }

  typename Net::Node* get_node(typename Net::Arc * a) const
  {
    assert(p);
    return (typename Net::Node*) (a->src_node == p ? a->tgt_node : a->src_node);
  }
};


template <class Net>
using __Net_Iterator = Digraph_Iterator<Net, Net_Filt<Net>>;

template <class Net, class Show_Arc = Dft_Show_Arc<Net>>
using Net_Iterator = 
  Filter_Iterator<typename Net::Node*, __Net_Iterator<Net>, Show_Arc>;


template <class Net>
bool is_residual(typename Net::Node * src, typename Net::Arc * a)
{
  return a->tgt_node == src;
}

/** 
  @brief Returns the remaining flow as compared to that "p" node whether or not a 
  residual arc.

  @ingroup Redes
*/
template <class Net>
typename Net::Flow_Type
remaining_flow(typename Net::Node * src, typename Net::Arc * a)
{
  return is_residual<Net>(src, a) ? a->flow : a->cap - a->flow;
}

template <typename Node_Info = Empty_Class>
using Net_Node = Graph_Anode<Node_Info>;


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

  @ingroup Redes
*/
  template <class NodeT = Net_Node<Empty_Class>, 
            class ArcT  = Net_Arc<Empty_Class, double>>
struct Net_Graph : public Array_Graph<NodeT, ArcT>
{
  using Base = Array_Graph<NodeT, ArcT>;

  using Base::Base;

  using Graph = Base;
  
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

  mutable Flow_Type Infinity;

/** @brief Returns the input total capacity of the "node" node. */
  Flow_Type get_in_cap(Node * node) const 
  {
    Flow_Type sum = 0.0;
    for (_In_Iterator<Net_Graph> it(node); it.has_curr(); it.next())
      sum += it.get_curr()->cap;
    return sum;
  }

/** @brief Returns the total capacity output of "node" node. */
  Flow_Type get_out_cap(Node * node) const 
  {
    Flow_Type sum = 0.0;
    for (_Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next())
      sum += it.get_curr()->cap;
    return sum;
  }

/** @brief Returns the degree of input node
(number of arcs that affect it) */
  size_t get_in_degree(Node * p) const 
  {
    return in_degree<Net_Graph>(p); 
  }

/** @brief Returns the degree of output node
(number of arcs leaving him) */
  size_t get_out_degree(Node * p) const 
  {
    return out_degree<Net_Graph>(p);
  }

/** @brief Returns the value of outflow of the node. */
  Flow_Type get_out_flow(Node * node) const 
  {
    Flow_Type sum = 0.0;
    for (_Out_Iterator<Net_Graph> it(node); it.has_curr(); it.next())
      sum += it.get_curr()->flow;
    return sum;
  }

/** @brief Returns the value of inflow node. */
  Flow_Type get_in_flow(Node * node) const 
  {
    Flow_Type sum = 0.0;
    for (_In_Iterator<Net_Graph> it(node); it.has_curr(); it.next())
      sum += it.get_curr()->flow;
    return sum;
  }

/** @brief Returns true if node is a source. */
  bool is_source(Node * node) { return src_nodes.contains(node); }

/** @brief Returns true if node is sink. */
  bool is_sink(Node * node)  { return sink_nodes.contains(node); }

/** @brief Returns true if the network is a single source. */
  bool is_single_source() const { return src_nodes.size() == 1; }

/** @brief Returns true if the network is a single sink. */
  bool is_single_sink() const { return sink_nodes.size() == 1; }

/** @brief Returns true if the "node" node is connected.
The purpose of this method is validation,
as part of the verification that a network is connected. */
  bool is_connected(Node * p) const 
  {
    return get_in_degree(p) != 0 or get_out_degree(p) != 0;
  }

/** @brief Returns true if the "node" node satisfies the conditions of flow;
ie, that the inflow is equal to the output.*/
  bool check_node(Node * node) 
  { 
    if (not is_connected(node)) 
      return false;
    
    typename Net_Graph::Flow_Type out_flow = get_out_flow(node);
    typename Net_Graph::Flow_Type in_flow = get_in_flow(node);

    if (is_sink(node)) 
      return out_flow == 0 and in_flow >= 0;

    if (is_source(node))
      return in_flow == 0 and out_flow >= 0;

    return out_flow == in_flow;
  }

private:

  DynSetTree<Node*> src_nodes;
  DynSetTree<Node*> sink_nodes;

public:

/** @brief Returns the set of source nodes containing the network. */
  DynSetTree<Node*> & get_src_nodes() { return src_nodes; }

/** @brief Returns the set of sink nodes comprising the network. */
  DynSetTree<Node*> & get_sink_nodes() { return sink_nodes; }

public:

/** @brief Converts a network with multiple sources to a single node network with a supra-source.
Shoot exception if the network does not contain a source node or if there is insufficient memory. */
  void make_super_source()
  {
    if (src_nodes.size() == 1) 
      return;

    if (src_nodes.size() == 0)
      throw std::domain_error("network has not source node (it has cicles)");

    Node * super_source = insert_node(); 
    src_nodes.for_each([super_source, this] (Node * p)
	      {
		insert_arc(super_source, p, get_out_cap(p));
	      });

    with_super_source = true;
  }

/** @brief Restores a network with a supra-source node to the original network with multiple source nodes. */
  void unmake_super_source()
  {
    if (not with_super_source) 
      return;

    assert(src_nodes.size() == 1);

    remove_node(src_nodes.get_item());
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

    Node * super_sink = insert_node(); 
    sink_nodes.for_each([super_sink, this] (Node * p)
	      {
		insert_arc(p, super_sink, get_in_cap(p));
	      });
    with_super_sink = true;
  }

/** @brief Restores a network with a supra-sink node to the original network
several sink nodes. */
  void unmake_super_sink()
  {
    if (not with_super_sink) 
      return;

    assert(sink_nodes.size() == 1);

    remove_node(sink_nodes.get_item());
    with_super_sink = false;
  }

/** @brief Converts a network with multiple source and sink nodes to a
node network with one source and one supra-supra-sink. */
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
supra-sump to the original network with multiple nodes sources and sinks. */
  void unmake_super_nodes()
  {
    unmake_super_source();
    unmake_super_sink();
  }

/** @brief Returns a source node of the network. */
  Node * get_source() const { return src_nodes.get_item(); }

/** @brief Returns a sink node in the network. */
  Node * get_sink() const { return sink_nodes.get_item(); }

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
    Node * p = Graph::insert_node(node_info); 
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
	    src_nodes.remove(p);
	    Graph::remove_node(p);
	    throw;
	  }
      }
    catch (bad_alloc)
      {
	Graph::remove_node(p);
	throw; /* Propagar excepción */
      }
  }

/** @brief Insert a new node on the network. 
The value of information is undetermined.
Shoot bad_alloc if there is insufficient memory. */
  Node * insert_node(Node_Type && info = Node_Type()) 
  { 
    return insert_node(info); 
  }

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
    Graph::insert_node(p); 
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
	    src_nodes.remove(p);
	    Graph::remove_node(p);
	    throw;
	  }
      }
    catch (bad_alloc)
      {
	Graph::remove_node(p);
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
  @param[in] cap Arch capacity value.
  @param[in] flow Arch flow value. 
  @param[in] arc_info Attribute value to save in the new arc.
  
  @return A pointer to the new arc inserted
  
  @throw std::bad_alloc If there is insufficient memory to create the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
  Arc * insert_arc(Node * src_node, Node * tgt_node, 
		   const Flow_Type & cap, const Flow_Type & flow,
		   const typename Arc::Arc_Type & arc_info = Arc_Type())
  {     /* Inserción en clase base */
    Arc * arc = Graph::insert_arc(src_node, tgt_node, arc_info); 

    src_nodes.remove(tgt_node);  /* Actualización de source/sink */
    sink_nodes.remove(src_node); 

    arc->cap  = cap;            /* Ajuste capacidad y flujo de arco */
    arc->flow = flow;

    if (not arc->check_arc())
      throw std::overflow_error("flow is greater than capacity");

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
    Graph::connect_arc(arc); 

    Node * src = this->get_src_node(arc);
    Node * tgt = this->get_tgt_node(arc);

    src_nodes.remove(tgt);        /* Elimina destino de src_nodes */
    sink_nodes.remove(src);       /* Elimina fuente de sink_nodes */

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

  @throw std::bad_alloc If there is insufficient memory to create the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
  Arc * insert_arc(Node * src_node, Node * tgt_node, const Flow_Type & cap)
  {
    return insert_arc(src_node, tgt_node, cap, 0, Arc_Type());
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

  @throw std::bad_alloc If there is insufficient memory to create the arc.
  @throw std::overflow_error If the flow value is greater
  to the capacity.
*/
  Arc * insert_arc(Node * src_node, Node * tgt_node, 
		   const typename Arc::Arc_Type & arc_info = Arc_Type())
  {
    return insert_arc(src_node, tgt_node, 0, 0, arc_info);
  }

/** @brief Eliminates of the network the arc "arc" */
  virtual void remove_arc(Arc * arc)
  {
    Node * src = this->get_src_node(arc);
    Node * tgt = this->get_tgt_node(arc);
    if (get_in_degree(tgt) == 1)
      src_nodes.insert(tgt);  /* tgt deviene un nodo fuente */

    Graph::remove_arc(arc); /* Eliminar en clase base */

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
    if (get_in_degree(tgt) == 1)
      src_nodes.insert(tgt); /* tgt deviene un nodo fuente  */

    Graph::disconnect_arc(arc); /* Desconeción en clase base */

    if (this->get_num_arcs(src) == 0)
      sink_nodes.insert(src); /* src deviene un nodo sumidero
  }

/** @brief Removes a node of a network flow with all their arches. */
  virtual void remove_node(Node * p)          
  {
    Graph::remove_node(p);  /* Eliminación en clase base */
    src_nodes.remove(p);
    sink_nodes.remove(p);
  }

/** @brief Build a network copy of the "net" network. Shoot bad_alloc otherwise
enough memory. */
  Net_Graph(const Net_Graph & net) 
    : Array_Graph<NodeT, ArcT>::Array_Graph(),
      Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(net.with_super_source),
      with_super_sink(net.with_super_sink)
  {
    copy_graph(*this, net, false); /* Copia sin mapear */
    
    using Pair = std::pair<typename Net_Graph::Arc*, typename Net_Graph::Arc*>;
    zip(this->arcs(), net.arcs()).for_each([] (const Pair & p)
      {
	typename Net_Graph::Arc * atgt = p.first;
	typename Net_Graph::Arc * asrc = p.second;
	atgt->cap = asrc->cap;
	atgt->flow = asrc->flow;
      });
  }

/** @brief Place capacitance value to an arc. */
  void set_cap(Arc * arc, const Flow_Type & cap) 
  {
    if (cap < arc->flow)
      throw std::out_of_range("capacity value is smaller than flow");

    const Flow_Type old_cap = arc->cap;
    arc->cap = cap;
  }

/** @brief Place flow value to an arc.
Shoot exception if the value is greater than capacity. */
  void set_flow(Arc * arc, const Flow_Type & flow)
  {
    if (flow > arc->cap)
      throw std::out_of_range("flow value is greater than capacity");

    const Flow_Type old_flow = arc->flow;
    arc->flow = flow;
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

	@details check_network() through all the nodes and arcs of a network
  trained in search of inconsistencies
  definition. Basically, it is verified that for every node
  not sink or source inflow quantity is equal to
  the ledge. For each inspected arc verified that the
  flow is less than or equal capacity.

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
  Flow_Type flow_value() const 
  {
    assert(get_in_flow(get_source()) == get_out_flow(get_sink()));
    return get_out_flow(get_source()); 
  }

/** @brief Returns true if the network has added a source supra. */
  bool with_super_source; 

/** @brief Returns true if the network has added an supra sink. */
  bool with_super_sink;

  Net_Graph() 
    : Infinity(numeric_limits<typename Arc::Flow_Type>::max()),
      with_super_source(false), with_super_sink(false)
  { /* empty */ }

  friend ostream & operator << (ostream & s, const Path<Net_Graph> & path)
  {
    if (path.is_empty())
      return s << "Path is Empty";

    const Net_Graph & net = path.get_graph();
    typename Path<Net_Graph>::Iterator it(path);
    s << it.get_current_node()->get_info();
    for (; it.has_current_arc(); it.next())
      {
	typename Net_Graph::Arc * a = it.get_current_arc();
	s << "(" << a->cap << "," << a->flow << ")" 
	  << net.get_connected_node(a, it.get_current_node())-> get_info();
      }
    return s;
  }
};

/** 
  @brief Defines an arc that belongs to a semipath
   
  @ingroup Redes
*/
template <class Net>
using Parc = tuple<typename Net::Arc*, bool>; /* 2do campo dice si normal */

/** 
  @brief Defines a semipath comprising:
    -. bool: Says if the road was or was not found
    -. Flow_Type: the value of the slack of the road
    -. List of arc pairs and indication whether it is normal or reduced. true ==> normal

  @ingroup Redes
*/
template <class Net>
using SemiPath = tuple<bool, typename Net::Flow_Type, DynList<Parc<Net>>>;

template <class Net>
inline void print(const DynList<Parc<Net>> & sp)
{
  if (sp.is_empty())
    {
      cout << "Semi path is Empty";
      return;
    }

  for (typename DynList<Parc<Net>>::Iterator it(sp); it.has_curr(); it.next())
    {
      const Parc<Net> & pa = it.get_curr();
      typename Net::Arc * a = get<0>(pa);
      typename Net::Node * s = (typename Net::Node *) a->src_node;
      typename Net::Node * t = (typename Net::Node *) a->tgt_node;
      cout << s->get_info() << "(" << a->flow << "," << a->cap << ")" 
	   << t->get_info() << " " << (get<1>(pa) ? "Normal" : "Reduced") 
	   << endl;
    }
}

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
typename Net::Flow_Type increase_flow(Net & net, const Path<Net> & path)
{
  typename Net::Flow_Type slack = net.Infinity; /* Eslabón mínimo */
  using Tuple = tuple<typename Net::Node*, typename Net::Arc*>;

    /* Calcular el eslabón mínimo del camino de aumento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      Tuple t = it.get_pair();
      typename Net::Node * p = get<0>(t);
      typename Net::Arc * arc = get<1>(t);
      const typename Net::Flow_Type w = remaining_flow<Net>(p, arc);
      if (w < slack)
        slack = w;
    }

    /* Aumentar el flujo de la red por el camino de aumento */
  for (typename Path<Net>::Iterator it(path); it.has_current_arc(); it.next())
    {
      Tuple t = it.get_pair();
      typename Net::Node * p = get<0>(t);
      typename Net::Arc * arc = get<1>(t);

      if (is_residual<Net>(p, arc))
	arc->flow -= slack; 
      else
	arc->flow += slack; 

      assert(arc->check_arc());
    }

  return slack;
}


/** 
  @brief Increases the flow of a network as a road of increase.

  @details increase_flow(net, semi_path) takes the path of increase
  content path
  and increases the value of net flow in the minimum link
  (slack) of the road.

  @param[in] net The network capable where you want to increase
  the flow by the path increase.
  @param[in] semi_path Semipath by where the flow is increased
  @param[in] slack dl path increase value. Must be less than or
  equal to the minimum path slack.

  @warning The slack is not valid

  @ingroup Redes
*/
    template <class Net>
void increase_flow(Net & net, 
		   const DynList<Parc<Net>> & semi_path,
		   const typename Net::Flow_Type slack)
{
    /* Aumentar el flujo de la red por el camino de aumento */
  for (typename DynList<Parc<Net>>::Iterator it(semi_path); it.has_curr(); 
	 it.next())
    {
      const Parc<Net> p = it.get_curr();
      typename Net::Arc * arc = get<0>(p);
      if (get<1>(p)) /* Es arco de normal? */
	arc->flow += slack; 
      else
	arc->flow -= slack; 

      assert(arc->check_arc());
    }
  assert(net.check_network());
}
  
/** 
  @brief Decreases the flow of a network according to a path increase.

  @details decrease_flow(net, semi_path) takes the path of increase
  content path
  and decreases the value of "net" flow in the minimum link
  (slack) of the road.

  @param[in] net Trained network where you want to increase
  the flow path increase.
  @param[in] semi_path Semipath by where the flow is increased
  @param[in] slack dl path increase value. Must be less than or
  equal to the minimum path slack.

  @warning The slack is not valid

  @ingroup Redes
*/
    template <class Net>
void decrease_flow(Net & net, const DynList<Parc<Net>> & semi_path,
		   typename Net::Flow_Type slack)
{
  increase_flow(net, semi_path, slack);
}


/** 
  @brief Search paths of increase on directed graphs defined
  through a class graph (not digraph).

  @ingroup Redes
*/
  template <class Net, template <typename T> class Q>
class Find_Aumenting_Path
{
  const Net & net;

/** @brief Returna "end" node if there is a way */
  typename Net::Node * search(typename Net::Node* start, 
			      typename Net::Node* end,
			      typename Net::Flow_Type min_slack)
  {
    using Itor = Net_Iterator<Net>;
    net.reset_nodes();
    net.reset_arcs();
    
    start->set_state(Processed);
    Q<typename Net::Arc*> q;
    for (Itor it(start); it.has_curr(); it.next())
      {
	typename Net::Arc * a = it.get_curr();
	if (remaining_flow<Net>(start, a) < min_slack)
	  continue;
	typename Net::Node * tgt = net.get_connected_node(a, start);
	tgt->set_state(Processing);
	a->set_state(Processing);
	q.put(a);
      }

    typename Net::Node * curr = NULL;
    while (not q.is_empty())
      {
	typename Net::Arc * arc = q.get();
	assert(arc->state() == Processing);
	arc->set_state(Processed);
	
	typename Net::Node * s = net.get_src_node(arc);
	typename Net::Node * t = net.get_tgt_node(arc);
	
	if (s->state() == Processed and t->state() == Processed)
	  continue;
	
	curr = s->state() == Processed ? t : s;
	assert(curr->state() == Processing);
	curr->set_state(Processed);
	NODE_COOKIE(curr) = net.get_connected_node(arc, curr);

	if (curr == end)
	  return curr;

	for (Itor it(curr); it.has_curr(); it.next()) 
	  {
	    typename Net::Arc * a  = it.get_curr();
	    if (remaining_flow<Net>(curr, a) < min_slack)
	      continue;
	    a->set_state(Processing);

	    typename Net::Node * tgt = net.get_connected_node(a, curr);
	    if (tgt->state() == Processed)
	      continue;

	    if (tgt->state() != Processed)
	      {
		q.put(a);
		tgt->set_state(Processing);
	      }
	    else
	      a->set_state(Processed);
	  }
      } /* End while */

    return NULL;
  }

  Path<Net> find(typename Net::Node * start, typename Net::Node * end,
		 const typename Net::Flow_Type & min_slack = 0.0)
  {
    typename Net::Node * curr = search(start, end, min_slack);

    Path<Net> ret(net);
    if (not curr)
      return ret;

    assert(curr == end);

    while (curr != start)
      {
	ret.insert(curr);
	curr = (typename Net::Node*) NODE_COOKIE(curr);
      }
    ret.insert(start);

    return ret;
  }

  SemiPath<Net> find_path(typename Net::Node * start,
			  typename Net::Node * end,
			  typename Net::Flow_Type min_slack = 0.0)
  {
    typename Net::Node * t = search(start, end, min_slack);
    if (not t)
      return make_tuple(false, 0, DynList<Parc<Net>>());

    assert(t == end);

    DynList<Parc<Net>> semi_path;
    typename Net::Flow_Type m = 
      std::numeric_limits<typename Net::Flow_Type>::max();
    while (t != start)
      {
	typename Net::Node * s = (typename Net::Node*) NODE_COOKIE(t);
	typename Net::Arc ** a_ptr = 
	  net.arcs(t).find_ptr([s, t] (typename Net::Arc * a)
          {
	    return ((a->src_node == s and a->tgt_node == t) or 
		    (a->src_node == t and a->tgt_node == s));
	  });
	assert(a_ptr);
	typename Net::Arc * a = *a_ptr;
	bool normal = a->tgt_node == t;
	typename Net::Flow_Type slack = normal ? a->cap - a->flow : a->flow;
	m = std::min(m, slack);
	semi_path.insert(make_tuple(a, normal));
	t = s;
      }

    return make_tuple(true, m, move(semi_path));
  }

public:

/** @brief Finds a way of increasing one slack */
  SemiPath<Net> find_aum_path(typename Net::Flow_Type min_slack = 0.0)
  {
    return find_path(net.get_source(), net.get_sink(), min_slack);
  }

/** @brief Finds a way to decrease at least one slack */
  SemiPath<Net> find_dec_path(typename Net::Flow_Type min_slack = 0.0)
  {
    return find_path(net.get_sink(), net.get_source(), min_slack);
  }

  Find_Aumenting_Path(const Net & __g) : net(__g)
      {
	       /* Empty */
      }

  Path<Net> operator () (typename Net::Node *    start, 
			 typename Net::Node *    end,
			 typename Net::Flow_Type min_slack = 0) 
  {
    return find(start, end, min_slack);
  }

  SemiPath<Net> operator () (typename Net::Flow_Type min_slack = 0) 
  {
    return find_aum_path(min_slack);
  }

  typename Net::Flow_Type 
  semi_path(typename Net::Node *            start, 
	    typename Net::Node *            end,
	    DynList<Parc<Net>> &            semi_path,
	    const typename Net::Flow_Type & min_slack = 0) 
  {
    return find_semi_path(start, end, semi_path, min_slack);
  }
};


template <class Net>
using Find_Aumenting_Path_DFS = Find_Aumenting_Path<Net, DynListStack>;


template <class Net>
using Find_Aumenting_Path_BSF = Find_Aumenting_Path<Net, DynListQueue>;


/** 
  @brief Finds a way to increase by a depth-first search
  or amplitude as the template parameter value "Q". "Q" can
  be a stack, in which case the search will cross, or
  tail, in which case the search will amplitude

  @param[in] net The network capable of flow
  @param[in] min_slack Minimum value of road decrement 
  @param[in] sa Arches filter value

  @ingroup Redes
*/
template <class Net, template <typename T> class Q>
Path<Net> find_aumenting_path(const Net &                     net,
			      const typename Net::Flow_Type & min_slack)
{
  typename Net::Node * s  = net.get_source();
  typename Net::Node * t = net.get_sink();
  return Find_Aumenting_Path<Net, Q> (net) (s, t, min_slack);
}


/** 
  @brief Finds a way to increase by a depth-first search.

  @param[in] net The network capable of flow
  @param[in] min_slack Minimum value of road increase
  @param[in] sa Arches filter value

  @ingroup Redes
*/
template <class Net>
Path<Net> find_aumenting_path_dfs(const Net &                     net,
				  const typename Net::Flow_Type & min_slack)
{
  return find_aumenting_path<Net, DynListStack> (net, min_slack);
}


/** 
  @brief Find a path of increase through a search in amplitude.

  @param[in] net The network capable of flow
  @param[in] min_slack Minimum value of road increase
  @param[in] sa Arches filter value

  @ingroup Redes
*/
template <class Net>
Path<Net> find_aumenting_path_bfs(const Net &                     net,
				  const typename Net::Flow_Type & min_slack)
{
  return find_aumenting_path<Net, DynListQueue> (net, min_slack);
}


/** 
  @brief Finds a way to increase as semipath by a 
  search depth or amplitude as the value of
  template parameter "Q". "Q" may be a stack, in which case
  search will cross, or a queue, in which case the search
  will be amplitude

  @param[in] net The network capable of flow
  @param[out] semi_path List of pairs (Arc,bool) describing the semipath
  @param[in] min_slack Minimum value of road decrement 
  @param[in] sa Arches filter value
  
  @return Returns in the semipath slack. A value equal to that given in parameter
  it indicates that the road was found satisfactory

  @ingroup Redes
*/
template <class Net, template <typename T> class Q>
SemiPath<Net> find_aumenting_semi_path(const Net &                     net,
				       const typename Net::Flow_Type & slack)
{
  return Find_Aumenting_Path<Net, Q> (net). find_aum_path(slack);
}


/** 
  @brief Finds a way to increase as semipath by
  depth-first search.

  @param[in] net The network capable of flow
  @param[out] semi_path List of pairs (Arc,bool) describing the semipath
  @param[in] min_slack Minimum value of road decrement 
  @param[in] sa Arches filter value
  
  @return Returns in the semipath slack. A value equal to that given in parameter
  it indicates that the road was found satisfactory

  @ingroup Redes
*/
template <class Net> 
SemiPath<Net> 
find_aumenting_semi_path_dfs(const Net &                     net,
			     const typename Net::Flow_Type & slack)
{
  return find_aumenting_semi_path<Net, DynListStack> (net, slack);
}


/** 
  @brief Finds a way to increase as semipath by
  breadth-first search.

  @param[in] net The network capable of flow
  @param[out] semi_path List of pairs (Arc,bool) describing the semipath
  @param[in] min_slack Minimum value of road decrement 
  @param[in] sa Arches filter value

  @return Returns in the semipath slack. A value equal to that given in parameter
  it indicates that the road was found satisfactory

  @ingroup Redes
*/
template <class Net>
SemiPath<Net> 
find_aumenting_semi_path_bfs(const Net &                     net,
			     const typename Net::Flow_Type & slack)
{
  return find_aumenting_semi_path<Net, DynListQueue> (net, slack);
}


/** 
  @brief Finds a way to decrease by a depth-first search
  or amplitude as the value of the template "Q" parameter. 
  
  @details "Q" may be a stack, in which case the search will be in
  depth, or a queue, in which case the search will amplitude

  @param[in] net red Capacitada de flujo
  @param[in] min_slack Minimum value of road decrement 
  @param[out] semi_path Lista de pares arco,forward que caracteriza el semi-camino 
  @param[in] sa Arches filter value
  
  @return The value of slack. If it is equal to that requested, then the
  semi-way was found

  @ingroup Redes
*/
template <class Net, template <typename T> class Q>
SemiPath<Net>
find_decrementing_path(const Net &                     net,
		       const typename Net::Flow_Type & slack)
{
  return Find_Aumenting_Path<Net, Q>(net).find_dec_path(slack);
}


/** 
  @brief Find a path of increase through a search in amplitude.

  @param[in] net The network capable of flow
  @param[in] min_slack Minimum value of road increase
  @param[in] sa Arches filter value

  @ingroup Redes
*/
template <class Net>
SemiPath<Net>
find_decrementing_path_dfs(const Net &                     net,
			   const typename Net::Flow_Type & slack)
{
  return find_decrementing_path<Net, DynListStack> (net, slack);
}


/** 
  @brief Find a path of increase through a search in amplitude.

  @param[in] net The network capable of flow
  @param[in] min_slack Minimum value of road increase
  @param[in] sa Arches filter value

  @ingroup Redes
*/
template <class Net>
SemiPath<Net> 
find_decrementing_path_bfs(const Net &                     net,
			   const typename Net::Flow_Type & slack)
{
  return find_decrementing_path<Net, DynListQueue> (net, slack);
}


/** 
  @brief Maximizes the network flow according searches of paths of increase.

  @details aumenting_path_maximum_flow() receives a trained network and
  maximizes flow value by successive searches of
  increase paths.

  The method takes three type parameters:
  -# Net: Network type whose flow is to be maximized and should
  Net_Graph be derived.
  -# Find_Path: Class of search of path.

  The network can have multiple sources and sinks.

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] sa Estado del filtro de arcos

  @return The maximized flow value of the network

  @ingroup Redes
*/
template <class Net, 
	  template <class> class Find_Path>
typename Net::Flow_Type aumenting_path_maximum_flow(Net & net)
{
  net.make_super_nodes();

  while (true) /* Mientras exista un camino de aumento */
    {
      SemiPath<Net> semi_path = Find_Path<Net> (net) ();
      if (not get<0>(semi_path))
	break;
	  
      increase_flow <Net> (net, get<2>(semi_path), get<1>(semi_path));
    }

  net.unmake_super_nodes();

  return net.get_out_flow(net.get_source());
}


/** 
  @details Maximizes the network flow according to the algorithm
  Ford-Fulkerson.

  @details ford_fulkerson_maximum_flow() receives a trained network and
  it maximizes flow value by the Ford-Fulkerson
  algorithm.

  The network can have multiple sources and sinks.

  During the calculation a partial residual network is built, whose
  space consumption is proportional to the amount of edges.

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] sa Filter of arches

  @return The maximized flow value of the network

  @throw bad_alloc If there is insufficient memory to the network
  waste and increase internal roads. In this case,
  the network status corresponds to the last increment of
  flow done before it occurs exception.
  @throw domain_error If the residual network has already been calculated.

  @ingroup Redes
*/
template <class Net>
typename Net::Flow_Type ford_fulkerson_maximum_flow(Net & net)
{
  return aumenting_path_maximum_flow <Net, Find_Aumenting_Path_DFS> (net);
} 

/** 
  @brief Invoking class the Ford Fulkerson algorithm.

  @see ford_fulkerson_maximum_flow()
  
  @ingroup Redes
*/
template <class Net> 
struct Ford_Fulkerson_Maximum_Flow 
{

/** 
  @brief Invoke to ford_fulkerson_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] sa Filter of arches

  @return The maximized flow value of the network
*/
  typename Net::Flow_Type operator () (Net & net) const
  {
    return ford_fulkerson_maximum_flow(net);
  }
};

/** 
  @brief Maximizes network flow according to the algorithm
  Edmonds-Karp.

  @details edmonds_karp_maximum_flow() receives a network
  capable and maximizes their value flow through the algorithm
  Edmonds-Karp.

  The network can have multiple sources and sinks.

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] sa Filter of arches

  @return The maximized flow value of the network

  @ingroup Redes
*/
template <class Net>
typename Net::Flow_Type edmonds_karp_maximum_flow(Net & net)
{
  return aumenting_path_maximum_flow <Net, Find_Aumenting_Path_BSF> (net);
}


/** 
  @brief Invoking class the Ford Fulkerson algorithm.

  @see edmonds_karp_maximum_flow()
  
  @ingroup Redes
*/
template <class Net> 
struct Edmonds_Karp_Maximum_Flow
{
  
/** 
  @brief Invoke to edmonds_karp_maximum_flow().

  @param[in,out] net Capable network whose flow is to be maximized. 
  @param[in] sa Filter of arches
  
  @return The maximized flow value of the network
*/
  typename Net::Flow_Type operator () (Net & net) const
  {
    return edmonds_karp_maximum_flow<Net>(net);
  }
};

    template <class Net> static inline
bool is_node_active(const Net & net, typename Net::Node * p) 
{
  return net.get_in_flow(p) > net.get_out_flow(p); 
}

    template <class Net> static inline
long & node_height(typename Net::Node * p) { return NODE_COUNTER(p); }


    template <class Net> static inline
void init_height_in_nodes(Net & net)
{
  Graph_Traverse_DFB<Net, In_Iterator<Net>>(net).
    exec(net.get_sink(), [&net] (typename Net::Node * p, typename Net::Arc * a)
	 {
	   if (a)
	     node_height<Net>(p) = node_height<Net>(net.get_tgt_node(a)) + 1;
	   return true;
	 });
}


    template <class Q_Type> static inline
void put_in_active_queue(Q_Type & q, typename Q_Type::Item_Type & p)
{
  if (NODE_BITS(p).get_bit(Aleph::Maximum_Flow))
    return;

  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, true);
  q.put(p);
}

    template <class Q_Type> static inline
typename Q_Type::Item_Type get_from_active_queue(Q_Type & q)
{
  typename Q_Type::Item_Type p = q.get();

  assert(NODE_BITS(p).get_bit(Aleph::Maximum_Flow));

  NODE_BITS(p).set_bit(Aleph::Maximum_Flow, false);

  return p;
}


   template <class Q_Type> static inline
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
  -# Net: Clase de red capacitada derivada del tipo Net_Graph.
  -# Q_Type: Clase conjunto de "nodos activos". Q_Type debe
  manejar punteros de tipo typename Net::Node y debe soportar
  las siguientes operaciones:
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
generic_preflow_vertex_push_maximum_flow(Net & net)
{
  net.make_super_nodes();
  net.reset_nodes();
  init_height_in_nodes(net); 
  typename Net::Node * source = net.get_source();
  typename Net::Node * sink   = net.get_sink();

  Q_Type q;  /* Instancia el conjunto de nodos activos */
  try
    {
      using Itor = Net_Iterator<Net>;
      for (Itor it(source); it.has_curr(); it.next())
	{
	  typename Net::Arc * arc  = it.get_curr();  
	  typename Net::Node * tgt = net.get_connected_node(arc, source); 
	  arc->flow = arc->cap - arc->flow; /* Inunde arco */
	  if (tgt != sink)
	    put_in_active_queue(q, tgt); /* tgt deviene activo */
	}

      while (not q.is_empty()) /* Mientras haya nodos activos */
	{
	  typename Net::Node * src = get_from_active_queue(q);
	  typename Net::Flow_Type excess = 
	    net.get_in_flow(src) - net.get_out_flow(src);

	  for (Itor it(src); it.has_curr() and excess > 0; it.next()) 
	    {
	      typename Net::Arc * arc  = it.get_curr();
	      typename Net::Node * tgt = net.get_connected_node(arc, src);
	      if (node_height<Net>(src) != node_height<Net>(tgt) + 1)
		continue; /* El nodo no es elegible */

	      typename Net::Flow_Type flow_to_push;
	      if (is_residual<Net>(src, arc))
		{
		  const typename Net::Flow_Type flow_avail_in_arc = arc->flow;
		  flow_to_push = std::min(flow_avail_in_arc, excess);
		  arc->flow -= flow_to_push;
		}
	      else
		{
		  const typename Net::Flow_Type flow_avail_in_arc = 
		    arc->cap - arc->flow;  
		  flow_to_push = std::min(flow_avail_in_arc, excess);
		  arc->flow += flow_to_push;
		}

	      excess -= flow_to_push;
	      if (is_node_active(net, tgt) and tgt != sink and tgt != source) 
		put_in_active_queue(q, tgt);
	    }

    /** ¿src aún sigue activo?
    Si la respuesta es SI, entonces incremente h(src) y re-inserte en q */
	  if (excess > 0) 
	    { 
	      node_height<Net>(src)++;
	      put_in_active_queue(q, src);
	    }
	}

      net.unmake_super_nodes();
      return net.flow_value(); 
    } 
  catch (...)
    {
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
fifo_preflow_maximum_flow(Net & net)
{
  return generic_preflow_vertex_push_maximum_flow
    <Net, DynListQueue<typename Net::Node*>>(net);
}

/** 
  @brief Invoking class to fifo_preflow_maximum_flow()

  @see fifo_preflow_maximum_flow()

  @ingroup Redes
*/
template <class Net> 
struct Fifo_Preflow_Maximum_Flow
{
  
/** 
  @brief Invoke to fifo_preflow_maximum_flow().

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
  operator () (Net & net) const
  {
    return fifo_preflow_maximum_flow(net);
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
heap_preflow_maximum_flow(Net & net)
{
  return generic_preflow_vertex_push_maximum_flow 
    <Net, DynBinHeap<typename Net::Node*, Compare_Height<Net>>> (net);
}

/** 
  @brief Invoking class to heap_preflow_maximum_flow()

  @see heap_preflow_maximum_flow()
  
  @ingroup Redes
*/
  template <class Net> 
struct Heap_Preflow_Maximum_Flow
{
  
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
  typename Net::Flow_Type operator () (Net & net) const
  {
    return heap_preflow_maximum_flow(net);
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
random_preflow_maximum_flow(Net & net)
{
  return generic_preflow_vertex_push_maximum_flow
    <Net, Random_Set<typename Net::Node*>> (net);
}


/** 
  @brief Invoking class to random_preflow_maximum_flow()

  @see random_preflow_maximum_flow()

  @ingroup Redes
*/
      template <class Net> 
struct Random_Preflow_Maximum_Flow
{
  

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
  typename Net::Flow_Type operator () (Net & net) const
  {
    return random_preflow_maximum_flow(net);
  }
};


/** 
  @brief Calculate the maximum flow of a trained network and determines the cut
  minimum.  

  @details min_cut() receives a trained network whose flow is maximized and
  then, from the maximum flow, minimum cut is calculated. The
  type routine receives two parameters:
  -# Net: Network class, which must be derived from the Net_Graph class.
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
                                DynSetTree<typename Net::Node*> & vs, 
                                DynSetTree<typename Net::Node*> & vt,
                                DynList<typename Net::Arc *> &    cuts,
                                DynList<typename Net::Arc *> &    cutt)
{
  Maxflow <Net> () (net); /* Calcula flujo máximo */

  typename Net::Node * source = net.get_source();

    /* Recorre en profundidad la red residual desde source. 
    Todo lo alcanzable pertenece a vs y por tanto es insertado */
  (Graph_Traverse_DFB<Net, Net_Iterator<Net>> (net))
   (source, [&vs] (typename Net::Node * p)
      {
	vs.insert(p); return true;
      });

  /* Calcula vt por el complemento */
  const size_t size_vt = net.get_num_nodes() - vs.size();
  for (Node_Iterator<Net> it(net); it.has_curr() and 
         vt.size() < size_vt; it.next())
    {
      typename Net::Node * p = it.get_curr();
      if (p->state() == Unprocessed)
        vt.insert(p);
    }     

  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next())
    {
      typename Net::Arc * arc = it.get_current();
      if (arc->flow == 0) /* ¿Candidato a arco de retroceso? */
        if (vt.contains(net.get_src_node(arc)) and /* ¿De vt hacia vs? */
            vs.contains(net.get_tgt_node(arc)) > 0)
          {
            cutt.insert(arc);
            continue;
          }

      if (arc->flow == arc->cap) /* ¿Candidato a arco de cruce? */
        if (vs.contains(net.get_src_node(arc)) and /* ¿De vs hacia vt? */
            vt.contains(net.get_tgt_node(arc)) > 0)
          cuts.insert(arc); 
    }

  return net.flow_value();
}


/** 
  @brief Invoking class calculation algorithm according to the minimal cut as the
  maximum flow. 

  @details Min_Cut() receives a trained network whose flow is maximized and
  then, from the maximum flow, minimum cut is calculated. The
  type routine receives two parameters:
  -# Net: Network class, which must be derived from the Net_Graph class.
  -# Maxflow: Class calculation algorithm maximum flow to be used. 

  @see min_cut()

  @ingroup  Redes
*/
  template <class Net, 
            template <class> class Maxflow = Heap_Preflow_Maximum_Flow>
struct Min_Cut
{

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
      
  @return Maximized the value of flow
  (which is equal to the minimum capacity of cutting)
*/
  typename Net::Flow_Type operator () (Net &                             net, 
                                       DynSetTree<typename Net::Node*> & vs, 
                                       DynSetTree<typename Net::Node*> & vt,
                                       DynList<typename Net::Arc *> &    cuts,
                                       DynList<typename Net::Arc *> &    cutt)
  {
    return min_cut <Net, Maxflow> (net, vs, vt, cuts, cutt);
  }
};


} /* End namespace Aleph */

# endif /* TPL_NET_H
