
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
  @file Tarjan.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TARJAN_H
# define TARJAN_H

# include <tpl_dynListStack.H>
# include <tpl_dynSetTree.H>
# include <htlist.H>
# include <tpl_graph_utils.H>
# include <tpl_find_path.H>

namespace Aleph {


/** 
  @brief Operations strongly connected components of a digraph
   Tarjan using the algorithm for calculating the
   strongly connected components of a graph.

	@details The class uses two type parameters:
	-# GT: Digraph class.
	-# SA: Arches filter uses the internal iterator.

	@ingroup Grafos
*/
     template <class GT, 
	       template <typename, class> class Itor = Node_Arc_Iterator,
	       class SA = Dft_Show_Arc<GT>> 
class Tarjan_Connected_Components
{
  SA & sa;

  GT * g_ptr = NULL;

  DynListStack<typename GT::Node*> stack;

  long df_count;
  mutable size_t n; /* Number of nodes in the graph */

/** 
  @brief List of lists; Each list stores the nodes of the blocks
*/
  DynList<DynList<typename GT::Node*>> * list_list_ptr; 

/** 
  @brief List strongly related blocks
*/
  DynList <GT> * block_list_ptr; 

/** 
  @brief List of component sizes
*/
  DynList<size_t> * list_len_ptr;

  Path<GT> * path_ptr;

public:

  Tarjan_Connected_Components(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Tarjan_Connected_Components(SA & __sa) : sa(__sa) { /* Empty */ }

private:

  struct Init_Tarjan_Node
  {
    void operator () (const GT & g, typename GT::Node * p)
    {
      g.reset_bits(p);
      g.reset_counter(p); /* initializes df */
      low <GT> (p) = -1;  /* initializes low */
    }
  };
 
  bool is_node_in_stack(typename GT::Node * p)
  {
    return IS_NODE_VISITED(p, Aleph::Min);
  }

  void init_node_and_push_in_stack(typename GT::Node * p)
  {
    assert(not is_node_in_stack(p));

    stack.push(p);
    NODE_BITS(p).set_bit(Aleph::Min, true);
    NODE_BITS(p).set_bit(Aleph::Depth_First, true);
    df<GT>(p) = low<GT>(p) = df_count++;
  }

  typename GT::Node * pop_from_stack()
  {
    typename GT::Node * ret = stack.pop();
    NODE_BITS(ret).set_bit(Aleph::Min, false);

    return ret;
  }

  void scc_by_blocks(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

    /** Go in depth all connected nodes to v */
    for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next())
    {
      typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
      if (not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          scc_by_blocks(w);
          low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
        }
      else if (is_node_in_stack(w))
    /* If it is in stack ==> v was visited beforep */
        low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
    }
    /** ¿First node visited the block?
    Yes? ==> Remove nodes that are battery block */
    if (low <GT> (v) == df <GT> (v)) 
      {	    
	const size_t & blk_idx = block_list_ptr->size();
	GT & blk = block_list_ptr->append(GT());

    /** Remove the battery block to get a v */
	while (true) 
	  {
	    typename GT::Node * p = pop_from_stack();
	    typename GT::Node * q = blk.insert_node(p->get_info());
	    *q = *q; /* Copia contenido del nodo */
	    NODE_COOKIE(p) = NODE_COOKIE(q) = NULL;
	    GT::map_nodes(p, q);
	    NODE_COUNTER(p) = NODE_COUNTER(q) = blk_idx;
	    if (p == v)
	      break;
	  }
      }
  } 

  void scc_by_lists(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

    /** Go in depth all connected nodes to v */
    for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next())
    { 
      typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
      if (not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          scc_by_lists(w);
          low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
        }
      else if (is_node_in_stack(w))
    /* If it is in stack ==> v was visited before p */
        low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
    }

    /** First node visited the block?
    Yes? ==> Remove nodes that are battery block */
    if (low<GT>(v) == df<GT>(v)) 
      {     
	DynList<typename GT::Node*> & l = 
	  list_list_ptr->append(DynList<typename GT::Node*>());

    /** Remove battery block until a v */
	while (true) 
	  {
	    typename GT::Node * p = pop_from_stack();
	    l.append(p);

	    if (p == v)
	      break;
	  }
      }
  }

