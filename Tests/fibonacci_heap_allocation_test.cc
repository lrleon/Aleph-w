
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
 * @file fibonacci_heap_allocation_test.cc
 * @brief Extractions from a Fibonacci heap when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * every allocation fail for a while. extract_min() and delete_node() must
 * allocate nothing, but the room that a heap which was moved from lacks;
 * they allocate it before they change the heap, so that without memory the
 * heap is as it was.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <cstdlib>
#include <new>
#include <numeric>
#include <utility>
#include <vector>

#include <tpl_fibonacci_heap.H>

using namespace Aleph;

// The tests need the operator new of allocation_failure.H
struct FibonacciHeapWithoutMemory : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

// consolidate() kept the roots in a vector that it allocated on each call,
// once extract_min() or delete_node() had taken their node out of the heap
TEST_F(FibonacciHeapWithoutMemory, ExtractAndDeleteAllocateNothing)
{
  using Heap = Fibonacci_Heap<int>;
  Heap heap;
  std::vector<Heap::Node *> by_key(1000);
  for (int k = 0; k < 1000; ++k)
    {
      const int key = k * 7919 % 1000; // every key from 0 to 999
      by_key[key] = heap.insert(key);
    }
  ASSERT_EQ(heap.extract_min(), 0); // the roots end in trees

  std::vector<int> extracted;
  extracted.reserve(400);
  size_t refused = 0;
  {
    No_Memory no_memory;
    for (int i = 0; i < 400; ++i)
      extracted.push_back(heap.extract_min());
    for (int key = 900; key < 1000; ++key)
      heap.delete_node(by_key[key]);
    refused = refused_allocations;
  }

  EXPECT_EQ(refused, 0u);
  std::vector<int> expected(400);
  std::iota(expected.begin(), expected.end(), 1);
  EXPECT_EQ(extracted, expected);
  ASSERT_EQ(heap.size(), 499u);
  for (int key = 401; key < 900; ++key)
    ASSERT_EQ(heap.extract_min(), key);
  EXPECT_TRUE(heap.is_empty());
}

// A heap that was moved from lacks the room of consolidate(), which
// extract_min() and delete_node() allocate before they change the heap
TEST_F(FibonacciHeapWithoutMemory, AHeapMovedFromIsAsItWasWithoutMemory)
{
  using Heap = Fibonacci_Heap<int>;
  for (bool by_assignment : {false, true})
    {
      SCOPED_TRACE(by_assignment ? "move assignment" : "move construction");
      Heap heap;
      (void) heap.insert(1);
      Heap other;
      if (by_assignment)
        other = std::move(heap);
      else
        {
          Heap moved(std::move(heap));
          other.swap(moved);
        }

      Heap::Node * eight = nullptr;
      for (int key : {5, 3, 8, 6})
        {
          Heap::Node * p = heap.insert(key);
          if (key == 8)
            eight = p;
        }

      bool extract_threw = false;
      bool delete_threw = false;
      size_t refused = 0;
      {
        No_Memory no_memory;
        try
          {
            (void) heap.extract_min();
          }
        catch (const std::bad_alloc &)
          {
            extract_threw = true;
          }
        try
          {
            heap.delete_node(eight);
          }
        catch (const std::bad_alloc &)
          {
            delete_threw = true;
          }
        refused = refused_allocations;
      }

      EXPECT_GT(refused, 0u); // it had no room
      EXPECT_TRUE(extract_threw);
      EXPECT_TRUE(delete_threw);
      ASSERT_EQ(heap.size(), 4u);
      EXPECT_EQ(heap.extract_min(), 3);
      heap.delete_node(eight);
      EXPECT_EQ(heap.extract_min(), 5);
      EXPECT_EQ(heap.extract_min(), 6);
      EXPECT_TRUE(heap.is_empty());
      EXPECT_EQ(other.extract_min(), 1);
    }
}
