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
  template <typename T, class Compare>
  inline
  T &sift_up(T *ptr, const size_t l, const size_t r, Compare & cmp)
  {
    size_t i = r;
    for (size_t p; i > l; i = p)
      {
        p = u_index(i); // parent index (c = i / 2)
        if (cmp(ptr[p], ptr[i])) // does the heap property hold?
          return ptr[i]; // yes, the entire range is already a heap

        std::swap(ptr[p], ptr[i]); // swap nodes and restore level p
      }

    return ptr[i];
  }

  template <typename T, class Compare>
  inline
  void sift_down(T *ptr, const size_t l, const size_t r, Compare & cmp)
  {
    size_t i = l;
    while (true)
      {
        size_t c = l_index(i); // left child index (c = i / 2)
        if (c > r) // does it have a left child?
          return; // no ==> stop

        if (c + 1 <= r) // does it have a right child?
          if (cmp(ptr[c + 1], ptr[c])) // yes ==> pick the smaller child
            c++;

        if (cmp(ptr[i], ptr[c])) // does the heap property hold?
          return; // yes ==> stop

        std::swap(ptr[c], ptr[i]);
        i = c;
      }
  }

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
    Aleph::Inversed_Compare<T, Compare> inv_cmp(cmp);

    --array; // shift backwards so array[1] is the first element
    for (size_t i = 2; i <= n; ++i)
      sift_up<T, Aleph::Inversed_Compare<T, Compare>>(array, 1, i, inv_cmp);
    for (size_t i = n; i > 1; --i)
      {
        std::swap(array[1], array[i]); // place the i-th item at the root
        sift_down<T, Aleph::Inversed_Compare<T, Compare>>(array, 1, i - 1, inv_cmp);
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
    Aleph::Inversed_Compare<T, Compare> inv_cmp(cmp);

    --array; // shift backwards so array[1] is the first element
    for (size_t i = n / 2; i >= 1; --i)
      sift_down(array, i, n, inv_cmp);
    for (size_t i = n; i > 1; --i)
      {
        std::swap(array[1], array[i]); // place the i-th item at the root
        sift_down(array, 1, i - 1, inv_cmp);
      }
  }

  /// Returns true if the array between l and r is a heap under comparator
  /// Compare()()
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
    T * array = nullptr;
    mutable size_t dim = 0;
    size_t num_items = 0;

    mutable bool array_allocated = false;

    Compare cmp;

    static size_t r_index(const size_t & i)
    {
      return (i << 1) + 1; // multiply i by 2 and add 1
    }

  public:
    void allocate_storage(const size_t new_dim)
    {
      ah_invalid_argument_if(new_dim == 0)
        << "Heap capacity must be positive";

      T * ptr = new T[new_dim + 1];
      if (array_allocated)
        delete [] array;

      array = ptr;
      dim = new_dim;
      num_items = 0;
      array_allocated = true;
    }

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

    /// Constructor with default capacity.
    ArrayHeap(const size_t d = 1024, Compare && __cmp = Compare())
      : array(nullptr), dim(0), num_items(0), array_allocated(false), cmp(__cmp)
    {
      allocate_storage(d);
    }

    /// Constructor with external buffer and explicit capacity.
    ArrayHeap(T *ptr, const size_t & d, Compare && __cmp = Compare())
      : array(ptr), dim(d), num_items(0), array_allocated(false), cmp(__cmp)
    {
      ah_invalid_argument_if(ptr == nullptr || d == 0)
        << "ArrayHeap requires non-null buffer";
    }

    ArrayHeap(const ArrayHeap & h)
      : array(nullptr), dim(0), num_items(0), array_allocated(false), cmp(h.cmp)
    {
      allocate_storage(h.dim);
      num_items = h.num_items;
      for (size_t i = 1; i <= num_items; ++i)
        array[i] = h.array[i];
    }

    ArrayHeap(ArrayHeap && h) noexcept
      : array_allocated(false), cmp(h.cmp)
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

    /// Destructor.
    virtual ~ArrayHeap()
    {
      if (array_allocated and array != nullptr)
        delete [] array;
    }

    /// Return the smallest element in the heap.
    T & top()
    {
      ah_underflow_error_if(num_items == 0)
        << "Heap is empty";

      return array[1];
    }

    /// Return the smallest element in the heap (const overload).
    const T & top() const
    {
      ah_underflow_error_if(num_items == 0)
        << "Heap is empty";

      return array[1];
    }

    T & insert_ne(const T & key)
    {
      array[++num_items] = key; // place the new element
      return sift_up(array, 1, num_items, cmp);
    }

    T & insert_ne(T && key)
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
    T & insert(const T & key)
    {
      ah_overflow_error_if(num_items >= dim)
        << "Heap out of capacity";
      return insert_ne(key);
    }

    T & insert(T && key)
    {
      ah_overflow_error_if(num_items >= dim)
        << "Heap out of capacity";
      return insert_ne(std::move(key));
    }

    T &put(const T & key) { return insert(key); }

    T &append(const T & key) { return insert(key); }

    T &put(T && key) { return insert(std::move(key)); }

    T &append(T && key) { return insert(std::move(key)); }

    /** Remove the smallest element in the heap and return a copy of its value.

        `getMin()` extracts the element with the lowest value according to the
        configured comparator.

        @throw underflow_error if the heap is empty.
        @return a copy of the removed value.
    */
    T getMin()
    {
      ah_underflow_error_if(num_items == 0)
        << "Heap is empty";

      T ret_val = array[1];
      array[1] = array[num_items--];
      if (num_items > 0)
        sift_down(array, 1, num_items, cmp);
      return ret_val;
    }

    /// \overload getMin()
    T get()
    {
      return getMin();
    }

    /// \overload getMin() to use when the priority order is inverted
    T getMax()
    {
      return getMin();
    }

    /// Return the number of elements
    [[nodiscard]] constexpr size_t size() const noexcept { return num_items; }

    /// Return true if the heap is empty
    [[nodiscard]] constexpr bool is_empty() const noexcept { return num_items == 0; }

    /// Return the total capacity of the heap (excluding sentinel slot).
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
      ah_underflow_error_if(num_items == 0)
        << "Heap is empty";

      assert(&data >= array + 1 and &data <= array + num_items);

      const auto i = static_cast<size_t>(&data - array);
      sift_down_up(array, 1, i, num_items, cmp);
    }

    void remove(T & item)
    {
      ah_underflow_error_if(num_items == 0)
        << "Heap is empty";

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

    /// Return the i-th entry in the heap.
    T & operator [](const size_t i)
    {
      return array[i];
    }

    /// Const overload of operator[]
    const T & operator [](const size_t i) const
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
    bool __traverse(Operation & operation)
    {
      for (size_t i = 1; i <= num_items; ++i)
        if (not operation(array[i]))
          return false;

      return true;
    }

  public:
    template <class Operation>
    bool traverse(Operation & operation) const
    {
      return const_cast<ArrayHeap &>(*this).__traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation & operation)
    {
      return __traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation()) const
    {
      return const_cast<ArrayHeap &>(*this).__traverse(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation())
    {
      return traverse(operation);
    }
  };
} // end namespace Aleph
# endif /* TPL_ARRAYHEAP_H */
