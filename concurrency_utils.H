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

/** @file concurrency_utils.H
 *  @brief Modern synchronization helpers for channels, shared state, and
 *  small producer-consumer queues.
 *
 *  This header complements \ref thread_pool.H with a few focused utilities for
 *  practical concurrent code:
 *
 *  - \ref Aleph::BoundedChannel for bounded producer-consumer pipelines
 *  - \ref Aleph::Synchronized for mutex-protected shared objects
 *  - \ref Aleph::RwSynchronized for read/write-lock protected objects
 *  - \ref Aleph::SpscQueue for a bounded single-producer/single-consumer queue
 *
 *  These facilities intentionally keep a small API surface. They are suitable
 *  for everyday coordination tasks and can be combined with `ThreadPool`,
 *  `TaskGroup`, and the parallel algorithms introduced in earlier phases.
 *
 *  Example:
 *
 *  ```cpp
 *  #include <concurrency_utils.H>
 *  #include <thread_pool.H>
 *
 *  Aleph::bounded_channel<int> jobs(8);
 *  Aleph::synchronized<std::vector<int>> results(std::in_place);
 *  Aleph::ThreadPool pool(4);
 *
 *  pool.enqueue_detached([&] {
 *    for (int i = 1; i <= 10; ++i)
 *      jobs.send(i);
 *    jobs.close();
 *  });
 *
 *  Aleph::TaskGroup workers(pool);
 *  for (int w = 0; w < 2; ++w)
 *    workers.launch([&] {
 *      while (auto item = jobs.recv())
 *        results.with_lock([&](auto &out) { out.push_back(*item * 2); });
 *    });
 *  workers.wait();
 *  ```
 *
 *  For legacy POSIX wrappers see \ref useMutex.H and \ref useCondVar.H.
 *  New code should generally prefer this header together with standard C++
 *  mutexes and condition variables.
 *
 *  `BoundedChannel` also offers overloads that accept
 *  \ref Aleph::CancellationToken. These preserve the same close semantics but
 *  throw \ref Aleph::operation_canceled if cancellation is requested while a
 *  sender or receiver is blocked waiting for space or data.
 *
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency
 */

#ifndef ALEPH_CONCURRENCY_UTILS_H
#define ALEPH_CONCURRENCY_UTILS_H

#include <atomic>
#include <condition_variable>
#include <concepts>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <thread_pool.H>

namespace Aleph
{
  /** @brief Bounded blocking channel for producer-consumer workflows.
   *
   *  `send()` blocks while the channel is full. `recv()` blocks while the
   *  channel is empty. Once `close()` is called, pending receivers drain any
   *  queued items and then observe end-of-stream; pending senders wake up and
   *  fail with `false`.
   */
  template <typename T>
  class BoundedChannel
  {
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> queue_;
    size_t capacity_;
    bool closed_ = false;

    template <typename Predicate>
    void wait_with_cancellation(std::condition_variable & cv,
                                std::unique_lock<std::mutex> & lock,
                                Predicate pred,
                                const CancellationToken & token)
    {
      if (pred())
        return;
      auto registration = token.notify_on_cancel(cv);
      token.throw_if_cancellation_requested();
      cv.wait(lock, [&] { return pred() or token.stop_requested(); });
      if (not pred())
        token.throw_if_cancellation_requested();
    }

    template <typename U>
    bool send_impl(U && value)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_full_.wait(lock, [&] { return closed_ or queue_.size() < capacity_; });
      if (closed_)
        return false;
      queue_.push_back(std::forward<U>(value));
      lock.unlock();
      not_empty_.notify_one();
      return true;
    }

