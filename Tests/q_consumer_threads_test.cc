
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
 * @file q_consumer_threads_test.cc
 * @brief QueueTheadsPool, deprecated: its threads consume the queue, its
 * destructor shuts it down, and an event owned by another thread starts no
 * thread.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

// QueueTheadsPool is deprecated, but supported until 8.0.0: this test is
// its coverage
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <q-consumer-threads.H>

namespace
{
  using Pool = QueueTheadsPool<int>;

  // Adds the items that its thread takes
  struct Adder : Pool::Event
  {
    std::atomic<int> * sum = nullptr;
    void run() override { *sum += item; }
  };

  void wait_for(const std::atomic<int> & value, int expected)
  {
    for (int k = 0; k < 1000 and value.load() != expected; ++k)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

TEST(QueueTheadsPool, ItsThreadsConsumeEveryItem)
{
  std::atomic<int> sum{0};
  Pool pool;
  Adder e1, e2;
  e1.sum = e2.sum = &sum;
  pool.create_thread(&e1);
  pool.create_thread(&e2);
  for (int i = 1; i <= 100; ++i)
    EXPECT_TRUE(pool.put(int(i)));
  pool.resume();
  wait_for(sum, 5050);
  EXPECT_EQ(sum.load(), 5050);
  pool.shutdown();
  EXPECT_TRUE(pool.is_shutdown());
}

TEST(QueueTheadsPool, TheDestructorShutsThePoolDown)
{
  // It threw when shutdown() had not been called, which ended the program
  std::atomic<int> sum{0};
  {
    Adder e;                          // it outlives the pool, whose thread
    e.sum = &sum;                     // writes its status after run()
    Pool pool;
    pool.create_thread(&e);
    EXPECT_TRUE(pool.put(7));
    pool.resume();
    wait_for(sum, 7);
  }
  EXPECT_EQ(sum.load(), 7);
}

TEST(QueueTheadsPool, AnEventOfAnotherThreadStartsNoThread)
{
  // The thread started before the event was checked: two threads ran with
  // it, and the one that the pool did not count left it with a negative
  // number of threads after shutdown()
  std::atomic<int> sum{0};
  Pool pool;
  Adder e;
  e.sum = &sum;
  pool.create_thread(&e);
  EXPECT_THROW(pool.create_thread(&e), std::domain_error);
  EXPECT_TRUE(pool.put(5));
  pool.resume();
  wait_for(sum, 5);
  pool.shutdown();
  EXPECT_TRUE(pool.is_shutdown());
  EXPECT_EQ(sum.load(), 5);
}

TEST(QueueTheadsPool, PutWithoutThreadsFails)
{
  Pool pool;
  EXPECT_FALSE(pool.put(1));
}

TEST(QueueTheadsPool, ASuspendedPoolShutsDown)
{
  // Its threads did not leave their wait while it was suspended, as it is
  // until resume(): shutdown(), and so the destructor, waited forever
  std::atomic<int> sum{0};
  {
    Adder e;                          // it outlives the pool
    e.sum = &sum;
    Pool pool;
    pool.create_thread(&e);
    EXPECT_TRUE(pool.put(1));
  }
  EXPECT_EQ(sum.load(), 0);           // never resumed: nothing consumed

  Adder e1, e2;
  e1.sum = e2.sum = &sum;
  Pool pool;
  pool.create_thread(&e1);
  pool.create_thread(&e2);
  pool.resume();
  EXPECT_TRUE(pool.put(2));
  wait_for(sum, 2);
  pool.suspend();
  EXPECT_TRUE(pool.put(3));
  pool.shutdown();
  EXPECT_TRUE(pool.is_shutdown());
  EXPECT_EQ(sum.load(), 2);           // the item put while suspended stays
}

TEST(QueueTheadsPool, AnEventShutsItsPoolDown)
{
  // An event that called shutdown() waited for its own thread, forever,
  // also when another thread had started the shutdown
  struct Stopper : Pool::Event
  {
    Pool * pool = nullptr;
    std::atomic<bool> started{false};
    std::atomic<bool> go{true};
    std::atomic<int> runs{0};
    void run() override
    {
      started = true;
      while (not go)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      pool->shutdown();
      ++runs;
    }
  };

  // The event starts the shutdown
  {
    Stopper e;
    Pool pool;
    e.pool = &pool;
    pool.create_thread(&e);
    EXPECT_TRUE(pool.put(1));
    pool.resume();
    for (int k = 0; k < 1000 and e.runs.load() == 0; ++k)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(e.runs.load(), 1);
    pool.shutdown();
    EXPECT_TRUE(pool.is_shutdown());
  }

  // Another thread starts it while the event runs, and waits for it
  Stopper e;
  e.go = false;
  Pool pool;
  e.pool = &pool;
  pool.create_thread(&e);
  EXPECT_TRUE(pool.put(1));
  pool.resume();
  for (int k = 0; k < 1000 and not e.started.load(); ++k)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::thread other([&pool] { pool.shutdown(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  e.go = true;
  other.join();
  EXPECT_EQ(e.runs.load(), 1);
  EXPECT_TRUE(pool.is_shutdown());
}

TEST(ConsumerQueueEvent, ItsRunIsVirtual)
{
  // run() was pure but not virtual: an event of it did not compile
  struct Event : ConsumerQueueEvent<int>
  {
    bool run() override { return item > 0; }
  };
  Event e;
  e.item = 3;
  ConsumerQueueEvent<int> & base = e;
  EXPECT_TRUE(base.run());
}

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif
