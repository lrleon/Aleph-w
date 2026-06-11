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

/** @file tpl_ca_storage.H
 *  @brief Dense, contiguous storage for cellular automata cells (1D/2D/3D).
 *
 *  Provides `Aleph::CA::Dense_Cell_Storage<T, N>`, a row-major contiguous
 *  container that satisfies the `LatticeLike` concept of
 *  `tpl_ca_concepts.H` (it can be used directly as the `current` /
 *  `next` buffer of an engine, or wrapped by `Lattice<Storage,
 *  Boundary>`).
 *  The implementation is built on top of `Aleph::Array<T>`.
 *
 *  Public surface:
 *
 *    - `state_type`, `coord_type`, `extents_type`, `rank`.
 *    - `dimension()`, `size()`, `size(d)`, `extents()`.
 *    - `linear_index(coord)`.
 *    - `at(coord)`, `at(i)`, `at(i,j)`, `at(i,j,k)`.
 *    - `set(coord, v)`.
 *    - `data()`, `data() const`.
 *    - `swap(other)` (O(1)), `fill(value)`.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_STORAGE_H
#define TPL_CA_STORAGE_H

#include <cstddef>
#include <utility>

#include <ah-errors.H>
#include <tpl_array.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Row-major dense storage for `N`-dimensional cellular automata.
 *
 *  Stores `extents()[0] * extents()[1] * ... * extents()[N-1]` cells of
 *  type `T` in a single contiguous buffer. Indexing is row-major (axis
 *  `N-1` varies fastest), matching the convention of C arrays and of
 *  `al-matrix.H`.
 *
 *  The class satisfies the `CA::LatticeLike` concept directly: it
 *  exposes `dimension()`, `size(d)`, `at(coord)` and `set(coord, v)`.
 *  Higher-level boundary handling is layered on top by
 *  `Lattice<Storage, Boundary>` in `tpl_ca_lattice.H`.
 *
 *  @tparam T cell state type. Must satisfy `CA::CellState`.
 *  @tparam N number of axes (1, 2 or 3 are the typical values).
 *
 *  @par Complexity
 *  - `at`, `set`, `linear_index`, `data`, `size`, `extents`: O(1).
 *  - `swap`: O(1) (swaps the internal `Array<T>` pointers).
 *  - `fill`: O(size()).
 *
 *  @par Thread-safety
 *  Concurrent reads on different cells are safe. Concurrent writes
 *  to overlapping cells are not synchronised — engines are expected
 *  to use a double-buffer pattern.
 */
template <typename T, std::size_t N>
class Dense_Cell_Storage
{
  static_assert(N >= 1, "Dense_Cell_Storage requires N >= 1");
  static_assert(CellState<T>, "Dense_Cell_Storage requires a CellState type");

public:
  using state_type = T;
  using coord_type = Coord_Vec<N>;
  using extents_type = std::array<ca_size_t, N>;

  /// Number of axes known at compile time.
  static constexpr std::size_t rank = N;

private:
  extents_type ext_{};         ///< per-axis extents
  ca_size_t total_cells_ = 0;  ///< product of `ext`
  Array<T> buffer_;            ///< contiguous row-major storage

  static ca_size_t product(const extents_type &e)
  {
    ca_size_t p = 1;
    for (std::size_t d = 0; d < N; ++d)
      {
        ah_overflow_error_if(e[d] != 0 and p > std::numeric_limits<ca_size_t>::max() / e[d])
          << "Dense_Cell_Storage: extent product overflows ca_size_t at dimension " << d;
        p *= e[d];
      }
    return p;
  }

public:
  /// Construct an empty storage (zero extents, no cells allocated).
  Dense_Cell_Storage() = default;

  /** @brief Construct a storage with the given extents and initial value.
   *
   *  Allocates and value-initialises `prod(extents)` cells of type `T`,
   *  each set to `init`.
   *
   *  @param[in] extents per-axis sizes.
   *  @param[in] init initial value for every cell (default: `T{}`).
   *  @throws std::bad_alloc on allocation failure.
   */
  explicit Dense_Cell_Storage(const extents_type &extents, const T &init = T{})
    : ext_(extents), total_cells_(product(extents)), buffer_(total_cells_, init)
  {}

  /// @return the number of axes (compile-time constant).
  [[nodiscard]] static constexpr std::size_t dimension() noexcept
  {
    return N;
  }

