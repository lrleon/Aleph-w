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

/** @file tpl_memArray.H
 *  @brief Simple, scalable, contiguous dynamic array.
 *
 *  This file provides MemArray, a dynamic array that maintains all elements
 *  in a single contiguous memory block. Unlike DynArray (which uses a
 *  two-level directory), MemArray offers direct array-like access but
 *  requires reallocation when growing beyond capacity.
 *
 *  ## Key Features
 *
 *  - Contiguous memory layout (cache-friendly)
 *  - Direct O(1) access like raw arrays
 *  - Automatic growth with configurable factor
 *  - Move semantics for efficient transfers
 *  - STL-compatible iteration
 *
 *  ## Complexity
 *
 *  | Operation | Time | Notes |
 *  |-----------|------|-------|
 *  | access [] | O(1) | Direct memory access |
 *  | append | O(1) amortized | O(n) when reallocating |
 *  | insert at i | O(n-i) | Shifts elements |
 *  | remove at i | O(n-i) | Shifts elements |
 *  | reserve | O(n) | May reallocate |
 *
 *  ## Growth Strategy
 *
 *  When capacity is exceeded, the array grows by a configurable factor
 *  (default: 2x). This gives amortized O(1) appends.
 *
 *  ## Comparison with DynArray
 *
 *  | Feature | MemArray | DynArray |
 *  |---------|----------|----------|
 *  | Memory layout | Contiguous | Two-level directory |
 *  | Reallocation | Full copy | Segment-based |
 *  | Cache locality | Excellent | Good |
 *  | Large arrays | May fail to allocate | Handles well |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  MemArray<int> arr(100);  // Initial capacity 100
 *
 *  arr.append(42);          // Add element
 *  arr[0] = 17;             // Direct access
 *
 *  for (auto& x : arr)      // Range-based for
 *    std::cout << x << " ";
 *  ```
 *
 *  @see tpl_dynArray.H For very large arrays or frequent resizing
 *  @see tpl_arrayStack.H Stack built on MemArray
 *  @see tpl_arrayQueue.H Queue built on MemArray
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_MEMARRAY_H
#define TPL_MEMARRAY_H

#include <utility>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <ah-errors.H>
#include <ahUtils.H>
#include <ahDry.H>
#include <ahIterator.H>
#include <array_it.H>
#include <array_utils.H>

// Opt-in AddressSanitizer container annotations for MemArray. Define
// ALEPH_ANNOTATE_CONTAINERS for the whole program, Aleph included, when it is
// built with -fsanitize=address: the macro changes the layout of MemArray, so
// every translation unit must agree on it. Without AddressSanitizer, or
// without the macro, the annotations compile to nothing.
#if defined(ALEPH_ANNOTATE_CONTAINERS)
#  if defined(__SANITIZE_ADDRESS__)
#    define ALEPH_MEMARRAY_ANNOTATIONS 1
#  elif defined(__has_feature)
#    if __has_feature(address_sanitizer)
#      define ALEPH_MEMARRAY_ANNOTATIONS 1
#    endif
#  endif
#endif
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
#  include <sanitizer/common_interface_defs.h>
#endif

namespace Aleph {
/** Simple, scalable and fast dynamic array.

    MemArray implements a totally sequential dynamic array. That is,
    conditioned to memory availability, the array is stored in a
    contiguous chunk of memory. So, the access is direct and with
    exactly the same cost of accessing a normal array.

    The array allows inserting and removing elements. These operations are
    conceived by the ends. The number of stored elements is called `n`.

    The allocation technique obeys to "buddy system"; that is, the
    exact dimension of array always is an exact two power.

    When the array is full, a new chunk twice as long is
    allocated and the current entries are moved (by move semantic if
    available).

    When the number of entries descends to a `contract_threshold` the
    array is half long contracted.

    \note This class is not really conceived for managing a
    set. Instead, it is conceived as implementation support of single
    sequentially based set as stacks and queues.

    @ingroup Sequences
 */
