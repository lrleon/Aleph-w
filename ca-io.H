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

/** @file ca-io.H
 *  @brief File-format readers and writers for cellular-automata frames.
 *
 *  Phase 11 consolidates the ad-hoc exporters that examples used to carry
 *  locally. This header provides dependency-free IO for 2D frames:
 *
 *    - Conway RLE (`x = W, y = H, rule = ...`, `b/o/$/!` body).
 *    - Life plaintext (`.cells`), Life 1.06 and Life 1.05.
 *    - CSV and JSON numeric snapshots.
 *    - Binary NetPBM images: PGM (`P5`) and PPM (`P6`).
 *    - A JSON frame-stream writer for trajectories.
 *
 *  All public APIs live under `Aleph::CA`, are header-only, and avoid new
 *  third-party dependencies. Coordinate convention is `(row, col)`, i.e.
 *  lattice axis 0 is the image/pattern row (`y`) and axis 1 is the column
 *  (`x`).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_IO_H
#define CA_IO_H

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief RGB byte triplet used by PPM exporters.
 *
 *  @par Complexity
 *  O(1) storage and access.
 */
struct RGB8
{
  std::uint8_t r = 0;  ///< red channel
  std::uint8_t g = 0;  ///< green channel
  std::uint8_t b = 0;  ///< blue channel

  friend constexpr bool operator==(RGB8, RGB8) noexcept = default;
};

/** @brief Binary pattern shared by RLE, plaintext and Life formats.
 *
 *  Coordinates in `alive` are normalised to the pattern rectangle:
 *  `{0,0}` is the top-left cell, `height` counts rows and `width` counts
 *  columns. `origin_row` / `origin_col` preserve the source coordinate
 *  origin for Life 1.05 / 1.06 files, whose coordinates may be negative.
 *
 *  @par Complexity
 *  O(number of live cells) storage.
 */
struct Binary_Cell_Pattern
{
  ca_size_t width = 0;                ///< number of columns
  ca_size_t height = 0;               ///< number of rows
  ca_index_t origin_row = 0;          ///< original row offset before normalisation
  ca_index_t origin_col = 0;          ///< original column offset before normalisation
  std::string rule;                   ///< optional rule string, e.g. `B3/S23`
  std::string name;                   ///< optional pattern name
  std::vector<std::string> comments;  ///< format comments without marker prefixes
  std::vector<Coord_Vec<2>> alive;    ///< live cells as `{row, col}`
};

/// Backwards-readable alias for Conway RLE patterns.
using RLE_Pattern = Binary_Cell_Pattern;

/** @brief Dense numeric snapshot used by CSV and JSON readers.
 *
 *  Values are stored row-major. The type is deliberately small so callers
 *  can adapt it into `Dense_Cell_Storage`, an engine frame or a custom
 *  lattice.
 *
 *  @tparam T numeric cell type.
 */
template <typename T>
struct Grid_Snapshot
{
  ca_size_t width = 0;   ///< number of columns
  ca_size_t height = 0;  ///< number of rows
  std::vector<T> cells;  ///< row-major payload

  /** @brief Return the value at `(row, col)`.
   *  @param[in] row row index.
   *  @param[in] col column index.
   *  @return value stored at that coordinate.
   *  @throws std::out_of_range if the coordinate is outside the snapshot.
   *
   *  @par Complexity
   *  O(1).
   */
  [[nodiscard]] T at(ca_size_t row, ca_size_t col) const
  {
    ah_out_of_range_error_if(row >= height or col >= width)
      << "Grid_Snapshot::at: coordinate (" << row << ", " << col << ") out of " << height << "x"
      << width;
    return cells[row * width + col];
  }
};

/** @brief Options controlling Conway RLE output.
 *
 *  The default (`trim_dead_border == false`) writes a full-frame snapshot
 *  whose declared dimensions match the lattice exactly, which makes
 *  lattice -> RLE -> lattice round-trips exact. Set `trim_dead_border`
 *  to `true` for compact, pattern-library-style RLE.
 */
struct RLE_Write_Options
{
  std::string rule = "B3/S23";    ///< rule field emitted in the header
  std::string name;               ///< optional `#N` line
  std::string comment;            ///< optional `#C` lines, split on newlines
  bool trim_dead_border = false;  ///< crop to the live-cell bounding box
  std::size_t line_width = 70;    ///< body wrap width; `0` disables wrapping
};

/** @brief Options controlling Life plaintext output.
 *
 *  @par Complexity
 *  O(1) storage.
 */
struct Plaintext_Write_Options
{
  char alive = 'O';     ///< glyph for live cells
  char dead = '.';      ///< glyph for dead cells
  std::string comment;  ///< optional `!` comments, split on newlines
};

/** @brief Options controlling CSV output and input.
 *
 *  @par Complexity
 *  O(1) storage.
 */
struct CSV_Options
{
  char delimiter = ',';          ///< separator between cells
  bool skip_empty_lines = true;  ///< reader ignores empty/whitespace-only rows
};

/** @brief Options controlling JSON snapshot output.
 *
 *  `pretty == false` emits a compact one-line document. `pretty == true`
 *  emits deterministic indentation that is friendlier for diffs.
 */
struct JSON_Write_Options
{
  bool pretty = false;  ///< pretty-print with indentation
  std::string rule;     ///< optional rule field
};

/** @brief Options controlling NetPBM output.
 *
 *  The current implementation writes 8-bit binary NetPBM (`max_value`
 *  must be in `[1, 255]`).
 */
struct NetPBM_Write_Options
{
  std::uint16_t max_value = 255;  ///< NetPBM maximum sample value
};

/** @brief Default binary grayscale mapper.
 *
 *  Cells equal to `dead` become `dead_value`; all other states become
 *  `alive_value`. This matches the usual Life visual convention:
 *  live cells are black, dead cells are white.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_Gray_Mapper
{
  State dead{};                   ///< state considered dead
  std::uint8_t dead_value = 255;  ///< grayscale for dead cells
  std::uint8_t alive_value = 0;   ///< grayscale for live cells

  /** @brief Map a cell to one byte.
   *  @param[in] v cell value.
   *  @return grayscale byte.
   */
  [[nodiscard]] std::uint8_t operator()(const State &v) const
  {
    return v == dead ? dead_value : alive_value;
  }
};

