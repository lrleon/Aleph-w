
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

/** @file tpl_array.H
    @brief Dynamic array container with automatic resizing.

    This file provides the `Array<T>` template class, a simple dynamic
    array that grows and shrinks automatically as elements are added
    or removed. It supports functional programming operations through
    the mixin classes.

    @ingroup Sequences
 *  @author Leandro Rabindranath León
*/

#ifndef TPL_ARRAY_H
#define TPL_ARRAY_H

#include <iostream>
#include <utility>
#include <vector>
#include <concepts>
#include <iterator>
#include <initializer_list>
#include <type_traits>
#include <aleph.H>
#include <htlist.H>
#include <tpl_dynDlist.H>
#include <ah-dry.H>
#include <ah-args-ctor.H>
#include <tpl_memArray.H>
#include <ah-iterator.H>

namespace Aleph {

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
    auto arr = Array<int>::create_reserved(10);  // Empty, room for 10 items

    // Add elements
    arr.append(42);
    arr.append(17);
    arr.append(99);

    // Indexed access (O(1))
    std::cout << arr[1] << '\n';  // 17
    arr[1] = 25;

    // Functional operations
    auto doubled = arr.maps<int>([](int x) { return x * 2; });
    int sum = arr.foldl(0, [](int acc, int x) { return acc + x; });
    auto evens = arr.filter([](int x) { return x % 2 == 0; });

    // Convert to vector
    std::vector<int> vec(arr.begin(), arr.end());

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
    @ingroup Sequences
 *  @author Leandro Rabindranath León
 */
template <typename T>
class Array : public LocateFunctions<Array<T>, T>,
              public FunctionalMethods<Array<T>, T>,
              public GenericKeys<Array<T>, T>,
              public EqualSequenceMethod<Array<T>>,
              public StlAlephIterator<Array<T>>
{
  MemArray<T> array;

  /// Tag that selects the capacity constructor used by create_reserved().
  struct Reserve_Tag {};

  /** @brief Construct an empty array with at least cap slots of capacity.
      @param[in] cap Requested capacity; size() remains zero.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note O(capacity()) default constructions. Private: reached only
            through create_reserved().
  */
  Array(Reserve_Tag, const size_t cap) : array(cap)
  { /* empty */
  }

public:
  using Item_Type = T;  /// The type of element
  using Key_Type = T;   /// The type of key

  /** @brief Construct an empty array with the default capacity (32 slots).
      @throws std::bad_alloc or an exception from default construction of T.
      @note O(capacity()) default constructions.
  */
  Array() : array(32)
  { /* empty */
  }

  /** @brief Deleted: a single integer argument is not accepted.

      Until Aleph-w 7.0, `Array<T> a(n)` built an empty array with capacity
      for n items, except that a variadic element constructor could win the
      overload resolution and build the one-element array {n}, depending on
      `T` and on the exact type of the argument. Readers familiar with
      `std::vector` expect n elements instead. To remove the ambiguity, every
      construction from one integer, including `Array<T> a = n;` and the
      implicit conversion of an integer argument, is a compile error. Say
      what is meant:

      - empty, room for n items: `auto a = Array<T>::create_reserved(n);`
      - n copies of value:       `Array<T> a(n, value);`
      - n slots to assign later: `auto a = Array<T>::create(n);`
      - the single element n:    `Array<T> a{n};`
  */
  Array(size_t) = delete;

  /** @brief Build an empty array with room for at least cap items.
      @param[in] cap Requested capacity; size() is zero.
      @return The empty array, with capacity() >= cap.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note O(capacity()) default constructions. Use it instead of the
            removed one-argument constructor when only capacity is meant.
  */
  [[nodiscard]] static Array create_reserved(const size_t cap)
  {
    return Array(Reserve_Tag{}, cap);
  }

  /** @brief Construct an array with `n` copies of `value`.
   *
   *  Creates an array of size `n` where each element is initialized by
   *  copy-assigning from `value` into default-constructed storage.
   *
   *  @param[in] n Number of elements to create.
   *  @param[in] value The value to copy into each element.
   *  @throws std::bad_alloc if memory allocation fails.
   *  @note Complexity: O(n) copies.
   *  @note Exception safety: if the allocation or the copy assignment of `T` throws, no
   *        array is built and no memory is leaked. The entries are default-constructed
   *        storage that receives copies of `value` by assignment.
   *  @note Thread safety: Safe to call concurrently on different instances; not thread-safe on the
   *        same instance.
   */
  Array(size_t n, const T &value)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
    : array(n)
  {
    for (size_t i = 0; i < n; ++i)
      array.put(value);
  }

  /** @brief Create an array with `n` logical elements.
   *
   *  This static factory creates an array and then grows it with
   *  `ret.putn(n)`. `putn()` delegates to `MemArray::putn()`, which only
   *  adjusts logical size (and backing storage if needed): it does NOT
   *  value-initialize new entries. Scalar entries are uninitialized and MUST
   *  be assigned before reading; class types retain their default-constructed
   *  state.
   *
   *  @param[in] n Number of elements to create.
   *  @return Array with `n` logical elements (scalars are uninitialized).
   *  @throws std::bad_alloc if there is not enough memory.
   *  @note Complexity: O(n) for allocation; logical growth via `putn()` is O(1) here.
   *  @note Exception safety: Strong guarantee.
   *  @note Thread safety: Safe to call concurrently.
   *
   *  @par Example
   *  @code
   *  auto arr = Array<int>::create(100);
   *  // arr[0] is undefined (garbage)
   *  arr[0] = 42; // Assign before read
   *  @endcode
   */
  static Array create(size_t n)
  {
    Array ret = create_reserved(n);
    ret.putn(n);
    return ret;
  }

  /// Copy constructor
  Array(const Array &s)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
    : array(s.array)
  { /* empty */
  }

  /// Move constructor
  Array(Array &&s) noexcept : array(std::forward<MemArray<T>>(s.array))
  { /* empty */
  }

  /** @brief Copy elements from an Aleph container in traversal order.
      @param[in] items Container providing for_each().
      @throws Any exception from allocation or copying T.
      @note O(items.size()) insertions.
  */
  template <template <typename> class List>
  Array(const List<T> &items) : Array()
  {
    items.for_each([this](const T &item) { append(item); });
  }

  /** @brief Construct from an input iterator range [first, last).
      @param[in] first Iterator to the first item.
      @param[in] last Sentinel after the last item.
      @throws Any exception from allocation, iteration or copying/moving T.
      @note Linear in the number of items. Integral arguments select the
            count/value constructor, never this overload.
  */
  template <std::input_iterator It, std::sentinel_for<It> Sentinel>
  Array(It first, Sentinel last) : Array()
  {
    for (; first != last; ++first)
      append(*first);
  }

  /** @brief Construct from the listed elements in order.
      @param[in] items Values to copy.
      @throws Any exception from allocation or copying T.
      @note O(items.size()) insertions. Use braces for element lists.
  */
  Array(std::initializer_list<T> items) : Array()
  {
    for (const auto &item : items)
      append(item);
  }

  /// Assign by copy `s` to `this`
  Array &operator = (const Array &s)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
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
  Array &operator = (Array &&s) noexcept
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
    requires std::is_copy_assignable_v<T>
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
    ret.append(a);
    return ret;  // returning ret.append(a), a reference, would copy ret again
  }

  /** insert a copy of `data` at the beginning of the array. All the
      items are moved

      @param[in] data to be pushed by copy
      @return a modifiable reference to the copy of inserted `data`
      @throw bad_alloc if there is no enough memory
  */
  T &insert(const T &data)
  {
    return array.push(data);
  }

  /** insert a copy of `data` at the beginning of the array. All the
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

  /// Return a reference to the first element of array
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

  /** @brief Empties the container.
      @throws none
      @note Like MemArray::empty(), and unlike `std::vector::clear()`, the
            items are not destroyed: they stay alive in the capacity until
            they are overwritten or the array is released.
  */
  void empty() noexcept
  {
    array.empty();
  }

  /** @brief Empties the container.
      @throws none
      @see empty()
  */
  void clear() noexcept
  {
    empty();
  }

  /** @brief Checks if the container is empty.
      @return true if empty, false otherwise.
      @throws none
  */
  [[nodiscard]] constexpr bool is_empty() const noexcept
  {
    return array.size() == 0;
  }

