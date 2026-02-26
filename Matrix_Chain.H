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


/** @file Matrix_Chain.H
 *  @brief Matrix-chain multiplication ordering via interval DP.
 *
 *  Given a chain of matrices A1 x A2 x ... x An with dimensions
 *  dims[0] x dims[1], dims[1] x dims[2], ..., dims[n-1] x dims[n],
 *  computes the optimal parenthesization that minimizes scalar
 *  multiplications.
 *
 *  @example matrix_chain_example.cc
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef MATRIX_CHAIN_H
# define MATRIX_CHAIN_H

# include <limits>
# include <string>
# include <cstddef>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Result of matrix-chain multiplication optimization. */
  struct Matrix_Chain_Result
  {
    size_t min_multiplications = 0; /**< Minimum scalar multiplications */
    std::string parenthesization; /**< Optimal parenthesization string */
    Array<Array<size_t>> split; /**< Split table for reconstruction */
  };


  namespace matrix_chain_detail
  {
    inline size_t checked_add(const size_t a, const size_t b,
                              const char *ctx)
    {
      ah_runtime_error_if(a > std::numeric_limits<size_t>::max() - b)
        << "matrix_chain_order: overflow while computing " << ctx;
      return a + b;
    }

    inline size_t checked_mul(const size_t a, const size_t b,
                              const char *ctx)
    {
      if (a == 0 or b == 0)
        return 0;

      ah_runtime_error_if(a > std::numeric_limits<size_t>::max() / b)
        << "matrix_chain_order: overflow while computing " << ctx;
      return a * b;
    }

    inline size_t scalar_cost(const size_t di, const size_t dk,
                              const size_t dj)
    {
      const size_t lhs = checked_mul(di, dk, "dims[i] * dims[k+1]");
      return checked_mul(lhs, dj, "dims[i] * dims[k+1] * dims[j+1]");
    }

    inline void build_parens(const Array<Array<size_t>> & s,
                             size_t i, size_t j,
                             std::string & out)
    {
      if (i == j)
        {
          out += "A";
          out += std::to_string(i + 1);
          return;
        }
      out += "(";
      build_parens(s, i, s[i][j], out);
      out += " ";
      build_parens(s, s[i][j] + 1, j, out);
      out += ")";
    }
  } // namespace matrix_chain_detail


  /** @brief Compute optimal matrix-chain multiplication order.
   *
   *  @param[in] dims  Array of dimensions: matrix i has dimensions
   *                   dims[i] x dims[i+1], so dims has n+1 entries
   *                   for n matrices.
   *  @return Matrix_Chain_Result with cost, parenthesization, and split table.
   *
   *  @throws ah_domain_error if dims has fewer than 2 entries (no matrices).
   *  @throws ah_domain_error if any dimension is zero.
   *  @throws ah_runtime_error if scalar cost overflows `size_t`.
   *
   *  @note Complexity: O(n^3) time, O(n^2) space.
   */
  [[nodiscard]] inline Matrix_Chain_Result
  matrix_chain_order(const Array<size_t> & dims)
  {
    ah_domain_error_if(dims.size() < 2)
      << "matrix_chain_order: dims must have at least 2 entries";
    for (const size_t dim : dims)
      ah_domain_error_if(dim == 0)
        << "matrix_chain_order: matrix dimensions must be positive";

    const size_t n = dims.size() - 1; // number of matrices
    if (n == 1)
      return Matrix_Chain_Result{0, "A1", Array<Array<size_t>>()};

    // dp[i][j] = min cost of multiplying matrices i..j (0-based)
    Array<Array<size_t>> dp;
    dp.reserve(n);
    for (size_t i = 0; i < n; ++i)
      {
        Array<size_t> row = Array<size_t>::create(n);
        for (size_t j = 0; j < n; ++j)
          row(j) = 0;
        dp.append(std::move(row));
      }

    Array<Array<size_t>> split;
    split.reserve(n);
    for (size_t i = 0; i < n; ++i)
      {
        Array<size_t> row = Array<size_t>::create(n);
        for (size_t j = 0; j < n; ++j)
          row(j) = 0;
        split.append(std::move(row));
      }

    // chain length l = 2..n
    for (size_t l = 2; l <= n; ++l)
      for (size_t i = 0; i <= n - l; ++i)
        {
          const size_t j = i + l - 1;
          dp[i][j] = std::numeric_limits<size_t>::max();
          for (size_t k = i; k < j; ++k)
            {
              const size_t mul_cost =
                matrix_chain_detail::scalar_cost(dims[i], dims[k + 1], dims[j + 1]);
              const size_t subtotal =
                matrix_chain_detail::checked_add(dp[i][k], dp[k + 1][j], "dp[i][k] + dp[k+1][j]");
              const size_t cost =
                matrix_chain_detail::checked_add(subtotal, mul_cost, "dp + scalar multiplications");
              if (cost < dp[i][j])
                {
                  dp[i][j] = cost;
                  split[i][j] = k;
                }
            }
        }

    std::string parens;
    matrix_chain_detail::build_parens(split, 0, n - 1, parens);

    return Matrix_Chain_Result{
          dp[0][n - 1], std::move(parens),
          std::move(split)
        };
  }


  /** @brief Compute only the minimum multiplication cost.
   *
   *  @param[in] dims  Array of matrix dimensions (n+1 entries for n matrices).
   *  @return Minimum number of scalar multiplications.
   *
   *  @throws ah_domain_error if dims has fewer than 2 entries.
   *  @throws ah_domain_error if any dimension is zero.
   *  @throws ah_runtime_error if scalar cost overflows `size_t`.
   *
   *  @note Complexity: O(n^3) time, O(n^2) space.
   */
  [[nodiscard]] inline size_t
  matrix_chain_min_cost(const Array<size_t> & dims)
  {
    return matrix_chain_order(dims).min_multiplications;
  }
} // namespace Aleph

# endif // MATRIX_CHAIN_H
