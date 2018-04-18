
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
  @file tpl_indexArc.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_INDEXARC_H
# define TPL_INDEXARC_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{


/** 
  @brief Build an index of arcs for quick search and recovery since 
  its nodes.

  @details IndexArc indexes arcs of a graph according to the node pair
  related arc. The idea is to implement the detection of existence
  arc and recovery in logarithmic time.

  The class receives the following parameters type:
  -# GT: Type of graph based on List_Graph.
  -# Tree: Type binary tree search used internally to index keys. 
  By default treaps is used.

  @warning For the successful compilation is imperative that the
  there builder GT::Arc(void * src, void * tgt, GT::Arc_Type).
  similarly, there should also be the default constructor GT::Arc_Type().

  @see IndexNode Index_Graph

  @ingroup Grafos
*/
  template <
    class GT,
    template <class /* Key */, class /* Compare */> class Tree = Rand_Tree,
    class SA = Dft_Show_Arc<GT>
    >
class IndexArc
{
private:
			
  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc_Type GT_Arc_Type;
  typedef typename GT::Node_Type GT_Node_Type;

  typedef std::pair<void*, void*> Node_Pair;

  struct Cmp_Arc
  {
    bool operator () (GT_Arc * a1, GT_Arc * a2) const
    {
      if (a1->src_node < a2->src_node)
	return true;

      return not (a2->src_node < a1->src_node) and a1->tgt_node < a2->tgt_node;
    }
  };

  GT & g;
  DynSetTree <GT_Arc *, Tree, Cmp_Arc> index;
  SA & sa;

public:

/** @brief Insert in the index the "a" arc. */
  GT_Arc * insert(GT_Arc * e)
  {
    return *index.put(e);
  }

/** 
  @brief Search an arc that connecting two nodes.

  @param[in] src Origin node.
  @param[in] tgt Destination node.

  @return A pointer to the arch if it exists, or NULL otherwise.
*/
  GT_Arc * search(void * src, void * tgt)
  {
    GT_Arc arc;
    arc.src_node = src; 
    arc.tgt_node = tgt;
    
    GT_Arc ** ret_val = index.search(&arc);

    if (ret_val != NULL)
      return *ret_val;
 
    if (g.is_digraph())
      return NULL;

    std::swap(arc.src_node, arc.tgt_node);
    ret_val = index.search(&arc);

    if (ret_val == NULL)
      return NULL;

    assert(((src == (*ret_val)->src_node) and (tgt == (*ret_val)->tgt_node)) or 
      ((tgt == (*ret_val)->src_node) and (src == (*ret_val)->tgt_node)));

    return *ret_val; 
  }

/** 
  @brief Search an arc according pointer.

  @param[in] a Arc to be searched.

  @return A pointer to the arch if it exists, or NULL otherwise.
*/
  GT_Arc * search(GT_Arc * a)
  {
    return search(a->src_node, a->tgt_node);
  }

/** 
  @brief Create a new arc between two nodes, inserts it in the graph and
  then the index.

  @param[in] src Ndo origen.
  @param[in] tgt Destination node.
  @param[in] info Information copied as content of node. By default 
  the content that is copied throw GT::Arc_Type() constructor.

  @return A pointer to the arc created.

  @throw bad_alloc If there is insufficient memory.
*/
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt, 
			   const GT_Arc_Type & info)
    throw (std::exception, std::domain_error)
  {
    GT_Arc * a = search(src, tgt);

    if (a != NULL)
      throw std::domain_error("There is already in arc between these nodes");

    a = g.insert_arc(src, tgt, info);
    insert(a);
    
    return a;
  }

/** @overload  insert_in_graph */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt, 
			   GT_Arc_Type && info = GT_Arc_Type())
    throw (std::exception, std::domain_error)
  {
    GT_Arc * a = search(src, tgt);

    if (a != NULL)
      throw std::domain_error("There is already in arc between these nodes");

    a = g.insert_arc(src, tgt, std::move(info));
    insert(a);
    
    return a;
  }

/** @brief Deleted from the index "e" arc. */
  void remove(GT_Arc * e)
  {
    index.remove(e);
  }

/** @brief Deleted from the index and graph the "a" arc */
  void remove_from_graph(GT_Arc * a)
  {
    remove(a);
    g.remove_arc(a);
  }

/** @brief Removes all index arches. */
  void clear_index()
  {
    index.empty();
  }

/** @brief Enter all the arcs of the graph in the index. */
  void build_index()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
	GT_Arc * a = it.get_curr();

	if (search(a) != a)
	  insert(a);
      }
  }

private:

  void init()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next())
      insert(it.get_curr());
  }

public:

/** 
  @brief Creates an index from the arches inserted in the graph.

  @param[in] __g Graph
  @param[in] with_init if the value is true, then the arches
  __g are inserted in the index; otherwise, the index
  remains empty

  @throw bad_alloc If there is insufficient memory.
*/
  IndexArc(GT & __g, bool with_init = true, SA && __sa = SA()) 
    : g(__g), sa(__sa)
  {
    if (with_init)
      init();
  }

/** 
  @brief Creates an index from the arches inserted in the graph.

  @param[in] __g Graph
  @param[in] with_init if the value is true, then the arches
  __g are inserted in the index; otherwise, the index
  remains empty

  @throw bad_alloc If there is insufficient memory.
*/
  IndexArc(GT & __g, bool with_init, SA & __sa) : g(__g), sa(__sa)
  {
    if (with_init)
      init();
  }

/** @brief Returns the amount of arcs that contains the index. */
  size_t size() const { return index.size(); }
};

}

# endif
