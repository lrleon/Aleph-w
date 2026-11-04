
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


/** @file Simplex.H
 *  @brief Simplex algorithm for linear programming.
 *
 *  Implements the Simplex method for solving linear programming problems:
 *  maximize/minimize c'x subject to Ax ≤ b, ≥ b or = b, x ≥ 0.
 *
 *  ## Features
 *  - Two-phase simplex method: the first phase finds a feasible basis or
 *    proves that there is none
 *  - Handles unbounded and infeasible cases
 *  - Negative right-hand sides in any constraint
 *  - Dual Simplex for re-optimization
 *  - Sensitivity analysis for objective and RHS coefficients
 *  - Bland's anti-cycling rule, which a run of degenerate pivots also
 *    turns on
 *  - Direct minimization and >= / = constraints
 *  - Execution statistics (iterations, pivot counts)
 *  - RevisedSimplex, which keeps the inverse of the basis instead of the
 *    tableau
 *
 *  ## Complexity: O(2^n) worst case, polynomial average
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# ifndef SIMPLEX_H
# define SIMPLEX_H

# include <algorithm>
# include <cassert>
# include <chrono>
# include <cmath>
# include <fstream>
# include <iomanip>
# include <iostream>
# include <limits>
# include <memory>
# include <sstream>
# include <string>
# include <vector>
# include <tpl_dynMat.H>
# include <tpl_dynDlist.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Constraint type for linear programming.
   *
   * Specifies the type of inequality or equality for a constraint.
   */
  enum class ConstraintType { LE, GE, EQ }; ///< ≤, ≥, or =

  /** @brief Optimization direction for linear programming.
   *
   * Specifies whether to maximize or minimize the objective function.
   */
  enum class OptimizationType { Maximize, Minimize };

  /** @brief Execution statistics for the Simplex algorithm.
   *
   * Tracks performance metrics during the solving process.
   */
  struct SimplexStats
  {
    size_t iterations = 0;           ///< Total simplex iterations
    size_t pivots = 0;               ///< Total pivot operations
    size_t degenerate_pivots = 0;    ///< Pivots with zero improvement
    double elapsed_ms = 0.0;         ///< Elapsed time in milliseconds
    bool bland_rule_used = false;    ///< Whether Bland's rule was activated

    void reset() noexcept
    {
      iterations = 0;
      pivots = 0;
      degenerate_pivots = 0;
      elapsed_ms = 0.0;
      bland_rule_used = false;
    }
  };

  /** @brief Sensitivity analysis result for a coefficient.
   *
   * Represents the range within which a coefficient can vary
   * without changing the optimal basis.
   */
  template <typename T>
  struct SensitivityRange
  {
    T lower_bound;   ///< Minimum value before basis change
    T upper_bound;   ///< Maximum value before basis change
    T current_value; ///< Current coefficient value

    bool is_unbounded_below() const noexcept
    { return lower_bound == -std::numeric_limits<T>::infinity(); }

    bool is_unbounded_above() const noexcept
    { return upper_bound == std::numeric_limits<T>::infinity(); }
  };

  /** @brief Linear program solver using the Simplex method.
   *
   * `Simplex<T>` allows expressing linear programs in standard or
   * non-standard forms, supporting both maximization and minimization,
   * as well as ≤, ≥, and = constraints directly.
   *
   * ## Standard Form
   *
   * The standard form of a linear program is:
   * ```
   * Maximize:    Z = c₁x₁ + c₂x₂ + ... + cₙxₙ
   * Subject to:  a₁₁x₁ + a₁₂x₂ + ... + a₁ₙxₙ ≤ b₁
   *              a₂₁x₁ + a₂₂x₂ + ... + a₂ₙxₙ ≤ b₂
   *              ...
   *              aₘ₁x₁ + aₘ₂x₂ + ... + aₘₙxₙ ≤ bₘ
   *              x₁, x₂, ..., xₙ ≥ 0
   * ```
   *
   * ## Non-Standard Constraints (Automatic Handling)
   *
   * This implementation handles non-standard forms directly:
   * 1. **Minimization**: Use `set_minimize()` instead of negating coefficients
   * 2. **≥ constraints**: Use `put_restriction(coefs, ConstraintType::GE)`
   * 3. **= constraints**: Use `put_restriction(coefs, ConstraintType::EQ)`
   * 4. **Negative right-hand sides**: allowed in any constraint
   * 5. **Unrestricted variables**: Replace xᵢ with (xᵢ⁺ - xᵢ⁻) where both ≥ 0
   *
   * ## Method
   *
   * prepare_linear_program() builds the tableau. A constraint whose
   * right-hand side is negative is multiplied by -1, which swaps ≤ and ≥,
   * so that every right-hand side is >= 0. A ≤ constraint gets a slack
   * variable, a ≥ one a surplus variable and an artificial one, and an =
   * one an artificial one.
   *
   * solve() runs two phases. The first one minimizes the sum of the
   * artificial variables: if it stays above zero, the problem is
   * infeasible. Else the artificial variables that are still basic, at
   * zero, leave the basis, but for those of constraints that are
   * combinations of others, and the second phase optimizes the objective
   * from that basis. An artificial variable never enters the basis again.
   * The basic variable of each row is kept, and the solution is read from
   * it.
   *
   * With Dantzig's rule, a run of degenerate pivots switches to Bland's
   * rule until a pivot improves the objective, so that the method always
   * ends.
   *
   * prepare_linear_program() scales the program: each constraint, each
   * column and the objective are multiplied by a power of two, which
   * changes no digit, so that their largest magnitudes are in [1/2, 1);
   * one whose largest magnitude is subnormal stays as it is, below the
   * tolerances. The solver works on the scaled program, so that its
   * tolerances do not depend on the units of the data, and scales its
   * results back: the solution, the shadow prices, the reduced costs and
   * the ranges are those of the program as it was given. A value of the
   * scaled program is taken as zero when its magnitude is below the
   * tolerance of set_epsilon(); a right-hand side, when it is below that
   * tolerance times the largest magnitude of the right-hand sides, if this
   * one is above 1.
   *
   * ## Complete Example: Production Planning Problem
   *
   * A factory produces two products (A and B) with limited resources:
   * - Product A: profit $40/unit, needs 1 hr labor, 2 hrs machine time
   * - Product B: profit $30/unit, needs 1 hr labor, 1 hr machine time
   * - Available: 40 labor hours, 60 machine hours per day
   *
   * **Mathematical formulation:**
   * ```
   * Maximize:    Z = 40·xₐ + 30·xᵦ  (daily profit)
   * Subject to:  xₐ + xᵦ ≤ 40       (labor constraint)
   *              2·xₐ + xᵦ ≤ 60     (machine constraint)
   *              xₐ, xᵦ ≥ 0         (non-negativity)
   * ```
   *
   * **Code:**
   * ```cpp
   * #include <Simplex.H>
   * #include <iostream>
   *
   * int main() {
   *   using namespace Aleph;
   *
   *   // Create solver with 2 decision variables
   *   Simplex<double> solver(2);
   *
   *   // Set objective function: maximize 40*x_A + 30*x_B
   *   solver.put_objetive_function_coef(0, 40.0);  // x_A coefficient
   *   solver.put_objetive_function_coef(1, 30.0);  // x_B coefficient
   *
   *   // Add constraints (format: {coef_xA, coef_xB, RHS})
   *   double labor[]   = {1.0, 1.0, 40.0};  // x_A + x_B <= 40
   *   double machine[] = {2.0, 1.0, 60.0};  // 2*x_A + x_B <= 60
   *
   *   solver.put_restriction(labor);
   *   solver.put_restriction(machine);
   *
   *   // Prepare tableau and solve
   *   solver.prepare_linear_program();
   *   auto state = solver.solve();
   *
   *   if (state == Simplex<double>::Solved) {
   *     solver.load_solution();
   *
   *     std::cout << "Optimal solution found!\n";
   *     std::cout << "Product A: " << solver.get_solution(0) << " units\n";
   *     std::cout << "Product B: " << solver.get_solution(1) << " units\n";
   *     std::cout << "Maximum profit: $" << solver.objetive_value() << "\n";
   *
   *     if (solver.verify_solution())
   *       std::cout << "Solution verified: all constraints satisfied.\n";
   *   }
   *   else if (state == Simplex<double>::Unbounded) {
   *     std::cout << "Problem is unbounded (check constraints).\n";
   *   }
   *   else if (state == Simplex<double>::Unfeasible) {
   *     std::cout << "No point satisfies all the constraints.\n";
   *   }
   *
   *   return 0;
   * }
   * // Output:
   * // Optimal solution found!
   * // Product A: 20 units
   * // Product B: 20 units
   * // Maximum profit: $1400
   * // Solution verified: all constraints satisfied.
   * ```
   *
   * ## Algorithm Complexity
   *
   * - **Time**: O(2ⁿ) worst case, but typically polynomial in practice
   * - **Space**: O((m+1) × (n+m+a+1)) for the tableau matrix, where a is
   *   the number of artificial variables
   *
   * Where n = number of variables, m = number of constraints.
   *
   * @tparam T Numeric type for coefficients and values (typically double).
   *
   * @see DynMatrix
   * @ingroup Networks
   */
  template <typename T>
  class Simplex
  {
  public:
    /** @brief Solution states for the linear program.
     *
     * - `Not_Solved`: Initial state, solve() has not been called yet.
     * - `Solving`: Algorithm is in progress (internal state).
     * - `Unbounded`: The objective improves without limit over the
     *   feasible points.
     * - `Solved`: An optimal solution has been found.
     * - `Unfeasible`: No point satisfies all the constraints.
     */
    enum State { Not_Solved, Solving, Unbounded, Solved, Unfeasible };

    /** @brief Pivot selection rule.
     *
     * - `Dantzig`: Most negative reduced cost; a run of degenerate pivots
     *   switches to Bland's rule until a pivot improves the objective.
     * - `Bland`: Least index (guaranteed termination, slower).
     */
    enum class PivotRule { Dantzig, Bland };

  private:
    std::unique_ptr<DynMatrix<T>> m; // Simplex tableau
    std::unique_ptr<T[]> objetive; // Objective function coefficients
    DynDlist<T *> rest_list; // List of restrictions
    DynDlist<ConstraintType> rest_types; // Type of each restriction
    size_t num_var; // Number of decision variables
    size_t num_rest; // Number of restrictions
    size_t num_artificial; // Number of artificial variables
    std::unique_ptr<T[]> solution; // Solution values
    State state; // Current state
    OptimizationType opt_type; // Maximize or Minimize
    PivotRule pivot_rule; // Pivot selection rule
    T eps; // Tolerance for floating point comparisons

    // Execution statistics
    mutable SimplexStats stats;

    // Reusable buffers for pivot operations (avoid repeated allocations)
    mutable std::vector<T> pivot_row_buffer;
    mutable std::vector<T> pivot_col_buffer;

    // The column of the basic variable of each row, the row i + 1 of the
    // tableau
    mutable std::vector<int> basic_vars;

    // Of each restriction: -1 if its row was multiplied by -1, so that its
    // right-hand side is >= 0; else 1
    std::vector<int> row_sign;

    // Of each restriction: the column that was the unit vector of its row
    // in the first tableau, its slack or its artificial variable, which
    // pivoting turns into the column of the inverse of the basis
    std::vector<size_t> unit_col;

    // The costs that the second phase minimizes: -c to maximize, c to
    // minimize, as prepare_linear_program() found them, scaled
    std::vector<T> phase2_cost;

    // The powers of two by which prepare_linear_program() multiplies each
    // row, each column and the objective, so that the largest magnitude of
    // each is in [1/2, 1): the solver works on the scaled program, whose
    // tolerances so do not depend on the units of the data, and the
    // results are scaled back
    std::vector<T> row_scale;
    std::vector<T> col_scale;
    T obj_scale = T{1};

    // The tolerances of eps, on the scaled program: times the largest
    // magnitude of its coefficients, of its right-hand sides and of its
    // objective, at least 1 each
    T tol_coef = T{0};
    T tol_rhs = T{0};
    T tol_cost = T{0};

    // The degenerate pivots in a row; past num_var + num_rest, Bland's rule
    size_t degenerate_run = 0;

    // During the first phase, the costs of the second one in terms of the
    // variables that are not basic. They break the ties of Dantzig's rule,
    // as a Big-M did: among the columns that lower the sum of the
    // artificial variables the most, the one that improves the objective
    // the most enters, which saves pivots to the second phase.
    std::vector<T> tie_cost;
    bool in_phase1 = false;

    // The columns of the tableau: the num_var decision variables, a slack
    // or surplus variable of each restriction (an = one has an empty
    // column), the artificial variables and the right-hand side
    size_t first_artificial() const noexcept { return num_var + num_rest; }

    size_t rhs_col() const noexcept { return num_var + num_rest + num_artificial; }

    bool is_artificial(const size_t j) const noexcept
    {
      return j >= first_artificial() and j < rhs_col();
    }

    // Whether the next pivot follows Bland's rule: always with it, and with
    // Dantzig's after a run of degenerate pivots
    bool bland_now() const noexcept
    {
      return pivot_rule == PivotRule::Bland or degenerate_run > num_var + num_rest;
    }

    // Whether two ratios of a ratio test are taken as equal
    bool same_ratio(const T & a, const T & b) const noexcept
    {
      return std::abs(a - b) <= eps * std::max(T{1}, std::abs(b));
    }

    // The column that enters the basis: one whose cost in row 0 is below
    // -tol, the most negative one with Dantzig's rule and the first one
    // with Bland's; -1 if there is none, which is optimal. In the first
    // phase, Dantzig's rule breaks its ties by tie_cost. An artificial
    // variable never enters.
    int compute_pivot_col(const T & tol) const noexcept
    {
      const size_t M = first_artificial();

      if (bland_now())
        {
          for (size_t j = 0; j < M; ++j)
            if (m->read(0, j) < -tol)
              return static_cast<int>(j);
          return -1;
        }

      int q = -1;
      T minimum = T{0};
      for (size_t j = 0; j < M; ++j)
        {
          const T & c = m->read(0, j);
          if (c >= -tol)
            continue;

          if (q == -1 or (c < minimum and not same_ratio(c, minimum)))
            {
              q = static_cast<int>(j);
              minimum = c;
            }
          else if (in_phase1 and same_ratio(c, minimum) and tie_cost[j] < tie_cost[q])
            q = static_cast<int>(j);
        }
      return q;
    }

    // The row whose basic variable leaves the basis when the column q
    // enters: the least ratio of its right-hand side to its entry in q,
    // among the positive entries. Among ties, Bland's rule takes the row
    // whose basic variable has the least index, and Dantzig's the largest
    // entry. -1 if no entry is positive, which is unbounded.
    int compute_pivot_row(const size_t q) const noexcept
    {
      const size_t rhs = rhs_col();
      const bool bland = bland_now();

      int p = -1;
      T min_ratio = T{0};
      T max_den = T{0};
      for (size_t i = 1; i <= num_rest; ++i)
        {
          const T den = m->read(i, q);
          if (den <= tol_coef)
            continue;

          const T ratio = std::max(m->read(i, rhs), T{0}) / den;
          if (p == -1 or (ratio < min_ratio and not same_ratio(ratio, min_ratio)))
            {
              p = static_cast<int>(i);
              min_ratio = ratio;
              max_den = den;
            }
          else if (same_ratio(ratio, min_ratio)
                   and (bland ? basic_vars[i - 1] < basic_vars[p - 1] : den > max_den))
            {
              p = static_cast<int>(i);
              max_den = den;
            }
        }
      return p;
    }

    // Performs pivot operation on element (p, q), whose variable becomes
    // the basic one of the row p.
    // Optimized version: uses reusable buffers to avoid repeated allocations.
    void to_pivot(size_t p, size_t q)
    {
      assert(p >= 1 and p <= num_rest and q < rhs_col());

      // Total columns: decision vars + slack/surplus + artificial + RHS
      const size_t M = rhs_col();  // last data column
      const size_t cols = M + 1;   // total columns including RHS
      const T pivot = m->read(p, q);
      const T inv_pivot = T{1} / pivot;

      // Resize buffers only if needed (amortized O(1) for repeated calls)
      if (pivot_row_buffer.size() < cols)
        pivot_row_buffer.resize(cols);
      if (pivot_col_buffer.size() < num_rest + 1)
        pivot_col_buffer.resize(num_rest + 1);

      // Cache pivot row (normalized) - avoids repeated reads during elimination
      for (size_t j = 0; j <= M; ++j)
        pivot_row_buffer[j] = (j == q) ? T{1} : m->read(p, j) * inv_pivot;

      // Write normalized pivot row back to matrix
      for (size_t j = 0; j <= M; ++j)
        m->write(p, j, pivot_row_buffer[j]);

      // The costs that break the ties of the first phase
      if (in_phase1)
        if (const T f = tie_cost[q]; f != T{0})
          {
            for (size_t j = 0; j <= M; ++j)
              tie_cost[j] -= f * pivot_row_buffer[j];
            tie_cost[q] = T{0};
          }

      // Cache pivot column values before modification
      for (size_t i = 0; i <= num_rest; ++i)
        pivot_col_buffer[i] = m->read(i, q);

      // Eliminate: for each row i != p, subtract (pivot_col[i] * pivot_row)
      for (size_t i = 0; i <= num_rest; ++i)
        {
          if (i == p)
            continue;

          const T factor = pivot_col_buffer[i];
          if (factor == T{0})
            continue;  // Skip rows where pivot column is already 0

          for (size_t j = 0; j <= M; ++j)
            if (j == q)
              m->write(i, j, T{0});  // Pivot column becomes 0
            else
              m->write(i, j, m->read(i, j) - factor * pivot_row_buffer[j]);
        }

      basic_vars[p - 1] = static_cast<int>(q);
    }

    // Pivots until row 0 has no cost below -cost_tol, which is optimal, or
    // the entering column has no positive entry, which is unbounded.
    // Before and after each pivot it calls hook(p, q, after).
    template <class Hook>
    State iterate(const T & cost_tol, Hook & hook)
    {
      const size_t rhs = rhs_col();
      degenerate_run = 0;
      while (true)
        {
          ++stats.iterations;
          const int q = compute_pivot_col(cost_tol);
          if (q < 0)
            return Solved;

          const int p = compute_pivot_row(static_cast<size_t>(q));
          if (p < 0)
            return Unbounded;

          if (bland_now())
            stats.bland_rule_used = true;

          // A degenerate pivot leaves the vertex, and the objective, as
          // they were
          if (m->read(p, rhs) <= tol_rhs)
            {
              ++stats.degenerate_pivots;
              ++degenerate_run;
            }
          else
            degenerate_run = 0;

          hook(static_cast<size_t>(p), static_cast<size_t>(q), false);
          ++stats.pivots;
          to_pivot(static_cast<size_t>(p), static_cast<size_t>(q));
          hook(static_cast<size_t>(p), static_cast<size_t>(q), true);
        }
    }

    // Sets row 0 to the costs of the first phase, the sum of the
    // artificial variables to minimize, as all of them are basic, and
    // tie_cost to the costs of the second phase, as the first basis costs
    // nothing
    void load_phase1_costs()
    {
      const size_t cols = rhs_col() + 1;
      std::vector<T> cost(cols, T{0});
      for (size_t i = 1; i <= num_rest; ++i)
        if (is_artificial(static_cast<size_t>(basic_vars[i - 1])))
          for (size_t j = 0; j < cols; ++j)
            if (not is_artificial(j))
              cost[j] -= m->read(i, j);

      for (size_t j = 0; j < cols; ++j)
        m->write(0, j, cost[j]);

      tie_cost.assign(cols, T{0});
      for (size_t j = 0; j < num_var; ++j)
        tie_cost[j] = phase2_cost[j];
    }

    // Sets row 0 to the costs of the second phase, in terms of the
    // variables that are not basic, which the pivots of the first phase
    // kept in tie_cost
    void load_phase2_costs()
    {
      for (size_t j = 0; j <= rhs_col(); ++j)
        m->write(0, j, tie_cost[j]);
    }

    // After the first phase, the artificial variables that are still basic,
    // at zero, leave the basis by a pivot on the largest entry of their row
    // in a column that is not artificial. A row without such an entry is a
    // combination of others: its artificial variable stays basic, at zero,
    // since no pivot changes the row.
    template <class Hook>
    void drive_out_artificials(Hook & hook)
    {
      const size_t M = first_artificial();
      const size_t rhs = rhs_col();
      for (size_t i = 1; i <= num_rest; ++i)
        {
          if (not is_artificial(static_cast<size_t>(basic_vars[i - 1])))
            continue;

          m->write(i, rhs, T{0}); // at most a rounding above zero

          size_t q = M;
          T largest = tol_coef;
          for (size_t j = 0; j < M; ++j)
            if (const T a = std::abs(m->read(i, j)); a > largest)
              {
                q = j;
                largest = a;
              }

          if (q == M) // what is left in the row is rounding
            {
              for (size_t j = 0; j < M; ++j)
                m->write(i, j, T{0});
              continue;
            }

          hook(i, q, false);
          ++stats.pivots;
          ++stats.degenerate_pivots;
          to_pivot(i, q);
          hook(i, q, true);
        }
    }

    // The two phases, with hook as in iterate()
    template <class Hook>
    State run(Hook & hook)
    {
      stats.reset();
      stats.bland_rule_used = pivot_rule == PivotRule::Bland;
      const auto start_time = std::chrono::high_resolution_clock::now();
      state = Solving;

      if (num_artificial > 0)
        {
          load_phase1_costs();
          in_phase1 = true;
          const T scale = T(num_artificial);
          (void) iterate(tol_coef * scale, hook); // bounded below by zero

          // the sum of the artificial variables, which is >= 0
          if (-m->read(0, rhs_col()) > tol_rhs * scale)
            state = Unfeasible;
          else
            {
              drive_out_artificials(hook);
              load_phase2_costs();
            }
          in_phase1 = false;
        }

      // Row 0 has the costs of the second phase: those of create_matrix(),
      // or those that the first phase left
      if (state == Solving)
        state = iterate(tol_cost, hook);

      const auto end_time = std::chrono::high_resolution_clock::now();
      stats.elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

      return state;
    }

    // value in fixed notation with d decimals
    static std::string format_entry(const T & value, const int d)
    {
      std::ostringstream s;
      s << std::fixed << std::setprecision(d) << value;
      return s.str();
    }

    // Validates variable index is within range.
    void verify_var_index(const size_t i) const
    {
      ah_out_of_range_error_if(i >= num_var)
        << "Variable index " << i << " out of range [0, " << (num_var - 1) << "]";
    }

    // Validates restriction index is within range.
    void verify_rest_index(const size_t i) const
    {
      ah_out_of_range_error_if(i >= num_rest)
        << "Restriction index " << i << " out of range [0, " << (num_rest - 1) << "]";
    }

    // Creates a new restriction array initialized to zero.
    T * create_restriction(ConstraintType type = ConstraintType::LE)
    {
      T *ptr = new T[num_var + 1];
      rest_list.append(ptr);
      rest_types.append(type);
      ++num_rest;
      for (size_t i = 0; i <= num_var; ++i)
        ptr[i] = 0;
      return ptr;
    }

    // The power of two that takes magnitude into [1/2, 1); 1 for 0, for a
    // subnormal magnitude, whose power could overflow, or for a magnitude
    // that is not finite
    static T scale_of(const T & magnitude) noexcept
    {
      if (not (magnitude >= std::numeric_limits<T>::min()) or not std::isfinite(magnitude))
        return T{1};
      int e;
      (void) std::frexp(magnitude, &e);
      return std::ldexp(T{1}, -e);
    }

    // Creates the simplex tableau from the objective and the constraints,
    // scaled, with the slack, surplus and artificial variables, and takes
    // the costs of the second phase and the tolerances
    void create_matrix()
    {
      // A row whose right-hand side is negative is multiplied by -1, which
      // swaps <= and >=
      std::vector<ConstraintType> type;
      type.reserve(num_rest);
      row_sign.assign(num_rest, 1);
      num_artificial = 0;
      {
        auto type_it = rest_types.get_it();
        for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), type_it.next_ne())
          {
            ConstraintType t = type_it.get_curr();
            if (it.get_curr()[num_var] < T{0})
              {
                row_sign[type.size()] = -1;
                if (t == ConstraintType::LE)
                  t = ConstraintType::GE;
                else if (t == ConstraintType::GE)
                  t = ConstraintType::LE;
              }
            if (t != ConstraintType::LE)
              ++num_artificial;
            type.push_back(t);
          }
      }

      // The scales: of each row, by its largest coefficient; of each
      // column, by its largest coefficient once the rows are scaled; and of
      // the objective, by its largest coefficient once the columns are
      row_scale.assign(num_rest, T{1});
      col_scale.assign(num_var, T{0});
      {
        size_t i = 0;
        for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
          {
            T largest = T{0};
            for (size_t j = 0; j < num_var; ++j)
              largest = std::max(largest, std::abs(it.get_curr()[j]));
            row_scale[i] = scale_of(largest);
          }
        i = 0;
        for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
          for (size_t j = 0; j < num_var; ++j)
            col_scale[j] = std::max(col_scale[j], std::abs(it.get_curr()[j]) * row_scale[i]);
        T largest = T{0};
        for (size_t j = 0; j < num_var; ++j)
          {
            col_scale[j] = scale_of(col_scale[j]);
            largest = std::max(largest, std::abs(objetive[j]) * col_scale[j]);
          }
        obj_scale = scale_of(largest);
      }

      const size_t total_cols = num_var + num_rest + num_artificial + 1; // +1 for RHS
      m = std::unique_ptr<DynMatrix<T>>(new DynMatrix<T>(num_rest + 1, total_cols));
      basic_vars.assign(num_rest, 0);
      unit_col.assign(num_rest, 0);

      T max_coef = T{0};
      T max_rhs = T{0};
      size_t artificial_col = num_var + num_rest;
      size_t row = 1;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++row)
        {
          const T *rest = it.get_curr();
          const T sign = T(row_sign[row - 1]) * row_scale[row - 1];

          // Decision variable coefficients
          for (size_t j = 0; j < num_var; ++j)
            {
              const T a = sign * rest[j] * col_scale[j];
              m->write(row, j, a);
              max_coef = std::max(max_coef, std::abs(a));
            }

          // RHS value, which is >= 0
          const T b = sign * rest[num_var];
          m->write(row, total_cols - 1, b);
          max_rhs = std::max(max_rhs, std::abs(b));

          // Slack/surplus and artificial variables
          const size_t slack_col = num_var + row - 1;
          switch (type[row - 1])
            {
            case ConstraintType::LE: // slack variable: +1, basic
              m->write(row, slack_col, T{1});
              unit_col[row - 1] = slack_col;
              break;
            case ConstraintType::GE: // surplus variable: -1, plus artificial: +1, basic
              m->write(row, slack_col, T{-1});
              m->write(row, artificial_col, T{1});
              unit_col[row - 1] = artificial_col++;
              break;
            case ConstraintType::EQ: // no slack/surplus, just artificial: +1, basic
              m->write(row, artificial_col, T{1});
              unit_col[row - 1] = artificial_col++;
              break;
            }
          basic_vars[row - 1] = static_cast<int>(unit_col[row - 1]);
        }

      // The costs of the second phase, which row 0 has until solve():
      // -c to maximize, c to minimize, so that a negative cost always
      // improves the objective
      const T sense = opt_type == OptimizationType::Maximize ? -obj_scale : obj_scale;
      T max_cost = T{0};
      phase2_cost.resize(num_var);
      for (size_t j = 0; j < num_var; ++j)
        {
          phase2_cost[j] = sense * objetive[j] * col_scale[j];
          m->write(0, j, phase2_cost[j]);
          max_cost = std::max(max_cost, std::abs(phase2_cost[j]));
        }

      tol_coef = eps * std::max(T{1}, max_coef);
      tol_rhs = eps * std::max(T{1}, max_rhs);
      tol_cost = eps * std::max(T{1}, max_cost);
    }

    // The restriction rest_idx
    T * restriction(const size_t rest_idx) const
    {
      size_t i = 0;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
        if (i == rest_idx)
          return it.get_curr();
      return nullptr;
    }

  public:
    /** @brief Constructs a Simplex solver for n variables.
     *
     * Initializes a linear program in standard form without constraints
     * and with all objective function coefficients set to zero.
     *
     * @param[in] n Number of decision variables in the system.
     * @param[in] type Optimization type: Maximize (default) or Minimize.
     *
     * @throw std::bad_alloc If memory allocation fails.
     * @throw std::invalid_argument If n is zero.
     */
    explicit Simplex(const size_t n,
                     OptimizationType type = OptimizationType::Maximize)
      : m(nullptr), objetive(new T[n]), num_var(n), num_rest(0),
        num_artificial(0), solution(new T[n]), state(Not_Solved),
        opt_type(type), pivot_rule(PivotRule::Dantzig),
        eps(std::numeric_limits<T>::epsilon() * 100)
    {
      ah_invalid_argument_if(n == 0)
        << "Number of variables must be greater than zero";

      // Initialize objective coefficients to zero
      for (size_t i = 0; i < n; ++i)
        objetive[i] = solution[i] = T{0};
    }

    /** @brief Destructor.
     *
     * Frees all allocated restriction arrays.
     */
    ~Simplex()
    {
      rest_list.for_each([](auto ptr) { delete[] ptr; });
    }

    // Non-copyable
    Simplex(const Simplex &) = delete;

    Simplex &operator=(const Simplex &) = delete;

    // Movable
    Simplex(Simplex &&) = default;

    Simplex &operator=(Simplex &&) = default;

    /** @brief Sets the optimization type.
     *
     * @param[in] type Maximize or Minimize.
     *
     * @note Must be called before prepare_linear_program().
     */
    void set_optimization_type(OptimizationType type) noexcept
    {
      opt_type = type;
    }

    /** @brief Sets minimization mode.
     *
     * Convenience method equivalent to set_optimization_type(Minimize).
     */
    void set_minimize() noexcept
    {
      opt_type = OptimizationType::Minimize;
    }

    /** @brief Sets maximization mode.
     *
     * Convenience method equivalent to set_optimization_type(Maximize).
     */
    void set_maximize() noexcept
    {
      opt_type = OptimizationType::Maximize;
    }

    /** @brief Sets the pivot selection rule.
     *
     * @param[in] rule Dantzig (default, faster) or Bland (anti-cycling).
     *
     * Bland's rule guarantees termination on degenerate problems but
     * may be slower on non-degenerate problems. Dantzig's rule switches to
     * it after a run of degenerate pivots, so that it ends too.
     */
    void set_pivot_rule(PivotRule rule) noexcept
    {
      pivot_rule = rule;
    }

    /** @brief Enables Bland's anti-cycling rule.
     *
     * Use this when dealing with potentially degenerate problems.
     */
    void enable_bland_rule() noexcept
    {
      pivot_rule = PivotRule::Bland;
      stats.bland_rule_used = true;
    }

    /** @brief Sets the floating-point comparison tolerance.
     *
     * A value of the scaled program (see the class documentation) is taken
     * as zero when its magnitude is below `epsilon`; a right-hand side,
     * when it is below `epsilon` times the largest magnitude of the
     * right-hand sides, if this one is above 1.
     *
     * @param[in] epsilon Tolerance for comparing values to zero.
     *
     * @note Must be called before prepare_linear_program().
     */
    void set_epsilon(T epsilon) noexcept
    {
      eps = epsilon;
    }

    /** @brief Gets the current optimization type.
     *
     * @return Maximize or Minimize.
     */
    [[nodiscard]] OptimizationType get_optimization_type() const noexcept
    {
      return opt_type;
    }

    /** @brief Sets a coefficient in the objective function.
     *
     * Sets the coefficient of variable x_i in the objective function.
     *
     * @param[in] i Variable index (0-based).
     * @param[in] coef Coefficient value.
     *
     * @throw std::out_of_range If i >= number of variables.
     */
    void put_objetive_function_coef(size_t i, const T & coef)
    {
      verify_var_index(i);
      objetive[i] = coef;
    }

    /** @brief Gets a coefficient from the objective function.
     *
     * @param[in] i Variable index (0-based).
     * @return The coefficient value.
     *
     * @throw std::out_of_range If i >= number of variables.
     */
    [[nodiscard]] const T &get_objetive_function_coef(size_t i) const
    {
      verify_var_index(i);
      return objetive[i];
    }

    /** @brief Sets all objective function coefficients from a DynArray.
     *
     * @param[in] coefs Array of coefficients. Must have at least num_var elements.
     */
    void put_objetive_function(const DynArray<T> & coefs)
    {
      for (size_t i = 0; i < num_var; ++i)
        objetive[i] = coefs[i];
    }

    /** @brief Sets all objective function coefficients from a C array.
     *
     * @param[in] coefs Array of coefficients. Must have at least num_var elements.
     */
    void put_objetive_function(const T coefs[])
    {
      for (size_t i = 0; i < num_var; ++i)
        objetive[i] = coefs[i];
    }

    /** @brief Adds a constraint to the linear program.
     *
     * Adds a constraint of the form:
     *   c[0]*x_0 + c[1]*x_1 + ... + c[n-1]*x_{n-1} [op] c[n]
     *
     * where c[n] is the RHS (right-hand side) bound and [op] is determined
     * by the constraint type (≤, ≥, or =). The RHS may be negative.
     *
     * @param[in] coefs Array of n+1 coefficients (n variable coefficients + RHS).
     *                  If nullptr, creates a zero-initialized constraint.
     * @param[in] type Constraint type: LE (≤), GE (≥), or EQ (=). Default is LE.
     *
     * @return Pointer to the internal constraint array (can be modified).
     *
     * @throw std::bad_alloc If memory allocation fails.
     */
    T * put_restriction(const T *coefs = nullptr,
                        ConstraintType type = ConstraintType::LE)
    {
      T *rest = create_restriction(type);

      if (coefs == nullptr)
        return rest;

      for (size_t i = 0; i <= num_var; ++i)
        rest[i] = coefs[i];

      return rest;
    }

    /** @brief Adds a ≥ constraint.
     *
     * Convenience method for adding greater-than-or-equal constraints.
     *
     * @param[in] coefs Array of n+1 coefficients.
     * @return Pointer to the internal constraint array.
     */
    T * put_ge_restriction(const T *coefs)
    {
      return put_restriction(coefs, ConstraintType::GE);
    }

    /** @brief Adds an = constraint.
     *
     * Convenience method for adding equality constraints.
     *
     * @param[in] coefs Array of n+1 coefficients.
     * @return Pointer to the internal constraint array.
     */
    T * put_eq_restriction(const T *coefs)
    {
      return put_restriction(coefs, ConstraintType::EQ);
    }

    /** @brief Adds a constraint from a DynArray.
     *
     * @param[in] coefs Array of n+1 coefficients.
     * @param[in] type Constraint type: LE (≤), GE (≥), or EQ (=). Default is LE.
     * @return Pointer to the internal constraint array.
     *
     * @throw std::bad_alloc If memory allocation fails.
     */
    T * put_restriction(const DynArray<T> & coefs,
                        ConstraintType type = ConstraintType::LE)
    {
      T *rest = create_restriction(type);

      for (size_t i = 0; i <= num_var; ++i)
        rest[i] = coefs[i];

      return rest;
    }

    /** @brief Gets a pointer to a constraint array.
     *
     * @param[in] rest_num Constraint index (0-based).
     * @return Pointer to the constraint coefficients array.
     *
     * @throw std::out_of_range If rest_num >= number of constraints.
     */
    [[nodiscard]] T * get_restriction(const size_t rest_num)
    {
      verify_rest_index(rest_num);
      return restriction(rest_num);
    }

    /** @brief Gets the number of constraints.
     *
     * @return Number of constraints added to the linear program.
     */
    [[nodiscard]] size_t get_num_restrictions() const noexcept
    {
      return num_rest;
    }

    /** @brief Gets the number of decision variables.
     *
     * @return Number of variables in the linear program.
     */
    [[nodiscard]] size_t get_num_vars() const noexcept
    {
      return num_var;
    }

    /** @brief Gets the objective function coefficients array.
     *
     * @return Pointer to the objective function coefficients.
     */
    [[nodiscard]] T * get_objetive_function() noexcept
    {
      return objetive.get();
    }

    /** @brief Gets the objective function coefficients array (const).
     *
     * @return Const pointer to the objective function coefficients.
     */
    [[nodiscard]] const T * get_objetive_function() const noexcept
    {
      return objetive.get();
    }

    /** @brief Gets a specific coefficient from a constraint.
     *
     * @param[in] rest_num Constraint index.
     * @param[in] idx Variable index within the constraint.
     * @return Reference to the coefficient.
     *
     * @throw std::out_of_range If indices are out of range.
     */
    [[nodiscard]] T &get_restriction_coef(const size_t rest_num, size_t idx)
    {
      verify_var_index(idx);
      return get_restriction(rest_num)[idx];
    }

    /** @brief Sets a specific coefficient in a constraint.
     *
     * @param[in] rest_num Constraint index.
     * @param[in] idx Variable index within the constraint.
     * @param[in] coef New coefficient value.
     *
     * @throw std::out_of_range If indices are out of range.
     */
    void put_restriction_coef(const size_t rest_num, const size_t idx, const T & coef)
    {
      get_restriction_coef(rest_num, idx) = coef;
    }

    /** @brief Prepares the linear program for solving.
     *
     * Must be called after all objective function coefficients and
     * constraints have been set, and before calling solve().
     *
     * Creates the internal simplex tableau matrix, with the slack, surplus
     * and artificial variables, after multiplying by -1 each constraint
     * whose right-hand side is negative.
     *
     * @throw std::bad_alloc If memory allocation fails.
     * @throw std::logic_error If no constraints have been added.
     */
    void prepare_linear_program()
    {
      ah_logic_error_if(num_rest == 0)
      << "Cannot prepare linear program without constraints";
      create_matrix();
    }

    /** @brief Solves the linear program.
     *
     * Solves a correctly and completely specified linear program
     * using the two-phase simplex method.
     *
     * @return Solution state:
     *   - `Unbounded`: The objective improves without limit.
     *   - `Solved`: Optimal solution found.
     *   - `Unfeasible`: No feasible solution exists.
     *
     * @note After solving, call load_solution() to retrieve variable values.
     *
     * @throw std::logic_error If solve() has already been called.
     * @throw std::logic_error If no constraints have been added.
     * @throw std::logic_error If prepare_linear_program() was not called.
     */
    [[nodiscard]] State solve()
    {
      ah_logic_error_if(state != Not_Solved) << "solve() has already been called";
      ah_logic_error_if(num_rest == 0) << "Linear program has no constraints";
      ah_logic_error_if(m == nullptr) << "prepare_linear_program() must be called before solve()";

      auto no_hook = [] (size_t, size_t, bool) { /* empty */ };
      return run(no_hook);
    }

    /** @brief Gets the current state of the solver.
     *
     * @return Current solution state.
     */
    [[nodiscard]] State get_state() const noexcept
    {
      return state;
    }

    /** @brief Loads the solution values into the solution array.
     *
     * Each decision variable takes the value of its row if it is basic,
     * and zero if not. After solve() returns `Solved`, it is an optimal
     * solution; after `Unbounded`, the vertex from which the objective
     * improves without limit.
     * After this, individual variable values can be retrieved with get_solution().
     */
    void load_solution() noexcept
    {
      for (size_t j = 0; j < num_var; ++j)
        solution[j] = T{0};

      if (m == nullptr)
        return;

      const size_t rhs = rhs_col();
      for (size_t i = 1; i <= num_rest; ++i)
        if (const int b = basic_vars[i - 1]; b < static_cast<int>(num_var))
          solution[b] = col_scale[b] * m->read(i, rhs);
    }

    /** @brief Gets the solution value for a specific variable.
     *
     * @param[in] i Variable index (0-based).
     * @return The optimal value of variable x_i.
     *
     * @pre load_solution() must have been called first.
     *
     * @note Does not validate index for performance. Use with valid indices.
     */
    [[nodiscard]] const T &get_solution(size_t i) const noexcept
    {
      assert(i < num_var);
      return solution[i];
    }

    /** @brief Computes and returns the objective function value.
     *
     * Calculates Z = sum(c_i * x_i) using the current solution.
     *
     * @return The optimal value of the objective function.
     *
     * @pre load_solution() must have been called first.
     */
    [[nodiscard]] T objetive_value() const noexcept
    {
      T sum = 0;
      for (size_t i = 0; i < num_var; ++i)
        sum += solution[i] * objetive[i];
      return sum;
    }

    /** @brief Verifies that the solution satisfies all constraints.
     *
     * Checks that every variable is >= 0 and that every constraint holds,
     * up to a tolerance relative to the magnitude of its terms: the
     * largest of the tolerance of set_epsilon() and 10000 times the
     * epsilon of `T`.
     *
     * @return true if all constraints are satisfied, false otherwise.
     *
     * @pre load_solution() must have been called first.
     */
    [[nodiscard]] bool verify_solution() const
    {
      const T tol = std::max(eps, T(10000) * std::numeric_limits<T>::epsilon());

      for (size_t j = 0; j < num_var; ++j)
        if (solution[j] < -tol * std::max(T{1}, std::abs(solution[j])))
          return false;

      auto type_it = rest_types.get_it();
      for (auto it = rest_list.get_it(); it.has_curr();
           it.next_ne(), type_it.next_ne())
        {
          const T *rest = it.get_curr();
          ConstraintType ctype = type_it.get_curr();
          T sum = 0;
          T size = std::abs(rest[num_var]);
          for (size_t j = 0; j < num_var; ++j)
            {
              sum += rest[j] * solution[j];
              size += std::abs(rest[j] * solution[j]);
            }

          const T rhs = rest[num_var];
          const T room = tol * std::max(T{1}, size);
          switch (ctype)
            {
            case ConstraintType::LE:
              if (sum > rhs + room)
                return false;
              break;
            case ConstraintType::GE:
              if (sum < rhs - room)
                return false;
              break;
            case ConstraintType::EQ:
              if (std::abs(sum - rhs) > room)
                return false;
              break;
            }
        }
      return true;
    }

    /** @brief Prints the simplex tableau to stdout.
     *
     * It is the tableau of the scaled program that the solver works on (see
     * the class documentation). Row 0 holds the costs that the current
     * phase minimizes, and the last column the right-hand sides. Useful for
     * debugging and understanding the algorithm's progress.
     *
     * @throw std::logic_error If prepare_linear_program() was not called.
     */
    void print_matrix() const
    {
      ah_logic_error_if(m == nullptr) << "prepare_linear_program() must be called first";

      for (size_t i = 0; i <= num_rest; ++i)
        {
          for (size_t j = 0; j <= rhs_col(); ++j)
            std::cout << format_entry(m->read(i, j), 2) << " ";

          std::cout << std::endl;
        }
    }

    /** @brief Exports the simplex tableau to a LaTeX file.
     *
     * @param[in] name Output file name.
     * @param[in] d Number of decimal places (default 2).
     * @param[in] p Pivot row to highlight (-1 for none).
     * @param[in] q Pivot column to highlight (-1 for none).
     *
     * @throw std::logic_error If prepare_linear_program() was not called.
     */
    void latex_matrix(const std::string & name, const int d = 2,
                      const int p = -1, const int q = -1) const
    {
      ah_logic_error_if(m == nullptr) << "prepare_linear_program() must be called first";

      std::ofstream out(name, std::ios::out);

      const size_t cols = rhs_col();

      out << "$\\left(\\begin{array}{c";
      for (size_t i = 0; i < cols; ++i)
        out << "c";
      out << "}" << std::endl;

      for (size_t i = 0; i <= num_rest; ++i)
        {
          for (size_t j = 0; j <= cols; ++j)
            {
              if (static_cast<int>(i) == p and static_cast<int>(j) == q)
                out << "\\circled{" << format_entry(m->read(i, j), d) << "}" << " ";
              else
                out << format_entry(m->read(i, j), d) << " ";
              if (j != cols)
                out << "& ";
            }

          if (i != num_rest)
            out << "\\\\";

          out << std::endl;
        }
      out << "\\end{array}\\right)$" << std::endl;
    }

    /** @brief Exports the linear program to a LaTeX file.
     *
     * Generates a LaTeX representation of the objective function
     * and all constraints.
     *
     * @param[in] name Output file name.
     */
    void latex_linear_program(const std::string & name) const
    {
      std::ofstream out(name, std::ios::out);
      out << "\\begin{equation*}" << std::endl
          << (opt_type == OptimizationType::Maximize ? "\\max " : "\\min ")
          << "Z = ";

      bool first = true;
      for (size_t i = 0; i < num_var; ++i)
        {
          if (objetive[i] == 0.0)
            continue;

          if (not first)
            out << " + ";
          first = false;

          if (objetive[i] != 1.0)
            out << objetive[i];
          out << "x_" << i;
        }
      if (first)
        out << "0";
      out << std::endl
          << "\\end{equation*}" << std::endl
          << "Subject to:" << std::endl
          << "\\begin{eqnarray*}" << std::endl;

      auto type_it = rest_types.get_it();
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), type_it.next_ne())
        {
          const T *rest = it.get_curr();

          bool first_term = true;
          for (size_t i = 0; i < num_var; ++i)
            {
              if (rest[i] == 0.0)
                continue;

              if (not first_term)
                out << " + ";
              first_term = false;

              if (rest[i] != 1.0)
                out << rest[i];
              out << " x_" << i;
            }
          if (first_term)
            out << "0";

          switch (type_it.get_curr())
            {
            case ConstraintType::LE: out << " & \\leq & "; break;
            case ConstraintType::GE: out << " & \\geq & "; break;
            case ConstraintType::EQ: out << " & = & "; break;
            }
          out << rest[num_var];

          if (not it.is_in_last())
            out << " \\\\";

          out << std::endl;
        }
      out << "\\end{eqnarray*}" << std::endl;
    }

    /** @brief Solves with LaTeX output at each iteration.
     *
     * As solve(), but it writes the tableau to LaTeX files for educational
     * purposes: `name-0.tex` before the first phase, `name-k.tex` with the
     * pivot of the pivot k circled, `name-k-after.tex` after it, and
     * `name-k.tex`, for the next k, at the end.
     *
     * @param[in] name Base name for output files.
     * @return Solution state.
     *
     * @throw std::logic_error As solve().
     */
    State latex_solve(const char *name = nullptr)
    {
      ah_logic_error_if(state != Not_Solved) << "solve() has already been called";
      ah_logic_error_if(num_rest == 0) << "Linear program has no constraints";
      ah_logic_error_if(m == nullptr) << "prepare_linear_program() must be called before solve()";

      const std::string base = name ? name : "simplex";
      latex_matrix(base + "-0.tex");

      size_t k = 1;
      auto hook = [&] (size_t p, size_t q, bool after)
        {
          if (after)
            latex_matrix(base + "-" + std::to_string(k++) + "-after.tex", 2,
                         static_cast<int>(p), static_cast<int>(q));
          else
            latex_matrix(base + "-" + std::to_string(k) + ".tex", 2,
                         static_cast<int>(p), static_cast<int>(q));
        };
      const State st = run(hook);
      latex_matrix(base + "-" + std::to_string(k) + ".tex");
      return st;
    }

    // ================== Statistics ==================

    /** @brief Gets execution statistics.
     *
     * Returns statistics about the most recent solve() call.
     *
     * @return Const reference to SimplexStats structure.
     */
    [[nodiscard]] const SimplexStats & get_stats() const noexcept
    {
      return stats;
    }

    /** @brief Gets the number of iterations performed.
     *
     * @return Number of simplex iterations in the last solve.
     */
    [[nodiscard]] size_t get_iteration_count() const noexcept
    {
      return stats.iterations;
    }

    /** @brief Gets the number of pivot operations.
     *
     * @return Number of pivots performed in the last solve.
     */
    [[nodiscard]] size_t get_pivot_count() const noexcept
    {
      return stats.pivots;
    }

    /** @brief Gets the number of degenerate pivots.
     *
     * A degenerate pivot is one that doesn't improve the objective.
     *
     * @return Number of degenerate pivots in the last solve.
     */
    [[nodiscard]] size_t get_degenerate_pivot_count() const noexcept
    {
      return stats.degenerate_pivots;
    }

    /** @brief Gets the elapsed solving time.
     *
     * @return Elapsed time in milliseconds.
     */
    [[nodiscard]] double get_elapsed_time_ms() const noexcept
    {
      return stats.elapsed_ms;
    }

    // ================== Sensitivity Analysis ==================

    /** @brief Computes sensitivity range for an objective coefficient.
     *
     * Determines the range [lower, upper] within which the objective
     * coefficient c_j can vary, the others fixed, while the optimal basis
     * stays optimal, and so the solution too.
     *
     * @param[in] var_idx Variable index (0-based).
     * @return SensitivityRange structure with bounds and current value; an
     *         infinite bound means no limit.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If var_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] SensitivityRange<T> objective_sensitivity(size_t var_idx) const
    {
      verify_var_index(var_idx);
      ah_logic_error_if(state != Solved) << "Must solve before sensitivity analysis";

      constexpr T inf = std::numeric_limits<T>::infinity();
      SensitivityRange<T> result;
      result.current_value = objetive[var_idx];

      // The range [lo, hi] of a change d of the cost that row 0 minimizes
      // (-c_j to maximize), for which no cost in row 0 goes below zero
      T lo = -inf, hi = inf;
      size_t row = 0;
      for (size_t i = 1; i <= num_rest; ++i)
        if (basic_vars[i - 1] == static_cast<int>(var_idx))
          row = i;

      if (row == 0) // not basic: only its own cost changes, by d
        lo = -std::max(m->read(0, var_idx), T{0});
      else // basic: the cost of each column k changes by -d times its entry
        for (size_t k = 0; k < first_artificial(); ++k)
          {
            const T a = m->read(row, k);
            if (k == var_idx or std::abs(a) <= tol_coef)
              continue;

            const T ratio = std::max(m->read(0, k), T{0}) / a;
            if (a > 0)
              hi = std::min(hi, ratio);
            else
              lo = std::max(lo, ratio);
          }

      // d is of the scaled cost: c_j changes by d / its scale
      const T unit = col_scale[var_idx] * obj_scale;
      lo /= unit;
      hi /= unit;

      if (opt_type == OptimizationType::Maximize) // c_j changes by -d
        {
          result.lower_bound = hi == inf ? -inf : result.current_value - hi;
          result.upper_bound = lo == -inf ? inf : result.current_value - lo;
        }
      else
        {
          result.lower_bound = lo == -inf ? -inf : result.current_value + lo;
          result.upper_bound = hi == inf ? inf : result.current_value + hi;
        }

      return result;
    }

    /** @brief Computes sensitivity range for a RHS value.
     *
     * Determines the range within which the right-hand side value b_i
     * can vary, the others fixed, while the optimal basis stays feasible,
     * and so optimal: over it, the optimal value changes by
     * shadow_price() per unit.
     *
     * @param[in] rest_idx Constraint index (0-based).
     * @return SensitivityRange structure with bounds and current value; an
     *         infinite bound means no limit.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If rest_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] SensitivityRange<T> rhs_sensitivity(size_t rest_idx) const
    {
      verify_rest_index(rest_idx);
      ah_logic_error_if(state != Solved) << "Must solve before sensitivity analysis";

      constexpr T inf = std::numeric_limits<T>::infinity();
      SensitivityRange<T> result;
      result.current_value = restriction(rest_idx)[num_var];

      // The range [lo, hi] of a change d of the right-hand side of the
      // row, which changes the right-hand sides by d times the column of
      // the inverse of the basis, for which none of them goes below zero;
      // an artificial variable that stays basic must stay at zero
      const size_t rhs = rhs_col();
      const size_t col = unit_col[rest_idx];
      T lo = -inf, hi = inf;
      for (size_t i = 1; i <= num_rest; ++i)
        {
          const T a = m->read(i, col);
          if (std::abs(a) <= tol_coef)
            continue;

          if (is_artificial(static_cast<size_t>(basic_vars[i - 1])))
            {
              lo = std::max(lo, T{0});
              hi = std::min(hi, T{0});
              continue;
            }

          const T ratio = -std::max(m->read(i, rhs), T{0}) / a;
          if (a > 0)
            lo = std::max(lo, ratio);
          else
            hi = std::min(hi, ratio);
        }

      // d is of the scaled row: b_i changes by d / its scale
      lo /= row_scale[rest_idx];
      hi /= row_scale[rest_idx];

      if (row_sign[rest_idx] < 0) // the row was multiplied by -1
        {
          const T old_lo = lo;
          lo = -hi;
          hi = -old_lo;
        }

      result.lower_bound = lo == -inf ? -inf : result.current_value + lo;
      result.upper_bound = hi == inf ? inf : result.current_value + hi;

      return result;
    }

    /** @brief Gets the shadow price (dual value) for a constraint.
     *
     * The shadow price is the change of the optimal value of the objective
     * per unit increase of the right-hand side b_i, over the range of
     * rhs_sensitivity(): when maximizing, >= 0 for a ≤ constraint and
     * <= 0 for a ≥ one; when minimizing, the other way. The shadow prices
     * are an optimal solution of the dual program.
     *
     * @param[in] rest_idx Constraint index (0-based).
     * @return Shadow price (dual variable value).
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If rest_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] T shadow_price(size_t rest_idx) const
    {
      verify_rest_index(rest_idx);
      ah_logic_error_if(state != Solved) << "Must solve before computing shadow prices";

      // The cost in row 0 of the column of the inverse of the basis of the
      // row is minus its dual value, in the problem that row 0 minimizes
      T price = m->read(0, unit_col[rest_idx]);
      if (opt_type == OptimizationType::Minimize)
        price = -price;

      // that of the scaled row and objective, scaled back
      price = price * row_scale[rest_idx] / obj_scale;

      return row_sign[rest_idx] < 0 ? -price : price;
    }

    /** @brief Gets the reduced cost for a variable.
     *
     * The reduced cost is the change of the objective per unit increase of
     * a non-basic variable, the others non-basic at zero:
     * c_j - sum_i shadow_price(i) * a_ij. At the optimum it is <= 0 when
     * maximizing and >= 0 when minimizing, and 0 for a basic variable.
     *
     * @param[in] var_idx Variable index (0-based).
     * @return Reduced cost value.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If var_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] T reduced_cost(size_t var_idx) const
    {
      verify_var_index(var_idx);
      ah_logic_error_if(state != Solved) << "Must solve before computing reduced costs";

      T rc = m->read(0, var_idx);

      // Adjust sign for maximization (reduced costs are negated in tableau)
      if (opt_type == OptimizationType::Maximize)
        rc = -rc;

      // that of the scaled column and objective, scaled back
      return rc / (col_scale[var_idx] * obj_scale);
    }

    /** @brief Checks if a variable is basic in the optimal solution.
     *
     * @param[in] var_idx Variable index (0-based).
     * @return true if variable is in the basis, false otherwise.
     *
     * @pre solve() must have been called and returned Solved.
     */
    [[nodiscard]] bool is_basic_variable(size_t var_idx) const
    {
      verify_var_index(var_idx);
      for (size_t i = 0; i < basic_vars.size(); ++i)
        if (basic_vars[i] == static_cast<int>(var_idx))
          return true;
      return false;
    }

    /** @brief Gets all shadow prices as a vector.
     *
     * @return Vector of shadow prices for all constraints.
     *
     * @pre solve() must have been called and returned Solved.
     */
    [[nodiscard]] std::vector<T> get_all_shadow_prices() const
    {
      ah_logic_error_if(state != Solved) << "Must solve before computing shadow prices";
      std::vector<T> prices(num_rest);
      for (size_t i = 0; i < num_rest; ++i)
        prices[i] = shadow_price(i);
      return prices;
    }

    /** @brief Gets all reduced costs as a vector.
     *
     * @return Vector of reduced costs for all variables.
     *
     * @pre solve() must have been called and returned Solved.
     */
    [[nodiscard]] std::vector<T> get_all_reduced_costs() const
    {
      ah_logic_error_if(state != Solved) << "Must solve before computing reduced costs";
      std::vector<T> costs(num_var);
      for (size_t i = 0; i < num_var; ++i)
        costs[i] = reduced_cost(i);
      return costs;
    }

    // ================== Dual Simplex ==================

    /** @brief Performs dual simplex iteration.
     *
     * Used for re-optimization when the RHS values change. The current
     * basis must be optimal for the objective (no cost of row 0 below
     * zero), as after solve(); each iteration takes out of the basis a
     * variable whose value is negative. An artificial variable never
     * enters the basis, and one that stays basic, of a constraint that is
     * a combination of others, must stay at zero.
     *
     * @return New solution state after re-optimization: `Solved`, or
     *         `Unfeasible` if no point satisfies the constraints.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::logic_error If the problem is not solved.
     */
    [[nodiscard]] State dual_solve()
    {
      ah_logic_error_if(state != Solved)
        << "dual_solve() requires a previously solved problem";

      state = Solving;

      const auto start_time = std::chrono::high_resolution_clock::now();
      stats.iterations = 0;
      stats.pivots = 0;

      const size_t rhs = rhs_col();
      const size_t M = first_artificial();
      degenerate_run = 0;

      while (state == Solving)
        {
          ++stats.iterations;
          const bool bland = bland_now();

          // Select leaving variable: a row with a negative RHS, the most
          // negative one, or with Bland's rule the one whose basic
          // variable has the least index
          int leaving_row = -1;
          T min_rhs = -tol_rhs;
          for (size_t i = 1; i <= num_rest; ++i)
            {
              const T b = m->read(i, rhs);
              if (b >= -tol_rhs)
                continue;

              if (leaving_row == -1
                  or (bland ? basic_vars[i - 1] < basic_vars[leaving_row - 1] : b < min_rhs))
                {
                  min_rhs = b;
                  leaving_row = static_cast<int>(i);
                }
            }

          if (leaving_row == -1)
            break;

          // Select entering variable: a negative entry of the row, not
          // artificial, with the least ratio of its cost to its magnitude;
          // among ties, the least index with Bland's rule and the largest
          // magnitude else
          int entering_col = -1;
          T min_ratio = T{0};
          T max_mag = T{0};
          for (size_t j = 0; j < M; ++j)
            {
              const T a = m->read(leaving_row, j);
              if (a >= -tol_coef)
                continue; // Must be negative

              const T ratio = std::max(m->read(0, j), T{0}) / -a;
              if (entering_col == -1 or (ratio < min_ratio and not same_ratio(ratio, min_ratio)))
                {
                  entering_col = static_cast<int>(j);
                  min_ratio = ratio;
                  max_mag = -a;
                }
              else if (same_ratio(ratio, min_ratio) and not bland and -a > max_mag)
                {
                  entering_col = static_cast<int>(j);
                  max_mag = -a;
                }
            }

          if (entering_col == -1)
            {
              state = Unfeasible;
              break;
            }

          if (bland)
            stats.bland_rule_used = true;

          // A degenerate pivot leaves the objective as it was
          if (m->read(0, entering_col) <= tol_cost)
            {
              ++stats.degenerate_pivots;
              ++degenerate_run;
            }
          else
            degenerate_run = 0;

          ++stats.pivots;
          to_pivot(static_cast<size_t>(leaving_row), static_cast<size_t>(entering_col));
        }

      if (state == Solving)
        {
          state = Solved;
          for (size_t i = 1; i <= num_rest; ++i)
            if (is_artificial(static_cast<size_t>(basic_vars[i - 1]))
                and m->read(i, rhs) > tol_rhs)
              state = Unfeasible;
        }

      const auto end_time = std::chrono::high_resolution_clock::now();
      stats.elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

      return state;
    }

    /** @brief Updates RHS value and re-optimizes using dual simplex.
     *
     * Efficiently re-solves the problem when only RHS values change: the
     * right-hand sides of the tableau change by the column of the inverse
     * of the basis of the constraint, and dual_solve() restores their
     * feasibility.
     *
     * @param[in] rest_idx Constraint index to update.
     * @param[in] new_rhs New right-hand side value.
     * @return Solution state after re-optimization.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If rest_idx is out of range.
     * @throw std::logic_error If the problem is not solved.
     */
    [[nodiscard]] State update_rhs_and_reoptimize(size_t rest_idx, T new_rhs)
    {
      verify_rest_index(rest_idx);
      ah_logic_error_if(state != Solved)
        << "Cannot update RHS without a solved problem";

      T *rest = restriction(rest_idx);
      const T delta = T(row_sign[rest_idx]) * row_scale[rest_idx] * (new_rhs - rest[num_var]);
      rest[num_var] = new_rhs;

      const size_t rhs = rhs_col();
      const size_t col = unit_col[rest_idx];
      for (size_t row = 0; row <= num_rest; ++row)
        m->write(row, rhs, m->read(row, rhs) + m->read(row, col) * delta);

      return dual_solve();
    }

    /** @brief Prints a summary of statistics to stdout.
     */
    void print_stats() const
    {
      std::cout << "=== Simplex Statistics ===\n"
                << "Iterations: " << stats.iterations << "\n"
                << "Pivots: " << stats.pivots << "\n"
                << "Degenerate pivots: " << stats.degenerate_pivots << "\n"
                << "Elapsed time: " << stats.elapsed_ms << " ms\n"
                << "Bland's rule used: " << (stats.bland_rule_used ? "yes" : "no") << "\n";
    }

    /** @brief Prints sensitivity analysis to stdout.
     *
     * @pre solve() must have been called and returned Solved.
     */
    void print_sensitivity_analysis() const
    {
      ah_logic_error_if(state != Solved)
        << "Must solve before printing sensitivity analysis";

      std::cout << "=== Sensitivity Analysis ===\n\n";

      std::cout << "Objective Coefficients:\n";
      for (size_t i = 0; i < num_var; ++i)
        {
          auto range = objective_sensitivity(i);
          std::cout << "  x_" << i << ": current=" << range.current_value
                    << ", range=[" << range.lower_bound << ", "
                    << range.upper_bound << "]\n";
        }

      std::cout << "\nRHS Values (Shadow Prices):\n";
      for (size_t i = 0; i < num_rest; ++i)
        {
          auto range = rhs_sensitivity(i);
          T sp = shadow_price(i);
          std::cout << "  Constraint " << i << ": current=" << range.current_value
                    << ", range=[" << range.lower_bound << ", " << range.upper_bound
                    << "], shadow price=" << sp << "\n";
        }

      std::cout << "\nReduced Costs:\n";
      for (size_t i = 0; i < num_var; ++i)
        {
          T rc = reduced_cost(i);
          bool basic = is_basic_variable(i);
          std::cout << "  x_" << i << ": " << rc
                    << (basic ? " (basic)" : " (non-basic)") << "\n";
        }
    }
  };


  // ============================================================================
  // REVISED SIMPLEX
  // ============================================================================

  /** @brief Revised Simplex algorithm for linear programming.
   *
   * Maximizes c·x subject to A·x ≤ b and x ≥ 0, where the entries of b
   * may be negative. The Revised Simplex method is a variant of the
   * standard Simplex algorithm that maintains only the basis inverse B⁻¹
   * instead of the full tableau, which saves memory and work when there
   * are many more variables than constraints.
   *
   * ## Key Differences from Standard Simplex
   *
   * | Aspect | Standard Simplex | Revised Simplex |
   * |--------|------------------|-----------------|
   * | Storage | Full tableau O((m+1)×(n+m+1)) | B⁻¹ + original data O(m² + mn) |
   * | Pivot | Update entire tableau | Update only B⁻¹ |
   * | Pricing | Read from tableau | Compute π = c_B·B⁻¹, then c̄_j = c_j - π·a_j |
   * | Best for | Small/dense problems | Large/sparse problems |
   *
   * ## Algorithm Overview
   *
   * 1. **Initialize**: the basis has the slack variable of each row whose
   *    b_i is >= 0, and an artificial variable, of column -e_i, of each
   *    other row
   * 2. **Phase I**, if there are artificial variables: maximize minus
   *    their sum; if it stays below zero, the problem is infeasible. The
   *    artificial variables that are still basic, at zero, leave the
   *    basis, but for those of rows that are combinations of others, and
   *    an artificial variable never enters it again
   * 3. **Phase II**, from that basis, on the objective; each phase
   *    iterates:
   *    - **Pricing**: compute dual prices π = c_B · B⁻¹
   *    - **Select entering**: the j with the most positive
   *      c̄_j = c_j - π · a_j (with Bland's rule, the first one)
   *    - **FTRAN**: compute d = B⁻¹ · a_j (tableau column)
   *    - **Ratio test**: find the leaving variable using d
   *    - **Update**: B⁻¹ by the pivot on d, in place
   *
   * Every refactorization-frequency pivots, B⁻¹ is computed again from
   * the basis by Gauss-Jordan elimination, and so are the values of the
   * basic variables, so that the rounding of the updates does not build
   * up. As Simplex, it works on the program with each row, each column and
   * the objective multiplied by a power of two, so that its tolerances do
   * not depend on the units of the data, and scales the solution back. A
   * run of degenerate pivots switches to Bland's rule until a pivot
   * improves the objective, so that the method always ends.
   *
   * ## Performance
   *
   * - **Memory**: O(m² + mn) vs O(mn + m²) for standard
   * - **Time per iteration**: O(m² + mn), and O(m³) for a refactorization
   * - **Best for**: n >> m (many variables, few constraints)
   *
   * @tparam T Numeric type for coefficients (typically double).
   *
   * @see Simplex
   * @ingroup Networks
   */
  template <typename T>
  class RevisedSimplex
  {
  public:
    /** @brief Solution states for the linear program. */
    enum State { Not_Solved, Solving, Unbounded, Solved, Unfeasible };

  private:
    // Original problem data
    size_t m;  // Number of constraints
    size_t n;  // Number of original variables
    std::vector<T> c;           // Objective coefficients (n)
    std::vector<std::vector<T>> A;  // Constraint matrix (m × n)
    std::vector<T> b;           // RHS values (m)

    // The scaled program that solve() works on: each row, each column and
    // the objective multiplied by a power of two, so that the largest
    // magnitude of each is in [1/2, 1), and the tolerances do not depend
    // on the units of the data
    std::vector<std::vector<T>> As;
    std::vector<T> bs;
    std::vector<T> cs;
    std::vector<T> row_scale;
    std::vector<T> col_scale;

    // The columns: the n original variables, the m slack variables, of
    // columns e_i, and the artificial variables, of columns -e_i for the
    // rows art_row
    std::vector<size_t> art_row;

    // Basis information
    std::vector<int> basic;     // basic[i] = index of basic variable in row i
    std::vector<bool> is_basic; // is_basic[j] = true if variable j is in basis

    std::vector<std::vector<T>> B_inv_explicit;  // Explicit B⁻¹

    // Current solution
    std::vector<T> x_B;         // Basic variable values (m)
    std::vector<T> solution;    // Full solution (n + m)

    // Working vectors (reused across iterations)
    mutable std::vector<T> pi;      // Dual prices (m)
    mutable std::vector<T> d;       // FTRAN result / tableau column (m)
    mutable std::vector<T> work;    // A column of the problem (m)
    std::vector<T> cost;            // The costs of the current phase

    // Configuration
    T eps;
    size_t refactor_frequency;  // Refactorize B⁻¹ every N pivots
    size_t refactorizations = 0;
    size_t since_refactor = 0;

    // Tolerances: eps times the largest magnitude of the entries of the
    // scaled As, bs and cs, at least 1 each
    T tol_coef = T{0};
    T tol_rhs = T{0};
    T tol_cost = T{0};

    // The degenerate pivots in a row; past n + m, Bland's rule
    size_t degenerate_run = 0;

    // State and statistics
    State state;
    mutable SimplexStats stats;

    size_t num_columns() const noexcept { return n + m + art_row.size(); }

    bool is_artificial(const size_t j) const noexcept { return j >= n + m; }

    // ============ The column j of the problem ============

    void get_column(size_t j, std::vector<T>& col) const
    {
      if (j < n)
        for (size_t i = 0; i < m; ++i)
          col[i] = As[i][j];
      else if (j < n + m)
        for (size_t i = 0; i < m; ++i)
          col[i] = i == j - n ? T{1} : T{0};
      else
        for (size_t i = 0; i < m; ++i)
          col[i] = i == art_row[j - n - m] ? T{-1} : T{0};
    }

    // ============ FTRAN: Solve B·y = a (forward transformation) ============
    // Computes y = B⁻¹·a

    void ftran(const std::vector<T>& a, std::vector<T>& y) const
    {
      for (size_t i = 0; i < m; ++i)
        {
          T sum = T{0};
          for (size_t j = 0; j < m; ++j)
            sum += B_inv_explicit[i][j] * a[j];
          y[i] = sum;
        }
    }

    // ============ BTRAN: Solve π·B = c_B (backward transformation) ============
    // Computes π = c_B · B⁻¹

    void btran(const std::vector<T>& c_B, std::vector<T>& pi_out) const
    {
      for (size_t j = 0; j < m; ++j)
        {
          T sum = T{0};
          for (size_t i = 0; i < m; ++i)
            sum += c_B[i] * B_inv_explicit[i][j];
          pi_out[j] = sum;
        }
    }

    // ============ Compute reduced cost for column j ============

    T compute_reduced_cost(size_t j) const
    {
      // c̄_j = c_j - π · a_j
      get_column(j, work);
      T rc = cost[j];
      for (size_t i = 0; i < m; ++i)
        rc -= pi[i] * work[i];
      return rc;
    }

    // Whether the next pivot follows Bland's rule: after a run of
    // degenerate pivots
    bool bland_now() const noexcept { return degenerate_run > n + m; }

    // ============ Pricing: Find entering variable ============

    // The column that enters the basis: the one of most positive reduced
    // cost, or with Bland's rule the first positive one; -1 if there is
    // none, which is optimal. An artificial variable never enters.
    int select_entering_variable(const T & tol)
    {
      std::vector<T> c_B(m);
      for (size_t i = 0; i < m; ++i)
        c_B[i] = cost[basic[i]];
      btran(c_B, pi);

      const bool bland = bland_now();
      T max_rc = tol;
      int entering = -1;
      for (size_t j = 0; j < n + m; ++j)
        {
          if (is_basic[j])
            continue;

          if (const T rc = compute_reduced_cost(j); rc > max_rc)
            {
              entering = static_cast<int>(j);
              if (bland)
                break;
              max_rc = rc;
            }
        }

      return entering;
    }

    // ============ Ratio test: Find leaving variable ============

    // The row whose basic variable leaves the basis: the least ratio of its
    // value to its positive entry in d = B⁻¹·a; among ties, with Bland's
    // rule the row whose basic variable has the least index, else the
    // largest entry. -1 if no entry is positive, which is unbounded.
    int select_leaving_variable(int entering, T& theta)
    {
      get_column(static_cast<size_t>(entering), work);
      ftran(work, d);

      const bool bland = bland_now();
      int leaving = -1;
      theta = T{0};
      T max_den = T{0};
      for (size_t i = 0; i < m; ++i)
        {
          if (d[i] <= tol_coef)
            continue;

          const T ratio = std::max(x_B[i], T{0}) / d[i];
          const bool tie =
            leaving != -1 and std::abs(ratio - theta) <= eps * std::max(T{1}, std::abs(theta));
          if (leaving == -1 or (ratio < theta and not tie))
            {
              leaving = static_cast<int>(i);
              theta = ratio;
              max_den = d[i];
            }
          else if (tie and (bland ? basic[i] < basic[leaving] : d[i] > max_den))
            {
              leaving = static_cast<int>(i);
              max_den = d[i];
            }
        }

      if (leaving != -1)
        theta = std::max(x_B[leaving], T{0}) / d[leaving];

      return leaving;
    }

    // ============ Update basis inverse after pivot ============
    // The row r of B⁻¹ is divided by d_r, and d_i times it is subtracted
    // from each other row i, in place

    void update_basis_inverse(int leaving_row, const std::vector<T>& d_col)
    {
      const size_t r = static_cast<size_t>(leaving_row);
      const T inv_pivot = T{1} / d_col[r];
      for (size_t j = 0; j < m; ++j)
        B_inv_explicit[r][j] *= inv_pivot;

      for (size_t i = 0; i < m; ++i)
        {
          if (i == r or d_col[i] == T{0})
            continue;

          const T factor = d_col[i];
          for (size_t j = 0; j < m; ++j)
            B_inv_explicit[i][j] -= factor * B_inv_explicit[r][j];
        }
    }

    // The variable of column entering becomes the basic one of the row
    // leaving, at the value theta, with d = B⁻¹ times its column
    void pivot(int leaving, int entering, const T & theta)
    {
      const size_t r = static_cast<size_t>(leaving);
      for (size_t i = 0; i < m; ++i)
        x_B[i] -= theta * d[i];
      x_B[r] = theta;

      is_basic[basic[r]] = false;
      is_basic[entering] = true;
      basic[r] = entering;

      update_basis_inverse(leaving, d);

      ++stats.pivots;
      if (++since_refactor >= refactor_frequency)
        refactorize();
    }

    // ============ Refactorize B⁻¹ explicitly ============

    // B⁻¹ from the columns of the basis, by Gauss-Jordan elimination with
    // partial pivoting, and the values of the basic variables, B⁻¹·b
    void refactorize()
    {
      // Build explicit B from current basis
      std::vector<std::vector<T>> B(m, std::vector<T>(m, T{0}));
      for (size_t i = 0; i < m; ++i)
        {
          get_column(static_cast<size_t>(basic[i]), work);
          for (size_t row = 0; row < m; ++row)
            B[row][i] = work[row];
        }

      // Compute B⁻¹ using Gauss-Jordan elimination
      B_inv_explicit.assign(m, std::vector<T>(m, T{0}));
      for (size_t i = 0; i < m; ++i)
        B_inv_explicit[i][i] = T{1};

      for (size_t col = 0; col < m; ++col)
        {
          // Find pivot
          size_t pivot_row = col;
          T max_val = std::abs(B[col][col]);
          for (size_t row = col + 1; row < m; ++row)
            if (std::abs(B[row][col]) > max_val)
              {
                max_val = std::abs(B[row][col]);
                pivot_row = row;
              }

          // Swap rows if needed
          if (pivot_row != col)
            {
              std::swap(B[col], B[pivot_row]);
              std::swap(B_inv_explicit[col], B_inv_explicit[pivot_row]);
            }

          // Scale pivot row
          const T pivot = B[col][col];
          for (size_t j = 0; j < m; ++j)
            {
              B[col][j] /= pivot;
              B_inv_explicit[col][j] /= pivot;
            }

          // Eliminate column
          for (size_t row = 0; row < m; ++row)
            {
              if (row == col)
                continue;

              const T factor = B[row][col];
              if (factor == T{0})
                continue;

              for (size_t j = 0; j < m; ++j)
                {
                  B[row][j] -= factor * B[col][j];
                  B_inv_explicit[row][j] -= factor * B_inv_explicit[col][j];
                }
            }
        }

      ftran(bs, x_B);
      ++refactorizations;
      since_refactor = 0;
    }

    // Pivots until no reduced cost is above tol, which is optimal, or the
    // entering column has no positive entry in B⁻¹, which is unbounded
    State iterate(const T & tol)
    {
      degenerate_run = 0;
      while (true)
        {
          ++stats.iterations;
          const int entering = select_entering_variable(tol);
          if (entering < 0)
            return Solved;

          T theta;
          const int leaving = select_leaving_variable(entering, theta);
          if (leaving < 0)
            return Unbounded;

          if (bland_now())
            stats.bland_rule_used = true;

          // A degenerate pivot leaves the vertex, and the objective, as
          // they were
          if (x_B[leaving] <= tol_rhs)
            {
              ++stats.degenerate_pivots;
              ++degenerate_run;
            }
          else
            degenerate_run = 0;

          pivot(leaving, entering, theta);
        }
    }

    // After the first phase, the artificial variables that are still basic,
    // at zero, leave the basis by a pivot on the largest entry of their row
    // of B⁻¹·a_j, for a column j that is not artificial. A row without such
    // an entry is a combination of others: its artificial variable stays
    // basic, at zero.
    void drive_out_artificials()
    {
      for (size_t r = 0; r < m; ++r)
        {
          if (not is_artificial(static_cast<size_t>(basic[r])))
            continue;

          x_B[r] = T{0}; // at most a rounding above zero

          int entering = -1;
          T largest = tol_coef;
          for (size_t j = 0; j < n + m; ++j)
            {
              if (is_basic[j])
                continue;

              get_column(j, work);
              T entry = T{0};
              for (size_t k = 0; k < m; ++k)
                entry += B_inv_explicit[r][k] * work[k];
              if (std::abs(entry) > largest)
                {
                  largest = std::abs(entry);
                  entering = static_cast<int>(j);
                }
            }

          if (entering == -1)
            continue;

          get_column(static_cast<size_t>(entering), work);
          ftran(work, d);
          ++stats.degenerate_pivots;
          pivot(static_cast<int>(r), entering, T{0});
        }
    }

    // The power of two that takes magnitude into [1/2, 1); 1 for 0, for a
    // subnormal magnitude, whose power could overflow, or for a magnitude
    // that is not finite
    static T scale_of(const T & magnitude) noexcept
    {
      if (not (magnitude >= std::numeric_limits<T>::min()) or not std::isfinite(magnitude))
        return T{1};
      int e;
      (void) std::frexp(magnitude, &e);
      return std::ldexp(T{1}, -e);
    }

    // The scaled program: each row by its largest coefficient, each column
    // by its largest coefficient once the rows are scaled, and the
    // objective by its largest coefficient once the columns are
    void scale()
    {
      row_scale.assign(m, T{1});
      col_scale.assign(n, T{0});
      for (size_t i = 0; i < m; ++i)
        {
          T largest = T{0};
          for (size_t j = 0; j < n; ++j)
            largest = std::max(largest, std::abs(A[i][j]));
          row_scale[i] = scale_of(largest);
        }
      for (size_t i = 0; i < m; ++i)
        for (size_t j = 0; j < n; ++j)
          col_scale[j] = std::max(col_scale[j], std::abs(A[i][j]) * row_scale[i]);
      T largest = T{0};
      for (size_t j = 0; j < n; ++j)
        {
          col_scale[j] = scale_of(col_scale[j]);
          largest = std::max(largest, std::abs(c[j]) * col_scale[j]);
        }
      const T obj_scale = scale_of(largest);

      As.assign(m, std::vector<T>(n));
      bs.resize(m);
      cs.resize(n);
      for (size_t i = 0; i < m; ++i)
        {
          for (size_t j = 0; j < n; ++j)
            As[i][j] = A[i][j] * row_scale[i] * col_scale[j];
          bs[i] = b[i] * row_scale[i];
        }
      for (size_t j = 0; j < n; ++j)
        cs[j] = c[j] * col_scale[j] * obj_scale;
    }

  public:
    /** @brief Constructs a Revised Simplex solver.
     *
     * @param[in] num_vars Number of decision variables.
     * @param[in] num_constraints Number of constraints.
     *
     * @throw std::invalid_argument If num_vars or num_constraints is zero.
     */
    RevisedSimplex(size_t num_vars, size_t num_constraints)
      : m(num_constraints), n(num_vars),
        c(num_vars, T{0}),
        A(num_constraints, std::vector<T>(num_vars, T{0})),
        b(num_constraints, T{0}),
        basic(num_constraints),
        x_B(num_constraints, T{0}),
        solution(num_vars + num_constraints, T{0}),
        pi(num_constraints), d(num_constraints), work(num_constraints),
        eps(std::numeric_limits<T>::epsilon() * 1000),
        refactor_frequency(std::max(size_t{50}, num_constraints)),
        state(Not_Solved)
    {
      ah_invalid_argument_if(num_vars == 0 || num_constraints == 0)
        << "Number of variables and constraints must be > 0";
    }

    /** @brief Sets objective coefficient.
     *
     * @param[in] j Variable index.
     * @param[in] coef Coefficient value.
     *
     * @throw std::out_of_range If j is out of range.
     */
    void set_objective(size_t j, T coef)
    {
      ah_out_of_range_error_if(j >= n) << "Variable index out of range";
      c[j] = coef;
    }

    /** @brief Sets all objective coefficients.
     *
     * @param[in] coefs Array of n coefficients.
     */
    void set_objective(const T* coefs)
    {
      for (size_t j = 0; j < n; ++j)
        c[j] = coefs[j];
    }

    /** @brief Sets constraint coefficient.
     *
     * @param[in] i Constraint index.
     * @param[in] j Variable index.
     * @param[in] coef Coefficient value.
     *
     * @throw std::out_of_range If i or j is out of range.
     */
    void set_constraint(size_t i, size_t j, T coef)
    {
      ah_out_of_range_error_if(i >= m || j >= n)
        << "Constraint or variable index out of range";
      A[i][j] = coef;
    }

    /** @brief Sets RHS value for a constraint.
     *
     * @param[in] i Constraint index.
     * @param[in] rhs Right-hand side value, which may be negative.
     *
     * @throw std::out_of_range If i is out of range.
     */
    void set_rhs(size_t i, T rhs)
    {
      ah_out_of_range_error_if(i >= m) << "Constraint index out of range";
      b[i] = rhs;
    }

    /** @brief Adds a complete constraint.
     *
     * @param[in] i Constraint index.
     * @param[in] coefs Array of n coefficients.
     * @param[in] rhs Right-hand side value, which may be negative.
     *
     * @throw std::out_of_range If i is out of range.
     */
    void set_constraint_row(size_t i, const T* coefs, T rhs)
    {
      ah_out_of_range_error_if(i >= m) << "Constraint index out of range";
      for (size_t j = 0; j < n; ++j)
        A[i][j] = coefs[j];
      b[i] = rhs;
    }

    /** @brief Sets refactorization frequency.
     *
     * B⁻¹ is recomputed explicitly every N pivots to maintain
     * numerical stability.
     *
     * @param[in] freq Refactorization frequency (default: max(50, m)).
     *
     * @throw std::invalid_argument If freq is zero.
     */
    void set_refactorization_frequency(size_t freq)
    {
      ah_invalid_argument_if(freq == 0) << "Refactorization frequency must be > 0";
      refactor_frequency = freq;
    }

    /** @brief Solves the linear program.
     *
     * @return Solution state (Solved, Unbounded, or Unfeasible).
     *
     * @throw std::logic_error If solve() has already been called.
     */
    [[nodiscard]] State solve()
    {
      ah_logic_error_if(state != Not_Solved) << "Already solved";

      stats.reset();
      auto start_time = std::chrono::high_resolution_clock::now();

      scale();
      T max_coef = T{0}, max_rhs = T{0}, max_cost = T{0};
      for (size_t i = 0; i < m; ++i)
        {
          for (size_t j = 0; j < n; ++j)
            max_coef = std::max(max_coef, std::abs(As[i][j]));
          max_rhs = std::max(max_rhs, std::abs(bs[i]));
        }
      for (size_t j = 0; j < n; ++j)
        max_cost = std::max(max_cost, std::abs(cs[j]));
      tol_coef = eps * std::max(T{1}, max_coef);
      tol_rhs = eps * std::max(T{1}, max_rhs);
      tol_cost = eps * std::max(T{1}, max_cost);

      // The initial basis: the slack variable of each row whose b_i is >=
      // 0, and an artificial variable of each other row, at -b_i; B⁻¹ is
      // diagonal, of 1 and -1
      art_row.clear();
      B_inv_explicit.assign(m, std::vector<T>(m, T{0}));
      for (size_t i = 0; i < m; ++i)
        if (bs[i] < T{0})
          {
            basic[i] = static_cast<int>(n + m + art_row.size());
            art_row.push_back(i);
            B_inv_explicit[i][i] = T{-1};
            x_B[i] = -bs[i];
          }
        else
          {
            basic[i] = static_cast<int>(n + i);
            B_inv_explicit[i][i] = T{1};
            x_B[i] = bs[i];
          }

      is_basic.assign(num_columns(), false);
      for (size_t i = 0; i < m; ++i)
        is_basic[basic[i]] = true;
      cost.assign(num_columns(), T{0});
      refactorizations = since_refactor = 0;

      state = Solving;

      if (not art_row.empty())
        {
          // Phase I: maximize minus the sum of the artificial variables
          for (size_t j = n + m; j < num_columns(); ++j)
            cost[j] = T{-1};
          const T scale = T(art_row.size());
          (void) iterate(tol_coef * scale); // bounded above by zero

          T sum = T{0};
          for (size_t i = 0; i < m; ++i)
            if (is_artificial(static_cast<size_t>(basic[i])))
              sum += x_B[i];

          if (sum > tol_rhs * scale)
            state = Unfeasible;
          else
            drive_out_artificials();
        }

      if (state == Solving)
        {
          // Phase II, on the objective
          for (size_t j = 0; j < num_columns(); ++j)
            cost[j] = j < n ? cs[j] : T{0};
          state = iterate(tol_cost);
        }

      // Load final solution, scaled back: a variable by its column, a
      // slack by its row
      std::fill(solution.begin(), solution.end(), T{0});
      for (size_t i = 0; i < m; ++i)
        if (const auto j = static_cast<size_t>(basic[i]); not is_artificial(j))
          solution[j] = j < n ? x_B[i] * col_scale[j] : x_B[i] / row_scale[j - n];

      auto end_time = std::chrono::high_resolution_clock::now();
      stats.elapsed_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();

      return state;
    }

    /** @brief Gets solution value for variable j.
     *
     * @param[in] j Variable index.
     * @return Optimal value of variable x_j.
     *
     * @throw std::out_of_range If j is out of range.
     */
    [[nodiscard]] T get_solution(size_t j) const
    {
      ah_out_of_range_error_if(j >= n) << "Variable index out of range";
      return solution[j];
    }

    /** @brief Computes objective function value.
     *
     * @return Optimal objective value.
     */
    [[nodiscard]] T objective_value() const
    {
      T sum = T{0};
      for (size_t j = 0; j < n; ++j)
        sum += c[j] * solution[j];
      return sum;
    }

    /** @brief Gets current state. */
    [[nodiscard]] State get_state() const noexcept { return state; }

    /** @brief Gets execution statistics. */
    [[nodiscard]] const SimplexStats& get_stats() const noexcept { return stats; }

    /** @brief Gets number of variables. */
    [[nodiscard]] size_t get_num_vars() const noexcept { return n; }

    /** @brief Gets number of constraints. */
    [[nodiscard]] size_t get_num_constraints() const noexcept { return m; }

    /** @brief Verifies solution satisfies all constraints.
     *
     * Checks that every variable is >= 0 and that every constraint holds,
     * up to a tolerance relative to the magnitude of its terms: 10 times
     * the tolerance of the solver.
     *
     * @return true if feasible, false otherwise.
     */
    [[nodiscard]] bool verify_solution() const
    {
      const T tol = T(10) * eps;
      for (size_t j = 0; j < n; ++j)
        if (solution[j] < -tol * std::max(T{1}, std::abs(solution[j])))
          return false;

      for (size_t i = 0; i < m; ++i)
        {
          T lhs = T{0};
          T size = std::abs(b[i]);
          for (size_t j = 0; j < n; ++j)
            {
              lhs += A[i][j] * solution[j];
              size += std::abs(A[i][j] * solution[j]);
            }
          if (lhs > b[i] + tol * std::max(T{1}, size))
            return false;
        }
      return true;
    }

    /** @brief Prints statistics to stdout. */
    void print_stats() const
    {
      std::cout << "=== Revised Simplex Statistics ===\n"
                << "Variables: " << n << ", Constraints: " << m << "\n"
                << "Iterations: " << stats.iterations << "\n"
                << "Pivots: " << stats.pivots << "\n"
                << "Elapsed time: " << stats.elapsed_ms << " ms\n"
                << "Refactorizations: " << refactorizations << "\n";
    }
  };

} // end namespace Aleph

# endif // SIMPLEX_H
