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

#include <gtest/gtest.h>

#include "concurrency_test_utils.H"

#include <atomic>
#include <map>
#include <stdexcept>
#include <vector>

using namespace Aleph::Testing;

TEST(ConcurrencyTestUtils, StartGateRejectsZeroParticipants)
{
  EXPECT_THROW(StartGate(0), std::invalid_argument);
}

TEST(ConcurrencyTestUtils, RunConcurrentlyStartsEveryWorker)
{
  std::atomic<size_t> count{0};

  run_concurrently(8, [&](const size_t)
  {
    count.fetch_add(1, std::memory_order_relaxed);
  });

  EXPECT_EQ(count.load(std::memory_order_relaxed), 8u);
}

TEST(ConcurrencyTestUtils, RunConcurrentlyRethrowsWorkerException)
{
  EXPECT_THROW(
    run_concurrently(4, [](const size_t index)
    {
      if (index == 2)
        throw std::runtime_error("worker failed");
    }),
    std::runtime_error);
}

TEST(ConcurrencyTestUtils, RunProducersConsumersRunsBothSides)
{
  std::atomic<size_t> producers{0};
  std::atomic<size_t> consumers{0};

  run_producers_consumers(
    3, 2,
    [&](const size_t) { producers.fetch_add(1, std::memory_order_relaxed); },
    [&](const size_t) { consumers.fetch_add(1, std::memory_order_relaxed); });

  EXPECT_EQ(producers.load(std::memory_order_relaxed), 3u);
  EXPECT_EQ(consumers.load(std::memory_order_relaxed), 2u);
}

TEST(ConcurrencyTestUtils, RandomOperationTraceIsDeterministic)
{
  OperationTraceConfig config;
  config.length = 16;
  config.key_range = 7;
  config.value_range = 11;
  config.seed = 12345;
  config.operations = { OperationKind::insert, OperationKind::erase,
                        OperationKind::find, OperationKind::update };

  const auto first = make_random_operation_trace(config);
  const auto second = make_random_operation_trace(config);

  ASSERT_EQ(first, second);
  ASSERT_EQ(first.size(), 16u);
  for (const auto &op : first)
    {
      EXPECT_LT(op.key, 7u);
      EXPECT_LT(op.value, 11u);
    }
}

TEST(ConcurrencyTestUtils, RandomOperationTraceValidatesConfiguration)
{
  OperationTraceConfig config;
  config.length = 1;

  config.key_range = 0;
  EXPECT_THROW((void) make_random_operation_trace(config), std::invalid_argument);

  config.key_range = 1;
  config.value_range = 0;
  EXPECT_THROW((void) make_random_operation_trace(config), std::invalid_argument);

  config.value_range = 1;
  config.operations.clear();
  EXPECT_THROW((void) make_random_operation_trace(config), std::invalid_argument);
}

TEST(ConcurrencyTestUtils, ReplayParityPassesMatchingStates)
{
  std::vector<OperationTraceEntry> trace =
    {
      { OperationKind::insert, 3, 30 },
      { OperationKind::insert, 4, 40 },
      { OperationKind::erase, 3, 0 }
    };

  std::map<size_t, size_t> model;
  std::map<size_t, size_t> subject;

  const auto mismatch = first_state_mismatch_after_replay(
    trace, model, subject,
    [](auto &map, const OperationTraceEntry &op)
    {
      if (op.kind == OperationKind::insert)
        map[op.key] = op.value;
      else if (op.kind == OperationKind::erase)
        map.erase(op.key);
    },
    [](auto &map, const OperationTraceEntry &op)
    {
      if (op.kind == OperationKind::insert)
        map[op.key] = op.value;
      else if (op.kind == OperationKind::erase)
        map.erase(op.key);
    },
    [](const auto &lhs, const auto &rhs) { return lhs == rhs; });

  EXPECT_FALSE(mismatch.has_value());
}

TEST(ConcurrencyTestUtils, ReplayParityReportsFirstMismatch)
{
  std::vector<OperationTraceEntry> trace =
    {
      { OperationKind::insert, 1, 10 },
      { OperationKind::insert, 2, 20 },
      { OperationKind::erase, 1, 0 }
    };

  std::map<size_t, size_t> model;
  std::map<size_t, size_t> subject;

  const auto mismatch = first_state_mismatch_after_replay(
    trace, model, subject,
    [](auto &map, const OperationTraceEntry &op)
    {
      if (op.kind == OperationKind::insert)
        map[op.key] = op.value;
      else if (op.kind == OperationKind::erase)
        map.erase(op.key);
    },
    [](auto &map, const OperationTraceEntry &op)
    {
      if (op.kind == OperationKind::insert)
        map[op.key] = op.key == 2 ? 99 : op.value;
      else if (op.kind == OperationKind::erase)
        map.erase(op.key);
    },
    [](const auto &lhs, const auto &rhs) { return lhs == rhs; });

  ASSERT_TRUE(mismatch.has_value());
  EXPECT_EQ(*mismatch, 1u);
}
