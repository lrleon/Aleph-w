
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
  @file tpl_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_GRAPH_H
# define TPL_GRAPH_H

# include <memory>
# include <bitArray.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynDlist.H>
# include <tpl_treapRk.H>
# include <aleph-graph.H>
# include <filter_iterator.H>

    
using namespace Aleph;

namespace Aleph {

template <typename Node_Info> struct Graph_Node; 

template <typename Arc_Info> struct Graph_Arc; 

class Arc_Node;

    template <class GT> 
class Path;

    template <typename __Graph_Node, typename __Graph_Arc> 
class List_Graph;

    template <typename __Graph_Node, typename __Graph_Arc> 
class List_Digraph;

    template <class GT> 
class Mat_Graph;

    template <typename MT, typename Entry_Info, typename Copy> 
class Ady_MaT;


/** 
  @brief Implanted node graph with adjacency lists.

  @details This class defines a node or vertex of a graph or digraph
  implemented by adjacency lists.

  There are basically three ways to define the information
  it belongs to a node of a graph:
  - By parameter inherent to the node type called\c Node_Info. 
  - Through inheritance of this base class. This is the way
  keep different types of node in a graph.
  - Combination of the previous two.

  @param Node_Info Type of data stored in the network and accessible
  by the method \c get_info().

  @note This class is only for the definition of nodes
  a graph. It is not intended to be used by the user. It is
  Preferably through using the methods provided by \ c
  List_Graph. While it is possible to use some methods or access
  some of its attributes, it is highly recommended not to use it.
  Instead, use the type \ c List_Graph :: Node
  and their associated methods.

  A graph node has three additional attributes present by
  each node and named as follows:
  - Control Bits (\ c Bit_Fields): Set of bits used by
  algorithms to mark the node.
  - Counter (\ c counter): usually used to mark Contador
  an order or processing business.
  - \ C cookie: opaque pointer to any other information
  time you want to associate with the node.

  @ingroup Grafos

  @see List_Graph List_Digraph Graph_Arc Bit_Fields
*/
    template <typename __Node_Info = Empty_Class> 
struct Graph_Node : public Dlink
{
  friend class Arc_Node;

  typedef __Node_Info Node_Info;
   
  GRAPH_NODE_COMMON(Graph_Node);

/** 
  @brief Default builder. 

  @details The value of the information contained in the node is determined
  by the value assigned by the constructor \c Node_Info::Node_Info,
  where \c Node_Info is the type associated with the node.

  The values of the control bits and the counter are placed in
  zero. The \c cookie is placed in \c NULL.
*/
  Graph_Node() : num_arcs(0) { /* Empty */ }

/** 
  @brief Constructor assigns an attribute value.

  @details Create a node and assigns the value \c info as value
  information contained in the node.

  The values of the control bits and the counter are placed in
  zero. The \c cookie is placed in \c NULL.

  @param[in] info Information value to be assigned to
  newly created node.

  @note You must be defined the copy constructor Node_Info class.
*/
  Graph_Node(const Node_Info & info) : node_info(info), num_arcs(0)
  { 
    /* Empty */ 
  }

  Graph_Node(const Graph_Node & node) : Graph_Node(node.node_info) 
  {
    /* Empty */
  }

  Graph_Node & operator = (const Graph_Node & node)
  {
    if (&node == this)
      return *this;
    node_info = node.node_info;
    return *this;
  }

/** 
  @brief Copy builder from a pointer to node.

  @details Create a node and assigns the value \c node->get_info() and value
  information contained in the node.

  The values of the control bits and the counter are placed in
  zero. The \c cookie is placed in \c NULL.

  @param[in] node Pointer to the node from which you want to copy the
  information value to be assigned to the newly created node.

  @note You must be defined the copy constructor of the class
  Node_Info.
  @note The created instance is a node other than \c node.
*/
  Graph_Node(Graph_Node * node) : node_info(node->get_info()), num_arcs(0)
  { 
    /* Empty */ 
  }

  Dlink arc_list; 
};

/** 
  @brief Arc implanted graph adjacency lists.

  @details This class defines an arc or axis of a graph or digraph
  implemented by adjacency lists.

  There are basically three ways to define the information
  it belongs to an arc of a graph:
  - The parameter type inherent to the node named \c
  Arc_Info. 
  - Through inheritance of this base class. This is the way
  keep different types of arcs in a graph.
  - Combination of the previous two.

  @param Arc_Info Type of data stored in the network and accessible
  by the method \c get_info().

  @note This class is only for the definition of arches
  a graph. It is not intended to be used by the user.
  It is preferable to use through the methods provided by \ c
  List_Graph. While it is possible to use some methods or access
  some of its attributes, it is highly recommended not
  use. Instead, use the type \ c
  List_Graph :: Arc and its associated methods.

  An arc of graph has three additional attributes present by
  each arc and denominated as Follows:
  - Control Bits (\ c Bit_Fields): Set of bits used by
  algorithms to mark the arc.
  - Counter (\ c counter): usually used to mark Contador
  an order or processing business.
  - \ C cookie: opaque pointer to any other information
  time you wish to associate the arc.

  @ingroup Grafos
  
  @see List_Graph List_Digraph Graph_Node Bit_Fields
*/
    template <typename __Arc_Info = Empty_Class> 
struct Graph_Arc : public Dlink    
{
  typedef __Arc_Info Arc_Info;

  GRAPH_ARC_COMMON(Graph_Arc);

  Arc_Node * src_arc_node; /* Pointer to Arc_Node of source node */
  Arc_Node * tgt_arc_node; /* Pointer to Arc_Node of target node */

  Graph_Arc() 
    : src_node(NULL), tgt_node(NULL), src_arc_node(NULL), tgt_arc_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Arc(const Arc_Info & info) 
    : arc_info(info), 
      src_node(NULL), tgt_node(NULL), src_arc_node(NULL), tgt_arc_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Arc(const Graph_Arc & arc) : Graph_Arc(arc.arc_info) { /* Empty */ }

  Graph_Arc & operator = (const Graph_Arc & arc)
  {
    if (&arc == this)
      return *this;
    arc_info = arc.arc_info;
    return *this;
  }

  Graph_Arc(void * src, void * tgt, const Arc_Info & data) 
    : arc_info(data),
      src_node(src), tgt_node(tgt), src_arc_node(NULL), tgt_arc_node(NULL)
  { 
    /* Empty */
  }

  Graph_Arc(void * src, void * tgt) 
    : src_node(src), tgt_node(tgt), src_arc_node(NULL), tgt_arc_node(NULL)
  { 
    /* Empty */
  }
};

class Arc_Node : public Dlink
{
public:
  void * arc;
  Arc_Node() : arc(NULL) {}
  Arc_Node(void * __arc) : arc(__arc) {}
}; 

/** 
  @brief Class graph with adjacency lists implemented.

  @details List_Graph<Node, Arc> It is a class that models graphs
  represented by adjacency lists.

  The class manages two parameters fundamental type:
  - __Graph_Node: The type of nodes that must be defined
    by the Graph_Node class.
  - __Graph_Arc: The type of nodes that must be defined
    Graph_Arc by class.
  These classes must have been previously defined.

  Una vez instanciado un List_Graph<Node, Arc>, los nodos y
  arcos deben accederse mediante los tipos internos:
  - List_Graph<Node, Arc>::Node
  - List_Graph<Node, Arc>::Arc
 
  @param __Graph_Node Node type. You must be defined
  it from the class \c __Graph_Node, either by inclusion of
  attributes by derivation or combination of both.
  @param __Graph_Arc Arc type. You must be defined
  it from the class \c __Graph_Arc, either by inclusion of
  attributes by derivation or combination of both.
  
  @see Graph_Node Graph_Arc        
  @see List_Digraph Path 
  
  @ingroup Grafos
*/
  template <typename __Graph_Node = Graph_Node<unsigned long>,
	    typename __Graph_Arc  = Graph_Arc<unsigned long>>
class List_Graph
{
  GRAPH_ATTR_COMMON(List_Graph);

  Dlink  node_list; /* Node list */
  Dlink  arc_list;  /* Arc list */

  static Node * dlink_to_node(Dlink * p) { return (Node*) p; }

  static Arc * dlink_to_arc(Dlink * p) { return (Arc*) p; }

  static Arc_Node * dlink_to_arc_node(Dlink * p) { return (Arc_Node*) p; }

  static Arc * void_to_arc(Arc_Node * arc_node) 
  {
    return (Arc*) arc_node->arc; 
  }

private:

      template <class Cmp>
  struct Cmp_Arc
  {
    Cmp & cmp;

    Cmp_Arc(Cmp && __cmp = Cmp()) : cmp(__cmp) { /* Empty */ }

    Cmp_Arc(Cmp & __cmp) : cmp(__cmp) { /* Empty */ }

    bool operator () (Dlink * d1, Dlink * d2) const
    {
      Arc * arc1 = dlink_to_arc(d1); /* Convert dlink d2 arc Arc */
      Arc * arc2 = dlink_to_arc(d2); /* Convert dlink d2 arc Arc */
      return cmp(arc1, arc2); /* Having two arches invoke Cmp */
    }
  };

public:
  
/** 
  @brief Inserting a node whose memory has already been set aside.

  @details This method assumes a node type \c List_Graph::Node targeted
  by the parameter \c node and inserts it in the graph.

  @param[in] node A pointer to a node already created that does not belong to
  no graph. 

  @return A pointer the node to inserted.

  @note Since when the node is removed or destroyed the
  graph the operator \c delete, \c node is invoked must have been
  \b imperatively paragraph \c new.
  @note Usually, this routine should not be used. Instead you should
  used other insertion (which automatically separates memory).
*/
  inline virtual Node * insert_node(Node * node);

/** 
  @brief Create a new node in a graph.

  @This method separatesdetails memory node type \ c
  List_Graph :: Node and assigned as an attribute value \ c
  node_info.

  @param [in] node_info information to be copied on the node.
  It is imperative that there is a copy constructor Node_Info.

  @return a pointer to the newly created node.
  
  @throw bad_alloc if there is no memory for the new.
*/
  inline virtual Node * insert_node(const Node_Type & node_info);


/** 
  @overload insert_node 
*/
  inline virtual Node * insert_node(Node_Type && node_info = Node_Type());

/** 
  @brief Removes a node with all their bows.

  @details This routine removes all arcs node \ c node and then
  eliminates the same node. All occupied memory and arches
  the node is released.

  @param [in] node node to be removed.

  @note If this rate is a digraph, this method can be very
  inefficient because it requires through all the arches of digraph
  and eliminate those that target node.
  In general it is not advisable to use this routine on digraphs.
*/
  inline virtual void remove_node(Node * node);

/** 
  @brief Returns the first node in the graph.

  @details This routine should be considered as an entry point to
  graph regardless of the value of the node. Do not assume
  any consideration about what the node
  returned by this routine.

  @return A pointer to the first node in the graph.
*/
  inline Node * get_first_node() const;

  inline Arc * get_first_arc(Node *) const;

/** 
  @brief Create a new arc Between two nodes.

  @details This method Separates memory for a new arc Between the nodes
  Previously defined, \ c src_node and \ c tgt_node with value of
  arc_info content in the arc.

  The nodes Must Have Been Previously inserted in the graph. in
  esta respect, no check is made. The assignment operator of the
  Arc :: Must Have Been Arc_Type class defined.

  No verification any prior existence of an arc Between the nodes
  Involved (this is Necessary to operate With multigraphs) is performed.

  @param [in] src_node Pointer to the source node.
  @param [in] tgt_node Pointer to the destination node.
  @param [in] arc_info Value of information to be copied to the arch.

  @return A pointer to the arc inserted

  @throw bad_alloc If no memory for the arc.
*/
      inline virtual Arc * 
  insert_arc(Node * src_node, Node * tgt_node, 
             const typename Arc::Arc_Type & arc_info);

      inline virtual Arc * 
  insert_arc(Node * src_node, Node * tgt_node, 
             typename Arc::Arc_Type && arc_info);

