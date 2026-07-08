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

/** @file concurrency_test_utils.H
 *  @brief Reusable helpers for concurrent data-structure tests.
 *
 *  This header is test-only infrastructure. It intentionally lives under
 *  Tests/ and must not be included by production headers.
 */

#ifndef ALEPH_TESTS_CONCURRENCY_TEST_UTILS_H
#define ALEPH_TESTS_CONCURRENCY_TEST_UTILS_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <mutex>
#include <random>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <ah-errors.H>

namespace Aleph::Testing
{
  /** @brief Single-use deterministic start gate for concurrent tests.
   *
   * Worker threads call arrive_and_wait(). The test thread calls
   * wait_until_ready() and release(). This ensures all workers are parked before
   * the measured/stressed section starts.
   *
   * @par Thread-safety
   * arrive_and_wait(), wait_until_ready(), release(), cancel(), and
   * arrived() may all be called concurrently; each synchronizes internally
   * on the gate's own mutex.
   */
  class Deterministic_Start_Gate
  {
    const size_t expected_ = 0;
    size_t arrived_ = 0;
    bool released_ = false;
    bool cancelled_ = false;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

  public:
    /** @brief Construct a gate for the given number of workers.
     *
     * @param[in] expected Number of workers that must arrive before release.
     * @throws std::invalid_argument if `expected == 0` (a gate with no
     *         participants would never distinguish "nobody has arrived
     *         yet" from "everybody has arrived", which is almost always a
     *         miscounted caller rather than an intentional no-op gate).
     */
    explicit Deterministic_Start_Gate(const size_t expected)
      : expected_(expected)
    {
      ah_invalid_argument_if(expected == 0)
        << "Deterministic_Start_Gate requires at least one participant";
    }

    /** @brief Mark the caller ready and block until release() or cancel().
     *
     * This function is intended for worker threads.
     *
     * @return `true` if the gate was released normally, `false` if
     *         cancel() unblocked this call instead.
     */
    bool arrive_and_wait()
    {
      std::unique_lock lock(mutex_);
      ++arrived_;
      cv_.notify_all();
      cv_.wait(lock, [this] { return released_ or cancelled_; });
      return released_;
    }

    /** @brief Wait until all expected workers are ready, or the gate is
     *         cancelled.
     *
     * This function is intended for the controlling test thread.
     */
    void wait_until_ready()
    {
      std::unique_lock lock(mutex_);
      cv_.wait(lock, [this] { return arrived_ == expected_ or cancelled_; });
    }

    /** @brief Release all waiting workers.
     *
     * Calling release() before all workers arrive is valid; late workers will
     * pass through immediately after incrementing the arrival count.
     */
    void release()
    {
      std::lock_guard lock(mutex_);
      released_ = true;
      cv_.notify_all();
    }

    /** @brief Abort the gate, waking every worker currently blocked in
     *         arrive_and_wait() or wait_until_ready().
     *
     * Woken participants observe arrive_and_wait() returning `false`
     * instead of waiting forever. Intended for exception-safety cleanup:
     * if launching one worker thread of a cohort fails partway through,
     * the threads already started would otherwise stay parked
     * indefinitely, since the full `expected` count will never arrive.
     *
     * @note Cancelling an already-released gate is a harmless no-op.
     */
    void cancel() noexcept
    {
      std::lock_guard lock(mutex_);
      cancelled_ = true;
      cv_.notify_all();
    }

    /** @brief Return the number of arrived workers.
     *
     * @return Number of workers that already called arrive_and_wait().
     */
    [[nodiscard]] size_t arrived() const
    {
      std::lock_guard lock(mutex_);
      return arrived_;
    }
  };

