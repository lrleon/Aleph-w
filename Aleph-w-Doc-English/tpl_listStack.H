
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
  @file tpl_listStack.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_LISTSTACK_H
# define TPL_LISTSTACK_H

# include <htlist.H>
# include <tpl_snode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Simply stack linked nodes.

  @details ListStack<T> modeled a stack of simple knots.

  In general, this is not intended for public use; it further serves
  basis for developing dynamic stack DynListStack

  @param T Type of housing elements each node of the list.

  @see DynListStack ArrayStack FixedStack 
  
  @ingroup Secuencias
*/
    template <typename T> 
class ListStack : public Snode<T>
{
  size_t num_nodes;

public:

/** @brief Node of a stack node. */
  typedef Snode<T> Node;

/** @brief Constructor. */
  ListStack() : num_nodes(0) {}

/** 
  @brief Inserts a node on the stack.

  @details Insert the "node" node on the stack.

  @param node Node to insert.
*/
  void push(Node * node) 
  { 
    ++num_nodes;
    this->insert_next(node); 
  }

/** 
  @brief Extract the node at the top of the stack.

  @details pop() extract from the stack the node in their top.

  @return The node removed.

  @throw underflow_error If the stack is empty.
*/
  Node * pop() throw(std::exception, std::underflow_error)
  {
    if (this->is_empty())
      throw std::underflow_error ("Stack underflow");

    --num_nodes;

    return this->remove_next();
  }

/** 
  @brief Returns the node at the top of the stack.

  @details Returns the node at the top of the stack.

  @return The node at the top of the stack.

  @throw underflow_error If the stack is empty.
*/      
  Node * top() 
  {
    if (this->is_empty())
      throw std::underflow_error ("Stack underflow");

    return static_cast<Node*>(this->get_next());
  }

/** @brief Returns true if the stack is empty. */
  bool is_empty() const { return Snode<T>::is_empty(); }

/** @brief Returns the amount of elements that have the stack. */
  const size_t & size() const { return num_nodes; }
};

} /* end namespace Aleph */

# endif /* TPL_LISTSTACK_H */
