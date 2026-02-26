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
 * @file lis_test.cc
 * @brief Tests for LIS.H.
 */

# include <gtest/gtest.h>
# include <algorithm>
# include <cstdint>
# include <random>

# include <LIS.H>

using namespace Aleph;

namespace
{
  bool is_subsequence(const Array<int> & seq, const Array<int> & sub)
  {
    size_t pos = 0;
    for (size_t i = 0; i < sub.size(); ++i)
      {
        while (pos < seq.size() and seq[pos] != sub[i])
          ++pos;
        if (pos == seq.size())
          return false;
        ++pos;
      }
    return true;
  }

  size_t lis_quadratic_length(const Array<int> & seq)
  {
    if (seq.is_empty())
      return 0;

    Array<size_t> dp = Array<size_t>::create(seq.size());
    size_t best = 1;
    for (size_t i = 0; i < seq.size(); ++i)
      {
        dp(i) = 1;
        for (size_t j = 0; j < i; ++j)
          if (seq[j] < seq[i] and dp[j] + 1 > dp[i])
            dp(i) = dp[j] + 1;
        best = std::max(best, dp[i]);
      }
    return best;
  }

  size_t lnds_quadratic_length(const Array<int> & seq)
  {
    if (seq.is_empty())
      return 0;

    Array<size_t> dp = Array<size_t>::create(seq.size());
    size_t best = 1;
    for (size_t i = 0; i < seq.size(); ++i)
      {
        dp(i) = 1;
        for (size_t j = 0; j < i; ++j)
          if (seq[j] <= seq[i] and dp[j] + 1 > dp[i])
            dp(i) = dp[j] + 1;
        best = std::max(best, dp[i]);
      }
    return best;
  }
} // namespace


TEST(LIS, EmptySequence)
{
  Array<int> empty;
  auto r = longest_increasing_subsequence(empty);
  EXPECT_EQ(r.length, 0u);
  EXPECT_EQ(r.subsequence.size(), 0u);
  EXPECT_EQ(lis_length(empty), 0u);
}

TEST(LIS, SingleElement)
{
  Array<int> single = {42};
  auto r = longest_increasing_subsequence(single);
  EXPECT_EQ(r.length, 1u);
  EXPECT_EQ(r.subsequence.size(), 1u);
  EXPECT_EQ(r.subsequence[0], 42);
}

TEST(LIS, ClassicExample)
{
  // LeetCode 300 classic: {10,9,2,5,3,7,101,18} -> LIS length 4
  Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
  auto r = longest_increasing_subsequence(seq);
  EXPECT_EQ(r.length, 4u);
  EXPECT_EQ(r.subsequence.size(), 4u);

  // Verify the subsequence is strictly increasing
  for (size_t i = 1; i < r.subsequence.size(); ++i)
    EXPECT_LT(r.subsequence[i - 1], r.subsequence[i]);

  // Verify subsequence preserves original order
  EXPECT_TRUE(is_subsequence(seq, r.subsequence));
}

TEST(LIS, AllEqual)
{
  Array<int> all_same = {5, 5, 5, 5, 5};
  auto r = longest_increasing_subsequence(all_same);
  EXPECT_EQ(r.length, 1u);
}

TEST(LIS, AlreadySorted)
{
  Array<int> sorted = {1, 2, 3, 4, 5};
  auto r = longest_increasing_subsequence(sorted);
  EXPECT_EQ(r.length, 5u);
}

TEST(LIS, ReverseSorted)
{
  Array<int> rev = {5, 4, 3, 2, 1};
  auto r = longest_increasing_subsequence(rev);
  EXPECT_EQ(r.length, 1u);
}

TEST(LIS, LengthOnly)
{
  Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
  EXPECT_EQ(lis_length(seq), 4u);
}

