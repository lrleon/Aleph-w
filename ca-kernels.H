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

/** @file ca-kernels.H
 *  @brief Fixed 2-D convolution kernels for continuous cellular automata.
 *
 *  Phase 9 introduces real-valued neighbourhoods for reaction-diffusion
 *  and image-like CA rules. `Kernel2D<T, Rows, Cols>` stores a small odd
 *  kernel in row-major order and exposes helpers that match the canonical
 *  `Moore<2, R>` neighbour order used by `tpl_ca_neighborhood.H`.
 *
 *  Predefined kernels include 5-point and 9-point Laplacians, mean filters,
 *  Sobel gradients and Gaussian smoothers. Small kernels use a fixed
 *  `std::array` for zero-allocation hot loops; `to_matrix()` exports the
 *  same weights into `Aleph::Matrix<int, int, T>` when sparse matrix tooling
 *  is more convenient for larger analysis pipelines.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_KERNELS_H
#define CA_KERNELS_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include <ah-errors.H>
#include <al-domain.H>
#include <al-matrix.H>

#include <ca-traits.H>

namespace Aleph {
namespace CA {

/** @brief Dense odd-sized 2-D convolution kernel.
 *
 *  Kernel coordinates are stored in row-major order. The centre weight
 *  is `operator()(Rows / 2, Cols / 2)`. Neighbour weights are exposed
 *  through `neighbour_weight(k)` using the same row-major, centre-skipping
 *  order as `Moore<2, radius_v>`, so a rule can combine this kernel with
 *  the neighbour buffer gathered by the standard CA engine without carrying
 *  offsets alongside the values.
 *
 *  @tparam T numeric weight type.
 *  @tparam Rows number of rows; must be odd.
 *  @tparam Cols number of columns; must be odd and equal to Rows for
 *          direct use with `Moore<2, radius_v>`.
 */
template <typename T, std::size_t Rows, std::size_t Cols>
class Kernel2D
{
  static_assert(std::is_arithmetic_v<T>, "Kernel2D requires arithmetic weights");
  static_assert(Rows >= 3 and Cols >= 3, "Kernel2D requires at least 3x3 weights");
  static_assert(Rows % 2 == 1 and Cols % 2 == 1, "Kernel2D dimensions must be odd");
  static_assert(Rows == Cols, "Kernel2D currently expects square kernels");

public:
  /// Numeric weight type.
  using value_type = T;

  /// Number of rows.
  static constexpr std::size_t rows_v = Rows;
  /// Number of columns.
  static constexpr std::size_t cols_v = Cols;
  /// Total number of weights including the centre.
  static constexpr std::size_t size_v = Rows * Cols;
  /// Number of neighbour weights excluding the centre.
  static constexpr std::size_t neighbour_count_v = size_v - 1;
  /// Chebyshev radius matched by `Moore<2, radius_v>`.
  static constexpr std::size_t radius_v = Rows / 2;

private:
  std::array<T, size_v> weights_{};

  [[nodiscard]] static constexpr std::size_t index(const std::size_t row, const std::size_t col) noexcept
  {
    return row * Cols + col;
  }

public:
  /** @brief Construct a zero kernel.
   *
   *  @throws This function does not throw.
   */
  constexpr Kernel2D() = default;

  /** @brief Construct a kernel from row-major weights.
   *
   *  @param[in] weights flat row-major weight array.
   *  @throws This function does not throw.
   */
  constexpr explicit Kernel2D(const std::array<T, size_v> &weights) : weights_(weights) {}

  /** @brief Construct a kernel from a flat initializer list.
   *
   *  @param[in] weights row-major weights; must contain `Rows * Cols`
   *             entries.
   *  @throws std::length_error if the initializer list has the wrong size.
   */
  explicit Kernel2D(std::initializer_list<T> weights)
  {
    ah_length_error_if(weights.size() != size_v)
      << "Kernel2D: expected " << size_v << " weights, got " << weights.size();
    std::size_t i = 0;
    for (const auto &w : weights)
      weights_[i++] = w;
  }

  /** @brief Construct a kernel from nested row initializer lists.
   *
   *  @param[in] rows rows of weights; every row must contain `Cols`
   *             entries and the outer list must contain `Rows` rows.
   *  @throws std::length_error if the shape is not exactly `Rows x Cols`.
   */
  explicit Kernel2D(std::initializer_list<std::initializer_list<T>> rows)
  {
    ah_length_error_if(rows.size() != Rows)
      << "Kernel2D: expected " << Rows << " rows, got " << rows.size();

    std::size_t r = 0;
    for (const auto &row : rows)
      {
        ah_length_error_if(row.size() != Cols)
          << "Kernel2D: expected " << Cols << " columns, got " << row.size();
        std::size_t c = 0;
        for (const auto &w : row)
          weights_[index(r, c++)] = w;
        ++r;
      }
  }