  void scc_by_len(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

    /** Go in depth all connected nodes to v */
    for (Itor<GT, SA> it(v, sa); it.has_curr(); it.next())
    { 
      typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
      if (not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          scc_by_len(w);
          low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
        }
      else if (is_node_in_stack(w))
    /* If you are in stack ==> v was visited before p */
        low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
    }

    /** First node visited the block?
    Yes? ==> Remove the nodes are battery block */
    if (low<GT>(v) == df<GT>(v)) 
      {     
	size_t count = 0;

    /** Remove block stack up to v */
	while (true) 
	  {
	    typename GT::Node * p = pop_from_stack();
	    ++count;

	    if (p == v)
	      break;
	  }
	list_len_ptr->append(count);
      }
  }

  void init_tarjan(const GT & g)
  {
    Operate_On_Nodes <GT, Init_Tarjan_Node> () (g); /* Starts bits, df y low */

    df_count = 0; /* Count */

    stack.empty();

    n = g.get_num_nodes();

    g_ptr = &const_cast<GT&>(g);
  }

/** 
  @brief Returns true if there is a cycle since v 
*/
  bool has_cycle(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

     /** Go in depth all connected nodes to v */
    for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next())
    {
      typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
      if (not IS_NODE_VISITED(w, Aleph::Depth_First))
        {
          if (has_cycle(w))
            return true;

          low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
        }
      else if (is_node_in_stack(w))
    /* If stack ==> v was visited before p */
        low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
    }
    /** First visited node block?
    Yes? ==> Check if you have two or more nodes */
    if (low<GT>(v) == df<GT>(v)) 
      {    
	int i = 0;
    /** Remove the battery block to get v */
	for (; i < 2; ++i) 
	  if (pop_from_stack() == v)
	    break;
      
	return i >= 2; /* Si i>= 2 ==> Cycle */
      }  

    return false; /* V was ran all without finding cycle */
  }

/** 
  @brief Take the block block, which is mapped from the original graph,
  which is strongly connected, and builds on the cycle path_ptr
*/
      void 
  build_path(const GT &                                              block, 
	     DynMapAvlTree<typename GT::Node*, typename GT::Node*> & table)
  {
    /* Search Cycle in blk. */
    typename GT::Arc * a = block.get_first_arc();
    typename GT::Node * start = block.get_tgt_node(a);
    typename GT::Node * end = block.get_src_node(a);
    assert(start != end);

    Path<GT> aux_path = 
      Directed_Find_Path<GT, Itor, SA>(block, sa).dfs(start, end);
    assert(not aux_path.is_empty()); /* As it is connected must be */

    /* aux_path block is mapped on.
    But we need the road is on the original graph.
    The nodes of the original graph is retrieved from the mapping table */
    path_ptr->clear_path();
    for (typename Path<GT>::Iterator i(aux_path); i.has_curr(); i.next())
      {
	path_ptr->append_directed(table.find(i.get_current_node()));
      }
    
    path_ptr->append_directed(path_ptr->get_first_node());
  }

