

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

# include <tpl_dynListQueue.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

constexpr size_t N = 17;

struct SimpleQueue : public Test
{
  size_t n = 0;
  DynListQueue<int> q;
  SimpleQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      q.put(i);
  }
};

struct ComplexQueue : public Test
{
  size_t n = 0;
  DynListQueue<DynList<int>> q;
  ComplexQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      q.put({ int(i), 0, 1, 2, int(i) });
  }
};

TEST(DynListQueue, empty_queue)
{
  DynListQueue<int> q;
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);
  EXPECT_THROW(q.front(), underflow_error);
  EXPECT_THROW(q.rear(), underflow_error);
}

TEST_F(SimpleQueue, get_put)
{
  EXPECT_FALSE(q.is_empty());
  EXPECT_EQ(q.size(), n);
  EXPECT_EQ(q.front(), 0);
  EXPECT_EQ(q.rear(), n - 1);

  const size_t m = 100;
  for (size_t i = 0; i < m; ++i)
    ASSERT_EQ(q.put(i), i);
  EXPECT_EQ(q.size(), n + m);

  for (size_t i = 0; i < n; ++i)
    ASSERT_EQ(q.get(), i);

  EXPECT_EQ(q.rear(), m - 1);
  EXPECT_EQ(q.front(), 0);
  EXPECT_EQ(q.size(), m);

  for (size_t i = 0; i < m; ++i)
    ASSERT_EQ(q.get(), i);

  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);

  for (size_t i = 0; i < m; ++i)
    EXPECT_EQ(q.put(i), i);
  EXPECT_EQ(q.size(), m);

  q.empty();
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);
}

TEST_F(ComplexQueue, push_pop)
{
  EXPECT_FALSE(q.is_empty());
  EXPECT_EQ(q.size(), n);
  EXPECT_EQ(q.rear().get_first(), n - 1);
  EXPECT_EQ(q.front().get_first(), 0);
    
  const size_t m = 100;
  for (size_t i = 0; i < m; ++i)
    {
      ASSERT_EQ(q.put({int(i), 0, 1, int(i)}).get_first(), i);
      ASSERT_EQ(q.rear().get_first(), i);
      ASSERT_EQ(q.rear().get_last(), i);
      ASSERT_EQ(q.rear().nth(1), 0);
      ASSERT_EQ(q.rear().nth(2), 1);
    }
  EXPECT_EQ(q.size(), n + m);

  for (size_t i = 0; i < n; ++i) // extract n items
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i);      
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 0);
      ASSERT_EQ(l.nth(2), 1);
    }

  EXPECT_EQ(q.rear().get_first(), m - 1);
  EXPECT_EQ(q.rear().get_last(), m - 1);
  EXPECT_EQ(q.front().get_first(), 0);
  EXPECT_EQ(q.front().get_last(), 0);
  EXPECT_EQ(q.size(), m);
  
  for (size_t i = 0; i < m; ++i)
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 0);
      ASSERT_EQ(l.nth(2), 1);
    }
  EXPECT_EQ(q.size(), 0);
  EXPECT_TRUE(q.is_empty());
  
  for (size_t i = 0; i < m; ++i)
    {
      auto & l = q.put({ int(i), 0, 1, int(i) });
      EXPECT_EQ(l.get_first(), i);
      EXPECT_EQ(l.get_last(), i);
      EXPECT_EQ(l.nth(1), 0);
      EXPECT_EQ(l.nth(2), 1);
    }
  EXPECT_EQ(q.size(), m);

  q.empty();
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);
}

TEST(DynListQueue, Iterator_on_empty_queue)
{
  DynListQueue<int> q;
  auto it = q.get_it();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
}

TEST_F(SimpleQueue, Iterator)
{
  auto it = q.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
}

TEST_F(ComplexQueue, Iterator)
{
  auto it = q.get_it(); 
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    {
      ASSERT_EQ(it.get_curr().get_first(), i);
      ASSERT_EQ(it.get_curr().get_last(), i);
      ASSERT_EQ(it.get_curr().nth(1), 0);
      ASSERT_EQ(it.get_curr().nth(2), 1);
    }
}

