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
 * @file sort_allocation_test.cc
 * @brief The sorts of lists when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * every allocation fail for a while. The sorts of a DynList and of a
 * DynDlist that sort in place link their nodes again, and allocate
 * nothing.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <algorithm>
#include <functional>
#include <random>
#include <utility>
#include <vector>

#include <ahSort.H>
#include <tpl_dynDlist.H>
#include <tpl_dynList.H>
#include <tpl_sort_utils.H>

using namespace Aleph;

namespace
{
  // The tests need the operator new of allocation_failure.H
  struct ListSortWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  // Copies the items of l to v, which has room for them, so that nothing
  // is allocated
  template <class List>
  void copy_items(const List & l, std::vector<int> & v)
  {
    size_t i = 0;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      v[i++] = it.get_curr();
  }
} // namespace

// mergesort() of a DynList took each item out of its node, and put it in a
// node that it allocated, at each merge: without memory, the list lost
// items. The references to the items now stay valid, too.
TEST_F(ListSortWithoutMemory, TheSortsInPlaceAllocateNothing)
{
  std::mt19937 gen(23);
  std::vector<int> items;
  for (int i = 0; i < 1024; ++i)
    items.push_back(int(gen() % 1000));
  std::vector<int> sorted = items;
  std::sort(sorted.begin(), sorted.end());
  std::vector<int> descending = sorted;
  std::reverse(descending.begin(), descending.end());

  DynList<int> list;
  DynDlist<int> dlist;
  for (const int x : items)
    {
      list.append(x);
      dlist.append(x);
    }
  const int * first = &list.get_first();

  // the items after each sort, of list and of dlist
  std::vector<std::vector<int>> results(14, std::vector<int>(items.size()));
  size_t refused = 0;
  {
    No_Memory no_memory;
    mergesort(list);
    mergesort(dlist);
    copy_items(list, results[0]);
    copy_items(dlist, results[1]);
    mergesort(list, std::greater<int>());
    mergesort(dlist, std::greater<int>());
    copy_items(list, results[2]);
    copy_items(dlist, results[3]);
    mergesort<DynList, int>(list);
    mergesort<DynDlist, int>(dlist);
    copy_items(list, results[4]);
    copy_items(dlist, results[5]);
    mergeinsertsort(list, std::greater<int>());
    mergeinsertsort(dlist, std::greater<int>());
    copy_items(list, results[6]);
    copy_items(dlist, results[7]);
    (void) in_place_sort(list);
    (void) in_place_sort(dlist);
    copy_items(list, results[8]);
    copy_items(dlist, results[9]);
    list = sort(std::move(list), std::greater<int>());
    dlist = sort(std::move(dlist), std::greater<int>());
    copy_items(list, results[10]);
    copy_items(dlist, results[11]);
    insertion_sort(list);
    insertion_sort(dlist);
    copy_items(list, results[12]);
    copy_items(dlist, results[13]);
    refused = refused_allocations;
  }
  EXPECT_EQ(refused, 0u);

  for (size_t i = 0; i < results.size(); ++i)
    {
      SCOPED_TRACE(i);
      const bool down = i / 2 == 1 or i / 2 == 3 or i / 2 == 5;
      EXPECT_EQ(results[i], down ? descending : sorted);
    }

  // the node of the first item is still in the list
  bool found = false;
  list.for_each([&found, first] (const int & x) { found = found or &x == first; });
  EXPECT_TRUE(found);
}
