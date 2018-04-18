
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
  @file tpl_arrayQueue.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_ARRAYQUEUE_H
# define TPL_ARRAYQUEUE_H

# include <ahAssert.H>
# include <ahDefs.H>
# include <tpl_memArray.H>

using namespace Aleph;

namespace Aleph {

/** 
  @brief Stack implemented with static arrays and verification number of items.  

  @details ArrayQueue<T> implements a queue of elements of type T with
  a maximum of dim elements specified in time construction. For this type, 
  all check operations performed on the number of elements and exceptions 
  are triggered if the queue is exceeded, or try to query or delete an empty 
  queue.

  A queue is instrumented with an array is much faster than an instrumented 
  with linked lists.

  The internal array moves in memory.

  @param T Element type that hosts the queue.
  equivalent to the size of the array.
  @param dim Dimension of internal array.

  @see FixedQueue DynListQueue

  @ingroup Secuencias

  @note If you have the certainty of the correctness, then use the
  FixedQueue type, which is faster.
*/
    template <typename T> 
class ArrayQueue : public MemArray<T>
{
private:

  size_t front_index; /* Items are extracted from this point */
  size_t rear_index;  /* New item are inserted by this point */

  void increase_index(size_t & i, const size_t inc = 1) const
  {
    i += inc;
    i %= this->dim;
  }

  T & rear_item(const size_t i = 0) 
  {
    return this->access((size_t) (rear_index - i - 1) % this->dim);
  }

  const T & rear_item(const size_t i = 0) const
  {
    return this->access((size_t) (rear_index - i - 1) % this->dim);
  }

public:

  void swap(ArrayQueue & q)
  {
    this->MemArray<T>::swap(q);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
  }

/** 
  @brief Builder that specifies maximum capacity.

  @details Build a line with a capacity of \f$ 2^{tp}\f$ elements.

  @details Build a queue with a capacity of \f$ 2^{tp}\f$ elements.

  @param[in] tp Power of two of the desired capacity; the capacity
  will be of \f$ 2^{tp}\f$ elements.
  
  @throw bad_alloc If there is insufficient memory.
*/
  ArrayQueue(const size_t sz = 8) 
    : MemArray<T>(sz), front_index(0), rear_index(0)
  {
    /* Empty */
  }

  ArrayQueue(const ArrayQueue & q)
    : MemArray<T>(q), front_index(q.front_index), rear_index(q.rear_index)
  {

  }

  ArrayQueue(ArrayQueue && q)
    : MemArray<T>(std::move(q)), 
      front_index(q.front_index), rear_index(q.rear_index)
  {
    /* Empty */
  }

  ArrayQueue & operator = (const ArrayQueue & q)
  {
    if (this == &q)
      return *this;

    static_cast<MemArray<T>&>(*this) = q;
    
    front_index = q.front_index;
    rear_index = q.rear_index;

    return *this;
  }

  ArrayQueue & operator = (ArrayQueue && q)
  {
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    static_cast<MemArray<T>*>(this)->swap(q);
    return *this;
  }

private:

  T & complete_put()
  {
    T & ret_val = this->access(rear_index);
    increase_index(rear_index);
    ++this->n;
    return ret_val;
  }

public:

/** 
  @brief Inserts an item into the queue.

  @details put(item) inserts a copy of item into the queue.

  @param[in] Item Element to insert.
  
  @return A reference writable to the newly inserted element.
  
  @throw overflow_error If the queue is full.
*/
  T & put(const T & item) throw(std::exception, std::bad_alloc)
  {
    if (this->expand(front_index))
      {
	front_index = 0;
	rear_index = this->n; 
      }
    this->access(rear_index) = item;
    return complete_put();
  }