/** 
  @brief Returns true if it finds a cycle, in which case the path starts,
  otherwise, it returns false and path remains intact
*/
  bool build_cycle(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

     /** Go in depth all connected nodes to v */
    for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next())
      {
	typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
	if (not IS_NODE_VISITED(w, Aleph::Depth_First))
	  {
	    if (build_cycle(w))
	      return true;

	    low<GT>(v) = std::min(low<GT>(v), low<GT>(w));
	  }
	else if (is_node_in_stack(w))
	  /* If stack ==> v was visited before p */
	  low<GT>(v) = std::min(low<GT>(v), df<GT>(w));
      }

    /* First node visited the block? */
    if (low<GT>(v) == df<GT>(v)) 
      {
	GT blk; /* Grafo auxiliar */

/** @brief Node mapping g blk (the cookies are busy) */
	DynMapAvlTree<typename GT::Node*, typename GT::Node*> table; 
	
	  /** Remove battery and insert nodes in auxiliary block
    takes the component and inserted in blk */
	while (true) 
	  {
	    typename GT::Node * p = pop_from_stack();
	    typename GT::Node * q = blk.insert_node();
	    *q = *p; /* Copy content of node */
	    table.insert(q, p);
	    table.insert(p, q);
	    if (p == v)
	      break;
	  }

	if (blk.get_num_nodes() == 1) 
	  return false; /* If you have only one node blk no cycle */

    /** Finished building block with bows */
	for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next())
	  {
	    typename GT::Node * bsrc = j.get_curr(); 
	    typename GT::Node * gsrc = table.find(bsrc); 

    /** Browse arches gsrc */
	    for (Itor <GT, SA> k(gsrc, sa); k.has_curr(); k.next())
	      {
		typename GT::Arc  * ga = k.get_curr();
		typename GT::Node * gtgt = g_ptr->get_tgt_node(ga);
		typename GT::Node ** btgt_ptr = table.test(gtgt);
		if (btgt_ptr == NULL) /* Block Arc? */
		  continue;

		typename GT::Arc * ta = blk.insert_arc(bsrc, *btgt_ptr);
		*ta = *ga; /* Copy contents of the arc */
	      }
	  }

	build_path(blk, table);

	return true;
      }  

    return false;


  bool is_connected(typename GT::Node * v)
  {
    init_node_and_push_in_stack(v);

     /** Go in depth all connected nodes to v */
    for (Itor <GT, SA> it(v, sa); it.has_curr(); it.next())
      {
	typename GT::Node * w = g_ptr->get_tgt_node(it.get_curr());
	if (not IS_NODE_VISITED(w, Aleph::Depth_First))
	  {
	    if (not is_connected(w))
	      return false;

	    low <GT> (v) = std::min(low <GT> (v), low <GT> (w));
	  }
	else if (is_node_in_stack(w))
	  low <GT> (v) = std::min(low <GT> (v), df <GT> (w));
      }

    /** First visited node block?
    Yes? Remove nodes stack to find v */
    if (low<GT>(v) == df<GT>(v)) 
      {     
    /** Remove stack nodes to find v */
	while (pop_from_stack() != v); 

	return stack.is_empty();
      }  

    return true;
  }

public:

