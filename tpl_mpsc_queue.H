
/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file tpl_mpsc_queue.H
 *  @brief Unbounded lock-free multi-producer/single-consumer queue
 *  (`Aleph::MpscQueue`).
 *
 *  `MpscQueue<T>` is an intrusive, singly-linked, unbounded FIFO queue that
 *  any number of producer threads can push into concurrently, while exactly
 *  one consumer thread pops from it. It implements the well-known
 *  Vyukov intrusive MPSC queue algorithm
 *  (<http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue>):
 *  each push is a single atomic exchange (lock-free, wait-free per push),
 *  and the consumer walks the resulting singly-linked list without ever
 *  taking a lock.
 *
 *  How it differs from the existing queues in this library:
 *  - `Aleph::SpscQueue<T>` (in `concurrency_utils.H`) is bounded and
 *    supports exactly one producer and one consumer; `MpscQueue` supports
 *    any number of producers but still exactly one consumer, and it never
 *    reports "full" (it grows one node per push, released back to the heap
 *    as the consumer pops).
 *  - `Aleph::BoundedChannel<T>` is a general MPMC blocking channel.
 *    `MpscQueue` is narrower (MPSC only) but non-blocking end to end: a
 *    push never waits, and `try_pop` never waits.
 *
 *  @par Thread-safety
 *  `push`/`emplace` may be called concurrently from any number of producer
 *  threads. `try_pop`/`is_empty` must only ever be called from a single,
 *  consistent consumer thread (never concurrently with each other, and
 *  never from more than one thread). Calling a consumer-side operation from
 *  more than one thread, or concurrently with another consumer-side call,
 *  is a precondition violation with no defined behavior -- see
 *  `docs/data_structure_thread_safety.md` for the vocabulary this file
 *  follows (this is an MPSC topology).
 *
 *  @par Progress guarantees
 *  Every `push`/`emplace` is lock-free: it completes after one atomic
 *  read-modify-write (`exchange`) on the shared insertion point, regardless
 *  of what other producers are doing, followed by one plain atomic store
 *  that never contends (each producer store targets a different, just-
 *  allocated node). No producer thread can block another. `try_pop` is
 *  normally lock-free too, but it has one narrow exception: if `try_pop` is
 *  called at the exact moment a producer has completed its `exchange` but
 *  not yet completed the following store, `try_pop` cannot yet tell "empty"
 *  from "a push is in flight" and conservatively reports empty
 *  (`try_pop` returns `false`/`std::nullopt`). That in-flight window is a
 *  single store's worth of time; the item becomes visible on the very next
 *  `try_pop` call once the racing producer finishes. This makes `try_pop`
 *  obstruction-free but not strictly lock-free in that narrow window --
 *  document and test it as such rather than claiming an unconditional
 *  lock-free consumer guarantee.
 *
 *  @par Memory ordering
 *  Producers publish a node with `exchange(..., acq_rel)` on the shared
 *  insertion pointer followed by `store(..., release)` on the previous
 *  node's link. The consumer reads links with `load(acquire)`, which
 *  synchronizes with the producer's release store and establishes
 *  happens-before for everything the producer wrote into the node
 *  (including the payload) before publishing it.
 *
 *  @par Ordering
 *  FIFO per producer: two pushes from the same thread are dequeued in the
 *  order they were pushed. There is no guaranteed *global* order between
 *  pushes from different producer threads beyond what their relative
 *  real-time completion already implies.
 *
 *  @par `T` requirements
 *  `T` must be move-constructible (or copy-constructible, for the
 *  const-reference `push` overload). Move-only types are fully supported.
 *
 *  @see concurrency_utils.H for `Aleph::SpscQueue` (bounded, single
 *       producer) and `Aleph::BoundedChannel` (bounded, blocking, MPMC).
 *  @ingroup Utilities
 *  @ingroup ThreadsAndConcurrency
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_MPSC_QUEUE_H
#define TPL_MPSC_QUEUE_H

#include <atomic>
#include <optional>
#include <utility>

