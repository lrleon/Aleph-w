
/*
                          Aleph_w

  Data structures & Algorithms
  version 5.1.0
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

/** @file ah-cpp-compat.H
 *  @brief C++20/23 compatibility layer: feature detection + `Aleph::` polyfills.
 *
 *  Aleph-w targets **C++20** as its baseline (GCC 11+, Clang 14+) and also
 *  supports **C++23**. (The library requires C++20 because it uses concepts
 *  throughout; this compatibility header alone happens to also compile under
 *  C++17.) This header centralizes the detection of selected C++23 *library*
 *  facilities and
 *  exposes them under the `Aleph::` namespace: it aliases the `std::` facility
 *  when the toolchain provides it, and otherwise provides a conforming-subset
 *  polyfill. The rest of the library can then use e.g. `Aleph::expected`
 *  unconditionally, regardless of `-std`.
 *
 *  @par Detection macros (each defined to 1 or 0)
 *  - `ALEPH_HAS_STD_EXPECTED`  — `<expected>` (`std::expected`, C++23)
 *  - `ALEPH_HAS_STD_GENERATOR` — `<generator>` (`std::generator`, C++23)
 *  - `ALEPH_HAS_STD_FLAT_MAP`  — `<flat_map>` (`std::flat_map`, C++23)
 *  - `ALEPH_HAS_STD_MDSPAN`    — `<mdspan>` (`std::mdspan`, C++23)
 *
 *  @par Provided in this release
 *  `Aleph::expected<T,E>`, `Aleph::unexpected<E>`, `Aleph::bad_expected_access`,
 *  and the `Aleph::unexpect` tag. The polyfill implements the common surface
 *  (construction, `has_value`/`value`/`error`/`operator bool`/`value_or`, and
 *  the monadic `and_then`/`transform`/`or_else`); it is **not** a bit-for-bit
 *  reimplementation of the standard. When `ALEPH_HAS_STD_EXPECTED` is 1,
 *  `Aleph::expected` is exactly `std::expected`.
 *
 *  Flat containers shipped natively as `Aleph::FlatSet`/`Aleph::FlatMap`
 *  (see `tpl_flat_set.H`/`tpl_flat_map.H`); they are independent of the
 *  standard library ones, so `ALEPH_HAS_STD_FLAT_MAP` remains detection-only.
 *  Generators and `mdspan` land in their own feature branches; only their
 *  detection macros are defined here for now.
 *
 *  @note Thread-safety: the types are value types with no shared state; the
 *        usual "distinct objects, distinct threads" rule applies.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath Leon
 */

#ifndef ALEPH_CPP_COMPAT_H
#define ALEPH_CPP_COMPAT_H

#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#if __has_include(<version>)
#  include <version>  // library feature-test macros (__cpp_lib_*)
#endif

// ---------------------------------------------------------------------------
// Feature detection for selected C++23 library facilities.
// ---------------------------------------------------------------------------

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202211L
#  define ALEPH_HAS_STD_EXPECTED 1
#else
#  define ALEPH_HAS_STD_EXPECTED 0
#endif

#if defined(__cpp_lib_generator) && __cpp_lib_generator >= 202207L
#  define ALEPH_HAS_STD_GENERATOR 1
#else
#  define ALEPH_HAS_STD_GENERATOR 0
#endif

#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202207L
#  define ALEPH_HAS_STD_FLAT_MAP 1
#else
#  define ALEPH_HAS_STD_FLAT_MAP 0
#endif

#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan >= 202207L
#  define ALEPH_HAS_STD_MDSPAN 1
#else
#  define ALEPH_HAS_STD_MDSPAN 0
#endif

// ---------------------------------------------------------------------------
// Aleph::expected
// ---------------------------------------------------------------------------

#if ALEPH_HAS_STD_EXPECTED

#include <expected>

namespace Aleph {
template <class T, class E> using expected = std::expected<T, E>;
template <class E> using unexpected = std::unexpected<E>;
template <class E> using bad_expected_access = std::bad_expected_access<E>;
using std::unexpect;
using std::unexpect_t;
}  // namespace Aleph

#else  // ----- C++20 / C++17 polyfill ---------------------------------------

