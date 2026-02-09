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
 * @file fenwick_tree.cc
 * @brief Tests for Gen_Fenwick_Tree, Fenwick_Tree, and Range_Fenwick_Tree
 */

# include <gtest/gtest.h>

# include <tpl_fenwick_tree.H>

# include <random>
# include <vector>

using namespace Aleph;
using namespace testing;

// ---------------------------------------------------------------
// Gen_Fenwick_Tree — basic tests (arithmetic defaults)
// ---------------------------------------------------------------

TEST(GenFenwickTree, EmptyConstruction)
{
  Gen_Fenwick_Tree<int> ft(0);
  EXPECT_TRUE(ft.is_empty());
  EXPECT_EQ(ft.size(), 0U);
}

TEST(GenFenwickTree, ZeroInitialized)
{
  Gen_Fenwick_Tree<int> ft(5);
  EXPECT_EQ(ft.size(), 5U);
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(ft.get(i), 0);
}

TEST(GenFenwickTree, InitializerListConstruction)
{
  Gen_Fenwick_Tree<int> ft = {3, 1, 4, 1, 5, 9};

  // prefix sums: 3, 4, 8, 9, 14, 23
  EXPECT_EQ(ft.prefix(0), 3);
  EXPECT_EQ(ft.prefix(1), 4);
  EXPECT_EQ(ft.prefix(2), 8);
  EXPECT_EQ(ft.prefix(3), 9);
  EXPECT_EQ(ft.prefix(4), 14);
  EXPECT_EQ(ft.prefix(5), 23);
}

TEST(GenFenwickTree, ArrayConstruction)
{
  Array<long> a = {10, 20, 30, 40, 50};
  Gen_Fenwick_Tree<long> ft(a);

  EXPECT_EQ(ft.prefix(0), 10);
  EXPECT_EQ(ft.prefix(2), 60);
  EXPECT_EQ(ft.prefix(4), 150);
}

TEST(GenFenwickTree, VectorConstruction)
{
  std::vector<int> values = {2, 7, 1, 8, 2, 8};
  Gen_Fenwick_Tree<int> ft(values);

  EXPECT_EQ(ft.size(), values.size());
  EXPECT_EQ(ft.get(0), 2);
  EXPECT_EQ(ft.get(3), 8);
  EXPECT_EQ(ft.prefix(4), 20);   // 2+7+1+8+2
  EXPECT_EQ(ft.query(1, 4), 18); // 7+1+8+2
}

TEST(GenFenwickTree, DynListConstruction)
{
  DynList<int> values;
  values.append(4);
  values.append(6);
  values.append(1);
  values.append(3);

  Gen_Fenwick_Tree<int> ft(values);

  EXPECT_EQ(ft.size(), 4U);
  EXPECT_EQ(ft.get(2), 1);
  EXPECT_EQ(ft.prefix(3), 14);  // 4+6+1+3
  EXPECT_EQ(ft.query(1, 3), 10); // 6+1+3
}

TEST(GenFenwickTree, PointUpdate)
{
  Gen_Fenwick_Tree<int> ft(4);
  ft.update(0, 5);
  ft.update(1, 3);
  ft.update(2, 7);
  ft.update(3, 2);

  EXPECT_EQ(ft.get(0), 5);
  EXPECT_EQ(ft.get(1), 3);
  EXPECT_EQ(ft.get(2), 7);
  EXPECT_EQ(ft.get(3), 2);

  EXPECT_EQ(ft.prefix(3), 17);
}

TEST(GenFenwickTree, RangeQuery)
{
  Gen_Fenwick_Tree<int> ft = {1, 2, 3, 4, 5, 6, 7, 8};

  EXPECT_EQ(ft.query(0, 7), 36);
  EXPECT_EQ(ft.query(0, 0), 1);
  EXPECT_EQ(ft.query(2, 5), 18); // 3+4+5+6
  EXPECT_EQ(ft.query(3, 3), 4);
  EXPECT_EQ(ft.query(4, 7), 26); // 5+6+7+8
}

TEST(GenFenwickTree, SetValue)
{
  Gen_Fenwick_Tree<int> ft = {10, 20, 30};

  ft.set(1, 50);

  EXPECT_EQ(ft.get(0), 10);
  EXPECT_EQ(ft.get(1), 50);
  EXPECT_EQ(ft.get(2), 30);
  EXPECT_EQ(ft.prefix(2), 90);
}

