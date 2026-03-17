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


/**
 * @file polynomial_example.cc
 * @brief Polynomial arithmetic in Aleph-w: construction, algebra,
 *        calculus, interpolation, and applications.
 *
 * ## Overview
 *
 * The `Gen_Polynomial<Coefficient>` class in `tpl_polynomial.H` provides
 * industrial-strength univariate polynomial arithmetic using a sparse
 * representation backed by `DynMapTree<size_t, Coefficient>`.  Only
 * non-zero coefficients are stored, making it efficient on high-degree
 * sparse polynomials such as x^1000 + 1.
 *
 * This example walks through five illustrative scenarios:
 *
 * 1. **Basic arithmetic** — constructing polynomials and performing
 *    addition, subtraction, multiplication, and division.
 * 2. **Calculus** — symbolic differentiation, integration, and the
 *    Fundamental Theorem verification.
 * 3. **Root finding and interpolation** — building polynomials from
 *    known roots and from data points via Lagrange interpolation.
 * 4. **Signal processing** — representing a transfer function as a
 *    ratio of polynomials and evaluating frequency response.
 * 5. **Sparse polynomials** — demonstrating efficiency on polynomials
 *    with very high degree but few non-zero terms.
 *
 * ## Compilation
 *
 * ```bash
 * cmake -S . -B build -G Ninja -DBUILD_EXAMPLES=ON
 * cmake --build build --target polynomial_example
 * ./build/Examples/polynomial_example
 * ```
 *
 * @ingroup Algebra
 * @author Leandro Rabindranath León
 */

# include <iostream>
# include <iomanip>
# include <cmath>

# include <tpl_polynomial.H>

using namespace std;
using namespace Aleph;


// ─────────────────────────────────────────────────────────────────────
// Helper: print a section header
// ─────────────────────────────────────────────────────────────────────
static void section(const string & title)
{
  cout << "\n" << string(65, '=') << "\n"
       << "  " << title << "\n"
       << string(65, '=') << "\n\n";
}


// =====================================================================
//  1. Basic Polynomial Arithmetic
// =====================================================================

/**
 * Demonstrates polynomial construction, printing, and the four
 * basic arithmetic operations.
 */
static void basic_arithmetic()
{
  section("1. Basic Polynomial Arithmetic");

  // Dense construction: coefficients in ascending exponent order
  // p(x) = 2 + 3x + x^2
  Polynomial p({2, 3, 1});
  cout << "  p(x) = " << p << "\n";

  // q(x) = 1 - x + 4x^3
  Polynomial q({1, -1, 0, 4});
  cout << "  q(x) = " << q << "\n\n";

  // Arithmetic
  cout << "  p + q  = " << (p + q) << "\n";
  cout << "  p - q  = " << (p - q) << "\n";
  cout << "  p * q  = " << (p * q) << "\n";

  // Scalar operations
  cout << "  3 * p  = " << (p * 3.0) << "\n";
  cout << "  p / 2  = " << (p / 2.0) << "\n\n";

  // Polynomial division: (x^3 - 1) / (x - 1) = x^2 + x + 1
  Polynomial num({-1, 0, 0, 1});   // x^3 - 1
  Polynomial den({-1, 1});          // x - 1
  auto [quotient, remainder] = num.divmod(den);
  cout << "  (" << num << ") / (" << den << ")\n";
  cout << "    quotient  = " << quotient << "\n";
  cout << "    remainder = " << remainder << "\n\n";

  // Verification: num == quotient * den + remainder
  Polynomial reconstructed = quotient * den + remainder;
  cout << "  Verify: q*d + r = " << reconstructed;
  cout << (reconstructed == num ? "  [OK]" : "  [FAIL]") << "\n";
}


// =====================================================================
//  2. Calculus: Differentiation and Integration
// =====================================================================

/**
 * Symbolic differentiation and integration, verifying the Fundamental
 * Theorem of Calculus: the derivative of the integral recovers the
 * original polynomial.
 */
static void calculus_example()
{
  section("2. Symbolic Calculus");

  // f(x) = 3x^4 - 2x^2 + x - 7
  Polynomial f({-7, 1, -2, 0, 3});
  cout << "  f(x)   = " << f << "\n\n";

  // First derivative: f'(x) = 12x^3 - 4x + 1
  Polynomial fp = f.derivative();
  cout << "  f'(x)  = " << fp << "\n";

  // Second derivative: f''(x) = 36x^2 - 4
  Polynomial fpp = f.nth_derivative(2);
  cout << "  f''(x) = " << fpp << "\n\n";

  // Indefinite integral (constant of integration = 0)
  Polynomial F = f.integral();
  cout << "  Integral of f(x) = " << F << "\n";

  // Fundamental theorem: d/dx[integral(f)] == f
  Polynomial roundtrip = F.derivative();
  cout << "  d/dx[integral(f)] = " << roundtrip << "\n";
  cout << "  Matches original? " << (roundtrip == f ? "Yes" : "No") << "\n\n";

  // Evaluate at a point: f(2)
  double x = 2.0;
  cout << "  f(" << x << ") = " << f(x) << "\n";
  cout << "  f'(" << x << ") = " << fp(x) << "\n";
}


// =====================================================================
//  3. Root Construction and Lagrange Interpolation
// =====================================================================

/**
 * Two ways to build polynomials from known data:
 *   (a) from_roots: given roots r1, r2, ..., builds (x - r1)(x - r2)...
 *   (b) interpolate: Lagrange interpolation through given (x, y) points
 */
static void roots_and_interpolation()
{
  section("3. Roots and Interpolation");

  // --- (a) Build from roots ---
  // Roots at x = 1, 2, 3 => (x-1)(x-2)(x-3) = x^3 - 6x^2 + 11x - 6
  DynList<double> roots;
  roots.append(1.0);
  roots.append(2.0);
  roots.append(3.0);

  Polynomial p = Polynomial::from_roots(roots);
  cout << "  Polynomial with roots {1, 2, 3}:\n";
  cout << "    p(x) = " << p << "\n\n";

  // Verify roots
  cout << "  Evaluation at roots:\n";
  roots.for_each([&p](double r)
  {
    cout << "    p(" << r << ") = " << p(r) << "\n";
  });

  // --- (b) Lagrange interpolation ---
  // Fit a quadratic through (0, 1), (1, 0), (2, 1)
  // Expected: x^2 - x + 1... let's see
  cout << "\n  Lagrange interpolation through (0,1), (1,0), (2,1):\n";

  DynList<std::pair<double, double>> points;
  points.append(std::make_pair(0.0, 1.0));
  points.append(std::make_pair(1.0, 0.0));
  points.append(std::make_pair(2.0, 1.0));

  Polynomial interp = Polynomial::interpolate(points);
  cout << "    L(x) = " << interp << "\n\n";

  // Verify interpolation
  cout << "  Verification:\n";
  points.for_each([&interp](const std::pair<double, double> & pt)
  {
    cout << "    L(" << pt.first << ") = " << interp(pt.first)
         << "  (expected " << pt.second << ")\n";
  });
}


// =====================================================================
//  4. Transfer Function (Signal Processing)
// =====================================================================

/**
 * A simple low-pass filter transfer function H(s) = N(s) / D(s).
 * We evaluate the numerator and denominator polynomials separately
 * at several frequency points to compute |H(jω)|.
 */
static void transfer_function()
{
  section("4. Transfer Function (Signal Processing)");

  // H(s) = 1 / (s^2 + 1.414s + 1)  — 2nd order Butterworth
  Polynomial num(1.0);                  // N(s) = 1
  Polynomial den({1, 1.414, 1});        // D(s) = 1 + 1.414s + s^2
  cout << "  H(s) = (" << num << ") / (" << den << ")\n\n";

  cout << "  Frequency response |H(jw)|^2 at sample points:\n";
  cout << "  " << setw(10) << "omega" << setw(15) << "|N(jw)|^2"
       << setw(15) << "|D(jw)|^2" << setw(15) << "|H(jw)|^2" << "\n";
  cout << "  " << string(55, '-') << "\n";

  // For a polynomial P(s) evaluated at s = jw:
  // P(jw) = sum(c_k * (jw)^k).  For magnitude, we compute real/imag parts.
  // Simpler approach: evaluate |P(jw)|^2 by noting:
  //   P(jw) = P_even(w^2) + jw * P_odd(w^2)
  //   |P(jw)|^2 = P_even(w^2)^2 + w^2 * P_odd(w^2)^2

  auto mag_squared = [](const Polynomial & p, double w) -> double
  {
    // Split into even and odd parts
    double even_sum = 0, odd_sum = 0;
    double w2 = w * w;
    p.for_each_term([&](size_t exp, const double & c)
    {
      double w_pow = pow(w2, exp / 2.0);
      // (jw)^k = j^k * w^k.  j^0=1, j^1=j, j^2=-1, j^3=-j
      switch (exp % 4)
        {
        case 0: even_sum += c * w_pow; break;
        case 1: odd_sum  += c * w_pow; break;
        case 2: even_sum -= c * w_pow; break;
        case 3: odd_sum  -= c * w_pow; break;
        }
    });
    return even_sum * even_sum + odd_sum * odd_sum;
  };

  DynList<double> freqs;
  freqs.append(0.0);
  freqs.append(0.5);
  freqs.append(1.0);
  freqs.append(2.0);
  freqs.append(5.0);
  freqs.append(10.0);

  freqs.for_each([&](double w)
  {
    double n2 = mag_squared(num, w);
    double d2 = mag_squared(den, w);
    double h2 = n2 / d2;
    cout << "  " << setw(10) << fixed << setprecision(2) << w
         << setw(15) << setprecision(6) << n2
         << setw(15) << d2
         << setw(15) << h2 << "\n";
  });

  cout << "\n  At w=1 (cutoff), |H(j)|^2 should be ~0.5 (-3dB).\n";
}


