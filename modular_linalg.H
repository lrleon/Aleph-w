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

/** @file modular_linalg.H
 *  @brief Linear algebra operations over finite fields (modulo a prime).
 *
 *  Provides Gaussian elimination, determinant computation, and matrix inversion
 *  modulo a prime number. Supports both dense (Array of Array) and sparse (DynMatrix)
 *  backends.
 *
 *  @ingroup Algorithms
 */

# ifndef MODULAR_LINALG_H
# define MODULAR_LINALG_H

# include <cstdint>
# include <algorithm>
# include <optional>
# include <type_traits>
# include <ah-errors.H>
# include <tpl_array.H>
# include <tpl_dynMat.H>
# include <modular_arithmetic.H>
# include <primality.H>
# include <concepts>
# include <utility>

namespace Aleph
{
  /** @concept SparseMatrix
   *  Expresses requirements for sparse matrix backends like DynMatrix.
   */
  template <typename T>
  concept SparseMatrix = requires(const T m, T & mut_m, size_t r, size_t c, uint64_t v) {
    { m.rows() } -> std::convertible_to<size_t>;
    { m.cols() } -> std::convertible_to<size_t>;
    { m.read_ne(r, c) } -> std::convertible_to<uint64_t>;
    { mut_m.write(r, c, v) };
    { mut_m.traverse_allocated([](uint64_t &) { return true; }) } -> std::convertible_to<bool>;
    { mut_m.set_dimension(r, c) };
  };

  /** @concept DenseMatrix
   *  Expresses requirements for dense matrix backends like Array<Array<uint64_t>>.
   */
  template <typename T>
  concept DenseMatrix = std::is_same_v<T, Array<Array<uint64_t>>>;

  /** @concept ModularMatrixBackend
   *  Union of supported matrix backends for modular operations.
   */
  template <typename T>
  concept ModularMatrixBackend = DenseMatrix<T> or SparseMatrix<T>;

  /** @brief Get dimensions of a matrix (dense or sparse).
   *
   *  @tparam MatrixT The matrix type (must satisfy ModularMatrixBackend).
   *  @param[in] m The input matrix.
   *  @return A pair containing (number of rows, number of columns).
   *  @throws none Does not throw (noexcept).
   *  @note Complexity: O(1).
   */
  template <DenseMatrix MatrixT>
  [[nodiscard]] constexpr std::pair<size_t, size_t>
  matrix_dims(const MatrixT & m) noexcept
  {
    const size_t rows = m.size();
    return {rows, rows > 0 ? m[0].size() : 0};
  }

  /// @overload matrix_dims(const MatrixT & m)
  template <SparseMatrix MatrixT>
  [[nodiscard]] constexpr std::pair<size_t, size_t>
  matrix_dims(const MatrixT & m) noexcept
  {
    return {m.rows(), m.cols()};
  }

  /** @brief Matrix operations modulo a prime.
   *
   *  Provides linear algebra algorithms over a finite field defined by a prime
   *  modulus. Supports various matrix backends like Array of Arrays for dense
   *  matrices or DynMatrix for sparse ones.
   *
   *  ## Thread-safety
   *  Methods are safe for concurrent reads from multiple threads. External
   *  synchronization is required if the underlying matrix backend is being
   *  modified.
   *
   *  @tparam MatrixT The matrix container type. Must satisfy ModularMatrixBackend concept.
   */
  template <ModularMatrixBackend MatrixT>
  class Modular_Matrix
  {
    MatrixT mat; ///< The underlying matrix data.
    uint64_t mod; ///< The prime modulus defining the finite field.

    // Private helpers for matrix element access using constrained overloads
    static uint64_t get_val(const MatrixT & m, size_t r, size_t c)
    {
      if constexpr (DenseMatrix<MatrixT>)
        return m[r][c];
      else
        return m.read_ne(r, c);
    }

    static void set_val(MatrixT & m, size_t r, size_t c, uint64_t v)
    {
      if constexpr (DenseMatrix<MatrixT>)
        m(r)(c) = v;
      else
        {
          // Avoid materializing zero entries in sparse matrices.
          // Only write if value is non-zero or if entry already exists.
          if (v != 0 or m.read_ne(r, c) != 0)
            m.write(r, c, v);
        }
    }