  T & put(T && item) throw(std::exception, std::bad_alloc)
  {
    if (this->expand(front_index))
      {
	front_index = 0;
	rear_index = this->n; 
      }
    this->access(rear_index) = std::move(item);
    return complete_put();
  }

/** 
  @brief Insert N spaces away in the queue.

  @details putn(n) n spaces away from the insertion into the queue. This
  it is equivalent to that they had n inserts made straight, with goodness 
  that the operation takes time constant.

  @details putn(n) Insert n spaces away in the queue. This
  it is equivalent to that they had inserts made n
  straight, with goodness that the operation takes time
  constant.

  The values of the inserted spaces are indeterminate.
  They can be accessed through the operations front() and rear().

  @param[in] n How many spaces to insert.
  
  @return A modified reference to the last space in the back of 
  the queue.
  
  @throw overflow_error If the total amount of elements after
  to insert n spaces exceeds capacity.
*/
  T & putn(size_t sz) throw(std::exception, std::bad_alloc)
  {
    const int avail_n = this->dim - this->n;
    if (avail_n > sz)
      {
	sz -= this->dim - this->n;
	this->n = this->dim;
	this->expand(front_index);
      }

    increase_index(rear_index, sz);
    this->n += sz;
    return rear_item();
  }

/** 
  @brief Removes the item from the front of the queue and returns a copy.

  @details get() It removes the queue on the front element and returns
  a copies the item freshly removed.

  @return A copy of the newly removed element.
  
  @throw underflow_error If i is out of range.
*/
  T get() throw(std::exception, std::underflow_error, std::bad_alloc)
  {
    if (this->n == 0)
      throw std::underflow_error ("queue is empty");

    T ret_val = std::move(this->access(front_index)); 
    this->n--;
    increase_index(front_index);

    if (this->contract(front_index))
      {
	front_index = 0;
	rear_index = this->n;
      }

    return ret_val;
  }

/** 
  @brief Remove i elements from the queue and returns the element located
  the front.

  @details getn(n) take i elements from the queue in constant time.

  @param[in] i Number of items to take.
  
  @return A modified reference to the element at the front.
  
  @throw underflow_error If i is out of range.
*/
  T & getn(const size_t i) throw(std::exception, std::underflow_error)
  {
    if (i >= this->n)
      throw std::underflow_error ("queue is empty");

    this->n -= i;
    increase_index(front_index, i);

    if (this->contract(front_index))
      {
	front_index = 0;
	rear_index = this->n;
      }

    return this->access(front_index);
  } 

/** 
  @brief Access the ith element from the front of the queue.

  @details front(i) access to the i-th position from the front of the queue. 
  This is equivalent to meet the i-th oldest item.

  @param[in] i Access position.

  @return A reference modifiable to the ith element from
  the front of the queue.

  @throw range_error If i is out of range.
*/
  T & front(const size_t i = 0) throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of front out of range");

    return this->access((front_index + i) % this->dim);
  }

  const T & front(const size_t i = 0) const
    throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of front out of range");

    return this->access((front_index + i) % this->dim);
  }