TEST_F(SimpleQueue, copy_operations)
{
  { // test copy ctor
    DynListQueue<int> qc = q;
    EXPECT_FALSE(qc.is_empty());
    EXPECT_EQ(qc.size(), n);
    EXPECT_EQ(q.size(), qc.size());
    int i = 0;
    for (; i < n; ++i)
      ASSERT_EQ(qc.get(), i);
    EXPECT_EQ(i, n);
  }
  
  { // test copy assignment 
    DynListQueue<int> qc;
    qc = q;
    EXPECT_FALSE(qc.is_empty());
    EXPECT_EQ(q.size(), qc.size());
    int i = 0;
    for (; i < n; ++i)
      ASSERT_EQ(qc.get(), i);
    EXPECT_EQ(i, q.size());
  }

  // test move ctor
  DynListQueue<int> qc(move(q));
  EXPECT_FALSE(qc.is_empty());
  EXPECT_EQ(qc.size(), n);
  EXPECT_EQ(q.size(), 0);
  EXPECT_TRUE(q.is_empty());
  int i = 0;
  for (; i < n; ++i)
    {
      q.put(qc.get());
      ASSERT_EQ(q.rear(), i);
      ASSERT_EQ(q.front(), 0);
    }

  EXPECT_EQ(i, n);
  EXPECT_EQ(q.size(), n);
  EXPECT_TRUE(qc.is_empty());

  qc = move(q);
  EXPECT_FALSE(qc.is_empty());
  EXPECT_EQ(qc.size(), n);
  EXPECT_TRUE(q.is_empty());
  i = 0;
  for (; i < n; ++i)
    {
      q.put(qc.get());
      ASSERT_EQ(q.rear(), i);
      ASSERT_EQ(q.front(), 0);
    }
  EXPECT_EQ(i, n);
  EXPECT_EQ(q.size(), n);
  EXPECT_TRUE(qc.is_empty());
}

TEST_F(ComplexQueue, copy_operations)
{
  { // test copy ctor
    DynListQueue<DynList<int>> qc = q;
    EXPECT_FALSE(qc.is_empty());
    EXPECT_EQ(q.size(), qc.size());
    EXPECT_TRUE(eq(q.front(), qc.front()));
    EXPECT_TRUE(eq(q.rear(), qc.rear()));
  }
  
  { // test copy assignment 
    DynListQueue<DynList<int>> qc;
    qc = q;
    EXPECT_FALSE(qc.is_empty());
    EXPECT_EQ(q.size(), qc.size());
    EXPECT_TRUE(eq(q.rear(), qc.rear()));
    EXPECT_TRUE(eq(q.front(), qc.front()));
  }

  // test move ctor
  DynListQueue<DynList<int>> qc(move(q));
  EXPECT_FALSE(qc.is_empty());
  EXPECT_EQ(qc.size(), n);
  EXPECT_EQ(q.size(), 0);
  EXPECT_TRUE(q.is_empty());

  int k = 0;
  for (auto it = qc.get_it(); it.has_curr(); it.next(), ++k)
    {
      auto & l = it.get_curr();
      ASSERT_EQ(l.get_first(), k);
      ASSERT_EQ(l.get_last(), k);
      ASSERT_EQ(l.nth(1), 0);
      ASSERT_EQ(l.nth(2), 1);
    }

  EXPECT_EQ(qc.rear().get_first(), n - 1);
  EXPECT_EQ(qc.rear().get_last(), n - 1);
  EXPECT_EQ(qc.rear().nth(1), 0);
  EXPECT_EQ(qc.rear().nth(2), 1);

  q = move(qc);
  EXPECT_FALSE(q.is_empty());
  EXPECT_EQ(q.size(), n);
  EXPECT_TRUE(qc.is_empty());

  k = 0;
  for (auto it = qc.get_it(); it.has_curr(); it.next(), ++k)
    {
      auto & l = it.get_curr();
      ASSERT_EQ(l.get_first(), k);
      ASSERT_EQ(l.get_last(), k);
      ASSERT_EQ(l.nth(1), 0);
      ASSERT_EQ(l.nth(2), 1);
    }

  EXPECT_EQ(q.rear().get_first(), n - 1);
  EXPECT_EQ(q.rear().get_last(), n - 1);
  EXPECT_EQ(q.rear().nth(1), 0);
  EXPECT_EQ(q.rear().nth(2), 1);
}

TEST_F(SimpleQueue, traverse)
{
  size_t i = 0;
  bool ret = q.traverse([&i] (int k) { return (k == i++); });
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}

TEST_F(ComplexQueue, traverse)
{
  int i = 0;
  auto ret = q.traverse([&i] (const DynList<int> & l)
			{
			  return l.get_first() == i and
			  l.get_last() == i++ and
			  l.nth(1) == 0 and l.nth(2) == 1;
			});
  EXPECT_TRUE(ret);
  EXPECT_EQ(i, n);
}