  inline virtual Arc * insert_arc(Node * src_node, Node * tgt_node);

/** 
  @brief Eliminates the arc called "arc".

  @details The operation removes the graph the arc \c arc and then releases its
  memory. 

  The arc must belong to the graph and do not make any
  verification regard.

  @param[in] arc Pointer to arc to delete.
*/
  inline virtual void remove_arc(Arc * arc); 

/** 
  @brief Disconnects the "arc" arc.

  @details The operation of the graph off the \c arc arc. Eventually
  the arc can be saved and then reinserted by connect_arc().

  The arc must belong to the graph and do not make any
  verification regard.

  @param[in] arc Pointer to arc to disconnects
*/
  inline virtual void disconnect_arc(Arc * arc); 

/** 
  @brief Connects an arc previously inserted and disconnected.

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
  inline virtual Arc * connect_arc(Arc * arc);

/** 
  @brief Returns the first arc of the graph.

  @details This routine should be considered as an entry point to
  graph regardless of the value of the "arc". Usually no
  it is advisable to take what the node returned by this routine.
  However, except for its equivalent
  nodes, the arcs can be sorted by any criteria
  specific comparison (weight for example).
  Consequently, in case of order, the first arc 
  return will be lower (or higher) at the discretion of order.

  @return A pointer to the first node in the graph.

  @see sort_arcs()
*/
  inline Arc * get_first_arc() const;

/** 
  @brief Order the arcs of a graph.

  @details Order the arcs of a graph according at the discretion of comparison 
  \c Compare.

  @param[inout] cmp Comparison operation.
*/
      template <class Compare> inline 
  void sort_arcs(Compare && cmp = Compare());

      template <class Compare> inline
  void sort_arcs(Compare & cmp);

  GRAPH_SEARCH_METHODS;

/** 
  @brief Builder copy of a graph.

  @details Instance a new graph copy of "g". The whole structure is
  copied with contents to both nodes and arcs.

  @param[in] g Graph to copy

  @throw bad_alloc If there is not enough memory for copy.

  @note The copy does not include the control attributes of the graph
  (bit, timer and cookies)
*/
  inline List_Graph(const List_Graph & g);

  inline List_Graph(List_Graph && g);

/** 
  @brief Assignation graph.

  @details Clean all the graph "this"
  (its nodes and arcs are eliminated and the memory is released);
  then copy the graph entirely "g" to "this".

  @param[in] g Graph to assign

  @throw bad_alloc If there is not enough memory for copy.

  @note The copy does not include the control attributes of the graph
  (bit, timer and cookies)
*/
  inline List_Graph & operator = (const List_Graph & g);

  inline List_Graph & operator = (List_Graph && g);

/** 
  @brief Completely destroys the graph
  (nodes and arcs are eliminated and the memory is freed). 
*/
  inline virtual ~List_Graph();

/** 
  @brief Iterator of nodes in a graph.

  @details This class iterates over all nodes in a graph.

  @ingroup Grafos
*/
  struct Node_Iterator : public Dlink::Iterator
  {
/** 
  @typedef Item type returning get_current() 
*/
    typedef Node * Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
    typedef List_Graph Set_Type;

    Node_Iterator() { /* Empty */ }

    inline Node_Iterator(const List_Graph & _g);

/** 
  @brief Returns the current node. 
*/
    inline Node * get_current_node(); 

/** 
  @overload get_current_node() 
*/
    Node * get_current() { return get_current_node(); }

/** 
  @overload get_current_node() 
*/
    Node * get_curr() { return get_current_node(); }
  };

/** 
  @brief Arches iterator of a node of graph.

  @details This class iterates over all the adjacent arcs of a node
  belonging to a graph or digraph.

  @ingroup Grafos
*/
  class Node_Arc_Iterator : public Dlink::Iterator
  {
    Node * src_node; 
    
  public:

/** 
  @typedef The data type that returns get_current() 
*/ 
    typedef Arc * Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
    typedef Node * Set_Type;

/** 
  @brief Instance an empty iterator (invalid) 
*/
    Node_Arc_Iterator() { /* Empty */ }

/** 
  @brief Instance an iterator on node _src_node 
*/
    Node_Arc_Iterator(Node * src) 
      : Dlink::Iterator(&(src->arc_list)), src_node(src)
    {
      /* Empty */ 
    }

/** 
  @brief Returns the current arc. 
*/
    inline Arc * get_current_arc() const;

/** 
  @overload get_current_arc() 
*/
    Arc * get_current() const { return get_current_arc(); }

/** 
  @overload get_current_arc() 
*/
    Arc * get_curr() const { return get_current_arc(); }

/** 
  @brief Returns the destination node of the current arc. 
*/
    Node * get_tgt_node() const
    {
      return (Node*) get_current_arc()->get_connected_node(src_node);
    }

/** 
  @brief Returns the source node of the current arc. 
*/
    Node * get_img_node() const
    {
      return (Node*) get_current_arc()->get_img_node(src_node);
    }

    Node * get_src_node() const { return get_src_node(); }

    Arc_Node * get_current_arc_node() const
    {
      return dlink_to_arc_node(Dlink::Iterator::get_current()); 
    }
  };

/** 
  @brief Iterator over arcs of a graph.

  @details This iterator traverses each arc of the graph. The visitation order
  it may be altered if the arches are arranged previously.

  @see sort_arcs()
*/
  struct Arc_Iterator : public Dlink::Iterator
  {
/** 
  @typedef Item type returning get_current() 
*/
    typedef Arc * Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
    typedef List_Graph Set_Type;

    Arc_Iterator() { /* Empty */ }

    inline Arc_Iterator(const List_Graph & _g);

/** 
  @brief Returns a pointer to the current arc. 
*/
    Arc * get_current_arc() const ;

/** 
  @brief Returns a pointer to the current arc. 
*/
    Arc * get_current() const { return get_current_arc(); }  

/** 
  @overload get_current() 
*/
    Arc * get_curr() const { return get_current_arc(); }  

/** 
  @brief Returns the source node of the current arc
  (only counts if digraph) 
*/
    Node * get_src_node() const { return (Node*) get_current_arc()->src_node; }

/** 
  @brief Returns the destination node of the current arc
  (only counts if digraph) 
*/
    Node * get_tgt_node() const { return (Node*) get_current_arc()->tgt_node; }
  };

  GRAPH_ITERATIVE_METHODS;
  
  List_Graph() : cookie(NULL), num_nodes(0), num_arcs(0), digraph(false)
  { 
    /* Empty */
  }

  void swap(List_Graph & g)
  {
    common_swap(g);
    node_list.swap(&g.node_list);
    arc_list.swap(&g.arc_list);
  }

  GRAPH_FUNCTIONAL_METHODS(List_Graph);
};


/** 
  @brief Filter default of iterators on arches.
  
  @ingroup Grafos
*/
    template <class GT>
struct Dft_Show_Arc
{
  bool operator () (typename GT::Arc * /* arc */) const
  { 
    return true; 
  }

  void set_cookie(void*) { /* Empty */ }
};

    template <class GT, class SA1, class SA2>
class Double_SA
{
  SA1 & sa1;
  SA2 & sa2;
  typename GT::Node * s;

public:

  Double_SA(SA1 && __sa1 = SA1(), SA2 && __sa2 = SA2()) 
    : sa1(__sa1) , sa2(__sa2) 
  {
    /* Empty */ 
  }

  Double_SA(SA1 & __sa1, SA2 & __sa2) : sa1(__sa1) , sa2(__sa2) 
  {
    /* Empty */ 
  }

  Double_SA(SA1 & __sa1, SA2 && __sa2 = SA2()) : sa1(__sa1) , sa2(__sa2) 
  {
    /* Empty */ 
  }

  Double_SA(SA1 && __sa1, SA2 & __sa2) : sa1(__sa1) , sa2(__sa2) 
  {
    /* Empty */ 
  }

