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


/** @file tpl_arrayHeap.H
    @brief Fixed-capacity binary heap and heapsort algorithms.

    This file provides:
    - `sift_up` and `sift_down` heap operations
    - `heapsort` and `faster_heapsort` sorting algorithms
    - `ArrayHeap<T, Compare>` fixed-capacity priority queue

    All operations use 1-based indexing internally for simpler
    parent/child arithmetic.

    @ingroup Trees
 *  @author Leandro Rabindranath León
*/

#ifndef TPL_ARRAYHEAP_H
#define TPL_ARRAYHEAP_H

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include <ahFunction.H>
#include <ahUtils.H>
#include <ahDefs.H>
#include <ahAssert.H>
#include <array_it.H>
#include <htlist.H>
#include <tpl_dynDlist.H>
#include <ah-args-ctor.H>
#include <ahDry.H>
#include <ah-dry.H>
#include <ah-errors.H>

namespace Aleph
{
  /** Restore the heap property by moving the element at position `r` upwards.

      This operation assumes the range `[l, r-1]` already satisfies the heap
      property and then restores it for the whole range `[l, r]`.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in,out] ptr 1-based array backing the heap
      @param[in] l left boundary (inclusive)
      @param[in] r right boundary (inclusive)
      @param[in] cmp comparison criteria
      @return a mutable reference to the element after it has been placed
      @ingroup Algorithms
  */
  template <typename T, class Compare>
  inline
  T &sift_up(T *ptr, const size_t l, const size_t r, Compare & cmp)
  {
    size_t i = r;
    for (size_t p; i > l; i = p)
      {
        p = u_index(i); // parent index (p = i / 2)

        // Prefetch grandparent for next iteration
        if (const size_t gp = u_index(p); gp >= l)
          __builtin_prefetch(&ptr[gp], 0, 1);

        if (cmp(ptr[p], ptr[i])) [[likely]] // does the heap property hold?
            return ptr[i]; // yes, the entire range is already a heap

        std::swap(ptr[p], ptr[i]); // swap nodes and restore level p
      }

    return ptr[i];
  }

  /** Restore the heap property by moving the element at position `l` downwards.

      This operation assumes the subtrees below `l` satisfy the heap property
      and then restores it for the whole range `[l, r]`.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in,out] ptr 1-based array backing the heap
      @param[in] l left boundary (inclusive) / starting index
      @param[in] r right boundary (inclusive)
      @param[in] cmp comparison criteria
      @ingroup Algorithms
  */
  template <typename T, class Compare>
  inline
  void sift_down(T *ptr, const size_t l, const size_t r, Compare & cmp)
  {
    size_t i = l;
    while (true)
      {
        size_t c = l_index(i); // left child index (c = 2 * i)
        if (c > r) [[unlikely]] // does it have a left child?
            return; // no ==> stop

        // Prefetch grandchildren for next iteration
        if (const size_t gc = l_index(c); gc <= r)
          __builtin_prefetch(&ptr[gc], 0, 1);

        if (c + 1 <= r) // does it have a right child?
          if (cmp(ptr[c + 1], ptr[c])) // yes ==> pick the smaller child
            c++;

        if (cmp(ptr[i], ptr[c])) [[likely]] // does the heap property hold?
            return; // yes ==> stop

        std::swap(ptr[c], ptr[i]);
        i = c;
      }
  }

  /** Restore the heap property by sifting down and then sifting up.

      Used when the element at `i` might need to move in either direction.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in,out] ptr 1-based array backing the heap
      @param[in] l left boundary (inclusive)
      @param[in] i position to update
      @param[in] r right boundary (inclusive)
      @param[in] cmp comparison criteria
      @ingroup Algorithms
  */
  template <typename T, class Compare>
  inline
  void sift_down_up(T *ptr, const size_t l, const size_t i, const size_t r,
                    Compare & cmp)
  {
    sift_down<T, Compare>(ptr, i, r, cmp);
    sift_up<T, Compare>(ptr, l, i, cmp);
  }

