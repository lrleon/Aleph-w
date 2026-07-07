
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

#include <algorithm>
#include <atomic>
#include <chrono>
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
  MpscQueue<size_t> q;

  Producer_Consumer_Stress_Config config;
  config.producers = 1;
  config.consumers = 1;
  config.items_per_producer = 20000;
  config.timeout = std::chrono::seconds(30);

  const auto result = run_producer_consumer_stress(
    config,
    [&](size_t value) { q.push(value); },
    [&](size_t &out) { return q.try_pop(out); });

  ASSERT_FALSE(result.timed_out);
  ASSERT_EQ(result.size(), config.items_per_producer);
  // Single producer, single consumer: consumption order (not just the set
  // of values) must exactly match push order.
  for (size_t i = 0; i < result.consumed.size(); ++i)
    EXPECT_EQ(result.consumed[i], i);

  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, MultiProducerSingleConsumerConservesEveryElementExactlyOnce)
{
  constexpr size_t producer_count = 6;
  constexpr size_t items_per_producer = 20000;

  MpscQueue<size_t> q;

  Producer_Consumer_Stress_Config config;
  config.producers = producer_count;
  config.consumers = 1;  // MpscQueue supports exactly one consumer.
  config.items_per_producer = items_per_producer;
  config.timeout = std::chrono::seconds(30);

  // The framework encodes each pushed value as
  // `producer_index * items_per_producer + local_sequence`, so both the
  // exactly-once conservation check (via sorting) and the FIFO-per-producer
  // check (via the unsorted, consumption-ordered `result.consumed`) can be
  // derived directly from the returned values.
  const auto result = run_producer_consumer_stress(
    config,
    [&](size_t value) { q.push(value); },
    [&](size_t &out) { return q.try_pop(out); });

  ASSERT_FALSE(result.timed_out);
  const size_t total = producer_count * items_per_producer;
  ASSERT_EQ(result.size(), total);

  std::vector<long long> last_seq(producer_count, -1);
  for (const size_t value : result.consumed)
    {
      const size_t producer = value / items_per_producer;
      const size_t seq = value % items_per_producer;
      ASSERT_LT(producer, producer_count);
      ASSERT_GT(static_cast<long long>(seq), last_seq[producer])
        << "producer " << producer << " delivered out of FIFO order";
      last_seq[producer] = static_cast<long long>(seq);
    }

  auto sorted = result.consumed;
  std::sort(sorted.begin(), sorted.end());
  for (size_t i = 0; i < total; ++i)
    ASSERT_EQ(sorted[i], i) << "index " << i << " missing or duplicated";

  EXPECT_TRUE(q.is_empty());
}

TEST(MpscQueue, RandomizedOperationTraceMatchesSequentialReferenceModel)
{
  // Single-producer trace replayed against std::vector-as-model semantics:
  // every insert corresponds to a push, and pops happen in strict FIFO
  // order matching the model's own front-to-back consumption.
  const auto trace = make_random_operation_trace(
    5000, 0xC0FFEE, 1000, {Trace_Operation_Kind::insert});

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