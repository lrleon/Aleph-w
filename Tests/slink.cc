
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file slink.cc
 * @brief Tests for Slink
 */

#include <gtest/gtest.h>

#include <slink.H>

using namespace Aleph;

namespace {

struct TestNode
{
  int value;
  Slink link;
  SLINK_TO_TYPE(TestNode, link);
};

void push_front(Slink & head, TestNode & node)
{
  node.link.reset();
  head.insert_next(&node.link);
}

} // namespace

TEST(Slink, BasicInsertRemove)
{
  Slink head;
  TestNode n1{1, {}};
  TestNode n2{2, {}};

  ASSERT_TRUE(head.is_empty());
  ASSERT_TRUE(n1.link.is_empty());

  push_front(head, n1);
  EXPECT_FALSE(head.is_empty());
  EXPECT_EQ(head.get_next(), &n1.link);

  push_front(head, n2);
  EXPECT_EQ(head.get_next(), &n2.link);
  EXPECT_EQ(n2.link.get_next(), &n1.link);

  auto removed = head.remove_next();
  EXPECT_EQ(removed, &n2.link);
  EXPECT_TRUE(n2.link.is_empty());
  EXPECT_EQ(head.get_next(), &n1.link);

  removed = head.remove_next();
  EXPECT_EQ(removed, &n1.link);
  EXPECT_TRUE(head.is_empty());
}

TEST(Slink, MacroConversionRetrievesOwningNode)
{
  Slink head;
  TestNode nodes[] = {{10, {}}, {20, {}}, {30, {}}};

  for (auto & node : nodes)
    push_front(head, node);

  int expected[] = {30, 20, 10};
  size_t idx = 0;
  for (Slink * it = head.get_next(); it != &head; it = it->get_next())
    {
      auto * owner = TestNode::slink_to_type(it);
      ASSERT_NE(owner, nullptr);
      EXPECT_EQ(owner->value, expected[idx++]);
    }
  EXPECT_EQ(idx, std::size(expected));

  while (not head.is_empty())
    {
      auto * removed = head.remove_next();
      auto * owner = TestNode::slink_to_type(removed);
      owner->link.reset();
      EXPECT_TRUE(owner->link.is_empty());
    }
  EXPECT_TRUE(head.is_empty());
}

TEST(Slink, ConstGetNextMatchesMutable)
{
  Slink head;
  TestNode node{42, {}};
  push_front(head, node);

  const Slink & cref = head;
  EXPECT_EQ(cref.get_next(), head.get_next());
}
