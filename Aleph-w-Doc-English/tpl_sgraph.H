
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
  @file tpl_sgraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SGRAPH_H
# define TPL_SGRAPH_H

# include <memory>
# include <htlist.H>
# include <tpl_graph.H>
# include <tpl_dynSetTree.H>

using namespace Aleph;

namespace Aleph {


/** 
  @brief Graph node implemented with simple adjacency lists.

  @details This class defines a node or vertex of a graph or digraph
  implemented by adjacency lists.

  There are basically three ways to define the information
  it belongs to a node of a graph:
  - The parameter type inherent to the node named \c
  Node_Info.
  - Through inheritance of this base class. This is the way
  keep different types of node in a graph.
  - Combination of the previous two.

  @param Node_Info Type of data stored in the network and accessible
  by the method \c get_info().

  @note This class is only for the definition of nodes
  a graph. It is not intended to be used by the user. It is
  preferably through using the methods provided by \c
  List_Graph. While it is possible to use some methods or access
  some of its attributes, it is highly recommended not
  use. Instead, use the type \c
  List_Graph::Node and its associated methods.

  An node of graph has three additional attributes present by
  each node and denominated as follows:
  - Control Bits (\c Bit_Fields) set of bits used for
  algorithms to mark the node.
  - Counter (\c counter): The counter usually used to mark
  an order of visit or processing.
  - \c cookie: opaque pointer to any other information
  time you want to associate with the node.

  @ingroup Grafos
  
  @see List_Graph List_Digraph Graph_Arc Bit_Fields
*/
    template <typename Node_Info = Empty_Class> 
class Graph_Snode
{
public:

  GRAPH_NODE_COMMON(Graph_Snode);
  
/** @brief Lista de adyacencia */
  DynList<void*> arc_list; 

/** 
  @brief Default builder. 

  @details The value of the information contained in the node is determined
  by the value assigned by the constructor \c Node_Info::Node_Info,
  where \c Node_Info is the type associated with the node.

  The values of the control bits and the counter are placed in
  zero. The \c cookie is placed in \c NULL.
*/
  Graph_Snode() : num_arcs(0) { /* Empty */ }

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
  Graph_Snode(const Node_Info & info) : node_info(info), num_arcs(0)
  { 
    /* Empty */ 
  }

  Graph_Snode(const Graph_Snode & node) : Graph_Snode(node.node_info) { /* Empty */ }

  Graph_Snode & operator = (const Graph_Snode & node)
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
  Graph_Snode(Graph_Snode * node) : node_info(node->get_info()), num_arcs(0)
  { 
    /* Empty */ 
  }
};

/** 
  @brief Arc of graph implemented with simple adjacency lists.

  @details This class defines an arc or axis of a graph or digraph
  implemented by adjacency lists simply enlzadas.

  There are basically three ways to define the information
  it belongs to an arc of a graph:
  - The parameter type inherent to the node named \c
  Arc_Info. 
  - Through inheritance of this base class. This is the way
  keep different types of arcs in a graph.
  - Combination of the previous two.

  @param Type of data stored in the network and accessible
  by the method \c get_info ().

  @note This class is only for the definition of arches
  a graph. It is not intended to be used by the user. It is
  Preferably through using the methods provided by \c
  List_Graph. While it is possible to use some methods or access
  some of its attributes, it is highly recommended not
  use. Instead, use the type \c
  List_Graph::Arc and its associated methods.

  An arc of graph has three additional attributes present by
  each arc and denominated as follows:
  - Control Bits (\c Bit_Fields) set of bits used for
  algorithms to mark the arc.
  - Counter (\c counter): The counter usually used to mark
  an order of visit or processing.
  - \c cookie: opaque pointer to any other information
  time you want to associate with the arc.

  @ingroup Grafos
  
  @see List_Graph List_Digraph Graph_Node Bit_Fields
*/
    template <typename Arc_Info = Empty_Class> 
class Graph_Sarc
{
public:

  GRAPH_ARC_COMMON(Graph_Sarc);

  Graph_Sarc() : src_node(NULL), tgt_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Sarc(const Arc_Info & info) 
    : arc_info(info), src_node(NULL), tgt_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Sarc(const Graph_Sarc & arc) : Graph_Sarc(arc.arc_info) { /* Empty */ }

