
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
  @file tpl_listQueue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_LISTQUEUE_H
# define TPL_LISTQUEUE_H

# include <tpl_snode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Instrumented with tail nodes simply linked lists.

  @details ListQueue<T> implements a queue of nodes simply linked
  data containing generic type T.

  @param T Data type contained in each node of the queue.

  @see DynListQueue

  @ingroup Secuencias

  @note The dicola can be implemented directly by
  Dlist or DynDlist.
*/
    template <typename T> 
class ListQueue 
{
public:

/** @brief Type of node. */
    typedef Snode<T>  Node;

/** @brief The set type on which iterates. */
    typedef ListQueue Set_Type;

/** @brief The type of item returning get_current() */
    typedef Node *    Item_Type;

private:

  Node * rear_ptr;
  size_t num_nodes;

public:

/** @brief Constructor. */
  ListQueue() : rear_ptr(NULL), num_nodes(0) { /* Empty */ }

/** 
  @brief Inserts a node in the queue.

  @details Inserted in the back of the queue the "node" node.

  @param[in] node Pointer to the node to insert.
*/
  void put(Node * node)
  {
    assert(node->is_empty());

    if (num_nodes > 0)
      rear_ptr->insert_next(node);
    num_nodes++;
    rear_ptr = node;
  }

/** 
  @brief Removes the node front of the queue.

  @return A pointer to the node removed.
  
  @throw underflow_error If the queue is empty
*/
  Node * get() throw(std::exception, std::underflow_error)
  {
    if (num_nodes == 0)
      throw std::underflow_error("stack is empty");

    Node * ret_val = rear_ptr->remove_next(); 
    num_nodes--;
    if (num_nodes == 0)
      rear_ptr = NULL;

    return ret_val;
  }

/** 
  @brief Removes the node front of the queue.

  @return A pointer to the node removed.

  @throw underflow_error If the queue is empty
*/
  Node * front() const throw(std::exception, std::underflow_error)
  {
    if (num_nodes == 0)
      throw std::underflow_error("stack is empty");

    return rear_ptr->get_next();
  }

/** 
  @brief Eliminates the node rear of the tail.

  @return A pointer to the node removed.

  @throw underflow_error If the queue is empty
*/
  Node * rear() const throw(std::exception, std::underflow_error)
  {
    if (num_nodes == 0)
      throw std::underflow_error("stack is empty");

    return rear_ptr;
  }

/** @brief Returns the number of elements that have the tail. */
  size_t size() const { return num_nodes; }

/** @brief Returns true if the queue is empty. */
  bool is_empty() const { return num_nodes == 0; }

  void swap(ListQueue & q)
  {
    std::swap(num_nodes, q.num_nodes);
    std::swap(rear_ptr, q.rear_ptr);
  }
};

} /* End namespace Aleph */

# endif /* TPL_LISTQUEUE_H */