namespace Aleph {

namespace cpp_compat_detail {
/// C++17-safe equivalent of C++20 `cpp_compat_detail::remove_cvref_t`.
template <class U>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<U>>;
}  // namespace cpp_compat_detail

/** @brief Tag type used to construct an `expected` holding an error in place.
 *
 *  Pass `Aleph::unexpect` as the first argument to `expected` constructors to
 *  signal in-place construction of the error member.
 *
 *  @par Thread Safety
 *  This is a stateless tag type; all operations are thread-safe.
 */
struct unexpect_t
{
  explicit unexpect_t() = default;
};

/** @brief Tag value of type `unexpect_t`.
 *
 *  Use this constant to construct an `expected` in the error state:
 *  @code
 *  expected<int, std::string> e(unexpect, "error message");
 *  @endcode
 */
inline constexpr unexpect_t unexpect{};

/** @brief Wrapper for the error value used to initialize an `expected`.
 *
 *  Represents an error in contexts where `expected` is used. Construct an
 *  `unexpected` to return an error from a function returning `expected<T,E>`:
 *  @code
 *  expected<int, std::string> parse(const std::string &s) {
 *    if (bad) return unexpected<std::string>("parse error");
 *    return value;
 *  }
 *  @endcode
 *
 *  @tparam E Error type (must be movable).
 *
 *  @par Exception Safety
 *  Basic guarantee: if `E` construction throws, the object is not constructed.
 *
 *  @par Thread Safety
 *  Value type with no shared state. Operations on distinct objects are safe.
 */
template <class E>
class unexpected
{
  E err_;

public:
  /** @brief Construct from an lvalue error.
   *  @param e Error value to copy.
   *  @throws Any exception thrown by `E`'s copy constructor.
   */
  constexpr explicit unexpected(const E &e) : err_(e) {}

  /** @brief Construct from an rvalue error.
   *  @param e Error value to move.
   *  @throws Any exception thrown by `E`'s move constructor.
   */
  constexpr explicit unexpected(E &&e) : err_(std::move(e)) {}

  /** @brief Access the error (const lvalue).
   *  @return Const reference to the stored error.
   *  @par Complexity O(1).
   */
  constexpr const E &error() const & noexcept { return err_; }

  /** @brief Access the error (lvalue).
   *  @return Mutable reference to the stored error.
   *  @par Complexity O(1).
   */
  constexpr E &error() & noexcept { return err_; }

  /** @brief Access the error (rvalue).
   *  @return Rvalue reference to the stored error.
   *  @par Complexity O(1).
   */
  constexpr E &&error() && noexcept { return std::move(err_); }
};

template <class E>
unexpected(E) -> unexpected<E>;

/** @brief Exception thrown by `expected::value()` when no value is held.
 *
 *  Thrown when calling `value()` on an `expected` in the error state. Carries
 *  a copy of the stored error for inspection.
 *
 *  @tparam E Error type (must be movable).
 *
 *  @par Thread Safety
 *  Exception objects may be copied across threads; `what()` and `error()` are
 *  safe to call concurrently on the same object.
 */
template <class E>
class bad_expected_access : public std::exception
{
  E err_;

public:
  /** @brief Construct with an error value.
   *  @param e The error to store in the exception.
   *  @throws Any exception thrown by `E`'s move constructor.
   */
  explicit bad_expected_access(E e) : err_(std::move(e)) {}

  /** @brief Get exception description.
   *  @return Static error string.
   *  @par Complexity O(1). Does not throw.
   */
  [[nodiscard]] const char *what() const noexcept override
  {
    return "Aleph::bad_expected_access: expected holds an error, not a value";
  }

  /** @brief Access the stored error (const lvalue).
   *  @return Const reference to the error.
   *  @par Complexity O(1). Does not throw.
   */
  [[nodiscard]] const E &error() const & noexcept { return err_; }

