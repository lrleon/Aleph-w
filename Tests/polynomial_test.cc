/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  MIT License (see tpl_polynomial.H for full text)
*/

# include <gtest/gtest.h>
# include <chrono>
# include <cmath>
# include <random>
# include <string>
# include <sstream>
# include <stdexcept>
# include <tpl_polynomial.H>

using namespace Aleph;

namespace {

Polynomial
make_random_polynomial(std::mt19937 & rng, size_t max_degree = 4,
                       int min_coeff = -2, int max_coeff = 2,
                       bool require_nonzero = false)
{
  std::uniform_int_distribution<size_t> degree_dist(0, max_degree);
  std::uniform_int_distribution<int> coeff_dist(min_coeff, max_coeff);

  while (true)
    {
      const size_t degree = degree_dist(rng);
      DynList<double> coeffs;

      for (size_t exp = 0; exp <= degree; ++exp)
        coeffs.append(static_cast<double>(coeff_dist(rng)));

      Polynomial p(coeffs);
      if (not require_nonzero or not p.is_zero())
        return p;
    }
}

} // namespace

// ===================================================================
// Construction
// ===================================================================

TEST(Polynomial, DefaultIsZero)
{
  Polynomial p;
  EXPECT_TRUE(p.is_zero());
  EXPECT_EQ(p.degree(), 0u);
  EXPECT_EQ(p.num_terms(), 0u);
}

