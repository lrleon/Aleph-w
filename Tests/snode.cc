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
 * @file snode.cc
 * @brief Tests for Snode
 */

#include <gtest/gtest.h>

#include <tpl_snode.H>

using namespace Aleph;
using namespace testing;

TEST(Snode, BasicInsertAndRemove)
{
  Snode<int> head; // sentinel
  head.reset();

  Snode<int> n1(1);
  Snode<int> n2(2);

  head.insert_next(&n1);
  EXPECT_EQ(head.get_next(), &n1);
  EXPECT_EQ(head.get_next()->get_data(), 1);

  head.insert_next(&n2);
  EXPECT_EQ(head.get_next(), &n2);
  EXPECT_EQ(head.get_next()->get_next(), &n1);

  auto removed = head.remove_next();
  ASSERT_EQ(removed, &n2);
  EXPECT_TRUE(removed->is_empty());

  removed = head.remove_first();
  EXPECT_EQ(removed, &n1);
  EXPECT_TRUE(head.is_empty());
}

TEST(Snode, ConstAccessors)
{
  const Snode<int> node(42);
  EXPECT_EQ(node.get_data(), 42);

  Snode<int> head;
  head.reset();
  head.insert_next(const_cast<Snode<int>*>(&node));
  const Snode<int> & cref = head;
  EXPECT_EQ(cref.get_next(), head.get_next());
}
