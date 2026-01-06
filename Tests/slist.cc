/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
  / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
 / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
/_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
              |_|

This file is part of Aleph-w library

Copyright (c) 2002-2018 Leandro Rabindranath Leon

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