/** @brief Default binary RGB mapper.
 *
 *  Cells equal to `dead` use `dead_colour`; every other state uses
 *  `alive_colour`.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_RGB_Mapper
{
  State dead{};                     ///< state considered dead
  RGB8 dead_colour{255, 255, 255};  ///< colour for dead cells
  RGB8 alive_colour{0, 0, 0};       ///< colour for live cells

  /** @brief Map a cell to RGB.
   *  @param[in] v cell value.
   *  @return RGB byte triplet.
   */
  [[nodiscard]] RGB8 operator()(const State &v) const
  {
    return v == dead ? dead_colour : alive_colour;
  }
};

namespace ca_io_detail {

template <typename Lattice>
inline void require_rank2(const char *fn)
{
  static_assert(LatticeLike<Lattice>, "CA IO requires a LatticeLike frame");
  static_assert(Lattice::rank == 2, "CA IO currently supports 2D frames");
  (void) fn;
}

inline std::string trim_copy(const std::string &s)
{
  std::size_t b = 0;
  while (b < s.size() and std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b and std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

inline std::string lower_copy(std::string s)
{
  for (char &c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool blank(const std::string &s)
{
  for (const char c : s)
    if (not std::isspace(static_cast<unsigned char>(c)))
      return false;
  return true;
}

inline ca_size_t parse_size_token(const std::string &s, const char *what)
{
  const std::string t = trim_copy(s);
  ah_domain_error_if(t.empty()) << what << ": empty size";
  ca_size_t value = 0;
  for (const char c : t)
    {
      ah_domain_error_if(not std::isdigit(static_cast<unsigned char>(c)))
        << what << ": invalid unsigned size '" << s << "'";
      const ca_size_t digit = static_cast<ca_size_t>(c - '0');
      ah_overflow_error_if(value > (std::numeric_limits<ca_size_t>::max() - digit) / 10)
        << what << ": size overflows ca_size_t";
      value = value * 10 + digit;
    }
  return value;
}

inline ca_index_t parse_index_token(const std::string &s, const char *what)
{
  const std::string t = trim_copy(s);
  ah_domain_error_if(t.empty()) << what << ": empty coordinate";
  std::size_t i = 0;
  bool neg = false;
  if (t[i] == '-' or t[i] == '+')
    {
      neg = t[i] == '-';
      ++i;
    }
  ah_domain_error_if(i == t.size()) << what << ": missing coordinate digits";
  ca_index_t value = 0;
  for (; i < t.size(); ++i)
    {
      const char c = t[i];
      ah_domain_error_if(not std::isdigit(static_cast<unsigned char>(c)))
        << what << ": invalid coordinate '" << s << "'";
      const ca_index_t digit = static_cast<ca_index_t>(c - '0');
      ah_overflow_error_if(value > (std::numeric_limits<ca_index_t>::max() - digit) / 10)
        << what << ": coordinate overflows ca_index_t";
      value = value * 10 + digit;
    }
  return neg ? -value : value;
}

inline std::vector<std::string> split_comment_lines(const std::string &comment)
{
  std::vector<std::string> lines;
  std::istringstream in(comment);
  std::string line;
  while (std::getline(in, line))
    lines.push_back(line);
  if (comment.empty())
    lines.clear();
  return lines;
}

inline std::string json_escape(const std::string &s)
{
  std::ostringstream out;
  for (const char c : s)
    {
      switch (c)
        {
        case '"':
          out << "\\\"";
          break;
        case '\\':
          out << "\\\\";
          break;
        case '\n':
          out << "\\n";
          break;
        case '\r':
          out << "\\r";
          break;
        case '\t':
          out << "\\t";
          break;
        default:
          if (static_cast<unsigned char>(c) < 0x20)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << static_cast<unsigned>(static_cast<unsigned char>(c)) << std::dec
                << std::setfill(' ');
          else
            out << c;
          break;
        }
    }
  return out.str();
}

inline std::string rle_token(ca_size_t n, char glyph)
{
  if (n == 0)
    return {};
  std::string token;
  if (n != 1)
    token = std::to_string(n);
  token.push_back(glyph);
  return token;
}

inline void append_rle_token(std::string &body, ca_size_t n, char glyph)
{
  body += rle_token(n, glyph);
}

inline void write_wrapped_rle_body(std::ostream &out,
                                   const std::string &body,
                                   const std::size_t line_width)
{
  std::size_t col = 0;
  std::size_t pos = 0;
  const std::size_t width = line_width == 0 ? std::numeric_limits<std::size_t>::max() : line_width;
  while (pos < body.size())
    {
      const std::size_t start = pos;
      while (pos < body.size() and std::isdigit(static_cast<unsigned char>(body[pos])))
        ++pos;
      ah_domain_error_if(pos == body.size()) << "write_rle: malformed body token";
      ++pos;
      const std::size_t len = pos - start;
      if (col != 0 and col + len > width)
        {
          out << '\n';
          col = 0;
        }
      out.write(body.data() + static_cast<std::ptrdiff_t>(start), static_cast<std::streamsize>(len));
      col += len;
      if (body[pos - 1] == '!')
        {
          out << '\n';
          col = 0;
        }
    }
  if (col != 0)
    out << '\n';
}

inline std::string header_value(const std::string &line, const std::string &key)
{
  std::istringstream parts(line);
  std::string part;
  while (std::getline(parts, part, ','))
    {
      const auto eq = part.find('=');
      if (eq == std::string::npos)
        continue;
      const std::string lhs = lower_copy(trim_copy(part.substr(0, eq)));
      if (lhs == key)
        return trim_copy(part.substr(eq + 1));
    }
  return {};
}

inline void finalise_absolute_pattern(Binary_Cell_Pattern &pattern,
                                      const std::vector<Coord_Vec<2>> &absolute,
                                      const bool have_extent,
                                      const ca_index_t min_row,
                                      const ca_index_t min_col,
                                      const ca_index_t max_row,
                                      const ca_index_t max_col)
{
  if (not have_extent)
    {
      pattern.width = 0;
      pattern.height = 0;
      pattern.origin_row = 0;
      pattern.origin_col = 0;
      pattern.alive.clear();
      return;
    }

  pattern.origin_row = min_row;
  pattern.origin_col = min_col;
  pattern.height = static_cast<ca_size_t>(max_row - min_row + 1);
  pattern.width = static_cast<ca_size_t>(max_col - min_col + 1);
  pattern.alive.clear();
  pattern.alive.reserve(absolute.size());
  for (const auto &c : absolute)
    pattern.alive.push_back(Coord_Vec<2>{c[0] - min_row, c[1] - min_col});
}

template <typename T>
inline T parse_scalar(const std::string &token, const char *format)
{
  std::istringstream in(trim_copy(token));
  T value{};
  in >> value;
  ah_domain_error_if(not in) << format << ": cannot parse scalar '" << token << "'";
  std::string rest;
  in >> rest;
  ah_domain_error_if(not rest.empty())
    << format << ": trailing characters in scalar '" << token << "'";
  return value;
}

template <typename T>
inline T parse_json_number(const std::string &s, std::size_t &pos)
{
  while (pos < s.size() and std::isspace(static_cast<unsigned char>(s[pos])))
    ++pos;
  const std::size_t start = pos;
  if (pos < s.size() and (s[pos] == '-' or s[pos] == '+'))
    ++pos;
  while (pos < s.size() and std::isdigit(static_cast<unsigned char>(s[pos])))
    ++pos;
  if (pos < s.size() and s[pos] == '.')
    {
      ++pos;
      while (pos < s.size() and std::isdigit(static_cast<unsigned char>(s[pos])))
        ++pos;
    }
  if (pos < s.size() and (s[pos] == 'e' or s[pos] == 'E'))
    {
      ++pos;
      if (pos < s.size() and (s[pos] == '-' or s[pos] == '+'))
        ++pos;
      while (pos < s.size() and std::isdigit(static_cast<unsigned char>(s[pos])))
        ++pos;
    }
  ah_domain_error_if(start == pos) << "JSON snapshot: expected number";
  return parse_scalar<T>(s.substr(start, pos - start), "JSON snapshot");
}

inline ca_size_t parse_json_size_field(const std::string &s, const std::string &key)
{
  const std::string needle = "\"" + key + "\"";
  const std::size_t key_pos = s.find(needle);
  ah_domain_error_if(key_pos == std::string::npos) << "JSON snapshot: missing field '" << key << "'";
  std::size_t colon = s.find(':', key_pos + needle.size());
  ah_domain_error_if(colon == std::string::npos)
    << "JSON snapshot: missing ':' after '" << key << "'";
  ++colon;
  while (colon < s.size() and std::isspace(static_cast<unsigned char>(s[colon])))
    ++colon;
  const std::size_t start = colon;
  while (colon < s.size() and std::isdigit(static_cast<unsigned char>(s[colon])))
    ++colon;
  ah_domain_error_if(start == colon)
    << "JSON snapshot: field '" << key << "' is not an unsigned integer";
  return parse_size_token(s.substr(start, colon - start), key.c_str());
}

inline void ensure_stream_ok(const std::ostream &out, const char *fn)
{
  ah_runtime_error_if(not out) << fn << ": output stream failed";
}

template <typename T>
inline void write_text_scalar(std::ostream &out, const T &value)
{
  if constexpr (std::is_integral_v<T> and sizeof(T) == 1 and not std::is_same_v<T, bool>)
    out << static_cast<int>(value);
  else
    out << value;
}

}  // namespace ca_io_detail

/** @brief Write a 2D frame in Conway RLE format.
 *
 *  A cell is considered alive when it differs from `dead`. With the
 *  default options, the complete frame dimensions are preserved in the
 *  RLE header. Set `opts.trim_dead_border` for compact pattern output.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts RLE formatting options.
 *  @param[in] dead state considered dead.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 *  @par Thread-safety
 *  The function is reentrant. Concurrent writes to the same stream are not
 *  synchronised.
 */
template <typename Lattice>
inline void write_rle(std::ostream &out,
                      const Lattice &frame,
                      const RLE_Write_Options &opts = {},
                      const typename Lattice::state_type &dead = typename Lattice::state_type{})
{
  ca_io_detail::require_rank2<Lattice>("write_rle");
  using coord_t = typename Lattice::coord_type;

  for (const auto &line : ca_io_detail::split_comment_lines(opts.name))
    out << "#N " << line << '\n';
  for (const auto &line : ca_io_detail::split_comment_lines(opts.comment))
    out << "#C " << line << '\n';

  const ca_size_t rows = frame.size(0);
  const ca_size_t cols = frame.size(1);
  ca_size_t r0 = 0, c0 = 0, r1 = rows, c1 = cols;

  if (opts.trim_dead_border)
    {
      bool any = false;
      ca_size_t min_r = rows, min_c = cols, max_r = 0, max_c = 0;
      for (ca_size_t r = 0; r < rows; ++r)
        for (ca_size_t c = 0; c < cols; ++c)
          if (frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) != dead)
            {
              any = true;
              min_r = std::min(min_r, r);
              min_c = std::min(min_c, c);
              max_r = std::max(max_r, r);
              max_c = std::max(max_c, c);
            }
      if (any)
        {
          r0 = min_r;
          c0 = min_c;
          r1 = max_r + 1;
          c1 = max_c + 1;
        }
      else
        {
          r0 = c0 = r1 = c1 = 0;
        }
    }

  const ca_size_t height = r1 - r0;
  const ca_size_t width = c1 - c0;
  out << "x = " << width << ", y = " << height << ", rule = " << opts.rule << '\n';

  std::string body;
  if (height == 0 or width == 0)
    {
      body = "!";
    }
  else
    {
      for (ca_size_t r = r0; r < r1; ++r)
        {
          bool run_alive
            = frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c0)}) != dead;
          ca_size_t run = 0;
          for (ca_size_t c = c0; c < c1; ++c)
            {
              const bool alive
                = frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) != dead;
              if (alive == run_alive)
                ++run;
              else
                {
                  ca_io_detail::append_rle_token(body, run, run_alive ? 'o' : 'b');
                  run_alive = alive;
                  run = 1;
                }
            }
          ca_io_detail::append_rle_token(body, run, run_alive ? 'o' : 'b');
          if (r + 1 < r1)
            ca_io_detail::append_rle_token(body, 1, '$');
        }
      body.push_back('!');
    }

  ca_io_detail::write_wrapped_rle_body(out, body, opts.line_width);
  ca_io_detail::ensure_stream_ok(out, "write_rle");
}