  /** @brief Access the stored error (lvalue).
   *  @return Mutable reference to the error.
   *  @par Complexity O(1). Does not throw.
   */
  [[nodiscard]] E &error() & noexcept { return err_; }
};

/** @brief Minimal `std::expected`-style result type for C++20.
 *
 *  Holds either a value of type `T` or an error of type `E`. Aliased to
 *  `std::expected` when the toolchain provides `<expected>`. See the file
 *  header for the supported surface.
 *
 *  @tparam T value type.
 *  @tparam E error type.
 */
template <class T, class E>
class expected
{
  static_assert(not std::is_same_v<std::remove_cv_t<T>, unexpect_t>,
                "T must not be unexpect_t");

  union
  {
    T val_;
    E err_;
  };
  bool has_;

  void destroy() noexcept
  {
    if (has_)
      val_.~T();
    else
      err_.~E();
  }

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  /** @brief Default-construct with a value.
   *
   *  Constructs the value member via default-initialization.
   *
   *  @pre `T` must be default-constructible.
   *  @post `has_value() == true`.
   *  @throws Any exception thrown by `T`'s default constructor.
   *  @par Complexity O(1) plus `T` construction.
   *  @par Exception Safety Basic guarantee: if construction throws, no object.
   */
  constexpr expected() : val_(), has_(true) {}

  /** @brief Construct from a value.
   *
   *  Implicit conversion from `U` to `expected<T,E>` when `T` is constructible
   *  from `U`. Enables patterns like `return 42;` from a function returning
   *  `expected<int, Error>`.
   *
   *  @tparam U Value type (forwarded to `T`).
   *  @param v The value to store.
   *  @post `has_value() == true` and `*this == v`.
   *  @throws Any exception thrown by `T(std::forward<U>(v))`.
   *  @par Complexity O(1) plus `T` construction.
   *  @par Exception Safety Basic guarantee.
   */
  template <class U = T,
            class = std::enable_if_t<
                std::is_constructible_v<T, U &&> and
                not std::is_same_v<cpp_compat_detail::remove_cvref_t<U>, expected> and
                not std::is_same_v<cpp_compat_detail::remove_cvref_t<U>, unexpect_t>>>
  constexpr expected(U &&v) : val_(std::forward<U>(v)), has_(true)
  {
  }

  /** @brief Construct an error from `unexpected` (lvalue).
   *
   *  @tparam G Error type of the `unexpected` (must convert to `E`).
   *  @param u The unexpected error wrapper.
   *  @post `has_value() == false` and `error() == u.error()`.
   *  @throws Any exception thrown by `E(u.error())`.
   *  @par Complexity O(1) plus `E` construction.
   */
  template <class G>
  constexpr expected(const unexpected<G> &u) : err_(u.error()), has_(false)
  {
  }

  /** @brief Construct an error from `unexpected` (rvalue).
   *
   *  @tparam G Error type of the `unexpected` (must convert to `E`).
   *  @param u The unexpected error wrapper.
   *  @post `has_value() == false` and `error() == std::move(u).error()`.
   *  @throws Any exception thrown by `E(std::move(u).error())`.
   *  @par Complexity O(1) plus `E` construction.
   */
  template <class G>
  constexpr expected(unexpected<G> &&u)
    : err_(std::move(u).error()), has_(false)
  {
  }

  /** @brief Construct an error in place.
   *
   *  Pass `Aleph::unexpect` as the first argument to construct the error from
   *  arbitrary arguments forwarded to `E`'s constructor:
   *  @code
   *  expected<int, std::string> e(unexpect, 5, 'x'); // E constructed as string(5,'x')
   *  @endcode
   *
   *  @tparam Args Argument types forwarded to `E`.
   *  @param args Arguments forwarded to `E`'s constructor.
   *  @post `has_value() == false`.
   *  @throws Any exception thrown by `E(std::forward<Args>(args)...)`.
   *  @par Complexity O(1) plus `E` construction.
   */
  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...args)
    : err_(std::forward<Args>(args)...), has_(false)
  {
  }

  /** @brief Copy constructor.
   *
   *  Copies the active member (value or error) from `o`.
   *
   *  @param o Source `expected` to copy from.
   *  @post `has_value() == o.has_value()` and active member equals `o`'s.
   *  @throws Any exception thrown by `T` or `E` copy constructor.
   *  @par Complexity O(1) plus active member copy.
   *  @par Exception Safety Basic guarantee.
   */
  expected(const expected &o) : has_(o.has_)
  {
    if (has_)
      ::new (static_cast<void *>(std::addressof(val_))) T(o.val_);
    else
      ::new (static_cast<void *>(std::addressof(err_))) E(o.err_);
  }

