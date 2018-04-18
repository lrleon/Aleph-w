
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
  @file tpl_dynListStack.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNLISTSTACK_H
# define TPL_DYNLISTSTACK_H

# include <htlist.H>

namespace Aleph {

/** 
  @brief Dynamic stack elements of T type.

  @details DynListStack<T> modeled a dynamics stack of elements of some
  generic T type.

  @param T Type of elements contained in the stack.

  @see ArrayStack FixedStack

  @ingroup Secuencias
*/
    template <typename T> 
class DynListStack : private DynList<T>
{
  size_t num_items;

public:

  DynListStack() : num_items(0) { /* Empty */ }

  const size_t & size() const { return num_items; }

  bool is_empty() const { return this->DynList<T>::is_empty(); }

  void swap(DynListStack & s)
  {
    this->DynList<T>::swap(s);
    std::swap(num_items, s.num_items);
  }

  DynListStack(const DynListStack & s) : num_items(0)
  {
    s.for_each([this] (const T & d) { this->append(d); ++num_items; });
  }

  DynListStack(DynListStack && s) : num_items(0)
  {
    swap(s);
  }

  DynListStack & operator = (const DynListStack & s)
  {
    if (this == &s)
      return *this;

    num_items = s.num_items;
    (DynList<T>&) *this = s;
    return *this;
  }

  DynListStack & operator = (DynListStack && s)
  {
    this->swap(s);
    return *this;
  }

/** @brief The set type. */
  typedef DynListStack Set_Type;

/** @brief The data type that houses the set. */
  typedef T Item_Type;

/** 
  @brief Returns a reference to the top of the stack.

  @details Returns a reference to the modifiable element at the top
  stack.

  @return A modified reference to the element at the top.

  @throw underflow_error If the stack is empty.
*/
  const T & top() const 
  {
    return this->get_first();
  }

/** 
  @brief Insert onto the stack a copy of __data value.

  @details push(__data) insert onto the stack a copy of the value contained
  in __data parameter.

  @param[in] item Element to insert.
  
  @return A modified reference to the top of the stack (which contains
  the same __data value).
  
  @throw bad_alloc If there is insufficient memory.
*/
  T & push(const T & item) 
  {
    T & ret_val = this->insert(item);
    ++num_items;

    return ret_val;
  }

  T & push(T && item) 
  {
    T & ret_val = this->insert(std::move(item));
    ++num_items;

    return ret_val;
  }

/** @overload push() */
  T & put(const T & item) { return push(item); }

  T & put(T && item) { return push(std::move(item)); }

/** 
  @brief Removes the element of the top.

  @details pop() removes the element at the top and returns a
  copy.

  @return A copy of the deleted item.

  @throw underflow_error If the stack is empty.
*/
  T pop() throw(std::exception, std::underflow_error)
  {
    T retVal = this->remove_first();
    --num_items;

    return retVal;
  }

/** @overload pop() */
  T get() { return pop(); }

/** @brief Empty stack;
all items are deleted */
  void empty()
  {
    this->DynList<T>::empty();
    num_items = 0;
  }

/** @brief Destroyer. */
  virtual ~DynListStack()
  {
    empty();
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

} // End namespace Aleph */

# endif /* TPL_DYNLISTSTACK_H */
