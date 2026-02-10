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
 * @file tpl_segment_tree_test.cc
 * @brief Tests for Gen_Segment_Tree, Lazy variants, and Segment_Tree_Beats.
 */

# include <gtest/gtest.h>

# include <tpl_segment_tree.H>

# include <algorithm>
# include <cstddef>
# include <numeric>
# include <random>
# include <utility>
# include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  // Brute-force helpers
  template <typename T>
  T brute_sum(const std::vector<T> & v, size_t l, size_t r)
  {
    T s = T();
    for (size_t i = l; i <= r; ++i)
      s += v[i];
    return s;
  }

  template <typename T>
  T brute_min(const std::vector<T> & v, size_t l, size_t r)
  {
    return *std::min_element(v.begin() + l, v.begin() + r + 1);
  }

  template <typename T>
  T brute_max(const std::vector<T> & v, size_t l, size_t r)
  {
    return *std::max_element(v.begin() + l, v.begin() + r + 1);
  }

  // XOR operation for custom monoid test
  struct Xor_Op
  {
    constexpr int operator()(const int a, const int b) const noexcept
    {
      return a ^ b;
    }
  };

  // GCD operation
  struct Gcd_Op
  {
    int operator()(const int a, const int b) const noexcept
    {
      return std::gcd(a, b);
    }
  };
} // namespace

// =================================================================
// Gen_Segment_Tree tests
// =================================================================

TEST(GenSegmentTree, EmptyConstruction)
{
  Gen_Segment_Tree<int, Aleph::plus<int>> st(0, 0, 0);

  EXPECT_TRUE(st.is_empty());
  EXPECT_EQ(st.size(), 0U);
  EXPECT_THROW(st.get(0), std::out_of_range);
  EXPECT_THROW(st.query(0, 0), std::out_of_range);
}

TEST(GenSegmentTree, UniformConstruction)
{
  Gen_Segment_Tree<int, Aleph::plus<int>> st(8, 5, 0);

  EXPECT_EQ(st.size(), 8U);
  EXPECT_FALSE(st.is_empty());

  for (size_t i = 0; i < 8; ++i)
    EXPECT_EQ(st.get(i), 5);

  EXPECT_EQ(st.query(0, 7), 40);
  EXPECT_EQ(st.query(2, 5), 20);
}

TEST(GenSegmentTree, SumAgainstBruteForce)
{
  const std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
  Gen_Segment_Tree<int, Aleph::plus<int>> st(values, 0);

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      EXPECT_EQ(st.query(l, r), brute_sum(values, l, r));
}

TEST(GenSegmentTree, MinAgainstBruteForce)
{
  const std::vector<int> values = {9, 4, 7, 1, 8, 2, 6, 3, 5, 0};
  Min_Segment_Tree<int> st(values);

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      EXPECT_EQ(st.query(l, r), brute_min(values, l, r));
}

TEST(GenSegmentTree, MaxAgainstBruteForce)
{
  const std::vector<int> values = {9, 4, 7, 1, 8, 2, 6, 3, 5, 0};
  Max_Segment_Tree<int> st(values);

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      EXPECT_EQ(st.query(l, r), brute_max(values, l, r));
}

TEST(GenSegmentTree, PointUpdate)
{
  Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  st.update(2, 10);  // a[2] += 10 => {1, 2, 13, 4, 5}
  EXPECT_EQ(st.get(2), 13);
  EXPECT_EQ(st.query(0, 4), 25);
  EXPECT_EQ(st.query(1, 3), 19);
}

TEST(GenSegmentTree, SetValue)
{
  Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  st.set(2, 100);
  EXPECT_EQ(st.get(2), 100);
  EXPECT_EQ(st.query(0, 4), 112);

  st.set(0, 0);
  EXPECT_EQ(st.get(0), 0);
  EXPECT_EQ(st.query(0, 4), 111);
}

