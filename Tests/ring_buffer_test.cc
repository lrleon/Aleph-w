/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file ring_buffer_test.cc
 * @brief Tests for Aleph::RingBuffer (tpl_ring_buffer.H).
 *
 * Covers FIFO order across wrap-around, overflow/underflow checking, the
 * put_overwrite sliding-window policy, logical indexing and iteration,
 * move-only element support, element lifetime balance, copy/move semantics
 * and a randomized parity check against std::deque bounded to the same
 * capacity.
 */

#include <deque>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <tpl_ring_buffer.H>

using Aleph::RingBuffer;

namespace
{
// Instrumented element type: counts live instances so tests can prove that
// placement-constructed elements are destroyed exactly once.
struct Probe
{
  static int live;
  int value = 0;

  Probe() { ++live; }
  explicit Probe(int v) : value(v) { ++live; }
  Probe(const Probe &p) : value(p.value) { ++live; }
  Probe(Probe &&p) noexcept : value(p.value) { ++live; }
  Probe &operator=(const Probe &) = default;
  Probe &operator=(Probe &&) noexcept = default;
  ~Probe() { --live; }
};

int Probe::live = 0;

struct CopyAssignableOnly
{
  int value = 0;

  explicit CopyAssignableOnly(int v) : value(v) {}
  CopyAssignableOnly(const CopyAssignableOnly &) = delete;
  CopyAssignableOnly(CopyAssignableOnly &&) noexcept = default;
  CopyAssignableOnly &operator=(const CopyAssignableOnly &) = default;
  CopyAssignableOnly &operator=(CopyAssignableOnly &&) noexcept = default;
};

static_assert(std::is_copy_assignable_v<CopyAssignableOnly>);
static_assert(not std::is_copy_constructible_v<CopyAssignableOnly>);

template <typename U, typename = void>
struct Has_Const_Put_Overwrite : std::false_type
{
};

template <typename U>
struct Has_Const_Put_Overwrite
  <U, std::void_t<decltype(std::declval<RingBuffer<U> &>().put_overwrite
                           (std::declval<const U &>()))>> : std::true_type
{
};

static_assert(not Has_Const_Put_Overwrite<CopyAssignableOnly>::value);
}  // namespace

TEST(RingBuffer, ConstructionAndBasicState)
{
  RingBuffer<int> rb(3);
  EXPECT_EQ(rb.capacity(), 3u);
  EXPECT_EQ(rb.size(), 0u);
  EXPECT_EQ(rb.available(), 3u);
  EXPECT_TRUE(rb.is_empty());
  EXPECT_FALSE(rb.is_full());

  EXPECT_THROW(RingBuffer<int>(0), std::invalid_argument);
}

TEST(RingBuffer, FifoOrderWithWrapAround)
{
  RingBuffer<int> rb(3);
  rb.put(1);
  rb.put(2);
  rb.put(3);
  EXPECT_TRUE(rb.is_full());

  EXPECT_EQ(rb.get(), 1);
  EXPECT_EQ(rb.get(), 2);
  rb.put(4);  // wraps physically
  rb.put(5);
  EXPECT_TRUE(rb.is_full());

  EXPECT_EQ(rb.get(), 3);
  EXPECT_EQ(rb.get(), 4);
  EXPECT_EQ(rb.get(), 5);
  EXPECT_TRUE(rb.is_empty());
}

TEST(RingBuffer, OverflowAndUnderflowThrow)
{
  RingBuffer<int> rb(2);
  rb.put(1);
  rb.put(2);
  EXPECT_THROW(rb.put(3), std::overflow_error);
  EXPECT_THROW(rb.emplace(3), std::overflow_error);

  (void) rb.get();
  (void) rb.get();
  EXPECT_THROW((void) rb.get(), std::underflow_error);
  EXPECT_THROW((void) rb.get_first(), std::underflow_error);
  EXPECT_THROW((void) rb.get_last(), std::underflow_error);
}

