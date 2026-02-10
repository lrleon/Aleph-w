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
 * @file tpl_sparse_table_test.cc
 * @brief Unit tests for Gen_Sparse_Table, Sparse_Table and Max_Sparse_Table.
 */

# include <gtest/gtest.h>

# include <tpl_sparse_table.H>

# include <algorithm>
# include <cstddef>
# include <numeric>
# include <utility>
# include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  struct Gcd_Op
  {
    int operator()(const int a, const int b) const noexcept
    {
      return std::gcd(a, b);
    }
  };

  template <typename T, class Op>
  T fold_range(const std::vector<T> & values,
               const size_t l, const size_t r, Op op)
  {
    T acc = values[l];
    for (size_t i = l + 1; i <= r; ++i)
      acc = op(acc, values[i]);
    return acc;
  }
} // namespace

TEST(SparseTable, EmptyConstructionAndErrors)
{
  Sparse_Table<int> st(std::vector<int>{});

  EXPECT_TRUE(st.is_empty());
  EXPECT_EQ(st.size(), 0U);
  EXPECT_EQ(st.num_levels(), 0U);

  EXPECT_THROW(st.get(0), std::out_of_range);
  EXPECT_THROW(st.query(0, 0), std::out_of_range);
}

TEST(SparseTable, UniformValueConstruction)
{
  Gen_Sparse_Table<int, Min_Op<int>> st(16, 7);

  EXPECT_EQ(st.size(), 16U);
  EXPECT_FALSE(st.is_empty());
  EXPECT_EQ(st.query(0, 15), 7);
  EXPECT_EQ(st.query(5, 11), 7);

  for (size_t i = 0; i < st.size(); ++i)
    EXPECT_EQ(st.get(i), 7);
}

TEST(SparseTable, MinAndMaxAgainstBruteForce)
{
  const std::vector<int> values = {9, 4, 7, 1, 8, 2, 6, 3, 5, 0};
  Sparse_Table<int> mn(values);
  Max_Sparse_Table<int> mx(values);

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      {
        const int expected_min = *std::min_element(values.begin() + l,
                                                   values.begin() + r + 1);
        const int expected_max = *std::max_element(values.begin() + l,
                                                   values.begin() + r + 1);
        EXPECT_EQ(mn.query(l, r), expected_min);
        EXPECT_EQ(mx.query(l, r), expected_max);
      }
}

TEST(SparseTable, ConstructorsFromAllContainers)
{
  const std::vector<int> values = {5, 3, 7, 1, 9, 2, 8, 4, 6};
  Sparse_Table<int> from_vector(values);

  Array<int> arr;
  for (const int x : values)
    arr.append(x);
  Sparse_Table<int> from_array(arr);

  DynList<int> list;
  for (const int x : values)
    list.append(x);
  Sparse_Table<int> from_list(list);

  Sparse_Table<int> from_init = {5, 3, 7, 1, 9, 2, 8, 4, 6};

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      {
        const int expected = *std::min_element(values.begin() + l,
                                               values.begin() + r + 1);
        EXPECT_EQ(from_vector.query(l, r), expected);
        EXPECT_EQ(from_array.query(l, r), expected);
        EXPECT_EQ(from_list.query(l, r), expected);
        EXPECT_EQ(from_init.query(l, r), expected);
      }
}

TEST(SparseTable, CustomAssociativeIdempotentOperation)
{
  const std::vector<int> values = {12, 18, 24, 36, 60, 48, 30, 90, 15, 45};
  Gen_Sparse_Table<int, Gcd_Op> st(values, Gcd_Op{});

  for (size_t l = 0; l < values.size(); ++l)
    for (size_t r = l; r < values.size(); ++r)
      EXPECT_EQ(st.query(l, r), fold_range(values, l, r, Gcd_Op{}));
}

TEST(SparseTable, ValuesCopyMoveAndSwap)
{
  const std::vector<int> base = {8, 6, 7, 5, 3, 0, 9};
  Sparse_Table<int> st(base);

  Array<int> vals = st.values();
  ASSERT_EQ(vals.size(), base.size());
  for (size_t i = 0; i < base.size(); ++i)
    EXPECT_EQ(vals(i), base[i]);

  Sparse_Table<int> copy = st;
  EXPECT_EQ(copy.query(1, 5), 0);

  Sparse_Table<int> moved = std::move(copy);
  EXPECT_EQ(moved.query(2, 6), 0);

  Sparse_Table<int> other = {100, 50, 75};
  moved.swap(other);

  EXPECT_EQ(moved.size(), 3U);
  EXPECT_EQ(moved.query(0, 2), 50);
  EXPECT_EQ(other.size(), base.size());
  EXPECT_EQ(other.query(0, 6), 0);
}

TEST(SparseTable, BoundsChecking)
{
  Sparse_Table<int> st = {1, 2, 3};

  EXPECT_THROW(st.get(3), std::out_of_range);
  EXPECT_THROW(st.query(0, 3), std::out_of_range);
  EXPECT_THROW(st.query(2, 1), std::out_of_range);
}
