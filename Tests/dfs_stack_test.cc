
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
 * @file dfs_stack_test.cc
 * @brief Tests of Dfs_Stack, the stack of frames of the searches that do
 * not recurse.
 *
 * This program replaces the global operator new, so that a test can see
 * that a stack reuses the room that another stack of its thread kept.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <tpl_dfs_stack.H>

using namespace Aleph;

namespace
{
  // Counts the frames that live, and throws when it is built from, or
  // copied from, the value throw_at
  struct Counted
  {
    static inline long alive = 0;
    static inline long throw_at = -1;

    long value;

    explicit Counted(long v) : value(v)
    {
      if (v == throw_at)
        throw std::runtime_error("built from throw_at");
      ++alive;
    }

    // Not noexcept, so that the stack copies the frames when it grows
    Counted(const Counted & other) : value(other.value)
    {
      if (value == throw_at)
        throw std::runtime_error("copied from throw_at");
      ++alive;
    }

    ~Counted() { --alive; }
  };

  // A frame that cannot be copied, and whose move can throw
  struct Move_Only
  {
    static inline long alive = 0;
    static inline long throw_at = -1;

    long value;

    explicit Move_Only(long v) : value(v) { ++alive; }

    Move_Only(Move_Only && other) : value(other.value)
    {
      if (value == throw_at)
        throw std::runtime_error("moved from throw_at");
      other.value = -1;
      ++alive;
    }

    Move_Only(const Move_Only &) = delete;

    ~Move_Only() { --alive; }
  };

  // Resets the counters of Counted around a test
  struct CountedFrames : public ::testing::Test
  {
    void SetUp() override
    {
      Counted::alive = 0;
      Counted::throw_at = -1;
    }

    void TearDown() override { Counted::throw_at = -1; }
  };
}

TEST(DfsStack, PopsInTheReverseOrderOfThePushes)
{
  Dfs_Stack<long> stack;
  EXPECT_TRUE(stack.is_empty());
  EXPECT_EQ(stack.size(), 0u);
  for (long i = 0; i < 10000; ++i)
    {
      long & frame = stack.push(i);
      EXPECT_EQ(frame, i);
      EXPECT_EQ(&frame, &stack.top());
    }
  EXPECT_EQ(stack.size(), 10000u);
  for (long i = 9999; i >= 0; --i)
    {
      ASSERT_EQ(stack.top(), i);
      stack.pop();
    }
  EXPECT_TRUE(stack.is_empty());
}

TEST(DfsStack, GrowingKeepsTheFrames)
{
  Dfs_Stack<std::string> stack;
  for (int i = 0; i < 5000; ++i)
    stack.push(std::to_string(i) + " is a frame long enough not to fit inline");
  ASSERT_EQ(stack.size(), 5000u);
  for (int i = 4999; i >= 0; --i)
    {
      ASSERT_EQ(stack.top(), std::to_string(i) + " is a frame long enough not to fit inline");
      stack.pop();
    }
}

TEST_F(CountedFrames, ClearAndTheDestructorDestroyTheFrames)
{
  {
    Dfs_Stack<Counted> stack;
    for (long i = 0; i < 300; ++i)
      stack.push(i);
    EXPECT_EQ(Counted::alive, 300);
    stack.clear();
    EXPECT_EQ(Counted::alive, 0);
    EXPECT_TRUE(stack.is_empty());
    for (long i = 0; i < 100; ++i)
      stack.push(i);
    EXPECT_EQ(Counted::alive, 100);
  }
  EXPECT_EQ(Counted::alive, 0);
}

TEST_F(CountedFrames, AFrameThatThrowsLeavesTheStackAsItWas)
{
  Dfs_Stack<Counted> stack;
  for (long i = 0; i < 10; ++i)
    stack.push(i);
  Counted::throw_at = 77;
  EXPECT_THROW(stack.push(77), std::runtime_error);
  EXPECT_EQ(stack.size(), 10u);
  EXPECT_EQ(stack.top().value, 9);
  EXPECT_EQ(Counted::alive, 10);
}

