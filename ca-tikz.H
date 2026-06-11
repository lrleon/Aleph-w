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

/** @file ca-tikz.H
 *  @brief TikZ renderers for cellular-automata frames.
 *
 *  Provides a generic Phase-11 visualisation facade:
 *
 *    - Rectangular 2D lattices are rendered as filled rectangles.
 *    - Hexagonal lattices dispatch to `render_hex_lattice_tikz()`.
 *    - Triangular lattices dispatch to `render_triangular_lattice_tikz()`.
 *
 *  The output is a `tikzpicture` body suitable for embedding in LaTeX
 *  reports. It intentionally does not emit a document preamble; callers can
 *  compose several figures in one document.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_TIKZ_H
#define CA_TIKZ_H

#include <concepts>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_triangular_lattice.H>

namespace Aleph {
namespace CA {

/** @brief TikZ rendering options for rectangular frames.
 *
 *  @par Complexity
 *  O(1) storage.
 */
struct Rect_Tikz_Options
{
  double cell_size = 0.35;              ///< cell side length in centimetres
  bool draw_grid = true;                ///< draw cell borders
  bool include_legend = false;          ///< append a compact alive/dead legend
  std::string grid_colour = "gray!45";  ///< TikZ colour used for grid lines
  std::string dead_label = "dead";      ///< legend label for dead cells
  std::string alive_label = "alive";    ///< legend label for live cells
};

/** @brief Default binary TikZ palette.
 *
 *  Cells equal to `dead` use `dead_colour`; every other state uses
 *  `alive_colour`.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_Tikz_Palette
{
  State dead{};                        ///< state considered dead
  std::string dead_colour = "white";   ///< fill colour for dead cells
  std::string alive_colour = "black";  ///< fill colour for live cells

  /** @brief Map a cell state to a TikZ fill colour.
   *  @param[in] v cell state.
   *  @return TikZ colour expression.
   */
  [[nodiscard]] std::string operator()(const State &v) const
  {
    return v == dead ? dead_colour : alive_colour;
  }
};

namespace ca_tikz_detail {

template <typename Lattice>
concept HexRenderable = requires(const Lattice &lat) {
  lat.rows();
  lat.cols();
  lat.at_axial(Hex_Axial{});
};

template <typename Lattice>
concept TriRenderable = requires(const Lattice &lat) {
  lat.rows();
  lat.cols();
  lat.at_tri(Tri_Coord{});
};

template <typename Lattice>
inline void require_rank2_rect(const char *fn)
{
  static_assert(LatticeLike<Lattice>, "TikZ rendering requires a LatticeLike frame");
  static_assert(Lattice::rank == 2, "Rectangular TikZ rendering requires rank 2");
  (void) fn;
}

}  // namespace ca_tikz_detail

/** @brief Render a rectangular 2D frame as a TikZ picture.
 *
 *  `palette(cell)` must return a TikZ colour expression. Coordinates are
 *  rendered with row 0 at the top (`y=-1cm` in the picture options).
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rendering options.
 *  @throws std::domain_error if `cell_size <= 0`.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 *  @par Thread-safety
 *  Reentrant, except for concurrent writes to the same stream.
 */