TEST(GenFenwickTree, Values)
{
  Gen_Fenwick_Tree<int> ft = {5, 3, 8, 1};
  Array<int> vals = ft.values();

  EXPECT_EQ(vals.size(), 4U);
  EXPECT_EQ(vals(0), 5);
  EXPECT_EQ(vals(1), 3);
  EXPECT_EQ(vals(2), 8);
  EXPECT_EQ(vals(3), 1);
}

TEST(GenFenwickTree, CopyAndMove)
{
  Gen_Fenwick_Tree<int> ft = {1, 2, 3, 4};

  // copy
  Gen_Fenwick_Tree<int> ft2 = ft;
  EXPECT_EQ(ft2.prefix(3), 10);

  // modify copy without affecting original
  ft2.update(0, 100);
  EXPECT_EQ(ft.prefix(3), 10);
  EXPECT_EQ(ft2.prefix(3), 110);

  // move
  Gen_Fenwick_Tree<int> ft3 = std::move(ft2);
  EXPECT_EQ(ft3.prefix(3), 110);
}

TEST(GenFenwickTree, Swap)
{
  Gen_Fenwick_Tree<int> a = {1, 2, 3};
  Gen_Fenwick_Tree<int> b = {10, 20};

  a.swap(b);

  EXPECT_EQ(a.size(), 2U);
  EXPECT_EQ(a.prefix(1), 30);
  EXPECT_EQ(b.size(), 3U);
  EXPECT_EQ(b.prefix(2), 6);
}

TEST(GenFenwickTree, BoundsChecking)
{
  Gen_Fenwick_Tree<int> ft(3);
  EXPECT_THROW(ft.update(3, 1), std::out_of_range);
  EXPECT_THROW(ft.prefix(3),    std::out_of_range);
  EXPECT_THROW(ft.query(2, 3),  std::out_of_range);
  EXPECT_THROW(ft.get(5),       std::out_of_range);
  EXPECT_THROW(ft.set(5, 0),    std::out_of_range);
}

// ---------------------------------------------------------------
// Gen_Fenwick_Tree — XOR group (tests true genericity)
// ---------------------------------------------------------------

namespace
{
  struct Xor_Op
  {
    int operator()(int a, int b) const noexcept { return a ^ b; }
  };
} // namespace

TEST(GenFenwickTree, XorGroup)
{
  // XOR is its own inverse
  Gen_Fenwick_Tree<int, Xor_Op, Xor_Op> ft(5, Xor_Op{}, Xor_Op{});

  ft.update(0, 0b1010);
  ft.update(1, 0b1100);
  ft.update(2, 0b0110);

  // prefix XOR: a[0] = 1010, a[0]^a[1] = 0110, a[0]^a[1]^a[2] = 0000
  EXPECT_EQ(ft.prefix(0), 0b1010);
  EXPECT_EQ(ft.prefix(1), 0b0110);
  EXPECT_EQ(ft.prefix(2), 0b0000);

  // range: a[1]^a[2] = 1100 ^ 0110 = 1010
  EXPECT_EQ(ft.query(1, 2), 0b1010);

  // get recovers original values
  EXPECT_EQ(ft.get(0), 0b1010);
  EXPECT_EQ(ft.get(1), 0b1100);
  EXPECT_EQ(ft.get(2), 0b0110);
}

// ---------------------------------------------------------------
// Gen_Fenwick_Tree — stress test against naive prefix sums
// ---------------------------------------------------------------

TEST(GenFenwickTree, StressRandomUpdates)
{
  const size_t N = 1000;
  const int num_ops = 5000;

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  std::uniform_int_distribution<int> val_dist(-1000, 1000);

  Gen_Fenwick_Tree<long long> ft(N);
  std::vector<long long> naive(N, 0);

  for (int op = 0; op < num_ops; ++op)
    {
      size_t i = idx_dist(rng);
      int delta = val_dist(rng);
      ft.update(i, delta);
      naive[i] += delta;
    }

  // verify all prefix sums
  long long expected = 0;
  for (size_t i = 0; i < N; ++i)
    {
      expected += naive[i];
      EXPECT_EQ(ft.prefix(i), expected) << "mismatch at prefix(" << i << ")";
    }

  // verify individual values
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(ft.get(i), naive[i]) << "mismatch at get(" << i << ")";
}

// ---------------------------------------------------------------
// Fenwick_Tree<T> — arithmetic specialisation
// ---------------------------------------------------------------

TEST(FenwickTree, InheritsBaseOperations)
{
  Fenwick_Tree<int> ft = {5, 3, 8, 1, 7};

  EXPECT_EQ(ft.size(), 5U);
  EXPECT_EQ(ft.prefix(0), 5);
  EXPECT_EQ(ft.prefix(4), 24);
  EXPECT_EQ(ft.query(1, 3), 12); // 3+8+1
}

// ---------------------------------------------------------------
// find_kth
// ---------------------------------------------------------------

TEST(FenwickTree, FindKthBasic)
{
  // values: [3, 1, 2, 4]
  // prefix: [3, 4, 6, 10]
  Fenwick_Tree<int> ft = {3, 1, 2, 4};

  EXPECT_EQ(ft.find_kth(1), 0U);  // prefix(0)=3 >= 1
  EXPECT_EQ(ft.find_kth(3), 0U);  // prefix(0)=3 >= 3
  EXPECT_EQ(ft.find_kth(4), 1U);  // prefix(1)=4 >= 4
  EXPECT_EQ(ft.find_kth(5), 2U);  // prefix(2)=6 >= 5
  EXPECT_EQ(ft.find_kth(6), 2U);  // prefix(2)=6 >= 6
  EXPECT_EQ(ft.find_kth(7), 3U);  // prefix(3)=10 >= 7
  EXPECT_EQ(ft.find_kth(10), 3U); // prefix(3)=10 >= 10
  EXPECT_EQ(ft.find_kth(11), 4U); // total=10 < 11, returns size()
}

TEST(FenwickTree, FindKthSingleElement)
{
  Fenwick_Tree<int> ft = {5};

  EXPECT_EQ(ft.find_kth(1), 0U);
  EXPECT_EQ(ft.find_kth(5), 0U);
  EXPECT_EQ(ft.find_kth(6), 1U);
}

TEST(FenwickTree, FindKthOrderStatistics)
{
  // Simulate an order-statistic set with values 0..9
  // Insert values {2, 5, 7} into the frequency array
  Fenwick_Tree<int> ft(10);
  ft.update(2, 1);
  ft.update(5, 1);
  ft.update(7, 1);

  // find_kth(1) → 1st smallest = 2
  EXPECT_EQ(ft.find_kth(1), 2U);
  // find_kth(2) → 2nd smallest = 5
  EXPECT_EQ(ft.find_kth(2), 5U);
  // find_kth(3) → 3rd smallest = 7
  EXPECT_EQ(ft.find_kth(3), 7U);
  // find_kth(4) → doesn't exist
  EXPECT_EQ(ft.find_kth(4), 10U);
}

TEST(FenwickTree, FindKthEmpty)
{
  Fenwick_Tree<int> ft(0);
  EXPECT_EQ(ft.find_kth(1), 0U);
}

TEST(FenwickTree, FindKthPowerOfTwo)
{
  // n = 8 (power of two — edge case for bit_floor)
  Fenwick_Tree<int> ft = {1, 1, 1, 1, 1, 1, 1, 1};

  for (int k = 1; k <= 8; ++k)
    EXPECT_EQ(ft.find_kth(k), static_cast<size_t>(k - 1));

  EXPECT_EQ(ft.find_kth(9), 8U);
}

TEST(FenwickTree, FindKthWithZeros)
{
  // values: [0, 0, 5, 0, 3]
  // prefix: [0, 0, 5, 5, 8]
  Fenwick_Tree<int> ft = {0, 0, 5, 0, 3};

  EXPECT_EQ(ft.find_kth(1), 2U);  // first non-zero prefix at index 2
  EXPECT_EQ(ft.find_kth(5), 2U);
  EXPECT_EQ(ft.find_kth(6), 4U);
  EXPECT_EQ(ft.find_kth(8), 4U);
}

// ---------------------------------------------------------------
// Fenwick_Tree — stress find_kth against linear scan
// ---------------------------------------------------------------

TEST(FenwickTree, StressFindKth)
{
  const size_t N = 512;
  std::mt19937 rng(123);
  std::uniform_int_distribution<int> val_dist(0, 10);

  Fenwick_Tree<int> ft(N);
  std::vector<int> naive(N, 0);

  for (size_t i = 0; i < N; ++i)
    {
      int v = val_dist(rng);
      ft.update(i, v);
      naive[i] = v;
    }

  // build prefix sums for reference
  std::vector<long long> pfx(N);
  pfx[0] = naive[0];
  for (size_t i = 1; i < N; ++i)
    pfx[i] = pfx[i - 1] + naive[i];

  long long total = pfx[N - 1];

  for (int k = 1; k <= static_cast<int>(total); k += std::max(1, static_cast<int>(total / 200)))
    {
      size_t expected = N;
      for (size_t i = 0; i < N; ++i)
        if (pfx[i] >= k)
          {
            expected = i;
            break;
          }

      EXPECT_EQ(ft.find_kth(k), expected)
        << "find_kth(" << k << ") mismatch";
    }
}

// ---------------------------------------------------------------
// Double type
// ---------------------------------------------------------------

TEST(FenwickTree, DoubleType)
{
  Fenwick_Tree<double> ft = {1.5, 2.5, 3.0};

  EXPECT_DOUBLE_EQ(ft.prefix(0), 1.5);
  EXPECT_DOUBLE_EQ(ft.prefix(1), 4.0);
  EXPECT_DOUBLE_EQ(ft.prefix(2), 7.0);
  EXPECT_DOUBLE_EQ(ft.query(1, 2), 5.5);
}

// ---------------------------------------------------------------
// Range_Fenwick_Tree — range update + range query
// ---------------------------------------------------------------

TEST(RangeFenwickTree, EmptyConstruction)
{
  Range_Fenwick_Tree<int> ft(0);
  EXPECT_TRUE(ft.is_empty());
  EXPECT_EQ(ft.size(), 0U);
}

TEST(RangeFenwickTree, ZeroInitialized)
{
  Range_Fenwick_Tree<int> ft(5);
  EXPECT_EQ(ft.size(), 5U);
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(ft.get(i), 0);
}

TEST(RangeFenwickTree, InitializerListConstruction)
{
  Range_Fenwick_Tree<int> ft = {3, 1, 4, 1, 5, 9};

  EXPECT_EQ(ft.get(0), 3);
  EXPECT_EQ(ft.get(1), 1);
  EXPECT_EQ(ft.get(2), 4);
  EXPECT_EQ(ft.get(3), 1);
  EXPECT_EQ(ft.get(4), 5);
  EXPECT_EQ(ft.get(5), 9);

  EXPECT_EQ(ft.prefix(0), 3);
  EXPECT_EQ(ft.prefix(2), 8);
  EXPECT_EQ(ft.prefix(5), 23);
}

TEST(RangeFenwickTree, ArrayConstruction)
{
  Array<long> a = {10, 20, 30, 40, 50};
  Range_Fenwick_Tree<long> ft(a);

  EXPECT_EQ(ft.prefix(0), 10);
  EXPECT_EQ(ft.prefix(2), 60);
  EXPECT_EQ(ft.prefix(4), 150);
  EXPECT_EQ(ft.query(1, 3), 90); // 20+30+40
}

TEST(RangeFenwickTree, RangeUpdate)
{
  Range_Fenwick_Tree<int> ft(6);

  ft.update(1, 4, 3); // a[1..4] += 3  → [0, 3, 3, 3, 3, 0]

  EXPECT_EQ(ft.get(0), 0);
  EXPECT_EQ(ft.get(1), 3);
  EXPECT_EQ(ft.get(2), 3);
  EXPECT_EQ(ft.get(3), 3);
  EXPECT_EQ(ft.get(4), 3);
  EXPECT_EQ(ft.get(5), 0);
}

TEST(RangeFenwickTree, OverlappingRangeUpdates)
{
  Range_Fenwick_Tree<int> ft(8);

  ft.update(1, 4, 3);  // [0, 3, 3, 3, 3, 0, 0, 0]
  ft.update(2, 6, 5);  // [0, 3, 8, 8, 8, 5, 5, 0]

  EXPECT_EQ(ft.get(0), 0);
  EXPECT_EQ(ft.get(1), 3);
  EXPECT_EQ(ft.get(2), 8);
  EXPECT_EQ(ft.get(3), 8);
  EXPECT_EQ(ft.get(4), 8);
  EXPECT_EQ(ft.get(5), 5);
  EXPECT_EQ(ft.get(6), 5);
  EXPECT_EQ(ft.get(7), 0);

  EXPECT_EQ(ft.query(1, 6), 37); // 3+8+8+8+5+5
}

TEST(RangeFenwickTree, PointUpdate)
{
  Range_Fenwick_Tree<int> ft(4);

  ft.point_update(0, 5);
  ft.point_update(1, 3);
  ft.point_update(2, 7);
  ft.point_update(3, 2);

  EXPECT_EQ(ft.get(0), 5);
  EXPECT_EQ(ft.get(1), 3);
  EXPECT_EQ(ft.get(2), 7);
  EXPECT_EQ(ft.get(3), 2);
  EXPECT_EQ(ft.prefix(3), 17);
}

