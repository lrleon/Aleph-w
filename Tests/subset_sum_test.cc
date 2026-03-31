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
 * @file subset_sum_test.cc
 * @brief Tests for Subset_Sum.H.
 */

# include <cstdint>
# include <random>
# include <bitArray.H>

# include <gtest/gtest.h>

# include <Subset_Sum.H>

using namespace Aleph;

namespace
{
  size_t brute_subset_count(const Array<int> & vals, const int target)
  {
    const size_t n = vals.size();
    const uint64_t total_masks = static_cast<uint64_t>(1) << n;
    size_t count = 0;
    for (uint64_t mask = 0; mask < total_masks; ++mask)
      {
        int sum = 0;
        for (size_t i = 0; i < n; ++i)
          if (mask & (uint64_t(1) << i))
            sum += vals[i];
        if (sum == target)
          ++count;
      }
    return count;
  }

  bool valid_selection(const Array<int> & vals,
                       const Array<size_t> & selected,
                       const int target)
  {
    int sum = 0;
    BitArray seen(vals.size());
    for (size_t idx : selected)
      {
        if (idx >= vals.size() or seen[idx])
          return false;
        seen[idx] = true;
        sum += vals[idx];
      }
    return sum == target;
  }
} // namespace


TEST(SubsetSum, EmptyArray)
{
  const Array<int> vals;
  auto r = subset_sum(vals, 0);
  EXPECT_TRUE(r.exists);
  EXPECT_EQ(r.selected_indices.size(), 0u);

  r = subset_sum(vals, 5);
  EXPECT_FALSE(r.exists);
}

TEST(SubsetSum, ZeroTarget)
{
  Array<int> vals = {1, 2, 3};
  auto r = subset_sum(vals, 0);
  EXPECT_TRUE(r.exists);
  EXPECT_EQ(r.selected_indices.size(), 0u);
}

TEST(SubsetSum, BasicFound)
{
  Array<int> vals = {3, 34, 4, 12, 5, 2};
  auto r = subset_sum(vals, 9);
  EXPECT_TRUE(r.exists);

  // Verify selected indices sum to target
  int total = 0;
  for (size_t selected_indice : r.selected_indices)
    total += vals[selected_indice];
  EXPECT_EQ(total, 9);
}

TEST(SubsetSum, NotFound)
{
  Array<int> vals = {3, 34, 4, 12, 5, 2};
  auto r = subset_sum(vals, 100);
  EXPECT_FALSE(r.exists);
}

TEST(SubsetSum, ExistsOnly)
{
  Array<int> vals = {3, 34, 4, 12, 5, 2};
  EXPECT_TRUE(subset_sum_exists(vals, 9));
  EXPECT_FALSE(subset_sum_exists(vals, 100));
}

TEST(SubsetSum, CountSubsets)
{
  Array<int> vals = {1, 2, 3, 4, 5};
  // Subsets summing to 5: {5}, {2,3}, {1,4} = 3
  EXPECT_EQ(subset_sum_count(vals, 5), 3u);
}

TEST(SubsetSum, CountZeroTarget)
{
  Array<int> vals = {1, 2, 3};
  // Only the empty subset sums to 0
  EXPECT_EQ(subset_sum_count(vals, 0), 1u);
}

TEST(SubsetSum, NegativeTarget)
{
  Array<int> vals = {1, 2, 3};
  EXPECT_THROW(subset_sum(vals, -1), std::domain_error);
  EXPECT_THROW(subset_sum_exists(vals, -1), std::domain_error);
  EXPECT_THROW(subset_sum_count(vals, -1), std::domain_error);
}

TEST(SubsetSum, NegativeValuesRejected)
{
  Array<int> vals = {5, -2, 7};
  EXPECT_THROW(subset_sum(vals, 10), std::domain_error);
  EXPECT_THROW(subset_sum_exists(vals, 10), std::domain_error);
  EXPECT_THROW(subset_sum_count(vals, 10), std::domain_error);
}

TEST(SubsetSum, ZeroValueMultiplicityInCount)
{
  Array<int> vals = {0, 0, 1};
  // Subsets summing to 1: {1}, {0,1}, {0,1}, {0,0,1} -> 4
  EXPECT_EQ(subset_sum_count(vals, 1), 4u);
  auto r = subset_sum(vals, 1);
  EXPECT_TRUE(r.exists);
  EXPECT_TRUE(valid_selection(vals, r.selected_indices, 1));
}


// Meet-in-the-middle tests

