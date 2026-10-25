
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
 * @file binheap.cc
 * @brief Tests for Binheap
 */
#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_binHeap.H>
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

namespace
{
  // Less than; counts its calls, and the call number fail_at throws (0 for
  // none).
  struct Failing_Less
  {
    long * calls = nullptr;
    long fail_at = 0;

    bool operator()(const int & a, const int & b) const
    {
      if (calls != nullptr and ++*calls == fail_at)
        throw std::runtime_error("comparison failed");
      return a < b;
    }
  };

  using Failing_Heap = BinHeap<int, Failing_Less>;

  // Inserts and then extracts values with a comparison that fails at its
  // call fail_at; returns the extracted values
  std::vector<int> insert_and_extract(const std::vector<int> & values,
                                      long & calls, long fail_at)
  {
    Failing_Heap heap(Failing_Less{&calls, fail_at});
    std::vector<int> out;
    try
      {
        for (const int v : values)
          heap.insert(new Failing_Heap::Node(v));

        heap.update(heap.getRoot()); // its key is unchanged

        while (not heap.is_empty())
          { // getMin() throws with the node already out of the heap
            std::unique_ptr<Failing_Heap::Node> min(heap.top());
            heap.getMin();
            out.push_back(min->get_key());
          }
      }
    catch (...)
      {
        heap.remove_all_and_delete();
        throw;
      }
    return out;
  }
} // namespace

// insert(), update() and getMin() were noexcept, so a comparison that
// threw ended the program.
TEST(BinHeapComparison, WhatTheComparisonThrowsReachesTheCaller)
{
  const std::vector<int> values = {7, 3, 11, 0, 5, 9, 1, 8};
  long calls = 0;
  const auto out = insert_and_extract(values, calls, 0);
  ASSERT_TRUE(std::is_sorted(out.begin(), out.end()));
  ASSERT_EQ(out.size(), values.size());

  for (long k = 1; k <= calls; ++k)
    {
      long count = 0;
      EXPECT_THROW(insert_and_extract(values, count, k), std::runtime_error)
        << "call " << k;
    }
}

// Emptying a heap of up to 3 nodes extracted them one by one, comparing
// the keys left, so in a destructor a comparison that throws ended the
// program.
TEST(BinHeapComparison, RemoveAllAndDeleteDoesNotCompare)
{
  for (int n = 1; n <= 6; ++n)
    {
      long calls = 0;
      Failing_Heap heap(Failing_Less{&calls, 0});
      for (int i = 0; i < n; ++i)
        heap.insert(new Failing_Heap::Node(n - i));

      const long before = calls;
      heap.remove_all_and_delete();
      EXPECT_EQ(calls, before) << n << " nodes";
      EXPECT_TRUE(heap.is_empty());

      heap.insert(new Failing_Heap::Node(4)); // the heap can be used again
      EXPECT_EQ(heap.top()->get_key(), 4);
      heap.remove_all_and_delete();
    }
}

namespace
{
  std::vector<int> drain(DynBinHeap<int> & heap)
  {
    std::vector<int> out;
    while (not heap.is_empty())
      out.push_back(heap.getMin());
    return out;
  }
} // namespace

// swap() exchanged the roots but left each root pointing up to the header
// of the other heap, so the moves of DynBinHeap, which swap, broke the heap.
TEST(DynBinHeapTest, MoveConstructionOutlivesTheSource)
{
  for (int n = 0; n <= 7; ++n)
    {
      std::unique_ptr<DynBinHeap<int>> moved;
      {
        DynBinHeap<int> source;
        for (int i = n; i > 0; --i)
          source.insert(i);
        moved = std::make_unique<DynBinHeap<int>>(std::move(source));
        EXPECT_TRUE(source.is_empty());
        source.insert(10); // the source can be used again
        EXPECT_EQ(source.getMin(), 10);
      }

      std::vector<int> expected(n);
      std::iota(expected.begin(), expected.end(), 1);
      EXPECT_EQ(drain(*moved), expected) << n << " values";
    }
}

TEST(DynBinHeapTest, MoveAssignmentOutlivesTheSource)
{
  DynBinHeap<int> target;
  for (int i : {20, 30})
    target.insert(i);
  {
    DynBinHeap<int> source;
    for (int i : {5, 3, 8, 1, 9, 2})
      source.insert(i);
    target = std::move(source);
  }
  EXPECT_EQ(drain(target), (std::vector<int>{1, 2, 3, 5, 8, 9}));
}

TEST(DynBinHeapTest, SwapExchangesTheContents)
{
  DynBinHeap<int> a, b, spare;
  for (int i : {4, 2, 6, 1})
    a.insert(i);
  for (int i : {7, 3, 5})
    b.insert(i);

  a.swap(b);
  a.insert(0); // both heaps keep working after the swap
  b.insert(10);
  EXPECT_EQ(drain(a), (std::vector<int>{0, 3, 5, 7}));
  EXPECT_EQ(drain(b), (std::vector<int>{1, 2, 4, 6, 10}));

  for (int i : {9, 8})
    a.insert(i);
  a.swap(spare); // with an empty heap
  EXPECT_TRUE(a.is_empty());
  a.insert(1);
  EXPECT_EQ(drain(a), std::vector<int>{1});
  EXPECT_EQ(drain(spare), (std::vector<int>{8, 9}));
}
