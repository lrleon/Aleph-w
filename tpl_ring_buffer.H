
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

/** @file tpl_ring_buffer.H
 *  @brief Bounded circular buffer (`Aleph::RingBuffer`) for FIFO streaming.
 *
 *  `RingBuffer<T>` is a fixed-capacity FIFO over one contiguous allocation:
 *  `put` appends at the tail, `get` extracts from the head, both O(1), and
 *  the storage never reallocates after construction. The optional
 *  `put_overwrite` policy silently evicts the oldest element when full,
 *  which turns the buffer into a *sliding window over a stream* — the
 *  classic use in producer/consumer stages, telemetry, audio and logging.
 *
 *  How it differs from the existing circular queues:
 *  - `ArrayQueue` grows dynamically (capacity is a power of two) — it is a
 *    general-purpose queue, not a bounded window.
 *  - `FixedQueue` is fixed-size but performs no checking at all and
 *    requires default-constructible elements.
 *  - `RingBuffer` has an exact user-chosen capacity, checked operations
 *    (Aleph error macros), an explicit overwrite policy, indexed access to
 *    the window, and constructs elements in place — `T` needs no default
 *    constructor and move-only types are supported.
 *
 *  @see tpl_arrayQueue.H Dynamic (`ArrayQueue`) and unchecked fixed
 *       (`FixedQueue`) circular queues.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_RING_BUFFER_H
#define TPL_RING_BUFFER_H

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

namespace Aleph {

/** @brief Fixed-capacity circular FIFO buffer over contiguous storage.
 *
 *  Elements enter at the tail (`put`) and leave from the head (`get`) in
 *  FIFO order. The capacity is chosen at construction (any positive value,
 *  not rounded) and never changes; no operation reallocates. When the
 *  buffer is full, `put` reports overflow while `put_overwrite` evicts the
 *  oldest element — a sliding window.
 *
 *  @tparam T Element type. Must be move-constructible and move-assignable.
 *          No default constructor is required; move-only types work.
 *
 *  @par Indexing and iteration
 *  `operator[](i)` and iterators address the *logical* window: index 0 is
 *  the oldest element (next to leave), `size() - 1` the newest. Iterators
 *  are random access and remain valid until any element is added, removed
 *  or the buffer is destroyed.
 *
 *  @par Aleph conventions
 *  Following the library convention (see `Array`), `empty()` **clears** the
 *  buffer and `is_empty()` is the emptiness predicate.
 *
 *  @par Exception Safety
 *  `put`/`emplace` offer the strong guarantee (the element is constructed
 *  before any state changes). `put_overwrite` on a full buffer assigns over
 *  the oldest element (basic guarantee if `T`'s assignment throws).
 *
 *  @par Thread Safety
 *  Distinct instances may be used from distinct threads. Concurrent access
 *  to the same instance requires external synchronization (this is a
 *  sequential ring buffer, not a lock-free SPSC queue).
 *
 *  @par Example
 *  @code
 *  RingBuffer<double> window(4);       // last 4 samples
 *  for (double s : samples)
 *    window.put_overwrite(s);           // evicts oldest when full
 *  for (double s : window)              // oldest → newest
 *    accumulate(s);
 *  @endcode
 *
 *  @ingroup Sequences
 */
template <typename T>
class RingBuffer
{
  static_assert(std::is_move_constructible_v<T>,
                "RingBuffer requires a move-constructible element type");
  static_assert(std::is_move_assignable_v<T>, "RingBuffer requires a move-assignable element type");

  T *buf_ = nullptr;
  size_t cap_ = 0;
  size_t head_ = 0;  // physical index of the oldest element
  size_t n_ = 0;

  static T *allocate(size_t m)
  {
    return std::allocator<T>().allocate(m);
  }

  static void deallocate(T *p, size_t m) noexcept
  {
    std::allocator<T>().deallocate(p, m);
  }

  // Physical index of the i-th logical element.
  size_t phys(const size_t i) const noexcept
  {
    return (head_ + i) % cap_;
  }

  // Destroys all elements without releasing the storage.
  void destroy_all() noexcept
  {
    for (size_t i = 0; i < n_; ++i)
      std::destroy_at(buf_ + phys(i));
    head_ = 0;
    n_ = 0;
  }

public:
  using Item_Type = T;       ///< Aleph convention: element type.
  using value_type = T;      ///< STL convention: value type.
  using size_type = size_t;  ///< STL convention: size type.

