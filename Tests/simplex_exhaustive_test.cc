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
 * @file simplex_exhaustive_test.cc
 * @brief Simplex and RevisedSimplex against an exact oracle.
 *
 * The oracle enumerates the vertices of a linear program with integer
 * data, in exact integer arithmetic: Cramer's rule, with determinants by
 * Bareiss' elimination. A box x <= U, with U above every coordinate of a
 * vertex of the program, makes the feasible set bounded; the program is
 * infeasible if no vertex is feasible, and unbounded if the optimum with
 * the box 2U differs from the one with U.
 *
 * Against it, every program of a small domain and thousands of random
 * ones check the state, the optimal value, the solution, the shadow
 * prices as an optimal dual solution, the reduced costs, the ranges of
 * the sensitivity analysis at their bounds and just beyond them, the
 * re-optimization of update_rhs_and_reoptimize(), Bland's rule, the type
 * float, latex_solve() and RevisedSimplex.
 */

#include <gtest/gtest.h>
#include <Simplex.H>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
  using CT = ConstraintType;
  using i64 = std::int64_t;

  // A linear program with integer data: optimize c·x subject to
  // a_i·x (type_i) b_i and x >= 0
  struct Program
  {
    size_t n = 0;
    bool maximize = true;
    std::vector<i64> c;
    std::vector<std::vector<i64>> a;
    std::vector<CT> type;
    std::vector<i64> b;

    [[nodiscard]] size_t m() const { return a.size(); }

    [[nodiscard]] std::string str() const
    {
      std::ostringstream s;
      s << (maximize ? "max" : "min");
      for (size_t j = 0; j < n; ++j)
        s << ' ' << c[j] << "x" << j;
      for (size_t i = 0; i < m(); ++i)
        {
          s << " |";
          for (size_t j = 0; j < n; ++j)
            s << ' ' << a[i][j] << "x" << j;
          s << (type[i] == CT::LE ? " <= " : type[i] == CT::GE ? " >= " : " = ") << b[i];
        }
      return s.str();
    }
  };

  // ------------------------------------------------------------------
  // Exact arithmetic of the oracle, which must not overflow
  // ------------------------------------------------------------------

  i64 magnitude(const i64 x) { return x < 0 ? -x : x; }

  i64 mul(const i64 x, const i64 y)
  {
    if (x != 0 and y != 0
        and magnitude(x) > std::numeric_limits<i64>::max() / magnitude(y))
      throw std::overflow_error("the oracle overflows");
    return x * y;
  }

  i64 add(const i64 x, const i64 y)
  {
    if ((y > 0 and x > std::numeric_limits<i64>::max() - y)
        or (y < 0 and x < -std::numeric_limits<i64>::max() - y))
      throw std::overflow_error("the oracle overflows");
    return x + y;
  }

  // Whether p1/q1 < p2/q2, with q1, q2 > 0
  bool less(const i64 p1, const i64 q1, const i64 p2, const i64 q2)
  {
    return mul(p1, q2) < mul(p2, q1);
  }

  // The determinant of M, by Bareiss' fraction-free elimination
  i64 det(std::vector<std::vector<i64>> M)
  {
    const size_t n = M.size();
    i64 sign = 1, prev = 1;
    for (size_t k = 0; k + 1 < n; ++k)
      {
        if (M[k][k] == 0)
          {
            size_t r = k + 1;
            while (r < n and M[r][k] == 0)
              ++r;
            if (r == n)
              return 0;
            std::swap(M[k], M[r]);
            sign = -sign;
          }
        for (size_t i = k + 1; i < n; ++i)
          for (size_t j = k + 1; j < n; ++j)
            M[i][j] = add(mul(M[i][j], M[k][k]), -mul(M[i][k], M[k][j])) / prev;
        prev = M[k][k];
      }
    return sign * M[n - 1][n - 1];
  }

  // The optimum that the oracle finds
  struct Optimum
  {
    enum Kind { Infeasible, Unbounded, Optimal } kind = Infeasible;
    i64 num = 0;            // the optimal value, num / den
    i64 den = 1;
    std::vector<i64> x;     // an optimal vertex, x / x_den
    i64 x_den = 1;

    [[nodiscard]] double value() const { return double(num) / double(den); }
  };

  // The best vertex of the program with the box x <= U, if one is feasible
  bool best_vertex(const Program & p, const i64 U, Optimum & best)
  {
    const size_t n = p.n, m = p.m();

    // The hyperplanes: the constraints, x_j = 0 and x_j = U
    const size_t h = m + 2 * n;
    auto row = [&] (size_t k, size_t j) -> i64
      {
        if (k < m)
          return p.a[k][j];
        return (k - m) % n == j ? 1 : 0;
      };
    auto rhs = [&] (size_t k) -> i64
      {
        if (k < m)
          return p.b[k];
        return k < m + n ? 0 : U;
      };

    bool found = false;
    std::vector<size_t> pick(n);
    std::vector<std::vector<i64>> M(n, std::vector<i64>(n));
    std::vector<i64> N(n);

    // every subset of n hyperplanes, in increasing order
    for (size_t i = 0; i < n; ++i)
      pick[i] = i;
    while (true)
      {
        for (size_t r = 0; r < n; ++r)
          for (size_t j = 0; j < n; ++j)
            M[r][j] = row(pick[r], j);

        if (i64 D = det(M); D != 0)
          {
            for (size_t j = 0; j < n; ++j)
              {
                auto Mj = M;
                for (size_t r = 0; r < n; ++r)
                  Mj[r][j] = rhs(pick[r]);
                N[j] = det(Mj);
              }
            if (D < 0)
              {
                D = -D;
                for (auto & v : N)
                  v = -v;
              }

            bool feasible = true;
            for (size_t j = 0; j < n and feasible; ++j)
              feasible = N[j] >= 0 and N[j] <= mul(U, D);
            for (size_t i = 0; i < m and feasible; ++i)
              {
                i64 lhs = 0;
                for (size_t j = 0; j < n; ++j)
                  lhs = add(lhs, mul(p.a[i][j], N[j]));
                const i64 bd = mul(p.b[i], D);
                feasible = p.type[i] == CT::LE ? lhs <= bd
                  : p.type[i] == CT::GE ? lhs >= bd : lhs == bd;
              }

            if (feasible)
              {
                i64 value = 0;
                for (size_t j = 0; j < n; ++j)
                  value = add(value, mul(p.c[j], N[j]));
                if (not found
                    or (p.maximize ? less(best.num, best.den, value, D)
                                   : less(value, D, best.num, best.den)))
                  {
                    best.num = value;
                    best.den = D;
                    best.x = N;
                    best.x_den = D;
                  }
                found = true;
              }
          }

        // the next subset
        size_t i = n;
        while (i > 0 and pick[i - 1] == h - n + i - 1)
          --i;
        if (i == 0)
          break;
        ++pick[i - 1];
        for (size_t k = i; k < n; ++k)
          pick[k] = pick[k - 1] + 1;
      }

    return found;
  }

  // A bound of the coordinates of the vertices of the program, with a
  // right-hand side up to extra_b too: by Cramer's rule,
  // n! * amax^(n-1) * bmax, at least 1. Scaling a row does not change the
  // vertices: the bound of the unscaled program holds.
  i64 vertex_bound(const Program & p, const i64 extra_b = 0)
  {
    i64 amax = 1, bmax = std::max<i64>(1, extra_b), f = 1;
    for (size_t i = 0; i < p.m(); ++i)
      {
        bmax = std::max(bmax, magnitude(p.b[i]));
        for (size_t j = 0; j < p.n; ++j)
          amax = std::max(amax, magnitude(p.a[i][j]));
      }
    for (size_t k = 2; k <= p.n; ++k)
      f = mul(f, i64(k));
    i64 bound = mul(f, bmax);
    for (size_t k = 1; k < p.n; ++k)
      bound = mul(bound, amax);
    return bound;
  }

  // The optimum of p, with the box U, above the coordinates of its
  // vertices; by default 2 * vertex_bound(p) + 1
  Optimum solve_exactly(const Program & p, i64 U = 0)
  {
    if (U == 0)
      U = 2 * vertex_bound(p) + 1;
    Optimum small, large;
    if (not best_vertex(p, U, small))
      return small; // Infeasible

    (void) best_vertex(p, 2 * U, large);
    if (small.num != large.num or small.den != large.den)
      if (mul(small.num, large.den) != mul(large.num, small.den))
        {
          large.kind = Optimum::Unbounded;
          return large;
        }

    small.kind = Optimum::Optimal;
    return small;
  }

  // ------------------------------------------------------------------
  // Rational approximation of a bound that the solver gives
  // ------------------------------------------------------------------

  // p/q, with q in [1, max_den], within tol of v, by continued fractions
  bool to_rational(const double v, i64 & p, i64 & q, const i64 max_den = 20000)
  {
    if (not std::isfinite(v) or std::abs(v) > 1e9)
      return false;

    double x = v;
    i64 p0 = 0, q0 = 1, p1 = 1, q1 = 0;
    for (int k = 0; k < 64; ++k)
      {
        const double f = std::floor(x);
        const i64 a = i64(f);
        const i64 p2 = add(mul(a, p1), p0), q2 = add(mul(a, q1), q0);
        if (q2 > max_den)
          break;
        p0 = p1; q0 = q1; p1 = p2; q1 = q2;
        if (std::abs(double(p1) / double(q1) - v) <= 1e-9 * std::max(1.0, std::abs(v)))
          {
            p = p1;
            q = q1;
            return true;
          }
        if (x - f < 1e-12)
          break;
        x = 1.0 / (x - f);
      }
    return false;
  }

  // ------------------------------------------------------------------
  // The solvers on a program
  // ------------------------------------------------------------------

  template <typename T>
  std::unique_ptr<Simplex<T>> make_simplex(const Program & p, const bool bland = false)
  {
    auto lp = std::make_unique<Simplex<T>>(p.n, p.maximize ? OptimizationType::Maximize
                                                           : OptimizationType::Minimize);
    for (size_t j = 0; j < p.n; ++j)
      lp->put_objetive_function_coef(j, T(p.c[j]));
    for (size_t i = 0; i < p.m(); ++i)
      {
        std::vector<T> row(p.n + 1);
        for (size_t j = 0; j < p.n; ++j)
          row[j] = T(p.a[i][j]);
        row[p.n] = T(p.b[i]);
        lp->put_restriction(row.data(), p.type[i]);
      }
    if (bland)
      lp->enable_bland_rule();
    lp->prepare_linear_program();
    return lp;
  }

  template <typename T>
  Optimum::Kind kind_of(const typename Simplex<T>::State s)
  {
    switch (s)
      {
      case Simplex<T>::Solved: return Optimum::Optimal;
      case Simplex<T>::Unbounded: return Optimum::Unbounded;
      default: return Optimum::Infeasible;
      }
  }

  const char * name(const Optimum::Kind k)
  {
    return k == Optimum::Optimal ? "Optimal" : k == Optimum::Unbounded ? "Unbounded"
      : "Infeasible";
  }

  // The tolerance for values of the size of v
  double tol(const double v, const double base = 1e-8)
  {
    return base * std::max(1.0, std::abs(v));
  }

  // What the checks have seen, so that a test can require that its programs
  // cover every case
  struct Coverage
  {
    size_t programs = 0;
    size_t optimal = 0;
    size_t infeasible = 0;
    size_t unbounded = 0;
    size_t degenerate = 0;           // optimal at a degenerate vertex
    size_t with_eq = 0;
    size_t with_negative_rhs = 0;
    size_t range_points = 0;         // sensitivity points checked
    size_t range_tight = 0;          // points beyond a range checked
    size_t reoptimized = 0;
  };

  // Of a solved program: the solution, its value, the shadow prices as an
  // optimal dual solution, and the reduced costs. Empty if all is right.
  std::string check_optimum(const Program & p, Simplex<double> & lp, const Optimum & want)
  {
    std::ostringstream err;
    const size_t n = p.n, m = p.m();
    lp.load_solution();
    std::vector<double> x(n);
    double value = 0;
    for (size_t j = 0; j < n; ++j)
      {
        x[j] = lp.get_solution(j);
        value += double(p.c[j]) * x[j];
        if (x[j] < -tol(x[j]))
          err << " x" << j << " = " << x[j] << " < 0;";
      }

    if (std::abs(value - want.value()) > tol(want.value()))
      err << " value " << value << ", want " << want.value() << ';';
    if (std::abs(lp.objetive_value() - value) > tol(value))
      err << " objetive_value() " << lp.objetive_value() << " != " << value << ';';

    for (size_t i = 0; i < m; ++i)
      {
        double lhs = 0;
        for (size_t j = 0; j < n; ++j)
          lhs += double(p.a[i][j]) * x[j];
        const double b = double(p.b[i]);
        const bool ok = p.type[i] == CT::LE ? lhs <= b + tol(b)
          : p.type[i] == CT::GE ? lhs >= b - tol(b) : std::abs(lhs - b) <= tol(b);
        if (not ok)
          err << " constraint " << i << " violated: " << lhs << " vs " << b << ';';
      }
    if (not lp.verify_solution())
      err << " verify_solution() is false;";

    // The shadow prices: signs, dual feasibility and strong duality
    const auto prices = lp.get_all_shadow_prices();
    double dual_value = 0;
    for (size_t i = 0; i < m; ++i)
      {
        const double y = lp.shadow_price(i);
        if (y != prices[i])
          err << " get_all_shadow_prices()[" << i << "] differs;";
        dual_value += y * double(p.b[i]);
        const bool up = (p.type[i] == CT::LE) == p.maximize; // y >= 0
        if (p.type[i] != CT::EQ and (up ? y < -tol(y) : y > tol(y)))
          err << " shadow price " << i << " = " << y << " of the wrong sign;";
      }
    if (std::abs(dual_value - want.value()) > tol(want.value(), 1e-7))
      err << " dual value " << dual_value << " != " << want.value() << ';';

    const auto costs = lp.get_all_reduced_costs();
    for (size_t j = 0; j < n; ++j)
      {
        double s = double(p.c[j]);
        for (size_t i = 0; i < m; ++i)
          s -= prices[i] * double(p.a[i][j]);
        const double rc = lp.reduced_cost(j);
        if (rc != costs[j])
          err << " get_all_reduced_costs()[" << j << "] differs;";
        if (std::abs(rc - s) > tol(s, 1e-7))
          err << " reduced cost " << j << " = " << rc << ", c - y A = " << s << ';';
        if (p.maximize ? rc > tol(rc, 1e-7) : rc < -tol(rc, 1e-7))
          err << " reduced cost " << j << " = " << rc << " of the wrong sign;";
        if ((x[j] > 1e-7 or lp.is_basic_variable(j)) and std::abs(rc) > tol(rc, 1e-7))
          err << " basic x" << j << " has reduced cost " << rc << ';';
      }

    return err.str();
  }

  // Whether the optimum of the solver is degenerate: fewer positive values
  // among the variables and the slacks of the inequalities than rows
  bool degenerate(const Program & p, const Simplex<double> & lp)
  {
    size_t positive = 0;
    for (size_t j = 0; j < p.n; ++j)
      if (lp.get_solution(j) > 1e-7)
        ++positive;
    for (size_t i = 0; i < p.m(); ++i)
      {
        if (p.type[i] == CT::EQ)
          continue;
        double lhs = 0;
        for (size_t j = 0; j < p.n; ++j)
          lhs += double(p.a[i][j]) * lp.get_solution(j);
        if (std::abs(lhs - double(p.b[i])) > 1e-7)
          ++positive;
      }
    return positive < p.m();
  }

  // Whether some non-basic reduced cost is zero: the dual is degenerate
  bool dual_degenerate(const Program & p, const Simplex<double> & lp)
  {
    for (size_t j = 0; j < p.n; ++j)
      if (lp.get_solution(j) <= 1e-7 and std::abs(lp.reduced_cost(j)) <= 1e-7)
        return true;
    for (size_t i = 0; i < p.m(); ++i)
      {
        if (p.type[i] == CT::EQ)
          continue;
        double lhs = 0;
        for (size_t j = 0; j < p.n; ++j)
          lhs += double(p.a[i][j]) * lp.get_solution(j);
        if (std::abs(lhs - double(p.b[i])) <= 1e-7 and std::abs(lp.shadow_price(i)) <= 1e-7)
          return true;
      }
    return false;
  }

  // The program with c_j = num / den: the whole objective scaled by den
  Program with_cost(const Program & p, const size_t j, const i64 num, const i64 den)
  {
    Program q = p;
    for (auto & v : q.c)
      v = mul(v, den);
    q.c[j] = num;
    return q;
  }

  // The program with b_i = num / den: the row i scaled by den
  Program with_rhs(const Program & p, const size_t i, const i64 num, const i64 den)
  {
    Program q = p;
    for (auto & v : q.a[i])
      v = mul(v, den);
    q.b[i] = num;
    return q;
  }

  // The ranges of objective_sensitivity(): at each finite bound and in the
  // middle, the solution stays optimal; just beyond a bound, of a
  // nondegenerate unique optimum, it is not
  std::string check_cost_ranges(const Program & p, Simplex<double> & lp, Coverage & cov)
  {
    std::ostringstream err;
    const bool tight = not degenerate(p, lp) and not dual_degenerate(p, lp);
    const i64 U = 2 * vertex_bound(p) + 1; // the vertices do not change
    std::vector<double> x(p.n);
    for (size_t j = 0; j < p.n; ++j)
      x[j] = lp.get_solution(j);

    for (size_t j = 0; j < p.n; ++j)
      {
        const auto r = lp.objective_sensitivity(j);
        if (r.current_value != double(p.c[j]))
          err << " cost range " << j << ": current " << r.current_value << ';';
        if (r.lower_bound > double(p.c[j]) + 1e-9 or r.upper_bound < double(p.c[j]) - 1e-9)
          err << " cost range " << j << " [" << r.lower_bound << ", " << r.upper_bound
              << "] misses " << p.c[j] << ';';

        // points: the bounds (or c_j -+ 5 if infinite), and beyond them
        struct Point { double v; };
        std::vector<Point> points;
        const double lo = r.is_unbounded_below() ? double(p.c[j]) - 5 : r.lower_bound;
        const double hi = r.is_unbounded_above() ? double(p.c[j]) + 5 : r.upper_bound;
        points.push_back({lo});
        points.push_back({hi});
        points.push_back({(lo + hi) / 2});

        for (const Point & pt : points)
          {
            i64 num, den;
            if (not to_rational(pt.v, num, den, 4000))
              continue;
            const Program q = with_cost(p, j, num, den);
            const Optimum o = solve_exactly(q, U);
            double value = 0;
            for (size_t k = 0; k < p.n; ++k)
              value += double(q.c[k]) * x[k];
            ++cov.range_points;
            if (o.kind != Optimum::Optimal or std::abs(o.value() - value) > tol(value, 1e-7))
              err << " cost " << j << " = " << pt.v << " inside its range: the solution gives "
                  << value << ", the optimum is " << name(o.kind) << ' ' << o.value() << ';';

            // just beyond a finite bound
            const bool is_lo = &pt == &points[0];
            const bool is_hi = &pt == &points[1];
            if (not tight or not ((is_lo and not r.is_unbounded_below())
                                  or (is_hi and not r.is_unbounded_above())))
              continue;

            const i64 out_num = mul(num, 2) + (is_lo ? -1 : 1), out_den = mul(den, 2);
            const Program qo = with_cost(p, j, out_num, out_den);
            const Optimum oo = solve_exactly(qo, U);
            double out_value = 0;
            for (size_t k = 0; k < p.n; ++k)
              out_value += double(qo.c[k]) * x[k];
            ++cov.range_tight;
            const bool still = oo.kind == Optimum::Optimal
              and std::abs(oo.value() - out_value) <= tol(out_value, 1e-9);
            if (still)
              err << " cost " << j << " just beyond " << pt.v << " keeps the solution optimal;";
          }
      }
    return err.str();
  }

  // The ranges of rhs_sensitivity(): at each finite bound and in the
  // middle, the optimum changes by the shadow price per unit, and
  // update_rhs_and_reoptimize() gives it without a pivot; just beyond a
  // bound, update_rhs_and_reoptimize() gives the optimum of the oracle, with
  // a pivot or as infeasible, and of a nondegenerate unique optimum the
  // shadow price no longer gives it
  std::string check_rhs_ranges(const Program & p, Simplex<double> & lp, const Optimum & want,
                               Coverage & cov)
  {
    std::ostringstream err;
    const bool tight = not degenerate(p, lp) and not dual_degenerate(p, lp);

    for (size_t i = 0; i < p.m(); ++i)
      {
        const auto r = lp.rhs_sensitivity(i);
        const double y = lp.shadow_price(i);
        const double b = double(p.b[i]);
        if (r.current_value != b)
          err << " rhs range " << i << ": current " << r.current_value << ';';
        if (r.lower_bound > b + 1e-9 or r.upper_bound < b - 1e-9)
          err << " rhs range " << i << " [" << r.lower_bound << ", " << r.upper_bound
              << "] misses " << b << ';';

        const double lo = r.is_unbounded_below() ? b - 5 : r.lower_bound;
        const double hi = r.is_unbounded_above() ? b + 5 : r.upper_bound;
        const double points[] = {lo, hi, (lo + hi) / 2};
        for (int k = 0; k < 3; ++k)
          {
            const double v = points[k];
            i64 num, den;
            if (not to_rational(v, num, den, 4000))
              continue;

            // the vertices of the program with b_i = v
            const i64 U = 2 * vertex_bound(p, i64(std::ceil(std::abs(v))) + 1) + 1;
            const Optimum o = solve_exactly(with_rhs(p, i, num, den), U);
            const double predicted = want.value() + y * (v - b);
            ++cov.range_points;
            if (o.kind != Optimum::Optimal or std::abs(o.value() - predicted) > tol(predicted, 1e-7))
              err << " rhs " << i << " = " << v << " inside its range: the optimum is "
                  << name(o.kind) << ' ' << o.value() << ", the shadow price gives "
                  << predicted << ';';

            auto again = make_simplex<double>(p);
            (void) again->solve();
            const auto st = again->update_rhs_and_reoptimize(i, v);
            again->load_solution();
            ++cov.reoptimized;
            if (st != Simplex<double>::Solved or again->get_pivot_count() != 0
                or std::abs(again->objetive_value() - predicted) > tol(predicted, 1e-7))
              err << " update_rhs_and_reoptimize(" << i << ", " << v << ") inside the range gives "
                  << name(kind_of<double>(st)) << ' ' << again->objetive_value() << " with "
                  << again->get_pivot_count() << " pivots;";

            const bool finite = k == 0 ? not r.is_unbounded_below()
              : k == 1 ? not r.is_unbounded_above() : false;
            if (not finite)
              continue;

            // just beyond the bound
            const i64 out_num = mul(num, 2) + (k == 0 ? -1 : 1), out_den = mul(den, 2);
            const double out = double(out_num) / double(out_den);
            const Program qo = with_rhs(p, i, out_num, out_den);
            const Optimum oo = solve_exactly(qo, U);
            auto beyond = make_simplex<double>(p);
            (void) beyond->solve();
            const auto sto = beyond->update_rhs_and_reoptimize(i, out);
            beyond->load_solution();
            ++cov.reoptimized;
            ++cov.range_tight;
            if (kind_of<double>(sto) != oo.kind
                or (oo.kind == Optimum::Optimal
                    and (std::abs(beyond->objetive_value() - oo.value()) > tol(oo.value(), 1e-7)
                         or beyond->get_pivot_count() == 0
                         or not beyond->verify_solution())))
              err << " update_rhs_and_reoptimize(" << i << ", " << out << ") beyond the range gives "
                  << name(kind_of<double>(sto)) << ' ' << beyond->objetive_value() << " with "
                  << beyond->get_pivot_count() << " pivots, the oracle " << name(oo.kind) << ' '
                  << oo.value() << ';';

            const double out_predicted = want.value() + y * (out - b);
            if (tight and oo.kind == Optimum::Optimal
                and std::abs(oo.value() - out_predicted) <= tol(out_predicted, 1e-9))
              err << " rhs " << i << " just beyond " << v << " keeps the shadow price;";
          }
      }
    return err.str();
  }

  // Checks the program; the deep checks (sensitivity, float, Bland) only
  // when asked. Empty if all is right.
  std::string check_program(const Program & p, Coverage & cov, const bool deep)
  {
    ++cov.programs;
    for (size_t i = 0; i < p.m(); ++i)
      {
        cov.with_eq += p.type[i] == CT::EQ;
        cov.with_negative_rhs += p.b[i] < 0;
      }

    const Optimum want = solve_exactly(p);
    auto lp = make_simplex<double>(p);
    const auto st = lp->solve();
    std::string err;
    if (kind_of<double>(st) != want.kind)
      return std::string(" state ") + name(kind_of<double>(st)) + ", want " + name(want.kind);

    switch (want.kind)
      {
      case Optimum::Infeasible:
        ++cov.infeasible;
        break;
      case Optimum::Unbounded:
        ++cov.unbounded;
        lp->load_solution();
        if (not lp->verify_solution())
          err += " the vertex of an unbounded program is not feasible;";
        break;
      case Optimum::Optimal:
        ++cov.optimal;
        err += check_optimum(p, *lp, want);
        if (degenerate(p, *lp))
          ++cov.degenerate;
        if (deep)
          {
            err += check_cost_ranges(p, *lp, cov);
            err += check_rhs_ranges(p, *lp, want, cov);
          }
        break;
      }

    // Bland's rule gives the same
    auto bland = make_simplex<double>(p, true);
    const auto stb = bland->solve();
    if (stb != st)
      err += std::string(" with Bland's rule: ") + name(kind_of<double>(stb));
    else if (st == Simplex<double>::Solved)
      {
        bland->load_solution();
        if (std::abs(bland->objetive_value() - want.value()) > tol(want.value()))
          err += " with Bland's rule the value is " + std::to_string(bland->objetive_value());
        if (not bland->get_stats().bland_rule_used and bland->get_pivot_count() > 0)
          err += " Bland's rule is not reported as used;";
      }

    if (deep)
      {
        auto lf = make_simplex<float>(p);
        const auto stf = lf->solve();
        if (kind_of<float>(stf) != want.kind)
          err += std::string(" with float: ") + name(kind_of<float>(stf));
        else if (stf == Simplex<float>::Solved)
          {
            lf->load_solution();
            if (std::abs(double(lf->objetive_value()) - want.value()) > tol(want.value(), 1e-3))
              err += " with float the value is " + std::to_string(lf->objetive_value());
          }
      }

    return err;
  }

  // RevisedSimplex on a program of <= constraints to maximize
  std::string check_revised(const Program & p, const size_t refactor_frequency)
  {
    RevisedSimplex<double> rs(p.n, p.m());
    for (size_t j = 0; j < p.n; ++j)
      rs.set_objective(j, double(p.c[j]));
    for (size_t i = 0; i < p.m(); ++i)
      {
        std::vector<double> row(p.n);
        for (size_t j = 0; j < p.n; ++j)
          row[j] = double(p.a[i][j]);
        rs.set_constraint_row(i, row.data(), double(p.b[i]));
      }
    rs.set_refactorization_frequency(refactor_frequency);

    const Optimum want = solve_exactly(p);
    const auto st = rs.solve();
    const Optimum::Kind got = st == RevisedSimplex<double>::Solved ? Optimum::Optimal
      : st == RevisedSimplex<double>::Unbounded ? Optimum::Unbounded : Optimum::Infeasible;
    if (got != want.kind)
      return std::string(" RevisedSimplex: ") + name(got) + ", want " + name(want.kind);
    if (got == Optimum::Optimal)
      {
        if (std::abs(rs.objective_value() - want.value()) > tol(want.value()))
          return " RevisedSimplex value " + std::to_string(rs.objective_value()) + ", want "
            + std::to_string(want.value());
        if (not rs.verify_solution())
          return " RevisedSimplex: verify_solution() is false";
      }
    return "";
  }

  // A random program: n in [1, max_n], m in [1, max_m], coefficients in
  // [-amax, amax], a third of them zero. In a third of the programs the
  // right-hand sides are in [-bmax, bmax], which makes most of them
  // infeasible; in the others they are such that a point x0 of
  // coordinates in [0, 3] is feasible, by up to 3 in an inequality.
  Program random_program(std::mt19937_64 & rng, const size_t max_n, const size_t max_m,
                         const i64 amax, const i64 bmax, const bool only_le = false)
  {
    auto pick = [&] (i64 lo, i64 hi) { return std::uniform_int_distribution<i64>(lo, hi)(rng); };
    Program p;
    p.n = size_t(pick(1, i64(max_n)));
    const size_t m = size_t(pick(1, i64(max_m)));
    p.maximize = only_le or pick(0, 1) == 1;
    p.c.resize(p.n);
    for (auto & v : p.c)
      v = pick(-amax, amax);

    const bool around_x0 = pick(0, 2) != 0;
    std::vector<i64> x0(p.n);
    for (auto & v : x0)
      v = pick(0, 3);

    for (size_t i = 0; i < m; ++i)
      {
        std::vector<i64> row(p.n);
        for (auto & v : row)
          v = pick(0, 2) == 0 ? 0 : pick(-amax, amax);
        const i64 t = pick(0, 9);
        const CT type = only_le or t < 5 ? CT::LE : t < 8 ? CT::GE : CT::EQ;
        i64 b = pick(-bmax, bmax);
        if (around_x0)
          {
            i64 ax = 0;
            for (size_t j = 0; j < p.n; ++j)
              ax += row[j] * x0[j];
            const i64 room = pick(0, 3);
            b = type == CT::LE ? ax + room : type == CT::GE ? ax - room : ax;
          }
        p.a.push_back(row);
        p.type.push_back(type);
        p.b.push_back(b);
      }
    return p;
  }
} // namespace

