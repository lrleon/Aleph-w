/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
1
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

/** @file experimental_async.H
 *  @brief Opt-in experimental coroutine-friendly bridge over `ThreadPool`.
 *
 *  This header intentionally keeps a very small scope. It does not attempt to
 *  redesign Aleph-w around coroutines or provide a full sender/receiver
 *  framework. Instead it offers:
 *
 *  - \ref Aleph::experimental::scheduled_operation: a move-only awaitable
 *    result handle for work scheduled on a `ThreadPool`
 *  - \ref Aleph::experimental::schedule: submit a callable to a pool and
 *    obtain a `scheduled_operation`
 *
 *  The bridge is disabled by default. To enable it, build or compile with
 *  `ALEPH_ENABLE_EXPERIMENTAL_ASYNC=1` and a C++20 compiler that provides
 *  `<coroutine>`. When these conditions are not met, the header still defines
 *  `ALEPH_HAS_EXPERIMENTAL_ASYNC` as `0`, but no experimental async API is
 *  declared.
 *
 *  Portability assessment for Aleph-w's current targets:
 *
 *  - GCC 11+ with `-std=c++20` provides the coroutine support needed here
 *  - Clang 14+ with `-std=c++20` likewise supports this limited bridge
 *  - below C++20 this header simply stays disabled; it does not introduce a
 *    new coroutine requirement for code that does not opt into the bridge
 *  - MSVC remains outside Aleph-w's documented support matrix
 *
 *  Design constraints:
 *
 *  - completion resumes an awaiting coroutine on the worker thread that
 *    finishes the scheduled callable
 *  - each `scheduled_operation<T>` has single-consumer semantics, like
 *    `std::future<T>`
 *  - return-by-reference is intentionally rejected; return a value or
 *    `std::reference_wrapper<T>` instead
 *
 *  Example:
 *
 *  @code
 *  #include <experimental_async.H>
 *  #include <thread_pool.H>
 *
 *  #if ALEPH_HAS_EXPERIMENTAL_ASYNC
 *  Aleph::ThreadPool pool(4);
 *  auto op = Aleph::experimental::schedule(pool, [] { return 42; });
 *  int value = op.get();
 *  (void) value;
 *  #endif
 *  @endcode
 *
 *  See `docs/experimental_async_bridge.md` and
 *  `Examples/experimental_async_example.cc`.
 *
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency
 */

#ifndef ALEPH_EXPERIMENTAL_ASYNC_H
#define ALEPH_EXPERIMENTAL_ASYNC_H

#include <ah-errors.H>
#include <thread_pool.H>

#ifndef ALEPH_ENABLE_EXPERIMENTAL_ASYNC
#  define ALEPH_ENABLE_EXPERIMENTAL_ASYNC 0
#endif

#if ALEPH_ENABLE_EXPERIMENTAL_ASYNC && __cplusplus >= 202002L
#  if defined(__has_include)
#    if __has_include(<coroutine>)
#      include <coroutine>
#      define ALEPH_DETAIL_HAS_COROUTINE_HEADER 1
#    else
#      define ALEPH_DETAIL_HAS_COROUTINE_HEADER 0
#    endif
#  else
#    define ALEPH_DETAIL_HAS_COROUTINE_HEADER 0
#  endif
#else
#  define ALEPH_DETAIL_HAS_COROUTINE_HEADER 0
#endif

#if ALEPH_ENABLE_EXPERIMENTAL_ASYNC && \
    __cplusplus >= 202002L && \
    ALEPH_DETAIL_HAS_COROUTINE_HEADER && \
    defined(__cpp_impl_coroutine) && \
    __cpp_impl_coroutine >= 201902L
#  define ALEPH_HAS_EXPERIMENTAL_ASYNC 1
#else
#  define ALEPH_HAS_EXPERIMENTAL_ASYNC 0
#endif

