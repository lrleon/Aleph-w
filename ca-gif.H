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

/** @file ca-gif.H
 *  @brief Animated GIF sink for cellular-automata trajectories.
 *
 *  The implementation is self-contained: frames are quantised to a
 *  global palette of at most 256 colours and encoded with baseline GIF
 *  LZW codes. This keeps the backend header-only and dependency-free.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_GIF_H
#define CA_GIF_H

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Options for animated GIF output. */
struct GIF_Write_Options
{
  unsigned delay_cs = 6;  ///< frame delay in centiseconds
  bool loop = true;       ///< add Netscape infinite-loop extension
};

namespace ca_gif_detail {

struct Frame
{
  ca_size_t width = 0;
  ca_size_t height = 0;
  std::vector<RGB8> pixels;
};

inline void put_le16(std::ostream &out, const std::uint16_t value)
{
  out.put(static_cast<char>(value & 0xffu));
  out.put(static_cast<char>((value >> 8) & 0xffu));
}

[[nodiscard]] inline std::uint32_t dist2(const RGB8 a, const RGB8 b) noexcept
{
  const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
  const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
  const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
  return static_cast<std::uint32_t>(dr * dr + dg * dg + db * db);
}

[[nodiscard]] inline std::uint8_t palette_index(std::vector<RGB8> &palette, const RGB8 c)
{
  for (std::size_t i = 0; i < palette.size(); ++i)
    if (palette[i] == c)
      return static_cast<std::uint8_t>(i);
  if (palette.size() < 256)
    {
      palette.push_back(c);
      return static_cast<std::uint8_t>(palette.size() - 1);
    }

  std::size_t best = 0;
  std::uint32_t best_d = std::numeric_limits<std::uint32_t>::max();
  for (std::size_t i = 0; i < palette.size(); ++i)
    {
      const std::uint32_t d = dist2(palette[i], c);
      if (d < best_d)
        {
          best = i;
          best_d = d;
        }
    }
  return static_cast<std::uint8_t>(best);
}

[[nodiscard]] inline unsigned ceil_log2(std::size_t n)
{
  unsigned bits = 0;
  std::size_t p = 1;
  while (p < n)
    {
      p <<= 1;
      ++bits;
    }
  return bits;
}

class Bit_Writer
{
  std::vector<std::uint8_t> bytes_;
  std::uint32_t buffer_ = 0;
  unsigned bits_ = 0;

public:
  void write(const unsigned code, const unsigned width)
  {
    buffer_ |= static_cast<std::uint32_t>(code) << bits_;
    bits_ += width;
    while (bits_ >= 8)
      {
        bytes_.push_back(static_cast<std::uint8_t>(buffer_ & 0xffu));
        buffer_ >>= 8;
        bits_ -= 8;
      }
  }

  [[nodiscard]] std::vector<std::uint8_t> finish()
  {
    if (bits_ != 0)
      bytes_.push_back(static_cast<std::uint8_t>(buffer_ & 0xffu));
    return std::move(bytes_);
  }
};

[[nodiscard]] inline std::vector<std::uint8_t> lzw_bytes(const std::vector<std::uint8_t> &indices,
                                                         const unsigned min_code_size)
{
  const unsigned clear = 1u << min_code_size;
  const unsigned end = clear + 1u;
  unsigned code_size = min_code_size + 1u;
  unsigned next_code = end + 1u;
  bool first = true;

  Bit_Writer bits;
  bits.write(clear, code_size);
  for (const std::uint8_t idx : indices)
    {
      bits.write(idx, code_size);
      if (first)
        {
          first = false;
          continue;
        }
      if (next_code < 4096u)
        {
          ++next_code;
          if (next_code == (1u << code_size) and code_size < 12u)
            ++code_size;
        }
    }
  bits.write(end, code_size);
  return bits.finish();
}

inline void write_subblocks(std::ostream &out, const std::vector<std::uint8_t> &bytes)
{
  std::size_t pos = 0;
  while (pos < bytes.size())
    {
      const std::size_t n = std::min<std::size_t>(255, bytes.size() - pos);
      out.put(static_cast<char>(n));
      out.write(reinterpret_cast<const char *>(bytes.data() + pos), static_cast<std::streamsize>(n));
      pos += n;
    }
  out.put('\0');
}

[[nodiscard]] inline std::vector<RGB8> build_palette(const std::vector<Frame> &frames)
{
  std::vector<RGB8> palette;
  palette.reserve(256);
  for (const Frame &f : frames)
    for (const RGB8 c : f.pixels)
      (void) palette_index(palette, c);
  if (palette.empty())
    palette.push_back(RGB8{255, 255, 255});
  if (palette.size() == 1)
    palette.push_back(palette.front() == RGB8{0, 0, 0} ? RGB8{255, 255, 255} : RGB8{0, 0, 0});
  return palette;
}

}  // namespace ca_gif_detail

/** @brief Write a sequence of RGB frames as an animated GIF.
 *
 *  @param[in,out] out output stream.
 *  @param[in] frames RGB frames, all with identical dimensions.
 *  @param[in] opts GIF options.
 *  @throws std::domain_error if no frames are supplied or dimensions differ.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(frames * width * height).
 */
