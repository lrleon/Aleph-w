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

/** @file ca-svg.H
 *  @brief Native SVG renderer for rectangular, hexagonal and triangular CAs.
 *
 *  The backend emits dependency-free SVG suitable for reports, web pages
 *  and golden visual tests. Rectangular frames use `<rect>` elements;
 *  hexagonal and triangular lattice wrappers use true polygon geometry
 *  instead of a bitmap approximation.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_SVG_H
#define CA_SVG_H

#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <ca-io.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_triangular_lattice.H>

namespace Aleph {
namespace CA {

/** @brief SVG rendering options.
 *
 *  `cell_size` is the rectangle side length for square lattices, the
 *  circumradius for hexagons and the side length for triangles.
 */
struct SVG_Render_Options
{
  double cell_size = 18.0;            ///< logical cell size in SVG units
  double stroke_width = 0.75;         ///< grid stroke width
  bool draw_grid = true;              ///< draw cell borders
  bool omit_dead_cells = false;       ///< skip cells equal to `dead_state`
  RGB8 stroke_colour{190, 190, 190};  ///< grid colour
};

namespace ca_svg_detail {

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

[[nodiscard]] inline std::string rgb_hex(const RGB8 c)
{
  const char digits[] = "0123456789abcdef";
  std::string out = "#000000";
  out[1] = digits[c.r >> 4];
  out[2] = digits[c.r & 0x0f];
  out[3] = digits[c.g >> 4];
  out[4] = digits[c.g & 0x0f];
  out[5] = digits[c.b >> 4];
  out[6] = digits[c.b & 0x0f];
  return out;
}

struct Bounds
{
  double min_x = std::numeric_limits<double>::infinity();
  double min_y = std::numeric_limits<double>::infinity();
  double max_x = -std::numeric_limits<double>::infinity();
  double max_y = -std::numeric_limits<double>::infinity();

  void include(const double x, const double y) noexcept
  {
    min_x = std::min(min_x, x);
    min_y = std::min(min_y, y);
    max_x = std::max(max_x, x);
    max_y = std::max(max_y, y);
  }
};

inline void write_svg_header(std::ostream &out, const Bounds &b, const SVG_Render_Options &opts)
{
  const double pad = opts.draw_grid ? opts.stroke_width : 0.0;
  const double x = b.min_x - pad;
  const double y = b.min_y - pad;
  const double w = std::max(1.0, b.max_x - b.min_x + 2.0 * pad);
  const double h = std::max(1.0, b.max_y - b.min_y + 2.0 * pad);
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << x << ' ' << y << ' ' << w << ' '
      << h << "\" width=\"" << w << "\" height=\"" << h << "\">\n";
}

inline void write_style(std::ostream &out, const SVG_Render_Options &opts)
{
  if (opts.draw_grid)
    out << " stroke=\"" << rgb_hex(opts.stroke_colour) << "\" stroke-width=\"" << opts.stroke_width
        << '"';
  else
    out << " stroke=\"none\"";
}

inline void write_points(std::ostream &out, const std::vector<std::array<double, 2>> &pts)
{
  for (std::size_t i = 0; i < pts.size(); ++i)
    {
      if (i != 0)
        out << ' ';
      out << pts[i][0] << ',' << pts[i][1];
    }
}

[[nodiscard]] inline std::vector<std::array<double, 2>> hex_points(Hex_Axial a, const double radius)
{
  const auto centre = axial_to_pixel_pointy(a, radius);
  std::vector<std::array<double, 2>> pts;
  pts.reserve(6);
  constexpr double pi = 3.14159265358979323846;
  for (int k = 0; k < 6; ++k)
    {
      const double angle = (60.0 * static_cast<double>(k) - 30.0) * pi / 180.0;
      pts.push_back({centre[0] + radius * std::cos(angle), centre[1] + radius * std::sin(angle)});
    }
  return pts;
}

[[nodiscard]] inline std::vector<std::array<double, 2>> tri_points(Tri_Coord c, const double side)
{
  const double sqrt3 = 1.7320508075688772;
  const double h = side * sqrt3 / 2.0;
  const auto centre = tri_pixel_centre(c, side);
  if (tri_is_up(c))
    return {std::array<double, 2>{centre[0], centre[1] - (2.0 * h / 3.0)},
            std::array<double, 2>{centre[0] - side / 2.0, centre[1] + h / 3.0},
            std::array<double, 2>{centre[0] + side / 2.0, centre[1] + h / 3.0}};
  return {std::array<double, 2>{centre[0], centre[1] + (2.0 * h / 3.0)},
          std::array<double, 2>{centre[0] - side / 2.0, centre[1] - h / 3.0},
          std::array<double, 2>{centre[0] + side / 2.0, centre[1] - h / 3.0}};
}

}  // namespace ca_svg_detail

