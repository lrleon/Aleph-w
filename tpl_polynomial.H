
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

/** @file tpl_polynomial.H
 *  @brief Univariate polynomial ring arithmetic over generic coefficients.
 *
 *  Provides `Gen_Polynomial<Coefficient>`, a sparse univariate polynomial
 *  backed by `DynMapTree<size_t, Coefficient>`.  Only non-zero coefficients
 *  are stored, making operations efficient on high-degree sparse polynomials
 *  such as @f$x^{1000} + 1@f$.
 *
 *  The coefficient type must satisfy:
 *  - Default-constructible (yielding the additive identity, zero).
 *  - Constructible from `int` literals `0` and `1`.
 *  - Copy/move constructible and assignable.
 *  - Arithmetic: `+`, `-` (binary and unary), `*`, `/`.
 *  - Comparison: `==`, `!=`.
 *  - Ordered comparison `<` (needed by `cauchy_bound`, `sign_variations`,
 *    `bisect_root`, and `to_str`).
 *  - Stream output: `operator<<`.
 *
 *  For floating-point types, `std::abs` is used internally for
 *  epsilon-based zero detection.
 *
 *  @ingroup Algebra
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_POLYNOMIAL_H
#define TPL_POLYNOMIAL_H

#include <cmath>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <numeric>  // std::gcd
#include <random>   // std::mt19937_64

#include <ah-errors.H>
#include <tpl_dynMapTree.H>
#include <tpl_array.H>
#include <htlist.H>
#include <modular_arithmetic.H>  // mod_inv, mod_mul

namespace Aleph {

/** @brief Internal helpers for polynomial operations. */
namespace polynomial_detail {

/** @brief Fast exponentiation by squaring.
 *
 *  Computes @f$\text{base}^{\text{exp}}@f$ in @f$O(\log \text{exp})@f$
 *  multiplications.
 */
template <typename C>
C power(C base, size_t exp)
{
  C result = C(1);
  while (exp > 0)
    {
      if (exp & 1)
        result = result * base;
      base = base * base;
      exp >>= 1;
    }
  return result;
}

template <typename Int>
  requires std::is_integral_v<Int>
[[nodiscard]] uint64_t abs_to_u64(Int value) noexcept
{
  if constexpr (std::is_signed_v<Int>)
    {
      using Signed = std::make_signed_t<Int>;
      return value < Int{} ? static_cast<uint64_t>(-(static_cast<Signed>(value)))
                           : static_cast<uint64_t>(value);
    }
  else
    return static_cast<uint64_t>(value);
}

template <typename Int>
  requires std::is_integral_v<Int>
[[nodiscard]] uint64_t normalize_mod_u64(Int value, const uint64_t modulus) noexcept
{
  if (modulus == 0)
    return 0;

  if constexpr (std::is_signed_v<Int>)
    {
      const auto mod_i64 = static_cast<int64_t>(modulus);
      auto rem = static_cast<int64_t>(value % static_cast<Int>(mod_i64));
      if (rem < 0)
        rem += mod_i64;
      return static_cast<uint64_t>(rem);
    }
  else
    return static_cast<uint64_t>(value % static_cast<Int>(modulus));
}

template <typename Int>
  requires std::is_integral_v<Int>
[[nodiscard]] Int centered_from_mod_u64(const uint64_t value, const uint64_t modulus) noexcept
{
  if constexpr (std::is_signed_v<Int>)
    {
      const uint64_t canonical = modulus == 0 ? 0 : value % modulus;
      const uint64_t half = modulus / 2;
      if (canonical > half)
        return static_cast<Int>(static_cast<int64_t>(canonical) - static_cast<int64_t>(modulus));
      return static_cast<Int>(canonical);
    }
  else
    return static_cast<Int>(modulus == 0 ? 0 : value % modulus);
}

}  // end namespace polynomial_detail

/** @brief Univariate polynomial over a generic coefficient ring.
 *
 *  Internally stores only non-zero coefficients in a
 *  `DynMapTree<size_t, Coefficient>` mapping exponents to coefficients.
 *  The zero polynomial is represented by an empty map.
 *
 *  For floating-point coefficient types, an epsilon-based zero test is
 *  used automatically to prevent accumulation of near-zero terms.
 *
 *  @tparam Coefficient Coefficient ring (default `double`).
 *
 *  @ingroup Algebra
 */
template <typename Coefficient = double>
class Gen_Polynomial
{
public:
  using Coeff = Coefficient;

private:
  using Term = std::pair<size_t, Coefficient>;
  DynMapTree<size_t, Coefficient> coeffs;

  // --- Epsilon-based zero detection for floating-point types ----------

  /** @brief Tolerance threshold for floating-point zero detection.
   *
   *  Returns @f$128 \cdot \varepsilon@f$ for floating-point types and
   *  the additive identity for all others.
   */
  static constexpr Coefficient epsilon() noexcept
  {
    if constexpr (std::is_floating_point_v<Coefficient>)
      return Coefficient(128) * std::numeric_limits<Coefficient>::epsilon();
    else
      return Coefficient{};
  }

  /** @brief Test whether a coefficient is (approximately) zero.
   *
   *  For floating-point types, uses `std::abs(c) <= epsilon()`.
   *  For exact types, uses `c == Coefficient{}`.
   */
  static bool coeff_is_zero(const Coefficient &c) noexcept
  {
    if constexpr (std::is_floating_point_v<Coefficient>)
      return std::abs(c) <= epsilon();
    else
      return c == Coefficient{};
  }

  /** @brief Remove all entries whose coefficient is (approximately) zero. */
  void remove_zeros()
  {
    DynList<size_t> zeros;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (coeff_is_zero(p.second))
          zeros.append(p.first);
      }
    zeros.for_each([this](size_t e)
    {
      coeffs.remove_key(e);
    });
  }

  /** @brief Read coefficient at exponent (0 if absent). */
  Coefficient coeff_at(size_t exp) const
  {
    auto *p = coeffs.search(exp);
    return p != nullptr ? p->second : Coefficient{};
  }

  /** @brief Pointer to stored coefficient, or null if absent. */
  Coefficient *coeff_ptr(size_t exp) noexcept
  {
    auto *p = coeffs.search(exp);
    return p != nullptr ? &p->second : nullptr;
  }

  /** @brief Pointer to stored coefficient, or null if absent. */
  const Coefficient *coeff_ptr(size_t exp) const noexcept
  {
    auto *p = coeffs.search(exp);
    return p != nullptr ? &p->second : nullptr;
  }

  /** @brief Add a delta to one coefficient, erasing the term if it cancels. */
  void add_to_coeff(size_t exp, const Coefficient &delta)
  {
    if (coeff_is_zero(delta))
      return;

    if (auto *slot = coeff_ptr(exp); slot != nullptr)
      {
        *slot = *slot + delta;
        if (coeff_is_zero(*slot))
          coeffs.remove_key(exp);
      }
    else
      coeffs.insert(exp, delta);
  }

  /** @brief Add @p scale times @p src shifted by @p shift to this polynomial. */
  void add_scaled_shifted(const Gen_Polynomial &src, const Coefficient &scale, size_t shift = 0)
  {
    if (coeff_is_zero(scale) or src.is_zero())
      return;

    src.for_each_term([this, &scale, shift](size_t exp, const Coefficient &c)
    {
      add_to_coeff(exp + shift, c * scale);
    });
  }

  /** @brief Multiply every coefficient in place by a scalar. */
  void scale_inplace(const Coefficient &s)
  {
    if (coeffs.is_empty())
      return;

    if (coeff_is_zero(s))
      {
        coeffs = DynMapTree<size_t, Coefficient>();
        return;
      }

    if (s == Coefficient(1))
      return;

    DynList<size_t> zeros;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        p.second = p.second * s;
        if (coeff_is_zero(p.second))
          zeros.append(p.first);
      }

    zeros.for_each([this](size_t exp)
    {
      coeffs.remove_key(exp);
    });
  }

  /** @brief Divide every coefficient in place by a scalar. */
  void divide_scalar_inplace(const Coefficient &s)
  {
    ah_domain_error_if(coeff_is_zero(s)) << "polynomial division by zero scalar";

    if (coeffs.is_empty())
      return;

    if (s == Coefficient(1))
      return;

    DynList<size_t> zeros;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        p.second = p.second / s;
        if (coeff_is_zero(p.second))
          zeros.append(p.first);
      }

    zeros.for_each([this](size_t exp)
    {
      coeffs.remove_key(exp);
    });
  }

  /** @brief Iterate over non-zero terms in descending exponent order. */
  template <class Op>
  void for_each_term_desc(Op &&op) const
  {
    Array<const Term *> desc_terms(coeffs.size());
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &p = it.get_curr();
        desc_terms.append(&p);
      }

    for (size_t i = desc_terms.size(); i > 0; --i)
      {
        const Term &term = *desc_terms(i - 1);
        op(term.first, term.second);
      }
  }

  /** @brief Specialized multiplication by a monic linear factor @f$x + c@f$. */
  [[nodiscard]] Gen_Polynomial multiply_by_monic_linear(const Coefficient &c) const
  {
    if (is_zero())
      return Gen_Polynomial();

    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.add_to_coeff(p.first, p.second * c);
        result.add_to_coeff(p.first + 1, p.second);
      }
    return result;
  }