  namespace detail
  {
    /** @brief RAII guard that leaves no thread joinable when it goes out
     *         of scope, cancelling `gate` first so any thread still
     *         blocked in Deterministic_Start_Gate::arrive_and_wait()
     *         wakes up instead of waiting forever.
     *
     * Launching a cohort of worker threads with
     * `std::vector::emplace_back` can throw partway through (typically
     * `std::system_error` from the OS refusing to create another
     * thread). Without this guard, threads already started would stay
     * blocked on the shared gate -- it will never reach its full
     * expected arrival count -- and destroying a still-joinable
     * `std::thread` (as the enclosing `std::vector<std::thread>` does
     * when unwound by the exception) calls `std::terminate()`. Keeping
     * this guard alive for the entire launch-and-join scope fixes both
     * problems on any exit path, normal or exceptional.
     */
    class Joining_Thread_Guard
    {
      std::vector<std::thread> &threads_;
      Deterministic_Start_Gate &gate_;
      bool launch_completed_ = false;

    public:
      Joining_Thread_Guard(std::vector<std::thread> &threads,
                            Deterministic_Start_Gate &gate) noexcept
        : threads_(threads), gate_(gate)
      {}

      Joining_Thread_Guard(const Joining_Thread_Guard &) = delete;
      Joining_Thread_Guard &operator = (const Joining_Thread_Guard &) = delete;

      /** @brief Mark the launch loop as having completed without
       *         throwing. Call once, right after the last
       *         `threads.emplace_back(...)`. */
      void dismiss() noexcept { launch_completed_ = true; }

      ~Joining_Thread_Guard()
      {
        if (not launch_completed_)
          gate_.cancel();
        for (auto &thread : threads_)
          if (thread.joinable())
            {
              try
                {
                  thread.join();
                }
              catch (const std::system_error &)
                {
                  try
                    {
                      thread.detach();
                    }
                  catch (const std::system_error &)
                    {
                      // Nothing actionable left from a destructor.
                    }
                }
            }
      }
    };
  } // namespace detail

  /** @brief Launch `thread_count` workers with a deterministic
   *         simultaneous start, join every one of them, and rethrow the
   *         first worker exception (if any) only after all have joined.
   *
   * @tparam Worker Callable invoked as `worker(size_t index)` from its own
   *         thread, `index` in `[0, thread_count)`. Must be safe to copy
   *         into one closure per thread; each thread runs its own copy (so
   *         `Worker` itself need not be thread-safe unless it captures
   *         shared state).
   * @param[in] thread_count Number of worker threads to launch.
   * @param[in,out] worker Per-worker callable.
   *
   * @throws std::invalid_argument if `thread_count == 0` (see
   *         `Deterministic_Start_Gate`'s constructor).
   * @throws std::system_error propagated as-is if the underlying
   *         `std::thread` constructor fails to create a thread; threads
   *         already launched are unblocked and joined (never left parked
   *         on the gate or abandoned joinable) before this propagates.
   * @throws Whatever the first worker to throw at runtime propagates with
   *         (whichever worker's `catch` block first acquires the internal
   *         exception mutex -- not necessarily the one with the smallest
   *         `index`), rethrown only once every thread has been launched,
   *         released, and joined. If more than one worker throws, only
   *         the first one recorded is rethrown; the others are discarded.
   */
  template <typename Worker>
  void run_workers(const size_t thread_count, Worker worker)
  {
    Deterministic_Start_Gate gate(thread_count);
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    std::mutex exception_mutex;
    std::exception_ptr first_exception;

    {
      detail::Joining_Thread_Guard guard(threads, gate);

      for (size_t i = 0; i < thread_count; ++i)
        threads.emplace_back([&, i, worker]() mutable
          {
            if (not gate.arrive_and_wait())
              return;
            try
              {
                worker(i);
              }
            catch (...)
              {
                std::lock_guard lock(exception_mutex);
                if (not first_exception)
                  first_exception = std::current_exception();
              }
          });

      guard.dismiss();  // every thread launched; let them reach the gate
                         // on their own instead of racing to cancel() it.
      gate.wait_until_ready();
      gate.release();
    } // guard destructor joins every thread (see Joining_Thread_Guard).

    if (first_exception)
      std::rethrow_exception(first_exception);
  }

