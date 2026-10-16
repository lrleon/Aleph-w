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
 * @file thread_pool_start_failure_test.cc
 * @brief Tests for a ThreadPool whose worker threads cannot all be started.
 *
 * The global operator new is replaced so that one allocation of the test
 * thread fails on demand. Starting a std::thread allocates the state of the
 * new thread with it in libstdc++, libc++ and the MSVC STL, so a failed
 * allocation is a worker that cannot be started, without exhausting the
 * resources of the system. The standard does not require that allocation;
 * the tests check it first and are skipped without it.
 */

#include <gtest/gtest.h>
#include <thread_pool.H>

#include <cstddef>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <thread>

// ThreadSanitizer has its own global operator new and delete, which collide
// with the ones below at link time; these tests are skipped under it.
#if defined(__SANITIZE_THREAD__)
#  define ALEPH_START_FAILURE_TEST_UNDER_TSAN 1
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define ALEPH_START_FAILURE_TEST_UNDER_TSAN 1
#  endif
#endif
#ifndef ALEPH_START_FAILURE_TEST_UNDER_TSAN
#  define ALEPH_START_FAILURE_TEST_UNDER_TSAN 0
#endif

namespace
{
  // successful allocations of this thread before one fails; -1 never fails
  thread_local int allocations_before_failure = -1;

  // allocations of this thread, counted while it is not negative
  thread_local long counted_allocations = -1;

  void fail_allocation_after(const int n) noexcept { allocations_before_failure = n; }

  void stop_failing() noexcept { allocations_before_failure = -1; }

  // Whether starting a std::thread allocates through the global operator new
  bool thread_start_allocates()
  {
    counted_allocations = 0;
    std::thread([] {}).join();
    const bool allocates = counted_allocations > 0;
    counted_allocations = -1;
    return allocates;
  }

#if !ALEPH_START_FAILURE_TEST_UNDER_TSAN
  void * allocate(std::size_t size)
  {
    if (counted_allocations >= 0)
      ++counted_allocations;

    if (allocations_before_failure == 0)
      {
        allocations_before_failure = -1;
        throw std::bad_alloc();
      }
    if (allocations_before_failure > 0)
      --allocations_before_failure;

    if (void * ptr = std::malloc(size == 0 ? 1 : size))
      return ptr;
    throw std::bad_alloc();
  }
#endif
}

#if !ALEPH_START_FAILURE_TEST_UNDER_TSAN

void * operator new(std::size_t size) { return allocate(size); }

void * operator new[](std::size_t size) { return allocate(size); }

void * operator new(std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void * operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void operator delete(void * ptr) noexcept { std::free(ptr); }

void operator delete[](void * ptr) noexcept { std::free(ptr); }

void operator delete(void * ptr, std::size_t) noexcept { std::free(ptr); }

void operator delete[](void * ptr, std::size_t) noexcept { std::free(ptr); }

void operator delete(void * ptr, const std::nothrow_t &) noexcept { std::free(ptr); }

void operator delete[](void * ptr, const std::nothrow_t &) noexcept { std::free(ptr); }

#endif  // !ALEPH_START_FAILURE_TEST_UNDER_TSAN

using namespace Aleph;

// A worker that could not be started left the ones started before it running
// on members that were being destroyed: std::terminate, or a hang. Now the
// constructor stops and joins them, and rethrows. Every allocation of the
// constructor fails in turn, so each worker fails to start once.
TEST(ThreadPoolStartFailure, ConstructorStopsTheStartedWorkersAndRethrows)
{
  if (ALEPH_START_FAILURE_TEST_UNDER_TSAN)
    GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  if (not thread_start_allocates())
    GTEST_SKIP() << "std::thread does not allocate through operator new here";

  int failures = 0;
  for (int n = 0;; ++n)
    {
      ASSERT_LT(n, 100) << "the constructor never stopped allocating";
      fail_allocation_after(n);
      try
        {
          ThreadPool pool(4);
          stop_failing();
          EXPECT_EQ(pool.num_threads(), 4u);
          EXPECT_EQ(pool.enqueue([] { return 42; }).get(), 42);
          break;
        }
      catch (const std::bad_alloc &)
        {
          stop_failing();
          ++failures;
        }
    }

  // at least the state of each of the 4 threads
  EXPECT_GE(failures, 4);
}

// A failed resize() left the pool with the new workers it could start, or
// with none and not stopped, so that a task would wait forever. Now it stops
// and joins them, and the pool is stopped.
TEST(ThreadPoolStartFailure, FailedResizeLeavesThePoolStopped)
{
  if (ALEPH_START_FAILURE_TEST_UNDER_TSAN)
    GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
  if (not thread_start_allocates())
    GTEST_SKIP() << "std::thread does not allocate through operator new here";

  int failures = 0;
  for (int n = 0;; ++n)
    {
      ASSERT_LT(n, 100) << "resize() never stopped allocating";
      ThreadPool pool(2);
      fail_allocation_after(n);
      try
        {
          pool.resize(5);
          stop_failing();
          EXPECT_EQ(pool.num_threads(), 5u);
          EXPECT_EQ(pool.enqueue([] { return 42; }).get(), 42);
          break;
        }
      catch (const std::bad_alloc &)
        {
          stop_failing();
          ++failures;
          EXPECT_TRUE(pool.is_stopped()) << n;
          EXPECT_EQ(pool.num_threads(), 0u) << n;
          EXPECT_TRUE(pool.is_idle()) << n;
          EXPECT_THROW((void) pool.enqueue([] { return 0; }), std::runtime_error) << n;
        }
    }

  // at least the state of each of the 5 new threads
  EXPECT_GE(failures, 5);
}
