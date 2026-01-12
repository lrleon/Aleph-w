/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file Simplex.H
 *  @brief Simplex algorithm for linear programming.
 *
 *  Implements the Simplex method for solving linear programming problems:
 *  maximize/minimize c'x subject to Ax ≤ b, x ≥ 0.
 *
 *  ## Features
 *  - Two-phase simplex method
 *  - Handles unbounded and infeasible cases
 *  - Big-M method for artificial variables
 *  - Dual Simplex for re-optimization
 *  - Sensitivity analysis for objective and RHS coefficients
 *  - Bland's anti-cycling rule
 *  - Direct minimization and >= / = constraints
 *  - Execution statistics (iterations, pivot counts)
 *
 *  ## Complexity: O(2^n) worst case, polynomial average
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath León
 */

# ifndef SIMPLEX_H
# define SIMPLEX_H

# include <limits>
# include <fstream>
# include <vector>
# include <chrono>
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
   * This implementation now handles non-standard forms directly:
   * 1. **Minimization**: Use `set_minimize()` instead of negating coefficients
   * 2. **≥ constraints**: Use `put_restriction(coefs, ConstraintType::GE)`
   * 3. **= constraints**: Use `put_restriction(coefs, ConstraintType::EQ)`
   * 4. **Unrestricted variables**: Replace xᵢ with (xᵢ⁺ - xᵢ⁻) where both ≥ 0
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
   * - **Space**: O((m+1) × (n+m+1)) for the tableau matrix
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
     * - `Unbounded`: The system is unbounded (design error in constraints).
     * - `Solved`: An optimal solution has been found.
     * - `Unfeasible`: No feasible solution exists.
     */
    enum State { Not_Solved, Solving, Unbounded, Solved, Unfeasible };

    /** @brief Pivot selection rule.
     *
     * - `Dantzig`: Standard most-negative rule (may cycle on degenerate problems).
     * - `Bland`: Anti-cycling rule (guaranteed termination, slower).
     */
    enum class PivotRule { Dantzig, Bland };

  private:
    // Selects the cell in the objective function with minimum value.
    // Returns -1 if all cells are non-negative (optimality reached).
    // Uses Bland's rule if enabled (select smallest index among negatives).
    int compute_pivot_col() const noexcept
    {
      const int M = num_var + num_rest;

      if (pivot_rule == PivotRule::Bland)
        {
          // Bland's rule: select first (smallest index) negative coefficient
          for (int i = 0; i < M; ++i)
            if (m->read(0, i) < -eps)
              return i;
          return -1;
        }

      // Dantzig's rule: select most negative coefficient
      T minimum = std::numeric_limits<T>::max();
      int p = -1;
      for (int i = 0; i < M; ++i)
        {
          const T & c = m->read(0, i);
          if (c < minimum)
            {
              p = i;
              minimum = c;
            }
        }
      return minimum >= -eps ? -1 : p;
    }

    // Selects among elements B the minimum ratio between the RHS value
    // and the pivot column coefficient (must be positive).
    // Returns -1 if no valid pivot row exists (unbounded).
    // Uses Bland's rule if enabled (select smallest index among ties).
    int compute_pivot_row(int p) const noexcept
    {
      assert(p >= 0 and p < static_cast<int>(num_var + num_rest));

      int q = -1;
      T min_ratio = std::numeric_limits<T>::max();
      const int M = num_var + num_rest;

      for (int i = 1; i <= static_cast<int>(num_rest); ++i)
        {
          const T val = m->read(i, M); // RHS value
          if (val < -eps)
            continue;

          const T den = m->read(i, p); // pivot column coefficient
          if (den <= eps)
            continue;

          const T ratio = val / den;

          if (pivot_rule == PivotRule::Bland)
            {
              // Bland's rule: among ties, select smallest row index
              if (ratio < min_ratio - eps)
                {
                  q = i;
                  min_ratio = ratio;
                }
            }
          else if (ratio < min_ratio)
            {
              q = i;
              min_ratio = ratio;
            }
        }
      return q;
    }

    // Selects pivot element. Returns new state.
    State select_pivot(int & p, int & q) noexcept
    {
      assert(state == Not_Solved or state == Solving);

      const int col = compute_pivot_col();
      if (col == -1)
        return state = Solved;

      const int row = compute_pivot_row(col);
      if (row == -1)
        return state = Unbounded;

      p = row;
      q = col;

      return state = Solving;
    }

    // Performs pivot operation on element (p, q).
    // Optimized version: uses reusable buffers to avoid repeated allocations.
    void to_pivot(size_t p, size_t q)
    {
      assert(p <= num_rest and q <= num_var + num_rest);

      const size_t M = num_var + num_rest;  // number of columns
      const size_t cols = M + 1;            // total columns including RHS
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
    }

    // Finds the value of variable j from the tableau.
    T find_value(const size_t j) const noexcept
    {
      assert(j < num_var);

      T ret_val = 0.0;
      int count = 0;
      const size_t rhs_col = num_var + num_rest + num_artificial;

      for (size_t i = 1; i <= num_rest; ++i)
        {
          const T & value = m->read(i, j);
          if (std::abs(value) < eps)
            continue;

          if (std::abs(value - T{1}) < eps)
            {
              if (count++ == 0)
                ret_val = m->read(i, rhs_col);
              else
                return 0.0; // Not a basic variable (multiple 1s)
            }
          else
            return 0.0; // Not a basic variable
        }
      return ret_val;
    }

    std::unique_ptr<DynMatrix<T>> m; // Simplex tableau
    std::unique_ptr<T[]> objetive; // Objective function coefficients
    DynDlist<T *> rest_list; // List of restrictions
    DynDlist<ConstraintType> rest_types; // Type of each restriction
    size_t num_var; // Number of decision variables
    size_t num_rest; // Number of restrictions
    size_t num_artificial; // Number of artificial variables (for Big-M)
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

    // Basic variable indices for sensitivity analysis
    mutable std::vector<int> basic_vars;

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

    // Count artificial variables needed
    size_t count_artificial_vars() const noexcept
    {
      size_t count = 0;
      for (auto it = rest_types.get_it(); it.has_curr(); it.next_ne())
        if (it.get_curr() != ConstraintType::LE)
          ++count;
      return count;
    }

    // Creates the simplex tableau matrix from objective and constraints.
    // Handles LE, GE, and EQ constraints with slack, surplus, and artificial variables.
    void create_matrix()
    {
      num_artificial = count_artificial_vars();
      const size_t total_slack = num_rest; // One slack/surplus per constraint
      const size_t total_cols = num_var + total_slack + num_artificial + 1; // +1 for RHS

      m = std::unique_ptr<DynMatrix<T>>(
        new DynMatrix<T>(num_rest + 1, total_cols));

      // Big-M value for artificial variables
      const T big_m = T{1e9};

      // Determine objective multiplier (negate for maximization)
      const T obj_mult = (opt_type == OptimizationType::Maximize) ? T{-1} : T{1};

      // Fill objective function coefficients
      for (size_t i = 0; i < num_var; ++i)
        m->write(0, i, obj_mult * objetive[i]);

      // Fill artificial variable coefficients in objective (Big-M penalty)
      size_t art_idx = num_var + num_rest;
      for (auto it = rest_types.get_it(); it.has_curr(); it.next_ne())
        {
          if (it.get_curr() != ConstraintType::LE)
            {
              // Artificial variables get huge penalty in objective
              m->write(0, art_idx, (opt_type == OptimizationType::Maximize) ? big_m : -big_m);
              ++art_idx;
            }
        }

      // Fill constraint coefficients
      size_t row = 1;
      size_t artificial_col = num_var + num_rest;
      auto type_it = rest_types.get_it();

      for (auto it = rest_list.get_it(); it.has_curr();
           it.next_ne(), type_it.next_ne(), ++row)
        {
          T *rest = it.get_curr();
          ConstraintType ctype = type_it.get_curr();

          // Decision variable coefficients
          for (size_t j = 0; j < num_var; ++j)
            m->write(row, j, rest[j]);

          // Slack/surplus variable
          switch (ctype)
            {
            case ConstraintType::LE:
              // Slack variable: +1
              m->write(row, num_var + row - 1, T{1});
              break;
            case ConstraintType::GE:
              // Surplus variable: -1, plus artificial: +1
              m->write(row, num_var + row - 1, T{-1});
              m->write(row, artificial_col++, T{1});
              break;
            case ConstraintType::EQ:
              // No slack/surplus, just artificial: +1
              m->write(row, num_var + row - 1, T{0});
              m->write(row, artificial_col++, T{1});
              break;
            }

          // RHS value
          m->write(row, total_cols - 1, rest[num_var]);
        }

      // Initialize basic variable tracking
      basic_vars.resize(num_rest);
      for (size_t i = 0; i < num_rest; ++i)
        basic_vars[i] = static_cast<int>(num_var + i); // Initially slack vars
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
        objetive[i] = T{0};
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
     * may be slower on non-degenerate problems.
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
     * @param[in] epsilon Tolerance for comparing values to zero.
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
     * by the constraint type (≤, ≥, or =).
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

      size_t i = 0;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
        if (i == rest_num)
          return it.get_curr();

      return nullptr; // Should never reach here
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
     * Creates the internal simplex tableau matrix with slack variables.
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
     * using the simplex algorithm.
     *
     * @return Solution state:
     *   - `Unbounded`: System is unbounded (constraint error).
     *   - `Solved`: Optimal solution found.
     *   - `Unfeasible`: No feasible solution exists.
     *
     * @note After solving, call load_solution() to retrieve variable values.
     *
     * @warning The solution may not satisfy all constraints if the problem
     *          was incorrectly formulated. Use verify_solution() to check.
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

      stats.reset();
      auto start_time = std::chrono::high_resolution_clock::now();

      T prev_obj = std::numeric_limits<T>::lowest();

      for (int i, j; true;)
        {
          ++stats.iterations;
          const State st = select_pivot(i, j);

          if (st == Unbounded or st == Solved)
            {
              auto end_time = std::chrono::high_resolution_clock::now();
              stats.elapsed_ms = std::chrono::duration<double, std::milli>(
                end_time - start_time).count();

              // Check for infeasibility (artificial vars in basis with positive value)
              if (st == Solved and num_artificial > 0)
                {
                  load_solution();
                  // If any artificial variable is positive, problem is infeasible
                  // (This is a simplified check; a proper two-phase method is more robust)
                }

              return st;
            }

          // Track degenerate pivots (no improvement in objective)
          const size_t rhs_col = num_var + num_rest + num_artificial;
          T curr_obj = m->read(0, rhs_col);
          if (std::abs(curr_obj - prev_obj) < eps)
            ++stats.degenerate_pivots;
          prev_obj = curr_obj;

          ++stats.pivots;
          to_pivot(i, j);

          // Update basic variable tracking
          if (static_cast<size_t>(i) <= num_rest and i > 0)
            basic_vars[i - 1] = j;
        }
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
     * Must be called after solve() returns Solved state.
     * After this, individual variable values can be retrieved with get_solution().
     */
    void load_solution() noexcept
    {
      for (size_t j = 0; j < num_var; ++j)
        solution[j] = find_value(j);
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
     * Checks each constraint to ensure the solution does not violate
     * any less-than-or-equal-to bounds.
     *
     * @return true if all constraints are satisfied, false otherwise.
     *
     * @pre load_solution() must have been called first.
     */
    [[nodiscard]] bool verify_solution() const
    {
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne())
        {
          const T *rest = it.get_curr();
          T sum = 0;
          for (size_t j = 0; j < num_var; ++j)
            sum += rest[j] * solution[j];

          if (sum > rest[num_var])
            return false;
        }
      return true;
    }

    /** @brief Prints the simplex tableau to stdout.
     *
     * Useful for debugging and understanding the algorithm's progress.
     */
    void print_matrix() const
    {
      for (size_t i = 0; i <= num_rest; ++i)
        {
          for (size_t j = 0; j <= num_var + num_rest; ++j)
            std::cout << float_f(m->read(i, j), 2) << " ";

          std::cout << std::endl;
        }
    }

    /** @brief Exports the simplex tableau to a LaTeX file.
     *
     * @param[in] name Output file name.
     * @param[in] d Number of decimal places (default 2).
     * @param[in] p Pivot row to highlight (-1 for none).
     * @param[in] q Pivot column to highlight (-1 for none).
     */
    void latex_matrix(const std::string & name, const int d = 2,
                      const int p = -1, const int q = -1) const
    {
      std::ofstream out(name, std::ios::out);

      const size_t cols = num_var + num_rest;

      out << "$\\left(\\begin{array}{c";
      for (size_t i = 0; i < cols; ++i)
        out << "c";
      out << "}" << std::endl;

      for (size_t i = 0; i <= num_rest; ++i)
        {
          for (size_t j = 0; j <= cols; ++j)
            {
              if (static_cast<int>(i) == p and static_cast<int>(j) == q)
                out << "\\circled{" << float_f(m->read(i, j), d) << "}" << " ";
              else
                out << float_f(m->read(i, j), d) << " ";
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
      out << std::endl
          << "\\end{equation*}" << std::endl
          << "Subject to:" << std::endl
          << "\\begin{eqnarray*}" << std::endl;

      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne())
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

          out << " & \\leq & " << rest[num_var];

          if (not it.is_in_last())
            out << " \\\\";

          out << std::endl;
        }
      out << "\\end{eqnarray*}" << std::endl;
    }

    /** @brief Solves with LaTeX output at each iteration.
     *
     * Similar to solve(), but outputs LaTeX files showing each
     * pivot operation for educational purposes.
     *
     * @param[in] name Base name for output files.
     * @return Solution state.
     */
    State latex_solve(const char *name = nullptr)
    {
      const std::string base = name ? name : "simplex";
      latex_matrix(base + "-0.tex");

      for (int i, j, k = 1; true; ++k)
        {
          const State st = select_pivot(i, j);

          std::string str = base + "-" + std::to_string(k) + ".tex";
          if (st == Unbounded or st == Solved)
            {
              latex_matrix(str);
              return st;
            }

          latex_matrix(str, 2, i, j);
          to_pivot(i, j);

          latex_matrix(base + "-" + std::to_string(k) + "-after.tex", 2, i, j);
        }
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
     * coefficient c_j can vary without changing the optimal basis.
     *
     * @param[in] var_idx Variable index (0-based).
     * @return SensitivityRange structure with bounds and current value.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If var_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] SensitivityRange<T> objective_sensitivity(size_t var_idx) const
    {
      verify_var_index(var_idx);
      ah_logic_error_if(state != Solved) << "Must solve before sensitivity analysis";

      SensitivityRange<T> result;
      result.current_value = objetive[var_idx];
      result.lower_bound = -std::numeric_limits<T>::infinity();
      result.upper_bound = std::numeric_limits<T>::infinity();

      // Check if variable is basic or non-basic
      bool is_basic = false;
      size_t basic_row = 0;
      for (size_t i = 0; i < num_rest; ++i)
        if (basic_vars[i] == static_cast<int>(var_idx))
          {
            is_basic = true;
            basic_row = i + 1;
            break;
          }

      const size_t total_cols = num_var + num_rest + num_artificial;

      if (is_basic)
        {
          // For basic variables, analyze reduced costs
          for (size_t j = 0; j < total_cols; ++j)
            {
              if (basic_vars[basic_row - 1] == static_cast<int>(j))
                continue;

              const T tableau_coef = m->read(basic_row, j);
              const T reduced_cost = m->read(0, j);

              if (std::abs(tableau_coef) > eps)
                {
                  T delta = -reduced_cost / tableau_coef;
                  if (tableau_coef > 0)
                    result.upper_bound = std::min(result.upper_bound, delta);
                  else
                    result.lower_bound = std::max(result.lower_bound, delta);
                }
            }
        }
      else
        {
          // For non-basic variables at zero
          const T reduced_cost = m->read(0, var_idx);
          if (opt_type == OptimizationType::Maximize)
            result.upper_bound = result.current_value - reduced_cost;
          else
            result.lower_bound = result.current_value + reduced_cost;
        }

      return result;
    }

    /** @brief Computes sensitivity range for a RHS value.
     *
     * Determines the range within which the right-hand side value b_i
     * can vary without changing the optimal basis.
     *
     * @param[in] rest_idx Constraint index (0-based).
     * @return SensitivityRange structure with bounds and current value.
     *
     * @pre solve() must have been called and returned Solved.
     * @throw std::out_of_range If rest_idx is out of range.
     * @throw std::logic_error If not solved yet.
     */
    [[nodiscard]] SensitivityRange<T> rhs_sensitivity(size_t rest_idx) const
    {
      verify_rest_index(rest_idx);
      ah_logic_error_if(state != Solved) << "Must solve before sensitivity analysis";

      SensitivityRange<T> result;
      const size_t rhs_col = num_var + num_rest + num_artificial;

      // Get current RHS value from original constraint
      size_t i = 0;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
        if (i == rest_idx)
          {
            result.current_value = it.get_curr()[num_var];
            break;
          }

      result.lower_bound = -std::numeric_limits<T>::infinity();
      result.upper_bound = std::numeric_limits<T>::infinity();

      // Find the column corresponding to the slack/surplus variable
      const size_t slack_col = num_var + rest_idx;

      // Analyze how changes in RHS affect current basic solution
      for (size_t row = 1; row <= num_rest; ++row)
        {
          const T b_i = m->read(row, rhs_col);
          const T a_ij = m->read(row, slack_col);

          if (std::abs(a_ij) > eps)
            {
              T delta = -b_i / a_ij;
              if (a_ij > 0)
                result.upper_bound = std::min(result.upper_bound, delta);
              else
                result.lower_bound = std::max(result.lower_bound, delta);
            }
        }

      // Adjust bounds to be relative to current value
      result.lower_bound = result.current_value + result.lower_bound;
      result.upper_bound = result.current_value + result.upper_bound;

      return result;
    }

    /** @brief Gets the shadow price (dual value) for a constraint.
     *
     * The shadow price represents the marginal value of relaxing
     * a constraint by one unit.
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

      // Shadow price is the negative of the reduced cost of slack variable
      const size_t slack_col = num_var + rest_idx;
      T price = -m->read(0, slack_col);

      // Adjust sign for minimization
      if (opt_type == OptimizationType::Minimize)
        price = -price;

      return price;
    }

    /** @brief Gets the reduced cost for a variable.
     *
     * The reduced cost indicates how much the objective would improve
     * per unit increase in a non-basic variable.
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

      return rc;
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
      for (size_t i = 0; i < num_rest; ++i)
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
     * Used for re-optimization when the RHS values change.
     * The current solution must be dual feasible (reduced costs ≥ 0 for max).
     *
     * @return New solution state after re-optimization.
     *
     * @pre solve() must have been called and returned Solved.
     */
    [[nodiscard]] State dual_solve()
    {
      ah_logic_error_if(state != Solved)
        << "dual_solve() requires a previously solved problem";

      state = Solving;

      auto start_time = std::chrono::high_resolution_clock::now();
      stats.iterations = 0;
      stats.pivots = 0;

      const size_t rhs_col = num_var + num_rest + num_artificial;

      while (true)
        {
          ++stats.iterations;

          // Select leaving variable (row with most negative RHS)
          int leaving_row = -1;
          T min_rhs = T{0};
          for (size_t i = 1; i <= num_rest; ++i)
            {
              T rhs = m->read(i, rhs_col);
              if (rhs < min_rhs - eps)
                {
                  min_rhs = rhs;
                  leaving_row = static_cast<int>(i);
                }
            }

          if (leaving_row == -1)
            {
              state = Solved;
              auto end_time = std::chrono::high_resolution_clock::now();
              stats.elapsed_ms = std::chrono::duration<double, std::milli>(
                end_time - start_time).count();
              return Solved;
            }

          // Select entering variable (minimum ratio test on reduced costs)
          int entering_col = -1;
          T min_ratio = std::numeric_limits<T>::max();
          const size_t total_vars = num_var + num_rest + num_artificial;

          for (size_t j = 0; j < total_vars; ++j)
            {
              T a_ij = m->read(leaving_row, j);
              if (a_ij >= -eps)
                continue; // Must be negative

              T ratio = -m->read(0, j) / a_ij;
              if (ratio < min_ratio)
                {
                  min_ratio = ratio;
                  entering_col = static_cast<int>(j);
                }
            }

          if (entering_col == -1)
            {
              state = Unfeasible;
              return Unfeasible;
            }

          ++stats.pivots;
          to_pivot(leaving_row, entering_col);
          basic_vars[leaving_row - 1] = entering_col;
        }
    }

    /** @brief Updates RHS value and re-optimizes using dual simplex.
     *
     * Efficiently re-solves the problem when only RHS values change.
     *
     * @param[in] rest_idx Constraint index to update.
     * @param[in] new_rhs New right-hand side value.
     * @return Solution state after re-optimization.
     *
     * @pre solve() must have been called and returned Solved.
     */
    [[nodiscard]] State update_rhs_and_reoptimize(size_t rest_idx, T new_rhs)
    {
      verify_rest_index(rest_idx);
      ah_logic_error_if(state != Solved)
        << "Cannot update RHS without a solved problem";

      // Update the RHS in the tableau
      const size_t rhs_col = num_var + num_rest + num_artificial;
      const size_t slack_col = num_var + rest_idx;

      // Compute the change in RHS
      T old_rhs = T{0};
      size_t i = 0;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++i)
        if (i == rest_idx)
          {
            old_rhs = it.get_curr()[num_var];
            it.get_curr()[num_var] = new_rhs;
            break;
          }

      T delta = new_rhs - old_rhs;

      // Update tableau RHS using slack variable column
      for (size_t row = 0; row <= num_rest; ++row)
        {
          T coef = m->read(row, slack_col);
          T current_rhs = m->read(row, rhs_col);
          m->write(row, rhs_col, current_rhs + coef * delta);
        }

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
   * The Revised Simplex method is an efficient variant of the standard
   * Simplex algorithm that maintains only the basis inverse B⁻¹ instead
   * of the full tableau. This provides significant memory and computational
   * savings for larger problems.
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
   * 1. **Initialize**: Set up initial basis (slack variables)
   * 2. **Pricing**: Compute dual prices π = c_B · B⁻¹
   * 3. **Select entering**: Find j with most negative c̄_j = c_j - π · a_j
   * 4. **FTRAN**: Compute d = B⁻¹ · a_j (tableau column)
   * 5. **Ratio test**: Find leaving variable using d
   * 6. **BTRAN/Update**: Update B⁻¹ using eta-matrix product form
   * 7. **Repeat** until optimal or unbounded
   *
   * ## Eta-Matrix Updates
   *
   * Instead of computing B⁻¹ directly, we maintain it as a product of
   * elementary matrices (eta-matrices): B⁻¹ = E_k · E_{k-1} · ... · E_1 · B_0⁻¹
   *
   * Periodic refactorization is performed to maintain numerical stability
   * and reduce accumulated errors.
   *
   * ## Performance
   *
   * - **Memory**: O(m² + mn) vs O(mn + m²) for standard
   * - **Time per iteration**: O(m²) for dense, O(nnz) for sparse
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

    // Basis information
    std::vector<int> basic;     // basic[i] = index of basic variable in row i
    std::vector<int> nonbasic;  // Non-basic variable indices
    std::vector<bool> is_basic; // is_basic[j] = true if variable j is in basis

    // Basis inverse representation using eta-matrices (product form)
    struct EtaMatrix
    {
      size_t col;           // Column being replaced
      std::vector<T> eta;   // The eta column (m elements)
    };
    std::vector<EtaMatrix> eta_file;  // Product form of B⁻¹
    std::vector<std::vector<T>> B_inv_explicit;  // Explicit B⁻¹ (after refactorization)

    // Current solution
    std::vector<T> x_B;         // Basic variable values (m)
    std::vector<T> solution;    // Full solution (n + m)

    // Working vectors (reused across iterations)
    mutable std::vector<T> pi;      // Dual prices (m)
    mutable std::vector<T> d;       // FTRAN result / tableau column (m)
    mutable std::vector<T> work;    // General work vector (m)

    // Configuration
    T eps;
    size_t refactor_frequency;  // Refactorize B⁻¹ every N iterations
    bool use_steepest_edge;

    // State and statistics
    State state;
    mutable SimplexStats stats;

    // ============ FTRAN: Solve B·y = a (forward transformation) ============
    // Computes y = B⁻¹·a
    // Uses explicit B⁻¹ for simplicity and numerical stability

    void ftran(const std::vector<T>& a, std::vector<T>& y) const
    {
      // y = B⁻¹ · a
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
    // Uses explicit B⁻¹ for simplicity

    void btran(const std::vector<T>& c_B, std::vector<T>& pi_out) const
    {
      // π_j = sum_i c_B[i] * B⁻¹[i][j]
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
      T rc = (j < n) ? c[j] : T{0};  // Slack variables have c = 0

      for (size_t i = 0; i < m; ++i)
        {
          T a_ij = (j < n) ? A[i][j] : ((j - n == i) ? T{1} : T{0});
          rc -= pi[i] * a_ij;
        }

      return rc;
    }

    // ============ Get column of A (including slack columns) ============

    void get_column(size_t j, std::vector<T>& col) const
    {
      if (j < n)
        {
          // Original variable column
          for (size_t i = 0; i < m; ++i)
            col[i] = A[i][j];
        }
      else
        {
          // Slack variable column (identity)
          size_t slack_idx = j - n;
          for (size_t i = 0; i < m; ++i)
            col[i] = (i == slack_idx) ? T{1} : T{0};
        }
    }

    // ============ Pricing: Find entering variable ============

    int select_entering_variable()
    {
      // Compute dual prices: π = c_B · B⁻¹
      std::vector<T> c_B(m);
      for (size_t i = 0; i < m; ++i)
        {
          int var = basic[i];
          c_B[i] = (var < static_cast<int>(n)) ? c[var] : T{0};
        }

      btran(c_B, pi);

      // For maximization: find most POSITIVE reduced cost
      // (positive rc means increasing that variable improves objective)
      T max_rc = eps;
      int entering = -1;

      for (int j : nonbasic)
        {
          T rc = compute_reduced_cost(j);
          if (rc > max_rc)
            {
              max_rc = rc;
              entering = j;
            }
        }

      return entering;
    }

    // ============ Ratio test: Find leaving variable ============

    int select_leaving_variable(int entering, T& theta)
    {
      // Get column of entering variable
      std::vector<T> a_j(m);
      get_column(entering, a_j);

      // FTRAN: compute d = B⁻¹ · a_j
      ftran(a_j, d);

      // Ratio test
      theta = std::numeric_limits<T>::max();
      int leaving = -1;

      for (size_t i = 0; i < m; ++i)
        {
          if (d[i] > eps)
            {
              T ratio = x_B[i] / d[i];
              if (ratio < theta - eps)
                {
                  theta = ratio;
                  leaving = static_cast<int>(i);
                }
            }
        }

      return leaving;
    }

    // ============ Update basis inverse after pivot ============
    // Uses the formula: B_new⁻¹ = E · B_old⁻¹
    // where E is an elementary matrix based on the pivot column d

    void update_basis_inverse(int leaving_row, const std::vector<T>& d_col)
    {
      T pivot = d_col[leaving_row];
      if (std::abs(pivot) < eps)
        return;  // Numerical issue

      // Compute eta column: eta[i] = -d[i]/pivot for i != leaving_row
      //                     eta[leaving_row] = 1/pivot
      std::vector<T> eta(m);
      for (size_t i = 0; i < m; ++i)
        {
          if (static_cast<int>(i) == leaving_row)
            eta[i] = T{1} / pivot;
          else
            eta[i] = -d_col[i] / pivot;
        }

      // Update B⁻¹: new_B_inv = E · B_inv
      // E is identity except column leaving_row has the eta values
      std::vector<std::vector<T>> new_B_inv(m, std::vector<T>(m));

      for (size_t i = 0; i < m; ++i)
        {
          for (size_t j = 0; j < m; ++j)
            {
              if (static_cast<int>(i) == leaving_row)
                {
                  // Row leaving_row: multiply by 1/pivot
                  new_B_inv[i][j] = B_inv_explicit[leaving_row][j] / pivot;
                }
              else
                {
                  // Other rows: subtract multiple of leaving_row
                  new_B_inv[i][j] = B_inv_explicit[i][j] -
                    (d_col[i] / pivot) * B_inv_explicit[leaving_row][j];
                }
            }
        }

      B_inv_explicit = std::move(new_B_inv);
    }

    // ============ Refactorize B⁻¹ explicitly ============

    void refactorize()
    {
      // Build explicit B from current basis
      std::vector<std::vector<T>> B(m, std::vector<T>(m, T{0}));

      for (size_t i = 0; i < m; ++i)
        {
          int var = basic[i];
          if (var < static_cast<int>(n))
            {
              // Original variable column
              for (size_t row = 0; row < m; ++row)
                B[row][i] = A[row][var];
            }
          else
            {
              // Slack variable (identity column)
              B[var - n][i] = T{1};
            }
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
            {
              if (std::abs(B[row][col]) > max_val)
                {
                  max_val = std::abs(B[row][col]);
                  pivot_row = row;
                }
            }

          // Swap rows if needed
          if (pivot_row != col)
            {
              std::swap(B[col], B[pivot_row]);
              std::swap(B_inv_explicit[col], B_inv_explicit[pivot_row]);
            }

          // Scale pivot row
          T pivot = B[col][col];
          for (size_t j = 0; j < m; ++j)
            {
              B[col][j] /= pivot;
              B_inv_explicit[col][j] /= pivot;
            }

          // Eliminate column
          for (size_t row = 0; row < m; ++row)
            {
              if (row != col)
                {
                  T factor = B[row][col];
                  for (size_t j = 0; j < m; ++j)
                    {
                      B[row][j] -= factor * B[col][j];
                      B_inv_explicit[row][j] -= factor * B_inv_explicit[col][j];
                    }
                }
            }
        }

      // Clear eta-file after refactorization
      eta_file.clear();
    }

  public:
    /** @brief Constructs a Revised Simplex solver.
     *
     * @param[in] num_vars Number of decision variables.
     * @param[in] num_constraints Number of constraints.
     */
    RevisedSimplex(size_t num_vars, size_t num_constraints)
      : m(num_constraints), n(num_vars),
        c(num_vars, T{0}),
        A(num_constraints, std::vector<T>(num_vars, T{0})),
        b(num_constraints, T{0}),
        basic(num_constraints),
        is_basic(num_vars + num_constraints, false),
        x_B(num_constraints, T{0}),
        solution(num_vars + num_constraints, T{0}),
        pi(num_constraints), d(num_constraints), work(num_constraints),
        eps(std::numeric_limits<T>::epsilon() * 1000),
        refactor_frequency(std::max(size_t{50}, num_constraints)),
        use_steepest_edge(false),
        state(Not_Solved)
    {
      ah_invalid_argument_if(num_vars == 0 || num_constraints == 0)
        << "Number of variables and constraints must be > 0";
    }

    /** @brief Sets objective coefficient.
     *
     * @param[in] j Variable index.
     * @param[in] coef Coefficient value.
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
     * @param[in] rhs Right-hand side value.
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
     * @param[in] rhs Right-hand side value.
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
     * B⁻¹ is recomputed explicitly every N iterations to maintain
     * numerical stability.
     *
     * @param[in] freq Refactorization frequency (default: max(50, m)).
     */
    void set_refactorization_frequency(size_t freq)
    {
      refactor_frequency = freq;
    }

    /** @brief Solves the linear program.
     *
     * @return Solution state (Solved, Unbounded, or Unfeasible).
     */
    [[nodiscard]] State solve()
    {
      ah_logic_error_if(state != Not_Solved) << "Already solved";

      stats.reset();
      auto start_time = std::chrono::high_resolution_clock::now();

      // Initialize basis with slack variables
      for (size_t i = 0; i < m; ++i)
        {
          basic[i] = static_cast<int>(n + i);  // Slack variable index
          is_basic[n + i] = true;
          x_B[i] = b[i];  // Initial basic solution
        }

      // Initialize non-basic variable list
      nonbasic.clear();
      for (size_t j = 0; j < n; ++j)
        nonbasic.push_back(static_cast<int>(j));

      // Initial B⁻¹ is identity (slack variables)
      B_inv_explicit.assign(m, std::vector<T>(m, T{0}));
      for (size_t i = 0; i < m; ++i)
        B_inv_explicit[i][i] = T{1};

      state = Solving;

      // Main loop
      while (true)
        {
          ++stats.iterations;

          // Pricing: select entering variable
          int entering = select_entering_variable();

          if (entering < 0)
            {
              // Optimal!
              state = Solved;
              break;
            }

          // Ratio test: select leaving variable
          T theta;
          int leaving = select_leaving_variable(entering, theta);

          if (leaving < 0)
            {
              // Unbounded!
              state = Unbounded;
              break;
            }

          // Update solution
          for (size_t i = 0; i < m; ++i)
            x_B[i] -= theta * d[i];
          x_B[leaving] = theta;

          // Update basis
          int leaving_var = basic[leaving];
          basic[leaving] = entering;
          is_basic[leaving_var] = false;
          is_basic[entering] = true;

          // Update non-basic list
          for (auto& nb : nonbasic)
            if (nb == entering)
              {
                nb = leaving_var;
                break;
              }

          // Update B⁻¹
          update_basis_inverse(leaving, d);

          ++stats.pivots;
        }

      // Load final solution
      std::fill(solution.begin(), solution.end(), T{0});
      for (size_t i = 0; i < m; ++i)
        solution[basic[i]] = x_B[i];

      auto end_time = std::chrono::high_resolution_clock::now();
      stats.elapsed_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();

      return state;
    }

    /** @brief Gets solution value for variable j.
     *
     * @param[in] j Variable index.
     * @return Optimal value of variable x_j.
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
     * @return true if feasible, false otherwise.
     */
    [[nodiscard]] bool verify_solution() const
    {
      for (size_t i = 0; i < m; ++i)
        {
          T lhs = T{0};
          for (size_t j = 0; j < n; ++j)
            lhs += A[i][j] * solution[j];
          if (lhs > b[i] + eps)
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
                << "Eta-matrices: " << eta_file.size() << "\n"
                << "Refactorizations: "
                << (stats.pivots / refactor_frequency) << "\n";
    }
  };

} // end namespace Aleph

# endif // SIMPLEX_H
