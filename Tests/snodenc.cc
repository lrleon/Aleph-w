
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
 * @file snodenc.cc
 * @brief Tests for Snodenc
 */
# include <gtest/gtest.h>

# include <htlist.H>

using namespace testing;
using namespace Aleph;

struct List_of_5_nodes : public Test
{
  Snodenc<int> list;
  List_of_5_nodes()
  {
    list.insert(new Snodenc<int>(5));
    list.insert(new Snodenc<int>(4));
    list.insert(new Snodenc<int>(3));
    list.insert(new Snodenc<int>(2));
    list.insert(new Snodenc<int>(1));
  }
  ~List_of_5_nodes()
  {
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
  }
};

TEST(Snodenc, conversion_from_slinknc)
{
  {
    Snodenc<int> node = 10;
    Slinknc * ptr = &node;
    ASSERT_EQ(ptr->to_snodenc<int>(), &node);
    ASSERT_EQ(ptr->to_data<int>(), 10);
  }

  {
    const Snodenc<int> node = 10;
    const Slinknc * ptr = &node;
    ASSERT_EQ(ptr->to_snodenc<int>(), &node);
    ASSERT_EQ(ptr->to_data<int>(), 10);
  }
}

TEST_F(List_of_5_nodes, Basic_operations)
{
  ASSERT_EQ(list.get_next()->get_data(), 1);

  int i = 1;
  for (Snodenc<int>::Iterator it = list; it.has_curr(); it.next())
    {
      auto ptr = static_cast<Snodenc<int>*>(it.get_curr());
      EXPECT_EQ(ptr->get_data(), i++);
    }

  auto ptr = list.remove_first();
  EXPECT_EQ(ptr->get_data(), 1);
  EXPECT_TRUE(ptr->is_empty());
  list.insert(ptr);
  EXPECT_EQ(list.get_next()->get_data(), 1);
}