  Graph_Sarc & operator = (const Graph_Sarc & arc)
  {
    if (&arc == this)
      return *this;
    arc_info = arc.arc_info;
    return *this;
  }

  Graph_Sarc(void * src, void * tgt, const Arc_Info & data) 
    : arc_info(data), src_node(src), tgt_node(tgt)
  { 
    /* Empty */
  }

  Graph_Sarc(void * src, void * tgt) : src_node(src), tgt_node(tgt)
  { 
    /* Empty */
  }
};


/** 
  @brief Class graph with adjacency lists implemented.

  @details List_SGraph<Node, Arc> is a class that models graphs
  represented by adjacency lists.

  The class manages two parameters fundamental type:
  - __Graph_Node: The type of nodes that must be defined
    by the Graph_Snode class.
  - __Graph_Arc: The type of nodes that must be defined
    Graph_Sarc by class.

  These classes must have been previously defined.

  Once instantiated one List_SGraph<Node, Arc>, the nodes and
  arches must be accessed via the built-in types:
  - List_SGraph<Node, Arc>::Node
  - List_SGraph<Node, Arc>::Arc
 
  @param __Graph_Node Node type. You must be defined
  it from the class \c __Graph_Node, either by inclusion of
  attributes by derivation or combination of both.
  @param __Graph_Arc Arc type. You must be defined
  it from the class \c __Graph_Arc, either by inclusion of
  attributes by derivation or combination of both.
  
  @see Graph_SNode Graph_SArc
  @see List_SDigraph Path
  
  @ingroup Grafos
*/
  template <typename __Graph_Node = Graph_Snode<unsigned long>, 
	    typename __Graph_Arc  = Graph_Sarc<unsigned long>>
class List_SGraph
{
  GRAPH_ATTR_COMMON(List_SGraph);

private:

  typedef DynSetTree<Node*, Rand_Tree> DynSetNode;
  typedef DynSetTree<Arc*, Rand_Tree> DynSetArc;

  DynSetNode node_list; /* Lista de nodos */
  DynSetArc  arc_list;  /* Lista de arcos */

public:

/** 
  @brief Iterator of nodes in a graph.

  @details This class iterates over all nodes in a graph.

  @ingroup Grafos
*/
  class Node_Iterator : public DynSetNode::Iterator
  {
  public:

/** @brief Item type returning get_current() */
    typedef Node * Item_Type;

/** @brief The set type on which iterates. */
    typedef List_SGraph Set_Type;

    Node_Iterator() { /* Empty */ }

    Node_Iterator(const List_SGraph & g) : DynSetNode::Iterator(g.node_list)
    {
      /* Empty */
    }

/** @brief Returns the current node. */
    Node * get_current_node() const
    {
      return this->get_curr();
    }
  };

/** 
  @brief Arches iterator of a node of graph.

  @details This class iterates over all the adjacent arcs of a node
  belonging to a graph or digraph.

  @ingroup Grafos
*/
  class Node_Arc_Iterator : public DynList<void*>::Iterator
  {
    Node * src_node; 
    
  public:

/** @brief The data type that returns get_current() */
    typedef Arc * Item_Type;

/** @brief The set type on which iterates. */
    typedef Node * Set_Type;

/** @brief Instance an empty iterator (invalid). */
    Node_Arc_Iterator() { /* Empty */ }

/** @brief Instance an iterator on node _src_node */
    Node_Arc_Iterator(Node * src) 
      : DynList<void*>::Iterator(src->arc_list), src_node(src)
    {
      /* Empty */ 
    }

/** @brief Returns the current arc. */
    Arc * get_current_arc() const
    {
      return (Arc*) this->DynList<void*>::Iterator::get_curr();
    }

/** @brief @overload get_current_arc() */
    Arc * get_current() const { return get_current_arc(); }

/** @brief @overload get_current_arc() */
    Arc * get_curr() const { return get_current_arc(); }

/** @brief Returns the destination node of the current arc. */
    Node * get_tgt_node() const
    {
      Arc * a = get_curr();
      return (Node*) a->get_connected_node(src_node);
    }
  };

/** 
  @brief Iterator over arcs of a graph.

  @details This iterator traverses each arc of the graph. The visitation order
  it may be altered if the arches are arranged previously.

  @see sort_arcs()
*/
  class Arc_Iterator : public DynSetArc::Iterator
  {
  public:

/** @brief Item type returning get_current() */
    typedef Arc * Item_Type;

/** @brief The set type on which iterates. */
    typedef List_SGraph Set_Type;

    Arc_Iterator() { /* Empty */ }

    Arc_Iterator(const List_SGraph & _g) : DynSetArc::Iterator(_g.arc_list)
    {
      /* Empty */
    }

/** @brief Retorna un puntero al arco actual */
    Arc * get_current_arc() const 
    {
      return this->get_curr();
    }

/** @brief Returns the source node of the current arc
(only counts if digraph) */
    Node * get_src_node() const { return (Node*) get_current_arc()->src_node; }

/** @brief Returns the destination node of the current arc
(only counts if digraph) */
    Node * get_tgt_node() const { return (Node*) get_current_arc()->tgt_node; }
  };

  GRAPH_ITERATIVE_METHODS;

  GRAPH_SEARCH_METHODS; 


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
  @note   Usually, this routine should not be used. Instead you should
  used other insertion (which automatically separates memory).
*/
  virtual Node * insert_node(Node * p)
  {
    assert(p->arc_list.is_empty());

    ++num_nodes;
    node_list.append(p);

    return p;
  }

private:

  Arc * insert_arc(Node * src, Node * tgt, void * a)
  {
    Arc * arc = (Arc*) a;

    arc->src_node = src;
    arc->tgt_node = tgt;

    src->arc_list.append(a);
    src->num_arcs++;
    if (not digraph and src != tgt)
      {
	tgt->arc_list.append(a);
	tgt->num_arcs++;
      }

    arc_list.append(arc);
    ++num_arcs;

    return arc;
  }

public:

/** 
  @brief Create a new arc between two nodes.

  @details This method separates memory for a new arc between the nodes
  previously defined, \c src_node and \c tgt_node with value of 
  content in the arc_info arc.

  The nodes must have been previously inserted in the graph. In 
  this respect, no check is made. The assignment operator of the 
  Arc::Arc_Type class must have been defined.

  No verification any prior existence of an arc between the nodes 
  involved (this is necessary to operate with multigraphs) is performed.

  @param[in] src Pointer to the origin node.
  @param[in] tgt Pointer to the destination node.
  @param[in] arc_info Value of information to be copied to the arch.

  @return A pointer to the arc inserted

  @throw bad_alloc If no memory for the arc.
*/
  GRAPH_INSERTION_METHODS;

private:

  void disconnect_arc(Arc * arc)
  {
    Node * src = (Node*) arc->src_node;
    Node * tgt = (Node*) arc->tgt_node;

    src->arc_list.remove(arc);
    src->num_arcs--;

    if (not digraph and src != tgt)
      {
	tgt->arc_list.remove(arc);
	tgt->num_arcs--;
      }
  }

public:

/** 
  @brief Eliminates the arc called "arc".

  @details The operation removes the graph the arc \c arc and then releases its
  memory.

  The arc must belong to the graph and do not make any
  verification regard.

  @param[in] arc Pointer to arc to delete.
*/
  virtual void remove_arc(Arc * arc)
  {
    disconnect_arc(arc);
    arc_list.remove(arc);
    --num_arcs;
    delete arc;
  }

  virtual void remove_node(Node * p)
  {
    for (typename DynSetArc::Iterator it(arc_list); it.has_curr(); )
      {
        Arc * arc = (Arc*) it.get_curr();

        if (get_src_node(arc) == p or get_tgt_node(arc) == p)
          {
            disconnect_arc(arc);
            --num_arcs;
            delete it.del();
          }
        else
          it.next();
      }

    node_list.remove(p);
    --num_nodes;
    
    delete p;
  }

  Node * get_first_node() const
  {
    return (Node*) node_list.get_first();
  }

  Arc * get_first_arc() const
  {
    return (Arc*) arc_list.get_first();
  }

  Arc * get_first_arc(Node * p) const
  {
    return (Arc*) p->arc_list.get_first();
  }

  List_SGraph()
  {
    init();
  }

private:

  void clear()
  {
    arc_list.for_each( [/* Lambda */] (const Arc * p) { delete p; } );
    node_list.for_each( [/* Lambda */] (const Node * p) { delete p; } );
  }

public:

  ~List_SGraph()
  {
    clear();
  }

      template <class GT>
  List_SGraph(GT & g)
  {
    init();
    inter_copy_graph<GT, List_SGraph>(*this, g, false);
  }

  void swap(List_SGraph & g)
  {
    common_swap(g);
    node_list.swap(g.node_list);
    arc_list.swap(g.arc_list);
  }

  List_SGraph(List_SGraph && g) 
  {
    init();
    swap(g);
  }

  List_SGraph & operator = (const List_SGraph & g)
  {
    if (&g == this)
      return *this;

    clear();

    copy_graph(*this, const_cast<List_SGraph&>(g));

    return *this;
  }

  List_SGraph & operator = (List_SGraph && g)
  {
    swap(g);

    return *this;
  }

private:

      template <class Cmp>
  struct Cmp_Arc
  {
    Cmp & cmp;

    Cmp_Arc(Cmp && __cmp = Cmp()) : cmp(__cmp) { /* Empty */ }

    Cmp_Arc(Cmp & __cmp) : cmp(__cmp) { /* Empty */ }

    bool operator () (Slinknc * s1, Slinknc * s2) const
    {
      Arc * arc1 = (Arc*) s1;
      Arc * arc2 = (Arc*) s2;
      return cmp(arc1, arc2);
    }
  };

public:

      template <class Compare> 
  void sort_arcs(Compare & cmp)
  {
    Cmp_Arc<Compare> comp(cmp);
    mergesort<HTList, Cmp_Arc<Compare>>(arc_list, comp);
  }

        template <class Compare> 
  void sort_arcs(Compare && cmp = Compare())
  {
    mergesort<Cmp_Arc<Compare>>(arc_list, Cmp_Arc<Compare>(cmp)); 
  }

  GRAPH_FUNCTIONAL_METHODS(List_SGraph);
};

/** 
  @brief Class digraph (directed graph) implemented with simple lists
  adjacency.

  @details This class models a directed graph. It is functionally
  equivalent to the class \c List_SGraph, with the exception of it
  handles directed graphs.

  @param __Graph_Snode Node type. You must be defined
  it from the class \c __Graph_Snode, either by inclusion of
  attributes by derivation or combination of both
  @param __Graph_Arc Type of arc. You must be defined
  it from the class \c __Graph_Arc, either by inclusion of
  attributes by derivation or combination of both

  @see Graph_Snode Graph_Sarc

  @ingroup Grafos

  @see List_SGraph
*/
  template <typename __Graph_Node = Graph_Snode<int>,
	    typename __Graph_Arc  = Graph_Sarc<int>>
class List_SDigraph : public List_SGraph<__Graph_Node, __Graph_Arc>
{
  typedef List_SGraph<__Graph_Node, __Graph_Arc> GT;

public:

  typedef __Graph_Node Node;

  typedef __Graph_Arc Arc;

  List_SDigraph() 
  {
    this->digraph = true; 
  }
  
  List_SDigraph(const List_SDigraph & dg) : GT()
  {
    this->digraph = true; 
    copy_graph(*this, dg, false);
  }

  List_SDigraph & operator = (const List_SDigraph<Node, Arc> & g)
  {
    if (this == &g) 
      return *this;

    this->digraph = true;
    copy_graph(*this, g, false);

    return *this;
  }

  List_SDigraph(List_SDigraph && dg) : GT()
  {
    this->digraph = true;
    this->swap(dg);
  }

  List_SDigraph & operator = (List_SDigraph<Node, Arc> && g)
  {
    this->digraph = true; 
    this->swap(g);

    return *this;
  }
};


GRAPH_METHODS_IMPLS(List_SGraph);


} /* End namespace Aleph */

# endif /* TPL_SGRAPH_H */