  /** Sort an array using the heapsort algorithm.

      `heapsort(array, n)` builds a heap on top of the range and performs
      repeated extractions. The running time is \f$O(n \log n)\f$ and does
      not require additional memory.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in,out] array pointer to the first element to sort
      @param[in] n number of elements in the range
      @param[in] cmp comparison function (default: Aleph::less<T>)
      @see insertion_sort() quicksort_rec() mergesort()
      @see selection_sort() faster_heapsort()
      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void heapsort(T *array, const size_t n, const Compare & cmp = Compare())
  {
    Inversed_Compare<T, Compare> inv_cmp(cmp);

    --array; // shift backwards so array[1] is the first element
    for (size_t i = 2; i <= n; ++i)
      sift_up<T, Inversed_Compare<T, Compare>>(array, 1, i, inv_cmp);
    for (size_t i = n; i > 1; --i)
      {
        std::swap(array[1], array[i]); // place the i-th item at the root
        sift_down<T, Inversed_Compare<T, Compare>>(array, 1, i - 1, inv_cmp);
      }
  }

  /** Optimized version of heapsort.

      `faster_heapsort(array, n)` builds the heap bottom-up (starting at
      `n / 2`) and therefore performs fewer comparisons during the build
      phase.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in,out] array pointer to the first element to sort
      @param[in] n number of elements in the range
      @param[in] cmp comparison function (default: Aleph::less<T>)
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      @see selection_sort()
      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void faster_heapsort(T *array, const size_t n, const Compare & cmp = Compare())
  {
    Inversed_Compare<T, Compare> inv_cmp(cmp);

    --array; // shift backwards so array[1] is the first element
    for (size_t i = n / 2; i >= 1; --i)
      sift_down(array, i, n, inv_cmp);
    for (size_t i = n; i > 1; --i)
      {
        std::swap(array[1], array[i]); // place the i-th item at the root
        sift_down(array, 1, i - 1, inv_cmp);
      }
  }

  /** Check whether a range satisfies the heap property.

      @tparam T element type
      @tparam Compare comparator defining the heap order
      @param[in] array 1-based array backing the heap
      @param[in] l left boundary (inclusive)
      @param[in] r right boundary (inclusive)
      @param[in] cmp comparison criteria
      @return true if `[l, r]` satisfies the heap property under `cmp`
      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  bool valid_heap(T *array, const size_t l, const size_t r,
                  const Compare & cmp = Compare())
  {
    size_t i;
    for (i = l_index(l) /* i = 2*l */; i <= r; i++)
      if (cmp(array[i], array[u_index(i)]))
        break;
    return i > r;
  }

  /** Fixed-capacity binary heap backed by a raw array.

      The heap stores elements from index 1 to `num_items` and keeps
      index 0 as sentinel, which simplifies the `parent/child`
      arithmetic.

      @tparam T element type
      @tparam Compare comparator defining the priority order

      @see DynArrayHeap BinHeap DynBinHeap
      @ingroup Trees
  */
  template <typename T, class Compare = Aleph::less<T>>
  class ArrayHeap : public LocateFunctions<ArrayHeap<T, Compare>, T>,
                    public FunctionalMethods<ArrayHeap<T, Compare>, T>,
                    public GenericItems<ArrayHeap<T, Compare>, T>,
                    public EqualToMethod<ArrayHeap<T, Compare>>,
                    public StlAlephIterator<ArrayHeap<T, Compare>>
  {
    T *array = nullptr;
    mutable size_t dim = 0;
    size_t num_items = 0;

    mutable bool array_allocated = false;

    Compare cmp;

    static size_t r_index(const size_t & i)
    {
      return (i << 1) + 1; // multiply i by 2 and add 1
    }

  public:
    /** Allocate internal storage and reset the heap.

        @param[in] new_dim new capacity (number of usable heap slots)
        @throw invalid_argument if `new_dim == 0`
    */
    void allocate_storage(const size_t new_dim)
    {
      ah_invalid_argument_if(new_dim == 0) << "Heap capacity must be positive";

      T *ptr = new T[new_dim + 1];
      if (array_allocated)
        delete [] array;

      array = ptr;
      dim = new_dim;
      num_items = 0;
      array_allocated = true;
    }

    /** Swap all state with another heap.

        @param[in,out] h other heap
    */
    void swap(ArrayHeap & h) noexcept
    {
      std::swap(array, h.array);
      std::swap(dim, h.dim);
      std::swap(num_items, h.num_items);
      std::swap(array_allocated, h.array_allocated);
      std::swap(cmp, h.cmp);
    }

    using Item_Type = T;

    using Key_Type = T;

    Special_Ctors(ArrayHeap, T);

    /** Construct an empty heap with internal storage.

        @param[in] d capacity (number of usable heap slots)
        @param[in] cmp_fct comparator
        @throw invalid_argument if `d == 0`
    */
    ArrayHeap(const size_t d = 1024, Compare cmp_fct = Compare())
      : array(nullptr), cmp(cmp_fct)
    {
      allocate_storage(d);
    }

    /** Construct an empty heap using an external buffer.

        The heap will not manage the lifetime of `ptr`.

        @param[in] ptr external buffer (must have at least `d + 1` elements)
        @param[in] d capacity (number of usable heap slots)
        @param[in] cmp_fct comparator
        @throw invalid_argument if `ptr == nullptr` or `d == 0`
    */
    ArrayHeap(T *ptr, const size_t & d, Compare cmp_fct = Compare())
      : array(ptr), dim(d), cmp(cmp_fct)
    {
      ah_invalid_argument_if(ptr == nullptr or d == 0) << "ArrayHeap requires non-null buffer";
    }

    ArrayHeap(const ArrayHeap & h)
      : array(nullptr), cmp(h.cmp)
    {
      allocate_storage(h.dim);
      num_items = h.num_items;
      for (size_t i = 1; i <= num_items; ++i)
        array[i] = h.array[i];
    }

    ArrayHeap(ArrayHeap && h) noexcept
      : cmp(h.cmp)
    {
      swap(h);
    }

    ArrayHeap &operator =(const ArrayHeap & h)
    {
      if (this == &h)
        return *this;

      if (dim < h.dim)
        allocate_storage(h.dim);

      num_items = h.num_items;
      for (size_t i = 1; i <= num_items; ++i)
        array[i] = h.array[i];
      cmp = h.cmp;

      return *this;
    }

    ArrayHeap &operator =(ArrayHeap && h)
      noexcept
    {
      swap(h);
      return *this;
    }

    /** Destructor. */
    virtual ~ArrayHeap()
    {
      if (array_allocated and array != nullptr)
        delete [] array;
    }

    /** Return a mutable reference to the minimum element.

        @throw underflow_error if the heap is empty
    */
    T &top()
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      return array[1];
    }

    /** Return a const reference to the minimum element.

        @throw underflow_error if the heap is empty
    */
    const T &top() const
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      return array[1];
    }

    /** Insert an element without checking capacity.

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @warning This method does not check for overflow.
        @see insert()
    */
    T &insert_ne(const T & key)
    {
      array[++num_items] = key; // place the new element
      return sift_up(array, 1, num_items, cmp);
    }

    /** Insert an element without checking capacity (move overload).

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @warning This method does not check for overflow.
        @see insert()
    */
    T &insert_ne(T && key)
    {
      array[++num_items] = std::move(key); // place the new element
      return sift_up(array, 1, num_items, cmp);
    }

    /** Insert an element into the heap.

        `insert(key)` stores a copy of `key` inside the heap.

        @param[in] key value to insert.
        @return a mutable reference to the inserted element.
        @throw overflow_error if the internal array is full.
    */
    T &insert(const T & key)
    {
      ah_overflow_error_if(num_items >= dim) << "Heap out of capacity";
      return insert_ne(key);
    }

    /** Insert an element into the heap (move overload).

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @throw overflow_error if the internal array is full
        @see insert(const T &)
    */
    T &insert(T && key)
    {
      ah_overflow_error_if(num_items >= dim) << "Heap out of capacity";
      return insert_ne(std::move(key));
    }

    /** Synonym for insert().

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @see insert()
    */
    T &put(const T & key) { return insert(key); }

    /** Synonym for insert().

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @see insert()
    */
    T &append(const T & key) { return insert(key); }

    /** Synonym for insert() (move overload).

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @see insert(T &&)
    */
    T &put(T && key) { return insert(std::move(key)); }

    /** Synonym for insert() (move overload).

        @param[in] key value to insert
        @return a mutable reference to the inserted element
        @see insert(T &&)
    */
    T &append(T && key) { return insert(std::move(key)); }

    /** Remove the smallest element in the heap and return a copy of its value.

        `getMin()` extracts the element with the lowest value according to the
        configured comparator.

        @throw underflow_error if the heap is empty.
        @return a copy of the removed value.
    */
    T getMin()
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      T ret_val = array[1];
      array[1] = array[num_items--];
      if (num_items > 0)
        sift_down(array, 1, num_items, cmp);
      return ret_val;
    }

    /** \overload getMin() */
    T get()
    {
      return getMin();
    }

    /** \overload getMin() */
    T getMax()
    {
      return getMin();
    }

    /** Return the number of elements currently stored. */
    [[nodiscard]] constexpr size_t size() const noexcept { return num_items; }

    /** Return true if the heap is empty. */
    [[nodiscard]] constexpr bool is_empty() const noexcept { return num_items == 0; }

    /** Return the maximum number of elements that can be stored.

        This excludes the sentinel slot at index 0.
    */
    [[nodiscard]] constexpr size_t capacity() const noexcept { return dim; }

    /** Update the priority of an element stored in the heap.

        `update(data)` takes a reference to an element inside the heap,
        presumably modified, and updates its priority inside the heap.

        The reference must have been obtained via a previous call to
        `insert()`.

        @param[in] data reference to the element inside the heap to update
        @see insert()
        @note It is essential that `data` is a valid reference into this heap.
        Results are unpredictable (and probably fatal) if this is not the case.
    */
    void update(T & data)
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      assert(&data >= array + 1 and &data <= array + num_items);

      const auto i = static_cast<size_t>(&data - array);
      sift_down_up(array, 1, i, num_items, cmp);
    }

    /** Remove an element from the heap given a reference to it.

        The reference must refer to an element stored in this heap.

        @param[in,out] item reference to the element to remove
        @throw underflow_error if the heap is empty
        @note It is essential that `item` is a valid reference into this heap.
    */
    void remove(T & item)
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      assert(&item >= array + 1 and &item <= array + num_items);

      const auto idx = static_cast<size_t>(&item - array);
      if (idx == num_items)
        {
          --num_items;
          return;
        }

      array[idx] = std::move(array[num_items--]);
      sift_down_up(array, 1, idx, num_items, cmp);
    }

    /** Return a mutable reference to the i-th entry (1-based).

        No bounds checking is performed.

        @param[in] i index
        @return mutable reference to entry `i`
    */
    T &operator [](const size_t i)
    {
      return array[i];
    }

    /** Const overload of operator[] (1-based).

        @param[in] i index
        @return const reference to entry `i`
    */
    const T &operator [](const size_t i) const
    {
      return array[i];
    }

    struct Iterator : public Array_Iterator<T>
    {
      Iterator(const ArrayHeap & h) noexcept
        : Array_Iterator<T>(no_exception_ctor, h.array + 1, h.dim, h.num_items)
      { /* empty */
      }
    };

  private:
    // superfast array traversal
    template <class Operation>
    bool traverse_impl(Operation & operation)
    {
      for (size_t i = 1; i <= num_items; ++i)
        if (not operation(array[i]))
          return false;

      return true;
    }

  public:
    /** Traverse all elements (const overload).

        Iterates over indices `[1, size()]` and applies `operation` to each
        element until it returns false.

        @tparam Operation callable type
        @param[in,out] operation functor to apply
        @return false if traversal was stopped early, true otherwise
    */
    template <class Operation>
    bool traverse(Operation & operation) const
    {
      return const_cast<ArrayHeap &>(*this).traverse_impl(operation);
    }

    /** Traverse all elements.

        @tparam Operation callable type
        @param[in,out] operation functor to apply
        @return false if traversal was stopped early, true otherwise
    */
    template <class Operation>
    bool traverse(Operation & operation)
    {
      return traverse_impl(operation);
    }

    /** Traverse all elements (const forwarding overload).

        @tparam Operation callable type
        @param[in] operation functor to apply
        @return false if traversal was stopped early, true otherwise
    */
    template <class Operation>
    bool traverse(Operation && operation = Operation()) const
    {
      return const_cast<ArrayHeap &>(*this).traverse_impl(operation);
    }

    /** Traverse all elements (forwarding overload).

        @tparam Operation callable type
        @param[in] operation functor to apply
        @return false if traversal was stopped early, true otherwise
    */
    template <class Operation>
    bool traverse(Operation && operation = Operation())
    {
      return traverse_impl(operation);
    }
  };
} // end namespace Aleph
# endif /* TPL_ARRAYHEAP_H */