  bool operator () (typename GT::Arc * a)
  {
    return sa1(a) and sa2(a);
  }
};

/** 
  @brief Selective arches iterator given a node.

  @details Node_Arc_Iterator is an iterator that loops through all the arches
  of a graph node. Its semantics are very similar to
  List_Graph::Node_Arc_Iterator, except that only show the
  arches for which the value of Show_Arc::operator()(node,arc) is
  true; where "node" is the node from which it is iterated and "arc" is the
  arc evaluates whether or not to display.
 
  @ingroup Grafos
*/
    template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
struct Node_Arc_Iterator : 
      public Filter_Iterator<typename GT::Node*, 
			     typename GT::Node_Arc_Iterator,
			     Show_Arc>
{
  using Filter_Itor = Filter_Iterator<typename GT::Node*, 
				      typename GT::Node_Arc_Iterator,
				      Show_Arc>;

  using Itor = Filter_Iterator<typename GT::Node*, 
			       typename GT::Node_Arc_Iterator, 
			       Show_Arc>;

/** 
  @typedef Item type returning get_current(). 
*/
  typedef typename Itor::Item_Type Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
  typedef typename Itor::Set_Type Set_Type;

  Node_Arc_Iterator() { /* Empty */ }

/** 
  @brief Iterator builder arches on a node.

  @param[in] p Node on which to iterate.
  @param[in] sa Filter of arches.
*/
  Node_Arc_Iterator(typename GT::Node * p, Show_Arc && sa = Show_Arc()) 
    : Itor(p, sa) 
  {
    /* Empty */
  }

/** 
  @brief Iterator builder arches on a node.

  @param[in] p Node on which to iterate.
  @param[in] sa Filter of arches.
*/
  Node_Arc_Iterator(typename GT::Node * p, Show_Arc & sa) : Itor(p, sa) 
  {
    /* Empty */
  }
};


/**
  @brief Selective iterator arches on a graph.

  @details Arc_Iterator is an iterator that loops through all the arches
  of graph. Its semantics are very similar to
  List_Graph::Arc_Iterator, except that only show the
  arches for which the value of Show_Arc::operator()(g, arc) is
  true; where "g" is the graph and "arc" is an arc that is judged whether
  or not show.
 
  @ingroup Grafos
*/
    template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
struct Arc_Iterator : 
      public Filter_Iterator<GT, typename GT::Arc_Iterator, Show_Arc>
{
  typedef Filter_Iterator<GT, typename GT::Arc_Iterator, Show_Arc> Itor;

/** 
  @typedef Item type returning get_current() 
*/
  typedef typename Itor::Item_Type Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
  typedef typename Itor::Set_Type Set_Type;

  Arc_Iterator() { /* Empty */ }

/** 
  @brief Iterator builder filter arches on a graph.

  @param[in] g Graph.
  @param[in] sa Filter on arches (with sa of rvalue).
*/
  Arc_Iterator(const GT & g, Show_Arc & sa) : Itor(g, sa) 
  {
    /* Empty */
  }

/** 
  @brief Iterator builder filter arches on a graph.

  @param[in] g Graph.
  @param[in] sa Filter on arches (with sa of rvalue).
*/
  Arc_Iterator(const GT & g, Show_Arc && sa = Show_Arc()) : Arc_Iterator(g, sa)
  {
    /* Empty */
  }
};


/** 
  @brief Selective iterator arches on a graph.

  @details Dual_Arc_Iterator is an iterator that loops through all the arches
  of graph. Its semantics are very similar to
  List_Graph::Dual_Arc_Iterator, except that only show the
  arches for which the value of Show_Arc::operator()(g, arc) is
  true; where "g" is the graph and "arc" is an arc that is judged whether
  or not show.

  @ingroup Grafos
*/
    template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
struct Dual_Arc_Iterator : 
      public Filter_Iterator<GT, typename GT::Dual_Arc_Iterator, Show_Arc>
{
  typedef Filter_Iterator<GT, typename GT::Dual_Arc_Iterator, Show_Arc> Itor;

/** 
  @typedef Item type returning get_current() 
*/
  typedef typename Itor::Item_Type Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
  typedef typename Itor::Set_Type Set_Type;

  Show_Arc * sa_ptr = NULL;

  Dual_Arc_Iterator() { /* Empty */ }

/** 
  @brief Iterator builder filter arches on a graph.

  @param[in] g Graph.
  @param[in] sa Filter on arches (with sa of rvalue).
*/
  Dual_Arc_Iterator(const GT & g, Show_Arc & sa) : Itor(g, sa), sa_ptr(&sa) 
  {
    sa.set_cookie(&this->first_tour);
  }

/** 
  @brief Iterator builder filter arches on a graph.

  @param[in] g Graph.
  @param[in] sa Filter on arches (with sa of rvalue).
*/
  Dual_Arc_Iterator(const GT & g, Show_Arc && sa = Show_Arc()) 
    : Dual_Arc_Iterator(g, sa)
  {
    /* Empty */
  }

  ~Dual_Arc_Iterator()
  {
    sa_ptr->set_cookie(NULL);
  }
};


/** 
  @brief Filter default of iterators on nodes.
  
  @ingroup Grafos
*/
     template <class GT>
struct Dft_Show_Node
{
  bool operator () (typename GT::Node *) const 
  { 
    return true; 
  }
};

/** 
  @brief Selective arches iterator given a node.

  @details Node_Arc_Iterator is an iterator that loops through all the arches
  of a graph node. Its semantics are very similar to
  List_Graph::Node_Arc_Iterator, except that only show the
  arches for which the value of Show_Arc::operator()(node,arc) is
  true; where "node" is the node from which it is iterated and "arc" is the
  arc evaluates whether or not to display.

  @ingroup Grafos
*/
    template <class GT, class Show_Node = Dft_Show_Node<GT>>
class Node_Iterator : 
  public Filter_Iterator<GT, typename GT::Node_Iterator, Show_Node>
{
public:

  typedef Filter_Iterator<GT, typename GT::Node_Iterator, Show_Node> Itor;

/** 
  @typedef Item type returning get_current() 
*/
  typedef typename Itor::Item_Type Item_Type;

/** 
  @typedef The set type on which iterates. 
*/
  typedef typename Itor::Set_Type Set_Type;

  Node_Iterator() { /* Empty */ }

/** 
  @brief Iterator builder over nodes of a graph.

  @param[in] g Graph.
  @param[in] sn Filter of nodes.
*/
  Node_Iterator(const GT & g, Show_Node && sn = Show_Node()) : Itor (g, sn)
  {
    /* Empty */ 
  }

/** 
  @brief Iterator builder over nodes of a graph.

  @param[in] g Graph.
  @param[in] sn Filter of nodes.
*/
  Node_Iterator(const GT & g, Show_Node & sn) 
    : Itor (g, sn) 
  {
    /* Empty */ 
  }
};

template <class GT, class SN = Dft_Show_Node<GT>>
void for_each_node(const GT & g, 
		   std::function<void(typename GT::Node*)> operation,
		   SN & sn)
{
  for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
    operation(it.get_curr());
}

template <class GT, class SN = Dft_Show_Node<GT>>
void for_each_node(const GT &                              g, 
		   std::function<void(typename GT::Node*)> operation,
		   SN &&                                   sn = SN())
{
  for_each_node<GT, SN>(g, operation, sn);
}

template <class GT, class SA = Dft_Show_Arc<GT>>
void for_each_arc(const GT &                             g, 
		  std::function<void(typename GT::Arc*)> operation,
		  SA &                                   sa)
{
  for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
    operation(it.get_curr());
}

template <class GT, class SA = Dft_Show_Arc<GT>>
void for_each_arc(const GT &                             g, 
		  std::function<void(typename GT::Arc*)> operation,
		  SA &&                                  sa = SA())
{
  for_each_arc<GT, SA>(g, operation, sa);
}

template <class GT, class SA = Dft_Show_Arc<GT>>
void for_each_arc(const GT&, 
		  typename GT::Node *                    p, 
		  std::function<void(typename GT::Arc*)> operation,
		  SA &                                   sa)
{
  for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
    operation(it.get_curr());
}

template <class GT, class SA = Dft_Show_Arc<GT>>
void for_each_arc(const GT &                             g,
		  typename GT::Node *                    p, 
		  std::function<void(typename GT::Arc*)> operation,
		  SA &&                                  sa = SA())
{
  for_each_arc<GT, SA>(g, p, operation, sa);
}

template <class GT, class SN = Dft_Show_Node<GT>>
bool forall_node(const GT &                               g, 
		 std::function<bool(typename GT::Node*)> operation,
		 SN &                                    sn)
{
  for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
    if (not operation(it.get_curr()))
      return false;
  return true;
}

template <class GT, class SN = Dft_Show_Node<GT>>
bool forall_node(const GT &                              g, 
		 std::function<bool(typename GT::Node*)> operation,
		 SN &&                                   sn = SN())
{
  return forall_node<GT, SN>(g, operation, sn);
}

template <class GT, class SA = Dft_Show_Arc<GT>>
bool forall_arc(const GT &                             g, 
		std::function<bool(typename GT::Arc*)> operation,
		SA &                                   sa)
{
  for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
    if (not operation(it.get_curr()))
      return false;
  return true;
}

template <class GT, class SA = Dft_Show_Arc<GT>>
bool forall_arc(const GT &                             g, 
		std::function<bool(typename GT::Arc*)> operation,
		SA &&                                  sa = SA())
{
  return forall_arc<GT, SA>(g, operation, sa);
}

template <class GT, class SA = Dft_Show_Arc<GT>>
bool forall_arc(typename GT::Node *                    p, 
		std::function<bool(typename GT::Arc*)> operation,
		SA &                                   sa)
{
  for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next())
    if (not operation(it.get_curr()))
      return false;
  return true;
}

template <class GT, class SA = Dft_Show_Arc<GT>>
bool forall_arc(typename GT::Node * p, 
		std::function<bool(typename GT::Arc*)> operation,
		SA &&                                  sa = SA())
{
  return forall_arc<GT, SA>(p, operation, sa);
}

template <class GT, 
	  class SN                            = Dft_Show_Node<GT>,
	  template <typename> class Container = DynDlist,
	  typename T                          = typename GT::Node_Type>
Container<T> map_nodes(GT &                                  g, 
		       std::function<T(typename GT::Node *)> operation,
		       SN &                                  sn)
{
  Container<T> ret_val;
  for_each_node<GT, SN>(g, [&ret_val, &operation] (typename GT::Node * p)
			{
			  ret_val.append(operation(p));
			}, sn);
  return ret_val;
}

template <class GT, 
	  typename T                          = typename GT::Node_Type,
	  template <typename> class Container = DynDlist,
	  class SN                            = Dft_Show_Node<GT>>
Container<T> map_nodes(GT &                                  g, 
		       std::function<T(typename GT::Node *)> operation,
		       SN &&                                 sn = SN())
{
  return map_nodes<GT, SN, Container, T>(g, operation, sn);
}

template <class GT, 
	  class SA                            = Dft_Show_Arc<GT>,
	  template <typename> class Container = DynDlist,
	  typename T                          = typename GT::Arc_Type>
Container<T> map_arcs(GT &                                 g, 
		      std::function<T(typename GT::Arc *)> operation,
		      SA &                                 sa)
{
  Container<T> ret_val;
  for_each_arc<GT, SA>(g, [&ret_val, &operation] (typename GT::Arc * p)
		       {
			 ret_val.append(operation(p));
		       }, sa);
  return ret_val;
}

template <class GT, 
	  typename T                          = typename GT::Arc_Type,
	  template <typename> class Container = DynDlist,
	  class SA                            = Dft_Show_Arc<GT>>
Container<T> map_arcs(GT &                                 g, 
		      std::function<T(typename GT::Arc *)> operation,
		      SA &&                                sa = SA())
{
  return map_arcs<GT, SA, Container, T>(g, operation, sa);
}

template <class GT, 
	  class SA                            = Dft_Show_Arc<GT>,
	  template <typename> class Container = DynDlist,
	  typename T                          = typename GT::Arc_Type>
Container<T> map_node_arcs(GT &                                 g, 
			   typename GT::Node *                  p,
			   std::function<T(typename GT::Arc *)> operation,
			   SA &                                 sa)
{
  Container<T> ret_val;
  for_each_arc<GT, SA>(g, p, [&ret_val, &operation] (typename GT::Arc * p)
		       {
			 ret_val.append(operation(p));
		       }, sa);
  return ret_val;
}

template <class GT, 
	  typename T                          = typename GT::Arc_Type,
	  template <typename> class Container = DynDlist,
	  class SA                            = Dft_Show_Arc<GT>>
Container<T> map_node_arcs(GT &                                 g, 
			   typename GT::Node *                  p,
			   std::function<T(typename GT::Arc *)> operation,
			   SA &&                                sa = SA())
{
  return map_node_arcs<GT, SA, Container, T>(g, p, operation, sa);
}

template <class GT, typename T, class SN = Dft_Show_Node<GT>>
T foldl_nodes(GT & g, const T & init, 
	      std::function<T(const T&, typename GT::Node*)> operation,
	      SN & sn)
{
  T ret_val = init;
  for_each_node<GT, SN>(g, [&ret_val, &operation] (typename GT::Node * p)
    {
      ret_val = operation(ret_val, p);
    }, sn);
  return ret_val;
}

template <class GT, typename T, class SN = Dft_Show_Node<GT>>
T foldl_nodes(GT & g, const T & init, 
	      std::function<T(const T&, typename GT::Node*)> operation,
	      SN && sn = SN())
{
  return foldl_nodes<GT, T, SN>(g, init, operation, sn);
}

template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
T foldl_arcs(GT & g, const T & init,
	     std::function<T(const T&, typename GT::Arc*)> operation,
	     SA & sa)
{
  T ret_val = init;
  for_each_arc<GT, SA>(g, [&ret_val, &operation] (typename GT::Arc* a)
    {
      ret_val = operation(ret_val, a);
    }, sa);
  return ret_val;
}

template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
T foldl_arcs(GT & g, const T & init,
	     std::function<T(const T&, typename GT::Arc*)> operation,
	     SA && sa = SA())
{
  return foldl_arcs<GT, T, SA>(g, init, operation, sa);
}

template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
T foldl_arcs(GT & g, typename GT::Node * p,
	     const T & init,
	     std::function<T(const T&, typename GT::Arc*)> operation,
	     SA & sa)
{
  T ret_val = init;
  for_each_arc<GT, SA>(g, p, [&ret_val, &operation] (typename GT::Arc* a)
    {
      ret_val = operation(ret_val, a);
    }, sa);
  return ret_val;
}

template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
T foldl_arcs(GT & g, typename GT::Node * p,
	     const T & init,
	     std::function<T(const T&, typename GT::Arc*)> operation,
	     SA && sa = SA())
{
  return foldl_arcs<GT, T, SA>(g, p, init, operation, sa);
}			 

/** 
  @brief Digraph class (directed graph) implemented adjacency lists.

  @details This class models a directed graph. It is functionally
  equivalent to the class \c List_Graph, with the exception of it
  handles directed graphs.
 
  @param __Graph_Node Node type. You must be defined
  it from the class \c __Graph_Node, either by inclusion of
  attributes by derivation or combination of both.
  @param __Graph_Arc Arc type. You must be defined
  it from the class \c __Graph_Arc, either by inclusion of
  attributes by derivation or combination of both.
  
  @see Graph_Node Graph_Arc
  
  @ingroup Grafos
  
  @see List_Graph
*/
template <typename __Graph_Node = Graph_Node<int>,
	  typename __Graph_Arc  = Graph_Arc<int>>
class List_Digraph : public List_Graph<__Graph_Node, __Graph_Arc>
{
  typedef List_Graph<__Graph_Node, __Graph_Arc> GT;

public:

  typedef __Graph_Node Node;

  typedef __Graph_Arc Arc;

  List_Digraph() 
  {
    this->digraph = true; 
  }
  
  List_Digraph(const List_Digraph & dg) : GT()
  {
    this->digraph = true; 
    *this = dg;
  }

  List_Digraph(List_Digraph && dg) : GT()
  {
    this->digraph = true; 
    this->swap(dg);
  }