  /** @brief Random-access iterator over the logical window.
   *
   *  Iterates from the oldest to the newest element. Invalidated by any
   *  insertion or extraction.
   *
   *  @tparam IsConst Whether the iterator provides const access.
   */
  template <bool IsConst>
  class basic_iterator
  {
    using BufPtr = std::conditional_t<IsConst, const RingBuffer *, RingBuffer *>;

    BufPtr rb_ = nullptr;
    size_t idx_ = 0;  // logical index

    /// Return `abs(i)` for negative signed offsets without signed overflow.
    static size_t negative_magnitude(const std::ptrdiff_t i) noexcept
    {
      return static_cast<size_t>(-(i + 1)) + 1;
    }

    /// Add signed offset `i` to logical index `idx`.
    static size_t add_offset(const size_t idx, const std::ptrdiff_t i) noexcept
    {
      return i < 0 ? idx - negative_magnitude(i) : idx + static_cast<size_t>(i);
    }

    /// Subtract signed offset `i` from logical index `idx`.
    static size_t sub_offset(const size_t idx, const std::ptrdiff_t i) noexcept
    {
      return i < 0 ? idx + negative_magnitude(i) : idx - static_cast<size_t>(i);
    }

  public:
    using iterator_category = std::random_access_iterator_tag;      ///< Category.
    using value_type = T;                                           ///< Element type.
    using difference_type = std::ptrdiff_t;                         ///< Signed distance type.
    using reference = std::conditional_t<IsConst, const T &, T &>;  ///< Ref.
    using pointer = std::conditional_t<IsConst, const T *, T *>;    ///< Ptr.

    /// Construct a singular iterator.
    basic_iterator() = default;

    /// Construct from a buffer and a logical index (internal use).
    basic_iterator(BufPtr rb, const size_t idx) noexcept : rb_(rb), idx_(idx) {}

    /// Convert a mutable iterator into a const iterator.
    template <bool B, class = std::enable_if_t<IsConst and not B>>
    basic_iterator(const basic_iterator<B> &it) noexcept : rb_(it.buffer()), idx_(it.index())
    {}

    /// Buffer this iterator walks (internal use).
    BufPtr buffer() const noexcept
    {
      return rb_;
    }

    /// Current logical index (internal use).
    size_t index() const noexcept
    {
      return idx_;
    }

    /// Dereference to the current element.
    reference operator * () const noexcept
    {
      return (*rb_)(idx_);
    }

    /// Member access on the current element.
    pointer operator ->() const noexcept
    {
      return &(*rb_)(idx_);
    }

    /// Element `i` logical positions away.
    reference operator [] (const difference_type i) const noexcept
    {
      return (*rb_)(add_offset(idx_, i));
    }

    /// Pre-increment.
    basic_iterator &operator ++ () noexcept
    {
      ++idx_;
      return *this;
    }

    /// Post-increment.
    basic_iterator operator ++ (int) noexcept
    {
      basic_iterator ret = *this;
      ++idx_;
      return ret;
    }

    /// Pre-decrement.
    basic_iterator &operator -- () noexcept
    {
      --idx_;
      return *this;
    }

    /// Post-decrement.
    basic_iterator operator -- (int) noexcept
    {
      basic_iterator ret = *this;
      --idx_;
      return ret;
    }

    /// Advance by `i` logical positions.
    basic_iterator &operator += (const difference_type i) noexcept
    {
      idx_ = add_offset(idx_, i);
      return *this;
    }

    /// Retreat by `i` logical positions.
    basic_iterator &operator -= (const difference_type i) noexcept
    {
      idx_ = sub_offset(idx_, i);
      return *this;
    }

    /// Iterator `i` positions forward.
    basic_iterator operator + (difference_type i) const noexcept
    {
      basic_iterator ret = *this;
      return ret += i;
    }

    /// Iterator `i` positions backward.
    basic_iterator operator - (difference_type i) const noexcept
    {
      basic_iterator ret = *this;
      return ret -= i;
    }

    /// Signed distance between iterators.
    difference_type operator - (const basic_iterator &it) const noexcept
    {
      return static_cast<difference_type>(idx_) - static_cast<difference_type>(it.index());
    }

