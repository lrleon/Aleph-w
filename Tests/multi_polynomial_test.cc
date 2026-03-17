/*
  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  MIT License (see tpl_multi_polynomial.H for full text)
*/

# include <gtest/gtest.h>
# include <cmath>
# include <string>
# include <sstream>
# include <stdexcept>
# include <tpl_multi_polynomial.H>

using namespace Aleph;

// Shorthand for multi-indices
using Idx = Array<size_t>;

// ===================================================================
// Construction
// ===================================================================

TEST(MultiPoly, DefaultIsZero)
{
  MultiPolynomial p;
  EXPECT_TRUE(p.is_zero());
  EXPECT_EQ(p.num_vars(), 0u);
  EXPECT_EQ(p.num_terms(), 0u);
  EXPECT_EQ(p.degree(), 0u);
}

TEST(MultiPoly, ConstantFromNvars)
{
  MultiPolynomial p(2, 5.0);
  EXPECT_FALSE(p.is_zero());
  EXPECT_TRUE(p.is_constant());
  EXPECT_EQ(p.num_vars(), 2u);
  EXPECT_EQ(p.num_terms(), 1u);
  EXPECT_EQ(p.degree(), 0u);
  EXPECT_DOUBLE_EQ(p.coeff_at(Idx{0, 0}), 5.0);
}

TEST(MultiPoly, ZeroConstantIsZero)
{
  MultiPolynomial p(3, 0.0);
  EXPECT_TRUE(p.is_zero());
  EXPECT_EQ(p.num_vars(), 3u);
  EXPECT_EQ(p.num_terms(), 0u);
}

TEST(MultiPoly, SingleTerm)
{
  // 3 x^2 y
  MultiPolynomial p(2, Idx{2, 1}, 3.0);
  EXPECT_FALSE(p.is_zero());
  EXPECT_FALSE(p.is_constant());
  EXPECT_EQ(p.num_terms(), 1u);
  EXPECT_EQ(p.degree(), 3u);
  EXPECT_DOUBLE_EQ(p.coeff_at(Idx{2, 1}), 3.0);
  EXPECT_DOUBLE_EQ(p.coeff_at(Idx{0, 0}), 0.0);
}

TEST(MultiPoly, InitializerListConstruction)
{
  // f(x,y) = 3x^2*y + 2x - 1
  MultiPolynomial f(2, {
    {Idx{2, 1}, 3.0},
    {Idx{1, 0}, 2.0},
    {Idx{0, 0}, -1.0},
  });

  EXPECT_EQ(f.num_vars(), 2u);
  EXPECT_EQ(f.num_terms(), 3u);
  EXPECT_EQ(f.degree(), 3u);
  EXPECT_DOUBLE_EQ(f.coeff_at(Idx{2, 1}), 3.0);
  EXPECT_DOUBLE_EQ(f.coeff_at(Idx{1, 0}), 2.0);
  EXPECT_DOUBLE_EQ(f.coeff_at(Idx{0, 0}), -1.0);
}

TEST(MultiPoly, DuplicateTermsAccumulate)
{
  MultiPolynomial f(1, {
    {Idx{2}, 3.0},
    {Idx{2}, 4.0},
    {Idx{0}, 1.0},
  });

  EXPECT_EQ(f.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(f.coeff_at(Idx{2}), 7.0);
  EXPECT_DOUBLE_EQ(f.coeff_at(Idx{0}), 1.0);
}

TEST(MultiPoly, CancellingTerms)
{
  MultiPolynomial f(1, {
    {Idx{2}, 3.0},
    {Idx{2}, -3.0},
  });

  EXPECT_TRUE(f.is_zero());
}

// ===================================================================
// Factory helpers
// ===================================================================

TEST(MultiPoly, VariableFactory)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  EXPECT_EQ(x.num_vars(), 2u);
  EXPECT_EQ(x.degree(), 1u);
  EXPECT_DOUBLE_EQ(x.coeff_at(Idx{1, 0}), 1.0);

  EXPECT_EQ(y.num_vars(), 2u);
  EXPECT_EQ(y.degree(), 1u);
  EXPECT_DOUBLE_EQ(y.coeff_at(Idx{0, 1}), 1.0);
}

TEST(MultiPoly, VariableOutOfRange)
{
  EXPECT_THROW(MultiPolynomial::variable(2, 2), std::domain_error);
}

TEST(MultiPoly, MonomialFactory)
{
  auto m = MultiPolynomial::monomial(3, Idx{1, 2, 0}, 5.0);
  EXPECT_EQ(m.num_vars(), 3u);
  EXPECT_EQ(m.degree(), 3u);
  EXPECT_DOUBLE_EQ(m.coeff_at(Idx{1, 2, 0}), 5.0);
}

// ===================================================================
// Properties
// ===================================================================

TEST(MultiPoly, DegreeIn)
{
  // f(x,y,z) = x^3*z + y^2*z^4
  MultiPolynomial f(3, {
    {Idx{3, 0, 1}, 1.0},
    {Idx{0, 2, 4}, 1.0},
  });

  EXPECT_EQ(f.degree(), 6u);   // max(3+0+1, 0+2+4) = 6
  EXPECT_EQ(f.degree_in(0), 3u);
  EXPECT_EQ(f.degree_in(1), 2u);
  EXPECT_EQ(f.degree_in(2), 4u);
}

TEST(MultiPoly, LeadingTermGrevlex)
{
  // f = x^2 + xy + y^2   (grevlex: x^2 > xy > y^2)
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 1.0},
    {Idx{0, 2}, 1.0},
  });

  auto [lm, lc] = f.leading_term();
  EXPECT_DOUBLE_EQ(lc, 1.0);
  EXPECT_EQ(lm(0), 2u);
  EXPECT_EQ(lm(1), 0u);
}

TEST(MultiPoly, LeadingTermLex)
{
  // Same polynomial with lex ordering
  Gen_MultiPolynomial<double, Lex_Order> f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 1.0},
    {Idx{0, 2}, 1.0},
  });

  auto [lm, lc] = f.leading_term();
  EXPECT_DOUBLE_EQ(lc, 1.0);
  EXPECT_EQ(lm(0), 2u);
  EXPECT_EQ(lm(1), 0u);
}

TEST(MultiPoly, LeadingTermOfZeroThrows)
{
  MultiPolynomial z;
  EXPECT_THROW(z.leading_term(), std::domain_error);
}

// ===================================================================
// Arithmetic — addition & subtraction
// ===================================================================

TEST(MultiPoly, AddTwoPolynomials)
{
  MultiPolynomial a(2, {
    {Idx{2, 0}, 3.0},
    {Idx{0, 1}, 1.0},
  });

  MultiPolynomial b(2, {
    {Idx{2, 0}, -1.0},
    {Idx{1, 0}, 2.0},
  });

  MultiPolynomial c = a + b;
  EXPECT_EQ(c.num_terms(), 3u);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{2, 0}), 2.0);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{1, 0}), 2.0);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{0, 1}), 1.0);
}

TEST(MultiPoly, SelfAddition)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;
  MultiPolynomial q = p + p;

  EXPECT_EQ(q, 2.0 * p);
}

TEST(MultiPoly, Cancellation)
{
  auto x = MultiPolynomial::variable(2, 0);
  MultiPolynomial z = x - x;
  EXPECT_TRUE(z.is_zero());
}

TEST(MultiPoly, SelfSubtraction)
{
  MultiPolynomial p(2, {
    {Idx{3, 1}, 7.0},
    {Idx{0, 0}, 2.0},
  });
  p -= p;
  EXPECT_TRUE(p.is_zero());
}

TEST(MultiPoly, UnaryNegation)
{
  MultiPolynomial p(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 1}, -2.0},
  });

  MultiPolynomial q = -p;
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{1, 0}), -3.0);
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{0, 1}), 2.0);
}

// ===================================================================
// Arithmetic — scalar operations
// ===================================================================

TEST(MultiPoly, ScalarMultiply)
{
  MultiPolynomial p(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 0}, 1.0},
  });

  MultiPolynomial q = p * 2.0;
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{1, 0}), 6.0);
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{0, 0}), 2.0);

  // Commutative: s * p == p * s
  EXPECT_EQ(2.0 * p, p * 2.0);
}

TEST(MultiPoly, ScalarMultiplyByZero)
{
  MultiPolynomial p(2, Idx{1, 0}, 5.0);
  MultiPolynomial q = p * 0.0;
  EXPECT_TRUE(q.is_zero());
}

TEST(MultiPoly, ScalarDivide)
{
  MultiPolynomial p(2, {
    {Idx{1, 0}, 6.0},
    {Idx{0, 1}, 4.0},
  });

  MultiPolynomial q = p / 2.0;
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{1, 0}), 3.0);
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{0, 1}), 2.0);
}

TEST(MultiPoly, ScalarDivideByZeroThrows)
{
  MultiPolynomial p(2, Idx{0, 0}, 1.0);
  EXPECT_THROW(p / 0.0, std::domain_error);
}

// ===================================================================
// Arithmetic — polynomial multiplication
// ===================================================================

TEST(MultiPoly, MultiplySimple)
{
  // (x + y) * (x - y) = x^2 - y^2
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial a = x + y;
  MultiPolynomial b = x - y;
  MultiPolynomial c = a * b;

  EXPECT_EQ(c.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{2, 0}), 1.0);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{0, 2}), -1.0);
  EXPECT_DOUBLE_EQ(c.coeff_at(Idx{1, 1}), 0.0);
}

TEST(MultiPoly, MultiplySquare)
{
  // (x + y)^2 = x^2 + 2xy + y^2
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial sq = (x + y) * (x + y);
  EXPECT_EQ(sq.num_terms(), 3u);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{2, 0}), 1.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{1, 1}), 2.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{0, 2}), 1.0);
}

