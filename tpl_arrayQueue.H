
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

# ifndef TPL_ARRAYQUEUE_H
# define TPL_ARRAYQUEUE_H

# include <ahAssert.H>
# include <ahDefs.H>
# include <htlist.H>
# include <ah-dry.H>
# include <ah-args-ctor.H>
# include <tpl_dynDlist.H>
# include <tpl_memArray.H>

using namespace Aleph;

namespace Aleph {

  /** Queue implemented with a single dynamic array.

      `ArrayQueue<T>` represents a queue if elements of generic type `T`
      which is stored in single dynamic. In this queue, modifiying
      operations test arrays bounds.

      The array is allocated with `new` operator and dynamically grows
      according to the size.

      @ingroup Secuencias
      @note If you are sure of knowing the bounds, then use `FixedStack`,
      which is faster because it does not perform bound checks,
   */
    template <typename T> 
class ArrayQueue 
      : public MemArray<T>,
	public LocateFunctions<ArrayQueue<T>, T>,
	public FunctionalMethods<ArrayQueue<T>, T>,
	public GenericKeys<ArrayQueue<T>, T>,
	public EqualToMethod<ArrayQueue<T>>,
	public StlAlephIterator<ArrayQueue<T>>
{
private:

  size_t front_index; // items are extracted from this point
  size_t rear_index;  // new item are inserted by this point

  void increase_index(size_t & i, const size_t inc = 1) const noexcept
  {
    i += inc;
    i %= this->dim;
  }

  T & rear_item(const size_t i = 0) const noexcept
  {
    return this->access((size_t) (rear_index - i - 1) % this->dim);
  }

public:

  /// Swap `this` with `q`
  void swap(ArrayQueue & q) noexcept
  {
    this->MemArray<T>::swap(q);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
  }

      /** Construct a queue with capacity `sz`

          @param[in] sz initial capacity
	  @throw bad_alloc if there is no enough memory
       */
  ArrayQueue(const size_t sz = 8) 
    : MemArray<T>(sz), front_index(0), rear_index(0)
  {
    // empty
  }

  /// Copy constructor
  ArrayQueue(const ArrayQueue & q)
    : MemArray<T>(q), front_index(q.front_index), rear_index(q.rear_index)
  {

  }

  /// Move constructor
  ArrayQueue(ArrayQueue && q) 
    : MemArray<T>(std::forward<ArrayQueue>(q)), 
      front_index(q.front_index), rear_index(q.rear_index)
  {
    // empty
  }

  Special_Ctors(ArrayQueue, T);

  /// Copy assign
  ArrayQueue & operator = (const ArrayQueue & q)
  {
    if (this == &q)
      return *this;

    static_cast<MemArray<T>&>(*this) = q;
    
    front_index = q.front_index;
    rear_index = q.rear_index;

    return *this;
  }

  /// Move assign
  ArrayQueue & operator = (ArrayQueue && q)
  {
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    static_cast<MemArray<T>*>(this)->swap(q);
    return *this;
  }

private:

  T & complete_put() noexcept
  {
    T & ret_val = this->access(rear_index);
    increase_index(rear_index);
    ++this->n;
    return ret_val;
  }

public:

      /** Copy and put an item in the queue

          @param[in] item to put
          @return a modifiable reference to the copied item
	  @throw bad_alloc if there is no enough memory
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

  /// \overload put(const T & item)
  T & put(T && item) throw(std::exception, std::bad_alloc)
  {
    if (this->expand(front_index))
      {
	front_index = 0;
	rear_index = this->n; 
      }
    this->access(rear_index) = std::forward<T>(item);
    return complete_put();
  }

  /// \overload put(const T & item)
  T & append(const T & item) { return put(item); }

  /// \overload put(T && item)
  T & append(T && item) { return put(std::forward<T>(item)); }

  /// \overload put(const T & item)
  T & insert(const T & item) { return put(item); }

  /// \overload put(T && item)
  T & insert(T && item) { return put(std::forward<T>(item)); }

      /** Put in constant time `sz` empty entries to the stack.

          `putn(n)` puts `sz` entries to the stack in an functionally
          equivalent way to `sz` pushes, except that the operation takes
          constant time.

	  The maximum value of `sz` is bounded by `2*capacity() - size()`.

          @param[in] sz number of cells to push
          @return a modifiabñpe reference to the stack's top
          @throw overflow_error if `sz > 2*capacity() - size()`
	  @throw bad_alloc if there is no enough memory
       */
  T & putn(size_t sz) throw(std::exception, std::bad_alloc)
  {
    const size_t max_sz = 2*this->dim - this->n;
    if (sz > max_sz)
      throw std::overflow_error("Maximum putn size reached");

    const int avail_n = this->dim - this->n;
    if (avail_n <= sz)
      {
	sz -= this->dim - this->n;
	this->n = this->dim;
	this->expand(front_index);
      }

    increase_index(rear_index, sz);
    this->n += sz;
    return rear_item();
  }

  /** Remove the oldest item of the queue and return a copy.

      @return a copy of extracted item
      @throw underflow_error if the queue is empty
  */
  T get() throw(std::exception, std::underflow_error, std::bad_alloc)
  {
    if (this->n == 0)
      throw std::underflow_error ("queue is empty");

    T ret_val = std::forward<T>(this->access(front_index)); 
    this->n--;
    increase_index(front_index);

    if (this->contract(front_index))
      {
	front_index = 0;
	rear_index = this->n;
      }

    return ret_val;
  }

  /** Remove the i oldest items of the queue.

      @param[in] i number of items to remove
      @return a modifiable reference to the oldest remaining item
      @throw underflow_error if `i` is greater than the number of items
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

  /** Return the i-th oldest item of the queue.

      @param[in] i position to access
      @return a modifiable reference to the oldest i-th item of the queue
      @throw range_error if `i` is greater than the number of items
  */
  T & front(const size_t i = 0) const throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of front out of range");

    return this->access((front_index + i) % this->dim);
  }