  /// Return the number of elements stored in the stack
  [[nodiscard]] constexpr size_t size() const noexcept
  {
    return array.size();
  }

  /// Return the internal capacity
  [[nodiscard]] constexpr size_t capacity() const noexcept
  {
    return array.capacity();
  }

  /** @brief Return a modifiable reference to the first element.
      @return Reference to element 0.
      @throws std::underflow_error if the array is empty.
  */
  [[nodiscard]] T &get_first()
  {
    return array.first();
  }

  /** @brief Return a constant reference to the first element.
      @return Constant reference to element 0.
      @throws std::underflow_error if the array is empty.
  */
  [[nodiscard]] const T &get_first() const
  {
    return array.first();
  }

  /** @brief Return a modifiable reference to the last element.
      @return Reference to element size() - 1.
      @throws std::underflow_error if the array is empty.
  */
  [[nodiscard]] T &get_last()
  {
    return array.last();
  }

  /** @brief Return a constant reference to the last element.
      @return Constant reference to element size() - 1.
      @throws std::underflow_error if the array is empty.
  */
  [[nodiscard]] const T &get_last() const
  {
    return array.last();
  }

  [[nodiscard]] T remove_last()
  {
    return array.remove_last();
  }

  [[nodiscard]] T remove_first()
  {
    return array.remove_first();
  }

  /// Return a reference to the ith element. Throws
  /// `range_error` if `i` is out of range
  [[nodiscard]] T &operator [] (size_t i)
  {
    return array[i];
  }

  /// Return a constant reference to the ith element. Throws
  /// `range_error` if `i` is out of range
  [[nodiscard]] const T &operator [] (size_t i) const
  {
    return array[i];
  }

  /// Return element i; requires i < size(), asserted in Debug builds.
  [[nodiscard]] constexpr T &operator () (const size_t i) noexcept
  {
    return array(i);
  }

  /// Return const element i; requires i < size(), asserted in Debug builds.
  [[nodiscard]] constexpr const T &operator () (const size_t i) const noexcept
  {
    return array(i);
  }

  /** @brief Reverse the order of items in this array, in place.

      @return A reference to `*this`, already reversed.
      @note Complexity: O(n).
      @throws Any exception thrown by swapping elements. If a swap throws,
              some elements may already have changed positions.
  */
  Array &reverse()
  {
    array.reverse();
    return *this;
  }

  /** @brief Return a copy of this array with its items reversed.

      `*this` is left unmodified.

      @return A new `Array` holding the elements of `*this` in reverse order.
      @note Complexity: O(n).
      @throws Any exception thrown by allocation or copying elements.
      @note `*this` remains unchanged if the operation throws.
  */
  Array reverse() const
  {
    const size_t &n = array.size();
    Array ret = create_reserved(n);
    for (size_t i = 0; i < n; ++i)
      ret.append(array(n - i - 1));
    return ret;
  }

  /** @brief Alias for `reverse()`. Reverses this array in place.

      @return A reference to `*this`, already reversed.
      @throws Any exception thrown by swapping elements.
      @note Complexity: O(n). If a swap throws, some elements may already
            have changed positions.
      @see reverse()
  */
  Array &reverse_in_place()
  {
    return reverse();
  }

  /** @brief Reverse this array in place.

      @return A reference to `*this`, already reversed.
      @throws Any exception thrown by swapping elements.
      @note Complexity: O(n). If a swap throws, some elements may already
            have changed positions.
      @see reverse()
  */
  Array &rev()
  {
    return reverse_in_place();
  }

  /** @brief Alias for `reverse() const`. Returns a reversed copy.

      `*this` is left unmodified.

      @return A new `Array` holding the elements of `*this` in reverse order.
      @throws Any exception thrown by allocation or copying elements.
      @note Complexity: O(n). `*this` remains unchanged if the operation throws.
      @see reverse() const
  */
  Array rev() const
  {
    return reverse();
  }

