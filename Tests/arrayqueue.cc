
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
 * @file arrayqueue.cc
 * @brief Tests for Arrayqueue
 */
# include <gtest/gtest.h>
# include <memory>
# include <deque>
# include <new>
# include <random>
# include <string>
# include <type_traits>

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

// Regression tests for queues whose front advanced past slot 0. Copy
// construction and copy assignment used to copy the raw buffer prefix, so the
// copy kept the indices of the source but not its items; empty() and
// reserve() also ignored the circular layout.

/// @brief Place g/h at slots 6/7 and wrap i/j around to slots 0/1.
static void fill_wrapped(ArrayQueue<string> &q)
{
  for (const char *item : {"a", "b", "c", "d", "e", "f", "g", "h"})
    q.put(item);
  for (int i = 0; i < 6; ++i)
    (void) q.get();
  q.put("i");
  q.put("j");
}

/// @brief Check the logical FIFO sequence against the expected values.
static void expect_order(const ArrayQueue<string> &q, initializer_list<const char *> expected)
{
  ASSERT_EQ(q.size(), expected.size());
  size_t i = 0;
  for (const char *item : expected)
    EXPECT_EQ(q.front(i++), item);
}

TEST(ArrayQueue, copy_keeps_the_items_of_a_wrapped_queue)
{
  ArrayQueue<string> q(8);
  fill_wrapped(q);

  ArrayQueue<string> copy(q);
  expect_order(copy, {"g", "h", "i", "j"});

  ArrayQueue<string> assigned;
  assigned.put("stale");
  assigned = q;
  expect_order(assigned, {"g", "h", "i", "j"});
  expect_order(q, {"g", "h", "i", "j"});

  // The copy keeps working as a queue.
  copy.put("k");
  EXPECT_EQ(copy.get(), "g");
  EXPECT_EQ(copy.rear(), "k");
  expect_order(copy, {"h", "i", "j", "k"});
}

TEST(ArrayQueue, empty_restarts_both_ends)
{
  ArrayQueue<int> q(8);
  for (int i = 1; i <= 5; ++i)
    q.put(i);
  (void) q.get();
  (void) q.get();
  q.empty();
  EXPECT_TRUE(q.is_empty());
  q.put(42);
  EXPECT_EQ(q.front(), 42);
  EXPECT_EQ(q.rear(), 42);

  q.put(43);
  (void) q.get();
  q.clear();
  q.put(7);
  EXPECT_EQ(q.front(), 7);

  for (int i = 0; i < 40; ++i)
    q.put(i);
  for (int i = 0; i < 30; ++i)
    (void) q.get();
  q.empty_and_release();
  EXPECT_TRUE(q.is_empty());
  q.put(9);
  EXPECT_EQ(q.front(), 9);
  EXPECT_EQ(q.rear(), 9);
}

TEST(ArrayQueue, reserve_keeps_the_order_of_a_wrapped_queue)
{
  ArrayQueue<string> q(8);
  fill_wrapped(q);
  q.reserve(64);
  EXPECT_GE(q.capacity(), 64U);
  expect_order(q, {"g", "h", "i", "j"});
  q.put("k");
  EXPECT_EQ(q.rear(), "k");
  EXPECT_EQ(q.get(), "g");
  expect_order(q, {"h", "i", "j", "k"});

  // A capacity that is already available leaves the queue untouched.
  q.reserve(4);
  expect_order(q, {"h", "i", "j", "k"});
}

static_assert(not std::is_convertible_v<ArrayQueue<int> *, MemArray<int> *>);
static_assert(std::is_nothrow_move_constructible_v<ArrayQueue<int>>);

