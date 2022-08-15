
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

# include <gmock/gmock.h>

# include <htlist.H>
# include <tpl_binNode.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(BinNode, single_node)
{
  BinNode<int> p(10);
  ASSERT_EQ(p.getL(), nullptr);
  ASSERT_EQ(p.getR(), nullptr);
  ASSERT_EQ(p.get_key(), 10);

  BinNode<int> q = p;                // test copy
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(q.get_key(), 10);
}

TEST(BinNode, incomplete_node_left)
{
  BinNode<int> p(10);
  BinNode<int> q(5);

  p.getL() = &q;

  ASSERT_EQ(p.getL(), &q);
  ASSERT_EQ(p.getR(), nullptr);
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(p.getL()->get_key(), q.get_key());
  ASSERT_EQ(p.get_key(), 10);
  ASSERT_EQ(q.get_key(), 5);
}

TEST(BinNode, incomplete_node_right)
{
  BinNode<int> p(10);
  BinNode<int> q(15);

  p.getR() = &q;

  ASSERT_EQ(p.getR(), &q);
  ASSERT_EQ(p.getL(), nullptr);
  ASSERT_EQ(q.getL(), nullptr);
  ASSERT_EQ(q.getR(), nullptr);
  ASSERT_EQ(p.getR()->get_key(), q.get_key());
  ASSERT_EQ(p.get_key(), 10);
  ASSERT_EQ(q.get_key(), 15);
}
