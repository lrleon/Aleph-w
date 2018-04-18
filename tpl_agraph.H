/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef TPL_AGRAPH_H
# define TPL_AGRAPH_H

# include <tpl_dynSetTree.H>
# include <array_it.H>
# include <tpl_sgraph.H>

namespace Aleph {

using namespace Aleph;

  /** Node of `Array_Graph`

      @ingroup Grafos
   */
    template <typename Node_Info = Empty_Class> 
class Graph_Anode 
      : public Dlink,
	public GTNodeCommon<Node_Info>
{
  using Base = GTNodeCommon<Node_Info>;
  friend class GTNodeCommon<Node_Info>;

  static const size_t Contract_Factor = 4;
  static const size_t Default_Cap = 4;

  void init(size_t dim)
  {
    arcs_dim = dim;
    this->num_arcs = 0;
    contract_threshold = arcs_dim/Contract_Factor;
    arc_array = nullptr;
    if (arcs_dim == 0)
      return;

    arc_array = (void**) malloc(arcs_dim*sizeof(void*));
    if (arc_array == nullptr)
      throw std::bad_alloc();
  }
  
public:

  void ** arc_array;
  size_t arcs_dim;
  size_t contract_threshold;
    
  Graph_Anode()
  {
    init(0);    
  }

  Graph_Anode(const Node_Info & info) : Base(info)
  {
    init(Default_Cap);
  }

  Graph_Anode(Node_Info && info) : Base(move(info))
  {
    init(Default_Cap);
  }

  Graph_Anode(const Graph_Anode & node) : Base(node.node_info)
  {
    init(0);
  }

  Graph_Anode & operator = (const Graph_Anode & node)
  noexcept(std::is_nothrow_copy_assignable<Node_Info>::value)
  {
    if (&node == this)
      return *this;
    this->node_info = node.node_info;
    return *this;
  }

  Graph_Anode(Graph_Anode * p) : Base(p->get_info())
  { 
    init(0);
  }
 
  virtual ~Graph_Anode()
  {
    if (arc_array != nullptr)
      free(arc_array);
  }

  void allocate_more(size_t new_size)
  {
    if (new_size == 0)
      new_size = 1;

    void ** new_array = (void**) realloc(arc_array, new_size*sizeof(void*));
    if (new_array == nullptr)
      throw std::bad_alloc();
    
    arc_array = new_array;
    arcs_dim = new_size;
    contract_threshold = arcs_dim/Contract_Factor;
  }

  void * insert_arc(void * arc) 
  {
    if (this->num_arcs == arcs_dim)
      allocate_more(arcs_dim << 1); // 2*arcs_dim
    
    arc_array[this->num_arcs++] = arc;

    return arc;
  }

  void remove_arc(void * arc) 
  {
    bool removed = false;
    for (size_t i = 0; i < this->num_arcs; ++i)
      if (arc_array[i] == arc)
	{
	  arc_array[i] = arc_array[--(this->num_arcs)];
	  removed = true;
	  break;
	}

    if (not removed)
      throw std::domain_error("arc for deleting not found");

    if (this->num_arcs > contract_threshold)
      return;
	
        // contraction 
    size_t new_sz = arcs_dim >> 1; // num_arcs/2
    arc_array = (void**) realloc(arc_array, new_sz*sizeof(void*));
    arcs_dim = new_sz;
    contract_threshold = arcs_dim/Contract_Factor;
  }

  bool compress() noexcept
  {
    void ** new_array = (void**) realloc(arc_array, 
					 this->num_arcs*sizeof(void*));
    if (new_array == nullptr)
      return false;
    
    arc_array = new_array;
    arcs_dim = this->num_arcs;
    contract_threshold = this->num_arcs/Contract_Factor;

    return true;
  }
};


    template <typename Arc_Info = Empty_Class> 
class Graph_Aarc 
      : public Dlink,
	public GTArcCommon<Arc_Info>
{
  friend class GTArcCommon<Arc_Info>;
  using Base = GTArcCommon<Arc_Info>;

public:

  Graph_Aarc(const Arc_Info & info) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value) : Base(info)
  { 
    /* empty */ 
  }

  Graph_Aarc(Arc_Info && info = Arc_Info()) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
  : Base(move(info))
  { 
    /* empty */ 
  }