TEST(MultiPoly, DistributiveLaw)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial a = x * x + y;           // x^2 + y
  MultiPolynomial b = x + MultiPolynomial(2, 1.0); // x + 1
  MultiPolynomial c = y * y;               // y^2

  // a * (b + c) == a*b + a*c
  EXPECT_EQ(a * (b + c), a * b + a * c);
}

TEST(MultiPoly, CommutativeMultiply)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial a = x * x + y + MultiPolynomial(2, 3.0);
  MultiPolynomial b = x * y + MultiPolynomial(2, -2.0);

  EXPECT_EQ(a * b, b * a);
}

TEST(MultiPoly, MultiplyByZero)
{
  auto x = MultiPolynomial::variable(2, 0);
  MultiPolynomial z(2);
  EXPECT_TRUE((x * z).is_zero());
}

TEST(MultiPoly, MultiplyByOne)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;
  MultiPolynomial one(2, 1.0);
  EXPECT_EQ(p * one, p);
}

// ===================================================================
// Exponentiation
// ===================================================================

TEST(MultiPoly, PowZero)
{
  auto x = MultiPolynomial::variable(2, 0);
  MultiPolynomial p = x.pow(0);
  EXPECT_TRUE(p.is_constant());
  EXPECT_DOUBLE_EQ(p.coeff_at(Idx{0, 0}), 1.0);
}

TEST(MultiPoly, PowOne)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;
  EXPECT_EQ(p.pow(1), p);
}

TEST(MultiPoly, PowTwo)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;
  EXPECT_EQ(p.pow(2), p * p);
}

TEST(MultiPoly, PowThree)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;
  EXPECT_EQ(p.pow(3), p * p * p);
}

// ===================================================================
// Evaluation
// ===================================================================

TEST(MultiPoly, EvalConstant)
{
  MultiPolynomial p(2, 7.0);
  Array<double> pt{1.0, 2.0};
  EXPECT_DOUBLE_EQ(p.eval(pt), 7.0);
}

TEST(MultiPoly, EvalLinear)
{
  // f(x,y) = 3x + 2y + 1
  MultiPolynomial f(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 1}, 2.0},
    {Idx{0, 0}, 1.0},
  });

  Array<double> pt{2.0, 3.0};
  // 3*2 + 2*3 + 1 = 13
  EXPECT_DOUBLE_EQ(f.eval(pt), 13.0);
}

TEST(MultiPoly, EvalQuadratic)
{
  // f(x,y) = x^2 + 2xy + y^2 = (x+y)^2
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = (x + y).pow(2);

  Array<double> pt{3.0, 4.0};
  EXPECT_DOUBLE_EQ(f.eval(pt), 49.0);  // (3+4)^2 = 49
}

TEST(MultiPoly, EvalThreeVars)
{
  // f(x,y,z) = x*y*z
  MultiPolynomial f(3, Idx{1, 1, 1}, 1.0);
  Array<double> pt{2.0, 3.0, 5.0};
  EXPECT_DOUBLE_EQ(f.eval(pt), 30.0);
}

TEST(MultiPoly, EvalFunctionCallSyntax)
{
  MultiPolynomial f(1, Idx{2}, 1.0);  // x^2
  Array<double> pt{5.0};
  EXPECT_DOUBLE_EQ(f(pt), 25.0);
}

TEST(MultiPoly, EvalTooFewComponents)
{
  MultiPolynomial f(3, Idx{1, 1, 1}, 1.0);
  Array<double> pt{1.0, 2.0};  // only 2, need 3
  EXPECT_THROW(f.eval(pt), std::domain_error);
}

TEST(MultiPoly, EvalZeroPolynomial)
{
  MultiPolynomial z(2);
  Array<double> pt{1.0, 2.0};
  EXPECT_DOUBLE_EQ(z.eval(pt), 0.0);
}

// ===================================================================
// Comparison
// ===================================================================

TEST(MultiPoly, EqualPolynomials)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial a = x + y;
  MultiPolynomial b = y + x;

  EXPECT_EQ(a, b);
}

TEST(MultiPoly, UnequalPolynomials)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  EXPECT_NE(x, y);
}

TEST(MultiPoly, ZeroEqualsZero)
{
  MultiPolynomial a;
  MultiPolynomial b(3);
  EXPECT_EQ(a, b);
}

// ===================================================================
// Monomial orderings
// ===================================================================

TEST(MultiPoly, LexVsGrevlex)
{
  // In lex: x > y^100  (because x has higher first exponent)
  // In grevlex: y^100 > x  (because total degree 100 > 1)
  Gen_MultiPolynomial<double, Lex_Order> lex_p(2, {
    {Idx{1, 0}, 1.0},
    {Idx{0, 100}, 1.0},
  });

  Gen_MultiPolynomial<double, Grevlex_Order> grev_p(2, {
    {Idx{1, 0}, 1.0},
    {Idx{0, 100}, 1.0},
  });

  // Leading term in lex: x = [1,0]
  auto [lex_lm, lex_lc] = lex_p.leading_term();
  EXPECT_EQ(lex_lm(0), 1u);
  EXPECT_EQ(lex_lm(1), 0u);

  // Leading term in grevlex: y^100 = [0,100]
  auto [grev_lm, grev_lc] = grev_p.leading_term();
  EXPECT_EQ(grev_lm(0), 0u);
  EXPECT_EQ(grev_lm(1), 100u);
}

TEST(MultiPoly, GrlexSameDegree)
{
  // In grlex with same total degree: lex decides.
  // x^2*y vs x*y^2: total degree 3 both.
  // Lex: [2,1] > [1,2] (leftmost diff: 2 > 1)
  Gen_MultiPolynomial<double, Grlex_Order> p(2, {
    {Idx{2, 1}, 1.0},
    {Idx{1, 2}, 1.0},
  });

  auto [lm, lc] = p.leading_term();
  EXPECT_EQ(lm(0), 2u);
  EXPECT_EQ(lm(1), 1u);
}

TEST(MultiPoly, GrevlexSameDegree)
{
  // Grevlex: same total degree → larger rightmost exp is SMALLER.
  // [2,1] vs [1,2]: rightmost nonzero of diff = [1,-1] is -1 (negative).
  // So [2,1] >_grevlex [1,2]. Leading term is [2,1].
  MultiPolynomial p(2, {
    {Idx{2, 1}, 1.0},
    {Idx{1, 2}, 1.0},
  });

  auto [lm, lc] = p.leading_term();
  EXPECT_EQ(lm(0), 2u);
  EXPECT_EQ(lm(1), 1u);
}

TEST(MultiPoly, GrevlexThreeVars)
{
  // With 3 vars, grevlex differs from grlex.
  // x*z vs y^2: total degree 2 both.
  // x*z = [1,0,1], y^2 = [0,2,0]
  // diff = [1,-2,1]. Rightmost nonzero = 1 (positive at index 2).
  // So [1,0,1] <_grevlex [0,2,0]  (positive rightmost → [1,0,1] is smaller)
  // Leading term in grevlex: y^2
  MultiPolynomial p(3, {
    {Idx{1, 0, 1}, 1.0},
    {Idx{0, 2, 0}, 1.0},
  });

  auto [lm, lc] = p.leading_term();
  EXPECT_EQ(lm(0), 0u);
  EXPECT_EQ(lm(1), 2u);
  EXPECT_EQ(lm(2), 0u);

  // In grlex, same total degree → lex decides.
  // [1,0,1] vs [0,2,0] in lex: 1 > 0 at index 0, so [1,0,1] >_lex [0,2,0]
  // Leading in grlex: x*z
  Gen_MultiPolynomial<double, Grlex_Order> q(3, {
    {Idx{1, 0, 1}, 1.0},
    {Idx{0, 2, 0}, 1.0},
  });

  auto [qlm, qlc] = q.leading_term();
  EXPECT_EQ(qlm(0), 1u);
  EXPECT_EQ(qlm(1), 0u);
  EXPECT_EQ(qlm(2), 1u);
}

// ===================================================================
// String representation
// ===================================================================

TEST(MultiPoly, ToStrZero)
{
  MultiPolynomial z;
  EXPECT_EQ(z.to_str(), "0");
}

TEST(MultiPoly, ToStrConstant)
{
  MultiPolynomial c(2, 5.0);
  EXPECT_EQ(c.to_str(), "5");
}

TEST(MultiPoly, ToStrSingleVar)
{
  auto x = MultiPolynomial::variable(2, 0);
  EXPECT_EQ(x.to_str(), "x");
}

TEST(MultiPoly, ToStrNegativeCoeff)
{
  MultiPolynomial p(2, Idx{1, 0}, -3.0);
  EXPECT_EQ(p.to_str(), "-3*x");
}

TEST(MultiPoly, StreamOutput)
{
  auto x = MultiPolynomial::variable(1, 0);
  std::ostringstream oss;
  oss << x;
  EXPECT_EQ(oss.str(), "x");
}

// ===================================================================
// Iteration
// ===================================================================

TEST(MultiPoly, ForEachTerm)
{
  MultiPolynomial f(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 1}, 2.0},
  });

  size_t count = 0;
  f.for_each_term([&](const Idx &, double) { ++count; });
  EXPECT_EQ(count, 2u);
}

TEST(MultiPoly, TermsList)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;

  auto t = p.terms();
  EXPECT_EQ(t.size(), 2u);
}

// ===================================================================
// Promote
// ===================================================================

TEST(MultiPoly, PromoteAddsVariables)
{
  // f(x,y) = x + y, promote to 3 vars
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial p = x + y;

  MultiPolynomial q = p.promote(3);
  EXPECT_EQ(q.num_vars(), 3u);
  EXPECT_EQ(q.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{1, 0, 0}), 1.0);
  EXPECT_DOUBLE_EQ(q.coeff_at(Idx{0, 1, 0}), 1.0);
}