  /** @brief Configuration for producer/consumer stress helpers. */
  struct Producer_Consumer_Stress_Config
  {
    size_t producers = 1;
    size_t consumers = 1;
    size_t items_per_producer = 1024;
    std::chrono::milliseconds timeout = std::chrono::seconds(10);
  };

  /** @brief Result returned by producer/consumer stress helpers. */
  struct Producer_Consumer_Stress_Result
  {
    std::vector<size_t> consumed;
    bool timed_out = false;

    /** @brief Return the expected number of consumed values.
     *
     * @return Number of recorded consumed values.
     */
    [[nodiscard]] size_t size() const noexcept { return consumed.size(); }
  };

  namespace detail
  {
    template <typename Push>
    bool invoke_push(Push &push, const size_t value)
    {
      if constexpr (std::is_void_v<std::invoke_result_t<Push &, size_t>>)
        {
          push(value);
          return true;
        }
      else
        {
          return static_cast<bool>(push(value));
        }
    }
  }

  /** @brief Run a deterministic producer/consumer stress scenario.
   *
   * @tparam Push Callable invoked as `push(size_t value)`. It may return void
   *         for always-successful pushes or bool for retryable pushes. Must
   *         be safe to copy into one closure per producer thread; each
   *         thread runs its own copy (so `Push` itself need not be
   *         thread-safe unless it captures shared state).
   * @tparam TryPop Callable invoked as `try_pop(size_t &out)` returning true
   *         when a value was consumed. Same per-thread-copy contract as
   *         `Push`.
   * @param[in] config Stress dimensions and timeout.
   * @param[in,out] push Producer operation.
   * @param[in,out] try_pop Consumer operation.
   * @return Consumed values and timeout flag.
   *
   * The helper assigns unique values in `[0, producers * items_per_producer)`.
   * It does not assume FIFO ordering; callers should verify conservation,
   * ordering, or topology-specific properties separately.
   *
   * @throws std::system_error propagated as-is if the underlying
   *         `std::thread` constructor fails to create a thread; threads
   *         already launched are unblocked and joined (never left parked on
   *         the gate or abandoned joinable) before this propagates.
   * @throws Whatever the first producer/consumer worker throws (e.g. from
   *         `push`/`try_pop` itself), once every thread has been joined --
   *         see `run_workers()`, which this delegates to.
   */
  template <typename Push, typename TryPop>
  Producer_Consumer_Stress_Result run_producer_consumer_stress(
      const Producer_Consumer_Stress_Config &config,
      Push push,
      TryPop try_pop)
  {
    const size_t total = config.producers * config.items_per_producer;
    Producer_Consumer_Stress_Result result;
    result.consumed.resize(total);

    std::atomic<size_t> consumed_count{0};
    std::atomic<bool> timed_out{false};
    const auto deadline = std::chrono::steady_clock::now() + config.timeout;

    // Workers `[0, producers)` produce; `[producers, producers+consumers)`
    // consume. `push`/`try_pop` are captured by value here once, then
    // copied again per-thread by run_workers() itself (each thread's copy
    // of this worker closure carries its own copy of both callables).
    run_workers(config.producers + config.consumers,
      [&, push, try_pop](const size_t index) mutable
      {
        if (index < config.producers)
          {
            const size_t p = index;
            for (size_t i = 0; i < config.items_per_producer; ++i)
              {
                const size_t value = p * config.items_per_producer + i;
                while (not detail::invoke_push(push, value))
                  {
                    if (std::chrono::steady_clock::now() >= deadline)
                      {
                        timed_out.store(true, std::memory_order_relaxed);
                        return;
                      }
                    std::this_thread::yield();
                  }
              }
          }
        else
          {
            while (consumed_count.load(std::memory_order_acquire) < total)
              {
                size_t value = 0;
                if (try_pop(value))
                  {
                    const size_t slot =
                        consumed_count.fetch_add(1, std::memory_order_acq_rel);
                    if (slot < total)
                      result.consumed[slot] = value;
                  }
                else
                  {
                    if (std::chrono::steady_clock::now() >= deadline)
                      {
                        timed_out.store(true, std::memory_order_relaxed);
                        return;
                      }
                    std::this_thread::yield();
                  }
              }
          }
      });

    const size_t n = consumed_count.load(std::memory_order_acquire);
    if (n < result.consumed.size())
      result.consumed.resize(n);
    result.timed_out = timed_out.load(std::memory_order_relaxed);
    return result;
  }