TEST(Polynomial, ConstantPolynomial)
{
  Polynomial p(5.0);
  EXPECT_FALSE(p.is_zero());
  EXPECT_EQ(p.degree(), 0u);
  EXPECT_EQ(p.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(p[0], 5.0);
  EXPECT_TRUE(p.is_constant());
}

TEST(Polynomial, ZeroConstantIsZeroPoly)
{
  Polynomial p(0.0);
  EXPECT_TRUE(p.is_zero());
}

TEST(Polynomial, MonomialConstruction)
{
  Polynomial p(3.0, 5); // 3x^5
  EXPECT_EQ(p.degree(), 5u);
  EXPECT_EQ(p.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(p[5], 3.0);
  EXPECT_DOUBLE_EQ(p[0], 0.0);
  EXPECT_TRUE(p.is_monomial());
}

TEST(Polynomial, InitializerListDense)
{
  Polynomial p({1, 0, 3}); // 1 + 3x^2
  EXPECT_EQ(p.degree(), 2u);
  EXPECT_EQ(p.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(p[0], 1.0);
  EXPECT_DOUBLE_EQ(p[1], 0.0);
  EXPECT_DOUBLE_EQ(p[2], 3.0);
}

TEST(Polynomial, DynListDense)
{
  DynList<double> l = {2, 3, 0, 5};
  Polynomial p(l); // 2 + 3x + 5x^3
  EXPECT_EQ(p.degree(), 3u);
  EXPECT_EQ(p.num_terms(), 3u);
  EXPECT_DOUBLE_EQ(p[0], 2.0);
  EXPECT_DOUBLE_EQ(p[1], 3.0);
  EXPECT_DOUBLE_EQ(p[3], 5.0);
}

TEST(Polynomial, SparseConstruction)
{
  DynList<std::pair<size_t, double>> terms;
  terms.append({0, 1.0});
  terms.append({100, 2.0});
  Polynomial p(terms); // 1 + 2x^100
  EXPECT_EQ(p.degree(), 100u);
  EXPECT_EQ(p.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(p[0], 1.0);
  EXPECT_DOUBLE_EQ(p[100], 2.0);
}

TEST(Polynomial, SparseConstructionMergesDuplicateExponents)
{
  DynList<std::pair<size_t, double>> terms;
  terms.append({2, 3.0});
  terms.append({0, 1.0});
  terms.append({2, -1.5});
  terms.append({2, -1.5});

  Polynomial p(terms);
  EXPECT_EQ(p.degree(), 0u);
  EXPECT_EQ(p.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(p[0], 1.0);
  EXPECT_DOUBLE_EQ(p[2], 0.0);
}

TEST(Polynomial, AllZerosIsZero)
{
  Polynomial p({0, 0, 0});
  EXPECT_TRUE(p.is_zero());
}

TEST(Polynomial, CopyConstructor)
{
  Polynomial p({1, 2, 3});
  Polynomial q(p);
  EXPECT_EQ(p, q);
}

TEST(Polynomial, MoveConstructor)
{
  Polynomial p({1, 2, 3});
  Polynomial copy(p);
  Polynomial q(std::move(p));
  EXPECT_EQ(q, copy);
}

TEST(Polynomial, CopyAssignment)
{
  Polynomial p({1, 2, 3});
  Polynomial q;
  q = p;
  EXPECT_EQ(p, q);
}

TEST(Polynomial, MoveAssignment)
{
  Polynomial p({1, 2, 3});
  Polynomial copy(p);
  Polynomial q;
  q = std::move(p);
  EXPECT_EQ(q, copy);
}

// ===================================================================
// Properties
// ===================================================================

TEST(Polynomial, DegreeOfQuadratic)
{
  Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
  EXPECT_EQ(p.degree(), 2u);
}

TEST(Polynomial, DegreeOfConstant)
{
  Polynomial p(7.0);
  EXPECT_EQ(p.degree(), 0u);
}

TEST(Polynomial, LeadingCoefficient)
{
  Polynomial p({1, 2, 5}); // 1 + 2x + 5x^2
  EXPECT_DOUBLE_EQ(p.leading_coeff(), 5.0);
}

TEST(Polynomial, LeadingCoeffOfZero)
{
  Polynomial p;
  EXPECT_DOUBLE_EQ(p.leading_coeff(), 0.0);
}

TEST(Polynomial, IsMonic)
{
  Polynomial p({-3, 0, 1}); // -3 + x^2
  EXPECT_TRUE(p.is_monic());

  Polynomial q({1, 2}); // 1 + 2x
  EXPECT_FALSE(q.is_monic());
}

TEST(Polynomial, HasTerm)
{
  Polynomial p({1, 0, 3}); // 1 + 3x^2
  EXPECT_TRUE(p.has_term(0));
  EXPECT_FALSE(p.has_term(1));
  EXPECT_TRUE(p.has_term(2));
  EXPECT_FALSE(p.has_term(99));
}

// ===================================================================
// Coefficient access
// ===================================================================

TEST(Polynomial, OperatorBracketReturnsZeroForAbsent)
{
  Polynomial p(3.0, 5); // 3x^5
  EXPECT_DOUBLE_EQ(p[0], 0.0);
  EXPECT_DOUBLE_EQ(p[5], 3.0);
  EXPECT_DOUBLE_EQ(p[999], 0.0);
}

// ===================================================================
// Evaluation
// ===================================================================

TEST(Polynomial, EvalConstant)
{
  Polynomial p(5.0);
  EXPECT_DOUBLE_EQ(p.eval(0.0), 5.0);
  EXPECT_DOUBLE_EQ(p.eval(100.0), 5.0);
  EXPECT_DOUBLE_EQ(p.eval(-3.14), 5.0);
}

TEST(Polynomial, EvalLinear)
{
  Polynomial p({3, 2}); // 3 + 2x
  EXPECT_DOUBLE_EQ(p.eval(0.0), 3.0);
  EXPECT_DOUBLE_EQ(p.eval(5.0), 13.0);
  EXPECT_DOUBLE_EQ(p.eval(-1.0), 1.0);
}

TEST(Polynomial, EvalQuadratic)
{
  Polynomial p({-1, 0, 1}); // x^2 - 1
  EXPECT_DOUBLE_EQ(p.eval(0.0), -1.0);
  EXPECT_DOUBLE_EQ(p.eval(1.0), 0.0);
  EXPECT_DOUBLE_EQ(p.eval(-1.0), 0.0);
  EXPECT_DOUBLE_EQ(p.eval(3.0), 8.0);
}

TEST(Polynomial, EvalZeroPoly)
{
  Polynomial p;
  EXPECT_DOUBLE_EQ(p.eval(42.0), 0.0);
}

TEST(Polynomial, EvalCallOperator)
{
  Polynomial p({1, 1}); // 1 + x
  EXPECT_DOUBLE_EQ(p(9.0), 10.0);
}

TEST(Polynomial, EvalSparseHighDegree)
{
  Polynomial p(1.0, 100); // x^100
  EXPECT_DOUBLE_EQ(p.eval(1.0), 1.0);
  EXPECT_DOUBLE_EQ(p.eval(0.0), 0.0);
  EXPECT_DOUBLE_EQ(p.eval(-1.0), 1.0); // (-1)^100 = 1
}

// ===================================================================
// Arithmetic
// ===================================================================

TEST(Polynomial, AddTwoPolynomials)
{
  Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
  Polynomial q({4, 5});     // 4 + 5x
  Polynomial sum = p + q;
  EXPECT_DOUBLE_EQ(sum[0], 5.0);
  EXPECT_DOUBLE_EQ(sum[1], 7.0);
  EXPECT_DOUBLE_EQ(sum[2], 3.0);
}

TEST(Polynomial, AddInPlace)
{
  Polynomial p({1, 2});
  Polynomial q({3, 4});
  p += q;
  EXPECT_DOUBLE_EQ(p[0], 4.0);
  EXPECT_DOUBLE_EQ(p[1], 6.0);
}

TEST(Polynomial, AddInPlaceSelf)
{
  Polynomial p({1, -2, 3});
  p += p;
  EXPECT_DOUBLE_EQ(p[0], 2.0);
  EXPECT_DOUBLE_EQ(p[1], -4.0);
  EXPECT_DOUBLE_EQ(p[2], 6.0);
}

TEST(Polynomial, AddScalarRight)
{
  Polynomial p({1, 2, 3});
  Polynomial q = p + 4.0;
  EXPECT_DOUBLE_EQ(q[0], 5.0);
  EXPECT_DOUBLE_EQ(q[1], 2.0);
  EXPECT_DOUBLE_EQ(q[2], 3.0);
}

TEST(Polynomial, AddScalarLeft)
{
  Polynomial p({1, 2, 3});
  Polynomial q = 4.0 + p;
  EXPECT_DOUBLE_EQ(q[0], 5.0);
  EXPECT_DOUBLE_EQ(q[1], 2.0);
  EXPECT_DOUBLE_EQ(q[2], 3.0);
}

TEST(Polynomial, SubtractPolynomials)
{
  Polynomial p({5, 3});
  Polynomial q({2, 1});
  Polynomial diff = p - q;
  EXPECT_DOUBLE_EQ(diff[0], 3.0);
  EXPECT_DOUBLE_EQ(diff[1], 2.0);
}

TEST(Polynomial, SubtractFromItself)
{
  Polynomial p({1, 2, 3});
  Polynomial zero = p - p;
  EXPECT_TRUE(zero.is_zero());
}

TEST(Polynomial, SubtractInPlaceSelf)
{
  Polynomial p({1, 2, 3});
  p -= p;
  EXPECT_TRUE(p.is_zero());
}

TEST(Polynomial, SubtractScalarRight)
{
  Polynomial p({1, 2, 3});
  Polynomial q = p - 4.0;
  EXPECT_DOUBLE_EQ(q[0], -3.0);
  EXPECT_DOUBLE_EQ(q[1], 2.0);
  EXPECT_DOUBLE_EQ(q[2], 3.0);
}

TEST(Polynomial, SubtractScalarLeft)
{
  Polynomial p({1, 2, 3});
  Polynomial q = 4.0 - p;
  EXPECT_DOUBLE_EQ(q[0], 3.0);
  EXPECT_DOUBLE_EQ(q[1], -2.0);
  EXPECT_DOUBLE_EQ(q[2], -3.0);
}

TEST(Polynomial, UnaryNegation)
{
  Polynomial p({1, -2, 3}); // 1 - 2x + 3x^2
  Polynomial neg = -p;
  EXPECT_DOUBLE_EQ(neg[0], -1.0);
  EXPECT_DOUBLE_EQ(neg[1], 2.0);
  EXPECT_DOUBLE_EQ(neg[2], -3.0);
}

TEST(Polynomial, MultiplyPolynomials)
{
  Polynomial p({1, 1}); // (x + 1)
  Polynomial q({-1, 1}); // (x - 1)
  Polynomial prod = p * q;
  // (x+1)(x-1) = x^2 - 1
  EXPECT_DOUBLE_EQ(prod[0], -1.0);
  EXPECT_DOUBLE_EQ(prod[1], 0.0);
  EXPECT_DOUBLE_EQ(prod[2], 1.0);
  EXPECT_EQ(prod.degree(), 2u);
}

TEST(Polynomial, MultiplyByZero)
{
  Polynomial p({1, 2, 3});
  Polynomial z;
  Polynomial prod = p * z;
  EXPECT_TRUE(prod.is_zero());
}

TEST(Polynomial, MultiplyByConstant)
{
  Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
  Polynomial c(2.0);
  Polynomial prod = p * c;
  EXPECT_DOUBLE_EQ(prod[0], 2.0);
  EXPECT_DOUBLE_EQ(prod[1], 4.0);
  EXPECT_DOUBLE_EQ(prod[2], 6.0);
}

TEST(Polynomial, ScalarMultiply)
{
  Polynomial p({1, 2, 3});
  Polynomial q = p * 3.0;
  EXPECT_DOUBLE_EQ(q[0], 3.0);
  EXPECT_DOUBLE_EQ(q[1], 6.0);
  EXPECT_DOUBLE_EQ(q[2], 9.0);

  // Commutative form
  Polynomial r = 3.0 * p;
  EXPECT_EQ(q, r);
}

TEST(Polynomial, ScalarDivide)
{
  Polynomial p({2, 4, 6});
  Polynomial q = p / 2.0;
  EXPECT_DOUBLE_EQ(q[0], 1.0);
  EXPECT_DOUBLE_EQ(q[1], 2.0);
  EXPECT_DOUBLE_EQ(q[2], 3.0);
}

TEST(Polynomial, ScalarDivideByZeroThrows)
{
  Polynomial p({1, 2});
  EXPECT_THROW(p / 0.0, std::domain_error);
}

// ===================================================================
// Polynomial division
// ===================================================================

TEST(Polynomial, DivideByLinear)
{
  // (x^2 - 1) / (x - 1) = (x + 1) with remainder 0
  Polynomial p({-1, 0, 1}); // x^2 - 1
  Polynomial d({-1, 1});     // x - 1
  auto [q, r] = p.divmod(d);

  EXPECT_DOUBLE_EQ(q[0], 1.0);
  EXPECT_DOUBLE_EQ(q[1], 1.0);
  EXPECT_EQ(q.degree(), 1u);
  EXPECT_TRUE(r.is_zero());
}

TEST(Polynomial, DivideWithRemainder)
{
  // (x^3 + x + 1) / (x + 1)
  Polynomial p({1, 1, 0, 1}); // 1 + x + x^3
  Polynomial d({1, 1});        // 1 + x
  auto [q, r] = p.divmod(d);

  // Verify: p == q * d + r
  Polynomial reconstructed = q * d + r;
  EXPECT_EQ(p, reconstructed);
}

TEST(Polynomial, DivideByHigherDegree)
{
  Polynomial p({1, 2}); // 1 + 2x (degree 1)
  Polynomial d({1, 0, 1}); // 1 + x^2 (degree 2)
  auto [q, r] = p.divmod(d);
  EXPECT_TRUE(q.is_zero());
  EXPECT_EQ(r, p);
}

TEST(Polynomial, DivideByZeroThrows)
{
  Polynomial p({1, 2});
  Polynomial z;
  EXPECT_THROW(p.divmod(z), std::domain_error);
}

TEST(Polynomial, DivisionIdentity)
{
  // For several polynomial pairs, verify a == (a/b)*b + (a%b)
  Polynomial a({3, -2, 0, 5, 1}); // 3 - 2x + 5x^3 + x^4
  Polynomial b({1, 1, 1});         // 1 + x + x^2

  Polynomial q = a / b;
  Polynomial r = a % b;
  EXPECT_EQ(a, q * b + r);
}

TEST(Polynomial, QuotientAndModOperators)
{
  Polynomial a({-1, 0, 1}); // x^2 - 1
  Polynomial b({-1, 1});     // x - 1

  Polynomial q = a / b;
  Polynomial r = a % b;
  EXPECT_DOUBLE_EQ(q[0], 1.0);
  EXPECT_DOUBLE_EQ(q[1], 1.0);
  EXPECT_TRUE(r.is_zero());
}

TEST(Polynomial, RandomizedPolynomialProperties)
{
  std::mt19937 rng(0xA13F2026u);
  std::uniform_int_distribution<int> scalar_dist(-3, 3);
  std::uniform_int_distribution<int> x_dist(-2, 2);

  for (size_t i = 0; i < 100; ++i)
    {
      Polynomial a = make_random_polynomial(rng);
      Polynomial b = make_random_polynomial(rng, 3, -2, 2, true);
      Polynomial c = make_random_polynomial(rng);
      const double scalar = static_cast<double>(scalar_dist(rng));
      const double x = static_cast<double>(x_dist(rng));

      EXPECT_EQ(a * (b + c), a * b + a * c);
      EXPECT_EQ((a + b) + c, a + (b + c));
      EXPECT_EQ(scalar * a, a * scalar);

      auto [q, r] = a.divmod(b);
      EXPECT_EQ(q, a / b);
      EXPECT_EQ(r, a % b);
      EXPECT_EQ(a, q * b + r);
      EXPECT_TRUE(r.is_zero() or r.degree() < b.degree());

      Polynomial sum = a + b;
      Polynomial diff = sum - b;
      EXPECT_EQ(sum[0], a[0] + b[0]);
      EXPECT_EQ(diff, a);
      EXPECT_TRUE((a - a).is_zero());
      EXPECT_TRUE(sum.is_zero() or sum[sum.degree()] != 0.0);

      Polynomial composed = a.compose(c);
      EXPECT_TRUE(std::abs(composed.eval(x) - a.eval(c.eval(x))) <= 1e-9);

      Polynomial zero;
      EXPECT_THROW(a.divmod(zero), std::domain_error);
    }
}

// ===================================================================
// Calculus
// ===================================================================

TEST(Polynomial, DerivativeOfConstant)
{
  Polynomial p(7.0);
  EXPECT_TRUE(p.derivative().is_zero());
}

TEST(Polynomial, DerivativeOfLinear)
{
  Polynomial p({3, 5}); // 3 + 5x
  Polynomial dp = p.derivative();
  EXPECT_EQ(dp.degree(), 0u);
  EXPECT_DOUBLE_EQ(dp[0], 5.0);
}

TEST(Polynomial, DerivativeOfQuadratic)
{
  Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
  Polynomial dp = p.derivative();
  // dp = 2 + 6x
  EXPECT_DOUBLE_EQ(dp[0], 2.0);
  EXPECT_DOUBLE_EQ(dp[1], 6.0);
  EXPECT_EQ(dp.degree(), 1u);
}

TEST(Polynomial, DerivativeOfZero)
{
  Polynomial p;
  EXPECT_TRUE(p.derivative().is_zero());
}

TEST(Polynomial, IntegralBasic)
{
  Polynomial p({2, 6}); // 2 + 6x
  Polynomial ip = p.integral();
  // integral = 2x + 3x^2 (+ 0)
  EXPECT_DOUBLE_EQ(ip[0], 0.0);
  EXPECT_DOUBLE_EQ(ip[1], 2.0);
  EXPECT_DOUBLE_EQ(ip[2], 3.0);
}

TEST(Polynomial, IntegralWithConstant)
{
  Polynomial p({2, 6}); // 2 + 6x
  Polynomial ip = p.integral(5.0);
  // integral = 5 + 2x + 3x^2
  EXPECT_DOUBLE_EQ(ip[0], 5.0);
  EXPECT_DOUBLE_EQ(ip[1], 2.0);
  EXPECT_DOUBLE_EQ(ip[2], 3.0);
}

TEST(Polynomial, IntegralDerivativeRoundTrip)
{
  Polynomial p({3, -1, 4, 0, 2}); // 3 - x + 4x^2 + 2x^4
  Polynomial q = p.integral().derivative();
  // integral -> derivative should recover p (constant lost, then regained as 0)
  EXPECT_EQ(q, p);
}

TEST(Polynomial, NthDerivative)
{
  Polynomial p({1, 0, 0, 6}); // 1 + 6x^3
  Polynomial d3 = p.nth_derivative(3);
  // 3rd derivative of 6x^3 = 36
  EXPECT_DOUBLE_EQ(d3[0], 36.0);
  EXPECT_EQ(d3.degree(), 0u);
}

// ===================================================================
// Composition and power
// ===================================================================

TEST(Polynomial, ComposeLinear)
{
  Polynomial p({1, 0, 1}); // x^2 + 1
  Polynomial q({1, 2});     // 2x + 1
  // p(q(x)) = (2x+1)^2 + 1 = 4x^2 + 4x + 2
  Polynomial comp = p.compose(q);
  EXPECT_DOUBLE_EQ(comp[0], 2.0);
  EXPECT_DOUBLE_EQ(comp[1], 4.0);
  EXPECT_DOUBLE_EQ(comp[2], 4.0);
}

TEST(Polynomial, ComposeIdentity)
{
  Polynomial p({1, 2, 3});
  Polynomial id({0, 1}); // x
  EXPECT_EQ(p.compose(id), p);
}

TEST(Polynomial, PowerZero)
{
  Polynomial p({1, 1}); // x + 1
  Polynomial one = p.pow(0);
  EXPECT_EQ(one.degree(), 0u);
  EXPECT_DOUBLE_EQ(one[0], 1.0);
}

TEST(Polynomial, PowerOne)
{
  Polynomial p({1, 2, 3});
  EXPECT_EQ(p.pow(1), p);
}

TEST(Polynomial, PowerSquare)
{
  Polynomial p({1, 1}); // x + 1
  Polynomial sq = p.pow(2);
  // (x+1)^2 = x^2 + 2x + 1
  EXPECT_DOUBLE_EQ(sq[0], 1.0);
  EXPECT_DOUBLE_EQ(sq[1], 2.0);
  EXPECT_DOUBLE_EQ(sq[2], 1.0);
}

TEST(Polynomial, PowerCube)
{
  Polynomial p({1, 1}); // x + 1
  Polynomial cu = p.pow(3);
  // (x+1)^3 = x^3 + 3x^2 + 3x + 1
  EXPECT_DOUBLE_EQ(cu[0], 1.0);
  EXPECT_DOUBLE_EQ(cu[1], 3.0);
  EXPECT_DOUBLE_EQ(cu[2], 3.0);
  EXPECT_DOUBLE_EQ(cu[3], 1.0);
}

// ===================================================================
// GCD
// ===================================================================

TEST(Polynomial, GCDBasic)
{
  // gcd(x^2 - 1, x - 1) = x - 1 (monic)
  Polynomial a({-1, 0, 1}); // x^2 - 1
  Polynomial b({-1, 1});     // x - 1
  Polynomial g = Polynomial::gcd(a, b);

  // g should be monic of degree 1
  EXPECT_EQ(g.degree(), 1u);
  EXPECT_DOUBLE_EQ(g.leading_coeff(), 1.0);
  // g should divide both a and b
  EXPECT_TRUE((a % g).is_zero());
  EXPECT_TRUE((b % g).is_zero());
}

TEST(Polynomial, GCDCoprime)
{
  Polynomial a({0, 0, 1}); // x^2
  Polynomial b({1, 1});     // x + 1
  Polynomial g = Polynomial::gcd(a, b);
  // Coprime: gcd should be constant (monic → 1)
  EXPECT_TRUE(g.is_constant());
  EXPECT_NEAR(g[0], 1.0, 1e-6);
}

// ===================================================================
// Comparison
// ===================================================================

TEST(Polynomial, EqualPolynomials)
{
  Polynomial p({1, 2, 3});
  Polynomial q({1, 2, 3});
  EXPECT_EQ(p, q);
}

TEST(Polynomial, UnequalPolynomials)
{
  Polynomial p({1, 2, 3});
  Polynomial q({1, 2, 4});
  EXPECT_NE(p, q);
}

TEST(Polynomial, ZeroPolynomialsEqual)
{
  Polynomial p;
  Polynomial q;
  EXPECT_EQ(p, q);
}

TEST(Polynomial, EqualDifferentConstruction)
{
  // Same polynomial built differently
  Polynomial p({1, 0, 3}); // 1 + 3x^2
  DynList<std::pair<size_t, double>> terms;
  terms.append({0, 1.0});
  terms.append({2, 3.0});
  Polynomial q(terms);
  EXPECT_EQ(p, q);
}

// ===================================================================
// Factory methods
// ===================================================================

TEST(Polynomial, FromRootsLinear)
{
  // Single root at 2: (x - 2)
  DynList<double> roots = {2.0};
  Polynomial p = Polynomial::from_roots(roots);
  EXPECT_DOUBLE_EQ(p.eval(2.0), 0.0);
  EXPECT_EQ(p.degree(), 1u);
}

TEST(Polynomial, FromRootsQuadratic)
{
  // Roots at 1 and -1: (x-1)(x+1) = x^2 - 1
  DynList<double> roots = {1.0, -1.0};
  Polynomial p = Polynomial::from_roots(roots);
  EXPECT_NEAR(p.eval(1.0), 0.0, 1e-9);
  EXPECT_NEAR(p.eval(-1.0), 0.0, 1e-9);
  EXPECT_DOUBLE_EQ(p[2], 1.0);
  EXPECT_NEAR(p[0], -1.0, 1e-9);
}

TEST(Polynomial, XToN)
{
  Polynomial p = Polynomial::x_to(3);
  EXPECT_EQ(p.degree(), 3u);
  EXPECT_DOUBLE_EQ(p[3], 1.0);
  EXPECT_DOUBLE_EQ(p[0], 0.0);
}

TEST(Polynomial, ZeroAndOneFactories)
{
  EXPECT_TRUE(Polynomial::zero().is_zero());
  EXPECT_FALSE(Polynomial::one().is_zero());
  EXPECT_DOUBLE_EQ(Polynomial::one()[0], 1.0);
}

// ===================================================================
// Interpolation
// ===================================================================

TEST(Polynomial, InterpolateTwoPoints)
{
  // Line through (0, 1) and (1, 3): y = 2x + 1
  DynList<std::pair<double, double>> pts;
  pts.append(std::make_pair(0.0, 1.0));
  pts.append(std::make_pair(1.0, 3.0));

  Polynomial p = Polynomial::interpolate(pts);
  EXPECT_NEAR(p(0.0), 1.0, 1e-9);
  EXPECT_NEAR(p(1.0), 3.0, 1e-9);
  EXPECT_NEAR(p[0], 1.0, 1e-9);
  EXPECT_NEAR(p[1], 2.0, 1e-9);
}

TEST(Polynomial, InterpolateQuadratic)
{
  // Points from y = x^2: (0,0), (1,1), (2,4)
  DynList<std::pair<double, double>> pts;
  pts.append(std::make_pair(0.0, 0.0));
  pts.append(std::make_pair(1.0, 1.0));
  pts.append(std::make_pair(2.0, 4.0));

  Polynomial p = Polynomial::interpolate(pts);
  EXPECT_NEAR(p(0.0), 0.0, 1e-9);
  EXPECT_NEAR(p(1.0), 1.0, 1e-9);
  EXPECT_NEAR(p(2.0), 4.0, 1e-9);
  // Should recover x^2
  EXPECT_NEAR(p[2], 1.0, 1e-9);
  EXPECT_NEAR(p(3.0), 9.0, 1e-7);
}

TEST(Polynomial, InterpolateEmptyThrows)
{
  DynList<std::pair<double, double>> pts;
  EXPECT_THROW(Polynomial::interpolate(pts), std::domain_error);
}

TEST(Polynomial, InterpolateDuplicateXThrows)
{
  DynList<std::pair<double, double>> pts;
  pts.append(std::make_pair(1.0, 2.0));
  pts.append(std::make_pair(1.0, 5.0));
  EXPECT_THROW(Polynomial::interpolate(pts), std::domain_error);
}

// ===================================================================
// Iteration
// ===================================================================

TEST(Polynomial, ForEachTerm)
{
  Polynomial p({1, 0, 3}); // 1 + 3x^2
  size_t count = 0;
  p.for_each_term([&count](size_t exp, const double & c)
  {
    if (exp == 0)
      EXPECT_DOUBLE_EQ(c, 1.0);
    else if (exp == 2)
      EXPECT_DOUBLE_EQ(c, 3.0);
    else
      FAIL() << "unexpected exponent " << exp;
    ++count;
  });
  EXPECT_EQ(count, 2u);
}

TEST(Polynomial, TermsList)
{
  Polynomial p({1, 0, 3});
  auto tl = p.terms_list();
  EXPECT_EQ(tl.size(), 2u);
}

TEST(Polynomial, Exponents)
{
  Polynomial p({1, 0, 3});
  auto exps = p.exponents();
  EXPECT_EQ(exps.size(), 2u);
}

// ===================================================================
// String representation
// ===================================================================

TEST(Polynomial, ToStrZero)
{
  Polynomial p;
  EXPECT_EQ(p.to_str(), "0");
}

TEST(Polynomial, ToStrConstant)
{
  Polynomial p(5.0);
  EXPECT_EQ(p.to_str(), "5");
}

TEST(Polynomial, ToStrLinear)
{
  Polynomial p({3, 2}); // 3 + 2x
  std::string s = p.to_str();
  // Should contain "2x" and "3"
  EXPECT_NE(s.find("2x"), std::string::npos);
  EXPECT_NE(s.find("3"), std::string::npos);
}

TEST(Polynomial, StreamOperator)
{
  Polynomial p({1, 2, 3});
  std::ostringstream oss;
  oss << p;
  EXPECT_FALSE(oss.str().empty());
  EXPECT_NE(oss.str(), "0");
}

// ===================================================================
// Edge cases
// ===================================================================

TEST(Polynomial, CancellationToZero)
{
  Polynomial p({1, 2, 3});
  Polynomial q = p - p;
  EXPECT_TRUE(q.is_zero());
}

TEST(Polynomial, HighDegreeSparse)
{
  // x^1000 + 1
  DynList<std::pair<size_t, double>> terms;
  terms.append({0, 1.0});
  terms.append({1000, 1.0});
  Polynomial p(terms);

  EXPECT_EQ(p.degree(), 1000u);
  EXPECT_EQ(p.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(p.eval(1.0), 2.0);
  EXPECT_DOUBLE_EQ(p.eval(0.0), 1.0);

  // Functional correctness checks (timing moved to benchmark suite)
  const double value = p.eval(1.5);
  EXPECT_GT(value, 1.0);

  Polynomial squared = p * p;
  EXPECT_EQ(squared.degree(), 2000u);
  EXPECT_EQ(squared.num_terms(), 3u);

  auto [quotient, remainder] = squared.divmod(p);
  EXPECT_EQ(quotient, p);
  EXPECT_TRUE(remainder.is_zero());

  Polynomial linear({0, 2}); // 2x
  Polynomial composed = p.compose(linear);
  EXPECT_EQ(composed.degree(), 1000u);
  EXPECT_EQ(composed.num_terms(), 2u);
}

TEST(Polynomial, SelfMultiply)
{
  Polynomial p({1, 1}); // x + 1
  Polynomial sq = p * p;
  EXPECT_EQ(sq, p.pow(2));
}

TEST(Polynomial, SelfAdd)
{
  Polynomial p({1, 2, 3});
  Polynomial doubled = p + p;
  EXPECT_EQ(doubled, p * 2.0);
}

TEST(Polynomial, IntegerCoefficients)
{
  Gen_Polynomial<int> p({1, 2, 3});
  Gen_Polynomial<int> q({4, 5});
  auto sum = p + q;
  EXPECT_EQ(sum[0], 5);
  EXPECT_EQ(sum[1], 7);
  EXPECT_EQ(sum[2], 3);
}

TEST(Polynomial, IntegerMultiplication)
{
  Gen_Polynomial<int> p({1, 1}); // x + 1
  Gen_Polynomial<int> q({-1, 1}); // x - 1
  auto prod = p * q;
  // x^2 - 1
  EXPECT_EQ(prod[0], -1);
  EXPECT_EQ(prod[1], 0);
  EXPECT_EQ(prod[2], 1);
}

TEST(Polynomial, IntegerDivision)
{
  Gen_Polynomial<int> a({-1, 0, 1}); // x^2 - 1
  Gen_Polynomial<int> b({-1, 1});     // x - 1
  auto [q, r] = a.divmod(b);
  EXPECT_EQ(q[0], 1);
  EXPECT_EQ(q[1], 1);
  EXPECT_TRUE(r.is_zero());
}

TEST(Polynomial, DerivativeThenEval)
{
  // p(x) = x^3, p'(x) = 3x^2, p'(2) = 12
  Polynomial p(1.0, 3);
  Polynomial dp = p.derivative();
  EXPECT_DOUBLE_EQ(dp.eval(2.0), 12.0);
}

TEST(Polynomial, ComposeWithConstant)
{
  Polynomial p({1, 2, 3}); // 1 + 2x + 3x^2
  Polynomial c(5.0);
  // p(5) = 1 + 10 + 75 = 86
  Polynomial comp = p.compose(c);
  EXPECT_TRUE(comp.is_constant());
  EXPECT_DOUBLE_EQ(comp[0], 86.0);
}

TEST(Polynomial, FromRootsThenEval)
{
  // Roots at 1, 2, 3
  DynList<double> roots = {1.0, 2.0, 3.0};
  Polynomial p = Polynomial::from_roots(roots);
  EXPECT_EQ(p.degree(), 3u);
  EXPECT_NEAR(p(1.0), 0.0, 1e-9);
  EXPECT_NEAR(p(2.0), 0.0, 1e-9);
  EXPECT_NEAR(p(3.0), 0.0, 1e-9);
  // p(0) = (-1)(-2)(-3) = -6
  EXPECT_NEAR(p(0.0), -6.0, 1e-9);
}

// ===================================================================
// Horner evaluation
// ===================================================================

TEST(Polynomial, HornerEvalDense)
{
  // p(x) = 2 + 3x + x^2, p(3) = 2 + 9 + 9 = 20
  Polynomial p({2, 3, 1});
  EXPECT_DOUBLE_EQ(p.horner_eval(3.0), 20.0);
  EXPECT_DOUBLE_EQ(p.horner_eval(0.0), 2.0);
  EXPECT_DOUBLE_EQ(p.horner_eval(1.0), 6.0);
}

TEST(Polynomial, SparseEvalMatches)
{
  // Compare horner_eval and sparse_eval on a polynomial
  Polynomial p({-1, 0, 0, 1}); // x^3 - 1
  for (double x = -2.0; x <= 3.0; x += 0.5)
    EXPECT_NEAR(p.horner_eval(x), p.sparse_eval(x), 1e-12);
}

TEST(Polynomial, EvalAdaptiveSelectsCorrectly)
{
  // Dense polynomial — should use Horner internally
  Polynomial dense({1, 2, 3, 4, 5});
  EXPECT_DOUBLE_EQ(dense(1.0), 15.0);

  // Sparse polynomial — should use sparse path
  Polynomial sparse(1.0, 100); // x^100
  EXPECT_DOUBLE_EQ(sparse(1.0), 1.0);
  EXPECT_DOUBLE_EQ(sparse(0.0), 0.0);
}

TEST(Polynomial, HornerEvalZeroPoly)
{
  Polynomial p;
  EXPECT_DOUBLE_EQ(p.horner_eval(42.0), 0.0);
}

// ===================================================================
// Multi-point evaluation
// ===================================================================

TEST(Polynomial, MultiEval)
{
  Polynomial p({0, 0, 1}); // x^2
  DynList<double> points = {0.0, 1.0, 2.0, 3.0, -1.0};
  DynList<double> results = p.multi_eval(points);

  DynList<double> expected = {0.0, 1.0, 4.0, 9.0, 1.0};
  auto it_r = results.get_it();
  auto it_e = expected.get_it();
  while (it_r.has_curr())
    {
      EXPECT_NEAR(it_r.get_curr(), it_e.get_curr(), 1e-12);
      it_r.next_ne();
      it_e.next_ne();
    }
}

TEST(Polynomial, MultiEvalEmpty)
{
  Polynomial p({1, 2});
  DynList<double> empty;
  DynList<double> results = p.multi_eval(empty);
  EXPECT_TRUE(results.is_empty());
}

// ===================================================================
// Definite integral
// ===================================================================

TEST(Polynomial, DefiniteIntegralLinear)
{
  // integral of 2x+1 from 0 to 3 = [x^2+x]_0^3 = 12
  Polynomial p({1, 2});
  EXPECT_NEAR(p.definite_integral(0.0, 3.0), 12.0, 1e-12);
}

TEST(Polynomial, DefiniteIntegralQuadratic)
{
  // integral of x^2 from 0 to 1 = 1/3
  Polynomial p({0, 0, 1});
  EXPECT_NEAR(p.definite_integral(0.0, 1.0), 1.0/3.0, 1e-12);
}

TEST(Polynomial, DefiniteIntegralSymmetric)
{
  // integral of x^3 from -1 to 1 = 0 (odd function)
  Polynomial p({0, 0, 0, 1});
  EXPECT_NEAR(p.definite_integral(-1.0, 1.0), 0.0, 1e-12);
}

// ===================================================================
// Extended GCD
// ===================================================================

TEST(Polynomial, XgcdBasic)
{
  // a = x^2 - 1, b = x - 1, gcd = x - 1
  Polynomial a({-1, 0, 1});
  Polynomial b({-1, 1});
  auto [g, s, t] = Polynomial::xgcd(a, b);

  // g should be monic and divide both a and b
  EXPECT_EQ(g.degree(), 1u);
  EXPECT_TRUE(g.is_monic());
  EXPECT_NEAR(g[0], -1.0, 1e-9);
  EXPECT_NEAR(g[1], 1.0, 1e-9);

  // Verify Bezout identity: s*a + t*b = g
  Polynomial bezout = s * a + t * b;
  EXPECT_EQ(bezout.degree(), g.degree());
  EXPECT_NEAR(bezout[0], g[0], 1e-9);
  EXPECT_NEAR(bezout[1], g[1], 1e-9);
}

TEST(Polynomial, XgcdCoprime)
{
  // x^2 + 1 and x - 1 are coprime
  Polynomial a({1, 0, 1});
  Polynomial b({-1, 1});
  auto [g, s, t] = Polynomial::xgcd(a, b);

  EXPECT_EQ(g.degree(), 0u);
  EXPECT_NEAR(g[0], 1.0, 1e-9);

  // Verify s*a + t*b = 1
  Polynomial bezout = s * a + t * b;
  EXPECT_TRUE(bezout.is_constant());
  EXPECT_NEAR(bezout[0], 1.0, 1e-9);
}

// ===================================================================
// LCM
// ===================================================================

TEST(Polynomial, LcmBasic)
{
  // a = (x-1)(x-2), b = (x-2)(x-3)
  // gcd = (x-2), lcm = (x-1)(x-2)(x-3)
  Polynomial a = Polynomial::from_roots(DynList<double>({1.0, 2.0}));
  Polynomial b = Polynomial::from_roots(DynList<double>({2.0, 3.0}));
  Polynomial l = Polynomial::lcm(a, b);

  EXPECT_EQ(l.degree(), 3u);
  EXPECT_NEAR(l(1.0), 0.0, 1e-8);
  EXPECT_NEAR(l(2.0), 0.0, 1e-8);
  EXPECT_NEAR(l(3.0), 0.0, 1e-8);
}

TEST(Polynomial, LcmWithZero)
{
  Polynomial a({1, 2});
  Polynomial z;
  EXPECT_TRUE(Polynomial::lcm(a, z).is_zero());
  EXPECT_TRUE(Polynomial::lcm(z, a).is_zero());
}

// ===================================================================
// Pseudo-division
// ===================================================================

TEST(Polynomial, PseudoDivmod)
{
  // a = 3x^3 + x + 2, b = x^2 + 1  (integer-like)
  Gen_Polynomial<int> a({2, 1, 0, 3});
  Gen_Polynomial<int> b({1, 0, 1});

  auto [q, r] = a.pseudo_divmod(b);

  // Verify: lc(b)^(deg(a)-deg(b)+1) * a = q*b + r
  int lc_b = b.leading_coeff();
  size_t delta = a.degree() - b.degree() + 1;
  int scale = 1;
  for (size_t i = 0; i < delta; ++i)
    scale *= lc_b;

  Gen_Polynomial<int> lhs = Gen_Polynomial<int>(scale) * a;
  Gen_Polynomial<int> rhs = q * b + r;
  EXPECT_EQ(lhs, rhs);
}

TEST(Polynomial, PseudoDivmodZeroDivisorThrows)
{
  Gen_Polynomial<int> a({1, 2, 3});
  Gen_Polynomial<int> z;
  EXPECT_THROW(a.pseudo_divmod(z), std::domain_error);
}

// ===================================================================
// Algebraic transformations
// ===================================================================

TEST(Polynomial, ToMonic)
{
  Polynomial p({2, 4, 6}); // 6x^2 + 4x + 2
  Polynomial m = p.to_monic();
  EXPECT_TRUE(m.is_monic());
  EXPECT_NEAR(m[2], 1.0, 1e-12);
  EXPECT_NEAR(m[1], 4.0/6.0, 1e-12);
  EXPECT_NEAR(m[0], 2.0/6.0, 1e-12);
}

TEST(Polynomial, ToMonicZeroThrows)
{
  Polynomial p;
  EXPECT_THROW(p.to_monic(), std::domain_error);
}

TEST(Polynomial, ReversePolynomial)
{
  // p(x) = 1 + 2x + 3x^2, reverse = 3 + 2x + x^2
  Polynomial p({1, 2, 3});
  Polynomial rev = p.reverse();
  EXPECT_NEAR(rev[0], 3.0, 1e-12);
  EXPECT_NEAR(rev[1], 2.0, 1e-12);
  EXPECT_NEAR(rev[2], 1.0, 1e-12);
}

TEST(Polynomial, ReverseOfZero)
{
  EXPECT_TRUE(Polynomial().reverse().is_zero());
}

TEST(Polynomial, ReversePalindrome)
{
  // Palindromic: 1 + 3x + 3x^2 + x^3
  Polynomial p({1, 3, 3, 1});
  EXPECT_EQ(p, p.reverse());
}

TEST(Polynomial, NegateArg)
{
  // p(x) = 1 + 2x + 3x^2, p(-x) = 1 - 2x + 3x^2
  Polynomial p({1, 2, 3});
  Polynomial neg = p.negate_arg();
  EXPECT_NEAR(neg[0], 1.0, 1e-12);
  EXPECT_NEAR(neg[1], -2.0, 1e-12);
  EXPECT_NEAR(neg[2], 3.0, 1e-12);
}

TEST(Polynomial, NegateArgVerify)
{
  // p(-x) evaluated at x should equal p evaluated at -x
  Polynomial p({-1, 3, 0, -2, 1});
  Polynomial neg = p.negate_arg();
  for (double x = -3.0; x <= 3.0; x += 0.7)
    EXPECT_NEAR(neg(x), p(-x), 1e-10);
}

TEST(Polynomial, Shift)
{
  // p(x) = x^2, shift by 1 => p(x+1) = (x+1)^2 = x^2 + 2x + 1
  Polynomial p({0, 0, 1});
  Polynomial shifted = p.shift(1.0);
  EXPECT_NEAR(shifted[0], 1.0, 1e-12);
  EXPECT_NEAR(shifted[1], 2.0, 1e-12);
  EXPECT_NEAR(shifted[2], 1.0, 1e-12);
}

TEST(Polynomial, ShiftVerify)
{
  Polynomial p({1, -2, 3});
  double k = 2.5;
  Polynomial shifted = p.shift(k);
  for (double x = -3.0; x <= 3.0; x += 0.7)
    EXPECT_NEAR(shifted(x), p(x + k), 1e-9);
}

TEST(Polynomial, ShiftUp)
{
  // p(x) = 1 + 2x, shift_up(2) => x^2 + 2x^3
  Polynomial p({1, 2});
  Polynomial up = p.shift_up(2);
  EXPECT_EQ(up.degree(), 3u);
  EXPECT_NEAR(up[0], 0.0, 1e-12);
  EXPECT_NEAR(up[1], 0.0, 1e-12);
  EXPECT_NEAR(up[2], 1.0, 1e-12);
  EXPECT_NEAR(up[3], 2.0, 1e-12);
}

TEST(Polynomial, ShiftDown)
{
  // p(x) = x^2 + 2x^3, shift_down(2) => 1 + 2x
  Polynomial p({0, 0, 1, 2});
  Polynomial down = p.shift_down(2);
  EXPECT_EQ(down.degree(), 1u);
  EXPECT_NEAR(down[0], 1.0, 1e-12);
  EXPECT_NEAR(down[1], 2.0, 1e-12);
}

TEST(Polynomial, ShiftDownDiscardsLowerTerms)
{
  // p(x) = 1 + x + x^2 + x^3, shift_down(2) => 1 + x
  Polynomial p({1, 1, 1, 1});
  Polynomial down = p.shift_down(2);
  EXPECT_EQ(down.degree(), 1u);
  EXPECT_NEAR(down[0], 1.0, 1e-12);
  EXPECT_NEAR(down[1], 1.0, 1e-12);
}

TEST(Polynomial, Truncate)
{
  // p(x) = 1 + 2x + 3x^2 + 4x^3, truncate(2) => 1 + 2x
  Polynomial p({1, 2, 3, 4});
  Polynomial t = p.truncate(2);
  EXPECT_EQ(t.degree(), 1u);
  EXPECT_NEAR(t[0], 1.0, 1e-12);
  EXPECT_NEAR(t[1], 2.0, 1e-12);
}

TEST(Polynomial, TruncateBeyondDegree)
{
  Polynomial p({1, 2, 3});
  Polynomial t = p.truncate(100);
  EXPECT_EQ(t, p);
}

TEST(Polynomial, ToDense)
{
  // p(x) = 1 + 3x^2 (sparse: no x^1 term)
  Polynomial p({1, 0, 3});
  Array<double> dense = p.to_dense();
  ASSERT_EQ(dense.size(), 3u);
  EXPECT_DOUBLE_EQ(dense(0), 1.0);
  EXPECT_DOUBLE_EQ(dense(1), 0.0);
  EXPECT_DOUBLE_EQ(dense(2), 3.0);
}

TEST(Polynomial, ToDenseOfZero)
{
  Polynomial p;
  Array<double> dense = p.to_dense();
  EXPECT_EQ(dense.size(), 0u);
}

TEST(Polynomial, ToDenseSparse)
{
  // x^5 + 1: should have 6 elements
  DynList<std::pair<size_t, double>> terms;
  terms.append(std::make_pair(size_t(0), 1.0));
  terms.append(std::make_pair(size_t(5), 1.0));
  Polynomial p(terms);
  Array<double> dense = p.to_dense();
  ASSERT_EQ(dense.size(), 6u);
  EXPECT_DOUBLE_EQ(dense(0), 1.0);
  EXPECT_DOUBLE_EQ(dense(1), 0.0);
  EXPECT_DOUBLE_EQ(dense(2), 0.0);
  EXPECT_DOUBLE_EQ(dense(3), 0.0);
  EXPECT_DOUBLE_EQ(dense(4), 0.0);
  EXPECT_DOUBLE_EQ(dense(5), 1.0);
}

// ===================================================================
// Square-free
// ===================================================================

TEST(Polynomial, SquareFreeBasic)
{
  // (x-1)^2 * (x-2) = x^3 - 4x^2 + 5x - 2
  // square-free part = (x-1)(x-2) = x^2 - 3x + 2
  Polynomial factor = Polynomial::from_roots(DynList<double>({1.0}));
  Polynomial p = factor * factor * Polynomial::from_roots(DynList<double>({2.0}));

  Polynomial sf = p.square_free();
  EXPECT_EQ(sf.degree(), 2u);
  EXPECT_NEAR(sf(1.0), 0.0, 1e-8);
  EXPECT_NEAR(sf(2.0), 0.0, 1e-8);
}

TEST(Polynomial, SquareFreeAlreadySquareFree)
{
  Polynomial p = Polynomial::from_roots(DynList<double>({1.0, 2.0, 3.0}));
  Polynomial sf = p.square_free();
  // Should have same roots, same degree
  EXPECT_EQ(sf.degree(), 3u);
  EXPECT_NEAR(sf(1.0), 0.0, 1e-8);
  EXPECT_NEAR(sf(2.0), 0.0, 1e-8);
  EXPECT_NEAR(sf(3.0), 0.0, 1e-8);
}

// ===================================================================
// Cauchy bound
// ===================================================================

TEST(Polynomial, CauchyBound)
{
  // p(x) = (x-1)(x-2)(x-3) = x^3 - 6x^2 + 11x - 6
  // All roots are |r| <= 3. Cauchy bound should be >= 3.
  Polynomial p = Polynomial::from_roots(DynList<double>({1.0, 2.0, 3.0}));
  double bound = p.cauchy_bound();
  EXPECT_GE(bound, 3.0);
}

TEST(Polynomial, CauchyBoundConstant)
{
  Polynomial p(5.0);
  EXPECT_DOUBLE_EQ(p.cauchy_bound(), 0.0);
}

TEST(Polynomial, CauchyBoundZeroThrows)
{
  Polynomial p;
  EXPECT_THROW(p.cauchy_bound(), std::domain_error);
}

// ===================================================================
// Descartes sign variations
// ===================================================================

TEST(Polynomial, SignVariationsPositiveRoots)
{
  // p(x) = x^2 - 3x + 2 = (x-1)(x-2). Coefficients: 2, -3, 1
  // Sign changes: + -> - -> + = 2 changes.
  // Descartes: at most 2 positive roots (and exactly 2).
  Polynomial p({2, -3, 1});
  EXPECT_EQ(p.sign_variations(), 2u);
}

TEST(Polynomial, SignVariationsNoChanges)
{
  // p(x) = 1 + x + x^2. All positive. No positive roots.
  Polynomial p({1, 1, 1});
  EXPECT_EQ(p.sign_variations(), 0u);
}

TEST(Polynomial, SignVariationsZero)
{
  Polynomial p;
  EXPECT_EQ(p.sign_variations(), 0u);
}

// ===================================================================
// Sturm chain and root counting
// ===================================================================

TEST(Polynomial, SturmChainLinear)
{
  Polynomial p({-5, 1}); // x - 5
  auto chain = p.sturm_chain();
  EXPECT_EQ(chain.size(), 2u); // p, p'
}

TEST(Polynomial, CountRealRootsQuadratic)
{
  // x^2 - 1 = (x-1)(x+1): 2 real roots
  Polynomial p({-1, 0, 1});
  EXPECT_EQ(p.count_real_roots(-10.0, 10.0), 2u);
  EXPECT_EQ(p.count_real_roots(0.0, 10.0), 1u);
  EXPECT_EQ(p.count_real_roots(2.0, 10.0), 0u);
}

TEST(Polynomial, CountRealRootsCubic)
{
  // (x-1)(x-2)(x-3) has 3 real roots
  Polynomial p = Polynomial::from_roots(DynList<double>({1.0, 2.0, 3.0}));
  EXPECT_EQ(p.count_real_roots(-10.0, 10.0), 3u);
  EXPECT_EQ(p.count_real_roots(0.0, 1.5), 1u);
  EXPECT_EQ(p.count_real_roots(1.5, 2.5), 1u);
  EXPECT_EQ(p.count_real_roots(2.5, 10.0), 1u);
}

TEST(Polynomial, CountRealRootsReversedInterval)
{
  Polynomial p({-1, 0, 1}); // x^2 - 1
  EXPECT_EQ(p.count_real_roots(10.0, -10.0), 2u);
}

TEST(Polynomial, CountAllRealRoots)
{
  // x^4 - 5x^2 + 4 = (x-1)(x+1)(x-2)(x+2): 4 real roots
  Polynomial p({4, 0, -5, 0, 1});
  EXPECT_EQ(p.count_all_real_roots(), 4u);
}

TEST(Polynomial, CountRealRootsNoReal)
{
  // x^2 + 1: no real roots
  Polynomial p({1, 0, 1});
  EXPECT_EQ(p.count_all_real_roots(), 0u);
}

// ===================================================================
// Bisection root finding
// ===================================================================

TEST(Polynomial, BisectRootLinear)
{
  // x - 3 = 0 => root at 3
  Polynomial p({-3, 1});
  double root = p.bisect_root(0.0, 10.0);
  EXPECT_NEAR(root, 3.0, 1e-10);
}

TEST(Polynomial, BisectRootQuadratic)
{
  // x^2 - 2 => root at sqrt(2) ≈ 1.4142
  Polynomial p({-2, 0, 1});
  double root = p.bisect_root(1.0, 2.0);
  EXPECT_NEAR(root, std::sqrt(2.0), 1e-10);
}

TEST(Polynomial, BisectRootSameSignThrows)
{
  Polynomial p({1, 0, 1}); // x^2 + 1, always positive
  EXPECT_THROW(p.bisect_root(1.0, 2.0), std::domain_error);
}

TEST(Polynomial, BisectRootReversedBounds)
{
  Polynomial p({-2, 1}); // x - 2
  EXPECT_NEAR(p.bisect_root(10.0, 0.0), 2.0, 1e-10);
}

// ===================================================================
// Newton-Raphson root finding
// ===================================================================

TEST(Polynomial, NewtonRootLinear)
{
  Polynomial p({-5, 2}); // 2x - 5 => root at 2.5
  double root = p.newton_root(0.0);
  EXPECT_NEAR(root, 2.5, 1e-10);
}

TEST(Polynomial, NewtonRootQuadratic)
{
  // x^2 - 2 => root at sqrt(2)
  Polynomial p({-2, 0, 1});
  double root = p.newton_root(1.5);
  EXPECT_NEAR(root, std::sqrt(2.0), 1e-10);
}

TEST(Polynomial, NewtonRootCubic)
{
  // x^3 - 27 => root at 3
  Polynomial p({-27, 0, 0, 1});
  double root = p.newton_root(2.0);
  EXPECT_NEAR(root, 3.0, 1e-10);
}

TEST(Polynomial, NewtonRootZeroDerivativeThrows)
{
  // x^2 + 1 has derivative 2x which is zero at x=0, but f(0)=1 != 0
  Polynomial p({1, 0, 1});
  EXPECT_THROW(p.newton_root(0.0), std::domain_error);
}

// ===================================================================
// Integration: shift_up / shift_down round-trip
// ===================================================================

TEST(Polynomial, ShiftUpDownRoundTrip)
{
  Polynomial p({1, 2, 3});
  Polynomial roundtrip = p.shift_up(5).shift_down(5);
  EXPECT_EQ(roundtrip, p);
}

TEST(Polynomial, ShiftUpZero)
{
  Polynomial p({1, 2, 3});
  EXPECT_EQ(p.shift_up(0), p);
}

// ===================================================================
// Truncate + shift integration
// ===================================================================

TEST(Polynomial, TruncateIsModXN)
{
  // p(x) mod x^n: only lower terms survive
  Polynomial p({1, 2, 3, 4, 5});
  Polynomial t = p.truncate(3);
  // Check that t(x) = p(x) mod x^3
  EXPECT_NEAR(t[0], 1.0, 1e-12);
  EXPECT_NEAR(t[1], 2.0, 1e-12);
  EXPECT_NEAR(t[2], 3.0, 1e-12);
  EXPECT_FALSE(t.has_term(3));
  EXPECT_FALSE(t.has_term(4));
}

// ===================================================================
// Layer 5 — Exact Univariate Factorization (Cantor-Zassenhaus)
// ===================================================================

namespace {

using IntPoly = Gen_Polynomial<long long>;

IntPoly reconstruct_factorization(const DynList<IntPoly::SfdTerm> &factors)
{
  IntPoly product(1);

  for (const auto &term : factors)
    {
      IntPoly block(1);
      for (size_t i = 0; i < term.multiplicity; ++i)
        block *= term.factor;
      product *= block;
    }

  return product;
}

bool congruent_mod(const IntPoly &a, const IntPoly &b, long long mod)
{
  const size_t max_degree = std::max(a.degree(), b.degree());
  for (size_t exp = 0; exp <= max_degree; ++exp)
    {
      long long diff = (a.get_coeff(exp) - b.get_coeff(exp)) % mod;
      if (diff < 0)
        diff += mod;
      if (diff != 0)
        return false;
    }

  return true;
}

} // namespace

// --- Content Tests ---

TEST(PolyLayer5, ContentBasic)
{
  // content(6x^2 + 4x + 2) = 2
  IntPoly p;
  p.set_coeff(0, 2);
  p.set_coeff(1, 4);
  p.set_coeff(2, 6);
  EXPECT_EQ(p.content(), 2);
}

TEST(PolyLayer5, ContentMonic)
{
  // content(x^2 + 2x + 3) = 1 (gcd(3, 2, 1))
  IntPoly p;
  p.set_coeff(0, 3);
  p.set_coeff(1, 2);
  p.set_coeff(2, 1);
  EXPECT_EQ(p.content(), 1);
}

TEST(PolyLayer5, ContentNegativeLeading)
{
  // content(-4x + 2) = -2 (negative leading coeff)
  IntPoly p;
  p.set_coeff(0, 2);
  p.set_coeff(1, -4);
  EXPECT_EQ(p.content(), -2);
}

TEST(PolyLayer5, ContentZero)
{
  IntPoly p;
  EXPECT_EQ(p.content(), 0);
}

// --- Primitive Part Tests ---

TEST(PolyLayer5, PrimitivePartDividesByContent)
{
  // primitive_part(6x^2 + 4x + 2) = 3x^2 + 2x + 1
  IntPoly p;
  p.set_coeff(0, 2);
  p.set_coeff(1, 4);
  p.set_coeff(2, 6);
  auto prim = p.primitive_part();
  EXPECT_EQ(prim.get_coeff(0), 1);
  EXPECT_EQ(prim.get_coeff(1), 2);
  EXPECT_EQ(prim.get_coeff(2), 3);
}

TEST(PolyLayer5, PrimitivePartLeadPositive)
{
  // primitive_part(-4x + 2) → lead coeff positive
  IntPoly p;
  p.set_coeff(0, 2);
  p.set_coeff(1, -4);
  auto prim = p.primitive_part();
  EXPECT_GT(prim.leading_coeff(), 0);
}

TEST(PolyLayer5, PrimitivePartZeroPoly)
{
  IntPoly p;
  auto prim = p.primitive_part();
  EXPECT_TRUE(prim.is_zero());
}

// --- Integer GCD Tests ---

TEST(PolyLayer5, IntegerGcdLinearLinear)
{
  // gcd(x - 1, x - 1) = x - 1
  IntPoly a, b;
  a.set_coeff(0, -1);
  a.set_coeff(1, 1);
  b.set_coeff(0, -1);
  b.set_coeff(1, 1);
  auto g = IntPoly::integer_gcd(a, b);
  EXPECT_EQ(g.degree(), 1);
  EXPECT_TRUE(g.get_coeff(0) == -1 or g.get_coeff(0) == 1);
}

TEST(PolyLayer5, IntegerGcdCommonFactor)
{
  // gcd(x^2 - 1, x - 1) = x - 1
  IntPoly a, b;
  a.set_coeff(0, -1);
  a.set_coeff(2, 1);
  b.set_coeff(0, -1);
  b.set_coeff(1, 1);
  auto g = IntPoly::integer_gcd(a, b);
  EXPECT_EQ(g.degree(), 1);
}

TEST(PolyLayer5, IntegerGcdCoprime)
{
  // gcd(x + 1, x + 2) = 1 (coprime)
  IntPoly a, b;
  a.set_coeff(0, 1);
  a.set_coeff(1, 1);
  b.set_coeff(0, 2);
  b.set_coeff(1, 1);
  auto g = IntPoly::integer_gcd(a, b);
  EXPECT_TRUE(g.is_constant());
}

TEST(PolyLayer5, IntegerGcdHighDegree)
{
  // gcd(x^3 - 1, x^2 - 1) = x - 1
  IntPoly a, b;
  a.set_coeff(0, -1);
  a.set_coeff(3, 1);
  b.set_coeff(0, -1);
  b.set_coeff(2, 1);
  auto g = IntPoly::integer_gcd(a, b);
  EXPECT_EQ(g.degree(), 1);
}

// --- Yun SFD Tests ---

TEST(PolyLayer5, YunSfdSquareFree)
{
  // x^2 + 2x + 1 = (x+1)^2, so SFD gives (x+1)^2
  IntPoly p;
  p.set_coeff(0, 1);
  p.set_coeff(1, 2);
  p.set_coeff(2, 1);
  auto sfd = p.yun_sfd();
  EXPECT_TRUE(sfd.size() >= 1);
  EXPECT_EQ(sfd.get_first().multiplicity, 2);
}

TEST(PolyLayer5, YunSfdRepeatedDeg2)
{
  // (x^2 - 1)^2 = (x-1)^2(x+1)^2
  IntPoly f, p;
  f.set_coeff(0, -1);
  f.set_coeff(2, 1);
  p = f * f;
  auto sfd = p.yun_sfd();
  EXPECT_EQ(sfd.size(), 1);
  EXPECT_EQ(sfd.get_first().multiplicity, 2);
}

TEST(PolyLayer5, YunSfdMixedDeg3)
{
  // x^3 - x = x(x-1)(x+1) (square-free)
  IntPoly p;
  p.set_coeff(0, 0);
  p.set_coeff(1, -1);
  p.set_coeff(3, 1);
  auto sfd = p.yun_sfd();
  // Should be square-free or show multiplicity 1
  for (auto &term : sfd)
    EXPECT_EQ(term.multiplicity, 1);
}

TEST(PolyLayer5, YunSfdDeg4Triple)
{
  // (x^2 - 1)^3 (repeated degree 2)
  IntPoly f, p;
  f.set_coeff(0, -1);
  f.set_coeff(2, 1);
  p = f * f * f;
  auto sfd = p.yun_sfd();
  EXPECT_EQ(sfd.get_first().multiplicity, 3);
}

TEST(PolyLayer5, YunSfdConstant)
{
  // Constant polynomial
  IntPoly p(5);
  auto sfd = p.yun_sfd();
  EXPECT_TRUE(sfd.is_empty());
}

// --- Factor Mod P Tests ---

TEST(PolyLayer5, FactorModPLinear)
{
  // x - 2 over F_7 is irreducible
  IntPoly p;
  p.set_coeff(0, -2);
  p.set_coeff(1, 1);
  auto factors = IntPoly::factor_mod_p(p, 7);
  EXPECT_EQ(factors.size(), 1);
}

TEST(PolyLayer5, FactorModPQuadSplits)
{
  // x^2 - 1 = (x-1)(x+1) over any odd prime
  IntPoly p;
  p.set_coeff(0, -1);
  p.set_coeff(2, 1);
  auto factors = IntPoly::factor_mod_p(p, 5);
  EXPECT_EQ(factors.size(), 2);
}

TEST(PolyLayer5, FactorModPCubic3Roots)
{
  // x^3 - x = x(x-1)(x+1) over F_5 (3 roots)
  IntPoly p;
  p.set_coeff(0, 0);
  p.set_coeff(1, -1);
  p.set_coeff(3, 1);
  auto factors = IntPoly::factor_mod_p(p, 5);
  EXPECT_GE(factors.size(), 1);
}

TEST(PolyLayer5, FactorModPIrred)
{
  // x^2 + 1 is irreducible over F_3
  IntPoly p;
  p.set_coeff(0, 1);
  p.set_coeff(2, 1);
  auto factors = IntPoly::factor_mod_p(p, 3);
  EXPECT_EQ(factors.size(), 1);
}

TEST(PolyLayer5, FactorModPProduct)
{
  // (x-1)(x-2)(x-3) over F_7
  IntPoly f1, f2, f3;
  f1.set_coeff(0, -1);
  f1.set_coeff(1, 1);
  f2.set_coeff(0, -2);
  f2.set_coeff(1, 1);
  f3.set_coeff(0, -3);
  f3.set_coeff(1, 1);
  IntPoly p = f1 * f2 * f3;
  auto factors = IntPoly::factor_mod_p(p, 7);
  EXPECT_GE(factors.size(), 1);
}

TEST(PolyLayer5, FactorModPRepeatedRootMultiplicity)
{
  // (x - 1)^2 should expose the repeated linear factor over F_5.
  IntPoly linear;
  linear.set_coeff(0, -1);
  linear.set_coeff(1, 1);

  auto factors = IntPoly::factor_mod_p(linear * linear, 5);
  ASSERT_EQ(factors.size(), 2);
  for (const auto &factor : factors)
    {
      EXPECT_EQ(factor.degree(), 1u);
      EXPECT_EQ(factor.get_coeff(1), 1);
      EXPECT_EQ(factor.get_coeff(0), 4);
    }
}

// --- Mignotte Bound Tests ---

TEST(PolyLayer5, MignotteBoundLinear)
{
  // x - 1, degree 1, max|c| = 1 → sqrt(2)*2*1
  IntPoly p;
  p.set_coeff(0, -1);
  p.set_coeff(1, 1);
  double bound = p.mignotte_bound();
  EXPECT_GT(bound, 0);
  EXPECT_LT(bound, 10);
}

TEST(PolyLayer5, MignotteBoundDeg4)
{
  // x^4 - 1, degree 4, max|c| = 1
  IntPoly p;
  p.set_coeff(0, -1);
  p.set_coeff(4, 1);
  double bound = p.mignotte_bound();
  EXPECT_GT(bound, 10);
}

// --- Hensel Lift Tests ---

TEST(PolyLayer5, HenselLiftPair)
{
  // x^2 - 6 ≡ (x-1)(x+1) mod 5, and the lifted roots mod 25 are ±9.
  IntPoly f1, f2;
  f1.set_coeff(0, -1);
  f1.set_coeff(1, 1);
  f2.set_coeff(0, 1);
  f2.set_coeff(1, 1);

  IntPoly f;
  f.set_coeff(0, -6);
  f.set_coeff(2, 1);

  DynList<IntPoly> factors;
  factors.append(f1);
  factors.append(f2);
  auto lifted = IntPoly::hensel_lift(f, factors, 5, 1);
  EXPECT_EQ(lifted.size(), 2);

  IntPoly product(1);
  for (const auto &factor : lifted)
    product *= factor;

  EXPECT_TRUE(congruent_mod(product, f, 25));
}

TEST(PolyLayer5, HenselLiftCubic)
{
  // Lift factors from mod 3
  IntPoly f;
  f.set_coeff(0, 1);
  f.set_coeff(1, 1);
  f.set_coeff(3, 1);
  DynList<IntPoly> factors;
  factors.append(f);
  auto lifted = IntPoly::hensel_lift(f, factors, 3, 1);
  EXPECT_GE(lifted.size(), 1);
}

TEST(PolyLayer5, HenselLiftLevel2)
{
  // Lift to modulus 5^4 = 625 and preserve congruence.
  IntPoly f1, f2;
  f1.set_coeff(0, -1);
  f1.set_coeff(1, 1);
  f2.set_coeff(0, 1);
  f2.set_coeff(1, 1);

  IntPoly f;
  f.set_coeff(0, -6);
  f.set_coeff(2, 1);

  DynList<IntPoly> factors;
  factors.append(f1);
  factors.append(f2);
  auto lifted = IntPoly::hensel_lift(f, factors, 5, 2);
  EXPECT_EQ(lifted.size(), 2);

  IntPoly product(1);
  for (const auto &factor : lifted)
    product *= factor;

  EXPECT_TRUE(congruent_mod(product, f, 625));
}

TEST(PolyLayer5, HenselLiftSingle)
{
  // Single factor: just reduce modulo p^(2^levels)
  IntPoly f;
  f.set_coeff(0, 1);
  f.set_coeff(1, 1);
  DynList<IntPoly> factors;
  factors.append(f);
  auto lifted = IntPoly::hensel_lift(f, factors, 7, 1);
  EXPECT_EQ(lifted.size(), 1);
}

// --- Factorize Tests ---

TEST(PolyLayer5, FactorizeX4Minus1)
{
  // x^4 - 1 = (x-1)(x+1)(x^2+1) over Z
  IntPoly p;
  p.set_coeff(0, -1);
  p.set_coeff(4, 1);
  auto fact = p.factorize();
  EXPECT_EQ(reconstruct_factorization(fact), p);
}

TEST(PolyLayer5, FactorizeQuarticIntoQuadratics)
{
  // x^4 + 4x^2 + 3 = (x^2 + 1)(x^2 + 3)
  IntPoly q1({1, 0, 1});
  IntPoly q2({3, 0, 1});
  IntPoly p = q1 * q2;

  auto fact = p.factorize();

  EXPECT_EQ(reconstruct_factorization(fact), p);

  bool found_q1 = false;
  bool found_q2 = false;
  for (const auto &term : fact)
    {
      if (term.multiplicity != 1)
        continue;
      found_q1 = found_q1 or (term.factor == q1);
      found_q2 = found_q2 or (term.factor == q2);
    }

  EXPECT_TRUE(found_q1);
  EXPECT_TRUE(found_q2);
}

TEST(PolyLayer5, FactorizeNonMonicQuarticIntoQuadratics)
{
  // (2x^2 + 1)(3x^2 + 2)
  IntPoly q1({1, 0, 2});
  IntPoly q2({2, 0, 3});
  IntPoly p = q1 * q2;

  auto fact = p.factorize();

  EXPECT_EQ(reconstruct_factorization(fact), p);

  bool found_q1 = false;
  bool found_q2 = false;
  for (const auto &term : fact)
    {
      if (term.multiplicity != 1)
        continue;
      found_q1 = found_q1 or (term.factor == q1);
      found_q2 = found_q2 or (term.factor == q2);
    }

  EXPECT_TRUE(found_q1);
  EXPECT_TRUE(found_q2);
}

TEST(PolyLayer5, FactorizeSexticIntoCubics)
{
  // (x^3 + x + 1)(x^3 + x + 3)
  IntPoly c1({1, 1, 0, 1});
  IntPoly c2({3, 1, 0, 1});
  IntPoly p = c1 * c2;

  auto fact = p.factorize();

  EXPECT_EQ(reconstruct_factorization(fact), p);

  bool found_c1 = false;
  bool found_c2 = false;
  for (const auto &term : fact)
    {
      if (term.multiplicity != 1)
        continue;
      found_c1 = found_c1 or (term.factor == c1);
      found_c2 = found_c2 or (term.factor == c2);
    }

  EXPECT_TRUE(found_c1);
  EXPECT_TRUE(found_c2);
}

TEST(PolyLayer5, FactorizeX3)
{
  // x^3 = x * x * x (cube of linear factor)
  IntPoly p;
  p.set_coeff(3, 1);
  auto fact = p.factorize();
  // Should find x^3 or x with multiplicity 3
  bool found_cubic_or_triple = false;
  for (auto &term : fact)
    {
      if ((term.factor.degree() == 3 and term.multiplicity == 1) or
          (term.factor.degree() == 1 and term.multiplicity == 3))
        found_cubic_or_triple = true;
    }
  EXPECT_TRUE(found_cubic_or_triple);
}

TEST(PolyLayer5, FactorizeMultipleRoots)
{
  // (x-1)^2 * (x+1) = (x^2 - 2x + 1)(x+1)
  IntPoly f1, f2;
  f1.set_coeff(0, 1);
  f1.set_coeff(1, -2);
  f1.set_coeff(2, 1);
  f2.set_coeff(0, 1);
  f2.set_coeff(1, 1);
  IntPoly p = f1 * f2;
  auto fact = p.factorize();
  EXPECT_EQ(reconstruct_factorization(fact), p);
}

TEST(PolyLayer5, FactorizeRationalLinearFactors)
{
  // (2x + 1)(x - 3) should be recovered exactly over Z.
  IntPoly f1, f2;
  f1.set_coeff(0, 1);
  f1.set_coeff(1, 2);
  f2.set_coeff(0, -3);
  f2.set_coeff(1, 1);

  IntPoly p = f1 * f2;
  auto    fact = p.factorize();

  EXPECT_EQ(reconstruct_factorization(fact), p);

  bool found_two_x_plus_one = false;
  bool found_x_minus_three  = false;
  for (const auto &term : fact)
    {
      if (term.multiplicity != 1)
        continue;
      found_two_x_plus_one = found_two_x_plus_one or (term.factor == f1);
      found_x_minus_three  = found_x_minus_three or (term.factor == f2);
    }

  EXPECT_TRUE(found_two_x_plus_one);
  EXPECT_TRUE(found_x_minus_three);
}