    static void swap_rows(MatrixT & m, size_t r1, size_t r2)
    {
      if constexpr (DenseMatrix<MatrixT>)
        std::swap(m(r1), m(r2));
      else
        {
          const size_t cols = m.cols();
          for (size_t j = 0; j < cols; ++j)
            {
              const uint64_t v1 = m.read_ne(r1, j);
              const uint64_t v2 = m.read_ne(r2, j);

              // Avoid redundant writes that can densify sparse backends
              if (v1 == v2)
                continue;

              set_val(m, r1, j, v2);
              set_val(m, r2, j, v1);
            }
        }
    }

  public:
    /** @brief Construct a modular matrix from an existing matrix and a prime modulus.
     *
     *  Initializes the internal matrix and ensures all elements are normalized
     *  within the range [0, p-1].
     *
     *  @param[in] m Source matrix data.
     *  @param[in] p Prime modulus.
     *  @throws std::invalid_argument if p is not prime or if m is a ragged Array<Array>.
     */
    Modular_Matrix(const MatrixT & m, const uint64_t p)
      : mat(m), mod(p)
    {
      ah_invalid_argument_if(not miller_rabin(mod))
        << "Modular_Matrix: modulus " << mod << " must be prime";

      // Normalize initial values
      if constexpr (DenseMatrix<MatrixT>)
        {
          const auto [n_rows, n_cols] = matrix_dims(mat);
          if (n_rows > 0)
            {
              for (size_t i = 0; i < n_rows; ++i)
                {
                  ah_invalid_argument_if(mat[i].size() != n_cols)
                    << "Modular_Matrix: ragged matrix input is not supported";
                  for (size_t j = 0; j < n_cols; ++j)
                    mat(i)(j) %= mod;
                }
            }
        }
      else // SparseMatrix
        {
          // Use traverse_allocated to efficiently normalize only stored entries
          mat.traverse_allocated([this](uint64_t & val)
                                   {
                                     val %= mod;
                                     return true;
                                   });
        }
    }

  private:
    // Internal constructor to skip validation when data is already normalized
    struct skip_validation_t {};
    Modular_Matrix(MatrixT && m, uint64_t p, skip_validation_t)
      : mat(std::move(m)), mod(p) {}

  public:

    /** @brief Get a constant reference to the underlying matrix.
     *  @return Constant reference to the matrix data.
     *  @note Thread-safety: Safe for concurrent reads.
     */
    const MatrixT &get() const noexcept { return mat; }

    /** @brief Computes the determinant of the matrix modulo p.
     *
     *  Uses Gaussian elimination to compute the determinant in O(n^3) time.
     *
     *  @pre Matrix must be non-empty and square.
     *  @return The determinant of the matrix modulo p.
     *  @post Returns determinant modulo p.
     *  @throws std::domain_error if the matrix is not square or is empty.
     */
    [[nodiscard]] uint64_t determinant() const
    {
      const auto [n_rows, n_cols] = matrix_dims(mat);

      ah_domain_error_if(n_rows == 0 or n_rows != n_cols)
          << "Determinant requires a square matrix";

      MatrixT a = mat;
      uint64_t det = 1;

      for (size_t i = 0; i < n_rows; ++i)
        {
          size_t pivot = i;
          if (get_val(a, i, i) == 0)
            {
              for (size_t j = i + 1; j < n_rows; ++j)
                if (get_val(a, j, i) != 0)
                  {
                    pivot = j;
                    break;
                  }
            }

          if (get_val(a, pivot, i) == 0)
            return 0; // Singular matrix

          if (i != pivot)
            {
              swap_rows(a, i, pivot);
              det = (mod - det) % mod;
            }

          const uint64_t diag = get_val(a, i, i);
          det = mod_mul(det, diag, mod);
          const uint64_t inv = mod_inv(diag, mod);

          for (size_t j = i + 1; j < n_rows; ++j)
            if (const uint64_t val_ji = get_val(a, j, i); val_ji != 0)
              {
                uint64_t factor = mod_mul(val_ji, inv, mod);
                for (size_t k = i; k < n_cols; ++k)
                  if (const uint64_t val_ik = get_val(a, i, k); val_ik != 0)
                    {
                      const uint64_t term = mod_mul(factor, val_ik, mod);
                      const uint64_t current = get_val(a, j, k);
                      const uint64_t res = (current >= term) ? (current - term) : (mod - (term - current));
                      set_val(a, j, k, res);
                    }
              }
        }
      return det;
    }

