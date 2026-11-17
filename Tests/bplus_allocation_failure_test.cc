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

/** @file bplus_allocation_failure_test.cc
 *  @brief An insertion into a Gen_BPlus_Tree that fails for lack of memory
 *         leaves the tree as it was.
 *
 *  Each test makes the n-th allocation of one insert() fail, for every n
 *  until the insert no longer allocates that many times, and then checks
 *  the tree. split_child() truncated the full child before allocating the
 *  new node and the separators, so a failure lost the keys of the new node,
 *  and the whole tree when insert() split a full root; and the separators
 *  that insert_nonfull() rebuilt on the way back were lost when one of
 *  their copies failed.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <new>
#include <string>

#include <tpl_bplus_tree.H>

using namespace Aleph;

// The tests need the operator new of allocation_failure.H
struct BPlusTreeWithoutMemory : public ::testing::Test
{
  void SetUp() override
  {
    if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
      GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  }
};

namespace
{
  template <typename Key> Key key_of(long i);

  template <> int key_of<int>(const long i)
  {
    return static_cast<int>((i * 7919) % 100003);
  }

  // long enough for its copies to allocate too
  template <> std::string key_of<std::string>(const long i)
  {
    return "a key long enough to allocate " + std::to_string((i * 7919) % 100003);
  }

  // Fail each allocation of the insertion of one more key into a tree of
  // n_keys keys; return the number of allocations that failed
  template <typename Key, size_t Degree>
  size_t fail_each_allocation_of_an_insert(const long n_keys)
  {
    using Tree = Gen_BPlus_Tree<Key, Aleph::less<Key>, Degree>;
    size_t failures = 0;
    for (size_t n = 1;; ++n)
      {
        Tree tree;
        for (long i = 0; i < n_keys; ++i)
          EXPECT_TRUE(tree.insert(key_of<Key>(i)));

        bool threw = false;
        {
          Fail_At fail(n);
          try
            {
              (void) tree.insert(key_of<Key>(n_keys));
            }
          catch (const std::bad_alloc &)
            {
              threw = true;
            }
        }
        if (not threw)
          return failures;
        ++failures;

        EXPECT_TRUE(tree.verify()) << n_keys << " keys, allocation " << n;
        EXPECT_EQ(tree.size(), static_cast<size_t>(n_keys)) << "allocation " << n;
        EXPECT_EQ(tree.keys().size(), static_cast<size_t>(n_keys)) << "allocation " << n;
        for (long i = 0; i < n_keys; ++i)
          EXPECT_TRUE(tree.contains(key_of<Key>(i))) << "key " << i << ", allocation " << n;
        EXPECT_TRUE(tree.insert(key_of<Key>(n_keys))) << "allocation " << n;
        EXPECT_TRUE(tree.verify()) << "allocation " << n;
      }
  }
}

// The root is full: the insertion splits it
TEST_F(BPlusTreeWithoutMemory, SplitOfAFullRoot)
{
  EXPECT_GT((fail_each_allocation_of_an_insert<int, 16>(31)), 0u);
  EXPECT_GT((fail_each_allocation_of_an_insert<int, 2>(3)), 0u);
}

// A leaf or an internal node below the root is split
TEST_F(BPlusTreeWithoutMemory, SplitsBelowTheRoot)
{
  for (const long n : {7L, 100L, 500L})
    (void) fail_each_allocation_of_an_insert<int, 2>(n);
}

// The copies of the keys allocate too
TEST_F(BPlusTreeWithoutMemory, KeysWhoseCopiesAllocate)
{
  for (const long n : {31L, 300L})
    (void) fail_each_allocation_of_an_insert<std::string, 16>(n);
  for (const long n : {3L, 20L, 200L})
    (void) fail_each_allocation_of_an_insert<std::string, 2>(n);
}