TEST(SubsetSumMITM, EmptyArray)
{
  Array<int> vals;
  auto r = subset_sum_mitm(vals, 0);
  EXPECT_TRUE(r.exists);

  r = subset_sum_mitm(vals, 5);
  EXPECT_FALSE(r.exists);
}

TEST(SubsetSumMITM, BasicFound)
{
  Array<int> vals = {3, 34, 4, 12, 5, 2};
  auto r = subset_sum_mitm(vals, 9);
  EXPECT_TRUE(r.exists);

  int total = 0;
  for (size_t k = 0; k < r.selected_indices.size(); ++k)
    total += vals[r.selected_indices[k]];
  EXPECT_EQ(total, 9);
}

TEST(SubsetSumMITM, NotFound)
{
  Array<int> vals = {3, 34, 4, 12, 5, 2};
  auto r = subset_sum_mitm(vals, 100);
  EXPECT_FALSE(r.exists);
}

TEST(SubsetSumMITM, LargerSet)
{
  // 20 elements to test MITM properly (too many for brute force 2^20 but ok)
  Array<int> vals;
  for (int i = 1; i <= 20; ++i)
    vals.append(i * 3);

  // Sum of all = 3*(1+...+20) = 3*210 = 630
  auto r = subset_sum_mitm(vals, 630);
  EXPECT_TRUE(r.exists);

  // Partial sum
  auto r2 = subset_sum_mitm(vals, 30); // 3+27=30, or 9+21=30, etc.
  EXPECT_TRUE(r2.exists);

  int total = 0;
  for (size_t k = 0; k < r2.selected_indices.size(); ++k)
    total += vals[r2.selected_indices[k]];
  EXPECT_EQ(total, 30);
}

TEST(SubsetSumMITM, SingleElement)
{
  Array<int> vals = {42};
  auto r = subset_sum_mitm(vals, 42);
  EXPECT_TRUE(r.exists);
  EXPECT_EQ(r.selected_indices.size(), 1u);
  EXPECT_EQ(r.selected_indices[0], 0u);

  r = subset_sum_mitm(vals, 43);
  EXPECT_FALSE(r.exists);
}

TEST(SubsetSum, StressVsBruteForce)
{
  // Compare DP vs MITM on small inputs
  Array<int> vals = {2, 3, 7, 8, 10};

  for (int target = 0; target <= 30; ++target)
    {
      bool dp_ans = subset_sum_exists(vals, target);

      auto mitm_r = subset_sum_mitm(vals, target);
      EXPECT_EQ(dp_ans, mitm_r.exists)
        << "Disagreement at target=" << target;
    }
}

TEST(SubsetSum, RandomExistsAndCountVsBruteForce)
{
  std::mt19937 rng(4242);
  for (int trial = 0; trial < 100; ++trial)
    {
      const size_t n = 1 + rng() % 18; // brute-force friendly
      Array<int> vals;
      vals.reserve(n);
      for (size_t i = 0; i < n; ++i)
        vals.append(static_cast<int>(rng() % 11)); // non-negative

      const int target = static_cast<int>(rng() % 45);

      const bool exists = subset_sum_exists(vals, target);
      const size_t count = subset_sum_count(vals, target);
      const size_t brute_count = brute_subset_count(vals, target);

      EXPECT_EQ(count, brute_count);
      EXPECT_EQ(exists, brute_count > 0);

      const auto r = subset_sum(vals, target);
      EXPECT_EQ(r.exists, brute_count > 0);
      if (r.exists)
        EXPECT_TRUE(valid_selection(vals, r.selected_indices, target));
    }
}

TEST(SubsetSumMITM, RandomVsBruteForce)
{
  std::mt19937 rng(2024);
  for (int trial = 0; trial < 80; ++trial)
    {
      const size_t n = 1 + rng() % 22; // keep brute-force feasible
      Array<int> vals;
      vals.reserve(n);
      for (size_t i = 0; i < n; ++i)
        vals.append(static_cast<int>(rng() % 41) - 20); // allow negatives

      const int target = static_cast<int>(rng() % 61) - 30;

      const auto mitm = subset_sum_mitm(vals, target);
      const bool brute_exists = brute_subset_count(vals, target) > 0;
      EXPECT_EQ(mitm.exists, brute_exists);
      if (mitm.exists)
        EXPECT_TRUE(valid_selection(vals, mitm.selected_indices, target));
    }
}

TEST(SubsetSumMITM, TooManyElementsThrows)
{
  Array<int> vals;
  for (int i = 0; i < 128; ++i)
    vals.append(1);
  EXPECT_THROW(subset_sum_mitm(vals, 10), std::out_of_range);
}
// satisfy CI policy