  List_Digraph & operator = (const List_Digraph & g)
  {
    if (this == &g) 
      return *this;

    copy_graph(*this, (const List_Digraph &) g);

    return *this;
  }

  List_Digraph & operator = (List_Digraph && g)
  {
    this->swap(g);
    return *this;
  }  
};


template <class GT>
using ArcPair = tuple<typename GT::Arc*, typename GT::Node*>;


template <class GT> struct __Out_Filt
{
  typename GT::Node * src = NULL;

  __Out_Filt(typename GT::Node * __src) : src(__src) { /* Empty */ }

  bool operator () (typename GT::Arc * a) const
  {
    assert(src);
    return a->src_node == src;
  }

  typename GT::Node * get_node(typename GT::Arc * a) const
  {
    assert(src);
    return (typename GT::Node *) a->tgt_node;
  }
};


template <class GT> struct __In_Filt
{
  typename GT::Node * tgt = NULL;

  __In_Filt(typename GT::Node * __tgt = NULL) : tgt(__tgt) { /* Empty */ }

  bool operator () (typename GT::Arc * a) const
  {
    assert(tgt);
    return a->tgt_node == tgt;
  }

  typename GT::Node * get_node(typename GT::Arc * a) const
  {
    assert(tgt);
    return (typename GT::Node *) a->src_node;
  }
};

template <class GT, class Filter>
struct Digraph_Iterator
{
  using Itor = 
    Filter_Iterator<typename GT::Node*, typename GT::Node_Arc_Iterator, Filter>;

  Filter filt;
  Itor it;

  using Item_Type = typename Itor::Item_Type;

  using Iterator_Type = Itor;

  Digraph_Iterator(typename GT::Node * p) : filt(p), it(p, filt) {}

  void next() { it.next(); }

  void prev() { it.next(); }

  bool has_curr() const { return it.has_curr(); }

  typename GT::Arc * get_curr() const { return it.get_curr(); }
				 
  typename GT::Node * get_node(typename GT::Arc * a) const
  {
    return filt.get_node(a);
  }

  typename GT::Node * get_node() 
  { 
    return this->get_node(this->get_curr()); 
  }

  void reset_first() { it.reset_first(); }

  void reset_last() { it.reset_last(); }
};


template <class GT>
using _Out_Iterator = Digraph_Iterator<GT, __Out_Filt<GT>>;


template <class GT>
using _In_Iterator = Digraph_Iterator<GT, __In_Filt<GT>>;


template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
using Out_Iterator = 
  Filter_Iterator<typename GT::Node*, _Out_Iterator<GT>, Show_Arc>;


template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
using In_Iterator = 
  Filter_Iterator<typename GT::Node*, _In_Iterator<GT>, Show_Arc>;


/** 
  @brief Returns the nodes connected to "p" by output arches of a 
  digraph represented by a graph.

  @ingroup Grafos
*/
template <class GT>
DynList<typename GT::Node*> __out_nodes(typename GT::Node * p)
{
  DynList<typename GT::Node *> ret;
  for (_Out_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_node());
  return ret;
}


/** 
  @brief Returns the nodes connected to "p" by output arches of a 
  digraph represented by a graph.

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Node*> out_nodes(typename GT::Node * p, Show_Arc & sa)
{
  DynList<typename GT::Node *> ret;
  for (Out_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_node());
  return ret;
}


template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Node*> out_nodes(typename GT::Node * p,
				      Show_Arc && sa = Show_Arc())
{
  return out_nodes<GT, Show_Arc>(p, sa);
}


/** 
  @brief Returns nodes that connect to "p" by input arches.

  @ingroup Grafos
*/
template <class GT>
DynList<typename GT::Node*> __in_nodes(typename GT::Node * p)
{
  DynList<typename GT::Node *> ret;
  for (_In_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_node());
  return ret;
}

/** 
  @brief Returns nodes that connect to "p" by input arches.

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Node*> in_nodes(typename GT::Node * p, Show_Arc & sa)
{
  DynList<typename GT::Node *> ret;
  for (In_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_node());
  return ret;
}

template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Node*> in_nodes(typename GT::Node * p, 
				     Show_Arc && sa = Show_Arc())
{
  return in_nodes<GT, Show_Arc>(p, sa);
}


/** 
  @brief Returns the arcs of p output

  @ingroup Grafos
*/
template <class GT>
DynList<typename GT::Arc*> __out_arcs(typename GT::Node * p)
{
  DynList<typename GT::Arc *> ret;
  for (_Out_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}


/** 
  @brief Returns the arcs of p output

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Arc*> out_arcs(typename GT::Node * p, Show_Arc & sa)
{
  DynList<typename GT::Arc *> ret;
  for (Out_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Arc*> out_arcs(typename GT::Node * p, 
				    Show_Arc && sa = Show_Arc())
{
  return out_arcs<GT, Show_Arc>(p, sa);
}


/** 
  @brief Returns the arcs of p input

  @ingroup Grafos
*/
template <class GT>
DynList<typename GT::Arc*> __in_arcs(typename GT::Node * p)
{
  DynList<typename GT::Arc *> ret;
  for (_In_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

/** 
  @brief Returns the arcs of p input

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Arc*> in_arcs(typename GT::Node * p, Show_Arc & sa)
{
  DynList<typename GT::Arc *> ret;
  for (In_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<typename GT::Arc*> in_arcs(typename GT::Node * p, 
				   Show_Arc && sa = Show_Arc())
{
  return in_arcs<GT, Show_Arc>(p, sa);
}


/** 
  @brief Returns all arches related to p are input or output

  @ingroup Grafos
*/
template <class GT>
DynList<typename GT::Arc*> arcs(typename GT::Node * p)
{
  DynList<typename GT::Arc *> ret;
  for (Node_Arc_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}


/** 
  @brief Returns pairs (arc, node) of p input

  @ingroup Grafos
*/
template <class GT>
DynList<ArcPair<GT>> __in_pairs(typename GT::Node * p)
{
  DynList<ArcPair<GT>> ret;
  for (_In_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

/** 
  @brief Returns pairs (arc, node) of p input

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<ArcPair<GT>> in_pairs(typename GT::Node * p, Show_Arc & sa)
{
  DynList<ArcPair<GT>> ret;
  for (In_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}

template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<ArcPair<GT>> in_pairs(typename GT::Node * p, 
			      Show_Arc && sa = Show_Arc())
{
  return in_pairs<GT, Show_Arc>(p, sa);
}


/** 
  @brief Returns pairs (arc, node) of p output

  @ingroup Grafos
*/
template <class GT>
DynList<ArcPair<GT>> __out_pairs(typename GT::Node * p)
{
  DynList<ArcPair<GT>> ret;
  for (_Out_Iterator<GT> it(p); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}


/** 
  @brief Returns pairs (arc, node) of p output

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<ArcPair<GT>> out_pairs(typename GT::Node * p, Show_Arc & sa)
{
  DynList<ArcPair<GT>> ret;
  for (Out_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ret.append(it.get_curr());
  return ret;
}


template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
DynList<ArcPair<GT>> out_pairs(typename GT::Node * p, 
			       Show_Arc && sa = Show_Arc())
{
  return out_pairs<GT, Show_Arc>(p, sa);
}


/**
  @brief Returns the output level of p

  @ingroup Grafos
*/
template <class GT>
size_t __in_degree(typename GT::Node * p)
{
  size_t count = 0;
  for (_In_Iterator<GT> it(p); it.has_curr(); it.next())
    ++count;
  return count;
}


/**
  @brief Returns the output level of p

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
size_t in_degree(typename GT::Node * p, Show_Arc & sa)
{
  size_t count = 0;
  for (In_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ++count;
  return count;
}

template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
size_t in_degree(typename GT::Node * p, Show_Arc && sa = Show_Arc())
{
  return in_degree<GT, Show_Arc>(p, sa);
}



/**
  @brief Returns the input level of p

  @ingroup Grafos
*/
template <class GT>
size_t __out_degree(typename GT::Node * p)
{
  size_t count = 0;
  for (_Out_Iterator<GT> it(p); it.has_curr(); it.next())
    ++count;
  return count;
}

/**
  @brief Returns the input level of p

  @ingroup Grafos
*/
template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
size_t out_degree(typename GT::Node * p, Show_Arc & sa)
{
  size_t count = 0;
  for (Out_Iterator<GT, Show_Arc> it(p, sa); it.has_curr(); it.next())
    ++count;
  return count;
}


template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
size_t out_degree(typename GT::Node * p, Show_Arc && sa = Show_Arc())
{
  return out_degree<GT, Show_Arc>(p, sa);
}



/** 
  @brief Search arc connecting two nodes.

  @details This generic function receives two nodes and returns the first arc
  that if indeed connect the nodes are connected; than
  otherwise, the routine returns NULL.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  If it is a digraph (directed graph), then the routine
  considers the direction of the arc; ie, there is an arc
  directed from src_node to tgt_node.

  Search only arcs refers nodes involved. 
  Therefore, this type of search is much faster
  that finding based on other criteria arc.

  If the graph is not addressed, then the routine selects the
  node less to look between their arcs.

  The routine uses the Node_Arc_Iterator iterator to traverse
  the arcs of one node.

  Eventually, the Node_Arc_Iterator iterator may be overloaded 
  to rule, depending on the nature of the problem, not arches
  you want to display. Typical usage of this technique is to
  expanded representations that add arcs that are not part
  graph model; for example, a residual graph for the maximum 
  flow problem.

  @param[in] g Graph or digraph where the search is performed.
  @param[in] src_node Arc origin node.
  @param[in] src_node Arc destination node.

  @return A pointer to the first arc found; \c NULL otherwise.

  @ingroup Grafos
*/
    template <class GT, class SA> typename GT::Arc * 
search_arc(const GT & g, typename GT::Node * src_node, 
           typename GT::Node * tgt_node);

/** 
  @brief Search exact arc connecting two nodes in a digraph.
        
  @details This generic function receives two nodes and returns the first arc
  that if indeed connect the nodes are connected; than
  otherwise, the routine returns NULL.

  By accurate it is meant that the arc was inserted in order
  "src-tgt". This is relevant in the case of graphs that are used as
  digraphs. Recall that in an undirected graph "src-tgt" is what
  same as "tgt-src". However, sometimes undirected graphs
  they are used as digraphs, in which case it is very important to distinguish
  if the arc is "src-tgt" or "tgt-src"

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  A specialization dispenses the parameter SA type and shows
  all the arcs of the graph without filtering.

  If it is a digraph (directed graph), then the routine
  considers the direction of the arc; ie, there is an arc
  directed from src_node to tgt_node.

  Search only arcs refers nodes involved. 
  Therefore, this type of search is much faster
  that finding based on other criteria arc.

  If the graph is not addressed, then the routine selects the
  node less to look between their arcs.

  The routine uses the Node_Arc_Iterator iterator to traverse
  the arcs of one node.

  Eventually, the Node_Arc_Iterator iterator may be overloaded 
  to rule, depending on the nature of the problem, not arches
  you want to display. Typical usage of this technique is to
  expanded representations that add arcs that are not part
  graph model; for example, a residual graph for the maximum 
  flow problem.

  @param[in] g Graph or digraph where the search is performed.
  @param[in] src_node Arc origin node.
  @param[in] src_node Arc destination node.

  @return A pointer to the first arc found; \c NULL otherwise.

  @ingroup Grafos
*/
template <class GT, class SA = Dft_Show_Arc<GT>> typename GT::Arc * 
search_directed_arc(const GT & g, 
		    typename GT::Node * src, typename GT::Node * tgt)
{
  assert(src != NULL and tgt != NULL);

  for (Node_Arc_Iterator<GT, SA> it(src); it.has_curr(); it.next())
    {
      typename GT::Arc * a = it.get_curr();
      if (a->src_node == src and a->tgt_node == tgt)
	return a;
    }

  return NULL; 
}

/** 
  @brief Returns the mapping node through of p cookie 
*/
    template <class GT> 
typename GT::Node * mapped_node(typename GT::Node * p)
{
 return (typename GT::Node *) NODE_COOKIE(p);
}

/** 
  @brief Returns the mapping arc through of p cookie 
*/
    template <class GT> 
typename GT::Arc * mapped_arc(typename GT::Arc * a)
{
  return (typename GT::Arc *) ARC_COOKIE(a);
}

/** 
  @brief Returns the mapping node through of p cookie with two different types 
*/
    template <class GTSRC, class GTTGT>
typename GTTGT::Node * mapped_node(typename GTSRC::Node * p)
{
 return (typename GTTGT::Node *) NODE_COOKIE(p);
}
/** 
  @brief Returns the mapping node through of p cookie with two different types 
*/
    template <class GTSRC, class GTTGT>
typename GTTGT::Arc * mapped_arc(typename GTSRC::Arc * a)
{
  return (typename GTTGT::Arc *) ARC_COOKIE(a);
}

/** 
  @brief Explicit copy of graphs.

  @details The first routine clean "this" graph (its nodes are removed
  arches and all the memory is freed); then copy entirely the
  graph (or digraph) "g" to "this". If the logical value cookie_map is
  true, then the copy is mapped; ie all cookies of nodes and arcs 
  of both graphs are mapped each.

  @param[in] gsrc Graph or digraph to be copied.
  @param[in] gtgt Graph or digraph destination of the copy.
  @param[in] cookie_map If the value is true, then the copy is
  mapped. Not mapped by default copying.
  
  @throw bad_alloc If there is insufficient memory to the copy.
  
  @note The copy does not include the control attributes of the graph
  (bit, timer and cookies)
  
  @ingroup Grafos
*/
    template <class GT>
void copy_graph(GT & gtgt, const GT & gsrc, const bool cookie_map = false);

/** 
  @brief Clean entire the g graph (all nodes and arcs are eliminated and
  memory is released).

  @ingroup Grafos
*/
template <class GT> inline void clear_graph(GT & g);

/** 
  @brief Actuator on all the nodes in a graph.

  @details This sort through each node in the graph and on the run
  \c Operation() operation (\c this, \c nodo_actual).

  The class has 3 parameters type:
  -# GT: Type of graph.
  -# Operation: Type of operation to be executed on each node of the graph.
  -# SN: Filter of nodes that internally uses the iterator.
  .

  @ingroup Grafos
*/
    template <class GT, class Operation, class SN = Dft_Show_Node<GT>> 
class Operate_On_Nodes
{
  SN & sn;

public:

  Operate_On_Nodes(SN & __sn) : sn(__sn) { /* Empty */ }

  Operate_On_Nodes(SN && __sn = SN()) : sn(__sn) { /* Empty */ }

/** 
  @brief Invoking the operation on all nodes.

  @param[in] g Graph on which you want to perform the 
  operation of its nodes.
  @param[in] op Operation that you want to perform.
*/
  void operator () (const GT & g, Operation op = Operation()) const
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      op (g, it.get_curr());
  }

/** 
  @brief Invoking the operation on all nodes with additional opaque parameter.

  @param[in] g Graph on which you want to perform the 
  operation of its nodes.
  @param[in] ptr Opaque pointer which can be passed or
  receive any other information to the operation.
  @param[in] op Operation that you want to perform.
*/ 
  void operator () (const GT & g, void * ptr, Operation op = Operation()) const
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      op (g, it.get_curr(), ptr);
 }

  void operator () (GT & g, Operation op = Operation())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      op (g, it.get_curr());
  }

  void operator () (GT & g, void * ptr, Operation op = Operation())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      op (g, it.get_curr(), ptr);
  }
};

/** 
  @brief Actuator on all arches of a graph.

  @details This class runs each arc of the graph and on the run
  \c Operation() operation (\c this, \c arco_actual).

  The class has 3 parameters type:
  -# GT: Type of graph.
  -# Operation: Type of operation to be executed on each node of the graph.
  -# SA: Arches filter than uses internally the iterator.
  .

  @ingroup Grafos
*/
    template <class GT, class Operation, 
              class SA = Dft_Show_Arc<GT>> 
class Operate_On_Arcs
{
  SA & sa;

public:

  Operate_On_Arcs(SA & __sa) : sa(__sa) { /* Empty */ }

  Operate_On_Arcs(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

/** 
  @brief Invokes the operation on all the arcs

  @param[in] g Graph on which the operation is invoked on their arcs.
  @param[in] op Operation that you want to perform.
*/
  void operator () (const GT & g, Operation op = Operation()) const
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
      op (g, it.get_curr());
  }

/** 
  @brief Invokes the operation on all the arcs with additional parameter.

  @param[in] g Graph on which the operation is invoked on their arcs.
  @param[in] ptr Opaque pointer which can pass any other information 
  on the operation.
  @param[in] op Operation that you want to perform.
*/
  void operator () (const GT & g, void * ptr, Operation op = Operation()) const
  {
    for (Arc_Iterator<GT, SA> itor(g, sa); itor.has_curr(); itor.next())
      op (g, itor.get_curr(), ptr);
  }

/** 
  @brief Invokes the operation on all the arcs of a node.

  @param[in] g Graph. 
  @param[in] p Node on which their arcs will travel.
  @param[in] op Operation that you want to perform.
*/
  void operator () (const GT & g, typename GT::Node * p, 
                    Operation op = Operation()) const
  {
    for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next())
      op (g, it.get_current_arc());
  }

/** 
  @brief Invokes the operation on all the arcs of a node having a
  additional parameter by an opaque pointer.

  @param[in] g Graph. 
  @param node Node on which their arcs will travel.
  @param ptr Opaque pointer which can pass any other information 
  on the operation.
  @param[in] op Operation that you want to perform.
*/
  void operator () (const GT & g, typename GT::Node * node, 
                    void * ptr, Operation op = Operation()) const
  {
    for (Node_Arc_Iterator<GT, SA> it(node); it.has_current(); it.next())
      op (g, it.get_current_arc(), ptr);
  }

  void operator () (GT & g, Operation op = Operation()) const
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
      op (g, it.get_curr());
  }

/** 
  @brief Invokes the operation on all the arcs with additional parameter.

  @param[in] g Graph on which the operation is invoked on their arcs.
  @param[in] ptr Opaque pointer which can pass any other information 
  on the operation.
  @param[in] op Operation that you want to perform.
*/
  void operator () (GT & g, void * ptr, Operation op = Operation()) const
  {
    for (Arc_Iterator<GT, SA> itor(g, sa); itor.has_curr(); itor.next())
      op (g, itor.get_curr(), ptr);
  }

/** 
  @brief Invokes the operation on all the arcs of a node.

  @param[in] g Graph. 
  @param[in] p Node on which their arcs will travel.
  @param[in] op Operation that you want to perform.
*/
  void operator () (GT & g, typename GT::Node * p, 
                    Operation op = Operation()) const
  {
    for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next())
      op (g, it.get_current_arc());
  }

/** 
  @brief Invokes the operation on all the arcs of a node having a
  additional parameter by an opaque pointer.

  @param[in] g Graph. 
  @param node Node on which their arcs will travel.
  @param ptr Opaque pointer which can pass any other information 
  on the operation.
  @param[in] op Operation that you want to perform.
*/
  void operator () (GT & g, typename GT::Node * node, 
                    void * ptr, Operation op = Operation()) const
  {
    for (Node_Arc_Iterator<GT, SA> it(node); it.has_current(); it.next())
      op (g, it.get_current_arc(), ptr);
  }
};

