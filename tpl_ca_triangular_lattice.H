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

/** @file tpl_ca_triangular_lattice.H
 *  @brief Triangular lattice with up/down parity helpers and pixel
 *         mapping suitable for visualisation.
 *
 *  A triangular lattice tiles the plane with alternating "up" and
 *  "down" triangles. Storing the cells as a regular 2D buffer with
 *  axes `(i, j)` is the simplest layout: the parity `(i + j) & 1`
 *  decides whether the cell at `(i, j)` is an up-triangle (parity 0)
 *  or a down-triangle (parity 1). The existing
 *  `Triangular_Neighborhood` enumerates the three edge-neighbours
 *  according to this parity, so the generic engine works unchanged.
 *
 *  This header provides:
 *
 *    - `Tri_Coord` value type (with `i, j` axes and parity helper).
 *    - `Triangular_Lattice<Storage, Boundary>`: subclass of `Lattice`
 *      that adds parity- and pixel-aware accessors plus a TikZ-style
 *      pixel mapping for triangle centres.
 *    - `tri_pixel_centre`: cartesian centre of an up/down triangle
 *      based on `(i, j)`.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_TRIANGULAR_LATTICE_H
#define TPL_CA_TRIANGULAR_LATTICE_H

#include <array>
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

/** @brief Coordinates of a triangular cell as `(i, j)` integers.
 *
 *  The axis-0 component (`i`) selects the row; the axis-1 component
 *  (`j`) selects the column. Parity `(i + j) & 1` determines the
 *  triangle's orientation:
 *
 *    - `parity == 0` ⇒ up triangle.
 *    - `parity == 1` ⇒ down triangle.
 */
struct Tri_Coord
{
  ca_index_t i = 0;
  ca_index_t j = 0;

  friend constexpr bool operator==(Tri_Coord a, Tri_Coord b) noexcept
  {
    return a.i == b.i and a.j == b.j;
  }
  friend constexpr bool operator!=(Tri_Coord a, Tri_Coord b) noexcept { return not(a == b); }
};

/// @return parity (0 = up triangle, 1 = down triangle).
[[nodiscard]] inline constexpr int tri_parity(Tri_Coord c) noexcept
{
  return static_cast<int>((c.i + c.j) & 1);
}

/// @return true if `c` is an up triangle.
[[nodiscard]] inline constexpr bool tri_is_up(Tri_Coord c) noexcept
{
  return tri_parity(c) == 0;
}

/// @return true if `c` is a down triangle.
[[nodiscard]] inline constexpr bool tri_is_down(Tri_Coord c) noexcept
{
  return tri_parity(c) == 1;
}

/** @brief Cartesian centre of a triangle in `(i, j)` units.
 *
 *  Each row `i` is offset vertically by `i * height`, where
 *  `height = side * sqrt(3) / 2`. Columns are spaced horizontally
 *  by `side / 2`. Up and down triangles inside the same row share
 *  their `y` baseline; their centres are vertically displaced by
 *  `±height / 6` (the centroid offset of a unit triangle).
 *
 *  @param[in] c    triangle coords.
 *  @param[in] side triangle side length.
 *  @return `{x, y}` Cartesian centroid.
 */
[[nodiscard]] inline std::array<double, 2> tri_pixel_centre(Tri_Coord c,
                                                            double side) noexcept
{
  const double sqrt3 = 1.7320508075688772;
  const double height = side * sqrt3 / 2.0;
  const double x = (static_cast<double>(c.j) + 1.0) * (side / 2.0);
  const double y_base = static_cast<double>(c.i) * height + height / 2.0;
  const double y = tri_is_up(c) ? y_base - height / 6.0 : y_base + height / 6.0;
  return {x, y};
}

/** @brief Triangular lattice with parity helpers over a 2D storage.
 *
 *  Inherits the full `Lattice` API. Iteration order, double buffering
 *  and engine integration work the same way as for any rectangular
 *  lattice; the only difference is that the rule should consume
 *  `Triangular_Neighborhood` (three edge-neighbours, parity-aware)
 *  rather than Moore / Von Neumann.
 *
 *  @tparam Storage  storage type (rank 2) satisfying `LatticeLike`.
 *  @tparam Boundary boundary tag, default `OpenBoundary`.
 */
template <typename Storage, typename Boundary = OpenBoundary>
class Triangular_Lattice : public Lattice<Storage, Boundary>
{
  static_assert(Storage::rank == 2, "Triangular_Lattice requires a 2D storage");

public:
  using base_type = Lattice<Storage, Boundary>;
  using state_type = typename base_type::state_type;
  using coord_type = typename base_type::coord_type;
  using extents_type = typename base_type::extents_type;
  static constexpr std::size_t rank = base_type::rank;

  using base_type::base_type;  ///< inherit constructors

  /// Number of rows (axis 0).
  [[nodiscard]] ca_size_t rows() const noexcept { return this->size(0); }

  /// Number of columns (axis 1).
  [[nodiscard]] ca_size_t cols() const noexcept { return this->size(1); }

  /// @return cell value at triangular coords `(i, j)` (strict).
  [[nodiscard]] state_type at_tri(Tri_Coord c) const
  {
    return this->at(coord_type{c.i, c.j});
  }

  /// Write cell value at triangular coords `(i, j)` (strict).
  void set_tri(Tri_Coord c, const state_type &v)
  {
    this->set(coord_type{c.i, c.j}, v);
  }

  /// Boundary-aware read at triangular coords.
  [[nodiscard]] state_type at_tri_safe(Tri_Coord c) const
  {
    return this->at_safe(coord_type{c.i, c.j});
  }
};

/** @brief Render a triangular frame as a TikZ picture.
 *
 *  Each triangle is drawn as a rotated equilateral polygon with
 *  side `side`, coloured by the user-supplied palette functor.
 *  The orientation is set automatically from the cell parity.
 *
 *  @tparam Lattice triangular lattice type.
 *  @tparam Palette `state_type -> std::string` colour mapping.
 *  @param[in,out] os    stream to write to.
 *  @param[in]     lat   lattice to render.
 *  @param[in]     palette colour mapping.
 *  @param[in]     side triangle side length (default 1).
 */
template <typename Lattice, typename Palette>
inline void render_triangular_lattice_tikz(std::ostream &os,
                                           const Lattice &lat,
                                           Palette &&palette,
                                           double side = 1.0)
{
  os << "% Aleph::CA::render_triangular_lattice_tikz\n";
  os << "\\begin{tikzpicture}[x=1cm, y=-1cm]\n";
  for (ca_size_t i = 0; i < lat.rows(); ++i)
    for (ca_size_t j = 0; j < lat.cols(); ++j)
      {
        const Tri_Coord c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
        const auto px = tri_pixel_centre(c, side);
        const std::string colour = palette(lat.at_tri(c));
        const double rotation = tri_is_up(c) ? 0.0 : 180.0;
        os << "  \\node[regular polygon, regular polygon sides=3, draw, fill=" << colour
           << ", inner sep=0pt, minimum size=" << side
           << "cm, rotate=" << rotation << "] at (" << px[0] << ", " << px[1] << ") {};\n";
      }
  os << "\\end{tikzpicture}\n";
}

/// @overload Returns a `std::string` instead of writing to a stream.
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_triangular_lattice_tikz(const Lattice &lat,
                                                                Palette &&palette,
                                                                double side = 1.0)
{
  std::ostringstream oss;
  render_triangular_lattice_tikz(oss, lat, std::forward<Palette>(palette), side);
  return oss.str();
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_TRIANGULAR_LATTICE_H