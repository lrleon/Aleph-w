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

#include "concurrency_test_utils.H"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <vector>

using namespace Aleph::Testing;
using namespace std::chrono_literals;

TEST(ConcurrencyTestUtils, StartGateReleasesAllWorkersTogether)
{
  Deterministic_Start_Gate gate(3);
  std::atomic<size_t> passed{0};
  std::vector<std::thread> threads;

  for (size_t i = 0; i < 3; ++i)
    threads.emplace_back([&]
      {
        gate.arrive_and_wait();
        passed.fetch_add(1, std::memory_order_relaxed);
      });

  gate.wait_until_ready();
  EXPECT_EQ(gate.arrived(), 3u);
  EXPECT_EQ(passed.load(std::memory_order_relaxed), 0u);

  gate.release();
  for (auto &thread : threads)
    thread.join();

  EXPECT_EQ(passed.load(std::memory_order_relaxed), 3u);
}

TEST(ConcurrencyTestUtils, ProducerConsumerStressConservesValues)
{
  std::queue<size_t> queue;
  std::mutex mutex;

  Producer_Consumer_Stress_Config config;
  config.producers = 4;
  config.consumers = 3;
  config.items_per_producer = 128;
  config.timeout = 5s;

  auto result = run_producer_consumer_stress(
      config,
      [&](const size_t value)
      {
        std::lock_guard lock(mutex);
        queue.push(value);
      },
      [&](size_t &value)
      {
        std::lock_guard lock(mutex);
        if (queue.empty())
          return false;
        value = queue.front();
        queue.pop();
        return true;
      });

  ASSERT_FALSE(result.timed_out);
  ASSERT_EQ(result.size(), config.producers * config.items_per_producer);

  std::sort(result.consumed.begin(), result.consumed.end());
  for (size_t i = 0; i < result.consumed.size(); ++i)
    EXPECT_EQ(result.consumed[i], i);
}

TEST(ConcurrencyTestUtils, RandomOperationTraceIsDeterministic)
{
  const auto first = make_random_operation_trace(
      64, 12345, 11,
      {
        Trace_Operation_Kind::insert,
        Trace_Operation_Kind::erase,
        Trace_Operation_Kind::contains
      });
  const auto second = make_random_operation_trace(
      64, 12345, 11,
      {
        Trace_Operation_Kind::insert,
        Trace_Operation_Kind::erase,
        Trace_Operation_Kind::contains
      });
  const auto different = make_random_operation_trace(
      64, 54321, 11,
      {
        Trace_Operation_Kind::insert,
        Trace_Operation_Kind::erase,
        Trace_Operation_Kind::contains
      });

  EXPECT_EQ(first, second);
  EXPECT_NE(first, different);
  for (const auto &op : first)
    EXPECT_LT(op.key, 11u);
}

TEST(ConcurrencyTestUtils, TraceReplayDetectsNoMismatchForEquivalentSets)
{
  const auto trace = make_random_operation_trace(256, 99, 17);
  std::set<size_t> subject;
  std::set<size_t> reference;

  auto apply = [](std::set<size_t> &set, const Trace_Operation &op)
  {
    switch (op.kind)
      {
      case Trace_Operation_Kind::insert:
        return set.insert(op.key).second;
      case Trace_Operation_Kind::erase:
        return set.erase(op.key) != 0;
      case Trace_Operation_Kind::contains:
        return set.find(op.key) != set.end();
      default:
        return false;
      }
  };

  const auto mismatches = replay_trace_and_collect_mismatches(
      trace,
      [&](const Trace_Operation &op) { return apply(subject, op); },
      [&](const Trace_Operation &op) { return apply(reference, op); });

  EXPECT_TRUE(mismatches.empty());
}

TEST(ConcurrencyTestUtils, TraceReplayReportsMismatches)
{
  std::vector<Trace_Operation> trace =
    {
      {Trace_Operation_Kind::insert, 1, 0},
      {Trace_Operation_Kind::contains, 1, 0},
      {Trace_Operation_Kind::erase, 1, 0},
      {Trace_Operation_Kind::contains, 1, 0}
    };
  std::set<size_t> subject;
  std::set<size_t> reference;

  const auto mismatches = replay_trace_and_collect_mismatches(
      trace,
      [&](const Trace_Operation &op)
      {
        if (op.kind == Trace_Operation_Kind::insert)
          return subject.insert(op.key).second;
        if (op.kind == Trace_Operation_Kind::erase)
          return subject.erase(op.key) != 0;
        return true;
      },
      [&](const Trace_Operation &op)
      {
        if (op.kind == Trace_Operation_Kind::insert)
          return reference.insert(op.key).second;
        if (op.kind == Trace_Operation_Kind::erase)
          return reference.erase(op.key) != 0;
        return reference.find(op.key) != reference.end();
      });

  ASSERT_EQ(mismatches.size(), 1u);
  EXPECT_EQ(mismatches[0], 3u);
}
