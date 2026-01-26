
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