  /// @return total cell count (product of extents).
  [[nodiscard]] ca_size_t size() const noexcept
  {
    return total_cells_;
  }

  /** @brief Extent of axis `d`.
   *  @param[in] d axis index in `[0, N)`.
   *  @throws std::out_of_range if `d >= N`.
   */
  [[nodiscard]] ca_size_t size(std::size_t d) const
  {
    ah_out_of_range_error_if(d >= N) << "Dense_Cell_Storage::size: axis " << d << " out of range";
    return ext_[d];
  }

  /// @return the per-axis extents array.
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return ext_;
  }

  /** @brief Map a multi-index to a linear offset.
   *
   *  Layout is row-major: the last axis varies fastest. All components
   *  of `c` must be non-negative and within range; this method does
   *  the bounds check and throws on violation.
   *
   *  @param[in] c multi-index.
   *  @return linear offset in `[0, size())`.
   *  @throws std::out_of_range if any component is out of range.
   */
  [[nodiscard]] ca_size_t linear_index(const coord_type &c) const
  {
    ca_size_t idx = 0;
    for (std::size_t d = 0; d < N; ++d)
      {
        ah_out_of_range_error_if(c[d] < 0 or static_cast<ca_size_t>(c[d]) >= ext_[d])
          << "Dense_Cell_Storage::linear_index: coord[" << d << "]=" << c[d] << " out of [0, "
          << ext_[d] << ")";
        idx = idx * ext_[d] + static_cast<ca_size_t>(c[d]);
      }
    return idx;
  }

  /** @brief Read access through a multi-index.
   *  @param[in] c multi-index, in range.
   *  @return value stored at `c`.
   *  @throws std::out_of_range if `c` is out of range.
   */
  [[nodiscard]] T at(const coord_type &c) const
  {
    return buffer_[linear_index(c)];
  }

  /** @brief Write access through a multi-index.
   *  @param[in] c multi-index, in range.
   *  @param[in] v new value.
   *  @throws std::out_of_range if `c` is out of range.
   */
  void set(const coord_type &c, const T &v)
  {
    buffer_[linear_index(c)] = v;
  }

  /// 1D shorthand for `at({i})`.
  [[nodiscard]] T at(ca_index_t i) const
    requires(N == 1)
  {
    return at(coord_type{i});
  }

  /// 2D shorthand for `at({i, j})`.
  [[nodiscard]] T at(ca_index_t i, ca_index_t j) const
    requires(N == 2)
  {
    return at(coord_type{i, j});
  }

  /// 3D shorthand for `at({i, j, k})`.
  [[nodiscard]] T at(ca_index_t i, ca_index_t j, ca_index_t k) const
    requires(N == 3)
  {
    return at(coord_type{i, j, k});
  }

  /// 1D shorthand for `set({i}, v)`.
  void set(ca_index_t i, const T &v)
    requires(N == 1)
  {
    set(coord_type{i}, v);
  }

  /// 2D shorthand for `set({i, j}, v)`.
  void set(ca_index_t i, ca_index_t j, const T &v)
    requires(N == 2)
  {
    set(coord_type{i, j}, v);
  }

  /// 3D shorthand for `set({i, j, k}, v)`.
  void set(ca_index_t i, ca_index_t j, ca_index_t k, const T &v)
    requires(N == 3)
  {
    set(coord_type{i, j, k}, v);
  }

  /// @return mutable pointer to the first cell (or `nullptr` if empty).
  [[nodiscard]] T *data() noexcept
  {
    return total_cells_ == 0 ? nullptr : &buffer_[0];
  }

  /// @return read-only pointer to the first cell (or `nullptr` if empty).
  [[nodiscard]] const T *data() const noexcept
  {
    return total_cells_ == 0 ? nullptr : &buffer_[0];
  }

  /// O(1) swap with another storage of the same type.
  void swap(Dense_Cell_Storage &other) noexcept
  {
    using std::swap;
    swap(ext_, other.ext_);
    swap(total_cells_, other.total_cells_);
    buffer_.swap(other.buffer_);
  }

  /// Set every cell to `value`.
  void fill(const T &value)
  {
    for (ca_size_t i = 0; i < total_cells_; ++i)
      buffer_[i] = value;
  }
};

/// Free-function swap so the storage plays nicely with std::swap.
template <typename T, std::size_t N>
inline void swap(Dense_Cell_Storage<T, N> &a, Dense_Cell_Storage<T, N> &b) noexcept
{
  a.swap(b);
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_STORAGE_H