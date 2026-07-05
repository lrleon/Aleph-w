
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

/** @file ah-generator.H
 *  @brief Lazy sequence type (`Aleph::Generator<T>`) built on C++20
 *         coroutines.
 *
 *  `Aleph::Generator<T>` is a single-pass, lazily-evaluated sequence
 *  produced by a coroutine that `co_yield`s values of type `T`. It is the
 *  reusable building block behind the lazy counterparts of Aleph's eager
 *  enumerators: combinatorics (`ah-comb.H`), tree traversals
 *  (`tpl_binNodeUtils.H`) and graph traversals (`graph-traverse.H`).
 *
 *  @code
 *  Aleph::Generator<int> countdown(int n)
 *  {
 *    for (; n > 0; --n)
 *      co_yield n;
 *  }
 *
 *  for (int i : countdown(3))   // prints 3 2 1, one at a time
 *    std::cout << i << '\n';
 *  @endcode
 *
 *  @note This is a **native** Aleph implementation, not an alias of C++23
 *        `std::generator<T>`. Coroutines are a C++20 *language* feature —
 *        `Aleph::Generator` needs no C++23 library support and behaves
 *        identically on every standard the library targets (17 is
 *        unsupported for coroutine use, 20 and 23 behave the same). This
 *        mirrors the `Aleph::FlatSet`/`Aleph::FlatMap` decision (see
 *        `tpl_flat_set.H`): a smaller, uniform surface beats aliasing a
 *        standard facility whose semantics (allocator support, `elements_of`,
 *        reference-type customization) would otherwise leak through and vary
 *        by `-std`. The `ALEPH_HAS_STD_GENERATOR` macro in `ah-cpp-compat.H`
 *        remains available for consumers who specifically want
 *        `std::generator` itself.
 *
 *  @par Laziness vs. performance
 *  Prefer `Aleph::Generator` for ergonomics and memory savings when the
 *  full sequence would be expensive to materialize or is only partially
 *  consumed (early `break`). It is **not** a drop-in replacement for
 *  hand-written iterators on hot paths: each resume is a coroutine-frame
 *  resumption, which most compilers cannot fully inline away.
 *
 *  @ingroup FunctionalProgramming
 *  @author Leandro Rabindranath Leon
 */

#ifndef AH_GENERATOR_H
#define AH_GENERATOR_H

#include <coroutine>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

namespace Aleph
{

/** @brief Lazy, single-pass sequence of `T` values produced by a coroutine.
 *
 *  A function returning `Aleph::Generator<T>` and using `co_yield` in its
 *  body is a *generator function*: calling it does not run the body — it
 *  creates a suspended coroutine frame. The body advances one `co_yield` at
 *  a time as the sequence is iterated (range-`for`, `begin()`/`operator++`,
 *  or manual `resume()`/`value()`).
 *
 *  @tparam T Type of the yielded values. `co_yield` accepts lvalues and
 *          rvalues convertible to `std::remove_reference_t<T>`; the iterator
 *          exposes the currently yielded object by reference.
 *
 *  @par Yielded value lifetime
 *  Dereferencing the iterator returns a reference to the object currently
 *  suspended at `co_yield`. That reference is valid only until the generator
 *  is resumed again (`operator++()` or a second `begin()`), or until the
 *  `Generator` is destroyed. Copy or move the value out if it must survive
 *  beyond the current iteration step.
 *
 *  @par Iteration model
 *  `Aleph::Generator<T>` models a C++ *input range*: `begin()` resumes the
 *  coroutine to its first `co_yield` (or to completion, if the body never
 *  yields) and returns an iterator; `end()` returns `std::default_sentinel`.
 *  The iterator is **single-pass** — advancing it resumes the underlying
 *  coroutine, so only one iterator on a given `Generator` should be
 *  advanced at a time. Copying a `Generator` is disallowed (it owns the
 *  coroutine frame); move it instead.
 *
 *  @par Exception propagation
 *  An exception thrown inside the coroutine body is captured and rethrown
 *  from the call that resumed the coroutine (`operator++` or `begin()` for
 *  the first element) — exactly where a hand-written iterator would throw.
 *
 *  @par Early termination
 *  Destroying the `Generator` (or letting it go out of scope) before it is
 *  exhausted destroys the coroutine frame and unwinds any pending stack
 *  frames inside the coroutine body — `co_yield` sits inside ordinary C++
 *  control flow, so local objects along that path are destroyed normally.
 *
 *  @par Thread Safety
 *  A single `Generator<T>` instance is not thread-safe: it owns one
 *  suspended coroutine frame with no synchronization. Distinct instances
 *  (including ones produced by concurrent calls to the same generator
 *  function) may be driven from distinct threads.
 *
 *  @par Example
 *  @code
 *  Aleph::Generator<int> range(int lo, int hi)
 *  {
 *    for (int i = lo; i < hi; ++i)
 *      co_yield i;
 *  }
 *
 *  int total = 0;
 *  for (int x : range(0, 5))
 *    total += x;   // 0+1+2+3+4 == 10, computed lazily
 *  @endcode
 *
 *  @ingroup FunctionalProgramming
 */
template <class T>
class Generator
{
public:
  /// The promise type the compiler instantiates for a coroutine returning
  /// `Generator<T>`. Not intended to be used directly by callers.
  struct promise_type
  {
    std::add_pointer_t<T> current_value = nullptr;
    std::exception_ptr pending_exception = nullptr;