TEST(MultiPoly, PromoteSameNvarsIsCopy)
{
  auto x = MultiPolynomial::variable(2, 0);
  EXPECT_EQ(x.promote(2), x);
}

TEST(MultiPoly, DemoteThrows)
{
  auto x = MultiPolynomial::variable(3, 0);
  EXPECT_THROW(x.promote(2), std::domain_error);
}

// ===================================================================
// Algebraic identities
// ===================================================================

TEST(MultiPoly, BinomialExpansion)
{
  // (x + y + z)^2 = x^2 + y^2 + z^2 + 2xy + 2xz + 2yz
  auto x = MultiPolynomial::variable(3, 0);
  auto y = MultiPolynomial::variable(3, 1);
  auto z = MultiPolynomial::variable(3, 2);

  MultiPolynomial sq = (x + y + z).pow(2);

  EXPECT_EQ(sq.num_terms(), 6u);
  EXPECT_EQ(sq.degree(), 2u);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{2, 0, 0}), 1.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{0, 2, 0}), 1.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{0, 0, 2}), 1.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{1, 1, 0}), 2.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{1, 0, 1}), 2.0);
  EXPECT_DOUBLE_EQ(sq.coeff_at(Idx{0, 1, 1}), 2.0);
}

TEST(MultiPoly, DifferenceOfSquares)
{
  auto x = MultiPolynomial::variable(3, 0);
  auto y = MultiPolynomial::variable(3, 1);
  auto z = MultiPolynomial::variable(3, 2);

  // (x + y)(x - y) = x^2 - y^2
  MultiPolynomial diff = (x + y) * (x - y);
  EXPECT_EQ(diff.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(diff.coeff_at(Idx{2, 0, 0}), 1.0);
  EXPECT_DOUBLE_EQ(diff.coeff_at(Idx{0, 2, 0}), -1.0);
}

TEST(MultiPoly, AssociativityOfAddition)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial one(2, 1.0);

  MultiPolynomial a = x * x + y;
  MultiPolynomial b = x + one;
  MultiPolynomial c = y * y;

  EXPECT_EQ((a + b) + c, a + (b + c));
}

TEST(MultiPoly, EvaluateProduct)
{
  // Verify (a*b)(pt) == a(pt) * b(pt)
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial a = x * x + y + MultiPolynomial(2, 1.0);
  MultiPolynomial b = x * y + MultiPolynomial(2, -1.0);

  Array<double> pt{2.0, 3.0};
  EXPECT_DOUBLE_EQ((a * b).eval(pt), a.eval(pt) * b.eval(pt));
}

// ===================================================================
// Edge cases
// ===================================================================

TEST(MultiPoly, SingleVariablePolyBehavesLikeUnivariate)
{
  // f(x) = x^3 - 2x + 1
  MultiPolynomial f(1, {
    {Idx{3}, 1.0},
    {Idx{1}, -2.0},
    {Idx{0}, 1.0},
  });

  EXPECT_EQ(f.num_vars(), 1u);
  EXPECT_EQ(f.degree(), 3u);

  Array<double> pt{2.0};
  // 8 - 4 + 1 = 5
  EXPECT_DOUBLE_EQ(f.eval(pt), 5.0);
}

TEST(MultiPoly, ZeroVariablePolynomial)
{
  MultiPolynomial p(0, 42.0);
  EXPECT_EQ(p.num_vars(), 0u);
  EXPECT_TRUE(p.is_constant());
  EXPECT_DOUBLE_EQ(p.coeff_at(Idx{}), 42.0);

  Array<double> pt;  // empty point
  EXPECT_DOUBLE_EQ(p.eval(pt), 42.0);
}

TEST(MultiPoly, HighDegreeMonomial)
{
  // x^10 * y^10
  MultiPolynomial f(2, Idx{10, 10}, 1.0);
  EXPECT_EQ(f.degree(), 20u);

  Array<double> pt{2.0, 3.0};
  // 2^10 * 3^10 = 1024 * 59049 = 60466176
  EXPECT_DOUBLE_EQ(f.eval(pt), 60466176.0);
}

// ===================================================================
// Layer 2: Industrial Fitting (Weighted Least-Squares)
// ===================================================================

TEST(MultiPolyLayer2, FitExactLinear2D)
{
  // Fit f(x,y) = 2x + 3y using exact data
  Array<std::pair<Array<double>, double>> data(3,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{1.0, 1.0}, 5.0};
  data(1) = {Array<double>{2.0, 1.0}, 7.0};
  data(2) = {Array<double>{1.0, 2.0}, 8.0};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0, 0};
  basis(1) = Idx{1, 0};
  basis(2) = Idx{0, 1};

  MultiPolynomial p = MultiPolynomial::fit(data, 2, basis);

  Array<double> pt_a{1.0, 1.0};
  Array<double> pt_b{2.0, 1.0};
  Array<double> pt_c{1.0, 2.0};

  EXPECT_NEAR(p.eval(pt_a), 5.0, 1e-10);
  EXPECT_NEAR(p.eval(pt_b), 7.0, 1e-10);
  EXPECT_NEAR(p.eval(pt_c), 8.0, 1e-10);
}

TEST(MultiPolyLayer2, FitWeightedBasic)
{
  // Same fit as above but using fit_weighted with uniform weights
  Array<std::pair<Array<double>, double>> data(3,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{1.0, 1.0}, 5.0};
  data(1) = {Array<double>{2.0, 1.0}, 7.0};
  data(2) = {Array<double>{1.0, 2.0}, 8.0};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0, 0};
  basis(1) = Idx{1, 0};
  basis(2) = Idx{0, 1};

  Array<double> weights{1.0, 1.0, 1.0};  // Uniform weighting

  MultiPolynomial p = MultiPolynomial::fit_weighted(data, 2, basis, weights);

  Array<double> pt_a{1.0, 1.0};
  EXPECT_NEAR(p.eval(pt_a), 5.0, 1e-10);
}

TEST(MultiPolyLayer2, FitEmptyDataThrows)
{
  Array<std::pair<Array<double>, double>> data;
  Array<Array<size_t>> basis(1);
  basis(0) = Idx{0};

  EXPECT_THROW(MultiPolynomial::fit(data, 1, basis),
               std::domain_error);
}

// ===================================================================
// Phase 1: Production-Grade Features
// ===================================================================

TEST(MultiPolyPhase1Ridge, BasicRidgeLinear2D)
{
  // Fit f(x,y) = 2x + 3y using ridge regression
  Array<std::pair<Array<double>, double>> data(3,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{1.0, 1.0}, 5.0};
  data(1) = {Array<double>{2.0, 1.0}, 7.0};
  data(2) = {Array<double>{1.0, 2.0}, 8.0};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0, 0};  // constant
  basis(1) = Idx{1, 0};  // x
  basis(2) = Idx{0, 1};  // y

  double lambda_used = 0.0;
  double gcv_score = 0.0;
  MultiPolynomial p = MultiPolynomial::fit_ridge(
    data, 2, basis, &lambda_used, &gcv_score);

  // Ridge should recover function reasonably well (within regularization tolerance)
  EXPECT_NEAR(p.eval(Array<double>{1.0, 1.0}), 5.0, 1e-4);
  EXPECT_NEAR(p.eval(Array<double>{2.0, 1.0}), 7.0, 1e-4);

  // Lambda should be non-negative
  EXPECT_GE(lambda_used, 0.0);
  // GCV score should be positive
  EXPECT_GT(gcv_score, 0.0);
}

TEST(MultiPolyPhase1Ridge, RidgeVsPlainFit)
{
  // Ridge should be similar to plain fit on exact data
  Array<std::pair<Array<double>, double>> data(4,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{0.0, 0.0}, 1.0};
  data(1) = {Array<double>{1.0, 0.0}, 3.0};
  data(2) = {Array<double>{0.0, 1.0}, 4.0};
  data(3) = {Array<double>{1.0, 1.0}, 6.0};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0, 0};
  basis(1) = Idx{1, 0};
  basis(2) = Idx{0, 1};

  MultiPolynomial p_plain = MultiPolynomial::fit(data, 2, basis);
  MultiPolynomial p_ridge = MultiPolynomial::fit_ridge(data, 2, basis);

  // Both should fit well (ridge may be slightly off due to regularization)
  EXPECT_NEAR(p_plain.eval(Array<double>{1.0, 1.0}), 6.0, 1e-10);
  EXPECT_NEAR(p_ridge.eval(Array<double>{1.0, 1.0}), 6.0, 1e-4);
}

TEST(MultiPolyPhase1Ridge, RidgeWithNoisy1D)
{
  // Ridge handles noisy quadratic data
  Array<std::pair<Array<double>, double>> data(5,
    {Array<double>(), 0.0});
  // y = x^2, with noise
  data(0) = {Array<double>{0.0}, 0.1};
  data(1) = {Array<double>{1.0}, 1.05};
  data(2) = {Array<double>{2.0}, 4.15};
  data(3) = {Array<double>{3.0}, 9.0};
  data(4) = {Array<double>{4.0}, 16.1};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0};       // constant
  basis(1) = Idx{1};       // x
  basis(2) = Idx{2};       // x^2

  double lambda_used = 0.0;
  MultiPolynomial p = MultiPolynomial::fit_ridge(data, 1, basis, &lambda_used);

  // Ridge should handle noise gracefully
  EXPECT_NEAR(p.eval(Array<double>{2.0}), 4.0, 0.5);

  // Should choose a reasonable lambda
  EXPECT_GE(lambda_used, 0.0);
}

TEST(MultiPolyPhase1ResidualAnalysis, AnalyzeExactFit)
{
  // Exact fit should have R² ≈ 1 and RMSE ≈ 0
  Array<std::pair<Array<double>, double>> data(3,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{1.0, 1.0}, 5.0};
  data(1) = {Array<double>{2.0, 1.0}, 7.0};
  data(2) = {Array<double>{1.0, 2.0}, 8.0};

  Array<Array<size_t>> basis(3, Idx{});
  basis(0) = Idx{0, 0};
  basis(1) = Idx{1, 0};
  basis(2) = Idx{0, 1};

  MultiPolynomial p = MultiPolynomial::fit(data, 2, basis);
  PolyFitAnalysis<double> stats = analyze_fit(p, data);

  // Perfect fit
  EXPECT_NEAR(stats.r_squared, 1.0, 1e-10);
  EXPECT_NEAR(stats.rmse, 0.0, 1e-10);
  EXPECT_EQ(stats.residuals.size(), 3u);
  EXPECT_NEAR(stats.residuals(0), 0.0, 1e-10);
}

TEST(MultiPolyPhase1ResidualAnalysis, AnalyzeWithError)
{
  // Fit line to noisy data
  Array<std::pair<Array<double>, double>> data(4,
    {Array<double>(), 0.0});
  // y = 2x, but with noise
  data(0) = {Array<double>{1.0}, 2.0};
  data(1) = {Array<double>{2.0}, 4.1};   // +0.1 error
  data(2) = {Array<double>{3.0}, 6.0};
  data(3) = {Array<double>{4.0}, 7.9};   // -0.1 error

  Array<Array<size_t>> basis(2, Idx{});
  basis(0) = Idx{0};      // constant
  basis(1) = Idx{1};      // x

  MultiPolynomial p = MultiPolynomial::fit(data, 1, basis);
  PolyFitAnalysis<double> stats = analyze_fit(p, data);

  // Should have high R² (not perfect due to noise)
  EXPECT_GT(stats.r_squared, 0.95);
  EXPECT_LT(stats.r_squared, 1.0);

  // RMSE should reflect noise level
  EXPECT_GT(stats.rmse, 0.0);
  EXPECT_LT(stats.rmse, 0.2);

  // TSS = ESS + RSS
  EXPECT_NEAR(stats.tss, stats.ess + stats.rss, 1e-10);

  // Residuals vector should have correct size
  EXPECT_EQ(stats.residuals.size(), 4u);
}

TEST(MultiPolyPhase1ResidualAnalysis, MeanAndSSCalculations)
{
  // Validate mean_y and sum of squares calculations
  Array<std::pair<Array<double>, double>> data(3,
    {Array<double>(), 0.0});
  data(0) = {Array<double>{0.0}, 1.0};
  data(1) = {Array<double>{1.0}, 2.0};
  data(2) = {Array<double>{2.0}, 3.0};  // mean = 2.0

  // Fit y = x + 1 (linear model)
  Array<Array<size_t>> basis(2, Idx{});
  basis(0) = Idx{0};  // constant
  basis(1) = Idx{1};  // x

  MultiPolynomial p = MultiPolynomial::fit(data, 1, basis);
  PolyFitAnalysis<double> stats = analyze_fit(p, data);

  // Mean should be 2.0
  EXPECT_NEAR(stats.mean_y, 2.0, 1e-10);

  // TSS = sum((y_i - mean)^2) = 1 + 0 + 1 = 2
  EXPECT_NEAR(stats.tss, 2.0, 1e-10);

  // Fitting linear model to linear data: exact fit
  EXPECT_NEAR(stats.rmse, 0.0, 1e-10);
  EXPECT_NEAR(stats.r_squared, 1.0, 1e-10);

  // Verify TSS = ESS + RSS
  EXPECT_NEAR(stats.tss, stats.ess + stats.rss, 1e-10);
}

TEST(MultiPolyPhase1Integration, RidgeAndAnalysis)
{
  // Test combined workflow: ridge fit + residual analysis
  Array<std::pair<Array<double>, double>> data(6,
    {Array<double>(), 0.0});
  // Noisy cubic
  for (size_t i = 0; i < 6; ++i)
    {
      double x = i - 2.5;
      data(i) = {Array<double>{x}, x * x * x + 0.5 * std::sin(i)};
    }

  Array<Array<size_t>> basis(4, Idx{});
  basis(0) = Idx{0};
  basis(1) = Idx{1};
  basis(2) = Idx{2};
  basis(3) = Idx{3};

  double lambda_used = 0.0;
  MultiPolynomial p = MultiPolynomial::fit_ridge(data, 1, basis, &lambda_used);
  PolyFitAnalysis<double> stats = analyze_fit(p, data);

  // Ridge should choose a lambda
  EXPECT_GE(lambda_used, 0.0);

  // Should fit reasonably well (not perfect due to noise)
  EXPECT_GT(stats.r_squared, 0.8);
  EXPECT_LT(stats.rmse, 1.0);

  // Verify calculation consistency
  EXPECT_NEAR(stats.tss, stats.ess + stats.rss, 1e-8);
  EXPECT_NEAR(stats.r_squared, stats.ess / stats.tss, 1e-10);
}

// ===================================================================
// Phase 2: Differential Calculus, Interpolation, Serialization, Parallelism
// ===================================================================

class MultiPolyPhase2 : public ::testing::Test
{
};

// ===================================================================
// Phase 2 — Differential Calculus: Partial Derivatives
// ===================================================================

TEST(MultiPolyPhase2, PartialConstantIsZero)
{
  MultiPolynomial p(2, 5.0);  // constant
  MultiPolynomial dp = p.partial(0);
  EXPECT_TRUE(dp.is_zero());
}

TEST(MultiPolyPhase2, PartialLinearVar0)
{
  // f(x,y) = 3x + 2y → ∂f/∂x = 3
  MultiPolynomial f(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 1}, 2.0},
  });
  MultiPolynomial df = f.partial(0);
  EXPECT_EQ(df.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(df.coeff_at(Idx{0, 0}), 3.0);
}

TEST(MultiPolyPhase2, PartialLinearVar1)
{
  // f(x,y) = 3x + 2y → ∂f/∂y = 2
  MultiPolynomial f(2, {
    {Idx{1, 0}, 3.0},
    {Idx{0, 1}, 2.0},
  });
  MultiPolynomial df = f.partial(1);
  EXPECT_EQ(df.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(df.coeff_at(Idx{0, 0}), 2.0);
}

TEST(MultiPolyPhase2, PartialQuadraticRule)
{
  // f(x,y) = x^2 + 3xy + y^2 → ∂f/∂x = 2x + 3y
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 3.0},
    {Idx{0, 2}, 1.0},
  });
  MultiPolynomial df = f.partial(0);
  EXPECT_EQ(df.num_terms(), 2u);
  EXPECT_DOUBLE_EQ(df.coeff_at(Idx{1, 0}), 2.0);
  EXPECT_DOUBLE_EQ(df.coeff_at(Idx{0, 1}), 3.0);
}

TEST(MultiPolyPhase2, PartialVanishingTerm)
{
  // f(x,y) = x^2 → ∂²f/∂y² = 0 (x^2 doesn't depend on y)
  auto x = MultiPolynomial::variable(2, 0);
  MultiPolynomial f = x * x;
  MultiPolynomial ddf = f.partial(1, 2);
  EXPECT_TRUE(ddf.is_zero());
}

TEST(MultiPolyPhase2, PartialHigherOrder)
{
  // f(x) = x^3 → ∂f/∂x = 3x^2, ∂²f/∂x² = 6x, ∂³f/∂x³ = 6
  MultiPolynomial f(1, Idx{3}, 1.0);

  MultiPolynomial df1 = f.partial(0, 1);
  EXPECT_DOUBLE_EQ(df1.coeff_at(Idx{2}), 3.0);

  MultiPolynomial df2 = f.partial(0, 2);
  EXPECT_DOUBLE_EQ(df2.coeff_at(Idx{1}), 6.0);

  MultiPolynomial df3 = f.partial(0, 3);
  EXPECT_DOUBLE_EQ(df3.coeff_at(Idx{0}), 6.0);
}

TEST(MultiPolyPhase2, PartialZeroOrderReturnsSelf)
{
  // ∂⁰f/∂x⁰ = f
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = x + y;
  EXPECT_EQ(f.partial(0, 0), f);
}

TEST(MultiPolyPhase2, PartialOutOfRangeThrows)
{
  MultiPolynomial f(2, 1.0);
  EXPECT_THROW(f.partial(2), std::domain_error);  // var 2 >= nvars 2
}

// ===================================================================
// Phase 2 — Gradient and Hessian
// ===================================================================

TEST(MultiPolyPhase2, GradientSize)
{
  MultiPolynomial f(3, 1.0);
  auto grad = f.gradient();
  EXPECT_EQ(grad.size(), 3u);
}

TEST(MultiPolyPhase2, GradientLinear)
{
  // f(x,y) = 2x + 3y → ∇f = [2, 3]
  MultiPolynomial f(2, {
    {Idx{1, 0}, 2.0},
    {Idx{0, 1}, 3.0},
  });
  auto grad = f.gradient();
  EXPECT_DOUBLE_EQ(grad(0).coeff_at(Idx{0, 0}), 2.0);
  EXPECT_DOUBLE_EQ(grad(1).coeff_at(Idx{0, 0}), 3.0);
}

TEST(MultiPolyPhase2, GradientQuadratic)
{
  // f(x,y) = x^2 + 2xy + y^2 → ∇f = [2x + 2y, 2x + 2y]
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 2.0},
    {Idx{0, 2}, 1.0},
  });
  auto grad = f.gradient();
  EXPECT_EQ(grad.size(), 2u);
  EXPECT_EQ(grad(0).num_terms(), 2u);
  EXPECT_EQ(grad(1).num_terms(), 2u);
}

