
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
  @file tpl_concurrent_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_CONCURRENT_GRAPH_H
# define TPL_CONCURRENT_GRAPH_H

# include <pthread.h>

# include <autosprintf.h>

# include <gsl/gsl_rng.h>
# include <gsl/gsl_randist.h>

# include <tpl_graph.H>
# include <useMutex.H>

using namespace Aleph;


namespace Aleph 
{


 template <template <class, class> class GT, class __Node, class __Arc> 
class Concurrent_Graph;

/** 
  @brief Node of a graph concurrent.

  @details A node in a concurrent graph is very similar to an interface
  Graph_Node node. The essential difference is that it is designed to
  be accessed by multiple threads. Consequently, a Concurrent_Node
  it contains a binary semaphore intended to protect your data.

  @ingroup Grafos
*/
  template <class Base_Object> 
class Lock_Object : public Base_Object 
{
public:

  pthread_mutex_t * mutex; /* It protects all the data associated with the node */

  typedef typename Base_Object::Item_Type Object_Type;

  Lock_Object(const Lock_Object & c_obj) 
    : Base_Object(c_obj) , mutex(c_obj.mutex)
  {
    assert(not c_obj.delete_mutex);
  }
   	
  Lock_Object(pthread_mutex_t * __mutex = NULL) 
    : Base_Object(), mutex(__mutex)
  {
    /* Empty */
  }
   	
  Lock_Object(const Object_Type & obj_info, pthread_mutex_t * __mutex = NULL) 
    : Base_Object(obj_info), mutex(__mutex)
  {
    /* Empty */
  }
  
  Lock_Object(Base_Object * pobj, pthread_mutex_t * __mutex = NULL) 
    : Base_Object(pobj) , mutex(__mutex)
  {
    /* Empty */
  }

  void set_mutex( pthread_mutex_t * ptrm)
  {
    mutex = ptrm;
  }

  void set_mutex(pthread_mutex_t & m)
  {
    mutex = &m;
  }

/** 
  @brief Making critical section of the node; released into the destroyer.

  @ingroup Grafos
*/
  class Critical_Section : public UseMutex
  {
  public:

/** @brief Making critical section of the "node" node. It is released into destruction. */
    Critical_Section(Lock_Object * node) 
      : UseMutex(node->mutex) { /* Empty */ }
  };
};


/** 
  @brief Class implemented concurrent graph adjacency lists.

	@details Concurrent_Graph <Node, Arc> is a class that models graphs
  represented by adjacency lists in which all
  operations are reentrant and can be handled concurrently and
  coherently by several threads.

  Essentially, the interface is nearly identical to that of
  List_Graph. The fundamental difference is that the methods
  modify the topology of the graph or operating with it are
  protected by a global binary semaphore. these methods
  concerning the insertion and removal of nodes and arcs, as well
  as to your search.

  Methods of consultation and modification of nodes and arcs are not
  protected; not the iterator arcs of a node. Wear the
  light of a node or arc if required protection.

	The class manages two parameters fundamental type:
	- Concurrent_Node: Type of nodes that must be defined
    Concurrent_Node by class.
	- Concurrent_Arc: Type of nodes that must be defined
    Concurrent_Arc by class.

	These classes Must Have Been Previously defined.

  Once instantiated one Concurrent_Graph <Node, Arc>, the nodes and
  arches must be accessed via the built-in types:
	- Concurrent_Graph<Node, Arc>::Node.
	- Concurrent_Graph<Node, Arc>::Arc.
       
	@param Concurrent_Node Node type. You must be defined
  Concurrent_Node from the class, either by inclusion of
  attributes by derivation or combination of both
	@param Concurrent_Arc Type of bow. You must be defined
  Concurrent_Arc from the class, either by inclusion of
  attributes by derivation or combination of both
	
  @see Concurrent_Node Concurrent_Arc
  
  @see List_Digraph Path 
	
  @ingroup Grafos
	
  @todo Camino concurrente.
	
  @note The methods of this class are briefly
  documented, as its semantics are the same as for the class
  List_Graph. The exception is that the methods of Concurrent_Graph
  They are reentrant and secure access to various threads.
*/
    template <template <class, class> class __GT, class __Node, class __Arc> 
class Concurrent_Graph : public __GT<Lock_Object<__Node>, Lock_Object<__Arc> >
{
public:

  typedef __GT<Lock_Object<__Node>, Lock_Object<__Arc> > GT;

  typedef typename GT::Node Node;

  typedef typename GT::Arc Arc;

protected:

  pthread_mutex_t mutex;

  size_t num_mutexes;

  DynArray<pthread_mutex_t> mutexes;

  void init_mutexes()
  {
    mutexes.reserve(num_mutexes);
    for (int i = 0; i < num_mutexes; ++i)
      init_mutex(&mutexes.access(i));
  }

  void uninit_mutexes()
  {
    for (int i = 0; i < num_mutexes; ++i)
      destroy_mutex(&mutexes.access(i));
  }

public:

  pthread_mutex_t * get_mutex(int i)
  {
    if (i >= num_mutexes)
      throw std::out_of_range(gnu::autosprintf("mutex index %d out of range", i));

    return mutexes.access(i);
  }

  pthread_mutex_t * allocate_mutex()
  {
    init_mutex(&mutexes.append());
    
    return mutexes.access(num_mutexes++);
  }
   	
/** 
  @brief Type of concurrent node 
*/
  typedef typename GT::Node_Type Node_Type; 

/** 
  @brief Type of concurrent arc 
*/
  typedef typename GT::Arc_Type Arc_Type;

/** 
  @brief Instance a empty concurrent graph 
*/
  Concurrent_Graph(const size_t & n_mut = 1) : GT(), num_mutexes(n_mut)
  {
    init_mutex(mutex);  
    init_mutexes();   
  }

/** 
  @brief Instance a concurrent graph copy of another concurrent graph. 
*/
  Concurrent_Graph(const Concurrent_Graph & g) : GT(g), num_mutexes(g.num_mutexes)
  {
    init_mutex(mutex); 
    init_mutexes();   
  }

  void set_num_mutexes(const size_t & n)
  {
    CRITICAL_SECTION(mutex);

    if (n == 0)
      throw std::out_of_range("n is equal to zero");

    if (n < num_mutexes)
      throw std::out_of_range("n is smaller than current number of mutexes");

    mutexes.reserve(n);
    
    for (int i = num_mutexes; i < n; ++i)
      init_mutex(&mutexes.access(i));

    num_mutexes = n;
  }

  void distribute_mutexes_randomly()
  {
    gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
    gsl_rng_set(r, time(NULL) % gsl_rng_max(r));

    for (typename Concurrent_Graph::Node_Iterator it(*this); it.has_current();
	 it.next())
      {
	int i = gsl_rng_uniform_int(r, num_mutexes); /* Index random mutex */
	it.get_current()->set_mutex(mutexes.access(i));
      }

    for (typename Concurrent_Graph::Arc_Iterator it(*this); it.has_current();
	 it.next())
      {
	int i = gsl_rng_uniform_int(r, num_mutexes); /* Index random mutex */
	it.get_current()->set_mutex(mutexes.access(i));
      }

    gsl_rng_free(r);
  }

  void distribute_mutexes_uniformly()
  {
    {
      const size_t & num_nodes = this->get_num_nodes();
      const size_t num_nodes_by_mutex =  num_nodes / num_mutexes;

      typename GT::Node_Iterator it(*this); 
      for (int k = 0; k < num_nodes_by_mutex; ++k)
	{
	  pthread_mutex_t & m = mutexes.access(k);

	  for (int i = 0; it.has_current() and i < num_nodes; ++i, it.next())
	    it.get_current()->set_mutex(m);
	}
    }

    {
      const size_t & num_arcs = this->get_num_arcs();
      const size_t num_arcs_by_mutex = num_arcs / num_mutexes;

      typename GT::Arc_Iterator it(*this); 
      for (int k = 0; k < num_arcs_by_mutex; ++k)
	{
	  pthread_mutex_t & m = mutexes.access(k);

	  for (int i = 0; it.has_current() and i < num_arcs; ++i, it.next())
	    it.get_current()->set_mutex(m);
	}
    }
  }

  virtual ~Concurrent_Graph() 
  {
    uninit_mutexes();
    destroy_mutex(mutex);
  }

/** 
  @brief Iterator over a concurrent graph nodes.

  @ingroup Grafos
*/
  class Node_Iterator  
  {
    Concurrent_Graph & cg;

    typename GT::Node_Iterator base_iterator;

  public:

    typedef typename GT::Node_Iterator::Item_Type Item_Type;
    
    Node_Iterator() : base_iterator() 
    { 
      /* Empty */
    }

/** 
  @brief Instantiates an iterator over the graph _g. 
*/
    Node_Iterator(Concurrent_Graph & _g) : cg(_g), base_iterator(_g)
    {
      /* Empty */
    }

/** 
  @brief Instantiates a copy iterator of the iterator "it". 
*/
    Node_Iterator(const Node_Iterator & it) 
      : cg(it.cg), base_iterator(it.base_iterator)
    {
      /* Empty */
    }

/** 
  @brief Assigns iterator 
*/
    Node_Iterator & operator = (const Node_Iterator & it) 
    {
      CRITICAL_SECTION(cg.mutex);

      if (&it == this)
	return *this;

      base_iterator = it.base_iterator;
      
      return *this;
    }
    
/** 
  @brief Returns the current node. 
*/
    Node * get_current_node() 
    {
      CRITICAL_SECTION(cg.mutex);

      Node * node = base_iterator.get_current_node();
	
      return node;
    }

/** 
  @brief Returns the current node 
*/
    Node * get_current() { return get_current_node(); }  
	
/** 
  @brief Returns true if the iterator has current item. 
*/
    bool has_current() 
    {
      CRITICAL_SECTION(cg.mutex);
      return base_iterator.has_current();
    }
	
/** 
  @brief Advances the iterator by one position. 
*/
    void next() 
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.next();
    }

/** 
  @brief Backs the iterator by one position
*/
    void prev() 
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.prev();
    }

/** 
  @brief Resets the iterator to the first node
*/
    void reset_first()
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.reset_first();
    }

/** 
  @brief Resets the iterator to the last node
*/
    void reset_last()
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.reset_last();
    }
  };

/** 
  @brief Iterator over the arches of a concurrent graph.

  @ingroup Grafos
*/
  class Arc_Iterator 
  {
    Concurrent_Graph & cg;
	
    typename GT::Arc_Iterator base_iterator;

  public:

    typedef typename GT::Node_Iterator::Item_Type Item_Type;
    
/** 
  @brief Instance an empty iterator
*/
    Arc_Iterator() : base_iterator() 
    { 
      /* Empty */
    }

/** 
  @brief Instance an iterator over the arches of concurrent graph _g. 
*/
    Arc_Iterator(Concurrent_Graph & _g) : cg(_g), base_iterator(_g)
    {
      /* Empty */
    }

    Arc_Iterator(const Arc_Iterator & it) 
      : cg(it.cg), base_iterator(it.base_iterator)
    {
      /* Empty */
    }
	
/** 
  @brief Instantiates a copy iterator of the iterator "it". 
*/
    Arc_Iterator & operator = (const Arc_Iterator & it) 
    {
      CRITICAL_SECTION(cg.mutex);

      base_iterator = it.base_iterator;

      return *this;
    }

/** 
  @brief Gets the current arc iterator. 
*/
    Arc * get_current_arc() 
    { 
      CRITICAL_SECTION(cg.mutex);

      Arc * arc = base_iterator.get_current_arc();

      return arc;
    }

/** 
  @brief Gets the current arc iterator. 
*/
    Arc * get_current() { return get_current_arc(); }

/** 
  @brief Returns true if the iterator has current arc. 
*/
    bool has_current() 
    {
      CRITICAL_SECTION(cg.mutex);
      return base_iterator.has_current();
    }
	
/** 
  @brief Advances the iterator position. 
*/
    void next() 
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.next();
    }

/** 
  @brief Back the iterator in a position. 
*/
    void prev() 
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.prev();
    }

/** 
  @brief Resets the iterator to the first arc
*/
    void reset_first()
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.reset_first();
    }

/** 
  @brief Resets the iterator to the last arc
*/
    void reset_last()
    {
      CRITICAL_SECTION(cg.mutex);
      base_iterator.reset_last();
    }
  };

/** 
  @brief Returns the number of nodes that have concurrent graph. 
*/
  size_t get_num_nodes()  
  {
    CRITICAL_SECTION(mutex);
    return GT::get_num_nodes(); 
  }

    /* get_num_arcs made visible (Node *) inherited from List_Graph*/
  using List_Graph<Node, Arc>::get_num_arcs;
  	
/** 
  @brief Returns the number of arcs having concurrent graph. 
*/
  size_t get_num_arcs() 
  {
    CRITICAL_SECTION(mutex); 
    return GT::get_num_arcs(); 
  }

  size_t get_num_mutexes()
  {
    CRITICAL_SECTION(mutex); 
    return num_mutexes;
  }
 
/** 
  @brief Search a node by contained information. 
*/
  Node * search_node(const Node_Type & node_info)
  {
    CRITICAL_SECTION(mutex);
    return GT::search_node(node_info);
  } 

/** 
  @brief Inserts a node already created in a graph. 
*/
  Node * insert_node(Node * node)
  {
    CRITICAL_SECTION(mutex);
    return GT::insert_node(node);
  }
	
/** 
  @brief Creates and inserts a new node with information node_info. 
*/
  Node * insert_node(const Node_Type & node_info)
  {
    return insert_node( new Node(node_info) );
  }

/** 
  @brief Returns the first node of a graph; there is a certain order. 
*/
  Node * get_first_node() 
  {
    CRITICAL_SECTION(mutex);
    return GT::get_first_node();
  }
	
/** 
  @brief Returns the first arc of a graph. 
*/
  Arc * get_first_arc() 
  {
    CRITICAL_SECTION(mutex);
    return GT::get_first_arc(); 
  }
	
  void verify_graphs(Concurrent_Graph& g) 
  {
    CRITICAL_SECTION(mutex); 
    GT::verify_graphs(g);		
  }
	
/** 
  @brief Removes a node in a graph (with all its adjacent arcs). 
*/
  void remove_node(Node * node)
  {
    CRITICAL_SECTION(mutex);
    GT::remove_node(node);  		
  }
  
/** 
  @brief Order the arches of a concurrent graph the discretion Compare. 
*/
  template <class Compare> void sort_arcs()
  {
    CRITICAL_SECTION(mutex);
    GT::template sort_arcs<Compare> ();
  }

/** 
  @brief Create an arc in a concurrent graph. 
*/
  Arc * insert_arc(Node *           src_node, 
		   Node *           tgt_node, 
		   const Arc_Type & arc_info) 
  {
    CRITICAL_SECTION(mutex);
    return GT::insert_arc(src_node, tgt_node, arc_info);    	
  }

/** 
  @brief Create an arc in a concurrent graph. 
*/
  Arc * insert_arc(Node *           src_node, 
		   Node *           tgt_node) 
  {
    CRITICAL_SECTION(mutex);
    return GT::insert_arc(src_node, tgt_node);    	
  }

/** 
  @brief Removes an arc in a concurrent graph. 
*/
  void remove_arc(Arc * arc)
  {
    CRITICAL_SECTION(mutex);
    GT::remove_arc(arc);
  }
    
/** 
  @brief Search an arc that connects two nodes. 
*/
  Arc * search_arc(Node * src_node, Node * tgt_node) 
  {
    CRITICAL_SECTION(mutex);
    return GT::search_arc(src_node, tgt_node);    	
  }
    
/** 
  @brief Search an arc arc_info information. 
*/
  Arc * search_arc(const Arc_Type & arc_info)
  {
    CRITICAL_SECTION(mutex);
    return GT::search_arc(arc_info);    	
  }
  
/** 
  @brief Returns true if the arc arc belongs to the graph. 
*/
  bool arc_belong_to_graph(Arc * arc) 
  {
    CRITICAL_SECTION(mutex);
    return GT::arc_belong_to_graph(arc);
  }
};

}

# endif // TPL_CONCURRENT_GRAPH_H
