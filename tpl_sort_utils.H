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


/** @file tpl_sort_utils.H

*  @brief Comprehensive sorting algorithms and search utilities for Aleph-w.
 *
 *  This file provides a complete collection of sorting algorithms and search
 *  utilities designed to work with arrays, dynamic arrays, and linked lists.
 *  All algorithms support custom comparison functors for flexible ordering.
 *
 *  ## Sorting Algorithms
 *
 *  | Algorithm | Time Complexity | Space | Stable | Best For |
 *  |-----------|-----------------|-------|--------|----------|
 *  | Selection Sort | O(n²) | O(1) | No | Very small arrays |
 *  | Insertion Sort | O(n²) | O(1) | Yes | Nearly sorted data |
 *  | Bubble Sort | O(n²) | O(1) | Yes | Educational purposes |
 *  | Shell Sort | O(n^1.5) | O(1) | No | Medium arrays |
 *  | Merge Sort | O(n log n) | O(n) or O(1) | Yes | Lists, guaranteed performance |
 *  | Quicksort | O(n log n)* | O(log n) | No | General purpose, fastest average |
 *  | Heapsort | O(n log n) | O(1) | No | Guaranteed O(n log n), in-place |
 *  | Bucket Sort | O(n) expected | O(n+k) | Yes | Uniformly distributed data |
 *  | Timsort | O(n log n) | O(n) | Yes | Real-world data, nearly sorted |
 *
 *  (* Quicksort has O(n²) worst case, but median-of-three pivot selection minimizes this risk)
 *
 *  ## Search Algorithms
 *
 *  | Algorithm | Time Complexity | Requirements |
 *  |-----------|-----------------|--------------|
 *  | Sequential Search | O(n) | None |
 *  | Binary Search | O(log n) | Sorted container |
 *  | Random Search | O(n log n) | Partially sorts during search |
 *  | Random Select | O(n) expected | None (finds k-th element) |
 *
 *  ## Usage Examples
 *
 *  @code{.cpp}
 *  #include <tpl_sort_utils.H>
 *  #include <tpl_dynArray.H>
 *
 *  // Sort a dynamic array
 *  DynArray<int> arr = {5, 2, 8, 1, 9};
 *  quicksort(arr);  // arr is now {1, 2, 5, 8, 9}
 *
 *  // Sort with custom comparator (descending order)
 *  quicksort(arr, [](int a, int b) { return a > b; });
 *
 *  // Sort a linked list
 *  DynList<int> list = {5, 2, 8, 1, 9};
 *  mergesort(list);  // O(1) space for lists!
 *
 *  // Binary search on sorted array
 *  long idx = binary_search(arr, 5);  // Returns index of 5
 *
 *  // Find k-th smallest element
 *  const int& third_smallest = random_select(arr.data(), 2, arr.size());
 *  @endcode
 *
 *  @see DynArray, DynList, DynDlist
 *  @author Leandro Rabindranath León
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_SORT_UTILS_H
#define TPL_SORT_UTILS_H

#include <ahUtils.H>
#include <ahFunctional.H>
#include <ah-concepts.H>
#include <ah-errors.H>
#include <tpl_arrayStack.H>
#include <tpl_array.H>
#include <tpl_dynArray.H>
#include <tpl_dynDlist.H>
#include <tpl_arrayHeap.H>
#include <htlist.H>
#include <limits>
#include <type_traits>
#include <vector>
#include <array>
#include <utility>
#include <cmath>
#include <algorithm>

namespace Aleph
{
  /** @brief Threshold for switching to insertion sort in hybrid algorithms.
   *
   *  When a partition becomes smaller than this threshold, hybrid sorting
   *  algorithms (like quicksort) switch to insertion sort, which is faster
   *  for small arrays due to lower overhead.
   *
   *  Default value: typically 10-20 elements.
   *
   *  @ingroup Algorithms
   */
  extern size_t Insertion_Threshold;

  /** @brief Threshold for using quicksort vs simpler algorithms.
   *
   *  Arrays smaller than this threshold may use simpler O(n²) algorithms.
   *
   *  @ingroup Algorithms
   */
  extern size_t Quicksort_Threshold;

  /** @brief Return value for search functions when element is not found.
   *
   *  Typically -1. Used by sequential_search(), random_search(), etc.
   *
   *  @ingroup Algorithms
   */
  extern const int Not_Found;

  /** @brief Check if a container is sorted in ascending order.
   *
   *  Tests whether all elements in the container are arranged in
   *  non-decreasing order according to the comparison functor.
   *
   *  Time complexity: O(n), where n is the container size.
   *
   *  @tparam Container Template container type (must provide Iterator).
   *  @tparam T Element type stored in the container.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param cont The container to check.
   *  @param cmp Comparison functor defining "less than" relationship.
   *
   *  @return `true` if container is empty or sorted; `false` if an
   *          inversion is found.
   *
   *  @see is_inversely_sorted(), search_inversion(), test_sorted()
   *  @ingroup Algorithms
   */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  [[nodiscard]] bool is_sorted(const Container<T> & cont, const Compare & cmp = Compare())
  {
    if (cont.is_empty())
      return true;

    typename Container<T>::Iterator it(cont);
    const T & first = it.get_curr();
    const T *prev = &first;
    it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr();
        if (cmp(curr, *prev))
          return false;
        prev = &curr;
      }
    return true;
  }

  /** @brief Find the first inversion in a container.
   *
   *  Searches for the first position where an element is less than its
   *  predecessor, indicating that the container is not sorted at that point.
   *
   *  Time complexity: O(n), where n is the container size.
   *
   *  @tparam Container Template container type.
   *  @tparam T Element type stored in the container.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param cont The container to check.
   *  @param cmp Comparison functor defining "less than" relationship.
   *
   *  @return A pair where:
   *          - `first` is `true` if the container is sorted, `false` otherwise.
   *          - `second` is the index of the inversion (if found), or the
   *            container size (if sorted).
   *
   *  @see is_sorted(), test_sorted()
   *  @ingroup Algorithms
   */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  [[nodiscard]] std::pair<bool, size_t> search_inversion(const Container<T> & cont,
                                                         const Compare & cmp = Compare())
  {
    if (cont.is_empty())
      return std::make_pair(true, 0);

    typename Container<T>::Iterator it(cont);
    const T & first = it.get_curr();
    const T *prev = &first;
    size_t i = 1;
    it.next_ne();
    for (; it.has_curr(); it.next_ne(), ++i)
      {
        const T & curr = it.get_curr();
        if (cmp(curr, *prev))
          return std::make_pair(false, i);
        prev = &curr;
      }

    return std::make_pair(true, i);
  }

  /** @brief Check if a container is sorted in descending order.
   *
   *  Tests whether all elements in the container are arranged in
   *  non-increasing order according to the comparison functor.
   *
   *  Time complexity: O(n), where n is the container size.
   *
   *  @tparam Container Template container type.
   *  @tparam T Element type stored in the container.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param cont The container to check.
   *  @param cmp Comparison functor defining "less than" relationship.
   *
   *  @return `true` if container is empty or inversely sorted; `false` otherwise.
   *
   *  @see is_sorted()
   *  @ingroup Algorithms
   */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  [[nodiscard]] bool is_inversely_sorted(const Container<T> & cont, const Compare & cmp = Compare())
  {
    if (cont.is_empty())
      return true;

    typename Container<T>::Iterator it(cont);
    const T & first = it.get_curr();
    const T *prev = &first;
    it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr();
        if (cmp(*prev, curr))
          return false;
        prev = &curr;
      }
    return true;
  }

  /** @brief Test if a container is sorted, returning the inversion position.
   *
   *  Similar to `is_sorted()` but returns additional information about
   *  where the ordering test failed or how many elements were checked.
   *
   *  Time complexity: O(n), where n is the container size.
   *
   *  @tparam Container Template container type.
   *  @tparam T Element type stored in the container.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param cont The container to test.
   *  @param cmp Comparison functor defining "less than" relationship.
   *
   *  @return A pair where:
   *          - `first` is `true` if sorted, `false` if an inversion exists.
   *          - `second` is the index where the test failed (if not sorted),
   *            or the total number of elements (if sorted).
   *
   *  @see is_sorted(), search_inversion()
   *  @ingroup Algorithms
   */
  template <template <typename> class Container, typename T,
            class Compare = Aleph::less<T>>
  [[nodiscard]] std::pair<bool, size_t>
  test_sorted(const Container<T> & cont, const Compare & cmp = Compare())
  {
    if (cont.is_empty())
      return {true, 0};

    typename Container<T>::Iterator it(cont);
    const T & first = it.get_curr();
    const T *prev = &first;
    size_t i = 1;
    it.next_ne();
    for (; it.has_curr(); it.next_ne(), ++i)
      {
        const T & curr = it.get_curr();
        if (cmp(curr, *prev))
          return {false, i};
        prev = &curr;
      }
    return {true, i};
  }


  /** @brief Sort an array using the selection sort algorithm.
   *
   *  Selection sort works by repeatedly finding the minimum element from
   *  the unsorted portion and placing it at the beginning. It has O(n²)
   *  time complexity but minimal memory overhead and a simple implementation.
   *
   *  ### Algorithm
   *  1. Find the minimum element in the unsorted portion [i, n)
   *  2. Swap it with the element at position i
   *  3. Repeat for i = 0, 1, ..., n-2
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n²) always (best, average, worst)
   *  - **Space complexity**: O(1) - in-place
   *  - **Stability**: Not stable (equal elements may be reordered)
   *  - **Comparisons**: Exactly n(n-1)/2
   *  - **Swaps**: At most n-1
   *
   *  ### When to Use
   *  - Very small arrays (n < 10-15)
   *  - When write operations are expensive (minimizes swaps)
   *  - When simplicity is more important than speed
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] n Number of elements in the array.
   *  @param cmp Comparison functor; `cmp(x, y)` returns true if x < y.
   *
   *  @see insertion_sort(), quicksort(), mergesort(), heapsort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void selection_sort(T *a, const size_t n, const Compare & cmp = Compare())
    noexcept(noexcept(cmp(a[0], a[0])) && std::is_nothrow_swappable_v<T>)
  {
    if (n < 2)
      return;

    for (size_t i = 0, min, j; i < n - 1; ++i)
      {
        for (min = i, j = i + 1; j < n; ++j)
          if (cmp(a[j], a[min]))
            min = j;

        if (cmp(a[min], a[i]))
          std::swap(a[min], a[i]);
      }
  }

  /** @brief Find the extreme (minimum or maximum) element in a linked list.
   *
   *  Performs a linear scan through the list to find the element that
   *  compares as "smallest" according to the provided comparator.
   *  By changing the comparator, you can find either minimum or maximum.
   *
   *  Time complexity: O(n), where n is the number of nodes in the list.
   *
   *  @tparam Link Link node type (e.g., Dlink, Slinknc).
   *  @tparam Compare Comparison functor that compares Link pointers.
   *
   *  @param list The list header to search.
   *  @param cmp Comparator: `cmp(p1, p2)` returns true if p1 is "more extreme".
   *
   *  @return Pointer to the node containing the extreme element.
   *
   *  @warning Does not check if the list is empty. Behavior is undefined
   *           for empty lists.
   *
   *  @see search_min(), search_max()
   *  @ingroup Algorithms
   */
  template <class Link, class Compare>
  inline
  Link * search_extreme(const Link & list, const Compare & cmp)
  {
    typename Link::Iterator it(const_cast<Link &>(list));
    Link *extreme = it.get_curr();

    for (it.next(); it.has_curr(); it.next_ne())
      {
        Link *curr = it.get_curr();
        if (cmp(curr, extreme))
          extreme = curr;
      }

    return extreme;
  }

  template <typename T, class Compare = Aleph::less<T>>
  inline
  const T * search_extreme(const DynList<T> & list, const Compare & cmp = Compare())
  {
    if (list.is_empty())
      return nullptr;

    typename DynList<T>::Iterator it(list);
    const T *extreme = &it.get_curr();
    it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr();
        if (cmp(curr, *extreme))
          extreme = &curr;
      }

    return extreme;
  }

  template <class Compare>
  inline
  Dlink * search_extreme(const Dlink & list, const Compare & cmp = Compare())
  {
    return search_extreme<Dlink, Compare>(list, cmp);
  }

  template <class Compare>
  inline
  Slinknc * search_extreme(const Slinknc & list, const Compare & cmp = Compare())
  {
    return search_extreme<Slinknc, Compare>(list, cmp);
  }

  /** @brief Selection sort for doubly-linked lists (`Dlink`).
   *
   *  Sorts the list by repeatedly finding the extreme element and appending
   *  it to a temporary auxiliary list.
   *
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <class Compare>
  inline
  void selection_sort(Dlink & list, Compare cmp)
  {
    Dlink aux;
    while (not list.is_empty())
      {
        Dlink *extreme = search_extreme<Dlink, Compare>(list, cmp);
        extreme->del(); // remove extreme from list
        aux.append(extreme); // insert it ordered into aux
      }

    list.swap(&aux);
  }

  /**
   * @brief Helper class to compare nodes of a linked list.
   * @details Adapts a comparison functor for elements of type @p T to 
   *          work with list nodes of type @p Tnode<T>.
   * @tparam Tlink Base link type (e.g., Dlink).
   * @tparam Tnode Template for the node type (e.g., Dnode).
   * @tparam T Element type.
   * @tparam Compare Comparison functor for @p T.
   */
  template <typename Tlink,
            template <class> class Tnode,
            typename T, class Compare>
  class Compare_Tnode
  {
    Compare cmp;

  public:
    /** @brief Construct from a comparison functor. */
    Compare_Tnode(Compare cmp_fct = Compare()) noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : cmp(cmp_fct)
    { /* empty */
    }

    /** @brief Compares two nodes based on their data. */
    bool operator ()(Tlink *l1, Tlink *l2) const
      noexcept(noexcept(std::declval<const Compare &>()(std::declval<const T &>(), std::declval<const T &>())))
    {
      auto *n1 = static_cast<Tnode<T> *>(l1);
      auto *n2 = static_cast<Tnode<T> *>(l2);

      assert(n1 == l1 and n2 == l2);

      return cmp(n1->get_data(), n2->get_data());
    }

    /** @brief Compares a node's data with a value. */
    bool operator ()(Tlink *l, const T & x) const
      noexcept(noexcept(std::declval<const Compare &>()(std::declval<const T &>(), std::declval<const T &>())))
    {
      auto *n = static_cast<Tnode<T> *>(l);

      assert(n == l);

      return cmp(n->get_data(), x);
    }
  };

  /**
   * @brief Comparator specialization for Dnode objects.
   */
  template <typename T, class Compare>
  struct Compare_Dnode : public Compare_Tnode<Dlink, Dnode, T, Compare>
  {
    Compare_Dnode(const Compare & cmp = Compare())
      noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : Compare_Tnode<Dlink, Dnode, T, Compare>(cmp)
    { /* empty */
    }
  };

  /**
   * @brief Comparator specialization for Snodenc objects.
   */
  template <typename T, class Compare>
  using Compare_Snodenc = Compare_Tnode<Slinknc, Snodenc, T, Compare>;


  /** @brief Selection sort for Dnode-based doubly-linked lists.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline void selection_sort(Dnode<T> & list, Compare cmp)
  {
    selection_sort<Compare_Dnode<T, Compare>>
        (list, Compare_Dnode<T, Compare>(cmp));
  }

  /** @brief Linear search for an element in an array.
   *
   *  Scans the array from left to right looking for the first occurrence
   *  of the specified element.
   *
   *  Time complexity: O(n), where n = r - l + 1
   *
   *  @tparam T Element type.
   *  @tparam Equal Equality comparison functor (default: `Aleph::equal_to<T>`).
   *
   *  @param a Pointer to the array.
   *  @param x The element to search for.
   *  @param l Left index (inclusive) where search begins.
   *  @param r Right index (inclusive) where search ends.
   *  @param eq Equality comparator; `eq(a[i], x)` returns true if equal.
   *
   *  @return Index of the first occurrence of x, or `Not_Found` (-1) if not found.
   *
   *  @see binary_search() for sorted arrays
   *  @ingroup Algorithms
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  [[nodiscard]] inline
  long sequential_search(T *a, const T & x, const long l, const long r,
                         Equal eq = Equal())
  {
    for (long i = l; i <= r; ++i)
      if (eq(a[i], x)) return i;

    return Not_Found;
  }

  /** @brief Linear search for an element in a dynamic array.
   *
   *  Scans the dynamic array between indices l and r looking for the first
   *  occurrence of the specified element.
   *
   *  @tparam T Element type.
   *  @tparam Equal Equality comparison functor (default: `Aleph::equal_to<T>`).
   *
   *  @param a Pointer to the dynamic array.
   *  @param x The element to search for.
   *  @param l Left index (inclusive).
   *  @param r Right index (inclusive).
   *  @param eq Equality comparator.
   *
   *  @return Index of the first occurrence of x, or `Not_Found` if not found.
   *  @note Only existing (mapped) entries in the DynArray are accessed.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  long sequential_search(const DynArray<T> & a, const T & x,
                         const long l, const long r, Equal eq = Equal())
  {
    for (long i = l; i <= r; ++i)
      if (a.exist(i))
        if (eq(a(i), x))
          return i;

    return Not_Found;
  }

  /** @brief Linear search for an element in a linked list.
   *
   *  @tparam Link Link node type (e.g., Dlink, Slinknc).
   *  @tparam T Element type.
   *  @tparam Equal Equality comparison functor.
   *
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *
   *  @return Pointer to the first node containing x, or nullptr if not found.
   */
  template <class Link, typename T, class Equal>
  Link * sequential_search(const Link & list, const T & x, Equal & eq)
  {
    for (typename Link::Iterator it(const_cast<Link &>(list));
         it.has_curr(); it.next_ne())
      if (Link *curr = it.get_curr(); eq(curr, x))
        return curr;

    return nullptr;
  }

  /** @brief Linear search for an element in a Dlink list.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the first Dlink containing x, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  Dlink * sequential_search(const Dlink & list, const T & x,
                            Equal eq = Equal())
  {
    Compare_Dnode<T, Equal> cmp(eq);
    return sequential_search<Dlink, T, Compare_Dnode<T, Equal>>(list, x, cmp);
  }

  /** @brief Linear search for an element in an Slinknc list.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the first Slinknc containing x, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  Slinknc * sequential_search(const Slinknc & list, const T & x, Equal eq = Equal())
  {
    Compare_Snodenc<T, Equal> cmp(eq);
    return sequential_search<Slinknc, T, Compare_Snodenc<T, Equal>>(list, x, cmp);
  }

  /** @brief Linear search over a list of Dnodes.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the first Dnode containing x, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  Dnode<T> * sequential_search(Dnode<T> & list, const T & x, Equal & eq)
  {
    Compare_Dnode<T, Equal> cmp(eq);
    Dlink *ret = sequential_search<Dlink, T, Compare_Dnode<T, Equal>>(list, x, cmp);

    return ret == nullptr ? nullptr : static_cast<Dnode<T> *>(ret);
  }

  /** @overload */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  Dnode<T> * sequential_search(Dnode<T> & list, const T & x, Equal && eq = Equal())
  {
    return sequential_search<T, Equal>(list, x, eq);
  }

  /** @brief Linear search over a list of Dnodes.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the first Dnode containing x, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  const Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, Equal & eq)
  {
    Compare_Dnode<T, Equal> cmp(eq);
    const auto & base = static_cast<const Dlink &>(list);
    const Dlink *ret = sequential_search<Dlink, T, Compare_Dnode<T, Equal>>(base, x, cmp);

    return ret == nullptr ? nullptr : static_cast<const Dnode<T> *>(ret);
  }

  /** @overload */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  const Dnode<T> * sequential_search(const Dnode<T> & list, const T & x, Equal && eq = Equal())
  {
    return sequential_search<T, Equal>(list, x, eq);
  }

  /** @brief Linear search on a DynDlist.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the data of the first matching element, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  T * sequential_search(DynDlist<T> & list, const T & x, Equal eq = Equal())
  {
    Dnode<T> *ret = sequential_search<T, Equal>(static_cast<Dnode<T> &>(list), x, eq);
    return ret != nullptr ? &ret->get_data() : nullptr;
  }

  /** @brief Linear search on a DynDlist (const version).
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the data of the first matching element, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  const T * sequential_search(const DynDlist<T> & list, const T & x, Equal eq = Equal())
  {
    const Dnode<T> *ret = sequential_search<T, Equal>(static_cast<const Dnode<T> &>(list), x, eq);
    return ret != nullptr ? &ret->get_data() : nullptr;
  }

  /** @brief Linear search on a DynList.
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the data of the first matching element, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  T * sequential_search(DynList<T> & list, const T & x, Equal eq = Equal())
  {
    for (typename DynList<T>::Iterator it(list); it.has_curr(); it.next_ne())
      {
        T & curr = it.get_curr_ne();
        if (eq(curr, x))
          return &curr;
      }
    return nullptr;
  }

  /** @brief Linear search on a DynList (const version).
   *  @param list The list header.
   *  @param x The element to search for.
   *  @param eq Equality comparator.
   *  @return Pointer to the data of the first matching element, or nullptr.
   */
  template <typename T, class Equal = Aleph::equal_to<T>>
  inline
  const T * sequential_search(const DynList<T> & list, const T & x, Equal eq = Equal())
  {
    for (typename DynList<T>::Iterator it(list); it.has_curr(); it.next_ne())
      {
        const T & curr = it.get_curr_ne();
        if (eq(curr, x))
          return &curr;
      }
    return nullptr;
  }


  /** Generic search in an array of an extreme element.

      search_extreme(a, l, r) searches the array sequentially
      a, between the indices l and r, the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] a the array on which to search.
      @param[in] l starting index of the search.
      @param[in] r search term index.
      @param cmp comparison object.
      @return containing index of the endpoint.
      @note There is no checking if the stream is empty.
      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long search_extreme(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    long extreme_index = l;
    for (long i = l + 1; i <= r; ++i)
      if (cmp(a[i], a[extreme_index])) // is there a new minimum?
        extreme_index = i; // yes

    return extreme_index;
  }

  /** Returns the smallest element of the array a between l and r.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long search_min(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(a, l, r, cmp);
  }

  /** Returns the maximum element of the array a between l and r.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::greater<T>>
  inline
  long search_max(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(a, l, r, cmp);
  }

  /** \overload search_extreme
      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  Dnode<T> * search_extreme(const Dnode<T> & list, const Compare & cmp = Compare())
  {
    Compare_Dnode<T, Compare> cmp_dnode(cmp);
    Dlink *ret =
        search_extreme<Dlink, Compare_Dnode<T, Compare>>
        (const_cast<Dlink &>(static_cast<const Dlink &>(list)), cmp_dnode);

    return static_cast<Dnode<T> *>(ret);
  }

  /** @brief Search for an extreme element in a DynDlist.
   *
   *  Performs a linear scan to find the minimum or maximum element according to
   *  the comparison criterion Compare.
   *
   *  @param[in] list the list to search on.
   *  @param cmp comparison object.
   *  @return pointer to the data of the extreme element, or nullptr if empty.
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  T * search_extreme(DynDlist<T> & list, const Compare & cmp = Compare())
  {
    const auto & base = static_cast<const Dnode<T> &>(list);
    Dnode<T> *ret = search_extreme<T, Compare>(const_cast<Dnode<T> &>(base), cmp);

    return ret != nullptr ? &(ret->get_data()) : nullptr;
  }

  /** @overload */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  const T * search_extreme(const DynDlist<T> & list, const Compare & cmp = Compare())
  {
    const auto & base = static_cast<const Dnode<T> &>(list);
    Dnode<T> *ret = search_extreme<T, Compare>(const_cast<Dnode<T> &>(base), cmp);

    return ret != nullptr ? &(ret->get_data()) : nullptr;
  }

  /** @brief Search for an extreme element in a DynList.
   *  @param list The list header.
   *  @param cmp Comparison functor.
   *  @return Pointer to the data of the extreme element, or nullptr if empty.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  T * search_extreme(DynList<T> & list, const Compare & cmp = Compare())
  {
    if (list.is_empty())
      return nullptr;

    typename DynList<T>::Iterator it(list);
    T *extreme = &it.get_curr_ne();
    it.next_ne();
    for (; it.has_curr(); it.next_ne())
      {
        T & curr = it.get_curr_ne();
        if (cmp(curr, *extreme))
          extreme = &curr;
      }

    return extreme;
  }

  /** @brief Search for the minimum element in a DynDlist.
   *  @param list The list header.
   *  @param cmp Comparison functor.
   *  @return Pointer to the data of the minimum element, or nullptr if empty.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  T * search_min(DynDlist<T> & list, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(list, cmp);
  }

  /** @overload */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  const T * search_min(const DynDlist<T> & list, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(list, cmp);
  }

  /** @brief Search for the maximum element in a DynDlist.
   *  @param list The list header.
   *  @param cmp Comparison functor.
   *  @return Pointer to the data of the maximum element, or nullptr if empty.
   */
  template <typename T, class Compare = Aleph::greater<T>>
  inline
  T * search_max(DynDlist<T> & list, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(list, cmp);
  }

  /** @overload */
  template <typename T, class Compare = Aleph::greater<T>>
  inline
  const T * search_max(const DynDlist<T> & list, const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(list, cmp);
  }

  /** @brief Sort an array using insertion sort.
   *
   *  Insertion sort builds the sorted array one element at a time by
   *  repeatedly taking the next element and inserting it into its correct
   *  position among the previously sorted elements.
   *
   *  ### Algorithm
   *  For each element from position l+1 to r:
   *  1. Store the element temporarily
   *  2. Shift larger elements one position to the right
   *  3. Insert the element in its correct position
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n²) worst/average, O(n) best (nearly sorted)
   *  - **Space complexity**: O(1) - in-place
   *  - **Stability**: Stable (equal elements maintain relative order)
   *  - **Adaptive**: Yes - runs faster on nearly sorted data
   *
   *  ### When to Use
   *  - Small arrays (n < 20-50)
   *  - Nearly sorted data (few inversions)
   *  - As a base case for recursive sorting algorithms
   *  - When stability is required with small data
   *
   *  @tparam T Element type (must be copy-assignable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] l Left index (inclusive) of the range to sort.
   *  @param[in] r Right index (inclusive) of the range to sort.
   *  @param cmp Comparison functor; `cmp(x, y)` returns true if x < y.
   *
   *  @see selection_sort(), quicksort(), mergesort(), heapsort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void insertion_sort(T *a, const long l, const long r,
                      const Compare & cmp = Compare())
    noexcept(noexcept(cmp(a[0], a[0])) and std::is_nothrow_move_assignable_v<T>)
  {
    if (l >= r)
      return;
    for (long i = l + 1, j; i <= r; ++i)
      {
        T tmp = a[i]; // store a[i], since it will be overwritten
        for (j = i; j > l and cmp(tmp, a[j - 1]); --j)
          a[j] = a[j - 1]; // shift to the right

        a[j] = tmp; // insert tmp into the gap
      }
  }

  /** Inserts a node orderly into a doubly linked list.

      insert_sorted(list,p) inserts into a list based on Dlink the
      node pointed to by p.

      The ordering criterion is given by the comparison class Compare.

      @param[in,out] list the list to which p will be inserted.
      @param[in] p the node to be inserted.
      @param cmp comparison object.
      @see Dlink
      \ingroup Algorithms
  */
  template <class Compare>
  inline
  void insert_sorted(Dlink & list, Dlink *p, const Compare & cmp)
  {
    Dlink::Iterator it(list);
    while (it.has_curr() and cmp(it.get_curr(), p))
      it.next_ne();

    if (it.has_curr())
      it.get_curr()->append(p); // insert before current
    else
      list.append(p);
  }

  template <class Compare>
  inline
  void insert_sorted(HTList & list, Slinknc *p, const Compare & cmp)
  {
    // Handle empty list case
    if (list.is_empty())
      {
        list.insert(p);
        return;
      }

    if (Slinknc *first = list.get_first(); cmp(p, first) or not cmp(first, p)) // p <= first?
      {
        list.insert(p);
        return;
      }

    if (Slinknc *last = list.get_last(); cmp(last, p) or not cmp(p, last)) // p >= last?
      {
        list.append(p);
        return;
      }

    Slinknc *prev = list.get_first();
    HTList::Iterator it(list);
    for (it.next(); it.has_curr(); it.next_ne())
      {
        Slinknc *curr = it.get_curr();
        if (cmp(p, curr)) // p < curr
          {
            prev->insert(p);
            return;
          }
        prev = curr;
      }
    ah_logic_error()
        << "insert_sorted(): reached end of list traversal without inserting";
  }

  /** @brief Generic insertion sort for linked lists.
   *
   *  @tparam ListType The list container type (e.g., DynList, DynDlist).
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <class ListType, class Compare>
  inline
  void list_insertion_sort(ListType & list, const Compare & cmp)
  {
    if (list.is_empty())
      return;

    ListType aux;
    aux.append(list.remove_first());
    while (not list.is_empty())
      insert_sorted<Compare>(aux, list.remove_first(), cmp);

    list.swap(aux);
  }

  /** @brief Insertion sort for DynList.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] l The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void insertion_sort(DynList<T> & l, const Compare & cmp = Compare())
  {
    using Cmp = Compare_Snodenc<T, Compare>;
    Cmp c(cmp);
    list_insertion_sort<HTList, Cmp>(l, c);
  }

  /** @brief Insertion sort for DynList (rvalue overload).
   *
   *  Moves the input list into the sorted result. Useful for sorting
   *  temporary lists efficiently.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] l The list to sort (rvalue).
   *  @param[in] cmp Comparison functor.
   *  @return The sorted list.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  DynList<T> insertion_sort(DynList<T> && l, const Compare & cmp = Compare())
  {
    using Cmp = Compare_Snodenc<T, Compare>;
    Cmp c(cmp);
    list_insertion_sort<HTList, Cmp>(l, c);
    return std::move(l);
  }

  /** @brief Insertion sort for Dnode-based lists.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline void insertion_sort(Dnode<T> & list, const Compare & cmp = Compare())
  {
    using Cmp = Compare_Dnode<T, Compare>;
    Cmp c(cmp);
    list_insertion_sort<Dlink, Cmp>(list, c);
  }

  /** @brief Merge two sorted partitions of an array using a buffer.
   *
   *  Merges the sorted subarrays [l..m] and [m+1..r] into a single sorted
   *  subarray [l..r]. Uses a provided buffer to avoid repeated allocations.
   *
   *  This is the core operation of merge sort. The technique copies elements
   *  to a buffer with the right half reversed, then merges from both ends
   *  toward the middle - eliminating the need for end-of-array checks.
   *
   *  Time complexity: O(n), where n = r - l + 1
   *  Space complexity: O(n) for the buffer
   *
   *  @tparam T Element type (must be move-assignable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array containing both partitions.
   *  @param[in] l Left index of the first partition.
   *  @param[in] m Last index of the first partition (m+1 is start of second).
   *  @param[in] r Right index of the second partition.
   *  @param[in,out] buf Buffer for temporary storage (cleared and reused).
   *  @param cmp Comparison functor.
   *
   *  @pre Both [l..m] and [m+1..r] must be sorted.
   *  @post [l..r] is sorted.
   *
   *  @see mergesort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void merge(T *a, const long l, const long m, const long r,
             std::vector<T> & buf, const Compare & cmp = Compare())
  {
    const long s = r - l + 1;

    // Resize buffer once instead of multiple push_back calls
    // This avoids repeated capacity checks and potential reallocations
    if (buf.size() < static_cast<size_t>(s))
      buf.resize(static_cast<size_t>(s));

    // Copy left partition [l..m] to buffer start
    long buf_idx = 0;
    for (long i = l; i <= m; ++i)
      buf[buf_idx++] = std::move(a[i]);

    // Copy right partition [m+1..r] in reverse to buffer end
    buf_idx = s - 1;
    for (long j = m + 1; j <= r; ++j)
      buf[buf_idx--] = std::move(a[j]);

    // Merge from both ends toward the middle
    long i = 0;
    long j = s - 1;

    for (long k = l; k <= r; ++k)
      if (cmp(buf[i], buf[j]))
        a[k] = std::move(buf[i++]);
      else
        a[k] = std::move(buf[j--]);
  }

  /** Merges two ordered partitions stored in an array.

      merge() assumes as input two partitions ordered between the
      ranges [l..m] and [m+1..r] and mixes them in an ordering between
      [l..r].

      @param[in] a the array
      @param[in] l left index of the array
      @param[in] m center of the array, of the partition
      @param[in] r right index of the array
      @param cmp comparison object.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void merge(T *a, const long l, const long m, const long r,
             const Compare & cmp = Compare())
  {
    std::vector<T> buf;
    buf.reserve(r - l + 1);
    merge(a, l, m, r, buf, cmp);
  }

  /** @brief Sort an array using merge sort with a reusable buffer.
   *  @param a Pointer to the array.
   *  @param l Left index (inclusive).
   *  @param r Right index (inclusive).
   *  @param buf Reusable buffer for merging.
   *  @param cmp Comparison functor.
   */
  template <typename T, class Compare>
  void mergesort(T *a, const long l, const long r, std::vector<T> & buf, Compare cmp)
  {
    if (l >= r)
      return;

    const long m = l + (r - l) / 2;

    mergesort<T, Compare>(a, l, m, buf, cmp);
    mergesort<T, Compare>(a, m + 1, r, buf, cmp);

    if (not cmp(a[m + 1], a[m]))
      return;

    merge<T, Compare>(a, l, m, r, buf, cmp);
  }

  /** @brief Sort an array using merge sort.
   *
   *  Merge sort is a divide-and-conquer algorithm that recursively divides
   *  the array in half, sorts each half, and merges the sorted halves.
   *
   *  ### Algorithm
   *  1. If l >= r, the subarray is trivially sorted
   *  2. Find the midpoint m = l + (r - l) / 2
   *  3. Recursively sort [l..m] and [m+1..r]
   *  4. Merge the two sorted halves
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) - always (best, average, worst)
   *  - **Space complexity**: O(n) for the auxiliary buffer
   *  - **Stability**: Stable (equal elements maintain relative order)
   *  - **Adaptive**: Semi-adaptive (skips merge if already sorted)
   *
   *  ### When to Use
   *  - When guaranteed O(n log n) is required
   *  - When stability is needed
   *  - External sorting (large data on disk)
   *  - Linked lists (where it achieves O(1) space!)
   *
   *  @tparam T Element type (must be move-assignable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] l Left index (inclusive) of the range to sort.
   *  @param[in] r Right index (inclusive) of the range to sort.
   *  @param cmp Comparison functor.
   *
   *  @see quicksort(), heapsort(), merge()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void mergesort(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    if (l >= r)
      return;

    std::vector<T> buf;
    buf.reserve(r - l + 1);
    mergesort(a, l, r, buf, cmp);
  }

  /** @brief Merge two sorted lists into a single sorted list.
   *
   *  Takes two sorted lists and merges them into a single sorted list.
   *  The input lists are emptied and their nodes are transferred to
   *  the result list. This is an O(n) operation with O(1) extra space.
   *
   *  Time complexity: O(n + m), where n and m are the list sizes
   *  Space complexity: O(1) - nodes are moved, not copied
   *
   *  @tparam Tlist List type (e.g., Dlink, HTList).
   *  @tparam Compare Comparison functor for list elements.
   *
   *  @param[in,out] l1 First sorted list (will be emptied).
   *  @param[in,out] l2 Second sorted list (will be emptied).
   *  @param[out] result Empty list to receive merged result.
   *  @param cmp Comparison functor.
   *
   *  @pre l1 and l2 must be sorted; result must be empty.
   *  @post l1 and l2 are empty; result contains all elements in sorted order.
   *
   *  @warning Does not verify that input lists are sorted.
   *
   *  @see mergesort()
   *  @ingroup Algorithms
   */
  template <typename Tlist, class Compare>
  inline
  void merge_lists(Tlist & l1, Tlist & l2, Tlist & result,
                   const Compare & cmp = Compare())
  {
    assert(result.is_empty());

    while (not l1.is_empty() and not l2.is_empty())
      if (cmp(l1.get_first_ne(), l2.get_first_ne()))
        result.append(l1.remove_first_ne());
      else
        result.append(l2.remove_first_ne());

    if (l1.is_empty())
      result.concat_list(l2);
    else
      result.concat_list(l1);

    assert(l1.is_empty() and l2.is_empty());
  }

  /** @brief Merge two sorted Dnode-based lists.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] l1 First sorted list.
   *  @param[in,out] l2 Second sorted list.
   *  @param[out] result Empty list to receive merged result.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void merge_lists(Dnode<T> & l1, Dnode<T> & l2, Dnode<T> & result,
                   const Compare & cmp = Compare())
  {
    merge_lists<Dnode<T>, Compare_Dnode<T, Compare>>(l1, l2, result, cmp);
  }

  /** @brief Sort a linked list using merge sort.
   *
   *  Merge sort is particularly efficient for linked lists because:
   *  - Splitting a list at the midpoint is O(n) (vs O(1) for arrays)
   *  - But merging is O(1) space (just pointer manipulation)
   *  - Overall, we get O(n log n) time with O(log n) stack space
   *
   *  ### Algorithm
   *  1. If list has 0 or 1 elements, it's sorted
   *  2. Split the list into two halves
   *  3. Recursively sort each half
   *  4. Merge the sorted halves
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) - always
   *  - **Space complexity**: O(log n) for recursion stack
   *  - **Stability**: Stable
   *  - **In-place**: Yes (for lists)
   *
   *  This is often the best sorting algorithm for linked lists.
   *
   *  @tparam Tlist List type (e.g., Dlink, HTList).
   *  @tparam Compare Comparison functor type.
   *
   *  @param[in,out] list The list to sort.
   *  @param cmp Comparison functor.
   *
   *  @see merge_lists(), quicksort()
   *  @ingroup Algorithms
   */
  template <typename Tlist, class Compare>
  inline
  void mergesort(Tlist & list, const Compare & cmp = Compare())
  {
    if (list.is_unitarian_or_empty())
      return;

    Tlist l, r;
    list.split_list_ne(l, r); // split into two lists

    mergesort<Tlist, Compare>(l, cmp);
    mergesort<Tlist, Compare>(r, cmp);

    merge_lists<Tlist, Compare>(l, r, list, cmp); // merge them
  }

  /** Sort a list by mergesort
      combined with the insert method.

      Sort a list using the mergesort method
      dynamic (DynList or DynDlist) according to comparison criteria Compare.

      This method has a deterministic performance of \f$O(n \; \lg n)\f$
      and space consumption of \f$O(\lg n)\f$ due to the recursive
      call stack. It is a very good method for lists.

      @param[in,out] list the list to be sorted.
      @param lsz size threshold to switch to insertion sort.
      @param cmp comparison object.

      @see mergesort(Dnode<T> & list)

      \ingroup Algorithms
  */
  template <template <typename> class Tlist, typename T,
            class Compare = Aleph::less<T>>
  inline
  void mergeinsertsort(Tlist<T> & list, const Compare & cmp = Compare(),
                       const size_t lsz = Aleph::Insertion_Threshold)
  {
    if (list.is_unitarian_or_empty())
      return;

    if (list.size() <= lsz)
      {
        insertion_sort<T, Compare>(list, cmp);
        return;
      }

    Tlist<T> l, r;
    list.split_list(l, r); // split into two lists

    mergeinsertsort<Tlist, T, Compare>(l, cmp, lsz);
    mergeinsertsort<Tlist, T, Compare>(r, cmp, lsz);

    merge_lists<Tlist<T>, Compare>(l, r, list, cmp); // merge them
  }

  /** @brief Merge sort for generic list templates.
   *
   *  @tparam Tlist List template type.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <template <typename> class Tlist, typename T,
            class Compare = Aleph::less<T>>
  inline
  void mergesort(Tlist<T> & list, const Compare & cmp = Compare())
  {
    if (list.is_unitarian_or_empty())
      return;

    Tlist<T> l, r;
    list.split_list(l, r); // split into two lists

    mergesort<Tlist, T, Compare>(l, cmp);
    mergesort<Tlist, T, Compare>(r, cmp);

    merge_lists<Tlist<T>, Compare>(l, r, list, cmp); // merge them
  }

  /** @brief Merge sort for Dnode-based lists.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void mergesort(Dnode<T> & list, const Compare & cmp = Compare())
  {
    mergesort<Dnode<T>, Compare_Dnode<T, Compare>>(list, cmp);
  }

  /** @brief Merge sort for DynDlist.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void mergesort(DynDlist<T> & list, const Compare & cmp = Compare())
  {
    mergesort<Dnode<T>, Compare_Dnode<T, Compare>>(list, cmp);
  }

  /** @brief Merge sort for DynList.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void mergesort(DynList<T> & list, const Compare & cmp = Compare())
  {
    mergesort<DynList<T>, Compare>(list, cmp);
  }


  /** @brief Merge sort for DynList (rvalue overload).
   *
   *  Moves the input list into the sorted result.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list The list to sort (rvalue).
   *  @param[in] cmp Comparison functor.
   *  @return The sorted list.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  DynList<T> mergesort(DynList<T> && list, const Compare & cmp = Compare())
  {
    mergesort<DynList<T>, Compare>(list, cmp);
    return std::move(list);
  }

  /** @brief Select a pivot for partitioning using median-of-three.
   *  @param a Pointer to the array.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the selected pivot.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long select_pivot(T *a, long l, long r, const Compare & cmp = Compare())
    noexcept(noexcept(cmp(a[0], a[0])) && std::is_nothrow_swappable_v<T>);


  /** @brief Partition an array range [l, r] around a pivot.
   *
   *  Rearranges elements such that all elements less than the pivot are
   *  on the left, and elements greater or equal are on the right.
   *
   *  Time complexity: O(n).
   *  Space complexity: O(1).
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *
   *  @param[in,out] a Pointer to the array.
   *  @param[in] l Left index.
   *  @param[in] r Right index.
   *  @param[in] cmp Comparison functor.
   *  @return The final index of the pivot.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long partition(T *a, const long l, const long r, const Compare & cmp = Compare())
    noexcept(noexcept(cmp(a[0], a[0])) && std::is_nothrow_swappable_v<T>)
  {
    if (l >= r)
      return l;

    const long p = select_pivot<T, Compare>(a, l, r, cmp);
    std::swap(a[p], a[r]);

    // Note: pivot stays at a[r] throughout the loop - no copy needed
    // This saves a potentially expensive copy for large T types
    long i = l - 1; // index first element to the left > pivot
    long j = r; // index first element to the right < pivot
    while (true)
      {
        // advance while a[i] < pivot (pivot is at a[r])
        while (cmp(a[++i], a[r])) {}

        while (cmp(a[r], a[--j])) // advance while pivot < a[j]
          if (j == l) // Has the left edge been reached?
            break; // yes ==> the iteration must be finished

        if (i >= j)
          break;

        // At this point there is an inversion a[i] > pivot > a[j]
        std::swap(a[i], a[j]); // Eliminate inversion
      }

    std::swap(a[i], a[r]); // Place pivot in its final position

    return i;
  }

  /** @brief Recursively sort an array using quicksort.
   *
   *  This method has an expected performance of \f$O(n \; \lg n)\f$
   *  and is considered the fastest ordering method.
   *
   *  This version of quicksort can take up \f$O(n)\f$ space.
   *  Use quicksort_rec_min() if you want to guarantee consumption
   *  maximum space of \f$O(\lg n)\f$.
   *
   *  @param a Pointer to the array to sort.
   *  @param l Left index (inclusive).
   *  @param r Right index (inclusive).
   *  @param cmp Comparison functor defining "less than" relationship.
   *
   *  @see selection_sort() insertion_sort() mergesort() heapsort()
   *  @see quicksort_rec_min() quicksort_insertion() quicksort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort_rec(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    if (l >= r)
      return;

    const long pivot = partition<T, Compare>(a, l, r, cmp);

    quicksort_rec<T, Compare>(a, l, pivot - 1, cmp);
    quicksort_rec<T, Compare>(a, pivot + 1, r, cmp);
  }

  /** @brief Quicksort implementation with tail-recursion optimization.
   *
   *  Recursively sorts the smaller partition and iterates on the larger one
   *  to guarantee O(log n) stack depth.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *
   *  @param[in,out] a Pointer to the array.
   *  @param[in] l Left index.
   *  @param[in] r Right index.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort_no_tail(T *a, long l, long r, const Compare & cmp = Compare())
  {
    while (l < r)
      {
        const long pivot = partition<T, Compare>(a, l, r, cmp);
        const long left_size = pivot - l;
        const long right_size = r - pivot;

        // Recurse on the smaller partition to ensure O(log n) stack depth.
        if (left_size < right_size)
          {
            quicksort_no_tail<T, Compare>(a, l, pivot - 1, cmp);
            l = pivot + 1; // tail recurse on the right by looping
          }
        else
          {
            quicksort_no_tail<T, Compare>(a, pivot + 1, r, cmp);
            r = pivot - 1; // tail recurse on the left by looping
          }
      }
  }

  /** Sorts an array according to the quicksort method with minimum
      space consumption.

      Purely recursive quicksort can consume proportional stack
      to the size of the arrangement to be ordered. To avoid this problem,
      at the expense of a slight execution cost, quicksort_rec_min()
      always sorts the smallest partition first. For others,
      The same considerations as for quicksort_rec() apply.

      @param[in,out] a the array to sort.
      @param[in] l initial index by which to sort.
      @param[in] r final index by which to sort.
      @param cmp comparison object.
      @see quicksort_rec() quicksort() quicksort_insertion()
      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort_rec_min(T *a, const long l, const long r,
                         const Compare & cmp = Compare())
  {
    if (r <= l)
      return;

    if (const long pivot = partition<T, Compare>(a, l, r, cmp); pivot - l < r - pivot)
    // which partition is smaller?
      { // left partition is smaller
        quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
        quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
      }
    else
      { // right partition is smaller
        quicksort_rec_min<T, Compare>(a, pivot + 1, r, cmp);
        quicksort_rec_min<T, Compare>(a, l, pivot - 1, cmp);
      }
  }

  template <typename T, class Compare>
  inline
  long select_pivot(T *a, const long l, const long r, const Compare & cmp)
    noexcept(noexcept(cmp(a[0], a[0])) && std::is_nothrow_swappable_v<T>)
  {
    const long m = l + (r - l) / 2;

    if (cmp(a[r], a[l]))
      std::swap(a[l], a[r]);
    if (cmp(a[m], a[l]))
      std::swap(a[m], a[l]);
    if (cmp(a[r], a[m]))
      std::swap(a[r], a[m]);

    return m;
  }

  /// Forward declaration
  inline size_t introsort_depth_limit(size_t n) noexcept;

  /** @brief Sort an array using iterative quicksort with optimizations.
   *
   *  This is the main quicksort implementation, featuring:
   *  - Iterative (stack-based) instead of recursive to control stack depth
   *  - Median-of-three pivot selection to avoid worst-case on sorted data
   *  - Falls back to insertion sort for small partitions (< Quicksort_Threshold)
   *  - Always processes smaller partition first for O(log n) stack space
   *
   *  ### Algorithm (Hoare partition scheme)
   *  1. Select pivot using median-of-three
   *  2. Partition: elements < pivot go left, >= pivot go right
   *  3. Push larger partition onto stack
   *  4. Process smaller partition immediately (tail recursion elimination)
   *  5. Repeat until stack is empty
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) expected, O(n²) worst case (rare)
   *  - **Space complexity**: O(log n) for the stack
   *  - **Stability**: Not stable
   *  - **In-place**: Yes
   *
   *  ### When to Use
   *  - General-purpose sorting (fastest in practice for most data)
   *  - When stability is not required
   *  - In-memory sorting of moderate to large arrays
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] l Left index (inclusive) of the range to sort.
   *  @param[in] r Right index (inclusive) of the range to sort.
   *  @param cmp Comparison functor.
   *
   *  @see quicksort_rec(), quicksort_insertion(), mergesort(), heapsort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    if (r - l < Quicksort_Threshold)
      {
        insertion_sort(a, l, r, cmp);
        return;
      }

    const auto n = static_cast<size_t>(r - l + 1);

    typedef std::pair<long, long> Partition;
    FixedStack<Partition> stack(introsort_depth_limit(n) + 2);
    stack.push(Partition(l, r));

    while (stack.size() > 0)
      {
        auto [fst, snd] = stack.pop();
        const long diff = snd - fst;

        if (diff <= 0)
          continue;

        if (diff < Quicksort_Threshold)
          {
            insertion_sort(a, fst, snd, cmp);
            continue;
          }

        if (const long pivot = partition<T, Compare>(a, fst, snd, cmp); pivot - fst < snd - pivot)
        // which one is smaller?
          { // the left partition is smaller
            stack.push(Partition(pivot + 1, snd));
            stack.push(Partition(fst, pivot - 1));
          }
        else
          { // the right partition is smaller
            stack.push(Partition(fst, pivot - 1));
            stack.push(Partition(pivot + 1, snd));
          }
      }
  }

  /** @brief Compute the depth limit for introsort.
   *
   *  Returns 2 * floor(log2(n)), which is the maximum recursion depth
   *  before introsort switches to heapsort.
   *
   *  @param n The number of elements.
   *  @return The depth limit.
   */
  inline size_t introsort_depth_limit(size_t n) noexcept
  {
    size_t depth = 0;
    while (n > 1)
      {
        n >>= 1;
        ++depth;
      }
    return depth * 2;
  }

  /** @brief Internal recursive function for introsort.
   *
   *  @internal
   */
  template <typename T, class Compare>
  void introsort_loop(T *a, long l, long r, size_t depth_limit,
                      const Compare & cmp)
  {
    while (r - l >= static_cast<long>(Quicksort_Threshold))
      {
        if (depth_limit == 0) [[unlikely]]
          {
            // Depth limit reached: switch to heapsort for guaranteed O(n log n)
            faster_heapsort(a + l, static_cast<size_t>(r - l + 1), cmp);
            return;
          }

        --depth_limit;

        // Median-of-three pivot selection
        // Recurse on smaller partition, iterate on larger (tail call elimination)
        if (const long pivot = partition<T, Compare>(a, l, r, cmp); pivot - l < r - pivot)
          {
            introsort_loop<T, Compare>(a, l, pivot - 1, depth_limit, cmp);
            l = pivot + 1;
          }
        else
          {
            introsort_loop<T, Compare>(a, pivot + 1, r, depth_limit, cmp);
            r = pivot - 1;
          }
      }
  }

  /** @brief Sort an array using introsort (introspective sort).
   *
   *  Introsort is a hybrid sorting algorithm that combines the best features
   *  of quicksort, heapsort, and insertion sort. It is used by std::sort
   *  in most C++ standard library implementations.
   *
   *  ### Algorithm
   *  1. Start with quicksort using median-of-three pivot selection
   *  2. Track recursion depth; if it exceeds 2*log2(n), switch to heapsort
   *  3. For small partitions (< Quicksort_Threshold), use insertion sort
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) - guaranteed (not just expected)
   *  - **Space complexity**: O(log n) for recursion stack
   *  - **Stability**: Not stable
   *  - **In-place**: Yes
   *
   *  ### Advantages over Pure Quicksort
   *  - Guaranteed O(n log n) worst case (quicksort has O(n²) worst case)
   *  - Immune to "quicksort killer" sequences
   *  - Same excellent average-case performance as quicksort
   *
   *  ### When to Use
   *  - Default choice for general-purpose sorting
   *  - When worst-case O(n log n) guarantee is required
   *  - Replacement for std::sort when working with raw arrays
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] l Left index (inclusive) of the range to sort.
   *  @param[in] r Right index (inclusive) of the range to sort.
   *  @param cmp Comparison functor.
   *
   *  @par Example
   *  @code
   *  int arr[] = {5, 2, 8, 1, 9, 3};
   *  introsort(arr, 0, 5);  // arr is now {1, 2, 3, 5, 8, 9}
   *
   *  // With custom comparator (descending order)
   *  introsort(arr, 0, 5, std::greater<int>());
   *  @endcode
   *
   *  @see quicksort(), heapsort(), mergesort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void introsort(T *a, const long l, const long r, const Compare & cmp = Compare())
  {
    if (r <= l)
      return;

    const auto n = static_cast<size_t>(r - l + 1);

    if (n < Quicksort_Threshold)
      {
        insertion_sort(a, l, r, cmp);
        return;
      }

    // Main introsort loop with depth limit
    introsort_loop<T, Compare>(a, l, r, introsort_depth_limit(n), cmp);

    // Final insertion sort pass for small unsorted subarrays
    insertion_sort(a, l, r, cmp);
  }

  /** @brief Sort an array using introsort (convenience overload).
   *
   *  This overload sorts the entire array [0, n-1].
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] n Number of elements in the array.
   *  @param cmp Comparison functor.
   *
   *  @see introsort(T*, long, long, Compare)
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void introsort(T *a, const size_t n, const Compare & cmp = Compare())
  {
    if (n <= 1)
      return;
    introsort(a, 0L, static_cast<long>(n - 1), cmp);
  }

  /** @brief Sort a range using introsort (STL-style iterator interface).
   *
   *  This overload provides an STL-compatible interface using begin/end
   *  pointers, similar to std::sort(begin, end).
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in] begin Pointer to the first element.
   *  @param[in] end Pointer to one past the last element.
   *  @param cmp Comparison functor.
   *
   *  @par Example
   *  @code
   *  int arr[] = {5, 2, 8, 1, 9, 3};
   *  introsort(arr, arr + 6);  // Sort entire array
   *
   *  // Sort only elements [1, 4)
   *  introsort(arr + 1, arr + 4);
   *
   *  // With custom comparator
   *  introsort(arr, arr + 6, std::greater<int>());
   *  @endcode
   *
   *  @see introsort(T*, long, long, Compare)
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void introsort(T *begin, T *end, const Compare & cmp = Compare())
  {
    if (begin >= end)
      return;
    const auto n = static_cast<size_t>(end - begin);
    introsort(begin, n, cmp);
  }

  /** @brief Introsort for Aleph Array.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] arr Array to sort.
   *  @param[in] cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void introsort(Array<T> & arr, const Compare & cmp = Compare())
  {
    const size_t n = arr.size();
    if (n <= 1)
      return;
    // Array uses contiguous memory via MemArray, so we can use pointer arithmetic
    introsort(&arr(0), n, cmp);
  }

  /** Sorts a linked list by the quicksort method.

      quicksort(list) sorts a list
      according to the quicksort method according to comparison criteria Compare.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      @param[in,out] list the list to sort.
      @param cmp Comparison functor defining "less than" relationship.

      \ingroup Algorithms
  */
  template <class Compare>
  void quicksort(Dlink & list, const Compare & cmp = Compare())
  {
    if (list.is_unitarian_or_empty())
      return;

    Dlink *pivot = list.remove_next();
    Dlink smaller, bigger; // lists of smaller and larger than pivot

    while (not list.is_empty())
      if (Dlink *p = list.remove_next(); cmp(p, pivot))
        smaller.append(p);
      else
        bigger.append(p);

    quicksort<Compare>(bigger, cmp);
    quicksort<Compare>(smaller, cmp);

    list.concat_list(&smaller); // restore sorted lists into list
    list.append(pivot);
    list.concat_list(&bigger);
  }

  /** Sorts a list by the quicksort method.

      quicksort(list) sorts a list using the method
      quicksort according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This primitive can be used for lists of type Dlist and DynDlist.

      @param[in,out] list the list to sort.
      @param cmp Comparison functor defining "less than" relationship.

      @see Dnode Dlist DynDlist

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  void quicksort(Dnode<T> & list, const Compare & cmp = Compare())
  {
    quicksort<Compare_Dnode<T, Compare>>(list, cmp);
  }

  /** @brief Sort an HTList using the quicksort method.
   *  @param list The list to sort.
   *  @param cmp Comparison functor for the data type @p T.
   */
  template <typename T, class Compare>
  void quicksort(HTList & list, const Compare & cmp = Compare())
  {
    if (list.is_unitarian_or_empty())
      return;

    auto *pivot = static_cast<Snodenc<T> *>(list.remove_first_ne());
    HTList smaller, bigger; // lists of smaller and larger than pivot

    while (not list.is_empty())
      if (auto *p = static_cast<Snodenc<T> *>(list.remove_first_ne()); cmp(p->get_data(), pivot->get_data()))
        smaller.append(p);
      else
        bigger.append(p);

    quicksort<T, Compare>(bigger, cmp);
    quicksort<T, Compare>(smaller, cmp);

    list.concat_list(smaller); // restore sorted lists into list
    list.append(pivot);
    list.concat_list(bigger);
  }

  /** @brief Sort a DynList using the quicksort method.
   *  @param list The list to sort.
   *  @param cmp Comparison functor for the data type @p T.
   */
  template <typename T, class Compare = Aleph::less<T>>
  void quicksort(DynList<T> & list, const Compare & cmp = Compare())
  {
    quicksort<T, Compare>(static_cast<HTList &>(list), cmp);
  }

  /** Sorts an array by the improved quicksort method.

      quicksort_insertion(a,l,r) sorts the array a among the
      indexes l and r according to the quicksort method according to criteria
      comparison Compare.

      The procedure combines various techniques to accelerate the
      ordering and at the same time avoid degradation due to bad cases.

      Bad cases are treated by pivot selection
      consisting of the median between l, r and the center of the partition.

      To ensure a maximum battery consumption of \f$O(\lg n)\f$,
      the method always proceeds recursively to sort the partition
      smaller.

      When the partition reaches a size less than or equal to the
      sim_id constant Aleph::Insertion_Threshold, then
      The partition is sorted by the insert method, which
      It is faster than quicksort for small sizes.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      @param[in,out] a the array to sort.
      @param[in] l lower index.
      @param[in] r top index.
      @param cmp comparison object.

      @see insertion_sort() mergesort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort()

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort_insertion(T *a, const long l, const long r,
                           const Compare & cmp = Compare())
  {
    if (r <= l)
      return;

    const long pivot = partition<T, Compare>(a, l, r, cmp);

    const long l_size = pivot - l; // left partition size
    const long r_size = r - pivot; // right partition size
    bool left_done = false; // true if left partition is sorted
    bool right_done = false; // true if der partition is sorted

    if (l_size <= Aleph::Insertion_Threshold)
      { // sort left partition by insertion
        insertion_sort<T, Compare>(a, l, pivot - 1, cmp);
        left_done = true;
      }

    if (r_size <= Aleph::Insertion_Threshold)
      { // sort der partition by insertion
        insertion_sort<T, Compare>(a, pivot + 1, r, cmp);
        right_done = true;
      }

    if (left_done and right_done)
      return; //both partitions sorted by insertion

    if (left_done) // left partition sorted by insertion?
      { // Yeah; It only remains to recursively sort the right partition
        quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
        return;
      }

    if (right_done) // der partition sorted by insertion?
      { // Yeah; It only remains to recursively sort the left partition
        quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
        return;
      }

    // here, both partitions were not insertion sorted
    if (l_size < r_size) // sort smallest partition first
      { //smaller left partition
        quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
        quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
      }
    else
      { // smaller right partition
        quicksort_insertion<T, Compare>(a, pivot + 1, r, cmp);
        quicksort_insertion<T, Compare>(a, l, pivot - 1, cmp);
      }
  }

  /** Random search for an element in an array.

      random_search(a,x,l,r) uses the partition algorithm
      quicksort to find element x in the array a
      between the limits l and r.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @warning The expected complexity is \f$O(n \; \lg n)\f$ for a single
      search, which is WORSE than sequential search \f$O(n)\f$.
      This function should only be used when you plan to perform
      multiple searches on the same array, as each search partially
      sorts the array. After several searches, the array becomes
      more ordered and subsequent searches become faster.

      For a single search, use sequential_search() instead.
      For searches on an already sorted array, use binary_search().

      @param[in,out] a  array to search for; is partially modified
      after the search.
      @param[in] x element to search for.
      @param[in] l lower index of the start of the search.
      @param[in] r top index of search term.
      @param cmp Comparison functor defining "less than" relationship.
      @return index of an array entry containing the
      value x if it is within the array;
      Not_Found (usually -1) otherwise.

      \ingroup Algorithms
      @see sequential_search() binary_search_rec()
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long random_search(T *a, const T & x, const long l, const long r,
                     const Compare & cmp = Compare())
  {
    if (l > r)
      return Not_Found;

    const long pivot = partition<T, Compare>(a, l, r, cmp);

    if (cmp(x, a[pivot]))
      return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
    if (cmp(a[pivot], x))
      return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

    return pivot; // element found at index x
  }

  /** @brief Random search for an element in a dynamic array.
   *  @param a The dynamic array to search.
   *  @param x The element to search for.
   *  @param l Left index (inclusive).
   *  @param r Right index (inclusive).
   *  @param cmp Comparison functor.
   *  @return Index of the element if found, Not_Found otherwise.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long random_search(DynArray<T> & a, const T & x, const long l, const long r,
                     const Compare & cmp = Compare())
  {
    if (l > r)
      return Not_Found;

    const long pivot = partition(a, l, r, cmp);

    if (cmp(x, a(pivot)))
      return random_search<T, Compare>(a, x, l, pivot - 1, cmp);
    if (cmp(a(pivot), x))
      return random_search<T, Compare>(a, x, pivot + 1, r, cmp);

    return pivot; // element found at index x
  }

  /** Random search for an element in a dlink list.

      dlink_random_search(list,x) uses the algorithm
      quicksort partition to find element x
      in the list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      @param[in,out] list list in which the search is performed;
      is partially modified after the search.
      @param[in] x element to search for.
      @param cmp comparison object.
      @return pointer to the node containing the value x if it is
      is found within the list; nullptr otherwise.

      @see random_search(Dlink&list,const T&x)

      \ingroup Algorithms
  */
  template <typename T, class Compare>
  inline
  Dnode<T> * dlink_random_search(Dlink & list, const T & x,
                                 const Compare & cmp = Compare())
  {
    if (list.is_empty())
      return nullptr;

    Dnode<T> item(x);
    Dnode<T> *item_ptr = &item; // pointer to the cell containing x

    Dlink smaller; // list of those smaller than pivot
    Dlink bigger; // list of those greater than pivot

    auto *pivot = static_cast<Dnode<T> *>(list.remove_next());

    while (not list.is_empty())
      if (Dlink *p = list.remove_next(); cmp(p, pivot))
        smaller.append(p);
      else
        bigger.append(p);

    Dnode<T> *ret_val = nullptr;
    if (cmp(item_ptr, pivot))
      ret_val = dlink_random_search<T, Compare>(smaller, x, cmp);
    else if (cmp(pivot, item_ptr))
      ret_val = dlink_random_search<T, Compare>(bigger, x, cmp);
    else
      ret_val = pivot;

    assert(list.is_empty());

    list.swap(&smaller);
    list.append(pivot);
    list.concat_list(&bigger);

    return ret_val;
  }

  /** Random search for an element in a Dnode<T> list.

      random_search(list,x) uses the partition algorithm
      quicksort to find element x in the list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      A specialization uses the "less than" relationship as
      comparison criterion and saves writing that
      class.

      @param[in,out] list list in which the search is performed;
      is partially modified after the search.
      @param[in] x element to search for.
      @param cmp Comparison functor defining "less than" relationship.
      @return pointer to the node containing the value x if it is
      found within the list; nullptr otherwise.

      @see dlink_random_search

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  Dnode<T> * random_search(Dlink & list, const T & x, const Compare & cmp = Compare())
  {
    return dlink_random_search<T, Compare_Dnode<T, Compare>>(list, x, cmp);
  }

  /** Random search for an element in a dynamic list.

      random_search(list,x) uses the partition algorithm
      quicksort to find element x in the dynamic list list.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      The procedure has an expected complexity of
      \f$O(n \; \lg n)\f$, which is greater than mere search
      sequential, but with the addition that after the
      search the list is partially ordered.

      A specialization uses the "less than" relationship as
      a comparison criterion and saves writing that
      class.

      @param[in,out] list dynamic list in which to perform
      the search; is partially modified after the search.
      @param[in] x element to search for.
      @param cmp comparison object.
      @return pointer x inside the list if it
      is found within it; nullptr otherwise.

      @see DynDlist

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  T * random_search(DynDlist<T> & list, const T & x, const Compare & cmp = Compare())
  {
    Dnode<T> *p = dlink_random_search<T, Compare_Dnode<T, Compare>>(list, x, cmp);

    return p == nullptr ? nullptr : &(p->get_data());
  }

  template <typename T, class Compare>
  static inline
  std::pair<long, long>
  partition_three_way(T *a, long l, long r, const Compare & cmp)
  {
    const long pivot_idx = select_pivot<T, Compare>(a, l, r, cmp);
    std::swap(a[pivot_idx], a[r]);

    long lt = l;
    long gt = r - 1;
    long current = l;

    while (current <= gt)
      if (cmp(a[current], a[r]))
        {
          std::swap(a[lt], a[current]);
          lt++;
          current++;
        }
      else if (cmp(a[r], a[current]))
        {
          std::swap(a[current], a[gt]);
          gt--;
        }
      else
        current++;

    std::swap(a[current], a[r]);
    return {lt, current};
  }

  template <class Container, typename T, class Compare>
  static inline
  std::pair<long, long>
  partition_three_way_op(Container & a, long l, long r, const Compare & cmp)
  {
    const long pivot_idx = select_pivot_op<T, Compare>(a, l, r, cmp);
    std::swap(a(pivot_idx), a(r));

    long lt = l;
    long gt = r - 1;
    long current = l;

    while (current <= gt)
      if (cmp(a(current), a(r)))
        {
          std::swap(a(lt), a(current));
          lt++;
          current++;
        }
      else if (cmp(a(r), a(current)))
        {
          std::swap(a(current), a(gt));
          gt--;
        }
      else
        current++;

    std::swap(a(current), a(r));
    return {lt, current};
  }

  template <typename T, class Compare>
  static inline
  const T &__random_select(T *a, const long i, long l, long r,
                           const Compare & cmp)
  {
    while (l < r)
      {
        if (r - l < 10)
          {
            insertion_sort<T, Compare>(a, l, r, cmp);
            return a[i];
          }

        const auto [lt, current] = partition_three_way<T, Compare>(a, l, r, cmp);

        if (i >= lt and i <= current)
          return a[i];

        if (i < lt)
          r = lt - 1;
        else
          l = current + 1;
      }

    return a[l];
  }

  /** @brief Implementation of pivot selection using operator().
   *  @tparam Container Container type.
   *  @param a The container.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the selected pivot.
   */
  template <typename T, class Compare, typename Container>
  inline
  long select_pivot_op_impl(const Container & a, const long l, const long r,
                            const Compare & cmp)
  {
    assert(l <= r);

    if (r - l <= 5)
      return r;

    const long m = l + (r - l) / 2; // central element

    // Access array entries only once
    const T & la = a(l);
    const T & ra = a(r);
    const T & ma = a(m);

    const T *med_ptr = Aleph::median<T, Compare>(la, ma, ra, cmp);

    if (med_ptr == &la)
      return l;

    if (med_ptr == &ma)
      return m;

    assert(med_ptr == &ra);

    return r;
  }

  /** Selects a pivot element as the median between
      the ends and the center.

      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long select_pivot_op(const DynArray<T> & a, const long l, const long r,
                       const Compare & cmp = Compare())
  {
    return select_pivot_op_impl<T, Compare>(a, l, r, cmp);
  }

  /** Selects a pivot element as the median between
      the ends and the center.

      @ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long select_pivot_op(const Array<T> & a, const long l, const long r,
                       const Compare & cmp = Compare())
  {
    return select_pivot_op_impl<T, Compare>(a, l, r, cmp);
  }

  /** @brief Implementation of partitioning using operator().
   *  @tparam Container Container type.
   *  @param a The container.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the pivot in its final position.
   */
  template <typename T, class Compare, typename Container>
  inline
  long partition_op_impl(Container & a, const long l, const long r,
                         const Compare & cmp)
  {
    if (l == r)
      return l;

    long i = l - 1;
    long j = r;

    // Move the selected pivot to position r
    if (const long pivot_idx = select_pivot_op<T, Compare>(a, l, r, cmp); pivot_idx != r)
      std::swap(a(pivot_idx), a(r));

    // Now pivot value is at a(r) - use direct access instead of reference
    while (true)
      {
        while (cmp(a(++i), a(r))) {}

        while (cmp(a(r), a(--j)))
          if (j == l)
            break;

        if (i >= j)
          break;

        std::swap(a(i), a(j));
      }

    std::swap(a(i), a(r));

    return i;
  }

  /** @brief Partition a DynArray using operator().
   *  @param a The dynamic array.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the pivot in its final position.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long partition_op(DynArray<T> & a, const long l, const long r,
                    const Compare & cmp = Compare())
  {
    return partition_op_impl<T, Compare>(a, l, r, cmp);
  }

  /** @brief Partition an Array using operator().
   *  @param a The array.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the pivot in its final position.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long partition_op(Array<T> & a, const long l, const long r,
                    const Compare & cmp = Compare())
  {
    return partition_op_impl<T, Compare>(a, l, r, cmp);
  }

  template <typename Container, class Compare>
  static inline
  void insertion_sort_range(Container & a, long l, long r, const Compare & cmp)
  {
    using T = typename Container::Item_Type;
    for (long p = l + 1; p <= r; ++p)
      {
        T key = std::move(a(p));
        long j = p - 1;
        while (j >= l and cmp(key, a(j)))
          {
            a(j + 1) = std::move(a(j));
            j--;
          }
        a(j + 1) = std::move(key);
      }
  }

  template <typename T, class Compare>
  static inline
  const T &__random_select(DynArray<T> & a, const long i,
                           long l, long r,
                           const Compare & cmp = Compare())
  {
    assert(i >= l and i <= r);

    while (l < r)
      {
        if (r - l < 10)
          {
            insertion_sort_range(a, l, r, cmp);
            return a(i);
          }

        const auto [lt, current] = partition_three_way_op<DynArray<T>, T, Compare>(a, l, r, cmp);

        if (i >= lt and i <= current)
          return a(i);

        if (i < lt)
          r = lt - 1;
        else
          l = current + 1;
      }

    return a(l);
  }

  template <typename T, class Compare>
  static inline
  const T &__random_select(Array<T> & a, const long i,
                           long l, long r,
                           const Compare & cmp = Compare())
  {
    assert(i >= l and i <= r);

    while (l < r)
      {
        if (r - l < 10)
          {
            insertion_sort_range(a, l, r, cmp);
            return a(i);
          }

        const auto [lt, current] = partition_three_way_op<Array<T>, T, Compare>(a, l, r, cmp);

        if (i >= lt and i <= current)
          return a(i);

        if (i < lt)
          r = lt - 1;
        else
          l = current + 1;
      }

    return a(l);
  }

  /** @brief Select the i-th smallest element in a DynArray.
   *  @param a The dynamic array.
   *  @param i Position to select.
   *  @param cmp Comparison functor.
   *  @return Reference to the selected element.
   *  @throw out_of_range if index is invalid.
   */
  template <typename T, class Compare = Aleph::less<T>>
  const T &random_select(DynArray<T> & a, const long i, const Compare & cmp = Compare())
  {
    const auto n_sz = a.size();
    ah_out_of_range_error_if(n_sz == 0 or i < 0) << "index out of range";

    const long n = static_cast<long>(n_sz) - 1;
    ah_out_of_range_error_if(i > n) << "index out of range";

    return __random_select<T, Compare>(a, i, 0, n, cmp);
  }

  /** @brief Select the i-th smallest element in an Array.
   *  @param a The array.
   *  @param i Position to select.
   *  @param cmp Comparison functor.
   *  @return Reference to the selected element.
   *  @throw out_of_range if index is invalid.
   */
  template <typename T, class Compare = Aleph::less<T>>
  const T &random_select(Array<T> & a, const long i, const Compare & cmp = Compare())
  {
    const auto n_sz = a.size();
    ah_out_of_range_error_if(n_sz == 0 or i < 0) << "index out of range";

    const long n = static_cast<long>(n_sz) - 1;
    ah_out_of_range_error_if(i > n) << "index out of range";

    return __random_select<T, Compare>(a, i, 0, n, cmp);
  }

  /** @brief Select the k-th smallest element in an array (Quickselect).
   *
   *  Finds the k-th smallest element without fully sorting the array.
   *  This is a selection algorithm based on quicksort's partition scheme.
   *
   *  ### Algorithm (Quickselect / Hoare's selection)
   *  1. Partition the array around a pivot
   *  2. If pivot is at position k, we're done
   *  3. If k < pivot position, recurse on left partition
   *  4. If k > pivot position, recurse on right partition
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n) expected, O(n²) worst case
   *  - **Space complexity**: O(log n) for recursion
   *  - **Side effect**: Partially sorts the array
   *
   *  ### Use Cases
   *  - Finding the median: `random_select(a, n/2, n)`
   *  - Finding k smallest/largest elements
   *  - Computing percentiles/quartiles
   *  - Order statistics
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Array to search (will be partially reordered).
   *  @param[in] i Position to select (0-indexed, so 0 = minimum).
   *  @param[in] n Number of elements in the array.
   *  @param cmp Comparison functor.
   *
   *  @return Reference to the i-th smallest element.
   *
   *  @throw out_of_range if i >= n.
   *
   *  @note After calling, elements at indices < i are not necessarily sorted,
   *        but are all <= the returned element.
   *
   *  @see partition()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  const T &random_select(T *a, const long i, const long n,
                         const Compare & cmp = Compare())
  {
    ah_invalid_argument_if(a == nullptr and n > 0) << "null pointer";

    ah_out_of_range_error_if(n <= 0 or i < 0 or i >= n) << "index out of range";

    return __random_select<T, Compare>(a, i, 0, n - 1, cmp);
  }

  /** @brief Select the k-th smallest element in an array (convenience overload).
   *  @param a Pointer to the array.
   *  @param i Position to select.
   *  @param n Number of elements.
   *  @param cmp Comparison functor.
   *  @return Reference to the selected element.
   */
  template <typename T, class Compare = Aleph::less<T>>
  const T &random_select(T *a, const long i, const long n,
                         Compare && cmp = Compare())
  {
    return random_select<T, Compare>(a, i, n, cmp);
  }

  /** Random selection of the ith element from a list based
       on Dlink.

      random_select(list,i) returns the ith smallest element
      content in the list according to order criteria
      determined by the Compare class.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      @param[in,out] list the list to select from (will be partially reordered).
      The list is left semi-ordered through successive
      partitions that have been made.
      @param[in] i position to be accessed (0-based).
      @param cmp Comparison functor defining "less than" relationship.
      @return pointer to Dlink corresponding to position i inside
      of the list;

      \ingroup Algorithms
  */
  template <class Compare>
  Dlink * dlink_random_select(Dlink & list, const size_t i,
                              const Compare & cmp = Compare())
  {
    if (list.is_empty())
      return nullptr;

    Dlink smaller; // list of minors than pivot
    Dlink bigger; // list of majors than pivot

    size_t smaller_count = 0, // number of elements in smaller
        bigger_count = 0; // number of elements in bigger

    Dlink *pivot = list.remove_next();

    while (not list.is_empty())
      if (Dlink *p = list.remove_next(); cmp(p, pivot)) // p < pivot?
        {
          smaller.append(p);
          ++smaller_count;
        }
      else
        {
          bigger.append(p);
          ++bigger_count;
        }

    if (i >= smaller_count + bigger_count + 1)
      {
        list.concat_list(&smaller);
        list.append(pivot);
        list.concat_list(&bigger);
        ah_out_of_range_error() << "index of selection greater than list's size";
      }

    Dlink *ret_val = nullptr;
    if (i == smaller_count)
      ret_val = pivot;
    else if (i < smaller_count)
      ret_val = dlink_random_select<Compare>(smaller, i, cmp);
    else
      ret_val = dlink_random_select<Compare>(bigger, i - (smaller_count + 1), cmp);

    list.concat_list(&smaller);
    list.append(pivot);
    list.concat_list(&bigger);

    return ret_val;
  }

  /** Random selection of the ith element from a list based
       on Dlink.

      random_select(list,i) returns the ith smallest element
      content in the list.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] list list where the ith element will be searched.
      The list is left semi-ordered through successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @param cmp comparison object.
      @return pointer to Dlink corresponding to position i inside
      of the list; nullptr if i is out of range.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  Dnode<T> * random_select(Dlink & list, const size_t i, const Compare & cmp = Compare())
  {
    return static_cast<Dnode<T> *>(dlink_random_select<Compare_Dnode<T, Compare>>(list, i, cmp));
  }

  /** Random selection of the ith element from a dynamic list.

      random_select(list,i) returns the ith smallest element
      content in the dynamic list.

      The routine uses the quicksort partition to
      search for position i in time \f$O(n \; \lg n)\f$, which
      It is a substantially better time than the search
      sequential (\f$O(n^2)\f$).

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] list dynamic list where the
      ith element. The list is semi-sorted through
      of the successive
      partitions that have been made.
      @param[in] i position to be accessed.
      @param cmp comparison object.
      @return pointer to the corresponding dynamic list element
      to position i within the list; nullptr if i is out of range.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  T * random_select(DynDlist<T> & list, const size_t i, const Compare & cmp = Compare())
  {
    Dlink *link = dlink_random_select<Compare_Dnode<T, Compare>>(list, i, cmp);

    auto *p = static_cast<Dnode<T> *>(link);

    return p != nullptr ? &(p->get_data()) : nullptr;
  }

  /** Sorts a dynamic array by the selection method.

      selection_sort(a) uses the selection method to sort
      the dynamic array of n elements.

      The selection method has a performance of \f$O(n^2)\f$.
      Due to its simplicity of implementation, its constant cost
      is low, so it is a good method for arrangements of
      very small dimension.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param cmp comparison object.
      @param[in,out] a the array to sort.
      @see insertion_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void selection_sort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    const long n = static_cast<long>(a.size());
    if (n < 2)
      return;

    for (long i = 0; i < n - 1; ++i)
      {
        long min = i;

        for (long j = i + 1; j < n; ++j)
          if (cmp(a(j), a(min)))
            min = j;

        if (cmp(a(min), a(i)))
          std::swap(a(min), a(i));
      }
  }

  /** @brief Sort a dynamic array using bubble sort.
   *
   *  Bubble sort repeatedly steps through the list, compares adjacent
   *  elements, and swaps them if they are in the wrong order. The pass
   *  through the list is repeated until the list is sorted.
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n²) always
   *  - **Space complexity**: O(1) - in-place
   *  - **Stability**: Stable
   *  - **Adaptive**: Can be made adaptive (not this implementation)
   *
   *  ### When to Use
   *  - Educational purposes (simple to understand)
   *  - Very small arrays where simplicity matters
   *  - Almost never in production code
   *
   *  @warning This is one of the slowest sorting algorithms. Use
   *           insertion_sort() for simple cases or quicksort() for
   *           better performance.
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param a The dynamic array to sort.
   *  @param cmp Comparison functor.
   *
   *  @see insertion_sort(), selection_sort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void bubble_sort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    const long n = static_cast<long>(a.size());
    if (n < 2)
      return;

    for (long i = 0; i < n - 1; ++i)
      for (long j = n - 1; j > i; --j)
        if (cmp(a(j), a(j - 1)))
          std::swap(a(j - 1), a(j));
  }

  /** Sorts a dynamic array by the insert method.

      insertion_sort(a) uses the selection method to sort
      the dynamic arrangement a.

      The insert method has a performance of \f$O(n^2)\f$.
      It is a simple method, so it consumes little constant time.
      On average, it performs half as many exchanges as the method
      of selection. Its time tends to be linear if the arrangement is
      semi-ordered. It is a good method for small arrangements and for
      partitions made by superior methods but with
      longer constant times.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      @param[in,out] a the array to sort.
      @param[in] l left index of the array art to be
      would you like to order
      @param[in] r right index of the desired part of the array
      order.
      @param cmp comparison object.
      @see selection_sort() quicksort_rec() mergesort() heapsort()
      \ingroup Algorithms
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  void insertion_sort(C<T> & a, const long l, const long r, const Compare & cmp = Compare())
  {
    for (long i = l + 1; i <= r; i++)
      {
        T tmp = a(i);
        long j = i;
        for (/* nothing */; j > l and cmp(tmp, a(j - 1)); --j)
          a(j) = a(j - 1);

        a(j) = tmp;
      }
  }

  /**
     \overload insertion_sort()
     \ingroup Algorithms
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  void insertion_sort(C<T> & a, const Compare & cmp = Compare())
  {
    const auto n = a.size();
    if (n <= 1)
      return;
    insertion_sort(a, 0, static_cast<long>(n) - 1, cmp);
  }

  /** @brief Sort a dynamic array using Shell sort.
   *
   *  Shell sort is an optimization of insertion sort that allows exchange
   *  of elements that are far apart. It works by sorting elements at various
   *  gap intervals, progressively reducing the gap until it becomes 1.
   *
   *  ### Algorithm
   *  Uses Ciura's gap sequence (empirically optimal): 1, 4, 10, 23, 57, 132, ...
   *  Extended with 2.25x multiplier for larger arrays.
   *  For each gap h (from largest to smallest):
   *    Perform h-sorting (insertion sort with stride h)
   *  When h=1, the array becomes fully sorted.
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n^1.3) empirical with Ciura sequence
   *  - **Space complexity**: O(1) - in-place
   *  - **Stability**: Not stable
   *  - **Adaptive**: Partially (benefits from presortedness)
   *
   *  ### When to Use
   *  - Medium-sized arrays (hundreds to thousands of elements)
   *  - When simplicity and low overhead matter
   *  - Embedded systems with limited stack space
   *  - When code size is a concern
   *
   *  @tparam T Element type (must be copy-assignable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a The dynamic array to sort.
   *  @param cmp Comparison functor.
   *
   *  @see insertion_sort(), quicksort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void shellsort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    const long n = a.size();
    if (n <= 1)
      return;

    // Ciura's gap sequence - empirically optimal for most data distributions
    // Extended with ~2.25x multiplier for larger arrays
    static constexpr long ciura_gaps[] =
        {
          1, 4, 10, 23, 57, 132, 301, 701, 1750, 4024, 9233,
          21223, 48805, 112217, 258100, 593630, 1365513, 3140680
        };
    static constexpr int num_gaps = sizeof(ciura_gaps) / sizeof(ciura_gaps[0]);

    // Find the first gap smaller than the array size
    int gap_idx = num_gaps - 1;
    while (gap_idx >= 0 && ciura_gaps[gap_idx] >= n)
      --gap_idx;

    // Sort with decreasing gaps
    for (; gap_idx >= 0; --gap_idx)
      {
        const long h = ciura_gaps[gap_idx];
        for (long i = h; i < n; i++)
          {
            T tmp = std::move(a(i));
            long j = i;

            while (j >= h and cmp(tmp, a(j - h)))
              {
                a(j) = std::move(a(j - h));
                j -= h;
              }

            a(j) = std::move(tmp);
          }
      }
  }


  /** @brief Convert a 1-based heap index to a 0-based array index. */
  inline static long back_index(const long i) noexcept { return i - 1; }

  /**
   * @brief Move an element up in a heap to restore heap property.
   * @param table The dynamic array containing the heap.
   * @param n The index of the element to sift up (1-based).
   * @param cmp Comparison functor.
   */
  template <typename T, class Compare>
  inline
  void sift_up(DynArray<T> & table, const long n, const Compare & cmp)
  {
    long p;
    for (long i = n; i > 1; i = p)
      {
        p = i >> 1; // c = i/2

        if (cmp(table(back_index(p)), table(back_index(i))))
          return;

        std::swap(table(back_index(p)), table(back_index(i)));
      }
  }

  /**
   * @brief Move an element down in a heap to restore heap property.
   * @param table The dynamic array containing the heap.
   * @param start The index of the element to sift down (1-based).
   * @param n The current size of the heap.
   * @param cmp Comparison functor.
   */
  template <typename T, class Compare>
  inline
  void sift_down(DynArray<T> & table, const long start, const long n, const Compare & cmp)
  {
    long i = start;

    while (true)
      {
        long c = i << 1; // c = 2*i (left child)

        if (c > n)
          return;

        // Select the smaller/larger child depending on comparator
        if (c + 1 <= n)
          if (cmp(table(back_index(c + 1)), table(back_index(c))))
            c++;

        if (cmp(table(back_index(i)), table(back_index(c))))
          return;

        std::swap(table(back_index(c)), table(back_index(i)));
        i = c;
      }
  }

  /** @overload */
  template <typename T, class Compare>
  inline
  void sift_down(DynArray<T> & table, const long n, const Compare & cmp)
  {
    sift_down<T, Compare>(table, 1, n, cmp);
  }

  /** @brief Comparator wrapper that inverts the comparison order.
   *
   *  Given a comparator `cmp`, this wrapper creates a comparator that
   *  returns `cmp(e2, e1)` instead of `cmp(e1, e2)`. This is useful for
   *  converting a min-heap algorithm to a max-heap algorithm.
   *
   *  @tparam T Element type being compared.
   *  @tparam Compare The underlying comparator type.
   *
   *  @see heapsort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare>
  class Negate_Compare
  {
    Compare cmp;

  public:
    /// Construct from a comparator.
    Negate_Compare(Compare __cmp = Compare())
      noexcept(std::is_nothrow_copy_assignable_v<Compare>)
      : cmp(__cmp)
    { /* Empty */
    }

    /// Compare in reverse order: returns cmp(e2, e1).
    bool operator ()(const T & e1, const T & e2) const
      noexcept(noexcept(std::declval<const Compare &>()(e2, e1)))
    {
      return cmp(e2, e1);
    }
  };

  /** @brief Sort a dynamic array using heapsort.
   *
   *  Heapsort uses the heap data structure to sort an array. It first
   *  builds a max-heap using Floyd's algorithm (O(n)), then repeatedly
   *  extracts the maximum element.
   *
   *  ### Algorithm
   *  1. Build a max-heap using Floyd's bottom-up construction (O(n))
   *  2. Repeatedly:
   *     a. Swap root (max element) with last unsorted element
   *     b. Reduce heap size by 1
   *     c. Sift down the new root to restore heap property
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) - always (best, average, worst)
   *  - **Space complexity**: O(1) - in-place
   *  - **Stability**: Not stable
   *  - **Adaptive**: No
   *
   *  ### When to Use
   *  - When guaranteed O(n log n) worst case is critical
   *  - When O(1) extra space is required
   *  - Real-time systems (predictable performance)
   *  - When data is expected to be adversarial (quicksort killers)
   *
   *  ### Comparison with Other O(n log n) Algorithms
   *  - Slower than quicksort in practice (poor cache locality)
   *  - Faster worst-case than quicksort
   *  - Uses less space than merge sort
   *  - Not stable like merge sort
   *
   *  @tparam T Element type (must be swappable and comparable).
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a The dynamic array to sort.
   *  @param cmp Comparison functor.
   *
   *  @see quicksort(), mergesort(), selection_sort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void heapsort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    const long n = a.size();
    if (n <= 1)
      return;

    auto nc = Negate_Compare<T, Compare>(cmp);

    // Floyd's heap construction: O(n) instead of O(n log n)
    // Start from the last non-leaf node and sift down each node
    for (long i = n / 2; i >= 1; --i)
      sift_down<T, Negate_Compare<T, Compare>>(a, i, n, nc);

    // Extract elements from the heap one by one
    for (long i = n; i >= 2; --i)
      {
        std::swap(a(0), a(i - 1));
        sift_down<T, Negate_Compare<T, Compare>>(a, 1, i - 1, nc);
      }
  }

  /** @brief Partition a container using the last element as pivot.
   *  @tparam C Container template (must support random access).
   *  @param a The container.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the pivot in its final position.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long partition(C<T> & a, const long l, const long r, const Compare & cmp = Compare())
  {
    if (l == r)
      return l;

    long i = l - 1;
    long j = r;
    const T & pivot = a(r);

    while (true)
      {
        while (cmp(a(++i), pivot)) { /* Nothing else */ }

        while (cmp(pivot, a(--j)))
          if (j == l)
            break;

        if (i >= j)
          break;

        std::swap(a(i), a(j));
      }

    std::swap(a(i), a(r));

    return i;
  }

  /** @brief Recursively sort a dynamic array using quicksort.
   *  @param a The dynamic array.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort_rec(DynArray<T> & a, const long l, const long r,
                     const Compare & cmp = Compare())
  {
    if (r <= l)
      return;

    if (const long i = partition(a, l, r, cmp); i - l < r - i)
      {
        quicksort_rec<T, Compare>(a, l, i - 1, cmp);
        quicksort_rec<T, Compare>(a, i + 1, r, cmp);
      }
    else
      {
        quicksort_rec<T, Compare>(a, i + 1, r, cmp);
        quicksort_rec<T, Compare>(a, l, i - 1, cmp);
      }
  }

  /** @brief Push two values onto a stack.
   *
   *  Convenience helper for iterative quicksort implementations.
   *
   *  @tparam Stack Stack container type.
   *  @tparam A First value type.
   *  @tparam B Second value type.
   *  @param stack The stack to push onto.
   *  @param a First value.
   *  @param b Second value.
   */
  template <class Stack, class A, class B>
  inline
  void push2(Stack & stack, const A & a, const B & b)
  {
    stack.push(b);
    stack.push(a);
  }

  /** Sorts a dynamic array by the quicksort method without recursion.

      quicksort(a) sorts the dynamic array according to the method
      quicksort according to comparison criteria Compare.

      The method uses two type parameters:
      -# T: the type of elements that the array has.
      -# Compare: comparison class.

      This method has an expected performance of \f$O(n \; \lg n)\f$
      and is considered the fastest ordering method.

      This version of quicksort takes up maximum space of \f$O(\lg n)\f$.

      Quicksort is a probabilistic method. In a very bad case
      -very bad luck- can be degraded to
      \f$O(n^2)\f$. To alleviate, to the extent of luck, the bad
      cases, use quicksort_insertion() which runs heuristics to
      alleviate the bad cases and invoke the insert method to
      small array partitions.

      @param[in,out] a the array to be ordered.
      @param cmp comparison object.
      @see selection_sort() insertion_sort() heapsort()
      @see quicksort_rec_min() quicksort_insertion() quicksort_rec()

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void quicksort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    long l = 0, r = a.size() - 1;

    const size_t n = a.size();
    FixedStack<long> stack((introsort_depth_limit(n) + 2) * 2);

    push2(stack, l, r);

    while (not stack.is_empty())
      {
        l = stack.pop();
        r = stack.pop();

        if (r <= l)
          continue;

        if (const long i = partition(a, l, r, cmp); i - l > r - i)
          {
            push2(stack, l, i - 1);
            push2(stack, i + 1, r);
          }
        else
          {
            push2(stack, i + 1, r);
            push2(stack, l, i - 1);
          }
      }
  }

  /** @brief Sift down operation for heapsort on a DynArray subrange.
   *
   *  @internal
   *  @param a The array.
   *  @param start Start index within the heap (1-based).
   *  @param n Heap size.
   *  @param offset Offset to the start of the subrange in the array.
   *  @param cmp Comparator.
   */
  template <typename T, class Compare>
  inline
  void sift_down_subrange(DynArray<T> & a, long start, const long n,
                          const long offset, const Compare & cmp)
  {
    while (true)
      {
        long c = start << 1; // c = 2*start (left child)
        if (c > n)
          return;

        // Select the child with higher priority
        if (c + 1 <= n)
          if (cmp(a(offset + c), a(offset + c - 1)))
            c++;

        if (cmp(a(offset + start - 1), a(offset + c - 1)))
          return;

        std::swap(a(offset + start - 1), a(offset + c - 1));
        start = c;
      }
  }

  /** @brief Heapsort implementation for a subrange of a DynArray.
   *
   *  Sorts the elements in the range [l, r] (inclusive) using the heapsort
   *  algorithm.
   *
   *  Time complexity: O(n log n).
   *  Space complexity: O(1).
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] a The dynamic array.
   *  @param[in] l Left index (inclusive).
   *  @param[in] r Right index (inclusive).
   *  @param[in] cmp Comparison functor.
   *  @internal
   */
  template <typename T, class Compare>
  inline
  void heapsort_subrange(DynArray<T> & a, const long l, const long r,
                         const Compare & cmp)
  {
    const long n = r - l + 1;
    if (n <= 1)
      return;

    auto nc = Negate_Compare<T, Compare>(cmp);

    // Floyd's heap construction: O(n)
    for (long i = n / 2; i >= 1; --i)
      sift_down_subrange<T, Negate_Compare<T, Compare>>(a, i, n, l, nc);

    // Extract elements
    for (long i = n; i >= 2; --i)
      {
        std::swap(a(l), a(l + i - 1));
        sift_down_subrange<T, Negate_Compare<T, Compare>>(a, 1, i - 1, l, nc);
      }
  }

  /** @brief Internal recursive function for introsort on DynArray.
   *
   *  @internal
   */
  template <typename T, class Compare>
  void introsort_loop(DynArray<T> & a, long l, long r, size_t depth_limit,
                      const Compare & cmp)
  {
    while (r - l >= static_cast<long>(Quicksort_Threshold))
      {
        if (depth_limit == 0) [[unlikely]]
          {
            // Depth limit reached: switch to heapsort for guaranteed O(n log n)
            heapsort_subrange<T, Compare>(a, l, r, cmp);
            return;
          }

        --depth_limit;

        // Partition and continue

        // Recurse on smaller partition, iterate on larger (tail call elimination)
        if (const long pivot = partition(a, l, r, cmp); pivot - l < r - pivot)
          {
            introsort_loop<T, Compare>(a, l, pivot - 1, depth_limit, cmp);
            l = pivot + 1;
          }
        else
          {
            introsort_loop<T, Compare>(a, pivot + 1, r, depth_limit, cmp);
            r = pivot - 1;
          }
      }
  }

  /** @brief Sort a dynamic array using introsort (introspective sort).
   *
   *  Introsort is a hybrid sorting algorithm that combines quicksort,
   *  heapsort, and insertion sort. It provides guaranteed O(n log n)
   *  worst-case performance while maintaining quicksort's excellent
   *  average-case performance.
   *
   *  This is the recommended general-purpose sorting algorithm.
   *
   *  ### Characteristics
   *  - **Time complexity**: O(n log n) - guaranteed
   *  - **Space complexity**: O(log n) for recursion
   *  - **Stability**: Not stable
   *  - **In-place**: Yes
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a The dynamic array to sort.
   *  @param cmp Comparison functor.
   *
   *  @par Example
   *  @code
   *  DynArray<int> arr = {5, 2, 8, 1, 9};
   *  introsort(arr);  // arr is now {1, 2, 5, 8, 9}
   *  @endcode
   *
   *  @see quicksort(), heapsort(), mergesort()
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  void introsort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    const long n = a.size();
    if (n <= 1)
      return;

    if (n < static_cast<long>(Quicksort_Threshold))
      {
        insertion_sort(a, 0, n - 1, cmp);
        return;
      }

    // Main introsort loop with depth limit
    introsort_loop<T, Compare>(a, 0, n - 1, introsort_depth_limit(n), cmp);

    // Final insertion sort pass for small unsorted subarrays
    insertion_sort(a, 0, n - 1, cmp);
  }

  /** Generic search in a dynamic array of an extreme element.

      search_extreme(a, l, r) searches the array sequentially
      a between the indices l and r, the extreme, minimum or element
      maximum according to the comparison criterion Compare.

      @param[in] a the dynamic array in which to perform the search.
      @param[in] l starting index of the search.
      @param[in] r search term index.
      @param cmp comparison object.
      @return containing index of the endpoint.
      @note There is no checking if the stream is empty.
      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long search_extreme(const DynArray<T> & a, const long l, const long r,
                      const Compare & cmp = Compare())
  {
    long extreme_index = l;

    for (long i = l + 1; i <= r; i++)
      if (cmp(a(i), a(extreme_index)))
        extreme_index = i;

    return extreme_index;
  }

  /** Returns the maximum element of the array a between l and r.

      \ingroup Algorithms
  */
  template <typename T, class Compare = Aleph::greater<T>>
  inline
  long search_max(const DynArray<T> & a, const long l, const long r,
                  const Compare & cmp = Compare())
  {
    return search_extreme<T, Compare>(a, l, r, cmp);
  }

  /** @brief Binary search on a sorted container.
   *
   *  Performs binary search to find an element or its insertion point
   *  in a sorted container. This is the fundamental search algorithm
   *  for sorted data, achieving O(log n) time complexity.
   *
   *  ### Algorithm
   *  1. Compare target with middle element
   *  2. If equal, return index
   *  3. If target < middle, search left half
   *  4. If target > middle, search right half
   *  5. Repeat until found or range exhausted
   *
   *  ### Semantics
   *  The return value has two meanings:
   *  - If element is found: index where it exists
   *  - If not found: index where it should be inserted to maintain order
   *
   *  To check if the element was actually found:
   *  @code
   *  long i = binary_search(a, x);
   *  if (i < a.size() && are_equals(a(i), x, cmp))
   *      // found at index i
   *  @endcode
   *
   *  Time complexity: O(log n)
   *  Space complexity: O(1)
   *
   *  @tparam C Container template (must support operator() and size()).
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param a Sorted container to search.
   *  @param x Element to search for.
   *  @param l Left index (inclusive) of the search range.
   *  @param r Right index (inclusive) of the search range.
   *  @param cmp Comparison functor.
   *
   *  @return Index where element is found, or where it should be inserted.
   *
   *  @pre Container must be sorted according to cmp.
   *
   *  @see bsearch(), sequential_search()
   *  @ingroup Algorithms
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long binary_search(const C<T> & a, const T & x, long l, long r,
                     const Compare & cmp = Compare())
  {
    if (l > r)
      return l;

    while (l <= r)
      if (long m = l + (r - l) / 2; cmp(x, a(m)))
        r = m - 1;
      else if (cmp(a(m), x))
        l = m + 1;
      else
        return m; // key found

    return l;
  }

  /** @brief Binary search on a sorted container of pointers.
   *
   *  Performs binary search to find an element or its insertion point
   *  in a sorted container that stores pointers to elements. 
   *  Comparisons are performed on the pointed-to values.
   *
   *  @tparam C Container template (must support operator() and size()).
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   *
   *  @param a Sorted container of pointers.
   *  @param x Element to search for.
   *  @param l Left index (inclusive).
   *  @param r Right index (inclusive).
   *  @param cmp Comparison functor.
   *  @return Index where element is found or should be inserted.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long binary_search(const C<T *> & a, const T & x, long l, long r,
                     const Compare & cmp = Compare())
  {
    if (l > r)
      return l;

    while (l <= r)
      if (long m = l + (r - l) / 2; cmp(x, *a(m)))
        r = m - 1;
      else if (cmp(*a(m), x))
        l = m + 1;
      else
        return m; // found key

    return l;
  }

  /** @brief Binary search on a sorted container of pointers (full range).
   *
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   *
   *  @param a Sorted container of pointers.
   *  @param x Element to search for.
   *  @param cmp Comparison functor.
   *  @return Index where element is found or should be inserted.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long binary_search(const C<T *> & a, const T & x, Compare && cmp = Compare())
  {
    const auto n = a.size();
    if (n == 0)
      return 0;

    return binary_search(a, x, 0, static_cast<long>(n) - 1, cmp);
  }

  /** Binary search on a dynamic ordered array.

      binary_search(a,x) searches for x
      in the array a between 0 and <code>size()</code>.

      The routine uses the binary search algorithm, which requires
      that the arrangement is orderly. This condition is not verified in
      the algorithm.

      The method always returns an integer between
      [0..size()). If the element is found, then the value of
      return is the index where this is located; otherwise,
      the index where x would be inserted is returned so that the array
      was ordered.

      @warning The routine does not fail if the array is empty, but
      be careful about accessing it in this situation

      @param[in] a the dynamic array on which to perform the search.
      @param[in] x the element to search for.
      @param cmp comparison object.
      @return index of the element where x is located or the index of
      the position where x would be inserted.
      @see sequential_search()
      \ingroup Algorithms
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long binary_search(const C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    const auto n = a.size();
    if (n == 0)
      return 0;

    return binary_search(a, x, 0, static_cast<long>(n) - 1, cmp);
  }

  /** @brief Binary search for all occurrences of a value.
   *
   *  Finds all indices in a sorted container that contain a value equal to @p x.
   *
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   *
   *  @param a Sorted container.
   *  @param x Element to search for.
   *  @param cmp Comparison functor.
   *  @return DynList containing all indices where @p x is found.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<size_t> binary_search_dup(const C<T> & a, const T & x,
                                    const Compare & cmp = Compare())
  {
    DynList<size_t> ret;
    const auto n = a.size();
    if (n == 0)
      return ret;

    long idx = binary_search(a, x, 0, static_cast<long>(n) - 1, cmp);
    if (idx < 0)
      return ret;

    if (not are_equals(a(idx), x, cmp))
      return ret;

    ret.append(idx);
    for (long i = idx - 1; i >= 0; --i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.insert(i);
      }

    for (long i = idx + 1; i < static_cast<long>(n); ++i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  /** @brief Build an index array of pointers for indirect sorting (const version).
   *  
   *  Creates a DynArray of constant pointers to the elements of container @p a,
   *  sorted according to @p cmp.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Container to index.
   *  @param cmp Comparison functor.
   *  @return DynArray of sorted pointers.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynArray<const T *> build_index_ptr(const C<T> & a, const Compare & cmp = Compare())
  {
    const size_t & n = a.size();
    DynArray<const T *> ret;
    ret.reserve(a.size());
    for (size_t i = 0; i < n; ++i)
      ret(i) = &a(i);

    quicksort_op(ret, [&cmp](const T *ptr1, const T *ptr2)
                   {
                     return cmp(*ptr1, *ptr2);
                   });

    return ret;
  }

  /** @brief Search for all occurrences of a value returning pointers (const version).
   * 
   *  Performs binary search on @p a and collects pointers to all elements
   *  equal to @p x.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return DynList of pointers to matching elements.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<const T *> bsearch_dup(const C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    DynList<const T *> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    const T *found_ptr = &a(idx);
    if (not are_equals(*found_ptr, x, cmp))
      return ret;

    for (long i = idx - 1; i >= 0; --i)
      {
        const T *ptr = &a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(ptr);
      }

    ret.append(found_ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        const T *ptr = &a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(ptr);
      }

    return ret;
  }

  /** @brief Search for a value in a sorted container returning a pointer.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return Pointer to the found element, or nullptr if not found.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  T * bsearch(C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    long i = binary_search(a, x, cmp);
    if (i < 0)
      return nullptr;
    T *ptr = &a(i);
    return are_equals(*ptr, x, cmp) ? ptr : nullptr;
  }

  /** @overload */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  const T * bsearch(const C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    long i = binary_search(a, x, cmp);
    if (i < 0)
      return nullptr;
    const T *ptr = &a(i);
    return are_equals(*ptr, x, cmp) ? ptr : nullptr;
  }

  /** @brief Search for a value in a sorted container of pointers.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container of pointers.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return The found pointer, or nullptr if not found.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  T * bsearch(const C<T *> & a, const T & x, const Compare & cmp = Compare())
  {
    long i = binary_search(a, x, cmp);
    if (i < 0)
      return nullptr;
    T *ptr = a(i);
    return are_equals(*ptr, x, cmp) ? ptr : nullptr;
  }

  /** @brief Search for all occurrences of a value returning pointers.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return DynList of pointers to matching elements.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<T *> bsearch_dup(C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    DynList<T *> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T *found_ptr = &a(idx);
    if (not are_equals(*found_ptr, x, cmp))
      return ret;

    for (long i = idx - 1; i >= 0; --i)
      {
        T *ptr = &a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(ptr);
      }

    ret.append(found_ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T *ptr = &a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(ptr);
      }

    return ret;
  }

  /** @brief Search for all occurrences of a value in a container of pointers.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container of pointers.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return DynList of found pointers.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<T *> bsearch_dup(const C<T *> & a, const T & x, const Compare & cmp = Compare())
  {
    DynList<T *> ret;
    const auto n = a.size();
    if (n == 0)
      return ret;

    long idx = binary_search(a, x, 0, static_cast<long>(n) - 1, cmp);
    if (idx < 0)
      return ret;

    T *found_ptr = a(idx);
    if (not are_equals(*found_ptr, x, cmp))
      return ret;

    for (long i = idx - 1; i >= 0; --i)
      {
        T *ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(ptr);
      }

    ret.append(found_ptr);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T *ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(ptr);
      }

    return ret;
  }

  /** Returns the index where a value appears (or should be inserted) in
      a sorted container.

      This function performs a binary search over the container `a` to
      locate the element `x`. The container is assumed to be ordered
      according to the comparison functor `cmp`.

      The search is delegated to `binary_search`, and therefore follows
      the same semantics:

      - If `x` is found, the returned value is the index at which it is
        located.
      - If `x` is not found, the returned value is the index at which
        `x` should be inserted to keep the container ordered.

      The function does not check that `a` is actually sorted.

      @tparam C       Template for the container type. It must support
                      random access via `operator()` and provide
                      `size()`.
      @tparam T       Type of the stored elements.
      @tparam Compare Comparison functor type; by default
                      `Aleph::less<T>`.

      @param[in] a  Sorted container in which the search is performed.
      @param[in] x  Value to search for.
      @param cmp    Comparison functor that defines the ordering.

      @return Index of the found element or the insertion position.

      @see binary_search()
      \ingroup Algorithms
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  long binindex(const C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    return binary_search(a, x, cmp);
  }

  /**
   * Returns the indices of all occurrences of a value in a sorted container.
   *
   * This function performs a binary search on the sorted container `a` to
   * locate the value `x`. If `x` is found, it then scans linearly to the
   * left and right of the first found position to collect the indices of
   * all elements equal to `x`.
   *
   * The container is assumed to be sorted according to the comparison functor
   * `cmp`. No verification of this precondition is performed.
   *
   * The returned list of indices is ordered from smallest to largest.
   *
   * @tparam C       Template for the container type. It must support
   *                 random access via `operator()` and provide `size()`.
   * @tparam T       Type of the stored elements.
   * @tparam Compare Comparison functor type; by default `Aleph::less<T>`.
   *
   * @param[in] a  Sorted container in which the search is performed.
   * @param[in] x  Value to search for.
   * @param cmp    Comparison functor that defines the ordering.
   *
   * @return A DynList<long> containing the indices of all elements equal to
   *         `x`. If `x` is not found, the returned list is empty.
   *
   * @see binary_search_dup()
   * @see binary_search()
   * \ingroup Algorithms
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<long> binindex_dup(const C<T> & a, const T & x, const Compare & cmp = Compare())
  {
    DynList<long> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    if (not are_equals(a(idx), x, cmp))
      return ret;

    const long mid = idx;

    for (long i = idx - 1; i >= 0; --i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.insert(i);
      }

    ret.append(mid);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        if (not are_equals(a(i), x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  /** @brief Search for all indices of a value in a sorted container of pointers.
   * 
   *  @tparam C Container template.
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type.
   * 
   *  @param a Sorted container of pointers.
   *  @param x Value to search for.
   *  @param cmp Comparison functor.
   *  @return DynList of indices where pointers point to values equal to @p x.
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynList<long> binindex_dup(const C<T *> & a, const T & x, const Compare & cmp = Compare())
  {
    DynList<long> ret;
    long idx = binary_search(a, x, cmp);
    if (idx < 0)
      return ret;

    T *ptr = a(idx);
    if (not are_equals(*ptr, x, cmp))
      return ret;

    for (long i = idx - 1; i >= 0; --i)
      {
        ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.insert(i);
      }

    ret.append(idx);

    for (long i = idx + 1, n = a.size(); i < n; ++i)
      {
        T *ptr = a(i);
        if (not are_equals(*ptr, x, cmp))
          break;
        ret.append(i);
      }

    return ret;
  }

  /** @brief Build an index array for indirect sorting.
   *
   *  Creates an array of indices that represents the sorted order of the
   *  container's elements without actually moving them. This is useful for:
   *  - Sorting large objects by a small key
   *  - Maintaining multiple sort orders simultaneously
   *  - Non-destructive sorting
   *
   *  ### Example
   *  @code
   *  DynArray<std::string> names = {"Charlie", "Alice", "Bob"};
   *  auto idx = build_index(names);
   *  // idx = {1, 2, 0}  (indices for "Alice", "Bob", "Charlie")
   *
   *  // Iterate in sorted order without modifying 'names':
   *  for (size_t i = 0; i < idx.size(); ++i)
   *      std::cout << names(idx(i)) << " ";  // Prints: Alice Bob Charlie
   *  @endcode
   *
   *  Time complexity: O(n log n) for the sort
   *  Space complexity: O(n) for the index array
   *
   *  @tparam C Container template type.
   *  @tparam T Element type in the container.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param a Container whose elements will be indexed.
   *  @param cmp Comparison functor for element ordering.
   *
   *  @return DynArray<size_t> of indices such that accessing a through
   *          these indices yields elements in sorted order.
   *
   *  @see build_index_ptr()
   *  @ingroup Algorithms
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynArray<size_t> build_index(const C<T> & a, const Compare & cmp = Compare())
  {
    const size_t & n = a.size();
    DynArray<size_t> ret;
    ret.reserve(a.size());
    for (size_t i = 0; i < n; ++i)
      ret(i) = i;

    quicksort_op(ret,
                 [&a, &cmp](size_t i, size_t j) { return cmp(a(i), a(j)); });

    return ret;
  }

  /** Builds and returns an array of pointers to the elements of container `a`
      sorted according to the comparison functor.

      This function does not move or copy the elements contained in `a`.
      Instead, it creates a `DynArray` of pointers to the elements in `a`
      and sorts those pointers using `cmp(*ptr1, *ptr2)` as the ordering
      criterion. The resulting array allows iteration over the elements of
      `a` in sorted order without modifying the original container.

      @tparam C       Template parameter of the container type.
                      The container must provide `size()` and `operator()`
                      for random access.
      @tparam T       Element type stored in the container.
      @tparam Compare Comparison functor type, defaulting to `Aleph::less<T>`.

      @param a   Constant reference to the container whose elements are to be
                 indirectly sorted.
      @param cmp Comparison functor that defines a strict weak ordering on
                 elements of type `T`.

      @return A `DynArray<T*>` where each entry is a pointer to an element of
              `a` such that `*ret(0), *ret(1), ..., *ret(n-1)` are in
              non-decreasing order according to `cmp`.
      \ingroup Algorithms
  */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  DynArray<T *> build_index_ptr(C<T> & a, const Compare & cmp = Compare())
  {
    const size_t & n = a.size();
    DynArray<T *> ret;
    ret.reserve(a.size());
    for (size_t i = 0; i < n; ++i)
      ret(i) = &a(i);

    quicksort_op(ret, [&cmp](const T *ptr1, const T *ptr2)
                   {
                     return cmp(*ptr1, *ptr2);
                   });

    return ret;
  }

  /** @brief Optimized quicksort for containers using operator().
   *
   *  This is the preferred quicksort implementation for Aleph containers
   *  like DynArray and Array. It uses the `operator()` access method
   *  instead of pointer arithmetic, allowing compatibility with various
   *  container types.
   *
   *  ### Optimizations
   *  - Iterative implementation (no recursion overhead)
   *  - Median-of-three pivot selection
   *  - Falls back to insertion sort for small partitions
   *  - Processes smaller partition first for O(log n) stack space
   *
   *  Time complexity: O(n log n) expected
   *  Space complexity: O(log n) for the stack
   *
   *  @tparam C Container template (must support operator() and size()).
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor type (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Container to sort.
   *  @param cmp Comparison functor.
   *  @param threshold Size below which insertion sort is used.
   *
   *  @see quicksort(), partition_op(), insertion_sort()
   *  @ingroup Algorithms
   */
  template <template <typename> class C, typename T,
            class Compare = Aleph::less<T>>
  inline
  void quicksort_op(C<T> & a, const Compare & cmp = Compare(),
                    const size_t threshold = Quicksort_Threshold)
  {
    const size_t n = a.size();
    if (n <= 1)
      return;

    size_t l = 0, r = n - 1;

    FixedStack<size_t> stack((introsort_depth_limit(n) + 2) * 2);

    push2(stack, l, r);

    while (not stack.is_empty())
      {
        l = stack.pop();
        r = stack.pop();

        const size_t partition_size = r - l + 1;
        if (partition_size <= 1)
          continue;

        if (partition_size <= threshold)
          {
            insertion_sort(a, l, r, cmp);
            continue;
          }

        const auto i =
            static_cast<size_t>(partition_op<T, Compare>(a,
                                                         static_cast<long>(l),
                                                         static_cast<long>(r),
                                                         cmp));

        const size_t left_size = i > l ? i - l : 0;
        const size_t right_size = r > i ? r - i : 0;

        if (left_size > right_size)
          {
            if (left_size > 0)
              push2(stack, l, i - 1);
            if (right_size > 0)
              push2(stack, i + 1, r);
          }
        else
          {
            if (right_size > 0)
              push2(stack, i + 1, r);
            if (left_size > 0)
              push2(stack, l, i - 1);
          }
      }
  }

  /** @brief Recursive binary search on an ordered array.
   *  @param a Pointer to the sorted array.
   *  @param x Value to search for.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the element if found, or its insertion position.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long binary_search_rec(T *a, const T & x, const long l, const long r,
                         const Compare & cmp = Compare())
  {
    if (l > r)
      return l;

    const long m = l + (r - l) / 2;

    if (cmp(x, a[m]))
      return binary_search_rec<T, Compare>(a, x, l, m - 1, cmp);
    if (cmp(a[m], x))
      return binary_search_rec<T, Compare>(a, x, m + 1, r, cmp);

    return m;
  }

  /** @brief Iterative binary search on an ordered array.
   *  @param a Pointer to the sorted array.
   *  @param x Value to search for.
   *  @param l Left index.
   *  @param r Right index.
   *  @param cmp Comparison functor.
   *  @return Index of the element if found, or its insertion position.
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline
  long binary_search(T *a, const T & x, long l, long r,
                     const Compare & cmp = Compare())
  {
    while (l <= r)
      {
        long m = l + (r - l) / 2;
        if (cmp(x, a[m]))
          r = m - 1;
        else if (cmp(a[m], x))
          l = m + 1;
        else
          return m;
      }
    return l;
  }


  /** @brief Stable counting sort on an index array by integer keys.
   *
   *  Reorders `sa[0..n-1]` so that the elements appear sorted by
   *  `key_of(sa[i])`.  Keys must lie in `[min_key, max_key]`.
   *
   *  @tparam KeyFn  Callable `(size_t) -> int` returning the sort key.
   *
   *  @param[in,out] sa      Index array to reorder.
   *  @param[out]    tmp     Scratch array (same size as `sa`).
   *  @param[in]     n       Number of elements to sort.
   *  @param[in]     min_key Minimum key value (inclusive).
   *  @param[in]     max_key Maximum key value (inclusive).
   *  @param[in]     key_of  Maps `sa[i]` to its integer sort key.
   *
   *  @throws std::domain_error if `max_key < min_key`.
   *  @throws std::out_of_range if `sa.size() < n` or `tmp.size() < n`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *  @throws std::out_of_range if a computed key is outside `[min_key, max_key]`.
   *
   *  @pre  `sa.size() >= n`, `tmp.size() >= n`.
   *  @pre  For all `i < n`: `min_key <= key_of(sa[i]) <= max_key`.
   *  @post `sa` is stably sorted by `key_of`.
   *
   *  @note O(n + k) time and O(k) extra space, where
   *        `k = max_key - min_key + 1`.
   */
  template <typename KeyFn>
    requires std::invocable<const KeyFn &, size_t> and
             std::convertible_to<std::invoke_result_t<const KeyFn &, size_t>, int>
  void counting_sort_indices(Array<size_t> & sa,
                             Array<size_t> & tmp,
                             const size_t n,
                             const int min_key,
                             const int max_key,
                             const KeyFn & key_of)
  {
    ah_domain_error_if(max_key < min_key)
      << "counting_sort_indices(): max_key < min_key";

    if (n == 0)
      return;

    ah_out_of_range_error_if(sa.size() < n or tmp.size() < n)
      << "counting_sort_indices(): sa/tmp size is smaller than n";

    const auto range_minus_one =
        static_cast<unsigned long long>(static_cast<long long>(max_key) -
                                        static_cast<long long>(min_key));
    constexpr auto max_count =
        static_cast<unsigned long long>(std::numeric_limits<size_t>::max() - size_t(1));
    ah_runtime_error_unless(range_minus_one <= max_count)
      << "counting_sort_indices(): key range is too large";

    const auto k = static_cast<size_t>(range_minus_one) + 1;
    Array<size_t> count = Array<size_t>::create(k);
    for (size_t i = 0; i < k; ++i)
      count[i] = 0;

    for (size_t i = 0; i < n; ++i)
      {
        const int key = key_of(sa[i]);
        ah_out_of_range_error_if(key < min_key or key > max_key)
          << "counting_sort_indices(): key out of expected range";
        const auto bucket =
            static_cast<size_t>(static_cast<unsigned long long>(
              static_cast<long long>(key) - static_cast<long long>(min_key)));
        ++count[bucket];
      }

    for (size_t j = 1; j < k; ++j)
      count[j] += count[j - 1];

    for (size_t i = n; i > 0; --i)
      {
        const int key = key_of(sa[i - 1]);
        const auto idx =
            static_cast<size_t>(static_cast<unsigned long long>(
              static_cast<long long>(key) - static_cast<long long>(min_key)));
        tmp[--count[idx]] = sa[i - 1];
      }

    for (size_t i = 0; i < n; ++i)
      sa[i] = tmp[i];
  }


  namespace sort_utils_detail
  {
    /** @brief Value concept for counting sort.
     *  @details Currently aliases IntegerSortableValue<T>, but may diverge
     *           if specific constraints are added for counting sort.
     */
    template <typename T>
    concept counting_integer = IntegerSortableValue<T>;


    /** @brief Value concept for radix sort.
     *  @details Currently aliases IntegerSortableValue<T>, but may diverge
     *           if specific constraints are added for radix sort.
     */
    template <typename T>
    concept radix_integer = IntegerSortableValue<T>;


    template <typename IntT>
    using counting_unsigned_t = std::make_unsigned_t<std::remove_cv_t<IntT>>;


    template <typename IntT>
    using radix_unsigned_t = std::make_unsigned_t<std::remove_cv_t<IntT>>;


    /** @brief Compute the number of buckets required for a counting sort range.
     *
     *  @tparam IntT Integer type.
     *  @param min_key Minimum value in the range.
     *  @param max_key Maximum value in the range.
     *  @return Total number of buckets needed.
     *  @throws ah_domain_error if max_key < min_key.
     *  @throws ah_runtime_error if the range exceeds size_t capacity.
     */
    template <typename IntT>
    [[nodiscard]] size_t
    counting_bucket_count(const IntT min_key, const IntT max_key)
    {
      ah_domain_error_if(max_key < min_key) << "counting_sort(): max_key < min_key";

      using U = counting_unsigned_t<IntT>;
      const U range_minus_one =
          static_cast<U>(max_key) - static_cast<U>(min_key);

      if constexpr (sizeof(U) >= sizeof(size_t))
        {
          constexpr size_t max_count =
              std::numeric_limits<size_t>::max() - static_cast<size_t>(1);
          ah_runtime_error_unless(range_minus_one <= static_cast<U>(max_count))
            << "counting_sort(): key range is too large";
        }

      return static_cast<size_t>(range_minus_one) + 1;
    }


    /** @brief Map a value to its bucket index in counting sort.
     *
     *  @tparam IntT Integer type.
     *  @param value Value to map.
     *  @param min_key Minimum key in the range.
     *  @return Bucket index in [0, bucket_count).
     */
    template <typename IntT>
    [[nodiscard]] inline size_t
    counting_bucket(const IntT value, const IntT min_key) noexcept
    {
      using U = counting_unsigned_t<IntT>;
      return static_cast<size_t>(static_cast<U>(value) - static_cast<U>(min_key));
    }


    /** @brief Retrieve the original value from a bucket index in counting sort.
     *
     *  @tparam IntT Integer type.
     *  @param bucket Bucket index.
     *  @param min_key Minimum key in the range.
     *  @return The original value corresponding to the bucket.
     */
    template <typename IntT>
    [[nodiscard]] inline IntT
    counting_value_from_bucket(const size_t bucket, const IntT min_key) noexcept
    {
      using U = counting_unsigned_t<IntT>;
      return static_cast<IntT>(static_cast<U>(min_key) + static_cast<U>(bucket));
    }


    /** @brief Internal implementation of counting sort for containers.
     *
     *  @tparam C Container template type.
     *  @tparam IntT Integer type.
     *  @param[in,out] a Container to sort.
     */
    template <template <typename> class C, typename IntT>
      requires counting_integer<IntT>
    void counting_sort_impl(C<IntT> & a)
    {
      const size_t n = a.size();
      if (n < 2)
        return;

      IntT min_key = a(0), max_key = a(0);
      for (size_t i = 1; i < n; ++i)
        {
          if (a(i) < min_key)
            min_key = a(i);
          if (a(i) > max_key)
            max_key = a(i);
        }

      const size_t k = counting_bucket_count(min_key, max_key);
      Array<size_t> count = Array<size_t>::create(k);
      for (size_t i = 0; i < k; ++i)
        count[i] = 0;

      for (size_t i = 0; i < n; ++i)
        ++count[counting_bucket(a(i), min_key)];

      for (size_t i = 1; i < k; ++i)
        count[i] += count[i - 1];

      Array<IntT> tmp = Array<IntT>::create(n);
      for (size_t i = n; i > 0; --i)
        {
          const auto bucket = counting_bucket(a(i - 1), min_key);
          tmp[--count[bucket]] = a(i - 1);
        }

      for (size_t i = 0; i < n; ++i)
        a(i) = tmp[i];
    }


    template <typename IntT>
      requires counting_integer<IntT>
    void counting_sort_impl(IntT *a, const size_t n)
    {
      if (n < 2)
        return;

      IntT min_key = a[0], max_key = a[0];
      for (size_t i = 1; i < n; ++i)
        {
          if (a[i] < min_key)
            min_key = a[i];
          if (a[i] > max_key)
            max_key = a[i];
        }

      const size_t k = counting_bucket_count(min_key, max_key);
      Array<size_t> count = Array<size_t>::create(k);
      for (size_t i = 0; i < k; ++i)
        count[i] = 0;

      for (size_t i = 0; i < n; ++i)
        ++count[counting_bucket(a[i], min_key)];

      for (size_t i = 1; i < k; ++i)
        count[i] += count[i - 1];

      Array<IntT> tmp = Array<IntT>::create(n);
      for (size_t i = n; i > 0; --i)
        {
          const auto bucket = counting_bucket(a[i - 1], min_key);
          tmp[--count[bucket]] = a[i - 1];
        }

      for (size_t i = 0; i < n; ++i)
        a[i] = tmp[i];
    }


    template <typename IntT>
      requires counting_integer<IntT>
    void counting_sort_impl(DynList<IntT> & list)
    {
      const size_t n = list.size();
      if (n < 2)
        return;

      typename DynList<IntT>::Iterator it(list);
      IntT min_key = it.get_curr_ne();
      IntT max_key = min_key;
      for (it.next_ne(); it.has_curr(); it.next_ne())
        {
          const IntT value = it.get_curr_ne();
          if (value < min_key)
            min_key = value;
          if (value > max_key)
            max_key = value;
        }

      const size_t k = counting_bucket_count(min_key, max_key);
      Array<size_t> count = Array<size_t>::create(k);
      for (size_t i = 0; i < k; ++i)
        count[i] = 0;

      for (typename DynList<IntT>::Iterator scan(list); scan.has_curr(); scan.next_ne())
        ++count[counting_bucket(scan.get_curr_ne(), min_key)];

      typename DynList<IntT>::Iterator out(list);
      for (size_t bucket = 0; bucket < k; ++bucket)
        {
          const size_t times = count[bucket];
          if (times == 0)
            continue;

          const IntT value = counting_value_from_bucket(bucket, min_key);
          for (size_t c = 0; c < times; ++c)
            {
              out.get_curr_ne() = value;
              out.next_ne();
            }
        }
    }


    template <typename IntT>
      requires counting_integer<IntT>
    void counting_sort_impl(DynDlist<IntT> & list)
    {
      const size_t n = list.size();
      if (n < 2)
        return;

      typename DynDlist<IntT>::Iterator it(list);
      IntT min_key = it.get_curr_ne();
      IntT max_key = min_key;
      for (it.next_ne(); it.has_curr(); it.next_ne())
        {
          const IntT value = it.get_curr_ne();
          if (value < min_key)
            min_key = value;
          if (value > max_key)
            max_key = value;
        }

      const size_t k = counting_bucket_count(min_key, max_key);
      Array<size_t> count = Array<size_t>::create(k);
      for (size_t i = 0; i < k; ++i)
        count[i] = 0;

      for (typename DynDlist<IntT>::Iterator scan(list); scan.has_curr(); scan.next_ne())
        ++count[counting_bucket(scan.get_curr_ne(), min_key)];

      typename DynDlist<IntT>::Iterator out(list);
      for (size_t bucket = 0; bucket < k; ++bucket)
        {
          const size_t times = count[bucket];
          if (times == 0)
            continue;

          const IntT value = counting_value_from_bucket(bucket, min_key);
          for (size_t c = 0; c < times; ++c)
            {
              out.get_curr_ne() = value;
              out.next_ne();
            }
        }
    }


    template <typename IntT>
    [[nodiscard]] constexpr radix_unsigned_t<IntT>
    radix_key(const IntT value) noexcept
    {
      using U = radix_unsigned_t<IntT>;
      if constexpr (std::is_signed_v<std::remove_cv_t<IntT>>)
        return static_cast<U>(value) ^
               (U{1} << (std::numeric_limits<U>::digits - 1));
      else
        return static_cast<U>(value);
    }


    template <template <typename> class C, typename IntT>
      requires radix_integer<IntT>
    void radix_sort_impl(C<IntT> & a)
    {
      using U = radix_unsigned_t<IntT>;

      const size_t n = a.size();
      if (n < 2)
        return;

      Array<IntT> tmp = Array<IntT>::create(n);
      Array<size_t> count = Array<size_t>::create(256);

      for (size_t pass = 0; pass < sizeof(U); ++pass)
        {
          for (size_t i = 0; i < 256; ++i)
            count[i] = 0;

          const size_t shift = pass * 8;
          for (size_t i = 0; i < n; ++i)
            {
              const U key = radix_key<IntT>(a(i));
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              ++count[bucket];
            }

          for (size_t i = 1; i < 256; ++i)
            count[i] += count[i - 1];

          for (size_t i = n; i > 0; --i)
            {
              const U key = radix_key<IntT>(a(i - 1));
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              tmp[--count[bucket]] = a(i - 1);
            }

          for (size_t i = 0; i < n; ++i)
            a(i) = tmp[i];
        }
    }


    /** @brief Core radix sort implementation for raw pointer ranges.
     *
     *  Uses counting sort as a stable subroutine to sort by each byte digit.
     *
     *  @tparam IntT Integer type.
     *  @param[in,out] a Pointer to the array.
     *  @param[in] n Number of elements.
     */
    template <typename IntT>
      requires radix_integer<IntT>
    void radix_sort_impl(IntT *a, const size_t n)
    {
      using U = radix_unsigned_t<IntT>;

      if (n < 2)
        return;

      Array<IntT> tmp = Array<IntT>::create(n);
      Array<size_t> count = Array<size_t>::create(256);

      for (size_t pass = 0; pass < sizeof(U); ++pass)
        {
          for (size_t i = 0; i < 256; ++i)
            count[i] = 0;

          const size_t shift = pass * 8;
          for (size_t i = 0; i < n; ++i)
            {
              const U key = radix_key<IntT>(a[i]);
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              ++count[bucket];
            }

          for (size_t i = 1; i < 256; ++i)
            count[i] += count[i - 1];

          for (size_t i = n; i > 0; --i)
            {
              const U key = radix_key<IntT>(a[i - 1]);
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              tmp[--count[bucket]] = a[i - 1];
            }

          for (size_t i = 0; i < n; ++i)
            a[i] = tmp[i];
        }
    }


    /** @brief LSD radix sort implementation for DynList.
     *
     *  Performs a stable radix sort on a linked list. This implementation
     *  is memory efficient as it reuses the original list nodes.
     *
     *  @tparam IntT Integer type.
     *  @param[in,out] list The list to sort.
     */
    template <typename IntT>
      requires radix_integer<IntT>
    void radix_sort_impl(DynList<IntT> & list)
    {
      using U = radix_unsigned_t<IntT>;

      if (const size_t n = list.size(); n < 2)
        return;

      static_assert(std::is_default_constructible_v<HTList>,
                    "HTList must be default constructible for std::array");
      std::array<HTList, 256> buckets;

      for (size_t pass = 0; pass < sizeof(U); ++pass)
        {
          const size_t shift = pass * 8;

          while (not list.is_empty())
            {
              Slinknc *link = list.HTList::remove_first_ne();
              auto *node = static_cast<Snodenc<IntT> *>(link);
              const U key = radix_key<IntT>(node->get_data());
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              buckets[bucket].append(link);
            }

          for (auto & bucket: buckets)
            list.HTList::append(bucket);
        }
    }


    /** @brief LSD radix sort implementation for DynDlist.
     *
     *  Performs a stable radix sort on a doubly-linked list.
     *
     *  @tparam IntT Integer type.
     *  @param[in,out] list The doubly-linked list to sort.
     */
    template <typename IntT>
      requires radix_integer<IntT>
    void radix_sort_impl(DynDlist<IntT> & list)
    {
      using U = radix_unsigned_t<IntT>;

      if (const size_t n = list.size(); n < 2)
        return;

      static_assert(std::is_default_constructible_v<Dnode<IntT>>,
                    "Dnode must be default constructible for std::array");
      std::array<Dnode<IntT>, 256> buckets;

      for (size_t pass = 0; pass < sizeof(U); ++pass)
        {
          const size_t shift = pass * 8;

          while (not list.template Dnode<IntT>::is_empty())
            {
              Dnode<IntT> *node = list.template Dnode<IntT>::remove_first_ne();
              const U key = radix_key<IntT>(node->get_data());
              const auto bucket = static_cast<size_t>((key >> shift) & U{0xFF});
              buckets[bucket].append(node);
            }

          for (auto & bucket: buckets)
            list.template Dnode<IntT>::append_list(&bucket);
        }
    }
  } // namespace sort_utils_detail


  /** @brief Value concept accepted by counting sort overloads.
   *  @tparam T Value type; must be an integral type (except bool) satisfying
   *            sort_utils_detail::counting_integer.
   */
  template <typename T>
  concept CountingSortable = sort_utils_detail::counting_integer<T>;


  /** @brief Value concept accepted by radix sort overloads.
   *  @tparam T Value type; must be an integral type (except bool) satisfying
   *            sort_utils_detail::radix_integer.
   */
  template <typename T>
  concept RadixSortable = sort_utils_detail::radix_integer<T>;


  /** @brief Stable counting sort for integral DynArray values.
   *
   *  Sorts in ascending order by counting frequencies of each distinct
   *  integer value in the observed key range `[min(a), max(a)]`.
   *
   *  @tparam T Value type.
   *  @param[in,out] a The DynArray to sort.
   *
   *  @throws std::domain_error if `max(a) < min(a)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *
   *  @par Complexity
   *  O(n + k) time and O(n + k) extra space, where
   *  `k = max(a) - min(a) + 1`.
   */
  template <typename T>
    requires CountingSortable<T>
  void counting_sort(DynArray<T> & a)
  {
    sort_utils_detail::counting_sort_impl(a);
  }


  /** @brief Stable counting sort for integral Array values.
   *
   *  Sorts in ascending order by counting frequencies of each distinct
   *  integer value in the observed key range `[min(a), max(a)]`.
   *
   *  @tparam T Value type.
   *  @param[in,out] a The Array to sort.
   *
   *  @throws std::domain_error if `max(a) < min(a)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *
   *  @par Complexity
   *  O(n + k) time and O(n + k) extra space, where
   *  `k = max(a) - min(a) + 1`.
   */
  template <typename T>
    requires CountingSortable<T>
  void counting_sort(Array<T> & a)
  {
    sort_utils_detail::counting_sort_impl(a);
  }


  /** @brief Stable counting sort for integral DynList values.
   *
   *  This overload performs an in-place value-replacement pass directly into
   *  existing list nodes. It preserves stability for plain integral types or
   *  when simple value equality semantics suffice. However, user-defined types
   *  wrapping keys may not observe true stability (preservation of original
   *  object identity and memory locations) because list nodes are not
   *  rearranged; only their data is updated.
   *
   *  @tparam T Value type (must satisfy CountingSortable).
   *  @param[in,out] list The DynList to sort.
   *
   *  @throws std::domain_error if `max(list) < min(list)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *
   *  @note Internally implemented by counting_sort_impl. If true
   *        node-reordering stability for wrapped types is required, use a
   *        stable algorithm that rearranges links, such as mergesort.
   */
  template <typename T>
    requires CountingSortable<T>
  void counting_sort(DynList<T> & list)
  {
    sort_utils_detail::counting_sort_impl(list);
  }


  /** @brief Stable counting sort for integral DynDlist values.
   *
   *  This overload performs an in-place value-replacement pass directly into
   *  existing list nodes. It preserves stability for plain integral types or
   *  when simple value equality semantics suffice. However, user-defined types
   *  wrapping keys may not observe true stability (preservation of original
   *  object identity and memory locations) because list nodes are not
   *  rearranged; only their data is updated.
   *
   *  @tparam T Value type (must satisfy CountingSortable).
   *  @param[in,out] list The DynDlist to sort.
   *
   *  @throws std::domain_error if `max(list) < min(list)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *
   *  @note Internally implemented by counting_sort_impl. If true
   *        node-reordering stability for wrapped types is required, use a
   *        stable algorithm that rearranges links, such as mergesort.
   */
  template <typename T>
    requires CountingSortable<T>
  void counting_sort(DynDlist<T> & list)
  {
    sort_utils_detail::counting_sort_impl(list);
  }


  /** @brief Stable counting sort for raw integral arrays.
   *
   *  @tparam T Value type.
   *  @param[in,out] a Pointer to the first element.
   *  @param[in] n Number of elements.
   *
   *  @throws std::domain_error if `max(a) < min(a)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   *  @throws std::invalid_argument if `a` is null and `n > 0`.
   *
   *  @par Complexity
   *  O(n + k) time and O(n + k) extra space, where
   *  `k = max(a) - min(a) + 1`.
   */
  template <typename T>
    requires CountingSortable<T>
  void counting_sort(T *a, const size_t n)
  {
    ah_invalid_argument_if(a == nullptr and n > 0)
      << "counting_sort(): null pointer with non-zero length";
    sort_utils_detail::counting_sort_impl(a, n);
  }


  /** @brief Stable counting sort for fixed-size integral C arrays.
   *
   *  @tparam T Value type.
   *  @tparam N Array size.
   *  @param[in,out] a The fixed-size array to sort.
   *
   *  @throws std::domain_error if `max(a) < min(a)`.
   *  @throws std::runtime_error if the key range is too large to fit in `size_t`.
   */
  template <typename T, size_t N>
    requires CountingSortable<T>
  void counting_sort(T (&a)[N])
  {
    counting_sort(a, N);
  }


  /** @brief LSD radix sort for integral DynArray values.
   *
   *  Sorts in ascending order. Signed integers are supported by applying a
   *  monotone bit transform on the sign bit.
   *
   *  @tparam T Value type.
   *  @param[in,out] a The DynArray to sort.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::runtime_error if an internal runtime error occurs.
   *
   *  @par Complexity
   *  O(w*n) time and O(n + 256) extra space, where `w = sizeof(T)`.
   */
  template <typename T>
    requires RadixSortable<T>
  void radix_sort(DynArray<T> & a)
  {
    sort_utils_detail::radix_sort_impl(a);
  }


  /** @brief LSD radix sort for integral Array values.
   *
   *  Sorts in ascending order. Signed integers are supported by applying a
   *  monotone bit transform on the sign bit.
   *
   *  @tparam T Value type.
   *  @param[in,out] a The Array to sort.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::runtime_error if an internal runtime error occurs.
   *
   *  @par Complexity
   *  O(w*n) time and O(n + 256) extra space, where `w = sizeof(T)`.
   */
  template <typename T>
    requires RadixSortable<T>
  void radix_sort(Array<T> & a)
  {
    sort_utils_detail::radix_sort_impl(a);
  }


  /** @brief LSD radix sort for integral DynList values.
   *
   *  This overload performs stable bucket distribution directly on list nodes.
   *
   *  @tparam T Value type.
   *  @param[in,out] list The DynList to sort.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::runtime_error if an internal runtime error occurs.
   */
  template <typename T>
    requires RadixSortable<T>
  void radix_sort(DynList<T> & list)
  {
    sort_utils_detail::radix_sort_impl(list);
  }


  /** @brief LSD radix sort for integral DynDlist values.
   *
   *  This overload performs stable bucket distribution directly on list nodes.
   *
   *  @tparam T Value type.
   *  @param[in,out] list The DynDlist to sort.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::runtime_error if an internal runtime error occurs.
   */
  template <typename T>
    requires RadixSortable<T>
  void radix_sort(DynDlist<T> & list)
  {
    sort_utils_detail::radix_sort_impl(list);
  }


  /** @brief LSD radix sort for raw integral arrays.
   *
   *  @tparam T Value type.
   *  @param[in,out] a Pointer to the first element.
   *  @param[in] n Number of elements.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::invalid_argument if `a` is null and `n > 0`.
   *
   *  @par Complexity
   *  O(w*n) time and O(n + 256) extra space, where `w = sizeof(T)`.
   */
  template <typename T>
    requires RadixSortable<T>
  void radix_sort(T *a, const size_t n)
  {
    ah_invalid_argument_if(a == nullptr and n > 0)
      << "radix_sort(): null pointer with non-zero length";
    sort_utils_detail::radix_sort_impl(a, n);
  }


  /** @brief LSD radix sort for fixed-size integral C arrays.
   *
   *  @tparam T Value type.
   *  @tparam N Array size.
   *  @param[in,out] a The fixed-size array to sort.
   *
   *  @throws std::domain_error if an internal domain error occurs.
   *  @throws std::runtime_error if an internal runtime error occurs.
   */
  template <typename T, size_t N>
    requires RadixSortable<T>
  void radix_sort(T (&a)[N])
  {
    radix_sort(a, N);
  }


  // ================================================================
  //                        Bucket Sort
  // ================================================================

  namespace bucket_sort_detail
  {
    /** @brief Insertion sort a sub-array [lo, hi).
     *  Used internally by bucket sort to sort individual buckets.
     */
    template <typename T, class Compare>
    void insertion_sort_range(T *a, const size_t lo, const size_t hi,
                              const Compare & cmp)
      noexcept(std::is_nothrow_move_constructible_v<T> and
               std::is_nothrow_move_assignable_v<T> and
               noexcept(std::declval<const Compare &>()(std::declval<const T &>(),
                                                        std::declval<const T &>())))
    {
      for (size_t i = lo + 1; i < hi; ++i)
        {
          T tmp = std::move(a[i]);
          size_t j = i;
          while (j > lo and cmp(tmp, a[j - 1]))
            {
              a[j] = std::move(a[j - 1]);
              --j;
            }
          a[j] = std::move(tmp);
        }
    }


    /** @brief Core bucket sort implementation on a raw pointer range.
     *
     *  Distributes elements into `num_buckets` buckets according to
     *  `bucket_key`, sorts each bucket with insertion sort, and
     *  writes the concatenated result back.
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     *  @tparam BucketKey Callable mapping T -> size_t bucket index.
     */
    template <typename T, class Compare, class BucketKey>
    void bucket_sort_impl(T *a, const size_t n, const size_t num_buckets,
                          const BucketKey & bucket_key, const Compare & cmp)
    {
      if (n < 2)
        return;

      ah_domain_error_if(num_buckets == 0)
        << "bucket_sort: num_buckets must be > 0";

      // Count elements per bucket
      Array<size_t> counts = Array<size_t>::create(num_buckets);
      for (size_t i = 0; i < num_buckets; ++i)
        counts[i] = 0;

      for (size_t i = 0; i < n; ++i)
        {
          const size_t b = bucket_key(a[i]);
          ah_domain_error_if(b >= num_buckets)
            << "bucket_sort: bucket_key returned " << b
            << " which is >= num_buckets (" << num_buckets << ")";
          ++counts[b];
        }

      // Compute bucket start offsets (prefix sum)
      Array<size_t> offsets = Array<size_t>::create(num_buckets);
      offsets[0] = 0;
      for (size_t i = 1; i < num_buckets; ++i)
        offsets[i] = offsets[i - 1] + counts[i - 1];

      // Distribute elements into a temporary array (stable)
      Array<T> tmp = Array<T>::create(n);
      // Reset counts for placement
      for (size_t i = 0; i < num_buckets; ++i)
        counts[i] = 0;

      for (size_t i = 0; i < n; ++i)
        {
          const size_t b = bucket_key(a[i]);
          tmp[offsets[b] + counts[b]] = std::move(a[i]);
          ++counts[b];
        }

      // Sort each bucket with insertion sort
      for (size_t b = 0; b < num_buckets; ++b)
        if (counts[b] > 1)
          insertion_sort_range(&tmp[0], offsets[b],
                               offsets[b] + counts[b], cmp);

      // Copy back
      for (size_t i = 0; i < n; ++i)
        a[i] = std::move(tmp[i]);
    }

    /** @brief Helper to sort any container by copying to a temporary array.
     *  
     *  Used by bucket_sort and timsort wrappers for non-contiguous containers.
     *
     *  @note This provides the basic exception-safety guarantee. If sort_fn or
     *        the comparator throws, the container `a` will remain in a valid
     *        but unspecified state because elements are moved to `tmp`.
     */
    template <typename T, class Container, class SortFn, class Compare>
    void apply_sort_with_temp(Container & a, SortFn sort_fn, const Compare & cmp)
    {
      const size_t n = a.size();
      if (n < 2)
        return;

      Array<T> tmp = Array<T>::create(n);
      size_t i = 0;
      for (auto it = a.get_it(); it.has_curr(); it.next_ne())
        tmp[i++] = std::move(it.get_curr_ne());

      sort_fn(&tmp[0], n, cmp);

      i = 0;
      for (auto it = a.get_it(); it.has_curr(); it.next_ne())
        it.get_curr_ne() = std::move(tmp[i++]);
    }
  } // namespace bucket_sort_detail


  /** @brief Bucket sort with user-supplied bucket mapping.
   *
   *  Distributes elements into `num_buckets` buckets using `bucket_key`,
   *  sorts each bucket with insertion sort, and concatenates.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @tparam BucketKey Callable: `bucket_key(element) -> size_t` in `[0, num_buckets)`.
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] n Number of elements.
   *  @param[in] num_buckets Number of buckets to distribute into.
   *  @param[in] bucket_key Mapping function from element to bucket index.
   *  @param[in] cmp Comparison functor.
   *
   *  @throws std::invalid_argument if `a` is null and `n > 0`.
   *  @throws std::domain_error if `bucket_key` returns an out-of-range index.
   *
   *  @par Complexity
   *  O(n + k) expected time (uniform distribution), O(n^2) worst case.
   *  O(n + k) extra space.
   *
   *  @par Stability
   *  Stable — equal elements preserve their original relative order.
   *
   *  @see counting_sort, radix_sort
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>, class BucketKey>
  void bucket_sort(T *a, const size_t n, const size_t num_buckets,
                   const BucketKey & bucket_key,
                   const Compare & cmp = Compare())
  {
    ah_invalid_argument_if(a == nullptr and n > 0)
      << "bucket_sort(): null pointer with non-zero length";
    bucket_sort_detail::bucket_sort_impl(a, n, num_buckets, bucket_key, cmp);
  }


  /** @brief Bucket sort convenience for floating-point arrays.
   *
   *  Automatically detects the ` [min, max] ` range and distributes it into `n` buckets
   *  using linear interpolation. Ideal for uniformly distributed data.
   *
   *  @tparam T Floating-point type (`float`, `double`, `long double`).
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] n Number of elements.
   *  @param[in] cmp Comparison functor.
   *
   *  @throws std::invalid_argument if `a` is null and `n > 0`.
   *
   *  @note If any element is non-finite (NaN or Infinity), the routine
   *        will skip bucket interpolation and fall back to timsort.
   *
   *  @par Complexity
   *  O(n) expected for uniformly distributed data, O(n^2) the worst case.
   *
   *  @par Stability
   *  Stable.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires std::floating_point<T>
  void bucket_sort(T *a, const size_t n, const Compare & cmp = Compare())
  {
    ah_invalid_argument_if(a == nullptr and n > 0)
      << "bucket_sort(): null pointer with non-zero length";

    if (n < 2)
      return;

    T min_val = a[0], max_val = a[0];
    for (size_t i = 0; i < n; ++i)
      {
        if (not std::isfinite(a[i])) [[unlikely]]
          {
            timsort(a, n, cmp);
            return;
          }
        if (a[i] < min_val) min_val = a[i];
        if (a[i] > max_val) max_val = a[i];
      }

    const T range = max_val - min_val;
    if (range == T{0})
      return; // all equal

    // Numeric interpolation is only correct for the default arithmetic
    // comparators (Aleph::less / Aleph::greater).  For any other comparator
    // the mapping from value to bucket index is undefined, so fall back to
    // the stable timsort which respects the caller's comparator directly.
    if constexpr (std::is_same_v<Compare, Aleph::less<T>> or
                  std::is_same_v<Compare, Aleph::greater<T>>)
      {
        const bool descending = cmp(max_val, min_val) and not cmp(min_val, max_val);
        const size_t num_buckets = n;
        auto key = [min_val, range, num_buckets, descending](const T & val) -> size_t
          {
            auto b = static_cast<size_t>((val - min_val) / range *
                                         static_cast<T>(num_buckets - 1));
            if (b >= num_buckets)
              b = num_buckets - 1;

            if (descending)
              b = (num_buckets - 1) - b;

            return b;
          };

        bucket_sort_detail::bucket_sort_impl(a, n, num_buckets, key, cmp);
      }
    else
      {
        // Custom comparator: bucket interpolation is not order-preserving;
        // delegate to timsort which respects any strict-weak-ordering comparator.
        timsort(a, n, cmp);
      }
  }


  /** @brief Bucket sort for DynArray (floating-point convenience).
   *
   *  @tparam T Floating-point type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] a DynArray to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @par Complexity
   *  O(n) expected for uniformly distributed data.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires std::floating_point<T>
  void bucket_sort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    bucket_sort_detail::apply_sort_with_temp<T>(a, [](T *p, size_t n, const Compare & c)
                                                  {
                                                    bucket_sort(p, n, c);
                                                  }, cmp);
  }


  /** @brief Bucket sort for Array (floating-point convenience).
   *
   *  @tparam T Floating-point type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] a Array to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires std::floating_point<T>
  void bucket_sort(Array<T> & a, const Compare & cmp = Compare())
  {
    if (a.size() < 2)
      return;
    bucket_sort(&a[0], a.size(), cmp);
  }


  /** @brief Bucket sort for fixed-size C arrays (floating-point convenience).
   *
   *  @tparam T Floating-point type.
   *  @tparam N Array size.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] a The array to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, size_t N, class Compare = Aleph::less<T>>
    requires std::floating_point<T>
  void bucket_sort(T (&a)[N], const Compare & cmp = Compare())
  {
    bucket_sort(static_cast<T *>(a), N, cmp);
  }


  /** @brief Bucket sort for DynList (floating-point convenience).
   *
   *  Copies elements to an Array, sorts with bucket sort, then writes
   *  sorted values back into the list nodes.
   *
   *  @tparam T Floating-point type.
   *  @tparam Compare Comparison functor.
   *  @param[in,out] list DynList to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires std::floating_point<T>
  void bucket_sort(DynList<T> & list, const Compare & cmp = Compare())
  {
    bucket_sort_detail::apply_sort_with_temp<T>(list, [](T *p, size_t n, const Compare & c)
                                                  {
                                                    bucket_sort(p, n, c);
                                                  }, cmp);
  }


  /** @brief Bucket sort for DynArray with user-supplied bucket mapping.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @tparam BucketKey Callable mapping T -> size_t.
   *  @param[in,out] a DynArray to sort.
   *  @param[in] num_buckets Number of buckets.
   *  @param[in] bucket_key Mapping function.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>, class BucketKey>
  void bucket_sort(DynArray<T> & a, const size_t num_buckets,
                   const BucketKey & bucket_key,
                   const Compare & cmp = Compare())
  {
    const size_t n = a.size();
    if (n < 2)
      return;

    Array<T> tmp = Array<T>::create(n);
    for (size_t i = 0; i < n; ++i)
      tmp[i] = a(i);

    bucket_sort(&tmp[0], n, num_buckets, bucket_key, cmp);

    for (size_t i = 0; i < n; ++i)
      a(i) = tmp[i];
  }


  /** @brief Bucket sort for Array with user-supplied bucket mapping.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor.
   *  @tparam BucketKey Callable mapping T -> size_t.
   *  @param[in,out] a Array to sort.
   *  @param[in] num_buckets Number of buckets.
   *  @param[in] bucket_key Mapping function.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>, class BucketKey>
  void bucket_sort(Array<T> & a, const size_t num_buckets,
                   const BucketKey & bucket_key,
                   const Compare & cmp = Compare())
  {
    if (a.size() < 2)
      return;
    bucket_sort(&a[0], a.size(), num_buckets, bucket_key, cmp);
  }


  // ================================================================
  //                          Timsort
  // ================================================================

  namespace timsort_detail
  {
    /** @brief Compute the minimum run length for timsort.
     *
     *  Returns a value in [32, 64] such that n/minrun is close to a power of 2.
     *  This is Python's algorithm: shift n right until it's in [32, 64),
     *  adding 1 if any bits were shifted out.
     */
    [[nodiscard]] constexpr size_t compute_minrun(size_t n) noexcept
    {
      size_t r = 0;
      while (n >= 64)
        {
          r |= n & 1;
          n >>= 1;
        }
      return n + r;
    }


    /** @brief Reverse elements in [lo, hi).
     *  @tparam T Element type.
     */
    template <typename T>
    void reverse_range(T *a, size_t lo, size_t hi)
      noexcept(std::is_nothrow_swappable_v<T>)
    {
      while (lo + 1 < hi)
        {
          std::swap(a[lo], a[hi - 1]);
          ++lo;
          --hi;
        }
    }


    /** @brief Find the length of the natural run starting at `lo`.
     *
     *  If the run is descending (strictly), it is reversed to become ascending.
     *  Returns the length of the run.
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     */
    template <typename T, class Compare>
    size_t count_run_and_make_ascending(T *a, const size_t lo,
                                        const size_t hi,
                                        const Compare & cmp)
    {
      if (hi - lo <= 1)
        return hi - lo;

      size_t run_hi = lo + 1;
      if (cmp(a[run_hi], a[lo])) // descending
        {
          while (run_hi < hi and cmp(a[run_hi], a[run_hi - 1]))
            ++run_hi;
          reverse_range(a, lo, run_hi);
        }
      else // ascending
        while (run_hi < hi and not cmp(a[run_hi], a[run_hi - 1]))
          ++run_hi;
      return run_hi - lo;
    }


    /** @brief Binary insertion sort on [lo, hi) starting from `start`.
     *
     *  Elements in [lo, start) are already sorted. This uses binary search
     *  to find the insertion point, then shifts elements.
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     */
    template <typename T, class Compare>
    void binary_insertion_sort(T *a, const size_t lo, const size_t hi,
                               const size_t start, const Compare & cmp)
      noexcept(std::is_nothrow_move_constructible_v<T> and
               std::is_nothrow_move_assignable_v<T> and
               noexcept(std::declval<const Compare &>()(std::declval<const T &>(),
                                                        std::declval<const T &>())))

    {
      size_t i = (start <= lo) ? lo + 1 : start;
      for (; i < hi; ++i)
        {
          T pivot = std::move(a[i]);

          // Binary search for insertion point in [lo, i)
          size_t left = lo;
          size_t right = i;
          while (left < right)
            {
              const size_t mid = left + (right - left) / 2;
              if (cmp(pivot, a[mid]))
                right = mid;
              else
                left = mid + 1;
            }

          // Shift elements [left, i) right by one
          for (size_t p = i; p > left; --p)
            a[p] = std::move(a[p - 1]);
          a[left] = std::move(pivot);
        }
    }


    /** @brief Gallop left: find the insertion point for a key in a sorted range.
     *
     *  Finds the leftmost position where `key` can be inserted while maintaining
     *  order. Uses a "galloping" strategy (exponential search followed by
     *  binary search) to find the position in O(log n) time, being faster
     *  than binary search when the key is near the hint.
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     *
     *  @param key The value to search for.
     *  @param a Pointer to the sorted array.
     *  @param len Length of the sorted range.
     *  @param hint Index to start the search from.
     *  @param cmp Comparison functor.
     *  @return Leftmost index where key could be inserted.
     */
    template <typename T, class Compare>
    size_t gallop_left(const T & key, const T *a, const size_t len,
                       const size_t hint, const Compare & cmp)
      noexcept(noexcept(std::declval<const Compare &>()(std::declval<const T &>(),
                                                        std::declval<const T &>())))
    {
      size_t last_ofs = 0;
      size_t ofs = 1;

      if (cmp(a[hint], key)) // key > a[hint]: search right
        {
          const size_t max_ofs = len - hint;
          while (ofs < max_ofs and cmp(a[hint + ofs], key))
            {
              last_ofs = ofs;
              ofs = (ofs << 1) + 1;
              if (ofs > max_ofs)
                ofs = max_ofs;
            }
          last_ofs += hint;
          ofs += hint;
        }
      else // key <= a[hint]: search left
        {
          const size_t max_ofs = hint + 1;
          while (ofs < max_ofs and not cmp(a[hint - ofs], key))
            {
              last_ofs = ofs;
              ofs = (ofs << 1) + 1;
              if (ofs > max_ofs)
                ofs = max_ofs;
            }
          const size_t tmp = last_ofs;
          // Note: last_ofs may intentionally underflow if hint < ofs.
          // This is compensated by the subsequent ++last_ofs to effectively
          // start the binary search at index 0. This is safe for unsigned types.
          last_ofs = hint - ofs;
          ofs = hint - tmp;
        }

      // Binary search in (last_ofs, ofs]
      ++last_ofs;
      while (last_ofs < ofs)
        {
          const size_t mid = last_ofs + (ofs - last_ofs) / 2;
          if (cmp(a[mid], key))
            last_ofs = mid + 1;
          else
            ofs = mid;
        }
      return ofs;
    }


    /** @brief Gallop right: find the insertion point for a key in a sorted range.
     *
     *  Finds the rightmost position where `key` can be inserted while maintaining
     *  order. Uses a "galloping" strategy (exponential search followed by
     *  binary search).
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     *
     *  @param key The value to search for.
     *  @param a Pointer to the sorted array.
     *  @param len Length of the sorted range.
     *  @param hint Index to start the search from.
     *  @param cmp Comparison functor.
     *  @return Rightmost index where key could be inserted.
     */
    template <typename T, class Compare>
    size_t gallop_right(const T & key, const T *a, const size_t len,
                        const size_t hint, const Compare & cmp)
      noexcept(noexcept(std::declval<const Compare &>()(std::declval<const T &>(),
                                                        std::declval<const T &>())))
    {
      size_t last_ofs = 0;
      size_t ofs = 1;

      if (cmp(key, a[hint])) // key < a[hint]: search left
        {
          const size_t max_ofs = hint + 1;
          while (ofs < max_ofs and cmp(key, a[hint - ofs]))
            {
              last_ofs = ofs;
              ofs = (ofs << 1) + 1;
              if (ofs > max_ofs)
                ofs = max_ofs;
            }
          const size_t tmp = last_ofs;
          // Note: last_ofs may intentionally underflow if hint < ofs.
          // This is compensated by the subsequent ++last_ofs to effectively
          // start the binary search at index 0. This is safe for unsigned types.
          last_ofs = hint - ofs;
          ofs = hint - tmp;
        }
      else // key >= a[hint]: search right
        {
          const size_t max_ofs = len - hint;
          while (ofs < max_ofs and not cmp(key, a[hint + ofs]))
            {
              last_ofs = ofs;
              ofs = (ofs << 1) + 1;
              if (ofs > max_ofs)
                ofs = max_ofs;
            }
          last_ofs += hint;
          ofs += hint;
        }

      ++last_ofs;
      while (last_ofs < ofs)
        {
          const size_t mid = last_ofs + (ofs - last_ofs) / 2;
          if (cmp(key, a[mid]))
            ofs = mid;
          else
            last_ofs = mid + 1;
        }
      return ofs;
    }


    /** @brief Merge two adjacent sorted runs a[base1..base1+len1) and
     *  a[base2..base2+len2) using temporary buffer. Assumes
     *  base2 == base1 + len1 and len1 <= len2.
     *
     *  Uses galloping to skip long monotone sequences.
     */
    template <typename T, class Compare>
    void merge_lo(T *a, const size_t base1, const size_t len1,
                  const size_t base2, const size_t len2,
                  T *tmp, const Compare & cmp)
    {
      // Copy run1 into tmp
      for (size_t i = 0; i < len1; ++i)
        tmp[i] = std::move(a[base1 + i]);

      size_t c1 = 0; // cursor into tmp (run1 copy)
      size_t c2 = base2; // cursor into a (run2)
      size_t dest = base1; // write cursor into a
      const size_t end1 = len1;
      const size_t end2 = base2 + len2;

      while (c1 < end1 and c2 < end2)
        if (cmp(a[c2], tmp[c1]))
          a[dest++] = std::move(a[c2++]);
        else
          a[dest++] = std::move(tmp[c1++]);

      // Copy remaining from tmp (run1)
      while (c1 < end1)
        a[dest++] = std::move(tmp[c1++]);
      // Remaining run2 elements are already in place
    }


    /** @brief Merge two adjacent sorted runs where len1 >= len2.
     *  a[base1..base1+len1) and a[base2..base2+len2).
     *  Copies run2 (the smaller) into tmp and merges backward.
     */
    template <typename T, class Compare>
    void merge_hi(T *a, const size_t base1, const size_t len1,
                  const size_t base2, const size_t len2,
                  T *tmp, const Compare & cmp)
    {
      // Copy run2 into tmp
      for (size_t i = 0; i < len2; ++i)
        tmp[i] = std::move(a[base2 + i]);

      // Merge from the right (highest to lowest)
      size_t dest = base2 + len2; // one past end
      size_t c1 = base1 + len1; // one past end of run1 in a
      size_t c2 = len2; // one past end of run2 in tmp

      while (c1 > base1 and c2 > 0)
        if (cmp(tmp[c2 - 1], a[c1 - 1]))
          a[--dest] = std::move(a[--c1]);
        else
          a[--dest] = std::move(tmp[--c2]);

      // Copy remaining from tmp (run2)
      while (c2 > 0)
        a[--dest] = std::move(tmp[--c2]);
      // Remaining run1 elements are already in place
    }


    /** @brief Merge runs[at] with runs[at+1].
     *
     *  Uses galloping to trim the merge range, then delegates to
     *  merge_lo or merge_hi depending on which run is smaller.
     */
    template <typename T, class Compare>
    void merge_at(T *a, size_t *run_base, size_t *run_len,
                  size_t & stack_size, const size_t at,
                  T *tmp, const Compare & cmp)
    {
      size_t base1 = run_base[at];
      size_t len1 = run_len[at];
      size_t base2 = run_base[at + 1];
      size_t len2 = run_len[at + 1];

      // Record merged run
      run_len[at] = len1 + len2;
      if (at == stack_size - 3)
        {
          run_base[at + 1] = run_base[at + 2];
          run_len[at + 1] = run_len[at + 2];
        }
      --stack_size;

      // Where does the first element of run2 go in run1?
      size_t k = gallop_right(a[base2], a + base1, len1, 0, cmp);
      base1 += k;
      len1 -= k;
      if (len1 == 0)
        return;

      // Where does the last element of run1 go in run2?
      len2 = gallop_left(a[base1 + len1 - 1], a + base2, len2,
                         len2 - 1, cmp);
      if (len2 == 0)
        return;

      if (len1 <= len2)
        merge_lo(a, base1, len1, base2, len2, tmp, cmp);
      else
        merge_hi(a, base1, len1, base2, len2, tmp, cmp);
    }


    /** @brief Maintain the timsort run stack invariants.
     *
     *  After pushing a new run, check and enforce:
     *  - run_len[i-2] > run_len[i-1] + run_len[i]
     *  - run_len[i-1] > run_len[i]
     *  Merge violating pairs until invariants hold.
     */
    template <typename T, class Compare>
    void merge_collapse(T *a, size_t *run_base, size_t *run_len,
                        size_t & stack_size, T *tmp,
                        const Compare & cmp)
    {
      while (stack_size > 1)
        {
          long n = static_cast<long>(stack_size) - 2;
          if (n > 0 and run_len[n - 1] <= run_len[n] + run_len[n + 1])
            {
              if (run_len[n - 1] < run_len[n + 1])
                --n;
              merge_at(a, run_base, run_len, stack_size, n, tmp, cmp);
            }
          else if (run_len[n] <= run_len[n + 1])
            merge_at(a, run_base, run_len, stack_size, n, tmp, cmp);
          else
            break;
        }
    }


    /** @brief Force-merge all remaining runs at the end. */
    template <typename T, class Compare>
    void merge_force_collapse(T *a, size_t *run_base, size_t *run_len,
                              size_t & stack_size, T *tmp,
                              const Compare & cmp)
    {
      while (stack_size > 1)
        {
          long n = static_cast<long>(stack_size) - 2;
          if (n > 0 and run_len[n - 1] < run_len[n + 1])
            --n;
          merge_at(a, run_base, run_len, stack_size, n, tmp, cmp);
        }
    }


    /** @brief Core Timsort implementation.
     *
     *  Implements the Timsort algorithm:
     *  1. Divide the array into runs.
     *  2. Extend short runs using binary insertion sort to at least minrun length.
     *  3. Merge runs using an adaptive merge strategy with galloping.
     *
     *  @tparam T Element type.
     *  @tparam Compare Comparison functor.
     *
     *  @param[in,out] a Pointer to the array to sort.
     *  @param[in] n Number of elements.
     *  @param[in] cmp Comparison functor.
     */
    template <typename T, class Compare>
    void timsort_impl(T *a, const size_t n, const Compare & cmp)
    {
      if (n < 2)
        return;

      // For very small arrays, just use binary insertion sort
      if (n < 64)
        {
          const size_t run_len = count_run_and_make_ascending(a, 0, n, cmp);
          binary_insertion_sort(a, 0, n, run_len, cmp);
          return;
        }

      const size_t minrun = compute_minrun(n);

      // Run stack — log2(n) + 1 entries is sufficient
      // (timsort invariant guarantees Fibonacci-like growth)
      constexpr size_t MAX_STACK = 85; // enough for 2^64 elements
      size_t run_base_arr[MAX_STACK];
      size_t run_len_arr[MAX_STACK];
      size_t stack_size = 0;

      // Temporary merge buffer — allocated once, sized to n/2
      // (merge_lo/merge_hi copy the smaller run)
      Array<T> tmp_buf = Array<T>::create(n / 2 + 1);
      T *tmp = &tmp_buf[0];

      size_t lo = 0;
      size_t remaining = n;

      while (remaining > 0)
        {
          // Find the next natural run
          size_t run_len = count_run_and_make_ascending(a, lo, lo + remaining,
                                                        cmp);

          // If the run is too short, extend it with insertion sort
          if (run_len < minrun)
            {
              const size_t force = remaining < minrun ? remaining : minrun;
              binary_insertion_sort(a, lo, lo + force, lo + run_len, cmp);
              run_len = force;
            }

          // Push this run onto the stack
          ah_runtime_error_unless(stack_size < MAX_STACK)
            << "timsort: run stack overflow (stack_size=" << stack_size
            << " >= MAX_STACK=" << MAX_STACK << "); this should never happen";
          run_base_arr[stack_size] = lo;
          run_len_arr[stack_size] = run_len;
          ++stack_size;

          // Maintain the stack invariants
          merge_collapse(a, run_base_arr, run_len_arr, stack_size, tmp, cmp);

          lo += run_len;
          remaining -= run_len;
        }

      // Force-merge all remaining runs
      merge_force_collapse(a, run_base_arr, run_len_arr, stack_size, tmp, cmp);
    }
  } // namespace timsort_detail


  /** @brief Timsort — adaptive, stable, natural merge sort.
   *
   *  Timsort is a hybrid sorting algorithm derived from merge sort and
   *  insertion sort. It identifies existing ordered subsequences (runs)
   *  in the data and merges them efficiently. This makes it particularly
   *  fast for real-world data that often contains pre-existing order.
   *
   *  @tparam T Element type (must be move-assignable).
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array to sort.
   *  @param[in] n Number of elements.
   *  @param[in] cmp Comparison functor.
   *
   *  @throws std::invalid_argument if `a` is null and `n > 0`.
   *
   *  @par Complexity
   *  - Best: O(n) — already sorted data (single run).
   *  - Average/Worst: O(n log n).
   *  - Space: O(n) for merge buffer.
   *
   *  @par Stability
   *  Stable — equal elements preserve their original relative order.
   *
   *  @see mergesort, insertion_sort, quicksort
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(T *a, const size_t n, const Compare & cmp = Compare())
  {
    if (n == 0)
      return;

    ah_invalid_argument_if(a == nullptr)
      << "timsort(): null pointer with non-zero length";

    if (n < 2)
      return;

    timsort_detail::timsort_impl(a, n, cmp);
  }


  /** @brief Timsort on a sub-range [l, r] (inclusive indices).
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *
   *  @param[in,out] a Pointer to the array.
   *  @param[in] l Left index (inclusive).
   *  @param[in] r Right index (inclusive).
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(T *a, const long l, const long r,
               const Compare & cmp = Compare())
  {
    ah_range_error_if(l < 0 or r < 0)
      << "timsort(): negative bounds [" << l << ", " << r << "] are not allowed";

    if (l >= r)
      return;

    ah_invalid_argument_if(a == nullptr)
      << "timsort(): null pointer contract violation for non-empty range ["
      << l << ", " << r << "]; cannot call timsort_detail::timsort_impl()";

    timsort_detail::timsort_impl(a + l, static_cast<size_t>(r - l + 1), cmp);
  }


  /** @brief Timsort for DynArray.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @param[in,out] a DynArray to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(DynArray<T> & a, const Compare & cmp = Compare())
  {
    bucket_sort_detail::apply_sort_with_temp<T>(a, [](T *p, size_t n, const Compare & c)
                                                  {
                                                    timsort(p, n, c);
                                                  }, cmp);
  }


  /** @brief Timsort for Array.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @param[in,out] a Array to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(Array<T> & a, const Compare & cmp = Compare())
  {
    if (a.size() < 2)
      return;
    timsort(&a[0], a.size(), cmp);
  }


  /** @brief Timsort for fixed-size C arrays.
   *
   *  @tparam T Element type.
   *  @tparam N Array size.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @param[in,out] a The array to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @ingroup Algorithms
   */
  template <typename T, size_t N, class Compare = Aleph::less<T>>
    requires std::is_invocable_r_v<bool, Compare, const T &, const T &>
  void timsort(T (&a)[N], const Compare & cmp = Compare())
  {
    timsort(static_cast<T *>(a), N, cmp);
  }


  /** @brief Timsort for DynList.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @param[in,out] list DynList to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @par Complexity
   *  Time O(n log n), Space O(n) due to temporary array copy.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(DynList<T> & list, const Compare & cmp = Compare())
  {
    bucket_sort_detail::apply_sort_with_temp<T>(list, [](T *p, size_t n, const Compare & c)
                                                  {
                                                    timsort(p, n, c);
                                                  }, cmp);
  }


  /** @brief Timsort for DynDlist.
   *
   *  @tparam T Element type.
   *  @tparam Compare Comparison functor (default: `Aleph::less<T>`).
   *  @param[in,out] list DynDlist to sort.
   *  @param[in] cmp Comparison functor.
   *
   *  @par Complexity
   *  Time O(n log n), Space O(n) due to temporary array copy.
   *
   *  @ingroup Algorithms
   */
  template <typename T, class Compare = Aleph::less<T>>
  void timsort(DynDlist<T> & list, const Compare & cmp = Compare())
  {
    bucket_sort_detail::apply_sort_with_temp<T>(list, [](T *p, size_t n, const Compare & c)
                                                  {
                                                    timsort(p, n, c);
                                                  }, cmp);
  }
}

#endif // TPL_SORT_UTILS_H