TEST(ArrayQueue, public_accessors_and_removals_use_fifo_order)
{
  ArrayQueue<string> q(8);
  fill_wrapped(q);
  EXPECT_EQ(q.first(), "g");
  EXPECT_EQ(q.last(), "j");
  EXPECT_EQ(q.get_first(), "g");
  EXPECT_EQ(q.get_last(), "j");
  EXPECT_EQ(q.top(), "g");
  EXPECT_EQ(q[0], "g");
  EXPECT_EQ(q(1), "h");
  EXPECT_THROW(q[q.size()], out_of_range);
  EXPECT_EQ(q.remove_first(), "g");
  expect_order(q, {"h", "i", "j"});
  EXPECT_EQ(q.rear(), "j");
  EXPECT_EQ(q.remove_last(), "j");
  q.push("k");
  expect_order(q, {"h", "i", "k"});
  q.reverse();
  expect_order(q, {"k", "i", "h"});
  EXPECT_EQ(q.pop(), "k");
  expect_order(q, {"i", "h"});
}

TEST(ArrayQueue, copied_full_queue_wraps_the_next_insertion)
{
  ArrayQueue<int> q(8);
  for (int i = 0; i < 8; ++i)
    q.put(i);
  ArrayQueue<int> copy(q);
  EXPECT_EQ(copy.get(), 0);
  copy.put(8);
  for (int i = 1; i <= 8; ++i)
    EXPECT_EQ(copy.get(), i);
}

TEST(ArrayQueue, moved_from_queue_can_be_iterated_copied_and_reused)
{
  ArrayQueue<string> q(8);
  fill_wrapped(q);
  ArrayQueue<string> moved(std::move(q));
  EXPECT_TRUE(q.is_empty());
  EXPECT_FALSE(q.get_it().has_curr());
  EXPECT_EQ(q.begin(), q.end());
  ArrayQueue<string> empty_copy(q);
  EXPECT_TRUE(empty_copy.is_empty());
  ArrayQueue<string> empty_moved(std::move(q));
  EXPECT_TRUE(empty_moved.is_empty());

  q.put("again");
  EXPECT_EQ(q.front(), "again");
  EXPECT_EQ(q.get(), "again");
  q.reserve(64);
  q.put("reserved");
  EXPECT_EQ(q.front(), "reserved");
  empty_moved.putn(2);
  empty_moved[0] = "first";
  empty_moved[1] = "last";
  expect_order(empty_moved, {"first", "last"});
  expect_order(moved, {"g", "h", "i", "j"});
  EXPECT_THROW(empty_copy.putn(0), underflow_error);
}

TEST(ArrayQueue, reserve_preserves_wrapped_move_only_elements)
{
  ArrayQueue<std::unique_ptr<int>> q(8);
  for (int i = 0; i < 8; ++i)
    q.put(std::make_unique<int>(i));
  for (int i = 0; i < 6; ++i)
    EXPECT_EQ(*q.get(), i);
  q.put(std::make_unique<int>(8));
  q.put(std::make_unique<int>(9));
  q.reserve(64);
  for (int i = 6; i < 10; ++i)
    EXPECT_EQ(*q.get(), i);
}

TEST(ArrayQueue, release_then_insert_reproduces_the_reported_capacity_64_case)
{
  ArrayQueue<int> q(64);
  for (int i = 0; i < 40; ++i)
    q.put(i);
  for (int i = 0; i < 30; ++i)
    EXPECT_EQ(q.get(), i);
  q.empty_and_release();
  EXPECT_EQ(q.capacity(), 4u);
  q.put(9);
  EXPECT_EQ(q.front(), 9);
  EXPECT_EQ(q.rear(), 9);
  EXPECT_EQ(q.get(), 9);
}

TEST(ArrayQueue, state_transitions_match_a_fifo_reference)
{
  std::mt19937 random(0xa1e9);
  std::deque<int> reference;
  ArrayQueue<int> q(8);
  for (int step = 0; step < 2500; ++step)
    {
      SCOPED_TRACE(step);
      switch (random() % 10)
        {
        case 0: case 1: case 2:
          q.put(step);
          reference.push_back(step);
          break;
        case 3:
          if (not reference.empty())
            {
              EXPECT_EQ(q.get(), reference.front());
              reference.pop_front();
            }
          break;
        case 4:
          {
            ArrayQueue<int> copy(q);
            q = copy;
            break;
          }
        case 5:
          {
            ArrayQueue<int> moved(std::move(q));
            q.put(-1);  // exercise the moved-from source before assignment
            q = std::move(moved);
            break;
          }
        case 6: q.reserve(16 + random() % 64); break;
        case 7:
          q.clear();
          reference.clear();
          break;
        case 8:
          q.empty_and_release();
          reference.clear();
          break;
        case 9:
          if (not reference.empty())
            {
              EXPECT_EQ(q.remove_last(), reference.back());
              reference.pop_back();
            }
          break;
        }
      ASSERT_EQ(q.size(), reference.size());
      size_t i = 0;
      for (int value : q)
        {
          ASSERT_LT(i, reference.size());
          EXPECT_EQ(value, reference[i]);
          EXPECT_EQ(q(i), reference[i]);
          ++i;
        }
      EXPECT_EQ(i, reference.size());
    }
}

