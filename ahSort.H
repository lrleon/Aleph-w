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

/** @file ahSort.H
 *  @brief High-level sorting functions for Aleph containers.
 *
 *  This file provides generic sorting functions that work with Aleph
 *  containers (DynList, DynDlist, DynArray, Array) using efficient
 *  sorting algorithms.
 *
 *  ## Sorting Algorithms Used
 *
 *  | Container | Algorithm | Stable | Time | Space |
 *  |-----------|-----------|--------|------|-------|
 *  | DynList   | Merge sort | Yes | O(n log n) | O(log n) stack |
 *  | DynDlist  | Merge sort | Yes | O(n log n) | O(log n) stack |
 *  | DynArray  | Quicksort  | No  | O(n log n) avg | O(log n) stack |
 *  | Array     | Quicksort  | No  | O(n log n) avg | O(log n) stack |
 *
 *  ## Key Features
 *
 *  - **Generic**: Works with all Aleph sequence containers
 *  - **Custom comparators**: All functions accept optional comparison functors
 *  - **Copy semantics**: `sort()` returns a sorted copy (original unchanged)
 *  - **Move semantics**: `sort(std::move(c))` sorts efficiently by moving
 *  - **In-place**: `in_place_sort()` modifies container directly
 *  - **Stable**: List sorting preserves relative order of equal elements
 *  - **Ranking**: `ranks()` and `pair_ranks()` compute element positions
 *  - **Multi-sort**: `in_place_multisort_arrays()` sorts multiple arrays together
 *
 *  ## Functions Summary
 *
 *  | Function | Description |
 *  |----------|-------------|
 *  | `sort(c)` | Returns sorted copy of container |
 *  | `sort(c, cmp)` | Sorted copy with custom comparator |
 *  | `in_place_sort(c)` | Sorts container in place |
 *  | `stdsort(c)` | Sorts STL containers using std::sort |
 *  | `ranks(c)` | Returns array of ranks for each element |
 *  | `pair_ranks(c)` | Returns pairs of (value, rank) |
 *  | `in_place_multisort_arrays(cmp, a, b, ...)` | Sorts multiple arrays by first |
 *
 *  ## Usage Examples
 *
 *  ### Basic Sorting
 *  ```cpp
 *  DynList<int> list = {5, 2, 8, 1, 9};
 *
 *  // Return sorted copy (original unchanged)
 *  auto sorted = sort(list);  // {1, 2, 5, 8, 9}
 *
 *  // Sort with custom comparator (descending)
 *  auto desc = sort(list, std::greater<int>());  // {9, 8, 5, 2, 1}
 *
 *  // In-place sorting (modifies list)
 *  in_place_sort(list);
 *  ```
 *
 *  ### Move Semantics for Efficiency
 *  ```cpp
 *  DynList<int> temp = {5, 2, 8, 1, 9};
 *  auto sorted = sort(std::move(temp));  // temp is now empty, no copy made
 *  ```
 *
 *  ### Ranking
 *  ```cpp
 *  DynArray<int> arr = {30, 10, 20};
 *  auto r = ranks(arr);  // r = {2, 0, 1} (30 is rank 2, 10 is rank 0, etc.)
 *  ```
 *
 *  ### Multi-array Sorting
 *  ```cpp
 *  std::vector<int> keys = {3, 1, 2};
 *  std::vector<std::string> values = {"c", "a", "b"};
 *
 *  // Sort both arrays by keys
 *  in_place_multisort_arrays(std::less<int>(), keys, values);
 *  // keys = {1, 2, 3}, values = {"a", "b", "c"}
 *  ```
 *
 *  @see tpl_sort_utils.H Low-level sorting algorithms (mergesort, quicksort, etc.)
 *  @see ahFunctional.H Functional utilities including comparison functors
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

#ifndef AHSORT_H
#define AHSORT_H

#include <algorithm>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>

#include <ah-errors.H>
#include <ahFunctional.H>
#include <tpl_sort_utils.H>
#include <tpl_dynDlist.H>
#include <htlist.H>
#include <ah-zip.H>

/// @cond INTERNAL
/// Internal macro to generate sort overloads for list types.
/// This generates four overloads of sort() and two of in_place_sort()
/// for a given list container type.
#define List_Sort(List)                                                 \
  /** @brief Returns a sorted copy of a List (lvalue, lvalue comparator).
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type (default: Aleph::less<T>)
   *  @param c The list to sort (not modified)
   *  @param cmp Comparison functor
   *  @return A new list with elements in sorted order
   *  @note Uses merge sort, O(n log n) time, stable */                 \
  template <typename T, class Cmp = Aleph::less<T>> [[nodiscard]] inline \
  List<T> sort(const List<T> & c, Cmp & cmp)                            \
  {                                                                     \
    List<T> ret_val = c;                                                \
    mergesort<List, T, Cmp>(ret_val, cmp);                              \
    return ret_val;                                                     \
  }                                                                     \
                                                                        \
  /** @brief Returns a sorted copy of a List (lvalue, rvalue comparator).
   *  @copydetails sort(const List<T>&, Cmp&) */                        \
  template <typename T, class Cmp = Aleph::less<T>> [[nodiscard]] inline \
  List<T> sort(const List<T> & c, Cmp && cmp = Cmp())                   \
  {                                                                     \
    return sort<T, Cmp>(c, cmp);                                        \
  }                                                                     \
                                                                        \
  /** @brief Sorts an rvalue List in place and returns it (move semantics).
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type
   *  @param c The list to sort (will be moved from)
   *  @param cmp Comparison functor
   *  @return The sorted list (moved)
   *  @note Original container is left in valid but unspecified state */\
  template <typename T, class Cmp = Aleph::less<T>> [[nodiscard]] inline \
  List<T> sort(List<T> && c, Cmp & cmp)                                 \
  {                                                                     \
    mergesort<List, T, Cmp>(c, cmp);                                    \
    return std::move(c);                                                \
  }                                                                     \
                                                                        \
  /** @brief Sorts an rvalue List (move semantics, rvalue comparator).
   *  @copydetails sort(List<T>&&, Cmp&) */                             \
  template <typename T, class Cmp = Aleph::less<T>> [[nodiscard]] inline \
  List<T> sort(List<T> && c, Cmp && cmp = Cmp())                        \
  {                                                                     \
    return sort<T, Cmp>(std::move(c), cmp);                             \
  }                                                                     \
                                                                        \
  /** @brief Sorts a List in place.
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type
   *  @param c The list to sort (modified in place)
   *  @param cmp Comparison functor
   *  @return Reference to the sorted list
   *  @note Uses merge-insert sort hybrid, O(n log n) time, stable */   \
  template <typename T, class Cmp = Aleph::less<T>> inline              \
  List<T> & in_place_sort(List<T> & c, Cmp & cmp)                       \
  {                                                                     \
    mergeinsertsort(c, cmp);                                            \
    return c;                                                           \
  }                                                                     \
                                                                        \
  /** @brief Sorts a List in place (rvalue comparator).
   *  @copydetails in_place_sort(List<T>&, Cmp&) */                     \
  template <typename T, class Cmp = Aleph::less<T>> inline              \
  List<T> & in_place_sort(List<T> & c, Cmp && cmp = Cmp())              \
  {                                                                     \
    return in_place_sort<T, Cmp>(c, cmp);                               \
  }
