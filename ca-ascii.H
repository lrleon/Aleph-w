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

/** @file ca-ascii.H
 *  @brief Terminal and golden-test renderers for CA frames.
 *
 *  Provides a small textual renderer that is useful in CI, logs and
 *  examples that must work without a display server. ANSI escape output
 *  is opt-in through user palettes; the default binary palette is plain
 *  ASCII so snapshots remain portable.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_ASCII_H
#define CA_ASCII_H

#include <concepts>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <ah-errors.H>

#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Options for textual CA rendering.
 *
 *  @par Complexity
 *  O(1) storage plus owned strings.
 */
struct Ascii_Render_Options
{
  bool include_border = false;           ///< draw a simple `+---+` border
  bool include_trailing_newline = true;  ///< append newline after the last row
  std::string line_prefix;               ///< text prepended to every row
};

/** @brief Default binary text palette.
 *
 *  Cells equal to `dead` use `dead_glyph`; all other states use
 *  `alive_glyph`.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_ASCII_Palette
{
  State dead{};                   ///< state considered dead
  std::string dead_glyph = ".";   ///< glyph for dead cells
  std::string alive_glyph = "#";  ///< glyph for live cells

  /** @brief Map a cell state to display text.
   *  @param[in] value cell value.
   *  @return glyph text.
   */
  [[nodiscard]] std::string operator()(const State &value) const
  {
    return value == dead ? dead_glyph : alive_glyph;
  }
};

/** @brief ANSI 24-bit colour palette for binary frames.
 *
 *  The returned string contains terminal escape codes and a reset
 *  sequence. It is intended for interactive terminals, not golden
 *  files.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_ANSI24_Palette
{
  State dead{};                      ///< state considered dead
  RGB8 dead_colour{20, 24, 28};      ///< background colour for dead cells
  RGB8 alive_colour{230, 245, 255};  ///< background colour for live cells
  std::string glyph = "  ";          ///< glyph painted with the background

  /** @brief Map a cell state to an ANSI-coloured glyph.
   *  @param[in] value cell value.
   *  @return terminal escape sequence plus glyph.
   */
  [[nodiscard]] std::string operator()(const State &value) const
  {
    const RGB8 c = value == dead ? dead_colour : alive_colour;
    std::ostringstream out;
    out << "\033[48;2;" << static_cast<unsigned>(c.r) << ';' << static_cast<unsigned>(c.g) << ';'
        << static_cast<unsigned>(c.b) << 'm' << glyph << "\033[0m";
    return out.str();
  }
};

/** @brief Render a rank-2 frame as text.
 *
 *  `palette(cell)` must return a string glyph. Coordinates are emitted
 *  in row-major order.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to render.
 *  @param[in] palette state-to-text mapper.
 *  @param[in] opts rendering options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Palette>
inline void render_ascii(std::ostream &out,
                         const Lattice &frame,
                         Palette &&palette,
                         const Ascii_Render_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  static_assert(LatticeLike<Lattice>, "render_ascii requires a LatticeLike frame");
  static_assert(Lattice::rank == 2, "render_ascii requires a rank-2 frame");
  using coord_t = typename Lattice::coord_type;

  auto write_border = [&]()
  {
    out << opts.line_prefix << '+';
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      out << '-';
    out << "+\n";
  };

  if (opts.include_border)
    write_border();

  for (ca_size_t r = 0; r < frame.size(0); ++r)
    {
      out << opts.line_prefix;
      if (opts.include_border)
        out << '|';
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        out << palette(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
      if (opts.include_border)
        out << '|';
      if (opts.include_trailing_newline or r + 1 < frame.size(0))
        out << '\n';
    }

  if (opts.include_border)
    write_border();

  ah_runtime_error_if(not out) << "render_ascii: output stream failed";
}

/** @brief Return the text rendering of a rank-2 frame.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in] frame frame to render.
 *  @param[in] palette state-to-text mapper.
 *  @param[in] opts rendering options.
 *  @return rendered text.
 */
template <typename Lattice, typename Palette>
[[nodiscard]] inline std::string render_ascii_string(const Lattice &frame,
                                                     Palette &&palette,
                                                     const Ascii_Render_Options &opts = {})
  requires requires(Palette &&p, const typename Lattice::state_type &v) {
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  std::ostringstream out;
  render_ascii(out, frame, std::forward<Palette>(palette), opts);
  return out.str();
}

/** @brief Render using the default binary ASCII palette.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to render.
 *  @param[in] opts rendering options.
 */
template <typename Lattice>
inline void render_ascii(std::ostream &out, const Lattice &frame, const Ascii_Render_Options &opts = {})
{
  render_ascii(out, frame, Binary_ASCII_Palette<typename Lattice::state_type>{}, opts);
}

/** @brief Return a default binary ASCII rendering.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in] frame frame to render.
 *  @param[in] opts rendering options.
 *  @return rendered text.
 */
template <typename Lattice>
[[nodiscard]] inline std::string render_ascii_string(const Lattice &frame,
                                                     const Ascii_Render_Options &opts = {})
{
  std::ostringstream out;
  render_ascii(out, frame, opts);
  return out.str();
}

/** @brief Stream sink that writes textual frames to an output stream.
 *
 *  Each accepted frame is preceded by a compact step marker unless
 *  `include_step_header` is disabled.
 *
 *  @tparam Palette invocable `state_type -> std::string`.
 */
template <typename Palette>
class Ascii_Frame_Sink
{
  std::ostream *out_ = nullptr;
  Palette palette_;
  Ascii_Render_Options opts_{};
  bool include_step_header_ = true;

public:
  /** @brief Build a stream-backed ASCII sink.
   *  @param[in,out] out destination stream.
   *  @param[in] palette state-to-text mapper.
   *  @param[in] opts rendering options.
   *  @param[in] include_step_header write `# step N` before each frame.
   */
  Ascii_Frame_Sink(std::ostream &out,
                   Palette palette,
                   Ascii_Render_Options opts = {},
                   const bool include_step_header = true)
    : out_(&out), palette_(std::move(palette)), opts_(std::move(opts)),
      include_step_header_(include_step_header)
  {}

  /** @brief Write one textual frame.
   *  @tparam Lattice 2D lattice type.
   *  @param[in] step simulation step.
   *  @param[in] frame frame to render.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    if (include_step_header_)
      *out_ << "# step " << step << '\n';
    render_ascii(*out_, frame, palette_, opts_);
  }

  /** @brief Flush the wrapped stream. */
  void flush()
  {
    out_->flush();
    ah_runtime_error_if(not *out_) << "Ascii_Frame_Sink::flush: output stream failed";
  }
};

template <typename Palette>
Ascii_Frame_Sink(std::ostream &, Palette, Ascii_Render_Options = {}, bool = true)
  -> Ascii_Frame_Sink<Palette>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_ASCII_H
