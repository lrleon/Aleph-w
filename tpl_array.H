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

/** @file tpl_array.H
    @brief Dynamic array container with automatic resizing.

    This file provides the `Array<T>` template class, a simple dynamic
    array that grows and shrinks automatically as elements are added
    or removed. It supports functional programming operations through
    mixin classes.

    @ingroup Secuencias
 *  @author Leandro Rabindranath León
*/

#ifndef TPL_ARRAY_H
#define TPL_ARRAY_H

# include <iostream>
# include <utility>
# include <vector>
# include <aleph.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <ah-dry.H>
# include <ah-args-ctor.H>
# include <tpl_memArray.H>

namespace Aleph
{

/** @brief Simple dynamic array with automatic resizing and functional operations.

    Array<T> is a straightforward dynamic array implemented using a contiguous
    memory buffer that automatically grows or shrinks as elements are added or
    removed. It provides O(1) indexed access and amortized O(1) append.

    This container offers a balance between simplicity and functionality,
    supporting both indexed access and a rich set of functional operations
    (map, filter, fold, etc.).

    @tparam T The type of elements stored in the array.

    @par Complexity:
    - Access by index: O(1)
    - append() / insert() at end: O(1) amortized
    - insert() at arbitrary position: O(n)
    - remove(): O(n) for search + O(n) for shift
    - size(): O(1)
    - Resize: O(n) when capacity is exceeded

    @par Key Features:
    - O(1) indexed access via operator[]
    - Automatic capacity management
    - Functional programming operations (map, filter, fold, etc.)
    - STL-compatible iterators
    - Conversion to/from STL vectors and other containers

    @par Comparison with Similar Containers:
    - **Array vs DynArray**: Array uses simple contiguous allocation; DynArray
      uses lazy three-level indexing for sparse arrays.
    - **Array vs DynList**: Array has O(1) indexed access but O(n) insertion
      in the middle; DynList has O(1) insertion/removal but O(n) indexed access.
    - **Array vs std::vector**: Similar behavior, but Array includes functional
      methods and integrates with Aleph-w ecosystem.

    @par Example:
    @code
    Array<int> arr(10);  // Initial capacity of 10

    // Add elements
    arr.push(42);
    arr.push(17);
    arr.push(99);

    // Indexed access (O(1))
    std::cout << arr[1] << '\n';  // 17
    arr[1] = 25;

    // Functional operations
    auto doubled = arr.maps<int>([](int x) { return x * 2; });
    int sum = arr.foldl(0, [](int acc, int x) { return acc + x; });
    auto evens = arr.filter([](int x) { return x % 2 == 0; });

    // Convert to vector
    std::vector<int> vec = arr.asVector();

    // Range-based for
    for (int x : arr)
      std::cout << x << " ";
    @endcode

    @note Memory is contiguous, providing good cache locality.
    @note Capacity grows automatically; manual control via reserve() if needed.
    @note For sparse arrays with many gaps, consider DynArray instead.

    @see DynArray Lazy dynamic array with three-level indexing for sparse data.
    @see DynList Doubly linked list alternative (O(1) insertion, O(n) access).
    @see FunctionalMethods Inherited functional operations.
    @ingroup Secuencias
 *  @author Leandro Rabindranath León
 */
template<typename T>
class Array : public LocateFunctions<Array<T>, T>,
              public FunctionalMethods<Array<T>, T>,
              public GenericKeys<Array<T>, T>,
              public EqualToMethod<Array<T>>,
              public StlAlephIterator<Array<T>>
{
  MemArray <T> array;

public:

  using Item_Type = T; /// The type of element
  using Key_Type = T;  /// The type of key

  /// Initializes a stack with a capacity of `dim`
  Array(size_t dim = 32) : array(dim) { /* empty */ }

  /// Copy constructor
  Array(const Array &s) : array(s.array) { /* empty */ }

  /// Move constructor
  Array(Array &&s) noexcept: array(std::forward<MemArray < T>>
  (s.array)) { /* empty */ }

  Special_Ctors(Array, T);

  Args_Ctor(Array, T);

  /// Assign by copy `s` to `this`
  Array &operator=(const Array &s)
  {
    if (this == &s)
      return *this;

    array = s.array;

    return *this;
  }

  /// Swap `this` with `s`
  void swap(Array &s) noexcept
  {
    array.swap(s.array);
  }

  /// Assign by moving `s` to `this`
  Array &operator=(Array &&s) noexcept
  {
    array.swap(s.array);
    return *this;
  }

  /** Append a copy of `data`

      @param[in] data to be pushed by copy
      @return a modifiable reference to the copy of inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T &append(const T &data)
  {
    return array.put(data);
  }

  /** Append `data`

      @param[in] data to be pushed by moving
      @return a modifiable reference to the moved inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T &append(T &&data)
  {
    return array.put(std::forward<T>(data));
  }

  Array &append(const Array &a)
  {
    array.append(a.array);
    return *this;
  }

  Array append(const Array &a) const
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
  T &insert(const T &data)
  {
    return array.push(data);
  }

  /** insert a copy of `data` at the begining of the array. All the
      items are moved

      @param[in] data to be pushed by moving
      @return a modifiable reference to the moved inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T &insert(T &&data)
  {
    return array.push(std::forward<T>(data));
  }

  /** Reserve `n` additional logical slots in the array without
      value-initializing them. The caller must assign to the new
      positions before reading them.

    @param[in] n number of cells to reserve
    @throw bad_alloc if there is no enough memory
*/
  void putn(const size_t n)
  {
    array.putn(n);
  }

  /** Reserves `cap` cells into the array.

      @param[in] cap new dimension
      @throw bad_alloc if there is no enough memory
   */
  void reserve(size_t cap)
  {
    array.reserve(cap);
  }

  /// Return a reference to first element of array
  T &base()
  {
    ah_underflow_error_if(is_empty()) << "Array::base(): empty array";
    return *array.get_ptr();
  }

  const T &base() const
  {
    ah_underflow_error_if(is_empty()) << "Array::base(): empty array";
    return *array.get_ptr();
  }

  /// Empty the stack
  void empty() noexcept { array.empty(); }

  /// Return `true` if stack is empty
  [[nodiscard]] constexpr bool is_empty() const noexcept { return array.size() == 0; }

  /// Return the number of elements stored in the stack
  [[nodiscard]] constexpr size_t size() const noexcept { return array.size(); }

  /// Return the internal capacity
  [[nodiscard]] constexpr size_t capacity() const noexcept { return array.capacity(); }

  /// return a modifiable reference to the first element. No bound_statics check
  /// is performed
  [[nodiscard]] T &get_first() noexcept { return array.first(); }

  /// return a constant reference to the first element. No bound_statics check
  /// is performed
  [[nodiscard]] const T &get_first() const noexcept { return array.first(); }

  /// return a modifiable reference to the last element. No bound_statics check
  /// is performed
  [[nodiscard]] T &get_last() noexcept { return array.last(); }

  /// return a constant reference to the last element. No bound_statics check
  /// is performed
  [[nodiscard]] const T &get_last() const noexcept { return array.last(); }

  [[nodiscard]] T remove_last() { return array.remove_last(); }

  [[nodiscard]] T remove_first() { return array.remove_first(); }

  /// Return a reference to the ith element. Throws
  /// `range_error` if `i` is out of range
  [[nodiscard]] T &operator[](size_t i)
  {
    return array[i];
  }

  /// Return a constant reference to the ith element. Throws
  /// `range_error` if `i` is out of range
  [[nodiscard]] const T &operator[](size_t i) const
  {
    return array[i];
  }

  /// Return a reference to the ith element. It does not perform bound_statics checks
  [[nodiscard]] constexpr T &operator()(const size_t i) noexcept
  {
    return array(i);
  }

  /// Return a constant reference to the ith element. It does not perform bound_statics checks
  [[nodiscard]] constexpr const T &operator()(const size_t i) const noexcept
  {
    return array(i);
  }

  /// Reverse the order of items in array
  Array &reverse()
  {
    array.reverse();
    return *this;
  }

  /// Return a copy of this with its items reversed
  Array reverse() const
  {
    const size_t &n = array.size();
    Array ret(n);
    for (size_t i = 0; i < n; ++i)
      ret.append(array(n - i - 1));
    return ret;
  }

  Array &rev() { return reverse(); }

  Array rev() const { return reverse(); }

  /** Traverse all the items of stack from the youngest to oldest and
      conditionally performs an operation.

      @param[in] operation to perform on the current element. If it
      returns `true`, then the traversal continues to the next item;
      otherwise the traversal stops
   */
  template<class Operation>
  bool traverse(Operation &operation)
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template<class Operation>
  bool traverse(Operation &operation) const
  {
    return array.traverse(operation);
  }

  /// \overload traverse(Operation & operation)
  template<class Operation>
  bool traverse(Operation &&operation = Operation()) const
  {
    return array.traverse(std::forward<Operation>(operation));
  }

  /// \overload traverse(Operation & operation)
  template<class Operation>
  bool traverse(Operation &&operation = Operation())
  {
    return array.traverse(std::forward<Operation>(operation));
  }

  [[nodiscard]] bool is_valid() const noexcept { return array.is_valid(); }

  /** Iterator on the items of array.

      @ingroup Secuencias
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = Array;

    /// Initialize an iterator on array `s`
    Iterator(const Array<T> &s) noexcept: Base(s.array) {}
  };
};

template<typename T, typename ... Args>
Array<T> build_array(Args ... args)
{
  return build_container<Array<T>>(args...);
}

template <class Container>
inline std::vector<typename Container::Item_Type>
to_stdvector(const Container & c)
{
  std::vector<typename Container::Item_Type> ret(c.size());
  for (size_t i = 0; i < c.size(); ++i)
    ret[i] = c(i);
  return ret;
}


} // end namespace Aleph

# endif /* TPL_ARRAY_H */