  /** @brief Traverse mutable elements from first to last.

      @param[in] op Operation accepting `T &`; returning `false` stops traversal.
      @return `true` if every element was visited; `false` if stopped early.
      @throws Any exception thrown by `op`.
      @note Time: O(n) in the number of visited elements; auxiliary space: O(1).
      @note If `op` throws, changes it already made to elements are not undone.
      @note Concurrent read-only traversals are safe when `op` only reads.
            External synchronization is required if any concurrent access
            may modify the array.
  */
  template <class Op>
    requires std::predicate<Op &, T &>
  bool traverse(Op &op)
  {
    return array.traverse(op);
  }

  /** @brief Traverse const elements from first to last.

      @param[in] op Operation accepting `const T &`; returning `false` stops traversal.
      @return `true` if every element was visited; `false` if stopped early.
      @throws Any exception thrown by `op`.
      @note Time: O(n) in the number of visited elements; auxiliary space: O(1).
      @note The traversal itself does not modify elements. Effects of `op`
            outside the array are not rolled back if it throws.
      @note Concurrent read-only access is safe when `op` only reads.
            External synchronization is required if another thread may
            modify the array.
  */
  template <class Op>
    requires std::predicate<Op &, const T &>
  bool traverse(Op &op) const
  {
    for (size_t i = 0; i < size(); ++i)
      if (not op((*this)[i]))
        return false;
    return true;
  }

  /** @brief Traverse mutable elements with a temporary operation.

      Kept as a separate overload from `traverse(Op &)` so that callers
      naming the operation type explicitly, as `traverse<Op>(lvalue)` does
      in `Aleph::all()`, still bind to the lvalue overload.
      @param[in] op Operation accepting `T &`.
      @return `true` if every element was visited; `false` if stopped early.
      @throws Any exception thrown by `op`.
      @note Time: O(n) in the number of visited elements; auxiliary space: O(1).
      @note If `op` throws, changes it already made to elements are not undone.
      @note Concurrent read-only traversals are safe when `op` only reads.
            External synchronization is required if any concurrent access
            may modify the array.
  */
  template <class Op>
    requires std::predicate<Op &, T &>
  bool traverse(Op &&op)
  {
    return traverse<Op>(op);
  }

  /** @brief Traverse const elements with a temporary operation.

      @param[in] op Operation accepting `const T &`.
      @return `true` if every element was visited; `false` if stopped early.
      @throws Any exception thrown by `op`.
      @note Time: O(n) in the number of visited elements; auxiliary space: O(1).
      @note The traversal itself does not modify elements. Effects of `op`
            outside the array are not rolled back if it throws.
      @note Concurrent read-only access is safe when `op` only reads.
            External synchronization is required if another thread may
            modify the array.
  */
  template <class Op>
    requires std::predicate<Op &, const T &>
  bool traverse(Op &&op) const
  {
    return traverse<Op>(op);
  }

  [[nodiscard]] bool is_valid() const noexcept
  {
    return array.is_valid();
  }

  /** Copy to Aleph::Array (requires copyable elements). */
  Array to_array() const
    requires std::copy_constructible<T>
  {
    return *this;  // copy
  }

  /** Iterator on the items of an array.

      @ingroup Sequences
  */
  struct Iterator : public MemArray<T>::Iterator
  {
    using Base = typename MemArray<T>::Iterator;
    using Base::Base;
    using Set_Type = Array;

    /// Opt-in marker for StlIterator random-access promotion. The iterator
    /// inherits get_pos()/set_pos() unchanged from Array_Iterator, so the
    /// logical position is consistent and random access is safe.
    using aleph_random_access_iterator = Iterator;

    /// Iterator on the modifiable array `s`.
    Iterator(Array<T> &s) noexcept : Base(s.array) {}

    /// A constant array is iterated through Const_Iterator.
    Iterator(const Array<T> &) = delete;
  };

  /// Read-only iterator: the iterator of a constant container.
  using Const_Iterator = Aleph::Const_Iterator<Iterator>;
};

template <typename T, typename... Args>
Array<T> build_array(Args... args)
{
  return build_container<Array<T>>(args...);
}

template <class Container>
inline std::vector<typename Container::Item_Type> to_stdvector(const Container &c)
{
  std::vector<typename Container::Item_Type> ret(c.size());
  for (size_t i = 0; i < c.size(); ++i)
    ret[i] = c(i);
  return ret;
}

}  // end namespace Aleph

#endif /* TPL_ARRAY_H */
