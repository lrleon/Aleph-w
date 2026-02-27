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

#ifndef MODULAR_LINALG_H
#define MODULAR_LINALG_H

#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <ah-errors.H>
#include <tpl_array.H>
#include <tpl_dynMat.H>
#include "modular_arithmetic.H"

namespace Aleph
{
  /** @brief Matrix operations modulo a prime.
   *  @tparam MatrixT The matrix container type.
   */
  template <typename MatrixT>
  class Modular_Matrix
  {
    MatrixT mat;
    uint64_t mod;

  public:
    Modular_Matrix(const MatrixT & m, const uint64_t p)
      : mat(m), mod(p)
    {
      // Normalize initial values
      if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
        {
          for (size_t i = 0; i < mat.size(); ++i)
            for (size_t j = 0; j < mat[i].size(); ++j)
              mat(i)(j) %= mod;
        }
      else // assume DynMatrix style
        {
          for (size_t i = 0; i < mat.rows(); ++i)
            for (size_t j = 0; j < mat.cols(); ++j)
              if (const uint64_t val = mat.read_ne(i, j); val != 0)
                mat.write(i, j, val % mod);
        }
    }

    /** @brief Get the underlying matrix. */
    const MatrixT &get() const { return mat; }

    /** @brief Computes the determinant of the matrix modulo p.
     *  @return The determinant.
     */
    [[nodiscard]] uint64_t determinant() const
    {
      size_t n_rows, n_cols;
      if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
        {
          n_rows = mat.size();
          n_cols = n_rows > 0 ? mat[0].size() : 0;
        }
      else
        {
          n_rows = mat.rows();
          n_cols = mat.cols();
        }

      ah_domain_error_if(n_rows == 0 or n_rows != n_cols)
          << "Determinant requires a square matrix";

      MatrixT a = mat;
      uint64_t det = 1;

      auto get_val = [](const MatrixT & m, size_t r, size_t c) -> uint64_t
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            return m[r][c];
          else
            return m.read_ne(r, c);
        };

      auto set_val = [](MatrixT & m, size_t r, size_t c, uint64_t v)
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            m(r)(c) = v;
          else
            m.write(r, c, v);
        };

      auto swap_rows = [](MatrixT & m, size_t r1, size_t r2)
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            std::swap(m(r1), m(r2));
          else
            for (size_t j = 0; j < m.cols(); ++j)
              {
                uint64_t tmp = m.read_ne(r1, j);
                m.write(r1, j, m.read_ne(r2, j));
                m.write(r2, j, tmp);
              }
        };

      for (size_t i = 0; i < n_rows; ++i)
        {
          size_t pivot = i;
          for (size_t j = i + 1; j < n_rows; ++j)
            if (get_val(a, j, i) != 0)
              {
                pivot = j;
                break;
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
          const uint64_t inv = mod_inv(static_cast<int64_t>(diag), static_cast<int64_t>(mod));

          for (size_t j = i + 1; j < n_rows; ++j)
            if (const uint64_t val_ji = get_val(a, j, i); val_ji != 0)
              {
                uint64_t factor = mod_mul(val_ji, inv, mod);
                for (size_t k = i; k < n_cols; ++k)
                  {
                    uint64_t term = mod_mul(factor, get_val(a, i, k), mod);
                    set_val(a, j, k, (get_val(a, j, k) + mod - term) % mod);
                  }
              }
        }
      return det;
    }

    /** @brief Computes the inverse of the matrix modulo p.
     *  @return A new Modular_Matrix representing the inverse, or std::nullopt if singular.
     */
    [[nodiscard]] std::optional<Modular_Matrix<MatrixT>> inverse() const
    {
      size_t n_rows, n_cols;
      if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
        {
          n_rows = mat.size();
          n_cols = n_rows > 0 ? mat[0].size() : 0;
        }
      else
        {
          n_rows = mat.rows();
          n_cols = mat.cols();
        }

      ah_domain_error_if(n_rows == 0 or n_rows != n_cols)
          << "Inverse requires a square matrix";

      MatrixT a = mat;
      MatrixT inv;

      if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
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

      auto get_val = [](const MatrixT & m, size_t r, size_t c) -> uint64_t
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            return m[r][c];
          else
            return m.read_ne(r, c);
        };

      auto set_val = [](MatrixT & m, size_t r, size_t c, uint64_t v)
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            m(r)(c) = v;
          else
            m.write(r, c, v);
        };

      auto swap_rows = [](MatrixT & m, size_t r1, size_t r2)
        {
          if constexpr (std::is_same_v<MatrixT, Array<Array<uint64_t>>>)
            std::swap(m(r1), m(r2));
          else
            for (size_t j = 0; j < m.cols(); ++j)
              {
                uint64_t tmp = m.read_ne(r1, j);
                m.write(r1, j, m.read_ne(r2, j));
                m.write(r2, j, tmp);
              }
        };

      for (size_t i = 0; i < n_rows; ++i)
        {
          size_t pivot = i;
          for (size_t j = i + 1; j < n_rows; ++j)
            if (get_val(a, j, i) != 0)
              {
                pivot = j;
                break;
              }

          if (get_val(a, pivot, i) == 0)
            return std::nullopt; // Singular matrix

          if (i != pivot)
            {
              swap_rows(a, i, pivot);
              swap_rows(inv, i, pivot);
            }

          const uint64_t pivot_val = get_val(a, i, i);
          uint64_t pivot_inv = mod_inv(static_cast<int64_t>(pivot_val), static_cast<int64_t>(mod));
          for (size_t j = 0; j < n_rows; ++j)
            {
              set_val(a, i, j, mod_mul(get_val(a, i, j), pivot_inv, mod));
              set_val(inv, i, j, mod_mul(get_val(inv, i, j), pivot_inv, mod));
            }

          for (size_t j = 0; j < n_rows; ++j)
            if (i != j)
              if (uint64_t factor = get_val(a, j, i); factor != 0)
                for (size_t k = 0; k < n_rows; ++k)
                  {
                    uint64_t term_a = mod_mul(factor, get_val(a, i, k), mod);
                    set_val(a, j, k, (get_val(a, j, k) + mod - term_a) % mod);

                    uint64_t term_inv = mod_mul(factor, get_val(inv, i, k), mod);
                    set_val(inv, j, k, (get_val(inv, j, k) + mod - term_inv) % mod);
                  }
        }

      return Modular_Matrix<MatrixT>(inv, mod);
    }
  };

  /** @brief Alias for common Array-based modular matrix. */
  using ModularMatrix = Modular_Matrix<Array<Array<uint64_t>>>;

  /** @brief Alias for sparse DynMatrix-based modular matrix. */
  using ModularSparseMatrix = Modular_Matrix<DynMatrix<uint64_t>>;
} // namespace Aleph

#endif // MODULAR_LINALG_H
