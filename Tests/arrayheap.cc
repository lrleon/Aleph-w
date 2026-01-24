
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
 * @file arrayheap.cc
 * @brief Tests for Arrayheap
 */

#include <algorithm>
#include <ranges>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_arrayHeap.H>

using namespace Aleph;
using namespace testing;

TEST(ArrayHeap, InsertAndExtract)
{
  ArrayHeap<int> heap(8);
  heap.insert(3);
  heap.insert(1);
  heap.insert(2);

  EXPECT_EQ(heap.size(), 3u);
  EXPECT_FALSE(heap.is_empty());
  EXPECT_EQ(heap.top(), 1);

  EXPECT_EQ(heap.getMin(), 1);
  EXPECT_EQ(heap.top(), 2);
  EXPECT_EQ(heap.get(), 2);
  EXPECT_EQ(heap.getMax(), 3); // alias of getMin when comparator flips
  EXPECT_TRUE(heap.is_empty());
}

TEST(ArrayHeap, UpdateAndRemove)
{
  ArrayHeap<int> heap(5);
  auto & ref = heap.insert(5);
  const auto & ref2 = heap.insert(7);

  ref = 1;
  heap.update(ref);
  EXPECT_EQ(heap.top(), 1);
  EXPECT_EQ(ref2, 7);

  heap.remove(ref);
  EXPECT_EQ(heap.size(), 1u);
  EXPECT_EQ(heap.top(), 7);
}

TEST(ArrayHeap, CapacityOverflow)
{
  ArrayHeap<int> heap(2);
  heap.insert(10);
  heap.insert(20);
  EXPECT_THROW(heap.insert(30), std::overflow_error);
}

TEST(ArrayHeap, ExtractsSortedOrder)
{
  ArrayHeap<int> heap(10);
  const std::vector<int> input{5, 3, 8, 1, 4, 9, 2, 7};
  for (int value : input)
    heap.insert(value);

  std::vector<int> drained;
  while (!heap.is_empty())
    drained.push_back(heap.getMin());

  std::vector<int> sorted = input;
  std::sort(sorted.begin(), sorted.end());
  EXPECT_EQ(drained, sorted);
  EXPECT_TRUE(heap.is_empty());
}
