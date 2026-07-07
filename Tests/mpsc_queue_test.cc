
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
 * @file mpsc_queue_test.cc
 * @brief Tests for Aleph::MpscQueue (tpl_mpsc_queue.H).
 */

#include <gtest/gtest.h>

#include "concurrency_test_utils.H"

#include <tpl_mpsc_queue.H>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Aleph;
using namespace Aleph::Testing;

namespace
{
  // Thread-safe lifetime counter (concurrent constructions/destructions
  // happen in the multi-producer tests below, unlike single-threaded
  // lifetime probes elsewhere in the suite).
  struct Probe
  {
    static std::atomic<int> live;
    int value = 0;

    Probe() noexcept { live.fetch_add(1, std::memory_order_relaxed); }
    explicit Probe(int v) noexcept : value(v)
    {
      live.fetch_add(1, std::memory_order_relaxed);
    }
    Probe(const Probe &p) noexcept : value(p.value)
    {
      live.fetch_add(1, std::memory_order_relaxed);
    }
    Probe(Probe &&p) noexcept : value(p.value)
    {
      live.fetch_add(1, std::memory_order_relaxed);
    }
    Probe & operator=(const Probe &) = default;
    Probe & operator=(Probe &&) noexcept = default;
    ~Probe() { live.fetch_sub(1, std::memory_order_relaxed); }
  };

  std::atomic<int> Probe::live{0};

  struct Throwing_Ctor
  {
    static bool should_throw;
    int value;

    explicit Throwing_Ctor(int v) : value(v)
    {
      if (should_throw)
        throw std::runtime_error("Throwing_Ctor: constructor failed");
    }
  };

  bool Throwing_Ctor::should_throw = false;
}  // namespace

TEST(MpscQueue, DefaultConstructedQueueIsEmpty)
{
  MpscQueue<int> q;
  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, PushAndPopSingleThreaded)
{
  MpscQueue<int> q;
  q.push(1);
  q.push(2);
  q.emplace(3);
  EXPECT_FALSE(q.is_empty());

  int out = 0;
  ASSERT_TRUE(q.try_pop(out));
  EXPECT_EQ(out, 1);
  ASSERT_TRUE(q.try_pop(out));
  EXPECT_EQ(out, 2);
  ASSERT_TRUE(q.try_pop(out));
  EXPECT_EQ(out, 3);

  EXPECT_FALSE(q.try_pop(out));
  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, TryPopOptionalOverload)
{
  MpscQueue<std::string> q;
  q.push(std::string("alpha"));
  q.push(std::string("beta"));

  auto a = q.try_pop();
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(*a, "alpha");

  auto b = q.try_pop();
  ASSERT_TRUE(b.has_value());
  EXPECT_EQ(*b, "beta");

  auto c = q.try_pop();
  EXPECT_FALSE(c.has_value());
}

TEST(MpscQueue, FifoOrderIsPreservedForASingleProducer)
{
  MpscQueue<int> q;
  constexpr int N = 1000;
  for (int i = 0; i < N; ++i)
    q.push(i);

  for (int i = 0; i < N; ++i)
    {
      int out = -1;
      ASSERT_TRUE(q.try_pop(out));
      EXPECT_EQ(out, i);
    }
  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, MoveOnlyPayloadIsSupported)
{
  MpscQueue<std::unique_ptr<int>> q;
  q.push(std::make_unique<int>(10));
  q.emplace(std::make_unique<int>(20));

  std::unique_ptr<int> out;
  ASSERT_TRUE(q.try_pop(out));
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(*out, 10);

  auto opt = q.try_pop();
  ASSERT_TRUE(opt.has_value());
  ASSERT_NE(*opt, nullptr);
  EXPECT_EQ(**opt, 20);
}

TEST(MpscQueue, ElementLifetimesAreBalancedAcrossPushAndPop)
{
  ASSERT_EQ(Probe::live.load(), 0);
  Probe out;  // outlives the inner scope below, so it can carry a live
              // count past the queue's destructor for the final check.
  {
    MpscQueue<Probe> q;
    q.emplace(1);
    q.emplace(2);
    q.emplace(3);
    EXPECT_EQ(Probe::live.load(), 4);  // 3 queued + out

    ASSERT_TRUE(q.try_pop(out));
    EXPECT_EQ(out.value, 1);
    // The popped-into `out` and the two still-queued elements are alive.
    EXPECT_EQ(Probe::live.load(), 3);
  }
  // Queue destructor released the two still-queued nodes; `out` (declared
  // above, still alive here) keeps the count at 1.
  EXPECT_EQ(Probe::live.load(), 1);
}

