
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
  @file tpl_agraph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_AGRAPH_H
# define TPL_AGRAPH_H

# include <tpl_dynSetTree.H>
# include <array_it.H>
# include <tpl_sgraph.H>

namespace Aleph {

using namespace Aleph;

    /* TODO Documentar para que se usa esta clase. */
    template <typename Node_Info = Empty_Class> 
class Graph_Anode
{
  static const size_t Contract_Factor = 4;
  static const size_t Default_Cap = 4;

  void init(size_t dim)
  {
    arcs_dim = dim;
    num_arcs = 0;
    contract_threshold = arcs_dim/Contract_Factor;
    arc_array = NULL;
    if (arcs_dim == 0)
      return;

    arc_array = (void**) malloc(arcs_dim*sizeof(void*));
    if (arc_array == NULL)
      throw std::bad_alloc();
  }
  
public:

  GRAPH_NODE_COMMON(Graph_Anode);

  void ** arc_array;
  size_t arcs_dim;
  size_t contract_threshold;
    
  Graph_Anode() 
  {
    init(0);    
  }

  Graph_Anode(const Node_Info & info) : node_info(info)
  {
    init(Default_Cap);
  }

  Graph_Anode(const Graph_Anode & node) : node_info(node.node_info)
  {
    init(0);
  }

  Graph_Anode & operator = (const Graph_Anode & node)
    {
      if (&node == this)
	return *this;
      node_info = node.node_info;
      return *this;
    }

  Graph_Anode(Graph_Anode * p) : node_info(p->get_info())
  { 
    init(0);
  }
 
  ~Graph_Anode()
  {
    if (arc_array != NULL)
      free(arc_array);
  }

  void allocate_more(size_t new_size)
  {
    if (new_size == 0)
      new_size = 1;

    void ** new_array = (void**) realloc(arc_array, new_size*sizeof(void*));
    if (new_array == NULL)
      throw std::bad_alloc();
    
    arc_array = new_array;
    arcs_dim = new_size;
    contract_threshold = arcs_dim/Contract_Factor;
  }

  void * insert_arc(void * arc)
  {
    if (num_arcs == arcs_dim)
      allocate_more(arcs_dim << 1); /* 2*arcs_dim */
    
    arc_array[num_arcs++] = arc;

    return arc;
  }

  void remove_arc(void * arc)
  {
    bool removed = false;
    for (size_t i = 0; i < num_arcs; ++i)
      if (arc_array[i] == arc)
	{
	  arc_array[i] = arc_array[--num_arcs];
	  removed = true;
	  break;
	}

    if (not removed)
      throw std::domain_error("arc for deleting not found");

    if (num_arcs > contract_threshold)
      return;
	
    /* Contraction */
    size_t new_sz = arcs_dim >> 1; /* num_arcs/2 */
    arc_array = (void**) realloc(arc_array, new_sz*sizeof(void*));
    arcs_dim = new_sz;
    contract_threshold = arcs_dim/Contract_Factor;
  }

  bool compress()
  {
    void ** new_array = (void**) realloc(arc_array, num_arcs*sizeof(void*));
    if (new_array == NULL)
      return false;
    
    arc_array = new_array;
    arcs_dim = num_arcs;
    contract_threshold = num_arcs/Contract_Factor;

    return true;
  }
};


    template <typename Arc_Info = Empty_Class> 
class Graph_Aarc 
{
public:

  GRAPH_ARC_COMMON(Graph_Aarc);

  Graph_Aarc() : src_node(NULL), tgt_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Aarc(const Arc_Info & info) 
    : arc_info(info), src_node(NULL), tgt_node(NULL)
  { 
    /* Empty */ 
  }

  Graph_Aarc(const Graph_Aarc & arc) : Graph_Aarc(arc.arc_info) { /* Empty */ }

  Graph_Aarc & operator = (const Graph_Aarc & arc)
  {
    if (&arc == this)
      return *this;
    arc_info = arc.arc_info;
    return *this;
  }

  Graph_Aarc(void * src, void * tgt, const Arc_Info & data) 
    : arc_info(data), src_node(src), tgt_node(tgt)
  { 
    /* Empty */
  }

  Graph_Aarc(void * src, void * tgt) : src_node(src), tgt_node(tgt)
  { 
    /* Empty */
  }
};


  template <class __Graph_Node = Graph_Anode<unsigned long>, 
	    class __Graph_Arc  = Graph_Aarc<unsigned long>>
class Array_Graph
{
  GRAPH_ATTR_COMMON(Array_Graph);

private:

  typedef DynSetTree<Node*, Rand_Tree> DynSetNode;
  typedef DynSetTree<Arc*, Rand_Tree> DynSetArc;

  DynSetNode node_set;
  DynSetArc  arc_set;

public:

  class Node_Iterator : public DynSetNode::Iterator
  {
  public:

/** @brief Item type that returning get_current() */
    typedef Node * Item_Type;

/** @brief The set type on which iterates. */
    typedef Array_Graph Set_Type;

    Node_Iterator() { /* Empty */ }

    Node_Iterator(const Array_Graph & g) 
      : DynSetNode::Iterator(g.node_set)
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
  @brief Iterator over arcs of a graph.

  @details This iterator traverses each arc of the graph.
  The order of visits may be altered if the arches are arranged previously.

  @see sort_arcs()
*/
  class Arc_Iterator : public DynSetArc::Iterator
  {
  public:

/** @brief Item type that returning get_current() */
    typedef Arc * Item_Type;

/** @brief The set type on which iterates. */
    typedef Array_Graph Set_Type;

    Arc_Iterator() { /* Empty */ }

    Arc_Iterator(const Array_Graph & _g) : DynSetArc::Iterator(_g.arc_set)
    {
      /* Empty */
    }

/** @brief Returns a pointer to the current arc. */
    Arc * get_current_arc() const
    {
      return this->get_curr();
    }

/** @brief Returns the source node of the current arc (only counts if digraph) */
    Node * get_src_node() { return (Node*) get_current_arc()->src_node; }

/** @brief Returns the destination node of the arc current (only counts if digraph) */
    Node * get_tgt_node() { return (Node*) get_current_arc()->tgt_node; }
  };

  
  class Node_Arc_Iterator : public Array_Iterator<void*>
  {
    Node * src_node;

  public:

/** @brief The data type that returns get_current() */
    typedef Arc * Item_Type;

/** @brief The set type on which iterates. */
    typedef Node * Set_Type;

/** @brief Instance an empty iterator (invalid). */
    Node_Arc_Iterator() : src_node(NULL) { /* Empty */ }

/** @brief An iterator instance on node _src_node */
    Node_Arc_Iterator(Node * src) 
      : Array_Iterator<void*>(src->arc_array, src->num_arcs), src_node(src)
    {
      /* Empty */ 
    }

/** @brief Returns the current arc. */
    Arc * get_current_arc() const
    {
      return (Arc*) const_cast<Node_Arc_Iterator*>(this)->
	Array_Iterator<void*>::get_curr();
    }

/** @overload get_current_arc() */
    Arc * get_current() const { return get_current_arc(); }

/** @overload get_current_arc() */
    Arc * get_curr() const { return get_current_arc(); }

/** @brief Returns the destination node of the current arc. */
    Node * get_tgt_node() const
    {
      Arc * a = get_curr();
      return (Node*) a->get_connected_node(src_node);
    }
  };

  GRAPH_ITERATIVE_METHODS;

  GRAPH_SEARCH_METHODS; 

  virtual Node * insert_node(Node * p)
  {
    assert(p->num_arcs == 0);

    Node ** ret_val = node_set.append(p);

    ++num_nodes;

    assert(num_nodes = node_set.size());
    assert(ret_val != NULL);

    return *ret_val;
  }

  void compress()
  {
    node_set.for_each([/* Lambda */] (Node * p) { p->compress(); });
  }

private:

  Arc * try_insert_arc(Node * src, Node * tgt, void * a)
  {
    Arc * aptr = (Arc*) a;

    aptr->src_node = src;
    aptr->tgt_node = tgt;
    src->insert_arc(aptr);

    if (not digraph and src != tgt)
      {
	try
	  {
	    tgt->insert_arc(aptr);
	  }
	catch (std::bad_alloc)
	  {
	    src->remove_arc(aptr);
	    throw;
	  }
      }
    
    try
      {
	arc_set.append(aptr);
	++num_arcs;
	assert(num_arcs == arc_set.size());
      }
    catch (std::bad_alloc)
      {
	src->remove_arc(aptr);
	if (not digraph and src != tgt)
	  tgt->remove_arc(aptr);
	throw;
      }
    
    return aptr;
  }

public:

  Arc * connect_arc(Arc * arc)
  {
    return try_insert_arc(get_src_node(arc), get_tgt_node(arc), arc);
  }

private:

  Arc * insert_arc(Node * src, Node * tgt, void * a)
  {
    bool compress_done = false;
    
  retry:
    try
      {
	return try_insert_arc(src, tgt, a);
      }
    catch (bad_alloc)
      {
	if (compress_done)
	  throw;
	
	compress();
	compress_done = true;
	goto retry;
      }
  }

public:

  Arc * disconnect_arc(Arc * arc)
  {
    Node * src = (Node*) arc->src_node;
    Node * tgt = (Node*) arc->tgt_node;

    src->remove_arc(arc);
    if (not digraph and src != tgt)
      tgt->remove_arc(arc);

    arc_set.remove(arc);
    --num_arcs;
    assert(num_arcs == arc_set.size());

    return arc;
  }