/** 
  @brief Path on a graph.

  @details Many problems on graphs require the dynamic construction
  of road. The Path<GT> class models a path on a graph of any tipo. 
  The class is designed to build roads sequentially, while visiting 
  nodes and arcs in the during execution of an algorithm.

  The roads are built for their tips, either by their first node or 
  its last node. Eventually, you may be inserted road in a road.

  @param GT Type of graph.

  @ingroup Grafos
  
  @see Path::Iterator List_Graph
*/
    template <class GT> 
class Path
{
public:

/** 
  @typedef The attribute type containing the nodes of the road. 
*/
    typedef typename GT::Node_Type Node_Type;

/** 
  @typedef The attribute type containing the arches of the road. 
*/
    typedef typename GT::Arc_Type Arc_Type;

private:

  const GT * g;
  void * cookie;

  typedef typename GT::Node Node;
  typedef typename GT::Arc  Arc;

  struct Path_Desc
  {
    Node * node; /* Origin node */
    Arc *  arc;  /* Adjacent arc */

    Path_Desc(Node * _node = NULL, Arc * _arc = NULL) 
      : node(_node), arc(_arc) {}

    bool operator == (const Path_Desc & r) const 
    { 
      if (not (node->get_info() == r.node->get_info()))
	return false;

      if (arc == NULL)
	return r.arc == NULL;

      if (r.arc == NULL)
	return false;

      return arc->get_info() == r.arc->get_info(); 
    }
  };

  DynDlist<Path_Desc> list;

public:

/** 
  @brief Gets a reference to the graph on which is the road. 
*/
  const GT & get_graph() const { return *g; }

  void *& get_cookie() { return cookie; }

/** 
  @brief Returns true if the path "this" refers to the graph "gr" 
*/
  bool inside_graph(const GT & gr) const { return g == &gr; }

/** 
  @brief Basic constructor.

  @param[in] _g grafo On which is built the road.
  @param[in] __cookie Cookie to associate the road.
*/
  Path(const GT & _g, void * __cookie = NULL) : g(&_g), cookie(__cookie) {}

/** @brief Empty constructor */
  Path() : g(NULL), cookie(NULL) { /* Empty */ }

/** 
  @brief Initializes a path that begins in start_node.

  @param start_node Pointer to node by where the road begins.

  @throw bad_alloc If no memory.

  @note It does not check whether if start_node belongs to the graph.
*/
  void init(Node * start_node) { list.append(Path_Desc(start_node)); }

/** 
  @brief Road constructor on graph from an initial node.

  @param[in] _g Graph which will be held on the way.
  @param[in] start_node Pointer to node by where the road begins.
  @param[in] __cookie Cookie to associate the road.

  @throw bad_alloc If no memory.

  @note It does not check whether if start_node belongs to the graph.
*/
  Path(const GT & _g, Node * start_node, void * __cookie = NULL) 
    : g(&_g), cookie(__cookie) { init(start_node); }

/** 
  @brief Clears the path "this" and set a new graph.

  @details The method clean the path "this" (all nodes are removed and
  arches) and resets the path it is built on the __g graph 
  with start_node initial node.

  @param[in] __g Graph which will be held on the road.
  @param[in] start_node Pointer to node by where the road begins.

  @throw bad_alloc If no memory.

  @note It does not check whether if start_node belongs to the graph.
*/
  void set_graph(const GT & __g, Node * start_node = NULL)
  {
    clear_path();
    g = &__g;
    if (start_node == NULL)
      return;
    init(start_node);
  }

/** 
  @brief Returns the length of road in nodes. 
*/
  const size_t & size() const { return list.size(); }

/** 
  @brief Returns true if the path is empty. 
*/
  bool is_empty() const { return list.is_empty(); }

/** 
  @brief Clears the way (all nodes and arcs are removed). 
*/
  void clear_path()
  {
    while (not list.is_empty())
      list.remove_first();
  }

/** 
  @overload clear_path 
*/
  void clear() { clear_path(); }

