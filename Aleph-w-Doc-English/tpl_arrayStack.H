
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
  @file tpl_arrayStack.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_ARRAYSTACK_H
# define TPL_ARRAYSTACK_H

# include <iostream>
# include <aleph.H>
# include <tpl_memArray.H>

using namespace std;

using namespace Aleph;

namespace Aleph {

/** 
  @brief Implanted stack with static arrays and verification number elements.

  @details ArrayStack<T> implements a stack of elements of type T with
  a maximum of dim items. For this, all check operations performed on 
  the number of elements and exceptions are triggered if the stack is 
  exceeded or try to query or delete an empty stack.

  A stack is instrumented with an array much faster than
  an instrumented with linked lists. 

  The internal array is static, saving calls to the handler
  memory, but it makes his delicate use as much elements is greater. 
  Usually it should be used to small stacks.

  @param T Element type that houses the stack
  equivalent to the size of the array.    
  @param dim Dimension of internal array.

  @see FixedStack DynListStack

  @ingroup Secuencias

  @note If you have the certainty of the correctness, then use the
  FixedStack type, which is faster.
*/
    template <typename T> 
class ArrayStack
{
  MemArray<T> array;

public:

  ArrayStack(size_t dim = 8) : array(dim) { /* Empty */ }

  ArrayStack(const ArrayStack & s) : array(s.array) { /* Empty */ }

  ArrayStack(ArrayStack && s) : array(std::move(s.array)) { /* Empty */ }

  ArrayStack & operator = (const ArrayStack & s)
  {
    if (this == &s)
      return *this;

    array = s.array;

    return *this;
  }

  void swap(ArrayStack & s)
  {
    std::swap(*this, s);
  }

  ArrayStack & operator = (ArrayStack && s)
  {
    array.swap(s.array);
    return *this;
  }

/** 
  @brief Instert into the stack a copy of data.

  @details push(data) insert into the stack a copy of the value of data.

  @param[in] data Data inserted into the stack.
  
  @return A reference to the newly inserted data and content
  the stack. 
  
  @throw overflow_error If the capacity is already full.
*/
  T & push(const T & data) throw(std::exception, std::overflow_error)
  {
    return array.put(data);
  }

  T & push(T && data) throw(std::exception, std::overflow_error)
  {
    return array.put(std::move(data));
  }

/** 
  @brief Push n spaces with undefined value on the stack.

  @details pushn(n) is equivalent to an element value push
  undetermined. The idea is to remove space in the stack then
  access it in constant time.

  The operation takes constant time.

  @param[in] n Number of spaces to be separated.

  @return A reference to the top of the stack (the value is indeterminate).

  @throw overflow_error If the capacity is already full.
*/
  T & pushn(const size_t & n = 1) throw(std::exception, std::overflow_error)
  {
    array.putn(n);
    return array.last();
  }

/** 
  @brief Remove an element of the stack.

  @details Extracts the element at the top of the stack.

  @return Copies the item removed of the stack.
  
  @throw underflow_error If the stack is empty.
*/
  T pop() throw(std::exception, std::underflow_error)
  {
    return array.get(1);
  }

/** 
  @brief Remove N elements from the stack.

  @details popn(n) remove the n elements from the top.

  The operation takes constant time.
  
  @param[in] n Number of items to take.
  
  @return A copy of the last element removed.
  
  @throw underflow_error If the stack is empty.
*/
  T popn(const int & n) throw(std::exception, std::underflow_error)
  {
    return array.get(n);
  }

/** @brief Returns a reference to the element at the top of the stack. */
  T & top() throw(std::exception,std::underflow_error)
  {
    return array.last();
  }

  T & base() 
  {
    return array.first();
  }

  const T & top() const throw(std::exception,std::underflow_error)
  {
    return array.last();
  }

/** 
  @brief Returns a reference to the element at position i after the top. 
      
  @details top(i) access the element of the stack located i positions
  after the top.

  @param[in] i Number of positions after the top.
  
  @return A reference to the element at position i after
  the top.
  
  @throw underflow_error If i exceeds the number of elements
  stack.
*/
  T & top(const int & i) throw(std::exception, std::underflow_error)
  {
    return array.access(array.size() - i - 1);
  }

  const T & top(const int & i) const throw(std::exception, std::underflow_error)
  {
    return array.access(array.size() - i - 1);
  }

/** @brief Empty all the elements of the stack. */
  void empty() { array.empty(); }

/** @brief Returns true if the stack is empty. */
  bool is_empty() const { return array.size() == 0; }

/** @brief Returns the number of elements contained in the stack. */
  const size_t & size() const { return array.size(); }

  const size_t & capacity() const { return array.capacity(); }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    return array.traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return array.traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return array.traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return array.traverse(operation);
  }

  Functional_Methods(T);

  Generic_Items(T);
};