public:
  // =================================================================
  //  Construction
  // =================================================================

  /** @brief Default constructor: the zero polynomial. */
  Gen_Polynomial() = default;

  /** @brief Constant polynomial @f$p(x) = c@f$.
   *  @param[in] c The constant value.
   */
  explicit Gen_Polynomial(const Coefficient &c)
  {
    if (not coeff_is_zero(c))
      coeffs[0] = c;
  }

  /** @brief Monomial @f$c \cdot x^e@f$.
   *  @param[in] c Coefficient.
   *  @param[in] exp Exponent.
   */
  Gen_Polynomial(const Coefficient &c, size_t exp)
  {
    if (not coeff_is_zero(c))
      coeffs[exp] = c;
  }

  /** @brief Dense construction from initializer list.
   *
   *  The list @f$\{a_0, a_1, \ldots, a_n\}@f$ yields
   *  @f$a_0 + a_1 x + \cdots + a_n x^n@f$.
   *
   *  @param[in] il Coefficients in ascending exponent order.
   */
  Gen_Polynomial(std::initializer_list<Coefficient> il)
  {
    size_t exp = 0;
    for (auto &c : il)
      {
        if (not coeff_is_zero(c))
          coeffs[exp] = c;
        ++exp;
      }
  }

  /** @brief Dense construction from DynList.
   *
   *  Same semantics as the initializer_list constructor.
   *
   *  @param[in] l Coefficients in ascending exponent order.
   */
  explicit Gen_Polynomial(const DynList<Coefficient> &l)
  {
    size_t exp = 0;
    l.for_each([this, &exp](const Coefficient &c)
    {
      if (not coeff_is_zero(c))
        coeffs[exp] = c;
      ++exp;
    });
  }

  /** @brief Sparse construction from a list of (exponent, coefficient) pairs.
   *
   *  Repeated exponents are accumulated, so the resulting polynomial
   *  is normalized even if the input list is not.
   *
   *  @param[in] term_list Pairs of (exponent, coefficient).
   */
  explicit Gen_Polynomial(const DynList<std::pair<size_t, Coefficient>> &term_list)
  {
    term_list.for_each([this](const Term &t)
    {
      if (not coeff_is_zero(t.second))
        add_to_coeff(t.first, t.second);
    });
  }

  // =================================================================
  //  Static factory methods
  // =================================================================

  /** @brief The zero polynomial. */
  [[nodiscard]] static Gen_Polynomial zero()
  {
    return Gen_Polynomial();
  }

  /** @brief The constant polynomial 1. */
  [[nodiscard]] static Gen_Polynomial one()
  {
    return Gen_Polynomial(Coefficient(1));
  }

  /** @brief The monomial @f$x^n@f$.
   *  @param[in] n Exponent.
   */
  [[nodiscard]] static Gen_Polynomial x_to(size_t n)
  {
    return Gen_Polynomial(Coefficient(1), n);
  }

  /** @brief Build polynomial from its roots: @f$\prod (x - r_i)@f$.
   *
   *  @param[in] roots List of roots.
   *  @return The monic polynomial whose roots are exactly @p roots
   *          (with multiplicity).
   *
   *  @note Uses a specialized linear-factor multiplication path to avoid
   *        the overhead of generic polynomial products.
   */
  [[nodiscard]] static Gen_Polynomial from_roots(const DynList<Coefficient> &roots)
  {
    Gen_Polynomial result(Coefficient(1));  // start with constant 1
    roots.for_each([&result](const Coefficient &r)
    {
      result = result.multiply_by_monic_linear(-r);  // (x - r)
    });
    return result;
  }

  /** @brief Polynomial interpolation through a set of points.
   *
   *  Given @f$n+1@f$ points @f$(x_i, y_i)@f$, returns the unique polynomial
   *  of degree at most @f$n@f$ passing through all of them.
   *
   *  @param[in] points List of (x, y) pairs.  The x-values must be distinct.
   *  @return The interpolating polynomial.
   *
   *  @throws std::domain_error if fewer than 1 point or duplicate x-values.
   *
   *  @note Implemented via Newton divided differences plus incremental
   *        linear-factor expansion, avoiding the large temporary basis
   *        polynomials of the naive Lagrange form.
   */
  [[nodiscard]] static Gen_Polynomial interpolate(
    const DynList<std::pair<Coefficient, Coefficient>> &points)
  {
    ah_domain_error_if(points.is_empty()) << "interpolate requires at least one point";

    const size_t n = points.size();
    Array<Coefficient> xs;
    Array<Coefficient> dd;
    xs.putn(n);
    dd.putn(n);

    size_t idx = 0;
    points.for_each([&](const std::pair<Coefficient, Coefficient> &pt)
    {
      xs[idx] = pt.first;
      dd[idx] = pt.second;
      ++idx;
    });

    for (size_t i = 0; i < n; ++i)
      for (size_t j = i + 1; j < n; ++j)
        ah_domain_error_if(coeff_is_zero(xs[i] - xs[j])) << "interpolate: duplicate x-values";

    for (size_t order = 1; order < n; ++order)
      for (size_t i = n - 1; i >= order; --i)
        {
          Coefficient denom = xs[i] - xs[i - order];
          dd[i] = (dd[i] - dd[i - 1]) / denom;
          if (i == order)
            break;
        }

    Gen_Polynomial result(dd[0]);
    Gen_Polynomial basis(Coefficient(1));
    for (size_t i = 1; i < n; ++i)
      {
        basis = basis.multiply_by_monic_linear(-xs[i - 1]);
        if (not coeff_is_zero(dd[i]))
          result.add_scaled_shifted(basis, dd[i]);
      }

    return result;
  }

  // =================================================================
  //  Properties
  // =================================================================

  /** @brief True if this is the zero polynomial. */
  [[nodiscard]] bool is_zero() const noexcept
  {
    return coeffs.is_empty();
  }

  /** @brief Degree of the polynomial.
   *
   *  Returns 0 for the zero polynomial.  Use `is_zero()` to distinguish
   *  the zero polynomial from a non-zero constant.
   */
  [[nodiscard]] size_t degree() const noexcept
  {
    if (coeffs.is_empty())
      return 0;
    return coeffs.max().first;
  }

  /** @brief Number of non-zero terms. */
  [[nodiscard]] size_t num_terms() const noexcept
  {
    return coeffs.size();
  }

  /** @brief True if constant or zero. */
  [[nodiscard]] bool is_constant() const noexcept
  {
    return coeffs.is_empty() or (coeffs.size() == 1 and coeffs.min().first == 0);
  }

  /** @brief True if exactly one non-zero term. */
  [[nodiscard]] bool is_monomial() const noexcept
  {
    return coeffs.size() == 1;
  }

  /** @brief True if leading coefficient equals 1. */
  [[nodiscard]] bool is_monic() const noexcept
  {
    return not coeffs.is_empty() and coeff_is_zero(coeffs.max().second - Coefficient(1));
  }

  // =================================================================
  //  Coefficient access
  // =================================================================

  /** @brief Coefficient at exponent @p exp (0 if absent). */
  [[nodiscard]] Coefficient operator[](size_t exp) const
  {
    return coeff_at(exp);
  }

  /** @brief Leading coefficient (of the highest-degree term).
   *  @return Coefficient{} if zero polynomial.
   */
  [[nodiscard]] Coefficient leading_coeff() const noexcept
  {
    if (coeffs.is_empty())
      return Coefficient{};
    return coeffs.max().second;
  }

  /** @brief True if there is a non-zero coefficient at @p exp. */
  [[nodiscard]] bool has_term(size_t exp) const noexcept
  {
    return coeffs.has(exp);
  }

  // =================================================================
  //  Evaluation
  // =================================================================

  /** @brief Evaluate @f$p(x)@f$ using gap-aware Horner's method.
   *
   *  Consecutive blocks of missing coefficients are collapsed into powers
   *  of @p x, so sparse high-degree polynomials do not pay a lookup per
   *  zero coefficient.
   *
   *  @param[in] x Evaluation point.
   *  @return @f$p(x)@f$.
   */
  [[nodiscard]] Coefficient horner_eval(const Coefficient &x) const
  {
    if (coeffs.is_empty())
      return Coefficient{};

    if (x == Coefficient{})
      return coeff_at(0);

    Coefficient result = Coefficient{};
    size_t prev_exp = 0;
    bool first = true;

    for_each_term_desc([&](size_t exp, const Coefficient &c)
    {
      if (first)
        {
          result = c;
          prev_exp = exp;
          first = false;
          return;
        }

      result = result * polynomial_detail::power(x, prev_exp - exp) + c;
      prev_exp = exp;
    });

    if (prev_exp > 0)
      result = result * polynomial_detail::power(x, prev_exp);

    return result;
  }

  /** @brief Evaluate @f$p(x)@f$ using sparse evaluation.
   *
   *  Each non-zero term @f$c \cdot x^e@f$ is computed via fast
   *  exponentiation.  Complexity: @f$O(k \log d)@f$ where @f$k@f$ is
   *  the number of terms and @f$d@f$ is the degree.
   *
   *  @param[in] x Evaluation point.
   *  @return @f$p(x)@f$.
   */
  [[nodiscard]] Coefficient sparse_eval(const Coefficient &x) const
  {
    Coefficient result = Coefficient{};
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result = result + p.second * polynomial_detail::power(x, p.first);
      }
    return result;
  }

  /** @brief Evaluate @f$p(x)@f$ with adaptive strategy.
   *
   *  Uses direct term evaluation for ultra-sparse polynomials and the
   *  gap-aware Horner path otherwise.
   *
   *  @param[in] x Evaluation point.
   *  @return @f$p(x)@f$.
   */
  [[nodiscard]] Coefficient eval(const Coefficient &x) const
  {
    if (coeffs.is_empty())
      return Coefficient{};

    if (x == Coefficient{})
      return coeff_at(0);

    if (num_terms() <= 2)
      return sparse_eval(x);

    return horner_eval(x);
  }

  /** @brief Syntactic sugar: @c p(x) calls @ref eval. */
  [[nodiscard]] Coefficient operator()(const Coefficient &x) const
  {
    return eval(x);
  }

  /** @brief Evaluate at multiple points.
   *
   *  @param[in] points List of evaluation points.
   *  @return List of @f$p(x_i)@f$ in corresponding order.
   *
   *  @note **Complexity**: @f$O(m \cdot T_{\text{eval}})@f$ where @f$m@f$
   *        is the number of points and @f$T_{\text{eval}}@f$ is the per-point
   *        evaluation cost (see @ref eval).
   */
  [[nodiscard]] DynList<Coefficient> multi_eval(const DynList<Coefficient> &points) const
  {
    DynList<Coefficient> results;
    points.for_each([this, &results](const Coefficient &x)
    {
      results.append(eval(x));
    });
    return results;
  }

  // =================================================================
  //  Arithmetic operators
  // =================================================================

  /** @brief Unary negation. */
  [[nodiscard]] Gen_Polynomial operator-() const
  {
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.coeffs[p.first] = -p.second;
      }
    return result;
  }

  /** @brief Polynomial addition. */
  [[nodiscard]] Gen_Polynomial operator+(const Gen_Polynomial &q) const
  {
    Gen_Polynomial result = *this;
    result += q;
    return result;
  }

  /** @brief In-place addition. */
  Gen_Polynomial &operator+=(const Gen_Polynomial &q)
  {
    if (this == &q)
      {
        scale_inplace(Coefficient(1) + Coefficient(1));
        return *this;
      }

    for (auto it = q.coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        add_to_coeff(p.first, p.second);
      }
    return *this;
  }

  /** @brief Add a scalar constant term. */
  [[nodiscard]] Gen_Polynomial operator+(const Coefficient &c) const
  {
    Gen_Polynomial result = *this;
    result += c;
    return result;
  }

  /** @brief In-place addition of a scalar constant term. */
  Gen_Polynomial &operator+=(const Coefficient &c)
  {
    add_to_coeff(0, c);
    return *this;
  }

  /** @brief Polynomial subtraction. */
  [[nodiscard]] Gen_Polynomial operator-(const Gen_Polynomial &q) const
  {
    Gen_Polynomial result = *this;
    result -= q;
    return result;
  }

  /** @brief In-place subtraction. */
  Gen_Polynomial &operator-=(const Gen_Polynomial &q)
  {
    if (this == &q)
      {
        coeffs = DynMapTree<size_t, Coefficient>();
        return *this;
      }

    for (auto it = q.coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        add_to_coeff(p.first, -p.second);
      }
    return *this;
  }

  /** @brief Subtract a scalar constant term. */
  [[nodiscard]] Gen_Polynomial operator-(const Coefficient &c) const
  {
    Gen_Polynomial result = *this;
    result -= c;
    return result;
  }

  /** @brief In-place subtraction of a scalar constant term. */
  Gen_Polynomial &operator-=(const Coefficient &c)
  {
    add_to_coeff(0, -c);
    return *this;
  }

  /** @brief Polynomial multiplication.
   *
   *  @note **Complexity**: @f$O(k_1 \cdot k_2 \cdot \log(k_1 k_2))@f$
   *        where @f$k_1, k_2@f$ are the numbers of terms.
   */
  [[nodiscard]] Gen_Polynomial operator*(const Gen_Polynomial &q) const
  {
    if (is_zero() or q.is_zero())
      return Gen_Polynomial();

    Gen_Polynomial result;
    const Gen_Polynomial *outer = this;
    const Gen_Polynomial *inner = &q;

    if (q.num_terms() < num_terms())
      {
        outer = &q;
        inner = this;
      }

    for (auto it1 = outer->coeffs.get_it(); it1.has_curr(); it1.next_ne())
      {
        auto &t1 = it1.get_curr();
        for (auto it2 = inner->coeffs.get_it(); it2.has_curr(); it2.next_ne())
          {
            auto &t2 = it2.get_curr();
            result.add_to_coeff(t1.first + t2.first, t1.second * t2.second);
          }
      }
    return result;
  }

  /** @brief In-place multiplication. */
  Gen_Polynomial &operator*=(const Gen_Polynomial &q)
  {
    *this = *this * q;
    return *this;
  }

  /** @brief Multiply by a scalar. */
  [[nodiscard]] Gen_Polynomial operator*(const Coefficient &s) const
  {
    if (coeff_is_zero(s))
      return Gen_Polynomial();
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        Coefficient prod = p.second * s;
        if (not coeff_is_zero(prod))
          result.coeffs[p.first] = prod;
      }
    return result;
  }

  /** @brief In-place scalar multiplication. */
  Gen_Polynomial &operator*=(const Coefficient &s)
  {
    scale_inplace(s);
    return *this;
  }

  /** @brief Divide by a scalar.
   *  @throws std::domain_error if @p s is zero.
   */
  [[nodiscard]] Gen_Polynomial operator/(const Coefficient &s) const
  {
    ah_domain_error_if(coeff_is_zero(s)) << "polynomial division by zero scalar";
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        Coefficient q = p.second / s;
        if (not coeff_is_zero(q))
          result.coeffs[p.first] = q;
      }
    return result;
  }

  /** @brief In-place scalar division. */
  Gen_Polynomial &operator/=(const Coefficient &s)
  {
    divide_scalar_inplace(s);
    return *this;
  }

  // =================================================================
  //  Polynomial division
  // =================================================================

  /** @brief Polynomial long division: returns (quotient, remainder).
   *
   *  Computes @f$q@f$ and @f$r@f$ such that
   *  @f$\text{this} = q \cdot d + r@f$ with
   *  @f$\deg(r) < \deg(d)@f$ or @f$r = 0@f$.
   *
   *  @param[in] d Divisor (must not be zero).
   *  @return Pair (quotient, remainder).
   *  @throws std::domain_error if @p d is zero.
   */
  [[nodiscard]] std::pair<Gen_Polynomial, Gen_Polynomial> divmod(const Gen_Polynomial &d) const
  {
    ah_domain_error_if(d.is_zero()) << "polynomial division by zero";

    if (is_zero() or degree() < d.degree())
      return {Gen_Polynomial(), *this};

    Gen_Polynomial q;          // quotient
    Gen_Polynomial r = *this;  // remainder
    const size_t d_degree = d.degree();
    const Coefficient d_lc = d.leading_coeff();

    while (not r.is_zero())
      {
        const size_t r_degree = r.degree();
        if (r_degree < d_degree)
          break;

        const Coefficient scale = r.leading_coeff() / d_lc;
        ah_domain_error_if(coeff_is_zero(scale))
          << "divmod: inexact coefficient division (leading_coeff=" << r.leading_coeff()
          << " / divisor_lc=" << d_lc << " = 0). "
          << "For integral/non-field coefficients, use pseudo_divmod() instead.";
        const size_t shift = r_degree - d_degree;

        q.add_to_coeff(shift, scale);
        r.add_scaled_shifted(d, -scale, shift);
      }

    return {q, r};
  }

  /** @brief Polynomial quotient. */
  [[nodiscard]] Gen_Polynomial operator/(const Gen_Polynomial &d) const
  {
    return divmod(d).first;
  }

  /** @brief Polynomial remainder. */
  [[nodiscard]] Gen_Polynomial operator%(const Gen_Polynomial &d) const
  {
    return divmod(d).second;
  }

  /** @brief In-place polynomial quotient. */
  Gen_Polynomial &operator/=(const Gen_Polynomial &d)
  {
    *this = divmod(d).first;
    return *this;
  }

  /** @brief In-place polynomial remainder. */
  Gen_Polynomial &operator%=(const Gen_Polynomial &d)
  {
    *this = divmod(d).second;
    return *this;
  }

  // =================================================================
  //  Calculus
  // =================================================================

  /** @brief Formal derivative @f$p'(x)@f$.
   *
   *  @f$\frac{d}{dx}\bigl[\sum c_i x^{e_i}\bigr]
   *     = \sum e_i \cdot c_i \cdot x^{e_i - 1}@f$.
   */
  [[nodiscard]] Gen_Polynomial derivative() const
  {
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (p.first == 0)
          continue;  // derivative of constant is 0
        Coefficient dc = p.second * Coefficient(p.first);
        if (not coeff_is_zero(dc))
          result.coeffs[p.first - 1] = dc;
      }
    return result;
  }

  /** @brief @f$n@f$-th derivative @f$p^{(n)}(x)@f$.
   *
   *  Applies the derivative operator @p n times.  Returns the zero
   *  polynomial if @f$n > \deg(p)@f$.
   *
   *  @param[in] n Differentiation order.
   *  @return The @p n-th derivative.
   *
   *  @note **Complexity**: @f$O(n \cdot k)@f$ where @f$k@f$ is the
   *        number of terms.
   */
  [[nodiscard]] Gen_Polynomial nth_derivative(size_t n) const
  {
    Gen_Polynomial result = *this;
    for (size_t i = 0; i < n and not result.is_zero(); ++i)
      result = result.derivative();
    return result;
  }

  /** @brief Formal indefinite integral with constant of integration.
   *
   *  @f$\int \sum c_i x^{e_i}\,dx
   *     = C + \sum \frac{c_i}{e_i + 1} x^{e_i + 1}@f$
   *
   *  @param[in] C Constant of integration (default 0).
   *
   *  @note For integer coefficient types, division truncates.  Use a
   *        rational type for exact results.
   */
  [[nodiscard]] Gen_Polynomial integral(const Coefficient &C = Coefficient{}) const
  {
    Gen_Polynomial result;
    if (not coeff_is_zero(C))
      result.coeffs[0] = C;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        Coefficient ic = p.second / Coefficient(p.first + 1);
        if (not coeff_is_zero(ic))
          result.coeffs[p.first + 1] = ic;
      }
    return result;
  }

  /** @brief Definite integral @f$\int_a^b p(x)\,dx@f$.
   *
   *  Computes the integral term by term without materializing the full
   *  antiderivative polynomial.
   *
   *  @param[in] a Lower bound.
   *  @param[in] b Upper bound.
   *  @return @f$\int_a^b p(x)\,dx@f$.
   */
  [[nodiscard]] Coefficient definite_integral(const Coefficient &a, const Coefficient &b) const
  {
    Coefficient result = Coefficient{};

    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        const size_t next_exp = p.first + 1;
        const Coefficient scale = p.second / Coefficient(next_exp);
        if (coeff_is_zero(scale))
          continue;

        result
          = result
          + scale * (polynomial_detail::power(b, next_exp) - polynomial_detail::power(a, next_exp));
      }

    return result;
  }

  // =================================================================
  //  Composition and power
  // =================================================================

  /** @brief Composition @f$p(q(x))@f$.
   *
   *  Evaluates this polynomial at @p q using a sparse-aware Horner scheme
   *  over non-zero coefficients, so large exponent gaps do not force
   *  repeated multiplication by @p q for zero coefficients.
   */
  [[nodiscard]] Gen_Polynomial compose(const Gen_Polynomial &q) const
  {
    if (is_zero())
      return Gen_Polynomial();

    if (q.is_constant())
      return Gen_Polynomial(eval(q[0]));

    Gen_Polynomial result;
    size_t prev_exp = 0;
    bool first = true;

    for_each_term_desc([&](size_t exp, const Coefficient &c)
    {
      if (first)
        {
          result = Gen_Polynomial(c);
          prev_exp = exp;
          first = false;
          return;
        }

      const size_t gap = prev_exp - exp;
      if (gap == 1)
        result *= q;
      else if (gap > 1)
        result *= q.pow(gap);

      result.add_to_coeff(0, c);
      prev_exp = exp;
    });

    if (prev_exp == 1)
      result *= q;
    else if (prev_exp > 1)
      result *= q.pow(prev_exp);

    return result;
  }

  /** @brief Exponentiation @f$p(x)^n@f$ by repeated squaring.
   *
   *  @param[in] n Exponent.
   *  @return @f$p(x)^n@f$ (1 if @p n == 0).
   */
  [[nodiscard]] Gen_Polynomial pow(size_t n) const
  {
    Gen_Polynomial result(Coefficient(1));
    Gen_Polynomial base = *this;
    while (n > 0)
      {
        if (n & 1)
          result *= base;
        base *= base;
        n >>= 1;
      }
    return result;
  }

  // =================================================================
  //  GCD
  // =================================================================

  /** @brief Greatest common divisor (Euclidean algorithm).
   *
   *  Returns a monic GCD (leading coefficient 1) for field-like
   *  coefficient types.  For integer coefficients, the result is
   *  normalized by dividing by its leading coefficient.
   *
   *  @note Only meaningful for coefficient types that form a field
   *        (rationals, floating-point).
   */
  [[nodiscard]] static Gen_Polynomial gcd(Gen_Polynomial a, Gen_Polynomial b)
  {
    while (not b.is_zero())
      {
        Gen_Polynomial r = a % b;
        a = b;
        b = r;
      }
    // Make monic
    if (not a.is_zero())
      a /= a.leading_coeff();
    return a;
  }

  /** @brief Extended GCD: computes g, s, t such that @f$sa + tb = g@f$.
   *
   *  Uses the extended Euclidean algorithm.  The result g is made monic.
   *
   *  @param[in] a First polynomial.
   *  @param[in] b Second polynomial.
   *  @return Tuple (g, s, t) with @f$sa + tb = \gcd(a, b)@f$.
   */
  /** @brief Result of the extended GCD algorithm. */
  struct Xgcd_Result
  {
    Gen_Polynomial g;  ///< Greatest common divisor (monic).
    Gen_Polynomial s;  ///< Bezout coefficient for the first argument.
    Gen_Polynomial t;  ///< Bezout coefficient for the second argument.
  };

  [[nodiscard]] static Xgcd_Result xgcd(Gen_Polynomial a, Gen_Polynomial b)
  {
    Gen_Polynomial s0(Coefficient(1)), s1;
    Gen_Polynomial t0, t1(Coefficient(1));

    while (not b.is_zero())
      {
        auto [q, r] = a.divmod(b);
        a = b;
        b = r;
        Gen_Polynomial s2 = s0 - q * s1;
        Gen_Polynomial t2 = t0 - q * t1;
        s0 = s1;
        s1 = s2;
        t0 = t1;
        t1 = t2;
      }

    if (not a.is_zero())
      {
        Coefficient lc = a.leading_coeff();
        a /= lc;
        s0 /= lc;
        t0 /= lc;
      }

    return {a, s0, t0};
  }

  /** @brief Least common multiple.
   *
   *  @f$\text{lcm}(a, b) = \frac{a \cdot b}{\gcd(a, b)}@f$.
   *
   *  @param[in] a First polynomial.
   *  @param[in] b Second polynomial.
   *  @return The LCM, or the zero polynomial if either input is zero.
   */
  [[nodiscard]] static Gen_Polynomial lcm(const Gen_Polynomial &a, const Gen_Polynomial &b)
  {
    if (a.is_zero() or b.is_zero())
      return Gen_Polynomial();
    Gen_Polynomial g = gcd(a, b);
    return a / g * b;
  }

  /** @brief Pseudo-division for non-field coefficient types.
   *
   *  Computes q, r such that
   *  @f$\text{lc}(d)^{\deg(a)-\deg(d)+1} \cdot a = q \cdot d + r@f$.
   *  No division of coefficients is performed, so this works
   *  correctly over integer rings.
   *
   *  @param[in] d Divisor (must not be zero).
   *  @return Pair (quotient, remainder).
   *  @throws std::domain_error if @p d is zero.
   */
  [[nodiscard]] std::pair<Gen_Polynomial, Gen_Polynomial> pseudo_divmod(const Gen_Polynomial &d) const
  {
    ah_domain_error_if(d.is_zero()) << "pseudo-division by zero polynomial";

    if (is_zero() or degree() < d.degree())
      return {Gen_Polynomial(), *this};

    const Coefficient lc_d = d.leading_coeff();
    const size_t d_degree = d.degree();
    size_t delta = degree() - d_degree + 1;

    Gen_Polynomial q;
    Gen_Polynomial r = *this;

    while (not r.is_zero())
      {
        const size_t r_degree = r.degree();
        if (r_degree < d_degree)
          break;

        const Coefficient lc_r = r.leading_coeff();
        const size_t exp = r_degree - d_degree;

        // Multiply q and r by lc(d) to avoid division
        q.scale_inplace(lc_d);
        r.scale_inplace(lc_d);
        --delta;

        q.add_to_coeff(exp, lc_r);
        r.add_scaled_shifted(d, -lc_r, exp);
      }

    // Multiply by remaining powers of lc(d)
    if (delta > 0)
      {
        const Coefficient scale = polynomial_detail::power(lc_d, delta);
        q.scale_inplace(scale);
        r.scale_inplace(scale);
      }

    return {q, r};
  }

  // =================================================================
  //  Algebraic transformations
  // =================================================================

  /** @brief Make monic: divide by leading coefficient.
   *
   *  @return Monic version of this polynomial.
   *  @throws std::domain_error if zero polynomial.
   */
  [[nodiscard]] Gen_Polynomial to_monic() const
  {
    ah_domain_error_if(is_zero()) << "cannot make zero polynomial monic";
    return *this / leading_coeff();
  }

  /** @brief Reciprocal polynomial: @f$x^d \cdot p(1/x)@f$.
   *
   *  Reverses the coefficient sequence.  If @f$p(x) = \sum c_i x^i@f$,
   *  the reciprocal is @f$\sum c_i x^{d-i}@f$ where @f$d = \deg(p)@f$.
   */
  [[nodiscard]] Gen_Polynomial reverse() const
  {
    if (is_zero())
      return Gen_Polynomial();
    size_t d = degree();
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.coeffs[d - p.first] = p.second;
      }
    return result;
  }

  /** @brief Argument negation: @f$p(-x)@f$.
   *
   *  Negates coefficients of odd-degree terms.
   */
  [[nodiscard]] Gen_Polynomial negate_arg() const
  {
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.coeffs[p.first] = (p.first % 2 == 0) ? p.second : -p.second;
      }
    return result;
  }

  /** @brief Taylor shift: @f$p(x + k)@f$.
   *
   *  Computed via composition with the linear polynomial @f$x + k@f$.
   *
   *  @param[in] k Shift amount.
   *  @return The polynomial @f$p(x + k)@f$.
   *
   *  @note Delegates to @ref compose and therefore benefits from the
   *        sparse-aware composition path.
   */
  [[nodiscard]] Gen_Polynomial shift(const Coefficient &k) const
  {
    return compose(Gen_Polynomial({k, Coefficient(1)}));
  }

  /** @brief Multiply by @f$x^k@f$ (shift exponents up).
   *
   *  @param[in] k Number of positions to shift up.
   *  @return @f$x^k \cdot p(x)@f$.
   */
  [[nodiscard]] Gen_Polynomial shift_up(size_t k) const
  {
    if (k == 0)
      return *this;
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.coeffs[p.first + k] = p.second;
      }
    return result;
  }

  /** @brief Divide by @f$x^k@f$ (shift exponents down).
   *
   *  Terms with exponent less than @p k are discarded (floor division).
   *
   *  @param[in] k Number of positions to shift down.
   *  @return @f$\lfloor p(x) / x^k \rfloor@f$.
   */
  [[nodiscard]] Gen_Polynomial shift_down(size_t k) const
  {
    if (k == 0)
      return *this;
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (p.first >= k)
          result.coeffs[p.first - k] = p.second;
      }
    return result;
  }

  /** @brief Truncate to degree less than @p n.
   *
   *  Keeps only terms with exponent strictly less than @p n,
   *  i.e., computes @f$p(x) \bmod x^n@f$.
   *
   *  @param[in] n Truncation order.
   */
  [[nodiscard]] Gen_Polynomial truncate(size_t n) const
  {
    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (p.first < n)
          result.coeffs[p.first] = p.second;
      }
    return result;
  }

  /** @brief Dense coefficient vector.
   *
   *  Returns an Array of size @f$d + 1@f$ (or empty for zero polynomial)
   *  containing all coefficients in ascending exponent order,
   *  zero-filled for absent terms.
   */
  [[nodiscard]] Array<Coefficient> to_dense() const
  {
    if (is_zero())
      return Array<Coefficient>();
    size_t d = degree();
    Array<Coefficient> result(d + 1, Coefficient{});
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result(p.first) = p.second;
      }
    return result;
  }

  /** @brief Square-free part: @f$p / \gcd(p, p')@f$.
   *
   *  Removes repeated root factors.  For example, if
   *  @f$p(x) = (x-1)^3 (x-2)@f$, the square-free part is
   *  @f$(x-1)(x-2)@f$.
   *
   *  @return A polynomial with no repeated roots.
   *
   *  @note Requires coefficient type to form a field.
   *  @note **Complexity**: dominated by the GCD computation,
   *        @f$O(d^2)@f$ polynomial divisions.
   */
  [[nodiscard]] Gen_Polynomial square_free() const
  {
    if (is_zero() or is_constant())
      return *this;
    Gen_Polynomial g = gcd(*this, derivative());
    return *this / g;
  }

  // =================================================================
  //  Root analysis (floating-point coefficients)
  // =================================================================

  /** @brief Cauchy upper bound on absolute value of roots.
   *
   *  For @f$p(x) = a_d x^d + \ldots + a_0@f$, all roots @f$|r|@f$
   *  satisfy @f$|r| \le 1 + \max_i |a_i / a_d|@f$.
   *
   *  @return Upper bound on @f$\max |r_i|@f$.
   *  @throws std::domain_error if zero polynomial.
   */
  [[nodiscard]] Coefficient cauchy_bound() const
  {
    ah_domain_error_if(is_zero()) << "cauchy_bound: zero polynomial has no roots";
    if (is_constant())
      return Coefficient{};

    const size_t d = degree();
    const Coefficient lc = leading_coeff();
    Coefficient max_ratio = Coefficient{};
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (p.first == d)
          continue;
        Coefficient ratio;
        if constexpr (std::is_floating_point_v<Coefficient>)
          ratio = std::abs(p.second / lc);
        else
          ratio = (p.second < Coefficient{} ? -p.second : p.second) / (lc < Coefficient{} ? -lc : lc);
        if (ratio > max_ratio)
          max_ratio = ratio;
      }
    return Coefficient(1) + max_ratio;
  }

  /** @brief Count sign changes in the coefficient sequence.
   *
   *  By Descartes' rule, the number of positive real roots (with
   *  multiplicity) is at most the number of sign changes and has
   *  the same parity.
   *
   *  @return Number of sign changes.
   */
  [[nodiscard]] size_t sign_variations() const
  {
    if (coeffs.size() < 2)
      return 0;

    size_t changes = 0;
    bool have_prev = false;
    bool prev_positive = false;

    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        bool positive;
        if constexpr (std::is_floating_point_v<Coefficient>)
          positive = p.second > epsilon();
        else
          positive = p.second > Coefficient{};

        bool is_neg;
        if constexpr (std::is_floating_point_v<Coefficient>)
          is_neg = p.second < -epsilon();
        else
          is_neg = p.second < Coefficient{};

        if (not positive and not is_neg)
          continue;  // skip zero coefficients

        if (have_prev and positive != prev_positive)
          ++changes;
        prev_positive = positive;
        have_prev = true;
      }
    return changes;
  }

  /** @brief Sturm sequence of this polynomial.
   *
   *  The Sturm chain @f$p_0 = p, p_1 = p', p_{i+1} = -\text{rem}(p_{i-1}, p_i)@f$
   *  is used to count distinct real roots in an interval.
   *
   *  @return DynList of polynomials forming the Sturm chain.
   *
   *  @note **Complexity**: @f$O(d^2)@f$ where @f$d = \deg(p)@f$,
   *        dominated by the sequence of polynomial remainders.
   */
  [[nodiscard]] DynList<Gen_Polynomial> sturm_chain() const
  {
    DynList<Gen_Polynomial> chain;
    chain.append(*this);
    if (is_zero() or is_constant())
      return chain;

    Gen_Polynomial p0 = *this;
    Gen_Polynomial p1 = derivative();
    chain.append(p1);

    while (not p1.is_zero())
      {
        Gen_Polynomial r = -(p0 % p1);
        if (r.is_zero())
          break;
        chain.append(r);
        p0 = p1;
        p1 = r;
      }

    return chain;
  }

  /** @brief Count sign changes in a Sturm chain at a given point.
   *
   *  @param[in] chain Sturm chain (from sturm_chain()).
   *  @param[in] x Evaluation point.
   *  @return Number of sign changes in the sequence @f$p_0(x), p_1(x), \ldots@f$.
   */
  [[nodiscard]] static size_t sturm_sign_changes(const DynList<Gen_Polynomial> &chain,
                                                 const Coefficient &x)
  {
    size_t changes = 0;
    bool have_prev = false;
    bool prev_positive = false;

    chain.for_each([&](const Gen_Polynomial &p)
    {
      Coefficient val = p(x);
      if (coeff_is_zero(val))
        return;  // skip zeros in Sturm evaluation

      bool positive = val > Coefficient{};
      if (have_prev and positive != prev_positive)
        ++changes;
      prev_positive = positive;
      have_prev = true;
    });

    return changes;
  }

  /** @brief Count distinct real roots in @f$[a, b]@f$ via Sturm's theorem.
   *
   *  @param[in] a Lower bound of interval.
   *  @param[in] b Upper bound of interval.
   *  @return Number of distinct real roots in @f$[a, b]@f$.
   *
   *  @note Only meaningful for floating-point coefficient types.
   *  If @p a > @p b, the endpoints are swapped.
   *
   *  @note **Complexity**: @f$O(d^2)@f$ to build the Sturm chain, plus
   *        @f$O(d^2)@f$ for evaluating it at both endpoints.
   */
  [[nodiscard]] size_t count_real_roots(const Coefficient &a, const Coefficient &b) const
  {
    if (b < a)
      return count_real_roots(b, a);

    auto chain = sturm_chain();
    size_t sa = sturm_sign_changes(chain, a);
    size_t sb = sturm_sign_changes(chain, b);
    size_t count = sa > sb ? sa - sb : 0;

    // Sturm's theorem counts roots in (a, b]; add root at a if present
    if (coeff_is_zero((*this)(a)))
      ++count;

    return count;
  }

  /** @brief Total number of distinct real roots.
   *
   *  Counts all distinct real roots using Sturm's theorem with
   *  the Cauchy bound.
   */
  [[nodiscard]] size_t count_all_real_roots() const
  {
    if (is_zero() or is_constant())
      return 0;
    Coefficient bound = cauchy_bound();
    return count_real_roots(-bound, bound);
  }

  /** @brief Find a root by bisection in @f$[a, b]@f$.
   *
   *  Requires @f$p(a)@f$ and @f$p(b)@f$ to have opposite signs.
   *
   *  @param[in] a Lower bound (must satisfy @f$p(a) \cdot p(b) < 0@f$).
   *  @param[in] b Upper bound.
   *  @param[in] tol Convergence tolerance (default @f$10^{-12}@f$).
   *  @param[in] max_iter Maximum iterations (default 200).
   *  @return Approximate root.
   *  @throws std::domain_error if same sign at endpoints.
   *
   *  If @p a > @p b, the endpoints are swapped.
   */
  [[nodiscard]] Coefficient bisect_root(Coefficient a,
                                        Coefficient b,
                                        Coefficient tol = Coefficient(1e-12),
                                        size_t max_iter = 200) const
  {
    if (b < a)
      std::swap(a, b);

    Coefficient fa = eval(a), fb = eval(b);
    if (coeff_is_zero(fa))
      return a;
    if (coeff_is_zero(fb))
      return b;

    ah_domain_error_if((fa > Coefficient{}) == (fb > Coefficient{}) and not coeff_is_zero(fa)
                       and not coeff_is_zero(fb))
      << "bisect_root: f(a) and f(b) must have opposite signs";

    for (size_t i = 0; i < max_iter; ++i)
      {
        Coefficient mid = (a + b) / Coefficient(2);
        if (b - a < tol)
          return mid;
        Coefficient fm = eval(mid);
        if (coeff_is_zero(fm))
          return mid;
        if (fa > Coefficient{} == fm > Coefficient{})
          {
            a = mid;
            fa = fm;
          }
        else
          {
            b = mid;
            fb = fm;
          }
      }
    return (a + b) / Coefficient(2);
  }

  /** @brief Find a root by Newton-Raphson iteration.
   *
   *  @param[in] x0 Initial guess.
   *  @param[in] tol Convergence tolerance (default @f$10^{-12}@f$).
   *  @param[in] max_iter Maximum iterations (default 100).
   *  @return Approximate root.
   *  @throws std::domain_error if derivative vanishes during iteration.
   *  @note Only available for floating-point coefficient types.
   */
  [[nodiscard]] Coefficient newton_root(Coefficient x0,
                                        Coefficient tol = Coefficient(1e-12),
                                        size_t max_iter = 100) const
    requires std::is_floating_point_v<Coefficient>
  {
    Gen_Polynomial dp = derivative();

    for (size_t i = 0; i < max_iter; ++i)
      {
        Coefficient fx = eval(x0);
        if (coeff_is_zero(fx))
          return x0;
        Coefficient dfx = dp.eval(x0);
        ah_domain_error_if(coeff_is_zero(dfx)) << "newton_root: derivative is zero at x = " << x0;
        Coefficient x1 = x0 - fx / dfx;
        Coefficient delta = x1 - x0;
        if constexpr (std::is_floating_point_v<Coefficient>)
          {
            if (std::abs(delta) < tol)
              return x1;
          }
        else
          {
            if (delta == Coefficient{})
              return x1;
          }
        x0 = x1;
      }
    return x0;
  }

  // =================================================================
  //  Comparison
  // =================================================================

  /** @brief Equality comparison.
   *
   *  Two polynomials are equal iff they have the same non-zero terms.
   *  For floating-point types, coefficients are compared within epsilon.
   */
  [[nodiscard]] bool operator==(const Gen_Polynomial &q) const noexcept
  {
    if (this == &q)
      return true;

    auto it1 = coeffs.get_it();
    auto it2 = q.coeffs.get_it();

    while (it1.has_curr() and it2.has_curr())
      {
        auto &lhs = it1.get_curr();
        auto &rhs = it2.get_curr();

        if (lhs.first == rhs.first)
          {
            if (not coeff_is_zero(lhs.second - rhs.second))
              return false;
            it1.next_ne();
            it2.next_ne();
            continue;
          }

        if (lhs.first < rhs.first)
          {
            if (not coeff_is_zero(lhs.second))
              return false;
            it1.next_ne();
            continue;
          }

        if (not coeff_is_zero(rhs.second))
          return false;
        it2.next_ne();
      }

    while (it1.has_curr())
      {
        if (not coeff_is_zero(it1.get_curr().second))
          return false;
        it1.next_ne();
      }

    while (it2.has_curr())
      {
        if (not coeff_is_zero(it2.get_curr().second))
          return false;
        it2.next_ne();
      }

    return true;
  }

  /** @brief Inequality comparison. */
  [[nodiscard]] bool operator!=(const Gen_Polynomial &q) const noexcept
  {
    return not(*this == q);
  }

  // =================================================================
  //  Iteration
  // =================================================================

  /** @brief Iterate over non-zero terms in ascending exponent order.
   *
   *  The callable @p op receives `(size_t exponent, const Coefficient &)`.
   */
  template <class Op>
  void for_each_term(Op &&op) const
  {
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        op(p.first, p.second);
      }
  }

  /** @brief All non-zero terms as a sorted list of (exponent, coefficient). */
  [[nodiscard]] DynList<std::pair<size_t, Coefficient>> terms_list() const
  {
    DynList<std::pair<size_t, Coefficient>> result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.append({p.first, p.second});
      }
    return result;
  }

  /** @brief All exponents with non-zero coefficients (sorted ascending). */
  [[nodiscard]] DynList<size_t> exponents() const
  {
    return coeffs.keys();
  }

  /** @brief All non-zero coefficients in ascending exponent order. */
  [[nodiscard]] DynList<Coefficient> coefficients() const
  {
    return coeffs.values();
  }

  // =================================================================
  //  String representation
  // =================================================================

  /** @brief Human-readable string representation.
   *
   *  Uses descending exponent order with mathematical sign conventions:
   *  `"3x^2 + 2x - 1"`, `"x^3 - x"`, `"5"`, `"0"`.
   *
   *  @param[in] var Variable name (default `"x"`).
   */
  [[nodiscard]] std::string to_str(const std::string &var = "x") const
  {
    if (coeffs.is_empty())
      return "0";

    std::ostringstream oss;
    bool first = true;

    for_each_term_desc([&](size_t exp, const Coefficient &c)
    {
      bool negative = false;
      if constexpr (requires { c < Coefficient{}; })
        negative = c < Coefficient{};

      Coefficient ac = negative ? -c : c;

      // Sign
      if (first)
        {
          if (negative)
            oss << "-";
        }
      else
        oss << (negative ? " - " : " + ");

      // Coefficient and variable
      if (exp == 0)
        oss << ac;
      else
        {
          if (not(ac == Coefficient(1)))
            oss << ac;
          oss << var;
          if (exp > 1)
            oss << "^" << exp;
        }

      first = false;
    });

    return oss.str();
  }

  // =================================================================
  //  Layer 5 — Exact Univariate Factorization
  // =================================================================

  /** @brief Square-free decomposition term: factor and multiplicity.
   *
   *  Result of Yun's algorithm: each term represents a factor with its
   *  multiplicity (how many times it divides the original polynomial).
   */
  struct SfdTerm
  {
    Gen_Polynomial factor;
    size_t multiplicity;
  };

  /** @brief Coefficient accessor (read-only at exponent).
   *
   *  Returns the coefficient at @p exp, or Coefficient{} (zero) if absent.
   *  This is equivalent to `operator[]`.
   */
  [[nodiscard]] Coefficient get_coeff(size_t exp) const noexcept
  {
    return coeff_at(exp);
  }

  /** @brief Set coefficient at exponent; removes entry if zero.
   *
   *  @param[in] exp Exponent of the term.
   *  @param[in] c The coefficient value.
   */
  void set_coeff(size_t exp, const Coefficient &c)
  {
    if (auto *slot = coeff_ptr(exp); slot != nullptr)
      {
        if (coeff_is_zero(c))
          coeffs.remove_key(exp);
        else
          *slot = c;
      }
    else if (not coeff_is_zero(c))
      coeffs.insert(exp, c);
  }

  /** @brief Content: GCD of all coefficients.
   *
   *  For an integer polynomial @f$p(x) = a_d x^d + \ldots + a_0@f$,
   *  the content is @f$\gcd(a_d, \ldots, a_0)@f$.
   *
   *  For the zero polynomial, returns zero.
   *  For a non-zero polynomial, the sign follows the leading coefficient.
   *
   *  @note Only available for integral coefficient types.
   *  @return The GCD of all coefficients.
   */
  [[nodiscard]] Coefficient content() const
    requires std::is_integral_v<Coefficient>
  {
    if (coeffs.is_empty())
      return Coefficient{};

    Coefficient result = Coefficient{};
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result = std::gcd(result, p.second);
      }

    // Ensure sign follows leading coefficient
    const Coefficient lc = leading_coeff();
    if (lc < Coefficient{} and result > Coefficient{})
      result = -result;

    return result;
  }

  /** @brief Primitive part: polynomial divided by its content.
   *
   *  Returns @f$p / \text{content}(p)@f$. For a non-zero polynomial,
   *  the result has leading coefficient positive (in absolute value).
   *
   *  @note Only available for integral coefficient types.
   *  @return A polynomial with gcd of coefficients = 1 (or zero poly).
   */
  [[nodiscard]] Gen_Polynomial primitive_part() const
    requires std::is_integral_v<Coefficient>
  {
    if (is_zero())
      return Gen_Polynomial();

    Coefficient c = content();
    if (c == Coefficient{} or c == Coefficient(1) or c == Coefficient(-1))
      {
        // Already primitive; ensure leading coefficient is positive
        Gen_Polynomial result = *this;
        if (leading_coeff() < Coefficient{})
          result.scale_inplace(Coefficient(-1));
        return result;
      }

    Gen_Polynomial result;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        result.coeffs.insert(p.first, p.second / c);
      }

    // Ensure leading coefficient is positive
    if (result.leading_coeff() < Coefficient{})
      result.scale_inplace(Coefficient(-1));

    return result;
  }

  /** @brief Polynomial GCD over integers (Euclidean algorithm with primitive parts).
   *
   *  Computes @f$\gcd(a, b)@f$ for integer polynomials using the
   *  Euclidean algorithm, normalizing by primitive parts at each step.
   *
   *  @param[in] a First polynomial.
   *  @param[in] b Second polynomial.
   *  @return The GCD polynomial (monic up to units).
   *
   *  @note Only available for integral coefficient types.
   *  @note **Complexity**: @f$O(d^2)@f$ polynomial divisions.
   */
  static Gen_Polynomial integer_gcd(Gen_Polynomial a, Gen_Polynomial b)
    requires std::is_integral_v<Coefficient>
  {
    if (a.is_zero())
      return b.primitive_part();
    if (b.is_zero())
      return a.primitive_part();

    a = a.primitive_part();
    b = b.primitive_part();

    while (not b.is_zero())
      {
        auto [quo, rem] = a.pseudo_divmod(b);
        a = b;
        b = rem.is_zero() ? rem : rem.primitive_part();
      }

    return a.primitive_part();
  }

  /** @brief Yun's square-free decomposition (SFD).
   *
   *  Decomposes a polynomial into square-free factors with multiplicities.
   *  Each factor has no repeated roots, and the union of all factors
   *  (with multiplicity) reconstructs the original.
   *
   *  Uses Yun's algorithm:
   *  1. Compute @f$f' = \text{derivative}(f)@f$.
   *  2. Iteratively remove common factors to extract multiplicities.
   *
   *  @return A list of SfdTerm, each with a square-free factor and its
   *          multiplicity. Empty list if polynomial is constant.
   *
   *  @note **Complexity**: @f$O(d^2)@f$ GCD operations.
   */
  [[nodiscard]] DynList<SfdTerm> yun_sfd() const
    requires std::is_integral_v<Coefficient>
  {
    DynList<SfdTerm> result;

    if (is_constant())
      return result;

    Gen_Polynomial f = primitive_part();
    Gen_Polynomial fp = f.derivative();

    Gen_Polynomial g = integer_gcd(f, fp);
    Gen_Polynomial h = _integer_exact_quot(f, g);

    size_t multiplicity = 1;

    while (not h.is_constant())
      {
        Gen_Polynomial h_prev = h;
        h = integer_gcd(g, h);
        Gen_Polynomial w = _integer_exact_quot(h_prev, h);

        if (not w.is_constant())
          result.append(SfdTerm{w.primitive_part(), multiplicity});

        g = _integer_exact_quot(g, h);
        multiplicity++;
      }

    // Final factor
    if (not g.is_constant())
      result.append(SfdTerm{g.primitive_part(), multiplicity});

    return result;
  }

  /** @brief Integer-safe exact quotient using pseudo-division.
   *
   *  Computes @f$a / b@f$ for integer polynomials where b is known to
   *  divide a.  Uses pseudo-division and normalizes the result.
   *
   *  @param[in] a Dividend.
   *  @param[in] b Divisor (must divide a exactly).
   *  @return The primitive part of the quotient.
   */
  static Gen_Polynomial _integer_exact_quot(const Gen_Polynomial &a, const Gen_Polynomial &b)
    requires std::is_integral_v<Coefficient>
  {
    if (b.is_zero())
      return Gen_Polynomial();
    if (b.is_constant())
      {
        Coefficient d = b.leading_coeff();
        Gen_Polynomial r;
        for (auto it = a.coeffs.get_it(); it.has_curr(); it.next_ne())
          {
            auto &p = it.get_curr();
            r.coeffs.insert(p.first, p.second / d);
          }
        return r;
      }
    auto [q, rem] = a.pseudo_divmod(b);
    return q.is_zero() ? q : q.primitive_part();
  }

  [[nodiscard]] static Gen_Polynomial reduce_coeffs_mod(const Gen_Polynomial &f, Coefficient mod)
    requires std::is_integral_v<Coefficient>
  {
    Gen_Polynomial reduced;
    f.for_each_term([&reduced, mod](size_t exp, const Coefficient &coeff)
    {
      Coefficient c = coeff % mod;
      if (c < Coefficient{})
        c = c + mod;
      if (c != Coefficient{})
        reduced.set_coeff(exp, c);
    });
    return reduced;
  }

  [[nodiscard]] static uint64_t eval_mod_u64(const Gen_Polynomial &f, uint64_t x, uint64_t mod)
    requires std::is_integral_v<Coefficient>
  {
    if (mod == 0 or mod == 1)
      return 0;

    uint64_t result = 0;
    f.for_each_term([&result, x, mod](size_t exp, const Coefficient &coeff)
    {
      const uint64_t c = polynomial_detail::normalize_mod_u64(coeff, mod);
      const uint64_t xpow = mod_exp(x, exp, mod);
      const uint64_t term = mod_mul(c, xpow, mod);
      result = (result + term) % mod;
    });
    return result;
  }

  [[nodiscard]] static Gen_Polynomial divide_by_monic_linear_mod(const Gen_Polynomial &f,
                                                                 Coefficient root,
                                                                 Coefficient mod)
    requires std::is_integral_v<Coefficient>
  {
    ah_domain_error_if(f.degree() == 0)
      << "divide_by_monic_linear_mod: constant polynomial is not divisible by x - r";

    const size_t degree = f.degree();
    Array<Coefficient> synthetic(degree + 1, Coefficient{});

    synthetic(degree) = ((f.get_coeff(degree) % mod) + mod) % mod;

    for (size_t idx = degree; idx-- > 0;)
      {
        Coefficient term = (f.get_coeff(idx) % mod + mod) % mod;
        Coefficient next = (term + (synthetic(idx + 1) * root) % mod) % mod;
        synthetic(idx) = next;
      }

    ah_domain_error_if(synthetic(0) != Coefficient{})
      << "divide_by_monic_linear_mod: x - r does not divide polynomial modulo p";

    Gen_Polynomial quotient;
    for (size_t exp = 0; exp < degree; ++exp)
      if (synthetic(exp + 1) != Coefficient{})
        quotient.set_coeff(exp, synthetic(exp + 1));

    return quotient;
  }

  [[nodiscard]] static DynList<Coefficient> positive_divisors(Coefficient value)
    requires std::is_integral_v<Coefficient>
  {
    DynList<Coefficient> result;

    const uint64_t abs_value = polynomial_detail::abs_to_u64(value);
    if (abs_value == 0)
      return result;

    for (uint64_t d = 1; d * d <= abs_value; ++d)
      if (abs_value % d == 0)
        {
          result.append(static_cast<Coefficient>(d));
          const uint64_t other = abs_value / d;
          if (other != d)
            result.append(static_cast<Coefficient>(other));
        }

    return result;
  }

  /** @brief Try exact division but treat inexact integral division as a rejected candidate.
   *
   *  Exhaustive integer factor searches intentionally test many polynomials
   *  that are not actual divisors. After hardening @c divmod() to throw on
   *  inexact leading-coefficient division, these search paths must convert that
   *  condition back into a simple "candidate does not divide" result.
   */
  [[nodiscard]] static bool try_exact_candidate_division(const Gen_Polynomial &f,
                                                         const Gen_Polynomial &candidate,
                                                         Gen_Polynomial &quotient)
    requires std::is_integral_v<Coefficient>
  {
    try
      {
        auto [q, r] = f.divmod(candidate);
        if (not r.is_zero())
          return false;
        quotient = std::move(q);
        return true;
      }
    catch (const std::domain_error &)
      {
        return false;
      }
  }

  [[nodiscard]] static bool try_divide_by_linear_factor(const Gen_Polynomial &f,
                                                        Coefficient a,
                                                        Coefficient b,
                                                        Gen_Polynomial &quotient)
    requires std::is_integral_v<Coefficient>
  {
    if (f.is_zero() or f.degree() == 0)
      return false;

    if (a == Coefficient{})
      return false;

    const size_t degree = f.degree();
    Array<Coefficient> q_coeffs(degree, Coefficient{});

    const Coefficient leading = f.get_coeff(degree);
    if (leading % a != Coefficient{})
      return false;

    q_coeffs(degree - 1) = leading / a;

    for (size_t k = degree - 1; k > 0; --k)
      {
        const Coefficient numerator = f.get_coeff(k) - b * q_coeffs(k);
        if (numerator % a != Coefficient{})
          return false;
        q_coeffs(k - 1) = numerator / a;
      }

    if (f.get_coeff(0) - b * q_coeffs(0) != Coefficient{})
      return false;

    quotient = Gen_Polynomial();
    for (size_t exp = 0; exp < degree; ++exp)
      if (q_coeffs(exp) != Coefficient{})
        quotient.set_coeff(exp, q_coeffs(exp));

    return true;
  }

  [[nodiscard]] static bool try_extract_rational_linear_factor(const Gen_Polynomial &f,
                                                               Gen_Polynomial &factor,
                                                               Gen_Polynomial &quotient)
    requires std::is_integral_v<Coefficient>
  {
    if (f.is_zero() or f.degree() == 0)
      return false;

    if (f.get_coeff(0) == Coefficient{})
      {
        if (try_divide_by_linear_factor(f, Coefficient(1), Coefficient{}, quotient))
          {
            factor = Gen_Polynomial(Coefficient(1), 1);
            return true;
          }
        return false;
      }

    DynList<Coefficient> numerator_divs = positive_divisors(f.get_coeff(0));
    DynList<Coefficient> denominator_divs = positive_divisors(f.leading_coeff());

    for (auto den_it = denominator_divs.get_it(); den_it.has_curr(); den_it.next_ne())
      {
        const Coefficient den = den_it.get_curr();

        for (auto num_it = numerator_divs.get_it(); num_it.has_curr(); num_it.next_ne())
          {
            const Coefficient num_abs = num_it.get_curr();

            for (Coefficient sign : {Coefficient(1), Coefficient(-1)})
              {
                const Coefficient num = sign * num_abs;
                if (std::gcd(polynomial_detail::abs_to_u64(num), polynomial_detail::abs_to_u64(den))
                    != 1)
                  continue;

                if (try_divide_by_linear_factor(f, den, -num, quotient))
                  {
                    factor = Gen_Polynomial();
                    factor.set_coeff(0, -num);
                    factor.set_coeff(1, den);
                    return true;
                  }
              }
          }
      }

    return false;
  }

  /** @brief Try to extract an exact primitive quadratic factor over @f$\mathbb{Z}@f$.
   *
   *  Exhaustively searches primitive quadratic candidates
   *  @f$a x^2 + b x + c@f$ such that @f$a@f$ divides the leading coefficient
   *  and @f$c@f$ divides the constant term. Candidate linear coefficients are
   *  bounded using a Cauchy-style root bound and verified by exact polynomial
   *  division.
   */
  [[nodiscard]] static bool try_extract_primitive_quadratic_factor(const Gen_Polynomial &f,
                                                                   Gen_Polynomial &factor,
                                                                   Gen_Polynomial &quotient)
    requires std::is_integral_v<Coefficient>
  {
    if (f.is_zero() or f.degree() < 2)
      return false;

    if (f.get_coeff(0) == Coefficient{})
      return false;

    DynList<Coefficient> leading_divs = positive_divisors(f.leading_coeff());
    DynList<Coefficient> constant_divs = positive_divisors(f.get_coeff(0));
    if (leading_divs.is_empty() or constant_divs.is_empty())
      return false;

    const double abs_lc = std::abs(static_cast<double>(f.leading_coeff()));
    double max_ratio = 0.0;
    for (auto it = f.coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &term = it.get_curr();
        if (term.first == f.degree())
          continue;
        const double ratio = std::abs(static_cast<double>(term.second)) / abs_lc;
        if (ratio > max_ratio)
          max_ratio = ratio;
      }

    const double root_bound = 1.0 + max_ratio;

    for (auto a_it = leading_divs.get_it(); a_it.has_curr(); a_it.next_ne())
      {
        const Coefficient a = a_it.get_curr();
        const int64_t b_bound
          = static_cast<int64_t>(std::ceil(2.0 * std::abs(static_cast<double>(a)) * root_bound));

        for (auto c_it = constant_divs.get_it(); c_it.has_curr(); c_it.next_ne())
          {
            const Coefficient c_abs = c_it.get_curr();

            for (Coefficient sign : {Coefficient(1), Coefficient(-1)})
              {
                const Coefficient c = sign * c_abs;

                for (int64_t b_i = -b_bound; b_i <= b_bound; ++b_i)
                  {
                    const Coefficient b = static_cast<Coefficient>(b_i);
                    const uint64_t gcd_ab
                      = std::gcd(polynomial_detail::abs_to_u64(a), polynomial_detail::abs_to_u64(b));
                    if (std::gcd(gcd_ab, polynomial_detail::abs_to_u64(c)) != 1)
                      continue;

                    Gen_Polynomial candidate;
                    candidate.set_coeff(0, c);
                    if (b != Coefficient{})
                      candidate.set_coeff(1, b);
                    candidate.set_coeff(2, a);

                    Gen_Polynomial q;
                    if (try_exact_candidate_division(f, candidate, q))
                      {
                        factor = candidate.primitive_part();
                        quotient = q.is_zero() ? q : q.primitive_part();
                        return true;
                      }
                  }
              }
          }
      }

    return false;
  }

  /** @brief Try to extract an exact primitive cubic factor over @f$\mathbb{Z}@f$.
   *
   *  Exhaustively searches primitive cubic candidates
   *  @f$a x^3 + b x^2 + c x + d@f$ such that @f$a@f$ divides the leading
   *  coefficient and @f$d@f$ divides the constant term. Candidate
   *  coefficients are bounded with a Landau-Mignotte style estimate, and
   *  each candidate is verified by exact division.
   */
  [[nodiscard]] static bool try_extract_primitive_cubic_factor(const Gen_Polynomial &f,
                                                               Gen_Polynomial &factor,
                                                               Gen_Polynomial &quotient)
    requires std::is_integral_v<Coefficient>
  {
    if (f.is_zero() or f.degree() < 3)
      return false;

    if (f.get_coeff(0) == Coefficient{})
      return false;

    DynList<Coefficient> leading_divs = positive_divisors(f.leading_coeff());
    DynList<Coefficient> constant_divs = positive_divisors(f.get_coeff(0));
    if (leading_divs.is_empty() or constant_divs.is_empty())
      return false;

    const double abs_lc = std::abs(static_cast<double>(f.leading_coeff()));
    double l2_norm_sq = 0.0;
    for (auto it = f.coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &term = it.get_curr();
        const double coeff_abs = std::abs(static_cast<double>(term.second));
        l2_norm_sq += coeff_abs * coeff_abs;
      }

    const int64_t coeff_bound = static_cast<int64_t>(std::ceil(8.0 * std::sqrt(l2_norm_sq) / abs_lc));
    if (coeff_bound <= 0)
      return false;

    for (auto a_it = leading_divs.get_it(); a_it.has_curr(); a_it.next_ne())
      {
        const Coefficient a = a_it.get_curr();
        if (polynomial_detail::abs_to_u64(a) > static_cast<uint64_t>(coeff_bound))
          continue;

        for (auto d_it = constant_divs.get_it(); d_it.has_curr(); d_it.next_ne())
          {
            const Coefficient d_abs = d_it.get_curr();
            if (polynomial_detail::abs_to_u64(d_abs) > static_cast<uint64_t>(coeff_bound))
              continue;

            for (Coefficient sign : {Coefficient(1), Coefficient(-1)})
              {
                const Coefficient d = sign * d_abs;

                for (int64_t b_i = -coeff_bound; b_i <= coeff_bound; ++b_i)
                  {
                    const Coefficient b = static_cast<Coefficient>(b_i);
                    const uint64_t gcd_ab
                      = std::gcd(polynomial_detail::abs_to_u64(a), polynomial_detail::abs_to_u64(b));

                    for (int64_t c_i = -coeff_bound; c_i <= coeff_bound; ++c_i)
                      {
                        const Coefficient c = static_cast<Coefficient>(c_i);
                        const uint64_t gcd_abc = std::gcd(gcd_ab, polynomial_detail::abs_to_u64(c));
                        if (std::gcd(gcd_abc, polynomial_detail::abs_to_u64(d)) != 1)
                          continue;

                        Gen_Polynomial candidate;
                        candidate.set_coeff(0, d);
                        if (c != Coefficient{})
                          candidate.set_coeff(1, c);
                        if (b != Coefficient{})
                          candidate.set_coeff(2, b);
                        candidate.set_coeff(3, a);

                        Gen_Polynomial q;
                        if (try_exact_candidate_division(f, candidate, q))
                          {
                            factor = candidate.primitive_part();
                            quotient = q.is_zero() ? q : q.primitive_part();
                            return true;
                          }
                      }
                  }
              }
          }
      }

    return false;
  }

  /** @brief Factorization over integers mod @p p (trial method).
   *
   *  Finds all linear factors over @f$\mathbb{Z}/p\mathbb{Z}@f$ by
   *  exhaustive root search and repeated extraction. Any unfactored
   *  residual polynomial is appended as the final element.
   *
   *  @param[in] f Polynomial to factor.
   *  @param[in] p The prime modulus.
   *  @return A list of monic linear factors (with multiplicity), plus
   *          a final residual factor if non-constant.
   *
   *  @throws std::domain_error if p <= 1.
   *
   *  @note Only available for integral coefficient types.
   *  @note This is still a root-search method, not Berlekamp or
   *        Cantor-Zassenhaus; non-linear irreducible factors remain grouped.
   */
  static DynList<Gen_Polynomial> factor_mod_p(const Gen_Polynomial &f, Coefficient p)
    requires std::is_integral_v<Coefficient>
  {
    ah_domain_error_if(p <= Coefficient(1)) << "factor_mod_p: modulus p must be > 1, got " << p;

    DynList<Gen_Polynomial> result;

    if (f.is_zero() or f.is_constant())
      return result;

    Gen_Polynomial remaining = reduce_coeffs_mod(f, p);

    for (Coefficient r = Coefficient{}; r < p; r = r + Coefficient(1))
      {
        while (
          not remaining.is_zero() and remaining.degree() > 0
          and eval_mod_u64(remaining,
                           polynomial_detail::normalize_mod_u64(r, polynomial_detail::abs_to_u64(p)),
                           polynomial_detail::abs_to_u64(p))
                == 0)
          {
            Gen_Polynomial factor;
            factor.set_coeff(0, ((-r) % p + p) % p);
            factor.set_coeff(1, Coefficient(1));
            result.append(factor);
            remaining = divide_by_monic_linear_mod(remaining, r, p);
          }
      }

    if (not remaining.is_zero() and not remaining.is_constant())
      result.append(remaining);

    return result;
  }

  /** @brief Mignotte bound on integer roots.
   *
   *  For an integer polynomial of degree @f$d@f$ with coefficients
   *  @f$a_0, \ldots, a_d@f$, any rational root @f$p/q@f$ with
   *  gcd(p,q)=1 satisfies:
   *  @f[
   *    |p| \le \sqrt{d+1} \cdot 2^d \cdot \max_i |a_i|
   *  @f]
   *
   *  @return Upper bound on absolute value of integer roots.
   */
  [[nodiscard]] double mignotte_bound() const
    requires std::is_integral_v<Coefficient>
  {
    if (is_zero())
      return 0.0;

    const size_t d = degree();
    if (d == 0)
      return 0.0;

    Coefficient max_coeff = Coefficient{};
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        Coefficient abs_c = p.second < Coefficient{} ? -p.second : p.second;
        if (abs_c > max_coeff)
          max_coeff = abs_c;
      }

    const auto d_real = static_cast<double>(d);
    const auto max_real = static_cast<double>(max_coeff);
    const double two_pow_d = polynomial_detail::power(2.0, d);

    return std::sqrt(d_real + 1.0) * two_pow_d * max_real;
  }

  /** @brief Hensel lifting of modular factors.
   *
   *  Given modular factors of @f$f \pmod{p}@f$, lifts any monic linear
   *  factor corresponding to a simple root to modulus
   *  @f$p^{2^\text{levels}}@f$ using repeated Hensel root lifting.
   *  Factors that are not monic linear simple-root factors are carried
   *  through reduced modulo the target modulus.
   *
   *  @param[in] f Original polynomial.
   *  @param[in] factors Factors mod @p p (must multiply to @p f mod @p p).
   *  @param[in] p The prime modulus.
   *  @param[in] levels Number of lifting iterations (quadratic).
   *  @return Factors lifted to @f$\mathbb{Z}/p^{2^\text{levels}}\mathbb{Z}@f$.
   *
   *  @throws std::domain_error if p <= 1 or levels > 30.
   *
   *  @note Only available for integral coefficient types.
   *  @note Singular roots are not lifted; such factors are returned only
   *        normalized modulo the target modulus.
   *  @warning levels > 30 is rejected to prevent overflow in (1 << levels).
   */
  static DynList<Gen_Polynomial> hensel_lift(const Gen_Polynomial &f,
                                             DynList<Gen_Polynomial> factors,
                                             Coefficient p,
                                             size_t levels)
    requires std::is_integral_v<Coefficient>
  {
    ah_domain_error_if(p <= Coefficient(1)) << "hensel_lift: modulus p must be > 1, got " << p;
    ah_domain_error_if(levels > 30)
      << "hensel_lift: levels must be <= 30 to avoid overflow, got " << levels;

    if (factors.is_empty())
      return factors;

    Coefficient p_power = p;
    const size_t target_steps = static_cast<size_t>(1) << levels;
    for (size_t i = 1; i < target_steps; ++i)
      p_power = p_power * p;

    const uint64_t p_u64 = polynomial_detail::abs_to_u64(p);
    const uint64_t target_u64 = polynomial_detail::abs_to_u64(p_power);
    Gen_Polynomial derivative = f.derivative();

    DynList<Gen_Polynomial> result;
    for (auto it = factors.get_it(); it.has_curr(); it.next_ne())
      {
        const Gen_Polynomial &factor = it.get_curr();
        bool lifted_linear_factor = false;

        if (factor.degree() == 1 and factor.get_coeff(1) == Coefficient(1))
          {
            uint64_t root = polynomial_detail::normalize_mod_u64(-factor.get_coeff(0), p_u64);
            uint64_t modulus = p_u64;
            bool simple = true;

            for (size_t step = 1; step < target_steps; ++step)
              {
                const uint64_t next_mod = modulus * p_u64;
                const uint64_t f_val = eval_mod_u64(f, root, next_mod);
                const uint64_t deriv_modp = eval_mod_u64(derivative, root % p_u64, p_u64);

                if (f_val % modulus != 0 or std::gcd(deriv_modp, p_u64) != 1)
                  {
                    simple = false;
                    break;
                  }

                const uint64_t e = (f_val / modulus) % p_u64;
                const uint64_t inv = mod_inv(deriv_modp, p_u64);
                const uint64_t t = (p_u64 - mod_mul(e, inv, p_u64)) % p_u64;

                root = (root + modulus * t) % next_mod;
                modulus = next_mod;
              }

            if (simple)
              {
                Gen_Polynomial lifted;
                lifted.set_coeff(0,
                                 polynomial_detail::centered_from_mod_u64<Coefficient>(
                                   (target_u64 - (root % target_u64)) % target_u64, target_u64));
                lifted.set_coeff(1, Coefficient(1));
                result.append(lifted);
                lifted_linear_factor = true;
              }
          }

        if (lifted_linear_factor)
          continue;

        Gen_Polynomial normalized;
        factor.for_each_term([&normalized, p_power](size_t exp, const Coefficient &coeff)
        {
          Coefficient canonical = coeff % p_power;
          if (canonical < Coefficient{})
            canonical += p_power;
          if (canonical != Coefficient{})
            normalized.set_coeff(exp, canonical);
        });
        result.append(normalized);
      }

    return result;
  }

  /** @brief Main factorization over integers.
   *
   *  Combines square-free decomposition (SFD), exact extraction of
   *  rational linear factors, and modular root search / Hensel lifting
   *  heuristics for any remaining square-free blocks.
   *
   *  **Algorithm outline**:
   *  1. Compute the primitive part (remove common factors).
   *  2. Apply Yun's SFD to group factors by multiplicity.
   *  3. For each square-free group, extract exact primitive linear,
   *     quadratic, and cubic factors over @f$\mathbb{Z}@f$.
   *  4. For any residual factor, use modular root search plus Hensel
   *     lifting of simple linear roots to discover additional exact divisors.
   *
   *  @return A list of SfdTerm where each factor is irreducible
   *          (or as close as possible with this algorithm).
   *
   *  @note Only available for integral coefficient types.
   *  @note This remains a lightweight implementation. It is strong on
   *        exact linear, quadratic, and cubic factors and conservative on
   *        higher-degree residuals.
   *  @note Every factor returned here is validated by exact division, but a
   *        remaining higher-degree block should be interpreted as
   *        "not split further by this algorithm", not as a proof of
   *        irreducibility over @f$\mathbb{Z}@f$.
   *  @note **Complexity**: depends on SFD and modular factorization.
   */
  [[nodiscard]] DynList<SfdTerm> factorize() const
    requires std::is_integral_v<Coefficient>
  {
    DynList<SfdTerm> result;

    if (is_zero() or is_constant())
      return result;

    // Step 1: Get primitive part
    Gen_Polynomial prim = primitive_part();

    // Step 2: Compute square-free decomposition
    DynList<SfdTerm> sfd_terms = prim.yun_sfd();

    if (sfd_terms.is_empty())
      return result;

    // Step 3: For each square-free group, attempt factorization
    for (auto it = sfd_terms.get_it(); it.has_curr(); it.next_ne())
      {
        auto &sfd_term = it.get_curr();
        Gen_Polynomial factor = sfd_term.factor;
        size_t mult = sfd_term.multiplicity;
        Gen_Polynomial remaining = factor.primitive_part();

        while (remaining.degree() > 0)
          {
            Gen_Polynomial exact_linear, quotient;
            if (not try_extract_rational_linear_factor(remaining, exact_linear, quotient))
              break;

            result.append(SfdTerm{exact_linear.primitive_part(), mult});
            remaining = quotient.is_zero() ? quotient : quotient.primitive_part();
          }

        while (remaining.degree() > 1)
          {
            Gen_Polynomial exact_quadratic, quotient;
            if (not try_extract_primitive_quadratic_factor(remaining, exact_quadratic, quotient))
              break;

            result.append(SfdTerm{exact_quadratic.primitive_part(), mult});
            remaining = quotient.is_zero() ? quotient : quotient.primitive_part();
          }

        while (remaining.degree() > 2)
          {
            Gen_Polynomial exact_cubic, quotient;
            if (not try_extract_primitive_cubic_factor(remaining, exact_cubic, quotient))
              break;

            result.append(SfdTerm{exact_cubic.primitive_part(), mult});
            remaining = quotient.is_zero() ? quotient : quotient.primitive_part();
          }

        if (remaining.is_constant() or remaining.is_zero())
          continue;

        // Try modular factorization with a few small primes and exact redivision.
        for (Coefficient p : {Coefficient(3), Coefficient(5), Coefficient(7), Coefficient(11)})
          {
            if (remaining.leading_coeff() % p == Coefficient{})
              continue;

            auto mod_factors = factor_mod_p(remaining, p);
            if (mod_factors.size() <= 1)
              continue;

            auto lifted = hensel_lift(remaining, mod_factors, p, 2);

            bool changed = false;
            for (auto jt = lifted.get_it(); jt.has_curr(); jt.next_ne())
              {
                Gen_Polynomial cand = jt.get_curr().primitive_part();
                if (cand.is_zero() or cand.is_constant() or cand.degree() != 1)
                  continue;

                Gen_Polynomial quotient;
                if (not try_divide_by_linear_factor(
                      remaining, cand.get_coeff(1), cand.get_coeff(0), quotient))
                  continue;

                result.append(SfdTerm{cand, mult});
                remaining = quotient.is_zero() ? quotient : quotient.primitive_part();
                changed = true;

                if (remaining.is_constant())
                  break;
              }

            if (changed)
              {
                if (remaining.is_constant())
                  break;
              }
          }

        if (not remaining.is_constant() and not remaining.is_zero())
          result.append(SfdTerm{remaining.primitive_part(), mult});
      }

    return result;
  }
};

