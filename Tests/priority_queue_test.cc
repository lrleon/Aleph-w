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
 * @file priority_queue_test.cc
 * @brief Tests for Priority Queue
 */

#include <gtest/gtest.h>

#include <Priority_Queue.H>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

using namespace Aleph;

namespace
{
  struct MoveOnly
  {
    int key = 0;

    MoveOnly() = default;

    explicit MoveOnly(int k) : key(k) {}

    MoveOnly(const MoveOnly &) = delete;
    MoveOnly & operator=(const MoveOnly &) = delete;

    MoveOnly(MoveOnly &&) noexcept = default;
    MoveOnly & operator=(MoveOnly &&) noexcept = default;
  };

  struct MoveOnlyLess
  {
    bool operator()(const MoveOnly & a, const MoveOnly & b) const noexcept
    {
      return a.key < b.key;
    }
  };

  template <typename PQ>
  std::vector<typename PQ::value_type> drain(PQ & pq)
  {
    std::vector<typename PQ::value_type> out;
    while (!pq.empty())
      {
        out.push_back(pq.top());
        pq.pop();
      }
    return out;
  }

  template <typename T>
  std::vector<T> drain_std_min(std::priority_queue<T, std::vector<T>, std::greater<T>> & pq)
  {
    std::vector<T> out;
    while (!pq.empty())
      {
        out.push_back(pq.top());
        pq.pop();
      }
    return out;
  }

  template <typename T>
  std::vector<T> drain_std_max(std::priority_queue<T> & pq)
  {
    std::vector<T> out;
    while (!pq.empty())
      {
        out.push_back(pq.top());
        pq.pop();
      }
    return out;
  }

  static int rand_int(std::mt19937_64 & rng)
  {
    std::uniform_int_distribution<int> dist(-100000, 100000);
    return dist(rng);
  }

} // namespace

TEST(PriorityQueueBasics, DefaultIsMinHeap)
{
  Aleph::priority_queue<int> pq;
  pq.push(5);
  pq.push(1);
  pq.push(3);

  EXPECT_EQ(pq.top(), 1);
  pq.pop();
  EXPECT_EQ(pq.top(), 3);
  pq.pop();
  EXPECT_EQ(pq.top(), 5);
}

TEST(PriorityQueueBasics, EmptyAndSizeInvariants)
{
  Aleph::priority_queue<int> pq;
  EXPECT_TRUE(pq.empty());
  EXPECT_EQ(pq.size(), 0U);

  pq.push(10);
  EXPECT_FALSE(pq.empty());
  EXPECT_EQ(pq.size(), 1U);

  pq.pop();
  EXPECT_TRUE(pq.empty());
  EXPECT_EQ(pq.size(), 0U);
}

TEST(PriorityQueueBasics, ClearEmptiesQueue)
{
  Aleph::priority_queue<int> pq;
  for (int i = 0; i < 100; ++i)
    pq.push(i);

  EXPECT_FALSE(pq.empty());
  pq.clear();
  EXPECT_TRUE(pq.empty());
  EXPECT_EQ(pq.size(), 0U);
  EXPECT_THROW((void)pq.top(), std::underflow_error);
}

TEST(PriorityQueueBasics, SwapExchangesContents)
{
  Aleph::priority_queue<int> a;
  Aleph::priority_queue<int> b;

  a.push(3);
  a.push(1);
  b.push(10);
  b.push(7);

  EXPECT_EQ(a.top(), 1);
  EXPECT_EQ(b.top(), 7);

  a.swap(b);

  EXPECT_EQ(a.top(), 7);
  EXPECT_EQ(b.top(), 1);
}

TEST(PriorityQueueBasics, TopOnEmptyThrowsUnderflow)
{
  Aleph::priority_queue<int> pq;
  EXPECT_THROW((void)pq.top(), std::underflow_error);
}

TEST(PriorityQueueBasics, PopOnEmptyThrowsUnderflow)
{
  Aleph::priority_queue<int> pq;
  EXPECT_THROW(pq.pop(), std::underflow_error);
}

TEST(PriorityQueueConstructors, FromContainer)
{
  std::vector<int> v = {4, 1, 9, 2, 7, 7};
  Aleph::priority_queue<int> pq(v);

  auto drained = drain(pq);
  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
  EXPECT_EQ(drained.size(), v.size());
}

TEST(PriorityQueueConstructors, HandlesDuplicates)
{
  Aleph::priority_queue<int> pq;
  pq.push(5);
  pq.push(5);
  pq.push(5);
  pq.push(1);
  pq.push(1);

  EXPECT_EQ(pq.size(), 5U);
  EXPECT_EQ(pq.top(), 1);
  pq.pop();
  EXPECT_EQ(pq.top(), 1);
  pq.pop();
  EXPECT_EQ(pq.top(), 5);
}