template <typename Lattice, typename Palette>
inline void render_rect_lattice_tikz(std::ostream &out,
                                     const Lattice &frame,
                                     Palette &&palette,
                                     const Rect_Tikz_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  ca_tikz_detail::require_rank2_rect<Lattice>("render_rect_lattice_tikz");
  using coord_t = typename Lattice::coord_type;
  ah_domain_error_if(opts.cell_size <= 0.0)
    << "render_rect_lattice_tikz: cell_size must be positive";

  out << "% Aleph::CA::render_rect_lattice_tikz\n";
  out << "\\begin{tikzpicture}[x=1cm, y=-1cm]\n";
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      {
        const double x0 = static_cast<double>(c) * opts.cell_size;
        const double y0 = static_cast<double>(r) * opts.cell_size;
        const double x1 = x0 + opts.cell_size;
        const double y1 = y0 + opts.cell_size;
        const std::string colour
          = palette(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
        out << "  \\path[fill=" << colour;
        if (opts.draw_grid)
          out << ", draw=" << opts.grid_colour;
        out << "] (" << x0 << ", " << y0 << ") rectangle (" << x1 << ", " << y1 << ");\n";
      }

  if (opts.include_legend)
    {
      const double y = (static_cast<double>(frame.size(0)) + 0.7) * opts.cell_size;
      out << "  \\path[fill=white, draw=" << opts.grid_colour << "] (0, " << y << ") rectangle ("
          << opts.cell_size << ", " << (y + opts.cell_size) << ");\n";
      out << "  \\node[anchor=west] at (" << (opts.cell_size * 1.25) << ", "
          << (y + opts.cell_size * 0.5) << ") {" << opts.dead_label << "};\n";
      out << "  \\path[fill=black, draw=" << opts.grid_colour << "] (" << (opts.cell_size * 3.5)
          << ", " << y << ") rectangle (" << (opts.cell_size * 4.5) << ", " << (y + opts.cell_size)
          << ");\n";
      out << "  \\node[anchor=west] at (" << (opts.cell_size * 4.75) << ", "
          << (y + opts.cell_size * 0.5) << ") {" << opts.alive_label << "};\n";
    }

  out << "\\end{tikzpicture}\n";
  ah_runtime_error_if(not out) << "render_rect_lattice_tikz: output stream failed";
}

/** @brief Return a rectangular frame rendered as TikZ.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in] frame lattice frame.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rendering options.
 *  @return TikZ picture.
 */
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_rect_lattice_tikz(const Lattice &frame,
                                                          Palette &&palette,
                                                          const Rect_Tikz_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  std::ostringstream out;
  render_rect_lattice_tikz(out, frame, std::forward<Palette>(palette), opts);
  return out.str();
}

/** @brief Render any supported 2D CA lattice as TikZ.
 *
 *  Hexagonal and triangular lattice wrappers reuse their specialised
 *  renderers. All other rank-2 lattices use the rectangular renderer.
 *
 *  @tparam Lattice lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rectangular options used for rectangular frames.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Palette>
inline void render_lattice_tikz(std::ostream &out,
                                const Lattice &frame,
                                Palette &&palette,
                                const Rect_Tikz_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  if constexpr (ca_tikz_detail::HexRenderable<Lattice>)
    render_hex_lattice_tikz(out, frame, std::forward<Palette>(palette));
  else if constexpr (ca_tikz_detail::TriRenderable<Lattice>)
    render_triangular_lattice_tikz(out, frame, std::forward<Palette>(palette));
  else
    render_rect_lattice_tikz(out, frame, std::forward<Palette>(palette), opts);
}

/** @brief Return any supported 2D CA lattice rendered as TikZ.
 *
 *  @tparam Lattice lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in] frame lattice frame.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rectangular options used for rectangular frames.
 *  @return TikZ picture.
 */
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_lattice_tikz(const Lattice &frame,
                                                     Palette &&palette,
                                                     const Rect_Tikz_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  std::ostringstream out;
  render_lattice_tikz(out, frame, std::forward<Palette>(palette), opts);
  return out.str();
}

/** @brief Render with the default binary palette.
 *
 *  @tparam Lattice lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts rectangular options used for rectangular frames.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void render_lattice_tikz(std::ostream &out,
                                const Lattice &frame,
                                const Rect_Tikz_Options &opts = {})
{
  render_lattice_tikz(out, frame, Binary_Tikz_Palette<typename Lattice::state_type>{}, opts);
}

/** @brief Return a TikZ picture using the default binary palette.
 *
 *  @tparam Lattice lattice type.
 *  @param[in] frame lattice frame.
 *  @param[in] opts rectangular options used for rectangular frames.
 *  @return TikZ picture.
 */
template <typename Lattice>
[[nodiscard]] inline std::string render_lattice_tikz(const Lattice &frame,
                                                     const Rect_Tikz_Options &opts = {})
{
  std::ostringstream out;
  render_lattice_tikz(out, frame, opts);
  return out.str();
}

}  // namespace CA
}  // namespace Aleph

#endif  // CA_TIKZ_H
