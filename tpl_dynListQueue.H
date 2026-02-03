
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_dynListQueue.H
 *  @brief Dynamic queue implementation based on linked lists.
 *
 *  This file provides `DynListQueue`, a FIFO queue backed by a linked list.
 *  It supports O(1) enqueue and dequeue operations with unlimited capacity
 *  and efficient memory management.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNLISTQUEUE_H
# define TPL_DYNLISTQUEUE_H

# include <iostream>
# include <ahDry.H>
# include <ahIterator.H>
# include <ah-args-ctor.H>
# include <htlist.H>
# include <tpl_dynDlist.H>

namespace Aleph {

  /** Dynamic queue of elements of generic type `T` based on single
      linked list.  

      @see DynDlist DynList
      @ingroup Sequences
   */
    template <typename T> 
class DynListQueue 
      : public LocateFunctions<DynListQueue<T>, T>,
	public FunctionalMethods<DynListQueue<T>, T>,
	public GenericKeys<DynListQueue<T>, T>,
	public EqualToMethod<DynListQueue<T>>,
	public StlAlephIterator<DynListQueue<T>>
{
  DynList<T> q;
  size_t num_items = 0;

  using Base = DynList<T>;

public:

  /// Swap `this` with `__q` in constant time
  void swap(DynListQueue & __q) noexcept
  {
    std::swap(num_items, __q.num_items);
    q.swap(__q.q);
  }

  /// Construct an empty queue
  DynListQueue() noexcept : num_items(0) { /* empty */ }

  /// Construct a copy of `__q`
  DynListQueue(const DynListQueue & __q) : q(__q.q), num_items(__q.num_items)
  {
    // empty
  }

  /// Construct in constant time a queue from `__q`
  DynListQueue(DynListQueue && __q) noexcept
  {
    swap(__q);
  }

  Special_Ctors(DynListQueue, T);

  /// Assign to `this` a copy of `rhs`
  DynListQueue & operator = (const DynListQueue & rhs)
  {
    if (this == &rhs)
      return *this;

    q = rhs.q;
    num_items = rhs.num_items;

    return *this;
  }

  /// Assign to `this` by moving `rsh` in constant time the queue `rhs`
  DynListQueue & operator = (DynListQueue && rhs) noexcept
  {
    std::swap(num_items, rhs.num_items);
    q.swap(rhs.q);
    return *this;
  }

  /// Return the number of elements
  [[nodiscard]] constexpr size_t size() const noexcept { return num_items; }

  /// Return `true` if `this` is empty
  [[nodiscard]] bool is_empty() const noexcept { return q.is_empty(); }

  using Set_Type = DynListQueue; /// The type of set

  using Item_Type = T; /// The type of element

      /** Put an item by copy

          @param[in] data to be copied and put
          @return a modifiable reference to just recent copy of
          inserted item
	  @throw bad_alloc if there is no enough memory
       */
  T & put(const T & data) 
  {
    T & ret_val = q.append(data);
    ++num_items;
    return ret_val;
  }

  /** Put an item by moving

      @param[in] data to be moved and put
      @return a modifiable reference to just recently moved
      inserted item
      @throw bad_alloc if there is no enough memory
  */
  T & put(T && data)
  {
    T & ret_val = q.append(std::forward<T>(data));
    ++num_items;
    return ret_val;
  }

  T & append(const T & data) { return put(data); }

  T & append(T && data)
  {
    return put(std::forward<T>(data)); 
  }

  T & insert(const T & data) { return put(data); }

  T & insert(T && data)
  {
    return put(std::forward<T>(data));
  }

  /** Construct an item in place at the rear of the queue.

      Constructs the item using the provided arguments and moves it
      into the queue.

      @param[in] args arguments to forward to the constructor of T
      @return a modifiable reference to the newly constructed item
      @throw bad_alloc if there is no enough memory
   */
  template <typename... Args>
  T & emplace(Args&&... args)
  {
    return put(T(std::forward<Args>(args)...));
  }

  /// Alias for get() - removes and returns the front item
  T pop() { return get(); }

  /// Alias for empty() - clears all items from the queue
  void clear() noexcept { empty(); }

      /** Remove the oldest item of the queue.

          @return a copy of the removed item
          @throw underflow_error if the queue is empty
       */
  T get() 
  {
    T ret_val = q.remove_first();
    --num_items;
    return ret_val;
  }
 
  /** Return a modifiable reference to the oldest item in the queue.

      @return a modifiable reference to the front item
      @throw underflow_error if the queue is empty
   */
  T & front()
  {
    return q.get_first();
  }

  /** Return a const reference to the oldest item in the queue.

      @return a const reference to the front item
      @throw underflow_error if the queue is empty
   */
  const T & front() const
  {
    return q.get_first();
  }

  /** Return a modifiable reference to the youngest item in the queue.

      @return a modifiable reference to the rear item
      @throw underflow_error if the queue is empty
   */
  T & rear()
  {
    return q.get_last();
  }

  /** Return a const reference to the youngest item in the queue.

      @return a const reference to the rear item
      @throw underflow_error if the queue is empty
   */
  const T & rear() const
  {
    return q.get_last();
  } 

  /// Empty the queue
  void empty() noexcept
  {
    q.empty();
    num_items = 0;
    assert(q.is_empty());
  }

  /** Traverse all the elements from the oldest (front) to the youngest (rear)
      and execute `operation` on each of them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
  */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    return q.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return q.traverse(operation);
  }

  /// \overload traverse (Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return q.traverse(std::forward<Operation>(operation));
  }

  /// \overload traverse (Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
  {
    return q.traverse(std::forward<Operation>(operation));
  }

  /** Search for an item in the queue using equality comparison.

      This method is required by EqualToMethod mixin.
      Performs a linear search O(n) from front to rear.

      @param[in] key the value to search for
      @return pointer to the item if found, nullptr otherwise
   */
  T * search(const T & key) noexcept
  {
    return this->find_ptr([&key](const T & item) { return item == key; });
  }

  /// \overload search(const T & key)
  const T * search(const T & key) const noexcept
  {
    return const_cast<DynListQueue*>(this)->search(key);
  }

  /// Key_Type is required by EqualToMethod mixin
  using Key_Type = T;

  /** Iterator on elements of a queue.

      The visit order is from the oldest (front) to the youngest (rear) item.

      @ingroup Sequences
  */
  struct Iterator : public DynList<T>::Iterator
  {
    using Base = typename DynList<T>::Iterator;
    using Base::Base;

    Iterator(const DynListQueue<T> & q) noexcept : Base(q.q) {}
  };
};

} // end namespace Aleph
# endif // TPL_DYNLISTQUEUE_H