// ==========================================================================
// The oracle itself, on programs whose answer is known
// ==========================================================================

TEST(SimplexOracle, KnowsTheTextbookPrograms)
{
  Program p;
  p.n = 2;
  p.c = {40, 30};
  p.a = {{1, 1}, {2, 1}};
  p.type = {CT::LE, CT::LE};
  p.b = {40, 60};
  auto o = solve_exactly(p);
  ASSERT_EQ(o.kind, Optimum::Optimal);
  EXPECT_DOUBLE_EQ(o.value(), 1400.0);

  p.a = {{1, 0}, {1, 0}};
  p.type = {CT::LE, CT::GE};
  p.b = {0, 1};
  EXPECT_EQ(solve_exactly(p).kind, Optimum::Infeasible);

  p.a = {{1, -1}};
  p.type = {CT::LE};
  p.b = {1};
  EXPECT_EQ(solve_exactly(p).kind, Optimum::Unbounded);

  p.maximize = false;
  p.c = {1, 1};
  p.a = {{1, 1}, {1, -1}};
  p.type = {CT::EQ, CT::EQ};
  p.b = {3, 1};
  o = solve_exactly(p);
  ASSERT_EQ(o.kind, Optimum::Optimal);
  EXPECT_DOUBLE_EQ(o.value(), 3.0);
}

