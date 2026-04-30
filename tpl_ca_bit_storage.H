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

/** @file tpl_ca_bit_storage.H
 *  @brief Bit-packed dense storage for boolean cellular automata.
 *
 *  Provides `Aleph::CA::Bit_Cell_Storage<N>` — a `1 bit/cell` row-major
 *  container backed by `Aleph::BitArray`. Useful for binary CAs (Game
 *  of Life, Wolfram 1D, Brian's Brain etc.) where memory and cache
 *  footprint matter.
 *
 *  The class mirrors the read/write surface of
 *  `Dense_Cell_Storage<bool, N>` so that lattices and engines can be
 *  parametrised on either backend transparently. The only API
 *  difference is the absence of a raw `data()` pointer: bit packing
 *  precludes a contiguous `bool*` view.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_BIT_STORAGE_H
#define TPL_CA_BIT_STORAGE_H

#include <cstddef>
#include <utility>

#include <ah-errors.H>
#include <bitArray.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Bit-packed row-major storage for `N`-dimensional binary CAs.
 *
 *  Stores cells as 1-bit values inside an `Aleph::BitArray`. Linear
 *  layout is row-major (axis `N-1` varies fastest), identical to
 *  `Dense_Cell_Storage<bool, N>`. Reading and writing are done with
 *  `bool` to keep the API uniform.
 *
 *  @tparam N number of axes (1, 2 or 3 are the typical values).
 */
template <std::size_t N>
class Bit_Cell_Storage
{
  static_assert(N >= 1, "Bit_Cell_Storage requires N >= 1");

public:
  using state_type = bool;
  using coord_type = Coord_Vec<N>;
  using extents_type = std::array<ca_size_t, N>;

  static constexpr std::size_t rank = N;

private:
  extents_type ext_{};
  ca_size_t total_cells_ = 0;
  BitArray buffer_;

  static ca_size_t product(const extents_type &e) noexcept
  {
    ca_size_t p = 1;
    for (std::size_t d = 0; d < N; ++d)
      p *= e[d];
    return p;
  }

public:
  /// Construct an empty storage (zero extents).
  Bit_Cell_Storage() = default;

  /** @brief Construct a storage with the given extents and initial value.
   *
   *  @param[in] extents per-axis sizes.
   *  @param[in] init initial value for every cell (default `false`).
   */
  explicit Bit_Cell_Storage(const extents_type &extents, bool init = false)
    : ext_(extents), total_cells_(product(extents)), buffer_(total_cells_, init ? 1u : 0u)
  {}

  [[nodiscard]] constexpr std::size_t dimension() const noexcept
  {
    return N;
  }

  [[nodiscard]] ca_size_t size() const noexcept
  {
    return total_cells_;
  }

  [[nodiscard]] ca_size_t size(std::size_t d) const
  {
    ah_out_of_range_error_if(d >= N) << "Bit_Cell_Storage::size: axis " << d << " out of range";
    return ext_[d];
  }

  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return ext_;
  }

  /** @brief Map a multi-index to a linear bit offset (row-major).
   *  @throws std::out_of_range if any component is out of range.
   */
  [[nodiscard]] ca_size_t linear_index(const coord_type &c) const
  {
    ca_size_t idx = 0;
    for (std::size_t d = 0; d < N; ++d)
      {
        ah_out_of_range_error_if(c[d] < 0 or static_cast<ca_size_t>(c[d]) >= ext_[d])
          << "Bit_Cell_Storage::linear_index: coord[" << d << "]=" << c[d] << " out of [0, "
          << ext_[d] << ")";
        idx = idx * ext_[d] + static_cast<ca_size_t>(c[d]);
      }
    return idx;
  }

  /// Read the cell at `c`.
  [[nodiscard]] bool at(const coord_type &c) const
  {
    return buffer_.read_bit(linear_index(c)) != 0;
  }

  /// Write the cell at `c`.
  void set(const coord_type &c, bool v)
  {
    buffer_.write_bit(linear_index(c), v ? 1u : 0u);
  }

  [[nodiscard]] bool at(ca_index_t i) const
    requires(N == 1)
  {
    return at(coord_type{i});
  }

  [[nodiscard]] bool at(ca_index_t i, ca_index_t j) const
    requires(N == 2)
  {
    return at(coord_type{i, j});
  }

  [[nodiscard]] bool at(ca_index_t i, ca_index_t j, ca_index_t k) const
    requires(N == 3)
  {
    return at(coord_type{i, j, k});
  }

  void set(ca_index_t i, bool v)
    requires(N == 1)
  {
    set(coord_type{i}, v);
  }

  void set(ca_index_t i, ca_index_t j, bool v)
    requires(N == 2)
  {
    set(coord_type{i, j}, v);
  }

  void set(ca_index_t i, ca_index_t j, ca_index_t k, bool v)
    requires(N == 3)
  {
    set(coord_type{i, j, k}, v);
  }

  /// O(1) swap.
  void swap(Bit_Cell_Storage &other) noexcept
  {
    using std::swap;
    swap(ext_, other.ext_);
    swap(total_cells_, other.total_cells_);
    buffer_.swap(other.buffer_);
  }

  /// Set every cell to `value`.
  void fill(bool value)
  {
    const unsigned int v = value ? 1u : 0u;
    for (ca_size_t i = 0; i < total_cells_; ++i)
      buffer_.write_bit(i, v);
  }
};

template <std::size_t N>
inline void swap(Bit_Cell_Storage<N> &a, Bit_Cell_Storage<N> &b) noexcept
{
  a.swap(b);
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_BIT_STORAGE_H