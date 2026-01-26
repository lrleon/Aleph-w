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


/** @file tpl_dynArrayHeap.H
 *  @brief Array-based dynamic binary heap.
 *
 *  Binary heap using DynArray for automatic growth.
 *  Efficient for priority queues with unknown size.
 *
 *  ## Features
 *  - O(log n) insert, extract-min
 *  - Automatic resizing
 *  - Cache-friendly array storage
 *
 *  @see tpl_binHeap.H Fixed-size array heap
 *  @see tpl_dynBinHeap.H Node-based heap
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNARRAYHEAP_H
# define TPL_DYNARRAYHEAP_H

# include <algorithm>
# include <cstddef>
# include <stdexcept>
# include <utility>

# include <tpl_dynArray.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Restore heap order by moving an element up.
   *
   * Restores the heap invariant in the subrange [l, r] by sifting the element
   * initially at index `r` upward until the heap property holds.
   *
   * @return The final index where the sifted element ended up.
   */
  template <typename T, class Compare>
  inline
  size_t sift_up(DynArray<T> & a, const size_t l, const size_t r, Compare & cmp) noexcept
  {
    for (size_t p, i = r; i > l; i = p)
      {
        p = u_index(i); // parent index (p = i / 2)

        // Prefetch grandparent for next iteration
        if (const size_t gp = u_index(p); gp >= l)
          __builtin_prefetch(&a.access(gp), 0, 1);

        T & ap = a.access(p);
        T & ai = a.access(i);
        if (cmp(ap, ai)) [[likely]] // heap property holds
            return i;

        std::swap(ap, ai); // swap and continue at parent
      }

    return l;
  }

  /** @brief Restore heap order by moving an element down.
   *
   * Restores the heap invariant in the subrange [l, r] by sifting the element
   * initially at index `l` downward until the heap property holds.
   */
  template <typename T, class Compare>
  inline
  void sift_down(DynArray<T> & a, const size_t l, const size_t r, Compare & cmp) noexcept
  {
    size_t i = l;
    while (true)
      {
        size_t c = l_index(i); // left child index (c = 2 * i)
        if (c > r) [[unlikely]] // no left child
            return;

        // Prefetch grandchildren for next iteration
        if (const size_t gc = l_index(c); gc <= r)
          __builtin_prefetch(&a.access(gc), 0, 1);

        T *ac = &a.access(c);
        if (c + 1 <= r) // right child exists
          {
            T *ac1 = &a.access(c + 1);
            if (cmp(*ac1, *ac)) // pick the child with higher priority
              {
                c++;
                ac = ac1;
              }
          }

        T & ai = a.access(i);
        if (cmp(ai, *ac)) [[likely]] // heap property holds
            return;

        std::swap(*ac, ai);
        i = c;
      }
  }


  /** Dynamic heap (priority queue) backed by `DynArray`.

      @tparam T element type stored in the heap
      @tparam Compare comparator defining the priority order

      @see BinHeap DynBinHeap ArrayHeap
      @ingroup Trees
  */
  template <typename T, class Compare = Aleph::less<T>>
  class DynArrayHeap : public LocateFunctions<DynArrayHeap<T, Compare>, T>,
                       public FunctionalMethods<DynArrayHeap<T, Compare>, T>,
                       public GenericKeys<DynArrayHeap<T, Compare>, T>,
                       public EqualToMethod<DynArrayHeap<T, Compare>>,
                       public StlAlephIterator<DynArrayHeap<T, Compare>>
  {
    DynArray<T> array;
    size_t num_items = 0;

    Compare cmp;

    static size_t r_index(const size_t & i) noexcept
    {
      return (i << 1) + 1; // right child index
    }

  public:
    using Item_Type = T;

    /// Default constructor.
    DynArrayHeap(Compare cmp_fct = Compare()) : cmp(cmp_fct)
    {
      // empty
    }

    Special_Ctors(DynArrayHeap, T);

    Args_Ctor(DynArrayHeap, T);

    /** Return the element with highest priority (the heap top).
     *
     * With the default comparator (`Aleph::less<T>`), this is the smallest
     * element.
     *
     * @throw underflow_error if the heap is empty.
     */
    T &top()
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      return array.access(1);
    }

    /// Const overload of top().
    const T &top() const
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      return array.access(1);
    }

    /** Insert a copy of @p key into the heap.
     *
     * @param[in] key Key to insert.
     * @return A modifiable reference to the stored element.
     *
     * @note This reference points to an element inside the heap and can become
     * invalid after further insertions/removals.
     */
    T &insert(const T & key)
    {
      array.touch(++num_items) = key; // place new element
      const size_t pos = sift_up(array, 1, num_items, cmp);
      return array.access(pos);
    }

    /** Insert a key by moving it into the heap.
     *
     * @param[in] key Key to move.
     * @return A modifiable reference to the stored element.
     */
    T &insert(T && key)
    {
      array.touch(++num_items) = std::move(key); // place new element
      const size_t pos = sift_up(array, 1, num_items, cmp);
      return array.access(pos);
    }

    /** Ensure the underlying array has capacity for at least @p n elements.
     *
     * @throw out_of_range_error if @p n is smaller than the current size.
     */
    void reserve(size_t n)
    {
      ah_out_of_range_error_if(num_items > n) << "DynArrayHeap::reserve: n smaller than size";
      array.reserve(n);
    }

    /** Insert by directly indexing into the backing array.
     *
     * This variant uses `operator()` instead of `touch()`.
     */
    T &insert_direct(const T & key)
    {
      array(++num_items) = key; // place new element
      const size_t pos = sift_up(array, 1, num_items, cmp);
      return array.access(pos);
    }

    /// Move overload of insert_direct().
    T &insert_direct(T && key)
    {
      array(++num_items) = std::move(key); // place new element
      const size_t pos = sift_up(array, 1, num_items, cmp);
      return array.access(pos);
    }

    /// Alias for insert().
    T &put(const T & key) { return insert(key); }

    /// Alias for insert() (move overload).
    T &put(T && key) { return insert(std::move(key)); }

    /// Alias for insert().
    T &append(const T & key) { return insert(key); }

    /// Alias for insert() (move overload).
    T &append(T && key) { return insert(std::move(key)); }

    /** Remove and return the top element.
     *
     * With the default comparator (`Aleph::less<T>`), this removes and returns
     * the smallest element.
     *
     * @throw underflow_error if the heap is empty.
     */
    T getMin()
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      T & a1 = array(1);
      T ret_val = std::move(a1);
      if (num_items > 1)
        {
          a1 = std::move(array(num_items));
          --num_items;
          sift_down(array, 1, num_items, cmp);
        }
      else
        --num_items;

      array.cut(num_items + 1);

      return ret_val;
    }

    /// \overload getMin()
    T get()
    {
      return getMin();
    }

    /// \overload getMin() (useful if priority order is inverted).
    T getMax()
    {
      return getMin();
    }

    /// Return the number of elements.
    [[nodiscard]] constexpr size_t size() const noexcept { return num_items; }

    /// Return true if the heap is empty.
    [[nodiscard]] constexpr bool is_empty() const noexcept { return num_items == 0; }

    struct Iterator : DynArray<T>::Iterator
    {
      using Base = typename DynArray<T>::Iterator;

      Iterator(const DynArrayHeap & h) noexcept : Base(h.array)
      {
        if (h.num_items != 0)
          this->next_ne();
      }

      Iterator() = default;

      [[nodiscard]] bool has_curr() const noexcept
      {
        return this->curr_idx != 0 and this->curr_idx != this->array_ptr->size();
      }

      [[nodiscard]] long get_pos() const noexcept { return this->Base::get_pos() - 1; }
    };

    /** Traverse all elements in the heap.
     *
     * Iteration order is the internal heap array order (not sorted).
     *
     * The traversal stops early if the operation returns false.
     */
    template <class Operation>
    bool traverse(Operation & operation)
    {
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        if (not operation(it.get_curr()))
          return false;
      return true;
    }

    template <class Operation>
    bool traverse(Operation & operation) const
    {
      return const_cast<DynArrayHeap &>(*this).traverse<Operation>(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation()) const
    {
      return traverse<Operation>(operation);
    }

    template <class Operation>
    bool traverse(Operation && operation = Operation())
    {
      return traverse<Operation>(operation);
    }
  };
} // end namespace Aleph

# endif // TPL_DYNARRAYHEAP_H