TEST(SimplexOracle, RationalApproximation)
{
  i64 p, q;
  ASSERT_TRUE(to_rational(2.5, p, q));
  EXPECT_EQ(p, 5);
  EXPECT_EQ(q, 2);
  ASSERT_TRUE(to_rational(-1.0 / 3.0, p, q));
  EXPECT_EQ(p, -1);
  EXPECT_EQ(q, 3);
  ASSERT_TRUE(to_rational(7.0, p, q));
  EXPECT_EQ(p, 7);
  EXPECT_EQ(q, 1);
  EXPECT_FALSE(to_rational(std::numeric_limits<double>::infinity(), p, q));
}

// ==========================================================================
// Every program of a small domain
// ==========================================================================

// n and m of 1 and 2, coefficients and right-hand sides in {-1, 0, 1},
// every type, every objective in {-1, 0, 1}^n, maximizing and minimizing
TEST(SimplexExhaustive, EveryProgramOfTheSmallDomain)
{
  Coverage cov;
  size_t failures = 0;
  const CT types[] = {CT::LE, CT::GE, CT::EQ};

  for (size_t n = 1; n <= 2; ++n)
    {
      // every row: n coefficients, a right-hand side and a type
      std::vector<std::pair<std::vector<i64>, std::pair<i64, CT>>> rows;
      const size_t combos = n == 1 ? 3 : 9;
      for (size_t code = 0; code < combos; ++code)
        {
          std::vector<i64> a(n);
          size_t rest = code;
          for (size_t j = 0; j < n; ++j, rest /= 3)
            a[j] = i64(rest % 3) - 1;
          for (i64 b = -1; b <= 1; ++b)
            for (CT t : types)
              rows.push_back({a, {b, t}});
        }

      std::vector<std::vector<i64>> objectives;
      for (size_t code = 0; code < combos; ++code)
        {
          std::vector<i64> c(n);
          size_t rest = code;
          for (size_t j = 0; j < n; ++j, rest /= 3)
            c[j] = i64(rest % 3) - 1;
          objectives.push_back(c);
        }

      for (size_t m = 1; m <= 2; ++m)
        {
          std::vector<size_t> idx(m, 0);
          while (true)
            {
              for (const auto & c : objectives)
                for (bool maximize : {true, false})
                  {
                    Program p;
                    p.n = n;
                    p.maximize = maximize;
                    p.c = c;
                    for (size_t k : idx)
                      {
                        p.a.push_back(rows[k].first);
                        p.b.push_back(rows[k].second.first);
                        p.type.push_back(rows[k].second.second);
                      }
                    const std::string e = check_program(p, cov, false);
                    if (not e.empty() and ++failures <= 20)
                      ADD_FAILURE() << p.str() << ":" << e;
                  }

              size_t k = 0;
              while (k < m and ++idx[k] == rows.size())
                idx[k++] = 0;
              if (k == m)
                break;
            }
        }
    }

  EXPECT_EQ(failures, 0u);
  EXPECT_GT(cov.programs, 100000u);
  EXPECT_GT(cov.optimal, 10000u);
  EXPECT_GT(cov.infeasible, 10000u);
  EXPECT_GT(cov.unbounded, 10000u);
  EXPECT_GT(cov.degenerate, 1000u);
}