    /** @brief Computes the inverse of the matrix modulo p.
     *
     *  Uses Gauss-Jordan elimination to find the inverse in O(n^3) time.
     *
     *  @pre Matrix must be non-empty and square.
     *  @return A new Modular_Matrix containing the inverse, or std::nullopt if the
     *          matrix is singular (not invertible).
     *  @post Returns inverse matrix modulo p or std::nullopt if matrix is
     *        singular (not invertible).
     *  @throws std::domain_error if the matrix is not square or is empty.
     */
    [[nodiscard]] std::optional<Modular_Matrix<MatrixT>> inverse() const
    {
      const auto [n_rows, n_cols] = matrix_dims(mat);

      ah_domain_error_if(n_rows == 0 or n_rows != n_cols)
          << "Inverse requires a square matrix";

      MatrixT a = mat;
      MatrixT inv;

      if constexpr (DenseMatrix<MatrixT>)
        {
          inv.reserve(n_rows);
          for (size_t i = 0; i < n_rows; ++i)
            {
              Array<uint64_t> row;
              row.reserve(n_rows);
              for (size_t j = 0; j < n_rows; ++j)
                row.append(i == j ? 1 : 0);
              inv.append(std::move(row));
            }
        }
      else
        {
          inv.set_dimension(n_rows, n_cols);
          for (size_t i = 0; i < n_rows; ++i)
            inv.write(i, i, 1);
        }

      for (size_t i = 0; i < n_rows; ++i)
        {
          size_t pivot = i;
          if (get_val(a, i, i) == 0)
            {
              for (size_t j = i + 1; j < n_rows; ++j)
                if (get_val(a, j, i) != 0)
                  {
                    pivot = j;
                    break;
                  }
            }

          if (get_val(a, pivot, i) == 0)
            return std::nullopt; // Singular matrix

          if (i != pivot)
            {
              swap_rows(a, i, pivot);
              swap_rows(inv, i, pivot);
            }

          const uint64_t pivot_val = get_val(a, i, i);
          const uint64_t pivot_inv = mod_inv(pivot_val, mod);
          for (size_t j = 0; j < n_rows; ++j)
            {
              if (const uint64_t val = get_val(a, i, j); val != 0)
                set_val(a, i, j, mod_mul(val, pivot_inv, mod));
              if (const uint64_t val = get_val(inv, i, j); val != 0)
                set_val(inv, i, j, mod_mul(val, pivot_inv, mod));
            }

          for (size_t j = 0; j < n_rows; ++j)
            if (i != j)
              if (uint64_t factor = get_val(a, j, i); factor != 0)
                for (size_t k = 0; k < n_rows; ++k)
                  {
                    if (const uint64_t val_ik = get_val(a, i, k); val_ik != 0)
                      {
                        const uint64_t term_a = mod_mul(factor, val_ik, mod);
                        const uint64_t curr_a = get_val(a, j, k);
                        set_val(a, j, k, (curr_a >= term_a) ? (curr_a - term_a) : (mod - (term_a - curr_a)));
                      }

                    if (const uint64_t val_inv_ik = get_val(inv, i, k); val_inv_ik != 0)
                      {
                        const uint64_t term_inv = mod_mul(factor, val_inv_ik, mod);
                        const uint64_t curr_inv = get_val(inv, j, k);
                        set_val(inv, j, k, (curr_inv >= term_inv) ? (curr_inv - term_inv) : (mod - (term_inv - curr_inv)));
                      }
                  }
        }

      return Modular_Matrix<MatrixT>(std::move(inv), mod, skip_validation_t{});
    }
  };

  /** @brief Alias for common Array-based modular matrix.
   *
   *  Recommended for dense matrices where O(1) access is prioritized over
   *  memory efficiency for zeros.
   */
  using ModularMatrix = Modular_Matrix<Array<Array<uint64_t>>>;

  /** @brief Alias for sparse DynMatrix-based modular matrix.
   *
   *  Recommended for large sparse matrices where unwritten entries (zeros)
   *  should not consume memory.
   */
  using ModularSparseMatrix = Modular_Matrix<DynMatrix<uint64_t>>;
} // namespace Aleph

# endif // MODULAR_LINALG_H