/** @brief Return a frame encoded as Conway RLE.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in] frame lattice frame.
 *  @param[in] opts RLE formatting options.
 *  @param[in] dead state considered dead.
 *  @return RLE document.
 */
template <typename Lattice>
[[nodiscard]] inline std::string write_rle_string(const Lattice &frame,
                                                  const RLE_Write_Options &opts = {},
                                                  const typename Lattice::state_type &dead
                                                  = typename Lattice::state_type{})
{
  std::ostringstream out;
  write_rle(out, frame, opts, dead);
  return out.str();
}

/** @brief Read a Conway RLE pattern.
 *
 *  Accepts comments, mixed whitespace, `b`/`.` dead cells and `o`/`A`
 *  live cells. Dimensions come from the required `x = ..., y = ...`
 *  header.
 *
 *  @param[in,out] in input stream.
 *  @return parsed binary pattern.
 *  @throws std::domain_error on malformed RLE.
 *
 *  @par Complexity
 *  O(number of RLE tokens + live cells).
 */
[[nodiscard]] inline RLE_Pattern read_rle(std::istream &in)
{
  RLE_Pattern pattern;
  std::string line;
  bool header_seen = false;
  ca_size_t cur_row = 0;
  ca_size_t cur_col = 0;
  ca_size_t run = 0;
  bool ended = false;

  while (not ended and std::getline(in, line))
    {
      if (line.empty())
        continue;
      if (line[0] == '#')
        {
          if (line.size() >= 2 and (line[1] == 'N' or line[1] == 'n'))
            pattern.name = ca_io_detail::trim_copy(line.substr(2));
          else if (line.size() >= 2 and (line[1] == 'C' or line[1] == 'c'))
            pattern.comments.push_back(ca_io_detail::trim_copy(line.substr(2)));
          continue;
        }

      if (not header_seen)
        {
          const std::string x = ca_io_detail::header_value(line, "x");
          const std::string y = ca_io_detail::header_value(line, "y");
          ah_domain_error_if(x.empty() or y.empty())
            << "read_rle: missing x/y header in line '" << line << "'";
          pattern.width = ca_io_detail::parse_size_token(x, "read_rle x");
          pattern.height = ca_io_detail::parse_size_token(y, "read_rle y");
          pattern.rule = ca_io_detail::header_value(line, "rule");
          header_seen = true;
          continue;
        }

      for (const char ch : line)
        {
          if (std::isdigit(static_cast<unsigned char>(ch)))
            {
              const ca_size_t digit = static_cast<ca_size_t>(ch - '0');
              ah_overflow_error_if(run > (std::numeric_limits<ca_size_t>::max() - digit) / 10)
                << "read_rle: run length overflows ca_size_t";
              run = run * 10 + digit;
            }
          else if (ch == 'b' or ch == '.')
            {
              const ca_size_t n = run == 0 ? 1 : run;
              ah_domain_error_if(cur_col + n > pattern.width)
                << "read_rle: dead run exceeds row width";
              cur_col += n;
              run = 0;
            }
          else if (ch == 'o' or ch == 'A')
            {
              const ca_size_t n = run == 0 ? 1 : run;
              ah_domain_error_if(cur_row >= pattern.height or cur_col + n > pattern.width)
                << "read_rle: live run exceeds declared dimensions";
              for (ca_size_t k = 0; k < n; ++k)
                pattern.alive.push_back(Coord_Vec<2>{static_cast<ca_index_t>(cur_row),
                                                     static_cast<ca_index_t>(cur_col + k)});
              cur_col += n;
              run = 0;
            }
          else if (ch == '$')
            {
              const ca_size_t n = run == 0 ? 1 : run;
              ah_domain_error_if(cur_row + n > pattern.height)
                << "read_rle: row advance exceeds declared height";
              cur_row += n;
              cur_col = 0;
              run = 0;
            }
          else if (ch == '!')
            {
              ended = true;
              run = 0;
              break;
            }
          else if (std::isspace(static_cast<unsigned char>(ch)))
            {
              // whitespace is accepted between tokens
            }
          else
            ah_domain_error_if(true) << "read_rle: invalid body character '" << ch << "'";
        }
    }

  ah_domain_error_if(not header_seen) << "read_rle: missing header";
  ah_domain_error_if(run != 0) << "read_rle: dangling run length";
  return pattern;
}