/** 
  @brief Access the i-th element from the back of the queue.

  @details rear(i) access to the i-th position from the back of the queue.
  This is equivalent to knowing the ith element younger.
  @param[in] i Access position.
  
  @return A modifiable reference to the ith element from
  the back of the queue.
  
  @throw range_error If i is out of range.
*/
  T & rear(const size_t i = 0) throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of rear out of range");

    return rear_item(i);
  }

  const T & rear(const size_t i = 0) const
    throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of rear out of range");

    return rear_item(i);
  }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    if (this->n == 0)
      return true;

    for (size_t i = front_index; i != rear_index; ++i)
      if (not operation(this->access(i)))
	return false;
    return true;
  }

  template <class Operation>
  bool traverse(Operation & operation) const 
  {
    return const_cast<ArrayQueue*>(this)->traverse(operation);
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

/** 
  @brief Queue implemented with static arrays without verification
  number of elements.

  @details FixedQueue<T> implements a queue of elements of type T with
  a maximum of dim items. For this type, no check operation performed 
  on the number of elements. For that reason FixedQueue is faster than 
  ArrayQueue.

  A queue is instrumented with an array much faster than
  an instrumented with linked lists.
  The internal array moves in memory.

  @param T Element type that hosts the queue.
  equivalent to the size of the array.

  @see ArrayQueue DynListQueue
  
  @ingroup Secuencias
  
  @note If you do not have the certainty of the correctness, then use the
  ArrayQueue type, which is slightly slower, but that performs
  checks positive and negative overflow.
*/
    template <typename T> 
class FixedQueue
{
private:

  size_t two_pow;
  size_t dim;
  T *    array;
  size_t front_index; /* Index of oldest inserted item */
  size_t rear_index;
  size_t mask;
  size_t num_items; 

  void increase_index(size_t & i, const size_t inc = 1) const
  {
    assert( ((i + inc)%dim) == ((i+ inc)&mask) );

    i += inc;
    i &= mask;  
  }

  T & rear_item(const size_t i = 0) 
  {
    assert(static_cast<size_t>((rear_index - i - 1) & mask) == 
      (rear_index - i - 1)%dim);

    return array[static_cast<size_t> ((rear_index - i - 1) & mask)];
  }

  const T & rear_item(const size_t i = 0) const
  {
    assert(static_cast<size_t>((rear_index - i - 1) & mask) == 
      (rear_index - i - 1)%dim);

    return array[static_cast<size_t> ((rear_index - i - 1) & mask)];
  }

public:

  void swap(FixedQueue & q)
  {
    std::swap(two_pow, q.two_pow);
    std::swap(dim, q.dim);
    std::swap(array, q.array);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    std::swap(mask, q.mask);
    std::swap(num_items, q.num_items);
  }

/** 
  @brief Builder that specifies maximum capacity.

  @details Build a line with a capacity of \f$ 2^{tp}\f$ elements.

  @param[in] tp Power of two of the desired capacity; the capacity
  will be of \f$ 2^{tp}\f$ elements.
*/
  FixedQueue(const size_t & tp = 8)  
    : two_pow(tp), dim(1<<two_pow), array(NULL), front_index(0), 
      rear_index(0), mask(dim - 1), num_items(0)
  { 
    array = new T [dim];
  }

/** @brief Destroyer. */
  ~FixedQueue()
  {
    if (array != NULL)
      delete [] array;
  }

  FixedQueue(const FixedQueue & q) 
    : two_pow(q.two_pow), dim(q.dim), array(new T [dim]),
      front_index(q.front_index), rear_index(q.rear_index), mask(q.mask), 
      num_items(q.num_items)
  {
    for (size_t i = front_index; i != rear_index; ++i)
      array[i] = q.array[i];
  }

  FixedQueue(FixedQueue && q) : FixedQueue()
  {
    swap(q);
  }

  FixedQueue & operator = (const FixedQueue & q)
  {
    if (this == &q)
      return *this;

    if (array != NULL)
      delete [] array;

    new (this) FixedQueue(q);
    return *this;
  }

  FixedQueue & operator = (FixedQueue && q)
  {
    swap(q);
    return *this;
  }

/** 
  @brief Inserts an item into the queue.

  @details put(item) inserts a copy of item into the queue.

  @param[in] item Element to insert.
  
  @return A modified reference to the newly inserted element.
*/
  T & put(const T& item) throw(std::exception, std::overflow_error)
  {
    assert(num_items < dim);
    array[rear_index] = item;
    T & ret_val = array[rear_index];
    increase_index(rear_index);
    num_items++;
    return ret_val;
  }

  T & put(T && item) throw(std::exception, std::overflow_error)
  {
    assert(num_items < dim);
    array[rear_index] = std::move(item);
    T & ret_val = array[rear_index];
    increase_index(rear_index);
    num_items++;
    return ret_val;
  }

/** 
  @brief Set aside n insert spaces in the queue.

  @details putn(n) n spaces away from the insertion into the queue. This
  it is equivalent to that they had n inserts made straight, with goodness 
  that the operation takes time constant.

  The values of the inserted spaces are indeterminate.
  They can be accessed through the operations front() and rear().

  @param[in] n How many spaces to insert.

  @return A modified reference to the last space in the back of the queue.
*/
  T & putn(const size_t n) throw(std::exception, std::overflow_error)
  {
    assert(num_items + n < dim);
    increase_index(rear_index, n);
    num_items += n;
    return rear_item();
  }

/** 
  @brief Removes the item from the front of the queue and returns a copy.

  @details get() on the queue removes the front element and returns an 
  copies the item freshly removed.

  @return A copy of the newly removed element.
*/
  T get() throw(std::exception, std::underflow_error)
  {
    assert(num_items > 0);
    num_items--;
    T ret_val = std::move(array[front_index]);
    increase_index(front_index);
    return ret_val;
  }

/** 
  @brief Remove n elements from the queue and returns the element at
  the front.

  @details getn(n) takes n elements of the queue in constant time.

  @param[in] n Number of items to take.
  
  @return A modified reference to the element at the  
  front after removing the n inputs
*/
  T & getn(const size_t n) throw(std::exception, std::underflow_error)
  {
    assert(num_items >= n);
    num_items -= n;
    increase_index(front_index, n);
    return array[front_index];
  } 

/** 
  @brief Access the ith element from the front of the queue. 

  @details front(i) access to the i-th position from the front of the queue. 
  This is equivalent to meet the i-th oldest item.

  @param[in] i Access position.
  
  @return A reference modifiable to the ith element from
  the front of the queue.
  
  @throw range_error If i is out of range.
*/
  T & front(const size_t i = 0) throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return array[front_index + i];
  }

  const T & front(const size_t i = 0) const
    throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return array[front_index + i];
  }

/** 
  @brief Access the i-th element from the back of the queue.

  @details rear(i) access to the i-th position from the back of the queue.
  This is equivalent to knowing the ith element younger.
  @param[in] i Access position.

  @return A modifiable reference to the ith element from
  the back of the queue.
*/
  T & rear(const size_t i = 0) throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return rear_item(i);
  }

  const T & rear(const size_t i = 0) const 
    throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return rear_item(i);
  }

/** @brief Returns the number of elements in the queue. */
  const size_t & size() const { return num_items; }

/** @brief Returns true if the queue is empty. */
  bool is_empty() const { return num_items == 0; }

/** @brief Returns the queue capacity (maximum number of items may contain). */
  const size_t & capacity() const { return dim; }

  template <class Operation>
  bool traverse(Operation & operation)
  {
    if (num_items == 0)
      return true;
    for (size_t i = front_index; i != rear_index; ++i)
      if (not operation(array[i]))
	return false;
    return true;
  }

  template <class Operation>
  bool traverse(Operation & operation) const 
  {
    return const_cast<FixedQueue*>(this)->traverse(operation);
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

# endif /* TPL_ARRAYQUEUE_H */