  /** @brief Move constructor.
   *
   *  Moves the active member (value or error) from `o`.
   *
   *  @param o Source `expected` to move from.
   *  @post `has_value() == o.has_value()` and active member moved from `o`.
   *  @par Complexity O(1) plus active member move.
   *  @par Exception Safety Conditional noexcept: noexcept if both `T` and `E`
   *       have noexcept move constructors, otherwise basic guarantee.
   */
  expected(expected &&o) noexcept(std::is_nothrow_move_constructible_v<T> and
                                  std::is_nothrow_move_constructible_v<E>)
    : has_(o.has_)
  {
    if (has_)
      ::new (static_cast<void *>(std::addressof(val_))) T(std::move(o.val_));
    else
      ::new (static_cast<void *>(std::addressof(err_))) E(std::move(o.err_));
  }

  /** @brief Copy assignment.
   *
   *  Replaces the active member with a copy of `o`'s active member. Uses a
   *  copy-and-move idiom to provide strong exception safety: if construction
   *  of the new member throws, `*this` is left in its original state.
   *
   *  @param o Source `expected` to assign from.
   *  @return `*this`.
   *  @post `has_value() == o.has_value()` and active member equals `o`'s.
   *  @throws Any exception thrown by `T` or `E` copy/move constructor.
   *  @par Complexity O(1) plus destruction + construction.
   *  @par Exception Safety Strong guarantee: if copy fails, `*this` unchanged.
   */
  expected &operator=(const expected &o)
  {
    if (this != std::addressof(o))
      {
        expected tmp(o);
        destroy();
        has_ = tmp.has_;
        if (has_)
          ::new (static_cast<void *>(std::addressof(val_))) T(std::move(tmp.val_));
        else
          ::new (static_cast<void *>(std::addressof(err_))) E(std::move(tmp.err_));
      }
    return *this;
  }

  /** @brief Move assignment.
   *
   *  Replaces the active member by moving from `o`. Uses move-and-swap idiom
   *  to provide strong exception safety.
   *
   *  @param o Source `expected` to move from.
   *  @return `*this`.
   *  @post `has_value() == o.has_value()` and active member moved from `o`.
   *  @par Complexity O(1) plus destruction + move construction.
   *  @par Exception Safety Strong guarantee if move is noexcept; otherwise basic.
   *  @par Conditional noexcept if both `T` and `E` have noexcept move ctors.
   */
  expected &operator=(expected &&o) noexcept(
      std::is_nothrow_move_constructible_v<T> and
      std::is_nothrow_move_constructible_v<E>)
  {
    if (this != std::addressof(o))
      {
        expected tmp(std::move(o));
        destroy();
        has_ = tmp.has_;
        if (has_)
          ::new (static_cast<void *>(std::addressof(val_))) T(std::move(tmp.val_));
        else
          ::new (static_cast<void *>(std::addressof(err_))) E(std::move(tmp.err_));
      }
    return *this;
  }

  /** @brief Destructor.
   *
   *  Destroys the active member (value or error).
   *
   *  @par Complexity O(1) plus active member destruction.
   *  @par Exception Safety No-throw guarantee.
   */
  ~expected() { destroy(); }

  // -- observers ----------------------------------------------------------