  /** @brief Return a weight by matrix position.
   *
   *  @param[in] row row index in `[0, Rows)`.
   *  @param[in] col column index in `[0, Cols)`.
   *  @return selected kernel weight.
   *  @throws std::out_of_range if either coordinate is out of range.
   */
  [[nodiscard]] T operator()(const std::size_t row, const std::size_t col) const
  {
    ah_out_of_range_error_if(row >= Rows or col >= Cols)
      << "Kernel2D::operator(): position (" << row << ", " << col << ") outside kernel";
    return weights_[index(row, col)];
  }

  /** @brief Return the weight at an offset from the centre.
   *
   *  @param[in] di signed row offset in `[-radius_v, radius_v]`.
   *  @param[in] dj signed column offset in `[-radius_v, radius_v]`.
   *  @return selected kernel weight.
   *  @throws std::out_of_range if the offset is outside the kernel.
   */
  [[nodiscard]] T weight_at_offset(const ca_index_t di, const ca_index_t dj) const
  {
    const ca_index_t r = di + static_cast<ca_index_t>(Rows / 2);
    const ca_index_t c = dj + static_cast<ca_index_t>(Cols / 2);
    ah_out_of_range_error_if(r < 0 or c < 0 or static_cast<std::size_t>(r) >= Rows
                             or static_cast<std::size_t>(c) >= Cols)
      << "Kernel2D::weight_at_offset: offset (" << di << ", " << dj << ") outside radius "
      << radius_v;
    return weights_[index(static_cast<std::size_t>(r), static_cast<std::size_t>(c))];
  }

  /** @brief Return a centre-skipping neighbour weight.
   *
   *  The index follows the canonical `Moore<2, radius_v>` order:
   *  row-major over the square stencil, excluding `(0, 0)`.
   *
   *  @param[in] k neighbour index in `[0, neighbour_count_v)`.
   *  @return selected neighbour weight.
   *  @throws std::out_of_range if `k` is out of range.
   */
  [[nodiscard]] T neighbour_weight(const std::size_t k) const
  {
    ah_out_of_range_error_if(k >= neighbour_count_v)
      << "Kernel2D::neighbour_weight: index " << k << " outside [0, " << neighbour_count_v << ")";

    std::size_t out = 0;
    for (std::size_t r = 0; r < Rows; ++r)
      for (std::size_t c = 0; c < Cols; ++c)
        {
          if (r == Rows / 2 and c == Cols / 2)
            continue;
          if (out == k)
            return weights_[index(r, c)];
          ++out;
        }
    return T{};
  }

  /** @brief Return the centre weight.
   *
   *  @return `operator()(Rows / 2, Cols / 2)`.
   *  @throws This function does not throw.
   */
  [[nodiscard]] constexpr T center() const noexcept
  {
    return weights_[index(Rows / 2, Cols / 2)];
  }

  /** @brief Return the raw row-major weight array.
   *
   *  @return const reference to the stored weights.
   *  @throws This function does not throw.
   */
  [[nodiscard]] constexpr const std::array<T, size_v> &weights() const noexcept
  {
    return weights_;
  }

  /** @brief Sum every kernel weight.
   *
   *  @return arithmetic sum of all weights.
   *  @throws This function does not throw.
   */
  [[nodiscard]] constexpr T sum() const noexcept
  {
    T acc{};
    for (const auto &w : weights_)
      acc += w;
    return acc;
  }

  /** @brief Apply the kernel to centre plus neighbour values.
   *
   *  @tparam State scalar value type convertible to `T`.
   *  @param[in] center_value value at the centre cell.
   *  @param[in] neighbours neighbour values in `Moore<2, radius_v>` order.
   *  @return weighted convolution result in `T`.
   *  @throws std::length_error if `neighbours.size()` is not
   *          `neighbour_count_v`.
   */
  template <typename State>
  [[nodiscard]] T apply(const State &center_value, Neighbor_View<State> neighbours) const
  {
    ah_length_error_if(neighbours.size() != neighbour_count_v)
      << "Kernel2D::apply: expected " << neighbour_count_v << " neighbours, got "
      << neighbours.size();

    T acc = static_cast<T>(center_value) * center();
    std::array<T, neighbour_count_v> neighbour_weights{};
    std::size_t out = 0;
    for (std::size_t r = 0; r < Rows; ++r)
      for (std::size_t c = 0; c < Cols; ++c)
        {
          if (r == Rows / 2 and c == Cols / 2)
            continue;
          neighbour_weights[out++] = weights_[index(r, c)];
        }
    for (std::size_t k = 0; k < neighbour_count_v; ++k)
      acc += static_cast<T>(neighbours[k]) * neighbour_weights[k];
    return acc;
  }

  /** @brief Export the kernel weights as an `Aleph::Matrix`.
   *
   *  The returned sparse matrix uses row domain `[0, Rows - 1]` and
   *  column domain `[0, Cols - 1]`. Zero weights are omitted by the
   *  matrix storage.
   *
   *  @return sparse matrix carrying the same weights.
   *  @throws Any exception thrown by `Matrix` construction or insertion.
   */
  [[nodiscard]] Matrix<int, int, T> to_matrix() const
  {
    auto rd = std::make_shared<IntRange>(static_cast<int>(Rows));
    auto cd = std::make_shared<IntRange>(static_cast<int>(Cols));
    Matrix<int, int, T> ret(rd, cd);
    for (std::size_t r = 0; r < Rows; ++r)
      for (std::size_t c = 0; c < Cols; ++c)
        ret.set_entry(static_cast<int>(r), static_cast<int>(c), weights_[index(r, c)]);
    return ret;
  }
};