    /// A `Generator` starts suspended: nothing in the body runs until the
    /// first `begin()`/`resume()`.
    static std::suspend_always initial_suspend() noexcept { return {}; }

    /// Stay suspended at the end so the promise remains valid until the
    /// `Generator` is destroyed.
    static std::suspend_always final_suspend() noexcept { return {}; }

    /// Construct the `Generator<T>` handle returned to the caller.
    Generator get_return_object() noexcept
    {
      return Generator{Handle::from_promise(*this)};
    }

    /// `co_yield value;` suspends the coroutine and exposes `value` via
    /// `current_value` until the next resume.
    std::suspend_always yield_value(std::remove_reference_t<T> &value) noexcept
    {
      current_value = std::addressof(value);
      return {};
    }

    /// \overload yield_value(std::remove_reference_t<T> &) for rvalues.
    std::suspend_always yield_value(std::remove_reference_t<T> &&value) noexcept
    {
      current_value = std::addressof(value);
      return {};
    }

    /// Generators do not return a value; `co_return;` (or falling off the
    /// end of the body) simply ends the sequence.
    static void return_void() noexcept {}

    /// Captures an exception thrown in the coroutine body so it can be
    /// rethrown from the resuming call (`operator++`/`begin()`).
    void unhandled_exception() noexcept
    {
      pending_exception = std::current_exception();
    }

    /// Generators are not awaited on; `co_await` inside the body is
    /// intentionally unsupported to keep `Generator<T>` a pure producer.
    template <class U>
    std::suspend_never await_transform(U &&) = delete;
  };

  using Handle = std::coroutine_handle<promise_type>;

  /** @brief Single-pass input iterator over the yielded sequence.
   *
   *  Obtained from `Generator::begin()`; compares against
   *  `std::default_sentinel` (returned by `Generator::end()`) to detect
   *  exhaustion. Advancing the iterator resumes the coroutine to its next
   *  `co_yield` (or to completion).
   */
  class iterator
  {
    Handle handle_{};

  public:
    using iterator_category = std::input_iterator_tag; ///< Category.
    using value_type = std::remove_reference_t<T>;      ///< Element type.
    using difference_type = std::ptrdiff_t;              ///< Distance type.
    using reference = T &;                               ///< Reference type.
    using pointer = std::add_pointer_t<T>;                ///< Pointer type.

    /// Construct a singular (past-the-end-compatible) iterator.
    iterator() noexcept = default;

    /// Construct from a coroutine handle (internal use).
    explicit iterator(Handle h) noexcept : handle_(h) {}

    /** @brief Resume the coroutine to its next `co_yield` (pre-increment).
     *  @return `*this`, now referring to the next value (or exhausted).
     *  @throws std::domain_error if the iterator is already at `end()`.
     *          Resuming a coroutine past its final suspend point is
     *          undefined behavior, so this is checked explicitly rather
     *          than left to manifest as UB.
     *  @throws Whatever the coroutine body threw before its next
     *          `co_yield`, if anything.
     */
    iterator &operator++()
    {
      ah_domain_error_if(done())
        << "Generator::iterator::operator++(): cannot advance past end()";
      handle_.resume();
      if (handle_.promise().pending_exception)
        std::rethrow_exception(std::exchange(handle_.promise().pending_exception, nullptr));
      return *this;
    }

