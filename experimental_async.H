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
#include <stdexcept>
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
        throw std::runtime_error("scheduled_operation has no state");
      }

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
        std::coroutine_handle<> continuation_{};

        template <typename F>
        void complete(F && fill)
        {
          std::coroutine_handle<> continuation;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ready_)
              return;
            fill();
            ready_ = true;
            continuation = std::exchange(continuation_, std::coroutine_handle<>{});
          }
          ready_cv_.notify_all();
          if (continuation)
            continuation.resume();
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
            std::coroutine_handle<> continuation)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (ready_)
            return false;
          if (continuation_)
            throw std::logic_error("scheduled_operation already has a waiting coroutine");
          continuation_ = continuation;
          return true;
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
        std::coroutine_handle<> continuation_{};

        template <typename F>
        void complete(F && fill)
        {
          std::coroutine_handle<> continuation;
          {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ready_)
              return;
            fill();
            ready_ = true;
            continuation = std::exchange(continuation_, std::coroutine_handle<>{});
          }
          ready_cv_.notify_all();
          if (continuation)
            continuation.resume();
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
            std::coroutine_handle<> continuation)
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (ready_)
            return false;
          if (continuation_)
            throw std::logic_error("scheduled_operation already has a waiting coroutine");
          continuation_ = continuation;
          return true;
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
     *  `scheduled_operation<T>` intentionally behaves like a tiny
     *  `std::future<T>`-style bridge. It supports blocking `wait()` / `get()`
     *  and also `co_await` in user-defined coroutine types.
     */
    template <typename T>
    class scheduled_operation
    {
      std::shared_ptr<detail::OperationState<T>> state_;

      explicit scheduled_operation(std::shared_ptr<detail::OperationState<T>> state)
        : state_(std::move(state)) {}

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
      scheduled_operation() = default;
      scheduled_operation(const scheduled_operation &) = delete;
      scheduled_operation & operator = (const scheduled_operation &) = delete;
      scheduled_operation(scheduled_operation &&) noexcept = default;
      scheduled_operation & operator = (scheduled_operation &&) noexcept = default;

      [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

      [[nodiscard]] bool is_ready() const
      {
        return state().is_ready();
      }

      void wait() const
      {
        state().wait();
      }

      T get()
      {
        return state().take();
      }

      [[nodiscard]] bool await_ready() const
      {
        return state().is_ready();
      }

      bool await_suspend(std::coroutine_handle<> continuation)
      {
        return state().attach_continuation(continuation);
      }

      T await_resume()
      {
        return state().take();
      }
    };

    template <>
    class scheduled_operation<void>
    {
      std::shared_ptr<detail::OperationState<void>> state_;

      explicit scheduled_operation(std::shared_ptr<detail::OperationState<void>> state)
        : state_(std::move(state)) {}

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
      scheduled_operation() = default;
      scheduled_operation(const scheduled_operation &) = delete;
      scheduled_operation & operator = (const scheduled_operation &) = delete;
      scheduled_operation(scheduled_operation &&) noexcept = default;
      scheduled_operation & operator = (scheduled_operation &&) noexcept = default;

      [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

      [[nodiscard]] bool is_ready() const
      {
        return state().is_ready();
      }

      void wait() const
      {
        state().wait();
      }

      void get()
      {
        state().take();
      }

      [[nodiscard]] bool await_ready() const
      {
        return state().is_ready();
      }

      bool await_suspend(std::coroutine_handle<> continuation)
      {
        return state().attach_continuation(continuation);
      }

      void await_resume()
      {
        state().take();
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