/// @endcond

namespace Aleph
{
  // Generate sort overloads for DynList and DynDlist
  List_Sort(DynList);
  List_Sort(DynDlist);

  /** @brief Returns a sorted copy of a DynArray.
   *
   *  Creates a copy of the input array and sorts it using quicksort.
   *
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type (default: Aleph::less<T>)
   *  @param a The array to sort (not modified)
   *  @param cmp Comparison functor
   *  @return A new array with elements in sorted order
   *
   *  @note Uses quicksort, O(n log n) average time, NOT stable
   *  @see in_place_sort() for sorting without copying
   */
  template<typename T, class Cmp = Aleph::less<T>>
  [[nodiscard]] inline
  DynArray<T> sort(const DynArray<T> & a, Cmp && cmp = Cmp())
  {
    DynArray<T> ret_val = a;
    quicksort_op(ret_val, cmp);
    return ret_val;
  }

  /** @brief Sorts an rvalue DynArray in place and returns it.
   *
   *  Efficient move-based sorting that avoids copying.
   *
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type
   *  @param a The array to sort (will be moved from)
   *  @param cmp Comparison functor
   *  @return The sorted array (moved)
   *
   *  @note Original container is left in valid but unspecified state
   */
  template<typename T, class Cmp = Aleph::less<T>>
  [[nodiscard]] inline
  DynArray<T> sort(DynArray<T> && a, Cmp && cmp = Cmp())
  {
    quicksort_op(a, cmp);
    return std::move(a);
  }