TEST(GenSegmentTree, ConstructorsAllContainers)
{
  const std::vector<int> values = {5, 3, 7, 1, 9, 2, 8, 4, 6};

  Sum_Segment_Tree<int> from_vector(values);

  Array<int> arr;
  for (const int x : values)
    arr.append(x);
  Sum_Segment_Tree<int> from_array(arr);

  DynList<int> list;
  for (const int x : values)
    list.append(x);
  Sum_Segment_Tree<int> from_list(list);

  Sum_Segment_Tree<int> from_init = {5, 3, 7, 1, 9, 2, 8, 4, 6};

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      {
        const int expected = brute_sum(values, l, r);
        EXPECT_EQ(from_vector.query(l, r), expected);
        EXPECT_EQ(from_array.query(l, r), expected);
        EXPECT_EQ(from_list.query(l, r), expected);
        EXPECT_EQ(from_init.query(l, r), expected);
      }
}

TEST(GenSegmentTree, CustomXorMonoid)
{
  const std::vector<int> values = {3, 5, 7, 2, 8};
  Gen_Segment_Tree<int, Xor_Op> st(values, 0, Xor_Op{});

  // XOR of full range
  int expected = 0;
  for (int v : values)
    expected ^= v;
  EXPECT_EQ(st.query(0, 4), expected);

  // Single element
  EXPECT_EQ(st.query(2, 2), 7);

  // Subrange [1, 3] = 5 ^ 7 ^ 2 = 0
  EXPECT_EQ(st.query(1, 3), 5 ^ 7 ^ 2);
}

TEST(GenSegmentTree, CustomGcdMonoid)
{
  const std::vector<int> values = {12, 18, 24, 36, 60};
  Gen_Segment_Tree<int, Gcd_Op> st(values, 0, Gcd_Op{});

  EXPECT_EQ(st.query(0, 1), std::gcd(12, 18));  // 6
  EXPECT_EQ(st.query(0, 4), 6);
  EXPECT_EQ(st.query(2, 4), std::gcd(std::gcd(24, 36), 60));  // 12
}

TEST(GenSegmentTree, CopyMoveSwap)
{
  Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  // Copy
  Sum_Segment_Tree<int> copy = st;
  EXPECT_EQ(copy.query(0, 4), 15);

  // Move
  Sum_Segment_Tree<int> moved = std::move(copy);
  EXPECT_EQ(moved.query(0, 4), 15);

  // Swap
  Sum_Segment_Tree<int> other = {10, 20, 30};
  moved.swap(other);

  EXPECT_EQ(moved.size(), 3U);
  EXPECT_EQ(moved.query(0, 2), 60);
  EXPECT_EQ(other.size(), 5U);
  EXPECT_EQ(other.query(0, 4), 15);
}

TEST(GenSegmentTree, BoundsChecking)
{
  Sum_Segment_Tree<int> st = {1, 2, 3};

  EXPECT_THROW(st.get(3), std::out_of_range);
  EXPECT_THROW(st.query(0, 3), std::out_of_range);
  EXPECT_THROW(st.query(2, 1), std::out_of_range);
  EXPECT_THROW(st.update(3, 1), std::out_of_range);
  EXPECT_THROW(st.set(3, 1), std::out_of_range);
}

TEST(GenSegmentTree, OneElement)
{
  Sum_Segment_Tree<int> st = {42};

  EXPECT_EQ(st.size(), 1U);
  EXPECT_EQ(st.get(0), 42);
  EXPECT_EQ(st.query(0, 0), 42);

  st.update(0, 8);
  EXPECT_EQ(st.get(0), 50);

  st.set(0, 100);
  EXPECT_EQ(st.get(0), 100);
}

TEST(GenSegmentTree, TwoElements)
{
  Min_Segment_Tree<int> st = {5, 3};

  EXPECT_EQ(st.query(0, 1), 3);
  EXPECT_EQ(st.query(0, 0), 5);
  EXPECT_EQ(st.query(1, 1), 3);

  st.set(1, 10);
  EXPECT_EQ(st.query(0, 1), 5);
}

TEST(GenSegmentTree, Values)
{
  Sum_Segment_Tree<int> st = {3, 1, 4, 1, 5};
  auto vals = st.values();

  ASSERT_EQ(vals.size(), 5U);
  EXPECT_EQ(vals(0), 3);
  EXPECT_EQ(vals(1), 1);
  EXPECT_EQ(vals(2), 4);
  EXPECT_EQ(vals(3), 1);
  EXPECT_EQ(vals(4), 5);
}

