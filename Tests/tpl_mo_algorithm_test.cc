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
 * @file tpl_mo_algorithm_test.cc
 * @brief Tests for Gen_Mo_Algorithm and built-in policies.
 */

# include <gtest/gtest.h>

# include <tpl_mo_algorithm.H>

# include <algorithm>
# include <cstddef>
# include <random>
# include <unordered_map>
# include <unordered_set>
# include <utility>
# include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  // Brute-force: count distinct elements in [l, r]
  template <typename T>
  size_t brute_distinct(const std::vector<T> & v, size_t l, size_t r)
  {
    std::unordered_set<T> s;
    for (size_t i = l; i <= r; ++i)
      s.insert(v[i]);
    return s.size();
  }

  // Brute-force: powerful array sum = sum(cnt[x]^2 * x) in [l, r]
  template <typename T>
  long long brute_powerful(const std::vector<T> & v, size_t l, size_t r)
  {
    std::unordered_map<T, long long> freq;
    for (size_t i = l; i <= r; ++i)
      ++freq[v[i]];
    long long sum = 0;
    for (auto & [val, cnt] : freq)
      sum += cnt * cnt * static_cast<long long>(val);
    return sum;
  }

  // Brute-force: range mode (max_freq, value) in [l, r]
  template <typename T>
  std::pair<size_t, T> brute_mode(const std::vector<T> & v,
                                  size_t l, size_t r)
  {
    std::unordered_map<T, size_t> freq;
    for (size_t i = l; i <= r; ++i)
      ++freq[v[i]];
    size_t best_f = 0;
    T best_v = T();
    for (auto & [val, f] : freq)
      if (f > best_f)
        {
          best_f = f;
          best_v = val;
        }
    return {best_f, best_v};
  }

  // A trivial sum policy for custom-policy testing
  struct Sum_Policy
  {
    using answer_type = long long;

    long long sum = 0;

    void init(const Array<int> &, size_t) { sum = 0; }

    void add(const Array<int> & data, size_t idx)
    {
      sum += data(idx);
    }

    void remove(const Array<int> & data, size_t idx)
    {
      sum -= data(idx);
    }

    answer_type answer() const { return sum; }
  };
} // namespace

// =================================================================
// Structural / construction tests
// =================================================================

TEST(MoAlgorithm, EmptyDataNoQueries)
{
  Distinct_Count_Mo<int> mo = {};
  EXPECT_EQ(mo.size(), 0u);
  EXPECT_TRUE(mo.is_empty());
  auto ans = mo.solve(Array<std::pair<size_t, size_t>>());
  EXPECT_EQ(ans.size(), 0u);
}

TEST(MoAlgorithm, SingleElement)
{
  Distinct_Count_Mo<int> mo = {42};
  EXPECT_EQ(mo.size(), 1u);
  auto ans = mo.solve({{0, 0}});
  EXPECT_EQ(ans(0), 1u);
}

TEST(MoAlgorithm, BoundsChecking)
{
  Distinct_Count_Mo<int> mo = {1, 2, 3};

  // r >= n
  EXPECT_THROW(mo.solve({{0, 3}}), std::out_of_range);

  // l > r (wrapping size_t isn't possible via pairs, but Mo_Query can)
  Array<Mo_Query> bad_queries = {{2, 1, 0}};
  EXPECT_THROW(mo.solve(bad_queries), std::out_of_range);
}

TEST(MoAlgorithm, ConstructorsAllContainers)
{
  // initializer_list
  Distinct_Count_Mo<int> mo1 = {1, 2, 3};
  EXPECT_EQ(mo1.size(), 3u);

  // Array<T>
  Array<int> arr = {1, 2, 3};
  Distinct_Count_Mo<int> mo2(arr);
  EXPECT_EQ(mo2.size(), 3u);

  // DynList<T>
  DynList<int> lst = {1, 2, 3};
  Distinct_Count_Mo<int> mo3(lst);
  EXPECT_EQ(mo3.size(), 3u);

  // All should give same answers
  auto a1 = mo1.solve({{0, 2}});
  auto a2 = mo2.solve({{0, 2}});
  auto a3 = mo3.solve({{0, 2}});
  EXPECT_EQ(a1(0), a2(0));
  EXPECT_EQ(a2(0), a3(0));
}

TEST(MoAlgorithm, CopyMoveSwap)
{
  Distinct_Count_Mo<int> mo1 = {1, 2, 1, 3};

  // Copy
  auto mo2 = mo1;
  EXPECT_EQ(mo2.size(), mo1.size());
  auto a1 = mo1.solve({{0, 3}});
  auto a2 = mo2.solve({{0, 3}});
  EXPECT_EQ(a1(0), a2(0));

  // Move
  auto mo3 = std::move(mo2);
  auto a3 = mo3.solve({{0, 3}});
  EXPECT_EQ(a1(0), a3(0));

  // Swap
  Distinct_Count_Mo<int> mo4 = {10, 20};
  mo3.swap(mo4);
  EXPECT_EQ(mo3.size(), 2u);
  EXPECT_EQ(mo4.size(), 4u);
}