  Graph_Aarc(const Graph_Aarc & arc) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
    : Graph_Aarc(arc.arc_info) { /* empty */ }

  Graph_Aarc & operator = (const Graph_Aarc & arc)
  noexcept(std::is_nothrow_copy_assignable<Arc_Info>::value)
  {
    if (&arc == this)
      return *this;
    this->arc_info = arc.arc_info;
    return *this;
  }

  Graph_Aarc(void * src, void * tgt, const Arc_Info & data) 
    noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value) 
  : Base(src, tgt, data)
  { 
    // empty
  }

  Graph_Aarc(void * src, void * tgt, Arc_Info && data = Arc_Info()) 
  noexcept(std::is_nothrow_copy_constructible<Arc_Info>::value)
  : Base(src, tgt, move(data))
  { 
    // empty
  }
};


  template <class __Graph_Node = Graph_Anode<unsigned long>, 
	    class __Graph_Arc  = Graph_Aarc<unsigned long>>
class Array_Graph
	      :  public GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
				    __Graph_Node, __Graph_Arc>
{
public:

  using Node = __Graph_Node;
  using Arc = __Graph_Arc;
  using Node_Type = typename Node::Node_Type;
  using Arc_Type = typename Arc::Arc_Type;

  friend class GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
			   __Graph_Node, __Graph_Arc>;

  using CommonBase = GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
				 __Graph_Node, __Graph_Arc>;

  using CommonBase::insert_node;
  using CommonBase::insert_arc;

private:

  // using DynSetNode = DynSetTree<Node*, Avl_Tree>;
  using DynSetArc = DynSetTree<Arc*, Rand_Tree>;

  Dlink node_set;
  Dlink arc_set;

public:

  struct Node_Iterator : public GTNodeIterator<Array_Graph>
  {
    using Base = GTNodeIterator<Array_Graph>;
    using Base::Base;
    
    Node_Iterator(const Array_Graph & g) noexcept
      : Base(const_cast<Dlink&>(g.node_set))
    {
      // empty
    }
  };

   /** Iterador sobre arcos de un grafo.

          Este iterador recorre cada arco del grafo. El orden de visita
          puede ser alterado si previamente se ordenan los arcos.

          @see sort_arcs()
       */
  struct Arc_Iterator : public GTArcIterator<Array_Graph>
  {
    using Base = GTArcIterator<Array_Graph>;
    using Base::Base;

    Arc_Iterator(const Array_Graph & g) 
      : Base(const_cast<Dlink&>(g.arc_set))
    {
      // empty
    }
  };
  // class Arc_Iterator : public DynSetArc::Iterator
  // {
  // public:

  //       /// Tipo de elemento que retorna get_current()
  //   using Item_Type = Arc *;

  //      /// El tipo de conjunto sobre el cual se itera.
  //   using Set_Type = Array_Graph;

  //   Arc_Iterator() noexcept { /* empty */ }

  //   Arc_Iterator(const Array_Graph & _g) noexcept
  //     : DynSetArc::Iterator(_g.arc_set)
  //   {
  //     // empty
  //   }

  //       /// Retorna un puntero al arco actual
  //   Arc * get_current_arc() const
  //   {
  //     return this->get_curr();
  //   }

  //       /// Retorna el nodo origen del arco actual (sólo cuenta si es digrafo) 
  //   Node * get_src_node() noexcept 
  //   {
  //     return (Node*) get_current_arc()->src_node; 
  //   }

  //       /// Retorna el nodo destino del arco actual (sólo cuenta si es digrafo) 
  //   Node * get_tgt_node() noexcept
  //   {
  //     return (Node*) get_current_arc()->tgt_node; 
  //   }
  // };
  
  class Node_Arc_Iterator : public Array_Iterator<void*>
  {
    Node * src_node = nullptr;

  public:

        /// El tipo de dato que retorna get_current().
    using Item_Type = Arc *;

        /// El tipo de conjunto sobre el cual se itera.
    using Set_Type = Node *;

       /// Instancia un iterador vacío (inválido).
    Node_Arc_Iterator() noexcept { /* empty */ }

        /// Instancia un iterador sobre el nodo _src_node
    Node_Arc_Iterator(Node * src) noexcept
      : Array_Iterator<void*>(src->arc_array, src->arcs_dim, src->num_arcs), 
	src_node(src)
    {
      // empty 
    }

        /// Retorna el arco actual.
    Arc * get_current_arc() const
    {
      return (Arc*) const_cast<Node_Arc_Iterator*>(this)->
	Array_Iterator<void*>::get_curr();
    }

        ///\overload get_current_arc()
    Arc * get_current() const { return get_current_arc(); }

        ///\overload get_current_arc()
    Arc * get_curr() const { return get_current_arc(); }

        /// Retorna el nodo destino del arco actual.
    Node * get_tgt_node() const
    {
      Arc * a = get_curr();
      return (Node*) a->get_connected_node(src_node);
    }
  };

  virtual Node * insert_node(Node * p)
  {
    assert(p != nullptr);
    assert(p->num_arcs == 0);

    node_set.append(p);
    this->num_nodes++;

    return p;
  }

  void compress()
  {
    for (Node_Iterator it(*this); it.has_curr(); it.next())
      it.get_curr()->compress();
  }

private:

  Arc * try_insert_arc(Node * src, Node * tgt, void * a)
  {
    Arc * aptr = (Arc*) a;

    aptr->src_node = src;
    aptr->tgt_node = tgt;
    src->insert_arc(aptr);

    if (not this->digraph and src != tgt)
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
	this->num_arcs++;
      }
    catch (std::bad_alloc)
      {
	src->remove_arc(aptr);
	if (not this->digraph and src != tgt)
	  tgt->remove_arc(aptr);
	throw;
      }
    
    return aptr;
  }