    /// Equality: same buffer and logical position.
    bool operator == (const basic_iterator &it) const noexcept
    {
      return rb_ == it.buffer() and idx_ == it.index();
    }

    /// Inequality: different buffer or logical position.
    bool operator != (const basic_iterator &it) const noexcept
    {
      return not (*this == it);
    }

    /// Strict ordering by logical position.
    bool operator < (const basic_iterator &it) const noexcept
    {
      return idx_ < it.index();
    }

    /// Ordering by logical position.
    bool operator <= (const basic_iterator &it) const noexcept
    {
      return idx_ <= it.index();
    }

    /// Strict ordering by logical position.
    bool operator > (const basic_iterator &it) const noexcept
    {
      return idx_ > it.index();
    }

    /// Ordering by logical position.
    bool operator >= (const basic_iterator &it) const noexcept
    {
      return idx_ >= it.index();
    }
  };

  using iterator = basic_iterator<false>;       ///< Mutable iterator.
  using const_iterator = basic_iterator<true>;  ///< Const iterator.

  /** @brief Construct a buffer with an exact capacity.
   *  @param cap Maximum number of elements (not rounded; must be positive).
   *  @throws std::invalid_argument if `cap == 0`.
   *  @throws std::bad_alloc if the storage cannot be allocated.
   */
  explicit RingBuffer(const size_t cap) : cap_(cap)
  {
    ah_invalid_argument_if(cap == 0) << "RingBuffer: capacity must be positive";
    buf_ = allocate(cap_);
  }

  /// Copy constructor (requires copyable `T`). The copy is normalized
  /// (its oldest element sits at physical index 0).
  RingBuffer(const RingBuffer &rb)
    requires std::is_copy_constructible_v<T>
    : cap_(rb.cap_)
  {
    buf_ = allocate(cap_);
    try
      {
        for (; n_ < rb.n_; ++n_)
          ::new (static_cast<void *>(buf_ + n_)) T(rb(n_));
      }
    catch (...)
      {
        destroy_all();
        deallocate(buf_, cap_);
        throw;
      }
  }

  /// Move constructor: steals the storage in O(1). The source is left
  /// valid but empty with null storage; it may only be destroyed or
  /// assigned to.
  RingBuffer(RingBuffer &&rb) noexcept
  {
    swap(rb);
  }

  /// Copy assignment (requires copyable `T`).
  RingBuffer &operator = (const RingBuffer &rb)
    requires std::is_copy_constructible_v<T>
  {
    if (this == &rb)
      return *this;
    RingBuffer tmp(rb);  // strong guarantee
    swap(tmp);
    return *this;
  }

  /// Move assignment: steals the storage in O(1).
  RingBuffer &operator = (RingBuffer &&rb) noexcept
  {
    swap(rb);
    return *this;
  }

  ~RingBuffer()
  {
    if (buf_ != nullptr)
      {
        destroy_all();
        deallocate(buf_, cap_);
      }
  }

  /** @brief Swap contents with `rb` in O(1).
   *  @param rb Buffer to swap with.
   */
  void swap(RingBuffer &rb) noexcept
  {
    std::swap(buf_, rb.buf_);
    std::swap(cap_, rb.cap_);
    std::swap(head_, rb.head_);
    std::swap(n_, rb.n_);
  }

  // -- capacity -------------------------------------------------------------

  /// Return the number of stored elements. O(1).
  [[nodiscard]] size_t size() const noexcept
  {
    return n_;
  }

  /// Return the fixed capacity chosen at construction. O(1).
  [[nodiscard]] size_t capacity() const noexcept
  {
    return cap_;
  }

  /// Return the number of free slots. O(1).
  [[nodiscard]] size_t available() const noexcept
  {
    return cap_ - n_;
  }

  /// Return `true` if no elements are stored. O(1).
  [[nodiscard]] bool is_empty() const noexcept
  {
    return n_ == 0;
  }

  /// Return `true` if the buffer holds `capacity()` elements. O(1).
  [[nodiscard]] bool is_full() const noexcept
  {
    return n_ == cap_;
  }