// ==========================================================================
// Random programs, with the deep checks
// ==========================================================================

TEST(SimplexExhaustive, RandomProgramsWithTheirSensitivity)
{
  std::mt19937_64 rng(20261104);
  Coverage cov;
  size_t failures = 0;
  for (int k = 0; k < 1500; ++k)
    {
      const Program p = random_program(rng, 3, 4, 5, 10);
      const std::string e = check_program(p, cov, true);
      if (not e.empty() and ++failures <= 20)
        ADD_FAILURE() << p.str() << ":" << e;
    }

  EXPECT_EQ(failures, 0u);
  EXPECT_GT(cov.optimal, 400u);
  EXPECT_GT(cov.infeasible, 150u);
  EXPECT_GT(cov.unbounded, 200u);
  EXPECT_GT(cov.degenerate, 150u);
  EXPECT_GT(cov.with_eq, 400u);
  EXPECT_GT(cov.with_negative_rhs, 800u);
  EXPECT_GT(cov.range_points, 5000u);
  EXPECT_GT(cov.range_tight, 1500u);
  EXPECT_GT(cov.reoptimized, 4000u);
}

TEST(SimplexExhaustive, LargerRandomPrograms)
{
  std::mt19937_64 rng(4111);
  Coverage cov;
  size_t failures = 0;
  for (int k = 0; k < 400; ++k)
    {
      const Program p = random_program(rng, 4, 5, 5, 10);
      const std::string e = check_program(p, cov, false);
      if (not e.empty() and ++failures <= 20)
        ADD_FAILURE() << p.str() << ":" << e;
    }

  EXPECT_EQ(failures, 0u);
  EXPECT_GT(cov.optimal, 100u);
  EXPECT_GT(cov.infeasible, 35u);
  EXPECT_GT(cov.unbounded, 55u);
}