/** 
  @brief Computes the strongly connected components of a digraph.
  
  @details connected_components () takes a digraph g,
  "supposedly weakly related" calculates its subgraphs or components
  disconnected and stored in a dynamic list of subdigrafos blk_list
  mapped both nodes and arcs. The emphasis on
  Of course you connected graph character is that if the graph does not
  it was connected, then the resulting list would contain a single
  copy the digraph corresponding element mapping g. If this
  were the case, then it is best done with the function
  copy_graph ().

  The function is based on the algorithm of Tarjan.

  The function uses the bit to mark nodes and build_subtree
  arches visited.

  The algorithm finds the components internally by
  depth searches.
  
  @param[in] g Graph on which you want to calculate your blocks.
  @param[out] blk_list List mapped to subgraphs with g
  subgraphs g or disconnected components.
  @param[out] arc_list List of arcs that connect the blocks.
  
  @throw bad_alloc If no memory to build a block or
  insert into the list.
  
  @see copy_graph()
*/
  void connected_components(const GT & g, DynList <GT> & blk_list,
			    DynList<typename GT::Arc*> & arc_list)
  {
    init_tarjan(g);

    block_list_ptr = &blk_list;

    for (typename GT::Node_Iterator it(g); df_count < n; it.next())
      {
	typename GT::Node * v = it.get_curr();
	if (not IS_NODE_VISITED(v, Aleph::Depth_First)) 
	  scc_by_blocks(v); 
      }

    assert(stack.is_empty());

    /** Through each of partial subgraphs and add their bows */
    for (typename DynList<GT>::Iterator i(blk_list); i.has_curr(); i.next())
      {     
    /* Browse all nodes in the block */
	GT & blk = i.get_curr();
	for (typename GT::Node_Iterator j(blk); j.has_curr(); j.next())
	  {
	    typename GT::Node * bsrc = j.get_curr();
	    typename GT::Node * gsrc = mapped_node<GT>(bsrc);

    /** Browse arches gsrc */
	    for (Itor<GT, SA> k(gsrc, sa); k.has_curr(); k.next())
	      {
		typename GT::Arc  * ga   = k.get_current_arc();
		typename GT::Node * gtgt = g_ptr->get_tgt_node(ga);
		if (NODE_COUNTER(gsrc) != NODE_COUNTER(gtgt)) 
		  {   
    /* Arc inter-block ==> add it to arc_list */
		    arc_list.append(ga);
		    continue;
		  }

    /* Insert and map the arc in the sub-block */
		typename GT::Node * btgt = mapped_node<GT>(gtgt);
		typename GT::Arc  * ba = blk.insert_arc(bsrc, btgt);
		*ba = *ga; /* Copy contents of the arc */
		GT::map_arcs(ga, ba);
	      }
	  }
      }
  

/** 
  @brief Computes the strongly connected components of a digraph.

  @details This overloaded version connected_components () takes a digraph g,
  "supposedly weakly related" calculates its subgraphs or components
  disconnected and stored in a list dynamic list of node lists.

  Each list contains the nodes belonging to the component
  strongly connected.

  Wear this function if not necessary get mapped back
  strongly connected components.

  The function is based on the algorithm of Tarjan.

  The function uses the bit to mark nodes and build_subtree
  arches visited.

  The algorithm finds the components internally by
  depth searches.

  Since the mapping is saved, this routine is faster and
  less expensive in memory than the previous version. the
  perhaps disadvantage is that it calculates the possible arcs
  They may interconnect a block in one direction.

  @param[in] g Graph on which you want to calculate your blocks.
  @param[out] blks List of lists of nodes. each list
  It contains nodes that form a connected component.

  @throw bad_alloc If no memory to insert into the list.

  @see copy_graph()
*/
  void connected_components(const GT & g, 
			    DynList<DynList<typename GT::Node*>> & blks)
  {
    init_tarjan(g);

    list_list_ptr = &blks;

    for (typename GT::Node_Iterator it(g); df_count < n; it.next())
      {
	typename GT::Node * v = it.get_curr();
	if (not IS_NODE_VISITED(v, Aleph::Depth_First))
	  scc_by_lists(v); 
      }
  }

/** 
  @brief Computes the Strongly connected components of a digraph.

  @details This overloaded version connected_components () takes a digraph g,
  "supposedly loosely connected" and has its subgraphs or components
  unconnected with their sizes.

  The function is based on the algorithm of Tarjan.

  The function uses the bit to mark nodes and build_subtree
  arches visited.

  The algorithm finds the components internally by
  depth searches.

  @param[in] g Graph on which you want to calculate your blocks.
  @param[out] blks List of positive integers. each list
  it contains a different size related component.

  @throw bad_alloc If no memory to insert into the list.

  @see copy_graph()
*/
  void connected_components(const GT & g, DynList<size_t> & blks)
  {
    init_tarjan(g);

    list_len_ptr = &blks;

    for (typename GT::Node_Iterator it(g); df_count < n; it.next())
      {
	typename GT::Node * v = it.get_curr();
	if (not IS_NODE_VISITED(v, Aleph::Depth_First))
	  scc_by_len(v); 
      }
  }

/** 
  @overload connected_components 
*/
  void operator () (const GT &                   g, 
                    DynList <GT> &               blk_list,
                    DynList<typename GT::Arc*> & arc_list)
  {
    connected_components(g, blk_list, arc_list);
  }

/** 
  @overload connected_components 
*/
  void operator () (const GT &                             g, 
                    DynList<DynList<typename GT::Node*>> & blks)
  {
    connected_components(g, blks);
  }

/** 
  @overload connected_components 
*/
  void operator () (const GT &                    g, 
                    DynDlist <GT> &               blk_list,
                    DynDlist<typename GT::Arc*> & arc_list)
  {
    DynList<GT> blist;
    DynList<typename GT::Arc*> alist;
    connected_components(g, blist, alist);

    for (typename DynList<GT>::Iterator it(blist); it.has_curr(); it.next())
      {
	GT & curr = it.get_curr();
	GT & block = blk_list.append(GT());
	curr.swap(block);
      }

    for (typename DynList<typename GT::Arc*>::Iterator it(alist); 
	 it.has_curr(); it.next())
      arc_list.append(it.get_curr());
  }

/** 
  @overload connected_components 
*/
  void operator () (const GT &                               g, 
                    DynDlist<DynDlist<typename GT::Node*>> & blks)
  {
    DynList<DynList<typename GT::Node*>> b;
    connected_components(g, b);
      
    for (typename DynList<DynList<typename GT::Node*>>::Iterator it(b);
         it.has_curr(); it.next())
      {
	DynDlist<typename GT::Node*> & tgt_list = 
	  blks.append(DynDlist<typename GT::Node*>());

	DynList<typename GT::Node*> & blk = it.get_curr();
	while (not blk.is_empty())
	  tgt_list.append(blk.remove_first());
      }   
  }

/** 
  @brief Returns true if the digraph g contains at least one cycle 
*/
  bool has_cycle(const GT & g)
  {
    init_tarjan(g);

    for (typename GT::Node_Iterator it(g); df_count < n; it.next())
    {
      typename GT::Node * v = it.get_curr();
      if (not IS_NODE_VISITED(v, Aleph::Depth_First)) 
        if (has_cycle(v)) 
          return true;
    }
    
    return false; 
  }

/** 
  @brief Returns true if the directed graph is acyclic
*/
  bool is_dag(const GT & g)
  {
    return not has_cycle(g);
  }

  bool compute_cycle(const GT & g, Path<GT> & path)
  {
    init_tarjan(g);
    path_ptr = &path;
    path_ptr->set_graph(g);

    for (Node_Iterator<GT> it(g); df_count < n; it.next())
      {
	typename GT::Node * v = it.get_curr();
	if (not IS_NODE_VISITED(v, Aleph::Depth_First)) /* ¿p visited? */
	  if (build_cycle(v)) 
	    return true;
      }

    return false; 
  }

  bool test_connectivity(const GT & g)
  {
    init_tarjan(g);

    for (typename GT::Node_Iterator it(g); df_count < n; it.next())
      {
	typename GT::Node * v = it.get_curr();
	if (not IS_NODE_VISITED(v, Aleph::Depth_First)) 
	  if (not is_connected(v))
	    return false;
      }

    assert(stack.is_empty());

    return true;
  }
};


