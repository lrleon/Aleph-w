
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
  @file tpl_.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_INDEXNODE_H
# define TPL_INDEXNODE_H

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace Aleph
{

  template <class GT>
struct Dft_Node_Cmp
{
      bool
  operator () (typename GT::Node * p1, typename GT::Node * p2) const
  {
    return p1->get_info() < p2->get_info();
  }
};

/** 
  @brief Build an nodes index for quick search and retrieval.

  @details IndexNode indexes the nodes of a graph according to a given key
  user-defined.

  The class receives the following parameters type:
  -# GT: Type of graph based on List_Graph.
  -# Compare: Comparation class to the key indexing. The 
  contract of this class is to implement operator () as follows:
  @code
      template <class GT>  
	struct Dft_Node_Cmp  
	{  
        bool  
      operator () (typename GT::Node * p1, typename GT::Node * p2) const 
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
  template
  <class GT, 
   class Compare = Dft_Node_Cmp<GT>,
   template <class /* Key */, class /* Compare */> class Tree = Treap,
   class SN = Dft_Show_Node<GT> >  
class IndexNode
{
private:

  typedef typename GT::Node GT_Node;
  typedef typename GT::Node_Type GT_Node_Type;
	
  DynSetTree<GT_Node*, Tree, Compare> index;

  GT & g;
  SN & sn;
		
public:

/** 
  @brief Insert the "p" node in index.

  @param[in] p Pointer to the node you want to store in the index

  @throw bad_alloc If there is insufficient memory
*/
  GT_Node * insert(GT_Node * p)
  {
    index.put(p);
    return p;
  }

/** 
  @brief Create a new node with "info" generic content, inserted into
  the graph and then the index

  @param[in] info Associated content with the new node.

  @return Un puntero al nodo

  @throw bad_alloc If there is insufficient memory
*/
  GT_Node * insert_in_graph(const GT_Node_Type & info)
  {
    GT_Node * ret_val = g.insert_node(info);

    try
      {
	insert(ret_val);

	return ret_val;
      }
    catch (...)
      {
	g.remove_node(ret_val);
	throw;
      }
  }

/** @overload insert_in_graph */
  GT_Node * insert_in_graph(GT_Node_Type && info = GT_Node_Type())
  {
    GT_Node * ret_val = g.insert_node(std::move(info));

    try
      {
	insert(ret_val);

	return ret_val;
      }
    catch (...)
      {
	g.remove_node(ret_val);
	throw;
      }
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
    GT_Node ** ret_val = index.search(p);

    if (ret_val == NULL)
      return NULL;

    return *ret_val;
  }

/** 
  @brief Search a node according to the "info" information.

  @details Note that the search criterion is determined
  by the class parameter of Compare comparison.

  @param[in] info information that you want to perform the
  search (which depends on the parameter type of comparison
  Compare). 

  @return A pointer to the node if found or null otherwise.
*/
  GT_Node * search(const GT_Node_Type & info)
  {
    GT_Node dummy_node(info);
    GT_Node * dummy_node_ptr = &dummy_node;

    return search(dummy_node_ptr);
  }

/** @brief Deleted from the index the node with address "p". */
  void remove(GT_Node * p)
  {
    index.remove(p);
  }

/** 
  @brief Removes the node p from the graph and index.

  @param[in] p Pointer to the node to remove

  @throw domain_error If the node is inserted in the index (the
  possibly means that even in the graph).
*/
  void remove_from_graph(GT_Node * p) throw(std::exception, std::domain_error)
  {
    index.find(p); // Dispara excepción si p no está en el índice */
    index.remove(p);
    g.remove_node(p);
  }

/** @brief Deletes the index; all nodes are removed. */
  void clear_index()
  {
    index.empty();
  }

/** @brief Insert all nodes of the graph in the index. */
  void build_index()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      {
	GT_Node * p = it.get_curr();

	if (search(p) != p)
	  insert(p);
      }
  }

/** @brief Removes all nodes of the graph and the index. */
  void clear_graph()
  {
    clear_index();
    g.clear_graph();
  }

private: 

  void init()
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next())
      insert(it.get_curr());
  }

public:

/** 
  @brief Creates an index from the nodes inserted in the graph.

  @param[in] __g Graph
  @param[in] build_index Whether to build the index
  during construction

  @throw bad_alloc If there is insufficient memory.
*/
  IndexNode(GT & __g, SN && __sn = SN()) : g(__g), sn(__sn)
  {
    init();
  }

  IndexNode(GT & __g, SN & __sn) : g(__g), sn(__sn) 
  {
    init();
  }

/** @brief Returns the number of arcs that contains the index. */
  size_t size() const { return index.size(); }
};

}

# endif