  /** @brief Returns a sorted copy of an Array.
   *  @copydetails sort(const DynArray<T>&, Cmp&&)
   */
  template<typename T, class Cmp = Aleph::less<T>>
  [[nodiscard]] inline
  Array<T> sort(const Array<T> & a, Cmp && cmp = Cmp())
  {
    Array<T> ret_val = a;
    quicksort_op(ret_val, cmp);
    return ret_val;
  }

  /** @brief Sorts an rvalue Array in place and returns it.
   *  @copydetails sort(DynArray<T>&&, Cmp&&)
   */
  template<typename T, class Cmp = Aleph::less<T>>
  [[nodiscard]] inline
  Array<T> sort(Array<T> && a, Cmp && cmp = Cmp())
  {
    quicksort_op(a, cmp);
    return std::move(a);
  }

  /** @brief Sorts an STL-compatible container using std::sort.
   *
   *  This function provides a convenient way to sort STL containers
   *  (std::vector, std::deque, etc.) with a consistent interface.
   *
   *  @tparam Container STL-compatible container type with begin()/end()
   *  @tparam Cmp Comparison functor type (default: std::less)
   *  @param c The container to sort (not modified)
   *  @param cmp Comparison functor
   *  @return A new sorted container
   *
   *  @note Uses std::sort internally, O(n log n) average, NOT stable
   *  @see std::stable_sort for stable sorting
   *
   *  @par Example
   *  @code
   *  std::vector<int> v = {3, 1, 4, 1, 5};
   *  auto sorted = stdsort(v);  // {1, 1, 3, 4, 5}
   *  @endcode
   */
  template<typename Container,
           class Cmp = std::less<typename Container::value_type>>
  [[nodiscard]] inline
  Container stdsort(const Container & c, Cmp cmp = Cmp())
  {
    Container ret = c;
    std::sort(ret.begin(), ret.end(), cmp);
    return ret;
  }

  /** @brief Sorts a DynArray in place.
   *
   *  Modifies the array directly without creating a copy.
   *
   *  @tparam T Element type
   *  @tparam Cmp Comparison functor type
   *  @param c The array to sort (modified in place)
   *  @param cmp Comparison functor
   *  @return Reference to the sorted array
   *
   *  @note Uses quicksort, O(n log n) average time
   */
  template<typename T, class Cmp = Aleph::less<T>>
  inline
  DynArray<T> & in_place_sort(DynArray<T> & c, Cmp cmp = Cmp())
  {
    quicksort_op(c, cmp);
    return c;
  }

  /** @brief Sorts an Array in place.
   *  @copydetails in_place_sort(DynArray<T>&, Cmp)
   */
  template<typename T, class Cmp = Aleph::less<T>>
  inline
  Array<T> & in_place_sort(Array<T> & c, Cmp cmp = Cmp())
  {
    quicksort_op(c, cmp);
    return c;
  }

