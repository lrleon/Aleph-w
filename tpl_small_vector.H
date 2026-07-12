
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

/** @file tpl_small_vector.H
 *  @brief Dynamic array with inline storage (`Aleph::SmallVector`).
 *
 *  `SmallVector<T, N>` stores up to `N` elements inside the object itself
 *  — no heap allocation at all — and transparently spills to the heap when
 *  it grows past `N`. It is the classic small-buffer-optimized vector
 *  (cf. `llvm::SmallVector`, `boost::container::small_vector`), ideal for
 *  containers that are *usually* small: argument lists, adjacency lists of
 *  sparse nodes, scratch buffers in hot loops.
 *
 *  | Aspect | SmallVector<T, N> | Array<T> / std::vector |
 *  |--------|--------------------|------------------------|
 *  | first N appends | no allocation | heap allocation |
 *  | element storage | inline, then heap | always heap |
 *  | move of heap-mode object | O(1) pointer steal | O(1) |
 *  | move of inline-mode object | O(n) element moves | O(1) |
 *
 *  Unlike the `MemArray`-based containers, `SmallVector` manages raw
 *  storage with placement construction, so `T` needs **no default
 *  constructor**; move-only types such as `std::unique_ptr` are supported.
 *
 *  @see tpl_array.H General-purpose dynamic array (`Array`).
 *  @see tpl_memArray.H Low-level contiguous buffer used by Aleph containers.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_SMALL_VECTOR_H
#define TPL_SMALL_VECTOR_H

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

namespace Aleph {

/** @brief Contiguous dynamic array with `N` elements of inline storage.
 *
 *  Elements live inside the object until the size exceeds `N`, at which
 *  point the contents move to a heap buffer that grows geometrically
 *  (factor 2). The container never returns to inline storage once spilled
 *  (even if it shrinks below `N`), so pointers into a heap-mode vector are
 *  only invalidated by growth.
 *
 *  @tparam T Element type. Must be move-constructible. Copy operations
 *          additionally require copy-constructible/assignable `T`.
 *  @tparam N Number of inline slots (must be positive). Choose it from the
 *          common-case size of the data, not the worst case.
 *
 *  @par Iterators
 *  Plain pointers (`T *` / `const T *`): contiguous, random access. Growth
 *  reallocates and invalidates all iterators and references.
 *
 *  @par Aleph conventions
 *  Following the library convention (see `Array`), `empty()` **clears** the
 *  container and `is_empty()` is the emptiness predicate. Both `append` and
 *  `push_back` names are provided.
 *
 *  @par Exception Safety
 *  Growth offers the strong guarantee when `T` has a non-throwing move
 *  constructor or is copyable (`std::move_if_noexcept` semantics).
 *
 *  @par Thread Safety
 *  Distinct instances may be used from distinct threads. Concurrent access
 *  to the same instance requires external synchronization.
 *
 *  @par Example
 *  @code
 *  SmallVector<int, 8> v;      // 8 slots inline, zero allocations
 *  for (int i = 0; i < 8; ++i)
 *    v.append(i);               // still inline
 *  v.append(8);                 // spills to heap
 *  assert(not v.is_small());
 *  @endcode
 *
 *  @ingroup Sequences
 */
template <typename T, size_t N = 8>
class SmallVector
{
  static_assert(N > 0, "SmallVector requires a positive inline capacity");
  static_assert(std::is_move_constructible_v<T>,
                "SmallVector requires a move-constructible element type");

  // Raw inline storage for the first N elements. The buffer is aligned as T so
  // placement-new can construct T objects here without a separate heap allocation.
  alignas(T) std::byte storage_[N * sizeof(T)];
  T *ptr_;
  size_t n_ = 0;
  size_t cap_ = N;

  T *inline_ptr() noexcept
  {
    return reinterpret_cast<T *>(storage_);
  }

  const T *inline_ptr() const noexcept
  {
    return reinterpret_cast<const T *>(storage_);
  }

  static T *allocate(size_t m)
  {
    return std::allocator<T>().allocate(m);
  }

  static void deallocate(T *p, size_t m) noexcept
  {
    std::allocator<T>().deallocate(p, m);
  }

