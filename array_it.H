/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \|  __/ |_) | | | |_____\ V  V /  version 1.9c
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

#ifndef ARRAY_IT_H
#define ARRAY_IT_H

/** @file array_it.H
    @brief Iterator wrapper for C++ raw arrays and circular buffers.

    Provides Array_Iterator for iterating over raw C arrays with bounds
    checking, including support for circular array regions. Also provides
    Array_Container as a lightweight wrapper that makes a raw array
    compatible with Aleph-w's functional interfaces.

    @ingroup Secuencias
*/

#include <cassert>
#include <tuple>
#include <aleph.H>
#include <ahDefs.H>
#include <htlist.H>
#include <ah-errors.H>

namespace Aleph {

template <typename T> class Array_Container;

/** @brief Iterator wrapper for C++ raw arrays.

    Provides a full iterator interface for raw C arrays, including support
    for circular buffers where the logical sequence wraps around the
    physical array boundaries.

    The iterator supports:
    - Forward and backward traversal
    - Bounds checking with exception throwing (or no-exception variants)
    - Circular array regions with arbitrary first/last indices
    - Position tracking

    @tparam T Element type.

    @ingroup Secuencias
*/
template <class T>
class Array_Iterator
{
  T * ptr = nullptr;
  long dim = 0;
  long num_items = 0;
  long idx = 0;
  long first = 0;
  long last = 0;
  long pos = 0;

  /// @cond INTERNAL
  static void ensure_base_pointer(T * p, const size_t sz)
  {
    ah_invalid_argument_if(sz > 0 and p == nullptr)
      << "Array_Iterator(): ptr == nullptr with dim > 0";
  }

  static void ensure_num_items(size_t sz, size_t n)
  {
    ah_domain_error_if(n > sz)
      << "Array_Iterator(): num_items greater than dim";
  }

  static void ensure_index_in_range(const size_t sz, const long value, const char * msg)
  {
    ah_domain_error_if(value < 0 or static_cast<size_t>(value) >= sz)
      << msg;
  }

  static long compute_last_index(const size_t n) noexcept
  {
    return n == 0 ? 0 : static_cast<long>(n - 1);
  }
  /// @endcond

public:

  /// The type of elements being iterated.
  using Item_Type = T;

  /** @brief Get the base pointer of the array.
      @return Pointer to the underlying array.
  */
  T * get_base() noexcept { return ptr; }

  /** @brief Get the base pointer of the array (const version).
      @return Const pointer to the underlying array.
  */
  const T * get_base() const noexcept { return ptr; }

  /// Default constructor - creates an invalid iterator.
  Array_Iterator() = default;

  /** @brief Construct an iterator over an array.

      Creates an iterator for elements [0, n) in an array of size sz.

      @param p Pointer to the array.
      @param sz Physical size of the array.
      @param n Number of valid items (logical size).

      @throw std::invalid_argument if p is null but sz > 0.
      @throw std::domain_error if n > sz.
  */
  Array_Iterator(T * p, const size_t sz, const size_t n)
    : ptr(p), dim(sz), num_items(n), last(compute_last_index(n))
  {
    ensure_base_pointer(ptr, sz);
    ensure_num_items(sz, n);
  }

  /** @brief Construct an iterator without exception checking.

      Same as the regular constructor but performs assertions instead
      of throwing exceptions. Use only when you are certain the
      parameters are valid.

      @param p Pointer to the array.
      @param sz Physical size of the array.
      @param n Number of valid items.
  */
  Array_Iterator(NoExceptionCtor, T * p, const size_t sz, const size_t n)
    : ptr(p), dim(sz), num_items(n), last(compute_last_index(n))
  {
    assert(ptr);
    assert(num_items <= dim);
  }