    /** @brief Resume the coroutine to its next `co_yield` (post-increment).
     *
     *  Returns `void` rather than the pre-increment value: single-pass
     *  input iterators cannot meaningfully support `*it++` (there is no
     *  previous value left to dereference once the coroutine has resumed),
     *  matching the standard library's own single-pass generator iterators.
     *
     *  @throws Same as `operator++()`.
     */
    void operator++(int) { ++*this; }

    /// Dereference to the most recently yielded value.
    reference operator*() const noexcept
    {
      return static_cast<reference>(*handle_.promise().current_value);
    }

    /// Member access on the most recently yielded value.
    pointer operator->() const noexcept
    {
      return handle_.promise().current_value;
    }

    /// True when the coroutine has run to completion (or was never begun).
    [[nodiscard]] bool done() const noexcept
    {
      return not handle_ or handle_.done();
    }

    /// Compare against the end sentinel: equal once the sequence is done.
    friend bool operator==(const iterator &it, std::default_sentinel_t) noexcept
    {
      return it.done();
    }
  };

  /// Construct an empty/singular `Generator` (no coroutine attached).
  Generator() noexcept = default;

  /// Construct from a coroutine handle (internal use by `promise_type`).
  explicit Generator(Handle h) noexcept : handle_(h) {}

  /// Deleted: a `Generator` owns a single coroutine frame, which cannot be
  /// duplicated. Move it instead.
  Generator(const Generator &) = delete;

  /// Deleted: see the copy constructor.
  Generator &operator=(const Generator &) = delete;

  /// Move constructor: transfers coroutine-frame ownership.
  Generator(Generator &&o) noexcept : handle_(std::exchange(o.handle_, {})) {}

  /// Move assignment: destroys any owned frame, then steals `o`'s.
  Generator &operator=(Generator &&o) noexcept
  {
    if (this != &o)
      {
        if (handle_)
          handle_.destroy();
        handle_ = std::exchange(o.handle_, {});
      }
    return *this;
  }

  /// Destroys the coroutine frame, if any, unwinding pending local state
  /// inside the (possibly still-suspended) coroutine body.
  ~Generator()
  {
    if (handle_)
      handle_.destroy();
  }

  /** @brief Begin iterating: resumes the coroutine to its first `co_yield`.
   *  @return An iterator positioned at the first value (or already `end()`
   *          if the body never yields, was already exhausted, or this is a
   *          default-constructed/moved-from `Generator` with no coroutine
   *          attached — an "empty/singular" `Generator` iterates zero times
   *          rather than throwing, matching the usual empty-range/empty-
   *          container convention).
   *  @throws Whatever the coroutine body throws before its first
   *          `co_yield`, if anything.
   *  @note `begin()` is lvalue-qualified on purpose: range-`for` over a
   *        temporary generator is safe because the loop binds the temporary to
   *        a hidden local variable, but a direct `make_generator().begin()`
   *        would leave the returned iterator pointing at a destroyed coroutine
   *        frame. That direct call is therefore a compile-time error.
   *  @note Calling `begin()` more than once while the sequence is still
   *        mid-traversal resumes the coroutine again, which is almost
   *        certainly not what you want on a single-pass sequence; iterate
   *        it exactly once. Calling it again *after* the sequence is
   *        exhausted is safe and simply returns `end()`: resuming a
   *        coroutine past its final suspend point is undefined behavior,
   *        so that case is detected and the resume is skipped instead.
   */
  iterator begin() &
  {
    if (handle_ and not handle_.done())
      {
        handle_.resume();
        if (handle_.promise().pending_exception)
          std::rethrow_exception(std::exchange(handle_.promise().pending_exception, nullptr));
      }
    return iterator{handle_};
  }

  /// End sentinel, per the C++20 sentinel-based range model.
  static std::default_sentinel_t end() noexcept { return std::default_sentinel; }

private:
  Handle handle_{};
};

} // end namespace Aleph

#endif // AH_GENERATOR_H