/** @brief Return the 5-point discrete Laplacian kernel.
 *
 *  Stencil:
 *
 *  ```
 *  0  1  0
 *  1 -4  1
 *  0  1  0
 *  ```
 *
 *  @tparam T numeric weight type.
 *  @return `Kernel2D<T, 3, 3>` with zero total mass.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> laplacian_5p_kernel() noexcept
{
  return Kernel2D<T, 3, 3>(std::array<T, 9>{T{0}, T{1}, T{0}, T{1}, T{-4}, T{1}, T{0}, T{1}, T{0}});
}

/** @brief Return the isotropic 9-point discrete Laplacian kernel.
 *
 *  Stencil:
 *
 *  ```
 *  1  4  1
 *  4 -20 4  * 1/6
 *  1  4  1
 *  ```
 *
 *  @tparam T numeric weight type.
 *  @return `Kernel2D<T, 3, 3>` with zero total mass.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> laplacian_9p_kernel() noexcept
{
  return Kernel2D<T, 3, 3>(std::array<T, 9>{T{1} / T{6}, T{4} / T{6}, T{1} / T{6}, T{4} / T{6},
                                            T{-20} / T{6}, T{4} / T{6}, T{1} / T{6}, T{4} / T{6},
                                            T{1} / T{6}});
}

/** @brief Return a 3x3 mean filter.
 *
 *  @tparam T numeric weight type.
 *  @return kernel whose weights all equal `1/9`.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> mean_3x3_kernel() noexcept
{
  constexpr T w = T{1} / T{9};
  return Kernel2D<T, 3, 3>(std::array<T, 9>{w, w, w, w, w, w, w, w, w});
}

/** @brief Return a 5x5 mean filter.
 *
 *  @tparam T numeric weight type.
 *  @return kernel whose weights all equal `1/25`.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 5, 5> mean_5x5_kernel() noexcept
{
  constexpr T w = T{1} / T{25};
  return Kernel2D<T, 5, 5>(
    std::array<T, 25>{w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w});
}

/** @brief Return the horizontal Sobel gradient kernel.
 *
 *  @tparam T numeric weight type.
 *  @return `Kernel2D<T, 3, 3>` detecting changes along the column axis.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> sobel_x_kernel() noexcept
{
  return Kernel2D<T, 3, 3>(std::array<T, 9>{T{-1}, T{0}, T{1}, T{-2}, T{0}, T{2}, T{-1}, T{0}, T{1}});
}

/** @brief Return the vertical Sobel gradient kernel.
 *
 *  @tparam T numeric weight type.
 *  @return `Kernel2D<T, 3, 3>` detecting changes along the row axis.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> sobel_y_kernel() noexcept
{
  return Kernel2D<T, 3, 3>(std::array<T, 9>{T{-1}, T{-2}, T{-1}, T{0}, T{0}, T{0}, T{1}, T{2}, T{1}});
}

/** @brief Return the separable 3x3 Gaussian blur kernel.
 *
 *  @tparam T numeric weight type.
 *  @return normalized Gaussian-like kernel with total mass 1.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 3, 3> gaussian_3x3_kernel() noexcept
{
  return Kernel2D<T, 3, 3>(std::array<T, 9>{T{1} / T{16}, T{2} / T{16}, T{1} / T{16}, T{2} / T{16},
                                            T{4} / T{16}, T{2} / T{16}, T{1} / T{16}, T{2} / T{16},
                                            T{1} / T{16}});
}

/** @brief Return the separable 5x5 Gaussian blur kernel.
 *
 *  @tparam T numeric weight type.
 *  @return normalized binomial kernel with total mass 1.
 *  @throws This function does not throw.
 */
template <typename T = double>
[[nodiscard]] constexpr Kernel2D<T, 5, 5> gaussian_5x5_kernel() noexcept
{
  return Kernel2D<T, 5, 5>(std::array<T, 25>{
    T{1} / T{256}, T{4} / T{256},  T{6} / T{256},  T{4} / T{256},  T{1} / T{256},
    T{4} / T{256}, T{16} / T{256}, T{24} / T{256}, T{16} / T{256}, T{4} / T{256},
    T{6} / T{256}, T{24} / T{256}, T{36} / T{256}, T{24} / T{256}, T{6} / T{256},
    T{4} / T{256}, T{16} / T{256}, T{24} / T{256}, T{16} / T{256}, T{4} / T{256},
    T{1} / T{256}, T{4} / T{256},  T{6} / T{256},  T{4} / T{256},  T{1} / T{256}});
}

}  // namespace CA
}  // namespace Aleph

#endif  // CA_KERNELS_H