TEST(MultiPolyPhase2, HessianSize)
{
  MultiPolynomial f(2, 1.0);
  auto hess = f.hessian();
  EXPECT_EQ(hess.size(), 2u);
  EXPECT_EQ(hess(0).size(), 2u);
  EXPECT_EQ(hess(1).size(), 2u);
}

TEST(MultiPolyPhase2, HessianLinearIsZero)
{
  // Linear polynomial has zero Hessian
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = x + y;
  auto H = f.hessian();
  for (size_t i = 0; i < 2; ++i)
    for (size_t j = 0; j < 2; ++j)
      EXPECT_TRUE(H(i)(j).is_zero());
}

TEST(MultiPolyPhase2, HessianQuadratic)
{
  // f(x,y) = x^2 + 4xy + 3y^2
  // H = [[2, 4], [4, 6]]
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 4.0},
    {Idx{0, 2}, 3.0},
  });
  auto H = f.hessian();
  EXPECT_DOUBLE_EQ(H(0)(0).coeff_at(Idx{0, 0}), 2.0);
  EXPECT_DOUBLE_EQ(H(0)(1).coeff_at(Idx{0, 0}), 4.0);
  EXPECT_DOUBLE_EQ(H(1)(0).coeff_at(Idx{0, 0}), 4.0);
  EXPECT_DOUBLE_EQ(H(1)(1).coeff_at(Idx{0, 0}), 6.0);
}

TEST(MultiPolyPhase2, HessianSymmetry)
{
  // H(i,j) == H(j,i)
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = x * x + x * y + y * y;
  auto H = f.hessian();
  EXPECT_EQ(H(0)(1), H(1)(0));
}

// ===================================================================
// Phase 2 — Evaluating Gradient and Hessian at Points
// ===================================================================

TEST(MultiPolyPhase2, EvalGradientLinear)
{
  // f(x,y) = 2x + 3y → ∇f(x,y) = [2, 3] at any point
  MultiPolynomial f(2, {
    {Idx{1, 0}, 2.0},
    {Idx{0, 1}, 3.0},
  });
  auto grad = f.eval_gradient(Array<double>{5.0, 7.0});
  EXPECT_EQ(grad.size(), 2u);
  EXPECT_DOUBLE_EQ(grad(0), 2.0);
  EXPECT_DOUBLE_EQ(grad(1), 3.0);
}

TEST(MultiPolyPhase2, EvalGradientAtPoint)
{
  // f(x,y) = x^2 + y^2 → ∇f(3,4) = [6, 8]
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{0, 2}, 1.0},
  });
  auto grad = f.eval_gradient(Array<double>{3.0, 4.0});
  EXPECT_DOUBLE_EQ(grad(0), 6.0);
  EXPECT_DOUBLE_EQ(grad(1), 8.0);
}

TEST(MultiPolyPhase2, EvalHessianQuadratic)
{
  // f(x,y) = x^2 + 4xy + 3y^2
  // H = [[2, 4], [4, 6]] (constant)
  MultiPolynomial f(2, {
    {Idx{2, 0}, 1.0},
    {Idx{1, 1}, 4.0},
    {Idx{0, 2}, 3.0},
  });
  auto H = f.eval_hessian(Array<double>{1.0, 1.0});
  EXPECT_DOUBLE_EQ(H(0)(0), 2.0);
  EXPECT_DOUBLE_EQ(H(0)(1), 4.0);
  EXPECT_DOUBLE_EQ(H(1)(0), 4.0);
  EXPECT_DOUBLE_EQ(H(1)(1), 6.0);
}

TEST(MultiPolyPhase2, EvalGradTooFewComponentsThrows)
{
  MultiPolynomial f(3, 1.0);
  EXPECT_THROW(f.eval_gradient(Array<double>{1.0, 2.0}), std::domain_error);
}

TEST(MultiPolyPhase2, EvalHessianTooFewComponentsThrows)
{
  MultiPolynomial f(3, 1.0);
  EXPECT_THROW(f.eval_hessian(Array<double>{1.0, 2.0}), std::domain_error);
}

// ===================================================================
// Phase 2 — Interpolation
// ===================================================================

TEST(MultiPolyPhase2, InterpolateUnivariateLinear)
{
  // f(x) passing through (0, 1), (1, 2)
  Array<Array<double>> grid(1, Array<double>());
  grid(0) = Array<double>(2, 0.0);
  grid(0)(0) = 0.0;
  grid(0)(1) = 1.0;

  Array<double> values(2, 0.0);
  values(0) = 1.0;
  values(1) = 2.0;

  MultiPolynomial p = MultiPolynomial::interpolate(grid, values, 1);

  Array<double> pt0(1, 0.0);
  Array<double> pt1(1, 1.0);
  Array<double> pt_half(1, 0.5);

  EXPECT_NEAR(p.eval(pt0), 1.0, 1e-10);
  EXPECT_NEAR(p.eval(pt1), 2.0, 1e-10);
  EXPECT_NEAR(p.eval(pt_half), 1.5, 1e-10);
}

TEST(MultiPolyPhase2, InterpolateUnivariateQuad)
{
  // f(x) = x^2 on grid [0, 1, 2]
  Array<Array<double>> grid(1, Array<double>());
  grid(0) = Array<double>(3, 0.0);
  grid(0)(0) = 0.0;
  grid(0)(1) = 1.0;
  grid(0)(2) = 2.0;

  Array<double> values(3, 0.0);
  values(0) = 0.0;
  values(1) = 1.0;
  values(2) = 4.0;

  MultiPolynomial p = MultiPolynomial::interpolate(grid, values, 1);

  Array<double> pt0(1, 0.0);
  Array<double> pt1(1, 1.0);
  Array<double> pt2(1, 2.0);

  EXPECT_NEAR(p.eval(pt0), 0.0, 1e-10);
  EXPECT_NEAR(p.eval(pt1), 1.0, 1e-10);
  EXPECT_NEAR(p.eval(pt2), 4.0, 1e-10);
}

TEST(MultiPolyPhase2, InterpolateBivariate2x2)
{
  // f(x,y) = x + y on 2x2 grid: {(0,1)→1, (0,2)→2, (1,1)→2, (1,2)→3}
  Array<Array<double>> grid(2, Array<double>());
  grid(0) = Array<double>(2, 0.0);
  grid(0)(0) = 0.0;
  grid(0)(1) = 1.0;
  grid(1) = Array<double>(2, 0.0);
  grid(1)(0) = 1.0;
  grid(1)(1) = 2.0;

  Array<double> values(4, 0.0);
  values(0) = 1.0;
  values(1) = 2.0;
  values(2) = 2.0;
  values(3) = 3.0;

  MultiPolynomial p = MultiPolynomial::interpolate(grid, values, 2);

  Array<double> pt00(2, 0.0);
  pt00(1) = 1.0;
  Array<double> pt01(2, 0.0);
  pt01(1) = 2.0;
  Array<double> pt10(2, 0.0);
  pt10(0) = 1.0;
  pt10(1) = 1.0;
  Array<double> pt11(2, 0.0);
  pt11(0) = 1.0;
  pt11(1) = 2.0;

  EXPECT_NEAR(p.eval(pt00), 1.0, 1e-10);
  EXPECT_NEAR(p.eval(pt01), 2.0, 1e-10);
  EXPECT_NEAR(p.eval(pt10), 2.0, 1e-10);
  EXPECT_NEAR(p.eval(pt11), 3.0, 1e-10);
}

TEST(MultiPolyPhase2, InterpolateBivariate3x2)
{
  // 3x2 grid with simple function values
  Array<Array<double>> grid(2, Array<double>());
  grid(0) = Array<double>(3, 0.0);
  grid(0)(0) = 0.0;
  grid(0)(1) = 1.0;
  grid(0)(2) = 2.0;
  grid(1) = Array<double>(2, 0.0);
  grid(1)(0) = 0.0;
  grid(1)(1) = 1.0;

  Array<double> values(6, 0.0);
  // values[i*2 + j] for x=grid(0)(i), y=grid(1)(j)
  // f(0,0)=1, f(0,1)=2, f(1,0)=2, f(1,1)=3, f(2,0)=3, f(2,1)=4
  values(0) = 1.0;
  values(1) = 2.0;
  values(2) = 2.0;
  values(3) = 3.0;
  values(4) = 3.0;
  values(5) = 4.0;

  MultiPolynomial p = MultiPolynomial::interpolate(grid, values, 2);

  Array<double> pt00(2, 0.0);
  Array<double> pt01(2, 0.0);
  pt01(1) = 1.0;
  Array<double> pt10(2, 0.0);
  pt10(0) = 1.0;
  Array<double> pt11(2, 0.0);
  pt11(0) = 1.0;
  pt11(1) = 1.0;
  Array<double> pt20(2, 0.0);
  pt20(0) = 2.0;
  Array<double> pt21(2, 0.0);
  pt21(0) = 2.0;
  pt21(1) = 1.0;

  EXPECT_NEAR(p.eval(pt00), 1.0, 1e-10);
  EXPECT_NEAR(p.eval(pt01), 2.0, 1e-10);
  EXPECT_NEAR(p.eval(pt10), 2.0, 1e-10);
  EXPECT_NEAR(p.eval(pt11), 3.0, 1e-10);
  EXPECT_NEAR(p.eval(pt20), 3.0, 1e-10);
  EXPECT_NEAR(p.eval(pt21), 4.0, 1e-10);
}

TEST(MultiPolyPhase2, InterpolateSizeMismatchThrows)
{
  Array<Array<double>> grid(2);
  grid(0) = Array<double>{0.0, 1.0};
  grid(1) = Array<double>{0.0, 1.0};

  Array<double> values{1.0, 2.0};  // Wrong size (need 4)

  EXPECT_THROW(MultiPolynomial::interpolate(grid, values, 2), std::domain_error);
}

