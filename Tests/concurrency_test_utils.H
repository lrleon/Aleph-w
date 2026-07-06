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

#include <ah-errors.H>

namespace Aleph
{
namespace Testing
{

/** @brief One-shot deterministic start gate for test worker threads.
 *
 *  All participants call `arrive_and_wait()` and block until the last one
 *  arrives, so a test can launch several worker threads and have them begin
 *  their timed/racy section as simultaneously as the scheduler allows,
 *  instead of racing from staggered `std::thread` start times.
 *
 *  @par Thread-safety
 *  `arrive_and_wait()` and `cancel()` may be called concurrently from any
 *  number of threads; both synchronize internally on the gate's own mutex.
 *  The constructor is not thread-safe with respect to concurrent use of the
 *  same (not-yet-published) instance, matching ordinary construction rules.
 *
 *  @par Complexity
 *  `arrive_and_wait()` and `cancel()` are O(1) plus the cost of waking up
 *  to `num_participants` blocked threads (a single `notify_all()`).
 */
class StartGate
{
  const size_t participants;
  size_t waiting = 0;
  bool open = false;
  bool cancelled = false;
  std::mutex mutex;
  std::condition_variable cv;

public:
  /** @brief Builds a gate that opens when `num_participants` threads arrive.
   *
   *  @param[in] num_participants number of worker threads expected.
   *  @throw std::invalid_argument if `num_participants` is zero.
   */
  explicit StartGate(const size_t num_participants)
    : participants(num_participants)
  {
    ah_invalid_argument_if(participants == 0)
      << "StartGate requires at least one participant";
  }

  /** @brief Blocks until all participants have arrived, or the gate is
   *         cancelled.
   *
   *  @pre Called at most once per logical participant (calling it more
   *       times than `num_participants` from distinct threads means more
   *       than `num_participants` threads will block here, and the gate
   *       never opens on its own).
   *  @post On a `true` return, every participant's `arrive_and_wait()` has
   *        also returned or is about to return -- the gate does not open
   *        for only some of them.
   *  @return `true` if the gate opened normally (all participants
   *          arrived); `false` if `cancel()` unblocked this call instead.
   *  @note Does not throw; blocking is done through
   *        `std::condition_variable::wait`, which only propagates spurious
   *        wakeups internally (handled by the wait predicate) and does not
   *        surface exceptions in normal use.
   */
  bool arrive_and_wait()
  {
    std::unique_lock lock(mutex);
    if (cancelled)
      return false;

    ++waiting;
    if (waiting == participants)
      {
        open = true;
        cv.notify_all();
        return true;
      }

    cv.wait(lock, [this] { return open or cancelled; });
    return open;
  }

  /** @brief Aborts the gate, releasing every participant currently blocked
   *         in `arrive_and_wait()` (and any that calls it afterward).
   *
   *  Released participants observe `arrive_and_wait()` returning `false`
   *  instead of waiting forever. Intended for exception-safety cleanup:
   *  if launching one worker thread of a cohort fails partway through
   *  (see `run_concurrently()`), the threads already started would
   *  otherwise block on this gate indefinitely, since the full
   *  `num_participants` will never arrive.
   *
   *  @post `open` remains as it was (cancelling an already-open gate is a
   *        harmless no-op); further `arrive_and_wait()` calls return
   *        `false` immediately.
   */
  void cancel() noexcept
  {
    std::lock_guard lock(mutex);
    cancelled = true;
    cv.notify_all();
  }
};

namespace concurrency_test_utils_detail
{

/** @brief RAII guard that leaves no thread joinable when it goes out of
 *         scope, cancelling `gate` first so any thread still blocked in
 *         `StartGate::arrive_and_wait()` wakes up instead of waiting
 *         forever.
 *
 *  Constructing a cohort of worker threads with `std::vector::emplace_back`
 *  can throw partway through (typically `std::system_error` from the OS
 *  refusing to create another thread). Without this guard, the threads
 *  already started stay blocked on the shared `StartGate` -- it will never
 *  reach `num_participants` arrivals -- and when the exception unwinds the
 *  enclosing `std::vector<std::thread>`, destroying still-joinable
 *  `std::thread` objects calls `std::terminate()`.
 *
 *  Usage is a commit/rollback pattern: construct the guard right before the
 *  launch loop, call `dismiss()` right after every thread has been launched
 *  successfully, then let normal execution join the threads as before (or
 *  let the guard's destructor do it). If an exception unwinds the scope
 *  *before* `dismiss()` runs, the destructor knows the full cohort never
 *  launched, so it cancels the gate first -- unblocking every thread
 *  already waiting in `StartGate::arrive_and_wait()` -- and only then joins
 *  them. After `dismiss()`, the destructor still joins any still-joinable
 *  thread (harmless once the caller's own join loop already ran), but does
 *  not cancel the gate, since all participants are known to have been
 *  launched and will reach it on their own.
 *
 *  @note Not copyable or movable; it is meant to be a single scope-local
 *        guard around one `std::vector<std::thread>` and `StartGate` pair.
 */
class Joining_Thread_Guard
{
  std::vector<std::thread> &threads;
  StartGate &gate;
  bool launch_completed = false;

public:
  Joining_Thread_Guard(std::vector<std::thread> &threads_, StartGate &gate_) noexcept
    : threads(threads_), gate(gate_)
  {}