TEST(RingBuffer, PutOverwriteImplementsSlidingWindow)
{
  RingBuffer<int> rb(3);
  EXPECT_FALSE(rb.put_overwrite(1));
  EXPECT_FALSE(rb.put_overwrite(2));
  EXPECT_FALSE(rb.put_overwrite(3));

  // Buffer full: each further put_overwrite evicts the oldest element.
  EXPECT_TRUE(rb.put_overwrite(4));  // evicts 1 → window {2, 3, 4}
  EXPECT_TRUE(rb.put_overwrite(5));  // evicts 2 → window {3, 4, 5}

  ASSERT_EQ(rb.size(), 3u);
  EXPECT_EQ(rb[0], 3);
  EXPECT_EQ(rb[1], 4);
  EXPECT_EQ(rb[2], 5);
  EXPECT_EQ(rb.get_first(), 3);
  EXPECT_EQ(rb.get_last(), 5);
}

TEST(RingBuffer, LogicalIndexingAndFrontBack)
{
  RingBuffer<std::string> rb(3);
  rb.put("a");
  rb.put("b");
  rb.put("c");
  (void) rb.get();   // drop "a"; head is now physical index 1
  rb.put("d");       // physically wraps to index 0

  EXPECT_EQ(rb[0], "b");
  EXPECT_EQ(rb[1], "c");
  EXPECT_EQ(rb[2], "d");
  EXPECT_EQ(rb.front(), "b");
  EXPECT_EQ(rb.back(), "d");
  EXPECT_THROW((void) rb[3], std::out_of_range);

  rb[0] = "B";  // mutable indexed access
  EXPECT_EQ(rb.get_first(), "B");
}

TEST(RingBuffer, IterationOldestToNewestAcrossWrap)
{
  RingBuffer<int> rb(4);
  for (int i = 1; i <= 4; ++i)
    rb.put(i);
  (void) rb.get();
  (void) rb.get();
  rb.put(5);
  rb.put(6);  // logical: 3 4 5 6, physically wrapped

  std::deque<int> seen;
  for (int x : rb)
    seen.push_back(x);
  EXPECT_EQ(seen, (std::deque<int>{3, 4, 5, 6}));

  // Random access iterator operations.
  auto it = rb.begin();
  EXPECT_EQ(rb.end() - it, 4);
  EXPECT_EQ(it[2], 5);
  EXPECT_EQ(*(it + 3), 6);
  auto tail = rb.end();
  tail += -1;
  EXPECT_EQ(*tail, 6);
  EXPECT_EQ(tail[-2], 4);
  auto mid = rb.begin() + 1;
  mid -= -2;
  EXPECT_EQ(*mid, 6);
  RingBuffer<int> other(4);
  other.put(30);
  other.put(40);
  other.put(50);
  other.put(60);
  EXPECT_NE(rb.begin(), other.begin());
  EXPECT_TRUE(rb.begin() == rb.begin());

  // Mutation through iterators.
  for (int &x : rb)
    x *= 10;
  EXPECT_EQ(rb[0], 30);
  EXPECT_EQ(rb[3], 60);

  // Const iteration.
  const RingBuffer<int> &crb = rb;
  int sum = 0;
  for (int x : crb)
    sum += x;
  EXPECT_EQ(sum, 180);
}

TEST(RingBuffer, TraverseInFifoOrderWithEarlyStop)
{
  RingBuffer<int> rb(3);
  rb.put(1);
  rb.put(2);
  rb.put(3);

  int sum = 0;
  EXPECT_TRUE(rb.traverse([&sum] (int &x) { sum += x; return true; }));
  EXPECT_EQ(sum, 6);

  int visited = 0;
  const RingBuffer<int> &crb = rb;
  EXPECT_FALSE(crb.traverse([&visited] (const int &x)
  {
    ++visited;
    return x < 2;
  }));
  EXPECT_EQ(visited, 2);
}

TEST(RingBuffer, MoveOnlyElementsAreSupported)
{
  RingBuffer<std::unique_ptr<int>> rb(2);
  rb.put(std::make_unique<int>(1));
  rb.emplace(std::make_unique<int>(2));
  ASSERT_TRUE(rb.is_full());

  std::unique_ptr<int> p = rb.get();
  EXPECT_EQ(*p, 1);

  rb.put_overwrite(std::make_unique<int>(3));
  rb.put_overwrite(std::make_unique<int>(4));  // evicts 2
  ASSERT_EQ(rb.size(), 2u);
  EXPECT_EQ(*rb[0], 3);
  EXPECT_EQ(*rb[1], 4);
}

TEST(RingBuffer, ElementLifetimesAreBalanced)
{
  ASSERT_EQ(Probe::live, 0);
  {
    RingBuffer<Probe> rb(3);
    EXPECT_EQ(Probe::live, 0);  // raw storage: no default constructions
    rb.emplace(1);
    rb.emplace(2);
    rb.emplace(3);
    EXPECT_EQ(Probe::live, 3);
    (void) rb.get();
    EXPECT_EQ(Probe::live, 2);
    rb.put_overwrite(Probe(7));
    rb.put_overwrite(Probe(8));  // eviction by assignment: count stable
    EXPECT_EQ(Probe::live, 3);
    rb.clear();
    EXPECT_EQ(Probe::live, 0);
    rb.emplace(9);
  }
  EXPECT_EQ(Probe::live, 0);  // destructor cleaned everything
}

TEST(RingBuffer, CopyAndMoveSemantics)
{
  RingBuffer<int> rb(3);
  rb.put(1);
  rb.put(2);
  rb.put(3);
  (void) rb.get();
  rb.put(4);  // wrapped: logical {2, 3, 4}

  RingBuffer<int> copy = rb;
  EXPECT_EQ(copy, rb);
  EXPECT_EQ(copy.capacity(), rb.capacity());
  EXPECT_EQ(copy.get(), 2);  // copy is independent
  EXPECT_EQ(rb.size(), 3u);

  RingBuffer<int> moved = std::move(rb);
  ASSERT_EQ(moved.size(), 3u);
  EXPECT_EQ(moved[0], 2);
  EXPECT_EQ(moved[2], 4);

  RingBuffer<int> assigned(1);
  assigned = moved;  // copy assignment replaces capacity and contents
  EXPECT_EQ(assigned.capacity(), 3u);
  EXPECT_EQ(assigned, moved);
}

TEST(RingBuffer, EqualityComparesLogicalContentOnly)
{
  RingBuffer<int> a(3);
  RingBuffer<int> b(5);  // different capacity
  a.put(1);
  a.put(2);
  b.put(1);
  b.put(2);
  EXPECT_TRUE(a == b);
  b.put(3);
  EXPECT_TRUE(a != b);
}

TEST(RingBuffer, AlephConventionsEmptyClear)
{
  RingBuffer<int> rb(2);
  rb.put(1);
  rb.put(2);
  rb.empty();  // Aleph convention: empty() clears
  EXPECT_TRUE(rb.is_empty());
  EXPECT_EQ(rb.available(), 2u);
  rb.put(3);   // reusable after clearing
  EXPECT_EQ(rb.get_first(), 3);
}

// Randomized parity test: RingBuffer must behave like a std::deque bounded
// to the same capacity under an arbitrary interleaving of puts, gets and
// sliding-window overwrites.
TEST(RingBuffer, RandomizedParityWithBoundedDeque)
{
  std::mt19937 rng(20260702);
  std::uniform_int_distribution<int> val_dist(0, 1000000);
  std::uniform_int_distribution<int> op_dist(0, 3);
  constexpr size_t cap = 8;

  RingBuffer<int> rb(cap);
  std::deque<int> ref;

  for (int step = 0; step < 4000; ++step)
    {
      const int v = val_dist(rng);
      switch (op_dist(rng))
        {
        case 0:  // bounded put
          if (ref.size() < cap)
            {
              rb.put(v);
              ref.push_back(v);
            }
          else
            EXPECT_THROW(rb.put(v), std::overflow_error);
          break;
        case 1:  // sliding-window put
          rb.put_overwrite(v);
          ref.push_back(v);
          if (ref.size() > cap)
            ref.pop_front();
          break;
        case 2:  // extraction
          if (ref.empty())
            EXPECT_THROW((void) rb.get(), std::underflow_error);
          else
            {
              EXPECT_EQ(rb.get(), ref.front());
              ref.pop_front();
            }
          break;
        default:  // full logical window comparison
          ASSERT_EQ(rb.size(), ref.size());
          for (size_t i = 0; i < ref.size(); ++i)
            EXPECT_EQ(rb[i], ref[i]);
          break;
        }
    }
}
