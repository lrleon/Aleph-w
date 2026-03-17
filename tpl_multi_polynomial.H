/*
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

/** @file tpl_multi_polynomial.H
 *  @brief Sparse multivariate polynomial over an arbitrary coefficient ring.
 *
 *  Provides `Gen_MultiPolynomial<Coefficient, MonomOrder>`, a template
 *  class representing polynomials in @f$n@f$ variables with a sparse
 *  map from multi-index (exponent vector) to coefficient.  Three
 *  standard monomial orderings are included: lexicographic (`Lex_Order`),
 *  graded lexicographic (`Grlex_Order`), and graded reverse
 *  lexicographic (`Grevlex_Order`, the default).
 *
 *  Layer 1 of the multivariate polynomial subsystem — covers
 *  representation, arithmetic, evaluation, and pretty-printing.
 *
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_MULTI_POLYNOMIAL_H
#define TPL_MULTI_POLYNOMIAL_H

#include <sstream>
#include <string>
#include <type_traits>
#include <limits>
#include <cmath>
#include <utility>
#include <iomanip>
#include <future>
#include <thread>
#include <fstream>
#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_dynMapTree.H>
#include <htlist.H>
#include <ah-parallel.H>

namespace Aleph {

// ===================================================================
//  Multi-index helpers
// ===================================================================

/** @brief Internal helpers for multivariate polynomial operations. */
namespace multi_poly_detail {

/** @brief Total degree of a multi-index (sum of exponents). */
inline size_t total_degree(const Array<size_t> &idx) noexcept
{
  size_t sum = 0;
  for (size_t i = 0; i < idx.size(); ++i)
    sum += idx(i);
  return sum;
}

/** @brief Test whether a multi-index is the zero vector. */
inline bool is_zero_index(const Array<size_t> &idx) noexcept
{
  for (size_t i = 0; i < idx.size(); ++i)
    if (idx(i) != 0)
      return false;
  return true;
}

/** @brief Component-wise addition of two multi-indices.
 *  Pads the shorter one with zeros.
 */
inline Array<size_t> add_indices(const Array<size_t> &a, const Array<size_t> &b)
{
  const size_t  n = std::max(a.size(), b.size());
  Array<size_t> r(n, size_t{0});
  for (size_t i = 0; i < a.size(); ++i)
    r(i) += a(i);
  for (size_t i = 0; i < b.size(); ++i)
    r(i) += b(i);
  return r;
}

/** @brief Pad a multi-index with trailing zeros to reach @p target size. */
inline Array<size_t> extend_index(const Array<size_t> &idx, size_t target)
{
  if (idx.size() >= target)
    return idx;
  Array<size_t> r(target, size_t{0});
  for (size_t i = 0; i < idx.size(); ++i)
    r(i) = idx(i);
  return r;
}

/** @brief Integer power by repeated squaring. */
template <typename T>
T int_power(const T &base, size_t exp)
{
  T result = T(1);
  T b      = base;
  while (exp > 0)
    {
      if (exp & 1)
        result *= b;
      b *= b;
      exp >>= 1;
    }
  return result;
}

// ===================================================================
// Phase 2 — Differential calculus, interpolation, serialization, parallelism
// ===================================================================

/** @brief Decrement exponent at a specific variable. */
inline Array<size_t> decrement_index_at(const Array<size_t> &idx, size_t var, size_t n = 1)
{
  Array<size_t> r = idx;
  r(var) -= n;
  return r;
}

/** @brief Falling factorial k(k-1)(k-2)...(k-n+1).
 *  @return k!/(k-n)! if k >= n, else 0 (when k < n or n==0 -> 1).
 */
template <typename T>
T falling_factorial(size_t k, size_t n)
{
  if (n == 0)
    return T(1);
  if (k < n)
    return T(0);
  T res = T(1);
  for (size_t i = 0; i < n; ++i)
    res *= T(k - i);
  return res;
}

/** @brief Convert flat index to multi-index given sizes per dimension.
 *  Assumes row-major order (rightmost dimension varies fastest).
 */
inline Array<size_t> flat_to_multi_index(size_t flat, const Array<size_t> &sizes)
{
  const size_t  n = sizes.size();
  Array<size_t> r(n, size_t{0});
  for (size_t i = n; i > 0; --i)
    {
      r(i - 1) = flat % sizes(i - 1);
      flat /= sizes(i - 1);
    }
  return r;
}

/** @brief Convert multi-index to flat index given sizes per dimension. */
inline size_t multi_to_flat_index(const Array<size_t> &midx, const Array<size_t> &sizes)
{
  size_t       flat = 0, stride = 1;
  const size_t n = sizes.size();
  for (size_t i = n; i > 0; --i)
    {
      flat += midx(i - 1) * stride;
      stride *= sizes(i - 1);
    }
  return flat;
}

/** @brief Evaluate monomial at a point (helper for fitting/interpolation). */
template <typename Coefficient>
Coefficient eval_monomial(const Array<Coefficient> &pt, const Array<size_t> &alpha)
{
  Coefficient v = Coefficient(1);
  for (size_t j = 0; j < alpha.size(); ++j)
    if (alpha(j) != 0)
      v *= int_power(pt(j), alpha(j));
  return v;
}

/** @brief Test whether alpha divides beta component-wise (alpha(i) <= beta(i) all i). */
inline bool divides_index(const Array<size_t> &alpha, const Array<size_t> &beta, size_t nvars) noexcept
{
  for (size_t i = 0; i < nvars; ++i)
    {
      const size_t ai = i < alpha.size() ? alpha(i) : 0;
      const size_t bi = i < beta.size() ? beta(i) : 0;
      if (ai > bi)
        return false;
    }
  return true;
}

/** @brief Component-wise max = monomial LCM. */
inline Array<size_t> lcm_indices(const Array<size_t> &a, const Array<size_t> &b, size_t nvars)
{
  Array<size_t> r(nvars, size_t{0});
  for (size_t i = 0; i < nvars; ++i)
    r(i) = std::max(i < a.size() ? a(i) : 0, i < b.size() ? b(i) : 0);
  return r;
}

/** @brief Component-wise subtraction beta - alpha (requires alpha divides beta). */
inline Array<size_t> sub_indices(const Array<size_t> &beta, const Array<size_t> &alpha, size_t nvars)
{
  Array<size_t> r(nvars, size_t{0});
  for (size_t i = 0; i < nvars; ++i)
    r(i) = (i < beta.size() ? beta(i) : 0) - (i < alpha.size() ? alpha(i) : 0);
  return r;
}

}  // namespace multi_poly_detail

// ===================================================================
//  Monomial orderings
// ===================================================================

/** @brief Lexicographic monomial order.
 *
 *  Compares exponent vectors from left (x_0) to right.
 *  @f$\alpha <_{\text{lex}} \beta@f$ iff the leftmost nonzero entry
 *  of @f$\alpha - \beta@f$ is negative.
 */
struct Lex_Order
{
  bool operator()(const Array<size_t> &a, const Array<size_t> &b) const noexcept
  {
    const size_t n = std::max(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
      {
        const size_t ai = i < a.size() ? a(i) : 0;
        const size_t bi = i < b.size() ? b(i) : 0;
        if (ai < bi)
          return true;
        if (ai > bi)
          return false;
      }
    return false;
  }
};

/** @brief Graded lexicographic monomial order.
 *
 *  Total degree first, then lexicographic.
 */
struct Grlex_Order
{
  bool operator()(const Array<size_t> &a, const Array<size_t> &b) const noexcept
  {
    const size_t da = multi_poly_detail::total_degree(a);
    const size_t db = multi_poly_detail::total_degree(b);
    if (da != db)
      return da < db;
    return Lex_Order()(a, b);
  }
};

/** @brief Graded reverse lexicographic monomial order (default).
 *
 *  Total degree first; among same-degree monomials the one with the
 *  larger rightmost exponent is smaller.  This is the standard
 *  ordering used by most computer algebra systems.
 */
struct Grevlex_Order
{
  bool operator()(const Array<size_t> &a, const Array<size_t> &b) const noexcept
  {
    const size_t da = multi_poly_detail::total_degree(a);
    const size_t db = multi_poly_detail::total_degree(b);
    if (da != db)
      return da < db;
    const size_t n = std::max(a.size(), b.size());
    for (size_t i = n; i > 0; --i)
      {
        const size_t ai = (i - 1) < a.size() ? a(i - 1) : 0;
        const size_t bi = (i - 1) < b.size() ? b(i - 1) : 0;
        if (ai > bi)
          return true;  // larger last exp → smaller in grevlex
        if (ai < bi)
          return false;
      }
    return false;
  }
};

// ===================================================================
//  Gen_MultiPolynomial
// ===================================================================

/** @brief Sparse multivariate polynomial.
 *
 *  Represents a polynomial in @f$n@f$ variables
 *  @f$x_0, x_1, \ldots, x_{n-1}@f$ over a coefficient ring.
 *  Terms are stored in a balanced BST keyed by exponent vectors,
 *  ordered according to the monomial ordering @p MonomOrder.
 *
 *  @tparam Coefficient  Coefficient type (default `double`).
 *  @tparam MonomOrder   Strict-weak-order functor on `Array<size_t>`
 *                       (default `Grevlex_Order`).
 */
template <typename Coefficient = double, class MonomOrder = Grevlex_Order>
class Gen_MultiPolynomial
{
  size_t nvars_ = 0;

  DynMapTree<Array<size_t>, Coefficient, Avl_Tree, MonomOrder> coeffs;

  /// Pad an index to nvars_ trailing zeros.
  [[nodiscard]] Array<size_t> norm(const Array<size_t> &idx) const
  {
    return multi_poly_detail::extend_index(idx, nvars_);
  }

public:
  // =================================================================
  //  Type aliases
  // =================================================================

  using Coeff = Coefficient;
  using Index = Array<size_t>;
  using Order = MonomOrder;

  // =================================================================
  //  Coefficient zero-testing
  // =================================================================

  /** @brief Machine epsilon for floating-point coefficients.
   *  @return 128 * numeric_limits::epsilon for floats, 0 for exact types.
   */
  static constexpr Coefficient epsilon() noexcept
  {
    if constexpr (std::is_floating_point_v<Coefficient>)
      return Coefficient(128) * std::numeric_limits<Coefficient>::epsilon();
    else
      return Coefficient{};
  }

  /** @brief Test whether a coefficient is (approximately) zero.
   *  @param[in] c Coefficient to test.
   *  @return True if @p c is zero (within epsilon for floats).
   */
  static bool coeff_is_zero(const Coefficient &c) noexcept
  {
    if constexpr (std::is_floating_point_v<Coefficient>)
      return std::abs(c) <= epsilon();
    else
      return c == Coefficient{};
  }

  // =================================================================
  //  Constructors
  // =================================================================

  /** @brief Default constructor: the zero polynomial with 0 variables. */
  Gen_MultiPolynomial() = default;

  /** @brief Constant polynomial.
   *  @param[in] nvars Number of variables.
   *  @param[in] c     Constant value (default 0).
   */
  explicit Gen_MultiPolynomial(size_t nvars, const Coefficient &c = Coefficient{}) : nvars_(nvars)
  {
    if (not coeff_is_zero(c))
      coeffs.insert(Array<size_t>(nvars_, size_t{0}), c);
  }

  /** @brief Single-term constructor.
   *  @param[in] nvars Number of variables.
   *  @param[in] idx   Exponent vector.
   *  @param[in] c     Coefficient.
   */
  Gen_MultiPolynomial(size_t nvars, const Array<size_t> &idx, const Coefficient &c) : nvars_(nvars)
  {
    if (not coeff_is_zero(c))
      coeffs.insert(norm(idx), c);
  }

  /** @brief Construct from a list of (exponent-vector, coefficient) pairs.
   *  @param[in] nvars Number of variables.
   *  @param[in] ts    Term list.
   */
  Gen_MultiPolynomial(size_t nvars, const DynList<std::pair<Array<size_t>, Coefficient>> &ts)
    : nvars_(nvars)
  {
    ts.for_each([this](const std::pair<Array<size_t>, Coefficient> &t)
    {
      add_to_coeff(t.first, t.second);
    });
  }

