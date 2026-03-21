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

#include <gtest/gtest.h>

#include <State_Search.H>

using namespace Aleph;

namespace
{

struct DummyEntry
{
  size_t depth = 0;
  int value = 0;
};

struct CollidingKey
{
  int value = 0;

  [[nodiscard]] bool operator == (const CollidingKey &) const noexcept = default;
};

[[nodiscard]] inline size_t aleph_hash_value(const CollidingKey &) noexcept
{
  return 1u;
}

} // end namespace

TEST(TranspositionTableFramework, ProbeAndReplacementStatsAreTracked)
{
  using Table = Transposition_Table<int,
                                    DummyEntry,
                                    MapOLhash,
                                    Aleph::equal_to<int>,
                                    Prefer_Deeper_Entry<DummyEntry>>;

  Table table;

  EXPECT_TRUE(table.is_empty());
  EXPECT_EQ(table.store(7, DummyEntry{1, 10}), TranspositionStoreStatus::Inserted);
  EXPECT_FALSE(table.is_empty());
  ASSERT_EQ(table.size(), 1u);

  auto *entry = table.probe(7);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->depth, 1u);
  EXPECT_EQ(entry->value, 10);

  EXPECT_EQ(table.store(7, DummyEntry{0, 5}), TranspositionStoreStatus::Rejected);
  entry = table.probe(7);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->depth, 1u);
  EXPECT_EQ(entry->value, 10);

  EXPECT_EQ(table.store(7, DummyEntry{3, 15}), TranspositionStoreStatus::Replaced);
  entry = table.probe(7);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->depth, 3u);
  EXPECT_EQ(entry->value, 15);

  EXPECT_EQ(table.probe(99), nullptr);

  const auto &stats = table.stats();
  EXPECT_EQ(stats.probes, 4u);
  EXPECT_EQ(stats.hits, 3u);
  EXPECT_EQ(stats.misses, 1u);
  EXPECT_EQ(stats.stores, 2u);
  EXPECT_EQ(stats.replacements, 1u);
  EXPECT_EQ(stats.rejected_updates, 1u);
}

TEST(TranspositionTableFramework, CollidingKeysRemainReachable)
{
  using Table = Transposition_Table<CollidingKey, DummyEntry>;

  Table table;

  EXPECT_EQ(table.store(CollidingKey{1}, DummyEntry{1, 11}),
            TranspositionStoreStatus::Inserted);
  EXPECT_EQ(table.store(CollidingKey{2}, DummyEntry{2, 22}),
            TranspositionStoreStatus::Inserted);

  auto *first = table.probe(CollidingKey{1});
  auto *second = table.probe(CollidingKey{2});

  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->depth, 1u);
  EXPECT_EQ(first->value, 11);
  EXPECT_EQ(second->depth, 2u);
  EXPECT_EQ(second->value, 22);
  EXPECT_EQ(table.size(), 2u);
}
