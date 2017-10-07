
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
  @file tpl_dynListQueue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNLISTQUEUE_H
# define TPL_DYNLISTQUEUE_H

# include <iostream>
# include <htlist.H>

using namespace std;


namespace Aleph {

/** 
  @brief dynamic elements Queue generic type T.

  @details DynListQueue <T> implements a generic queue elements of type T.

  @param T Type elements which hosts the queue.
  
  @see DynDlist
  
  @ingroup Secuencias
*/
    template <typename T> 
class DynListQueue : private DynList<T>
{
  size_t num_items;

public:

  void swap(DynListQueue & q)
  {
    std::swap(num_items, q.num_items);
    this->DynList<T>::swap(q);
  }

  DynListQueue() : num_items(0) { /* Empty */ }

  DynListQueue(const DynListQueue & q) 
    : DynList<T>(q), num_items(q.num_items)
  {
    /* Empty */
  }

  DynListQueue(DynListQueue && q) 
  {
    swap(q);
  }

  DynListQueue & operator = (const DynListQueue & q)
  {
    if (this == &q)
      return *this;

    *static_cast<DynList<T>*>(this) = q;
    num_items = q.num_items;

    return *this;
  }

  DynListQueue & operator = (DynListQueue && q)
  {
    std::swap(num_items, q.num_items);
    this->DynList<T>::swap(q);
    return *this;
  }

  const size_t & size() const { return num_items; }

  bool is_empty() const { return this->DynList<T>::is_empty(); }

/** 
  @typedef The set type on which iterates. 
*/
  typedef DynListQueue Set_Type;

/** 
  @typedef The type of item returning get_current(). 
*/
  typedef T Item_Type;

/** 
  @brief Inserts an item into the queue.

  @details put(data) inserta en el trasero de la cola una copia de
  data.

  @param[in] data Elemento a ser insertado.
  
  @return Una referencia modificable al elemento trasero de
  la cola.
  
  @throw bad_alloc If there is insufficient memory.
*/
  T & put(const T & data) 
  {
    T & ret_val = this->append(data);
    ++num_items;
    return ret_val;
  }

  T & put(T && data) 
  {
    T & ret_val = this->append(std::move(data));
    ++num_items;
    return ret_val;
  }

/** 
  @brief Removes the element at the front of the queue.

  @details Removes the element at the front of the queue.

  @return A copy of the value removed.
  
  @throw underflow_error If the queue is empty.
*/
  T get() 
  {
    T ret_val = this->remove_first();
    --num_items;
    return ret_val;
  }
 
/** 
  @brief Returns a modified reference to the front of the queue. 
*/
  T & front() 
  {
    return this->get_first();
  }

/** 
  @brief Returns a modified reference to the back of the queue. 
*/
  T & rear() 
  {
    return this->get_last();
  } 

/** 
  @brief Returns a constant reference to the front of the queue. 
*/
  const T & front() const
  {
    return this->get_first();
  }

/** 
  @brief Returns a constant reference to the back of the queue. 
*/
  const T & rear() const 
  {
    return this->get_last();
  } 

  void empty()
  {
    this->DynList<T>::empty();
  }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    return this->DynList<T>::traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return this->DynList<T>::traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return this->DynList<T>::traverse(std::move(operation));
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
  {
    return this->DynList<T>::traverse(std::move(operation));
  }

  Functional_Methods(T);

  Generic_Items(T);
};

} /* End namespace Aleph */

# endif // TPL_DYNLISTQUEUE_H