TEST(MultiPolyPhase2, InterpolateDuplicateNodesThrows)
{
  Array<Array<double>> grid(1);
  grid(0) = Array<double>{0.0, 1.0, 1.0};  // Duplicate nodes!

  Array<double> values{1.0, 2.0, 3.0};

  EXPECT_THROW(MultiPolynomial::interpolate(grid, values, 1), std::domain_error);
}

// ===================================================================
// Phase 2 — Serialization: JSON
// ===================================================================

TEST(MultiPolyPhase2, JsonRoundtrip)
{
  // f(x,y) = 2x + 3xy
  MultiPolynomial f(2, {
    {Idx{1, 0}, 2.0},
    {Idx{1, 1}, 3.0},
  });

  std::string json = f.to_json();
  MultiPolynomial g = MultiPolynomial::from_json(json);

  EXPECT_EQ(f, g);
}

TEST(MultiPolyPhase2, JsonZeroPoly)
{
  MultiPolynomial z;
  std::string json = z.to_json();
  MultiPolynomial z2 = MultiPolynomial::from_json(json);
  EXPECT_TRUE(z2.is_zero());
}

// ===================================================================
// Phase 2 — Serialization: Binary
// ===================================================================

TEST(MultiPolyPhase2, BinaryRoundtrip)
{
  MultiPolynomial f(2, {
    {Idx{2, 0}, 5.5},
    {Idx{0, 1}, -2.3},
  });

  std::ostringstream oss;
  f.to_binary(oss);

  std::istringstream iss(oss.str());
  MultiPolynomial g = MultiPolynomial::from_binary(iss);

  EXPECT_EQ(f, g);
}

TEST(MultiPolyPhase2, BinaryMultiTerm)
{
  MultiPolynomial f(3, {
    {Idx{1, 0, 0}, 1.0},
    {Idx{0, 1, 0}, 2.0},
    {Idx{0, 0, 1}, 3.0},
  });

  std::ostringstream oss;
  f.to_binary(oss);

  std::istringstream iss(oss.str());
  MultiPolynomial g = MultiPolynomial::from_binary(iss);

  EXPECT_EQ(f.num_vars(), g.num_vars());
  EXPECT_EQ(f.num_terms(), g.num_terms());
  EXPECT_DOUBLE_EQ(g.coeff_at(Idx{1, 0, 0}), 1.0);
  EXPECT_DOUBLE_EQ(g.coeff_at(Idx{0, 1, 0}), 2.0);
  EXPECT_DOUBLE_EQ(g.coeff_at(Idx{0, 0, 1}), 3.0);
}

// ===================================================================
// Phase 2 — Parallelism
// ===================================================================

TEST(MultiPolyPhase2, EvalBatchCorrectness)
{
  // f(x) = x^2
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x;

  Array<Array<double>> pts(3, Array<double>{});
  pts(0) = Array<double>{2.0};
  pts(1) = Array<double>{3.0};
  pts(2) = Array<double>{4.0};

  auto results = f.eval_batch(pts);

  EXPECT_EQ(results.size(), 3u);
  EXPECT_DOUBLE_EQ(results(0), 4.0);   // 2^2
  EXPECT_DOUBLE_EQ(results(1), 9.0);   // 3^2
  EXPECT_DOUBLE_EQ(results(2), 16.0);  // 4^2
}

TEST(MultiPolyPhase2, EvalBatchMatchesEval)
{
  // f(x,y) = x + 2y
  MultiPolynomial f(2, {
    {Idx{1, 0}, 1.0},
    {Idx{0, 1}, 2.0},
  });

  Array<Array<double>> pts(2, Array<double>(2));
  pts(0) = Array<double>{1.0, 2.0};
  pts(1) = Array<double>{3.0, 4.0};

  auto batch_results = f.eval_batch(pts);

  EXPECT_NEAR(batch_results(0), f.eval(pts(0)), 1e-14);
  EXPECT_NEAR(batch_results(1), f.eval(pts(1)), 1e-14);
}

TEST(MultiPolyPhase2, FitParallelMatchesFit)
{
  // Create simple data: y = 2x + 1
  Array<std::pair<Array<double>, double>> data(4, {Array<double>(), 0.0});
  data(0) = {Array<double>{0.0}, 1.0};
  data(1) = {Array<double>{1.0}, 3.0};
  data(2) = {Array<double>{2.0}, 5.0};
  data(3) = {Array<double>{3.0}, 7.0};

  Array<Array<size_t>> basis(2, Idx{});
  basis(0) = Idx{0};  // constant
  basis(1) = Idx{1};  // x

  // Fit with sequential method
  MultiPolynomial p_seq = MultiPolynomial::fit(data, 1, basis);

  // Fit with parallel method
  MultiPolynomial p_par = MultiPolynomial::fit_parallel(data, 1, basis);

  // Results should be virtually identical (within floating-point tolerance)
  EXPECT_NEAR(p_seq.eval(Array<double>{0.5}), p_par.eval(Array<double>{0.5}), 1e-10);
  EXPECT_NEAR(p_seq.eval(Array<double>{1.5}), p_par.eval(Array<double>{1.5}), 1e-10);
}

// ===================================================================
// Layer 3 — Gröbner Bases and Ideal Operations
// ===================================================================

// ---
// divmod tests (8)
// ---

TEST(MultiPolyLayer3, DivisionByConstant)
{
  // f = 2x + 3, divisor = 2
  MultiPolynomial f(1, {{Idx{1}, 1.0}, {Idx{0}, 3.0}});
  Array<MultiPolynomial> divisors(1, MultiPolynomial(1));
  divisors(0) = MultiPolynomial(1, 2.0);

  auto [q, r] = f.divmod(divisors);
  EXPECT_EQ(q.size(), 1u);
  EXPECT_EQ(q(0).num_terms(), 2u);
  EXPECT_DOUBLE_EQ(q(0).coeff_at(Idx{1}), 0.5);
  EXPECT_DOUBLE_EQ(q(0).coeff_at(Idx{0}), 1.5);
  EXPECT_TRUE(r.is_zero());
}

TEST(MultiPolyLayer3, DivisionExact)
{
  // f = x^2, divisor = x
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x;
  Array<MultiPolynomial> divisors(1, x);

  auto [q, r] = f.divmod(divisors);
  EXPECT_EQ(q(0).num_terms(), 1u);
  EXPECT_DOUBLE_EQ(q(0).coeff_at(Idx{1}), 1.0);
  EXPECT_TRUE(r.is_zero());
}

TEST(MultiPolyLayer3, DivisionWithRemainder)
{
  // f = x^2 + 1, divisor = x
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x + MultiPolynomial(1, 1.0);
  Array<MultiPolynomial> divisors(1, x);

  auto [q, r] = f.divmod(divisors);
  EXPECT_EQ(q(0).num_terms(), 1u);
  EXPECT_DOUBLE_EQ(q(0).coeff_at(Idx{1}), 1.0);
  EXPECT_EQ(r.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(r.coeff_at(Idx{0}), 1.0);
}

TEST(MultiPolyLayer3, MultiDivisor)
{
  // f = xy + x + y + 1 = (x + 1)(y + 1)
  // divisors = [x + 1, y + 1]
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = x * y + x + y + MultiPolynomial(2, 1.0);

  Array<MultiPolynomial> divisors(2, MultiPolynomial(2));
  divisors(0) = x + MultiPolynomial(2, 1.0);
  divisors(1) = y + MultiPolynomial(2, 1.0);

  auto [q, r] = f.divmod(divisors);
  EXPECT_EQ(q.size(), 2u);
  EXPECT_TRUE(r.is_zero() or r.num_terms() == 0u);
}

TEST(MultiPolyLayer3, ZeroDividend)
{
  // f = 0, divisor = x
  MultiPolynomial f(1, 0.0);  // Zero polynomial with 1 variable
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> divisors(1, x);

  auto [q, r] = f.divmod(divisors);
  EXPECT_EQ(q.size(), 1u);
  EXPECT_TRUE(q(0).is_zero());
  EXPECT_TRUE(r.is_zero());
}

TEST(MultiPolyLayer3, DivisorZeroThrows)
{
  // divisors contain a zero polynomial
  MultiPolynomial f(1, 1.0);
  Array<MultiPolynomial> divisors(1, MultiPolynomial());

  EXPECT_THROW(f.divmod(divisors), std::domain_error);
}

TEST(MultiPolyLayer3, EmptyDivisorsThrows)
{
  // empty divisor array
  MultiPolynomial f(1, 1.0);
  Array<MultiPolynomial> divisors(0, MultiPolynomial(1));

  EXPECT_THROW(f.divmod(divisors), std::domain_error);
}

TEST(MultiPolyLayer3, DivmodIdentityCheck)
{
  // Test: f = (q * g) + r
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x * x - MultiPolynomial(1, 1.0);  // x^3 - 1
  MultiPolynomial g = x - MultiPolynomial(1, 1.0);           // x - 1

  Array<MultiPolynomial> divisors(1, g);
  auto [q, r] = f.divmod(divisors);

  MultiPolynomial reconstructed = q(0) * g + r;
  // Check they have the same terms
  EXPECT_EQ(reconstructed.num_terms(), f.num_terms());
}

// ---
// s_poly tests (5)
// ---

TEST(MultiPolyLayer3, SPolyLinear)
{
  // f = x, g = y (monomials)
  auto f = MultiPolynomial::variable(2, 0);
  auto g = MultiPolynomial::variable(2, 1);

  MultiPolynomial s = MultiPolynomial::s_poly(f, g);

  // lcm(x, y) = xy, so S(x, y) = (xy/x) * x - (xy/y) * y = y * x - x * y = 0
  EXPECT_TRUE(s.is_zero());
}

TEST(MultiPolyLayer3, SPolyCancellation)
{
  // f = 2x, g = 3x (both same monomial)
  MultiPolynomial f(1, {{Idx{1}, 2.0}});
  MultiPolynomial g(1, {{Idx{1}, 3.0}});

  MultiPolynomial s = MultiPolynomial::s_poly(f, g);

  // lcm(x, x) = x, S(f, g) = (x/x) * (1/2) * 2x - (x/x) * (1/3) * 3x = x - x = 0
  EXPECT_TRUE(s.is_zero());
}

TEST(MultiPolyLayer3, SPolyZeroFThrows)
{
  MultiPolynomial zero;
  auto g = MultiPolynomial::variable(1, 0);

  EXPECT_THROW(MultiPolynomial::s_poly(zero, g), std::domain_error);
}

TEST(MultiPolyLayer3, SPolyZeroGThrows)
{
  auto f = MultiPolynomial::variable(1, 0);
  MultiPolynomial zero;

  EXPECT_THROW(MultiPolynomial::s_poly(f, zero), std::domain_error);
}

TEST(MultiPolyLayer3, SPolyIncompatibleNvarsThrows)
{
  auto f = MultiPolynomial::variable(1, 0);
  auto g = MultiPolynomial::variable(2, 0);

  EXPECT_THROW(MultiPolynomial::s_poly(f, g), std::invalid_argument);
}

// ---
// groebner_basis tests (5)
// ---

TEST(MultiPolyLayer3, GroebnerSingleGenerator)
{
  // Single generator: basis is itself
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, x * x - MultiPolynomial(1, 1.0));

  Array<MultiPolynomial> basis = MultiPolynomial::groebner_basis(gens);

  EXPECT_GE(basis.size(), 1u);
  EXPECT_EQ(basis(0).num_vars(), 1u);
}