  /** @brief Check if a value is held.
   *  @return `true` if the object holds a value, `false` if it holds an error.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr bool has_value() const noexcept { return has_; }

  /** @brief Contextual conversion to bool.
   *  @return `true` if the object holds a value, `false` if it holds an error.
   *  @par Complexity O(1).
   *  @par Usage Enables `if (e) ...` where `e` is an `expected`.
   */
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_; }

  /** @brief Access the value (const lvalue).
   *  @return Const reference to the stored value.
   *  @pre `has_value() == true`. Behavior is undefined if an error is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr const T &operator*() const & noexcept { return val_; }

  /** @brief Access the value (lvalue).
   *  @return Mutable reference to the stored value.
   *  @pre `has_value() == true`. Behavior is undefined if an error is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr T &operator*() & noexcept { return val_; }

  /** @brief Access the value via pointer (const).
   *  @return Const pointer to the stored value.
   *  @pre `has_value() == true`. Behavior is undefined if an error is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr const T *operator->() const noexcept
  {
    return std::addressof(val_);
  }

  /** @brief Access the value via pointer (mutable).
   *  @return Mutable pointer to the stored value.
   *  @pre `has_value() == true`. Behavior is undefined if an error is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr T *operator->() noexcept
  {
    return std::addressof(val_);
  }

  /** @brief Access the value with exception on error (const lvalue).
   *  @return Const reference to the stored value.
   *  @throws bad_expected_access<E> if an error is held.
   *  @par Complexity O(1).
   *  @par Exception Safety Strong guarantee: throws if error is held.
   */
  [[nodiscard]] constexpr const T &value() const &
  {
    if (not has_)
      throw bad_expected_access<E>(err_);
    return val_;
  }

  /** @brief Access the value with exception on error (lvalue).
   *  @return Mutable reference to the stored value.
   *  @throws bad_expected_access<E> if an error is held.
   *  @par Complexity O(1).
   *  @par Exception Safety Strong guarantee: throws if error is held.
   */
  [[nodiscard]] constexpr T &value() &
  {
    if (not has_)
      throw bad_expected_access<E>(err_);
    return val_;
  }

  /** @brief Access the error (const lvalue).
   *  @return Const reference to the stored error.
   *  @pre `has_value() == false`. Behavior is undefined if a value is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr const E &error() const & noexcept { return err_; }

  /** @brief Access the error (lvalue).
   *  @return Mutable reference to the stored error.
   *  @pre `has_value() == false`. Behavior is undefined if a value is held.
   *  @par Complexity O(1).
   */
  [[nodiscard]] constexpr E &error() & noexcept { return err_; }

  /** @brief Get the value or a default.
   *
   *  Returns the stored value if present, otherwise returns `default_value`
   *  converted to `T`. Useful for providing fallbacks without branching:
   *  @code
   *  int x = parse(input).value_or(0);  // 0 if parse failed
   *  @endcode
   *
   *  @tparam U Type of the default value (must convert to `T`).
   *  @param default_value Fallback value to return if an error is held.
   *  @return The stored value if present, otherwise `static_cast<T>(default_value)`.
   *  @par Complexity O(1) plus `T` copy if value is held, or `T` construction from `U` if not.
   */
  template <class U>
  [[nodiscard]] constexpr T value_or(U &&default_value) const &
  {
    return has_ ? val_ : static_cast<T>(std::forward<U>(default_value));
  }

  // -- monadic operations -------------------------------------------------

  /** @brief Monadic bind: chain another fallible operation.
   *
   *  If a value is held, invokes `f(value)` and returns its result (which must
   *  be an `expected`). If an error is held, propagates the error without
   *  calling `f`. Enables chaining fallible operations:
   *  @code
   *  auto res = parse(input)
   *    .and_then([](int v) -> expected<int, Error> { return validate(v); })
   *    .and_then([](int v) -> expected<int, Error> { return process(v); });\n  @endcode
   *
   *  @tparam F Callable type: `F(const T&) -> expected<U, E>`.
   *  @param f Function to apply to the value.
   *  @return Result of `f(value)` if value is held, otherwise `expected<U,E>(error)`.
   *  @par Complexity O(1) plus `f` invocation if value is held.
   */
  template <class F>
  [[nodiscard]] auto and_then(F &&f) const &
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const T &>>;
    if (has_)
      return std::invoke(std::forward<F>(f), val_);
    return R(unexpected<E>(err_));
  }

  /** @brief Monadic map: transform the value.
   *
   *  If a value is held, invokes `f(value)` and wraps the result in an
   *  `expected<U, E>`. If an error is held, propagates the error. Use this to
   *  apply pure transformations without changing the error channel:
   *  @code
   *  auto doubled = parse(input).transform([](int v) { return v * 2; });
   *  @endcode
   *
   *  @tparam F Callable type: `F(const T&) -> U`.
   *  @param f Function to apply to the value.
   *  @return `expected<U,E>(f(value))` if value is held, otherwise `expected<U,E>(error)`.
   *  @par Complexity O(1) plus `f` invocation if value is held.
   */
  template <class F>
  [[nodiscard]] auto transform(F &&f) const &
  {
    using U = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const T &>>;
    using R = expected<U, E>;
    if (has_)
      return R(std::invoke(std::forward<F>(f), val_));
    return R(unexpected<E>(err_));
  }

  /** @brief Monadic error recovery: handle the error.
   *
   *  If an error is held, invokes `f(error)` and returns its result (which must
   *  be an `expected`). If a value is held, propagates the value without calling
   *  `f`. Use this to recover from errors or provide fallbacks:
   *  @code
   *  auto recovered = parse(input)
   *    .or_else([](const Error &e) -> expected<int, Error> {
   *      log(e);
   *      return 0;  // default value on error
   *    });
   *  @endcode
   *
   *  @tparam F Callable type: `F(const E&) -> expected<T, F>`.
   *  @param f Function to apply to the error.
   *  @return Result of `f(error)` if error is held, otherwise `expected<T,F>(value)`.
   *  @par Complexity O(1) plus `f` invocation if error is held.
   */
  template <class F>
  [[nodiscard]] auto or_else(F &&f) const &
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const E &>>;
    if (has_)
      return R(val_);
    return std::invoke(std::forward<F>(f), err_);
  }
};