template <typename T>
class MemArray
{
public:
  static constexpr size_t Min_Dim = 4;

protected:
  T *ptr = nullptr;
  size_t dim = Min_Dim;
  size_t n = 0;
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
  bool annotated = true;  // false while the live items are not the prefix [0, n)
#endif

public:
  /** Number of items at or below which contract() halves the capacity.

      It is recomputed as a quarter of the capacity whenever the array is
      reallocated. Changing it only changes when the array contracts:
      contract() never shrinks the capacity below the number of items, so
      no item can be lost. `ArrayQueue` exports it with a using-declaration.
  */
  mutable size_t contract_threshold;

  /// Return the current base of array.
  T *get_ptr() const noexcept
  {
    return ptr;
  }

  /// Return the current dimension of array
  const size_t &get_dim() const noexcept
  {
    return dim;
  }

protected:
  /** Tell AddressSanitizer that the live items, previously the prefix
      [0, old_n), are now [0, new_n): accesses to the slots in [new_n, dim)
      are then reported as container-overflow. The whole buffer must be
      accessible again before it is deleted. No-op unless the annotations
      are in effect (ALEPH_ANNOTATE_CONTAINERS with AddressSanitizer).

      @param[in] old_n previous end of the accessible prefix
      @param[in] new_n new end of the accessible prefix
  */
  void annotate(const size_t old_n, const size_t new_n) const noexcept
  {
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    if (annotated and ptr != nullptr and old_n != new_n)
      __sanitizer_annotate_contiguous_container(ptr, ptr + dim, ptr + old_n, ptr + new_n);
#else
    (void) old_n;
    (void) new_n;
#endif
  }

  /** Make the whole buffer accessible and stop annotating it. For users
      that keep their items in a circular layout (ArrayQueue), where the
      live items are not a prefix of the buffer.
  */
  void disable_annotations() noexcept
  {
    annotate(n, dim);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    annotated = false;
#endif
  }

  static size_t next_power_of_two(const size_t requested)
  {
    size_t ret = 1;
    while (ret < requested)
      {
        ah_overflow_error_if(ret > (std::numeric_limits<size_t>::max() >> 1))
          << "MemArray capacity overflow";
        ret <<= 1;
      }
    return ret;
  }

  /// Allocate memory for the current dimension
  void allocate()
  {
    static_assert(std::is_default_constructible_v<T>,
                  "MemArray<T> requires a default-constructible T (storage is new T[dim])");
    assert(is_power_of_2(dim));
    ptr = new T[dim];
    contract_threshold = dim / 4;
    annotate(dim, n);
  }

  /** @brief Relocate the logical sequence into a new buffer.
      @param[in] cap Requested capacity, at least size().
      @param[in] first Physical index of the first logical element.
      @throws std::bad_alloc or any exception from constructing/assigning T.
      @note O(cap + n) time. A temporary owns the allocation on all paths;
            throwing moves may leave some source elements moved from.
  */
  void reallocate(const size_t cap, const size_t first = 0)
  {
    assert(cap >= n);
    MemArray replacement(cap);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    if (not annotated)
      replacement.disable_annotations();
#endif
    replacement.putn(n);
    for (size_t i = 0; i < n; ++i)
      replacement.ptr[i] = std::move(ptr[(first + i) & (dim - 1)]);
    swap(replacement);
  }

  /** @brief Grow a full array, including an empty moved-from array.
      @param[in] first Physical index of the first logical element.
      @return true if storage was allocated and the sequence starts at zero.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note O(dim) on growth; otherwise O(1). Zero capacity grows to Min_Dim.
  */
  bool expand(const size_t first = 0)
  {
    assert((ptr != nullptr and is_power_of_2(dim)) or (dim == 0 and n == 0));
    if (n < dim)
      return false;

    ah_overflow_error_if(dim > (std::numeric_limits<size_t>::max() >> 1))
      << "MemArray::expand(): capacity overflow";
    reallocate(dim == 0 ? Min_Dim : dim << 1, first);

    return true;
  }