TEST(GenSegmentTree, StressRandomSumUpdates)
{
  constexpr size_t N = 1000;
  constexpr size_t OPS = 5000;

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> val_dist(-100, 100);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);

  std::vector<int> brute(N);
  for (size_t i = 0; i < N; ++i)
    brute[i] = val_dist(rng);

  Sum_Segment_Tree<int> st(brute);

  for (size_t op = 0; op < OPS; ++op)
    {
      if (rng() % 2 == 0)
        {
          // update
          size_t i = idx_dist(rng);
          int delta = val_dist(rng);
          brute[i] += delta;
          st.update(i, delta);
        }
      else
        {
          // query
          size_t a = idx_dist(rng);
          size_t b = idx_dist(rng);
          if (a > b) std::swap(a, b);
          EXPECT_EQ(st.query(a, b), brute_sum(brute, a, b));
        }
    }
}

TEST(GenSegmentTree, StressRandomMinUpdates)
{
  constexpr size_t N = 500;
  constexpr size_t OPS = 3000;

  std::mt19937 rng(123);
  std::uniform_int_distribution<int> val_dist(-1000, 1000);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);

  std::vector<int> brute(N);
  for (size_t i = 0; i < N; ++i)
    brute[i] = val_dist(rng);

  Min_Segment_Tree<int> st(brute);

  for (size_t op = 0; op < OPS; ++op)
    {
      if (rng() % 2 == 0)
        {
          // set
          size_t i = idx_dist(rng);
          int val = val_dist(rng);
          brute[i] = val;
          st.set(i, val);
        }
      else
        {
          // query
          size_t a = idx_dist(rng);
          size_t b = idx_dist(rng);
          if (a > b) std::swap(a, b);
          EXPECT_EQ(st.query(a, b), brute_min(brute, a, b));
        }
    }
}

// =================================================================
// Typedef convenience tests
// =================================================================

TEST(SegmentTreeTypedefs, SumSegmentTree)
{
  Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};
  EXPECT_EQ(st.query(0, 4), 15);
  st.update(2, 7);
  EXPECT_EQ(st.query(0, 4), 22);
}

TEST(SegmentTreeTypedefs, MinSegmentTree)
{
  Min_Segment_Tree<int> st = {5, 2, 4, 7, 1, 3};
  EXPECT_EQ(st.query(0, 5), 1);
  EXPECT_EQ(st.query(0, 3), 2);
  st.set(4, 10);
  EXPECT_EQ(st.query(0, 5), 2);
}

TEST(SegmentTreeTypedefs, MaxSegmentTree)
{
  Max_Segment_Tree<int> st = {5, 2, 4, 7, 1, 3};
  EXPECT_EQ(st.query(0, 5), 7);
  EXPECT_EQ(st.query(4, 5), 3);
  st.set(3, 0);
  EXPECT_EQ(st.query(0, 5), 5);
}

TEST(SegmentTreeTypedefs, DoubleSum)
{
  Sum_Segment_Tree<double> st = {1.5, 2.5, 3.0};
  EXPECT_DOUBLE_EQ(st.query(0, 2), 7.0);
  st.update(1, 0.5);
  EXPECT_DOUBLE_EQ(st.query(0, 2), 7.5);
}

// =================================================================
// Gen_Lazy_Segment_Tree tests
// =================================================================

TEST(LazySegmentTree, EmptyConstruction)
{
  Lazy_Sum_Segment_Tree<int> st(0);

  EXPECT_TRUE(st.is_empty());
  EXPECT_EQ(st.size(), 0U);
  EXPECT_THROW(st.query(0, 0), std::out_of_range);
}

TEST(LazySegmentTree, RangeUpdateSum)
{
  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  st.update(1, 3, 10);  // a[1..3] += 10
  EXPECT_EQ(st.query(0, 4), 45);
  EXPECT_EQ(st.get(0), 1);
  EXPECT_EQ(st.get(1), 12);
  EXPECT_EQ(st.get(2), 13);
  EXPECT_EQ(st.get(3), 14);
  EXPECT_EQ(st.get(4), 5);
}