/** @brief Specialization of `expected` for `void` value type.
 *
 *  Models success/failure without carrying a value on success. Common pattern
 *  for operations that report errors but have no meaningful return value.
 *
 *  @tparam E error type.
 */
template <class E>
class expected<void, E>
{
  union
  {
    E err_;
  };
  bool has_;

  void destroy() noexcept
  {
    if (not has_)
      err_.~E();
  }

public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  constexpr expected() noexcept : has_(true) {}

  template <class G>
  constexpr expected(const unexpected<G> &u) : err_(u.error()), has_(false)
  {
  }

  template <class G>
  constexpr expected(unexpected<G> &&u)
    : err_(std::move(u).error()), has_(false)
  {
  }

  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...args)
    : err_(std::forward<Args>(args)...), has_(false)
  {
  }

  expected(const expected &o) : has_(o.has_)
  {
    if (not has_)
      ::new (static_cast<void *>(std::addressof(err_))) E(o.err_);
  }

  expected(expected &&o) noexcept(std::is_nothrow_move_constructible_v<E>)
    : has_(o.has_)
  {
    if (not has_)
      ::new (static_cast<void *>(std::addressof(err_))) E(std::move(o.err_));
  }

  expected &operator=(const expected &o)
  {
    if (this != std::addressof(o))
      {
        expected tmp(o);
        destroy();
        has_ = tmp.has_;
        if (not has_)
          ::new (static_cast<void *>(std::addressof(err_))) E(std::move(tmp.err_));
      }
    return *this;
  }

  expected &operator=(expected &&o) noexcept(
      std::is_nothrow_move_constructible_v<E>)
  {
    if (this != std::addressof(o))
      {
        expected tmp(std::move(o));
        destroy();
        has_ = tmp.has_;
        if (not has_)
          ::new (static_cast<void *>(std::addressof(err_))) E(std::move(tmp.err_));
      }
    return *this;
  }

  ~expected() { destroy(); }

  [[nodiscard]] constexpr bool has_value() const noexcept { return has_; }
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_; }

  constexpr void value() const
  {
    if (not has_)
      throw bad_expected_access<E>(err_);
  }

  [[nodiscard]] constexpr const E &error() const & noexcept { return err_; }
  [[nodiscard]] constexpr E &error() & noexcept { return err_; }

  template <class F>
  [[nodiscard]] auto and_then(F &&f) const
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F>>;
    if (has_)
      return std::invoke(std::forward<F>(f));
    return R(unexpected<E>(err_));
  }

  template <class F>
  [[nodiscard]] auto or_else(F &&f) const
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const E &>>;
    if (has_)
      return R();
    return std::invoke(std::forward<F>(f), err_);
  }
};

}  // namespace Aleph

#endif  // ALEPH_HAS_STD_EXPECTED

#endif  // ALEPH_CPP_COMPAT_H