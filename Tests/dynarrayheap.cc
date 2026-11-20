
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

namespace
{
  // The calls of a Counted_Less, and the one that throws (0: none)
  struct Calls
  {
    size_t count = 0;
    size_t throw_at = 0;
  };

  struct Counted_Less
  {
    Calls *calls;

    bool operator()(const int a, const int b) const
    {
      if (++calls->count == calls->throw_at)
        throw std::runtime_error("comparison");
      return a < b;
    }
  };

  using Counted_Heap = DynArrayHeap<int, Counted_Less>;

  // The array of the heap, in its order, as traverse() and the iterator
  // see it. The iterator goes to the end of the array, which is that of
  // the items unless the heap reserved more
  template <class Heap>
  std::vector<int> layout(const Heap & h, const bool reserved = false)
  {
    std::vector<int> by_traverse;
    h.traverse([&by_traverse](const int x) { by_traverse.push_back(x); return true; });
    if (reserved)
      return by_traverse;
    std::vector<int> by_iterator;
    for (typename Heap::Iterator it(h); it.has_curr(); it.next_ne())
      by_iterator.push_back(it.get_curr());
    EXPECT_EQ(by_traverse, by_iterator);
    return by_traverse;
  }

  // The sifts of 7.7.4, which exchanged an item with its parent or child
  // at each level, as a reference for the order of the new ones
  size_t old_sift_up(DynArray<int> & a, size_t l, size_t r, Counted_Less & cmp)
  {
    for (size_t p, i = r; i > l; i = p)
      {
        p = i / 2;
        if (cmp(a.access(p), a.access(i)))
          return i;
        std::swap(a.access(p), a.access(i));
      }
    return l;
  }

  void old_sift_down(DynArray<int> & a, size_t l, size_t r, Counted_Less & cmp)
  {
    for (size_t i = l; ;)
      {
        size_t c = 2 * i;
        if (c > r)
          return;
        if (c + 1 <= r and cmp(a.access(c + 1), a.access(c)))
          ++c;
        if (cmp(a.access(i), a.access(c)))
          return;
        std::swap(a.access(c), a.access(i));
        i = c;
      }
  }
}

// Each failed comparison of an insertion or an extraction leaves the heap
// as it was: its items in the same places and its size, with the key of
// an insertion by move untouched. 7.7.4 left the item counted, or the
// minimum lost, and the heap out of order
TEST(DynArrayHeapThrowingCompare, EveryFailedCallLeavesTheHeapAsItWas)
{
  std::mt19937 rng(20261120);
  std::uniform_int_distribution<int> values(0, 30);
  for (size_t n = 0; n <= 40; ++n)
    for (int op = 0; op < 4; ++op) // insert, insert of a move, insert_direct, getMin
      {
        if (op == 3 and n == 0)
          continue;
        std::vector<int> items;
        for (size_t k = 0; k < n; ++k)
          items.push_back(values(rng));
        const int key = values(rng);
        for (size_t at = 1; ; ++at)
          {
            SCOPED_TRACE(testing::Message() << "n " << n << ", op " << op << ", call " << at);
            Calls calls;
            Counted_Heap heap(Counted_Less{&calls});
            const bool reserved = op == 2; // insert_direct() writes in reserved entries
            if (reserved)
              heap.reserve(n + 2);
            for (const int x : items)
              heap.insert(x);
            const std::vector<int> before = layout(heap, reserved);
            calls.throw_at = calls.count + at;
            bool thrown = false;
            int moved = key;
            try
              {
                switch (op)
                  {
                  case 0: heap.insert(key); break;
                  case 1: heap.insert(std::move(moved)); break;
                  case 2: heap.insert_direct(key); break;
                  default: (void) heap.getMin();
                  }
              }
            catch (const std::runtime_error &)
              {
                thrown = true;
              }
            calls.throw_at = 0;
            if (not thrown)
              break;
            EXPECT_EQ(heap.size(), n);
            EXPECT_EQ(layout(heap, reserved), before);
            EXPECT_EQ(moved, key);
          }
      }
}

// The new sifts compare in the order of the old ones and leave every item
// in the same place
TEST(DynArrayHeapThrowingCompare, SameComparisonsAndPlacesAsTheOldSifts)
{
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> values(0, 50); // many equal items
  Calls calls, old_calls;
  Counted_Heap heap(Counted_Less{&calls});
  Counted_Less old_cmp{&old_calls};
  DynArray<int> old;
  size_t old_n = 0;
  for (int k = 0; k < 5000; ++k)
    {
      if (old_n == 0 or rng() % 3 != 0)
        {
          const int x = values(rng);
          heap.insert(x);
          old.touch(++old_n) = x;
          old_sift_up(old, 1, old_n, old_cmp);
        }
      else
        {
          const int min = heap.getMin();
          ASSERT_EQ(min, old.access(1));
          old.access(1) = old.access(old_n);
          --old_n;
          old_sift_down(old, 1, old_n, old_cmp);
        }
      ASSERT_EQ(calls.count, old_calls.count) << "operation " << k;
      std::vector<int> old_layout;
      for (size_t i = 1; i <= old_n; ++i)
        old_layout.push_back(old.access(i));
      ASSERT_EQ(layout(heap), old_layout) << "operation " << k;
    }
}