// The same programs, scaled by 1e6 and by 1e-6: the same states, and the
// values scaled as the data
TEST(SimplexExhaustive, ScaledProgramsGiveScaledResults)
{
  std::mt19937_64 rng(77);
  size_t failures = 0, solved = 0;
  for (int k = 0; k < 300; ++k)
    {
      const Program p = random_program(rng, 3, 4, 5, 10);
      const Optimum want = solve_exactly(p);
      for (double scale : {1e6, 1e-6})
        {
          // the constraints scaled by scale; the objective too
          Simplex<double> lp(p.n, p.maximize ? OptimizationType::Maximize
                                             : OptimizationType::Minimize);
          for (size_t j = 0; j < p.n; ++j)
            lp.put_objetive_function_coef(j, double(p.c[j]) * scale);
          for (size_t i = 0; i < p.m(); ++i)
            {
              std::vector<double> row(p.n + 1);
              for (size_t j = 0; j < p.n; ++j)
                row[j] = double(p.a[i][j]) * scale;
              row[p.n] = double(p.b[i]) * scale;
              lp.put_restriction(row.data(), p.type[i]);
            }
          lp.prepare_linear_program();
          const auto st = lp.solve();
          bool ok = kind_of<double>(st) == want.kind;
          if (ok and st == Simplex<double>::Solved)
            {
              ++solved;
              lp.load_solution();
              const double v = want.value() * scale;
              ok = std::abs(lp.objetive_value() - v) <= tol(v, 1e-7) and lp.verify_solution();
            }
          if (not ok and ++failures <= 20)
            ADD_FAILURE() << p.str() << " scaled by " << scale << ": "
                          << name(kind_of<double>(st)) << ", want " << name(want.kind);
        }
    }
  EXPECT_EQ(failures, 0u);
  EXPECT_GT(solved, 100u);
}

