
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


/**
 * @file ah_stdc++_utils.H
 * @brief Iterator and container validation utilities.
 *
 * This header provides utility functions for validating that iterators
 * belong to specific containers. These are useful for debugging and
 * for implementing robust container operations.
 *
 * ## Overview
 *
 * When working with STL-style iterators, it's often necessary to verify
 * that an iterator is valid for a particular container before performing
 * operations. These utilities provide a consistent way to perform such
 * checks and throw appropriate exceptions on failure.
 *
 * ## Iterator Requirements
 *
 * The iterators used with these functions must provide a `verify()` method:
 * - `bool verify(const Container&)` - returns true if iterator belongs to container
 * - `bool verify(const Iterator&)` - returns true if both iterators belong to same container
 *
 * ## Example Usage
 *
 * ```cpp
 * template <typename T>
 * void insert_range(DynList<T>& dest, Iterator first, Iterator last)
 * {
 *     // Verify iterators belong to the same source container
 *     verify_iterators(first, last);
 *
 *     // ... perform insertion ...
 * }
 * ```
 *
 * @ingroup Utilities
 * @author Leandro Rabindranath León
 */

# ifndef AH_STDC_UTILS_H
# define AH_STDC_UTILS_H

# include <exception>
# include <stdexcept>
# include <ah-errors.H>

namespace Aleph
{

/**
 * @brief Verifies that an iterator belongs to a specific container.
 *
 * Checks that the iterator is associated with the given container.
 * Throws std::domain_error if the verification fails.
 *
 * @tparam Container The container type.
 * @tparam Iterator The iterator type (must have `verify(Container&)` method).
 * @param container The container to check against.
 * @param itor The iterator to verify.
 * @throws std::domain_error if the iterator does not belong to the container.
 *
 * ## Example
 *
 * ```cpp
 * DynList<int> list = {1, 2, 3};
 * auto it = list.begin();
 * verify_container_and_iterator(list, it);  // OK
 *
 * DynList<int> other_list;
 * // verify_container_and_iterator(other_list, it);  // throws!
 * ```
 */
template <typename Container, typename Iterator>
inline void verify_container_and_iterator(const Container& container,
                                          const Iterator& itor)
{
  ah_domain_error_if(not itor.verify(container))
    << "Iterator does not belong to the specified container";
}

/**
 * @brief Verifies that two iterators belong to the same container.
 *
 * Checks that both iterators are associated with the same underlying
 * container. Throws std::domain_error if they belong to different containers.
 *
 * @tparam Iterator The iterator type (must have `verify(Iterator&)` method).
 * @param itor1 First iterator.
 * @param itor2 Second iterator.
 * @throws std::domain_error if iterators belong to different containers.
 *
 * ## Example
 *
 * ```cpp
 * DynList<int> list = {1, 2, 3, 4, 5};
 * auto first = list.begin();
 * auto last = list.end();
 * verify_iterators(first, last);  // OK - same container
 *
 * DynList<int> other;
 * auto other_it = other.begin();
 * // verify_iterators(first, other_it);  // throws!
 * ```
 */
template <typename Iterator>
inline void verify_iterators(const Iterator& itor1, const Iterator& itor2)
{
  ah_domain_error_if(not itor1.verify(itor2))
    << "Iterators do not belong to the same container";
}

/**
 * @brief Verifies container-iterator relationships with distinctness check.
 *
 * This overload performs three verifications:
 * 1. `itor_container` belongs to `container`
 * 2. `itor1` and `itor2` belong to the same container (as each other)
 * 3. `itor_container` is distinct from `itor1` (they reference different containers)
 *
 * This is useful when you have a "destination" iterator (itor_container) and
 * a "source" range (itor1, itor2), and you need to ensure they don't overlap.
 *
 * @tparam Container The container type.
 * @tparam Iterator The iterator type.
 * @param container The destination container.
 * @param itor_container Iterator into the destination container.
 * @param itor1 Start of source range.
 * @param itor2 End of source range.
 * @throws std::domain_error if any verification fails.
 */
template <typename Container, typename Iterator>
inline void verify_container_and_iterators(const Container& container,
                                           const Iterator& itor_container,
                                           const Iterator& itor1,
                                           const Iterator& itor2)
{
  verify_container_and_iterator(container, itor_container);
  verify_iterators(itor1, itor2);

  // Ensure destination iterator is from a different container than source range
  ah_domain_error_if(itor_container.verify(itor1))
    << "Destination and source iterators reference the same container";
}

/**
 * @brief Verifies that two iterators both belong to a specific container.
 *
 * A simpler overload that just checks both iterators belong to the given container.
 *
 * @tparam Container The container type.
 * @tparam Iterator The iterator type.
 * @param container The container to check against.
 * @param itor1 First iterator.
 * @param itor2 Second iterator.
 * @throws std::domain_error if either iterator doesn't belong to the container.
 *
 * ## Example
 *
 * ```cpp
 * DynList<int> list = {1, 2, 3, 4, 5};
 * auto first = list.begin();
 * auto last = list.end();
 * verify_container_and_iterators(list, first, last);  // OK
 * ```
 */
template <typename Container, typename Iterator>
inline void verify_container_and_iterators(const Container& container,
                                           const Iterator& itor1,
                                           const Iterator& itor2)
{
  verify_container_and_iterator(container, itor1);
  verify_container_and_iterator(container, itor2);
}

} // namespace Aleph

# endif // AH_STDC_UTILS_H