  /** @brief Grow a full array and insert one item into the new buffer.

      The item is assigned before any element leaves the current buffer,
      so it may be one of those elements: growth never reads freed memory
      and inserts the value that `item` had on entry.

      @param[in] pos Logical position of the new item, at most size().
      @param[in] item Value to insert; it may be an element of this array.
      @param[in] first Physical index of the first logical element.
      @return Reference to the inserted item, at physical index `pos`.
      @throws std::overflow_error, std::bad_alloc or an exception from T.
      @note O(dim). Requires size() == capacity(). The array is unchanged
            if the allocation or the assignment of `item` throws; a throwing
            move may leave some elements moved from, as in reallocate().
  */
  template <class U>
  T &grow_and_insert(const size_t pos, U &&item, const size_t first = 0)
  {
    assert(n == dim and pos <= n);
    ah_overflow_error_if(dim > (std::numeric_limits<size_t>::max() >> 1))
      << "MemArray::grow_and_insert(): capacity overflow";

    MemArray replacement(dim == 0 ? Min_Dim : dim << 1);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    if (not annotated)
      replacement.disable_annotations();
#endif
    replacement.putn(n + 1);
    replacement.ptr[pos] = std::forward<U>(item);  // the old buffer is still alive
    for (size_t i = 0; i < n; ++i)
      replacement.ptr[i < pos ? i : i + 1] = std::move(ptr[(first + i) & (dim - 1)]);
    swap(replacement);

    return ptr[pos];
  }

  /** Test if `n` is lesser than `contract_threshold` and eventually
      contract the array half long and copies its content.

      `contract(first)` first testes `n` with `contract_threshold`. If
      it is lesser, then a new array half as long is allocated and the
      `n` elements from `first` are copied.

      @param[in] first index of first element
      @return `true` if the array is reallocated
  */
  bool contract(const size_t first = 0)
  {
    assert(ptr);
    if (n > contract_threshold)
      return false;

    const size_t newsz = dim >> 1;  // dim/2

    if (newsz <= Min_Dim or n > newsz)
      return false;

    reallocate(newsz, first);

    return true;
  }

  /// Initialize the dimension of the array to `d` or to the next two power if `d` is not a two power.
  ///
  /// If `d` is 0 then `d` is set to `Min_Dim`.
  ///
  /// @param[in] d the proposed dimension of array
  void init_dim(size_t d)
  {
    if (d == 0)
      d = Min_Dim;

    dim = is_power_of_2(d) ? d : next_power_of_two(d);

    assert(dim >= d);
    assert(is_power_of_2(dim));
  }

public:
  using Item_Type = T;  /// The type of element of array

  /// Return the capacity of array (its dimension)
  [[nodiscard]] constexpr size_t capacity() const noexcept
  {
    return dim;
  }

  /// Return the number of elements
  size_t size() const noexcept
  {
    return n;
  }

  /// Return `true` is the array is empty
  bool is_empty() const noexcept
  {
    return n == 0;
  }

  /** Construct an array con capacity equal or greater than `_dim`.

      @param[in] _dim proposed dimension of array which could be
      adjusted to the next two power greater than `_dim`
      @throw bad_alloc if there is not enough memory.
  */
  MemArray(size_t _dim = Min_Dim) : ptr(nullptr), n(0)
  {
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
    static_assert(std::is_move_assignable_v<T>, "T must be move assignable");
    init_dim(_dim);
    allocate();
  }

  ~MemArray()
  {
    if (ptr != nullptr)
      {
        annotate(n, dim);
        delete[] ptr;
        ptr = nullptr;
      }
  }

  /// Swap in constant time `this` with `a`
  void swap(MemArray &a) noexcept
  {
    std::swap(ptr, a.ptr);
    std::swap(dim, a.dim);
    std::swap(n, a.n);
    std::swap(contract_threshold, a.contract_threshold);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    std::swap(annotated, a.annotated);  // the flag describes the buffer
#endif
  }

