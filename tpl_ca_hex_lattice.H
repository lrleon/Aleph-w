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

/** @file tpl_ca_hex_lattice.H
 *  @brief Hexagonal lattice with axial / offset / cube coordinate
 *         conversions and a TikZ-compatible pixel mapping.
 *
 *  Hexagonal CAs are conventionally described in *axial* coordinates
 *  `(q, r)`: two integer axes selected so that neighbour offsets are
 *  symmetric (six unit steps, including two diagonals). Storing the
 *  cells as a regular 2D row-major buffer indexed by `(r, q)` matches
 *  the rectangular bounding parallelogram of the hex grid; visualizers
 *  switch to *offset* coordinates `(col, row)` (even-r or odd-r) where
 *  alternate rows are shifted by half a hex.
 *
 *  This header does three things:
 *
 *    - Defines `Hex_Axial`, `Hex_Offset`, `Hex_Cube` value types and
 *      free functions to convert between them. All conversions are
 *      lossless and round-trip exactly for valid inputs.
 *    - Provides `Hex_Lattice<Storage, Boundary>`, a thin subclass of
 *      `Lattice<Storage, Boundary>` that adds axial/offset accessors
 *      and a hexagonal pixel mapping. The 2D rank is preserved, so
 *      the same `Synchronous_Engine` / `Parallel_Synchronous_Engine`
 *      and the existing `Hex_Neighborhood` (six axial offsets) work
 *      unchanged.
 *    - Provides `render_hex_lattice_tikz` to dump a frame as a TikZ
 *      picture for inclusion in reports / papers.
 *
 *  Storage convention: coord `c = {r, q}`. Axis 0 stores `r` (axial
 *  row), axis 1 stores `q` (axial column). This matches the standard
 *  CA convention where axis 0 is the slowest-varying axis, and keeps
 *  the data layout cache-friendly for row-by-row iteration.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_HEX_LATTICE_H
#define TPL_CA_HEX_LATTICE_H

#include <array>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_lattice.H>

namespace Aleph {
namespace CA {

/** @brief Axial integer coordinates `(q, r)` of a hex cell.
 *
 *  `q` selects the column along the `+x` direction; `r` selects the
 *  row along the `+y` direction tilted 60 degrees from `q`. Six
 *  neighbours sit at canonical offsets
 *
 *      (+1, 0), (-1, 0), (0, +1), (0, -1), (+1, -1), (-1, +1)
 *
 *  which is exactly the offset list used by `Hex_Neighborhood`.
 */
struct Hex_Axial
{
  ca_index_t q = 0;  ///< column index
  ca_index_t r = 0;  ///< row index

  friend constexpr bool operator==(Hex_Axial a, Hex_Axial b) noexcept
  {
    return a.q == b.q and a.r == b.r;
  }
  friend constexpr bool operator!=(Hex_Axial a, Hex_Axial b) noexcept { return not(a == b); }
};

/** @brief Offset coordinates `(col, row)` for visualisation.
 *
 *  Even-r and odd-r conventions are supported through dedicated
 *  conversions. Use the offset coords when you need to print the
 *  hex grid as a rectangular table where every other row is shifted
 *  by half a column.
 */
struct Hex_Offset
{
  ca_index_t col = 0;
  ca_index_t row = 0;

  friend constexpr bool operator==(Hex_Offset a, Hex_Offset b) noexcept
  {
    return a.col == b.col and a.row == b.row;
  }
  friend constexpr bool operator!=(Hex_Offset a, Hex_Offset b) noexcept { return not(a == b); }
};

/** @brief Cube coordinates `(x, y, z)` with the constraint `x+y+z = 0`.
 *
 *  Cube coordinates make several geometric operations trivial (e.g.
 *  Euclidean distance, rotations by 60°). They are the canonical
 *  representation used by Red Blob Games and by most academic
 *  references on hex grids.
 */
struct Hex_Cube
{
  ca_index_t x = 0;
  ca_index_t y = 0;
  ca_index_t z = 0;

  friend constexpr bool operator==(Hex_Cube a, Hex_Cube b) noexcept
  {
    return a.x == b.x and a.y == b.y and a.z == b.z;
  }
  friend constexpr bool operator!=(Hex_Cube a, Hex_Cube b) noexcept { return not(a == b); }
};

// ---------------------------------------------------------------------------
// Coordinate conversions.
// ---------------------------------------------------------------------------

/// Axial -> offset (even-r convention: even rows are shifted right).
[[nodiscard]] inline constexpr Hex_Offset axial_to_offset_even_r(Hex_Axial a) noexcept
{
  // even-r: col = q + (r + (r & 1)) / 2
  const ca_index_t col = a.q + (a.r + (a.r & 1)) / 2;
  return Hex_Offset{col, a.r};
}