/** 
  @brief Determines whether a digraph containing a cycle and builds.
        
  Compute_Cycle_In_Digraph() takes a digraph g determines whether
  it contains a cycle and, if so, build a road
  contentive cycle in question.

  The class is based on the algorithm of Tarjan.

  The function takes two type parameters:
  -# GT: Digraph class.
  -# SA: Arches filter uses the internal iterator.

  The class uses the bit to mark nodes and build_subtree
  arches visited.

  @ingroup Grafos

  @throw domain_error If G is a graph (not a digraph)
*/
     template <class GT, 
	       template <typename, class> class Itor = Node_Arc_Iterator,
	       class SA = Dft_Show_Arc<GT>> 
class Compute_Cycle_In_Digraph
{
  SA & sa;

public:

  Compute_Cycle_In_Digraph(SA && __sa = SA()) : sa(__sa) { /* Empty */ }

  Compute_Cycle_In_Digraph(SA & __sa) : sa(__sa) { /* Empty */ }

/** 
  @brief Invokes the calculation of a cycle in a digraph.

  @param[in] g Graph on which you want to calculate your blocks.
  @param[out] path Path that defines the cycle.

  @return true if the graph contains a cycle; false otherwise. 
  In the latter case the value of path is undetermined.

  @throw bad_alloc If there is insufficient memory.
*/
  bool operator () (GT & g, Path<GT> & path) const
  {
    Tarjan_Connected_Components<GT, Itor, SA> tarjan(sa);

    return tarjan.compute_cycle(g, path);
  }
};

} /* End namespace Aleph */

# endif // TARJAN_H