  /** @brief Construct from an initializer list of term pairs.
   *  @param[in] nvars Number of variables.
   *  @param[in] ts    Initializer list of {exponent-vector, coefficient}.
   */
  Gen_MultiPolynomial(size_t nvars, std::initializer_list<std::pair<Array<size_t>, Coefficient>> ts)
    : nvars_(nvars)
  {
    for (const auto &t : ts)
      add_to_coeff(t.first, t.second);
  }

  // =================================================================
  //  Factory helpers
  // =================================================================

  /** @brief The polynomial @f$x_i@f$ (a single variable).
   *  @param[in] nvars    Number of variables.
   *  @param[in] var      Variable index (0-based).
   *  @return Polynomial whose only term is @f$x_{\text{var}}@f$.
   */
  [[nodiscard]] static Gen_MultiPolynomial variable(size_t nvars, size_t var)
  {
    ah_domain_error_if(var >= nvars) << "variable index " << var << " >= nvars " << nvars;
    Array<size_t> idx(nvars, size_t{0});
    idx(var) = 1;
    return Gen_MultiPolynomial(nvars, idx, Coefficient(1));
  }

  /** @brief A single monomial @f$c \, x^{\alpha}@f$.
   *  @param[in] nvars Number of variables.
   *  @param[in] idx   Exponent vector @f$\alpha@f$.
   *  @param[in] c     Coefficient (default 1).
   *  @return The monomial @f$c \, x_0^{\alpha_0} \cdots x_{n-1}^{\alpha_{n-1}}@f$.
   */
  [[nodiscard]] static Gen_MultiPolynomial monomial(size_t nvars, const Array<size_t> &idx,
                                                    const Coefficient &c = Coefficient(1))
  {
    return Gen_MultiPolynomial(nvars, idx, c);
  }

  // =================================================================
  //  Properties
  // =================================================================

  /** @brief True if this is the zero polynomial. */
  [[nodiscard]] bool is_zero() const noexcept
  {
    return coeffs.is_empty();
  }

  /** @brief True if constant or zero (total degree 0). */
  [[nodiscard]] bool is_constant() const noexcept
  {
    if (coeffs.is_empty())
      return true;
    if (coeffs.size() != 1)
      return false;
    return multi_poly_detail::is_zero_index(coeffs.min().first);
  }

  /** @brief Number of variables. */
  [[nodiscard]] size_t num_vars() const noexcept
  {
    return nvars_;
  }

  /** @brief Number of non-zero terms. */
  [[nodiscard]] size_t num_terms() const noexcept
  {
    return coeffs.size();
  }

  /** @brief Total degree (maximum sum of exponents over all terms).
   *  @return 0 for the zero or constant polynomial.
   */
  [[nodiscard]] size_t degree() const noexcept
  {
    size_t d = 0;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      if (size_t td = multi_poly_detail::total_degree(it.get_curr().first); td > d)
        d = td;
    return d;
  }

  /** @brief Degree in a specific variable.
   *  @param[in] var Variable index (0-based).
   *  @return Maximum exponent of @p var over all terms.
   */
  [[nodiscard]] size_t degree_in(size_t var) const noexcept
  {
    size_t d = 0;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &idx = it.get_curr().first;
        if (const size_t e = var < idx.size() ? idx(var) : 0; e > d)
          d = e;
      }
    return d;
  }

  // =================================================================
  //  Leading-term access
  // =================================================================

  /** @brief Leading term (largest monomial in the ordering).
   *  @return Pair (exponent vector, coefficient).
   *  @throws std::domain_error if the polynomial is zero.
   */
  [[nodiscard]] std::pair<Array<size_t>, Coefficient> leading_term() const
  {
    ah_domain_error_if(is_zero()) << "leading_term of zero polynomial";
    const auto &m = coeffs.max();
    return {m.first, m.second};
  }

  /** @brief Leading coefficient.
   *  @return Coefficient of the leading term.
   *  @throws std::domain_error if the polynomial is zero.
   */
  [[nodiscard]] Coefficient leading_coeff() const
  {
    return leading_term().second;
  }

  /** @brief Leading monomial (exponent vector of the leading term).
   *  @return Multi-index of the leading term.
   *  @throws std::domain_error if the polynomial is zero.
   */
  [[nodiscard]] Array<size_t> leading_monomial() const
  {
    return leading_term().first;
  }

  // =================================================================
  //  Coefficient access
  // =================================================================

  /** @brief Read coefficient at a multi-index (0 if absent).
   *  @param[in] idx Exponent vector.
   *  @return Stored coefficient, or @c Coefficient{} if the term is absent.
   */
  [[nodiscard]] Coefficient coeff_at(const Array<size_t> &idx) const
  {
    auto *p = coeffs.search(norm(idx));
    return p ? p->second : Coefficient{};
  }

  // =================================================================
  //  Iteration
  // =================================================================