// =================================================================
// Distinct count tests
// =================================================================

TEST(MoDistinct, BasicSmall)
{
  //                            0  1  2  3  4  5
  Distinct_Count_Mo<int> mo = {1, 2, 1, 3, 2, 1};
  auto ans = mo.solve({{0, 0}, {0, 2}, {1, 4}, {0, 5}, {3, 3}});
  EXPECT_EQ(ans(0), 1u);  // [1]       -> {1}
  EXPECT_EQ(ans(1), 2u);  // [1,2,1]   -> {1,2}
  EXPECT_EQ(ans(2), 3u);  // [2,1,3,2] -> {1,2,3}
  EXPECT_EQ(ans(3), 3u);  // [1,2,1,3,2,1] -> {1,2,3}
  EXPECT_EQ(ans(4), 1u);  // [3]       -> {3}
}

TEST(MoDistinct, AllSame)
{
  Distinct_Count_Mo<int> mo = {5, 5, 5, 5, 5};
  auto ans = mo.solve({{0, 0}, {0, 4}, {2, 3}});
  EXPECT_EQ(ans(0), 1u);
  EXPECT_EQ(ans(1), 1u);
  EXPECT_EQ(ans(2), 1u);
}

TEST(MoDistinct, AllDistinct)
{
  Distinct_Count_Mo<int> mo = {10, 20, 30, 40, 50};
  auto ans = mo.solve({{0, 4}, {1, 3}, {2, 2}});
  EXPECT_EQ(ans(0), 5u);
  EXPECT_EQ(ans(1), 3u);
  EXPECT_EQ(ans(2), 1u);
}

TEST(MoDistinct, ExhaustiveBruteForce)
{
  const size_t N = 30;
  std::mt19937 rng(42);
  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 10;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  // Generate all O(n^2) queries
  Array<std::pair<size_t, size_t>> queries;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      queries.append(std::make_pair(l, r));

  Distinct_Count_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  size_t qi = 0;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      {
        EXPECT_EQ(ans(qi), brute_distinct(vec, l, r))
            << "l=" << l << " r=" << r;
        ++qi;
      }
}

TEST(MoDistinct, StressRandom)
{
  const size_t N = 1000;
  const size_t Q = 5000;
  std::mt19937 rng(42);

  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 50;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t i = 0; i < Q; ++i)
    {
      size_t a = rng() % N, b = rng() % N;
      if (a > b) std::swap(a, b);
      queries.append(std::make_pair(a, b));
    }

  Distinct_Count_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  for (size_t i = 0; i < Q; ++i)
    {
      auto [l, r] = queries(i);
      EXPECT_EQ(ans(i), brute_distinct(vec, l, r))
          << "query " << i << ": l=" << l << " r=" << r;
    }
}

// =================================================================
// Powerful array tests
// =================================================================

TEST(MoPowerful, BasicSmall)
{
  //                            0  1  2  3  4
  Powerful_Array_Mo<int> mo = {1, 2, 1, 1, 2};

  // [0,0]: {1:1} -> 1^2*1 = 1
  // [0,2]: {1:2, 2:1} -> 4*1 + 1*2 = 6
  // [0,4]: {1:3, 2:2} -> 9*1 + 4*2 = 17
  auto ans = mo.solve({{0, 0}, {0, 2}, {0, 4}});
  EXPECT_EQ(ans(0), 1LL);
  EXPECT_EQ(ans(1), 6LL);
  EXPECT_EQ(ans(2), 17LL);
}

TEST(MoPowerful, ExhaustiveBruteForce)
{
  const size_t N = 30;
  std::mt19937 rng(123);
  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 5 + 1;  // positive values

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      queries.append(std::make_pair(l, r));

  Powerful_Array_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  size_t qi = 0;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      {
        EXPECT_EQ(ans(qi), brute_powerful(vec, l, r))
            << "l=" << l << " r=" << r;
        ++qi;
      }
}

TEST(MoPowerful, StressRandom)
{
  const size_t N = 500;
  const size_t Q = 3000;
  std::mt19937 rng(99);

  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 20 + 1;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t i = 0; i < Q; ++i)
    {
      size_t a = rng() % N, b = rng() % N;
      if (a > b) std::swap(a, b);
      queries.append(std::make_pair(a, b));
    }

  Powerful_Array_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  for (size_t i = 0; i < Q; ++i)
    {
      auto [l, r] = queries(i);
      EXPECT_EQ(ans(i), brute_powerful(vec, l, r))
          << "query " << i << ": l=" << l << " r=" << r;
    }
}