public:

  Arc * connect_arc(Arc * arc)
  {
    return 
    try_insert_arc(this->get_src_node(arc), this->get_tgt_node(arc), arc);
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
    if (not this->digraph and src != tgt)
      tgt->remove_arc(arc);

    arc->del(); // delete it from arc_set
    this->num_arcs--;

    return arc;
  }

  virtual void remove_arc(Arc * a)
  {
    delete disconnect_arc(a);
  }  

  virtual void remove_node(Node * p) 
  {
    DynList<Arc*> arcs; // store arcs to delete
    if (this->digraph)
      // traverse all arcs of graph and store those whose source or target is p
      for (Arc_Iterator it(*this); it.has_curr(); it.next())
        {
          Arc * arc = it.get_curr();
          if (this->get_src_node(arc) == p or this->get_tgt_node(arc) == p)
            arcs.append(arc);
        }
    else
      // traverse arc of node
      for (size_t i = 0, n = p->num_arcs; i < n; ++i)
        {
	  Arc * arc = (Arc*) p->arc_array[i];
	  arcs.append(arc);
        }
    
    arcs.for_each([this] (auto arc) { this->remove_arc(arc); });

    p->del(); // remove from node_set
    this->num_nodes--;

    delete p;
  }

  Node * get_first_node() const
  {
    return (Node*) const_cast<Dlink&>(node_set).get_first(); 
  }

  Arc * get_first_arc() const 
  {
    return (Arc*) const_cast<Dlink&>(arc_set).get_first(); 
  }

  Arc * get_first_arc(Node * p) const
  {
    if (get_num_arcs(p) == 0)
      throw std::range_error("Node has not arcs");
    return (Arc*) p->arc_array[0];
  }

  Array_Graph() 
  { 
    assert(this->num_nodes == 0 and this->num_arcs == 0 and 
	   node_set.is_empty() and arc_set.is_empty()); 
  }

private:

  void clear() noexcept
  {
    while (not arc_set.is_empty())
      delete static_cast<Arc*>(arc_set.remove_first());

    while (not node_set.is_empty())
      delete static_cast<Node*>(node_set.remove_first());

    this->num_nodes = this->num_arcs = 0;
  }

public:

  virtual ~Array_Graph() noexcept
  {
    clear();
  }

  Array_Graph(const Array_Graph & g) 
  {
    assert(this->num_nodes == 0 and this->num_arcs == 0);
    copy_graph(*this, g, false);
  }

  void swap(Array_Graph & g) noexcept
  {
    this->common_swap(g);
    node_set.swap(g.node_set);
    arc_set.swap(g.arc_set);
  }

  Array_Graph(Array_Graph && g) noexcept
  {
    swap(g);
  }

  Array_Graph & operator = (const Array_Graph & g)
  {
    if (&g == this)
      return *this;

    copy_graph(*this, g, false);
    
    return *this;
  }

  Array_Graph & operator = (Array_Graph && g) noexcept
  {
    swap(g);
    return *this;
  }

