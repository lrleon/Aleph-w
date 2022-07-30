
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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

# include <tpl_arrayStack.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

constexpr size_t N = 17;

struct SimpleStack : public Test
{
  size_t n = 0;
  FixedStack<int> s;
  SimpleStack() : s(N)
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push(i);
  }
};

struct ComplexStack : public Test
{
  size_t n = 0;
  FixedStack<DynList<int>> s;
  ComplexStack() : s(N)
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push({ int(i), 0, 1, 2, int(i) });
  }
};

TEST(FixedStack, empty_stack)
{
  FixedStack<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST_F(SimpleStack, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top(), n - 1);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.top(i), n - i - 1);

  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.pop(), n - i - 1);
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  s.pushn(n);
  EXPECT_EQ(s.size(), n);
  for (size_t i = 0; i < n; ++i)
    s.top(i) = i;

  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(s.top(i), i);

  EXPECT_EQ(s.popn(n), n - 1);
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  s.empty();
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST_F(ComplexStack, push_pop)
{
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top().get_first(), n - 1);
  for (size_t i = 0; i < n; ++i)
    {
      EXPECT_EQ(s.top(i).get_first(), n - i - 1);
      EXPECT_EQ(s.top(i).get_last(), n - i - 1);
      EXPECT_EQ(s.top(i).nth(1), 0);
    }

  for (size_t i = 0; i < n; ++i)
    {
      auto l = s.pop();
      EXPECT_EQ(l.get_first(), n - i - 1);
      EXPECT_EQ(l.get_last(), n - i - 1);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);

  {
    auto & ll = s.pushn(n);
    EXPECT_TRUE(ll.is_empty());
  }
  EXPECT_EQ(s.size(), n);
  for (size_t i = 0; i < n; ++i)
    s.top(i) = { int(i), 0, 1, int(i) };

  for (size_t i = 0; i < n; ++i)
    {
      auto & l = s.top(i);
      EXPECT_EQ(l.get_first(), i);
      EXPECT_EQ(l.get_last(), i);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }

  {
    auto ll = s.popn(n);
    EXPECT_EQ(ll.get_first(), n - 1);
    EXPECT_EQ(ll.get_last(), n - 1);
    EXPECT_EQ(ll.nth(1), 0);
    EXPECT_EQ(ll.nth(2), 1);
  }
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(FixedStack, Iterator_on_empty_stack)
{
  FixedStack<int> s;
  auto it = s.get_it();
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);
}

TEST_F(SimpleStack, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);

  it.reset_last();
  for (size_t i = 0; it.has_curr(); it.prev(), ++i)
    ASSERT_EQ(it.get_curr(), n - i -1);
}

TEST_F(ComplexStack, Iterator)
{
  auto it = s.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    {
      ASSERT_EQ(it.get_curr().get_first(), i);
      ASSERT_EQ(it.get_curr().get_last(), i);
      ASSERT_EQ(it.get_curr().nth(1), 0);
      ASSERT_EQ(it.get_curr().nth(2), 1);
    }

  it.reset_last();
  for (size_t i = 0; it.has_curr(); it.prev(), ++i)
    {
      ASSERT_EQ(it.get_curr().get_first(), n - i -1);
      ASSERT_EQ(it.get_curr().get_last(), n - i -1);
      ASSERT_EQ(it.get_curr().nth(1), 0);
      ASSERT_EQ(it.get_curr().nth(2), 1);
    }
}

TEST_F(SimpleStack, copy_operations)
{
  { // test copy ctor
    FixedStack<int> sc = s;
    ASSERT_FALSE(sc.is_empty());
    ASSERT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      ASSERT_EQ(s.top(i), sc.top(i));
    ASSERT_EQ(i, s.size());
  }
  
  { // test copy assignment 
    FixedStack<int> sc;
    sc = s;
    ASSERT_FALSE(sc.is_empty());
    ASSERT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      ASSERT_EQ(s.top(i), sc.top(i));
    ASSERT_EQ(i, s.size());
  }

  // test move ctor
  FixedStack<int> sc(move(s));
  ASSERT_FALSE(sc.is_empty());
  ASSERT_EQ(sc.size(), n);
  ASSERT_EQ(s.size(), 0);
  ASSERT_TRUE(s.is_empty());
  int i = 0;
  for (; i < sc.size(); ++i)
    ASSERT_EQ(sc.top(i), n - i - 1);
  ASSERT_EQ(i, sc.size());

  s = move(sc);
  ASSERT_FALSE(s.is_empty());
  ASSERT_EQ(s.size(), n);
  ASSERT_TRUE(sc.is_empty());
  i = 0;
  for (; i < s.size(); ++i)
    ASSERT_EQ(s.top(i), n - i - 1);
  ASSERT_EQ(i, s.size());
}

TEST_F(ComplexStack, copy_operations)
{
  { // test copy ctor
    FixedStack<DynList<int>> sc = s;
    ASSERT_FALSE(sc.is_empty());
    ASSERT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      ASSERT_TRUE(eq(s.top(i), sc.top(i)));
    ASSERT_EQ(i, s.size());
  }
  
  { // test copy assignment 
    FixedStack<DynList<int>> sc;
    sc = s;
    ASSERT_FALSE(sc.is_empty());
    ASSERT_EQ(s.size(), sc.size());
    int i = 0;
    for (; i < sc.size(); ++i)
      ASSERT_TRUE(eq(s.top(i), sc.top(i)));
    ASSERT_EQ(i, s.size());
  }

  // test move ctor
  FixedStack<DynList<int>> sc(move(s));
  ASSERT_FALSE(sc.is_empty());
  ASSERT_EQ(sc.size(), n);
  ASSERT_EQ(s.size(), 0);
  ASSERT_TRUE(s.is_empty());
  int i = 0;
  for (; i < sc.size(); ++i)
    {
      ASSERT_EQ(sc.top(i).get_first(), n - i - 1);
      ASSERT_EQ(sc.top(i).get_last(), n - i - 1);
      ASSERT_EQ(sc.top(i).nth(1), 0);
      ASSERT_EQ(sc.top(i).nth(2), 1);
    }
  ASSERT_EQ(i, sc.size());

  s = move(sc);
  ASSERT_FALSE(s.is_empty());
  ASSERT_EQ(s.size(), n);
  ASSERT_TRUE(sc.is_empty());
  i = 0;
  for (; i < s.size(); ++i)
    {
      ASSERT_EQ(s.top(i).get_first(), n - i - 1);
      ASSERT_EQ(s.top(i).get_last(), n - i - 1);
      ASSERT_EQ(s.top(i).nth(1), 0);
      ASSERT_EQ(s.top(i).nth(2), 1);
    }
  ASSERT_EQ(i, s.size());
}

TEST_F(SimpleStack, traverse)
{
  size_t i = 0;
  s.traverse([&i] (auto k) { EXPECT_EQ(k, i); return k == i++; });
  ASSERT_EQ(i, n);
}

TEST_F(ComplexStack, traverse)
{
  int i = 0;
  s.traverse([&i] (const DynList<int> & l)
	     {
	       EXPECT_EQ(l.get_first(), i);
	       EXPECT_EQ(l.get_last(), i);
	       EXPECT_EQ(l.nth(1), 0);
	       EXPECT_EQ(l.nth(2), 1);
	       return l.get_first()  == i++;
	     });
  ASSERT_EQ(i, n);
}
