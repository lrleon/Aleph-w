
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
 * @file hash_chain_stats_test.cc
 * @brief Tests for HashStats::stats() on separate-chaining hash tables.
 *
 * The chain-length histogram used to miss every length recorded inside an
 * already allocated block, so the average was wrong (zero for a table with
 * entries). On a linear hash table, whose buckets live in a lazy DynArray,
 * stats() also threw when a bucket had never been written, for instance on
 * an empty table.
 */
#include <gtest/gtest.h>

#include <cstddef>

#include <tpl_dynSetHash.H>

using namespace Aleph;

namespace
{
  /// Sum of lens(i): the number of chains, which must equal capacity().
  template <class Stats>
  size_t num_chains(const Stats & st)
  {
    size_t count = 0;
    for (size_t i = 0; i < st.lens.size(); ++i)
      count += st.lens(i);
    return count;
  }

  /// Sum of i * lens(i): the entries stored in all chains, i.e. size().
  template <class Stats>
  size_t num_entries(const Stats & st)
  {
    size_t count = 0;
    for (size_t i = 0; i < st.lens.size(); ++i)
      count += i * st.lens(i);
    return count;
  }

  /// Hash function that sends every key to the same bucket.
  size_t constant_hash(const int &) noexcept { return 0; }

  template <class Table>
  class HashChainStats : public ::testing::Test
  {
  };

  using Chained_Tables = ::testing::Types<DynSetHash<int>, DynSetLinHash<int>>;
  TYPED_TEST_SUITE(HashChainStats, Chained_Tables);
} // namespace

TYPED_TEST(HashChainStats, EmptyTableHasOnlyEmptyChains)
{
  const TypeParam table;
  const auto st = table.stats();
  EXPECT_EQ(num_chains(st), table.capacity());
  EXPECT_EQ(num_entries(st), 0u);
  EXPECT_EQ(st.avg, 0.0f);
}

TYPED_TEST(HashChainStats, HistogramAccountsForEveryChainAndEntry)
{
  TypeParam table;
  for (int i = 0; i < 1000; ++i)
    ASSERT_NE(table.insert(i * 7919), nullptr);

  const auto st = table.stats();
  EXPECT_EQ(num_chains(st), table.capacity());
  EXPECT_EQ(num_entries(st), table.size());
  EXPECT_NEAR(st.avg, static_cast<float>(table.size()) / table.capacity(), 1e-4);
}

TEST(HashChainStats, ChainLongerThanSeveralHistogramBlocks)
{
  // Every key collides, so the histogram only records lengths 0 and n. Its
  // DynArray must still be readable at every index below lens.size().
  DynSetHash<int> table(101, constant_hash);
  constexpr int n = 9000;
  for (int i = 0; i < n; ++i)
    ASSERT_NE(table.insert(i), nullptr);

  const auto st = table.stats();
  ASSERT_EQ(st.lens.size(), static_cast<size_t>(n) + 1);
  EXPECT_EQ(st.lens(n), 1u);
  EXPECT_EQ(st.lens(n / 2), 0u);
  EXPECT_EQ(num_chains(st), table.capacity());
  EXPECT_EQ(num_entries(st), static_cast<size_t>(n));
}
