
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
  @file tpl_dnode.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DNODE_H
# define TPL_DNODE_H

# include <ahFunction.H>
# include <dlink.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Node belonging to a circular doubly linked list.

  @param T Data type that stores the node.
  
  @ingroup Secuencias
  
  @see Dnode::Iterator
*/ 
template <typename T> class Dnode : public Dlink 
{
private:

  mutable T data;

public:

/** @brief  Returns a reference pointer to the next node "this" */
  Dnode<T> *& get_next() { return (Dnode<T>*&) next; }

/** @brief  Returns reference to previous node pointer to "this" */
  Dnode<T> *& get_prev() { return (Dnode<T>*&) prev; }

/** @brief  Eliminates before "this" and returns its address */
  Dnode<T>* remove_prev() { return (Dnode<T>*) Dlink::remove_prev(); }

/** @brief  Eliminates successor to "this" and returns its address */
  Dnode<T>* remove_next() { return (Dnode<T>*) Dlink::remove_next(); }

/** @brief  Returns reference to the first node pointer "this" */
  Dnode<T> *& get_first() { return reinterpret_cast<Dnode<T>*&>(next); } 

/** @brief  Returns reference to the last node pointer "this" */
  Dnode<T> *& get_last() { return reinterpret_cast<Dnode<T>*&> (prev); }

/** @brief  Removes the last element of the "this" list and returns its address */  
  Dnode<T>* remove_last() 
  {
    return static_cast<Dnode<T>*>(Dlink::remove_prev()); 
  }

/** @brief  Removes the first element of the "this" list and returns its address */  
  Dnode<T>* remove_first() 
  {
    return static_cast<Dnode<T>*>(Dlink::remove_next()); 
  }
  
  Dnode() { /* Nothing */ }

/** @brief Build node with _data data */
  Dnode(const T & item) : data(item) 
  {
    /* Empty */
  }

  Dnode(T && item)
  {
    std::swap(data, item);
  }

  Dnode & swap(Dnode & p)
  {
    Dlink::swap(p);
    std::swap(data, p.data);
    return *this;
  }

  Dnode & swap(Dnode * p)
  {
    return swap(*p);
  }

/** @brief Node copy constructor.
Only the data is copied */
  Dnode(const Dnode & node) : Dlink(), data(node.data) 
  {
    /* Empty */ 
  }

/** @brief Rvalue node copy constructor.
Only the data is copied */
  Dnode(Dnode && node) : data(std::move(node).data) 
  {
    /* Empty */ 
  }

  Dnode & operator = (const Dnode & p)
  {
    if (&p == this)
      return *this;
  
    data = p.data;

    return *this;
  }

  Dnode & operator = (Dnode && p)
  {
    std::swap(data, p.data);
    return *this;
  }

/** @brief Assigned to data node the _data value */
  Dnode & operator = (const T & _data)
  {
    data = _data;
    return *this;
  } 

/** @brief Assigned to data node the _data value */
  Dnode & operator = (T && _data)
  {
    data = std::move(_data);
    return *this;
  } 
/** @brief Returns reference to data contained in the node */
  T & get_data() { return data; }

/** @brief Datatype */
  typedef T key_type;

  T & get_key() { return data; }

/** @brief Export data type that contains the node. */
  typedef T dnode_type;

  static Dnode * data_to_node(T & data)
  {
    Dnode * zero = 0;
    long offset = (long) &(zero->data);
    unsigned long  addr = (unsigned long) (&data);
    return (Dnode*) (addr - offset);
  }

/** 
  @brief Iterator over the nodes of a double circular list

  @ingroup Secuencias
*/
  class Iterator : public Dlink::Iterator
  {
  public:
    
/** @brief Set type on which is iterated. */
    typedef Dnode<T> Set_Type;
    
/** @brief Type element containing */
    typedef Dnode<T>* Item_Type;

    Iterator() { /* Empty */ }

/** @brief Iterator instance in list whose head node is head_ptr */
    Iterator(Dlink * head_ptr) : Dlink::Iterator(head_ptr) { /* Empty */ }

/** @brief Iterator instance in list whose head node is head_ptr */
    Iterator(Dlink & head) : Dlink::Iterator(&head) { /* Empty */ }

/** @brief Iterator instance in list whose head node is head_ptr y
coloca el elemento actual en el apuntado por curr_ptr */
    Iterator(Dlink * head_ptr, Dnode * curr_ptr) 
      : Dlink::Iterator(head_ptr, curr_ptr)
    {
      /* Empty */
    }

/** @brief Assigned to the targeted list iterator by head */
    Iterator & operator = (Dnode * head)
    {
      Dlink::Iterator::operator = (head);
      return *this;
    }

/** @brief Returns pointer to the current node */
    Dnode<T> * get_current() const 
    {
      return static_cast<Dnode<T>*>(this->Dlink::Iterator::get_current()); 
    }
/** @brief Returns pointer to the current node */
    Dnode<T> * get_curr() const 
    {
      return Dnode<T>::Iterator::get_current(); 
    }

/** @brief Removes the current item and returns its address.
The iterator is positioned in the following the deleted */
    Dnode * del() { return static_cast<Dnode*>(Dlink::Iterator::del()); }
  };
};

}

# endif /* TPL_DNODE_H */