// ==========================================================================
// latex_solve() gives what solve() gives, and writes its files
// ==========================================================================

TEST(SimplexExhaustive, LatexSolveGivesWhatSolveGives)
{
  const auto dir = std::filesystem::temp_directory_path()
    / ("aleph_simplex_latex_" + std::to_string(std::random_device{}()));
  std::filesystem::create_directories(dir);

  std::mt19937_64 rng(5);
  size_t failures = 0;
  for (int k = 0; k < 60; ++k)
    {
      const Program p = random_program(rng, 3, 3, 5, 10);
      auto a = make_simplex<double>(p);
      auto b = make_simplex<double>(p);
      const auto sa = a->solve();
      const std::string base = (dir / ("p" + std::to_string(k))).string();
      const auto sb = b->latex_solve(base.c_str());
      bool ok = sa == sb and std::filesystem::exists(base + "-0.tex");
      if (ok and sa == Simplex<double>::Solved)
        {
          a->load_solution();
          b->load_solution();
          ok = std::abs(a->objetive_value() - b->objetive_value()) <= tol(a->objetive_value())
            and a->get_pivot_count() == b->get_pivot_count();
        }
      if (ok and b->get_pivot_count() > 0)
        ok = std::filesystem::exists(base + "-1.tex")
          and std::filesystem::exists(base + "-1-after.tex");
      if (not ok and ++failures <= 10)
        ADD_FAILURE() << p.str();
    }
  EXPECT_EQ(failures, 0u);

  std::filesystem::remove_all(dir);
}