/** @brief Read a Conway RLE pattern from a string.
 *  @param[in] s RLE document.
 *  @return parsed pattern.
 */
[[nodiscard]] inline RLE_Pattern read_rle_string(const std::string &s)
{
  std::istringstream in(s);
  return read_rle(in);
}

/** @brief Fill a lattice from a binary pattern.
 *
 *  The destination is cleared to `dead`, then all pattern live cells are
 *  stamped as `alive` at `(origin_row, origin_col)`.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in] pattern source binary pattern.
 *  @param[in,out] frame destination lattice.
 *  @param[in] alive state assigned to live pattern cells.
 *  @param[in] dead state used to clear the frame.
 *  @param[in] origin_row row offset in the destination.
 *  @param[in] origin_col column offset in the destination.
 *  @throws std::domain_error if the pattern does not fit.
 *
 *  @par Complexity
 *  O(frame cells + live cells).
 */
template <typename Lattice>
inline void load_binary_pattern(const Binary_Cell_Pattern &pattern,
                                Lattice &frame,
                                const typename Lattice::state_type &alive
                                = static_cast<typename Lattice::state_type>(1),
                                const typename Lattice::state_type &dead
                                = typename Lattice::state_type{},
                                const ca_index_t origin_row = 0,
                                const ca_index_t origin_col = 0)
{
  ca_io_detail::require_rank2<Lattice>("load_binary_pattern");
  using coord_t = typename Lattice::coord_type;
  ah_domain_error_if(origin_row < 0 or origin_col < 0)
    << "load_binary_pattern: negative destination origin";
  ah_domain_error_if(static_cast<ca_size_t>(origin_row) + pattern.height > frame.size(0)
                     or static_cast<ca_size_t>(origin_col) + pattern.width > frame.size(1))
    << "load_binary_pattern: pattern " << pattern.height << "x" << pattern.width
    << " does not fit in frame " << frame.size(0) << "x" << frame.size(1);

  frame.fill(dead);
  for (const auto &cell : pattern.alive)
    frame.set(coord_t{origin_row + cell[0], origin_col + cell[1]}, alive);
}

