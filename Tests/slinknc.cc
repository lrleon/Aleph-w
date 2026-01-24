
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
 * @file slinknc.cc
 * @brief Tests for Slinknc
 */
# include <gtest/gtest.h>

# include <htlist.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(Slinknc, Initialized_in_nullptr)
{
  Slinknc l;
  ASSERT_EQ(l.get_next(), nullptr);
}

TEST(Slinknc, Basic_operations)
{
  Slinknc l;
  EXPECT_TRUE(l.is_empty());
  ASSERT_EQ(l.get_next(), nullptr);

  Slinknc n1;
  l.insert(&n1);
  ASSERT_EQ(l.get_next(), &n1);

  Slinknc c1(l);
  ASSERT_EQ(c1.get_next(), nullptr);

  Slinknc c2;
  ASSERT_EQ(c2.get_next(), nullptr);
  c2 = l;
  ASSERT_EQ(c2.get_next(), nullptr);

  Slinknc n2;
  l.insert(&n2);
  ASSERT_EQ(l.get_next(), &n2);
  ASSERT_EQ(l.remove_next(), &n2);
  ASSERT_EQ(l.get_next(), &n1);
  ASSERT_EQ(l.remove_next(), &n1);
  ASSERT_EQ(l.get_next(), nullptr);
}

struct Slinknc_of_5_items : public testing::Test
{
  Slinknc list;
  Slinknc n1, n2, n3, n4, n5;
  Slinknc_of_5_items()
  {
    list.insert(&n5);
    list.insert(&n4);
    list.insert(&n3);
    list.insert(&n2);
    list.insert(&n1);    
  }
};

TEST_F(Slinknc_of_5_items, Complex_operations)
{
  EXPECT_FALSE(list.is_empty());
  ASSERT_EQ(list.get_next(), &n1);
  ASSERT_EQ(list.get_next()->get_next(), &n2);
  ASSERT_EQ(list.get_next()->get_next()->get_next(), &n3);
  ASSERT_EQ(list.get_next()->get_next()->get_next()->get_next(), &n4);
  ASSERT_EQ(list.get_next()->get_next()->get_next()->get_next()->get_next(), &n5);

  Slinknc laux = list;
  EXPECT_TRUE(laux.is_empty());
  ASSERT_EQ(laux.get_next(), nullptr);
  laux = list;
  ASSERT_EQ(laux.get_next(), nullptr);

  Slinknc n6;
  list.insert(&n6);
  ASSERT_EQ(list.get_next(), &n6);
  EXPECT_FALSE(list.is_empty());
  
  ASSERT_EQ(list.remove_next(), &n6);
  EXPECT_TRUE(n6.is_empty());
  EXPECT_FALSE(list.is_empty());

  ASSERT_EQ(list.remove_next(), &n1);
  EXPECT_TRUE(n1.is_empty());
  EXPECT_FALSE(list.is_empty());

  ASSERT_EQ(list.remove_next(), &n2);
  EXPECT_TRUE(n2.is_empty());
  EXPECT_FALSE(list.is_empty());

  ASSERT_EQ(list.remove_next(), &n3);
  EXPECT_TRUE(n3.is_empty());
  EXPECT_FALSE(list.is_empty());

  ASSERT_EQ(list.remove_next(), &n4);
  EXPECT_TRUE(n4.is_empty());
  EXPECT_FALSE(list.is_empty());

  ASSERT_EQ(list.remove_next(), &n5);
  EXPECT_TRUE(n5.is_empty());
  EXPECT_TRUE(list.is_empty());
}
  
TEST(Slinknc, Iterator_on_empty)
{
  Slinknc l;
  Slinknc::Iterator it(l);
  ASSERT_EQ(it.has_curr(), false);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

TEST_F(Slinknc_of_5_items, Iterator_operations)
{
  Slinknc::Iterator it = list;

  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n1);
  it.next();

  it.reset_first();
  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n1);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n2);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n3);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n4);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &n5);
  it.next();
  
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