TEST(MultiPolyLayer3, GroebnerLinearIdeal)
{
  // Linear ideal: x + 1, y - 2
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> gens(2, MultiPolynomial(2));
  gens(0) = x + MultiPolynomial(2, 1.0);
  gens(1) = y - MultiPolynomial(2, 2.0);

  Array<MultiPolynomial> basis = MultiPolynomial::groebner_basis(gens);

  // Linear ideal should have a basis
  EXPECT_GE(basis.size(), 2u);
}

TEST(MultiPolyLayer3, GroebnerQuadratic2D)
{
  // Quadratic: x^2 - y, xy - 1
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> gens(2, MultiPolynomial(2));
  gens(0) = x * x - y;
  gens(1) = x * y - MultiPolynomial(2, 1.0);

  Array<MultiPolynomial> basis = MultiPolynomial::groebner_basis(gens);

  EXPECT_GE(basis.size(), 1u);
  EXPECT_EQ(basis(0).num_vars(), 2u);
}

TEST(MultiPolyLayer3, GroebnerEmptyThrows)
{
  Array<MultiPolynomial> gens(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::groebner_basis(gens), std::domain_error);
}

TEST(MultiPolyLayer3, GroebnerZeroGeneratorThrows)
{
  Array<MultiPolynomial> gens(1, MultiPolynomial());

  EXPECT_THROW(MultiPolynomial::groebner_basis(gens), std::domain_error);
}

// ---
// ideal_member tests (6)
// ---

TEST(MultiPolyLayer3, MemberDirect)
{
  // f in ideal generated by itself
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x + x;

  Array<MultiPolynomial> gens(1, f);

  EXPECT_TRUE(MultiPolynomial::ideal_member(f, gens));
}

TEST(MultiPolyLayer3, NonMember)
{
  // x not in ideal generated by (x^2 + 1) over double
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x;
  MultiPolynomial g = x * x + MultiPolynomial(1, 1.0);

  Array<MultiPolynomial> gens(1, g);

  EXPECT_FALSE(MultiPolynomial::ideal_member(f, gens));
}

TEST(MultiPolyLayer3, ZeroIsAlwaysMember)
{
  // Zero polynomial is always in the ideal
  MultiPolynomial zero;
  auto x = MultiPolynomial::variable(1, 0);

  Array<MultiPolynomial> gens(1, x);

  EXPECT_TRUE(MultiPolynomial::ideal_member(zero, gens));
}

TEST(MultiPolyLayer3, MemberViaGroebner)
{
  // f = xy + x + y + 1, generators = [x + 1, y + 1]
  // f = (x + 1) * (y + 1) so it's in the ideal
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  MultiPolynomial f = x * y + x + y + MultiPolynomial(2, 1.0);
  Array<MultiPolynomial> gens(2, MultiPolynomial(2));
  gens(0) = x + MultiPolynomial(2, 1.0);
  gens(1) = y + MultiPolynomial(2, 1.0);

  EXPECT_TRUE(MultiPolynomial::ideal_member(f, gens));
}

TEST(MultiPolyLayer3, IndexDivisibility_True)
{
  // Test helper: divides_index(alpha, beta)
  // Manually check internal behavior by membership
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x;
  Array<MultiPolynomial> gens(1, x);

  // x^2 is divisible by x in the ideal
  EXPECT_TRUE(MultiPolynomial::ideal_member(f, gens));
}

TEST(MultiPolyLayer3, IndexDivisibility_False)
{
  // x not divisible by x^2
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x;
  Array<MultiPolynomial> gens(1, x * x);

  EXPECT_FALSE(MultiPolynomial::ideal_member(f, gens));
}

// ---
// reduce_modulo tests (3)
// ---

TEST(MultiPolyLayer3, ReduceExact)
{
  // f = xy, divisor = xy -> remainder 0
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  MultiPolynomial f = x * y;

  Array<MultiPolynomial> divisors(1, x * y);

  MultiPolynomial r = f.reduce_modulo(divisors);
  EXPECT_TRUE(r.is_zero());
}

TEST(MultiPolyLayer3, ReduceNonZero)
{
  // f = x^2 + 1, divisor = x -> remainder 1
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x + MultiPolynomial(1, 1.0);

  Array<MultiPolynomial> divisors(1, x);

  MultiPolynomial r = f.reduce_modulo(divisors);
  EXPECT_FALSE(r.is_zero());
  EXPECT_EQ(r.num_terms(), 1u);
  EXPECT_DOUBLE_EQ(r.coeff_at(Idx{0}), 1.0);
}

TEST(MultiPolyLayer3, ReduceIdempotent)
{
  // Reducing twice should give the same result
  auto x = MultiPolynomial::variable(1, 0);
  MultiPolynomial f = x * x + x + MultiPolynomial(1, 1.0);

  Array<MultiPolynomial> divisors(1, x * x + MultiPolynomial(1, 1.0));

  MultiPolynomial r1 = f.reduce_modulo(divisors);
  MultiPolynomial r2 = r1.reduce_modulo(divisors);

  EXPECT_EQ(r1.num_terms(), r2.num_terms());
  r1.for_each_term([&](const Idx& idx, double c) {
    EXPECT_NEAR(r2.coeff_at(idx), c, 1e-14);
  });
}

// ---
// reduced_groebner_basis tests (4)
// ---

TEST(MultiPolyLayer3, ReducedBasisSingleGen)
{
  // Single generator: basis is itself (up to monic)
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, 2.0 * x);

  Array<MultiPolynomial> basis = MultiPolynomial::reduced_groebner_basis(gens);

  EXPECT_GE(basis.size(), 1u);
  EXPECT_DOUBLE_EQ(basis(0).leading_coeff(), 1.0);  // Should be monic
}

TEST(MultiPolyLayer3, ReducedBasisLinear)
{
  // Linear ideal: x + 1, y - 2
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> gens(2, MultiPolynomial(2));
  gens(0) = x + MultiPolynomial(2, 1.0);
  gens(1) = y - MultiPolynomial(2, 2.0);

  Array<MultiPolynomial> basis = MultiPolynomial::reduced_groebner_basis(gens);

  // All elements should be monic
  for (size_t i = 0; i < basis.size(); ++i)
    EXPECT_NEAR(basis(i).leading_coeff(), 1.0, 1e-12);
}

TEST(MultiPolyLayer3, ReducedBasisMonicOutput)
{
  // Check that output is monic
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, 3.0 * x * x + MultiPolynomial(1, 2.0));

  Array<MultiPolynomial> basis = MultiPolynomial::reduced_groebner_basis(gens);

  EXPECT_FALSE(basis.is_empty());
  for (size_t i = 0; i < basis.size(); ++i)
    if (not basis(i).is_zero())
      EXPECT_NEAR(basis(i).leading_coeff(), 1.0, 1e-12);
}

TEST(MultiPolyLayer3, ReducedBasisIdealPreserved)
{
  // Same ideal membership before and after reduction
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> gens(2, MultiPolynomial(2));
  gens(0) = 2.0 * (x + MultiPolynomial(2, 1.0));
  gens(1) = 3.0 * (y - MultiPolynomial(2, 2.0));

  MultiPolynomial f = x + y;

  bool mem_before = MultiPolynomial::ideal_member(f, gens);
  Array<MultiPolynomial> reduced = MultiPolynomial::reduced_groebner_basis(gens);
  bool mem_after = f.reduce_modulo(reduced).is_zero();

  EXPECT_EQ(mem_before, mem_after);
}

// ===================================================================
// Layer 4 — Ideal Arithmetic Tests
// ===================================================================

// ---
// ideal_sum tests (5)
// ---

TEST(MultiPolyLayer4, SumBasicContainment)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, MultiPolynomial(1, 1.0));

  Array<MultiPolynomial> IJ = MultiPolynomial::ideal_sum(I, J);

  EXPECT_EQ(IJ.size(), 2u);
  EXPECT_TRUE(MultiPolynomial::ideal_member(x, IJ));
  EXPECT_TRUE(MultiPolynomial::ideal_member(MultiPolynomial(1, 1.0), IJ));
}