/// @brief A string too long for the small-string buffer.
static string long_string(const char c)
{
  return string(40, c);
}

/// @brief Element whose default construction, that of every slot of a new
/// buffer, can fail as an allocation does.
struct Failing_Slot
{
  static inline int constructions_left = -1;
  string value;

  /// @brief Fail at the configured default construction.
  Failing_Slot()
  {
    if (constructions_left == 0)
      throw bad_alloc();
    if (constructions_left > 0)
      --constructions_left;
  }

  /// @brief Build an element that holds `v`.
  explicit Failing_Slot(string v) : value(std::move(v)) {}
};

// The argument of put() may be an item of the queue itself, which growth
// used to free before reading it.

TEST(ArrayQueue, put_of_own_item_copies_it_before_growing)
{
  ArrayQueue<int> q(4);
  for (int i : {10, 20, 30, 40})
    q.put(i);
  ASSERT_EQ(q.size(), q.capacity());

  EXPECT_EQ(q.put(q.front()), 10);
  EXPECT_EQ(q.put(q.rear()), 10);  // with free capacity
  ASSERT_EQ(q.size(), 6u);
  for (int i : {10, 20, 30, 40, 10, 10})
    EXPECT_EQ(q.get(), i);
}

TEST(ArrayQueue, put_of_own_item_of_a_full_wrapped_queue)
{
  ArrayQueue<string> q(4);
  for (char c : {'a', 'b', 'c', 'd'})
    q.put(long_string(c));
  EXPECT_EQ(q.get(), long_string('a'));
  q.put(long_string('e'));  // the rear wraps around to slot 0
  ASSERT_EQ(q.size(), q.capacity());

  EXPECT_EQ(q.put(std::move(q.front(1))), long_string('c'));
  ASSERT_EQ(q.size(), 5u);
  EXPECT_EQ(q.front(), long_string('b'));
  EXPECT_EQ(q.front(3), long_string('e'));
  EXPECT_EQ(q.rear(), long_string('c'));

  EXPECT_EQ(q.put(q.front(3)), long_string('e'));  // with free capacity
  EXPECT_EQ(q.rear(1), long_string('c'));
}

TEST(ArrayQueue, failed_growth_leaves_the_queue_and_the_argument_untouched)
{
  ArrayQueue<Failing_Slot> q(4);
  for (char c : {'a', 'b', 'c', 'd'})
    q.put(Failing_Slot(long_string(c)));
  (void) q.get();
  q.put(Failing_Slot(long_string('e')));  // full and wrapped
  ASSERT_EQ(q.size(), q.capacity());

  Failing_Slot x(long_string('x'));
  Failing_Slot::constructions_left = 0;  // the new buffer cannot be built
  EXPECT_THROW(q.put(std::move(x)), bad_alloc);
  EXPECT_THROW(q.put(q.front()), bad_alloc);
  Failing_Slot::constructions_left = -1;

  EXPECT_EQ(x.value, long_string('x'));
  ASSERT_EQ(q.size(), 4u);
  for (size_t i = 0; i < 4; ++i)
    EXPECT_EQ(q.front(i).value, long_string(static_cast<char>('b' + i)));

  EXPECT_EQ(q.put(std::move(x)).value, long_string('x'));
  EXPECT_EQ(q.front().value, long_string('b'));
}
