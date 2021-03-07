
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <tpl_arrayQueue.H>

# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

constexpr size_t N = 17;

struct SimpleQueue : public Test
{
  size_t n = 0;
  ArrayQueue<int> q;
  SimpleQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      q.put(i);
  }
  void print() const
  {
    cout << "q ="; q.for_each([] (auto i) { cout << " " << i; }); cout << endl;
  }
};

struct ComplexQueue : public Test
{
  size_t n = 0;
  ArrayQueue<DynList<int>> q;
  ComplexQueue()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      q.put({ int(i), 1, 2, int(i) });
  }
};

TEST(ArrayQueue, empty_queue)
{
  ArrayQueue<int> q;
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);
  EXPECT_THROW(q.rear(), range_error);
  EXPECT_THROW(q.front(), range_error);
  EXPECT_THROW(q.rear(2), range_error);
  EXPECT_THROW(q.front(2), range_error);
  EXPECT_THROW(q.rear(q.capacity()), range_error);
  EXPECT_THROW(q.front(q.capacity()), range_error);
  EXPECT_THROW(q.get(), underflow_error);
  EXPECT_THROW(q.getn(0), underflow_error);
  EXPECT_THROW(q.getn(1), underflow_error);
  EXPECT_THROW(q.getn(q.capacity()), underflow_error);
}

TEST(ArrayQueue, fill_and_empty_queue)
{
  ArrayQueue<int> q;
  const size_t N = q.capacity();
  for (int i = 0; i < N; ++i)
    {
      ASSERT_EQ(q.put(i), i);
      ASSERT_EQ(q.rear(), i);
      ASSERT_EQ(q.front(), 0);
    }
  EXPECT_EQ(q.size(), N);
  EXPECT_FALSE(q.is_empty());

  for (int i = 0; i < N; ++i)
    {
      ASSERT_EQ(q.front(i), i);
      ASSERT_EQ(q.rear(i), N - i - 1);
    }

  for (int i = 0; i < N; ++i)
    {
      ASSERT_EQ(q.front(), i);
      ASSERT_EQ(q.rear(), N - 1);
      ASSERT_EQ(q.get(), i);
    }
  EXPECT_TRUE(q.is_empty());
  EXPECT_EQ(q.size(), 0);
  EXPECT_EQ(q.capacity(), N);
}

TEST_F(SimpleQueue, put_and_get_stress)
{
  EXPECT_LT(q.size(), q.capacity());

  // fill until complete initial_cap
  for (int i = q.size(); i < q.capacity(); ++i)
    ASSERT_EQ(q.put(i), i);

  EXPECT_EQ(q.size(), q.capacity());

  for (size_t i = 0; i < q.size(); ++i)
    {
      ASSERT_EQ(q.front(i), i);
      ASSERT_EQ(q.rear(i), q.size() - i - 1);
    }

  // now put more entries
  for (size_t i = q.size(), n = 2*q.size(); i < n; ++i)
    ASSERT_EQ(q.put(i), i);

  EXPECT_EQ(q.size(), q.capacity());

  const size_t nn = q.size();

  // get out the half
  for (size_t i = 0; i < nn/2; ++i)
    ASSERT_EQ(q.get(), i);

  EXPECT_EQ(q.size(), nn/2);

  // test consistency of remaining items
  for (size_t i = 0; i < nn/2; ++i)
    ASSERT_EQ(q.front(i), i + nn/2);

  // now extract them all
  for (size_t i = 0; i < nn/2; ++i)
    ASSERT_EQ(q.get(), i + nn/2);

  EXPECT_EQ(q.size(), 0);
  EXPECT_TRUE(q.is_empty());

  // now we put the queue thus
  //
  // xxx------xxxxxxx
  //
  // where x is an item
  const size_t cap = 16; 
  for (size_t i = 0; i < cap; ++i)
    ASSERT_EQ(q.put(i), i);

  for (size_t i = 0; i < cap/4; ++i) // extract a fourth 
    ASSERT_EQ(q.get(), i);

  ASSERT_FALSE(q.is_empty());
  ASSERT_EQ(q.size(), 3*cap/4);

  for (size_t i = 0; i < cap/4; ++i) // put them again
    ASSERT_EQ(q.put(i), i);

  for (size_t i = 0; i < 3*cap/4; ++i) // extract and verify the 3/4 oldest
    ASSERT_EQ(q.get(), cap/4 + i);

  // now extract and verify the 1/4 remaining
  for (size_t i = 0; i < cap/4; ++i)
    ASSERT_EQ(q.get(), i);
}

TEST_F(ComplexQueue, put_and_stress_capacity)
{
  EXPECT_LT(q.size(), q.capacity());

  // fill until complete initial_cap
  for (int i = q.size(); i < q.capacity(); ++i)
    {
      auto & l = q.put({int(i), 1, 2, int(i)});
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  EXPECT_EQ(q.size(), q.capacity());

  for (size_t i = 0; i < q.size(); ++i)
    {
      auto & lf = q.front(i);
      ASSERT_EQ(lf.get_first(), i);
      ASSERT_EQ(lf.get_last(), i);
      ASSERT_EQ(lf.nth(1), 1);
      ASSERT_EQ(lf.nth(2), 2);

      auto & lr = q.rear(i);
      ASSERT_EQ(lr.get_first(), q.size() - i - 1);
      ASSERT_EQ(lr.get_last(),  q.size() - i - 1);
      ASSERT_EQ(lr.nth(1), 1);
      ASSERT_EQ(lr.nth(2), 2);
    }

  // now put more entries
  for (size_t i = q.size(), n = 2*q.size(); i < n; ++i)
    {
      auto & l = q.put({int(i), 1, 2, int(i)});
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  EXPECT_EQ(q.size(), q.capacity());

  const size_t nn = q.size();

  // get out the half
  for (size_t i = 0; i < nn/2; ++i)
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  EXPECT_EQ(q.size(), nn/2);

  // test consistency of remaining items
  for (size_t i = 0; i < nn/2; ++i)
    {
      auto & l = q.front(i);
      ASSERT_EQ(l.get_first(), i + nn/2);
      ASSERT_EQ(l.get_last(), i + nn/2);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  // now extract them all
  for (size_t i = 0; i < nn/2; ++i)
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i + nn/2);
      ASSERT_EQ(l.get_last(), i + nn/2);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  EXPECT_EQ(q.size(), 0);
  EXPECT_TRUE(q.is_empty());

  // now we put the queue thus
  //
  // xxx------xxxxxxx
  //
  // where x is an item
  const size_t cap = 16; 
  for (size_t i = 0; i < cap; ++i)
    {
      auto & l = q.put({int(i), 1, 2, int(i)});
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  for (size_t i = 0; i < cap/4; ++i) // extract a fourth
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  ASSERT_FALSE(q.is_empty());
  ASSERT_EQ(q.size(), 3*cap/4);

  for (size_t i = 0; i < cap/4; ++i) // put them again
    {
      auto l = q.put({int(i), 1, 2, int(i)});
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  for (size_t i = 0; i < 3*cap/4; ++i) // extract and verify the 3/4 oldest
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), cap/4 + i);
      ASSERT_EQ(l.get_last(), cap/4 + i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }

  // now extract and verify the 1/4 remaining
  for (size_t i = 0; i < cap/4; ++i)
    {
      auto l = q.get();
      ASSERT_EQ(l.get_first(), i);
      ASSERT_EQ(l.get_last(), i);
      ASSERT_EQ(l.nth(1), 1);
      ASSERT_EQ(l.nth(2), 2);
    }
}

TEST(ArrayQueue, Iterator_on_empty_queue)
{
  ArrayQueue<int> q;
  auto it = q.get_it();
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);
  ASSERT_THROW(it.prev(), underflow_error);
}

static size_t primes[] = {13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59,
			  61, 67, 71, 73, 79, 83, 89, 97, 197 };
TEST(ArrayQueue, Iterator)
{  
  for (size_t i = 0, N = primes[i]; N < 100; ++i, N = primes[i])
    {
      ArrayQueue<int> q;

      for (size_t i = 0; i < N; ++i)
	ASSERT_EQ(q.put(i), i);

      int k = 0;
      for (auto it = q.get_it(); it.has_curr(); it.next(), ++k)
	ASSERT_EQ(it.get_curr(), k);
      ASSERT_EQ(k, N); // test that queue has been traversed

      // extract 1/4 of items
      for (size_t i = 0; i < N/4; ++i)
	ASSERT_EQ(q.get(), i);

      ASSERT_FALSE(q.is_empty());

      // Test iterator again
      k = N/4;
      for (auto it = q.get_it(); it.has_curr(); it.next(), ++k)
	ASSERT_EQ(it.get_curr(), k);
      ASSERT_EQ(k, N); // test that queue has been traversed

      // now put again N/4 for testing iterator on queue of form xxx----xxxxxxx
      for (size_t i = 0; i < N/4; ++i)
	ASSERT_EQ(q.put(i), i);
      ASSERT_EQ(q.size(), N);

      // now test if iterator still works
      k = 0;
      for (auto it = q.get_it(); it.has_curr(); it.next(), ++k)
	ASSERT_EQ(it.get_curr(), (k + N/4) % N);
      ASSERT_EQ(k, N); // test that queue has been traversed

      k = N - 1;
      auto it = q.get_it();
      it.reset_last();
      for (; it.has_curr(); it.prev(), --k)
	ASSERT_EQ(it.get_curr(), (k + N/4) % N);
      ASSERT_EQ(k, -1); // test that queue has been traversed
    }
}

TEST(ArrayQueue, traverse)
{
  for (size_t i = 0, N = primes[i]; N < 100; ++i, N = primes[i])
    {
      ArrayQueue<int> q;

      for (size_t i = 0; i < N; ++i)
	ASSERT_EQ(q.put(i), i);

      int k = 0;
      auto ret = q.traverse([&k] (int i) { return i == k++; });
      ASSERT_TRUE(ret);
      ASSERT_EQ(k, N); 

      // extract 1/4 of items
      for (size_t i = 0; i < N/4; ++i)
	ASSERT_EQ(q.get(), i);

      ASSERT_FALSE(q.is_empty());

      // Test traverse
      k = N/4;
      ret = q.traverse([&k] (int i) { return i == k++; });
      ASSERT_TRUE(ret);
      ASSERT_EQ(k, N); 

      // now put again N/4 for testing iterator on queue of form xxx----xxxxxxx
      for (size_t i = 0; i < N/4; ++i)
	ASSERT_EQ(q.put(i), i);
      ASSERT_EQ(q.size(), N);

      // now test if iterator still works
      k = 0;
      ret = q.traverse([&k, N] (int i) { return i == (k++ + N/4) % N; });
      ASSERT_TRUE(ret);
      ASSERT_EQ(k, N);

      // finally test partial traverse
      k = 0;
      ret = q.traverse([&k, n = N/4] (int) { return ++k < n; });
      ASSERT_FALSE(ret);
      ASSERT_EQ(k, N/4);
    }
}

TEST(ArrayQueue, copy_operations)
{
  size_t N = 31;
  {
    ArrayQueue<int> q;
    for (size_t i = 0; i < N; ++i)
      ASSERT_EQ(q.put(i), i);

    {
      ArrayQueue<int> qc = q;
      ASSERT_TRUE(eq(q, qc));
    }

    {
      ArrayQueue<int> qc = move(q);
      ASSERT_TRUE(q.is_empty());
      ASSERT_EQ(q.size(), 0);
      int k = 0;
      auto ret = qc.traverse([&k] (int i) { return i == k++; });
      ASSERT_TRUE(ret);
      ASSERT_EQ(k, qc.size());

      q.swap(qc);
      ASSERT_EQ(qc.size(), 0);
      ASSERT_TRUE(qc.is_empty());
      ASSERT_EQ(q.size(), N);
      ASSERT_FALSE(q.is_empty());
    }

    ArrayQueue<int> qc;
    qc = q;
    ASSERT_TRUE(eq(q, qc));

    qc.empty();
    ASSERT_EQ(qc.size(), 0);
    ASSERT_TRUE(qc.is_empty());

    qc = move(q);
    ASSERT_TRUE(q.is_empty());
    ASSERT_EQ(q.size(), 0);
    int k = 0;
    auto ret = qc.traverse([&k] (int i) { return i == k++; });
    ASSERT_TRUE(ret);
    ASSERT_EQ(k, qc.size());
  }
}