TEST(PriorityQueueConstructors, FromIteratorRange)
{
  std::vector<int> v = {4, 1, 9, 2, 7, 7};
  Aleph::priority_queue<int> pq(v.begin(), v.end());

  auto drained = drain(pq);
  EXPECT_TRUE(std::is_sorted(drained.begin(), drained.end()));
  EXPECT_EQ(drained.size(), v.size());
}

TEST(PriorityQueueCompare, MaxHeapWithGreater)
{
  // With Compare=greater, the smallest element according to Compare is the maximum value.
  Aleph::priority_queue<int, Aleph::greater<int>> pq;
  pq.push(5);
  pq.push(1);
  pq.push(3);

  EXPECT_EQ(pq.top(), 5);
  pq.pop();
  EXPECT_EQ(pq.top(), 3);
  pq.pop();
  EXPECT_EQ(pq.top(), 1);
}

TEST(PriorityQueueMove, PushRvalueAndEmplace)
{
  Aleph::priority_queue<int> pq;

  int x = 7;
  pq.push(std::move(x));
  pq.emplace(3);
  pq.emplace(10);

  EXPECT_EQ(pq.top(), 3);
  pq.pop();
  EXPECT_EQ(pq.top(), 7);
  pq.pop();
  EXPECT_EQ(pq.top(), 10);
}

TEST(PriorityQueueMove, MoveOnlyTypeWorks)
{
  Aleph::priority_queue<MoveOnly, MoveOnlyLess> pq;

  pq.emplace(5);
  pq.push(MoveOnly(2));
  pq.emplace(9);

  EXPECT_EQ(pq.top().key, 2);
  pq.pop();
  EXPECT_EQ(pq.top().key, 5);
  pq.pop();
  EXPECT_EQ(pq.top().key, 9);
}

TEST(PriorityQueueRandomized, MatchesStdPriorityQueueMinHeap)
{
  std::mt19937_64 rng(0xC0FFEEULL);

  Aleph::priority_queue<int> pq;
  std::priority_queue<int, std::vector<int>, std::greater<int>> ref;

  constexpr int ops = 20000;
  std::uniform_int_distribution<int> op_dist(0, 99);

  for (int i = 0; i < ops; ++i)
    {
      const int op = op_dist(rng);

      if (op < 60) // push
        {
          const int x = rand_int(rng);
          pq.push(x);
          ref.push(x);
        }
      else if (op < 85) // pop
        {
          if (ref.empty())
            {
              EXPECT_TRUE(pq.empty());
              EXPECT_THROW(pq.pop(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(pq.empty());
              EXPECT_EQ(pq.top(), ref.top());
              pq.pop();
              ref.pop();
            }
        }
      else // top
        {
          if (ref.empty())
            {
              EXPECT_TRUE(pq.empty());
              EXPECT_THROW((void)pq.top(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(pq.empty());
              EXPECT_EQ(pq.top(), ref.top());
            }
        }

      EXPECT_EQ(pq.empty(), ref.empty());
      EXPECT_EQ(pq.size(), ref.size());
    }

  auto drained_pq = drain(pq);
  auto drained_ref = drain_std_min(ref);
  EXPECT_EQ(drained_pq, drained_ref);
}

TEST(PriorityQueueRandomized, MatchesStdPriorityQueueMaxHeap)
{
  std::mt19937_64 rng(0xBADC0DEULL);

  Aleph::priority_queue<int, Aleph::greater<int>> pq;
  std::priority_queue<int> ref;

  constexpr int ops = 20000;
  std::uniform_int_distribution<int> op_dist(0, 99);

  for (int i = 0; i < ops; ++i)
    {
      const int op = op_dist(rng);

      if (op < 60) // push
        {
          const int x = rand_int(rng);
          pq.push(x);
          ref.push(x);
        }
      else if (op < 85) // pop
        {
          if (ref.empty())
            {
              EXPECT_TRUE(pq.empty());
              EXPECT_THROW(pq.pop(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(pq.empty());
              EXPECT_EQ(pq.top(), ref.top());
              pq.pop();
              ref.pop();
            }
        }
      else // top
        {
          if (ref.empty())
            {
              EXPECT_TRUE(pq.empty());
              EXPECT_THROW((void)pq.top(), std::underflow_error);
            }
          else
            {
              ASSERT_FALSE(pq.empty());
              EXPECT_EQ(pq.top(), ref.top());
            }
        }

      EXPECT_EQ(pq.empty(), ref.empty());
      EXPECT_EQ(pq.size(), ref.size());
    }

  auto drained_pq = drain(pq);
  auto drained_ref = drain_std_max(ref);
  EXPECT_EQ(drained_pq, drained_ref);
}
