/*
  This file is part of Aleph-w system.

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/**
  @file multi_polynomial_examples.cc
  @brief Practical examples demonstrating multivariate polynomial usage

  This file showcases key features of Gen_MultiPolynomial from tpl_multi_polynomial.H:
  - Constructing polynomials with variables and monomials
  - Polynomial arithmetic (addition, multiplication, powers)
  - Evaluation and substitution
  - Algebraic identities and expansions
  - Monomial ordering effects
  - Integration with univariate polynomials
*/

#include <iostream>
#include <iomanip>
#include "tpl_array.H"
#include "tpl_multi_polynomial.H"

using namespace std;
using namespace Aleph;

// Convenience typedefs for readable examples
using Multipoly = Gen_MultiPolynomial<double>;
using Multipoly_Lex = Gen_MultiPolynomial<double, Lex_Order>;
using Multipoly_Grlex = Gen_MultiPolynomial<double, Grlex_Order>;
using Multipoly_Grevlex = Gen_MultiPolynomial<double, Grevlex_Order>;

// Helper function to print section headers
void print_section(const string& title) {
  cout << "\n" << string(70, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(70, '=') << "\n";
}

// Helper function to print example result
void print_result(const string& label, const Multipoly& poly) {
  cout << "\n" << label << ": " << poly.to_str() << "\n";
}

int main() {
  cout << "\n" << string(70, '*') << "\n";
  cout << "*  Gen_MultiPolynomial Examples\n";
  cout << "*  Demonstrating multivariate polynomial operations\n";
  cout << string(70, '*') << "\n";

  // ========================================================================
  // Example 1: Basic Construction and Variable Definition
  // ========================================================================
  print_section("Example 1: Basic Construction");

  // Create polynomials in 2 variables
  const size_t nvars = 2;
  Multipoly x = Multipoly::variable(nvars, 0);  // x is variable 0
  Multipoly y = Multipoly::variable(nvars, 1);  // y is variable 1

  cout << "Variable x: " << x.to_str() << "\n";
  cout << "Variable y: " << y.to_str() << "\n";

  // Create a monomial: 3*x^2*y
  Multipoly mon = Multipoly::monomial(nvars, {2, 1}, 3.0);
  print_result("Monomial 3*x^2*y", mon);

  // ========================================================================
  // Example 2: Polynomial Arithmetic
  // ========================================================================
  print_section("Example 2: Polynomial Arithmetic");

  // (x + 2)
  Multipoly p1 = x + 2.0;
  print_result("p1 = x + 2", p1);

  // (y - 3)
  Multipoly p2 = y - 3.0;
  print_result("p2 = y - 3", p2);

  // (x + 2) * (y - 3) = xy - 3x + 2y - 6
  Multipoly product = p1 * p2;
  print_result("(x + 2) * (y - 3)", product);

  // ========================================================================
  // Example 3: Algebraic Identities - Difference of Squares
  // ========================================================================
  print_section("Example 3: Algebraic Identity - (a² - b²) = (a+b)(a-b)");

  // Create (x + y) and (x - y)
  Multipoly sum = x + y;
  Multipoly diff = x - y;

  // Compute (x + y)(x - y) = x² - y²
  Multipoly identity1 = sum * diff;
  print_result("(x + y)(x - y) expansion", identity1);

  // Direct computation: x² - y²
  Multipoly identity2 = x * x - y * y;
  print_result("x² - y² direct", identity2);

  cout << "\nAre they equal? " << (identity1 == identity2 ? "YES" : "NO") << "\n";

  // ========================================================================
  // Example 4: Binomial Expansion - (x + y)²
  // ========================================================================
  print_section("Example 4: Binomial Expansion - (x + y)²");

  Multipoly binomial = x + y;
  Multipoly expanded = binomial * binomial;
  print_result("(x + y)² expansion", expanded);

  // Manual form: x² + 2xy + y²
  Multipoly manual = x*x + 2.0*x*y + y*y;
  print_result("x² + 2xy + y² manual", manual);

  cout << "\nAre they equal? " << (expanded == manual ? "YES" : "NO") << "\n";

  // ========================================================================
  // Example 5: Using Polynomial Powers
  // ========================================================================
  print_section("Example 5: Polynomial Powers");

  Multipoly base = x + y;
  Multipoly pow2 = base.pow(2);
  Multipoly pow3 = base.pow(3);

  print_result("(x + y)^2", pow2);
  print_result("(x + y)^3", pow3);

  // (x + y)³ = x³ + 3x²y + 3xy² + y³
  cout << "\nVerification: (x + y)³ expands correctly\n";

  // ========================================================================
  // Example 6: Evaluation and Substitution
  // ========================================================================
  print_section("Example 6: Evaluation at Points");

  // Define a polynomial: f(x,y) = x² + xy + y²
  Multipoly f = x*x + x*y + y*y;
  print_result("f(x,y) = x² + xy + y²", f);

  // Evaluate at several points
  Array<double> pt1{1.0, 2.0};
  Array<double> pt2{2.0, 3.0};
  Array<double> pt3{0.0, 0.0};

  double val1 = f(pt1);  // f(1,2) = 1 + 2 + 4 = 7
  double val2 = f(pt2);  // f(2,3) = 4 + 6 + 9 = 19
  double val3 = f(pt3);  // f(0,0) = 0

  cout << "f(1, 2) = " << val1 << "\n";
  cout << "f(2, 3) = " << val2 << "\n";
  cout << "f(0, 0) = " << val3 << "\n";

  // ========================================================================
  // Example 7: Algebraic Application - Distance Function
  // ========================================================================
  print_section("Example 7: Algebraic Application - Distance Metric");

  // In optimization, we often work with squared distance to avoid sqrt
  // ||v||² = x² + y² (magnitude squared for vector (x,y))
  Multipoly distance_sq = x*x + y*y;
  print_result("||v||² = x² + y²", distance_sq);

  // Shifted origin: distance squared from point (a,b) is (x-a)² + (y-b)²
  Multipoly x_centered = x - 1.0;
  Multipoly y_centered = y - 2.0;
  Multipoly distance_from_point = x_centered * x_centered +
                                   y_centered * y_centered;
  print_result("Distance² from (1,2): (x-1)² + (y-2)²", distance_from_point);

  // ========================================================================
  // Example 8: Monomial Ordering Effects
  // ========================================================================
  print_section("Example 8: Monomial Ordering Effects");

  // Create polynomials with mixed degree terms using initializer_list
  // This demonstrates how different orderings rank the same monomials differently
  Multipoly_Lex poly_lex(2, {
    {Array<size_t>{2, 1}, 1.0},  // x²y
    {Array<size_t>{1, 2}, 1.0},  // xy²
    {Array<size_t>{0, 3}, 1.0}   // y³
  });

  Multipoly_Grlex poly_grlex(2, {
    {Array<size_t>{2, 1}, 1.0},
    {Array<size_t>{1, 2}, 1.0},
    {Array<size_t>{0, 3}, 1.0}
  });

  Multipoly_Grevlex poly_grevlex(2, {
    {Array<size_t>{2, 1}, 1.0},
    {Array<size_t>{1, 2}, 1.0},
    {Array<size_t>{0, 3}, 1.0}
  });

  cout << "Polynomial: x²y + xy² + y³\n\n";
  cout << "Lex ordering (x before y):    "
       << poly_lex.to_str() << "\n";
  cout << "Grlex ordering (total deg):   "
       << poly_grlex.to_str() << "\n";
  cout << "Grevlex ordering (std CAS):   "
       << poly_grevlex.to_str() << "\n";
  cout << "\nNote: The orderings determine term precedence in Gröbner basis\n"
       << "algorithms and polynomial division. Lex is used for elimination,\n"
       << "Grevlex is the standard CAS choice.\n";

  // ========================================================================
  // Example 9: Working with Three Variables
  // ========================================================================
  print_section("Example 9: Three Variable Polynomial");

  const size_t n3 = 3;
  Multipoly x3 = Multipoly::variable(n3, 0);
  Multipoly y3 = Multipoly::variable(n3, 1);
  Multipoly z3 = Multipoly::variable(n3, 2);

  // Sphere equation: x² + y² + z² - 1 = 0
  // (stored as polynomial x² + y² + z² - 1)
  Multipoly sphere = x3*x3 + y3*y3 + z3*z3 - 1.0;
  print_result("Sphere: x² + y² + z² - 1", sphere);

  // Evaluate on sphere at point (0.6, 0.8, 0)
  // Should be close to 0
  Array<double> sphere_pt{0.6, 0.8, 0.0};
  double sphere_val = sphere(sphere_pt);
  cout << "Evaluation at (0.6, 0.8, 0): " << sphere_val << "\n";
  cout << "(Expected ~0 for points on sphere)\n";

  // ========================================================================
  // Example 10: Polynomial Properties and Introspection
  // ========================================================================
  print_section("Example 10: Polynomial Properties");

  // Example polynomial: 2x²y + 3xy² + 5
  Multipoly p = 2.0 * x * x * y + 3.0 * x * y * y + 5.0;
  print_result("Polynomial p", p);

  cout << "\nPolynomial properties:\n";
  cout << "  Number of variables: " << p.num_vars() << "\n";
  cout << "  Number of terms: " << p.num_terms() << "\n";
  cout << "  Total degree: " << p.degree() << "\n";
  cout << "  Degree in x: " << p.degree_in(0) << "\n";
  cout << "  Degree in y: " << p.degree_in(1) << "\n";
  cout << "  Is zero? " << (p.is_zero() ? "YES" : "NO") << "\n";
  cout << "  Is constant? " << (p.is_constant() ? "YES" : "NO") << "\n";
  cout << "  Leading coefficient: " << p.leading_coeff() << "\n";

  // ========================================================================
  // Example 11: Cancellation and Simplification
  // ========================================================================
  print_section("Example 11: Cancellation of Opposite Terms");

  // Build p = (x + 1) - (x + 1) which should be zero
  Multipoly q = x + 1.0;
  Multipoly cancellation = q - q;

  print_result("(x + 1) - (x + 1)", cancellation);
  cout << "\nResulting polynomial is zero? "
       << (cancellation.is_zero() ? "YES" : "NO") << "\n";

  // ========================================================================
  // Example 12: Scalar Operations
  // ========================================================================
  print_section("Example 12: Scalar Multiplication and Division");

  Multipoly base_poly = x*x + y*y;
  print_result("Base polynomial: x² + y²", base_poly);

  Multipoly scaled = base_poly * 2.5;
  print_result("2.5 * (x² + y²)", scaled);

  Multipoly divided = scaled / 2.5;
  print_result("Result / 2.5 (back to original)", divided);

  cout << "\nAre they equal? "
       << (divided == base_poly ? "YES" : "NO") << "\n";

  // ========================================================================
  // Example 13: Iteration Over Terms
  // ========================================================================
  print_section("Example 13: Iterating Over Terms");

  Multipoly iter_poly = 2.0*x*x + 3.0*x*y + 5.0*y*y + 1.0;
  print_result("Polynomial", iter_poly);

  cout << "\nTerms in descending order (by monomial ordering):\n";
  size_t term_count = 0;
  iter_poly.for_each_term_desc([&term_count](const Array<size_t>& idx,
                                              double coeff) {
    cout << "  Term " << (++term_count) << ": coeff=" << coeff << ", ";
    cout << "exponents=[" << idx[0] << ", " << idx[1] << "]\n";
  });

  // ========================================================================
  // Example 14: Building Complex Expressions
  // ========================================================================
  print_section("Example 14: Complex Algebraic Expressions");

  // Build Rosenbrock-like expression: (1 - x)² + 100(y - x²)²
  // Simplified multivariate version: (1 - x)² + (y - x²)²
  Multipoly one_minus_x = 1.0 - x;
  Multipoly y_minus_x2 = y - x*x;

  Multipoly rosenbrock = one_minus_x * one_minus_x +
                         y_minus_x2 * y_minus_x2;
  print_result("(1-x)² + (y-x²)²", rosenbrock);

  // Evaluate at the optimum (1, 1)
  Array<double> optimum{1.0, 1.0};
  double val_at_optimum = rosenbrock(optimum);
  cout << "\nEvaluation at optimum (1, 1): " << val_at_optimum << "\n";

  // ========================================================================
  // Example 15: Relationship with Univariate Polynomials
  // ========================================================================
  print_section("Example 15: Univariate as Special Case");

  // A multivariate polynomial in 1 variable behaves like univariate
  const size_t n_uni = 1;
  Multipoly uni_poly = Multipoly::variable(n_uni, 0);

  // Build x² + 2x + 1 = (x + 1)²
  Multipoly uni_expr = uni_poly * uni_poly + 2.0 * uni_poly + 1.0;
  print_result("Single variable: x² + 2x + 1", uni_expr);

  // Evaluate
  Array<double> uni_pt{3.0};
  double uni_val = uni_expr(uni_pt);
  cout << "Evaluation at x=3: " << uni_val << "\n";
  cout << "(Expected: 9 + 6 + 1 = 16)\n";

  // ========================================================================
  // Example 16: Constant and Zero Polynomials
  // ========================================================================
  print_section("Example 16: Special Polynomials");

  Multipoly zero_poly = Multipoly(2);
  Multipoly const_poly = Multipoly(2, 42.0);
  Multipoly one_poly = const_poly / 42.0;

  print_result("Zero polynomial", zero_poly);
  print_result("Constant 42", const_poly);
  print_result("Constant 1 (42/42)", one_poly);

  cout << "\nzero_poly.is_zero() = " << (zero_poly.is_zero() ? "true" : "false") << "\n";
  cout << "const_poly.is_constant() = " << (const_poly.is_constant() ? "true" : "false") << "\n";
  cout << "one_poly.is_constant() = " << (one_poly.is_constant() ? "true" : "false") << "\n";

  // ========================================================================
  // Summary
  // ========================================================================
  print_section("Summary");

  cout << "\nGen_MultiPolynomial supports:\n"
       << "  • Construction via variables, monomials, and initializer lists\n"
       << "  • Full arithmetic: +, -, *, scalar /, powers\n"
       << "  • Evaluation at points (Array<Coefficient>)\n"
       << "  • Degree queries and term inspection\n"
       << "  • Three monomial orderings (Lex, Grlex, Grevlex)\n"
       << "  • Promotion to higher variable counts\n"
       << "  • Pretty-printing with automatic variable names\n"
       << "\nUse cases:\n"
       << "  • Algebraic computation and symbolic manipulation\n"
       << "  • Polynomial interpolation and least-squares fitting\n"
       << "  • Optimization (Rosenbrock, quadratic forms)\n"
       << "  • Foundation for Gröbner bases and factorization\n";

  cout << "\n" << string(70, '*') << "\n";
  cout << "*  End of Examples\n";
  cout << string(70, '*') << "\n\n";

  return 0;
}