  /** @brief Destroy all elements (Aleph convention). Capacity is kept.
   *  @note Following the Aleph container convention (see `Array::empty()`),
   *        this **mutates** the buffer; use `is_empty()` to test emptiness.
   */
  void empty() noexcept
  {
    destroy_all();
  }

  /// Destroy all elements. Alias of `empty()`. Capacity is kept.
  void clear() noexcept
  {
    destroy_all();
  }

  // -- element access -------------------------------------------------------

  /** @brief Checked access to the i-th logical element (0 = oldest).
   *  @param i Logical index from the head.
   *  @return Mutable reference to the element.
   *  @throws std::out_of_range if `i >= size()`.
   */
  [[nodiscard]] T &operator [] (const size_t i)
  {
    ah_out_of_range_error_if(i >= n_) << "RingBuffer: index out of range";
    return buf_[phys(i)];
  }

  /// Checked const access to the i-th logical element (0 = oldest).
  [[nodiscard]] const T &operator [] (const size_t i) const
  {
    ah_out_of_range_error_if(i >= n_) << "RingBuffer: index out of range";
    return buf_[phys(i)];
  }

  /// Unchecked access to the i-th logical element (must be `< size()`).
  [[nodiscard]] T &operator () (const size_t i) noexcept
  {
    return buf_[phys(i)];
  }

  /// Unchecked const access to the i-th logical element (must be `< size()`).
  [[nodiscard]] const T &operator () (const size_t i) const noexcept
  {
    return buf_[phys(i)];
  }

  /** @brief Oldest element — the next to leave (checked).
   *  @return Mutable reference to the front element.
   *  @throws std::underflow_error if the buffer is empty.
   */
  [[nodiscard]] T &get_first()
  {
    ah_underflow_error_if(n_ == 0) << "RingBuffer::get_first(): empty";
    return buf_[head_];
  }

  /// \overload get_first()
  [[nodiscard]] const T &get_first() const
  {
    ah_underflow_error_if(n_ == 0) << "RingBuffer::get_first(): empty";
    return buf_[head_];
  }

  /** @brief Newest element — the last one inserted (checked).
   *  @return Mutable reference to the back element.
   *  @throws std::underflow_error if the buffer is empty.
   */
  [[nodiscard]] T &get_last()
  {
    ah_underflow_error_if(n_ == 0) << "RingBuffer::get_last(): empty";
    return buf_[phys(n_ - 1)];
  }

  /// \overload get_last()
  [[nodiscard]] const T &get_last() const
  {
    ah_underflow_error_if(n_ == 0) << "RingBuffer::get_last(): empty";
    return buf_[phys(n_ - 1)];
  }

  /// Oldest element (checked). Alias of `get_first()`.
  [[nodiscard]] T &front()
  {
    return get_first();
  }

  /// \overload front()
  [[nodiscard]] const T &front() const
  {
    return get_first();
  }

  /// Newest element (checked). Alias of `get_last()`.
  [[nodiscard]] T &back()
  {
    return get_last();
  }

  /// \overload back()
  [[nodiscard]] const T &back() const
  {
    return get_last();
  }

  // -- modifiers ------------------------------------------------------------

  /** @brief Construct an element in place at the tail.
   *  @tparam Args Argument types forwarded to `T`'s constructor.
   *  @param args Arguments forwarded to `T`'s constructor.
   *  @return Mutable reference to the new element.
   *  @throws std::overflow_error if the buffer is full.
   *  @note O(1). Strong guarantee.
   */
  template <class... Args>
  T &emplace(Args &&...args)
  {
    ah_overflow_error_if(is_full()) << "RingBuffer::emplace(): buffer full";
    T *slot = buf_ + phys(n_);
    ::new (static_cast<void *>(slot)) T(std::forward<Args>(args)...);
    ++n_;
    return *slot;
  }

  /** @brief Append a copy of `item` at the tail.
   *  @param item Element to copy.
   *  @return Mutable reference to the new element.
   *  @throws std::overflow_error if the buffer is full.
   *  @note O(1). Strong guarantee.
   */
  T &put(const T &item)
    requires std::is_copy_constructible_v<T>
  {
    return emplace(item);
  }

  /** @brief Append `item` at the tail by moving.
   *  @param item Element to move in.
   *  @return Mutable reference to the new element.
   *  @throws std::overflow_error if the buffer is full.
   *  @note O(1). Strong guarantee.
   */
  T &put(T &&item)
  {
    return emplace(std::move(item));
  }