    template <typename U>
    bool send_impl(U && value, const CancellationToken & token)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_full_, lock,
                             [&] { return closed_ or queue_.size() < capacity_; },
                             token);
      if (closed_)
        return false;
      queue_.push_back(std::forward<U>(value));
      lock.unlock();
      not_empty_.notify_one();
      return true;
    }

  public:
    explicit BoundedChannel(size_t capacity)
      : capacity_(capacity)
    {
      if (capacity_ == 0)
        throw std::invalid_argument("BoundedChannel capacity must be greater than zero");
    }

    BoundedChannel(const BoundedChannel &) = delete;
    BoundedChannel & operator = (const BoundedChannel &) = delete;

    /** @brief Return the maximum number of queued items. */
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }

    /** @brief Return true if the channel has been closed. */
    [[nodiscard]] bool is_closed() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return closed_;
    }

    /** @brief Return the current number of queued items. */
    [[nodiscard]] size_t size() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.size();
    }

    /** @brief Return true if the channel is currently empty. */
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.empty();
    }

    /** @brief Close the channel and wake all blocked senders/receivers. */
    void close()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_)
        return;
      closed_ = true;
      not_empty_.notify_all();
      not_full_.notify_all();
    }

    /** @brief Blocking send. Returns false if the channel was closed. */
    bool send(const T & value) { return send_impl(value); }

    /** @brief Blocking send. Returns false if the channel was closed. */
    bool send(T && value) { return send_impl(std::move(value)); }

    /** @brief Cancellation-aware blocking send.
     *  @throws operation_canceled if cancellation is requested while blocked.
     */
    bool send(const T & value, const CancellationToken & token)
    {
      return send_impl(value, token);
    }

    /** @brief Cancellation-aware blocking send.
     *  @throws operation_canceled if cancellation is requested while blocked.
     */
    bool send(T && value, const CancellationToken & token)
    {
      return send_impl(std::move(value), token);
    }

    /** @brief Blocking in-place construction. Returns false if closed. */
    template <typename... Args>
      requires (sizeof...(Args) == 0)
    bool emplace(Args && ... args)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_full_.wait(lock, [&] { return closed_ or queue_.size() < capacity_; });
      if (closed_)
        return false;
      queue_.emplace_back(std::forward<Args>(args)...);
      lock.unlock();
      not_empty_.notify_one();
      return true;
    }

    /** @brief Blocking in-place construction. Returns false if closed. */
    template <typename First, typename... Rest>
      requires (not std::same_as<std::remove_cvref_t<First>, CancellationToken>)
    bool emplace(First && first, Rest && ... rest)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_full_.wait(lock, [&] { return closed_ or queue_.size() < capacity_; });
      if (closed_)
        return false;
      queue_.emplace_back(std::forward<First>(first),
                          std::forward<Rest>(rest)...);
      lock.unlock();
      not_empty_.notify_one();
      return true;
    }

    /** @brief Cancellation-aware blocking in-place construction.
     *  @throws operation_canceled if cancellation is requested while blocked.
     */
    template <typename... Args>
    bool emplace(const CancellationToken & token, Args && ... args)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_full_, lock,
                             [&] { return closed_ or queue_.size() < capacity_; },
                             token);
      if (closed_)
        return false;
      queue_.emplace_back(std::forward<Args>(args)...);
      lock.unlock();
      not_empty_.notify_one();
      return true;
    }

    /** @brief Non-blocking send. Returns false if full or closed. */
    bool try_send(const T & value)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_ or queue_.size() >= capacity_)
        return false;
      queue_.push_back(value);
      not_empty_.notify_one();
      return true;
    }

    /** @brief Non-blocking send. Returns false if full or closed. */
    bool try_send(T && value)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_ or queue_.size() >= capacity_)
        return false;
      queue_.push_back(std::move(value));
      not_empty_.notify_one();
      return true;
    }

    /** @brief Blocking receive into an output variable.
     *  @return false when the channel is closed and drained.
     */
    bool recv(T & out)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_empty_.wait(lock, [&] { return closed_ or not queue_.empty(); });
      if (queue_.empty())
        return false;
      out = std::move(queue_.front());
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return true;
    }

    /** @brief Cancellation-aware blocking receive into an output variable.
     *  @return false when the channel is closed and drained.
     *  @throws operation_canceled if cancellation is requested while blocked.
     */
    bool recv(T & out, const CancellationToken & token)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_empty_, lock,
                             [&] { return closed_ or not queue_.empty(); },
                             token);
      if (queue_.empty())
        return false;
      out = std::move(queue_.front());
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return true;
    }

    /** @brief Blocking receive.
     *  @return an item or `std::nullopt` when the channel is closed and empty.
     */
    [[nodiscard]] std::optional<T> recv()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_empty_.wait(lock, [&] { return closed_ or not queue_.empty(); });
      if (queue_.empty())
        return std::nullopt;
      T value = std::move(queue_.front());
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return value;
    }

    /** @brief Cancellation-aware blocking receive.
     *  @return an item or `std::nullopt` when the channel is closed and empty.
     *  @throws operation_canceled if cancellation is requested while blocked.
     */
    [[nodiscard]] std::optional<T> recv(const CancellationToken & token)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_empty_, lock,
                             [&] { return closed_ or not queue_.empty(); },
                             token);
      if (queue_.empty())
        return std::nullopt;
      T value = std::move(queue_.front());
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return value;
    }

    /** @brief Non-blocking receive into an output variable. */
    bool try_recv(T & out)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        return false;
      out = std::move(queue_.front());
      queue_.pop_front();
      not_full_.notify_one();
      return true;
    }

    /** @brief Non-blocking receive. */
    [[nodiscard]] std::optional<T> try_recv()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        return std::nullopt;
      T value = std::move(queue_.front());
      queue_.pop_front();
      not_full_.notify_one();
      return value;
    }
  };

  /** @brief Mutex-protected shared object wrapper.
   *
   *  The recommended interface is `with_lock()` for short critical sections.
   *  When the caller needs to hold the lock across multiple operations,
   *  `lock()` returns a small guard exposing `operator*` and `operator->`.
   *
   *  The callback passed to `with_lock()` should not return references or
   *  pointers that outlive the callback itself.
   */
  template <typename T, typename Mutex = std::mutex>
  class Synchronized
  {
  public:
    class LockedPtr
    {
      std::unique_lock<Mutex> lock_;
      T *ptr_;

    public:
      LockedPtr(Mutex & mutex, T & value)
        : lock_(mutex), ptr_(&value) {}

      [[nodiscard]] T & get() noexcept { return *ptr_; }
      [[nodiscard]] T * operator -> () noexcept { return ptr_; }
      [[nodiscard]] T & operator * () noexcept { return *ptr_; }
    };

    class ConstLockedPtr
    {
      std::unique_lock<Mutex> lock_;
      const T *ptr_;

    public:
      ConstLockedPtr(Mutex & mutex, const T & value)
        : lock_(mutex), ptr_(&value) {}

      [[nodiscard]] const T & get() const noexcept { return *ptr_; }
      [[nodiscard]] const T * operator -> () const noexcept { return ptr_; }
      [[nodiscard]] const T & operator * () const noexcept { return *ptr_; }
    };

  private:
    mutable Mutex mutex_;
    T value_;

  public:
    Synchronized() requires std::default_initializable<T> = default;

    explicit Synchronized(const T & value)
      : value_(value) {}

    explicit Synchronized(T && value)
      : value_(std::move(value)) {}

    template <typename... Args>
    explicit Synchronized(std::in_place_t, Args && ... args)
      : value_(std::forward<Args>(args)...) {}

    [[nodiscard]] LockedPtr lock() { return LockedPtr(mutex_, value_); }

    [[nodiscard]] ConstLockedPtr lock() const
    {
      return ConstLockedPtr(mutex_, value_);
    }

    template <typename F>
    decltype(auto) with_lock(F && f)
    {
      std::unique_lock<Mutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), value_);
    }

    template <typename F>
    decltype(auto) with_lock(F && f) const
    {
      std::unique_lock<Mutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), std::as_const(value_));
    }
  };

  /** @brief Read/write-lock protected shared object wrapper.
   *
   *  Readers acquire a shared lock through `read()` / `with_read_lock()`.
   *  Writers acquire an exclusive lock through `write()` / `with_write_lock()`.
   */
  template <typename T, typename SharedMutex = std::shared_mutex>
  class RwSynchronized
  {
  public:
    class ReadLockedPtr
    {
      std::shared_lock<SharedMutex> lock_;
      const T *ptr_;

    public:
      ReadLockedPtr(SharedMutex & mutex, const T & value)
        : lock_(mutex), ptr_(&value) {}

      [[nodiscard]] const T & get() const noexcept { return *ptr_; }
      [[nodiscard]] const T * operator -> () const noexcept { return ptr_; }
      [[nodiscard]] const T & operator * () const noexcept { return *ptr_; }
    };

    class WriteLockedPtr
    {
      std::unique_lock<SharedMutex> lock_;
      T *ptr_;

    public:
      WriteLockedPtr(SharedMutex & mutex, T & value)
        : lock_(mutex), ptr_(&value) {}

      [[nodiscard]] T & get() noexcept { return *ptr_; }
      [[nodiscard]] T * operator -> () noexcept { return ptr_; }
      [[nodiscard]] T & operator * () noexcept { return *ptr_; }
    };

  private:
    mutable SharedMutex mutex_;
    T value_;

  public:
    RwSynchronized() requires std::default_initializable<T> = default;

    explicit RwSynchronized(const T & value)
      : value_(value) {}

    explicit RwSynchronized(T && value)
      : value_(std::move(value)) {}

    template <typename... Args>
    explicit RwSynchronized(std::in_place_t, Args && ... args)
      : value_(std::forward<Args>(args)...) {}

    [[nodiscard]] ReadLockedPtr read() const
    {
      return ReadLockedPtr(mutex_, value_);
    }

    [[nodiscard]] WriteLockedPtr write()
    {
      return WriteLockedPtr(mutex_, value_);
    }

    template <typename F>
    decltype(auto) with_read_lock(F && f) const
    {
      std::shared_lock<SharedMutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), std::as_const(value_));
    }

    template <typename F>
    decltype(auto) with_write_lock(F && f)
    {
      std::unique_lock<SharedMutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), value_);
    }
  };

  /** @brief Bounded single-producer/single-consumer queue.
   *
   *  This queue is intentionally small and non-blocking. It is useful for
   *  low-overhead handoff between exactly one producer thread and one consumer
   *  thread. If you need multi-producer/multi-consumer coordination or close
   *  semantics, prefer \ref Aleph::BoundedChannel.
   */
  template <typename T>
  class SpscQueue
  {
    std::vector<std::optional<T>> slots_;
    size_t storage_size_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

    [[nodiscard]] size_t advance(size_t idx) const noexcept
    {
      return (idx + 1) % storage_size_;
    }

    template <typename... Args>
    bool emplace_impl(Args && ... args)
    {
      const size_t tail = tail_.load(std::memory_order_relaxed);
      const size_t next = advance(tail);
      if (next == head_.load(std::memory_order_acquire))
        return false;
      slots_[tail].emplace(std::forward<Args>(args)...);
      tail_.store(next, std::memory_order_release);
      return true;
    }

  public:
    explicit SpscQueue(size_t capacity)
      : slots_(capacity + 1), storage_size_(capacity + 1)
    {
      if (capacity == 0)
        throw std::invalid_argument("SpscQueue capacity must be greater than zero");
    }

    SpscQueue(const SpscQueue &) = delete;
    SpscQueue & operator = (const SpscQueue &) = delete;

    [[nodiscard]] size_t capacity() const noexcept { return storage_size_ - 1; }

    [[nodiscard]] bool empty() const noexcept
    {
      return head_.load(std::memory_order_acquire) ==
             tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool full() const noexcept
    {
      const size_t tail = tail_.load(std::memory_order_acquire);
      return advance(tail) == head_.load(std::memory_order_acquire);
    }

    [[nodiscard]] size_t size() const noexcept
    {
      const size_t head = head_.load(std::memory_order_acquire);
      const size_t tail = tail_.load(std::memory_order_acquire);
      if (tail >= head)
        return tail - head;
      return storage_size_ - head + tail;
    }

    bool try_push(const T & value) { return emplace_impl(value); }
    bool try_push(T && value) { return emplace_impl(std::move(value)); }

    template <typename... Args>
    bool emplace(Args && ... args)
    {
      return emplace_impl(std::forward<Args>(args)...);
    }

    bool try_pop(T & out)
    {
      const size_t head = head_.load(std::memory_order_relaxed);
      if (head == tail_.load(std::memory_order_acquire))
        return false;
      out = std::move(*slots_[head]);
      slots_[head].reset();
      head_.store(advance(head), std::memory_order_release);
      return true;
    }

    [[nodiscard]] std::optional<T> try_pop()
    {
      const size_t head = head_.load(std::memory_order_relaxed);
      if (head == tail_.load(std::memory_order_acquire))
        return std::nullopt;
      T value = std::move(*slots_[head]);
      slots_[head].reset();
      head_.store(advance(head), std::memory_order_release);
      return value;
    }
  };

  template <typename T>
  using bounded_channel = BoundedChannel<T>;

  template <typename T, typename Mutex = std::mutex>
  using synchronized = Synchronized<T, Mutex>;

  template <typename T, typename SharedMutex = std::shared_mutex>
  using rw_synchronized = RwSynchronized<T, SharedMutex>;

  template <typename T>
  using spsc_queue = SpscQueue<T>;
}

#endif
