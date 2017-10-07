
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
  @file tpl_indexGraph.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_INDEXGRAPH
# define TPL_INDEXGRAPH

# include <tpl_indexNode.H>
# include <tpl_indexArc.H>

namespace Aleph
{

/** 
  @brief Builds indexes of nodes and arcs for quick search and retrieval.

  @details Index_Graph indexed nodes and arcs for the purposes of recovery
  quick.

  To facilitate its use and make safer,
  Index_Graph offers classical topological operations of a
  graph: insert_node(), insert_arc(), etc.

  The class receives the following parameters type:
  -# GT: Type of graph based on List_Graph.
  -# Compare: Comparation class to the key indexing of the
  nodes. The contract of this class is to implement operator () as follows: 
  @code
    template <class GT>  
  struct Dft_Node_Cmp   
  {  
      bool  
      operator () (typename GT::Node * p1, typename GT::Node * p2) const 
    {  
     // Acceso a los nodos y comparación según el campo deseado  
    }  
  }; 
  @endcode

  By default, this class is programmed to compare
  the value returned by get_info() on each node. For this, the
  operator < of type GT::Node_Type must be implemented
  -# Tree: type binary search tree used internally
  to index key. By default treaps used
  .

  @see IndexArc IndexNode

  @ingroup Grafos

  @author Leandro Rabindranath León (lrleon en ula punto ve)
  @author Alejandro Mujica (aledrums en gmail punto com)
*/
template 
<class GT, 
 class Compare = Dft_Node_Cmp<GT>,
 template <class /* Key */, class /* Compare */> class Tree = Treap>
class Index_Graph
{
private:

  typedef typename GT::Arc GT_Arc;
  typedef typename GT::Node GT_Node;
  typedef typename GT::Arc_Type GT_Arc_Type;
  typedef typename GT::Node_Type GT_Node_Type;

  IndexNode<GT, Compare, Tree> idx_node;
  IndexArc<GT, Tree>           idx_arc;

public:

/** @brief Create an index graph: nodes and arcs are indexed. */
  Index_Graph(GT & g) : idx_node(g), idx_arc(g)
  {
    /* Empty */
  }

/** 
  @brief Create a new node and inserts it in the graph and in the index.

  @param[in] info Information to be copied to the node.

  @return A pointer to the new node.

  @throw bad_alloc If there is insufficient memory.
*/
  GT_Node * insert_node(const GT_Node_Type & info)
  {
    return idx_node.insert_in_graph(info);
  }

  /** @brief Create a new arc between two nodes and inserts it in the graph and
      the index.

  @param[in] src Origin node.
  @param[in] tgt Destination node.
  @param[in] info Information to be copied to the arc. The default is
  which gives the GT_Arc_Type() constructor.

  @return A pointer to the new arc.

  @throw bad_alloc If there is insufficient memory.
*/
  GT_Arc * insert_arc(GT_Node * src, GT_Node * tgt, 
		      const GT_Arc_Type & info = GT_Arc_Type())
    throw (std::exception, std::domain_error)
  {
    if (idx_node.search(src) == NULL)
      throw std::domain_error("src node not in index");

    if (idx_node.search(tgt) == NULL)
      throw std::domain_error("tgt node not in index");

    return idx_arc.insert_in_graph(src, tgt, info);
  }

/** 
  @brief Search in the index a node. 

  @param[in] p Pointer to the node.

  @return A pointer to the node should be in the index; NULL otherwise.
*/
  GT_Node * search_node(GT_Node * p)
  {
    return idx_node.search(p);
  } 

/** 
  @brief Search in the index a node.

  @param[in] info Information according to which will seek the node.

  @return A pointer to the node should be in the index; NULL otherwise.

  @warning Consider that the search is performed according to the
  implementation of the Compare comparison class last time
  instantiation.
*/
  GT_Node * search_node(const GT_Node_Type & info)
  {
    return idx_node.search(info);
  }

/** 
  @brief Search in the index an arc given two nodes.

  @param[in] src Pointer to the origin node.
  @param[in] tgt Pointer to the destination node.

  @return A pointer to the arc should be in the index; NULL otherwise.
*/
  GT_Arc * search_arc(GT_Node * src, GT_Node * tgt)
  {
    return idx_arc.search(src, tgt);
  }

/**
  @brief Removes the node p from the graph and index.

  @param[in] p Pointer to the node to remove
*/
  void remove_node(GT_Node * p)
  {
    
    /** Remove the index the arcs emanating from node
    (they will be removed by removing node of the graph) */
    for (typename GT::Node_Arc_Iterator it(p); it.has_current(); it.next())
      idx_arc.remove(it.get_current());

    return idx_node.remove_from_graph(p); 
  }

/** @brief Eliminates the graph and from the arc index a */
  void remove_arc(GT_Arc * a)
  {
    return idx_arc.remove_from_graph(a);
  }

/** @brief Returns the number of arcs that contains the index. */
  size_t get_num_arcs() const { return idx_arc.size(); }

/** @brief Returns the number of nodes that contains the index. */
  size_t get_num_nodes() const { return idx_node.size(); }
};


}

# endif // TPL_INDEXGRAPH
