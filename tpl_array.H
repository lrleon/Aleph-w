
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

# ifndef TPL_ARRAY_H
# define TPL_ARRAY_H

# include <iostream>
# include <aleph.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <ah-dry.H>
# include <ah-args-ctor.H>
# include <tpl_memArray.H>

using namespace std;

using namespace Aleph;

namespace Aleph {

  /** Simple array implemented with a simple dynamic array

      `Array<T>` instruments a simple array of elements of generic type
      `T`. The array is stored into an internal continuous array which
      grows and ungrows dynamically according to the number of elements.

      @ingroup Secuencias
   */
    template <typename T>
class Array : public LocateFunctions<Array<T>, T>,
              public FunctionalMethods<Array<T>, T>,
              public GenericKeys<Array<T>, T>,
              public EqualToMethod<Array<T>>,
              public StlAlephIterator<Array<T>>
{
  MemArray<T> array;

public:

  using Item_Type = T; /// The type of element

  /// Initializes a stack with a capacity of `dim`
  Array(size_t dim = 32) : array(dim) { /* empty */ }

      /// Copy constructor
  Array(const Array & s) : array(s.array) { /* empty */ }

      /// Move constructor
  Array(Array && s) : array(std::forward<MemArray<T>>(s.array)) { /* empty */ }

  Special_Ctors(Array, T);

  Args_Ctor(Array, T);

      /// Assign by copy `s` to `this`
  Array & operator = (const Array & s)
  {
    if (this == &s)
      return *this;

    array = s.array;

    return *this;
  }

  /// Swap `this` with `s`
  void swap(Array & s) noexcept
  {
    array.swap(s.array);
  }

  /// Assign by moving `s` to `this`
  Array & operator = (Array && s) noexcept
  {
    array.swap(s.array);
    return *this;
  }

  /** Append a copy of `data`

      @param[in] data to be pushed by copy
      @return a modifiable reference to the copy of inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & append(const T & data)
  {
    return array.put(data);
  }

  /** Append `data`

      @param[in] data to be pushed by moving
      @return a modifiable reference to the moved inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & append(T && data)
  {
    return array.put(std::forward<T>(data));
  }

  Array & append(const Array & a)
  {
    array.append(a.array);
    return *this;
  }

  Array append(const Array & a) const
  {
    Array ret = *this;
    return ret.append(a);
  }

  /** insert a copy of `data` at the begining of the array. All the
      items are moved

      @param[in] data to be pushed by copy
      @return a modifiable reference to the copy of inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & insert(const T & data)
  {
    return array.push(data);
  }

  /** insert a copy of `data` at the begining of the array. All the
      items are moved

      @param[in] data to be pushed by moving
      @return a modifiable reference to the moved inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T & insert(T && data)
  {
    return array.push(std::forward<T>(data));
  }

    /** Put `n` cells into the array.

      `putn(more)` is functionally equivalent to perform `more` puts of
      `T()` in constant time. The idea is to reserve enough space for
      later use.

      @param[in] n number of cells to reserve
      @throw bad_alloc if there is no enough memory
  */
  void putn(const size_t n) { array.putn(n); }

  /** Reserves `cap` cells into the array.

      @param[in] cap new dimension
      @throw bad_alloc if there is no enough memory
   */
  void reserve(size_t cap) { array.reserve(cap); }

  /// Return a modifiable reference to first element of array
  T & base() const noexcept { return *array.get_ptr(); }

    /// Empty the stack
  void empty() noexcept { array.empty(); }

    /// Return `true` if stack is empty
  bool is_empty() const noexcept { return array.size() == 0; }

    /// Return the number of elements stored in the stack
  size_t size() const noexcept { return array.size(); }

  /// Return the internal capacity
  size_t capacity() const noexcept { return array.capacity(); }

  /// return a modifiable reference to the first element. No bound check
  /// is perfomed
  T & get_first() const { return array.first(); }

  /// return a modifiable reference to the last element. No bound check
  /// is perfomed
  T & get_last() const { return array.last(); }

  T remove_last() { return array.remove_last(); }

  T remove_first() { return array.remove_first(); }

  /// Return a reference to the ith element. Throws
  /// `range_error` if `i` is out of range
  T & operator [] (size_t i) const
  {
    return array[i];
  }

  /// Return a reference to the ith element. It does not perform bound checks
  T & operator () (const size_t i) const noexcept
  {
    return array(i);
  }

    /// Reverse the order of items in array
  Array & reverse()
  {
    array.reverse();
    return *this;
  }

  /// Return a copy of this with its items reversed
  Array reverse() const
  {
    const size_t & n = array.size();
    Array ret(n);
    for (size_t i = 0; i < n; ++i)
      ret.append(array(n - i - 1));
    return ret;
  }

  Array & rev() { return reverse(); }

  const Array rev() const { return reverse(); }

  /** Traverse all the items of stack from the youngest to oldest and
      conditionally performs an operation.

      @param[in] operation to perform on the current element. If it
      returns `true`, the the traversal continues to the next item;
      otherwise the traversal stops
   */
  template <class Operation>
  bool traverse(Operation & operation) noexcept(noexcept(operation))
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation & operation) const noexcept(noexcept(operation))
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation()) const
    noexcept(noexcept(operation))
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
  bool traverse(Operation && operation = Operation())
    noexcept(noexcept(operation))
  {
    return array.traverse(operation);
  }

  bool is_valid() const noexcept { return array.is_valid(); }

  /** Iterator on the items of array.

      @ingroup Secuencias
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = Array;

    /// Initialize an iterator on array `s`
    Iterator(const Array<T> & s) noexcept : Base(s.array) {}
  };
};

    template <typename T, typename ... Args>
Array<T> build_array(Args ... args)
{
  return build_container<Array<T>>(args...);
}


} // end namespace Aleph

# endif /* TPL_ARRAY_H */