  // Moves the elements into a fresh heap buffer of capacity new_cap
  // (strong guarantee via move_if_noexcept) and releases the old buffer.
  void relocate(const size_t new_cap)
  {
    T *np = allocate(new_cap);
    size_t i = 0;
    try
      {
        for (; i < n_; ++i)
          ::new (static_cast<void *>(np + i)) T(std::move_if_noexcept(ptr_[i]));
      }
    catch (...)
      {
        std::destroy(np, np + i);
        deallocate(np, new_cap);
        throw;
      }
    std::destroy(ptr_, ptr_ + n_);
    if (not is_small())
      deallocate(ptr_, cap_);
    ptr_ = np;
    cap_ = new_cap;
  }

  // Ensures room for at least min_cap elements, growing geometrically.
  void grow(const size_t min_cap)
  {
    size_t new_cap = cap_;
    while (new_cap < min_cap)
      {
        ah_overflow_error_if(new_cap > (static_cast<size_t>(-1) >> 1))
          << "SmallVector::grow(): capacity overflow";
        new_cap <<= 1;
      }
    relocate(new_cap);
  }

  // Takes over o's contents. *this must be empty and inline.
  void steal(SmallVector &&o) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    if (o.is_small())
      {
        if constexpr (std::is_nothrow_move_constructible_v<T>)
          {
            for (; n_ < o.n_; ++n_)
              ::new (static_cast<void *>(ptr_ + n_)) T(std::move(o.ptr_[n_]));
          }
        else
          {
            try
              {
                for (; n_ < o.n_; ++n_)
                  ::new (static_cast<void *>(ptr_ + n_)) T(std::move(o.ptr_[n_]));
              }
            catch (...)
              {
                release();
                throw;
              }
          }
        std::destroy(o.ptr_, o.ptr_ + o.n_);
        o.n_ = 0;
      }
    else
      {
        ptr_ = o.ptr_;
        cap_ = o.cap_;
        n_ = o.n_;
        o.ptr_ = o.inline_ptr();
        o.cap_ = N;
        o.n_ = 0;
      }
  }

  // Destroys all elements and releases the heap buffer, back to inline.
  void release() noexcept
  {
    std::destroy(ptr_, ptr_ + n_);
    if (not is_small())
      deallocate(ptr_, cap_);
    ptr_ = inline_ptr();
    cap_ = N;
    n_ = 0;
  }