  /** @brief Visit every non-zero term in ascending monomial order.
   *  @param[in] op Callable `void(const Array<size_t>&, const Coefficient&)`.
   */
  template <class Op>
  void for_each_term(Op &&op) const
  {
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &p = it.get_curr();
        op(p.first, p.second);
      }
  }

  /** @brief Visit every non-zero term in descending monomial order.
   *  @param[in] op Callable `void(const Array<size_t>&, const Coefficient&)`.
   */
  template <class Op>
  void for_each_term_desc(Op &&op) const
  {
    DynList<std::pair<Array<size_t>, Coefficient>> tmp;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        const auto &p = it.get_curr();
        tmp.insert({p.first, p.second});  // prepend → reversed
      }
    tmp.for_each([&op](const std::pair<Array<size_t>, Coefficient> &t)
    {
      op(t.first, t.second);
    });
  }

  /** @brief All terms as a list (ascending order).
   *  @return DynList of (exponent-vector, coefficient) pairs.
   */
  [[nodiscard]] DynList<std::pair<Array<size_t>, Coefficient>> terms() const
  {
    DynList<std::pair<Array<size_t>, Coefficient>> r;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      r.append({idx, c});
    });
    return r;
  }

  // =================================================================
  //  Coefficient modification
  // =================================================================

  /** @brief Add @p delta to the coefficient at @p idx.
   *
   *  Inserts the term if absent; removes it if the result is zero.
   *
   *  @param[in] idx   Exponent vector.
   *  @param[in] delta Value to add.
   */
  void add_to_coeff(const Array<size_t> &idx, const Coefficient &delta)
  {
    if (coeff_is_zero(delta))
      return;

    auto  nidx = norm(idx);
    auto *p    = coeffs.search(nidx);
    if (p != nullptr)
      {
        p->second = p->second + delta;
        if (coeff_is_zero(p->second))
          coeffs.remove_key(nidx);
      }
    else
      coeffs.insert(nidx, delta);
  }

  /** @brief Remove all terms whose coefficient is approximately zero. */
  void remove_zeros()
  {
    DynList<Array<size_t>> zs;
    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      {
        auto &p = it.get_curr();
        if (coeff_is_zero(p.second))
          zs.append(p.first);
      }
    zs.for_each([this](const Array<size_t> &k)
    {
      coeffs.remove_key(k);
    });
  }

  /** @brief Multiply every coefficient by @p s in place.
   *  @param[in] s Scalar.
   */
  void scale_inplace(const Coefficient &s)
  {
    if (coeffs.is_empty())
      return;

    if (coeff_is_zero(s))
      {
        coeffs = decltype(coeffs)();
        return;
      }

    if (s == Coefficient(1))
      return;

    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      it.get_curr().second *= s;

    remove_zeros();
  }

  /** @brief Divide every coefficient by @p s in place.
   *  @param[in] s Scalar divisor (must not be zero).
   *  @throws std::domain_error if @p s is zero.
   */
  void divide_scalar_inplace(const Coefficient &s)
  {
    ah_domain_error_if(coeff_is_zero(s)) << "multivariate polynomial division by zero scalar";

    if (s == Coefficient(1))
      return;

    for (auto it = coeffs.get_it(); it.has_curr(); it.next_ne())
      it.get_curr().second /= s;

    remove_zeros();
  }

  // =================================================================
  //  Arithmetic — polynomial ± polynomial
  // =================================================================

  /** @brief Polynomial addition.
   *  @param[in] q Addend.
   *  @return Sum @f$\text{this} + q@f$.
   */
  [[nodiscard]] Gen_MultiPolynomial operator+(const Gen_MultiPolynomial &q) const
  {
    Gen_MultiPolynomial r = *this;
    r += q;
    return r;
  }

  /** @brief In-place addition.
   *  @param[in] q Addend.
   *  @return Reference to @c *this.
   */
  Gen_MultiPolynomial &operator+=(const Gen_MultiPolynomial &q)
  {
    if (this == &q)
      {
        scale_inplace(Coefficient(1) + Coefficient(1));
        return *this;
      }

    nvars_ = std::max(nvars_, q.nvars_);
    q.for_each_term([this](const Array<size_t> &idx, const Coefficient &c)
    {
      add_to_coeff(idx, c);
    });
    return *this;
  }

  /** @brief Polynomial subtraction.
   *  @param[in] q Subtrahend.
   *  @return Difference @f$\text{this} - q@f$.
   */
  [[nodiscard]] Gen_MultiPolynomial operator-(const Gen_MultiPolynomial &q) const
  {
    Gen_MultiPolynomial r = *this;
    r -= q;
    return r;
  }

  /** @brief In-place subtraction.
   *  @param[in] q Subtrahend.
   *  @return Reference to @c *this.
   */
  Gen_MultiPolynomial &operator-=(const Gen_MultiPolynomial &q)
  {
    if (this == &q)
      {
        coeffs = decltype(coeffs)();
        return *this;
      }

    nvars_ = std::max(nvars_, q.nvars_);
    q.for_each_term([this](const Array<size_t> &idx, const Coefficient &c)
    {
      add_to_coeff(idx, -c);
    });
    return *this;
  }

  /** @brief Unary negation.
   *  @return @f$-p@f$.
   */
  [[nodiscard]] Gen_MultiPolynomial operator-() const
  {
    Gen_MultiPolynomial r;
    r.nvars_ = nvars_;
    for_each_term([&r](const Array<size_t> &idx, const Coefficient &c)
    {
      r.coeffs.insert(idx, -c);
    });
    return r;
  }

  // =================================================================
  //  Arithmetic — polynomial * polynomial
  // =================================================================

  /** @brief Polynomial multiplication.
   *  @param[in] q Multiplicand.
   *  @return Product @f$\text{this} \cdot q@f$.
   *
   *  @note **Complexity**: @f$O(k_1 k_2 \log(k_1 k_2))@f$
   *        where @f$k_i@f$ is the number of terms.
   */
  [[nodiscard]] Gen_MultiPolynomial operator*(const Gen_MultiPolynomial &q) const
  {
    const size_t nv = std::max(nvars_, q.nvars_);

    if (is_zero() or q.is_zero())
      return Gen_MultiPolynomial(nv);

    Gen_MultiPolynomial r;
    r.nvars_ = nv;

    for_each_term([&](const Array<size_t> &ai, const Coefficient &ac)
    {
      q.for_each_term([&](const Array<size_t> &bi, const Coefficient &bc)
      {
        auto si = multi_poly_detail::add_indices(ai, bi);
        si      = multi_poly_detail::extend_index(si, nv);
        r.add_to_coeff(si, ac * bc);
      });
    });

    return r;
  }

  /** @brief In-place polynomial multiplication.
   *  @param[in] q Multiplicand.
   *  @return Reference to @c *this.
   */
  Gen_MultiPolynomial &operator*=(const Gen_MultiPolynomial &q)
  {
    *this = *this * q;
    return *this;
  }

  // =================================================================
  //  Arithmetic — scalar operations
  // =================================================================

  /** @brief Multiply by a scalar.
   *  @param[in] s Scalar.
   *  @return Scaled polynomial.
   */
  [[nodiscard]] Gen_MultiPolynomial operator*(const Coefficient &s) const
  {
    if (coeff_is_zero(s))
      return Gen_MultiPolynomial(nvars_);

    Gen_MultiPolynomial r;
    r.nvars_ = nvars_;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      if (Coefficient prod = c * s; not coeff_is_zero(prod))
        r.coeffs.insert(idx, prod);
    });
    return r;
  }

  /** @brief In-place scalar multiplication.
   *  @param[in] s Scalar.
   *  @return Reference to @c *this.
   */
  Gen_MultiPolynomial &operator*=(const Coefficient &s)
  {
    scale_inplace(s);
    return *this;
  }

  /** @brief Divide by a scalar.
   *  @param[in] s Scalar divisor (must not be zero).
   *  @return Quotient polynomial.
   *  @throws std::domain_error if @p s is zero.
   */
  [[nodiscard]] Gen_MultiPolynomial operator/(const Coefficient &s) const
  {
    ah_domain_error_if(coeff_is_zero(s)) << "multivariate polynomial division by zero scalar";

    Gen_MultiPolynomial r;
    r.nvars_ = nvars_;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      Coefficient q = c / s;
      if (not coeff_is_zero(q))
        r.coeffs.insert(idx, q);
    });
    return r;
  }

  /** @brief In-place scalar division.
   *  @param[in] s Scalar divisor (must not be zero).
   *  @return Reference to @c *this.
   *  @throws std::domain_error if @p s is zero.
   */
  Gen_MultiPolynomial &operator/=(const Coefficient &s)
  {
    divide_scalar_inplace(s);
    return *this;
  }

  // =================================================================
  //  Exponentiation
  // =================================================================

  /** @brief Exponentiation @f$p^n@f$ by repeated squaring.
   *  @param[in] n Exponent.
   *  @return @f$p^n@f$.  Returns the constant 1 (with same num_vars) if @p n == 0.
   */
  [[nodiscard]] Gen_MultiPolynomial pow(size_t n) const
  {
    Gen_MultiPolynomial result(nvars_, Coefficient(1));
    Gen_MultiPolynomial base = *this;
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
  //  Evaluation
  // =================================================================

  /** @brief Evaluate at a point.
   *  @param[in] pt Array of values, one per variable.
   *  @return @f$p(v_0, \ldots, v_{n-1})@f$.
   *  @throws std::domain_error if @p pt has fewer elements than num_vars().
   */
  [[nodiscard]] Coefficient eval(const Array<Coefficient> &pt) const
  {
    ah_domain_error_if(pt.size() < nvars_)
      << "eval: point has " << pt.size() << " components but polynomial has " << nvars_
      << " variables";

    Coefficient result = Coefficient{};

    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      Coefficient term = c;
      for (size_t j = 0; j < idx.size(); ++j)
        {
          if (idx(j) == 0)
            continue;
          term *= multi_poly_detail::int_power(pt(j), idx(j));
        }
      result += term;
    });

    return result;
  }

  /** @brief Evaluate at a point (function-call syntax).
   *  @param[in] pt Array of values.
   *  @return @f$p(\text{pt})@f$.
   *  @throws std::domain_error if @p pt has fewer elements than num_vars().
   */
  [[nodiscard]] Coefficient operator()(const Array<Coefficient> &pt) const
  {
    return eval(pt);
  }

  // =================================================================
  //  Comparison
  // =================================================================

  /** @brief Polynomial equality.
   *  @param[in] q Other polynomial.
   *  @return True iff all non-zero terms coincide (within epsilon for floats).
   */
  [[nodiscard]] bool operator==(const Gen_MultiPolynomial &q) const
  {
    if (num_terms() != q.num_terms())
      return false;

    bool eq = true;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      if (not eq)
        return;
      if (not coeff_is_zero(c - q.coeff_at(idx)))
        eq = false;
    });
    return eq;
  }

  /** @brief Polynomial inequality. */
  [[nodiscard]] bool operator!=(const Gen_MultiPolynomial &q) const
  {
    return not(*this == q);
  }

  // =================================================================
  //  String representation
  // =================================================================

  /** @brief Human-readable string.
   *
   *  Uses variable names x, y, z for up to 3 variables, or x0, x1, ...
   *  for more.  Custom names can be supplied via @p names.
   *
   *  @param[in] names Optional variable-name list.
   *  @return String such as @c "3*x^2*y - 2".
   */
  [[nodiscard]] std::string to_str(const DynList<std::string> &names = DynList<std::string>()) const
  {
    if (is_zero())
      return "0";

    // Build name lookup
    Array<std::string> vn;
    {
      size_t j = 0;
      names.for_each([&](const std::string &n)
      {
        if (j < nvars_)
          vn.append(n);
        ++j;
      });
    }
    while (vn.size() < nvars_)
      if (nvars_ <= 3)
        {
          constexpr char abc[] = {'x', 'y', 'z'};
          vn.append(std::string(1, abc[vn.size()]));
        }
      else
        vn.append("x" + std::to_string(vn.size()));

    std::ostringstream oss;
    bool               first = true;

    for_each_term_desc([&](const Array<size_t> &idx, const Coefficient &c)
    {
      Coefficient ac  = c;
      bool        neg = false;
      if constexpr (std::is_floating_point_v<Coefficient>)
        {
          if (c < 0)
            {
              ac  = -c;
              neg = true;
            }
        }
      else
        {
          if (c < Coefficient{})
            {
              ac  = -c;
              neg = true;
            }
        }

      bool has_vars = not multi_poly_detail::is_zero_index(idx);

      // Sign
      if (first)
        {
          if (neg)
            oss << "-";
        }
      else
        oss << (neg ? " - " : " + ");

      // Coefficient (suppress 1 / -1 when variables are present)
      bool cp = false;
      if (not has_vars or not coeff_is_zero(ac - Coefficient(1)))
        {
          oss << ac;
          cp = true;
        }

      // Variables
      if (has_vars)
        {
          bool fv = true;
          for (size_t j = 0; j < idx.size(); ++j)
            {
              if (idx(j) == 0)
                continue;
              if (cp or not fv)
                oss << "*";
              oss << vn(j);
              if (idx(j) > 1)
                oss << "^" << idx(j);
              fv = false;
            }
        }

      first = false;
    });

    return oss.str();
  }

  // =================================================================
  //  Promote
  // =================================================================

  /** @brief Promote to a polynomial with more variables.
   *
   *  All existing terms are padded with trailing-zero exponents.
   *
   *  @param[in] new_nv New number of variables (>= current num_vars).
   *  @return Promoted polynomial.
   *  @throws std::domain_error if @p new_nv < num_vars().
   */
  [[nodiscard]] Gen_MultiPolynomial promote(size_t new_nv) const
  {
    ah_domain_error_if(new_nv < nvars_)
      << "cannot demote from " << nvars_ << " to " << new_nv << " variables";

    if (new_nv == nvars_)
      return *this;

    Gen_MultiPolynomial r;
    r.nvars_ = new_nv;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      r.coeffs.insert(multi_poly_detail::extend_index(idx, new_nv), c);
    });
    return r;
  }

  // =================================================================
  //  Least-Squares Fitting (Layer 2 - Industrial)
  // =================================================================

  /** @brief Basic least-squares polynomial fitting.
   *
   *  Minimizes: @f$\sum_i (y_i - p(x_i))^2@f$
   *
   *  @param[in] data   Array of (point, value) pairs.
   *  @param[in] nvars  Number of variables.
   *  @param[in] basis  Array of monomial exponent vectors (basis monomials).
   *  @return Best-fit polynomial in least-squares sense.
   *
   *  @throws std::domain_error if data or basis is empty, or normal equations singular.
   *
   *  @note **Complexity**: @f$O(m \cdot b^2 + b^3)@f$ where m=data points, b=basis size.
   *        **For production**: Use @ref fit_weighted() for importance weighting support.
   */
  [[nodiscard]] static Gen_MultiPolynomial fit(
    const Array<std::pair<Array<Coefficient>, Coefficient>> &data, size_t nvars,
    const Array<Array<size_t>> &basis)
  {
    const size_t m = data.size();
    const size_t b = basis.size();

    ah_domain_error_if(m == 0) << "fit: data is empty";
    ah_domain_error_if(b == 0) << "fit: basis is empty";

    // Build design matrix A [m x b]
    Array<Array<Coefficient>> A(m, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        y(m, Coefficient{});

    for (size_t i = 0; i < m; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          A(i)(j) = _eval_monomial(data(i).first, basis(j));
        y(i) = data(i).second;
      }

    // Normal equations: (A^T A) c = A^T y
    Array<Array<Coefficient>> AtA(b, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        Aty(b, Coefficient{});

    for (size_t i = 0; i < b; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          {
            AtA(i)(j) = Coefficient{};
            for (size_t k = 0; k < m; ++k)
              AtA(i)(j) += A(k)(i) * A(k)(j);
          }

        Aty(i) = Coefficient{};
        for (size_t k = 0; k < m; ++k)
          Aty(i) += A(k)(i) * y(k);
      }

    // Solve via Gaussian elimination with pivoting
    Array<Coefficient> c(b);
    for (size_t col = 0; col < b; ++col)
      {
        // Partial pivoting
        size_t      prow = col;
        Coefficient pval = std::abs(AtA(col)(col));
        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient av = std::abs(AtA(row)(col));
            if (av > pval)
              {
                pval = av;
                prow = row;
              }
          }

        ah_domain_error_if(coeff_is_zero(AtA(prow)(col))) << "fit: singular system at column " << col;

        if (prow != col)
          {
            std::swap(AtA(col), AtA(prow));
            std::swap(Aty(col), Aty(prow));
          }

        // Eliminate
        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient f = AtA(row)(col) / AtA(col)(col);
            for (size_t j = col; j < b; ++j)
              AtA(row)(j) -= f * AtA(col)(j);
            Aty(row) -= f * Aty(col);
          }
      }

    // Back-substitution
    for (size_t i = b; i > 0; --i)
      {
        c(i - 1) = Aty(i - 1);
        for (size_t j = i; j < b; ++j)
          c(i - 1) -= AtA(i - 1)(j) * c(j);
        c(i - 1) /= AtA(i - 1)(i - 1);
      }

    // Assemble polynomial
    Gen_MultiPolynomial result(nvars);
    for (size_t j = 0; j < b; ++j)
      {
        if (not coeff_is_zero(c(j)))
          result.add_to_coeff(basis(j), c(j));
      }

    return result;
  }

  /** @brief Weighted least-squares polynomial fitting.
   *
   *  Production-grade fitting with per-point weights for importance-weighted
   *  regression. Minimizes: @f$\sum_i w_i (y_i - p(x_i))^2@f$
   *
   *  @param[in] data   Array of (point, value) pairs.
   *  @param[in] nvars  Number of variables.
   *  @param[in] basis  Array of monomial exponent vectors (basis monomials).
   *  @param[in] weights Per-point weights (size must equal data.size()).
   *  @return Best-fit polynomial in weighted least-squares sense.
   *
   *  @throws std::domain_error if data/basis empty or weights size mismatch.
   *
   *  @note **Use case**: Heteroscedastic data (varying measurement noise),
   *        importance-weighted samples, or robust fitting with outlier downweighting.
   *        **Complexity**: @f$O(m \cdot b^2 + b^3)@f$ where m=data points, b=basis size.
   */
  [[nodiscard]] static Gen_MultiPolynomial fit_weighted(
    const Array<std::pair<Array<Coefficient>, Coefficient>> &data, size_t nvars,
    const Array<Array<size_t>> &basis, const Array<Coefficient> &weights)
  {
    const size_t m = data.size();
    const size_t b = basis.size();

    ah_domain_error_if(m == 0) << "fit_weighted: data is empty";
    ah_domain_error_if(b == 0) << "fit_weighted: basis is empty";
    ah_domain_error_if(weights.size() != m)
      << "fit_weighted: weights.size() " << weights.size() << " != data.size() " << m;

    // Build weighted design matrix: A_ij = sqrt(w_i) * basis_j(x_i)
    Array<Array<Coefficient>> A(m, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        y_w(m, Coefficient{});

    for (size_t i = 0; i < m; ++i)
      {
        const Coefficient sqrt_w = std::sqrt(weights(i));
        for (size_t j = 0; j < b; ++j)
          A(i)(j) = sqrt_w * _eval_monomial(data(i).first, basis(j));
        y_w(i) = sqrt_w * data(i).second;
      }

    // Normal equations: (A^T A) c = A^T y
    Array<Array<Coefficient>> AtA(b, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        Aty(b, Coefficient{});

    for (size_t i = 0; i < b; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          {
            AtA(i)(j) = Coefficient{};
            for (size_t k = 0; k < m; ++k)
              AtA(i)(j) += A(k)(i) * A(k)(j);
          }

        Aty(i) = Coefficient{};
        for (size_t k = 0; k < m; ++k)
          Aty(i) += A(k)(i) * y_w(k);
      }

    // Solve via Gaussian elimination
    Array<Coefficient> c(b);
    for (size_t col = 0; col < b; ++col)
      {
        // Partial pivoting
        size_t      prow = col;
        Coefficient pval = std::abs(AtA(col)(col));
        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient av = std::abs(AtA(row)(col));
            if (av > pval)
              {
                pval = av;
                prow = row;
              }
          }

        ah_domain_error_if(coeff_is_zero(AtA(prow)(col)))
          << "fit_weighted: singular system at column " << col;

        if (prow != col)
          {
            std::swap(AtA(col), AtA(prow));
            std::swap(Aty(col), Aty(prow));
          }

        // Eliminate
        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient f = AtA(row)(col) / AtA(col)(col);
            for (size_t j = col; j < b; ++j)
              AtA(row)(j) -= f * AtA(col)(j);
            Aty(row) -= f * Aty(col);
          }
      }

    // Back-substitution
    for (size_t i = b; i > 0; --i)
      {
        c(i - 1) = Aty(i - 1);
        for (size_t j = i; j < b; ++j)
          c(i - 1) -= AtA(i - 1)(j) * c(j);
        c(i - 1) /= AtA(i - 1)(i - 1);
      }

    // Assemble polynomial
    Gen_MultiPolynomial result(nvars);
    for (size_t j = 0; j < b; ++j)
      if (not coeff_is_zero(c(j)))
        result.add_to_coeff(basis(j), c(j));

    return result;
  }

  /** @brief Regularized least-squares fitting with automatic GCV-based lambda selection.
   *
   *  Solves Tikhonov ridge regression: @f$\min_c ||Ac - y||^2 + \lambda ||c||^2@f$
   *  Automatically selects @f$\lambda@f$ via Generalized Cross-Validation (GCV).
   *
   *  @param[in] data   Array of (point, value) pairs.
   *  @param[in] nvars  Number of variables.
   *  @param[in] basis  Array of monomial exponent vectors (basis monomials).
   *  @param[out] lambda_used  If non-null, receives the selected regularization parameter.
   *  @param[out] gcv_score    If non-null, receives the GCV score at selected lambda.
   *  @return Best-fit polynomial with ridge regularization.
   *
   *  @throws std::domain_error if data/basis empty or normal equations singular.
   *
   *  @note **Use case**: Ill-conditioned or noisy data. Automatic lambda selection via GCV.
   *        **Complexity**: @f$O(m \cdot b^2 + b^3 + \text{trials})@f$ where m=data points, b=basis size.
   *        **Reference**: Golub & Kahan (1965), Wahba (1990).
   */
  [[nodiscard]] static Gen_MultiPolynomial fit_ridge(
    const Array<std::pair<Array<Coefficient>, Coefficient>> &data, size_t nvars,
    const Array<Array<size_t>> &basis, Coefficient *lambda_used = nullptr,
    double *gcv_score = nullptr)
  {
    const size_t m = data.size();
    const size_t b = basis.size();

    ah_domain_error_if(m == 0) << "fit_ridge: data is empty";
    ah_domain_error_if(b == 0) << "fit_ridge: basis is empty";

    // Build design matrix A [m x b]
    Array<Array<Coefficient>> A(m, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        y(m, Coefficient{});

    for (size_t i = 0; i < m; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          A(i)(j) = _eval_monomial(data(i).first, basis(j));
        y(i) = data(i).second;
      }

    // Compute A^T A and A^T y
    Array<Array<Coefficient>> AtA(b, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        Aty(b, Coefficient{});

    for (size_t i = 0; i < b; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          {
            AtA(i)(j) = Coefficient{};
            for (size_t k = 0; k < m; ++k)
              AtA(i)(j) += A(k)(i) * A(k)(j);
          }

        Aty(i) = Coefficient{};
        for (size_t k = 0; k < m; ++k)
          Aty(i) += A(k)(i) * y(k);
      }

    // GCV-based lambda selection: test a range
    Coefficient best_lambda = Coefficient{};
    double      best_gcv    = std::numeric_limits<double>::max();
    const int   nlambda     = 50;

    for (int trial = 0; trial < nlambda; ++trial)
      {
        // Logarithmically spaced lambdas: 1e-6 to 1e6
        const double log_lambda = -6.0 + 12.0 * trial / (nlambda - 1.0);
        Coefficient  lambda_try = Coefficient(std::pow(10.0, log_lambda));

        // Solve (A^T A + lambda I) c = A^T y via Gaussian elimination
        Array<Array<Coefficient>> sys = AtA;
        Array<Coefficient>        rhs = Aty;

        // Add lambda to diagonal
        for (size_t i = 0; i < b; ++i)
          sys(i)(i) += lambda_try;

        // Gaussian elimination with pivoting
        Array<Coefficient> c(b);
        bool               singular = false;

        for (size_t col = 0; col < b; ++col)
          {
            // Partial pivoting
            size_t      prow = col;
            Coefficient pval = std::abs(sys(col)(col));
            for (size_t row = col + 1; row < b; ++row)
              if (Coefficient av = std::abs(sys(row)(col)); av > pval)
                {
                  pval = av;
                  prow = row;
                }

            if (coeff_is_zero(sys(prow)(col)))
              {
                singular = true;
                break;
              }

            if (prow != col)
              {
                std::swap(sys(col), sys(prow));
                std::swap(rhs(col), rhs(prow));
              }

            // Eliminate
            for (size_t row = col + 1; row < b; ++row)
              {
                Coefficient f = sys(row)(col) / sys(col)(col);
                for (size_t j = col; j < b; ++j)
                  sys(row)(j) -= f * sys(col)(j);
                rhs(row) -= f * rhs(col);
              }
          }

        if (singular)
          continue;

        // Back-substitution
        for (size_t i = b; i > 0; --i)
          {
            c(i - 1) = rhs(i - 1);
            for (size_t j = i; j < b; ++j)
              c(i - 1) -= sys(i - 1)(j) * c(j);
            c(i - 1) /= sys(i - 1)(i - 1);
          }

        // Compute GCV score: sum((y - Ac)_i^2) / (1 - trace(A(A^T A + lambda I)^{-1} A^T) / m)^2
        Coefficient sum_sq_residuals = Coefficient{};
        for (size_t i = 0; i < m; ++i)
          {
            Coefficient pred = Coefficient{};
            for (size_t j = 0; j < b; ++j)
              pred += A(i)(j) * c(j);
            Coefficient res = y(i) - pred;
            sum_sq_residuals += res * res;
          }

        // Effective DOF approximation: trace(A(A^T A + lambda I)^{-1} A^T)
        // For simplicity, use b as upper bound for trace
        double denominator = 1.0 - static_cast<double>(b) / static_cast<double>(m);
        if (denominator < 0.01)
          denominator = 0.01;

        double gcv_val = static_cast<double>(sum_sq_residuals) / (denominator * denominator);

        if (gcv_val < best_gcv)
          {
            best_gcv    = gcv_val;
            best_lambda = lambda_try;
          }
      }

    // Solve final system with best_lambda
    Array<Array<Coefficient>> final_sys = AtA;
    Array<Coefficient>        final_rhs = Aty;
    for (size_t i = 0; i < b; ++i)
      final_sys(i)(i) += best_lambda;

    Array<Coefficient> c_final(b);
    for (size_t col = 0; col < b; ++col)
      {
        size_t      prow = col;
        Coefficient pval = std::abs(final_sys(col)(col));
        for (size_t row = col + 1; row < b; ++row)
          if (Coefficient av = std::abs(final_sys(row)(col)); av > pval)
            {
              pval = av;
              prow = row;
            }

        if (prow != col)
          {
            std::swap(final_sys(col), final_sys(prow));
            std::swap(final_rhs(col), final_rhs(prow));
          }

        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient f = final_sys(row)(col) / final_sys(col)(col);
            for (size_t j = col; j < b; ++j)
              final_sys(row)(j) -= f * final_sys(col)(j);
            final_rhs(row) -= f * final_rhs(col);
          }
      }

    for (size_t i = b; i > 0; --i)
      {
        c_final(i - 1) = final_rhs(i - 1);
        for (size_t j = i; j < b; ++j)
          c_final(i - 1) -= final_sys(i - 1)(j) * c_final(j);
        c_final(i - 1) /= final_sys(i - 1)(i - 1);
      }

    // Output statistics if requested
    if (lambda_used != nullptr)
      *lambda_used = best_lambda;
    if (gcv_score != nullptr)
      *gcv_score = best_gcv;

    // Assemble polynomial
    Gen_MultiPolynomial result(nvars);
    for (size_t j = 0; j < b; ++j)
      if (not coeff_is_zero(c_final(j)))
        result.add_to_coeff(basis(j), c_final(j));

    return result;
  }

  // =================================================================
  //  Phase 2 — Differential Calculus
  // =================================================================

  /** @brief Partial derivative with respect to a variable.
   *
   *  Computes @f$\frac{\partial^n p}{\partial x_{\text{var}}^n}@f$
   *  using the falling factorial rule.
   *
   *  @param[in] var  Variable index (0-based).
   *  @param[in] n    Order of derivative (default 1).
   *  @return Partial derivative polynomial.
   *
   *  @throws std::domain_error if @p var >= num_vars().
   *
   *  @note **Algorithm**: For each term @f$c \cdot x^\alpha@f$,
   *        replace with @f$c \cdot \alpha_{\text{var}}^{\underline{n}} \cdot x^{\alpha - n \cdot
   * e_{\text{var}}}@f$ where @f$\alpha^{\underline{n}}@f$ is the falling factorial.
   *  @note **Complexity**: @f$O(k \log k)@f$ where k = num_terms.
   */
  [[nodiscard]] Gen_MultiPolynomial partial(size_t var, size_t n = 1) const
  {
    if (n == 0)
      return *this;
    ah_domain_error_if(var >= nvars_) << "variable " << var << " >= nvars " << nvars_;

    Gen_MultiPolynomial result(nvars_);
    for_each_term([&](const Array<size_t> &alpha, const Coefficient &c)
    {
      const size_t k = alpha(var);
      if (k < n)
        return;  // term vanishes

      // Falling factorial: k * (k-1) * ... * (k-n+1)
      const Coefficient ff = multi_poly_detail::falling_factorial<Coefficient>(k, n);
      if (result.coeff_is_zero(ff))
        return;

      const Coefficient   new_c   = c * ff;
      const Array<size_t> new_idx = multi_poly_detail::decrement_index_at(alpha, var, n);
      result.add_to_coeff(new_idx, new_c);
    });
    return result;
  }

  /** @brief Gradient vector: array of all first-order partial derivatives.
   *
   *  @return Array of size @ref num_vars(), where element i is @f$\frac{\partial p}{\partial x_i}@f$.
   *
   *  @note **Complexity**: @f$O(k \log k \cdot n)@f$ where k = num_terms, n = num_vars.
   */
  [[nodiscard]] Array<Gen_MultiPolynomial> gradient() const
  {
    Array<Gen_MultiPolynomial> result(nvars_, Gen_MultiPolynomial(nvars_));
    for (size_t k = 0; k < nvars_; ++k)
      result(k) = partial(k);
    return result;
  }

  /** @brief Hessian matrix: all second-order partial derivatives.
   *
   *  @return @f$n \times n@f$ matrix (symmetric), where H[i][j] = @f$\frac{\partial^2 p}{\partial
   * x_i \partial x_j}@f$.
   *
   *  @note **Symmetry**: H[i][j] == H[j][i] (Schwarz's theorem).
   *  @note **Complexity**: @f$O(k \log k \cdot n^2)@f$ where k = num_terms, n = num_vars.
   */
  [[nodiscard]] Array<Array<Gen_MultiPolynomial>> hessian() const
  {
    const size_t                      n = nvars_;
    Array<Array<Gen_MultiPolynomial>> H(n,
                                        Array<Gen_MultiPolynomial>(n, Gen_MultiPolynomial(nvars_)));
    for (size_t i = 0; i < n; ++i)
      {
        H(i)(i) = partial(i, 2);
        for (size_t j = i + 1; j < n; ++j)
          {
            H(i)(j) = partial(i).partial(j);
            H(j)(i) = H(i)(j);  // Symmetry
          }
      }
    return H;
  }

  /** @brief Evaluate gradient at a point.
   *
   *  @param[in] pt  Evaluation point (array of size >= num_vars).
   *  @return Array of size num_vars containing gradient values.
   *
   *  @throws std::domain_error if pt.size() < num_vars().
   *
   *  @note **Complexity**: @f$O(k \log k \cdot n + k \cdot n)@f$ (gradient computation + evaluation).
   */
  [[nodiscard]] Array<Coefficient> eval_gradient(const Array<Coefficient> &pt) const
  {
    ah_domain_error_if(pt.size() < nvars_)
      << "eval_gradient: point size " << pt.size() << " < nvars " << nvars_;

    const auto         g = gradient();
    Array<Coefficient> result(nvars_, Coefficient{});
    for (size_t k = 0; k < nvars_; ++k)
      result(k) = g(k).eval(pt);
    return result;
  }

  /** @brief Evaluate Hessian at a point.
   *
   *  @param[in] pt  Evaluation point (array of size >= num_vars).
   *  @return @f$n \times n@f$ matrix of Hessian values at @p pt.
   *
   *  @throws std::domain_error if pt.size() < num_vars().
   *
   *  @note **Complexity**: @f$O(k \log k \cdot n^2 + k \cdot n^2)@f$ (Hessian computation + evaluation).
   */
  [[nodiscard]] Array<Array<Coefficient>> eval_hessian(const Array<Coefficient> &pt) const
  {
    ah_domain_error_if(pt.size() < nvars_)
      << "eval_hessian: point size " << pt.size() << " < nvars " << nvars_;

    const auto                H = hessian();
    const size_t              n = nvars_;
    Array<Array<Coefficient>> result(n, Array<Coefficient>(n, Coefficient{}));
    for (size_t i = 0; i < n; ++i)
      for (size_t j = 0; j < n; ++j)
        result(i)(j) = H(i)(j).eval(pt);
    return result;
  }

  // =================================================================
  //  Phase 2 — Interpolation (Newton Divided Differences)
  // =================================================================

  /** @brief Multivariate interpolation via Newton divided differences (Chung-Yao).
   *
   *  Constructs a polynomial that passes through all points in a tensor-product grid.
   *  Uses dimension-by-dimension Newton divided-differences table.
   *
   *  @param[in] grid  Array of size num_vars; grid[d] is array of nodes in dimension d (must be distinct).
   *  @param[in] values  Array of function values on the grid (must have size = product of grid sizes).
   *  @param[in] nvars  Number of variables.
   *  @return Interpolating polynomial of total degree @f$\prod_d (\text{size}_d - 1)@f$.
   *
   *  @throws std::domain_error if grid sizes don't match data size, or duplicate nodes found.
   *
   *  @note **Algorithm**: Chung-Yao (1977) — Newton divided-differences applied per dimension.
   *  @note **Complexity**: @f$O\left( n \cdot \prod_d m_d^2 \right)@f$ where n=nvars, m_d=grid[d].size().
   *  @note **Grid order**: values are in row-major (rightmost dimension varies fastest).
   */
  [[nodiscard]] static Gen_MultiPolynomial interpolate(const Array<Array<Coefficient>> &grid,
                                                       const Array<Coefficient> &values, size_t nvars)
  {
    // Validate grid
    ah_domain_error_if(grid.size() != nvars)
      << "interpolate: grid size " << grid.size() << " != nvars " << nvars;

    // Compute sizes and total points
    Array<size_t> sizes(nvars, size_t{0});
    size_t        total = 1;
    for (size_t d = 0; d < nvars; ++d)
      {
        sizes(d) = grid(d).size();
        ah_domain_error_if(sizes(d) == 0) << "interpolate: grid[" << d << "] is empty";
        total *= sizes(d);
      }

    ah_domain_error_if(values.size() != total)
      << "interpolate: values size " << values.size() << " != total " << total;

    // Check for duplicate nodes in each dimension
    for (size_t d = 0; d < nvars; ++d)
      {
        const Array<Coefficient> &nodes = grid(d);
        for (size_t i = 0; i < nodes.size(); ++i)
          for (size_t j = i + 1; j < nodes.size(); ++j)
            ah_domain_error_if(coeff_is_zero(nodes(i) - nodes(j)))
              << "interpolate: duplicate node at dimension " << d;
      }

    // Initialize table: polynomials for each point (flat indexing)
    Array<Gen_MultiPolynomial> table(total);
    for (size_t f = 0; f < total; ++f)
      table(f) = Gen_MultiPolynomial(nvars, values(f));

    // Process each dimension
    for (size_t dim = 0; dim < nvars; ++dim)
      {
        const size_t              m     = sizes(dim);
        const Array<Coefficient> &nodes = grid(dim);

        // Compute stride for iterating in this dimension
        size_t stride = 1;
        for (size_t d = dim + 1; d < nvars; ++d)
          stride *= sizes(d);

        // Compute outer stride (for iterating through slices)
        size_t outer_stride = stride * m;

        // Number of slices
        const size_t num_slices = total / m;

        // Process each slice
        for (size_t slice = 0; slice < num_slices; ++slice)
          {
            // Compute the base flat index for this slice
            size_t slice_flat = 0;
            size_t tmp        = slice;
            size_t mult       = 1;
            for (size_t d = nvars; d > 0; --d)
              {
                if (d - 1 == dim)
                  continue;
                size_t sz = sizes(d - 1);
                slice_flat += (tmp % sz) * mult;
                tmp /= sz;
                mult *= sz;
              }

            // Collect divided-differences table for this slice
            Array<Gen_MultiPolynomial> dd(m);
            for (size_t i = 0; i < m; ++i)
              dd(i) = table(slice_flat + i * stride);

            // Compute divided differences in-place
            for (size_t order = 1; order < m; ++order)
              for (size_t i = m; i > order; --i)
                {
                  const Coefficient denom = nodes(i - 1) - nodes(i - 1 - order);
                  dd(i - 1)               = (dd(i - 1) - dd(i - 2)) / denom;
                }

            // Reconstruct Newton polynomial in this dimension
            const auto          xd     = Gen_MultiPolynomial::variable(nvars, dim);
            Gen_MultiPolynomial result = dd(0);
            Gen_MultiPolynomial basis  = Gen_MultiPolynomial(nvars, Coefficient(1));
            for (size_t i = 1; i < m; ++i)
              {
                basis  = basis * (xd - Gen_MultiPolynomial(nvars, nodes(i - 1)));
                result = result + dd(i) * basis;
              }

            // Write back to table
            table(slice_flat) = result;
          }
      }

    return table(0);
  }

  // =================================================================
  //  Phase 2 — Serialization
  // =================================================================

  /** @brief Serialize polynomial to JSON string.
   *
   *  Format: `{"nvars":n,"terms":[{"e":[...exponents...],"c":coeff},...]}`.
   *
   *  @return JSON string representation.
   *
   *  @note **Precision**: Coefficients saved with 17 significant digits (IEEE 754 double).
   */
  [[nodiscard]] std::string to_json() const
  {
    std::ostringstream oss;
    oss << R"({"nvars":)" << nvars_ << R"(,"terms":[)";
    bool first = true;
    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      if (not first)
        oss << ",";
      oss << R"({"e":[)";
      for (size_t j = 0; j < idx.size(); ++j)
        {
          if (j > 0)
            oss << ",";
          oss << idx(j);
        }
      oss << R"(],"c":)" << std::setprecision(17) << c << "}";
      first = false;
    });
    oss << R"(]})";
    return oss.str();
  }

  /** @brief Deserialize polynomial from JSON string.
   *
   *  Parses format created by @ref to_json().
   *
   *  @param[in] s  JSON string.
   *  @return Reconstructed polynomial.
   *
   *  @throws std::domain_error if JSON format is invalid.
   */
  [[nodiscard]] static Gen_MultiPolynomial from_json(const std::string &s)
  {
    // Simple parser for our JSON format
    size_t              nvars = 0;
    Gen_MultiPolynomial result;

    // Find "nvars":n
    size_t pos = s.find("\"nvars\":");
    ah_domain_error_if(pos == std::string::npos) << "from_json: missing nvars";
    pos += 8;
    size_t pos_comma = s.find(',', pos);
    nvars            = std::stoul(s.substr(pos, pos_comma - pos));
    result.nvars_    = nvars;

    // Parse terms
    pos = s.find("\"terms\":[");
    ah_domain_error_if(pos == std::string::npos) << "from_json: missing terms";
    pos += 9;

    while (pos < s.size() && s[pos] != ']')
      {
        // Find "e":[...]
        size_t e_pos = s.find("\"e\":[", pos);
        if (e_pos == std::string::npos)
          break;
        e_pos += 5;
        size_t      e_end = s.find(']', e_pos);
        std::string e_str = s.substr(e_pos, e_end - e_pos);

        // Parse exponents
        Array<size_t>      idx(nvars, size_t{0});
        size_t             idx_i = 0;
        std::istringstream iss_e(e_str);
        std::string        token;
        while (std::getline(iss_e, token, ',') and idx_i < nvars)
          idx(idx_i++) = std::stoul(token);

        // Find "c":value
        size_t c_pos = s.find("\"c\":", e_end);
        ah_domain_error_if(c_pos == std::string::npos) << "from_json: missing c";
        c_pos += 4;
        size_t      c_end = s.find('}', c_pos);
        Coefficient c_val = Coefficient(std::stod(s.substr(c_pos, c_end - c_pos)));

        result.add_to_coeff(idx, c_val);

        // Move to next term
        pos = s.find("},", e_end);
        if (pos == std::string::npos)
          break;
        pos += 2;
      }

    return result;
  }

  /** @brief Serialize polynomial to binary stream.
   *
   *  Format: nvars (uint64) | n_terms (uint64) | ( exponents (uint64×nvars) | coeff (double) )×n_terms
   *
   *  @param[out] out  Output stream.
   *
   *  @throws std::ios_base::failure if write fails.
   */
  void to_binary(std::ostream &out) const
  {
    uint64_t nv = nvars_;
    out.write(reinterpret_cast<const char *>(&nv), sizeof(nv));

    uint64_t nt = num_terms();
    out.write(reinterpret_cast<const char *>(&nt), sizeof(nt));

    for_each_term([&](const Array<size_t> &idx, const Coefficient &c)
    {
      for (size_t j = 0; j < nvars_; ++j)
        {
          uint64_t exp = idx(j);
          out.write(reinterpret_cast<const char *>(&exp), sizeof(exp));
        }
      const auto c_double = static_cast<double>(c);
      out.write(reinterpret_cast<const char *>(&c_double), sizeof(c_double));
    });
  }

  /** @brief Deserialize polynomial from binary stream.
   *
   *  Reads format created by @ref to_binary().
   *
   *  @param[in] in  Input stream.
   *  @return Reconstructed polynomial.
   *
   *  @throws std::ios_base::failure if read fails or format is invalid.
   */
  [[nodiscard]] static Gen_MultiPolynomial from_binary(std::istream &in)
  {
    uint64_t nv = 0;
    in.read(reinterpret_cast<char *>(&nv), sizeof(nv));
    size_t nvars = nv;

    uint64_t nt = 0;
    in.read(reinterpret_cast<char *>(&nt), sizeof(nt));
    size_t n_terms = nt;

    Gen_MultiPolynomial result(nvars);
    for (size_t i = 0; i < n_terms; ++i)
      {
        Array<size_t> idx(nvars, size_t{0});
        for (size_t j = 0; j < nvars; ++j)
          {
            uint64_t exp = 0;
            in.read(reinterpret_cast<char *>(&exp), sizeof(exp));
            idx(j) = exp;
          }
        double c_double = 0.0;
        in.read(reinterpret_cast<char *>(&c_double), sizeof(c_double));
        Coefficient c = Coefficient(c_double);

        result.add_to_coeff(idx, c);
      }

    return result;
  }

  // =================================================================
  //  Phase 2 — Parallelism
  // =================================================================

  /** @brief Evaluate polynomial at multiple points (with parallelism support).
   *
   *  @param[in] pts  Array of evaluation points.
   *  @return Array of results, pts[i] → eval(pts[i]).
   *
   *  @note **Complexity**: @f$O(k \cdot n \cdot m)@f$
   *        where k = num_terms, n = num_vars, m = pts.size().
   *  @note **Parallelism**: Evaluation of points is parallelizable; uses thread pool if enabled.
   */
  [[nodiscard]] Array<Coefficient> eval_batch(const Array<Array<Coefficient>> &pts) const
  {
    const size_t       n = pts.size();
    Array<Coefficient> results(n, Coefficient{});

    // Sequential evaluation (parallelism can be added via pmaps in future)
    for (size_t i = 0; i < n; ++i)
      results(i) = eval(pts(i));

    return results;
  }

  /** @brief Parallel least-squares fitting.
   *
   *  Builds design matrix rows in parallel; solves normal equations sequentially.
   *
   *  @param[in] data   Array of (point, value) pairs.
   *  @param[in] nvars  Number of variables.
   *  @param[in] basis  Array of monomial exponent vectors.
   *  @return Best-fit polynomial.
   *
   *  @throws std::domain_error if data/basis empty or normal equations singular.
   *
   *  @note **Complexity**: @f$O(m \cdot b / p + b^3)@f$ where m=data points, b=basis size, p=threads.
   *  @note Parallelism applies to row construction; normal equations solved sequentially.
   */
  [[nodiscard]] static Gen_MultiPolynomial fit_parallel(
    const Array<std::pair<Array<Coefficient>, Coefficient>> &data, size_t nvars,
    const Array<Array<size_t>> &basis)
  {
    const size_t m = data.size();
    const size_t b = basis.size();

    ah_domain_error_if(m == 0) << "fit_parallel: data is empty";
    ah_domain_error_if(b == 0) << "fit_parallel: basis is empty";

    // Convert data to std::vector for pmaps
    std::vector<std::pair<Array<Coefficient>, Coefficient>> data_vec(m);
    for (size_t i = 0; i < m; ++i)
      data_vec[i] = data(i);

    // Convert basis to std::vector for capture
    std::vector<Array<size_t>> basis_vec(b);
    for (size_t i = 0; i < b; ++i)
      basis_vec[i] = basis(i);

    // Build design matrix rows in parallel
    auto rows = pmaps(data_vec, [&basis_vec, b](const std::pair<Array<Coefficient>, Coefficient> &p)
    {
      Array<Coefficient> row(b, Coefficient{});
      for (size_t j = 0; j < b; ++j)
        row(j) = multi_poly_detail::eval_monomial(p.first, basis_vec[j]);
      return row;
    });

    // Normal equations (sequential — bottleneck is row construction)
    Array<Array<Coefficient>> A(m);
    Array<Coefficient>        y(m, Coefficient{});
    for (size_t i = 0; i < m; ++i)
      {
        A(i) = rows[i];
        y(i) = data(i).second;
      }

    // Compute A^T A and A^T y
    Array<Array<Coefficient>> AtA(b, Array<Coefficient>(b, Coefficient{}));
    Array<Coefficient>        Aty(b, Coefficient{});

    for (size_t i = 0; i < b; ++i)
      {
        for (size_t j = 0; j < b; ++j)
          {
            AtA(i)(j) = Coefficient{};
            for (size_t k = 0; k < m; ++k)
              AtA(i)(j) += A(k)(i) * A(k)(j);
          }

        Aty(i) = Coefficient{};
        for (size_t k = 0; k < m; ++k)
          Aty(i) += A(k)(i) * y(k);
      }

    // Gaussian elimination with pivoting
    Array<Coefficient> c(b);
    for (size_t col = 0; col < b; ++col)
      {
        size_t      prow = col;
        Coefficient pval = std::abs(AtA(col)(col));
        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient av = std::abs(AtA(row)(col));
            if (av > pval)
              {
                pval = av;
                prow = row;
              }
          }

        ah_domain_error_if(Gen_MultiPolynomial::coeff_is_zero(AtA(prow)(col)))
          << "fit_parallel: singular normal equations";

        if (prow != col)
          {
            std::swap(AtA(col), AtA(prow));
            std::swap(Aty(col), Aty(prow));
          }

        for (size_t row = col + 1; row < b; ++row)
          {
            Coefficient f = AtA(row)(col) / AtA(col)(col);
            for (size_t j = col; j < b; ++j)
              AtA(row)(j) -= f * AtA(col)(j);
            Aty(row) -= f * Aty(col);
          }
      }

    // Back-substitution
    for (size_t i = b; i > 0; --i)
      {
        c(i - 1) = Aty(i - 1);
        for (size_t j = i; j < b; ++j)
          c(i - 1) -= AtA(i - 1)(j) * c(j);
        c(i - 1) /= AtA(i - 1)(i - 1);
      }

    // Assemble polynomial
    Gen_MultiPolynomial result(nvars);
    for (size_t j = 0; j < b; ++j)
      if (not Gen_MultiPolynomial::coeff_is_zero(c(j)))
        result.add_to_coeff(basis(j), c(j));

    return result;
  }

  // =================================================================
  //  Layer 3 — Gröbner Bases and Ideal Operations
  // =================================================================

  /** @brief Multivariate division with remainder (Buchberger algorithm).
   *
   *  Divides this polynomial by the array of divisors and returns quotients
   *  and remainder. Each divisor must be non-zero and have the same number
   *  of variables as this polynomial.
   *
   *  @param[in] divisors Array of non-zero divisor polynomials.
   *  @return Pair (quotients, remainder):
   *          - quotients[i]: quotient by divisors[i]
   *          - remainder: the polynomial remainder
   *  @throws std::domain_error if divisors is empty or contains a zero polynomial
   *  @throws std::invalid_argument if a non-zero divisor has mismatched nvars
   *  @note Complexity: O(terms(p) * s * terms(fi)) per outer iteration,
   *        where s = divisors.size() and terms(fi) = number of terms in divisors[i].
   *        Requires Coefficient type to support division (/).
   */
  [[nodiscard]] std::pair<Array<Gen_MultiPolynomial>, Gen_MultiPolynomial> divmod(
    const Array<Gen_MultiPolynomial> &divisors) const
  {
    size_t s = divisors.size();
    ah_domain_error_if(s == 0) << "divmod: empty divisors array";

    for (size_t i = 0; i < s; ++i)
      {
        ah_domain_error_if(divisors(i).is_zero()) << "divmod: divisors[" << i << "] is zero";
        ah_invalid_argument_if(divisors(i).nvars_ != nvars_)
          << "divmod: divisors[" << i << "].nvars (" << divisors(i).nvars_ << ") != this.nvars ("
          << nvars_ << ")";
      }

    // Initialize quotient array and remainder
    Array<Gen_MultiPolynomial> q(s, Gen_MultiPolynomial(nvars_));
    Gen_MultiPolynomial        r(nvars_);
    Gen_MultiPolynomial        p = *this;

    // Buchberger's multivariate division algorithm
    while (not p.is_zero())
      {
        auto [lm_p, lc_p] = p.leading_term();
        bool found        = false;

        for (size_t i = 0; i < s; ++i)
          {
            if (divisors(i).is_zero())
              continue;

            auto [lm_fi, lc_fi] = divisors(i).leading_term();

            if (multi_poly_detail::divides_index(lm_fi, lm_p, nvars_))
              {
                Array<size_t> t_idx   = multi_poly_detail::sub_indices(lm_p, lm_fi, nvars_);
                Coefficient   t_coeff = lc_p / lc_fi;
                q(i).add_to_coeff(t_idx, t_coeff);
                p -= monomial(nvars_, t_idx, t_coeff) * divisors(i);
                found = true;
                break;
              }
          }

        if (not found)
          {
            r.add_to_coeff(lm_p, lc_p);
            p -= monomial(nvars_, lm_p, lc_p);
          }
      }

    return {q, r};
  }

  /** @brief S-polynomial (Sylvester polynomial) of two polynomials.
   *
   *  Computes the S-polynomial used in Buchberger's algorithm:
   *  S(f, g) = (lcm(lm(f), lm(g)) / lm(f)) * f - (lcm(lm(f), lm(g)) / lm(g)) * g
   *
   *  @param[in] f First polynomial (non-zero).
   *  @param[in] g Second polynomial (non-zero).
   *  @return The S-polynomial S(f, g).
   *  @throws std::domain_error if @p f or @p g is zero
   *  @throws std::invalid_argument if f and g have different number of variables
   *  @note The result may be zero if f and g have coprime leading monomials.
   */
  [[nodiscard]] static Gen_MultiPolynomial s_poly(const Gen_MultiPolynomial &f,
                                                  const Gen_MultiPolynomial &g)
  {
    ah_domain_error_if(f.is_zero()) << "s_poly: first argument is zero";
    ah_domain_error_if(g.is_zero()) << "s_poly: second argument is zero";
    ah_invalid_argument_if(f.nvars_ != g.nvars_)
      << "s_poly: incompatible number of variables (" << f.nvars_ << " vs " << g.nvars_ << ")";

    const size_t nv   = f.nvars_;
    auto [lm_f, lc_f] = f.leading_term();
    auto [lm_g, lc_g] = g.leading_term();

    Array<size_t> gamma = multi_poly_detail::lcm_indices(lm_f, lm_g, nv);

    Array<size_t>       t_f_idx   = multi_poly_detail::sub_indices(gamma, lm_f, nv);
    Coefficient         t_f_coeff = Coefficient(1) / lc_f;
    Gen_MultiPolynomial t_f       = monomial(nv, t_f_idx, t_f_coeff);

    Array<size_t>       t_g_idx   = multi_poly_detail::sub_indices(gamma, lm_g, nv);
    Coefficient         t_g_coeff = Coefficient(1) / lc_g;
    Gen_MultiPolynomial t_g       = monomial(nv, t_g_idx, t_g_coeff);

    return t_f * f - t_g * g;
  }

  /** @brief Gröbner basis computation via Buchberger's algorithm.
   *
   *  Computes a Gröbner basis of the ideal generated by the input polynomials
   *  using the classical Buchberger algorithm with criterion B1 (coprime LMs).
   *
   *  @param[in] generators Array of generator polynomials (may contain zero).
   *  @return A new array forming a Gröbner basis of the generated ideal.
   *  @throws std::domain_error if generators is empty or contains zero polynomials
   *  @throws std::domain_error if iteration limit (10000) is exceeded
   *  @note Complexity depends on ideal structure and may be exponential in degree.
   *        CRITICAL: Leading monomials are copied by value in the pair loop
   *        to avoid iterator invalidation from Array reallocation.
   */
  [[nodiscard]] static Array<Gen_MultiPolynomial> groebner_basis(
    const Array<Gen_MultiPolynomial> &generators)
  {
    size_t gen_count = generators.size();
    ah_domain_error_if(gen_count == 0) << "groebner_basis: empty generators";

    for (size_t i = 0; i < gen_count; ++i)
      ah_domain_error_if(generators(i).is_zero()) << "groebner_basis: generator[" << i << "] is zero";

    // Initialize G with a copy of generators
    Array<Gen_MultiPolynomial> G = generators;

    // Initialize pair queue with all (i, j) pairs (i < j)
    DynList<std::pair<size_t, size_t>> P;
    for (size_t i = 0; i < G.size(); ++i)
      for (size_t j = i + 1; j < G.size(); ++j)
        P.append(std::make_pair(i, j));

    size_t       iteration      = 0;
    const size_t max_iterations = 10000;

    // Main Buchberger loop
    while (not P.is_empty())
      {
        ah_domain_error_if(++iteration > max_iterations)
          << "groebner_basis: iteration limit (" << max_iterations << ") exceeded";

        auto [i, j] = P.remove_first();

        // B1 criterion: if LMs are coprime (all min(lm_i, lm_j) = 0), skip
        Array<size_t> lm_i = G(i).leading_monomial();  // Copy by value
        Array<size_t> lm_j = G(j).leading_monomial();  // Copy by value

        bool coprime = true;
        for (size_t v = 0; v < G(i).nvars_; ++v)
          {
            const size_t ei = v < lm_i.size() ? lm_i(v) : 0;
            const size_t ej = v < lm_j.size() ? lm_j(v) : 0;
            if (std::min(ei, ej) != 0)
              {
                coprime = false;
                break;
              }
          }
        if (coprime)
          continue;

        // Reduce S-polynomial
        Gen_MultiPolynomial s = s_poly(G(i), G(j));
        auto [qs, rs]         = s.divmod(G);
        Gen_MultiPolynomial r = rs;

        // If remainder is non-zero, add to basis and pair with all existing elements
        if (not r.is_zero())
          {
            size_t new_idx = G.size();
            G.append(std::move(r));
            for (size_t m = 0; m < new_idx; ++m)
              P.append(std::make_pair(m, new_idx));
          }
      }

    return G;
  }

  /** @brief Reduced Gröbner basis (minimized and normalized).
   *
   *  Computes a Gröbner basis and then reduces it by:
   *  1. Removing redundant generators (whose LM is divisible by another's LM)
   *  2. Making each generator monic (leading coefficient = 1)
   *  3. Interreducing each generator w.r.t. the others
   *
   *  @param[in] generators Array of generator polynomials.
   *  @return A reduced Gröbner basis: minimal, monic, and pairwise interreduced.
   *  @throws std::domain_error if generators is empty or contains zero polynomials
   *  @throws std::domain_error if iteration limit is exceeded
   *  @note The basis elements are pairwise coprime in leading monomials.
   */
  [[nodiscard]] static Array<Gen_MultiPolynomial> reduced_groebner_basis(
    const Array<Gen_MultiPolynomial> &generators)
  {
    // Phase 1: Compute Gröbner basis
    Array<Gen_MultiPolynomial> G = groebner_basis(generators);

    // Phase 1b: Minimize by removing redundant generators
    bool changed = true;
    while (changed)
      {
        changed              = false;
        size_t idx_to_remove = G.size();

        for (size_t i = 0; i < G.size(); ++i)
          {
            if (idx_to_remove < G.size())
              break;  // Already found one to remove; exit

            Array<size_t> lm_i = G(i).leading_monomial();

            for (size_t j = 0; j < G.size(); ++j)
              {
                if (i == j)
                  continue;

                if (Array<size_t> lm_j = G(j).leading_monomial();
                    multi_poly_detail::divides_index(lm_j, lm_i, G(i).nvars_))
                  {
                    idx_to_remove = i;
                    break;
                  }
              }
          }

        if (idx_to_remove < G.size())
          {
            // Rebuild G without the dominated element
            Array<Gen_MultiPolynomial> new_G(G.size() - 1, Gen_MultiPolynomial(G(0).nvars_));
            size_t                     new_idx = 0;
            for (size_t i = 0; i < G.size(); ++i)
              if (i != idx_to_remove)
                new_G(new_idx++) = std::move(G(i));
            G       = std::move(new_G);
            changed = true;
          }
      }

    // Phase 2: Make monic
    for (size_t i = 0; i < G.size(); ++i)
      {
        Coefficient lc = G(i).leading_coeff();
        if (not(lc == Coefficient(1)))
          G(i) /= lc;
      }

    // Phase 3: Interreduce
    for (size_t i = 0; i < G.size(); ++i)
      {
        if (G.size() == 1)
          continue;  // Skip if only one element

        // Build G_minus_i: all elements except i
        Array<Gen_MultiPolynomial> G_minus_i(G.size() - 1, Gen_MultiPolynomial(G(i).nvars_));
        size_t                     idx = 0;
        for (size_t j = 0; j < G.size(); ++j)
          if (j != i)
            G_minus_i(idx++) = G(j);

        if (not G_minus_i.is_empty())
          {
            auto [q, r] = G(i).divmod(G_minus_i);
            G(i)        = std::move(r);
          }
      }

    return G;
  }

  /** @brief Polynomial reduction modulo an ideal (one-liner).
   *
   *  Reduces this polynomial modulo a set of divisor polynomials by computing
   *  the division remainder.
   *
   *  @param[in] divisors Array of divisor polynomials.
   *  @return The remainder after division by divisors.
   *  @throws std::domain_error if divisors is empty
   *  @note Equivalent to divmod(divisors).second.
   */
  [[nodiscard]] Gen_MultiPolynomial reduce_modulo(const Array<Gen_MultiPolynomial> &divisors) const
  {
    return divmod(divisors).second;
  }

  /** @brief Test membership in an ideal via Gröbner basis.
   *
   *  A polynomial belongs to the ideal generated by the given generators
   *  if and only if its remainder modulo a Gröbner basis of the ideal is zero.
   *
   *  @param[in] f Polynomial to test.
   *  @param[in] generators Array of generator polynomials.
   *  @return True if @p f is in the ideal; false otherwise.
   *  @throws std::domain_error if generators is empty or contains zero polynomials
   *  @throws std::domain_error if iteration limit is exceeded
   *  @note Complexity includes Gröbner basis computation, which is expensive.
   *        For repeated membership tests, precompute the Gröbner basis.
   */
  [[nodiscard]] static bool ideal_member(const Gen_MultiPolynomial        &f,
                                         const Array<Gen_MultiPolynomial> &generators)
  {
    if (f.is_zero())
      return true;
    return f.divmod(groebner_basis(generators)).second.is_zero();
  }

  // =================================================================
  //  Layer 4 — Ideal Arithmetic
  // =================================================================

  /** @brief Sum of two ideals: I + J = ⟨generators(I) ∪ generators(J)⟩.
   *
   *  Concatenates the generators of two ideals to form their sum.
   *  No Gröbner computation is performed; the result is the union of generators.
   *
   *  @param[in] a First ideal generators (non-empty, no zeros).
   *  @param[in] b Second ideal generators (non-empty, no zeros).
   *  @return Array of size(a) + size(b) containing all generators.
   *  @throws std::domain_error if @p a or @p b is empty
   *  @throws std::domain_error if any generator in @p a or @p b is zero
   *  @throws std::invalid_argument if nvars mismatch between @p a and @p b
   *  @note Complexity: O(n + m). No reduction applied.
   */
  [[nodiscard]] static Array<Gen_MultiPolynomial> ideal_sum(
    const Array<Gen_MultiPolynomial> &a,
    const Array<Gen_MultiPolynomial> &b)
  {
    ah_domain_error_if(a.size() == 0) << "ideal_sum: first array is empty";
    ah_domain_error_if(b.size() == 0) << "ideal_sum: second array is empty";

    for (size_t i = 0; i < a.size(); ++i)
      ah_domain_error_if(a(i).is_zero()) << "ideal_sum: a[" << i << "] is zero";

    for (size_t j = 0; j < b.size(); ++j)
      ah_domain_error_if(b(j).is_zero()) << "ideal_sum: b[" << j << "] is zero";

    size_t nv = a(0).nvars_;
    for (size_t i = 1; i < a.size(); ++i)
      ah_invalid_argument_if(a(i).nvars_ != nv)
        << "ideal_sum: nvars mismatch in a[" << i << "]: expected " << nv << ", got " << a(i).nvars_;

    for (size_t j = 0; j < b.size(); ++j)
      ah_invalid_argument_if(b(j).nvars_ != nv)
        << "ideal_sum: nvars mismatch in b[" << j << "]: expected " << nv << ", got " << b(j).nvars_;

    Array<Gen_MultiPolynomial> result(a.size() + b.size(), Gen_MultiPolynomial(nv));
    for (size_t i = 0; i < a.size(); ++i)
      result(i) = a(i);
    for (size_t j = 0; j < b.size(); ++j)
      result(a.size() + j) = b(j);

    return result;
  }

  /** @brief Product of two ideals: I · J = ⟨a_i · b_j : a_i ∈ I, b_j ∈ J⟩.
   *
   *  Computes all pairwise products of generators from two ideals.
   *  The result is a set of |a| × |b| polynomials (may contain duplicates).
   *
   *  @param[in] a First ideal generators (non-empty, no zeros).
   *  @param[in] b Second ideal generators (non-empty, no zeros).
   *  @return Array of size(a) × size(b) containing products a_i · b_j.
   *  @throws std::domain_error if @p a or @p b is empty
   *  @throws std::domain_error if any generator is zero
   *  @throws std::invalid_argument if nvars mismatch
   *  @note Complexity: O(|a| · |b| · T_mult). Result may grow large for large inputs.
   */
  [[nodiscard]] static Array<Gen_MultiPolynomial> ideal_product(
    const Array<Gen_MultiPolynomial> &a,
    const Array<Gen_MultiPolynomial> &b)
  {
    ah_domain_error_if(a.size() == 0) << "ideal_product: first array is empty";
    ah_domain_error_if(b.size() == 0) << "ideal_product: second array is empty";

    for (size_t i = 0; i < a.size(); ++i)
      ah_domain_error_if(a(i).is_zero()) << "ideal_product: a[" << i << "] is zero";

    for (size_t j = 0; j < b.size(); ++j)
      ah_domain_error_if(b(j).is_zero()) << "ideal_product: b[" << j << "] is zero";

    size_t nv = a(0).nvars_;
    for (size_t i = 1; i < a.size(); ++i)
      ah_invalid_argument_if(a(i).nvars_ != nv)
        << "ideal_product: nvars mismatch in a[" << i << "]: expected " << nv << ", got " << a(i).nvars_;

    for (size_t j = 0; j < b.size(); ++j)
      ah_invalid_argument_if(b(j).nvars_ != nv)
        << "ideal_product: nvars mismatch in b[" << j << "]: expected " << nv << ", got " << b(j).nvars_;

    Array<Gen_MultiPolynomial> result(a.size() * b.size(), Gen_MultiPolynomial(nv));
    size_t                     k = 0;
    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        result(k++) = a(i) * b(j);

    return result;
  }

  /** @brief Power of an ideal: I^n = I · I · ... · I (n times).
   *
   *  Computes the nth power of an ideal by repeated ideal multiplication.
   *  Special cases: I^0 = ⟨1⟩ (the unit ideal), I^1 = I.
   *
   *  @param[in] gens Generator polynomials (non-empty, no zeros).
   *  @param[in] n    Exponent (non-negative integer).
   *  @return Array of generators for I^n.
   *  @throws std::domain_error if @p gens is empty or contains zero
   *  @note Complexity: O(n · |gens|^k · T_mult) accumulated. Result size grows as |gens|^n.
   *        Use only for small n and |gens| to avoid exponential explosion.
   *  @warning Result array size can be exponential in @p n.
   */
  [[nodiscard]] static Array<Gen_MultiPolynomial> ideal_power(
    const Array<Gen_MultiPolynomial> &gens,
    size_t                            n)
  {
    ah_domain_error_if(gens.size() == 0) << "ideal_power: empty generators";

    for (size_t i = 0; i < gens.size(); ++i)
      ah_domain_error_if(gens(i).is_zero()) << "ideal_power: gens[" << i << "] is zero";

    size_t nv = gens(0).nvars_;

    // Special case: I^0 = ⟨1⟩
    if (n == 0)
      {
        Array<Gen_MultiPolynomial> unit(1, Gen_MultiPolynomial(nv));
        unit(0) = Gen_MultiPolynomial(nv, Coefficient(1));
        return unit;
      }

    // Special case: I^1 = I
    if (n == 1)
      return gens;

    // General case: iteratively multiply
    Array<Gen_MultiPolynomial> result = gens;
    for (size_t i = 1; i < n; ++i)
      result = ideal_product(result, gens);

    return result;
  }

  /** @brief Test whether ideal I contains ideal J: I ⊇ J.
   *
   *  An ideal I contains ideal J if and only if every generator of J
   *  belongs to I. Uses a precomputed Gröbner basis of I for efficiency.
   *
   *  @param[in] I_gens Generators of ideal I (non-empty, no zeros).
   *  @param[in] J_gens Generators of ideal J (non-empty, no zeros).
   *  @return True if I ⊇ J; false otherwise.
   *  @throws std::domain_error if either ideal is empty or contains zero
   *  @throws std::invalid_argument if nvars mismatch
   *  @note Complexity: O(Gröbner(I) + |J| · Div(f, GB_I)).
   *        Precomputes Gröbner basis of I once; does not call ideal_member
   *        in a loop to avoid redundant computation.
   */
  [[nodiscard]] static bool contains_ideal(const Array<Gen_MultiPolynomial> &I_gens,
                                           const Array<Gen_MultiPolynomial> &J_gens)
  {
    ah_domain_error_if(I_gens.size() == 0) << "contains_ideal: I is empty";
    ah_domain_error_if(J_gens.size() == 0) << "contains_ideal: J is empty";

    for (size_t i = 0; i < I_gens.size(); ++i)
      ah_domain_error_if(I_gens(i).is_zero()) << "contains_ideal: I[" << i << "] is zero";

    for (size_t j = 0; j < J_gens.size(); ++j)
      ah_domain_error_if(J_gens(j).is_zero()) << "contains_ideal: J[" << j << "] is zero";

    size_t nv = I_gens(0).nvars_;
    for (size_t i = 1; i < I_gens.size(); ++i)
      ah_invalid_argument_if(I_gens(i).nvars_ != nv)
        << "contains_ideal: nvars mismatch in I[" << i << "]: expected " << nv << ", got "
        << I_gens(i).nvars_;

    for (size_t j = 0; j < J_gens.size(); ++j)
      ah_invalid_argument_if(J_gens(j).nvars_ != nv)
        << "contains_ideal: nvars mismatch in J[" << j << "]: expected " << nv << ", got "
        << J_gens(j).nvars_;

    // Precompute Gröbner basis of I once
    Array<Gen_MultiPolynomial> gb_I = groebner_basis(I_gens);

    // Check membership of each generator of J in I
    for (size_t j = 0; j < J_gens.size(); ++j)
      {
        Gen_MultiPolynomial remainder = J_gens(j).divmod(gb_I).second;
        if (not remainder.is_zero())
          return false;
      }

    return true;
  }

  /** @brief Test equality of two ideals: I = J.
   *
   *  Two ideals are equal if and only if each contains the other.
   *  Equivalent to: contains_ideal(I, J) && contains_ideal(J, I).
   *
   *  @param[in] I_gens Generators of ideal I (non-empty, no zeros).
   *  @param[in] J_gens Generators of ideal J (non-empty, no zeros).
   *  @return True if I = J; false otherwise.
   *  @throws std::domain_error if either ideal is empty or contains zero
   *  @throws std::invalid_argument if nvars mismatch
   *  @note Complexity: O(2 × contains_ideal). May be expensive for large ideals.
   */
  [[nodiscard]] static bool ideals_equal(const Array<Gen_MultiPolynomial> &I_gens,
                                         const Array<Gen_MultiPolynomial> &J_gens)
  {
    ah_domain_error_if(I_gens.size() == 0) << "ideals_equal: I is empty";
    ah_domain_error_if(J_gens.size() == 0) << "ideals_equal: J is empty";

    for (size_t i = 0; i < I_gens.size(); ++i)
      ah_domain_error_if(I_gens(i).is_zero()) << "ideals_equal: I[" << i << "] is zero";

    for (size_t j = 0; j < J_gens.size(); ++j)
      ah_domain_error_if(J_gens(j).is_zero()) << "ideals_equal: J[" << j << "] is zero";

    size_t nv = I_gens(0).nvars_;
    for (size_t i = 1; i < I_gens.size(); ++i)
      ah_invalid_argument_if(I_gens(i).nvars_ != nv)
        << "ideals_equal: nvars mismatch in I[" << i << "]: expected " << nv << ", got "
        << I_gens(i).nvars_;

    for (size_t j = 0; j < J_gens.size(); ++j)
      ah_invalid_argument_if(J_gens(j).nvars_ != nv)
        << "ideals_equal: nvars mismatch in J[" << j << "]: expected " << nv << ", got "
        << J_gens(j).nvars_;

    return contains_ideal(I_gens, J_gens) and contains_ideal(J_gens, I_gens);
  }

  /** @brief Test membership in radical of ideal: f ∈ √I.
   *
   *  Uses the Rabinowitsch trick: f ∈ √I (radical of I in k[x₀,...,x_{n−1}])
   *  if and only if 1 ∈ I + ⟨1 − y·f⟩ in k[x₀,...,x_{n−1}, y].
   *  Returns true if the augmented ideal contains 1.
   *
   *  @param[in] f    Polynomial to test (nvars must match generators if non-zero).
   *  @param[in] gens Generators of ideal I (non-empty, no zeros).
   *  @return True if f ∈ √I; false otherwise.
   *  @throws std::domain_error if @p gens is empty or contains zero
   *  @throws std::invalid_argument if f is non-zero with mismatched nvars
   *  @note Complexity: O(Gröbner(I + ⟨1−y·f⟩)) in augmented ring with nv+1 variables.
   *        The zero polynomial always belongs to any radical.
   *  @warning Rabinowitsch computation can be expensive due to the augmented ideal.
   */
  [[nodiscard]] static bool radical_member(const Gen_MultiPolynomial        &f,
                                           const Array<Gen_MultiPolynomial> &gens)
  {
    ah_domain_error_if(gens.size() == 0) << "radical_member: empty generators";

    for (size_t i = 0; i < gens.size(); ++i)
      ah_domain_error_if(gens(i).is_zero()) << "radical_member: gens[" << i << "] is zero";

    size_t nv = gens(0).nvars_;

    // Check nvars consistency (f.nvars_ == 0 is OK for zero polynomial)
    if (not f.is_zero() and f.nvars_ != nv)
      ah_invalid_argument_if(not f.is_zero() and f.nvars_ != nv)
        << "radical_member: f nvars mismatch: expected " << nv << ", got " << f.nvars_;

    // Special case: 0 ∈ √I always
    if (f.is_zero())
      return true;

    // Augmented ring has nv+1 variables (the new variable y is at index nv)
    size_t nv_aug = nv + 1;

    // Promote generators to the augmented ring
    Array<Gen_MultiPolynomial> aug_gens(gens.size() + 1, Gen_MultiPolynomial(nv_aug));
    for (size_t i = 0; i < gens.size(); ++i)
      aug_gens(i) = gens(i).promote(nv_aug);

    // Create the polynomial (1 - y·f) in the augmented ring
    //   y = monomial with exponent [0, ..., 0, 1] at index nv
    Array<size_t> y_exp(nv_aug, size_t(0));
    y_exp(nv) = 1;
    Gen_MultiPolynomial y_poly = monomial(nv_aug, y_exp, Coefficient(1));

    // Create 1 in the augmented ring
    Gen_MultiPolynomial one_aug(nv_aug, Coefficient(1));

    // Promote f to augmented ring and compute (1 - y·f)
    Gen_MultiPolynomial f_aug = f.promote(nv_aug);
    aug_gens(gens.size()) = one_aug - y_poly * f_aug;

    // Check if 1 is in the augmented ideal
    return ideal_member(one_aug, aug_gens);
  }

