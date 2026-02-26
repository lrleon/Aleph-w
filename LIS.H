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


/** @file LIS.H
 *  @brief Longest Increasing Subsequence (LIS) via patience sorting.
 *
 *  Provides O(n log n) computation of the longest increasing subsequence
 *  using patience sorting with binary search, plus optional reconstruction
 *  of the actual subsequence.
 *
 *  @example lis_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef LIS_H
# define LIS_H

# include <algorithm>
# include <cstddef>
# include <limits>
# include <utility>

# include <ahFunction.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Result of a LIS computation. */
  template <typename T>
  struct LIS_Result
  {
    size_t length = 0;        /**< Length of the LIS */
    Array<T> subsequence;     /**< One optimal LIS */
  };

  namespace lis_detail
  {
    template <typename T, class Compare>
    [[nodiscard]] inline size_t
    lower_bound_pos(const Array<T> & tails, const T & value, Compare cmp)
    {
      size_t lo = 0, hi = tails.size();
      while (lo < hi)
        {
          const size_t mid = lo + (hi - lo) / 2;
          if (cmp(tails[mid], value))
            lo = mid + 1;
          else
            hi = mid;
        }
      return lo;
    }

    template <typename T, class Compare>
    [[nodiscard]] inline size_t
    upper_bound_pos(const Array<T> & tails, const T & value, Compare cmp)
    {
      size_t lo = 0, hi = tails.size();
      while (lo < hi)
        {
          const size_t mid = lo + (hi - lo) / 2;
          if (cmp(value, tails[mid]))
            hi = mid;
          else
            lo = mid + 1;
        }
      return lo;
    }
  } // namespace lis_detail


  /** @brief Compute the Longest Increasing Subsequence (patience sorting).
   *
   *  @tparam T        Element type (must be comparable via @p Compare).
   *  @tparam Compare  Strict weak ordering (default `Aleph::less<T>`).
   *
   *  @param[in] seq  Input sequence.
   *  @param[in] cmp  Comparator instance.
   *  @return LIS_Result containing the length and one optimal subsequence.
   *
   *  @note Complexity: O(n log n) time, O(n) space.
   */
  template <typename T, class Compare = Aleph::less<T>>
  [[nodiscard]] LIS_Result<T>
  longest_increasing_subsequence(const Array<T> & seq,
                                 Compare cmp = Compare())
  {
    const size_t n = seq.size();
    if (n == 0)
      return LIS_Result<T>{0, Array<T>()};

    // tails[i] = smallest tail element for IS of length i+1
    Array<T> tails;
    tails.reserve(n);

    // parent[i] = index of predecessor of seq[i] in best IS ending at seq[i]
    Array<size_t> parent = Array<size_t>::create(n);
    // pos[i] = index in seq of the element at tails[i]
    Array<size_t> tail_idx;
    tail_idx.reserve(n);

    for (size_t i = 0; i < n; ++i)
      {
        const size_t lo = lis_detail::lower_bound_pos(tails, seq[i], cmp);

        if (lo == tails.size())
          {
            tails.append(seq[i]);
            tail_idx.append(i);
          }
        else
          {
            tails[lo] = seq[i];
            tail_idx[lo] = i;
          }

        parent(i) = lo > 0 ? tail_idx[lo - 1]
                            : std::numeric_limits<size_t>::max();
      }

    // reconstruct
    const size_t lis_len = tails.size();
    Array<T> result = Array<T>::create(lis_len);
    size_t idx = tail_idx[lis_len - 1];
    for (size_t k = lis_len; k > 0; --k)
      {
        result(k - 1) = seq[idx];
        idx = parent(idx);
      }

    return LIS_Result<T>{lis_len, std::move(result)};
  }


  /** @brief Compute only the length of the LIS (no reconstruction).
   *
   *  @tparam T        Element type.
   *  @tparam Compare  Strict weak ordering.
   *
   *  @param[in] seq  Input sequence.
   *  @param[in] cmp  Comparator instance.
   *  @return Length of the LIS.
   *
   *  @note Complexity: O(n log n) time, O(n) space.
   */
  template <typename T, class Compare = Aleph::less<T>>
  [[nodiscard]] size_t
  lis_length(const Array<T> & seq, Compare cmp = Compare())
  {
    const size_t n = seq.size();
    if (n == 0)
      return 0;

    Array<T> tails;
    tails.reserve(n);

    for (size_t i = 0; i < n; ++i)
      {
        const size_t lo = lis_detail::lower_bound_pos(tails, seq[i], cmp);

        if (lo == tails.size())
          tails.append(seq[i]);
        else
          tails[lo] = seq[i];
      }

    return tails.size();
  }


  /** @brief Compute the Longest Non-Decreasing Subsequence.
   *
   *  Uses upper-bound semantics over patience tails:
   *  first position where `tail > x`, preserving non-decreasing order.
   *
   *  @tparam T        Element type.
   *  @tparam Compare  Strict weak ordering (default `Aleph::less<T>`).
   *  @param[in] seq  Input sequence.
   *  @param[in] cmp  Comparator instance.
   *  @return LIS_Result with the longest non-decreasing subsequence.
   *
   *  @note Complexity: O(n log n) time, O(n) space.
   */
  template <typename T, class Compare = Aleph::less<T>>
  [[nodiscard]] LIS_Result<T>
  longest_nondecreasing_subsequence(const Array<T> & seq,
                                    Compare cmp = Compare())
  {
    const size_t n = seq.size();
    if (n == 0)
      return LIS_Result<T>{0, Array<T>()};

    Array<T> tails;
    tails.reserve(n);

    Array<size_t> parent = Array<size_t>::create(n);
    Array<size_t> tail_idx;
    tail_idx.reserve(n);

    for (size_t i = 0; i < n; ++i)
      {
        const size_t pos = lis_detail::upper_bound_pos(tails, seq[i], cmp);

        if (pos == tails.size())
          {
            tails.append(seq[i]);
            tail_idx.append(i);
          }
        else
          {
            tails[pos] = seq[i];
            tail_idx[pos] = i;
          }

        parent(i) = pos > 0 ? tail_idx[pos - 1]
                            : std::numeric_limits<size_t>::max();
      }

    const size_t len = tails.size();
    Array<T> result = Array<T>::create(len);
    size_t idx = tail_idx[len - 1];
    for (size_t k = len; k > 0; --k)
      {
        result(k - 1) = seq[idx];
        idx = parent(idx);
      }

    return LIS_Result<T>{len, std::move(result)};
  }
} // namespace Aleph

# endif // LIS_H