TEST(LazySegmentTree, OverlappingRangeUpdates)
{
  Lazy_Sum_Segment_Tree<int> st(10, 0);

  st.update(0, 4, 5);   // [0..4] += 5
  st.update(3, 7, 3);   // [3..7] += 3
  st.update(6, 9, 1);   // [6..9] += 1

  // Expected: {5, 5, 5, 8, 8, 3, 4, 4, 1, 1}
  EXPECT_EQ(st.get(0), 5);
  EXPECT_EQ(st.get(3), 8);
  EXPECT_EQ(st.get(5), 3);
  EXPECT_EQ(st.get(6), 4);
  EXPECT_EQ(st.get(9), 1);
  EXPECT_EQ(st.query(0, 9), 44);
}

TEST(LazySegmentTree, PointUpdate)
{
  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  st.point_update(2, 100);
  EXPECT_EQ(st.get(2), 103);
  EXPECT_EQ(st.query(0, 4), 115);
}

TEST(LazySegmentTree, SetValue)
{
  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  st.set(2, 100);
  EXPECT_EQ(st.get(2), 100);
  EXPECT_EQ(st.query(0, 4), 112);
}

TEST(LazySegmentTree, ConstructorsAllContainers)
{
  const std::vector<int> values = {5, 3, 7, 1, 9};

  Lazy_Sum_Segment_Tree<int> from_vector(values);

  Array<int> arr;
  for (const int x : values)
    arr.append(x);
  Lazy_Sum_Segment_Tree<int> from_array(arr);

  DynList<int> list;
  for (const int x : values)
    list.append(x);
  Lazy_Sum_Segment_Tree<int> from_list(list);

  Lazy_Sum_Segment_Tree<int> from_init = {5, 3, 7, 1, 9};

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      {
        const int expected = brute_sum(values, l, r);
        EXPECT_EQ(from_vector.query(l, r), expected);
        EXPECT_EQ(from_array.query(l, r), expected);
        EXPECT_EQ(from_list.query(l, r), expected);
        EXPECT_EQ(from_init.query(l, r), expected);
      }
}

TEST(LazySegmentTree, CopyMoveSwap)
{
  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3, 4, 5};

  Lazy_Sum_Segment_Tree<int> copy = st;
  EXPECT_EQ(copy.query(0, 4), 15);

  Lazy_Sum_Segment_Tree<int> moved = std::move(copy);
  EXPECT_EQ(moved.query(0, 4), 15);

  Lazy_Sum_Segment_Tree<int> other = {10, 20, 30};
  moved.swap(other);

  EXPECT_EQ(moved.size(), 3U);
  EXPECT_EQ(moved.query(0, 2), 60);
  EXPECT_EQ(other.size(), 5U);
  EXPECT_EQ(other.query(0, 4), 15);
}

TEST(LazySegmentTree, BoundsChecking)
{
  Lazy_Sum_Segment_Tree<int> st = {1, 2, 3};

  EXPECT_THROW(st.query(0, 3), std::out_of_range);
  EXPECT_THROW(st.query(2, 1), std::out_of_range);
  EXPECT_THROW(st.update(0, 3, 1), std::out_of_range);
  EXPECT_THROW(st.set(3, 1), std::out_of_range);
}

TEST(LazySegmentTree, OneElement)
{
  Lazy_Sum_Segment_Tree<int> st = {42};

  EXPECT_EQ(st.get(0), 42);
  st.update(0, 0, 8);
  EXPECT_EQ(st.get(0), 50);
  st.set(0, 100);
  EXPECT_EQ(st.get(0), 100);
}

TEST(LazySegmentTree, StressRandom)
{
  constexpr size_t N = 200;
  constexpr size_t OPS = 2000;

  std::mt19937 rng(99);
  std::uniform_int_distribution<int> val_dist(-50, 50);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);

  std::vector<int> brute(N, 0);
  Lazy_Sum_Segment_Tree<int> st(N, 0);

  for (size_t op = 0; op < OPS; ++op)
    {
      size_t a = idx_dist(rng);
      size_t b = idx_dist(rng);
      if (a > b) std::swap(a, b);

      if (rng() % 2 == 0)
        {
          int delta = val_dist(rng);
          for (size_t i = a; i <= b; ++i)
            brute[i] += delta;
          st.update(a, b, delta);
        }
      else
        {
          EXPECT_EQ(st.query(a, b), brute_sum(brute, a, b));
        }
    }
}

// =================================================================
// Policy-specific tests
// =================================================================