  /** Return the i-th youngest item of the queue.

      @param[in] i position to access
      @return a modifiable reference to the i-th youngest item of the queue
      @throw range_error if `i` is greater than the number of items
  */
  T & rear(const size_t i = 0) const throw(std::exception, std::range_error)
  {
    if (i >= this->n) 
      throw std::range_error ("index of rear out of range");

    return rear_item(i);
  }

  /** Traverse all the elements from the youngest to the oldest and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    if (this->n == 0)
      return true;

    for (size_t i = 0, idx = front_index; i < this->n; increase_index(idx), ++i)
      if (not operation(this->access(idx)))
	return false;
    return true;
  }

  /// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation & operation) const 
  {
    return const_cast<ArrayQueue*>(this)->traverse(operation);
  }

/// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation && operation) const
  {
    return traverse<Operation>(operation);
  }

/// \overload traverse(Operation & operation) 
  template <class Operation>
  bool traverse(Operation && operation)
  {
    return traverse<Operation>(operation);
  }

  /** Simple iterator on elements of a queue.

      The visit order is from the youngest to the oldest item

      @ingroup Secuencias
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = ArrayQueue;

    Iterator(const ArrayQueue & q)
      : Base(q.ptr, q.dim, q.n, q.front_index, (q.rear_index - 1) % q.dim) {}
  };
};


  /** Very simple queue implemented with a contiguous array

      The capacity is given in construction time, but this is adjusted
      to the next two power immediately greater.

      This queue is the fastest of `Aleph-w` (\f$\aleph_\omega\f$), but
      take in account that no bound cheks are performed. You must be
      sure that you usage is correct.

      @see ArrayQueue DynListQueue
      @ingroup Secuencias
   */
    template <typename T> 
class FixedQueue 
      : public LocateFunctions<FixedQueue<T>, T>,
	public FunctionalMethods<FixedQueue<T>, T>,
	public GenericKeys<FixedQueue<T>, T>,
	public EqualToMethod<FixedQueue<T>>,
	public StlAlephIterator<FixedQueue<T>>
{
private:

  size_t dim;
  T *    array;
  size_t front_index; /* index of oldest inserted item */
  size_t rear_index;
  size_t mask;
  size_t num_items; 

  void increase_index(size_t & i, const size_t inc = 1) const noexcept
  {
    assert( ((i + inc)%dim) == ((i+ inc)&mask) );

    i += inc;
    i &= mask;  
  }

  T & rear_item(const size_t i = 0) const noexcept
  {
    assert(static_cast<size_t>((rear_index - i - 1) & mask) == 
      (rear_index - i - 1)%dim);

    return array[static_cast<size_t> ((rear_index - i - 1) & mask)];
  }

public:

  using Item_Type = T;

  void swap(FixedQueue & q) noexcept
  {
    std::swap(dim, q.dim);
    std::swap(array, q.array);
    std::swap(front_index, q.front_index);
    std::swap(rear_index, q.rear_index);
    std::swap(mask, q.mask);
    std::swap(num_items, q.num_items);
  }

  /// empty the queue
  void empty()
  {
    front_index = rear_index = num_items = 0;
  }

      /** Construct a queue of capacity the immediately two power of `d`

	  Te resulting capacity will be the first number such \f$2^k
	  \geq d\f$,

          @param[in] minimum capacity
       */
  FixedQueue(size_t d = 128)  
    : array(nullptr), front_index(0), rear_index(0), num_items(0)
  { 
    size_t k = log2(d);
    dim = is_power_of_2(d) ? d : 1 << ++k;
    mask = dim - 1;
    array = new T [dim];
  }
      
  ~FixedQueue()
  {
    if (array != nullptr)
      delete [] array;
  }

  /// Copy constructor
  FixedQueue(const FixedQueue & q) 
    : dim(q.dim), array(new T [dim]), front_index(q.front_index), 
      rear_index(q.rear_index), mask(q.mask), num_items(q.num_items)
  {
    for (size_t i = front_index; i != rear_index; ++i)
      array[i] = q.array[i];
  }

    /// Move constructor
  FixedQueue(FixedQueue && q) noexcept : FixedQueue() 
  {
    swap(q);
  }

  Special_Ctors(FixedQueue, T);

  /// Copy assign
  FixedQueue & operator = (const FixedQueue & q)
  {
    if (this == &q)
      return *this;

    if (array != nullptr)
      delete [] array;

    new (this) FixedQueue(q);
    return *this;
  }

  /// Move assign
  FixedQueue & operator = (FixedQueue && q) noexcept
  {
    swap(q);
    return *this;
  }

      /** Put an item into the queue by copy

          @param[in] item to put
          @return a modifiable reference to the copied item.
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

  /// \overload T & put(const T& item)
  T & put(T && item) throw(std::exception, std::overflow_error)
  {
    assert(num_items < dim);
    array[rear_index] = std::forward<T>(item);
    T & ret_val = array[rear_index];
    increase_index(rear_index);
    num_items++;
    return ret_val;
  }

  /// \overload T & put(const T& item)
  T & append(const T& item) { return put(item); }

  /// \overload T & put(const T& item)
  T & append(T && item) { return put(std::forward<T>(item)); }

      /** Put `n` cells to the queue in constant time.

          @param[in] n number of cells
          @return a modifiable reference to the last inserted item
       */
  T & putn(const size_t n) throw(std::exception, std::overflow_error)
  {
    assert(num_items + n < dim);
    increase_index(rear_index, n);
    num_items += n;
    return rear_item();
  }

      /** Remove the oldest item of the queue.

          @return a copy of removed item
       */
  T get() throw(std::exception, std::underflow_error)
  {
    assert(num_items > 0);
    num_items--;
    T ret_val = std::move(array[front_index]);
    increase_index(front_index);
    return ret_val;
  }

      /** Remove in constant time the n oldest items of the queue

          @param[in] n number of items to remove
          @return a modifiable reference to the current oldes item in
          the queue after the removal of `n` entries
       */
  T & getn(const size_t n) throw(std::exception, std::underflow_error)
  {
    assert(num_items >= n);
    num_items -= n;
    increase_index(front_index, n);
    return array[front_index];
  } 

      /** Return the i-th oldest item

        @param[in] i access position respect to oldest item
        @return a modifiable reference to the i-th item
     */
  T & front(const size_t i = 0) const throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return array[front_index + i];
  }