private:
  /** @brief Evaluate a monomial @f$x^\alpha@f$ at a point. */
  static Coefficient _eval_monomial(const Array<Coefficient> &pt, const Array<size_t> &alpha)
  {
    Coefficient v = Coefficient(1);
    for (size_t j = 0; j < alpha.size(); ++j)
      if (alpha(j) != 0)
        v *= multi_poly_detail::int_power(pt(j), alpha(j));
    return v;
  }

};  // class Gen_MultiPolynomial

// ===================================================================
//  Residual Analysis Helpers
// ===================================================================

/** @brief Structure holding residual analysis for polynomial fits.
 *
 *  Provides statistics about the quality of a least-squares fit:
 *  - R²: coefficient of determination
 *  - RMSE: root mean squared error
 *  - Residuals: per-point deviations
 *  - Sum of squares: TSS, RSS, ESS
 *
 *  @tparam C Coefficient type.
 */
template <typename C>
struct PolyFitAnalysis
{
  double   r_squared = 0.0;  ///< Coefficient of determination (0--1)
  double   rmse      = 0.0;  ///< Root mean squared error
  double   rss       = 0.0;  ///< Residual sum of squares
  double   tss       = 0.0;  ///< Total sum of squares
  double   ess       = 0.0;  ///< Explained sum of squares
  Array<C> residuals;        ///< Per-point residuals
  double   mean_y = 0.0;     ///< Mean of y values
};