// ==========================================================================
// RevisedSimplex
// ==========================================================================

TEST(RevisedSimplexExhaustive, RandomProgramsOfLessOrEqualConstraints)
{
  std::mt19937_64 rng(1234);
  size_t failures = 0, optimal = 0, infeasible = 0, unbounded = 0;
  for (int k = 0; k < 800; ++k)
    {
      const Program p = random_program(rng, 4, 5, 5, 10, true);
      const Optimum want = solve_exactly(p);
      optimal += want.kind == Optimum::Optimal;
      infeasible += want.kind == Optimum::Infeasible;
      unbounded += want.kind == Optimum::Unbounded;
      for (size_t f : {size_t(1), size_t(2), size_t(7), size_t(50)})
        if (const std::string e = check_revised(p, f); not e.empty() and ++failures <= 20)
          ADD_FAILURE() << p.str() << " refactorization every " << f << ":" << e;
    }
  EXPECT_EQ(failures, 0u);
  EXPECT_GT(optimal, 180u);
  EXPECT_GT(infeasible, 60u);
  EXPECT_GT(unbounded, 150u);
}

TEST(RevisedSimplexExhaustive, AgreesWithSimplex)
{
  std::mt19937_64 rng(99);
  size_t failures = 0;
  for (int k = 0; k < 300; ++k)
    {
      const Program p = random_program(rng, 4, 5, 5, 10, true);
      auto lp = make_simplex<double>(p);
      const auto st = lp->solve();
      RevisedSimplex<double> rs(p.n, p.m());
      for (size_t j = 0; j < p.n; ++j)
        rs.set_objective(j, double(p.c[j]));
      for (size_t i = 0; i < p.m(); ++i)
        {
          std::vector<double> row(p.n);
          for (size_t j = 0; j < p.n; ++j)
            row[j] = double(p.a[i][j]);
          rs.set_constraint_row(i, row.data(), double(p.b[i]));
        }
      const auto sr = rs.solve();
      bool ok = int(st) == int(sr);
      if (ok and st == Simplex<double>::Solved)
        {
          lp->load_solution();
          ok = std::abs(lp->objetive_value() - rs.objective_value()) <= tol(rs.objective_value());
        }
      if (not ok and ++failures <= 10)
        ADD_FAILURE() << p.str();
    }
  EXPECT_EQ(failures, 0u);
}