// =================================================================
//  Free functions
// =================================================================

/** @brief Scalar times polynomial (commutative form). */
template <typename C>
[[nodiscard]] Gen_Polynomial<C> operator*(const C &s, const Gen_Polynomial<C> &p)
{
  return p * s;
}

/** @brief Scalar plus polynomial (commutative form). */
template <typename C>
[[nodiscard]] Gen_Polynomial<C> operator+(const C &s, const Gen_Polynomial<C> &p)
{
  return p + s;
}

/** @brief Scalar minus polynomial. */
template <typename C>
[[nodiscard]] Gen_Polynomial<C> operator-(const C &s, const Gen_Polynomial<C> &p)
{
  return Gen_Polynomial<C>(s) - p;
}

/** @brief Stream output. */
template <typename C>
std::ostream &operator<<(std::ostream &out, const Gen_Polynomial<C> &p)
{
  return out << p.to_str();
}

// =================================================================
//  Convenience type aliases
// =================================================================

/** @brief Polynomial with `double` coefficients. */
using Polynomial = Gen_Polynomial<double>;

/** @brief Polynomial with `float` coefficients. */
using PolynomialF = Gen_Polynomial<float>;

/** @brief Polynomial with `long double` coefficients. */
using PolynomialLD = Gen_Polynomial<long double>;

/** @brief Polynomial with `long long` (integer) coefficients. */
using IntPolynomial = Gen_Polynomial<long long>;

}  // end namespace Aleph

#endif  // TPL_POLYNOMIAL_H
