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
 *  @note Due to the underlying DynArray directory, segment, and block
 *  structuring, not all entries in the heap are contiguous in memory. Only
 *  the entries within the same block are contiguous.
 *
 *  @see tpl_binHeap.H Fixed-size array heap
 *  @see tpl_dynBinHeap.H Node-based heap
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNARRAYHEAP_H
# define TPL_DYNARRAYHEAP_H

# include <ah-concepts.H>
# include <algorithm>
# include <climits>
# include <cstddef>
# include <stdexcept>
# include <type_traits>
# include <utility>

# include <tpl_dynArray.H>
# include <ah-errors.H>
#include <ah-iterator.H>

namespace Aleph
{
  namespace dynarrayheap_detail
  {
    /// @brief The places that an item passes, in order, when it is sifted.
    template <typename T>
    struct Way
    {
      T *at[sizeof(size_t) * CHAR_BIT]; ///< The places, from the first one.
      size_t len = 0; ///< The number of places.
    };

    /** @brief Find where an item that enters at the end of a heap goes.
     *
     *  The comparisons of sift_up(), which move nothing.
     *
     *  @param[in] a the array of the heap.
     *  @param[in] l the first index of the heap.
     *  @param[in] r the index where `x` enters, after the heap [l, r - 1].
     *  @param[in] x the item that enters.
     *  @param[in] cmp the comparison.
     *  @param[out] way the parents that `x` passes, in order.
     *  @return the index where `x` goes.
     *  @throws what the comparison throws; nothing when it is `noexcept`.
     */
    template <typename T, class Compare>
    inline
    size_t up_index(DynArray<T> & a, const size_t l, const size_t r, const T & x,
                    Compare & cmp, Way<T> & way)
      noexcept(Aleph::NothrowPredicateWith<Compare &, T &, T &>)
    {
      size_t i = r;
      while (i > l)
        {
          const size_t p = u_index(i); // parent index (p = i / 2)

          // Prefetch grandparent for next iteration
          if (const size_t gp = u_index(p); gp >= l)
            __builtin_prefetch(&a.access(gp), 0, 1);

          T & ap = a.access(p);
          if (cmp(ap, x)) [[likely]] // heap property holds
            break;
          way.at[way.len++] = &ap;
          i = p;
        }
      return i;
    }

    /** @brief Find where an item that enters at the root of a heap goes.
     *
     *  The comparisons of sift_down(), which move nothing and do not read
     *  the entry `l`.
     *
     *  @param[in] a the array of the heap.
     *  @param[in] l the index where `x` enters, the root of the heap [l, r].
     *  @param[in] r the last index of the heap.
     *  @param[in] x the item that enters.
     *  @param[in] cmp the comparison.
     *  @param[out] way the children that `x` passes, in order.
     *  @return the index where `x` goes.
     *  @throws what the comparison throws; nothing when it is `noexcept`.
     */
    template <typename T, class Compare>
    inline
    size_t down_index(DynArray<T> & a, const size_t l, const size_t r, const T & x,
                      Compare & cmp, Way<T> & way)
      noexcept(Aleph::NothrowPredicateWith<Compare &, T &, T &>)
    {
      size_t i = l;
      while (true)
        {
          size_t c = l_index(i); // left child index (c = 2 * i)
          if (c > r) [[unlikely]] // no left child
            return i;

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

          if (cmp(x, *ac)) [[likely]] // heap property holds
            return i;
          way.at[way.len++] = ac;
          i = c;
        }
    }

    /** @brief Move an item along its way.
     *
     *  Each item of the way takes the place of the one before it, the first
     *  one that of `first`, and `x` takes the last place.
     *
     *  @param[in,out] first the place where `x` enters.
     *  @param[in] way the places that `x` passes, from up_index() or
     *             down_index().
     *  @param[in,out] x the item, left moved from.
     */
    template <typename T>
    inline
    void shift(T & first, const Way<T> & way, T & x)
    {
      T *hole = &first;
      for (size_t k = 0; k < way.len; ++k)
        {
          *hole = std::move(*way.at[k]);
          hole = way.at[k];
        }
      *hole = std::move(x);
    }
  }