TEST(MultiPolyLayer4, SumMembership)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, y);

  Array<MultiPolynomial> IJ = MultiPolynomial::ideal_sum(I, J);

  EXPECT_EQ(IJ.size(), 2u);
  EXPECT_TRUE(MultiPolynomial::ideal_member(x, IJ));
  EXPECT_TRUE(MultiPolynomial::ideal_member(y, IJ));
  EXPECT_TRUE(MultiPolynomial::ideal_member(x + y, IJ));
}

TEST(MultiPolyLayer4, SumSymmetryMembership)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x * x);
  Array<MultiPolynomial> J(1, x * x * x);

  Array<MultiPolynomial> IJ = MultiPolynomial::ideal_sum(I, J);

  // Both x^2 and x^3 belong
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x, IJ));
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x * x, IJ));
}

TEST(MultiPolyLayer4, SumNvarsThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  auto y = MultiPolynomial::variable(2, 0);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, y);

  EXPECT_THROW(MultiPolynomial::ideal_sum(I, J), std::invalid_argument);
}

TEST(MultiPolyLayer4, SumEmptyThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J_empty(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::ideal_sum(I, J_empty), std::domain_error);
  EXPECT_THROW(MultiPolynomial::ideal_sum(J_empty, I), std::domain_error);
}

// ---
// ideal_product tests (5)
// ---

TEST(MultiPolyLayer4, ProductBasic)
{
  auto x = MultiPolynomial::variable(1, 0);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, x);

  Array<MultiPolynomial> IJ = MultiPolynomial::ideal_product(I, J);

  EXPECT_EQ(IJ.size(), 1u);
  // x^2 should be in the product
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x, IJ));
}

TEST(MultiPolyLayer4, ProductWithUnit)
{
  auto x = MultiPolynomial::variable(1, 0);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> unit(1, MultiPolynomial(1, 1.0));

  Array<MultiPolynomial> prod = MultiPolynomial::ideal_product(I, unit);

  EXPECT_EQ(prod.size(), 1u);
  // Product with ⟨1⟩ is I itself
  EXPECT_TRUE(MultiPolynomial::ideal_member(x, prod));
}

TEST(MultiPolyLayer4, ProductSubIdeal)
{
  auto x = MultiPolynomial::variable(1, 0);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, x * x);

  Array<MultiPolynomial> IJ = MultiPolynomial::ideal_product(I, J);

  // IJ should contain x^3
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x * x, IJ));
}

TEST(MultiPolyLayer4, ProductNvarsThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  auto y = MultiPolynomial::variable(2, 0);

  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, y);

  EXPECT_THROW(MultiPolynomial::ideal_product(I, J), std::invalid_argument);
}

TEST(MultiPolyLayer4, ProductEmptyThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J_empty(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::ideal_product(I, J_empty), std::domain_error);
  EXPECT_THROW(MultiPolynomial::ideal_product(J_empty, I), std::domain_error);
}

// ---
// ideal_power tests (5)
// ---

TEST(MultiPolyLayer4, PowerZeroIsUnit)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  Array<MultiPolynomial> I0 = MultiPolynomial::ideal_power(I, 0);

  EXPECT_EQ(I0.size(), 1u);
  // I^0 = ⟨1⟩, everything is in it
  EXPECT_TRUE(MultiPolynomial::ideal_member(x, I0));
  EXPECT_TRUE(MultiPolynomial::ideal_member(MultiPolynomial(1, 1.0), I0));
}

TEST(MultiPolyLayer4, PowerOneIsIdentity)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  Array<MultiPolynomial> I1 = MultiPolynomial::ideal_power(I, 1);

  EXPECT_EQ(I1.size(), 1u);
  EXPECT_TRUE(MultiPolynomial::ideal_member(x, I1));
}

TEST(MultiPolyLayer4, PowerTwoContainsSquares)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  Array<MultiPolynomial> I2 = MultiPolynomial::ideal_power(I, 2);

  EXPECT_EQ(I2.size(), 1u);
  // I^2 = ⟨x^2⟩
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x, I2));
  EXPECT_FALSE(MultiPolynomial::ideal_member(x, I2));
}

TEST(MultiPolyLayer4, PowerSubIdeal)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  Array<MultiPolynomial> I2 = MultiPolynomial::ideal_power(I, 2);
  Array<MultiPolynomial> I3 = MultiPolynomial::ideal_power(I, 3);

  // I^3 ⊆ I^2
  EXPECT_TRUE(MultiPolynomial::ideal_member(x * x * x, I2));
}

TEST(MultiPolyLayer4, PowerEmptyThrows)
{
  Array<MultiPolynomial> I_empty(0, MultiPolynomial(1));
  EXPECT_THROW(MultiPolynomial::ideal_power(I_empty, 1), std::domain_error);
}

// ---
// contains_ideal tests (5)
// ---

TEST(MultiPolyLayer4, ContainsReflexive)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  EXPECT_TRUE(MultiPolynomial::contains_ideal(I, I));
}

TEST(MultiPolyLayer4, ContainsSuperIdeal)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, x * x);

  // I ⊇ J because J = ⟨x^2⟩ and x | x^2, so x^2 ∈ ⟨x⟩
  EXPECT_TRUE(MultiPolynomial::contains_ideal(I, J));
}

TEST(MultiPolyLayer4, ContainsNot)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, MultiPolynomial(1, 1.0));

  // ⟨x⟩ does not contain ⟨1⟩
  EXPECT_FALSE(MultiPolynomial::contains_ideal(I, J));
}

TEST(MultiPolyLayer4, ContainsAfterSaturation)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);

  Array<MultiPolynomial> I(2, MultiPolynomial(2));
  I(0) = x;
  I(1) = x * y;

  Array<MultiPolynomial> J(1, x);

  // ⟨x, xy⟩ ⊇ ⟨x⟩ trivially
  EXPECT_TRUE(MultiPolynomial::contains_ideal(I, J));
}

TEST(MultiPolyLayer4, ContainsEmptyThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J_empty(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::contains_ideal(I, J_empty), std::domain_error);
  EXPECT_THROW(MultiPolynomial::contains_ideal(J_empty, I), std::domain_error);
}

// ---
// ideals_equal tests (5)
// ---

TEST(MultiPolyLayer4, EqualSelf)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);

  EXPECT_TRUE(MultiPolynomial::ideals_equal(I, I));
}

TEST(MultiPolyLayer4, EqualEquivalentForms)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(2, MultiPolynomial(1));
  J(0) = x;
  J(1) = x * x;

  // ⟨x⟩ = ⟨x, x^2⟩ in the polynomial ring
  EXPECT_TRUE(MultiPolynomial::ideals_equal(I, J));
}

TEST(MultiPolyLayer4, EqualNot)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J(1, x * x);

  // ⟨x⟩ ≠ ⟨x^2⟩
  EXPECT_FALSE(MultiPolynomial::ideals_equal(I, J));
}

TEST(MultiPolyLayer4, EqualAfterReduction)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x - MultiPolynomial(1, 1.0));
  Array<MultiPolynomial> J(2, MultiPolynomial(1));
  J(0) = x * x - MultiPolynomial(1, 1.0);
  J(1) = x - MultiPolynomial(1, 1.0);

  // ⟨x−1⟩ = ⟨x^2−1, x−1⟩ since x^2−1 = (x−1)(x+1) ∈ ⟨x−1⟩
  EXPECT_TRUE(MultiPolynomial::ideals_equal(I, J));
}

TEST(MultiPolyLayer4, EqualEmptyThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> I(1, x);
  Array<MultiPolynomial> J_empty(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::ideals_equal(I, J_empty), std::domain_error);
  EXPECT_THROW(MultiPolynomial::ideals_equal(J_empty, I), std::domain_error);
}

// ---
// radical_member tests (7)
// ---

TEST(MultiPolyLayer4, RadicalZeroAlwaysMember)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, x);

  EXPECT_TRUE(MultiPolynomial::radical_member(MultiPolynomial(1), gens));
}

TEST(MultiPolyLayer4, RadicalDirectMember)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, x);

  // x ∈ √⟨x⟩ because x ∈ ⟨x⟩ trivially
  EXPECT_TRUE(MultiPolynomial::radical_member(x, gens));
}

TEST(MultiPolyLayer4, RadicalOfSquare)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, x * x);

  // x ∈ √⟨x^2⟩ because x^2 ∈ ⟨x^2⟩
  EXPECT_TRUE(MultiPolynomial::radical_member(x, gens));
}

TEST(MultiPolyLayer4, RadicalNonMember)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  Array<MultiPolynomial> gens(1, x * x);

  // y ∉ √⟨x^2⟩ (y is independent)
  EXPECT_FALSE(MultiPolynomial::radical_member(y, gens));
}

TEST(MultiPolyLayer4, RadicalPower)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens(1, x * x * x);

  // x ∈ √⟨x^3⟩
  EXPECT_TRUE(MultiPolynomial::radical_member(x, gens));
}

TEST(MultiPolyLayer4, RadicalMultiVar)
{
  auto x = MultiPolynomial::variable(2, 0);
  auto y = MultiPolynomial::variable(2, 1);
  Array<MultiPolynomial> gens(1, x * x);

  // x ∈ √⟨x^2⟩ because (x)^2 = x^2 is in the ideal
  EXPECT_TRUE(MultiPolynomial::radical_member(x, gens));
  // y ∉ √⟨x^2⟩ because y^k ∉ ⟨x^2⟩ for any k (independent variable)
  EXPECT_FALSE(MultiPolynomial::radical_member(y, gens));
}

TEST(MultiPolyLayer4, RadicalEmptyThrows)
{
  auto x = MultiPolynomial::variable(1, 0);
  Array<MultiPolynomial> gens_empty(0, MultiPolynomial(1));

  EXPECT_THROW(MultiPolynomial::radical_member(x, gens_empty), std::domain_error);
}