/// Offset (even-r) -> axial.
[[nodiscard]] inline constexpr Hex_Axial offset_to_axial_even_r(Hex_Offset o) noexcept
{
  const ca_index_t q = o.col - (o.row + (o.row & 1)) / 2;
  return Hex_Axial{q, o.row};
}

/// Axial -> offset (odd-r convention: odd rows are shifted right).
[[nodiscard]] inline constexpr Hex_Offset axial_to_offset_odd_r(Hex_Axial a) noexcept
{
  const ca_index_t col = a.q + (a.r - (a.r & 1)) / 2;
  return Hex_Offset{col, a.r};
}

/// Offset (odd-r) -> axial.
[[nodiscard]] inline constexpr Hex_Axial offset_to_axial_odd_r(Hex_Offset o) noexcept
{
  const ca_index_t q = o.col - (o.row - (o.row & 1)) / 2;
  return Hex_Axial{q, o.row};
}

/// Axial -> cube.
[[nodiscard]] inline constexpr Hex_Cube axial_to_cube(Hex_Axial a) noexcept
{
  const ca_index_t x = a.q;
  const ca_index_t z = a.r;
  return Hex_Cube{x, -x - z, z};
}

/// Cube -> axial.
[[nodiscard]] inline constexpr Hex_Axial cube_to_axial(Hex_Cube c) noexcept
{
  return Hex_Axial{c.x, c.z};
}

/** @brief Hex distance between two axial coordinates.
 *
 *  Computed via the cube representation: `(|dx| + |dy| + |dz|) / 2`,
 *  which equals the minimum number of unit hex steps from `a` to `b`.
 */
[[nodiscard]] inline constexpr ca_size_t hex_distance(Hex_Axial a, Hex_Axial b) noexcept
{
  const Hex_Cube ca = axial_to_cube(a);
  const Hex_Cube cb = axial_to_cube(b);
  auto absv = [](ca_index_t v) noexcept -> ca_index_t { return v < 0 ? -v : v; };
  const ca_index_t d = absv(ca.x - cb.x) + absv(ca.y - cb.y) + absv(ca.z - cb.z);
  return static_cast<ca_size_t>(d / 2);
}

/** @brief Pointy-top pixel coordinates of an axial cell.
 *
 *  Returns the centre of the hex with the given axial coords assuming
 *  pointy-top tiling and circumradius `radius`. Axis 0 (`r`) maps to
 *  Y growing downward; axis 1 (`q`) maps to X growing rightward.
 *
 *  @param[in] a      axial coords.
 *  @param[in] radius hex circumradius (centre-to-vertex distance).
 *  @return `{x, y}` Cartesian centre, in the same units as `radius`.
 */
[[nodiscard]] inline std::array<double, 2> axial_to_pixel_pointy(Hex_Axial a,
                                                                 double radius) noexcept
{
  const double sqrt3 = 1.7320508075688772;
  const double x = radius * (sqrt3 * static_cast<double>(a.q)
                             + (sqrt3 / 2.0) * static_cast<double>(a.r));
  const double y = radius * (3.0 / 2.0) * static_cast<double>(a.r);
  return {x, y};
}

/** @brief Flat-top pixel coordinates of an axial cell.
 *
 *  Returns the centre of the hex assuming flat-top tiling.
 *
 *  @param[in] a      axial coords.
 *  @param[in] radius hex circumradius.
 *  @return `{x, y}` Cartesian centre.
 */
[[nodiscard]] inline std::array<double, 2> axial_to_pixel_flat(Hex_Axial a,
                                                               double radius) noexcept
{
  const double sqrt3 = 1.7320508075688772;
  const double x = radius * (3.0 / 2.0) * static_cast<double>(a.q);
  const double y = radius * (sqrt3 * static_cast<double>(a.r)
                             + (sqrt3 / 2.0) * static_cast<double>(a.q));
  return {x, y};
}

/** @brief Hexagonal lattice with axial accessors over a 2D storage.
 *
 *  Inherits the full `Lattice` API (so the generic engine and
 *  `Hex_Neighborhood` work unchanged) and adds axial/offset helpers
 *  that translate the user's hex coords into the underlying
 *  `(r, q)` storage layout.
 *
 *  Coord convention: `coord = {r, q}` (axis 0 is row, axis 1 is
 *  column). The strict `at` / `set` of the base class continue to
 *  consume `coord_type`, so the generic engine can iterate row-major
 *  exactly as it does for any rectangular lattice.
 *
 *  @tparam Storage  storage type (rank 2) satisfying `LatticeLike`.
 *  @tparam Boundary boundary tag, default `OpenBoundary`.
 */