    /** Return the i-th youngest item

        @param[in] i access position respect to youngest item
        @return a modifiable reference to the i-th item
     */
  T & rear(const size_t i = 0) const throw(std::exception, std::range_error)
  {
    assert(i < num_items);
    return rear_item(i);
  }

  /// Return the number of items
  size_t size() const noexcept { return num_items; }

      /// Return `true` if the queue is empty
  bool is_empty() const noexcept { return num_items == 0; }

  /// Return the queue capacity (maximum number of element to be stored)
  size_t capacity() const noexcept { return dim; }

  /** Traverse all the elements from the youngest to the oldest and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    if (num_items == 0)
      return true;
    for (size_t i = 0, idx = front_index; i < num_items; increase_index(idx), ++i)
      if (not operation(array[idx]))
	return false;
    return true;
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const 
  {
    return const_cast<FixedQueue*>(this)->traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return traverse<Operation>(operation);
  }

  /** Simple iterator on elements of a queue.

      The visit order is from the youngest to the oldest item

      @ingroup Secuencias
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = FixedQueue;

    Iterator(const FixedQueue & q) noexcept
      : Base(q.array, q.dim, q.num_items, q.front_index,
	     (q.rear_index - 1) % q.dim) {}
  };
};

} // end namespace Aleph

# endif /* TPL_ARRAYQUEUE_H */






