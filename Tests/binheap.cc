
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/



/**
 * @file binheap.cc
 * @brief Tests for Binheap
 */
#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_dynBinHeap.H>

using namespace Aleph;

TEST(DynBinHeapTest, InsertExtractInOrder)
{
  DynBinHeap<int> heap;
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1};

  for (const int v : values)
    heap.insert(v);

  std::vector<int> expected = values;
  std::sort(expected.begin(), expected.end());

  for (const int v : expected)
    {
      ASSERT_EQ(heap.top(), v);
      EXPECT_EQ(heap.getMin(), v);
    }

  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);
}

TEST(DynBinHeapTest, RemoveArbitraryNodes)
{
  DynBinHeap<int> heap;
  const std::vector<int> values = {40, 10, 30, 20, 50, 60, 70, 15};
  std::vector<std::reference_wrapper<int>> refs;
  refs.reserve(values.size());

  for (const int v : values)
    refs.emplace_back(heap.insert(v));

  ASSERT_EQ(heap.size(), values.size());

  heap.remove(refs[3]);  // remove an interior node
  heap.remove(refs.back()); // remove what used to be the last inserted node

  std::vector<int> remaining;
  while (!heap.is_empty())
    remaining.push_back(heap.getMin());

  std::vector<int> expected = values;
  const auto erase_value = [&expected](int needle) {
    const auto it = std::find(expected.begin(), expected.end(), needle);
    ASSERT_NE(it, expected.end());
    expected.erase(it);
  };
  erase_value(20);
  erase_value(15);
  std::sort(expected.begin(), expected.end());

  EXPECT_EQ(remaining, expected);
}

TEST(DynBinHeapTest, UpdateReordersAfterPriorityChange)
{
  DynBinHeap<int> heap;
  auto &high = heap.insert(50);
  auto &low = heap.insert(5);
  heap.insert(20);

  high = 1; // becomes the best priority
  heap.update(high);
  EXPECT_EQ(heap.top(), 1);

  low = 60; // now should sink
  heap.update(low);

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  EXPECT_EQ(drained, std::vector<int>({1, 20, 60}));
}

TEST(DynBinHeapTest, RandomizedInsertExtract)
{
  DynBinHeap<int> heap;
  std::vector<int> values(200);
  std::iota(values.begin(), values.end(), -100);
  std::mt19937 rng(42);
  std::shuffle(values.begin(), values.end(), rng);

  for (int v : values)
    heap.insert(v);

  std::vector<int> extracted;
  while (!heap.is_empty())
    extracted.push_back(heap.getMin());

  std::vector<int> expected = values;
  std::sort(expected.begin(), expected.end());
  EXPECT_EQ(extracted, expected);
}

TEST(DynBinHeapTest, IteratorTraversesAllElements)
{
  DynBinHeap<int> heap;
  constexpr size_t N = 64;
  for (size_t i = 0; i < N; ++i)
    heap.insert(static_cast<int>(i));

  size_t count = 0;
  for (auto it = heap.get_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, N);
}

TEST(DynBinHeapTest, EmptyRemovesAllNodes)
{
  DynBinHeap<int> heap;
  for (int i = 0; i < 50; ++i)
    heap.insert(i);

  heap.empty();
  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0u);

  // Verify heap can be reused after empty
  heap.insert(10);
  EXPECT_EQ(heap.getMin(), 10);
  EXPECT_TRUE(heap.is_empty());
}

TEST(DynBinHeapTest, UpdateWithDuplicatePriorities)
{
  DynBinHeap<int> heap;
  std::vector<std::reference_wrapper<int>> refs;
  for (int i = 0; i < 5; ++i)
    refs.emplace_back(heap.insert(10)); // duplicates

  // Make one element the new minimum
  refs.front().get() = -1;
  heap.update(refs.front());
  EXPECT_EQ(heap.top(), -1);

  // Make another element very large to ensure sift-down works
  refs.back().get() = 100;
  heap.update(refs.back());

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
  EXPECT_EQ(drained.front(), -1);
  EXPECT_EQ(drained.back(), 100);
}