private:

      template <class Cmp>
  struct Cmp_Arc
  {
    Cmp & cmp;

    Cmp_Arc(Cmp && __cmp = Cmp()) 
    noexcept(std::is_nothrow_constructible<Cmp>::value and
	     std::is_nothrow_move_assignable<Cmp>::value)
      : cmp(__cmp) { /* empty */ }

    Cmp_Arc(Cmp & __cmp) noexcept(std::is_nothrow_copy_assignable<Cmp>::value)
      : cmp(__cmp) { /* empty */ }

    bool operator () (Arc * a1, Arc * a2) const noexcept
    {
      return cmp(a1, a2);
    }
  };

public:

  /** Sort all the nodes of the graph according to a specific
      criteria.

      The method takes an comparison operation between two nodes and
      sorts the nodes. 

      The comparison criteria should match the following signature:

      bool cmp(Node * a1, Node * a2);

      So, inside `cmp` you would access to the data arcs and would
      perform the comparison.

      After execution of this method, provided that no new nodes are
      inserted or others sorts are not performed with a different
      criteria, **it is guaranted that order of apparition of the
      nodes by the `Node_Iterator` will be sorted**.

      \param[inout] cmp operation of comparison that defines the
      sorting criteria
  */
  template <class Compare> inline
  void sort_nodes(Compare & cmp) noexcept
  {
    Cmp_Dlink_Node<Array_Graph, Compare> c = cmp;
    mergesort(node_set, c);
  }

  /// \overload sort_arcs(Compare & cmp)
  template <class Compare> inline 
  void sort_nodes(Compare && cmp = Compare()) noexcept
  {
    sort_nodes(cmp);
  }

      /** Sort all the arcs of the graph according to a specific
	  criteria.

	  The method takes an comparison operation between two arcs and
	  sorts the arcs. 

	  The comparison criteria should match the following signature:

	      bool cmp(Arc * a1, Arc * a2);

	  So, inside `cmp` you would access to the data arcs and would
	  perform the comparison.

	  After execution of this method, provided that no new arcs are
	  inserted or others sorts are not performed with a different
	  criteria, **it is guaranted that order of apparition of the
	  arc by the `Arc_Iterator` will be sorted**.

	  \param[inout] cmp operation of comparison that defines the
	  sorting criteria
       */
      template <class Compare> 
  void sort_arcs(Compare & cmp)
  {
    Cmp_Dlink_Arc<Array_Graph, Compare> c = cmp;
    mergesort(arc_set, c);
  }

   /// \overload sort_arcs(Compare & cmp)
      template <class Compare> 
  void sort_arcs(Compare && cmp) { sort_arcs(cmp); }
};

    /** Clase digrafo (grafo dirigido) implementado con listas simples
	de adyacencia.  

        Esta clase modeliza un grafo dirigido. Funcionalmente es
        equivalente a la clase \c List_SGraph, a la excepción de que ésta
        maneja grafos dirigidos.
       
        @param __Graph_Snode El tipo de nodo. Debe estar definido a
        partir de la clase \c __Graph_Snode, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @param __Graph_Arc El tipo de arco. Debe estar definido a
        partir de la clase \c __Graph_Arc, bien sea por inclusión de
        atributos, por derivación o por combinación de ambos
        @see Graph_Snode Graph_Sarc
        @ingroup Grafos
        @see List_SGraph
     */
template <typename __Graph_Node = Graph_Anode<int>, 
	  typename __Graph_Arc  = Graph_Aarc<int>>
class Array_Digraph : public Array_Graph<__Graph_Node, __Graph_Arc>
{
  using GT = Array_Graph<__Graph_Node, __Graph_Arc>;

public:

  using Node = __Graph_Node;

  using Arc = __Graph_Arc;

  Array_Digraph() noexcept
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

  Array_Digraph(Array_Digraph && dg) noexcept : GT()
  {
    this->digraph = true; 
    this->swap(dg);
  }

  Array_Digraph & operator = (Array_Digraph<Node, Arc> && g) noexcept
  {
    this->swap(g);

    return *this;
  }
};


} // namespace Aleph {

# endif //  TPL_AGRAPH_H