  /// Queue-style alias of `put(const T &)`.
  T &push(const T &item)
    requires std::is_copy_constructible_v<T>
  {
    return put(item);
  }

  /// Queue-style alias of `put(T &&)`.
  T &push(T &&item)
  {
    return put(std::move(item));
  }

  /** @brief Append at the tail, evicting the oldest element when full.
   *
   *  This is the sliding-window insertion: on a full buffer the oldest
   *  element is overwritten (by assignment) and the head advances.
   *
   *  @param item Element to copy in.
   *  @return `true` if an old element was evicted, `false` if the buffer
   *          still had room.
   *  @note O(1). Basic guarantee if `T`'s copy assignment throws.
   */
  bool put_overwrite(const T &item)
    requires(std::is_copy_constructible_v<T> and std::is_copy_assignable_v<T>)
  {
    if (not is_full())
      {
        put(item);
        return false;
      }
    buf_[head_] = item;
    head_ = (head_ + 1) % cap_;
    return true;
  }

  /// \overload put_overwrite(const T & item) moving `item`.
  bool put_overwrite(T &&item)
    requires std::is_move_assignable_v<T>
  {
    if (not is_full())
      {
        put(std::move(item));
        return false;
      }
    buf_[head_] = std::move(item);
    head_ = (head_ + 1) % cap_;
    return true;
  }

  /** @brief Extract the oldest element from the head.
   *  @return The removed element, moved out.
   *  @throws std::underflow_error if the buffer is empty.
   *  @note O(1).
   */
  T get()
  {
    ah_underflow_error_if(n_ == 0) << "RingBuffer::get(): empty buffer";
    T *slot = buf_ + head_;
    T ret = std::move(*slot);
    std::destroy_at(slot);
    head_ = (head_ + 1) % cap_;
    --n_;
    return ret;
  }

  /// Queue-style alias of `get()`.
  T pop()
  {
    return get();
  }

  // -- iteration ------------------------------------------------------------

  /// Iterator on the oldest element. O(1).
  iterator begin() noexcept
  {
    return iterator(this, 0);
  }

  /// Iterator past the newest element. O(1).
  iterator end() noexcept
  {
    return iterator(this, n_);
  }

  /// Const iterator on the oldest element. O(1).
  const_iterator begin() const noexcept
  {
    return const_iterator(this, 0);
  }

  /// Const iterator past the newest element. O(1).
  const_iterator end() const noexcept
  {
    return const_iterator(this, n_);
  }

  /// Const iterator on the oldest element. O(1).
  const_iterator cbegin() const noexcept
  {
    return begin();
  }

  /// Const iterator past the newest element. O(1).
  const_iterator cend() const noexcept
  {
    return end();
  }

  /** @brief Traverse from oldest to newest while `operation` returns `true`.
   *
   *  Aleph-style bounded traversal: `operation` receives each element in
   *  FIFO order; returning `false` stops the walk.
   *
   *  @tparam Operation Callable `bool(T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all elements were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation)
  {
    for (size_t i = 0; i < n_; ++i)
      if (not operation(buf_[phys(i)]))
        return false;
    return true;
  }

  /** @brief Const traversal from oldest to newest.
   *  @tparam Operation Callable `bool(const T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all elements were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation) const
  {
    for (size_t i = 0; i < n_; ++i)
      if (not operation(buf_[phys(i)]))
        return false;
    return true;
  }

  // -- comparison -----------------------------------------------------------

  /** @brief Equality: same logical contents (capacity is not compared).
   *  @param rb Buffer to compare against.
   *  @return `true` if both buffers hold equal elements in FIFO order.
   *  @note Complexity: O(n). Requires `T` equality comparable.
   */
  bool operator == (const RingBuffer &rb) const
  {
    if (n_ != rb.n_)
      return false;
    for (size_t i = 0; i < n_; ++i)
      if (not ((*this)(i) == rb(i)))
        return false;
    return true;
  }

  /// Inequality: negation of `operator==`.
  bool operator != (const RingBuffer &rb) const
  {
    return not (*this == rb);
  }
};

}  // end namespace Aleph

#endif  // TPL_RING_BUFFER_H