// Without a move that cannot throw, the stack copies its frames when it
// grows; if a copy throws, the frames are those it had
TEST_F(CountedFrames, ACopyThatThrowsWhileGrowingLeavesTheStackAsItWas)
{
  Dfs_Stack<Counted> stack;
  for (long i = 0; i < 10; ++i)
    stack.push(i);

  Counted::throw_at = 5; // copying the frame 5 throws, so growing does
  long next = 10;
  bool thrown = false;
  while (not thrown and next < 10000000)
    try
      {
        stack.push(next);
        ++next;
      }
    catch (const std::runtime_error &)
      {
        thrown = true;
      }
  Counted::throw_at = -1;

  ASSERT_TRUE(thrown);
  ASSERT_EQ(stack.size(), size_t(next));
  EXPECT_EQ(Counted::alive, next);
  for (long i = next - 1; i >= 0; --i)
    {
      ASSERT_EQ(stack.top().value, i);
      stack.pop();
    }
  EXPECT_EQ(Counted::alive, 0);
}

// A frame that cannot be copied, and whose move throws while the stack
// grows: the new room went to no one, which LeakSanitizer reports
TEST(DfsStack, AMoveThatThrowsWhileGrowingFreesTheNewRoom)
{
  Move_Only::alive = 0;
  {
    Dfs_Stack<Move_Only> stack;
    for (long i = 0; i < 10; ++i)
      stack.push(i);

    Move_Only::throw_at = 5; // moving the frame 5 throws, so growing does
    long next = 10;
    bool thrown = false;
    while (not thrown and next < 10000000)
      try
        {
          stack.push(next);
          ++next;
        }
      catch (const std::runtime_error &)
        {
          thrown = true;
        }
    Move_Only::throw_at = -1;

    ASSERT_TRUE(thrown);
    EXPECT_EQ(stack.size(), size_t(next)); // the frame 5 and the next ones
    EXPECT_EQ(stack.top().value, next - 1);
    EXPECT_EQ(Move_Only::alive, next);
  }
  EXPECT_EQ(Move_Only::alive, 0);
}

TEST(DfsStack, MovesTakeTheFrames)
{
  Dfs_Stack<long> a;
  for (long i = 0; i < 1000; ++i)
    a.push(i);

  Dfs_Stack<long> b(std::move(a));
  EXPECT_TRUE(a.is_empty());
  ASSERT_EQ(b.size(), 1000u);
  EXPECT_EQ(b.top(), 999);

  Dfs_Stack<long> c;
  c.push(-1);
  c = std::move(b);
  EXPECT_TRUE(b.is_empty());
  ASSERT_EQ(c.size(), 1000u);
  EXPECT_EQ(c.top(), 999);

  // the moved from stacks still work
  a.push(5);
  b.push(6);
  EXPECT_EQ(a.top(), 5);
  EXPECT_EQ(b.top(), 6);
}

namespace
{
  // The tests need the operator new of allocation_failure.H
  struct DfsStackRoom : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  // Leaves in the room of the thread one for at least n frames
  void keep_room_for(size_t n)
  {
    Dfs_Stack<long> stack;
    for (size_t i = 0; i < n; ++i)
      stack.push(long(i));
  }
}

// A search that follows another in the same thread allocates nothing
TEST_F(DfsStackRoom, AStackOfTheSameThreadReusesTheRoom)
{
  keep_room_for(20000);
  {
    Dfs_Stack<long> stack;
    No_Memory no_memory;
    for (long i = 0; i < 20000; ++i)
      ASSERT_NO_THROW(stack.push(i));
  }
  EXPECT_EQ(refused_allocations, 0u);
}

TEST_F(DfsStackRoom, AStackOfAnotherThreadHasNoRoom)
{
  keep_room_for(20000);
  bool thrown = false;
  std::thread other([&thrown]
    {
      Dfs_Stack<long> stack;
      No_Memory no_memory;
      try
        {
          stack.push(1);
        }
      catch (const std::bad_alloc &)
        {
          thrown = true;
        }
    });
  other.join();
  EXPECT_TRUE(thrown);
}

// A room larger than 1 MiB is freed, so that it does not stay taken
TEST_F(DfsStackRoom, ALargeRoomIsNotKept)
{
  keep_room_for(1024 * 1024); // 8 MiB of frames
  Dfs_Stack<long> stack;
  No_Memory no_memory;
  EXPECT_THROW(stack.push(1), std::bad_alloc);
}