TEST(LazyPolicies, AddMin)
{
  Lazy_Min_Segment_Tree<int> st = {5, 3, 7, 1, 9};

  EXPECT_EQ(st.query(0, 4), 1);
  st.update(2, 4, -5);  // {5, 3, 2, -4, 4}
  EXPECT_EQ(st.query(0, 4), -4);
  EXPECT_EQ(st.query(0, 1), 3);
}

TEST(LazyPolicies, AddMax)
{
  Lazy_Max_Segment_Tree<int> st = {5, 3, 7, 1, 9};

  EXPECT_EQ(st.query(0, 4), 9);
  st.update(0, 3, 10);  // {15, 13, 17, 11, 9}
  EXPECT_EQ(st.query(0, 4), 17);
  EXPECT_EQ(st.query(3, 4), 11);
}

TEST(LazyPolicies, AssignSum)
{
  Gen_Lazy_Segment_Tree<Assign_Sum_Policy<int>> st = {1, 2, 3, 4, 5};

  // Range assign: set [1..3] to 10
  st.update(1, 3, {true, 10});
  EXPECT_EQ(st.get(0), 1);
  EXPECT_EQ(st.get(1), 10);
  EXPECT_EQ(st.get(2), 10);
  EXPECT_EQ(st.get(3), 10);
  EXPECT_EQ(st.get(4), 5);
  EXPECT_EQ(st.query(0, 4), 36);

  // Overlapping assign
  st.update(2, 4, {true, 0});
  EXPECT_EQ(st.query(0, 4), 11);  // {1, 10, 0, 0, 0}
}

TEST(LazyPolicies, AddMinStress)
{
  constexpr size_t N = 100;
  constexpr size_t OPS = 1000;

  std::mt19937 rng(77);
  std::uniform_int_distribution<int> val_dist(-20, 20);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);

  std::vector<int> brute(N, 0);
  Lazy_Min_Segment_Tree<int> st(N, 0);

  for (size_t op = 0; op < OPS; ++op)
    {
      size_t a = idx_dist(rng);
      size_t b = idx_dist(rng);
      if (a > b) std::swap(a, b);

      if (rng() % 2 == 0)
        {
          int delta = val_dist(rng);
          for (size_t i = a; i <= b; ++i)
            brute[i] += delta;
          st.update(a, b, delta);
        }
      else
        {
          EXPECT_EQ(st.query(a, b), brute_min(brute, a, b));
        }
    }
}

// =================================================================
// Segment_Tree_Beats tests
// =================================================================

TEST(SegmentTreeBeats, EmptyConstruction)
{
  Segment_Tree_Beats<int> st(0);

  EXPECT_TRUE(st.is_empty());
  EXPECT_EQ(st.size(), 0U);
  EXPECT_THROW(st.query_sum(0, 0), std::out_of_range);
}

TEST(SegmentTreeBeats, BasicChmin)
{
  Segment_Tree_Beats<int> st = {5, 2, 4, 7, 1, 3};

  st.chmin(0, 5, 4);
  // {4, 2, 4, 4, 1, 3}
  EXPECT_EQ(st.query_max(0, 5), 4);
  EXPECT_EQ(st.query_min(0, 5), 1);
  EXPECT_EQ(st.query_sum(0, 5), 18);
}

TEST(SegmentTreeBeats, BasicChmax)
{
  Segment_Tree_Beats<int> st = {5, 2, 4, 7, 1, 3};

  st.chmax(0, 5, 4);
  // {5, 4, 4, 7, 4, 4}
  EXPECT_EQ(st.query_min(0, 5), 4);
  EXPECT_EQ(st.query_max(0, 5), 7);
  EXPECT_EQ(st.query_sum(0, 5), 28);
}

TEST(SegmentTreeBeats, InterleavedChminChmax)
{
  Segment_Tree_Beats<int> st = {10, 20, 30, 40, 50};

  st.chmin(0, 4, 35);   // {10, 20, 30, 35, 35}
  st.chmax(0, 4, 25);   // {25, 25, 30, 35, 35}

  EXPECT_EQ(st.query_sum(0, 4), 150);
  EXPECT_EQ(st.query_min(0, 4), 25);
  EXPECT_EQ(st.query_max(0, 4), 35);
}