  Joining_Thread_Guard(const Joining_Thread_Guard &) = delete;
  Joining_Thread_Guard &operator=(const Joining_Thread_Guard &) = delete;

  /** @brief Marks the launch loop as having completed without throwing.
   *
   *  Call this once, right after the last `threads.emplace_back(...)`. It
   *  must not be called if any launch attempt failed.
   */
  void dismiss() noexcept { launch_completed = true; }

  ~Joining_Thread_Guard()
  {
    if (not launch_completed)
      gate.cancel();
    for (auto &thread : threads)
      if (thread.joinable())
        thread.join();
  }
};

} // namespace concurrency_test_utils_detail

/** @brief Runs `worker(index)` in `thread_count` threads with a shared
 *         start gate.
 *
 *  Every worker blocks on a shared `StartGate` before calling
 *  `worker(index)`, so all `thread_count` invocations begin as close to
 *  simultaneously as the scheduler allows. Any exception thrown by a
 *  worker is captured (the first one, by arrival order under the internal
 *  mutex) and rethrown from this function after every thread has joined.
 *
 *  @tparam Worker callable, invocable as `worker(size_t)`; must be safe to
 *          copy into `thread_count` closures and to call concurrently from
 *          different threads (each thread calls its own copy, so `Worker`
 *          itself need not be thread-safe unless it captures shared state).
 *  @param[in] thread_count number of worker threads to launch.
 *  @param[in] worker callable receiving the worker index in
 *             `[0, thread_count)`.
 *  @throw std::invalid_argument if `thread_count` is zero.
 *  @throw std::system_error propagated as-is if the underlying
 *         `std::thread` constructor fails to create a thread; threads
 *         already launched are joined (never left blocked on the gate or
 *         abandoned joinable) before this propagates.
 *  @note Exception safety: if a worker throws anything other than what it
 *        raises through normal execution, that exception (the first one
 *        observed) is rethrown by this function; all other workers still
 *        run to completion and are joined regardless.
 *  @note Complexity: O(thread_count) thread creations/joins plus whatever
 *        `worker` itself costs.
 */
template <class Worker>
void run_concurrently(const size_t thread_count, Worker worker)
{
  ah_invalid_argument_if(thread_count == 0)
    << "run_concurrently requires at least one thread";

  StartGate gate(thread_count);
  std::mutex exception_mutex;
  std::exception_ptr first_exception;
  std::vector<std::thread> threads;
  threads.reserve(thread_count);

  {
    concurrency_test_utils_detail::Joining_Thread_Guard guard(threads, gate);
    for (size_t i = 0; i < thread_count; ++i)
      threads.emplace_back([&, i]
      {
        if (not gate.arrive_and_wait())
          return;
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
    guard.dismiss();  // every thread launched; let them reach the gate on
                       // their own instead of racing to cancel() it here.
  } // guard destructor: on the normal path above, just joins (already-
    // dismissed); if emplace_back threw before dismiss(), cancels the gate
    // first so threads already blocked on it wake up, then joins them.

  if (first_exception != nullptr)
    std::rethrow_exception(first_exception);
}

/** @brief Runs producer and consumer workers from the same deterministic
 *         start gate.
 *
 *  The producer callable receives a producer index in `[0, producer_count)`.
 *  The consumer callable receives a consumer index in `[0, consumer_count)`.
 *  All `producer_count + consumer_count` workers share one `StartGate`, so
 *  producers and consumers start together rather than producers racing
 *  ahead. Exceptions are captured (the first one observed) and rethrown
 *  after all threads join.
 *
 *  @tparam Producer callable, invocable as `producer(size_t)`.
 *  @tparam Consumer callable, invocable as `consumer(size_t)`.
 *  @param[in] producer_count number of producer workers.
 *  @param[in] consumer_count number of consumer workers.
 *  @param[in] producer producer callable.
 *  @param[in] consumer consumer callable.
 *  @throw std::invalid_argument if both counts are zero.
 *  @throw std::system_error propagated as-is if the underlying
 *         `std::thread` constructor fails to create a thread; threads
 *         already launched are joined (never left blocked on the gate or
 *         abandoned joinable) before this propagates.
 *  @note Exception safety: same as `run_concurrently()` -- the first
 *        exception observed from any producer or consumer is rethrown
 *        once every thread has joined.
 *  @note Complexity: O(producer_count + consumer_count) thread
 *        creations/joins plus whatever `producer`/`consumer` cost.
 */
template <class Producer, class Consumer>
void run_producers_consumers(const size_t producer_count,
                             const size_t consumer_count,
                             Producer producer, Consumer consumer)
{
  const size_t total = producer_count + consumer_count;
  ah_invalid_argument_if(total == 0)
    << "run_producers_consumers requires workers";

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

  {
    concurrency_test_utils_detail::Joining_Thread_Guard guard(threads, gate);

    for (size_t i = 0; i < producer_count; ++i)
      threads.emplace_back([&, i]
      {
        if (not gate.arrive_and_wait())
          return;
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
        if (not gate.arrive_and_wait())
          return;
        try
          {
            std::invoke(consumer, i);
          }
        catch (...)
          {
            capture_exception();
          }
      });
    guard.dismiss();  // every thread launched; let them reach the gate on
                       // their own instead of racing to cancel() it here.
  } // guard destructor: on the normal path above, just joins (already-
    // dismissed); if emplace_back threw before dismiss(), cancels the gate
    // first so threads already blocked on it wake up, then joins them.

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

/** @brief One randomized operation entry for parity and stress tests.
 *
 *  `kind` selects which operation to replay; `key`/`value` are always
 *  populated (in `[0, key_range)`/`[0, value_range)` of whatever
 *  `OperationTraceConfig` produced this entry), even for operations that
 *  do not use one of them (e.g. `find` ignores `value`) -- callers decide
 *  which fields their `apply_model`/`apply_subject` callables read.
 */
struct OperationTraceEntry
{
  OperationKind kind = OperationKind::find;
  size_t key = 0;
  size_t value = 0;

  /// Structural equality of all three fields (C++20 defaulted comparison).
  bool operator == (const OperationTraceEntry &) const = default;
};

/** @brief Configuration for `make_random_operation_trace()`.
 *
 *  @var OperationTraceConfig::length
 *  Number of entries to generate. Zero is valid (produces an empty trace).
 *  @var OperationTraceConfig::key_range
 *  Exclusive upper bound for generated keys (`key` is drawn from
 *  `[0, key_range)`). Must be positive.
 *  @var OperationTraceConfig::value_range
 *  Exclusive upper bound for generated values (`value` is drawn from
 *  `[0, value_range)`). Must be positive.
 *  @var OperationTraceConfig::seed
 *  Seed for the internal `std::mt19937_64` engine; the same seed and the
 *  rest of the configuration always reproduce the same trace (see
 *  `make_random_operation_trace()`'s determinism note).
 *  @var OperationTraceConfig::operations
 *  Pool `kind` is drawn from, uniformly at random. Must be non-empty.
 */
struct OperationTraceConfig
{
  size_t length = 0;
  size_t key_range = 1;
  size_t value_range = 1;
  uint64_t seed = 0xA13F2026u;
  std::vector<OperationKind> operations =
    { OperationKind::insert, OperationKind::erase, OperationKind::find };
};

/** @brief Builds a deterministic randomized operation trace.
 *
 *  Draws `config.length` entries from a `std::mt19937_64` engine seeded
 *  with `config.seed`: `kind` is chosen uniformly from `config.operations`,
 *  `key` uniformly from `[0, config.key_range)`, and `value` uniformly from
 *  `[0, config.value_range)`.
 *
 *  @param[in] config trace generation parameters.
 *  @return generated trace entries, `config.length` of them, in generation
 *          order.
 *  @throw std::invalid_argument if `config.key_range` or
 *         `config.value_range` is zero, or if `config.operations` is empty.
 *  @note Determinism: calling this twice with equal `OperationTraceConfig`
 *        values (same `seed`, ranges, and `operations`, compared
 *        element-wise) always yields an identical trace, since the only
 *        source of randomness is the seeded engine constructed from
 *        `config` -- there is no reliance on global or time-based state.
 *  @note Complexity: O(config.length) time, and the returned vector is
 *        preallocated to `config.length` (one allocation, no reallocation
 *        growth).
 */
inline std::vector<OperationTraceEntry>
make_random_operation_trace(const OperationTraceConfig &config)
{
  ah_invalid_argument_if(config.key_range == 0)
    << "operation trace key_range must be positive";
  ah_invalid_argument_if(config.value_range == 0)
    << "operation trace value_range must be positive";
  ah_invalid_argument_if(config.operations.empty())
    << "operation trace requires operations";

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

/** @brief Replays a trace against a model and a subject and reports the
 *         first mismatch.
 *
 *  For each entry `op` in `trace`, in order: applies `op` to `model`, then
 *  to `subject`, then checks `equivalent(model, subject)`. Stops and
 *  reports the index of the first operation after which the two disagree;
 *  a typical use is a simple reference container (`model`) checked against
 *  an `Aleph` container under test (`subject`) after every step of a
 *  randomized trace from `make_random_operation_trace()`.
 *
 *  @tparam Trace any range of entries `op` accepted by both `apply_model`
 *          and `apply_subject` (typically `std::vector<OperationTraceEntry>`).
 *  @tparam Model reference/model container type.
 *  @tparam Subject container type under test.
 *  @tparam ApplyModel callable, invocable as `apply_model(model, op)`.
 *  @tparam ApplySubject callable, invocable as `apply_subject(subject, op)`.
 *  @tparam Equivalent callable, invocable as `equivalent(model, subject)`,
 *          returning a value contextually convertible to `bool`.
 *  @param[in] trace operations to replay, in order.
 *  @param[in,out] model reference model mutated by `apply_model`.
 *  @param[in,out] subject container under test mutated by `apply_subject`.
 *  @param[in] apply_model applies one trace entry to `model`.
 *  @param[in] apply_subject applies one trace entry to `subject`.
 *  @param[in] equivalent full-state parity check between `model` and
 *             `subject`, called after every replayed operation.
 *  @return index of the first mismatching operation, or `std::nullopt` if
 *          `model` and `subject` stayed equivalent through the whole trace
 *          (including the empty-trace case, which never runs the check).
 *  @throw Whatever `apply_model`, `apply_subject`, or `equivalent` throw;
 *         this function itself does not throw. No cleanup beyond the
 *         basic guarantee is provided if one of them does -- `model` and
 *         `subject` may be left mid-replay.
 *  @note Complexity: O(trace.size()) calls to `apply_model`/
 *        `apply_subject`/`equivalent`, plus whatever those cost.
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
