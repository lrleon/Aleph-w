
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
