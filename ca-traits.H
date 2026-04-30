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


/** @file ca-traits.H
 *  @brief Common typedefs and tag types for the Cellular Automata module.
 *
 *  Lives in `namespace Aleph::CA`. Provides:
 *
 *    - Index/coordinate aliases (`ca_index_t`, `ca_size_t`).
 *    - Boundary policy tag types (`OpenBoundary`, `ToroidalBoundary`,
 *      `ReflectiveBoundary`, `ConstantBoundary<v>`).
 *    - Iteration order tag types (`RowMajor`, `ColumnMajor`).
 *
 *  No code here depends on a particular storage or rule layout, so it
 *  is the natural shared header for every later phase of the CA module.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

# ifndef CA_TRAITS_H
# define CA_TRAITS_H

# include <array>
# include <cstddef>

namespace Aleph
{
namespace CA
{

  /// Signed coordinate component used by lattices and neighborhoods.
  /// Signed because neighborhoods produce negative offsets.
  using ca_index_t = std::ptrdiff_t;

  /// Unsigned size component used for extents and counts.
  using ca_size_t  = std::size_t;

  /** @brief Default coordinate vector.
   *  @tparam N number of axes.
   */
  template <std::size_t N>
  using Coord_Vec = std::array<ca_index_t, N>;

  // -----------------------------------------------------------------------
  // Boundary policies
  // -----------------------------------------------------------------------

  /// Out-of-range neighbours behave as if the lattice ended.
  struct OpenBoundary {};

  /// The lattice wraps around on every axis.
  struct ToroidalBoundary {};

  /// Out-of-range coordinates mirror back into the lattice.
  struct ReflectiveBoundary {};

  /** @brief Constant-value boundary. Out-of-range cells take value `V`.
   *  @tparam T cell state type.
   *  @tparam V constant value used outside the lattice.
   */
  template <typename T, T V>
  struct ConstantBoundary
  {
    using state_type = T;
    static constexpr T value = V;
  };

  // -----------------------------------------------------------------------
  // Iteration order tags
  // -----------------------------------------------------------------------

  /// Iterate the lattice in row-major (C) order.
  struct RowMajor {};

  /// Iterate the lattice in column-major (Fortran) order.
  struct ColumnMajor {};

} // namespace CA
} // namespace Aleph

# endif // CA_TRAITS_H