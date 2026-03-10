/**
 * @file experimental_async_example.cc
 * @brief Minimal example for Aleph-w's experimental async bridge.
 *
 * This program demonstrates the opt-in coroutine-friendly wrapper in
 * `experimental_async.H`. The bridge is intentionally small: it lets callers
 * schedule a callable on a `ThreadPool`, then either block with `get()` or
 * `co_await` the resulting operation inside their own coroutine type.
 *
 * Build with:
 *   cmake -S . -B build -DALEPH_ENABLE_EXPERIMENTAL_ASYNC=ON
 *   cmake --build build --target experimental_async_example
 *
 * Usage:
 *   ./experimental_async_example
 */

#include <experimental_async.H>
#include <thread_pool.H>

#include <iostream>
#include <string>

#if ALEPH_HAS_EXPERIMENTAL_ASYNC
#  include <coroutine>
#  include <future>
#endif

using namespace Aleph;

#if ALEPH_HAS_EXPERIMENTAL_ASYNC
namespace
{
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

  sync_task<int> coroutine_chain(ThreadPool & pool)
  {
    const int left = co_await experimental::schedule(pool, [] { return 20; });
    const int right = co_await experimental::schedule(pool, [left] {
      return left + 22;
    });
    co_return right;
  }
} // namespace
#endif

int main()
{
#if ALEPH_HAS_EXPERIMENTAL_ASYNC
  ThreadPool pool(4);

  auto greeting = experimental::schedule(pool, [] {
    return std::string("hello from the experimental async bridge");
  });

  std::cout << greeting.get() << "\n";
  std::cout << "coroutine chain result: " << coroutine_chain(pool).get() << "\n";

  auto default_result = experimental::schedule([] { return 7 * 6; });
  std::cout << "default pool result: " << default_result.get() << "\n";
#else
  std::cout
      << "Experimental async bridge is disabled.\n"
      << "Reconfigure with -DALEPH_ENABLE_EXPERIMENTAL_ASYNC=ON and use C++20.\n";
#endif

  return 0;
}
