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
 * @file bin-node.cc
 * @brief Tests for Bin Node
 */
# include <gtest/gtest.h>

# include <string>

# include <htlist.H>
# include <tpl_binNode.H>

using namespace std;
using namespace testing;
using namespace Aleph;

namespace
{
  struct SentinelControl
  {
    SentinelControl() = default;

    explicit SentinelControl(SentinelCtor) noexcept
    { /* empty */
    }

    static void reset() noexcept
    { /* empty */
    }

    static SentinelControl & get_data()
    {
      throw std::domain_error("SentinelControl has not data");
    }
  };
}

DECLARE_BINNODE_SENTINEL(TestSentinelNode, 64, SentinelControl);

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

TEST(BinNode, default_constructor_sets_null_links)
{
  BinNode<int> p;
  EXPECT_EQ(p.getL(), BinNode<int>::NullPtr);
  EXPECT_EQ(p.getR(), BinNode<int>::NullPtr);
}

TEST(BinNode, reset_restores_links_to_null)
{
  BinNode<int> p(10);
  BinNode<int> left(5);
  BinNode<int> right(15);
  p.getL() = &left;
  p.getR() = &right;

  ASSERT_NE(p.getL(), BinNode<int>::NullPtr);
  ASSERT_NE(p.getR(), BinNode<int>::NullPtr);

  p.reset();
  EXPECT_EQ(p.getL(), BinNode<int>::NullPtr);
  EXPECT_EQ(p.getR(), BinNode<int>::NullPtr);
}

TEST(BinNode, helpers_work_with_const_nodes)
{
  const BinNode<int> p(10);
  EXPECT_EQ(LLINK(&p), BinNode<int>::NullPtr);
  EXPECT_EQ(RLINK(&p), BinNode<int>::NullPtr);
  EXPECT_EQ(KEY(&p), 10);
}

TEST(BinNode, node_traits_provide_generic_access)
{
  using Tr = node_traits<BinNode<int>>;
  BinNode<int> p(10);
  BinNode<int> child(5);

  EXPECT_EQ(Tr::null(), BinNode<int>::NullPtr);
  EXPECT_EQ(Tr::left(&p), BinNode<int>::NullPtr);
  Tr::left(&p) = &child;
  EXPECT_EQ(Tr::left(&p), &child);
  EXPECT_EQ(Tr::key(&p), 10);

  const BinNode<int> & cp = p;
  EXPECT_EQ(Tr::left(&cp), &child);
  EXPECT_EQ(Tr::key(&cp), 10);
}

TEST(BinNode, move_constructor_moves_key_and_preserves_null_links)
{
  BinNode<std::string> p(std::string("hello"));
  BinNode<std::string> q(std::move(p));
  EXPECT_EQ(q.getL(), BinNode<std::string>::NullPtr);
  EXPECT_EQ(q.getR(), BinNode<std::string>::NullPtr);
  EXPECT_EQ(q.get_key(), "hello");
}

TEST(BinNode, key_to_node_returns_container_node)
{
  BinNode<int> p(7);
  int & k = p.get_key();
  auto * node = BinNode<int>::key_to_node(k);
  EXPECT_EQ(node, &p);
}

TEST(BinNodeSentinel, nullptr_is_sentinel_address)
{
  using N = TestSentinelNode<int>;
  ASSERT_NE(N::NullPtr, nullptr);

  N p(10);
  EXPECT_EQ(p.getL(), N::NullPtr);
  EXPECT_EQ(p.getR(), N::NullPtr);

  const N cp(10);
  EXPECT_EQ(LLINK(&cp), N::NullPtr);
  EXPECT_EQ(RLINK(&cp), N::NullPtr);
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
