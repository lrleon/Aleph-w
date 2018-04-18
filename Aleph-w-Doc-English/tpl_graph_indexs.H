
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
  @file tpl_graph_indexs.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_GRAPH_INDEXS_H
# define TPL_GRAPH_INDEXS_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{

template <class GT>
struct Dft_Node_Cmp
{
  bool operator () (typename GT::Node * p1, typename GT::Node * p2) const
  {
    return p1->get_info() < p2->get_info();
  }
};

template <class GT>
struct Dft_Arc_Cmp
{
  bool operator () (typename GT::Arc * a1, typename GT::Arc * a2) const
  {
    if (a1->src_node < a2->src_node)
      return true;

    return a1->tgt_node < a2->tgt_node;
  }
};

/** 
  @brief Build an nodes index for quick search and retrieval.

  @details IndexNode indexes the nodes of a graph according to a given key
  user-defined.

  The class receives the following parameters type:
  -# GT: Type of graph based on List_Graph.
  -# Compare: Comparation class to the key indexing. 
  
  The contract of this class is to implement operator () as follows:  
  @code
      template <class GT>  
	struct Dft_Node_Cmp  
	{  
    bool operator () (typename GT::Node * p1, typename GT::Node * p2) const 
      {  
        // Access to the nodes and comparation by the desired field 
      }  
  }; 
  @endcode
  
  IndexNode receive this class by default programmed to compare
  the value returned by get_info() on each node. For this, the
  operator < del tipo GT::Node_Type must be implemented.
  -# SN: Filter class for the iterator node if it is built
  -# Tree: Type binary tree search used internally to index keys. 
  By default treaps is used.

  @see IndexArc Index_Graph

  @ingroup Grafos
*/
template <
          class GT,
          class Compare = Dft_Node_Cmp<GT>,
          template <typename /* Key */, class /* Compare */> class Tree = Treap,
          class SN = Dft_Show_Node<GT>
         >
class Nodes_Index : public DynSetTree <Tree, typename GT::Node *, Compare>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Info;
  typedef DynSetTree <Tree, GT_Node *, Compare> Tree_Type;

  GT & g;
  SN & sn;

  void init()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_current(); it.next())
      insert(it.get_current());
  }

public:
  Nodes_Index(GT & _g, Compare & cmp, SN & _sn)
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

  Nodes_Index(GT & _g, Compare && cmp = Compare(), SN && _sn = SN())
    : Tree_Type(cmp), g(_g), sn(_sn)
  {
    init();
  }

/** 
  @brief Inserts the p node in the graph and then in the index.

  @param[in] p Pointer to the node you want to insert.

  @return A pointer the node to inserted.
*/
  GT_Node * insert_in_graph(GT_Node * p)
  {
    g.insert_node(p);

    if (insert(p) == NULL)
      {
        g.remove_node(p);
        return NULL;
      }

    return p;
  }

/** 
  @brief Crea un nuevo nodo con contenido genÃ©rico info, lo inserta en
  el grafo y luego en el índice

  @param[in] info Associated content with the new node.

  @return A pointer to the node

  @throw bad_alloc If there is insufficient memory
*/
  GT_Node * insert_in_graph(const GT_Node_Info & info)
  {
    GT_Node * p = g.insert_node(info);
    
    if (insert(p) == NULL)
      {
        g.remove_node(p);
        return NULL;
      }

    return p;
  }
  
/** @overload insert_in_graph */
  GT_Node * insert_in_graph(const GT_Node_Info && info = GT_Node_Info())
  {
    return insert_in_graph(info);
  }

/** 
  @brief Looking for a node based on the content of p.

  @details Note that the search criterion is determined
  by the class parameter of Compare comparison.

  @param[in] p Node whose information you want to search.

  @return A pointer to the node if found or null otherwise.
*/
  GT_Node * search(GT_Node * p)
  {
    GT_Node ** ret_val = Tree_Type::search(p);

    if (ret_val == NULL)
      return NULL;

    return *ret_val;
  }

  GT_Node * search(const GT_Node_Info & info)
  {
    GT_Node dummy_node(info);
    GT_Node * dummy_node_ptr = &dummy_node;

    return search(dummy_node_ptr);
  }