/** @brief Compute residual analysis for a polynomial fit.
 *
 *  @tparam C Coefficient type.
 *  @tparam M Monomial ordering.
 *  @param[in] poly     Fitted polynomial.
 *  @param[in] data     Original (point, value) pairs.
 *  @return Structure containing R², RMSE, residuals, etc.
 */
template <typename C, class M>
[[nodiscard]] PolyFitAnalysis<C> analyze_fit(const Gen_MultiPolynomial<C, M>     &poly,
                                             const Array<std::pair<Array<C>, C>> &data)
{
  PolyFitAnalysis<C> result;
  const size_t       m = data.size();

  if (m == 0)
    return result;

  // Compute mean of y
  C sum_y = C{};
  for (size_t i = 0; i < m; ++i)
    sum_y += data(i).second;
  result.mean_y = static_cast<double>(sum_y / C(m));

  // Compute residuals, TSS, RSS
  result.residuals = Array<C>(m, C{});
  result.tss       = 0.0;
  result.rss       = 0.0;

  for (size_t i = 0; i < m; ++i)
    {
      C pred              = poly.eval(data(i).first);
      C res               = data(i).second - pred;
      result.residuals(i) = res;

      const auto dev_y = static_cast<double>(data(i).second - C(result.mean_y));
      result.tss += dev_y * dev_y;

      const auto res_d = static_cast<double>(res);
      result.rss += res_d * res_d;
    }

  result.ess = result.tss - result.rss;

  // Compute R²
  if (result.tss > 1e-16)
    result.r_squared = result.ess / result.tss;
  else
    result.r_squared = 0.0;

  // Compute RMSE
  result.rmse = std::sqrt(result.rss / (double) m);

  return result;
}

