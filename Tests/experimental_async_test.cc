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

#include <experimental_async.H>
#include <thread_pool.H>

#include <memory>
#include <stdexcept>

#if ALEPH_HAS_EXPERIMENTAL_ASYNC
#  include <coroutine>
#  include <future>
#endif

using namespace Aleph;

#if ALEPH_HAS_EXPERIMENTAL_ASYNC
namespace
{
  class detached_task
  {
  public:
    struct promise_type
    {
      detached_task get_return_object()
      {
        return detached_task(
            std::coroutine_handle<promise_type>::from_promise(*this));
      }

      std::suspend_never initial_suspend() noexcept { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }
      void return_void() noexcept {}
      void unhandled_exception() { std::terminate(); }
    };

  private:
    std::coroutine_handle<promise_type> handle_;

    explicit detached_task(std::coroutine_handle<promise_type> handle) noexcept
      : handle_(handle) {}

  public:
    detached_task(const detached_task &) = delete;
    detached_task & operator = (const detached_task &) = delete;

    detached_task(detached_task && other) noexcept
      : handle_(other.handle_)
    {
      other.handle_ = {};
    }

    detached_task & operator = (detached_task && other) noexcept
    {
      if (this == &other)
        return *this;
      if (handle_)
        handle_.destroy();
      handle_ = other.handle_;
      other.handle_ = {};
      return *this;
    }

    ~detached_task()
    {
      if (handle_)
        handle_.destroy();
    }
  };

  template <typename T>
  class sync_task
  {
    std::future<T> future_;

  public:
    struct promise_type
    {
      std::promise<T> promise_;

      sync_task get_return_object()
      {
        return sync_task(promise_.get_future(),
                         std::coroutine_handle<promise_type>::from_promise(*this));
      }

      std::suspend_never initial_suspend() noexcept { return {}; }
      std::suspend_always final_suspend() noexcept { return {}; }

      void return_value(T value)
      {
        promise_.set_value(std::move(value));
      }

      void unhandled_exception()
      {
        promise_.set_exception(std::current_exception());
      }
    };

  private:
    std::coroutine_handle<promise_type> handle_;

    sync_task(std::future<T> future,
              std::coroutine_handle<promise_type> handle) noexcept
      : future_(std::move(future)), handle_(handle) {}

  public:
    sync_task(const sync_task &) = delete;
    sync_task & operator = (const sync_task &) = delete;

    sync_task(sync_task && other) noexcept
      : future_(std::move(other.future_)), handle_(other.handle_)
    {
      other.handle_ = {};
    }

    sync_task & operator = (sync_task && other) noexcept
    {
      if (this == &other)
        return *this;
      if (handle_)
        handle_.destroy();
      future_ = std::move(other.future_);
      handle_ = other.handle_;
      other.handle_ = {};
      return *this;
    }

    ~sync_task()
    {
      if (handle_)
        handle_.destroy();
    }

    T get()
    {
      return future_.get();
    }
  };

  sync_task<int> chained_sum(ThreadPool & pool)
  {
    const int first = co_await experimental::schedule(pool, [] { return 20; });
    const int second = co_await experimental::schedule(pool, [first] {
      return first + 22;
    });
    co_return second;
  }

  detached_task await_and_mark(ThreadPool & pool,
                               std::shared_future<void> gate,
                               std::promise<void> started,
                               std::atomic<bool> & resumed)
  {
    co_await experimental::schedule(pool, [gate, started = std::move(started)]() mutable {
      started.set_value();
      gate.wait();
      return 11;
    });
    resumed.store(true, std::memory_order_release);
  }
} // namespace

TEST(ExperimentalAsync, ScheduleGetReturnsValue)
{
  ThreadPool pool(2);
  auto op = experimental::schedule(pool, [] { return 42; });

  EXPECT_EQ(op.get(), 42);
}

TEST(ExperimentalAsync, SchedulePropagatesExceptions)
{
  ThreadPool pool(2);
  auto op = experimental::schedule(pool, []() -> int {
    throw std::runtime_error("boom");
  });

  EXPECT_THROW(op.get(), std::runtime_error);
}

TEST(ExperimentalAsync, ScheduleSupportsMoveOnlyResults)
{
  ThreadPool pool(2);
  auto op = experimental::schedule(pool, [] {
    return std::make_unique<int>(17);
  });

  auto value = op.get();
  ASSERT_TRUE(value != nullptr);
  EXPECT_EQ(*value, 17);
}

TEST(ExperimentalAsync, ScheduleCanBeAwaitedInsideCoroutine)
{
  ThreadPool pool(4);

  EXPECT_EQ(chained_sum(pool).get(), 42);
}

TEST(ExperimentalAsync, ScheduleHasSingleConsumerSemantics)
{
  ThreadPool pool(2);
  auto op = experimental::schedule(pool, [] { return 5; });

  EXPECT_TRUE(op.valid());
  EXPECT_EQ(op.get(), 5);
  EXPECT_FALSE(op.valid());
  EXPECT_THROW((void) op.get(), std::runtime_error);
}

TEST(ExperimentalAsync, ScheduleVoidInvalidAfterConsumption)
{
  ThreadPool pool(2);
  auto op = experimental::schedule(pool, [] {});

  EXPECT_TRUE(op.valid());
  op.get();
  EXPECT_FALSE(op.valid());
  EXPECT_THROW(op.get(), std::runtime_error);
}

TEST(ExperimentalAsync, DestroyedAwaiterUnregistersContinuation)
{
  ThreadPool pool(2);
  std::promise<void> gate_promise;
  auto gate = gate_promise.get_future().share();
  std::promise<void> started;
  auto started_future = started.get_future();
  std::atomic<bool> resumed{false};

  {
    auto task = await_and_mark(pool, gate, std::move(started), resumed);
    started_future.wait();
  }

  gate_promise.set_value();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  EXPECT_FALSE(resumed.load(std::memory_order_acquire));
}
#else
TEST(ExperimentalAsync, DisabledWithoutOptIn)
{
  EXPECT_EQ(ALEPH_HAS_EXPERIMENTAL_ASYNC, 0);
}
#endif
