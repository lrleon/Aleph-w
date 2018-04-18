
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
  @file topological_sort.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TOPOLOGICAL_SORT_H
# define TOPOLOGICAL_SORT_H

# include <tpl_dynListQueue.H>
# include <tpl_graph.H>

namespace Aleph {


/** 
  @brief Calculate a topological order for a digraph in suffix mode. 

  @details Topological_Sort recursively runs deep and
  suffix nodes in a graph and performs a topological order
  the result of which it stores a given in the parameter list.

  @ingroup Grafos
*/
    template <class GT, 
	      template <typename, class> class Itor = Node_Arc_Iterator,
	      class SA = Dft_Show_Arc<GT> > 
class Topological_Sort
{
  SA & sa;
  const GT * gptr;

public:

  Topological_Sort(SA && __sa = SA()) : sa(__sa), gptr(NULL) { /* Empty */ }

  Topological_Sort(SA & __sa) : sa(__sa), gptr(NULL)  { /* Empty */ }

private:
 
      template <template <class> class List>
  void topological_sort(typename GT::Node * curr, 
			List<typename GT::Node*> & list)
  {
    assert(gptr != NULL);

    if (IS_NODE_VISITED(curr, Depth_First))
      return;

    NODE_BITS(curr).set_bit(Depth_First, 1); /* Mark as visited*/

    const size_t & n = gptr->get_num_nodes();

    /** Visit recursively suffix nodes adjacent to curr */
    for (Itor<GT,SA> it(curr,sa); it.has_curr() and list.size() < n; it.next()) 
      topological_sort(it.get_tgt_node(), list);

    list.insert(curr); /* Inserting suffixed became sink node */
  }

public:

/** 
  @brief Returns a topological order by suffix.

  @param[in] g Acyclic digraph.
        
  @return A list of nodes corresponding to the ordering 
        
  @throw bad_alloc If there is insufficient memory to insert into list.
        
  @note The routine does not verify that g is indeed an acyclic digraph.  
*/
  template <template <class> class List>
  List<typename GT::Node*> perform (const GT & g)
  {
    g.reset_bit_nodes(Depth_First); /* Start memory bit nodes visiting */

    gptr = &g;
    List<typename GT::Node*> list;

    /* Browse all nodes */
    const size_t & n = gptr->get_num_nodes();
    for (typename GT::Node_Iterator it(g); it.has_curr() and list.size() < n; 
	 it.next())
    {
      typename GT::Node * curr = it.get_current_node();
      if (not IS_NODE_VISITED(curr, Depth_First))
        topological_sort(curr, list);
    }
    
    return list;
  }

/** 
  @overload () only for compatibility reasons backward
*/
  void operator () (const GT & g, DynDlist<typename GT::Node*> & list) 
  {
    perform<DynDlist>(g).swap(list);
  }
};

/** 
  @brief Calculate a topological order for the digraph by
  tour amplitude.

  @details Q_Topdological_Sort walking in amplitude
  nodes in a digraph g performs a topological sort
  it stores the result in a given list.

  @ingroup Grafos
*/
   template <class GT, 
	     template <typename, class> class Itor = Node_Arc_Iterator,
	     class SA = Dft_Show_Arc<GT> > 
class Q_Topological_Sort
{
  SA & sa;

public:

  Q_Topological_Sort(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Q_Topological_Sort(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Returns a topological order in amplitude.
      
  @param[in] g Acyclic digraph.
  
  @return A list of nodes corresponding to the topological order

  @throw bad_alloc If there is insufficient memory to insert into
  list or internal queue that is required to tour the digraph amplitude.
  @throw domain_error If g is not a digraph.

  @note The routine does not verify that g is indeed an acyclic digraph.
*/
      template <template <class> class List>
  List<typename GT::Node*> perform (const GT & g)
  {
    g.reset_counter_nodes();

    List<typename GT::Node*> list;

    /** Scroll all grades arches and have input */
    for (Arc_Iterator<GT,SA> it(g, sa); it.has_curr(); it.next()) 
      NODE_COUNTER(it.get_tgt_node())++; 

    
    DynListQueue<typename GT::Node*> q; /* Cola sources */
    /** Buscar nodos con grado de entrada 0 y meterlos en cola */
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	typename GT::Node * p = it.get_current_node();

    /** Is it a source node?
    Yes? ==> Place it in the tail */
	if (NODE_COUNTER(p) == 0) 
	  q.put(p);  
      }

    while (not q.is_empty()) 
      {
	typename GT::Node * p = q.get(); /* Take last source */

	assert(NODE_COUNTER(p) == 0);

	list.append(p); /* Insert it into the topological order */

    /** Decrease input level of each node adjacent to p */
	for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next())
	  {
	    typename GT::Node * tgt = it.get_tgt_node();

    /* tgt becomes source? */
	    if (--NODE_COUNTER(tgt) == 0) 
	      q.put(tgt);
	  }
      }

    return list;
  }

/** 
  @brief Return the topological sort amplitude ranges.
      
  @param[in] g Acyclic digraph.
  @param[out] ranks List for the topological sort ranges.
  Each range specifies a set of nodes that can look in parallel.
  
  @throw bad_alloc If there is insufficient memory to insert into
  list or internal queue that is required to tour the
  digraph amplitude.
  @throw domain_error If g is not a digraph.
  
  @note The routine does not verify that g is indeed an acyclic digraph. 
*/
  template <template <class> class RankList = DynDlist, 
	    template <class> class List = DynList>
  RankList<List<typename GT::Node*>> ranks(const GT & g)
  {
    g.reset_counter_nodes();

    /** Scroll all nodes to account indegree */
    for (typename GT::Node_Iterator i(g); i.has_curr(); i.next()) 
      for (Itor<GT, SA> j(i.get_current_node(), sa); j.has_curr(); j.next()) 
	NODE_COUNTER(j.get_tgt_node())++;

    /** Check input nodes with degree 0 and put them in line */
    DynListQueue<typename GT::Node*> q; /* Cola sources */
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
	typename GT::Node * p = it.get_current_node();

    /** Is it a source node?
    Yes? ==> Place it in the tail */
	if (NODE_COUNTER(p) == 0) 
	  q.put(p);
      }
    
    RankList<List<typename GT::Node*>> ranks; /* Return value */
    while (not q.is_empty()) 
      {
	List<typename GT::Node*> rank;
	DynListQueue<typename GT::Node*> aq;

    /** Get all nodes of level i */
	while (not q.is_empty()) 
	  {

    /* Pull Plug this latest source topological range*/
	    typename GT::Node * p = q.get(); 
	    rank.append(p); 

    /** Decrease input level of each node adjacent to p */
	    for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next())
	      {
		typename GT::Node * tgt = it.get_tgt_node();

    /** tgt becomes source?
    Yes? ==> Place it in auxiliary tail */
		if (--NODE_COUNTER(tgt) == 0) 
		  aq.put(tgt); 
	      }
	  }

	ranks.append(std::move(rank));
	q.swap(aq);
      }

    return ranks; 
  }

  void operator () (const GT & g, DynDlist<DynList<typename GT::Node *>> & list)
  {
    this->ranks<>(g).swap(list);
  }

  void operator () (const GT & g, DynList<DynList<typename GT::Node *>> & list)
  {
    this->ranks<DynList>(g).swap(list);
  }

/** 
  @overload () only for compatibility reasons backward
*/
  void operator () (const GT & g, DynDlist<typename GT::Node*> & list) 
  {
    this->perform<DynDlist>(g).swap(list);
  }
};

} /* End namespace Aleph */

# endif // TOPOLOGICAL_SORT_H