TEST(MpscQueue, DestructorReleasesAllRemainingElements)
{
  ASSERT_EQ(Probe::live.load(), 0);
  {
    MpscQueue<Probe> q;
    for (int i = 0; i < 50; ++i)
      q.emplace(i);
    EXPECT_EQ(Probe::live.load(), 50);
  }
  EXPECT_EQ(Probe::live.load(), 0);
}

TEST(MpscQueue, FailedEmplaceLeavesQueueUnchanged)
{
  MpscQueue<Throwing_Ctor> q;
  q.emplace(1);
  EXPECT_FALSE(q.is_empty());

  Throwing_Ctor::should_throw = true;
  EXPECT_THROW(q.emplace(2), std::runtime_error);
  Throwing_Ctor::should_throw = false;

  // The successful push before the throwing one must still be there,
  // and nothing from the failed attempt should have been enqueued.
  Throwing_Ctor out(0);
  ASSERT_TRUE(q.try_pop(out));
  EXPECT_EQ(out.value, 1);
  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, SingleProducerSingleConsumerConservesAllElements)
{
  constexpr int N = 20000;
  MpscQueue<int> q;
  std::atomic<bool> producer_done{false};

  run_concurrently(2, [&](size_t idx)
  {
    if (idx == 0)
      {
        for (int i = 0; i < N; ++i)
          q.push(i);
        producer_done.store(true, std::memory_order_release);
      }
    else
      {
        int expected = 0;
        while (expected < N)
          {
            int out;
            if (q.try_pop(out))
              {
                EXPECT_EQ(out, expected);
                ++expected;
              }
          }
      }
  });

  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, MultiProducerSingleConsumerConservesEveryElementExactlyOnce)
{
  constexpr int producer_count = 6;
  constexpr int items_per_producer = 20000;
  constexpr long long total = static_cast<long long>(producer_count) * items_per_producer;

  MpscQueue<long long> q;
  std::vector<int> last_seq(producer_count, -1);
  std::vector<char> seen(static_cast<size_t>(total), 0);
  long long consumed = 0;

  run_producers_consumers(
    producer_count, 1,
    [&](size_t p)
    {
      for (int i = 0; i < items_per_producer; ++i)
        q.push(static_cast<long long>(p) * items_per_producer + i);
    },
    [&](size_t /*consumer_index*/)
    {
      while (consumed < total)
        {
          long long v;
          if (not q.try_pop(v))
            continue;  // transient empty: race window or producers still working

          const long long producer = v / items_per_producer;
          const long long seq = v % items_per_producer;
          ASSERT_GE(producer, 0);
          ASSERT_LT(producer, producer_count);
          ASSERT_GT(seq, last_seq[static_cast<size_t>(producer)]);
          last_seq[static_cast<size_t>(producer)] = static_cast<int>(seq);

          const size_t idx = static_cast<size_t>(producer) * items_per_producer + seq;
          ASSERT_FALSE(seen[idx]);
          seen[idx] = 1;
          ++consumed;
        }
    });

  EXPECT_EQ(consumed, total);
  EXPECT_TRUE(q.is_empty());
  for (long long i = 0; i < total; ++i)
    ASSERT_TRUE(seen[static_cast<size_t>(i)]) << "index " << i << " never consumed";
}

TEST(MpscQueue, RandomizedOperationTraceMatchesSequentialReferenceModel)
{
  // Single-producer trace replayed against std::vector-as-model semantics:
  // every insert corresponds to a push, and pops happen in strict FIFO
  // order matching the model's own front-to-back consumption.
  OperationTraceConfig config;
  config.length = 5000;
  config.key_range = 1000;
  config.value_range = 1;
  config.seed = 0xC0FFEE;
  config.operations = {OperationKind::insert};

  const auto trace = make_random_operation_trace(config);

  std::vector<size_t> model;
  MpscQueue<size_t> subject;

  for (const auto &op : trace)
    {
      model.push_back(op.key);
      subject.push(op.key);
    }

  for (size_t expected : model)
    {
      size_t out = 0;
      ASSERT_TRUE(subject.try_pop(out));
      EXPECT_EQ(out, expected);
    }
  EXPECT_TRUE(subject.is_empty());
}