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

/** @file Subset_Sum.H
 *  @brief Subset sum algorithms: classical DP and meet-in-the-middle.
 *
 *  The subset sum problem is a decision problem in computer science that asks
 *  whether a subset of a given set of integers sums to a target value.
 *
 *  This header provides several variants:
 *  - **Classical DP**: O(n * target) for integral values, includes reconstruction.
 *  - **Existence check**: Optimized O(target) space version.
 *  - **Counting**: Counts the number of distinct subsets that achieve the target.
 *  - **Meet-in-the-middle (MITM)**: Efficient for larger target values but
 *    smaller sets (up to ~40 elements).
 *
 *  @example subset_sum_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

#ifndef SUBSET_SUM_H
#define SUBSET_SUM_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_sort_utils.H>

namespace Aleph {
/** @brief Result of a subset sum computation.
 *
 *  @tparam T Element type.
 */
template <typename T>
struct Subset_Sum_Result
{
  bool exists = false;            /**< Whether a valid subset was found. */
  Array<size_t> selected_indices; /**< Indices (0-based) of the elements forming the subset. */
};

namespace subset_sum_detail {
template <std::integral T>
[[nodiscard]] inline size_t to_size_checked(const T value, const char *fn_name, const char *field_name)
{
  if constexpr (std::is_signed_v<T>)
    ah_domain_error_if(value < T{}) << fn_name << ": " << field_name << " must be non-negative";

  using UT = std::make_unsigned_t<T>;
  const UT uvalue = static_cast<UT>(value);
  if constexpr (sizeof(UT) > sizeof(size_t))
    ah_out_of_range_error_if(uvalue > static_cast<UT>(std::numeric_limits<size_t>::max()))
      << fn_name << ": " << field_name << " is too large for size_t";

  return static_cast<size_t>(uvalue);
}

template <std::integral T>
[[nodiscard]] inline Array<size_t> extract_values_checked(const Array<T> &values, const char *fn_name)
{
  Array<size_t> converted = Array<size_t>::create(values.size());
  for (size_t i = 0; i < values.size(); ++i)
    converted(i) = to_size_checked(values[i], fn_name, "value");
  return converted;
}

// Enumerate all subset sums of arr[start..start+len-1]
// Returns array of pairs (sum, bitmask relative to start)
template <typename T>
Array<std::pair<long long, uint64_t>> enumerate_sums(const Array<T> &arr, size_t start, size_t len)
{
  ah_out_of_range_error_if(len >= std::numeric_limits<uint64_t>::digits)
    << "subset_sum_mitm: each half must have fewer than 64 elements";

  const uint64_t count = static_cast<uint64_t>(1) << len;
  ah_out_of_range_error_if(count > std::numeric_limits<size_t>::max())
    << "subset_sum_mitm: subset count does not fit size_t";

  Array<std::pair<long long, uint64_t>> result;
  result.reserve(static_cast<size_t>(count));

  for (uint64_t mask = 0; mask < count; ++mask)
    {
      long long s = 0;
      for (size_t j = 0; j < len; ++j)
        if (mask & (static_cast<uint64_t>(1) << j))
          s += static_cast<long long>(arr[start + j]);
      result.append(std::make_pair(s, mask));
    }

  return result;
}
}  // namespace subset_sum_detail

/** @brief Solve the subset sum problem via classical DP with reconstruction.
 *
 *  Finds if there exists a subset of `values` that sums exactly to `target`.
 *  If it exists, the indices of the elements are returned.
 *
 *  @tparam T  Integral type (must support comparison and addition).
 *
 *  @param[in] values  Array of non-negative integers.
 *  @param[in] target  Target sum to achieve.
 *  @return A Subset_Sum_Result with the existence flag and selected indices.
 *
 *  @throws ah_domain_error if target or any value is negative.
 *  @throws ah_bad_alloc if memory allocation for the DP table fails.
 *
 *  @note **Complexity**: Time O(n * target), Space O(n * target), where n is values.size().
 */
template <std::integral T>
[[nodiscard]] Subset_Sum_Result<T> subset_sum(const Array<T> &values, T target)
{
  const size_t n = values.size();
  const size_t tgt = subset_sum_detail::to_size_checked(target, "subset_sum", "target");
  const Array<size_t> weights = subset_sum_detail::extract_values_checked(values, "subset_sum");

  if (tgt == 0)
    return Subset_Sum_Result<T>{true, Array<size_t>()};

  if (n == 0)
    return Subset_Sum_Result<T>{false, Array<size_t>()};

  // dp[i][s] = can we achieve sum s using items 0..i-1?
  // Use bit-per-row for space efficiency? No, we need reconstruction.
  Array<Array<char>> dp;
  dp.reserve(n + 1);
  for (size_t i = 0; i <= n; ++i)
    {
      Array<char> row = Array<char>::create(tgt + 1);
      for (size_t s = 0; s <= tgt; ++s)
        row(s) = 0;
      dp.append(std::move(row));
    }

  dp[0][0] = 1;

  for (size_t i = 1; i <= n; ++i)
    {
      const size_t vi = weights[i - 1];
      for (size_t s = 0; s <= tgt; ++s)
        {
          dp[i][s] = dp[i - 1][s];
          if (not dp[i][s] and vi <= s and dp[i - 1][s - vi])
            dp[i][s] = 1;
        }
    }

  if (not dp[n][tgt])
    return Subset_Sum_Result<T>{false, Array<size_t>()};

  // reconstruct
  Array<size_t> sel;
  size_t s = tgt;
  for (size_t i = n; i > 0 and s > 0; --i)
    if (dp[i][s] and not dp[i - 1][s])
      {
        sel.append(i - 1);
        s -= weights[i - 1];
      }

  // reverse
  Array<size_t> selected;
  selected.reserve(sel.size());
  for (size_t k = sel.size(); k > 0; --k)
    selected.append(sel[k - 1]);

  return Subset_Sum_Result<T>{true, std::move(selected)};
}

/** @brief Check if a subset summing to target exists (space-optimized).
 *
 *  Similar to subset_sum() but only checks for existence, using O(target) space.
 *
 *  @tparam T  Integral type.
 *
 *  @param[in] values  Array of non-negative integers.
 *  @param[in] target  Target sum to achieve.
 *  @return `true` if a subset summing to target exists, `false` otherwise.
 *
 *  @throws ah_domain_error if target or any value is negative.
 *
 *  @note **Complexity**: Time O(n * target), Space O(target).
 */
template <std::integral T>
[[nodiscard]] bool subset_sum_exists(const Array<T> &values, T target)
{
  const size_t n = values.size();
  const size_t tgt = subset_sum_detail::to_size_checked(target, "subset_sum_exists", "target");
  const Array<size_t> weights
    = subset_sum_detail::extract_values_checked(values, "subset_sum_exists");

  if (tgt == 0)
    return true;
  if (n == 0)
    return false;

  Array<char> dp = Array<char>::create(tgt + 1);
  for (size_t s = 0; s <= tgt; ++s)
    dp(s) = 0;
  dp(0) = 1;

  for (size_t i = 0; i < n; ++i)
    {
      const size_t vi = weights[i];
      for (size_t s = tgt; s >= vi and s != static_cast<size_t>(-1); --s)
        if (dp[s - vi])
          dp(s) = 1;
    }

  return dp[tgt];
}

/** @brief Count the number of subsets that sum to target.
 *
 *  @tparam T  Integral type.
 *
 *  @param[in] values  Array of non-negative integers.
 *  @param[in] target  Target sum to achieve.
 *  @return The total number of distinct subsets summing to target.
 *          Capped at `std::numeric_limits<size_t>::max()`.
 *
 *  @throws ah_domain_error if target or any value is negative.
 *
 *  @note **Complexity**: Time O(n * target), Space O(target).
 */
template <std::integral T>
[[nodiscard]] size_t subset_sum_count(const Array<T> &values, T target)
{
  const size_t n = values.size();
  const size_t tgt = subset_sum_detail::to_size_checked(target, "subset_sum_count", "target");
  const Array<size_t> weights
    = subset_sum_detail::extract_values_checked(values, "subset_sum_count");

  Array<size_t> dp = Array<size_t>::create(tgt + 1);
  for (size_t s = 0; s <= tgt; ++s)
    dp(s) = 0;
  dp(0) = 1;

  for (size_t i = 0; i < n; ++i)
    {
      const size_t vi = weights[i];
      for (size_t s = tgt; s >= vi and s != static_cast<size_t>(-1); --s)
        if (const size_t count = dp[s - vi]; count > 0)
          if (dp(s) > std::numeric_limits<size_t>::max() - count)
            dp(s) = std::numeric_limits<size_t>::max();
          else
            dp(s) += count;
    }

  return dp[tgt];
}

/** @brief Solve the subset sum problem via meet-in-the-middle (MITM).
 *
 *  This algorithm is efficient when the target sum is very large, making
 *  classical DP impractical, but the number of elements is small.
 *  It splits the input into two halves, enumerates all 2^(n/2) sums for
 *  each half, and uses sorting and binary search to find complementary pairs.
 *
 *  @tparam T  Integral type.
 *
 *  @param[in] values  Array of integers.
 *  @param[in] target  Target sum to achieve.
 *  @return A Subset_Sum_Result with the existence flag and selected indices.
 *
 *  @throws ah_out_of_range_error if either half of the input would contain
 *          64 or more elements (limited by 64-bit mask representation),
 *          which effectively restricts the total size to at most 127 elements.
 *
 *  @note **Complexity**: Time O(n * 2^(n/2)), Space O(2^(n/2)).
 *  @note Recommended for n up to ~40.
 */
template <std::integral T>
[[nodiscard]] Subset_Sum_Result<T> subset_sum_mitm(const Array<T> &values, T target)
{
  const size_t n = values.size();

  if (n == 0)
    return Subset_Sum_Result<T>{target == T{}, Array<size_t>()};

  const size_t half1 = n / 2;
  const size_t half2 = n - half1;

  auto sums1 = subset_sum_detail::enumerate_sums(values, 0, half1);
  auto sums2 = subset_sum_detail::enumerate_sums(values, half1, half2);

  // sort sums2 by sum value
  introsort(sums2,
            [](const auto &a, const auto &b)
  {
    return a.first < b.first;
  });

  const auto target_ll = static_cast<long long>(target);
  for (size_t i = 0; i < sums1.size(); ++i)
    {
      const long long need = target_ll - sums1[i].first;

      // binary search in sums2 for 'need'
      size_t lo = 0, hi = sums2.size();
      while (lo < hi)
        {
          const size_t mid = lo + (hi - lo) / 2;
          if (sums2[mid].first < need)
            lo = mid + 1;
          else
            hi = mid;
        }

      if (lo < sums2.size() and sums2[lo].first == need)
        {
          // reconstruct indices
          Array<size_t> sel;
          const uint64_t mask1 = sums1[i].second;
          const uint64_t mask2 = sums2[lo].second;

          for (size_t j = 0; j < half1; ++j)
            if (mask1 & (static_cast<uint64_t>(1) << j))
              sel.append(j);

          for (size_t j = 0; j < half2; ++j)
            if (mask2 & (static_cast<uint64_t>(1) << j))
              sel.append(half1 + j);

          return Subset_Sum_Result<T>{true, std::move(sel)};
        }
    }

  return Subset_Sum_Result<T>{false, Array<size_t>()};
}
}  // namespace Aleph

#endif  // SUBSET_SUM_H