  /// @cond INTERNAL
  /** @brief Internal helper class for computing element ranks.
   *
   *  This class provides the implementation for ranks() and pair_ranks().
   *  It is not intended for direct use; use the free functions instead.
   *
   *  @tparam T Element type
   *  @tparam C Array container template (DynArray or Array)
   *
   *  @note Primary template uses DynArray-style semantics where operator(i)
   *        auto-expands the array. A specialization exists for Array<T>.
   */
  template<typename T, template <typename> class C>
  class Compute_Ranks
  {
    using P = std::pair<T, size_t>;

  public:
    /// Computes ranks for elements in an array container (DynArray version).
    C<size_t> compute_ranks(const C<T> & c)
    {
      const size_t n = c.size();
      C<size_t> indexes;
      indexes.reserve(n);
      for (size_t i = 0; i < n; ++i)
        indexes(i) = i;  // DynArray auto-expands
      quicksort_op(indexes, [&c](auto i1, auto i2)
      {
        return c(i1) < c(i2);
      });
      C<size_t> ret;
      ret.reserve(n);
      for (size_t i = 0; i < n; ++i)
        ret(indexes(i)) = i;  // DynArray auto-expands
      return ret;
    }

    /// Computes ranks for elements in a list container.
    template<template <typename Type> class List>
    C<size_t> list_compute_ranks(const List<T> & c)
    {
      C<T> items;
      size_t n = 0;
      C<size_t> indexes;
      c.for_each([&items, &n, &indexes](auto k)
      {
        items.append(k);
        indexes.append(n++);
      });
      quicksort_op(indexes, [&items](auto i1, auto i2)
      {
        return items(i1) < items(i2);
      });
      C<size_t> ret;
      ret.reserve(n);
      for (size_t i = 0; i < n; ++i)
        ret(indexes(i)) = i;  // DynArray auto-expands
      return ret;
    }

    /// Computes (value, rank) pairs for elements in an array container.
    C<P> compute_pair_ranks(const C<T> & c)
    {
      const size_t n = c.size();
      C<size_t> indexes;
      indexes.reserve(n);
      for (size_t i = 0; i < n; ++i)
        indexes(i) = i;
      quicksort_op(indexes, [&c](auto i1, auto i2)
      {
        return c(i1) < c(i2);
      });
      C<P> ret;
      ret.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          auto idx = indexes(i);
          ret(idx) = P(c(idx), i);
        }
      return ret;
    }

