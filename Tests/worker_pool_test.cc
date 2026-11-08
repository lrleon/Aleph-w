
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
 * @file worker_pool_test.cc
 * @brief WorkersSet, deprecated: every scheduled item is processed and
 * freed, the wait returns, and a full queue throws.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <thread>

// WorkersSet is deprecated, but supported until 8.0.0: this test is its
// coverage
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <worker_pool.H>

namespace
{
  std::atomic<int> calls{0};
  std::atomic<int> item_sum{0};

  // An item for the pool, which frees it with free()
  void * make_item(int v)
  {
    auto * p = static_cast<int *>(std::malloc(sizeof(int)));
    *p = v;
    return p;
  }

  bool add_item(void * p)
  {
    item_sum += *static_cast<int *>(p);
    ++calls;
    return false;
  }

  // Ends the job at a negative item
  bool stop_at_a_negative(void * p)
  {
    ++calls;
    return *static_cast<int *>(p) < 0;
  }

  using Pool = WorkersSet<bool (*)(void *)>;
}

TEST(WorkersSet, ProcessesEveryScheduledItem)
{
  calls = 0;
  item_sum = 0;
  Pool pool(&add_item, 100, 4);
  pool.prepare_num_workers(50);
  for (int i = 1; i <= 50; ++i)
    pool.schedule_call(make_item(i));
  pool.wait_until_all_workers_finished_or_job_is_done();
  EXPECT_EQ(calls.load(), 50);
  EXPECT_EQ(item_sum.load(), 1275);

  // is_jobs_done() is const, and locked a mutex that was not mutable: it
  // did not compile
  const Pool & cpool = pool;
  EXPECT_FALSE(cpool.is_jobs_done());
}

TEST(WorkersSet, TheWaitReturnsWhenTheItemsAreDoneBeforeIt)
{
  // The wait had no predicate: it waited forever when the items were done
  // before it started
  calls = 0;
  item_sum = 0;
  Pool pool(&add_item, 10, 2);
  pool.prepare_num_workers(3);
  for (int i = 1; i <= 3; ++i)
    pool.schedule_call(make_item(i));
  for (int k = 0; k < 500 and calls.load() < 3; ++k)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  pool.wait_until_all_workers_finished_or_job_is_done();
  EXPECT_EQ(item_sum.load(), 6);
}

TEST(WorkersSet, ItemsScheduledBeforeTheirCountAreProcessed)
{
  // prepare_num_workers() did not wake the workers: the items scheduled
  // before it waited forever
  calls = 0;
  item_sum = 0;
  Pool pool(&add_item, 10, 2);
  for (int i = 1; i <= 4; ++i)
    pool.schedule_call(make_item(i));
  // So that the workers that the items woke wait again for the count
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  pool.prepare_num_workers(4);
  pool.wait_until_all_workers_finished_or_job_is_done();
  EXPECT_EQ(item_sum.load(), 10);
}

TEST(WorkersSet, AnItemThatReturnsTrueEndsTheJob)
{
  // job_done was written without the lock, and the items after it were
  // not freed
  calls = 0;
  Pool pool(&stop_at_a_negative, 100, 1);
  pool.prepare_num_workers(20);
  pool.schedule_call(make_item(-1));
  for (int i = 1; i < 20; ++i)
    pool.schedule_call(make_item(i));
  pool.wait_until_all_workers_finished_or_job_is_done();
  EXPECT_TRUE(pool.is_jobs_done());
  EXPECT_GE(calls.load(), 1);
}

TEST(WorkersSet, AFullQueueThrows)
{
  // The queue took more items than its size, which an assert only checked
  calls = 0;
  item_sum = 0;
  Pool pool(&add_item, 2, 1);
  pool.schedule_call(make_item(1));   // no count yet: the items wait
  pool.schedule_call(make_item(2));
  void * third = make_item(3);
  EXPECT_THROW(pool.schedule_call(third), std::overflow_error);
  std::free(third);                   // the pool did not take it
  // The destructor frees the two items that are still queued
}

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif
