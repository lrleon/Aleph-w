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
 * @file polynomial_reference_probe.cc
 * @brief Emits deterministic polynomial workloads/results as JSON for
 *        optional differential checks against external CAS tools.
 */

# include <cstdlib>
# include <iostream>
# include <sstream>
# include <string>

# include <tpl_polynomial.H>
# include <tpl_multi_polynomial.H>

using namespace Aleph;

namespace
{
  using IntPoly = Gen_Polynomial<long long>;
  using IntMultiPoly = Gen_MultiPolynomial<long long>;
  using LexMultiPoly = Gen_MultiPolynomial<double, Lex_Order>;

  template <typename Coeff>
  void
  print_coeff_array(const Gen_Polynomial<Coeff> &p)
  {
    std::cout << "[";
    for (size_t exp = 0; exp <= p.degree(); ++exp)
      {
        std::cout << p.get_coeff(exp);
        if (exp + 1 <= p.degree())
          std::cout << ", ";
      }
    std::cout << "]";
  }

  template <typename Poly>
  void
  print_term_array(const Poly &p)
  {
    bool first = true;
    std::cout << "[";
    p.for_each_term([&first](const Array<size_t> &idx, const typename Poly::Coeff &coeff)
    {
      if (not first)
        std::cout << ", ";
      first = false;
      std::cout << "{\"coeff\": " << coeff << ", \"exponents\": [";
      for (size_t i = 0; i < idx.size(); ++i)
        {
          std::cout << idx(i);
          if (i + 1 < idx.size())
            std::cout << ", ";
        }
      std::cout << "]}";
    });
    std::cout << "]";
  }

  template <typename TermList>
  void
  print_factor_terms(const TermList &terms)
  {
    bool first = true;
    std::cout << "[";
    for (auto it = terms.get_it(); it.has_curr(); it.next_ne())
      {
        if (not first)
          std::cout << ", ";
        first = false;
        const auto &term = it.get_curr();
        std::cout << "{\"multiplicity\": " << term.multiplicity << ", \"factor\": ";
        if constexpr (std::is_same_v<decltype(term.factor), IntPoly>)
          print_coeff_array(term.factor);
        else
          print_term_array(term.factor);
        std::cout << "}";
      }
    std::cout << "]";
  }

  void
  print_univariate_section()
  {
    IntPoly a({-5, 2, -3, 0, 1});
    IntPoly b({2, -1, 1});
    IntPoly prod = a * b;
    auto [q, r] = prod.divmod(b);

    IntPoly outer({1, -2, 0, 0, 1});
    IntPoly inner({1, 1, 2});
    IntPoly composed = outer.compose(inner);

    IntPoly f1({1, 1, 1});
    IntPoly f2({-3, 1});
    IntPoly f3({1, 2});
    IntPoly factorable = f3 * f2 * f1 * f1;
    auto uni_factors = factorable.factorize();

    std::cout << "  \"univariate\": [\n"
              << "    {\n"
              << "      \"name\": \"sum_mul_divmod\",\n"
              << "      \"a\": ";
    print_coeff_array(a);
    std::cout << ",\n"
              << "      \"b\": ";
    print_coeff_array(b);
    std::cout << ",\n"
              << "      \"sum\": ";
    print_coeff_array(a + b);
    std::cout << ",\n"
              << "      \"product\": ";
    print_coeff_array(prod);
    std::cout << ",\n"
              << "      \"quotient\": ";
    print_coeff_array(q);
    std::cout << ",\n"
              << "      \"remainder\": ";
    print_coeff_array(r);
    std::cout << "\n"
              << "    },\n"
              << "    {\n"
              << "      \"name\": \"compose_sparse\",\n"
              << "      \"outer\": ";
    print_coeff_array(outer);
    std::cout << ",\n"
              << "      \"inner\": ";
    print_coeff_array(inner);
    std::cout << ",\n"
              << "      \"composed\": ";
    print_coeff_array(composed);
    std::cout << "\n"
              << "    },\n"
              << "    {\n"
              << "      \"name\": \"factorize_integer\",\n"
              << "      \"poly\": ";
    print_coeff_array(factorable);
    std::cout << ",\n"
              << "      \"factors\": ";
    print_factor_terms(uni_factors);
    std::cout << "\n"
              << "    }\n"
              << "  ]";
  }

  void
  print_multivariate_section()
  {
    auto x = IntMultiPoly::variable(2, 0);
    auto y = IntMultiPoly::variable(2, 1);

    IntMultiPoly diff_sq = x * x - y * y;
    auto diff_sq_factors = diff_sq.factorize();

    IntMultiPoly q1 = 2LL * x * x + 3LL * y + IntMultiPoly(2, 1LL);
    IntMultiPoly q2 = 3LL * x * x + 5LL * y + IntMultiPoly(2, 2LL);
    IntMultiPoly same_degree = q1 * q2;
    auto same_degree_factors = same_degree.factorize();

    auto xlex = LexMultiPoly::variable(2, 0);
    auto ylex = LexMultiPoly::variable(2, 1);
    Array<LexMultiPoly> gens(2, LexMultiPoly(2));
    gens(0) = xlex * xlex - ylex;
    gens(1) = xlex * ylex - LexMultiPoly(2, 1.0);
    auto gb = LexMultiPoly::reduced_groebner_basis(gens);

    std::cout << ",\n"
              << "  \"multivariate\": [\n"
              << "    {\n"
              << "      \"name\": \"factorize_difference_of_squares\",\n"
              << "      \"nvars\": 2,\n"
              << "      \"poly\": ";
    print_term_array(diff_sq);
    std::cout << ",\n"
              << "      \"factors\": ";
    print_factor_terms(diff_sq_factors);
    std::cout << "\n"
              << "    },\n"
              << "    {\n"
              << "      \"name\": \"factorize_same_degree_non_monic\",\n"
              << "      \"nvars\": 2,\n"
              << "      \"poly\": ";
    print_term_array(same_degree);
    std::cout << ",\n"
              << "      \"factors\": ";
    print_factor_terms(same_degree_factors);
    std::cout << "\n"
              << "    },\n"
              << "    {\n"
              << "      \"name\": \"groebner_reduced_lex\",\n"
              << "      \"nvars\": 2,\n"
              << "      \"generators\": [";
    for (size_t i = 0; i < gens.size(); ++i)
      {
        print_term_array(gens(i));
        if (i + 1 < gens.size())
          std::cout << ", ";
      }
    std::cout << "],\n"
              << "      \"basis\": [";
    for (size_t i = 0; i < gb.size(); ++i)
      {
        print_term_array(gb(i));
        if (i + 1 < gb.size())
          std::cout << ", ";
      }
    std::cout << "]\n"
              << "    }\n"
              << "  ]";
  }
}

int
main(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i)
    {
      const std::string arg = argv[i];
      if (arg == "--help")
        {
          std::cout << "Usage: polynomial_reference_probe [--json]\n";
          return 0;
        }
      if (arg != "--json")
        {
          std::cerr << "Unknown option: " << arg << "\n";
          return 1;
        }
    }

  std::cout << "{\n"
            << "  \"mode\": \"reference\",\n";
  print_univariate_section();
  print_multivariate_section();
  std::cout << "\n}\n";
  return 0;
}
