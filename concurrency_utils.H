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
   *  `BoundedChannel` provides a fixed-capacity FIFO queue for passing values
   *  between any number of producer threads and any number of consumer
   *  threads. All public member functions are thread-safe. Distinct senders
   *  and receivers may operate concurrently, but individual calls are not
   *  reentrant on the same object because they synchronize through an internal
   *  mutex and condition variables.
   *
   *  Blocking operations wait for space or data:
   *  - `send()` / `emplace()` block while the channel is full.
   *  - `recv()` blocks while the channel is empty.
   *  - `try_*` operations never block.
   *
   *  Close semantics:
   *  - `close()` wakes all blocked senders and receivers.
   *  - Once closed, future sends fail and return `false`.
   *  - Receivers continue draining already queued items.
   *  - After the queue is drained, receives report end-of-stream by returning
   *    `false` or `std::nullopt`.
   *
   *  Cancellation-aware overloads accept a \ref Aleph::CancellationToken. They
   *  preserve the same close semantics and throw
   *  \ref Aleph::operation_canceled if cancellation is requested while the
   *  caller is blocked waiting for progress.
   *
   *  Exception safety:
   *  - If element construction or assignment throws, the channel remains in a
   *    valid state and the element stays queued until extraction succeeds.
   *  - Methods marked `noexcept` do not throw; all others may propagate
   *    exceptions from user-supplied element operations or cancellation.
   *
   *  @tparam T The type of elements stored in the channel.
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

    /** @brief Wait on a condition variable with cancellation support.
     *
     *  @param cv Condition variable to wait on.
     *  @param lock Locked mutex protecting the condition.
     *  @param pred Predicate that must be true to stop waiting.
     *  @param token Token to monitor for cancellation.
     *  @throws Aleph::operation_canceled if cancellation is requested.
     */
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

    /** @brief Common implementation for send operations.
     *
     *  @tparam U Type of the value to send.
     *  @param value Value to push into the queue.
     *  @return true if sent, false if the channel was closed.
     */
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

    /** @brief Common implementation for cancellation-aware send operations.
     *
     *  @tparam U Type of the value to send.
     *  @param value Value to push into the queue.
     *  @param token Token for cooperative cancellation.
     *  @return true if sent, false if the channel was closed.
     *  @throws Aleph::operation_canceled if canceled while blocked.
     */
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
    /** @brief Construct a channel with a fixed capacity.
     *  @param capacity Maximum number of items the channel can hold.
     *  @throws std::invalid_argument if @p capacity is zero.
     *  @note This constructor does not block and is not thread-safe against
     *        concurrent access to the object being constructed.
     */
    explicit BoundedChannel(size_t capacity)
      : capacity_(capacity)
    {
      if (capacity_ == 0)
        throw std::invalid_argument("BoundedChannel capacity must be greater than zero");
    }

    BoundedChannel(const BoundedChannel &) = delete;
    BoundedChannel & operator = (const BoundedChannel &) = delete;

    /** @brief Return the maximum number of queued items.
     *  @return The channel's capacity.
     *  @throws Nothing.
     *  @note Thread-safe, non-blocking, reentrant, and `noexcept`.
     */
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }

    /** @brief Check whether the channel has been closed.
     *  @return `true` if `close()` has been called, otherwise `false`.
     *  @throws Nothing unless locking the internal mutex throws.
     *  @note Thread-safe, non-reentrant on the same object, and does not block
     *        except to acquire the internal mutex.
     */
    [[nodiscard]] bool is_closed() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return closed_;
    }

    /** @brief Return the number of currently queued items.
     *  @return Current size of the internal queue.
     *  @throws Nothing unless locking the internal mutex throws.
     *  @note Thread-safe, non-reentrant on the same object, and does not block
     *        except to acquire the internal mutex.
     */
    [[nodiscard]] size_t size() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.size();
    }

    /** @brief Check whether the channel currently holds no queued items.
     *  @return `true` if there are no queued items, otherwise `false`.
     *  @throws Nothing unless locking the internal mutex throws.
     *  @note Thread-safe, non-reentrant on the same object, and does not block
     *        except to acquire the internal mutex.
     */
    [[nodiscard]] bool empty() const
    {
      std::lock_guard<std::mutex> lock(mutex_);
      return queue_.empty();
    }

    /** @brief Close the channel.
     *
     *  Wakes up all blocked senders and receivers. Blocked senders will
     *  subsequently return `false`. Blocked receivers will drain any remaining
     *  items before returning `false` or `std::nullopt`.
     *
     *  Calling `close()` on an already closed channel is a no-op.
     *
     *  @note **Returns:** Nothing. 
     *  @throws Nothing unless locking the internal mutex throws.
     *  @note Thread-safe, non-reentrant on the same object, and non-blocking
     *        apart from internal mutex acquisition. After closure, no send or
     *        emplace operation can succeed.
     */
    void close()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_)
        return;
      closed_ = true;
      not_empty_.notify_all();
      not_full_.notify_all();
    }

    /** @brief Send a value by copying it into the channel.
     *
     *  Blocks the calling thread if the channel is full until space becomes
     *  available or the channel is closed.
     *
     *  @param value The item to send.
     *  @return `true` if the value was queued, `false` if the channel was
     *          already closed before space became available.
     *  @throws Any exception thrown while copying `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        On close, blocked senders wake and return `false`.
     */
    bool send(const T & value) { return send_impl(value); }

    /** @brief Send a value by moving it into the channel.
     *
     *  Blocks the calling thread if the channel is full until space becomes
     *  available or the channel is closed.
     *
     *  @param value The item to send.
     *  @return `true` if the value was queued, `false` if the channel was
     *          already closed before space became available.
     *  @throws Any exception thrown while moving `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        On close, blocked senders wake and return `false`.
     */
    bool send(T && value) { return send_impl(std::move(value)); }

    /** @brief Cancellation-aware blocking send (copy).
     *
     *  Blocks like `send(const T&)` but monitors @p token for cancellation.
     *
     *  @param value The item to send.
     *  @param token Token for cooperative cancellation.
     *  @return `true` if the value was queued, `false` if the channel was
     *          closed before space became available.
     *  @throws Aleph::operation_canceled if cancellation is requested while the
     *          call is blocked waiting for space.
     *  @throws Any exception thrown while copying `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If the channel closes first, blocked senders return `false`
     *        instead of throwing.
     */
    bool send(const T & value, const CancellationToken & token)
    {
      return send_impl(value, token);
    }

    /** @brief Cancellation-aware blocking send (move).
     *
     *  Blocks like `send(T&&)` but monitors @p token for cancellation.
     *
     *  @param value The item to send.
     *  @param token Token for cooperative cancellation.
     *  @return `true` if the value was queued, `false` if the channel was
     *          closed before space became available.
     *  @throws Aleph::operation_canceled if cancellation is requested while the
     *          call is blocked waiting for space.
     *  @throws Any exception thrown while moving `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If the channel closes first, blocked senders return `false`
     *        instead of throwing.
     */
    bool send(T && value, const CancellationToken & token)
    {
      return send_impl(std::move(value), token);
    }

    /** @brief Construct an element in-place at the back of the queue (no args).
     *
     *  Blocks if full. Returns false if closed.
     *
     *  @param args Constructor arguments for `T` (empty by construction).
     *  @return `true` if the element was queued, `false` if the channel was
     *          closed before space became available.
     *  @throws Any exception thrown while constructing `T` in-place.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        On close, blocked callers wake and return `false`.
     */
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

    /** @brief Construct an element in-place at the back of the queue.
     *
     *  Blocks if full. Returns false if closed.
     *
     *  @tparam First First argument type.
     *  @tparam Rest Remaining argument types.
     *  @param first First constructor argument.
     *  @param rest Remaining constructor arguments.
     *  @return `true` if the element was queued, `false` if the channel was
     *          closed before space became available.
     *  @throws Any exception thrown while constructing `T` in-place.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If `T` itself is constructible from `CancellationToken`, this
     *        overload remains eligible and is preferred for payload
     *        construction.
     */
    template <typename First, typename... Rest>
      requires std::constructible_from<T, First, Rest...>
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

    /** @brief Cancellation-aware in-place construction.
     *
     *  Blocks like `emplace()` but monitors @p token for cancellation.
     *
     *  @tparam First Type of the first argument (must be CancellationToken).
     *  @tparam Args Types of constructor arguments.
     *  @param token Token for cooperative cancellation.
     *  @param args Constructor arguments.
     *  @return `true` if the element was queued, `false` if the channel was
     *          closed before space became available.
     *  @throws Aleph::operation_canceled if cancellation is requested while the
     *          call is blocked waiting for space.
     *  @throws Any exception thrown while constructing `T` in-place.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        This overload only participates when the first argument is used as
     *        a control token rather than a payload constructor argument.
     */
    template <typename First, typename... Args>
      requires std::same_as<std::remove_cvref_t<First>, CancellationToken> &&
               (not std::constructible_from<T, First, Args...>) &&
               std::constructible_from<T, Args...>
    bool emplace(First && token, Args && ... args)
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

    /** @brief Attempt to send a value without blocking.
     *
     *  @param value Value to send.
     *  @return `true` if the value was queued immediately, `false` if the
     *          channel is full or closed.
     *  @throws Any exception thrown while copying `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and never blocks
     *        waiting for capacity. Close is observed immediately.
     */
    bool try_send(const T & value)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_ or queue_.size() >= capacity_)
        return false;
      queue_.push_back(value);
      not_empty_.notify_one();
      return true;
    }

    /** @brief Attempt to send a value without blocking (move).
     *
     *  @param value Value to send.
     *  @return `true` if the value was queued immediately, `false` if the
     *          channel is full or closed.
     *  @throws Any exception thrown while moving `T` into the queue.
     *  @note Thread-safe, not reentrant on the same object, and never blocks
     *        waiting for capacity. Close is observed immediately.
     */
    bool try_send(T && value)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_ or queue_.size() >= capacity_)
        return false;
      queue_.push_back(std::move(value));
      not_empty_.notify_one();
      return true;
    }

    /** @brief Receive an item from the channel into @p out.
     *
     *  Blocks the calling thread if the channel is empty until an item is
     *  available or the channel is closed and drained.
     *
     *  @param out Reference where the received item will be moved.
     *  @return `true` if an item was received, `false` if the channel is
     *          closed and empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into a temporary or assigning into @p out.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If extraction throws, the queued element remains present.
     */
    bool recv(T & out)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_empty_.wait(lock, [&] { return closed_ or not queue_.empty(); });
      if (queue_.empty())
        return false;
      T value(std::move_if_noexcept(queue_.front()));
      out = std::move(value);
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return true;
    }

    /** @brief Cancellation-aware blocking receive into @p out.
     *
     *  Blocks like `recv(T&)` but monitors @p token for cancellation.
     *
     *  @param out Reference where the received item will be moved.
     *  @param token Token for cooperative cancellation.
     *  @return `true` if an item was received, `false` if the channel is
     *          closed and empty.
     *  @throws Aleph::operation_canceled if cancellation is requested while the
     *          call is blocked waiting for data.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into a temporary or assigning into @p out.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If extraction throws, the queued element remains present.
     */
    bool recv(T & out, const CancellationToken & token)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_empty_, lock,
                             [&] { return closed_ or not queue_.empty(); },
                             token);
      if (queue_.empty())
        return false;
      T value(std::move_if_noexcept(queue_.front()));
      out = std::move(value);
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      return true;
    }

    /** @brief Receive the next item from the channel.
     *
     *  Blocks until an item becomes available or the channel closes and drains.
     *
     *  @return The next queued item wrapped in `std::optional`, or
     *          `std::nullopt` if the channel is closed and empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into the returned optional.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If optional construction throws, the queued element remains
     *        present.
     */
    [[nodiscard]] std::optional<T> recv()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      not_empty_.wait(lock, [&] { return closed_ or not queue_.empty(); });
      if (queue_.empty())
        return std::nullopt;
      T value(std::move_if_noexcept(queue_.front()));
      std::optional<T> result;
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        result.emplace(std::move(value));
      else
        result.emplace(value);
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        return result;
      else
        return static_cast<const std::optional<T> &>(result);
    }

    /** @brief Cancellation-aware receive of the next item from the channel.
     *
     *  Blocks like `recv()` but monitors @p token for cancellation.
     *
     *  @param token Token for cooperative cancellation.
     *  @return The next queued item wrapped in `std::optional`, or
     *          `std::nullopt` if the channel is closed and empty.
     *  @throws Aleph::operation_canceled if cancellation is requested while the
     *          call is blocked waiting for data.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into the returned optional.
     *  @note Thread-safe, not reentrant on the same object, and may block.
     *        If optional construction throws, the queued element remains
     *        present.
     */
    [[nodiscard]] std::optional<T> recv(const CancellationToken & token)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      wait_with_cancellation(not_empty_, lock,
                             [&] { return closed_ or not queue_.empty(); },
                             token);
      if (queue_.empty())
        return std::nullopt;
      T value(std::move_if_noexcept(queue_.front()));
      std::optional<T> result;
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        result.emplace(std::move(value));
      else
        result.emplace(value);
      queue_.pop_front();
      lock.unlock();
      not_full_.notify_one();
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        return result;
      else
        return static_cast<const std::optional<T> &>(result);
    }

    /** @brief Attempt to receive an item into @p out without blocking.
     *
     *  @param out Reference where the received item will be moved.
     *  @return `true` if an item was received immediately, `false` if the
     *          channel is empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into a temporary or assigning into @p out.
     *  @note Thread-safe, not reentrant on the same object, and never blocks
     *        waiting for data. If extraction throws, the queued element remains
     *        present.
     */
    bool try_recv(T & out)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        return false;
      T value(std::move_if_noexcept(queue_.front()));
      out = std::move(value);
      queue_.pop_front();
      not_full_.notify_one();
      return true;
    }

    /** @brief Attempt to receive an item without blocking.
     *
     *  @return The next queued item wrapped in `std::optional`, or
     *          `std::nullopt` if the channel is empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into the returned optional.
     *  @note Thread-safe, not reentrant on the same object, and never blocks
     *        waiting for data. If optional construction throws, the queued
     *        element remains present.
     */
    [[nodiscard]] std::optional<T> try_recv()
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (queue_.empty())
        return std::nullopt;
      T value(std::move_if_noexcept(queue_.front()));
      std::optional<T> result;
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        result.emplace(std::move(value));
      else
        result.emplace(value);
      queue_.pop_front();
      not_full_.notify_one();
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        return result;
      else
        return static_cast<const std::optional<T> &>(result);
    }
  };

  /** @brief Mutex-protected shared object wrapper.
   *
   *  `Synchronized<T>` encapsulates a value of type `T` and a mutex. Access to
   *  the value is only possible by acquiring a lock, ensuring thread-safe
   *  concurrent access. All public member functions are thread-safe. Calls are
   *  not reentrant on the same object unless `Mutex` itself is recursive.
   *
   *  The recommended interface is `with_lock()` for short critical sections.
   *  When the caller needs to hold the lock across multiple operations,
   *  `lock()` returns a small guard (`LockedPtr` or `ConstLockedPtr`)
   *  exposing `operator*` and `operator->`.
   *
   *  The callback passed to `with_lock()` should not return references or
   *  pointers that outlive the callback itself.
   *
   *  @tparam T The type of the protected object.
   *  @tparam Mutex The mutex type (defaults to `std::mutex`).
   */
  template <typename T, typename Mutex = std::mutex>
  class Synchronized
  {
  public:
    /** @brief RAII guard for exclusive access to the synchronized value.
     *  @note Thread-safe to use only while the guard object remains alive on
     *        the thread that acquired it. The destructor releases the lock.
     */
    class LockedPtr
    {
      std::unique_lock<Mutex> lock_;
      T *ptr_;

    public:
      /** @brief Construct a guard and acquire the lock.
       *  @param mutex Mutex protecting the value.
       *  @param value Protected object to expose through the guard.
       *  @throws Any exception thrown while locking `mutex`.
       */
      LockedPtr(Mutex & mutex, T & value)
        : lock_(mutex), ptr_(&value) {}

      /** @brief Access the protected value.
       *  @return A mutable reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T & get() noexcept { return *ptr_; }
      /** @brief Access members of the protected value.
       *  @return A mutable pointer to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T * operator -> () noexcept { return ptr_; }
      /** @brief Access the protected value.
       *  @return A mutable reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T & operator * () noexcept { return *ptr_; }
    };

    /** @brief RAII guard for read-only access to the synchronized value.
     *  @note Thread-safe to use only while the guard object remains alive on
     *        the thread that acquired it. The destructor releases the lock.
     */
    class ConstLockedPtr
    {
      std::unique_lock<Mutex> lock_;
      const T *ptr_;

    public:
      /** @brief Construct a guard and acquire the lock.
       *  @param mutex Mutex protecting the value.
       *  @param value Protected object to expose through the guard.
       *  @throws Any exception thrown while locking `mutex`.
       */
      ConstLockedPtr(Mutex & mutex, const T & value)
        : lock_(mutex), ptr_(&value) {}

      /** @brief Access the protected value.
       *  @return A const reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T & get() const noexcept { return *ptr_; }
      /** @brief Access members of the protected value.
       *  @return A const pointer to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T * operator -> () const noexcept { return ptr_; }
      /** @brief Access the protected value.
       *  @return A const reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T & operator * () const noexcept { return *ptr_; }
    };

  private:
    mutable Mutex mutex_;
    T value_;

  public:
    /** @brief Default-construct the protected value.
     *  @throws Any exception thrown by `T`'s default constructor.
     */
    Synchronized() requires std::default_initializable<T> = default;

    /** @brief Construct from a copy of @p value.
     *  @param value Initial value to copy into the wrapper.
     *  @throws Any exception thrown while copying `T`.
     */
    explicit Synchronized(const T & value)
      : value_(value) {}

    /** @brief Construct by moving from @p value.
     *  @param value Initial value to move into the wrapper.
     *  @throws Any exception thrown while moving `T`.
     */
    explicit Synchronized(T && value)
      : value_(std::move(value)) {}

    /** @brief Construct the protected value in-place.
     *  @tparam Args Constructor argument types.
     *  @param args Arguments to pass to the constructor of T.
     *  @throws Any exception thrown by `T`'s constructor.
     */
    template <typename... Args>
    explicit Synchronized(std::in_place_t, Args && ... args)
      : value_(std::forward<Args>(args)...) {}

    /** @brief Acquire an exclusive lock and return a guard.
     *  @return A `LockedPtr` that keeps the mutex locked until destruction.
     *  @throws Any exception thrown while locking `Mutex`.
     *  @note Thread-safe. This call may block until another holder releases the
     *        lock. Reentrancy depends on `Mutex`.
     */
    [[nodiscard]] LockedPtr lock() { return LockedPtr(mutex_, value_); }

    /** @brief Acquire an exclusive lock (for const access) and return a guard.
     *  @return A `ConstLockedPtr` that keeps the mutex locked until
     *          destruction.
     *  @throws Any exception thrown while locking `Mutex`.
     *  @note Thread-safe. This call may block until another holder releases the
     *        lock. Reentrancy depends on `Mutex`.
     */
    [[nodiscard]] ConstLockedPtr lock() const
    {
      return ConstLockedPtr(mutex_, value_);
    }

    /** @brief Execute a callback with exclusive access to the value.
     *
     *  Acquires the lock, calls @p f with a reference to the protected value,
     *  and releases the lock when the callback returns.
     *
     *  @tparam F Callback type.
     *  @param f The callback to execute.
     *  @return The value returned by @p f.
     *  @throws Any exception thrown while locking `Mutex` or by the callback
     *          itself.
     *  @note Thread-safe and may block waiting for the mutex. Not reentrant on
     *        the same object unless `Mutex` allows recursive locking. The
     *        callback must not return a reference; otherwise compilation fails
     *        to prevent dangling references after the lock is released.
     */
    template <typename F>
    decltype(auto) with_lock(F && f)
    {
      using Result = std::invoke_result_t<F, T &>;
      static_assert(not std::is_reference_v<Result>,
                    "Synchronized::with_lock callback must not return a reference");
      std::unique_lock<Mutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), value_);
    }

    /** @brief Execute a callback with read-only access to the value.
     *
     *  Acquires the lock, calls @p f with a const reference to the value,
     *  and releases the lock when the callback returns.
     *
     *  @tparam F Callback type.
     *  @param f The callback to execute.
     *  @return The value returned by @p f.
     *  @throws Any exception thrown while locking `Mutex` or by the callback
     *          itself.
     *  @note Thread-safe and may block waiting for the mutex. Not reentrant on
     *        the same object unless `Mutex` allows recursive locking. The
     *        callback must not return a reference; otherwise compilation fails
     *        to prevent dangling references after the lock is released.
     */
    template <typename F>
    decltype(auto) with_lock(F && f) const
    {
      using Result = std::invoke_result_t<F, const T &>;
      static_assert(not std::is_reference_v<Result>,
                    "Synchronized::with_lock callback must not return a reference");
      std::unique_lock<Mutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), std::as_const(value_));
    }
  };

  /** @brief Read/write-lock protected shared object wrapper.
   *
   *  `RwSynchronized<T>` encapsulates a value and a shared mutex, allowing
   *  multiple concurrent readers OR a single exclusive writer. All public
   *  member functions are thread-safe. Calls are not reentrant on the same
   *  object unless `SharedMutex` itself supports recursive acquisition.
   *
   *  Readers acquire a shared lock through `read()` / `with_read_lock()`.
   *  Writers acquire an exclusive lock through `write()` / `with_write_lock()`.
   *
   *  @tparam T The type of the protected object.
   *  @tparam SharedMutex The shared mutex type (defaults to `std::shared_mutex`).
   */
  template <typename T, typename SharedMutex = std::shared_mutex>
  class RwSynchronized
  {
  public:
    /** @brief RAII guard for shared (read-only) access.
     *  @note The guard holds a shared lock until destruction.
     */
    class ReadLockedPtr
    {
      std::shared_lock<SharedMutex> lock_;
      const T *ptr_;

    public:
      /** @brief Construct a guard and acquire a shared lock.
       *  @param mutex Shared mutex protecting the value.
       *  @param value Protected object to expose through the guard.
       *  @throws Any exception thrown while locking `mutex`.
       */
      ReadLockedPtr(SharedMutex & mutex, const T & value)
        : lock_(mutex), ptr_(&value) {}

      /** @brief Access the protected value.
       *  @return A const reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T & get() const noexcept { return *ptr_; }
      /** @brief Access members of the protected value.
       *  @return A const pointer to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T * operator -> () const noexcept { return ptr_; }
      /** @brief Access the protected value.
       *  @return A const reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] const T & operator * () const noexcept { return *ptr_; }
    };

    /** @brief RAII guard for exclusive (write) access.
     *  @note The guard holds an exclusive lock until destruction.
     */
    class WriteLockedPtr
    {
      std::unique_lock<SharedMutex> lock_;
      T *ptr_;

    public:
      /** @brief Construct a guard and acquire an exclusive lock.
       *  @param mutex Shared mutex protecting the value.
       *  @param value Protected object to expose through the guard.
       *  @throws Any exception thrown while locking `mutex`.
       */
      WriteLockedPtr(SharedMutex & mutex, T & value)
        : lock_(mutex), ptr_(&value) {}

      /** @brief Access the protected value.
       *  @return A mutable reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T & get() noexcept { return *ptr_; }
      /** @brief Access members of the protected value.
       *  @return A mutable pointer to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T * operator -> () noexcept { return ptr_; }
      /** @brief Access the protected value.
       *  @return A mutable reference to the protected object.
       *  @throws Nothing.
       */
      [[nodiscard]] T & operator * () noexcept { return *ptr_; }
    };

  private:
    mutable SharedMutex mutex_;
    T value_;

  public:
    /** @brief Default-construct the protected value.
     *  @throws Any exception thrown by `T`'s default constructor.
     */
    RwSynchronized() requires std::default_initializable<T> = default;

    /** @brief Construct from a copy of @p value.
     *  @param value Initial value to copy into the wrapper.
     *  @throws Any exception thrown while copying `T`.
     */
    explicit RwSynchronized(const T & value)
      : value_(value) {}

    /** @brief Construct by moving from @p value.
     *  @param value Initial value to move into the wrapper.
     *  @throws Any exception thrown while moving `T`.
     */
    explicit RwSynchronized(T && value)
      : value_(std::move(value)) {}

    /** @brief Construct the protected value in-place.
     *  @tparam Args Constructor argument types.
     *  @param args Arguments to pass to the constructor of T.
     */
    template <typename... Args>
    explicit RwSynchronized(std::in_place_t, Args && ... args)
      : value_(std::forward<Args>(args)...) {}

    /** @brief Acquire a shared lock and return a guard.
     *  @return A `ReadLockedPtr` that keeps a shared lock until destruction.
     *  @throws Any exception thrown while locking `SharedMutex`.
     *  @note Thread-safe. This call may block until there is no active writer.
     */
    [[nodiscard]] ReadLockedPtr read() const
    {
      return ReadLockedPtr(mutex_, value_);
    }

    /** @brief Acquire an exclusive lock and return a guard.
     *  @return A `WriteLockedPtr` that keeps an exclusive lock until
     *          destruction.
     *  @throws Any exception thrown while locking `SharedMutex`.
     *  @note Thread-safe. This call may block until all readers and writers
     *        have released the mutex.
     */
    [[nodiscard]] WriteLockedPtr write()
    {
      return WriteLockedPtr(mutex_, value_);
    }

    /** @brief Execute a callback with shared (read-only) access.
     *
     *  @tparam F Callback type.
     *  @param f The callback to execute.
     *  @return The value returned by @p f.
     *  @throws Any exception thrown while locking `SharedMutex` or by the
     *          callback itself.
     *  @note Thread-safe and may block waiting for active writers. The
     *        callback must not return a reference; otherwise compilation fails
     *        to prevent dangling references after the lock is released.
     */
    template <typename F>
    decltype(auto) with_read_lock(F && f) const
    {
      using Result = std::invoke_result_t<F, const T &>;
      static_assert(not std::is_reference_v<Result>,
                    "RwSynchronized::with_read_lock callback must not return a reference");
      std::shared_lock<SharedMutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), std::as_const(value_));
    }

    /** @brief Execute a callback with exclusive (write) access.
     *
     *  @tparam F Callback type.
     *  @param f The callback to execute.
     *  @return The value returned by @p f.
     *  @throws Any exception thrown while locking `SharedMutex` or by the
     *          callback itself.
     *  @note Thread-safe and may block waiting for other readers or writers.
     *        The callback must not return a reference; otherwise compilation
     *        fails to prevent dangling references after the lock is released.
     */
    template <typename F>
    decltype(auto) with_write_lock(F && f)
    {
      using Result = std::invoke_result_t<F, T &>;
      static_assert(not std::is_reference_v<Result>,
                    "RwSynchronized::with_write_lock callback must not return a reference");
      std::unique_lock<SharedMutex> lock(mutex_);
      return std::invoke(std::forward<F>(f), value_);
    }
  };

  /** @brief Bounded single-producer/single-consumer queue.
   *
   *  This queue is a lock-free, bounded circular buffer designed for high-
   *  performance handoff between exactly one producer thread and one
   *  consumer thread. It is thread-safe only for that single-producer /
   *  single-consumer usage pattern. Concurrent calls from multiple producers
   *  or multiple consumers are not supported.
   *
   *  @warning This class is NOT thread-safe for multiple producers or multiple
   *           consumers. Use \ref Aleph::BoundedChannel for MPMC scenarios.
   *
   *  Operations are non-blocking and never close. Producers observe failure by
   *  receiving `false` when the ring is full; consumers observe emptiness by
   *  receiving `false` or `std::nullopt`. If element extraction throws, the
   *  slot remains occupied until extraction succeeds.
   *
   *  @tparam T The type of elements stored in the queue.
   */
  template <typename T>
  class SpscQueue
  {
    std::vector<std::optional<T>> slots_;
    size_t storage_size_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

    /** @brief Compute the next index in the circular buffer. */
    [[nodiscard]] size_t advance(size_t idx) const noexcept
    {
      return (idx + 1) % storage_size_;
    }

    /** @brief Internal implementation for push/emplace. */
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
    /** @brief Construct a queue with the specified capacity.
     *  @param capacity Maximum number of items the queue can hold.
     *  @throws std::invalid_argument if @p capacity is zero.
     *  @note This constructor does not block and is not thread-safe against
     *        concurrent access to the object being constructed.
     */
    explicit SpscQueue(size_t capacity)
      : slots_(capacity + 1), storage_size_(capacity + 1)
    {
      if (capacity == 0)
        throw std::invalid_argument("SpscQueue capacity must be greater than zero");
    }

    SpscQueue(const SpscQueue &) = delete;
    SpscQueue & operator = (const SpscQueue &) = delete;

    /** @brief Return the queue's capacity.
     *  @return The maximum number of user-visible items.
     *  @throws Nothing.
     *  @note Safe to call from either the producer or consumer thread,
     *        non-blocking, reentrant, and `noexcept`.
     */
    [[nodiscard]] size_t capacity() const noexcept { return storage_size_ - 1; }

    /** @brief Check if the queue is empty.
     *  @return `true` if the queue currently contains no items.
     *  @throws Nothing.
     *  @note Safe to call from either the producer or consumer thread.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return head_.load(std::memory_order_acquire) ==
             tail_.load(std::memory_order_acquire);
    }

    /** @brief Check if the queue is full.
     *  @return `true` if the queue currently has no free slots.
     *  @throws Nothing.
     *  @note Safe to call from either the producer or consumer thread.
     */
    [[nodiscard]] bool full() const noexcept
    {
      const size_t tail = tail_.load(std::memory_order_acquire);
      return advance(tail) == head_.load(std::memory_order_acquire);
    }

    /** @brief Return the number of items currently in the queue.
     *  @return The current number of queued items.
     *  @throws Nothing.
     *  @note Safe to call from either the producer or consumer thread.
     */
    [[nodiscard]] size_t size() const noexcept
    {
      const size_t head = head_.load(std::memory_order_acquire);
      const size_t tail = tail_.load(std::memory_order_acquire);
      if (tail >= head)
        return tail - head;
      return storage_size_ - head + tail;
    }

    /** @brief Attempt to push an item (copy) into the queue.
     *  @param value The item to push.
     *  @return `true` if the item was enqueued, `false` if the queue is full.
     *  @throws Any exception thrown while copying `T` into the ring buffer.
     *  @note Non-blocking and only safe to call from the single producer
     *        thread.
     */
    bool try_push(const T & value) { return emplace_impl(value); }

    /** @brief Attempt to push an item (move) into the queue.
     *  @param value The item to push.
     *  @return `true` if the item was enqueued, `false` if the queue is full.
     *  @throws Any exception thrown while moving `T` into the ring buffer.
     *  @note Non-blocking and only safe to call from the single producer
     *        thread.
     */
    bool try_push(T && value) { return emplace_impl(std::move(value)); }

    /** @brief Construct an item in-place at the tail of the queue.
     *  @tparam Args Constructor argument types.
     *  @param args Constructor arguments.
     *  @return `true` if the item was enqueued, `false` if the queue is full.
     *  @throws Any exception thrown while constructing `T` in-place.
     *  @note Non-blocking and only safe to call from the single producer
     *        thread.
     */
    template <typename... Args>
    bool emplace(Args && ... args)
    {
      return emplace_impl(std::forward<Args>(args)...);
    }

    /** @brief Attempt to pop an item from the queue into @p out.
     *  @param out Reference where the popped item will be moved.
     *  @return `true` if an item was extracted, `false` if the queue is empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into a temporary or assigning into @p out.
     *  @note Non-blocking and only safe to call from the single consumer
     *        thread. If extraction throws, the slot remains occupied.
     */
    bool try_pop(T & out)
    {
      const size_t head = head_.load(std::memory_order_relaxed);
      if (head == tail_.load(std::memory_order_acquire))
        return false;
      T value(std::move_if_noexcept(*slots_[head]));
      out = std::move(value);
      slots_[head].reset();
      head_.store(advance(head), std::memory_order_release);
      return true;
    }

    /** @brief Attempt to pop an item from the queue.
     *  @return The next item wrapped in `std::optional`, or `std::nullopt` if
     *          the queue is empty.
     *  @throws Any exception thrown while copying or moving the queued value
     *          into the returned optional.
     *  @note Non-blocking and only safe to call from the single consumer
     *        thread. If optional construction throws, the slot remains
     *        occupied.
     */
    [[nodiscard]] std::optional<T> try_pop()
    {
      const size_t head = head_.load(std::memory_order_relaxed);
      if (head == tail_.load(std::memory_order_acquire))
        return std::nullopt;
      T value(std::move_if_noexcept(*slots_[head]));
      std::optional<T> result;
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        result.emplace(std::move(value));
      else
        result.emplace(value);
      slots_[head].reset();
      head_.store(advance(head), std::memory_order_release);
      if constexpr (std::is_nothrow_move_constructible_v<T> or
                    not std::is_copy_constructible_v<T>)
        return result;
      else
        return static_cast<const std::optional<T> &>(result);
    }
  };

  /** @brief Alias for \ref Aleph::BoundedChannel.
   *  @tparam T Value type stored in the channel.
   *  @note Inherits the same thread-safety, blocking, close, and cancellation
   *        semantics as `BoundedChannel<T>`.
   */
  template <typename T>
  using bounded_channel = BoundedChannel<T>;

  /** @brief Alias for \ref Aleph::Synchronized.
   *  @tparam T Protected value type.
   *  @tparam Mutex Mutex type guarding the value.
   *  @note Inherits the same thread-safety and callback lifetime rules as
   *        `Synchronized<T, Mutex>`.
   */
  template <typename T, typename Mutex = std::mutex>
  using synchronized = Synchronized<T, Mutex>;

  /** @brief Alias for \ref Aleph::RwSynchronized.
   *  @tparam T Protected value type.
   *  @tparam SharedMutex Shared mutex type guarding the value.
   *  @note Inherits the same reader/writer concurrency and callback lifetime
   *        rules as `RwSynchronized<T, SharedMutex>`.
   */
  template <typename T, typename SharedMutex = std::shared_mutex>
  using rw_synchronized = RwSynchronized<T, SharedMutex>;

  /** @brief Alias for \ref Aleph::SpscQueue.
   *  @tparam T Value type stored in the queue.
   *  @note Inherits the same non-blocking single-producer/single-consumer
   *        semantics as `SpscQueue<T>`.
   */
  template <typename T>
  using spsc_queue = SpscQueue<T>;
}

#endif
