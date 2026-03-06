
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

# ifndef TPL_MEMARRAY_H
# define TPL_MEMARRAY_H

# include <utility>
# include <cstdlib>
# include <cmath>
# include <new>
# include <type_traits>
# include <stdexcept>

# include <ah-errors.H>
# include <ahUtils.H>
# include <ahDry.H>
# include <ahIterator.H>
# include <array_it.H>
# include <array_utils.H>

namespace Aleph
{

  /** Simple, scalable and fast dynamic array.

      MemArray implement a totally sequential dynamic array. That is,
      conditioned to memory availability, all the array is stored in a
      contiguous chunk of memory. So, the access is direct and with
      exactly the same cost of accessing a normal array.

      The array allows to insert and remove elements. These operation are
      conceived by the ends. The number of stored elements is called `n`.

      The allocation technique obeys to "buddy system"; that is, the
      exact dimension of array always is an exact two power.

      When the array is full, a new chunk twice as long is
      allocated and the current entries are moved (by move semantic if
      available).

      When the number of entries descend to a `contract_threshold` the
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

   public:

    mutable size_t contract_threshold;

    /// Return the current base of array.
    T *get_ptr() const noexcept { return ptr; }

    /// Return the current dimension of array
    const size_t &get_dim() const noexcept { return dim; }

   protected:

    /// Allocate raw (uninitialized) memory for `count` elements of T.
    static T * raw_alloc(const size_t count)
    {
      return static_cast<T *>(::operator new(count * sizeof(T)));
    }

    /// Free raw memory previously obtained from raw_alloc.
    static void raw_dealloc(T * p) noexcept
    {
      ::operator delete(p);
    }

    static void destroy_one(T * p) noexcept
    {
      if constexpr (not std::is_trivially_destructible_v<T>)
        p->~T();
    }

    /// Destroy `count` elements starting at `base[start]` (linear).
    static void destroy_elems(T * base, const size_t start,
                              const size_t count) noexcept
    {
      if constexpr (not std::is_trivially_destructible_v<T>)
        for (size_t i = 0; i < count; ++i)
          base[start + i].~T();
    }

    /// Destroy `count` elements starting at circular index `first`.
    static void destroy_elems_circular(T * base, const size_t first,
                                       const size_t count,
                                       const size_t mask) noexcept
    {
      if constexpr (not std::is_trivially_destructible_v<T>)
        for (size_t i = 0; i < count; ++i)
          base[(first + i) & mask].~T();
    }

    /// Default-construct `count` elements at `base[start..start+count)`.
    /// For trivially default-constructible types this is a no-op.
    static void default_construct(T * base, const size_t start,
                                  const size_t count)
    {
      size_t done = 0;
      try
        {
          for (; done < count; ++done)
            new (base + start + done) T;
        }
      catch (...)
        {
          destroy_elems(base, start, done);
          throw;
        }
    }

    /// Move-construct `count` elements from src into dst (raw memory).
    static void move_construct(T * dst, T * src,
                               const size_t count) noexcept
    {
      for (size_t i = 0; i < count; ++i)
        new (dst + i) T(std::move(src[i]));
    }

    /// Copy-construct `count` elements from src into dst (raw memory).
    static void copy_construct(T * dst, const T * src, const size_t count)
    {
      size_t done = 0;
      try
        {
          for (; done < count; ++done)
            new (dst + done) T(src[done]);
        }
      catch (...)
        {
          for (size_t j = 0; j < done; ++j)
            dst[j].~T();
          throw;
        }
    }

    template <typename Index_Fn>
    static void relocate_construct(T * dst, T * src,
                                   const size_t count,
                                   Index_Fn index_fn)
    {
      size_t done = 0;
      try
        {
          for (; done < count; ++done)
            {
              T & item = src[index_fn(done)];
              if constexpr (std::is_nothrow_move_constructible_v<T>
                            or not std::is_copy_constructible_v<T>)
                new (dst + done) T(std::move(item));
              else
                new (dst + done) T(item);
            }
        }
      catch (...)
        {
          destroy_elems(dst, 0, done);
          throw;
        }
    }

    /// Allocate raw memory for the current dimension (no construction)
    void allocate()
    {
      assert(is_power_of_2(dim));
      ptr = raw_alloc(dim);
      contract_threshold = dim / 4;
    }

    /** Test is array is full and if affrimative, then expand the array
        twice as long and copy the content by swapping.

        This method first allocates a chunck of 2*dim and then copies from
        `first` index the `n` contained entries to the new chunck.

        @param[in] first index where is found the first item of array
        @return `true` if the array was full and then a new twice as long
        was allocated
        @throw bad_alloc if there is no enough memory
    */
    bool expand(const size_t first = 0)
    {
      assert(ptr);
      assert(is_power_of_2(dim));
      if (n < dim)
        return false;

      const size_t newsz = dim << 1; // 2*dim
      const size_t mask = dim - 1;
      const size_t curr_dim = dim;
      T *new_ptr = raw_alloc(newsz);
      try
        {
          relocate_construct(new_ptr, ptr, n,
                             [first, mask, curr_dim](const size_t i)
                             {
                               assert(((i + first) & mask)
                                      == ((i + first) % curr_dim));
                               return (i + first) & mask;
                             });
        }
      catch (...)
        {
          raw_dealloc(new_ptr);
          throw;
        }
      destroy_elems_circular(ptr, first, n, mask);
      raw_dealloc(ptr);
      ptr = new_ptr;
      dim = newsz;
      contract_threshold = dim / 4;

      return true;
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

      const size_t newsz = dim >> 1; // dim/2

      if (newsz <= Min_Dim)
        return false;

      T *new_ptr = raw_alloc(newsz);
      const size_t mask = dim - 1;
      const size_t curr_dim = dim;
      try
        {
          relocate_construct(new_ptr, ptr, n,
                             [first, mask, curr_dim](const size_t i)
                             {
                               assert(((first + i) & mask)
                                      == ((first + i) % curr_dim));
                               return (first + i) & mask;
                             });
        }
      catch (...)
        {
          raw_dealloc(new_ptr);
          throw;
        }
      destroy_elems_circular(ptr, first, n, mask);

      raw_dealloc(ptr);
      ptr = new_ptr;
      dim = newsz;
      contract_threshold = dim / 4;

      return true;
    }

