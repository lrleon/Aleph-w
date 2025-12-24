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

# ifndef SIMPLEX_H
# define SIMPLEX_H

# include <limits>
# include <fstream>
# include <tpl_dynMat.H>
# include <tpl_dynDlist.H>
# include <format.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Linear program solver using the Simplex method.
   *
   * `Simplex<T>` allows expressing linear programs in standard form:
   * a maximization objective function and a set of constraints with
   * positive coefficients and less-than-or-equal-to inequalities.
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
   * ## Converting to Standard Form
   *
   * For non-standard cases:
   * 1. **Minimization**: Minimize Z = -Maximize(-Z)
   * 2. **≥ constraints**: Multiply both sides by -1 to get ≤
   * 3. **= constraints**: Replace with two constraints (≤ and ≥)
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
   * @ingroup Redes
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

  private:
    // Selects the cell in the objective function with minimum value.
    // Returns -1 if all cells are non-negative (optimality reached).
    int compute_pivot_col() const noexcept
    {
      T minimum = std::numeric_limits<T>::max();
      int p = -1;
      const int M = num_var + num_rest;
      for (int i = 0; i < M; ++i)
        {
          const T & c = m->read(0, i);
          if (c < minimum)
            {
              p = i;
              minimum = c;
            }
        }
      return minimum >= 0 ? -1 : p;
    }

    // Selects among elements B the minimum ratio between the RHS value
    // and the pivot column coefficient (must be positive).
    // Returns -1 if no valid pivot row exists (unbounded).
    int compute_pivot_row(int p) const noexcept
    {
      assert(p >= 0 and p < static_cast<int>(num_var + num_rest));

      int q = -1;
      T min_ratio = std::numeric_limits<T>::max();
      const int M = num_var + num_rest;

      for (int i = 1; i <= static_cast<int>(num_rest); ++i)
        {
          const T val = m->read(i, M); // RHS value
          if (val < 0)
            continue;

          const T den = m->read(i, p); // pivot column coefficient
          if (den <= 0)
            continue;

          const T ratio = val / den;
          if (ratio < min_ratio)
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
    // Optimized version: caches pivot row and column to reduce matrix accesses.
    void to_pivot(size_t p, size_t q)
    {
      assert(p <= num_rest and q <= num_var + num_rest);

      const size_t M = num_var + num_rest;  // number of columns
      const size_t cols = M + 1;            // total columns including RHS
      const T pivot = m->read(p, q);
      const T inv_pivot = T{1} / pivot;

      // Cache pivot row (normalized) - avoids repeated reads during elimination
      std::unique_ptr<T[]> pivot_row(new T[cols]);
      for (size_t j = 0; j <= M; ++j)
        pivot_row[j] = (j == q) ? T{1} : m->read(p, j) * inv_pivot;

      // Write normalized pivot row back to matrix
      for (size_t j = 0; j <= M; ++j)
        m->write(p, j, pivot_row[j]);

      // Cache pivot column values before modification
      std::unique_ptr<T[]> pivot_col(new T[num_rest + 1]);
      for (size_t i = 0; i <= num_rest; ++i)
        pivot_col[i] = m->read(i, q);

      // Eliminate: for each row i != p, subtract (pivot_col[i] * pivot_row)
      for (size_t i = 0; i <= num_rest; ++i)
        {
          if (i == p)
            continue;

          const T factor = pivot_col[i];
          if (factor == T{0})
            continue;  // Skip rows where pivot column is already 0

          for (size_t j = 0; j <= M; ++j)
            if (j == q)
              m->write(i, j, T{0});  // Pivot column becomes 0
            else
              m->write(i, j, m->read(i, j) - factor * pivot_row[j]);
        }
    }

    // Finds the value of variable j from the tableau.
    T find_value(const size_t j) const noexcept
    {
      assert(j < num_var);

      T ret_val = 0.0;
      int count = 0;

      for (size_t i = 1; i <= num_rest; ++i)
        {
          const T & value = m->read(i, j);
          if (value == 0.0)
            continue;

          if (value == 1.0)
            {
              if (count++ == 0)
                ret_val = m->read(i, num_var + num_rest);
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
    size_t num_var; // Number of decision variables
    size_t num_rest; // Number of restrictions
    std::unique_ptr<T[]> solution; // Solution values
    State state; // Current state

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
    T * create_restriction()
    {
      T *ptr = new T[num_var + 1];
      rest_list.append(ptr);
      ++num_rest;
      for (size_t i = 0; i <= num_var; ++i)
        ptr[i] = 0;
      return ptr;
    }

    // Creates the simplex tableau matrix from objective and constraints.
    void create_matrix()
    {
      m = std::unique_ptr<DynMatrix<T>>(
                                        new DynMatrix<T>(num_rest + 1, num_var + num_rest + 1));

      // Fill objective function coefficients (negated for maximization)
      for (size_t i = 0; i < num_var; ++i)
        m->write(0, i, -objetive[i]);

      // Fill constraint coefficients
      size_t row = 1;
      for (auto it = rest_list.get_it(); it.has_curr(); it.next_ne(), ++row)
        {
          T *rest = it.get_curr();

          // Decision variable coefficients
          for (size_t j = 0; j < num_var; ++j)
            m->write(row, j, rest[j]);

          // Slack variable coefficient (1 for this constraint's slack)
          m->write(row, num_var + row - 1, 1);

          // RHS value
          m->write(row, num_var + num_rest, rest[num_var]);
        }
    }

  public:
    /** @brief Constructs a Simplex solver for n variables.
     *
     * Initializes a linear program in standard form without constraints
     * and with all objective function coefficients set to zero.
     *
     * @param[in] n Number of decision variables in the system.
     *
     * @throw std::bad_alloc If memory allocation fails.
     * @throw std::invalid_argument If n is zero.
     */
    explicit Simplex(const size_t n)
      : m(nullptr), objetive(new T[n]), num_var(n), num_rest(0),
        solution(new T[n]), state(Not_Solved)
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
     *   c[0]*x_0 + c[1]*x_1 + ... + c[n-1]*x_{n-1} <= c[n]
     *
     * where c[n] is the RHS (right-hand side) bound.
     *
     * @param[in] coefs Array of n+1 coefficients (n variable coefficients + RHS).
     *                  If nullptr, creates a zero-initialized constraint.
     *
     * @return Pointer to the internal constraint array (can be modified).
     *
     * @throw std::bad_alloc If memory allocation fails.
     */
    T * put_restriction(const T *coefs = nullptr)
    {
      T *rest = create_restriction();

      if (coefs == nullptr)
        return rest;

      for (size_t i = 0; i <= num_var; ++i)
        rest[i] = coefs[i];

      return rest;
    }

    /** @brief Adds a constraint from a DynArray.
     *
     * @param[in] coefs Array of n+1 coefficients.
     * @return Pointer to the internal constraint array.
     *
     * @throw std::bad_alloc If memory allocation fails.
     */
    T * put_restriction(const DynArray<T> & coefs)
    {
      T *rest = create_restriction();

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

      for (int i, j; true;)
        {
          const State st = select_pivot(i, j);
          if (st == Unbounded or st == Solved)
            return st;

          to_pivot(i, j);
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
  };
} // end namespace Aleph

# endif // SIMPLEX_H