/** 
  @brief Stack Implanted with static arrays and without verification
  number of elements.

  @details FixedStack<T> implements a stack of elements of type T with
  a maximum of dim items. For this type, no check operation performed 
  on the number of elements. For that reason FixedStack is faster than 
  ArrayStack.

  A stack is instrumented with an array much faster than
  an instrumented with linked lists.

  The internal array is static, saving calls to the handler memory, 
  but it makes his delicate use as much elements is greater. 
  Usually it should be used to small stacks.

  @param T Element type that houses the stack
  equivalent to the size of the array.

  @see ArrayStack DynListStack
  
  @ingroup Secuencias
  
  @note If you do not have the certainty of the correctness, then use the
  ArrayStack type, which is slightly slower, but that performs checks 
  positive and negative overflow.
*/
    template <typename T> 
class FixedStack
{
  T *    array = NULL; 
  size_t head;
  size_t dim;

public:

/** @brief Constructor. */
  FixedStack(size_t d = 512) 
    : array(new T [d]), head(0), dim(d) { /* Empty */ }

  ~FixedStack()
  {
    if (array)
      delete [] array;
  }

  FixedStack(const FixedStack & s) 
    : array(new T [s.dim]), head(s.head), dim(s.dim)
  {
    for (int i = 0; i < head; ++i)
      array[i] = s.array[i];
  }

  void swap(FixedStack & s)
  {
    std::swap(array, s.array);
    std::swap(head, s.head);
    std::swap(dim, s.dim);
  }

  FixedStack(FixedStack && s) 
    : array(NULL), head(0), dim(0)
  {
    swap(s);
  }

  FixedStack & operator = (const FixedStack & s)
  {
    if (this == &s)
      return *this;

    T * ptr = new T [s.dim];
    if (array)
      delete [] array;
    array = ptr;
    for (size_t i = 0; i < s.head; ++i)
      array[i] = s.array[i];
    head  = s.head;
    dim   = s.dim;

    return *this;
  }

  FixedStack & operator = (FixedStack && s)
  {
    swap(s);
    return *this;
  }

/** 
  @brief Instert into the stack a copy of data.

  @details push(data) insert into the stack a copy of the value of data.

  @param[in] data Data inserted into the stack.
  
  @return A reference to the newly inserted data and content
  the stack. 
*/
  T & push(const T & data) 
  {
    assert(head < dim );

    array[head++] = data;
    return array[head - 1];
  }

  T & push(T && data) 
  {
    assert(head < dim );

    std::swap(array[head++], data);
    return array[head - 1];
  }

/** 
  @brief Push n spaces with undefined value on the stack.

  @details pushn(n) is equivalent to an element value push
  undetermined. The idea is to remove space in the stack then
  access it in constant time. 

  The operation takes constant time.

  @param[in] n Number of spaces to be separated.
  
  @return A reference to the top of the stack (the value is indeterminate).
*/
  T & pushn(const size_t & n = 1) 
  {
    assert(head + n <= dim);

    head += n;
    return array[head - 1];
  }

/** 
  @brief Remove an element of the stack.

  @details Extracts the element at the top of the stack.

  @return Copies the item removed of the stack.
*/
  T pop() 
  {
    assert(head > 0);

    return std::move(array[--head]);
  }

/** 
  @brief Remove N elements from the stack.

  @details popn(n) remove the n elements from the top.

  The operation takes constant time.
  
  @param[in] n Number of items to take.
  
  @return A copy of the last element removed.
*/
  T popn(const int & n) 
  {
    assert((int (head - n)) >= 0);

    head -= n;
    return std::move(array[head]);
  }

/** @brief Returns a reference to the element at the top of the stack. */
  T & top() 
  {
    assert(head > 0);

    return array[head - 1]; 
  }

  T & top() const 
  {
    assert(head > 0);

    return array[head - 1]; 
  }

  T & base() { return array[0]; }

/** 
  @brief Returns a reference to the element at position i after the top.
      
  @details top(i) access the element of the stack located i positions
  after the top.

  @param[in] i Number of positions after the top.
  
  @return A reference to the element at position i after
  the top. 
*/
  T & top(const int & i) 

  {
    assert(i < head);

    return array[head - i - 1];   
  }

/** @brief Returns true if the stack is empty. */
  bool is_empty() const { return head == 0; }

/** @brief Empty all the elements of the stack. */
  void empty() { head = 0; }

/** @brief Returns the number of elements contained in the stack. */
  const size_t & size() const { return head; }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    for (int i = 0; i < head; i++)
      if (not operation(array[i]))
	return false;

    return true;
  }

  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return const_cast<FixedStack*>(this)->traverse(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse<Operation>(operation);
  }

  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return traverse<Operation>(operation);
  }

  Functional_Methods(T);

  Generic_Items(T);
};

} /* End namespace Aleph */

# endif /* TPL_ARRAYSTACK_H */
