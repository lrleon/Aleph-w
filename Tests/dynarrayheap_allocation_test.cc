
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
 * @file dynarrayheap_allocation_test.cc
 * @brief Items that enter a DynArrayHeap when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * one allocation fail. insert() counted the item before it made its place
 * and copied it: without memory for either, the heap counted an item that
 * it did not have.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <new>
#include <string>
#include <vector>

#include <tpl_dynArrayHeap.H>

using namespace Aleph;

namespace
{
  // The tests need the operator new of allocation_failure.H
  struct DynArrayHeapWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  using Heap = DynArrayHeap<std::string>;

  // Longer than the buffer of a short string: copying it allocates
  std::string item(const size_t k)
  {
    return std::string(40, 'x') + std::to_string(k);
  }

  // The array of the heap, in its order
  std::vector<std::string> layout(const Heap & h)
  {
    std::vector<std::string> out;
    h.traverse([&out](const std::string & s) { out.push_back(s); return true; });
    return out;
  }

  // On a heap of n items, the allocation k of add fails, for each k until
  // add needs no more. The heap must stay as it was, with the key, and
  // then take it with memory. Returns how many allocations add needs
  template <class Add>
  size_t allocations_as_it_was(const size_t n, Add add)
  {
    for (size_t k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "size " << n << ", allocation " << k);
        Heap heap;
        for (size_t i = 1; i <= n; ++i)
          heap.insert(item(i));
        const std::vector<std::string> before = layout(heap);
        std::string key = item(0);

        bool thrown = false;
        {
          Fail_At failing(k);
          try
            {
              add(heap, key);
            }
          catch (const std::bad_alloc &)
            {
              thrown = true;
            }
        }

        if (not thrown)
          {
            EXPECT_EQ(heap.size(), n + 1);
            return k - 1;
          }
        EXPECT_EQ(heap.size(), n);
        EXPECT_EQ(layout(heap), before);
        EXPECT_EQ(key, item(0));
        add(heap, key);
        EXPECT_EQ(heap.size(), n + 1);
        EXPECT_EQ(heap.top(), item(0));
      }
  }

  // The sizes up to 70 are in the first block of the array, which the
  // first insertion allocates with its segment. A heap that fills that
  // block is the other case: its next item is the first of another block,
  // one allocation more, which fails with the items in the heap
  template <class Add>
  void expect_as_it_was(Add add)
  {
    size_t in_the_block = 0;
    for (size_t n = 0; n <= 70; ++n)
      in_the_block = allocations_as_it_was(n, add);

    const size_t block_size = DynArray<std::string>().get_block_size();
    ASSERT_GT(block_size, 71U); // the items are the entries from 1 on
    EXPECT_GT(allocations_as_it_was(block_size - 1, add), in_the_block);
  }
}

TEST_F(DynArrayHeapWithoutMemory, InsertOfACopyLeavesTheHeapAsItWas)
{
  expect_as_it_was([](Heap & h, std::string & key) { h.insert(key); });
}

TEST_F(DynArrayHeapWithoutMemory, InsertOfAMoveLeavesTheHeapAsItWas)
{
  expect_as_it_was([](Heap & h, std::string & key) { h.insert(std::move(key)); });
}