  /** @brief Restore heap order by moving an element up.
   *
   * Restores the heap invariant in the subrange [l, r] by sifting the element
   * initially at index `r` upward until the heap property holds. All the
   * comparisons come before the moves, so that if one throws, nothing has
   * moved.
   *
   * @return The final index where the sifted element ended up.
   * @throws what the comparison throws, with the subrange as it was;
   *         nothing when it is `noexcept`.
   * @note A move of the items that throws leaves the subrange out of
   *       order, and ends the program when the comparison does not throw.
   */
  template <typename T, class Compare>
  inline
  size_t sift_up(DynArray<T> & a, const size_t l, const size_t r, Compare & cmp)
    noexcept(Aleph::NothrowPredicateWith<Compare &, T &, T &>)
  {
    T & x = a.access(r);
    dynarrayheap_detail::Way<T> way;
    const size_t i = dynarrayheap_detail::up_index(a, l, r, x, cmp, way);
    if (i != r)
      {
        T moving = std::move(x);
        dynarrayheap_detail::shift(x, way, moving);
      }
    return i;
  }

  /** @brief Restore heap order by moving an element down.
   *
   * Restores the heap invariant in the subrange [l, r] by sifting the element
   * initially at index `l` downward until the heap property holds. All the
   * comparisons come before the moves, so that if one throws, nothing has
   * moved.
   *
   * @throws what the comparison throws, with the subrange as it was;
   *         nothing when it is `noexcept`.
   * @note A move of the items that throws leaves the subrange out of
   *       order, and ends the program when the comparison does not throw.
   */
  template <typename T, class Compare>
  inline
  void sift_down(DynArray<T> & a, const size_t l, const size_t r, Compare & cmp)
    noexcept(Aleph::NothrowPredicateWith<Compare &, T &, T &>)
  {
    T & x = a.access(l);
    dynarrayheap_detail::Way<T> way;
    if (dynarrayheap_detail::down_index(a, l, r, x, cmp, way) != l)
      {
        T moving = std::move(x);
        dynarrayheap_detail::shift(x, way, moving);
      }
  }


  /** Dynamic heap (priority queue) backed by `DynArray`.

      @tparam T element type stored in the heap
      @tparam Compare comparator defining the priority order

      @see BinHeap DynBinHeap ArrayHeap
      @ingroup Trees
  */
  template <typename T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  class DynArrayHeap : public LocateFunctions<DynArrayHeap<T, Compare>, T>,
                       public FunctionalMethods<DynArrayHeap<T, Compare>, T>,
                       public GenericKeys<DynArrayHeap<T, Compare>, T>,
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

    /** @brief Copy the elements and the comparator of another heap.
     *  @param[in] other heap to copy.
     */
    DynArrayHeap(const DynArrayHeap & other) = default;

    /** @brief Take the elements of another heap, which is left empty.
     *
     *  The implicit move took the array but copied the number of items:
     *  the moved-from heap then read entries that it no longer had. The
     *  comparator is copied, since the heap left empty still uses it.
     *
     *  @param[in,out] other heap to move from.
     */
    DynArrayHeap(DynArrayHeap && other) noexcept(
      std::is_nothrow_move_constructible_v<DynArray<T>> and
      std::is_nothrow_copy_constructible_v<Compare>)
      : array(std::move(other.array)), num_items(std::exchange(other.num_items, 0)),
        cmp(other.cmp)
    {}

    /** @brief Copy the elements and the comparator of another heap.
     *  @param[in] other heap to copy.
     *  @return this heap.
     */
    DynArrayHeap & operator = (const DynArrayHeap & other) = default;

    /** @brief Exchange this heap with another, which is left with the
     *         previous elements of this one.
     *  @param[in,out] other heap to move from.
     *  @return this heap.
     */
    DynArrayHeap & operator = (DynArrayHeap && other) noexcept(noexcept(swap(other)))
    {
      swap(other);
      return *this;
    }

    /** @brief Exchange in O(1) the elements and the comparators of this
     *         heap and another.
     *  @param[in,out] other heap to exchange with.
     *  @throw what swapping the comparators, or the default values of the
     *         arrays, throws. The comparators go first; if the swap of the
     *         arrays throws, they are exchanged back, and both heaps are
     *         left as they were, if the swap that threw left its arguments
     *         as they were. If the swap that puts the comparators back
     *         throws too, both heaps can only be destroyed or assigned.
     */
    void swap(DynArrayHeap & other) noexcept(
      std::is_nothrow_swappable_v<T> and std::is_nothrow_swappable_v<Compare>)
    {
      // the swap that the specification evaluates: that of Compare found by
      // its arguments, else std::swap(); first, since it may throw
      using std::swap;
      swap(cmp, other.cmp);
      if constexpr (std::is_nothrow_swappable_v<T>)
        array.swap(other.array);
      else
        try
          {
            array.swap(other.array);
          }
        catch (...)
          {
            swap(cmp, other.cmp);
            throw;
          }
      std::swap(num_items, other.num_items);
    }

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
     * @throws what the comparison, the copy of @p key or its place throw,
     *         with the heap as it was.
     * @note This reference points to an element inside the heap and can become
     * invalid after further insertions/removals.
     */
    T &insert(const T & key)
    {
      // The comparisons, the copy and the place come before any move, so
      // that if one throws, the heap is as it was
      const size_t r = num_items + 1;
      dynarrayheap_detail::Way<T> way;
      const size_t i = dynarrayheap_detail::up_index(array, 1, r, key, cmp, way);
      T item = key;
      dynarrayheap_detail::shift(array.touch(r), way, item);
      ++num_items;
      return array.access(i);
    }