namespace Aleph
{

/** @brief Unbounded lock-free multi-producer/single-consumer queue.
 *
 *  See the file-level documentation in `tpl_mpsc_queue.H` for the full
 *  algorithm description, thread-safety contract, progress guarantees, and
 *  memory-ordering rationale.
 *
 *  @tparam T Type of the elements stored in the queue. Must be move- or
 *            copy-constructible; move-only types are supported.
 */
template <typename T>
class MpscQueue
{
  struct Node
  {
    std::atomic<Node *> next{nullptr};
    std::optional<T> value;

    Node() = default;

    template <typename... Args>
    explicit Node(std::in_place_t, Args &&... args)
      : value(std::in_place, std::forward<Args>(args)...)
    {}
  };

  alignas(64) std::atomic<Node *> head_;
  alignas(64) Node * tail_;
  Node stub_;

  /// Publish `n` as the new last node. Safe to call from any producer
  /// thread, and internally (single-threaded) to recycle `&stub_`.
  void push_node(Node * n) noexcept
  {
    n->next.store(nullptr, std::memory_order_relaxed);
    Node * prev = head_.exchange(n, std::memory_order_acq_rel);
    prev->next.store(n, std::memory_order_release);
  }

  /** @brief Consumer-only: detach and return the front node, or `nullptr`.
   *
   *  Implements Vyukov's `pop` exactly: on success, the returned node owns
   *  the dequeued value and is no longer reachable from the queue (the
   *  caller must destroy it once it is done reading `->value`). Returning
   *  `nullptr` means either the queue is genuinely empty, or a producer's
   *  `push_node` was caught between its `exchange` and its `store` --
   *  both look identical to the consumer, and the documented contract is
   *  to treat that rare race as a transient "nothing to pop yet".
   */
  Node * pop_node() noexcept
  {
    Node * tail = tail_;
    Node * next = tail->next.load(std::memory_order_acquire);

    if (tail == &stub_)
      {
        if (next == nullptr)
          return nullptr;  // genuinely empty
        tail_ = next;
        tail = next;
        next = next->next.load(std::memory_order_acquire);
      }

    if (next != nullptr)
      {
        tail_ = next;
        return tail;
      }

    Node * head = head_.load(std::memory_order_acquire);
    if (tail != head)
      return nullptr;  // a producer is mid-push; try again later

    push_node(&stub_);  // re-anchor the queue so progress remains possible
    next = tail->next.load(std::memory_order_acquire);
    if (next != nullptr)
      {
        tail_ = next;
        return tail;
      }

    return nullptr;
  }

public:
  /** @brief Construct an empty queue.
   *
   *  @throws Nothing.
   *  @note Not thread-safe against concurrent use of the object being
   *        constructed (ordinary construction rules).
   */
  MpscQueue() noexcept : head_(&stub_), tail_(&stub_) {}

  /** @brief Destroy the queue, releasing any still-queued nodes.
   *
   *  @note Not thread-safe: no producer or consumer operation may be in
   *        progress on this queue when the destructor runs.
   */
  ~MpscQueue()
  {
    Node * n = tail_;
    if (n == &stub_)
      n = n->next.load(std::memory_order_relaxed);
    while (n != nullptr)
      {
        Node * next = n->next.load(std::memory_order_relaxed);
        delete n;
        n = next;
      }
  }

  /// @brief Deleted copy constructor: the queue owns heap nodes with
  /// internal atomics that cannot be safely duplicated.
  MpscQueue(const MpscQueue &) = delete;
  /// @brief Deleted copy assignment operator.
  MpscQueue & operator=(const MpscQueue &) = delete;
  /// @brief Deleted move constructor: producers may hold a reference to a
  /// fixed queue address; see the class-level thread-safety note.
  MpscQueue(MpscQueue &&) = delete;
  /// @brief Deleted move assignment operator.
  MpscQueue & operator=(MpscQueue &&) = delete;