inline void write_gif(std::ostream &out,
                      const std::vector<ca_gif_detail::Frame> &frames,
                      const GIF_Write_Options &opts = {})
{
  ah_domain_error_if(frames.empty()) << "write_gif: at least one frame is required";
  const ca_size_t width = frames.front().width;
  const ca_size_t height = frames.front().height;
  ah_domain_error_if(width == 0 or height == 0) << "write_gif: dimensions must be non-zero";
  for (const auto &f : frames)
    ah_domain_error_if(f.width != width or f.height != height or f.pixels.size() != width * height)
      << "write_gif: all frames must have identical dimensions";

  std::vector<RGB8> palette = ca_gif_detail::build_palette(frames);
  const unsigned palette_bits = std::max(1u, ca_gif_detail::ceil_log2(palette.size()));
  const std::size_t table_size = std::size_t{1} << palette_bits;
  palette.resize(table_size, RGB8{0, 0, 0});
  const unsigned min_code_size = std::max(2u, palette_bits);

  out.write("GIF89a", 6);
  ca_gif_detail::put_le16(out, static_cast<std::uint16_t>(width));
  ca_gif_detail::put_le16(out, static_cast<std::uint16_t>(height));
  const unsigned gct_code = palette_bits - 1u;
  out.put(static_cast<char>(0x80u | 0x70u | gct_code));
  out.put('\0');
  out.put('\0');
  for (const RGB8 c : palette)
    {
      out.put(static_cast<char>(c.r));
      out.put(static_cast<char>(c.g));
      out.put(static_cast<char>(c.b));
    }

  if (opts.loop)
    {
      out.put(0x21);
      out.put(0xff);
      out.put(0x0b);
      out.write("NETSCAPE2.0", 11);
      out.put(0x03);
      out.put(0x01);
      ca_gif_detail::put_le16(out, 0);
      out.put('\0');
    }

  for (const auto &frame : frames)
    {
      std::vector<std::uint8_t> indices;
      indices.reserve(frame.pixels.size());
      std::vector<RGB8> palette_for_index = palette;
      for (const RGB8 c : frame.pixels)
        indices.push_back(ca_gif_detail::palette_index(palette_for_index, c));

      out.put(0x21);
      out.put(0xf9);
      out.put(0x04);
      out.put(0x00);
      ca_gif_detail::put_le16(out, static_cast<std::uint16_t>(opts.delay_cs));
      out.put(0x00);
      out.put(0x00);

      out.put(0x2c);
      ca_gif_detail::put_le16(out, 0);
      ca_gif_detail::put_le16(out, 0);
      ca_gif_detail::put_le16(out, static_cast<std::uint16_t>(width));
      ca_gif_detail::put_le16(out, static_cast<std::uint16_t>(height));
      out.put(0x00);
      out.put(static_cast<char>(min_code_size));
      ca_gif_detail::write_subblocks(out, ca_gif_detail::lzw_bytes(indices, min_code_size));
    }

  out.put(';');
  ah_runtime_error_if(not out) << "write_gif: output stream failed";
}

/** @brief Sink that buffers frames and writes an animated GIF on flush.
 *
 *  @tparam Mapper invocable `state_type -> RGB8`.
 */
template <typename Mapper>
class Gif_Frame_Sink
{
  std::filesystem::path path_;
  Mapper mapper_;
  GIF_Write_Options opts_{};
  std::vector<ca_gif_detail::Frame> frames_;

public:
  /** @brief Build a GIF frame sink.
   *  @param[in] path output GIF path.
   *  @param[in] mapper state-to-RGB mapper.
   *  @param[in] opts GIF options.
   */
  Gif_Frame_Sink(std::filesystem::path path, Mapper mapper, GIF_Write_Options opts = {})
    : path_(std::move(path)), mapper_(std::move(mapper)), opts_(opts)
  {}

  /** @brief Collect one frame.
   *  @tparam Lattice rank-2 lattice type.
   *  @param[in] step simulation step, accepted for interface symmetry.
   *  @param[in] frame frame to collect.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    (void) step;
    static_assert(LatticeLike<Lattice>, "Gif_Frame_Sink requires LatticeLike frames");
    static_assert(Lattice::rank == 2, "Gif_Frame_Sink requires rank-2 frames");
    using coord_t = typename Lattice::coord_type;

    ca_gif_detail::Frame f;
    f.width = frame.size(1);
    f.height = frame.size(0);
    f.pixels.reserve(f.width * f.height);
    for (ca_size_t r = 0; r < frame.size(0); ++r)
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        f.pixels.push_back(
          mapper_(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)})));
    if (not frames_.empty())
      ah_domain_error_if(frames_.front().width != f.width or frames_.front().height != f.height)
        << "Gif_Frame_Sink::accept: frame dimensions changed";
    frames_.push_back(std::move(f));
  }

  /** @brief Write the animated GIF.
   *  @throws std::runtime_error on file or stream failure.
   */
  void flush() const
  {
    if (const auto parent = path_.parent_path(); not parent.empty())
      std::filesystem::create_directories(parent);
    std::ofstream out(path_, std::ios::binary);
    ah_runtime_error_if(not out) << "Gif_Frame_Sink::flush: cannot open '" << path_.string() << "'";
    write_gif(out, frames_, opts_);
  }

  /** @brief Return buffered frame count.
   *  @return number of collected frames.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return frames_.size();
  }
};

template <typename Mapper>
Gif_Frame_Sink(std::filesystem::path, Mapper, GIF_Write_Options = {}) -> Gif_Frame_Sink<Mapper>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_GIF_H