  void empty() { clear_path(); }

/** 
  @brief Copy constructor way; all nodes and arcs are copied. 
*/
  Path(const Path & path) : g(path.g), list(path.list) {}

/** 
  @brief Assigning way; clean "this" and copy all nodes and arcs "path" 
*/
  Path & operator = (const Path & path)
  {
    if (this == &path)
      return *this;

    clear_path();
    g    = path.g;
    list = path.list;
    return *this;
  }

/** 
  @brief Concatenate an arc to path.

  @details The method inserts the "arc" arc and inserts its end as the last node
  path "this".

  One of the ends of the arc should be the last node of way.

  The operation takes constant time.

  After the last node operation is end of road is newly inserted arc.

  This is the most common and more sense to build operation
  progressively a road. She operates on all types of
  graphs, digraphs, or multidigrafos multigraphs.

  If it is a graph, then the path should be at least
  a node inserted. Otherwise, is triggered the domain_error
  exception.
      
  @param[in] arc The arc to be inserted on the road.
  
  @throw bad_alloc If no memory to insert the arc.
  @throw invalid_argument If the arc is not connected to last
  node road.
  @throw domain_error If the road is empty.
*/
  void append(Arc * arc)
  {
    if (list.is_empty())
      throw std::domain_error("path is empty");

    Path_Desc & last_path_desc = list.get_last();

    if (g == NULL)
      throw std::domain_error("Graph has not been specified");

    last_path_desc.arc = arc;
    list.append(Path_Desc(g->get_connected_node(arc, last_path_desc.node)));
  }

/** 
  @brief Concatenate the path node.

  @details The method inserts the "node" node as the last node in the path "this".

  The node "node" must be connected by one of the arches
  adjacent to the first node of the way. Otherwise it is generated
  the domain_error exception.

  The operation seeks sequentially among the arches
  first node of the way to find a arc that connects to
  node "node". The expected execution time is therefore proportional
  the number of adjacent arches of the last node in the path.

  This method is not suitable for multigraphs or multidigrafos,
  because the arc associated with the road will be the first one found
  to "node" between all adjacent arches of the last node of 
  way.

  After the "node" operation becomes the last node in the path.
  
  @param[in] node The node to be inserted on the road.
  
  @throw bad_alloc If no memory to insert the arc.
  @throw invalid_argument If the arc is not connected to last
  node road
  @throw domain_error If node is not connected by any arc of
  the first node of the road.
*/
  void append(Node * node)
  {
    if (list.is_empty())
      {
        init(node);
        return;
      }

    if (g == NULL)
      throw std::domain_error("Graph has not been specified");

    Node * last_node = get_last_node();
    Arc * arc        = search_arc(*g, last_node, node); 

    if (arc == NULL)
      throw std::domain_error("There is no an arc connecting to the node");

    append(arc);
  }

  void append_directed(Node * p)
  {
    if (list.is_empty())
      {
        init(p);
        return;
      }

    if (g == NULL)
      throw std::domain_error("Graph has not been specified");

    Node * last_node = get_last_node();
    Arc * arc        = search_directed_arc(*g, last_node, p);

    if (arc == NULL)
      throw std::domain_error("There is no an arc connecting to the node");

    append(arc);
  }

/** 
  @brief Insert a arc as first of way.

  @details The method inserts the "arc" arc and inserts its end as the first
  node path "this".

  One of the ends of the arc should be the first node way.

  The operation takes constant time.

  After the operation the first node is end of road 
  newly inserted arc.

  The operation works on all types of
  graphs, digraphs, or multidigrafos multigraphs.

  If it is a graph, then the path should be at least
  a node inserted. Otherwise, is triggered the domain_error 
  exception.

  @param[in] arc The arc to be inserted on the road.

  @throw bad_alloc If no memory to insert the arc.
  @throw invalid_argument If the arc is not connected to the first
  node of path
  @throw domain_error If the road is empty.
*/
  void insert(Arc * arc)
  {
    if (list.is_empty())
      throw std::domain_error("path is empty");

    if (g == NULL)
      throw std::domain_error("Graph has not been specified");

    Path_Desc & first_path_desc = list.get_first();

    Path_Desc item(g->get_connected_node(arc, first_path_desc.node), arc);

    list.insert(item);
  }

/** 
  @brief Insert a node as first of way.

  @details The method inserts the "node" node as the first node in the path "this".

  The node "node" must be connected by one of the arches
  adjacent to the first node of the way. Otherwise it is generated
  the domain_error exception.

  The operation seeks sequentially among the arches of the 
  first node of the way to find a arc that connects to
  node "node". The expected execution time is therefore proportional
  the number of adjacent arches of the first node in the path.

  This method is not suitable for multigraphs or multidigrafos,
  because the arc associated with the road will be the first one found
  to "node" between all adjacent arches of the last node of 
  way. 

  After the "node" operation becomes the first node in the path.

  @param[in] node The node to be inserted on the road.

  @throw bad_alloc If no memory to insert the arc.
  @throw invalid_argument If the arc is not connected to the first
  node of path
  @throw domain_error If node is not connected by any arc of
  the first node of the road.
*/
  void insert(Node * node)
  {
    if (list.is_empty())
      {
        init(node);
        return;
      }

    if (g == NULL)
      throw std::domain_error("Graph has not been specified");

    Node * first_node = get_first_node();
    Arc * arc = search_arc(*g, node, first_node); /* Busque arco last_node-node */
    if (arc == NULL)
      throw std::domain_error("There is no arc connecting node");

    insert(arc);
  }

/** 
  @brief Returns the first node in the path. 
*/
  Node * get_first_node() const { return list.get_first().node; }

/** 
  @brief Returns the last node in the path. 
*/
  Node * get_last_node() const { return list.get_last().node; }

/** 
  @brief Returns the first arc in the path. 
*/
  Arc * get_first_arc() const { return list.get_first().arc; }

/** 
  @brief Returns the last arc in the path. 
*/
  Arc * get_last_arc() const 
  {
    if (list.is_unitarian())
      throw std::domain_error("Path with only a node (without any arc");

    typename DynDlist<Path_Desc>::Iterator it(list);
    it.reset_last();
    it.prev();
    return it.get_current().arc; 
  }

/** 
  @brief Returns true if the path forms a cycle. 
*/
  bool is_cycle() const { return get_first_node() == get_last_node(); }

/** 
  @brief Removes the last node of the way. 
*/
  void remove_last_node()
  {
    list.remove_last();
    list.get_last().arc = NULL;
  }

/** 
  @brief Removes the first node of the way. 
*/
  void remove_first_node()
  {
    list.remove_first();
  }

/** 
  @brief Intercambia los contenidos del camino this con el de path. 
*/
  void swap(Path & path)
  {
    std::swap(g, path.g);
    list.swap(path.list);
  }

/** 
  @brief Iterator nodes and arcs on a path.

  @ingroup Grafos

  @see Path
*/
  class Iterator : public DynDlist<Path_Desc>::Iterator
  {
  public:

/** 
  @brief An iterator instance path on the road. 
*/
    Iterator(const Path & path) : DynDlist<Path_Desc>::Iterator(path.list) { }

  private:

    Path_Desc & get_curr_path_desc() const
    {
      return this->DynDlist<Path_Desc>::Iterator::get_current();
    }

  public:

/** 
  @brief Returns the current node on which is positioned
  the road iterator. 
*/
    Node * get_current_node() const { return this->get_curr_path_desc().node; }

/** 
  @brief Returns the current arc on which is positioned
  the road iterator. 
*/
    Arc * get_current_arc() const
    { 
      if (this->is_in_last())
        throw std::overflow_error("Path iterator is in last node of path");

      return this->get_curr_path_desc().arc; 
    }

/** 
  @overload get_current_node 
*/
    Node * get_current()  const { return get_current_node(); }

/** 
  @overload get_current_node 
*/
    Node * get_curr()  const { return get_current_node(); }

    tuple<Node*, Arc*> get_pair() const
    {
      return make_tuple(get_current_node(), get_current_arc());
    }

/** 
  @brief Returns true if there is a current arc; false otherwise.
  
  @details The iterator is in the last arc, then there is no current arc 
  and therefore returns false; 
*/
    bool has_current_arc() const
    {
      return this->has_current() and not this->is_in_last();
    }

/** 
  @brief Returns true if the iterator has current node; false otherwise. 
*/
    bool has_current_node() const { return this->has_current(); }
  };

/** 
  @brief Perform an operation on all the nodes of a road.

  @details This actuator path through each node and executes the operation
  op(node).

  @param[inout] op Operation to be performed on each node
*/
      template <class Operation> 
  void for_each_node(Operation & op) const
  {
    for (Iterator it(*this); it.has_current_node(); it.next())
      op (it.get_current_node());
  }

      template <class Operation> 
  void for_each_node(Operation && op = Operation()) const
  {
    for_each_node<Operation>(op);
  }

/** 
  @brief Perform an operation on all the arcs of a road.

  @details This actuator path through each arc and executes the operation
  op(arc).
	      
  @param[inout] op Operation to be performed on each arc
*/
      template <class Operation> 
  void for_each_arc(Operation & op) const
  {
    for (Iterator it(*this); it.has_current_arc(); it.next())
      op (it.get_current_arc());
  }

      template <class Operation> 
  void for_each_arc(Operation && op = Operation()) const
  {
    for_each_arc<Operation>(op);
  }

/** 
  @brief Returns true if node belongs to the road; false otherwise. 
*/
  bool contains_node(Node * node) const
  {
    for (Iterator it(*this); it.has_current_node(); it.next())
      if (it.get_current_node() == node)
        return true;
    return false;
  }

/** 
  @brief Returns true if node belongs to the road; false otherwise. 
*/
  bool contains_arc(Arc * arc) const
  {
    for (Iterator it(*this); it.has_current_arc(); it.next())
      if (it.get_current_arc() == arc) 
        return true;
    return false;
  }

  template <template <typename> class Container = DynList>
  Container<Node*> nodes() const
  {
    Container<Node*> ret_val;
    for_each_node([&ret_val] (Node * p)
		  {
		    ret_val.append(p);
		  });

    return ret_val;
  }

  template <template <typename> class Container = DynList>
  Container<Arc*> arcs() const
  {
    Container<Arc*> ret_val;
    for_each_arc([&ret_val] (Arc * a)
		  {
		    ret_val.append(a);
		  });

    return ret_val;
  }

  bool operator == (const Path & p) const
  {
    return eq(this->list, p.list);
  }

  bool operator != (const Path & p) const
  {
    return not eq(this->list, p.list);
  }

  template <class Operation>
  bool traverse(Operation & op) const
  {
    for (Iterator it(*this); it.has_current_arc(); it.next())
      if (not op(it.get_current_arc()))
	return false;
    return true;
  }

  template <class Operation>
  bool traverse(Operation && op) const
  {
    return traverse<Operation>(op);
  }
}; 

    template <class GT, class SA = Dft_Show_Arc<GT>> inline 
bool find_path_depth_first(GT & g, typename GT::Node * start_node, 
                           typename GT::Node * end_node, Path<GT> & path);

    template <class GT, class SA> inline 
bool __find_path_depth_first(GT& g, typename GT::Node * curr_node,
                             typename GT::Arc *  curr_arc, 
                             typename GT::Node * end_node, Path<GT> & curr_path)
{
    /** The end node is reached?
    YES? ==> then add the finish arc and end */
  if (curr_node == end_node) 
    {
      curr_path.append(curr_arc);
      return true;
    }
    /** Have you been visited?
    YES? ==> from him no way */
  if (IS_NODE_VISITED(curr_node, Find_Path)) 
    return false;

  curr_path.append(curr_arc); /* Add curr_arc at path */
  NODE_BITS(curr_node).set_bit(Find_Path, true); 

  /** Search recursively through arches of curr_node */
  for (Node_Arc_Iterator<GT, SA> i(curr_node); i.has_current(); i.next())
    {
      typename GT::Arc * next_arc = i.get_current_arc();
      if (IS_ARC_VISITED(next_arc, Find_Path)) 
        continue;

      ARC_BITS(next_arc).set_bit(Find_Path, true); 
      typename GT::Node * next_node = i.get_tgt_node(); 
      if (__find_path_depth_first<GT, SA> (g, next_node, next_arc, 
                                           end_node, curr_path))
        {
          assert(curr_path.get_last_node() == end_node);
	  return true; /* Se encontró camino */
        }
    }

  curr_path.remove_last_node(); 

  return false;
}