// =================================================================
// Range mode tests
// =================================================================

TEST(MoMode, BasicSmall)
{
  //                          0  1  2  3  4  5
  Range_Mode_Mo<int> mo = {3, 1, 3, 3, 1, 2};

  auto ans = mo.solve({{0, 5}, {0, 0}, {4, 5}});

  // [0,5]: 3 appears 3 times -> mode freq = 3
  EXPECT_EQ(ans(0).first, 3u);
  EXPECT_EQ(ans(0).second, 3);

  // [0,0]: single element 3 -> freq = 1
  EXPECT_EQ(ans(1).first, 1u);

  // [4,5]: {1,2} -> freq = 1 each
  EXPECT_EQ(ans(2).first, 1u);
}

TEST(MoMode, ExhaustiveBruteForce)
{
  const size_t N = 30;
  std::mt19937 rng(77);
  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 6;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      queries.append(std::make_pair(l, r));

  Range_Mode_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  size_t qi = 0;
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      {
        auto [bf, bv] = brute_mode(vec, l, r);
        // Frequency must match; value may differ on ties
        EXPECT_EQ(ans(qi).first, bf)
            << "l=" << l << " r=" << r;
        ++qi;
      }
}

TEST(MoMode, StressRandom)
{
  const size_t N = 1000;
  const size_t Q = 5000;
  std::mt19937 rng(4242);
  std::vector<int> vec(N);
  for (auto & v : vec)
    v = static_cast<int>(rng() % 200);

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t i = 0; i < Q; ++i)
    {
      size_t a = rng() % N, b = rng() % N;
      if (a > b) std::swap(a, b);
      queries.append(std::make_pair(a, b));
    }

  Range_Mode_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  for (size_t i = 0; i < Q; ++i)
    {
      auto [l, r] = queries(i);
      const auto [bf_freq, bf_val] = brute_mode(vec, l, r);
      (void) bf_val; // Value can differ when there are ties.
      EXPECT_EQ(ans(i).first, bf_freq)
          << "query " << i << ": l=" << l << " r=" << r;
    }
}

// =================================================================
// Custom policy test
// =================================================================

TEST(MoCustom, SumPolicy)
{
  Gen_Mo_Algorithm<int, Sum_Policy> mo = {3, 1, 4, 1, 5};

  auto ans = mo.solve({{0, 4}, {0, 0}, {2, 3}, {1, 2}});
  EXPECT_EQ(ans(0), 14LL);  // 3+1+4+1+5
  EXPECT_EQ(ans(1), 3LL);   // 3
  EXPECT_EQ(ans(2), 5LL);   // 4+1
  EXPECT_EQ(ans(3), 5LL);   // 1+4
}

// =================================================================
// Snake optimization correctness
// =================================================================

TEST(MoAlgorithm, SnakeOptimizationCorrectness)
{
  // Verify that the snake sort doesn't affect final answers
  // by solving the same queries with a known-correct brute force
  const size_t N = 50;
  std::mt19937 rng(2026);
  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 15;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  // Generate queries that hit different blocks
  Array<std::pair<size_t, size_t>> queries;
  for (size_t i = 0; i < 200; ++i)
    {
      size_t a = rng() % N, b = rng() % N;
      if (a > b) std::swap(a, b);
      queries.append(std::make_pair(a, b));
    }

  Distinct_Count_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  for (size_t i = 0; i < queries.size(); ++i)
    {
      auto [l, r] = queries(i);
      EXPECT_EQ(ans(i), brute_distinct(vec, l, r))
          << "query " << i;
    }
}

// =================================================================
// Large stress test
// =================================================================

TEST(MoAlgorithm, LargeStress)
{
  const size_t N = 5000;
  const size_t Q = 10000;
  std::mt19937 rng(12345);

  std::vector<int> vec(N);
  for (auto & v : vec)
    v = rng() % 100;

  auto arr = Array<int>::create(N);
  for (size_t i = 0; i < N; ++i)
    arr(i) = vec[i];

  Array<std::pair<size_t, size_t>> queries;
  for (size_t i = 0; i < Q; ++i)
    {
      size_t a = rng() % N, b = rng() % N;
      if (a > b) std::swap(a, b);
      queries.append(std::make_pair(a, b));
    }

  Distinct_Count_Mo<int> mo(arr);
  auto ans = mo.solve(queries);

  // Verify all queries against brute force
  for (size_t i = 0; i < Q; ++i)
    {
      auto [l, r] = queries(i);
      EXPECT_EQ(ans(i), brute_distinct(vec, l, r))
          << "query " << i;
    }
}