template <typename Storage, typename Boundary = OpenBoundary>
class Hex_Lattice : public Lattice<Storage, Boundary>
{
  static_assert(Storage::rank == 2, "Hex_Lattice requires a 2D storage");

public:
  using base_type = Lattice<Storage, Boundary>;
  using state_type = typename base_type::state_type;
  using coord_type = typename base_type::coord_type;
  using extents_type = typename base_type::extents_type;
  static constexpr std::size_t rank = base_type::rank;

  using base_type::base_type;  ///< inherit constructors

  /// Number of rows (axis 0, the `r` axis).
  [[nodiscard]] ca_size_t rows() const noexcept { return this->size(0); }

  /// Number of columns per row (axis 1, the `q` axis).
  [[nodiscard]] ca_size_t cols() const noexcept { return this->size(1); }

  /// @return cell value at axial coords `(q, r)` (strict).
  [[nodiscard]] state_type at_axial(Hex_Axial a) const
  {
    return this->at(coord_type{a.r, a.q});
  }

  /// Write cell value at axial coords `(q, r)` (strict).
  void set_axial(Hex_Axial a, const state_type &v)
  {
    this->set(coord_type{a.r, a.q}, v);
  }

  /// Boundary-aware read at axial coords.
  [[nodiscard]] state_type at_axial_safe(Hex_Axial a) const
  {
    return this->at_safe(coord_type{a.r, a.q});
  }

  /// Convenience: read using the even-r offset convention.
  [[nodiscard]] state_type at_offset_even_r(Hex_Offset o) const
  {
    return at_axial(offset_to_axial_even_r(o));
  }

  /// Convenience: write using the even-r offset convention.
  void set_offset_even_r(Hex_Offset o, const state_type &v)
  {
    set_axial(offset_to_axial_even_r(o), v);
  }
};

/** @brief Render a hex frame as a stand-alone TikZ picture.
 *
 *  Produces a TikZ `tikzpicture` body with one filled hexagon per
 *  cell, coloured according to a user-supplied palette functor that
 *  maps a `state_type` to a TikZ colour (LaTeX colour name).
 *
 *  The output is appended to `os`. The caller is responsible for
 *  wrapping the LaTeX preamble (`\documentclass`, `\usepackage{tikz}`)
 *  if a stand-alone document is desired.
 *
 *  @tparam Lattice    hex lattice type.
 *  @tparam Palette    invocable `state_type -> std::string` palette.
 *  @param[in,out] os  stream to write to.
 *  @param[in]     lat hex lattice.
 *  @param[in]     palette colour mapping.
 *  @param[in]     radius hex circumradius (default 0.5cm).
 *  @param[in]     pointy_top use pointy-top orientation (default true).
 */
template <typename Lattice, typename Palette>
inline void render_hex_lattice_tikz(std::ostream &os,
                                    const Lattice &lat,
                                    Palette &&palette,
                                    double radius = 0.5,
                                    bool pointy_top = true)
{
  os << "% Aleph::CA::render_hex_lattice_tikz\n";
  os << "\\begin{tikzpicture}[x=1cm, y=-1cm]\n";
  for (ca_size_t r = 0; r < lat.rows(); ++r)
    for (ca_size_t q = 0; q < lat.cols(); ++q)
      {
        const Hex_Axial a{static_cast<ca_index_t>(q), static_cast<ca_index_t>(r)};
        const auto px
          = pointy_top ? axial_to_pixel_pointy(a, radius) : axial_to_pixel_flat(a, radius);
        const std::string colour = palette(lat.at_axial(a));
        const double rotation = pointy_top ? 30.0 : 0.0;
        os << "  \\node[regular polygon, regular polygon sides=6, draw, fill="
           << colour << ", inner sep=0pt, minimum size=" << (2 * radius)
           << "cm, rotate=" << rotation << "] at (" << px[0] << ", " << px[1] << ") {};\n";
      }
  os << "\\end{tikzpicture}\n";
}

/// @overload Returns a `std::string` instead of writing to a stream.
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_hex_lattice_tikz(const Lattice &lat,
                                                         Palette &&palette,
                                                         double radius = 0.5,
                                                         bool pointy_top = true)
{
  std::ostringstream oss;
  render_hex_lattice_tikz(oss, lat, std::forward<Palette>(palette), radius, pointy_top);
  return oss.str();
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_HEX_LATTICE_H