/** 
  @brief Depth searches a path between start_node and end_node;
  built the road viewed if you find the way.

  @details find_path_depth_first() depth searches a path between
  start_node and end_node, while building a road going
  recursive equivalent to the depth of the search. If the is
  finds a way, then the method returns true and the
  parameter "path" holds the road in question; on the contrary,
  the function returns false and the way value is indeterminate.

  The function takes two type parameters:
  -# GT: Graph type, which must be derived from List_Graph.
  -# SA: Class is responsible for displaying the arc. Internally,
  the function uses iterator filter Node_Arc_Iterator (based on
  Filter_Iterator) to traverse the arcs of each node. SA is the 
  class that determines whether or not to display bow to
  traveled.

  @param[in] g Graph on which you want to find the road.
  @param[in] start_node A pointer at the beginning node of the road.
  @param[in] end_node A pointer to the destination node of the road.
  @param[out] path Road seen during the search in depth; 
  only it makes sense if the return value is true.

  @throw bad_alloc If no memory is to continue building the road

  @see find_path_breadth_first()
  @see dijkstra_min_spanning_tree() dijkstra_min_path()
  @see bellman_ford_min_spanning_tree() q_bellman_ford_min_spanning_tree()

  @ingroup Grafos
*/
    template <class GT, class SA> inline 
bool find_path_depth_first(GT & g, typename GT::Node * start_node, 
                           typename GT::Node * end_node, Path<GT> & path)
{
  if (not path.inside_graph(g))
    throw std::invalid_argument("Path does not belong to graph");

  path.clear_path();         
  path.init(start_node);     
  g.reset_bit_nodes(Find_Path);
  g.reset_bit_arcs(Find_Path); 
  NODE_BITS(start_node).set_bit(Find_Path, true);

    /* Recursively scan each arc start_node */
  for (Node_Arc_Iterator<GT, SA> i(start_node); i.has_current(); i.next())
    {
      typename GT::Arc * arc = i.get_current_arc();
      ARC_BITS(arc).set_bit(Find_Path, true); 
      typename GT::Node * next_node = i.get_tgt_node();
      if (IS_NODE_VISITED(next_node, Find_Path)) 
        continue; 

      if (__find_path_depth_first<GT, SA>(g, next_node, arc, end_node, path)) 
        return true;
    }

  return false;
}

    template <class GTS, class GTT>
void map_nodes(typename GTS::Node * p, typename GTT::Node * q)
{
  assert(p != NULL and q != NULL);						
   									
  if (NODE_COOKIE(p) == NULL)						
    {									
      NODE_COOKIE(p) = q;						
      NODE_COOKIE(q) = p;						
      return;								
    }									
   									
  NODE_COOKIE(q) = NODE_COOKIE(p);					
  NODE_COOKIE(p) = q;							
}									
									
    template <class GTS, class GTT>
void map_arcs(typename GTS::Arc * p, typename GTT::Arc * q)
{									
  assert(p != NULL and q != NULL);						
									
  if (ARC_COOKIE(p) == NULL)						
    {									
      ARC_COOKIE(p) = q;						
      ARC_COOKIE(q) = p;						
      
      return;								
    }									
									
  ARC_COOKIE(q) = ARC_COOKIE(p);					
  ARC_COOKIE(p) = q;							
}


    //    /********************************
    //    List_Graph Method Implementations
    //    ********************************/


   template <typename Node, typename Arc>
Node * List_Graph<Node, Arc>::get_first_node() const
{
  if (num_nodes == 0)
    throw std::range_error("Graph has not nodes");

  return dlink_to_node(const_cast<Dlink&>(node_list).get_next());
}

   template <typename Node, typename Arc>
Arc * List_Graph<Node, Arc>::get_first_arc() const
{
  if (get_num_arcs() == 0)
    throw std::range_error("Graph has not arcs");

  return dlink_to_arc(const_cast<Dlink&>(arc_list).get_next());
}

   template <typename Node, typename Arc>
Arc * List_Graph<Node, Arc>::get_first_arc(Node * node) const
{
  if (get_num_arcs(node) == 0)
    throw std::range_error("node has not arcs");

  void * arc = dlink_to_arc_node(node->arc_list.get_next())->arc;
  return reinterpret_cast <Arc *> (arc);
}
   template <typename Node, typename Arc>
List_Graph<Node, Arc>::Node_Iterator::Node_Iterator
   (const List_Graph<Node, Arc> & _g)
     : Dlink::Iterator(&const_cast<Dlink&>(_g.node_list))
{
  /* Empty */
}

    template <typename Node, typename Arc>
Node * List_Graph<Node, Arc>::Node_Iterator::get_current_node() 
{
  return dlink_to_node(Dlink::Iterator::get_current()); 
}

   template <typename Node, typename Arc>
List_Graph<Node, Arc>::Arc_Iterator::Arc_Iterator
   (const List_Graph<Node, Arc> & _g) 
     : Dlink::Iterator(&const_cast<Dlink&>(_g.arc_list))
{
  /* Empty */
}

   template <typename Node, typename Arc>
Arc * List_Graph<Node, Arc>::Arc_Iterator::get_current_arc() const
{
  return dlink_to_arc(const_cast<Dlink*>(Dlink::Iterator::get_current())); 
}

   template <typename Node, typename Arc>
Arc * List_Graph<Node, Arc>::Node_Arc_Iterator::get_current_arc() const
{
  return static_cast<Arc*>(get_current_arc_node()->arc);
}

    template <typename Node, typename Arc>
Node * List_Graph<Node, Arc>::insert_node(Node * node)
{
  ++num_nodes;
  node_list.append(node);
  return node;
}

    template <typename Node, typename Arc> Node * 
List_Graph<Node, Arc>::insert_node(const typename Node::Node_Type & node_info)
{
  return List_Graph<Node, Arc>::insert_node( new Node (node_info) );
}

    template <typename Node, typename Arc> Node * 
List_Graph<Node, Arc>::insert_node(typename Node::Node_Type && node_info)
{
  return List_Graph<Node, Arc>::insert_node
    (new Node (std::move(node_info)) );
}

   template <typename Node, typename Arc> Arc * 
List_Graph<Node, Arc>::insert_arc(Node * src_node, Node * tgt_node)
{  
    /* Step 1: Set aside memory for Arc and start */
  unique_ptr<Arc> arc ( new Arc ); /* Set aside memory */
  arc->src_node   = src_node;      /* Start the fields */
  arc->tgt_node   = tgt_node;

    /* Step 3: (parcial): Set aside Arc_Node of src_node   */
  unique_ptr<Arc_Node> src_arc_node (new Arc_Node (arc.get()));

    /* Step 2: if it is graph ==> set aside Arc_Node from tgt_node */
  if (not digraph)  /* If it is digraph ==> Don't put in another node */
    {               /* Insertion in target node  */
      if (src_node == tgt_node) /* is a loop? */
        arc->tgt_arc_node = src_arc_node.get(); 
      else
        {     
    /* Set aside node for tgt_node */
          unique_ptr<Arc_Node> tgt_arc_node(new Arc_Node(arc.get())); 

    /* Insertion adjacency list tgt_node */
          arc->tgt_arc_node = tgt_arc_node.get();    
          tgt_node->arc_list.append(tgt_arc_node.get());
	  tgt_node->num_arcs++;
          tgt_arc_node.release(); 
        }
    }

    /* Step 3 (resto): Insertion adjacency list src_node */
  arc->src_arc_node = src_arc_node.get();
  src_node->arc_list.append(src_arc_node.get());
  src_node->num_arcs++;

  arc_list.append(arc.get()); /* Step 4: Insert graph arcs list */
  ++num_arcs;
  src_arc_node.release();

  return arc.release();  
}


    template <typename Node, typename Arc> 
Arc * List_Graph<Node, Arc>::connect_arc(Arc * arc)
{      
  Node * src_node         = get_src_node(arc); 
  Node * tgt_node         = get_tgt_node(arc); 
  Arc_Node * src_arc_node = arc->src_arc_node;
  Arc_Node * tgt_arc_node = arc->tgt_arc_node;

  if (not digraph) /* If it is digraph ==> don't insert on the other node */
    {     /* Insert in target node */
      if (src_node != tgt_node) /* Check if this is a cycle */
        {     /* Insertion adjacency listtgt_node */
          tgt_node->arc_list.append(tgt_arc_node);
          tgt_node->num_arcs++;
        }
    }

  src_node->arc_list.append(src_arc_node);
  src_node->num_arcs++;
  arc_list.append(arc);
  ++num_arcs;

  return arc;
}

   template <typename Node, typename Arc> Arc * 
List_Graph<Node, Arc>::insert_arc(Node * src_node, Node * tgt_node, 
                                  const typename Arc::Arc_Type & arc_info)
{
  Arc * arc = List_Graph<Node, Arc>::insert_arc(src_node, tgt_node);
  arc->get_info() = arc_info;

  return arc;
}

   template <typename Node, typename Arc> Arc * 
List_Graph<Node, Arc>::insert_arc(Node * src_node, Node * tgt_node, 
                                  typename Arc::Arc_Type && arc_info)
{
  Arc * arc = List_Graph<Node, Arc>::insert_arc(src_node, tgt_node);
  arc->get_info() = std::move(arc_info);

  return arc;
}


   template <typename Node, typename Arc> 
void List_Graph<Node, Arc>::remove_arc(Arc * arc) 
{      /* Paso 1: Delete Arc_node from src_node */
  Node * src_node         = get_src_node(arc); 
  Arc_Node * src_arc_node = arc->src_arc_node;

  src_arc_node->del();  /* src_node unlinked from the list of nodes */
  src_node->num_arcs--; /* Arches counter decrements src_node */
  delete src_arc_node;  /* Delivery report */

  if (not digraph) 
    {     /* Delete arc in target node */
      Node * tgt_node = get_tgt_node(arc); 
      if (src_node != tgt_node) /* Removing verify cycle */
        {  /* Paso 2: Delete Arc_node from tgt_node */
          Arc_Node * tgt_arc_node = arc->tgt_arc_node;
          tgt_arc_node->del(); 
          tgt_node->num_arcs--;
          delete tgt_arc_node;
        }
    }

    /* Elimination of arc-arc graph V> of ();
    List unlink arc arches graph*/
  --num_arcs;
  delete arc;
}

   template <typename Node, typename Arc> 
void List_Graph<Node, Arc>::disconnect_arc(Arc * arc) 
{      /* Paso 1: Removing the source node Arc_node src_node */
  Node * src_node         = get_src_node(arc); 
  Arc_Node * src_arc_node = arc->src_arc_node;
  src_arc_node->del();   /* src_node unlinked from the list of nodes */
  src_node->num_arcs--;  /* Arches counter decrements src_node */

  if (not digraph) 
    {     
    /* Arch destination node removal */
      Node * tgt_node = get_tgt_node(arc); 
      if (src_node != tgt_node) /* Removing verify cycle */
        {  /* Paso 2: Removing Arc_node the destination node tgt_node*/
          Arc_Node * tgt_arc_node = arc->tgt_arc_node;
          tgt_arc_node->del(); 
          tgt_node->num_arcs--;
        }
    }

    /* Elimination arc graph */
  arc->del(); /* Unlink arc of the graph arcs list */
  --num_arcs;
}

   template <typename Node, typename Arc> 
void List_Graph<Node, Arc>::remove_node(Node * node)
{
  if (not digraph)
        /* Remove adjacent arcs to node */
    while (not node->arc_list.is_empty()) /* While supplies arches */
      {     /* Ger Arc_Node */
        Arc_Node * arc_node = 
          dlink_to_arc_node(node->arc_list.get_next());
        Arc * arc = void_to_arc(arc_node); /* Get the Arc */
        remove_arc(arc); /* Remove from the arc */
      }
  else
    for (Arc_Iterator it(*this); it.has_current();)
      {
        Arc * arc = it.get_current();
        if (get_src_node(arc) == node or get_tgt_node(arc) == node)
          {
            it.next();
            remove_arc(arc);
          }
        else
          it.next();
      }

    /* At this point the node no longer has arches */
  node->del(); /* Unlink node list of nodes in the graph */
  --num_nodes;
  delete node; 
}

   template <typename Node, typename Arc>