// =====================================================================
//  5. Sparse High-Degree Polynomials
// =====================================================================

/**
 * Demonstrates efficiency of sparse representation by constructing
 * polynomials with very high degree but few terms.
 */
static void sparse_polynomials()
{
  section("5. Sparse High-Degree Polynomials");

  // p(x) = x^1000 + x^500 + 1  (only 3 terms stored)
  DynList<std::pair<size_t, double>> terms_p;
  terms_p.append(std::make_pair(size_t(0), 1.0));
  terms_p.append(std::make_pair(size_t(500), 1.0));
  terms_p.append(std::make_pair(size_t(1000), 1.0));
  Polynomial p(terms_p);

  cout << "  p(x) = " << p << "\n";
  cout << "  degree = " << p.degree() << ", terms stored = "
       << p.num_terms() << "\n\n";

  // q(x) = x^1000 - 1
  DynList<std::pair<size_t, double>> terms_q;
  terms_q.append(std::make_pair(size_t(0), -1.0));
  terms_q.append(std::make_pair(size_t(1000), 1.0));
  Polynomial q(terms_q);

  cout << "  q(x) = " << q << "\n";
  cout << "  degree = " << q.degree() << ", terms stored = "
       << q.num_terms() << "\n\n";

  // Subtraction: p - q = x^500 + 2  (cancels x^1000)
  Polynomial diff = p - q;
  cout << "  p - q = " << diff << "\n";
  cout << "  degree = " << diff.degree() << ", terms = "
       << diff.num_terms() << "\n\n";

  // Derivative of p: 1000*x^999 + 500*x^499
  Polynomial dp = p.derivative();
  cout << "  p'(x) = " << dp << "\n";
  cout << "  degree = " << dp.degree() << ", terms = "
       << dp.num_terms() << "\n\n";

  // Evaluate p at x = 1: 1 + 1 + 1 = 3
  cout << "  p(1) = " << p(1.0) << "\n";
  cout << "  p(0) = " << p(0.0) << "\n";

  // Composition: p(2x+1) — still sparse in stored terms
  Polynomial linear({1, 2}); // 2x + 1
  Polynomial composed = p.compose(linear);
  cout << "\n  p(2x+1) has degree " << composed.degree()
       << " with " << composed.num_terms() << " terms\n";
}


// =====================================================================
//  6. Root Analysis and Finding
// =====================================================================

/**
 * Demonstrates Sturm sequences, root counting, Cauchy bounds,
 * Descartes' rule of signs, bisection, and Newton-Raphson.
 */
