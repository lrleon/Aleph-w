
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
 * @file slist.cc
 * @brief Tests for Slist
 */

#include <gtest/gtest.h>

#include <tpl_slist.H>

using namespace Aleph;
using namespace testing;

TEST(Slist, InsertAndRemoveFirst)
{
  Slist<int> list;
  auto * n1 = new Snode<int>(1);
  auto * n2 = new Snode<int>(2);

  list.insert_first(n1);
  EXPECT_EQ(list.get_first(), n1);

  list.insert_first(n2);
  EXPECT_EQ(list.get_first(), n2);

  auto removed = list.remove_first();
  EXPECT_EQ(removed, n2);
  EXPECT_TRUE(removed->is_empty());
  delete removed;

  removed = list.remove_first();
  EXPECT_EQ(removed, n1);
  delete removed;

  EXPECT_TRUE(list.is_empty());
  EXPECT_THROW(list.remove_first(), std::underflow_error);
}

TEST(Slist, GetFirstNeAndRemoveFirstNe)
{
  Slist<int> list;
  auto * n1 = new Snode<int>(1);
  auto * n2 = new Snode<int>(2);

  list.insert_first(n1);
  list.insert_first(n2);

  ASSERT_FALSE(list.is_empty());
  EXPECT_EQ(list.get_first_ne(), n2);

  auto * removed = list.remove_first_ne();
  EXPECT_EQ(removed, n2);
  EXPECT_TRUE(removed->is_empty());
  delete removed;

  EXPECT_EQ(list.get_first_ne(), n1);
  removed = list.remove_first_ne();
  EXPECT_EQ(removed, n1);
  delete removed;

  EXPECT_TRUE(list.is_empty());
}

TEST(Slist, IteratorOverflowOnEmptyAndAfterEnd)
{
  Slist<int> list;
  Slist<int>::Iterator it(list);
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);

  list.insert_first(new Snode<int>(1));
  Slist<int>::Iterator it2(list);
  EXPECT_TRUE(it2.has_curr());
  ASSERT_NE(it2.get_curr(), nullptr);
  it2.next();
  EXPECT_FALSE(it2.has_curr());
  EXPECT_THROW(it2.get_curr(), std::overflow_error);
  EXPECT_THROW(it2.next(), std::overflow_error);

  while (not list.is_empty())
    delete list.remove_first();
}

TEST(Slist, IteratorTraversesAllNodes)
{
  Slist<int> list;
  for (int i = 0; i < 5; ++i)
    list.insert_first(new Snode<int>(i));

  int count = 0;
  for (Slist<int>::Iterator it(list); it.has_curr(); it.next())
    {
      ASSERT_NE(it.get_curr(), nullptr);
      ++count;
    }
  EXPECT_EQ(count, 5);

  while (not list.is_empty())
    delete list.remove_first();
}