/** @brief Render a supported rank-2 CA frame as SVG.
 *
 *  Rectangular frames produce `<rect>` cells. `Hex_Lattice` and
 *  `Triangular_Lattice` frames produce exact `<polygon>` cells.
 *
 *  @tparam Lattice lattice type.
 *  @tparam Palette invocable `state_type -> RGB8`.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to render.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rendering options.
 *  @param[in] dead_state state skipped when `omit_dead_cells` is true.
 *  @throws std::domain_error if `cell_size <= 0`.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Palette>
inline void render_lattice_svg(std::ostream &out,
                               const Lattice &frame,
                               Palette &&palette,
                               const SVG_Render_Options &opts = {},
                               const typename Lattice::state_type &dead_state
                               = typename Lattice::state_type{})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<RGB8>;
  }
{
  static_assert(LatticeLike<Lattice>, "render_lattice_svg requires a LatticeLike frame");
  static_assert(Lattice::rank == 2, "render_lattice_svg requires a rank-2 frame");
  ah_domain_error_if(opts.cell_size <= 0.0) << "render_lattice_svg: cell_size must be positive";

  ca_svg_detail::Bounds b;
  if constexpr (ca_svg_detail::HexRenderable<Lattice>)
    {
      for (ca_size_t r = 0; r < frame.rows(); ++r)
        for (ca_size_t q = 0; q < frame.cols(); ++q)
          for (const auto &p : ca_svg_detail::hex_points(
                 Hex_Axial{static_cast<ca_index_t>(q), static_cast<ca_index_t>(r)}, opts.cell_size))
            b.include(p[0], p[1]);
    }
  else if constexpr (ca_svg_detail::TriRenderable<Lattice>)
    {
      for (ca_size_t r = 0; r < frame.rows(); ++r)
        for (ca_size_t c = 0; c < frame.cols(); ++c)
          for (const auto &p : ca_svg_detail::tri_points(
                 Tri_Coord{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}, opts.cell_size))
            b.include(p[0], p[1]);
    }
  else
    {
      b.include(0.0, 0.0);
      b.include(static_cast<double>(frame.size(1)) * opts.cell_size,
                static_cast<double>(frame.size(0)) * opts.cell_size);
    }

  ca_svg_detail::write_svg_header(out, b, opts);
  if constexpr (ca_svg_detail::HexRenderable<Lattice>)
    {
      for (ca_size_t r = 0; r < frame.rows(); ++r)
        for (ca_size_t q = 0; q < frame.cols(); ++q)
          {
            const Hex_Axial a{static_cast<ca_index_t>(q), static_cast<ca_index_t>(r)};
            const auto state = frame.at_axial(a);
            if (opts.omit_dead_cells and state == dead_state)
              continue;
            out << "  <polygon points=\"";
            ca_svg_detail::write_points(out, ca_svg_detail::hex_points(a, opts.cell_size));
            out << "\" fill=\"" << ca_svg_detail::rgb_hex(palette(state)) << '"';
            ca_svg_detail::write_style(out, opts);
            out << "/>\n";
          }
    }
  else if constexpr (ca_svg_detail::TriRenderable<Lattice>)
    {
      for (ca_size_t r = 0; r < frame.rows(); ++r)
        for (ca_size_t c = 0; c < frame.cols(); ++c)
          {
            const Tri_Coord tc{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)};
            const auto state = frame.at_tri(tc);
            if (opts.omit_dead_cells and state == dead_state)
              continue;
            out << "  <polygon points=\"";
            ca_svg_detail::write_points(out, ca_svg_detail::tri_points(tc, opts.cell_size));
            out << "\" fill=\"" << ca_svg_detail::rgb_hex(palette(state)) << '"';
            ca_svg_detail::write_style(out, opts);
            out << "/>\n";
          }
    }
  else
    {
      using coord_t = typename Lattice::coord_type;
      for (ca_size_t r = 0; r < frame.size(0); ++r)
        for (ca_size_t c = 0; c < frame.size(1); ++c)
          {
            const auto state
              = frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)});
            if (opts.omit_dead_cells and state == dead_state)
              continue;
            out << "  <rect x=\"" << static_cast<double>(c) * opts.cell_size << "\" y=\""
                << static_cast<double>(r) * opts.cell_size << "\" width=\"" << opts.cell_size
                << "\" height=\"" << opts.cell_size << "\" fill=\""
                << ca_svg_detail::rgb_hex(palette(state)) << '"';
            ca_svg_detail::write_style(out, opts);
            out << "/>\n";
          }
    }
  out << "</svg>\n";
  ah_runtime_error_if(not out) << "render_lattice_svg: output stream failed";
}

/** @brief Render a frame as SVG using the default binary RGB mapper.
 *
 *  @tparam Lattice lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to render.
 *  @param[in] opts rendering options.
 */