  /** @brief Operation kind for randomized reference traces. */
  enum class Trace_Operation_Kind
  {
    push,
    pop,
    insert,
    erase,
    contains
  };

  /** @brief One randomized operation trace entry. */
  struct Trace_Operation
  {
    Trace_Operation_Kind kind = Trace_Operation_Kind::contains;
    size_t key = 0;
    size_t value = 0;

    /** @brief Compare two trace operations for equality.
     *
     * @param[in] rhs Operation to compare with.
     * @return true if both operations have the same kind, key, and value.
     */
    [[nodiscard]] bool operator == (const Trace_Operation &rhs) const noexcept
    {
      return kind == rhs.kind and key == rhs.key and value == rhs.value;
    }
  };

  /** @brief Build a deterministic pseudo-random operation trace.
   *
   * @param[in] count Number of operations to generate.
   * @param[in] seed Random seed.
   * @param[in] key_range Exclusive upper bound for generated keys.
   * @param[in] kinds Allowed operation kinds.
   * @return Generated operation trace.
   *
   * @pre `key_range > 0`.
   * @pre `kinds.size() > 0`.
   */
  inline std::vector<Trace_Operation> make_random_operation_trace(
      const size_t count,
      const uint32_t seed,
      const size_t key_range,
      const std::initializer_list<Trace_Operation_Kind> kinds =
        {
          Trace_Operation_Kind::insert,
          Trace_Operation_Kind::erase,
          Trace_Operation_Kind::contains
        })
  {
    std::vector<Trace_Operation_Kind> allowed(kinds.begin(), kinds.end());
    ah_invalid_argument_if(key_range == 0) << "key_range must be > 0";
    ah_invalid_argument_if(allowed.empty()) << "kinds must not be empty";

    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> kind_dist(0, allowed.size() - 1);
    std::uniform_int_distribution<size_t> key_dist(0, key_range - 1);
    std::uniform_int_distribution<size_t> value_dist;

    std::vector<Trace_Operation> trace;
    trace.reserve(count);
    for (size_t i = 0; i < count; ++i)
      trace.push_back(
        {
          allowed[kind_dist(rng)],
          key_dist(rng),
          value_dist(rng)
        });

    return trace;
  }

  /** @brief Replay a trace against a subject and a reference implementation.
   *
   * @tparam SubjectStep Callable invoked as `subject_step(operation)`.
   * @tparam ReferenceStep Callable invoked as `reference_step(operation)`.
   * @param[in] trace Operation trace to replay.
   * @param[in,out] subject_step Subject implementation step.
   * @param[in,out] reference_step Reference implementation step.
   * @return Operation indexes where returned results differ.
   */
  template <typename SubjectStep, typename ReferenceStep>
  std::vector<size_t> replay_trace_and_collect_mismatches(
      const std::vector<Trace_Operation> &trace,
      SubjectStep subject_step,
      ReferenceStep reference_step)
  {
    std::vector<size_t> mismatches;
    for (size_t i = 0; i < trace.size(); ++i)
      if (not (subject_step(trace[i]) == reference_step(trace[i])))
        mismatches.push_back(i);
    return mismatches;
  }
} // namespace Aleph::Testing

#endif // ALEPH_TESTS_CONCURRENCY_TEST_UTILS_H