  /** @brief Construct an iterator over a circular array region.

      Creates an iterator for a circular array where the logical
      sequence starts at index f and ends at index l, possibly
      wrapping around the array boundaries.

      @param p Pointer to the array.
      @param sz Physical size of the array.
      @param n Number of valid items.
      @param f First index (starting position).
      @param l Last index (ending position).

      @throw std::invalid_argument if p is null but sz > 0.
      @throw std::domain_error if parameters are inconsistent.
  */
  Array_Iterator(T * p, const size_t sz, const size_t n, const long f, const long l)
    : ptr(p), dim(sz), num_items(n), idx(f), first(f), last(n == 0 ? f : l)
  {
    ensure_base_pointer(ptr, sz);
    ensure_num_items(sz, n);
    ah_domain_error_if(dim == 0 and num_items > 0)
      << "Array_Iterator(): dim == 0 but num_items > 0";
    if (dim > 0)
      {
        ensure_index_in_range(dim, first, "Array_Iterator(): first >= dim");
        ensure_index_in_range(dim, last, "Array_Iterator(): last >= dim");
      }
  }

  /** @brief Construct a circular iterator without exception checking.

      @param p Pointer to the array.
      @param sz Physical size of the array.
      @param n Number of valid items.
      @param f First index.
      @param l Last index.
  */
  Array_Iterator(NoExceptionCtor, T * p, const size_t sz, const size_t n, const long f, const long l)
    : ptr(p), dim(sz), num_items(n), idx(f), first(f), last(n == 0 ? f : l)
  {
    assert(ptr);
    assert(num_items <= dim);
    assert(dim == 0 or (first >= 0 and first < dim));
    assert(dim == 0 or (last >= 0 and last < dim));
  }

  /** @brief Construct an iterator from an Array_Container.
      @param c Container to iterate over.
  */
  Array_Iterator(const Array_Container<T> & c)
    : Array_Iterator(c.get_base(), c.capacity(), c.size()) {}

  /** @brief Check if there is a current valid item.
      @return true if positioned at a valid item, false otherwise.
  */
  [[nodiscard]] bool has_curr() const noexcept { return pos >= 0 and pos < num_items; }

  /** @brief Check if positioned at the last item.
      @return true if at the last item.
  */
  [[nodiscard]] bool is_last() const noexcept { return pos == num_items - 1; }

  /** @brief Get the current position index.
      @return Logical position (0 to num_items-1).
  */
  [[nodiscard]] long get_pos() const noexcept { return pos; }

  /** @brief Get the current item without bounds checking.
      @return Reference to the current item.
      @warning Does not check bounds - use only when you know the iterator is valid.
  */
  T & get_curr_ne() const noexcept
  {
    return ptr[idx];
  }

  /** @brief Get the current item with bounds checking.
      @return Reference to the current item.
      @throw std::underflow_error if positioned before the first item.
      @throw std::overflow_error if positioned past the last item.
  */
  T & get_curr() const
  {
    ah_underflow_error_if(pos < 0)
      << "Array_Iterator::get_curr(): no current item";

    ah_overflow_error_if(pos >= num_items)
      << "Array_Iterator::get_curr(): no current item";

    return ptr[idx];
  }

  /** @brief Advance to the next item without bounds checking.
      @warning Does not check bounds.
  */
  void next_ne() noexcept
  {
    if (++idx == dim)
      idx = 0;
    ++pos;
  }

  /** @brief Advance to the next item with bounds checking.
      @throw std::overflow_error if already at or past the end.
  */
  void next()
  {
    ah_overflow_error_if(num_items == 0 or pos >= num_items)
      << "Array_Iterator::next(): no current item";
    next_ne();
  }

  /** @brief Move to the previous item without bounds checking.
      @warning Does not check bounds.
  */
  void prev_ne() noexcept
  {
    if (--idx < 0)
      idx = dim - 1;
    --pos;
  }

  /** @brief Move to the previous item with bounds checking.
      @throw std::underflow_error if already at or before the beginning.
  */
  void prev()
  {
    ah_underflow_error_if(num_items == 0 or pos < 0)
      << "Array_Iterator::prev(): no current item";
    prev_ne();
  }