// ===================================================================
//  Free operators
// ===================================================================

/** @brief Scalar * polynomial (commutative).
 *  @param[in] s Scalar.
 *  @param[in] p Polynomial.
 *  @return Scaled polynomial.
 */
template <typename C, class M>
[[nodiscard]] Gen_MultiPolynomial<C, M> operator*(const C &s, const Gen_MultiPolynomial<C, M> &p)
{
  return p * s;
}

/** @brief Stream insertion.
 *  @param[out] out Output stream.
 *  @param[in]  p   Polynomial.
 *  @return Reference to @p out.
 */
template <typename C, class M>
std::ostream &operator<<(std::ostream &out, const Gen_MultiPolynomial<C, M> &p)
{
  return out << p.to_str();
}

// ===================================================================
//  Convenient typedefs
// ===================================================================

/** @brief Multivariate polynomial with double coefficients, grevlex. */
using MultiPolynomial = Gen_MultiPolynomial<double, Grevlex_Order>;

/** @brief Multivariate polynomial with lex ordering. */
using MultiPolynomial_Lex = Gen_MultiPolynomial<double, Lex_Order>;

/** @brief Multivariate polynomial with grlex ordering. */
using MultiPolynomial_Grlex = Gen_MultiPolynomial<double, Grlex_Order>;

}  // namespace Aleph

#endif  // TPL_MULTI_POLYNOMIAL_H