TEST(SegmentTreeBeats, SumAfterOps)
{
  Segment_Tree_Beats<int> st = {1, 2, 3, 4, 5, 6, 7, 8};

  st.chmin(2, 5, 4);  // {1, 2, 3, 4, 4, 4, 7, 8}
  EXPECT_EQ(st.query_sum(0, 7), 33);

  st.chmax(0, 3, 3);  // {3, 3, 3, 4, 4, 4, 7, 8}
  EXPECT_EQ(st.query_sum(0, 7), 36);
}

TEST(SegmentTreeBeats, OneElement)
{
  Segment_Tree_Beats<int> st = {42};

  EXPECT_EQ(st.query_sum(0, 0), 42);
  EXPECT_EQ(st.query_min(0, 0), 42);
  EXPECT_EQ(st.query_max(0, 0), 42);

  st.chmin(0, 0, 10);
  EXPECT_EQ(st.get(0), 10);

  st.chmax(0, 0, 20);
  EXPECT_EQ(st.get(0), 20);
}

TEST(SegmentTreeBeats, CopyMoveSwap)
{
  Segment_Tree_Beats<int> st = {1, 2, 3, 4, 5};

  Segment_Tree_Beats<int> copy = st;
  EXPECT_EQ(copy.query_sum(0, 4), 15);

  Segment_Tree_Beats<int> moved = std::move(copy);
  EXPECT_EQ(moved.query_sum(0, 4), 15);

  Segment_Tree_Beats<int> other = {10, 20};
  moved.swap(other);

  EXPECT_EQ(moved.size(), 2U);
  EXPECT_EQ(moved.query_sum(0, 1), 30);
  EXPECT_EQ(other.size(), 5U);
  EXPECT_EQ(other.query_sum(0, 4), 15);
}

TEST(SegmentTreeBeats, BoundsChecking)
{
  Segment_Tree_Beats<int> st = {1, 2, 3};

  EXPECT_THROW(st.query_sum(0, 3), std::out_of_range);
  EXPECT_THROW(st.chmin(0, 3, 0), std::out_of_range);
  EXPECT_THROW(st.chmax(2, 1, 0), std::out_of_range);
}

TEST(SegmentTreeBeats, StressBruteForce)
{
  constexpr size_t N = 100;
  constexpr size_t OPS = 1000;

  std::mt19937 rng(2024);
  std::uniform_int_distribution<int> val_dist(0, 100);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  std::uniform_int_distribution<int> op_dist(0, 4);

  std::vector<int> brute(N);
  for (size_t i = 0; i < N; ++i)
    brute[i] = val_dist(rng);

  Segment_Tree_Beats<int> st(brute);

  for (size_t op = 0; op < OPS; ++op)
    {
      size_t a = idx_dist(rng);
      size_t b = idx_dist(rng);
      if (a > b) std::swap(a, b);

      switch (op_dist(rng))
        {
        case 0:
          {
            // chmin
            int v = val_dist(rng);
            for (size_t i = a; i <= b; ++i)
              brute[i] = std::min(brute[i], v);
            st.chmin(a, b, v);
            break;
          }
        case 1:
          {
            // chmax
            int v = val_dist(rng);
            for (size_t i = a; i <= b; ++i)
              brute[i] = std::max(brute[i], v);
            st.chmax(a, b, v);
            break;
          }
        case 2:
          EXPECT_EQ(st.query_sum(a, b), brute_sum(brute, a, b));
          break;
        case 3:
          EXPECT_EQ(st.query_max(a, b), brute_max(brute, a, b));
          break;
        case 4:
          EXPECT_EQ(st.query_min(a, b), brute_min(brute, a, b));
          break;
        }
    }

  // Final full verification
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(st.get(i), brute[i]);
}

TEST(SegmentTreeBeats, Values)
{
  Segment_Tree_Beats<int> st = {10, 20, 30, 40, 50};
  st.chmin(0, 4, 35);

  auto vals = st.values();
  ASSERT_EQ(vals.size(), 5U);
  EXPECT_EQ(vals(0), 10);
  EXPECT_EQ(vals(1), 20);
  EXPECT_EQ(vals(2), 30);
  EXPECT_EQ(vals(3), 35);
  EXPECT_EQ(vals(4), 35);
}
