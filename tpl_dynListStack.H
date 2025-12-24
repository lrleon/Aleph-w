
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

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

# ifndef TPL_DYNLISTSTACK_H
# define TPL_DYNLISTSTACK_H

# include <ahDry.H>
# include <ahIterator.H>
# include <ah-args-ctor.H>
# include <htlist.H>

using namespace std;

namespace Aleph {

  /** Dynamic stack of elements of generic type `T` based on single
      linked list.

      `DynListStack<T>` models a dynamic LIFO (Last-In-First-Out) stack
      with O(1) push, pop, and top operations.

      @see DynListQueue ArrayStack FixedStack
      @ingroup Secuencias
   */
    template <typename T>
class DynListStack
      : public LocateFunctions<DynListStack<T>, T>,
        public FunctionalMethods<DynListStack<T>, T>,
        public GenericKeys<DynListStack<T>, T>,
        public EqualToMethod<DynListStack<T>>,
        public StlAlephIterator<DynListStack<T>>
{
  DynList<T> s;
  size_t num_items = 0;

public:

  /// Swap `this` with `other` in constant time
  void swap(DynListStack & other) noexcept
  {
    std::swap(num_items, other.num_items);
    s.swap(other.s);
  }

  /// Construct an empty stack
  DynListStack() noexcept : num_items(0) { /* empty */ }

  /// Construct a copy of `other`
  DynListStack(const DynListStack & other) : s(other.s), num_items(other.num_items)
  {
    // empty
  }

  /// Construct by moving `other` in constant time
  DynListStack(DynListStack && other) noexcept
  {
    swap(other);
  }

  Special_Ctors(DynListStack, T);

  /// Assign to `this` a copy of `rhs`
  DynListStack & operator = (const DynListStack & rhs)
  {
    if (this == &rhs)
      return *this;

    s = rhs.s;
    num_items = rhs.num_items;

    return *this;
  }

  /// Assign to `this` by moving `rhs` in constant time
  DynListStack & operator = (DynListStack && rhs) noexcept
  {
    std::swap(num_items, rhs.num_items);
    s.swap(rhs.s);
    return *this;
  }

  /// Return the number of elements
  size_t size() const noexcept { return num_items; }

  /// Return `true` if `this` is empty
  bool is_empty() const noexcept { return s.is_empty(); }

  using Set_Type = DynListStack; /// The type of set

  using Item_Type = T; /// The type of element

  /** Push an item by copy onto the top of the stack.

      @param[in] data item to be copied and pushed
      @return a modifiable reference to the newly pushed item
      @throw bad_alloc if there is no enough memory
   */
  T & push(const T & data)
  {
    T & ret_val = s.insert(data);
    ++num_items;
    return ret_val;
  }

  /** Push an item by moving onto the top of the stack.

      @param[in] data item to be moved and pushed
      @return a modifiable reference to the newly pushed item
      @throw bad_alloc if there is no enough memory
   */
  T & push(T && data)
  {
    T & ret_val = s.insert(std::forward<T>(data));
    ++num_items;
    return ret_val;
  }

  /** Construct an item in place at the top of the stack.

      Constructs the item using the provided arguments and pushes it
      onto the stack.

      @param[in] args arguments to forward to the constructor of T
      @return a modifiable reference to the newly constructed item
      @throw bad_alloc if there is no enough memory
   */
  template <typename... Args>
  T & emplace(Args&&... args)
  {
    return push(T(std::forward<Args>(args)...));
  }

  /// Alias for push() - required by Special_Ctors macro
  T & append(const T & data) { return push(data); }

  /// \overload append(const T & data)
  T & append(T && data) { return push(std::forward<T>(data)); }

  /// Alias for push() - for compatibility with queue-like interfaces
  T & put(const T & data) { return push(data); }

  /// \overload put(const T & data)
  T & put(T && data) { return push(std::forward<T>(data)); }

  /// Alias for push() - for STL-like insert semantics
  T & insert(const T & data) { return push(data); }

  /// \overload insert(const T & data)
  T & insert(T && data) { return push(std::forward<T>(data)); }

  /** Remove and return the top item of the stack.

      @return a copy of the removed item
      @throw underflow_error if the stack is empty
   */
  T pop()
  {
    T ret_val = s.remove_first();
    --num_items;
    return ret_val;
  }

  /// Alias for pop() - removes and returns the top item
  T get() { return pop(); }

  /** Return a modifiable reference to the top item of the stack.

      @return a modifiable reference to the top item
      @throw underflow_error if the stack is empty
   */
  T & top()
  {
    return s.get_first();
  }

  /** Return a const reference to the top item of the stack.

      @return a const reference to the top item
      @throw underflow_error if the stack is empty
   */
  const T & top() const
  {
    return s.get_first();
  }

  /// Alias for top() - returns reference to top item
  T & peek() { return top(); }

  /// Alias for top() const - returns const reference to top item
  const T & peek() const { return top(); }

  /// Empty the stack (remove all elements)
  void empty() noexcept
  {
    s.empty();
    num_items = 0;
  }

  /// Alias for empty() - clears all items from the stack
  void clear() noexcept { empty(); }

  /** Traverse all the elements from top to bottom and execute
      `operation` on each of them.

      @param[in] operation to be performed on each element
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
   */
  template <class Operation>
  bool traverse(Operation & operation)
  {
    return s.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const
  {
    return s.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
  {
    return s.traverse(std::forward<Operation>(operation));
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
  {
    return s.traverse(std::forward<Operation>(operation));
  }

  /** Search for an item in the stack using equality comparison.

      This method is required by EqualToMethod mixin.
      Performs a linear search O(n) from top to bottom.

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
    return const_cast<DynListStack*>(this)->search(key);
  }

  /// Key_Type is required by EqualToMethod mixin
  using Key_Type = T;

  /** Iterator on elements of a stack.

      The visit order is from top to bottom (LIFO order).

      @ingroup Secuencias
   */
  struct Iterator : public DynList<T>::Iterator
  {
    using Base = typename DynList<T>::Iterator;
    using Base::Base;

    Iterator(const DynListStack<T> & stack) noexcept : Base(stack.s) {}
  };
};

} // end namespace Aleph
# endif /* TPL_DYNLISTSTACK_H */