static void root_analysis()
{
  section("6. Root Analysis and Finding");

  // p(x) = (x-1)(x-2)(x+3) = x^3 - 7x + 6... let's build it
  Polynomial p = Polynomial::from_roots(DynList<double>({1.0, 2.0, -3.0}));
  cout << "  p(x) = " << p << "\n";
  cout << "  degree = " << p.degree() << "\n\n";

  // Cauchy bound: all roots |r| <= bound
  double bound = p.cauchy_bound();
  cout << "  Cauchy root bound: |roots| <= " << bound << "\n";
  cout << "  (actual roots: 1, 2, -3; max |root| = 3)\n\n";

  // Descartes' rule of signs
  cout << "  Sign variations of p(x):  " << p.sign_variations()
       << " (upper bound on positive roots)\n";
  cout << "  Sign variations of p(-x): " << p.negate_arg().sign_variations()
       << " (upper bound on negative roots)\n\n";

  // Sturm root counting
  cout << "  Real roots in [-10, 10]: "
       << p.count_real_roots(-10.0, 10.0) << "\n";
  cout << "  Real roots in [0, 5]:    "
       << p.count_real_roots(0.0, 5.0) << "\n";
  cout << "  Real roots in [-5, 0]:   "
       << p.count_real_roots(-5.0, 0.0) << "\n";
  cout << "  Total real roots:        "
       << p.count_all_real_roots() << "\n\n";

  // Root finding: bisection
  cout << "  Root finding (bisection):\n";
  double r1 = p.bisect_root(0.5, 1.5);
  double r2 = p.bisect_root(1.5, 3.0);
  double r3 = p.bisect_root(-5.0, -1.0);
  cout << "    root in [0.5, 1.5]:  " << fixed << setprecision(10)
       << r1 << "\n";
  cout << "    root in [1.5, 3.0]:  " << r2 << "\n";
  cout << "    root in [-5, -1]:    " << r3 << "\n\n";

  // Root finding: Newton-Raphson
  cout << "  Root finding (Newton-Raphson):\n";
  r1 = p.newton_root(0.5);
  r2 = p.newton_root(2.5);
  r3 = p.newton_root(-2.0);
  cout << "    from x0=0.5:  " << r1 << "\n";
  cout << "    from x0=2.5:  " << r2 << "\n";
  cout << "    from x0=-2.0: " << r3 << "\n";
}


// =====================================================================
//  7. Algebraic Transformations
// =====================================================================

/**
 * Demonstrates square-free factorization, extended GCD (Bezout
 * identity), polynomial reversal, and Taylor shift.
 */
static void algebraic_transformations()
{
  section("7. Algebraic Transformations");

  // --- Square-free part ---
  // (x-1)^3 * (x-2) has repeated root at x=1
  Polynomial x_minus_1({-1, 1});
  Polynomial x_minus_2({-2, 1});
  Polynomial p = x_minus_1.pow(3) * x_minus_2;

  cout << "  p(x) = (x-1)^3*(x-2) = " << p << "\n";
  cout << "  degree = " << p.degree()
       << ", roots: 1 (mult 3), 2 (mult 1)\n\n";

  Polynomial sf = p.square_free();
  cout << "  Square-free part: " << sf << "\n";
  cout << "  degree = " << sf.degree() << " (repeated roots removed)\n\n";

  // --- Extended GCD (Bezout identity) ---
  Polynomial a({-1, 0, 1});  // x^2 - 1
  Polynomial b({-1, 1});     // x - 1
  auto [g, s, t] = Polynomial::xgcd(a, b);

  cout << "  Extended GCD:\n";
  cout << "    a(x) = " << a << "\n";
  cout << "    b(x) = " << b << "\n";
  cout << "    gcd   = " << g << "\n";
  cout << "    s(x)  = " << s << "\n";
  cout << "    t(x)  = " << t << "\n";

  Polynomial bezout = s * a + t * b;
  cout << "    s*a + t*b = " << bezout
       << (bezout == g ? "  [Bezout OK]" : "  [FAIL]") << "\n\n";

  // --- Polynomial reversal ---
  Polynomial q({1, 2, 3, 4}); // 1 + 2x + 3x^2 + 4x^3
  cout << "  Reversal:\n";
  cout << "    p(x)    = " << q << "\n";
  cout << "    rev(x)  = " << q.reverse() << "\n";
  cout << "    (reverses coefficient order: a_i -> a_{d-i})\n\n";

  // --- Taylor shift ---
  Polynomial f({0, 0, 1}); // x^2
  cout << "  Taylor shift:\n";
  cout << "    f(x)   = " << f << "\n";
  cout << "    f(x+2) = " << f.shift(2.0) << "\n";
  cout << "    f(x-1) = " << f.shift(-1.0) << "\n";
}


// =====================================================================
//  Main
// =====================================================================

int main()
{
  cout << string(65, '*') << "\n";
  cout << "  Aleph-w Polynomial Example\n";
  cout << "  Gen_Polynomial<Coefficient> — Sparse Polynomial Arithmetic\n";
  cout << string(65, '*') << "\n";

  basic_arithmetic();
  calculus_example();
  roots_and_interpolation();
  transfer_function();
  sparse_polynomials();
  root_analysis();
  algebraic_transformations();

  cout << "\n" << string(65, '*') << "\n";
  cout << "  All examples completed successfully.\n";
  cout << string(65, '*') << "\n";

  return 0;
}