/** 
  @brief Removes the node p from the graph and index.

  @param[in] p Pointer to the node to remove
      	  
  @throw domain_error If the node is not inserted in the index (It
  possibly means that even in the graph).
*/
  void remove_from_graph(GT_Node * p) throw(std::exception, std::domain_error)
  {
    find(p); /* Shoot exception if p is not indexed */
    remove(p);
    g.remove_node(p);
  }
};

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

  @warning Para que la compilación correcta es imperativo que el
  exista el constructor GT::Arc(void * src, void * tgt, GT::Arc_Type). 
  Del mismo modo, tambiÃ©n debe existir el constructor
  por omisión GT::Arc_Type().

  @see IndexNode Index_Graph

  @ingroup Grafos
*/

template <
          class GT,
          class Compare = Dft_Arc_Cmp<GT>,
          template <typename /* Key */, class /* Compare */> class Tree = Treap,
          class SA = Dft_Show_Arc<GT>
         >
class Arcs_Index : public DynSetTree <Tree, typename GT::Arc *, Compare>
{
  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Info;
  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Arc_Type GT_Arc_Info;
  typedef DynSetTree <Tree, typename GT::Arc *, Compare> Tree_Type;

  GT & g;
  SA & sa;

  void init()
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_current(); it.next())
      insert(it.get_current());
  }

public:
  Arcs_Index(GT & _g, Compare & cmp, SA & _sa)
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

  Arcs_Index(GT & _g, Compare && cmp = Compare(), SA && _sa = SA())
    : Tree_Type(cmp), g(_g), sa(_sa)
  {
    init();
  }

/** 
  @brief Create a new arc between two nodes, inserts it in the graph and
  then the index.

  @param[in] src Origin node.
  @param[in] tgt Destination node.
  @param[in] info Information copied as content of node. By default 
  the content that is copied throw GT::Arc_Type() constructor.

  @return A pointer to the arc created.

  @throw bad_alloc If there is insufficient memory.
*/
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info & info)
  {
    GT_Arc * a = g.insert_arc(src, tgt, info);
    
    if (insert(a) == NULL)
      {
        g.remove_arc(a);
        return NULL;
      }

    return a;
  }

/** @overload insert_in_graph */
  GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                           const GT_Arc_Info && info = GT_Arc_Info())
  {
    return insert_in_graph(src, tgt, info);
  }

/** 
  @brief Search an arc that connecting two nodes.

  @param[in] src Origin node.
  @param[in] tgt Destination node.
  @param[in] info Information associated with the arc.

  @return A pointer to the arch if it exists, or NULL otherwise.
*/
  GT_Arc * search(GT_Node * src, GT_Node * tgt, const GT_Arc_Info & info)
  {
    GT_Arc arc(info);
    arc.src_node = src; arc.tgt_node = tgt;
    GT_Arc * ptr_arc = &arc;
    
    GT_Arc ** ret_val = Tree_Type::search(ptr_arc);

    if (ret_val != NULL)
      return *ret_val;
 
    if (g.is_digraph())
      return NULL;

    std::swap(arc.src_node, arc.tgt_node);
    ret_val = Tree_Type::search(ptr_arc);
    if (ret_val == NULL)
      return NULL;

    assert(((src == (*ret_val)->src_node) and (tgt == (*ret_val)->tgt_node)) or 
      ((tgt == (*ret_val)->src_node) and (src == (*ret_val)->tgt_node)));

    return *ret_val;
  }

/** @overload  search */
  GT_Arc * search(GT_Node * src, GT_Node * tgt,
                  const GT_Arc_Info && info = GT_Arc_Info())
  {
    return search(src, tgt, info);
  }

/** 
  @brief Eliminates the arc of the graph and the index.

  @param[in] a Pointer to arc to remove

  @throw domain_error If the arc is not inserted in the index (It 
  possibly means that even in the graph).
*/
  void remove_from_graph(GT_Arc * a)  throw(std::exception, std::domain_error)
  {
    find(a);
    remove(a);
    g.remove_arc(a);
  }

};

} /* End namespace Aleph */

# endif
