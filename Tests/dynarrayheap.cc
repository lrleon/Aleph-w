
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
 * @file dynarrayheap.cc
 * @brief Tests for Dynarrayheap
 */

#include <gtest/gtest.h>

#include <tpl_dynArrayHeap.H>

#include <algorithm>
#include <cstdint>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  template <typename Heap>
  std::vector<typename Heap::Item_Type> drain(Heap & h)
  {
    std::vector<typename Heap::Item_Type> out;
    while (!h.is_empty())
      out.push_back(h.getMin());
    return out;
  }

  struct Greater
  {
    bool operator()(int a, int b) const noexcept { return a > b; }
  };

} // namespace

TEST(DynArrayHeapBasics, EmptyAndSize)
{
  DynArrayHeap<int> heap;
  EXPECT_TRUE(heap.is_empty());
  EXPECT_EQ(heap.size(), 0U);
}

TEST(DynArrayHeapBasics, TopAndGetMinThrowOnEmpty)
{
  DynArrayHeap<int> heap;
  EXPECT_THROW((void)heap.top(), std::underflow_error);
  EXPECT_THROW((void)static_cast<const DynArrayHeap<int> &>(heap).top(), std::underflow_error);
  EXPECT_THROW((void)heap.getMin(), std::underflow_error);
}

TEST(DynArrayHeapBasics, InsertAndExtractInOrder)
{
  DynArrayHeap<int> heap;
  heap.insert(5);
  heap.insert(2);
  heap.insert(4);
  heap.insert(1);

  EXPECT_EQ(heap.size(), 4U);
  EXPECT_EQ(heap.top(), 1);

  EXPECT_EQ(heap.getMin(), 1);
  EXPECT_EQ(heap.get(), 2);
  EXPECT_EQ(heap.getMax(), 4);
  EXPECT_EQ(heap.getMin(), 5);
  EXPECT_TRUE(heap.is_empty());
}

TEST(DynArrayHeapBasics, ReserveRejectsShrinkingBelowSize)
{
  DynArrayHeap<int> heap;
  heap.insert(3);
  heap.insert(1);
  EXPECT_THROW(heap.reserve(1), std::out_of_range);
}

TEST(DynArrayHeapBasics, InsertDirectBehavesLikeInsert)
{
  DynArrayHeap<int> heap;
  heap.reserve(16);
  heap.insert(10);
  heap.insert_direct(1);
  heap.insert_direct(5);
  EXPECT_EQ(heap.top(), 1);

  auto drained = drain(heap);
  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
}

TEST(DynArrayHeapBasics, PutAndAppendAliasesWork)
{
  DynArrayHeap<int> heap;
  heap.put(3);
  heap.append(2);
  heap.put(1);
  EXPECT_EQ(heap.top(), 1);
}

TEST(DynArrayHeapCompare, GreaterMakesMaxHeap)
{
  DynArrayHeap<int, Greater> heap;
  heap.insert(1);
  heap.insert(10);
  heap.insert(3);
  EXPECT_EQ(heap.top(), 10);
  EXPECT_EQ(heap.getMin(), 10);
  EXPECT_EQ(heap.getMin(), 3);
  EXPECT_EQ(heap.getMin(), 1);
}

TEST(DynArrayHeapIterator, TraverseVisitsAllElements)
{
  DynArrayHeap<int> heap;
  for (int i = 1; i <= 10; ++i)
    heap.insert(i);

  std::vector<int> visited;
  struct Op
  {
    std::vector<int> *v;
    bool operator()(int x)
    {
      v->push_back(x);
      return true;
    }
  };

  Op op{&visited};
  EXPECT_TRUE(heap.traverse(op));
  EXPECT_EQ(visited.size(), 10U);
}

TEST(DynArrayHeapIterator, TraverseStopsEarly)
{
  DynArrayHeap<int> heap;
  for (int i = 1; i <= 10; ++i)
    heap.insert(i);

  int count = 0;
  struct Op
  {
    int *c;
    bool operator()(int)
    {
      ++(*c);
      return *c < 3;
    }
  };

  Op op{&count};
  EXPECT_FALSE(heap.traverse(op));
  EXPECT_EQ(count, 3);
}

TEST(DynArrayHeapRandomized, MatchesStdPriorityQueueMinHeap)
{
  std::mt19937_64 rng(0xD15EA5EULL);
  std::uniform_int_distribution<int> op_dist(0, 99);
  std::uniform_int_distribution<int> val_dist(-10000, 10000);

  DynArrayHeap<int> heap;
  std::priority_queue<int, std::vector<int>, std::greater<int>> ref;

  constexpr int ops = 30000;
  for (int i = 0; i < ops; ++i)
    {
      const int op = op_dist(rng);
      if (op < 60)
        {
          const int v = val_dist(rng);
          heap.insert(v);
          ref.push(v);
        }
      else
        {
          if (ref.empty())
            {
              EXPECT_TRUE(heap.is_empty());
              EXPECT_THROW((void)heap.getMin(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(heap.is_empty());
              EXPECT_EQ(heap.top(), ref.top());
              EXPECT_EQ(heap.getMin(), ref.top());
              ref.pop();
            }
        }

      EXPECT_EQ(heap.size(), ref.size());
      EXPECT_EQ(heap.is_empty(), ref.empty());
      if (!ref.empty())
        EXPECT_EQ(heap.top(), ref.top());
    }

  while (!ref.empty())
    {
      ASSERT_FALSE(heap.is_empty());
      EXPECT_EQ(heap.top(), ref.top());
      EXPECT_EQ(heap.getMin(), ref.top());
      ref.pop();
    }
  EXPECT_TRUE(heap.is_empty());
}