  /// Construct a copy of `a`
  MemArray(const MemArray &a)
    requires(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
    : MemArray(a.dim)
  {
    putn(a.n);
    for (size_t i = 0; i < n; ++i)
      ptr[i] = a.ptr[i];
  }

  /** @brief Transfer storage without allocation.
      @param[in,out] a Source; left empty with zero capacity and reusable.
      @note O(1). The next insertion into a allocates storage lazily.
      @throws Nothing.
  */
  MemArray(MemArray &&a) noexcept : ptr(nullptr), dim(0), n(0), contract_threshold(0)
  {
    swap(a);
  }

  /// Assign by copy `a` to `this`
  MemArray &operator=(const MemArray &a)
    requires(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
  {
    if (this == &a)
      return *this;

    MemArray copy(a);
    swap(copy);

    return *this;
  }

  /// Assign by moving `a` to `this`
  MemArray &operator=(MemArray &&a) noexcept
  {
    swap(a);
    return *this;
  }

  /** @brief Empties the container.

      The array size is set to zero, but the allocated memory is not released.

      @throws none
      @note Unlike `std::vector::clear()`, the items are not destroyed: the
            storage is an array of `T` built with `new T[dim]`, so the
            objects stay alive, with their last values, until they are
            overwritten or the array is released. Use empty_and_release()
            or the destructor to release them.
  */
  void empty() noexcept
  {
    annotate(n, 0);
    n = 0;
  }

  /** @brief Alias for empty().

      @throws none
      @see empty()
  */
  void clear() noexcept
  {
    empty();
  }

  /** @brief Clear the array and reduce allocated storage to Min_Dim.
      @throws std::bad_alloc or an exception from default construction of T.
      @note Allocation failure leaves the original array unchanged.
            An already small or moved-from array simply becomes empty.
  */
  void empty_and_release()
  {
    if (dim <= Min_Dim)
      {
        empty();
        return;
      }

    MemArray replacement(Min_Dim);
#ifdef ALEPH_MEMARRAY_ANNOTATIONS
    if (not annotated)
      replacement.disable_annotations();
#endif
    swap(replacement);
  }

  /** @brief Put a copy of `item` at the end of the sequence.
      @param[in] item Value to append; it may be an element of this array,
                 as in `a.put(a(0))`, even when the array must grow.
      @return Reference to the appended copy.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note Amortized O(1); O(n) when the array is full and grows. The array
            is unchanged if the allocation or the copy throws.
  */
  T &put(const T &item)
    requires std::is_copy_assignable_v<T>
  {
    return put_item(item);
  }

  /** @brief Move `item` to the end of the sequence.
      @param[in] item Value to append; it may be an element of this array,
                 as in `a.put(std::move(a(0)))`, which leaves that element
                 moved from.
      @return Reference to the appended item.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note Amortized O(1); O(n) when the array is full and grows. `item`
            is untouched if the allocation throws.
  */
  T &put(T &&item)
  {
    return put_item(std::move(item));
  }

private:
  /// Shared body of both put() overloads.
  template <class U>
  T &put_item(U &&item)
  {
    if (n == dim)  // growing frees the buffer that item may live in
      return grow_and_insert(n, std::forward<U>(item));

    annotate(n, n + 1);
    try
      {
        ptr[n] = std::forward<U>(item);
      }
    catch (...)
      {
        annotate(n + 1, n);
        throw;
      }
    T &ret = ptr[n++];
    return ret;
  }

  /// Shared body of both push() overloads.
  template <class U>
  T &push_item(U &&item)
  {
    if (n == dim)  // growing frees the buffer that item may live in
      return grow_and_insert(0, std::forward<U>(item));

    T tmp(std::forward<U>(item));  // open_gap() shifts the element item may be
    open_gap();
    ptr[0] = std::move(tmp);
    T &ret = ptr[0];
    return ret;
  }

  void open_gap(size_t pos = 0, size_t num_entries = 1)
  {
    putn(num_entries);
    Aleph::open_gap(ptr, n, pos, num_entries);
  }

  void close_gap(size_t pos, size_t num_entries = 1)
  {
    ah_underflow_error_if(num_entries == 0) << "MemArray::close_gap(): no entries to remove";
    Aleph::close_gap(ptr, n, pos, num_entries);
    // The last num_entries items are now moved-from: drop them without
    // moving them out again, as get() would do.
    const size_t old_n = n;
    n -= num_entries;
    annotate(old_n, n);
    contract();
  }

public:
  /** @brief Push a copy of `item` at the beginning of the sequence.
      @param[in] item Value to insert; it may be an element of this array,
                 as in `a.push(a(1))`, even when the array must grow.
      @return Reference to the inserted copy, now the first element.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note O(n): every element shifts one position. The array is unchanged
            if the allocation or the copy throws.
  */
  T &push(const T &item)
    requires std::is_copy_assignable_v<T>
  {
    return push_item(item);
  }

  /** @brief Move `item` to the beginning of the sequence.
      @param[in] item Value to insert; it may be an element of this array,
                 as in `a.push(std::move(a(1)))`, which leaves that element
                 moved from.
      @return Reference to the inserted item, now the first element.
      @throws std::bad_alloc, std::overflow_error or an exception from T.
      @note O(n): every element shifts one position.
  */
  T &push(T &&item)
  {
    return push_item(std::move(item));
  }

  T &top() const
  {
    ah_underflow_error_if(n == 0) << "top(): MemArray is empty";

    return ptr[0];
  }

  /// Remove the first item. Gap is closed
  T remove_first()
  {
    ah_underflow_error_if(n == 0) << "remove_first(): MemArray is empty";
    assert(ptr);
    T ret = std::move(ptr[0]);
    this->close_gap(0, 1);
    return ret;
  }

  /// pop() the most recently pushed item
  T pop()
  {
    return remove_first();
  }

  /// \overload put(const T & item)
  T &append(const T &item)
    requires std::is_copy_assignable_v<T>
  {
    return put(item);
  }

  /// \overload put(T && item)
  T &append(T &&item)
  {
    return put(std::forward<T>(item));
  }

  /// \overload push(const T & item)
  T &insert(const T &item)
    requires std::is_copy_assignable_v<T>
  {
    return push(item);
  }

  /// \overload push(T && item)
  T &insert(T &&item)
  {
    return push(std::forward<T>(item));
  }

  /** Reserve `more` additional logical slots in the array.

      `putn(more)` only adjusts the logical size and grows the backing
      storage if necessary; it does **not** value-initialize the new
      entries. Callers must assign to the returned positions before
      reading them.

      @param[in] more number of cells to reserve
      @throw bad_alloc if there is no enough memory
  */
  void putn(const size_t more)
  {
    ah_overflow_error_if(more > std::numeric_limits<size_t>::max() - n)
      << "MemArray::putn(): size overflow";
    const size_t new_n = n + more;
    if (new_n <= dim)
      {
        annotate(n, new_n);
        n = new_n;
        return;
      }

    size_t new_dim = dim == 0 ? Min_Dim : dim;
    while (new_dim < new_n)
      {
        ah_overflow_error_if(new_dim > (std::numeric_limits<size_t>::max() >> 1))
          << "MemArray::putn(): capacity overflow";
        new_dim <<= 1;  // dim = 2*dim
      }

    reallocate(new_dim);
    annotate(n, new_n);
    n = new_n;
  }

  MemArray &append(const MemArray &a)
    requires std::is_copy_assignable_v<T>
  {
    const size_t old_n = n;
    const size_t num_entries = a.size();
    putn(num_entries);
    for (size_t i = 0; i < num_entries; ++i)
      ptr[old_n + i] = a.ptr[i];

    return *this;
  }

  /** Reserves `cap` cells into the array.

      @param[in] cap new dimension
      @throw bad_alloc if there is no enough memory
  */
  void reserve(const size_t cap)
  {
    if (cap <= dim)
      return;

    const size_t new_dim = is_power_of_2(cap) ? cap : next_power_of_two(cap);
    reallocate(new_dim);
  }

  /// Remove `i` elements from the end. Return the value of the last
  /// removed element
  T get(const size_t i = 1)
  {
    ah_underflow_error_if(i == 0 or i > n)
      << "MemArray::get(): invalid number of entries";

    const size_t old_n = n;
    T ret = std::move(ptr[n - i]);
    n -= i;
    annotate(old_n, n);

    contract();

    return ret;
  }

  T get_ne(const size_t i = 1) noexcept
  {
    assert(ptr);
    assert(i > 0);
    assert(i <= n);
    const size_t old_n = n;
    T ret = std::move(ptr[n - i]);
    n -= i;
    annotate(old_n, n);

    contract();

    return ret;
  }

  /// \overload get()
  T remove_last()
  {
    return get();
  }

  /// Return a modifiable reference to the last element
  T &last() const
  {
    ah_underflow_error_if(n == 0) << "MemArray::last(): empty array";
    return ptr[n - 1];
  }

  /// Return a modifiable reference to the first element
  T &first() const
  {
    ah_underflow_error_if(n == 0) << "MemArray::first(): empty array";
    return ptr[0];
  }

  /// \overload first
  T &get_first() const
  {
    return first();
  }

  /// \overload last
  T &get_last() const
  {
    return last();
  }

  /// Reverse the order of items in array
  MemArray &reverse()
  {
    if (n < 2)
      return *this;

    for (size_t i = 0, j = n - 1; i < j; ++i, --j)
      std::swap(ptr[i], ptr[j]);
    return *this;
  }

  /// Return a modifiable reference to the ith element. No bound_statics check
  /// is performed
  T &access(const size_t i) const noexcept
  {
    assert(ptr);
    return ptr[i];
  }

  /// Return a reference to the ith element. Throws
  /// `out_of_range` if `i` is out of range
  T &operator[](const size_t i) const
  {
    ah_out_of_range_error_if(i >= n) << "access out of range";

    return ptr[i];
  }

  /// \overload access(const size_t i)
  ///
  /// Unlike `access()`, which also reaches the reserved slots beyond the
  /// logical size, debug builds assert that `i` is a live item (`i < n`).
  T &operator()(const size_t i) const noexcept
  {
    assert(ptr);
    assert(i < n);
    return ptr[i];
  }

  /** Traverse all the elements from index 0 to `n - 1` and execute
      `operation` on each on them.

      @param[in] operation to be performed on each element; it receives
                 a modifiable reference.
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &operation) noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    assert(ptr or n == 0);
    for (size_t i = 0; i < n; i++)
      if (not operation(ptr[i]))
        return false;

    return true;
  }

  /** Traverse a constant array; each element is handed out as a
      constant reference.

      @param[in] operation to be performed on each element; it must accept
                 `const T &`.
      @return `true` if `operation` was executed on all elements;
      `false` otherwise.
      @throw anything that `operation` throws.
  */
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &operation) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    assert(ptr or n == 0);
    for (size_t i = 0; i < n; i++)
      if (not operation(static_cast<const T &>(ptr[i])))
        return false;

    return true;
  }

  /// \overload traverse(Operation & operation) const
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, const T &>
  bool traverse(Operation &&operation) const
    noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
  {
    return traverse<Operation>(operation);
  }

  /// \overload traverse(Operation & operation)
  template <class Operation>
    requires Aleph::PredicateWith<Operation &, T &>
  bool traverse(Operation &&operation) noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
  {
    return traverse<Operation>(operation);
  }

  bool is_valid() const noexcept
  {
    return ptr;
  }

  /** Simple iterator on elements of array

      @ingroup Sequences
  */
  struct Iterator : public Array_Iterator<T>
  {
    using Base = Array_Iterator<T>;
    using Base::Base;

    /// Construct an iterator on array `a`
    Iterator(const MemArray<T> &a) noexcept
      : Array_Iterator<T>(a.ptr, a.dim, a.n)
    {}
  };
};
}  // end namespace Aleph

#endif  // TPL_MEMARRAY_H
