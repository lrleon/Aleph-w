
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
 *  Generators, flat containers and `mdspan` land in their own feature branches;
 *  only their detection macros are defined here for now.
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
#include <type_traits>
#include <utility>

#if __has_include(<version>)
#  include <version>  // library feature-test macros (__cpp_lib_*)
#endif

// ---------------------------------------------------------------------------
// Feature detection for selected C++23 library facilities.
// ---------------------------------------------------------------------------

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
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

/// Tag type used to construct an `expected` holding an error in place.
struct unexpect_t
{
  explicit unexpect_t() = default;
};

/// Tag value of type `unexpect_t`.
inline constexpr unexpect_t unexpect{};

/** @brief Wrapper for the error value used to initialize an `expected`.
 *  @tparam E error type.
 */
template <class E>
class unexpected
{
  E err_;

public:
  constexpr explicit unexpected(const E &e) : err_(e) {}
  constexpr explicit unexpected(E &&e) : err_(std::move(e)) {}

  constexpr const E &error() const & noexcept { return err_; }
  constexpr E &error() & noexcept { return err_; }
  constexpr E &&error() && noexcept { return std::move(err_); }
};

template <class E>
unexpected(E) -> unexpected<E>;

/** @brief Exception thrown by `expected::value()` when no value is held.
 *  @tparam E error type carried by the exception.
 */
template <class E>
class bad_expected_access : public std::exception
{
  E err_;

public:
  explicit bad_expected_access(E e) : err_(std::move(e)) {}

  [[nodiscard]] const char *what() const noexcept override
  {
    return "Aleph::bad_expected_access: expected holds an error, not a value";
  }

  [[nodiscard]] const E &error() const & noexcept { return err_; }
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

  /// Default-construct a value (requires `T` default-constructible).
  constexpr expected() : val_(), has_(true) {}

  /// Construct from a value.
  template <class U = T,
            class = std::enable_if_t<
                std::is_constructible_v<T, U &&> and
                not std::is_same_v<cpp_compat_detail::remove_cvref_t<U>, expected> and
                not std::is_same_v<cpp_compat_detail::remove_cvref_t<U>, unexpect_t>>>
  constexpr expected(U &&v) : val_(std::forward<U>(v)), has_(true)
  {
  }

  /// Construct an error from `unexpected`.
  template <class G>
  constexpr expected(const unexpected<G> &u) : err_(u.error()), has_(false)
  {
  }

  template <class G>
  constexpr expected(unexpected<G> &&u)
    : err_(std::move(u).error()), has_(false)
  {
  }

  /// Construct an error in place with the `unexpect` tag.
  template <class... Args>
  constexpr explicit expected(unexpect_t, Args &&...args)
    : err_(std::forward<Args>(args)...), has_(false)
  {
  }

  expected(const expected &o) : has_(o.has_)
  {
    if (has_)
      ::new (static_cast<void *>(std::addressof(val_))) T(o.val_);
    else
      ::new (static_cast<void *>(std::addressof(err_))) E(o.err_);
  }

  expected(expected &&o) noexcept(std::is_nothrow_move_constructible_v<T> and
                                  std::is_nothrow_move_constructible_v<E>)
    : has_(o.has_)
  {
    if (has_)
      ::new (static_cast<void *>(std::addressof(val_))) T(std::move(o.val_));
    else
      ::new (static_cast<void *>(std::addressof(err_))) E(std::move(o.err_));
  }

  expected &operator=(const expected &o)
  {
    if (this != std::addressof(o))
      {
        destroy();
        has_ = o.has_;
        if (has_)
          ::new (static_cast<void *>(std::addressof(val_))) T(o.val_);
        else
          ::new (static_cast<void *>(std::addressof(err_))) E(o.err_);
      }
    return *this;
  }

  expected &operator=(expected &&o) noexcept(
      std::is_nothrow_move_constructible_v<T> and
      std::is_nothrow_move_constructible_v<E>)
  {
    if (this != std::addressof(o))
      {
        destroy();
        has_ = o.has_;
        if (has_)
          ::new (static_cast<void *>(std::addressof(val_))) T(std::move(o.val_));
        else
          ::new (static_cast<void *>(std::addressof(err_))) E(std::move(o.err_));
      }
    return *this;
  }

  ~expected() { destroy(); }

  // -- observers ----------------------------------------------------------

  [[nodiscard]] constexpr bool has_value() const noexcept { return has_; }
  [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_; }

  [[nodiscard]] constexpr const T &operator*() const & noexcept { return val_; }
  [[nodiscard]] constexpr T &operator*() & noexcept { return val_; }
  [[nodiscard]] constexpr const T *operator->() const noexcept
  {
    return std::addressof(val_);
  }
  [[nodiscard]] constexpr T *operator->() noexcept
  {
    return std::addressof(val_);
  }

  [[nodiscard]] constexpr const T &value() const &
  {
    if (not has_)
      throw bad_expected_access<E>(err_);
    return val_;
  }
  [[nodiscard]] constexpr T &value() &
  {
    if (not has_)
      throw bad_expected_access<E>(err_);
    return val_;
  }

  [[nodiscard]] constexpr const E &error() const & noexcept { return err_; }
  [[nodiscard]] constexpr E &error() & noexcept { return err_; }

  template <class U>
  [[nodiscard]] constexpr T value_or(U &&default_value) const &
  {
    return has_ ? val_ : static_cast<T>(std::forward<U>(default_value));
  }

  // -- monadic operations -------------------------------------------------

  /// If a value is held, return `f(value)` (which must yield an `expected`);
  /// otherwise propagate the error.
  template <class F>
  [[nodiscard]] auto and_then(F &&f) const &
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const T &>>;
    if (has_)
      return std::invoke(std::forward<F>(f), val_);
    return R(unexpected<E>(err_));
  }

  /// If a value is held, return `expected<f(value), E>`; otherwise propagate
  /// the error.
  template <class F>
  [[nodiscard]] auto transform(F &&f) const &
  {
    using U = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const T &>>;
    using R = expected<U, E>;
    if (has_)
      return R(std::invoke(std::forward<F>(f), val_));
    return R(unexpected<E>(err_));
  }

  /// If an error is held, return `f(error)` (which must yield an `expected`);
  /// otherwise propagate the value.
  template <class F>
  [[nodiscard]] auto or_else(F &&f) const &
  {
    using R = cpp_compat_detail::remove_cvref_t<std::invoke_result_t<F, const E &>>;
    if (has_)
      return R(val_);
    return std::invoke(std::forward<F>(f), err_);
  }
};

}  // namespace Aleph

#endif  // ALEPH_HAS_STD_EXPECTED

#endif  // ALEPH_CPP_COMPAT_H