    /// Initialize the dimension of the array to `d` or to the next two power if `d` is not a two power.
    ///
    /// If `d` is 0 then `d` is set to `Min_Dim`.
    ///
    /// @param[in] d the proposed dimension of array
    void init_dim(size_t d) noexcept
    {
      if (d == 0)
        d = Min_Dim;

      size_t k = log2(d);
      dim = is_power_of_2(d) ? d : 1 << ++k;

      assert(dim >= d);
      assert(dim == 1 << k);
    }

   public:

    using Item_Type = T; /// The type of element of array

    /// Return the capacity of array (its dimension)
    [[nodiscard]] constexpr size_t capacity() const noexcept { return dim; }

    /// Return the number of elements
    size_t size() const noexcept { return n; }

    /// Return `true` is the array is empty
    bool is_empty() const noexcept { return n == 0; }

    /** Construct an array con capacity equal or gre+ater than `__dim`.

        @param[in] __dim proposed dimension of array which could be
        adjusted to the next two power greater than `__dim`
        @throw bad_alloc if there is not enough memory.
    */
    MemArray(size_t _dim = Min_Dim) : ptr(nullptr), n(0)
    {
      static_assert(std::is_move_constructible_v<T>,
                    "T must be move constructible");
      static_assert(std::is_move_assignable_v<T>,
                    "T must be move assignable");
      init_dim(_dim);
      allocate();
    }

