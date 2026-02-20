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

/** @file ah-unique.H
 *  @brief Deduplicate sequential Aleph containers in-place.
 *
 *  Provides `in_place_unique`, a free function that removes duplicates while
 *  preserving the first occurrence order for Aleph sequential containers
 *  (Array, DynArray, DynList, DynDlist). Complexity is O(n log n) using a
 *  `std::set` as the seen-set.
 */

#ifndef AH_UNIQUE_H
#define AH_UNIQUE_H

#include <concepts>
#include <set>
#include <ah-concepts.H>

namespace Aleph {

  /// Concept for Aleph sequential containers with traversal and append semantics.
  template <class Container>
  concept AlephSequential = requires(Container c,
                                     typename Container::Key_Type v,
                                     bool (&op)(typename Container::Key_Type &))
  {
    typename Container::Key_Type;
    { c.traverse(op) } -> std::convertible_to<bool>;
    c.append(v);
    c.swap(c);
    { c.size() } -> std::convertible_to<size_t>;
  };

  /**
   * @brief Remove duplicates in-place preserving first occurrence order.
   *
   * @tparam Container AlephSequential container type (Array, DynArray, DynList, DynDlist).
   * @tparam Compare Strict weak ordering on Key_Type (defaults to std::less<Key>).
   *
   * @param c   Container to deduplicate.
   * @param cmp Comparator for keys.
   */
  template <AlephSequential Container,
            class Compare = std::less<typename Container::Key_Type>>
    requires Aleph::StrictWeakOrder<Compare, typename Container::Key_Type>
  void in_place_unique(Container & c, Compare cmp = {})
  {
    using Key = typename Container::Key_Type;

    std::set<Key, Compare> seen(cmp);
    Container clean;

    // Reserve only for vector-like containers (e.g. Array) where reserve()
    // does not change logical size. DynArray is intentionally excluded.
    if constexpr (requires(Container & x)
                  {
                    { x.capacity() } -> std::convertible_to<size_t>;
                    x.reserve(size_t{});
                  })
      if (clean.capacity() < c.size())
        clean.reserve(c.size());

    c.traverse([&](Key & item)
               {
                 if (seen.contains(item))
                   return true; // skip duplicates

                 seen.insert(item);
                 clean.append(item);
                 return true;
               });

    c.swap(clean);
  }

} // namespace Aleph

#endif // AH_UNIQUE_H