public:
  using Item_Type = T;               ///< Aleph convention: element type.
  using value_type = T;              ///< STL convention: value type.
  using size_type = size_t;          ///< STL convention: size type.
  using iterator = T *;              ///< Random-access iterator.
  using const_iterator = const T *;  ///< Random-access const iterator.

  /// Construct an empty vector using the inline storage. Never allocates.
  SmallVector() noexcept : ptr_(inline_ptr()) {}

  /** @brief Construct with `n` copies of `value`.
   *  @param n Number of elements.
   *  @param value Value copied into each element.
   *  @throws std::bad_alloc if `n > N` and the heap buffer cannot be
   *          allocated.
   */
  SmallVector(const size_t n, const T &value)
    requires std::is_copy_constructible_v<T>
    : ptr_(inline_ptr())
  {
    if (n > cap_)
      grow(n);
    try
      {
        for (; n_ < n; ++n_)
          ::new (static_cast<void *>(ptr_ + n_)) T(value);
      }
    catch (...)
      {
        release();
        throw;
      }
  }

  /** @brief Construct from an initializer list.
   *  @param l Elements to copy, in order.
   *  @throws std::bad_alloc if the heap buffer cannot be allocated.
   */
  SmallVector(std::initializer_list<T> l)
    requires std::is_copy_constructible_v<T>
    : ptr_(inline_ptr())
  {
    if (l.size() > cap_)
      grow(l.size());
    try
      {
        for (const T &x : l)
          {
            ::new (static_cast<void *>(ptr_ + n_)) T(x);
            ++n_;
          }
      }
    catch (...)
      {
        release();
        throw;
      }
  }

  /** @brief Construct from an iterator range.
   *  @tparam It Input iterator whose value type converts to `T`.
   *  @param first Beginning of the range.
   *  @param last End of the range.
   *  @throws std::bad_alloc if the heap buffer cannot be allocated.
   */
  template <class It, class = std::enable_if_t<not std::is_integral_v<It>>>
  SmallVector(It first, It last) : ptr_(inline_ptr())
  {
    try
      {
        for (; first != last; ++first)
          append(*first);
      }
    catch (...)
      {
        release();
        throw;
      }
  }

  /// Copy constructor (requires copyable `T`).
  SmallVector(const SmallVector &v)
    requires std::is_copy_constructible_v<T>
    : ptr_(inline_ptr())
  {
    if (v.n_ > cap_)
      grow(v.n_);
    try
      {
        for (; n_ < v.n_; ++n_)
          ::new (static_cast<void *>(ptr_ + n_)) T(v.ptr_[n_]);
      }
    catch (...)
      {
        release();
        throw;
      }
  }

  /** @brief Move constructor.
   *
   *  Heap-mode sources are stolen in O(1); inline-mode sources are moved
   *  element by element (O(n)). The source is left empty.
   */
  SmallVector(SmallVector &&v) noexcept(std::is_nothrow_move_constructible_v<T>)
    : ptr_(inline_ptr())
  {
    steal(std::move(v));
  }

  /// Copy assignment (requires copyable `T`).
  SmallVector &operator = (const SmallVector &v)
    requires std::is_copy_constructible_v<T>
  {
    if (this == &v)
      return *this;
    release();
    if (v.n_ > cap_)
      grow(v.n_);
    for (; n_ < v.n_; ++n_)
      ::new (static_cast<void *>(ptr_ + n_)) T(v.ptr_[n_]);
    return *this;
  }

  /// Move assignment. The source is left empty.
  SmallVector &operator = (SmallVector &&v) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    if (this == &v)
      return *this;
    release();
    steal(std::move(v));
    return *this;
  }

  ~SmallVector()
  {
    release();
  }

  /** @brief Swap contents with `v`.
   *
   *  O(1) when both vectors are in heap mode; otherwise elements are moved
   *  individually (O(n + m)).
   *
   *  @param v Vector to swap with.
   */
  void swap(SmallVector &v) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    if (not is_small() and not v.is_small())
      {
        std::swap(ptr_, v.ptr_);
        std::swap(n_, v.n_);
        std::swap(cap_, v.cap_);
        return;
      }
    SmallVector tmp(std::move(*this));
    *this = std::move(v);
    v = std::move(tmp);
  }

  // -- capacity -------------------------------------------------------------

  /// Return the number of stored elements. O(1).
  [[nodiscard]] size_t size() const noexcept
  {
    return n_;
  }

  /// Return the current capacity (inline or heap). O(1).
  [[nodiscard]] size_t capacity() const noexcept
  {
    return cap_;
  }

  /// Return `true` if no elements are stored. O(1).
  [[nodiscard]] bool is_empty() const noexcept
  {
    return n_ == 0;
  }

  /// Return `true` while the elements still live in the inline buffer. O(1).
  [[nodiscard]] bool is_small() const noexcept
  {
    return ptr_ == inline_ptr();
  }

  /** @brief Reserve capacity for at least `cap` elements.
   *  @param cap Desired capacity. Values `<= capacity()` are no-ops.
   *  @throws std::bad_alloc if the heap buffer cannot be allocated.
   */
  void reserve(const size_t cap)
  {
    if (cap > cap_)
      grow(cap);
  }

  /** @brief Destroy all elements (Aleph convention). Capacity is kept.
   *  @note Following the Aleph container convention (see `Array::empty()`),
   *        this **mutates** the vector; use `is_empty()` to test emptiness.
   */
  void empty() noexcept
  {
    std::destroy(ptr_, ptr_ + n_);
    n_ = 0;
  }

  /// Destroy all elements. Alias of `empty()`. Capacity is kept.
  void clear() noexcept
  {
    empty();
  }

  // -- element access -------------------------------------------------------

  /** @brief Checked access to the i-th element.
   *  @param i Zero-based index.
   *  @return Mutable reference to the element.
   *  @throws std::out_of_range if `i >= size()`.
   */
  [[nodiscard]] T &operator [] (size_t i)
  {
    ah_out_of_range_error_if(i >= n_) << "SmallVector: index out of range";
    return ptr_[i];
  }

  /// Checked const access to the i-th element (throws `std::out_of_range`).
  [[nodiscard]] const T &operator [] (size_t i) const
  {
    ah_out_of_range_error_if(i >= n_) << "SmallVector: index out of range";
    return ptr_[i];
  }

  /// Unchecked access to the i-th element (must be `< size()`).
  [[nodiscard]] T &operator () (size_t i) noexcept
  {
    return ptr_[i];
  }

  /// Unchecked const access to the i-th element (must be `< size()`).
  [[nodiscard]] const T &operator () (size_t i) const noexcept
  {
    return ptr_[i];
  }

  /** @brief First element (checked).
   *  @return Mutable reference to the first element.
   *  @throws std::underflow_error if the vector is empty.
   */
  [[nodiscard]] T &get_first()
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::get_first(): empty";
    return ptr_[0];
  }

  /// \overload get_first()
  [[nodiscard]] const T &get_first() const
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::get_first(): empty";
    return ptr_[0];
  }

  /** @brief Last element (checked).
   *  @return Mutable reference to the last element.
   *  @throws std::underflow_error if the vector is empty.
   */
  [[nodiscard]] T &get_last()
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::get_last(): empty";
    return ptr_[n_ - 1];
  }

  /// \overload get_last()
  [[nodiscard]] const T &get_last() const
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::get_last(): empty";
    return ptr_[n_ - 1];
  }

  /// Pointer to the contiguous element storage. O(1).
  [[nodiscard]] T *data() noexcept
  {
    return ptr_;
  }

  /// \overload data()
  [[nodiscard]] const T *data() const noexcept
  {
    return ptr_;
  }

  // -- modifiers ------------------------------------------------------------

  /** @brief Construct an element in place at the end.
   *  @tparam Args Argument types forwarded to `T`'s constructor.
   *  @param args Arguments forwarded to `T`'s constructor.
   *  @return Mutable reference to the new element.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Amortized O(1). Safe even when `args` reference elements of
   *        this vector (the element is built before any reallocation).
   */
  template <class... Args>
  T &emplace_back(Args &&...args)
  {
    if (n_ == cap_)
      {
        T tmp(std::forward<Args>(args)...);
        grow(n_ + 1);
        ::new (static_cast<void *>(ptr_ + n_)) T(std::move(tmp));
      }
    else
      ::new (static_cast<void *>(ptr_ + n_)) T(std::forward<Args>(args)...);
    return ptr_[n_++];
  }

  /** @brief Append a copy of `item`.
   *  @param item Element to copy.
   *  @return Mutable reference to the new element.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Amortized O(1).
   */
  T &append(const T &item)
    requires std::is_copy_constructible_v<T>
  {
    return emplace_back(item);
  }

  /** @brief Append `item` by moving.
   *  @param item Element to move in.
   *  @return Mutable reference to the new element.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note Amortized O(1).
   */
  T &append(T &&item)
  {
    return emplace_back(std::move(item));
  }

  /** @brief Append `count` copies from `[first, first + count)`, in order.
   *  @param first Pointer to the first source element. May be `nullptr`
   *         only when `count == 0` (nothing is read from it then).
   *  @param count Number of elements to copy.
   *  @pre `[first, first + count)` is a valid range of live `T` objects
   *       (readable, not past-the-end of their own storage).
   *  @pre `[first, first + count)` does not overlap this vector's own
   *       element storage (i.e. `first` is not itself `data()` or a
   *       pointer derived from it), *including* across a reallocation
   *       this very call might trigger: if `count` grows the vector past
   *       `capacity()`, `grow()` relocates to a new buffer and frees the
   *       old one before any copying happens, so a `first` that pointed
   *       into the old buffer would already be dangling by the time it
   *       is read. Like `std::memcpy`, this does not support appending a
   *       vector's own elements to itself.
   *  @throws std::invalid_argument if `first == nullptr` and `count > 0`.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @throws std::overflow_error if `count` would overflow `size_t` when
   *          added to the current `size()`, or if growing the capacity
   *          to fit the result would overflow (see `grow()`'s own
   *          capacity-doubling guard).
   *  @note Equivalent to calling `append(first[i])` for `i` in
   *        `[0, count)`, in order, but faster: when `T` is trivially
   *        copyable, the whole range is copied with a single
   *        `std::uninitialized_copy_n` call instead of `count` individual
   *        placement-new calls (safe because a trivially copyable type's
   *        copy can never throw and has no observable side effects
   *        beyond the bytes themselves; standard library implementations
   *        lower this to a single `memcpy` for such types).
   *        For any other copy-constructible `T`, this still grows at
   *        most once for the whole range (rather than up to `count`
   *        times) before placement-constructing each element, with the
   *        same strong exception guarantee as `append`: if a mid-range
   *        copy throws, only the newly-appended elements are unwound and
   *        `size()` is left exactly as it was before the call.
   *  @note Complexity: `O(count)`, or `O(size() + count)` on the
   *        occasions this triggers a growing reallocation.
   *  @note Not thread-safe: as with every other mutating method (see the
   *        class-level `@par Thread Safety`), concurrent calls on the
   *        same instance require external synchronization.
   */
  void append_range(const T *first, const size_t count)
    requires std::is_copy_constructible_v<T>
  {
    if (count == 0)
      return;
    ah_invalid_argument_if(first == nullptr)
      << "SmallVector::append_range(): null source pointer";
    // Checked *before* computing `n_ + count`, not after: for an
    // adversarial or mistaken huge `count`, the raw sum can wrap around
    // `size_t` and come out `<= cap_`, which would skip the `grow()`
    // call below entirely and then `memcpy` (or the placement-new loop)
    // straight past the end of the real, too-small buffer.
    ah_overflow_error_if(count > std::numeric_limits<size_t>::max() - n_)
      << "SmallVector::append_range(): count overflows size_t when added "
         "to the current size";
    if (n_ + count > cap_)
      grow(n_ + count);
    if constexpr (std::is_trivially_copyable_v<T>)
      {
        // std::uninitialized_copy_n, not a raw memcpy: copying the object
        // representation into storage that holds no object yet only
        // implicitly starts a `T` object's lifetime there (C++20
        // [basic.types.general]) when `T` is an *implicit-lifetime*
        // type -- trivially copyable does not by itself guarantee that
        // (e.g. a type whose only non-deleted special member is a
        // trivial assignment operator, with every constructor deleted,
        // is trivially copyable but not implicit-lifetime). Standard
        // library implementations detect the trivially-copyable case
        // and lower this to the same memcpy, so this costs nothing.
        std::uninitialized_copy_n(first, count, ptr_ + n_);
        n_ += count;
      }
    else
      {
        size_t i = 0;
        try
          {
            for (; i < count; ++i)
              ::new (static_cast<void *>(ptr_ + n_ + i)) T(first[i]);
          }
        catch (...)
          {
            std::destroy(ptr_ + n_, ptr_ + n_ + i);
            throw;
          }
        n_ += count;
      }
  }

  /// STL-style alias of `append(const T &)`.
  T &push_back(const T &item)
    requires std::is_copy_constructible_v<T>
  {
    return append(item);
  }

  /// STL-style alias of `append(T &&)`.
  T &push_back(T &&item)
  {
    return append(std::move(item));
  }

  /** @brief Remove and return the last element.
   *  @return The removed element, moved out.
   *  @throws std::underflow_error if the vector is empty.
   *  @note O(1). The storage never shrinks back to inline mode.
   */
  T remove_last()
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::remove_last(): empty";
    T ret = std::move(ptr_[n_ - 1]);
    std::destroy_at(ptr_ + --n_);
    return ret;
  }

  /** @brief Remove the last element (STL style).
   *  @throws std::underflow_error if the vector is empty.
   *  @note O(1).
   */
  void pop_back()
  {
    ah_underflow_error_if(n_ == 0) << "SmallVector::pop_back(): empty";
    std::destroy_at(ptr_ + --n_);
  }

  /** @brief Insert an element at position `pos`, shifting the tail right.
   *  @param pos Insertion position in `[0, size()]`.
   *  @param item Element to insert (taken by value: pass rvalues to move).
   *  @return Mutable reference to the inserted element.
   *  @throws std::out_of_range if `pos > size()`.
   *  @throws std::bad_alloc if a growing reallocation fails.
   *  @note O(n - pos) moves.
   */
  T &insert(size_t pos, T item)
    requires std::is_move_assignable_v<T>
  {
    ah_out_of_range_error_if(pos > n_) << "SmallVector::insert(): position out of range";
    if (n_ == cap_)
      grow(n_ + 1);
    if (pos == n_)
      {
        ::new (static_cast<void *>(ptr_ + n_)) T(std::move(item));
        return ptr_[n_++];
      }
    ::new (static_cast<void *>(ptr_ + n_)) T(std::move(ptr_[n_ - 1]));
    for (size_t i = n_ - 1; i > pos; --i)
      ptr_[i] = std::move(ptr_[i - 1]);
    ptr_[pos] = std::move(item);
    ++n_;
    return ptr_[pos];
  }

  /** @brief Remove the element at position `pos`, shifting the tail left.
   *  @param pos Position of the element to remove, in `[0, size())`.
   *  @throws std::out_of_range if `pos >= size()`.
   *  @note O(n - pos) moves.
   */
  void erase(const size_t pos)
    requires std::is_move_assignable_v<T>
  {
    ah_out_of_range_error_if(pos >= n_) << "SmallVector::erase(): position out of range";
    for (size_t i = pos; i + 1 < n_; ++i)
      ptr_[i] = std::move(ptr_[i + 1]);
    std::destroy_at(ptr_ + --n_);
  }

  // -- iteration ------------------------------------------------------------

  /// Iterator to the first element. O(1).
  iterator begin() noexcept
  {
    return ptr_;
  }

  /// Iterator past the last element. O(1).
  iterator end() noexcept
  {
    return ptr_ + n_;
  }

  /// Const iterator to the first element. O(1).
  const_iterator begin() const noexcept
  {
    return ptr_;
  }

  /// Const iterator past the last element. O(1).
  const_iterator end() const noexcept
  {
    return ptr_ + n_;
  }

  /// Const iterator to the first element. O(1).
  const_iterator cbegin() const noexcept
  {
    return ptr_;
  }

  /// Const iterator past the last element. O(1).
  const_iterator cend() const noexcept
  {
    return ptr_ + n_;
  }

  /** @brief Traverse elements in order while `operation` returns `true`.
   *
   *  Aleph-style bounded traversal: `operation` receives each element in
   *  index order; returning `false` stops the walk.
   *
   *  @tparam Operation Callable `bool(T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all elements were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation)
  {
    for (size_t i = 0; i < n_; ++i)
      if (not operation(ptr_[i]))
        return false;
    return true;
  }

  /** @brief Const traversal in order while `operation` returns `true`.
   *  @tparam Operation Callable `bool(const T &)`.
   *  @param operation Operation to apply.
   *  @return `true` if all elements were visited, `false` if stopped early.
   */
  template <class Operation>
  bool traverse(Operation operation) const
  {
    for (size_t i = 0; i < n_; ++i)
      if (not operation(ptr_[i]))
        return false;
    return true;
  }

  // -- comparison -----------------------------------------------------------

  /** @brief Equality: same size and pairwise equal elements.
   *  @param v Vector to compare against (any inline capacity).
   *  @return `true` if both vectors hold equal elements in the same order.
   *  @note Complexity: O(n). Requires `T` equality comparable.
   */
  template <size_t M>
  bool operator == (const SmallVector<T, M> &v) const
  {
    if (n_ != v.size())
      return false;
    for (size_t i = 0; i < n_; ++i)
      if (not (ptr_[i] == v(i)))
        return false;
    return true;
  }

  /// Inequality: negation of `operator==`.
  template <size_t M>
  bool operator != (const SmallVector<T, M> &v) const
  {
    return not (*this == v);
  }
};

}  // end namespace Aleph

#endif  // TPL_SMALL_VECTOR_H