    /** Insert a key by moving it into the heap.
     *
     * @param[in] key Key to move.
     * @return A modifiable reference to the stored element.
     * @throws what the comparison or the place of @p key throw, with the
     *         heap and @p key as they were.
     */
    T &insert(T && key)
    {
      const size_t r = num_items + 1;
      dynarrayheap_detail::Way<T> way;
      const size_t i = dynarrayheap_detail::up_index(array, 1, r, key, cmp, way);
      T & place = array.touch(r);
      T item = std::move(key); // key may be an item on the way
      dynarrayheap_detail::shift(place, way, item);
      ++num_items;
      return array.access(i);
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
     *
     * @throws what the comparison or the copy of @p key throw, with the
     *         heap as it was.
     */
    T &insert_direct(const T & key)
    {
      const size_t r = num_items + 1;
      dynarrayheap_detail::Way<T> way;
      const size_t i = dynarrayheap_detail::up_index(array, 1, r, key, cmp, way);
      T item = key;
      dynarrayheap_detail::shift(array.access(r), way, item);
      ++num_items;
      return array.access(i);
    }

    /// Move overload of insert_direct().
    T &insert_direct(T && key)
    {
      const size_t r = num_items + 1;
      dynarrayheap_detail::Way<T> way;
      const size_t i = dynarrayheap_detail::up_index(array, 1, r, key, cmp, way);
      T item = std::move(key);
      dynarrayheap_detail::shift(array.access(r), way, item);
      ++num_items;
      return array.access(i);
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
     * @throws what the comparison throws, with the heap as it was.
     */
    T getMin()
    {
      ah_underflow_error_if(num_items == 0) << "Heap is empty";

      // The comparisons come before any move, with the last item as if it
      // were at the root, so that if one throws, the heap is as it was
      const size_t last = num_items;
      dynarrayheap_detail::Way<T> way;
      if (last > 1)
        dynarrayheap_detail::down_index(array, 1, last - 1, array.access(last), cmp, way);
      T & root = array(1);
      T ret_val = std::move(root);
      if (last > 1)
        {
          T item = std::move(array.access(last));
          dynarrayheap_detail::shift(root, way, item);
        }
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
        return this->curr_idx != 0 and
               static_cast<size_t>(this->curr_idx) != this->array_ptr->size();
      }

      [[nodiscard]] long get_pos() const noexcept { return this->Base::get_pos() - 1; }

      /// The current item, read-only: the heap order must not be broken.
      [[nodiscard]] const T & get_curr() const { return Base::get_curr(); }

      /// \overload get_curr()
      [[nodiscard]] const T & get_curr_ne() const
        noexcept(noexcept(std::declval<const Base &>().get_curr_ne()))
      {
        return Base::get_curr_ne();
      }
    };

    /// Read-only iterator: the iterator of a constant container.
    using Const_Iterator = Aleph::Const_Iterator<Iterator>;

    /** Traverse all elements in the heap.
     *
     * Iteration order is the internal heap array order (not sorted).
     *
     * The traversal stops early if the operation returns false.
     */
    template <class Operation>
      requires Aleph::PredicateWith<Operation &, T &>
    bool traverse(Operation & operation)
      noexcept(Aleph::NothrowPredicateWith<Operation &, T &>)
    {
      for (size_t i = 1; i <= num_items; ++i)  // the items are entries 1 to num_items
        if (not operation(array.access(i)))
          return false;
      return true;
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, const T &>
    bool traverse(Operation & operation) const
      noexcept(Aleph::NothrowPredicateWith<Operation &, const T &>)
    {
      for (size_t i = 1; i <= num_items; ++i)
        if (not operation(static_cast<const T &>(array.access(i))))
          return false;
      return true;
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, const T &>
    bool traverse(Operation && operation = Operation()) const
    {
      return traverse<Operation>(operation);
    }

    template <class Operation>
      requires Aleph::PredicateWith<Operation &, T &>
    bool traverse(Operation && operation = Operation())
    {
      return traverse<Operation>(operation);
    }
  };
} // end namespace Aleph

# endif // TPL_DYNARRAYHEAP_H