template <typename Lattice>
inline void render_lattice_svg(std::ostream &out,
                               const Lattice &frame,
                               const SVG_Render_Options &opts = {})
{
  render_lattice_svg(out, frame, Binary_RGB_Mapper<typename Lattice::state_type>{}, opts);
}

/** @brief Return a frame rendered as SVG.
 *
 *  @tparam Lattice lattice type.
 *  @tparam Palette invocable `state_type -> RGB8`.
 *  @param[in] frame frame to render.
 *  @param[in] palette state-to-colour mapper.
 *  @param[in] opts rendering options.
 *  @return SVG document.
 */
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_lattice_svg_string(const Lattice &frame,
                                                           Palette &&palette,
                                                           const SVG_Render_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<RGB8>;
  }
{
  std::ostringstream out;
  render_lattice_svg(out, frame, std::forward<Palette>(palette), opts);
  return out.str();
}

/** @brief Path-pattern sink that writes one SVG file per accepted frame.
 *
 *  @tparam Palette invocable `state_type -> RGB8`.
 */
template <typename Palette>
class Svg_Frame_Sink
{
  std::filesystem::path pattern_;
  Palette palette_;
  SVG_Render_Options opts_{};
  std::size_t zero_pad_ = 6;

public:
  /** @brief Build an SVG frame sink.
   *  @param[in] path_pattern output path pattern.
   *  @param[in] palette state-to-colour mapper.
   *  @param[in] opts rendering options.
   *  @param[in] zero_pad minimum width for `{step}`.
   */
  Svg_Frame_Sink(std::filesystem::path path_pattern,
                 Palette palette,
                 SVG_Render_Options opts = {},
                 const std::size_t zero_pad = 6)
    : pattern_(std::move(path_pattern)), palette_(std::move(palette)), opts_(std::move(opts)),
      zero_pad_(zero_pad)
  {}

  /** @brief Write one SVG frame.
   *  @tparam Lattice 2D lattice type.
   *  @param[in] step simulation step.
   *  @param[in] frame frame to write.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    const std::filesystem::path path = format_step_path(pattern_, step, zero_pad_);
    if (const auto parent = path.parent_path(); not parent.empty())
      std::filesystem::create_directories(parent);
    std::ofstream out(path);
    ah_runtime_error_if(not out) << "Svg_Frame_Sink::accept: cannot open '" << path.string() << "'";
    render_lattice_svg(out, frame, palette_, opts_);
  }

  /** @brief SVG frames are written eagerly; this is a no-op. */
  void flush() {}
};

template <typename Palette>
Svg_Frame_Sink(std::filesystem::path, Palette, SVG_Render_Options = {}, std::size_t = 6)
  -> Svg_Frame_Sink<Palette>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_SVG_H