  /// Reset the iterator to the first item.
  void reset() noexcept
  {
    idx = first;
    pos = 0;
  }

  /// Reset the iterator to the first item (alias for reset()).
  void reset_first() noexcept { reset(); }

  /// Reset the iterator to the last item.
  void reset_last() noexcept
  {
    idx = last;
    pos = num_items - 1;
  }

  /// Put the iterator at the end (past the last item).
  void end() noexcept
  {
    put_itor_at_the_end(*this);
  }
};

/** @brief Create an iterator for a raw array.

    @tparam T Element type.
    @param array Pointer to the array.
    @param n Number of elements.
    @return Array_Iterator for the array.

    @ingroup Secuencias
*/
template <typename T>
[[nodiscard]] inline Array_Iterator<T> get_array_it(const T * array, size_t n)
{
  return Array_Iterator<T>(const_cast<T*>(array), n, n);
}

/** @brief Create an Array_Container from a raw array.

    @tparam T Element type.
    @param array Pointer to the array.
    @param n Number of elements.
    @return Array_Container wrapping the array.

    @ingroup Secuencias
*/
template <typename T>
[[nodiscard]] inline Array_Container<T> make_array_container(T * array, size_t n)
{
  return Array_Container<T>(array, n);
}


/** @brief Lightweight wrapper that provides Aleph-w container interface for raw arrays.

    This class wraps a raw C array pointer and size, providing:
    - Size and capacity queries
    - First/last element access
    - Iterator access
    - Compatibility with Aleph-w functional methods (map, filter, fold, etc.)

    The container does not own the array - it is a non-owning view.

    @tparam T Element type.

    @ingroup Secuencias
*/
template <typename T>
class Array_Container
  : public GenericTraverse<Array_Container<T>>,
    public LocateFunctions<Array_Container<T>, T>,
    public FunctionalMethods<Array_Container<T>, T>,
    public GenericKeys<Array_Container<T>, T>,
    public EqualToMethod<Array_Container<T>>,
    public StlAlephIterator<Array_Container<T>>
{
  T * base = nullptr;
  const size_t n = 0;

public:

  /// The type of elements in the container.
  using Item_Type = T;

  /** @brief Get the base pointer.
      @return Pointer to the underlying array.
  */
  T * get_base() const noexcept { return base; }

  /** @brief Construct a container wrapping a raw array.

      @param base_ptr Pointer to the array.
      @param d Size of the array.
  */
  Array_Container(T * base_ptr, const size_t d) : base(base_ptr), n(d) 
  {
    // empty
  }

  /** @brief Check if the container is empty.
      @return true if size is 0.
  */
  [[nodiscard]] bool is_empty() const noexcept { return n == 0; }

  /** @brief Get the number of elements.
      @return Number of elements.
  */
  [[nodiscard]] size_t size() const noexcept { return n; }

  /** @brief Get the capacity.
      @return Same as size() for this container.
  */
  [[nodiscard]] size_t capacity() const noexcept { return n; }

  /** @brief Get the first element.
      @return Reference to the first element.
      @throw std::underflow_error if empty.
  */
  T & get_first() const
  {
    ah_underflow_error_if(n == 0)
      << "Array_Container::get_first(): n == 0";
    return base[0];
  }

  /** @brief Get the last element.
      @return Reference to the last element.
      @throw std::underflow_error if empty.
  */
  T & get_last() const
  {
    ah_underflow_error_if(n == 0)
      << "Array_Container::get_last(): n == 0";
    return base[n - 1];
  }

  /** @brief Iterator type for Array_Container. */
  struct Iterator : public Array_Iterator<T>
  {
    /// Construct an iterator for the given container.
    Iterator(const Array_Container & c) : Array_Iterator<T>(c.base, c.n, c.n) {}
  };

  /** @brief Get an iterator to the beginning.
      @return Iterator positioned at the first element.
  */
  Iterator get_it() const { return Iterator(*this); }
};

} // end namespace Aleph

#endif // ARRAY_IT_H