List_Graph<Node, Arc>::List_Graph(const List_Graph<Node, Arc> & g) 
{
  init();
  copy_graph(*this, g);
}

   template <typename Node, typename Arc>
List_Graph<Node, Arc>::List_Graph(List_Graph<Node, Arc> && g) 
{
  init();
  swap(g);
}

   template <typename Node, typename Arc>
List_Graph<Node, Arc> & List_Graph<Node, Arc>::operator = 
   (const List_Graph<Node, Arc> & g) 
{
  copy_graph(*this, g);

  return *this;
}


   template <typename Node, typename Arc>
List_Graph<Node, Arc> & List_Graph<Node, Arc>::operator = 
   (List_Graph<Node, Arc> && g) 
{
  swap(g);
  return *this;
}

   template <typename Node, typename Arc>
List_Graph<Node, Arc>::~List_Graph() 
{ 
  clear_graph(*this); 
}

   template <typename Node, typename Arc>
   template <class Compare>
void List_Graph<Node, Arc>::sort_arcs(Compare && cmp) 
{
  mergesort<Dlink, Cmp_Arc<Compare>>(arc_list, Cmp_Arc<Compare>(cmp)); 
}

   template <typename Node, typename Arc>
   template <class Compare>
void List_Graph<Node, Arc>::sort_arcs(Compare & cmp) 
{
  Cmp_Arc<Compare> comp(cmp);
  mergesort<Dlink, Cmp_Arc<Compare>>(arc_list, comp); 
}

GRAPH_METHODS_IMPLS(List_Graph);

   template <class GT, class SA>
typename GT::Arc * search_arc(const GT & g, 
			      typename GT::Node * src, typename GT::Node * tgt) 
{
  assert(src != NULL and tgt != NULL);

    /* Escoger nodo con menos arcos */
  if (not g.is_digraph() and tgt->num_arcs < src->num_arcs)
    std::swap(tgt, src);

  for (Node_Arc_Iterator<GT, SA> itor(src); itor.has_curr(); itor.next())  
    if (itor.get_tgt_node() == tgt)
      return itor.get_current_arc();

  return NULL; 
}


   template <class GT>
typename GT::Arc * search_arc(const GT & g, typename GT::Node * src_node, 
                              typename GT::Node * tgt_node)
{
  return search_arc<GT, Dft_Show_Arc<GT>>(g, src_node, tgt_node);
}

    template <class GT> 
void clear_graph(GT & g)
{     
  for (typename GT::Arc_Iterator it(g); it.has_current(); ) /* Remove arches */
    {
      typename GT::Arc * arc = it.get_current();
      it.next();
      g.remove_arc(arc);
    }

  for (typename GT::Node_Iterator it(g); it.has_current(); ) /* Remove nodes */
    {     
      typename GT::Node * p = it.get_current(); 
      it.next(); /* Advance before deleting (iterator consistency) */
      g.remove_node(p); /* Removes from the arc */
    }
}


    template <class GT> 
void copy_graph(GT & gtgt, const GT & gsrc, const bool cookie_map)
{
  try
    {
      clear_graph(gtgt); /* Clean this before copying */
      DynMapAvlTree<typename GT::Node*, typename GT::Node*> map;  

    /* Fase 1: Through nodes of copy and insert src_graph this */
      for (typename GT::Node_Iterator it(gsrc); it.has_curr(); it.next())
	{
	  typename GT::Node * src_node = it.get_current_node(); 
	  unique_ptr<typename GT::Node> 
	    tgt_node(new typename GT::Node(src_node->get_info()));
	  map.insert(src_node, tgt_node.get()); 

	  typename GT::Node * tgt = tgt_node.release();
	  assert(tgt->get_info() == src_node->get_info());
	  gtgt.insert_node(tgt); /* Embed in graph destination */

	  if (cookie_map)
	    GT::map_nodes(src_node, tgt);
	}

      assert(gtgt.get_num_nodes() == gsrc.get_num_nodes());

    /* Fase 2: Each arc src_graph on this create one
    arc that connects mapped nodes map */
      for (typename GT::Arc_Iterator it(gsrc); it.has_current(); it.next())
	{
	  typename GT::Arc * src_arc = it.get_current_arc();

    /* Get pictures of nodes in the graph destination and create arc */
	  typename GT::Node * src_node = map(gsrc.get_src_node(src_arc)); 
	  typename GT::Node * tgt_node = map(gsrc.get_tgt_node(src_arc));
	  typename GT::Arc * tgt_arc   = gtgt.insert_arc(src_node, tgt_node); 
	  //tgt_arc->get_info() = src_arc->get_info(); 
	  *tgt_arc = *src_arc;
	  if (cookie_map)
	    GT::map_arcs(src_arc, tgt_arc);
	}

      assert(gtgt.get_num_arcs() == gsrc.get_num_arcs());
    }
  catch (...)
    {     /* If exception occurs is cleaned this */
      clear_graph(gtgt); 
      throw;
    }
}

    template <class GTT, class GTS>
struct Dft_Copy_Node
{
  void operator () (typename GTT::Node * tgt, typename GTS::Node * src)
  {
    tgt->get_info() = src->get_info();
  }
};


    template <class GTT, class GTS>
struct Dft_Copy_Arc
{
  void operator () (typename GTT::Arc * tgt, typename GTS::Arc * src)
  {
    tgt->get_info() = src->get_info();
  }
};

/** 
  @brief Copy between different classes of graphs.

  @ingroup Grafos
*/
    template <class GTT, class GTS, 
	      class Copy_Node = Dft_Copy_Node<GTT, GTS>, 
	      class Copy_Arc = Dft_Copy_Arc<GTT, GTS>> 
void inter_copy_graph(GTT & gtgt, const GTS & gsrc, 
		      const bool cookie_map = false)
{
  try
    {
      clear_graph(gtgt); /* Clean this before copying */
      DynMapAvlTree<typename GTS::Node*, typename GTT::Node*> map;  

          /* Fase 1: Through nodes of copy and insert src_graph this */
      for (typename GTS::Node_Iterator it(gsrc); it.has_current(); it.next())
	{
	  typename GTS::Node * src_node = it.get_current_node(); 
	  unique_ptr<typename GTT::Node> tgt_node(new typename GTT::Node);
	  Copy_Node () (tgt_node.get(), src_node);
	  map.insert(src_node, tgt_node.get()); 

	  typename GTT::Node * tgt = tgt_node.release();
	  gtgt.insert_node(tgt); /* Embed in graph destination */

	  if (cookie_map)
	    map_nodes<GTS, GTT>(src_node, tgt);
	}

      assert(gtgt.get_num_nodes() == gsrc.get_num_nodes());

    /* Fase 2: Each arc src_graph on this create one
     arc that connects mapped nodes map */
      for (typename GTS::Arc_Iterator it(gsrc); it.has_current(); it.next())
	{
	  typename GTS::Arc * src_arc = it.get_current_arc();

    /* Get pictures of nodes in the graph destination and create arc */
	  typename GTT::Node * src_node = map[gsrc.get_src_node(src_arc)]; 
	  typename GTT::Node * tgt_node = map[gsrc.get_tgt_node(src_arc)];
	  typename GTT::Arc * tgt_arc   = gtgt.insert_arc(src_node, tgt_node);
	  Copy_Arc () (tgt_arc, src_arc);
	  if (cookie_map)
	    map_arcs<GTS, GTT>(src_arc, tgt_arc);
	}

      assert(gtgt.get_num_arcs() == gsrc.get_num_arcs());
    }
  catch (...)
    {     /* If exception occurs is cleaned this */
      clear_graph(gtgt); 
      throw;
    }
}


/** 
  @brief Selective copy of graphs.

  @details The class Copy_Graph made copy of a graph according to
  filters nodes and arcs.

  The parameters of the class are:
  -# GT: Type of graph to be copied.
  -# SN: Filter of nodes.
  -# SA: Filter of arches.

  Only filtered by SN nodes are copied.

  Only filtered by SA arches are copied.

  By default, the two filters return all nodes. Namely,
  copying is entirely carried out.

  @ingroup Grafos
*/
template <class GT, class SN = Dft_Show_Node<GT>, class SA = Dft_Show_Arc<GT>>
class Copy_Graph
{
  SN & sn;
  SA & sa;

public:

/** 
  @brief Builder copying.

  @param[in] __sa Arches filter value
  @param[in] __sn Nodes filter value
*/
  Copy_Graph(SA & __sa, SN & __sn)
    : sn(__sn), sa(__sa)
  {
    /* Empty */
  }

/** 
  @overload Copy_Graph 
*/
  Copy_Graph(SA && __sa = SA(), SN && __sn = SN())
    : sn(__sn), sa(__sa)
  {
    /* Empty */
  }

/** @overload Copy_Graph */
  Copy_Graph(SA & __sa, SN && __sn = SN())
    : sn(__sn), sa(__sa)
  {
    /* Empty */
  }

private:

  void copy(GT & gtgt, const GT & gsrc, const bool cookie_map)
  {
    try
      {
	clear_graph(gtgt); /* Clear this before copy */
	DynMapAvlTree<typename GT::Node*, typename GT::Node*> map;  

    /* Fase 1: Through nodes of copy and insert src_graph this */
	for (Node_Iterator<GT, SN> it(gsrc, sn); it.has_curr(); it.next())
	  {
	    typename GT::Node * src_node = it.get_curr(); 
	    unique_ptr<typename GT::Node> 
	      tgt_node(new typename GT::Node(src_node));
	    map.insert(src_node, tgt_node.get()); 
	    
	    typename GT::Node * tgt = tgt_node.release();
	    gtgt.insert_node(tgt); /* Insert in graph destination */

	    if (cookie_map)
	      GT::map_nodes(src_node, tgt);
	  }

    /* Fase 2: por each arch src_graph on this, create one
     arc that connects mapped nodes map */
	for (Arc_Iterator<GT, SA> it(gsrc, sa); it.has_curr(); it.next())
	  {
	    typename GT::Arc * src_arc = it.get_curr();

    /* Get pictures of nodes in the graph destination and create arc */
	    typename GT::Node * src_node = map[gsrc.get_src_node(src_arc)]; 
	    typename GT::Node * tgt_node = map[gsrc.get_tgt_node(src_arc)];
	    typename GT::Arc * tgt_arc   = 
	      gtgt.insert_arc(src_node, tgt_node, src_arc->get_info()); 

	    if (cookie_map)
	      GT::map_arcs(src_arc, tgt_arc);
	  }
      }
    catch (...)
      {     /* If exception occurs is cleaned this */
	clear_graph(gtgt); 
	throw;
      }
  }

public:

/** 
  @brief Copy "gsrc" the graph "gtgt".

  @details Performs the copy of the nodes belonging to "gsrc" as
  SN filtering in construction time to the destination graph
  "gtgt". Then the arches filtered by SA in construction time
  they are copied from "gsrc" to "gtgt".

  @param[out] gtgt Graph destination of the copy.
  @param[in] gsrc Graph source of the copy.
  @param[in] cookie_map if the value to true, then the nodes and
  arcs of the graphs are mapped.
  \*/
  void operator () (GT & gtgt, GT & gsrc, const bool cookie_map = true)
  {
    copy(gtgt, gsrc, cookie_map);
  }
};

/** 
  @brief Filtro de arcos pintados por algoritmos de árboles abarcadores. 

  @details A usarse para observar el resultado sobre el grafo y no sobre una copia.

  Funciona con los grafos pintados mediante los algoritmos de Kruskal,
  Prim, Dijkstra y Bellman-Ford. 

  @ingroup Grafos
*/
template <class GT, class Distance>
struct Painted_Min_Spanning_Tree 
{
  typename Distance::Distance_Type dist;

  Painted_Min_Spanning_Tree() : dist(0) { /* Empty */ }

  bool operator () (typename GT::Arc * a)
  {
    if (not IS_ARC_VISITED(a, Aleph::Spanning_Tree))
      return false;

    dist = dist + ARC_DIST(a);

    return true;
  }
};

} /* End namespace Aleph */ 

# endif /* TPL_GRAPH_H */
