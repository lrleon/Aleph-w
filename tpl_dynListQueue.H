/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# ifndef TPL_DYNLISTQUEUE_H
# define TPL_DYNLISTQUEUE_H

# include <iostream>
# include <ahDry.H>
# include <ahIterator.H>
# include <ah-args-ctor.H>
# include <htlist.H>
# include <tpl_dynDlist.H>

using namespace std;


namespace Aleph {

  /** Dynamic queue of elements of generic type `T` based on single
      linked list.  

      @see DynDlist DynList
      @ingroup Secuencias
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

  /// Contruct a empty queue
  DynListQueue() noexcept : num_items(0) { /* empty */ }

  /// Construc a copy of `__q`
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
  size_t size() const noexcept { return num_items; }

  /// rioReturn `true` if `this` is empty
  bool is_empty() const noexcept { return q.is_empty(); }

  using Set_Type = DynListQueue; /// The type of set

  using Item_Type = T; /// The type of element

      /** Put an item by copy

          @param[in] data to be copied and put
          @return a modifiable reference to just recently copy of
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
  T & put(T && data) noexcept(noexcept(std::forward<T>(data)))
  {
    T & ret_val = q.append(std::forward<T>(data));
    ++num_items;
    return ret_val;
  }

  T & append(const T & data) { return put(data); }

  T & append(T && data) noexcept(noexcept(std::forward<T>(data)))
  {
    return put(std::forward<T>(data)); 
  }

      /** Remove the oldest item of the queue.

          @return a copy of removed item
          @throw underflow_error if the queue is empty
       */
  T get() 
  {
    T ret_val = q.remove_first();
    --num_items;
    return ret_val;
  }
 
  /// Return a modifiable reference to the oldest item in the queue
  T & front() const
  {
    return q.get_first();
  }

  /// Return a modifiable reference to the youngest item in the queue
  T & rear() const 
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

  /** Traverse all the elements from the youngest to the oldest and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
  */
  template <class Operation>
  bool traverse(Operation & operation) noexcept(noexcept(operation))
  {
    return q.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const noexcept(noexcept(operation))
  {
    return q.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
    noexcept(noexcept(operation))
  {
    return q.traverse(std::forward<Operation>(operation));
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) 
    noexcept(noexcept(operation))
  {
    return q.traverse(std::forward<Operation>(operation));
  }

  /** Iterator on elements of a queue.

      The visit order is from the youngest to the oldest item

      @ingroup Secuencias
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