#if ALEPH_HAS_EXPERIMENTAL_ASYNC

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Aleph
{
  namespace experimental
  {
    namespace detail
    {
      [[noreturn]] inline void throw_no_state()
      {
        ah_runtime_error() << "scheduled_operation has no state";
        std::terminate();
      }

      class ContinuationRegistration
      {
        mutable std::mutex mutex_;
        std::coroutine_handle<> handle_{};
        bool active_ = true;

      public:
        explicit ContinuationRegistration(std::coroutine_handle<> handle) noexcept
          : handle_(handle) {}

        [[nodiscard]] bool is_active() const noexcept
        {
          std::lock_guard<std::mutex> lock(mutex_);
          return active_ and static_cast<bool>(handle_);
        }

        void cancel() noexcept
        {
          std::lock_guard<std::mutex> lock(mutex_);
          active_ = false;
          handle_ = std::coroutine_handle<>{};
        }

        void resume_if_active()
        {
          std::coroutine_handle<> handle;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            if (not active_ or not handle_)
              return;
            active_ = false;
            handle = std::exchange(handle_, std::coroutine_handle<>{});
          }
          handle.resume();
        }
      };

      template <typename F, typename... Args>
      auto make_invocable(F && f, Args && ... args)
      {
        return [func = std::forward<F>(f),
                args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
          {
            return std::apply([&func](auto && ... a)
                                {
                                  return std::invoke(std::move(func),
                                                     std::forward<decltype(a)>(a)...);
                                }, std::move(args_tuple));
          };
      }

      template <typename T>
      class OperationState
      {
        mutable std::mutex mutex_;
        mutable std::condition_variable ready_cv_;
        bool ready_ = false;
        bool consumed_ = false;
        std::exception_ptr exception_;
        std::optional<T> value_;
        std::weak_ptr<ContinuationRegistration> continuation_;

        template <typename F>
        void complete(F && fill)
        {
          std::weak_ptr<ContinuationRegistration> continuation;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ready_)
              return;
            fill();
            ready_ = true;
            continuation = std::exchange(continuation_, {});
          }
          ready_cv_.notify_all();
          if (auto registration = continuation.lock(); registration != nullptr)
            registration->resume_if_active();
        }

      public:
        OperationState() = default;

        [[nodiscard]] bool is_ready() const
        {
          std::lock_guard<std::mutex> lock(mutex_);
          return ready_;
        }

        void wait() const
        {
          std::unique_lock<std::mutex> lock(mutex_);
          ready_cv_.wait(lock, [this] { return ready_; });
        }

        template <typename U>
        void set_value(U && value)
        {
          complete([this, &value]
                   {
                     value_.emplace(std::forward<U>(value));
                   });
        }

        void set_exception(std::exception_ptr exception)
        {
          complete([this, &exception]
                   {
                     exception_ = exception;
                   });
        }

        [[nodiscard]] bool attach_continuation(
            const std::shared_ptr<ContinuationRegistration> & continuation)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (ready_)
            return false;
          if (auto registered = continuation_.lock();
              registered != nullptr and registered->is_active())
            ah_logic_error_if(true)
              << "scheduled_operation already has a waiting coroutine";
          continuation_ = continuation;
          return true;
        }

        void detach_continuation(
            const std::shared_ptr<ContinuationRegistration> & continuation) noexcept
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (auto registered = continuation_.lock(); registered == continuation)
            continuation_.reset();
        }

        T take()
        {
          std::unique_lock<std::mutex> lock(mutex_);
          ready_cv_.wait(lock, [this] { return ready_; });
          if (consumed_)
            throw_no_state();
          consumed_ = true;
          if (exception_ != nullptr)
            std::rethrow_exception(exception_);
          return std::move(*value_);
        }
      };

      template <>
      class OperationState<void>
      {
        mutable std::mutex mutex_;
        mutable std::condition_variable ready_cv_;
        bool ready_ = false;
        bool consumed_ = false;
        std::exception_ptr exception_;
        std::weak_ptr<ContinuationRegistration> continuation_;

        template <typename F>
        void complete(F && fill)
        {
          std::weak_ptr<ContinuationRegistration> continuation;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ready_)
              return;
            fill();
            ready_ = true;
            continuation = std::exchange(continuation_, {});
          }
          ready_cv_.notify_all();
          if (auto registration = continuation.lock(); registration != nullptr)
            registration->resume_if_active();
        }

      public:
        OperationState() = default;

        [[nodiscard]] bool is_ready() const
        {
          std::lock_guard<std::mutex> lock(mutex_);
          return ready_;
        }

        void wait() const
        {
          std::unique_lock<std::mutex> lock(mutex_);
          ready_cv_.wait(lock, [this] { return ready_; });
        }

        void set_value()
        {
          complete([] {});
        }

        void set_exception(std::exception_ptr exception)
        {
          complete([this, &exception]
                   {
                     exception_ = exception;
                   });
        }

        [[nodiscard]] bool attach_continuation(
            const std::shared_ptr<ContinuationRegistration> & continuation)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (ready_)
            return false;
          if (auto registered = continuation_.lock();
              registered != nullptr and registered->is_active())
            ah_logic_error_if(true)
              << "scheduled_operation already has a waiting coroutine";
          continuation_ = continuation;
          return true;
        }

        void detach_continuation(
            const std::shared_ptr<ContinuationRegistration> & continuation) noexcept
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (auto registered = continuation_.lock(); registered == continuation)
            continuation_.reset();
        }

        void take()
        {
          std::unique_lock<std::mutex> lock(mutex_);
          ready_cv_.wait(lock, [this] { return ready_; });
          if (consumed_)
            throw_no_state();
          consumed_ = true;
          if (exception_ != nullptr)
            std::rethrow_exception(exception_);
        }
      };
    } // namespace detail

    /** @brief Move-only awaitable result of work scheduled on a `ThreadPool`.
     *
     *  `scheduled_operation<T>` behaves like a tiny move-only
     *  `std::future<T>`-style bridge for a single scheduled result.
     *  Callers may:
     *
     *  - poll readiness with `valid()` / `is_ready()`,
     *  - block with `wait()` / `get()`,
     *  - or `co_await` the operation inside their own coroutine type.
     *
     *  Handles are move-only and single-consumer. Consuming operations
     *  (`get()` and `await_resume()`) detach the shared state from the handle,
     *  after which `valid()` returns false. Any public operation except
     *  `valid()` throws via `throw_no_state()` when invoked on a moved-from or
     *  otherwise empty handle.
     *
     *  @tparam T Result type produced by the scheduled callable.
     *  @thread_safety Safe to observe completion (`valid()`, `is_ready()`,
     *                 `wait()`, `await_ready()`) concurrently with task
     *                 completion.
     *  @thread_safety Not safe to call consuming or mutating operations
     *                 concurrently on the same handle object.
     *  @thread_safety Destroying the handle while a coroutine is suspended on
     *                 it safely unregisters the continuation so completion will
     *                 not resume a dead frame.
     */
    template <typename T>
    class scheduled_operation
    {
      std::shared_ptr<detail::OperationState<T>> state_;
      std::shared_ptr<detail::ContinuationRegistration> continuation_registration_;

      explicit scheduled_operation(std::shared_ptr<detail::OperationState<T>> state)
        : state_(std::move(state)) {}

      void unregister_continuation() noexcept
      {
        if (state_ != nullptr and continuation_registration_ != nullptr)
          state_->detach_continuation(continuation_registration_);
        if (continuation_registration_ != nullptr)
          continuation_registration_->cancel();
        continuation_registration_.reset();
      }

      std::shared_ptr<detail::OperationState<T>> consume_state()
      {
        unregister_continuation();
        auto state = std::exchange(state_, nullptr);
        if (state == nullptr)
          detail::throw_no_state();
        return state;
      }

      detail::OperationState<T> & state()
      {
        if (state_ == nullptr)
          detail::throw_no_state();
        return *state_;
      }

      const detail::OperationState<T> & state() const
      {
        if (state_ == nullptr)
          detail::throw_no_state();
        return *state_;
      }

      template <typename F, typename... Args>
      friend auto schedule(ThreadPool & pool, F && f, Args && ... args)
        -> scheduled_operation<std::invoke_result_t<F, Args...>>;

    public:
      /** @brief Construct an empty operation handle.
       *  @thread_safety Thread-safe for independent objects. Concurrent access
       *                 to the same handle instance is not synchronized.
       */
      scheduled_operation() = default;
      scheduled_operation(const scheduled_operation &) = delete;
      scheduled_operation & operator = (const scheduled_operation &) = delete;
      scheduled_operation(scheduled_operation &&) noexcept = default;
      scheduled_operation & operator = (scheduled_operation &&) noexcept = default;
      /** @brief Destroy the handle and unregister any suspended continuation.
       *  @thread_safety Safe with respect to task completion. Not safe for
       *                 concurrent use with other member functions on the same
       *                 object.
       */
      ~scheduled_operation() { unregister_continuation(); }

      /** @brief Check whether this handle still owns an unconsumed state.
       *  @return true if the operation can still be waited on or consumed.
       *  @throws Nothing.
       *  @note Returns false after `get()` or `await_resume()` consumes the state.
       *  @thread_safety Safe for concurrent read-only observation if no other
       *                 thread mutates the same handle object concurrently.
       */
      [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

      /** @brief Check whether the scheduled work has completed.
       *  @return true if a value or exception is ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion and
       *                 other const observers on the same shared state.
       */
      [[nodiscard]] bool is_ready() const
      {
        return state().is_ready();
      }

      /** @brief Block until the scheduled work completes.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion.
       *  @thread_safety Not safe to race with `get()`, `await_suspend()`, or
       *                 `await_resume()` on the same handle object.
       */
      void wait() const
      {
        state().wait();
      }

      /** @brief Consume the result of the scheduled work.
       *  @return The computed value.
       *  @throws std::runtime_error if the handle is empty, moved-from, or was
       *          already consumed.
       *  @throws Any exception captured from the scheduled callable.
       *  @note Invalidates this handle; `valid()` returns false afterward.
       *  @thread_safety Single-consumer only. Do not call concurrently with
       *                 any other consuming or mutating operation on the same
       *                 handle object.
       */
      T get()
      {
        return consume_state()->take();
      }

      /** @brief Check whether `co_await` can continue without suspension.
       *  @return true if the result is already ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion and
       *                 other const observers on the same shared state.
       */
      [[nodiscard]] bool await_ready() const
      {
        return state().is_ready();
      }

      /** @brief Register the awaiting coroutine for resumption on completion.
       *  @param continuation Coroutine to resume when the operation completes.
       *  @return true if the coroutine should suspend, false if already ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @throws std::logic_error if a second coroutine tries to await the same operation.
       *  @note Safe against awaiter destruction before completion; destruction
       *        unregisters the continuation.
       *  @thread_safety Handles races with completion internally.
       *  @thread_safety Single-awaiter only. Do not call concurrently with
       *                 other mutating operations on the same handle object.
       */
      bool await_suspend(std::coroutine_handle<> continuation)
      {
        unregister_continuation();
        auto registration =
            std::make_shared<detail::ContinuationRegistration>(continuation);
        if (state().attach_continuation(registration))
          {
            continuation_registration_ = std::move(registration);
            return true;
          }
        return false;
      }

      /** @brief Consume the awaited result after coroutine resumption.
       *  @return The computed value.
       *  @throws std::runtime_error if the handle is empty, moved-from, or was
       *          already consumed.
       *  @throws Any exception captured from the scheduled callable.
       *  @note Invalidates this handle; `valid()` returns false afterward.
       *  @thread_safety Single-consumer only. Do not call concurrently with
       *                 any other consuming or mutating operation on the same
       *                 handle object.
       */
      T await_resume()
      {
        return consume_state()->take();
      }
    };

    /** @brief Move-only awaitable result of work scheduled on a `ThreadPool`
     *         for `void`-returning callables.
     *
     *  `scheduled_operation<void>` exposes the same move-only, single-consumer
     *  contract as `scheduled_operation<T>`, but models completion without a
     *  value. Callers may poll readiness, block with `wait()` / `get()`, or
     *  `co_await` the operation inside their own coroutine type.
     *
     *  Consuming operations (`get()` and `await_resume()`) detach the shared
     *  state from the handle, after which `valid()` returns false. Any public
     *  operation except `valid()` throws via `throw_no_state()` when invoked
     *  on a moved-from or otherwise empty handle.
     *
     *  @thread_safety Safe to observe completion (`valid()`, `is_ready()`,
     *                 `wait()`, `await_ready()`) concurrently with task
     *                 completion.
     *  @thread_safety Not safe to call consuming or mutating operations
     *                 concurrently on the same handle object.
     *  @thread_safety Destroying the handle while a coroutine is suspended on
     *                 it safely unregisters the continuation so completion will
     *                 not resume a dead frame.
     */
    template <>
    class scheduled_operation<void>
    {
      std::shared_ptr<detail::OperationState<void>> state_;
      std::shared_ptr<detail::ContinuationRegistration> continuation_registration_;

      explicit scheduled_operation(std::shared_ptr<detail::OperationState<void>> state)
        : state_(std::move(state)) {}

      void unregister_continuation() noexcept
      {
        if (state_ != nullptr and continuation_registration_ != nullptr)
          state_->detach_continuation(continuation_registration_);
        if (continuation_registration_ != nullptr)
          continuation_registration_->cancel();
        continuation_registration_.reset();
      }

      std::shared_ptr<detail::OperationState<void>> consume_state()
      {
        unregister_continuation();
        auto state = std::exchange(state_, nullptr);
        if (state == nullptr)
          detail::throw_no_state();
        return state;
      }

      detail::OperationState<void> & state()
      {
        if (state_ == nullptr)
          detail::throw_no_state();
        return *state_;
      }

      const detail::OperationState<void> & state() const
      {
        if (state_ == nullptr)
          detail::throw_no_state();
        return *state_;
      }

      template <typename F, typename... Args>
      friend auto schedule(ThreadPool & pool, F && f, Args && ... args)
        -> scheduled_operation<std::invoke_result_t<F, Args...>>;

    public:
      /** @brief Construct an empty operation handle.
       *  @thread_safety Thread-safe for independent objects. Concurrent access
       *                 to the same handle instance is not synchronized.
       */
      scheduled_operation() = default;
      scheduled_operation(const scheduled_operation &) = delete;
      scheduled_operation & operator = (const scheduled_operation &) = delete;
      scheduled_operation(scheduled_operation &&) noexcept = default;
      scheduled_operation & operator = (scheduled_operation &&) noexcept = default;
      /** @brief Destroy the handle and unregister any suspended continuation.
       *  @thread_safety Safe with respect to task completion. Not safe for
       *                 concurrent use with other member functions on the same
       *                 object.
       */
      ~scheduled_operation() { unregister_continuation(); }

      /** @brief Check whether this handle still owns an unconsumed state.
       *  @return true if the operation can still be waited on or consumed.
       *  @throws Nothing.
       *  @note Returns false after `get()` or `await_resume()` consumes the state.
       *  @thread_safety Safe for concurrent read-only observation if no other
       *                 thread mutates the same handle object concurrently.
       */
      [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

      /** @brief Check whether the scheduled work has completed.
       *  @return true if completion or an exception is ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion and
       *                 other const observers on the same shared state.
       */
      [[nodiscard]] bool is_ready() const
      {
        return state().is_ready();
      }

      /** @brief Block until the scheduled work completes.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion.
       *  @thread_safety Not safe to race with `get()`, `await_suspend()`, or
       *                 `await_resume()` on the same handle object.
       */
      void wait() const
      {
        state().wait();
      }

      /** @brief Consume completion of the scheduled work.
       *  @throws std::runtime_error if the handle is empty, moved-from, or was
       *          already consumed.
       *  @throws Any exception captured from the scheduled callable.
       *  @note Invalidates this handle; `valid()` returns false afterward.
       *  @thread_safety Single-consumer only. Do not call concurrently with
       *                 any other consuming or mutating operation on the same
       *                 handle object.
       */
      void get()
      {
        consume_state()->take();
      }

      /** @brief Check whether `co_await` can continue without suspension.
       *  @return true if completion is already ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @thread_safety Safe to call concurrently with task completion and
       *                 other const observers on the same shared state.
       */
      [[nodiscard]] bool await_ready() const
      {
        return state().is_ready();
      }

      /** @brief Register the awaiting coroutine for resumption on completion.
       *  @param continuation Coroutine to resume when the operation completes.
       *  @return true if the coroutine should suspend, false if already ready.
       *  @throws std::runtime_error if the handle is empty or moved-from.
       *  @throws std::logic_error if a second coroutine tries to await the same operation.
       *  @note Safe against awaiter destruction before completion; destruction
       *        unregisters the continuation.
       *  @thread_safety Handles races with completion internally.
       *  @thread_safety Single-awaiter only. Do not call concurrently with
       *                 other mutating operations on the same handle object.
       */
      bool await_suspend(std::coroutine_handle<> continuation)
      {
        unregister_continuation();
        auto registration =
            std::make_shared<detail::ContinuationRegistration>(continuation);
        if (state().attach_continuation(registration))
          {
            continuation_registration_ = std::move(registration);
            return true;
          }
        return false;
      }

      /** @brief Consume completion after coroutine resumption.
       *  @return Nothing.
       *  @throws std::runtime_error if the handle is empty, moved-from, or was
       *          already consumed.
       *  @throws Any exception captured from the scheduled callable.
       *  @note Invalidates this handle; `valid()` returns false afterward.
       *  @thread_safety Single-consumer only. Do not call concurrently with
       *                 any other consuming or mutating operation on the same
       *                 handle object.
       */
      void await_resume()
      {
        consume_state()->take();
      }
    };

    /** @brief Submit work to a pool and get a coroutine-friendly result handle.
     *
     *  The scheduled callable starts eagerly, using the provided `ThreadPool`.
     *  Callers may block with `get()`/`wait()` or `co_await` the returned
     *  `scheduled_operation<T>`.
     *
     *  @note Awaiting resumes on the worker thread that completes the callable.
     *  @note Results are single-consumer, just like `std::future<T>`.
     */
    template <typename F, typename... Args>
    [[nodiscard]] auto schedule(ThreadPool & pool, F && f, Args && ... args)
      -> scheduled_operation<std::invoke_result_t<F, Args...>>
    {
      using return_type = std::invoke_result_t<F, Args...>;
      static_assert(not std::is_reference_v<return_type>,
                    "experimental::schedule() does not support reference returns");

      auto state = std::make_shared<detail::OperationState<return_type>>();
      auto invocable = detail::make_invocable(std::forward<F>(f),
                                              std::forward<Args>(args)...);

      pool.enqueue_detached([state, invocable = std::move(invocable)]() mutable
                            {
                              try
                                {
                                  if constexpr (std::is_void_v<return_type>)
                                    {
                                      invocable();
                                      state->set_value();
                                    }
                                  else
                                    {
                                      state->set_value(invocable());
                                    }
                                }
                              catch (...)
                                {
                                  state->set_exception(std::current_exception());
                                }
                            });

      return scheduled_operation<return_type>(std::move(state));
    }

    /** @brief Convenience overload that uses the global default pool. */
    template <typename F, typename... Args>
    [[nodiscard]] auto schedule(F && f, Args && ... args)
      -> scheduled_operation<std::invoke_result_t<F, Args...>>
    {
      return schedule(default_pool(),
                      std::forward<F>(f),
                      std::forward<Args>(args)...);
    }
  } // namespace experimental
} // namespace Aleph

#endif

#endif
