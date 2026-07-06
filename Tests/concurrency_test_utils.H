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
 * @file concurrency_test_utils.H
 * @brief Reusable helpers for concurrent data-structure tests.
 */

#ifndef ALEPH_CONCURRENCY_TEST_UTILS_H
#define ALEPH_CONCURRENCY_TEST_UTILS_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace Aleph
{
namespace Testing
{

/** One-shot deterministic start gate for test worker threads. */
class StartGate
{
  const size_t participants;
  size_t waiting = 0;
  bool open = false;
  std::mutex mutex;
  std::condition_variable cv;

public:
  /** Builds a gate that opens when `num_participants` threads arrive.

      @param[in] num_participants number of worker threads expected.
      @throw std::invalid_argument if `num_participants` is zero.
  */
  explicit StartGate(const size_t num_participants)
    : participants(num_participants)
  {
    if (participants == 0)
      throw std::invalid_argument("StartGate requires at least one participant");
  }

  /** Blocks until all participants have arrived. */
  void arrive_and_wait()
  {
    std::unique_lock lock(mutex);
    ++waiting;
    if (waiting == participants)
      {
        open = true;
        cv.notify_all();
        return;
      }

    cv.wait(lock, [this] { return open; });
  }
};

/** Runs `worker(index)` in `thread_count` threads with a shared start gate.

    Any exception thrown by a worker is rethrown after all threads join.

    @param[in] thread_count number of worker threads to launch.
    @param[in] worker callable receiving the worker index.
    @throw std::invalid_argument if `thread_count` is zero.
*/
template <class Worker>
void run_concurrently(const size_t thread_count, Worker worker)
{
  if (thread_count == 0)
    throw std::invalid_argument("run_concurrently requires at least one thread");

  StartGate gate(thread_count);
  std::mutex exception_mutex;
  std::exception_ptr first_exception;
  std::vector<std::thread> threads;
  threads.reserve(thread_count);

  for (size_t i = 0; i < thread_count; ++i)
    threads.emplace_back([&, i]
    {
      gate.arrive_and_wait();
      try
        {
          std::invoke(worker, i);
        }
      catch (...)
        {
          std::lock_guard lock(exception_mutex);
          if (first_exception == nullptr)
            first_exception = std::current_exception();
        }
    });

  for (auto &thread : threads)
    thread.join();

  if (first_exception != nullptr)
    std::rethrow_exception(first_exception);
}

/** Runs producer and consumer workers from the same deterministic start gate.

    The producer callable receives a producer index in `[0, producer_count)`.
    The consumer callable receives a consumer index in `[0, consumer_count)`.
    Exceptions are rethrown after all threads join.

    @param[in] producer_count number of producer workers.
    @param[in] consumer_count number of consumer workers.
    @param[in] producer producer callable.
    @param[in] consumer consumer callable.
    @throw std::invalid_argument if both counts are zero.
*/
template <class Producer, class Consumer>
void run_producers_consumers(const size_t producer_count,
                             const size_t consumer_count,
                             Producer producer, Consumer consumer)
{
  const size_t total = producer_count + consumer_count;
  if (total == 0)
    throw std::invalid_argument("run_producers_consumers requires workers");

  StartGate gate(total);
  std::mutex exception_mutex;
  std::exception_ptr first_exception;
  std::vector<std::thread> threads;
  threads.reserve(total);

  auto capture_exception = [&]
  {
    std::lock_guard lock(exception_mutex);
    if (first_exception == nullptr)
      first_exception = std::current_exception();
  };

  for (size_t i = 0; i < producer_count; ++i)
    threads.emplace_back([&, i]
    {
      gate.arrive_and_wait();
      try
        {
          std::invoke(producer, i);
        }
      catch (...)
        {
          capture_exception();
        }
    });

  for (size_t i = 0; i < consumer_count; ++i)
    threads.emplace_back([&, i]
    {
      gate.arrive_and_wait();
      try
        {
          std::invoke(consumer, i);
        }
      catch (...)
        {
          capture_exception();
        }
    });

  for (auto &thread : threads)
    thread.join();

  if (first_exception != nullptr)
    std::rethrow_exception(first_exception);
}

/** Operation kinds used by randomized data-structure parity traces. */
enum class OperationKind
{
  insert,
  erase,
  find,
  update,
  push,
  pop
};

/** One randomized operation entry for parity and stress tests. */
struct OperationTraceEntry
{
  OperationKind kind = OperationKind::find;
  size_t key = 0;
  size_t value = 0;

  bool operator == (const OperationTraceEntry &) const = default;
};

/** Configuration for `make_random_operation_trace()`. */
struct OperationTraceConfig
{
  size_t length = 0;
  size_t key_range = 1;
  size_t value_range = 1;
  uint64_t seed = 0xA13F2026u;
  std::vector<OperationKind> operations =
    { OperationKind::insert, OperationKind::erase, OperationKind::find };
};

/** Builds a deterministic randomized operation trace.

    @param[in] config trace generation parameters.
    @return generated trace entries.
    @throw std::invalid_argument if a range is zero or no operations are given.
*/
inline std::vector<OperationTraceEntry>
make_random_operation_trace(const OperationTraceConfig &config)
{
  if (config.key_range == 0)
    throw std::invalid_argument("operation trace key_range must be positive");
  if (config.value_range == 0)
    throw std::invalid_argument("operation trace value_range must be positive");
  if (config.operations.empty())
    throw std::invalid_argument("operation trace requires operations");

  std::mt19937_64 rng(config.seed);
  std::uniform_int_distribution<size_t> op_dist(0, config.operations.size() - 1);
  std::uniform_int_distribution<size_t> key_dist(0, config.key_range - 1);
  std::uniform_int_distribution<size_t> value_dist(0, config.value_range - 1);

  std::vector<OperationTraceEntry> trace;
  trace.reserve(config.length);
  for (size_t i = 0; i < config.length; ++i)
    trace.push_back({ config.operations[op_dist(rng)],
                      key_dist(rng),
                      value_dist(rng) });

  return trace;
}

/** Replays a trace against a model and a subject and reports first mismatch.

    `apply_model(model, op)` and `apply_subject(subject, op)` perform one
    operation. `equivalent(model, subject)` checks full-state parity after each
    operation.

    @return index of the first mismatching operation, or `std::nullopt`.
*/
template <class Trace, class Model, class Subject,
          class ApplyModel, class ApplySubject, class Equivalent>
std::optional<size_t>
first_state_mismatch_after_replay(const Trace &trace,
                                  Model &model,
                                  Subject &subject,
                                  ApplyModel apply_model,
                                  ApplySubject apply_subject,
                                  Equivalent equivalent)
{
  size_t index = 0;
  for (const auto &op : trace)
    {
      std::invoke(apply_model, model, op);
      std::invoke(apply_subject, subject, op);
      if (not std::invoke(equivalent, model, subject))
        return index;
      ++index;
    }

  return std::nullopt;
}

} // namespace Testing
} // namespace Aleph

#endif // ALEPH_CONCURRENCY_TEST_UTILS_H