/** @brief Construct a lattice with the exact dimensions of a pattern.
 *
 *  @tparam Lattice 2D lattice type constructible from extents and value.
 *  @param[in] pattern source binary pattern.
 *  @param[in] alive state assigned to live cells.
 *  @param[in] dead state assigned to dead cells.
 *  @return lattice containing the pattern at the top-left corner.
 *
 *  @par Complexity
 *  O(pattern area + live cells).
 */
template <typename Lattice>
[[nodiscard]] inline Lattice make_lattice_from_pattern(const Binary_Cell_Pattern &pattern,
                                                       const typename Lattice::state_type &alive
                                                       = static_cast<typename Lattice::state_type>(1),
                                                       const typename Lattice::state_type &dead
                                                       = typename Lattice::state_type{})
{
  ca_io_detail::require_rank2<Lattice>("make_lattice_from_pattern");
  Lattice frame({pattern.height, pattern.width}, dead);
  load_binary_pattern(pattern, frame, alive, dead);
  return frame;
}

/** @brief Write a Life plaintext (`.cells`) pattern.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts plaintext options.
 *  @param[in] dead state considered dead.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void write_plaintext(std::ostream &out,
                            const Lattice &frame,
                            const Plaintext_Write_Options &opts = {},
                            const typename Lattice::state_type &dead = typename Lattice::state_type{})
{
  ca_io_detail::require_rank2<Lattice>("write_plaintext");
  using coord_t = typename Lattice::coord_type;
  for (const auto &line : ca_io_detail::split_comment_lines(opts.comment))
    out << '!' << line << '\n';

  for (ca_size_t r = 0; r < frame.size(0); ++r)
    {
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        out << (frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) == dead
                  ? opts.dead
                  : opts.alive);
      out << '\n';
    }
  ca_io_detail::ensure_stream_ok(out, "write_plaintext");
}

/** @brief Return a Life plaintext document.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in] frame lattice frame.
 *  @param[in] opts plaintext options.
 *  @param[in] dead state considered dead.
 *  @return plaintext document.
 */
template <typename Lattice>
[[nodiscard]] inline std::string write_plaintext_string(const Lattice &frame,
                                                        const Plaintext_Write_Options &opts = {},
                                                        const typename Lattice::state_type &dead
                                                        = typename Lattice::state_type{})
{
  std::ostringstream out;
  write_plaintext(out, frame, opts, dead);
  return out.str();
}

/** @brief Read a Life plaintext (`.cells`) pattern.
 *
 *  Lines beginning with `!` are comments. `O`, `o` and `*` are live;
 *  `.` is dead. Whitespace inside rows is ignored.
 *
 *  @param[in,out] in input stream.
 *  @return parsed binary pattern.
 *  @throws std::domain_error on invalid row characters.
 *
 *  @par Complexity
 *  O(input characters + live cells).
 */
[[nodiscard]] inline Binary_Cell_Pattern read_plaintext(std::istream &in)
{
  Binary_Cell_Pattern pattern;
  std::vector<std::string> rows;
  std::string line;
  while (std::getline(in, line))
    {
      if (line.empty())
        {
          rows.emplace_back();
          continue;
        }
      if (line[0] == '!')
        {
          pattern.comments.push_back(ca_io_detail::trim_copy(line.substr(1)));
          continue;
        }
      std::string row;
      for (const char c : line)
        {
          if (c == 'O' or c == 'o' or c == '*' or c == '.')
            row.push_back(c);
          else if (std::isspace(static_cast<unsigned char>(c)))
            {
            }
          else
            ah_domain_error_if(true) << "read_plaintext: invalid character '" << c << "'";
        }
      rows.push_back(std::move(row));
    }

  pattern.height = rows.size();
  pattern.width = 0;
  for (const auto &row : rows)
    pattern.width = std::max(pattern.width, row.size());
  for (ca_size_t r = 0; r < rows.size(); ++r)
    for (ca_size_t c = 0; c < rows[r].size(); ++c)
      if (rows[r][c] == 'O' or rows[r][c] == 'o' or rows[r][c] == '*')
        pattern.alive.push_back(Coord_Vec<2>{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)});
  return pattern;
}

/** @brief Read a Life plaintext pattern from a string.
 *  @param[in] s plaintext document.
 *  @return parsed pattern.
 */
[[nodiscard]] inline Binary_Cell_Pattern read_plaintext_string(const std::string &s)
{
  std::istringstream in(s);
  return read_plaintext(in);
}

/** @brief Write a Life 1.06 file.
 *
 *  Live cells are emitted as `col row` integer coordinate pairs.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] dead state considered dead.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void write_life_106(std::ostream &out,
                           const Lattice &frame,
                           const typename Lattice::state_type &dead = typename Lattice::state_type{})
{
  ca_io_detail::require_rank2<Lattice>("write_life_106");
  using coord_t = typename Lattice::coord_type;
  out << "#Life 1.06\n";
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      if (frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) != dead)
        out << c << ' ' << r << '\n';
  ca_io_detail::ensure_stream_ok(out, "write_life_106");
}

/** @brief Read a Life 1.06 pattern.
 *
 *  Coordinates may be negative; the returned pattern is normalised while
 *  preserving the original minimum coordinate in `origin_row/col`.
 *
 *  @param[in,out] in input stream.
 *  @return parsed binary pattern.
 *  @throws std::domain_error on malformed input.
 *
 *  @par Complexity
 *  O(live cells).
 */
[[nodiscard]] inline Binary_Cell_Pattern read_life_106(std::istream &in)
{
  Binary_Cell_Pattern pattern;
  std::string line;
  bool header_seen = false;
  std::vector<Coord_Vec<2>> absolute;
  ca_index_t min_r = 0, min_c = 0, max_r = 0, max_c = 0;
  bool have = false;

  while (std::getline(in, line))
    {
      const std::string t = ca_io_detail::trim_copy(line);
      if (t.empty())
        continue;
      if (t[0] == '#')
        {
          if (t == "#Life 1.06")
            header_seen = true;
          else if (t.size() >= 2 and (t[1] == 'D' or t[1] == 'd'))
            pattern.comments.push_back(ca_io_detail::trim_copy(t.substr(2)));
          continue;
        }
      std::istringstream ls(t);
      std::string xs, ys, extra;
      ls >> xs >> ys >> extra;
      ah_domain_error_if(not extra.empty()) << "read_life_106: too many fields in '" << line << "'";
      const ca_index_t col = ca_io_detail::parse_index_token(xs, "read_life_106 x");
      const ca_index_t row = ca_io_detail::parse_index_token(ys, "read_life_106 y");
      absolute.push_back(Coord_Vec<2>{row, col});
      if (not have)
        {
          min_r = max_r = row;
          min_c = max_c = col;
          have = true;
        }
      else
        {
          min_r = std::min(min_r, row);
          min_c = std::min(min_c, col);
          max_r = std::max(max_r, row);
          max_c = std::max(max_c, col);
        }
    }

  ah_domain_error_if(not header_seen) << "read_life_106: missing #Life 1.06 header";
  ca_io_detail::finalise_absolute_pattern(pattern, absolute, have, min_r, min_c, max_r, max_c);
  return pattern;
}