    ~MemArray()
    {
      if (ptr != nullptr)
        {
          destroy_elems(ptr, 0, n);
          raw_dealloc(ptr);
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
    }

    /// Construct a copy of `a`
    MemArray(const MemArray &a)
      requires (std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
      : ptr(nullptr), dim(a.dim), n(a.n)
    {
      allocate();
      copy_construct(ptr, a.ptr, n);
    }

    /// Construct an array moved of rvalue `a`
    MemArray(MemArray &&a) noexcept
      : ptr(nullptr), dim(0), n(0), contract_threshold(0)
    {
      assert(a.ptr);
      swap(a);
    }

    /// Assign by copy `a` to `this`
    MemArray &operator=(const MemArray &a)
      requires (std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)
    {
      if (this == &a)
        return *this;

      assert(a.ptr);

      T *newptr = raw_alloc(a.dim);
      copy_construct(newptr, a.ptr, a.n);

      destroy_elems(ptr, 0, n);
      if (ptr != nullptr)
        raw_dealloc(ptr);
      ptr = newptr;
      dim = a.dim;
      n = a.n;
      contract_threshold = dim / 4;

      return *this;
    }

    /// Assign by moving `a` to `this`
    MemArray &operator=(MemArray &&a) noexcept
    {
      swap(a);
      return *this;
    }

    /// Empty the container. The array is not contracted
    void empty()
    {
      destroy_elems(ptr, 0, n);
      n = 0;
    }

    /// Empty the array and release all memory
    void empty_and_release()
    {
      destroy_elems(ptr, 0, n);
      n = 0;
      if (dim <= Min_Dim)
        return;

      assert(ptr);
      raw_dealloc(ptr);
      ptr = nullptr;
      dim = Min_Dim;
      allocate();
    }

    /// Put a copy of `item` at the end of the sequence. The array expands if
    /// this is already full
    T &put(const T &item)
      requires std::is_copy_constructible_v<T>
    {
      assert(ptr);
      expand();

      new (ptr + n) T(item);
      T &ret = ptr[n++];
      return ret;
    }

    /// Move `item` at the end of the sequence. The array expands if
    /// this is already full
    T &put(T &&item)
    {
      assert(ptr);
      expand();

      new (ptr + n) T(std::forward<T>(item));
      T &ret = ptr[n++];
      return ret;
    }

   private:

    void close_gap(size_t pos, size_t num_entries = 1)
    {
      Aleph::close_gap(ptr, n, pos, num_entries);
      get(num_entries);
    }

   public:

    /// Push a copy of `item` at the beginning of sequence. The array expands if
    /// this is already full
    T &push(const T &item)
      requires std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>
    {
      assert(ptr);

      expand();

      if (n == 0)
        {
          new (ptr) T(item);
          ++n;
          return ptr[0];
        }

      new (ptr + n) T(std::move(ptr[n - 1]));
      for (size_t i = n - 1; i > 0; --i)
        ptr[i] = std::move(ptr[i - 1]);

      ptr[0] = item;
      ++n;
      T &ret = ptr[0];
      return ret;
    }

    /// Push a copy of `item` at the beginning of sequence. The array expands if
    /// this is already full
    T &push(T &&item)
    {
      assert(ptr);

      expand();

      if (n == 0)
        {
          new (ptr) T(std::forward<T>(item));
          ++n;
          return ptr[0];
        }

      new (ptr + n) T(std::move(ptr[n - 1]));
      for (size_t i = n - 1; i > 0; --i)
        ptr[i] = std::move(ptr[i - 1]);

      ptr[0] = std::forward<T>(item);
      ++n;
      T &ret = ptr[0];
      return ret;
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
    T pop() { return remove_first(); }

    /// \overload put(const T & item)
    T &append(const T &item)
      requires std::is_copy_assignable_v<T>
    {
      assert(ptr);
      return put(item);
    }

    /// \overload put(T && item)
    T &append(T &&item)
    {
      assert(ptr);
      return put(std::forward<T>(item));
    }

    /// \overload push(const T & item)
    T &insert(const T &item)
      requires std::is_copy_assignable_v<T>
    {
      assert(ptr);
      return push(item);
    }

    /// \overload push(T && item)
    T &insert(T &&item)
    {
      assert(ptr);
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
      requires std::is_default_constructible_v<T>
    {
      assert(ptr);
      if (more == 0)
        return;

      const size_t new_n = n + more;
      if (new_n <= dim)
        {
          default_construct(ptr, n, more);
          n = new_n;
          return;
        }

      size_t new_dim = dim;
      while (new_dim < new_n)
        new_dim <<= 1; // dim = 2*dim

      T *new_ptr = raw_alloc(new_dim);
      try
        {
          relocate_construct(new_ptr, ptr, n,
                             [](const size_t i) { return i; });
          try
            {
              default_construct(new_ptr, n, more);
            }
          catch (...)
            {
              destroy_elems(new_ptr, 0, n);
              throw;
            }
        }
      catch (...)
        {
          raw_dealloc(new_ptr);
          throw;
        }

      destroy_elems(ptr, 0, n);
      raw_dealloc(ptr);
      ptr = new_ptr;
      dim = new_dim;
      n = new_n;
      contract_threshold = dim / 4;
    }

    MemArray &append(const MemArray &a)
      requires std::is_copy_constructible_v<T>
    {
      const size_t old_n = n;
      const size_t num_entries = a.size();
      if (num_entries == 0)
        return *this;

      reserve(old_n + num_entries);

      size_t done = 0;
      try
        {
          for (; done < num_entries; ++done)
            new (ptr + old_n + done) T(a.ptr[done]);
        }
      catch (...)
        {
          destroy_elems(ptr, old_n, done);
          throw;
        }

      n = old_n + num_entries;

      return *this;
    }

    /** Reserves `cap` cells into the array.

        @param[in] cap new dimension
        @throw bad_alloc if there is no enough memory
    */
    void reserve(const size_t cap)
    {
      assert(ptr);
      if (cap < dim)
        return;

      size_t k = log2(cap);
      const size_t new_dim = is_power_of_2(cap) ? cap : 1 << ++k;

      T *new_ptr = raw_alloc(new_dim);
      try
        {
          relocate_construct(new_ptr, ptr, n,
                             [](const size_t i) { return i; });
        }
      catch (...)
        {
          raw_dealloc(new_ptr);
          throw;
        }

      destroy_elems(ptr, 0, n);
      raw_dealloc(ptr);
      ptr = new_ptr;
      dim = new_dim;
      contract_threshold = dim / 4;
    }

    /// Remove `i` elements from the end. Return the value of the last
    /// removed element
    T get(const size_t i = 1)
    {
      assert(ptr);
      const long idx_long = static_cast<long>(n) - static_cast<long>(i);
      ah_underflow_error_if(idx_long < 0)
          << "MemArray::get(): deleted more entries than capacity";

      const size_t old_n = n;
      const auto idx = static_cast<size_t>(idx_long);
      T ret = std::move(ptr[idx]);
      destroy_elems(ptr, idx, old_n - idx);
      n = idx;

      contract();

      return ret;
    }

    T get_ne(const size_t i = 1) noexcept
    {
      assert(ptr);
      const size_t old_n = n;
      const size_t idx = n - i;
      T ret = std::move(ptr[idx]);
      destroy_elems(ptr, idx, old_n - idx);
      n = idx;

      contract();

      return ret;
    }

    /// \overload get()
    T remove_last() { return get(); }

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
    T &get_first() const { return first(); }

    /// \overload last
    T &get_last() const { return last(); }

    /// Reverse the order of items in array
    MemArray &reverse()
    {
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
      assert(ptr);
      ah_out_of_range_error_if(i >= n) << "access out of range";

      return ptr[i];
    }

    /// \overload access(const size_t i)
    T &operator()(const size_t i) const noexcept
    {
      assert(ptr);
      assert(i < dim);
      return ptr[i];
    }

    /** Traverse all the elements from index 0 to `n - 1` and execute
        `operation` on each on them.

        @param[in] operation to be performed on each element
        @return `true` if `operation` was executed on all elements;
        `false` otherwise.
    */
    template <class Operation>
    bool traverse(Operation &operation)
    {
      assert(ptr);
      for (int i = 0; i < n; i++)
        if (not operation(ptr[i]))
          return false;

      return true;
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation &operation) const
    {
      return const_cast<MemArray *>(this)->traverse(operation);
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation &&operation) const
    {
      return traverse<Operation>(operation);
    }

    /// \overload traverse(Operation & operation)
    template <class Operation>
    bool traverse(Operation &&operation)
    {
      return traverse<Operation>(operation);
    }

    bool is_valid() const noexcept { return ptr; }

    /** Simple iterator on elements of an array

        @ingroup Sequences
    */
    struct Iterator : public Array_Iterator<T>
    {
      using Base = Array_Iterator<T>;
      using Base::Base;

      /// Construct an iterator on array `a`
      Iterator(const MemArray<T> &a) noexcept
        : Array_Iterator<T>(no_exception_ctor, a.ptr, a.dim, a.n)
      {
        assert(a.ptr != nullptr);
      }
    };
  };

} // end namespace Aleph

# endif // TPL_MEMARRAY_H