TEST(RangeFenwickTree, SetValue)
{
  Range_Fenwick_Tree<int> ft = {10, 20, 30};

  ft.set(1, 50);

  EXPECT_EQ(ft.get(0), 10);
  EXPECT_EQ(ft.get(1), 50);
  EXPECT_EQ(ft.get(2), 30);
  EXPECT_EQ(ft.prefix(2), 90);
}

TEST(RangeFenwickTree, UpdateLastElement)
{
  // Edge case: update touching the last element (r+1 == n)
  Range_Fenwick_Tree<int> ft(4);

  ft.update(2, 3, 10); // [0, 0, 10, 10]

  EXPECT_EQ(ft.get(0), 0);
  EXPECT_EQ(ft.get(1), 0);
  EXPECT_EQ(ft.get(2), 10);
  EXPECT_EQ(ft.get(3), 10);
  EXPECT_EQ(ft.prefix(3), 20);
}

TEST(RangeFenwickTree, UpdateEntireRange)
{
  Range_Fenwick_Tree<int> ft(5);

  ft.update(0, 4, 7); // [7, 7, 7, 7, 7]

  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(ft.get(i), 7);

  EXPECT_EQ(ft.prefix(4), 35);
}

TEST(RangeFenwickTree, Values)
{
  Range_Fenwick_Tree<int> ft = {5, 3, 8, 1};
  ft.update(1, 2, 10); // [5, 13, 18, 1]

  Array<int> vals = ft.values();
  EXPECT_EQ(vals.size(), 4U);
  EXPECT_EQ(vals(0), 5);
  EXPECT_EQ(vals(1), 13);
  EXPECT_EQ(vals(2), 18);
  EXPECT_EQ(vals(3), 1);
}

TEST(RangeFenwickTree, CopyAndMove)
{
  Range_Fenwick_Tree<int> ft = {1, 2, 3, 4};

  Range_Fenwick_Tree<int> ft2 = ft;
  EXPECT_EQ(ft2.prefix(3), 10);

  ft2.update(0, 3, 100);
  EXPECT_EQ(ft.prefix(3), 10);
  EXPECT_EQ(ft2.prefix(3), 410);

  Range_Fenwick_Tree<int> ft3 = std::move(ft2);
  EXPECT_EQ(ft3.prefix(3), 410);
}

TEST(RangeFenwickTree, BoundsChecking)
{
  Range_Fenwick_Tree<int> ft(3);
  EXPECT_THROW(ft.update(0, 3, 1), std::out_of_range);
  EXPECT_THROW(ft.update(2, 1, 1), std::out_of_range);
  EXPECT_THROW(ft.prefix(3),       std::out_of_range);
  EXPECT_THROW(ft.query(2, 3),     std::out_of_range);
  EXPECT_THROW(ft.get(5),          std::out_of_range);
}

// ---------------------------------------------------------------
// Range_Fenwick_Tree — stress test against naive array
// ---------------------------------------------------------------

TEST(RangeFenwickTree, StressRandomUpdates)
{
  const size_t N = 200;
  const int num_ops = 2000;

  std::mt19937 rng(77);
  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  std::uniform_int_distribution<int> val_dist(-100, 100);

  Range_Fenwick_Tree<long long> ft(N);
  std::vector<long long> naive(N, 0);

  for (int op = 0; op < num_ops; ++op)
    {
      size_t a = idx_dist(rng);
      size_t b = idx_dist(rng);
      if (a > b)
        std::swap(a, b);
      int delta = val_dist(rng);

      ft.update(a, b, delta);
      for (size_t i = a; i <= b; ++i)
        naive[i] += delta;
    }

  // verify all prefix sums
  long long expected = 0;
  for (size_t i = 0; i < N; ++i)
    {
      expected += naive[i];
      EXPECT_EQ(ft.prefix(i), expected)
        << "mismatch at prefix(" << i << ")";
    }

  // verify individual values
  for (size_t i = 0; i < N; ++i)
    EXPECT_EQ(ft.get(i), naive[i])
      << "mismatch at get(" << i << ")";

  // verify range queries
  for (int q = 0; q < 500; ++q)
    {
      size_t a = idx_dist(rng);
      size_t b = idx_dist(rng);
      if (a > b)
        std::swap(a, b);

      long long naive_sum = 0;
      for (size_t i = a; i <= b; ++i)
        naive_sum += naive[i];

      EXPECT_EQ(ft.query(a, b), naive_sum)
        << "mismatch at query(" << a << ", " << b << ")";
    }
}