TEST(LIS, NonDecreasing)
{
  Array<int> seq = {3, 1, 2, 2, 3};
  auto r = longest_nondecreasing_subsequence(seq);
  // LNDS: {1, 2, 2, 3} = length 4
  EXPECT_EQ(r.length, 4u);

  // Verify non-decreasing
  for (size_t i = 1; i < r.subsequence.size(); ++i)
    EXPECT_LE(r.subsequence[i - 1], r.subsequence[i]);

  EXPECT_TRUE(is_subsequence(seq, r.subsequence));
}

TEST(LIS, NonDecreasingAllEqual)
{
  Array<int> seq = {7, 7, 7, 7, 7};
  auto r = longest_nondecreasing_subsequence(seq);
  EXPECT_EQ(r.length, seq.size());
  EXPECT_EQ(r.subsequence.size(), seq.size());
  EXPECT_TRUE(is_subsequence(seq, r.subsequence));
}

TEST(LIS, CustomComparator)
{
  // Longest decreasing subsequence via reversed comparator
  Array<int> seq = {5, 1, 4, 2, 3};
  auto r = longest_increasing_subsequence(seq, Aleph::greater<int>());
  EXPECT_EQ(r.length, 3u); // e.g. {5, 4, 2} or {5, 4, 3}

  // Verify strictly decreasing
  for (size_t i = 1; i < r.subsequence.size(); ++i)
    EXPECT_GT(r.subsequence[i - 1], r.subsequence[i]);

  EXPECT_TRUE(is_subsequence(seq, r.subsequence));
}

TEST(LIS, NonIncreasingWithComparator)
{
  Array<int> seq = {5, 5, 4, 4, 3, 2, 2, 1};
  auto r = longest_nondecreasing_subsequence(seq, Aleph::greater<int>());
  EXPECT_EQ(r.length, seq.size());
  for (size_t i = 1; i < r.subsequence.size(); ++i)
    EXPECT_GE(r.subsequence[i - 1], r.subsequence[i]);
  EXPECT_TRUE(is_subsequence(seq, r.subsequence));
}

TEST(LIS, StressVsBruteForce)
{
  // Random arrays: compare O(n log n) against O(n^2) reference DP.
  std::mt19937 rng(12345);
  for (int trial = 0; trial < 120; ++trial)
    {
      const size_t n = 20 + rng() % 60; // 20-79 elements
      Array<int> arr;
      for (size_t i = 0; i < n; ++i)
        arr.append(static_cast<int>(rng() % 200) - 100);

      const auto lis = longest_increasing_subsequence(arr);
      const size_t fast_len = lis.length;
      const size_t fast_len_only = lis_length(arr);
      const size_t ref_len = lis_quadratic_length(arr);

      EXPECT_EQ(fast_len, ref_len)
        << "Mismatch at trial " << trial;
      EXPECT_EQ(fast_len_only, ref_len)
        << "Mismatch at trial " << trial;

      EXPECT_TRUE(is_subsequence(arr, lis.subsequence));
      for (size_t i = 1; i < lis.subsequence.size(); ++i)
        EXPECT_LT(lis.subsequence[i - 1], lis.subsequence[i]);
    }
}

TEST(LIS, NonDecreasingStressVsQuadraticDP)
{
  std::mt19937 rng(98765);
  for (int trial = 0; trial < 120; ++trial)
    {
      const size_t n = 20 + rng() % 60;
      Array<int> arr;
      for (size_t i = 0; i < n; ++i)
        arr.append(static_cast<int>(rng() % 40)); // many duplicates

      const auto lnds = longest_nondecreasing_subsequence(arr);
      const size_t ref_len = lnds_quadratic_length(arr);

      EXPECT_EQ(lnds.length, ref_len)
        << "Mismatch at trial " << trial;
      EXPECT_TRUE(is_subsequence(arr, lnds.subsequence));
      for (size_t i = 1; i < lnds.subsequence.size(); ++i)
        EXPECT_LE(lnds.subsequence[i - 1], lnds.subsequence[i]);
    }
}
