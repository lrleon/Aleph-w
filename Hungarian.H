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


/** @file Hungarian.H
 *  @brief Hungarian (Munkres) algorithm for the assignment problem.
 *
 *  This file implements the Hungarian algorithm, also known as the
 *  Kuhn-Munkres algorithm, for solving the assignment problem: given an
 *  m x n cost matrix, find a minimum-cost assignment of rows to columns.
 *
 *  ## Algorithm Overview
 *
 *  The implementation uses the shortest augmenting paths variant with
 *  dual variables (potentials). For each row, a Dijkstra-like scan
 *  finds the shortest augmenting path using reduced costs
 *  `c[i][j] - u[i] - v[j]`, then augments and updates potentials.
 *  This avoids the classical cover-lines/find-zeros Munkres steps and
 *  yields a clean, efficient O(n^3) implementation.
 *
 *  ## Key Features
 *
 *  - Solves both minimization and maximization assignment problems
 *  - Handles rectangular cost matrices (m != n)
 *  - Works with integer and floating-point cost types
 *  - Supports negative costs
 *  - Query-after-compute API (constructor solves, methods query)
 *
 *  ## Complexity
 *
 *  | Metric | Value |
 *  |--------|-------|
 *  | Time | O(n^3) where n = max(rows, cols) |
 *  | Space | O(n^2) for the padded cost matrix |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Direct construction from initializer list
 *  Hungarian_Assignment<int> ha({
 *    {10, 5, 13},
 *    {3,  9, 18},
 *    {10, 6, 12}
 *  });
 *
 *  auto cost = ha.get_total_cost();    // optimal cost
 *  auto pairs = ha.get_assignments();  // list of (row, col) pairs
 *
 *  // Or from a DynMatrix
 *  DynMatrix<double> cost_mat(3, 3);
 *  // ... fill cost_mat ...
 *  auto result = hungarian_assignment(cost_mat);
 *  ```
 *
 *  @see tpl_mincost.H For min-cost flow-based assignment (solve_assignment)
 *  @see tpl_bipartite.H For bipartite matching
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef HUNGARIAN_H
# define HUNGARIAN_H

# include <limits>
# include <type_traits>
# include <utility>
# include <initializer_list>
# include <tpl_dynMat.H>
# include <tpl_array.H>
# include <htlist.H>
# include <ah-errors.H>
# include <ahFunction.H>

namespace Aleph
{
  /** @brief Result of the Hungarian assignment algorithm.
   *
   *  Holds the optimal cost and the row-to-column / column-to-row
   *  mappings. For rectangular matrices, dummy assignments (to padded
   *  rows or columns) are marked with -1.
   *
   *  @tparam Cost_Type Numeric type for costs (default: double).
   *  @ingroup Graphs
   */
  template <typename Cost_Type = double>
  struct Hungarian_Result
  {
    static_assert(std::is_signed_v<std::remove_cv_t<Cost_Type>> or
                  std::is_floating_point_v<std::remove_cv_t<Cost_Type>>,
                  "Cost_Type must be signed or floating-point to avoid "
                  "underflow when negating costs");
    Cost_Type total_cost = Cost_Type{0}; ///< Optimal total cost
    Array<long> row_to_col; ///< row i is assigned to column row_to_col[i]
    Array<long> col_to_row; ///< column j is assigned to row col_to_row[j]
    size_t orig_rows = 0; ///< Original number of rows
    size_t orig_cols = 0; ///< Original number of columns

    /** @brief Get the assignment pairs, excluding dummy entries.
     *
     *  Returns only the pairs (row, col) where both row and col are
     *  within the original (non-padded) dimensions.
     *
     *  @return List of (row, col) pairs.
     */
    [[nodiscard]] DynList<std::pair<size_t, size_t>> get_pairs() const
    {
      DynList<std::pair<size_t, size_t>> pairs;
      for (size_t i = 0; i < orig_rows; ++i)
        if (const long j = row_to_col[i]; j >= 0 and static_cast<size_t>(j) < orig_cols)
          pairs.append(std::make_pair(i, static_cast<size_t>(j)));
      return pairs;
    }
  };

  /** @brief Hungarian (Munkres) algorithm for optimal assignment.
   *
   *  Given an m x n cost matrix, finds the minimum-cost assignment of
   *  rows to columns. The constructor computes the solution; query
   *  methods retrieve the results.
   *
   *  For rectangular matrices, the smaller dimension is padded with
   *  zero-cost rows or columns. Assignments to padded entries are
   *  reported as -1.
   *
   *  The algorithm uses the shortest augmenting paths variant with
   *  dual variables, running in O(n^3) time where n = max(m, cols).
   *
   *  @tparam Cost_Type Numeric type for costs (default: double).
   *
   *  @par Example
   *  ```cpp
   *  Hungarian_Assignment<int> ha({
   *    {82, 83, 69, 92},
   *    {77, 37, 49, 92},
   *    {11, 69, 5,  86},
   *    {8,  9,  98, 23}
   *  });
   *
   *  std::cout << ha.get_total_cost() << "\n";  // 140
   *  for (auto [r, c] : ha.get_assignments())
   *    std::cout << r << " -> " << c << "\n";
   *  ```
   *
   *  @ingroup Graphs
   */
  template <typename Cost_Type = double>
  class Hungarian_Assignment
  {
    static_assert(std::is_signed_v<std::remove_cv_t<Cost_Type>> or
                  std::is_floating_point_v<std::remove_cv_t<Cost_Type>>,
                  "Cost_Type must be signed or floating-point to avoid "
                  "underflow when negating costs");
    size_t n_ = 0; // Padded square dimension
    size_t orig_rows_ = 0; // Original row count
    size_t orig_cols_ = 0; // Original column count
    Cost_Type total_cost_ = Cost_Type{0};
    Array<long> row_to_col_; // row i -> column (or -1 if dummy)
    Array<long> col_to_row_; // column j -> row (or -1 if dummy)

    /** Core algorithm: shortest augmenting paths with dual variables.
     *
     *  Uses 1-indexed arrays with virtual column 0 trick.
     *  cost is an (n+1) x (n+1) matrix with 1-based indexing.
     */
    void solve(const DynMatrix<Cost_Type> & cost)
    {
      const long n = static_cast<long>(n_);
      const auto sz = static_cast<size_t>(n + 1);

      // Dual variables (potentials), 1-indexed
      Array<Cost_Type> u(sz, Cost_Type{0});
      Array<Cost_Type> v(sz, Cost_Type{0});

      // p[j] = row matched to column j (0 = unmatched)
      Array<long> p(sz, 0L);

      const Cost_Type Inf = std::numeric_limits<Cost_Type>::max() / 2;

      // For each row, find the shortest augmenting path
      for (long i = 1; i <= n; ++i)
        {
          // "Virtual" column 0 is matched to row i
          p(0) = i;

          // Minimum reduced cost to reach each column
          Array<Cost_Type> dist(sz, Inf);
          dist(0) = Cost_Type{0};

          // way[j] = previous column on the shortest path to j
          Array<long> way(sz, 0L);

          // visited[j] = true if column j is in the "tree"
          Array<bool> visited(sz, false);

          // Dijkstra-like scan
          long j0 = 0; // current column (start at virtual column 0)
          do
            {
              visited(j0) = true;
              const long i0 = p(j0); // row matched to current column
              Cost_Type delta = Inf;
              long j1 = -1;

              for (long j = 1; j <= n; ++j)
                {
                  if (visited(j))
                    continue;

                  // Reduced cost: c[i0][j] - u[i0] - v[j]
                  const Cost_Type reduced = cost.read(static_cast<size_t>(i0 - 1),
                                                      static_cast<size_t>(j - 1)) - u(i0) - v(j);

                  if (reduced < dist(j))
                    {
                      dist(j) = reduced;
                      way(j) = j0;
                    }

                  if (dist(j) < delta)
                    {
                      delta = dist(j);
                      j1 = j;
                    }
                }

              // Update potentials
              for (long j = 0; j <= n; ++j)
                if (visited(j))
                  {
                    u(p(j)) += delta;
                    v(j) -= delta;
                  }
                else
                  dist(j) -= delta;

              j0 = j1;
            }
          while (p(j0) != 0); // until we reach a free column

          // Augment along the path
          do
            {
              const long j1 = way(j0);
              p(j0) = p(j1);
              j0 = j1;
            }
          while (j0 != 0);
        }

      // Extract results
      total_cost_ = -v(0); // v[0] accumulates the total cost

      row_to_col_ = Array<long>(orig_rows_, -1L);
      col_to_row_ = Array<long>(orig_cols_, -1L);

      for (long j = 1; j <= n; ++j)
        {
          const long row = p(j) - 1; // convert to 0-based
          const long col = j - 1;

          if (row >= 0 and static_cast<size_t>(row) < orig_rows_ and
              col >= 0 and static_cast<size_t>(col) < orig_cols_)
            {
              row_to_col_(static_cast<size_t>(row)) = col;
              col_to_row_(static_cast<size_t>(col)) = row;
            }
        }
    }

  public:
    /** @brief Construct and solve from a DynMatrix cost matrix.
     *
     *  Computes the minimum-cost assignment for the given cost matrix.
     *  Rectangular matrices are padded with zeros to form a square.
     *
     *  @param[in] cost The m x n cost matrix.
     *  @throws std::invalid_argument if the matrix is empty.
     */
    Hungarian_Assignment(const DynMatrix<Cost_Type> & cost)
    {
      orig_rows_ = cost.rows();
      orig_cols_ = cost.cols();
      ah_invalid_argument_if(orig_rows_ == 0 or orig_cols_ == 0)
        << "Hungarian_Assignment: cost matrix must not be empty";

      n_ = orig_rows_ > orig_cols_ ? orig_rows_ : orig_cols_;

      // Build padded square matrix (zeros for padding)
      DynMatrix<Cost_Type> padded(n_, n_, Cost_Type{0});
      padded.allocate();
      for (size_t i = 0; i < orig_rows_; ++i)
        for (size_t j = 0; j < orig_cols_; ++j)
          padded(i, j) = cost.read(i, j);

      solve(padded);
    }

    /** @brief Construct and solve from an initializer list of rows.
     *
     *  Convenience constructor for inline cost matrices.
     *
     *  @param[in] rows Initializer list of rows, each an initializer
     *                   list of costs. All rows must have the same length.
     *  @throws std::invalid_argument if rows is empty or rows have
     *          different lengths.
     *
     *  @par Example
     *  ```cpp
     *  Hungarian_Assignment<int> ha({
     *    {10, 5, 13},
     *    {3,  9, 18},
     *    {10, 6, 12}
     *  });
     *  ```
     */
    Hungarian_Assignment(std::initializer_list<std::initializer_list<Cost_Type>> rows)
    {
      ah_invalid_argument_if(rows.size() == 0) <<
        "Hungarian_Assignment: cost matrix must not be empty";

      orig_rows_ = rows.size();
      orig_cols_ = rows.begin()->size();
      n_ = orig_rows_ > orig_cols_ ? orig_rows_ : orig_cols_;

      DynMatrix<Cost_Type> padded(n_, n_, Cost_Type{0});
      padded.allocate();

      size_t i = 0;
      for (const auto & row: rows)
        {
          ah_invalid_argument_if(row.size() != orig_cols_)
            << "Hungarian_Assignment: all rows must have the same length";
          size_t j = 0;
          for (const auto & val: row)
            padded(i, j++) = val;
          ++i;
        }

      solve(padded);
    }

    /** @brief Get the optimal total cost.
     *  @return The minimum total assignment cost.
     */
    [[nodiscard]] Cost_Type get_total_cost() const noexcept
    {
      return total_cost_;
    }

    /** @brief Get the column assigned to a given row.
     *
     *  @param[in] row Row index (0-based, must be < original rows).
     *  @return Column index, or -1 if the row is unassigned (dummy).
     *  @throws std::out_of_range if row >= original number of rows.
     */
    [[nodiscard]] long get_assignment(const size_t row) const
    {
      ah_out_of_range_error_if(row >= orig_rows_)
        << "Hungarian_Assignment::get_assignment: row " << row
        << " out of range [0, " << orig_rows_ << ")";
      return row_to_col_[row];
    }

    /** @brief Get all assignment pairs.
     *
     *  Returns only the pairs (row, col) where both indices are within
     *  the original (non-padded) dimensions.
     *
     *  @return List of (row, col) pairs.
     */
    [[nodiscard]] DynList<std::pair<size_t, size_t>> get_assignments() const
    {
      DynList<std::pair<size_t, size_t>> pairs;
      for (size_t i = 0; i < orig_rows_; ++i)
        if (const long j = row_to_col_[i]; j >= 0 and static_cast<size_t>(j) < orig_cols_)
          pairs.append(std::make_pair(i, static_cast<size_t>(j)));
      return pairs;
    }

    /** @brief Get the row-to-column assignment array.
     *  @return Const reference to array where entry i is the column
     *          assigned to row i (-1 if dummy).
     */
    [[nodiscard]] const Array<long> &get_row_assignments() const noexcept
    {
      return row_to_col_;
    }

    /** @brief Get the column-to-row assignment array.
     *  @return Const reference to array where entry j is the row
     *          assigned to column j (-1 if dummy).
     */
    [[nodiscard]] const Array<long> &get_col_assignments() const noexcept
    {
      return col_to_row_;
    }

    /** @brief Get the padded square dimension.
     *  @return max(original_rows, original_cols).
     */
    [[nodiscard]] size_t dimension() const noexcept { return n_; }

    /** @brief Get the original number of rows.
     *  @return The number of rows in the input cost matrix.
     */
    [[nodiscard]] size_t rows() const noexcept { return orig_rows_; }

    /** @brief Get the original number of columns.
     *  @return The number of columns in the input cost matrix.
     */
    [[nodiscard]] size_t cols() const noexcept { return orig_cols_; }
  };

  /** @brief Compute minimum-cost assignment (free function).
   *
   *  Convenience free function that returns a Hungarian_Result.
   *
   *  @tparam Cost_Type Numeric type for costs.
   *  @param[in] cost The m x n cost matrix.
   *  @return Hungarian_Result with optimal assignment.
   *
   *  @par Example
   *  ```cpp
   *  DynMatrix<int> cost(3, 3);
   *  // ... fill ...
   *  auto result = hungarian_assignment(cost);
   *  ```
   *
   *  @ingroup Graphs
   */
  template <typename Cost_Type>
  [[nodiscard]] Hungarian_Result<Cost_Type>
  hungarian_assignment(const DynMatrix<Cost_Type> & cost)
  {
    Hungarian_Assignment<Cost_Type> ha(cost);
    Hungarian_Result<Cost_Type> result;
    result.total_cost = ha.get_total_cost();
    result.row_to_col = ha.get_row_assignments();
    result.col_to_row = ha.get_col_assignments();
    result.orig_rows = ha.rows();
    result.orig_cols = ha.cols();
    return result;
  }

  /** @brief Compute maximum-profit assignment (free function).
   *
   *  Negates the cost matrix and solves the resulting minimization
   *  problem. The returned total_cost is the maximum profit (positive).
   *
   *  @tparam Cost_Type Numeric type for costs.
   *  @param[in] cost The m x n profit matrix.
   *  @return Hungarian_Result with optimal assignment maximizing profit.
   *
   *  @par Example
   *  ```cpp
   *  DynMatrix<int> profit(3, 3);
   *  // ... fill with profits ...
   *  auto result = hungarian_max_assignment(profit);
   *  // result.total_cost is the maximum total profit
   *  ```
   *
   *  @ingroup Graphs
   */
  template <typename Cost_Type>
  [[nodiscard]] Hungarian_Result<Cost_Type>
  hungarian_max_assignment(const DynMatrix<Cost_Type> & cost)
  {
    // Use a promoted signed type for negation to avoid overflow when
    // Cost_Type is a signed integer and a cell contains its minimum value.
    using Common = std::common_type_t<Cost_Type, long long>;
    using Promoted = std::conditional_t<std::is_floating_point_v<Common>,
                                        Common,
                                        std::make_signed_t<Common>>;

    const size_t rows = cost.rows();
    const size_t cols = cost.cols();
    DynMatrix<Promoted> negated(rows, cols);
    negated.allocate();
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < cols; ++j)
        {
          if constexpr (std::is_signed_v<Promoted>)
            {
              auto v = static_cast<Promoted>(cost.read(i, j));
              ah_overflow_error_if(v == std::numeric_limits<Promoted>::min())
                << "Cannot negate minimum integer value";
              negated(i, j) = -v;
            }
          else
            {
              negated(i, j) = -static_cast<Promoted>(cost.read(i, j));
            }
        }

    auto inner = hungarian_assignment(negated);

    Hungarian_Result<Cost_Type> result;
    result.total_cost  = static_cast<Cost_Type>(-inner.total_cost);
    result.row_to_col  = std::move(inner.row_to_col);
    result.col_to_row  = std::move(inner.col_to_row);
    result.orig_rows   = inner.orig_rows;
    result.orig_cols   = inner.orig_cols;
    return result;
  }
} // end namespace Aleph

# endif // HUNGARIAN_H