    /// Computes (value, rank) pairs for elements in a list container.
    template<template <typename Type> class List>
    C<P> list_pair_ranks(const List<T> & c)
    {
      C<T> items;
      size_t n = 0;
      C<size_t> indexes;
      c.for_each([&items, &n, &indexes](auto k)
      {
        items.append(k);
        indexes.append(n++);
      });
      quicksort_op(indexes, [&items](auto i1, auto i2)
      {
        return items(i1) < items(i2);
      });
      C<P> ret;
      ret.reserve(n);
      for (size_t i = 0; i < n; ++i)
        {
          auto idx = indexes(i);
          ret(idx) = P(items(idx), i);
        }
      return ret;
    }
  };

  /** @brief Specialization of Compute_Ranks for Array<T>.
   *
   *  Array<T> does not auto-expand on operator(i) like DynArray does.
   *  This specialization uses putn() to pre-allocate slots before
   *  assignment, which properly expands the size.
   */
  template<typename T>
  class Compute_Ranks<T, Array>
  {
    using P = std::pair<T, size_t>;

  public:
    /// Computes ranks for elements in an Array.
    Array<size_t> compute_ranks(const Array<T> & c)
    {
      const size_t n = c.size();
      Array<size_t> indexes(n);
      for (size_t i = 0; i < n; ++i)
        indexes.append(i);  // Use append to grow size
      quicksort_op(indexes, [&c](auto i1, auto i2)
      {
        return c(i1) < c(i2);
      });
      Array<size_t> ret(n);
      ret.putn(n);  // Allocate n slots (expands size)
      for (size_t i = 0; i < n; ++i)
        ret(indexes(i)) = i;
      return ret;
    }

    /// Computes (value, rank) pairs for elements in an Array.
    Array<P> compute_pair_ranks(const Array<T> & c)
    {
      const size_t n = c.size();
      Array<size_t> indexes(n);
      for (size_t i = 0; i < n; ++i)
        indexes.append(i);
      quicksort_op(indexes, [&c](auto i1, auto i2)
      {
        return c(i1) < c(i2);
      });
      Array<P> ret(n);
      ret.putn(n);  // Allocate n slots
      for (size_t i = 0; i < n; ++i)
        {
          auto idx = indexes(i);
          ret(idx) = P(c(idx), i);
        }
      return ret;
    }
  };
  /// @endcond

  /** @brief Computes the rank of each element in an Array.
   *
   *  The rank of an element is its position in the sorted order.
   *  For n elements, ranks range from 0 (smallest) to n-1 (largest).
   *
   *  @tparam T Element type (must be comparable with operator<)
   *  @param array The array to compute ranks for
   *  @return An Array where result[i] is the rank of array[i]
   *
   *  @par Complexity
   *  O(n log n) time, O(n) space
   *
   *  @par Example
   *  @code
   *  Array<int> arr = {30, 10, 20};
   *  auto r = ranks(arr);
   *  // r[0] = 2 (30 is the largest, rank 2)
   *  // r[1] = 0 (10 is the smallest, rank 0)
   *  // r[2] = 1 (20 is in the middle, rank 1)
   *  @endcode
   *
   *  @see pair_ranks() for getting (value, rank) pairs
   */
  template<typename T>
  [[nodiscard]] Array<size_t> ranks(const Array<T> & array)
  {
    return Compute_Ranks<T, Array>().compute_ranks(array);
  }

  /** @brief Computes the rank of each element in a DynArray.
   *  @copydetails ranks(const Array<T>&)
   */
  template<typename T>
  [[nodiscard]] DynArray<size_t> ranks(const DynArray<T> & array)
  {
    return Compute_Ranks<T, DynArray>().compute_ranks(array);
  }

  /** @brief Computes the rank of each element in a DynList.
   *
   *  @tparam T Element type
   *  @param l The list to compute ranks for
   *  @return A DynArray where result[i] is the rank of the i-th element
   *
   *  @note The list is first copied to an array for efficient random access
   */
  template<typename T>
  [[nodiscard]] DynArray<size_t> ranks(const DynList<T> & l)
  {
    return Compute_Ranks<T, DynArray>().list_compute_ranks(l);
  }

  /** @brief Computes the rank of each element in a DynDlist.
   *  @copydetails ranks(const DynList<T>&)
   */
  template<typename T>
  [[nodiscard]] DynArray<size_t> ranks(const DynDlist<T> & l)
  {
    return Compute_Ranks<T, DynArray>().list_compute_ranks(l);
  }

  /** @brief Computes (value, rank) pairs for each element in an Array.
   *
   *  Similar to ranks(), but returns pairs containing both the original
   *  value and its rank for convenience.
   *
   *  @tparam T Element type
   *  @param c The array to compute ranks for
   *  @return An Array of pairs where result[i] = (c[i], rank of c[i])
   *
   *  @par Example
   *  @code
   *  Array<int> arr = {30, 10, 20};
   *  auto pr = pair_ranks(arr);
   *  // pr[0] = {30, 2}
   *  // pr[1] = {10, 0}
   *  // pr[2] = {20, 1}
   *  @endcode
   *
   *  @see ranks() for getting only the ranks
   */
  template<typename T>
  [[nodiscard]] auto pair_ranks(const Array<T> & c)
  {
    return Compute_Ranks<T, Array>().compute_pair_ranks(c);
  }

  /** @brief Computes (value, rank) pairs for each element in a DynArray.
   *  @copydetails pair_ranks(const Array<T>&)
   */
  template<typename T>
  [[nodiscard]] auto pair_ranks(const DynArray<T> & c)
  {
    return Compute_Ranks<T, DynArray>().compute_pair_ranks(c);
  }

  /** @brief Computes (value, rank) pairs for each element in a DynList.
   *
   *  @tparam T Element type
   *  @param l The list to compute ranks for
   *  @return A DynArray of pairs
   */
  template<typename T>
  [[nodiscard]] auto pair_ranks(const DynList<T> & l)
  {
    Compute_Ranks<T, DynArray> func;
    return func.list_pair_ranks(l);
  }

  /** @brief Computes (value, rank) pairs for each element in a DynDlist.
   *  @copydetails pair_ranks(const DynList<T>&)
   */
  template<typename T>
  [[nodiscard]] auto pair_ranks(const DynDlist<T> & l)
  {
    Compute_Ranks<T, DynArray> func;
    return func.list_pair_ranks(l);
  }

  /** @brief Sorts multiple arrays in place, using the first array as the key.
   *
   *  This function sorts multiple arrays together, keeping them synchronized.
   *  The first array provides the sort keys, and all other arrays are
   *  rearranged to maintain the correspondence between elements.
   *
   *  Uses stable sort, preserving relative order of equal elements.
   *
   *  @tparam C Container type for first array (must have operator[], size())
   *  @tparam Args Container types for additional arrays
   *  @tparam Cmp Comparison functor type (default: std::less)
   *
   *  @param cmp Comparison functor for elements of the first array
   *  @param first The array providing sort keys (modified in place)
   *  @param args Additional arrays to sort in sync (modified in place)
   *
   *  @throws std::invalid_argument if arrays have different sizes
   *
   *  @par Complexity
   *  O(n log n) time for sorting, O(n) space for indices
   *
   *  @par Example
   *  @code
   *  std::vector<int> ids = {3, 1, 2};
   *  std::vector<std::string> names = {"Charlie", "Alice", "Bob"};
   *  std::vector<int> ages = {30, 25, 28};
   *
   *  in_place_multisort_arrays(std::less<int>(), ids, names, ages);
   *
   *  // After sorting by id:
   *  // ids   = {1, 2, 3}
   *  // names = {"Alice", "Bob", "Charlie"}
   *  // ages  = {25, 28, 30}
   *  @endcode
   *
   *  @note All arrays MUST have the same size
   *  @note This is useful for sorting parallel arrays (struct of arrays pattern)
   */
  template<typename C, typename... Args,
           typename Cmp = std::less<typename C::value_type>>
  inline
  void in_place_multisort_arrays(Cmp cmp, C & first, Args &... args)
  {
    const size_t n = first.size();
    if (n == 0)
      return;

    const bool all_same_size = ((args.size() == n) && ...);
    ah_invalid_argument_if(not all_same_size)
      << "all arrays must have the same size";

    std::vector<size_t> indices(n);
    std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

    std::stable_sort(indices.begin(), indices.end(), [&first, &cmp](size_t i1, size_t i2)
    {
      return cmp(first[i1], first[i2]);
    });

    // Apply permutation using cycle-chasing algorithm
    // Mark visited positions by setting indices[i] = i after processing
    using std::swap;  // Enable ADL for user-defined swap
    for (size_t i = 0; i < n; ++i)
      {
        // Skip if this position is already in its final place
        // or if we've already processed this cycle
        if (indices[i] == i)
          continue;

        // Follow the cycle starting at position i
        size_t current = i;
        while (indices[current] != i)
          {
            size_t next = indices[current];
            swap(first[current], first[next]);
            (swap(args[current], args[next]), ...);
            indices[current] = current;  // Mark as processed
            current = next;
          }
        indices[current] = current;  // Mark the last element of the cycle
      }
  }


} // end namespace Aleph

#endif // AHSORT_H
