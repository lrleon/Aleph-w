 
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

# ifndef TPL_NODEPOOL_H
# define TPL_NODEPOOL_H

# include <tpl_arrayStack.H>

/**
  @file tpl_nodePool.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


/** 
  @brief   Set of nodes to refuse.

	@details The Node_Pool class is intended as a "buffer" of nodes
  any type of data (lists, trees, etc.) available to
  quick use avoiding calls to "new" and "delete" operators.

	The class manages a "n" parameter representing the maximum amount
  of backup nodes.
  
  @ingroup utils
*/
    template <class Node>
class Node_Pool
{

  FixedStack<Node*> node_pool;
  const size_t n;

public:

  Node_Pool(size_t __n = 100) : node_pool(__n), n(__n) { /* Empty */ }

/** @overload allocate() */
  Node * allocate()
  {
    return node_pool.is_empty() ? new Node : node_pool.pop();
  }

/** 
  @brief Aside memory for a node and assigns a value of key the "Key" type.

  @details allocate_node() checks for a node has previously section
  among n possible. If this is the case, then it chooses a
  between existing node assembly. Otherwise, the
  allocates memory for a new node using "new". In both cases the
  key "key" is assigned to the result to give get_key() invoked on
  node.

  @param[in] key The key to assign to the node.

  @throw bad_alloc If there is insufficient memory.
*/
  Node * allocate(const typename Node::key_type & key)
  {
    return new (allocate()) Node (key);
  }

  Node * allocate(typename Node::key_type && key)
  {
    return new (allocate()) Node (std::move(key));
  }  

/** 
  @brief Set free the "p" node.

  @details deallocate_node(p) delivery "p" node for eventual later
  reuse.

  Each time a node is set free by this method, it is saved in
  for later reuse a set of until a maximum
  n nodes. If the set is not full, then the node can
  be reused. Otherwise, if the array is full,
  then the node is set free by "delete".

  @note Keep in mind that if the node save it for later use 
  is not called the destroyer. For therefore, it is important 
  that p concerns a new node paragraph.

  @param[in] p Node to set free.
*/
  void deallocate(Node * p)
  {
    if (node_pool.size() == n)
      delete p;
    else
      node_pool.push(p);
  }

  ~Node_Pool()
  {
    while (not node_pool.is_empty())
      delete node_pool.pop();
  }
};

# endif // TPL_NODEPOOL_H