  GRAPH_INSERTION_METHODS; 

/** @brief Remove Arcos */
  virtual void remove_arc(Arc * a)
  {
    delete disconnect_arc(a);
  }  

/** @brief Remove Nodes */
  virtual void remove_node(Node * p)
  {
    
/** @brief Record the arches to be removed */
    DynArray<Arc*> arcs; 
    if (digraph)
      for (size_t i = 0; i < num_arcs; ++i)
        {
          Arc * arc = arc_set(i);

          if (get_src_node(arc) == p or get_tgt_node(arc) == p)
            arcs.append(arc);
        }
    else
      for (size_t i = 0, n = p->num_arcs; i < n; ++i)
        {
	  Arc * arc = (Arc*) p->arc_array[i];
	  arcs.append(arc);
        }
    
    for (size_t i = 0; i < arcs.size(); ++i)
      remove_arc(arcs(i));

    node_set.remove(p);
    --num_nodes;

    assert(num_nodes == node_set.size());
    
    delete p;
  }

  Node * get_first_node() const
  {
    return const_cast<DynSetNode&>(node_set).get_first(); 
  }

  Arc * get_first_arc() const 
  {
    return const_cast<DynSetArc&>(arc_set).get_first(); 
  }

  Arc * get_first_arc(Node * p) const
  {
    if (get_num_arcs(p) == 0)
      throw std::range_error("Node has not arcs");
    return (Arc*) p->arc_array[0];
  }

  Array_Graph()
  {
    init();
  }

private:

  void clear()
  {
    arc_set.for_each( [/* Lambda */] (const Arc * p) { delete p; } );
    node_set.for_each( [/* Lambda */] (const Node * p) { delete p; } );
  }

public:

  ~Array_Graph()
  {
    clear();
  }

  Array_Graph(const Array_Graph & g)
  {
    init();
    copy_graph(*this, g, false);
  }

  void swap(Array_Graph & g)
  {
    common_swap(g);
    node_set.swap(g.node_set);
    arc_set.swap(g.arc_set);
  }

  Array_Graph(Array_Graph && g)
  {
    init();
    swap(g);
  }

  Array_Graph & operator = (const Array_Graph & g)
  {
    if (&g == this)
      return *this;

    copy_graph(*this, g, false);
    
    return *this;
  }

  Array_Graph & operator = (Array_Graph && g)
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

    bool operator () (Arc * a1, Arc * a2) const
    {
      return cmp(a1, a2);
    }
  };

public:

      template <class Compare> 
  void sort_arcs(Compare &)
  {
    throw std::domain_error("sortarcs is not defined for Array_Graph");
  }

      template <class Compare> 
  void sort_arcs(Compare &&)
  {
    throw std::domain_error("sortarcs is not defined for Array_Graph");
  }

  GRAPH_FUNCTIONAL_METHODS(Array_Graph);
};

/** 
  @brief Digraph class (directed graph) implemented with simple lists
  of adjacency.

  @details This class models a directed graph. It is functionally
  equivalent to the class \c List_SGraph, with the exception of it
  handles directed graphs.

  @param __Graph_Snode Node type.
  Must be defined from the class \c __Graph_Snode,
  either by inclusion of attributes,
  by derivation or combination of both
  
  @param __Graph_Arc Arc Type.
  Must be defined from the class \c __Graph_Arc,
  either by inclusion of attributes,
  by derivation or combination of both
  
  @see Graph_Snode Graph_Sarc
  @see List_SGraph

  @ingroup Grafos
*/
template <typename __Graph_Node = Graph_Anode<int>, 
	  typename __Graph_Arc  = Graph_Aarc<int>>
class Array_Digraph : public Array_Graph<__Graph_Node, __Graph_Arc>
{
  typedef Array_Graph<__Graph_Node, __Graph_Arc> GT;

public:

  typedef __Graph_Node Node;

  typedef __Graph_Arc Arc;

  Array_Digraph() 
  {
    this->digraph = true; 
  }
  
  Array_Digraph(const Array_Digraph & dg) 
    : Array_Graph<Node, Arc>((Array_Digraph&) dg)
  {
    this->digraph = true; 
  }

  Array_Digraph & operator = (const Array_Digraph<Node, Arc> & g)
  {
    if (this == &g) 
      return *this;

    copy_graph(*this, g, false);

    return *this;
  }

  Array_Digraph(Array_Digraph && dg) : GT()
  {
    this->digraph = true; 
    this->swap(dg);
  }

  Array_Digraph & operator = (Array_Digraph<Node, Arc> && g)
  {
    this->swap(g);

    return *this;
  }
};


GRAPH_METHODS_IMPLS(Array_Graph);


} /* End namespace Aleph */

# endif //  TPL_AGRAPH_H
