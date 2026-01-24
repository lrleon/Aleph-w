
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


/** @file array_utils.H
 *  @brief Utility functions for array manipulation.
 *
 *  This file provides efficient algorithms for manipulating contiguous
 *  arrays including gap operations and rotations.
 *
 *  ## Functions
 *
 *  | Function | Description | Complexity |
 *  |----------|-------------|------------|
 *  | open_gap | Insert space at position | O(n) |
 *  | close_gap | Remove elements at position | O(n) |
 *  | reverse | Reverse array in-place | O(n) |
 *  | rotate_left | Rotate elements left | O(n) |
 *  | rotate_right | Rotate elements right | O(n) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  int arr[] = {1, 2, 3, 4, 5};
 *
 *  // Rotate left by 2: {3, 4, 5, 1, 2}
 *  Aleph::rotate_left(arr, 5, 2);
 *
 *  // Reverse: {2, 1, 5, 4, 3}
 *  Aleph::reverse(arr, 5);
 *  ```
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef ARRAY_UTILS_H
# define ARRAY_UTILS_H

# include <cstdlib>
# include <cassert>
# include <stdexcept>

# include <ah-errors.H>
# include <iostream>
# include <utility>

namespace Aleph
{

/**
 * @brief Open a gap in an array by shifting elements right.
 *
 * Moves elements from [pos, n-num_entries) to [pos+num_entries, n),
 * creating space for num_entries new elements at position pos.
 *
 * @tparam Tarray Array-like type with operator[]
 * @param ptr Array to modify
 * @param n Current number of elements
 * @param pos Position where to open gap (default 0)
 * @param num_entries Number of positions to open (default 1)
 *
 * @throw std::out_of_range if pos >= n or num_entries > available space
 *
 * @note Uses move semantics for efficiency
 */
template <class Tarray> inline
void open_gap(Tarray & ptr, size_t n, size_t pos = 0, size_t num_entries = 1)
{
  ah_out_of_range_error_if(pos >= n) << "pos is greater than n";

  const long navail = long(n) - pos;
  ah_out_of_range_error_if(long(num_entries) > navail)
      << "num_entries is greater than number of available entries";

  for (long k = n - num_entries - 1, i = n - 1; k >= long(pos); --i, --k)
    ptr[i] = std::move(ptr[k]);
}

/**
 * @brief Close a gap in an array by shifting elements left.
 *
 * Removes num_entries elements starting at pos by shifting
 * subsequent elements left.
 *
 * @tparam T Element type
 * @param ptr Pointer to array
 * @param n Number of elements
 * @param pos Position of gap to close
 * @param num_entries Number of positions to close (default 1)
 *
 * @throw std::out_of_range if pos >= n or num_entries > remaining elements
 *
 * @note Uses move semantics for efficiency
 */
template <typename T> inline
void close_gap(T * ptr, size_t n, size_t pos, size_t num_entries = 1)
{
  assert(ptr);

  ah_out_of_range_error_if(pos >= n) << "pos is greater than n";

  const long navail = n - pos;
  ah_out_of_range_error_if(long(num_entries) > navail)
      << "num_entries is greater than pos";

  for (size_t i = pos; i < n - num_entries; ++i)
    ptr[i] = std::move(ptr[i + num_entries]);
}

/**
 * @brief Reverse an array in-place.
 *
 * Swaps elements symmetrically around the center.
 *
 * @tparam T Element type
 * @param ptr Pointer to array
 * @param n Number of elements
 *
 * @par Example
 * @code
 * int arr[] = {1, 2, 3, 4, 5};
 * reverse(arr, 5);  // arr = {5, 4, 3, 2, 1}
 * @endcode
 */
template <typename T> inline
void reverse(T * ptr, const size_t n) noexcept
{
  for (size_t i = 0; i < n/2; ++i)
    std::swap(ptr[i], ptr[n - i - 1]);
}

/**
 * @brief Rotate array elements left by m positions.
 *
 * Uses the three-reversal algorithm for O(n) time and O(1) space.
 *
 * @tparam T Element type
 * @param ptr Pointer to array
 * @param n Number of elements
 * @param m Number of positions to rotate (wrapped if > n)
 *
 * @par Example
 * @code
 * int arr[] = {1, 2, 3, 4, 5};
 * rotate_left(arr, 5, 2);  // arr = {3, 4, 5, 1, 2}
 * @endcode
 *
 * @note No-op if n <= 1 or m == 0
 */
template <typename T> inline
void rotate_left(T * ptr, const size_t n, size_t m) noexcept
{
  if (n <= 1 or m == 0)
    return;

  m = m % n;  // Safe: n > 0 guaranteed
  if (m == 0)
    return;

  reverse(ptr, m);
  reverse(ptr + m, n - m);
  reverse(ptr, n);
}

/**
 * @brief Rotate array elements right by m positions.
 *
 * Uses the three-reversal algorithm for O(n) time and O(1) space.
 *
 * @tparam T Element type
 * @param ptr Pointer to array
 * @param n Number of elements
 * @param m Number of positions to rotate (wrapped if > n)
 *
 * @par Example
 * @code
 * int arr[] = {1, 2, 3, 4, 5};
 * rotate_right(arr, 5, 2);  // arr = {4, 5, 1, 2, 3}
 * @endcode
 *
 * @note No-op if n <= 1 or m == 0
 */
template <typename T> inline
void rotate_right(T * ptr, const size_t n, size_t m) noexcept
{
  if (n <= 1 or m == 0)
    return;

  m = m % n;  // Safe: n > 0 guaranteed
  if (m == 0)
    return;

  reverse(ptr, n);
  reverse(ptr, m);
  reverse(ptr + m, n - m);
}

} // namespace Aleph

# endif