  /** @brief Push a copy of `value` onto the queue.
   *
   *  @param value The value to enqueue (copied).
   *  @throws Whatever `T`'s copy constructor throws, or `std::bad_alloc`.
   *          If construction throws, nothing is enqueued and the queue is
   *          left exactly as it was before the call.
   *  @note Lock-free. Safe to call concurrently from any number of
   *        producer threads.
   */
  void push(const T & value) { emplace(value); }

  /** @brief Push `value` onto the queue, moving it in.
   *
   *  @param value The value to enqueue (moved from).
   *  @throws Whatever `T`'s move constructor throws, or `std::bad_alloc`.
   *          If construction throws, `value` is left in the state `T`'s
   *          move constructor leaves a throwing source in, and nothing is
   *          enqueued.
   *  @note Lock-free. Safe to call concurrently from any number of
   *        producer threads.
   */
  void push(T && value) { emplace(std::move(value)); }

  /** @brief Construct a new element in place at the back of the queue.
   *
   *  @tparam Args Constructor argument types.
   *  @param args Arguments forwarded to `T`'s constructor.
   *  @throws Whatever constructing `T(std::forward<Args>(args)...)` throws,
   *          or `std::bad_alloc` from allocating the node. If construction
   *          throws, the partially-built node is destroyed and nothing is
   *          enqueued.
   *  @note Lock-free. Safe to call concurrently from any number of
   *        producer threads.
   */
  template <typename... Args>
  void emplace(Args &&... args)
  {
    Node * n = new Node(std::in_place, std::forward<Args>(args)...);
    push_node(n);
  }

  /** @brief Attempt to pop the front element into `out`.
   *
   *  @param[out] out Reference receiving the dequeued value (move-assigned).
   *  @return `true` if an element was dequeued, `false` if the queue looked
   *          empty (see the class-level note on the narrow producer-race
   *          window that can also yield `false` transiently).
   *  @throws Whatever `T`'s move assignment throws. If it throws, the node
   *          has already been unlinked from the queue; the element is lost
   *          (this matches the "no defined recovery from a throwing move
   *          assignment" convention used elsewhere in Aleph's queues).
   *  @note Consumer-only: must never be called concurrently with another
   *        `try_pop`/`is_empty` call, nor from more than one thread.
   */
  bool try_pop(T & out)
  {
    Node * n = pop_node();
    if (n == nullptr)
      return false;
    out = std::move(*n->value);
    delete n;
    return true;
  }

  /** @brief Attempt to pop the front element.
   *
   *  @return The dequeued value wrapped in `std::optional`, or
   *          `std::nullopt` if the queue looked empty (see the class-level
   *          note on the narrow producer-race window).
   *  @throws Whatever moving `T` into the returned `std::optional` throws.
   *          If it throws, the node has already been unlinked from the
   *          queue; the element is lost.
   *  @note Consumer-only: must never be called concurrently with another
   *        `try_pop`/`is_empty` call, nor from more than one thread.
   */
  [[nodiscard]] std::optional<T> try_pop()
  {
    Node * n = pop_node();
    if (n == nullptr)
      return std::nullopt;
    std::optional<T> result(std::move(*n->value));
    delete n;
    return result;
  }

  /** @brief Advisory check for whether the queue currently has no elements.
   *
   *  @return `true` if the queue looked empty at the moment of the call.
   *  @throws Nothing.
   *  @note Consumer-only, same restriction as `try_pop`. Advisory under
   *        concurrent pushes: a producer may complete a push immediately
   *        after this returns `true`. Like `try_pop`, this can also report
   *        `true` during the narrow window where a push's `exchange` has
   *        completed but its `store` has not.
   */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return tail_ == &stub_ and
           tail_->next.load(std::memory_order_acquire) == nullptr;
  }
};

/** @brief Alias for \ref Aleph::MpscQueue.
 *  @tparam T Value type stored in the queue.
 *  @note Inherits the same lock-free, multi-producer/single-consumer
 *        semantics as `MpscQueue<T>`.
 */
template <typename T>
using mpsc_queue = MpscQueue<T>;

}  // namespace Aleph

#endif  // TPL_MPSC_QUEUE_H