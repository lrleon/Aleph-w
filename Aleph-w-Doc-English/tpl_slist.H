
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
  @file tpl_slist.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_SLIST_H
# define TPL_SLIST_H

# include <ahDefs.H>
# include <tpl_snode.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief The singly-linked list node containing data of type T.

  @details Slist<T> is a list whose elements are nodes. Each node
  contains a data generic type T.

  @param T Data type that houses each node.

  @ingroup Secuencias
*/
template <typename T> class Slist : public Snode<T>
{

public:

  typedef Snode<T> Node;

/** @brief Empty constructor. */
  Slist() { /* Empty */ }
    
/** 
  @brief Insert a node after "this".

  @details insert_first(node) just insert the node after node
  "this". 

  @param[in] node Node to insert.
*/
  void insert_first(Node * node)
  {

    assert(node not_eq NULL);
    assert(node->is_empty());

    this->insert_next(node);
  }
    
/** 
  @brief Removes the first node in the list and returns its address.

  @details Removes the first node in the list.

  @throw underflow_error If the list is empty.
  
  @return A pointer to the node removed.
*/
  Node * remove_first() 

    throw(std::exception, std::underflow_error)

  {

    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->remove_next();
  }

/** @brief Returns the first node in the list. */
  Node * get_first() 

    Exception_Prototypes(std::underflow_error)

  {

    if (this->is_empty())
      throw std::underflow_error ("list is empty");

    return this->get_next(); 
  }
      
/** 
  @brief Iterator over list of simple nodes.

  @details This class defines an iterator on a singly linked list
  node.

  @ingroup Secuencias
  
  @see Slist
*/
  class Iterator
  {

  private:

    Slist * list;
    Node  * current;

  public:

/** @brief The set type on which iterates. */
    typedef Snode<T> Set_Type;
    
/** @brief The type of item returning get_current() */
    typedef Snode<T> * Item_Type;

/** 
  @brief Constructor.

  @details Constructs an iterator over the list of nodes "list".

  @param[in] _list The list on which you want to iterate.
*/
    Iterator(Slist & _list) : list(&_list), current(list->get_first()) {}

/** @brief Returns true if the iterator has the current node. */
    bool has_current() const { return current != list; }

/** 
  @brief Returns the current node on which the iterator is positioned.

  @return A pointer to the current item.

  @throw overflow_error If the iterator is not positioned on
  an element.
*/
    Node * get_current() 

      throw(std::exception, std::overflow_error)

    {

      if (not this->has_current())
        throw std::overflow_error ("");

      return current;
    }
        
/** 
  @brief Advances the iterator forward.

  @details Advances the iterator to the next element of the current.

  If the iterator is at the last element of the sequence,
  then this is in a state in which no current element.

  @throw overflow_error If the iterator is not an item current.
*/
    void next()

      throw(std::exception, std::overflow_error)

    {

      if (not this->has_current())
        throw std::overflow_error ("");

      current = current->get_next();
    }

/** @brief Place the iterator at the first item in the list. */
    void reset_first() { current = list->get_next(); }
    Iterator & operator = (Node * node)
    {
      if (this == node) 
        return *this;

      current = node;
      return *this;
    }
  };
};

} /* End namespace Aleph */

# endif /* TPL_SLIST_H */