/** @brief Write a Life 1.05 file.
 *
 *  The writer emits one `#P 0 0` block that covers the full frame. Live
 *  cells use `*`; dead cells use `.`.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] comment optional `#D` comment text.
 *  @param[in] dead state considered dead.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void write_life_105(std::ostream &out,
                           const Lattice &frame,
                           const std::string &comment = {},
                           const typename Lattice::state_type &dead = typename Lattice::state_type{})
{
  ca_io_detail::require_rank2<Lattice>("write_life_105");
  using coord_t = typename Lattice::coord_type;
  out << "#Life 1.05\n";
  for (const auto &line : ca_io_detail::split_comment_lines(comment))
    out << "#D " << line << '\n';
  out << "#P 0 0\n";
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    {
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        out << (frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) == dead
                  ? '.'
                  : '*');
      out << '\n';
    }
  ca_io_detail::ensure_stream_ok(out, "write_life_105");
}

/** @brief Read a Life 1.05 pattern.
 *
 *  Supports the common `#P x y` block dialect with `*` or `O` live cells
 *  and `.` dead cells. Multiple blocks are merged and then normalised.
 *
 *  @param[in,out] in input stream.
 *  @return parsed binary pattern.
 *  @throws std::domain_error on malformed input.
 *
 *  @par Complexity
 *  O(input characters + live cells).
 */
[[nodiscard]] inline Binary_Cell_Pattern read_life_105(std::istream &in)
{
  Binary_Cell_Pattern pattern;
  std::vector<Coord_Vec<2>> absolute;
  std::string line;
  bool header_seen = false;
  bool in_block = false;
  ca_index_t block_col = 0, block_row = 0, local_row = 0;
  ca_index_t min_r = 0, min_c = 0, max_r = 0, max_c = 0;
  bool have_extent = false;

  auto touch_extent = [&](ca_index_t row, ca_index_t col)
  {
    if (not have_extent)
      {
        min_r = max_r = row;
        min_c = max_c = col;
        have_extent = true;
      }
    else
      {
        min_r = std::min(min_r, row);
        min_c = std::min(min_c, col);
        max_r = std::max(max_r, row);
        max_c = std::max(max_c, col);
      }
  };

  while (std::getline(in, line))
    {
      const std::string t = ca_io_detail::trim_copy(line);
      if (t.empty())
        continue;
      if (t[0] == '#')
        {
          if (t == "#Life 1.05")
            {
              header_seen = true;
              continue;
            }
          if (t.size() >= 2 and (t[1] == 'D' or t[1] == 'd'))
            {
              pattern.comments.push_back(ca_io_detail::trim_copy(t.substr(2)));
              continue;
            }
          if (t.size() >= 2 and (t[1] == 'P' or t[1] == 'p'))
            {
              std::istringstream ps(t.substr(2));
              std::string xs, ys, extra;
              ps >> xs >> ys >> extra;
              ah_domain_error_if(xs.empty() or ys.empty() or not extra.empty())
                << "read_life_105: malformed #P line '" << line << "'";
              block_col = ca_io_detail::parse_index_token(xs, "read_life_105 #P x");
              block_row = ca_io_detail::parse_index_token(ys, "read_life_105 #P y");
              local_row = 0;
              in_block = true;
              continue;
            }
          continue;
        }

      ah_domain_error_if(not in_block) << "read_life_105: pattern row before #P block";
      ca_index_t local_col = 0;
      for (const char c : line)
        {
          if (c == '*' or c == 'O' or c == 'o' or c == '.')
            {
              const ca_index_t row = block_row + local_row;
              const ca_index_t col = block_col + local_col;
              touch_extent(row, col);
              if (c == '*' or c == 'O' or c == 'o')
                absolute.push_back(Coord_Vec<2>{row, col});
              ++local_col;
            }
          else if (std::isspace(static_cast<unsigned char>(c)))
            {
            }
          else
            ah_domain_error_if(true) << "read_life_105: invalid row character '" << c << "'";
        }
      ++local_row;
    }

  ah_domain_error_if(not header_seen) << "read_life_105: missing #Life 1.05 header";
  ca_io_detail::finalise_absolute_pattern(pattern, absolute, have_extent, min_r, min_c, max_r, max_c);
  return pattern;
}

/** @brief Write a numeric CSV snapshot.
 *
 *  Each lattice row becomes one CSV record. Values are emitted with
 *  `operator<<`.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts CSV options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void write_csv(std::ostream &out, const Lattice &frame, const CSV_Options &opts = {})
{
  ca_io_detail::require_rank2<Lattice>("write_csv");
  using coord_t = typename Lattice::coord_type;
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    {
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        {
          if (c != 0)
            out << opts.delimiter;
          ca_io_detail::write_text_scalar(
            out, frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
        }
      out << '\n';
    }
  ca_io_detail::ensure_stream_ok(out, "write_csv");
}

/** @brief Read a numeric CSV snapshot.
 *
 *  The reader supports rectangular, delimiter-separated numeric matrices.
 *  Quoting is intentionally not implemented because CA snapshots are plain
 *  numeric grids.
 *
 *  @tparam T target scalar type.
 *  @param[in,out] in input stream.
 *  @param[in] opts CSV options.
 *  @return row-major snapshot.
 *  @throws std::domain_error if rows have inconsistent widths or a scalar
 *          cannot be parsed.
 *
 *  @par Complexity
 *  O(input cells).
 */
template <typename T>
[[nodiscard]] inline Grid_Snapshot<T> read_csv_snapshot(std::istream &in,
                                                        const CSV_Options &opts = {})
{
  Grid_Snapshot<T> snap;
  std::string line;
  ca_size_t row = 0;
  while (std::getline(in, line))
    {
      if (opts.skip_empty_lines and ca_io_detail::blank(line))
        continue;
      std::vector<T> values;
      std::string token;
      std::istringstream ls(line);
      while (std::getline(ls, token, opts.delimiter))
        values.push_back(ca_io_detail::parse_scalar<T>(token, "CSV snapshot"));
      if (line.empty() and not opts.skip_empty_lines)
        values.push_back(T{});
      if (row == 0)
        snap.width = values.size();
      ah_domain_error_if(values.size() != snap.width)
        << "read_csv_snapshot: row " << row << " has width " << values.size() << " instead of "
        << snap.width;
      for (const T &v : values)
        snap.cells.push_back(v);
      ++row;
    }
  snap.height = row;
  return snap;
}

/** @brief Construct a lattice from a numeric snapshot.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam T snapshot scalar type.
 *  @param[in] snap source snapshot.
 *  @return lattice with `static_cast<state_type>` converted cells.
 *
 *  @par Complexity
 *  O(snapshot cells).
 */
template <typename Lattice, typename T>
[[nodiscard]] inline Lattice make_lattice_from_snapshot(const Grid_Snapshot<T> &snap)
{
  ca_io_detail::require_rank2<Lattice>("make_lattice_from_snapshot");
  using state_t = typename Lattice::state_type;
  using coord_t = typename Lattice::coord_type;
  Lattice frame({snap.height, snap.width}, state_t{});
  for (ca_size_t r = 0; r < snap.height; ++r)
    for (ca_size_t c = 0; c < snap.width; ++c)
      frame.set(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)},
                static_cast<state_t>(snap.at(r, c)));
  return frame;
}

/** @brief Write a JSON snapshot.
 *
 *  Schema:
 *
 *  @code
 *  {"type":"Aleph::CA::frame","rank":2,"height":H,"width":W,"cells":[...]}
 *  @endcode
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts JSON options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice>
inline void write_json(std::ostream &out, const Lattice &frame, const JSON_Write_Options &opts = {})
{
  ca_io_detail::require_rank2<Lattice>("write_json");
  using coord_t = typename Lattice::coord_type;

  const auto nl = opts.pretty ? "\n" : "";
  const auto i1 = opts.pretty ? "  " : "";
  const auto i2 = opts.pretty ? "    " : "";
  const auto sp = opts.pretty ? " " : "";

  out << '{' << nl;
  out << i1 << "\"type\":" << sp << "\"Aleph::CA::frame\"," << nl;
  out << i1 << "\"rank\":" << sp << "2," << nl;
  out << i1 << "\"height\":" << sp << frame.size(0) << ',' << nl;
  out << i1 << "\"width\":" << sp << frame.size(1);
  if (not opts.rule.empty())
    out << ',' << nl << i1 << "\"rule\":" << sp << '"' << ca_io_detail::json_escape(opts.rule) << '"';
  out << ',' << nl << i1 << "\"cells\":" << sp << '[' << nl;
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    {
      out << i2 << '[';
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        {
          if (c != 0)
            out << ',';
          if (opts.pretty)
            out << ' ';
          ca_io_detail::write_text_scalar(
            out, frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
        }
      if (opts.pretty and frame.size(1) != 0)
        out << ' ';
      out << ']';
      if (r + 1 < frame.size(0))
        out << ',';
      out << nl;
    }
  out << i1 << ']' << nl << "}" << nl;
  ca_io_detail::ensure_stream_ok(out, "write_json");
}

/** @brief Read a JSON numeric snapshot produced by `write_json`.
 *
 *  This intentionally small parser handles the deterministic schema emitted
 *  by this header. It supports integer and floating-point numeric cell
 *  values, but not arbitrary user JSON.
 *
 *  @tparam T target scalar type.
 *  @param[in,out] in input stream.
 *  @return row-major snapshot.
 *  @throws std::domain_error on malformed JSON or shape mismatch.
 *
 *  @par Complexity
 *  O(input characters + cells).
 */
template <typename T>
[[nodiscard]] inline Grid_Snapshot<T> read_json_snapshot(std::istream &in)
{
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string s = ss.str();
  Grid_Snapshot<T> snap;
  snap.height = ca_io_detail::parse_json_size_field(s, "height");
  snap.width = ca_io_detail::parse_json_size_field(s, "width");

  const std::size_t key = s.find("\"cells\"");
  ah_domain_error_if(key == std::string::npos) << "JSON snapshot: missing cells field";
  std::size_t pos = s.find('[', key);
  ah_domain_error_if(pos == std::string::npos) << "JSON snapshot: missing cells array";
  ++pos;

  ca_size_t rows = 0;
  while (true)
    {
      while (pos < s.size() and (std::isspace(static_cast<unsigned char>(s[pos])) or s[pos] == ','))
        ++pos;
      ah_domain_error_if(pos >= s.size()) << "JSON snapshot: unterminated cells array";
      if (s[pos] == ']')
        {
          ++pos;
          break;
        }
      ah_domain_error_if(s[pos] != '[') << "JSON snapshot: expected row array";
      ++pos;
      ca_size_t cols = 0;
      while (true)
        {
          while (pos < s.size() and std::isspace(static_cast<unsigned char>(s[pos])))
            ++pos;
          ah_domain_error_if(pos >= s.size()) << "JSON snapshot: unterminated row";
          if (s[pos] == ']')
            {
              ++pos;
              break;
            }
          if (cols != 0)
            {
              ah_domain_error_if(s[pos] != ',') << "JSON snapshot: expected comma";
              ++pos;
            }
          snap.cells.push_back(ca_io_detail::parse_json_number<T>(s, pos));
          ++cols;
        }
      ah_domain_error_if(cols != snap.width)
        << "JSON snapshot: row " << rows << " has width " << cols << " instead of " << snap.width;
      ++rows;
    }

  ah_domain_error_if(rows != snap.height)
    << "JSON snapshot: got " << rows << " rows instead of " << snap.height;
  return snap;
}

/** @brief Write a binary PGM (`P5`) image.
 *
 *  `mapper(cell)` must return a grayscale byte. The top lattice row is
 *  the first image row.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Mapper invocable `state_type -> uint8_t`.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] mapper grayscale mapper.
 *  @param[in] opts NetPBM options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Mapper>
inline void write_pgm(std::ostream &out,
                      const Lattice &frame,
                      Mapper &&mapper,
                      const NetPBM_Write_Options &opts = {})
  requires requires(Mapper &&m, const typename Lattice::state_type &v) {
    { m(v) } -> std::convertible_to<std::uint8_t>;
  }
{
  ca_io_detail::require_rank2<Lattice>("write_pgm");
  using coord_t = typename Lattice::coord_type;
  ah_domain_error_if(opts.max_value == 0 or opts.max_value > 255)
    << "write_pgm: only 8-bit max_value in [1,255] is supported";
  out << "P5\n" << frame.size(1) << ' ' << frame.size(0) << '\n' << opts.max_value << '\n';
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      {
        const std::uint8_t v = static_cast<std::uint8_t>(
          mapper(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)})));
        out.write(reinterpret_cast<const char *>(&v), 1);
      }
  ca_io_detail::ensure_stream_ok(out, "write_pgm");
}

/** @brief Write a binary PGM (`P5`) image using the default binary mapper.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts NetPBM options.
 */
template <typename Lattice>
inline void write_pgm(std::ostream &out, const Lattice &frame, const NetPBM_Write_Options &opts = {})
{
  write_pgm(out, frame, Binary_Gray_Mapper<typename Lattice::state_type>{}, opts);
}

/** @brief Write a binary PPM (`P6`) image.
 *
 *  `mapper(cell)` must return an `RGB8` triplet.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Mapper invocable `state_type -> RGB8`.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] mapper RGB mapper.
 *  @param[in] opts NetPBM options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Mapper>
inline void write_ppm(std::ostream &out,
                      const Lattice &frame,
                      Mapper &&mapper,
                      const NetPBM_Write_Options &opts = {})
  requires requires(Mapper &&m, const typename Lattice::state_type &v) {
    { m(v) } -> std::convertible_to<RGB8>;
  }
{
  ca_io_detail::require_rank2<Lattice>("write_ppm");
  using coord_t = typename Lattice::coord_type;
  ah_domain_error_if(opts.max_value == 0 or opts.max_value > 255)
    << "write_ppm: only 8-bit max_value in [1,255] is supported";
  out << "P6\n" << frame.size(1) << ' ' << frame.size(0) << '\n' << opts.max_value << '\n';
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      {
        const RGB8 rgb
          = mapper(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
        const char bytes[3]
          = {static_cast<char>(rgb.r), static_cast<char>(rgb.g), static_cast<char>(rgb.b)};
        out.write(bytes, 3);
      }
  ca_io_detail::ensure_stream_ok(out, "write_ppm");
}

/** @brief Write a binary PPM (`P6`) image using the default binary mapper.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame lattice frame.
 *  @param[in] opts NetPBM options.
 */
template <typename Lattice>
inline void write_ppm(std::ostream &out, const Lattice &frame, const NetPBM_Write_Options &opts = {})
{
  write_ppm(out, frame, Binary_RGB_Mapper<typename Lattice::state_type>{}, opts);
}

/** @brief Streaming JSON writer for CA trajectories.
 *
 *  The writer emits one JSON object with a `frames` array. Each accepted
 *  frame contains its `step` and a full snapshot. The caller must call
 *  `close()` (or let the destructor run on an open stream that tolerates
 *  best-effort writes) to finish the JSON document.
 *
 *  @par Thread-safety
 *  Not thread-safe; external synchronisation is required if several
 *  threads write to the same stream.
 */
class JSON_Frame_Stream_Writer
{
  std::ostream *out_ = nullptr;
  bool first_ = true;
  bool closed_ = false;
  JSON_Write_Options opts_{};

public:
  /** @brief Start a JSON frame stream.
   *  @param[in,out] out destination stream.
   *  @param[in] opts JSON options used for every frame.
   *  @throws std::runtime_error on stream write failure.
   *
   *  @par Complexity
   *  O(1).
   */
  explicit JSON_Frame_Stream_Writer(std::ostream &out, JSON_Write_Options opts = {})
    : out_(&out), opts_(std::move(opts))
  {
    *out_ << "{\n  \"type\": \"Aleph::CA::frame-stream\",\n  \"frames\": [\n";
    ca_io_detail::ensure_stream_ok(*out_, "JSON_Frame_Stream_Writer");
  }

  JSON_Frame_Stream_Writer(const JSON_Frame_Stream_Writer &) = delete;
  JSON_Frame_Stream_Writer &operator=(const JSON_Frame_Stream_Writer &) = delete;

  /** @brief Finish the JSON document if it is still open.
   *
   *  Destructors must not throw; call `close()` explicitly when the caller
   *  needs to observe stream errors.
   */
  ~JSON_Frame_Stream_Writer()
  {
    if (out_ != nullptr and not closed_)
      {
        try
          {
            close();
          }
        catch (...)
          {}
      }
  }

  /** @brief Append one frame to the stream.
   *
   *  @tparam Lattice 2D lattice type.
   *  @param[in] step step index associated with the frame.
   *  @param[in] frame lattice frame.
   *  @throws std::runtime_error on stream write failure.
   *
   *  @par Complexity
   *  O(rows * cols).
   */
  template <typename Lattice>
  void accept(std::size_t step, const Lattice &frame)
  {
    ca_io_detail::require_rank2<Lattice>("JSON_Frame_Stream_Writer::accept");
    ah_runtime_error_if(closed_) << "JSON_Frame_Stream_Writer::accept: stream already closed";
    if (not first_)
      *out_ << ",\n";
    first_ = false;
    *out_ << "    {\n      \"step\": " << step << ",\n      \"frame\": ";
    JSON_Write_Options compact = opts_;
    compact.pretty = false;
    write_json(*out_, frame, compact);
    *out_ << "    }";
    ca_io_detail::ensure_stream_ok(*out_, "JSON_Frame_Stream_Writer::accept");
  }

  /** @brief Finish the stream.
   *
   *  @throws std::runtime_error on stream write failure.
   *
   *  @par Complexity
   *  O(1).
   */
  void close()
  {
    if (closed_)
      return;
    *out_ << "\n  ]\n}\n";
    closed_ = true;
    ca_io_detail::ensure_stream_ok(*out_, "JSON_Frame_Stream_Writer::close");
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_IO_H
