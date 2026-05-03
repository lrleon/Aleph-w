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

/** @file ca-png.H
 *  @brief Dependency-free PNG frame sink for cellular automata.
 *
 *  The writer emits standards-compliant 8-bit RGB PNG files using PNG
 *  filter type 0 and zlib stored blocks. This keeps the backend
 *  header-only and avoids a build-time dependency while still producing
 *  files accepted by browsers, image viewers and scientific notebooks.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_PNG_H
#define CA_PNG_H

#include <array>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_png_detail {

inline void append_be32(std::vector<std::uint8_t> &out, const std::uint32_t value)
{
  out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffu));
  out.push_back(static_cast<std::uint8_t>(value & 0xffu));
}

inline void write_be32(std::ostream &out, const std::uint32_t value)
{
  const char bytes[4]
    = {static_cast<char>((value >> 24) & 0xffu), static_cast<char>((value >> 16) & 0xffu),
       static_cast<char>((value >> 8) & 0xffu), static_cast<char>(value & 0xffu)};
  out.write(bytes, 4);
}

[[nodiscard]] inline std::uint32_t crc32(const std::uint8_t *data, const std::size_t n)
{
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < n; ++i)
    {
      crc ^= data[i];
      for (int k = 0; k < 8; ++k)
        crc = (crc & 1u) != 0 ? 0xedb88320u ^ (crc >> 1) : (crc >> 1);
    }
  return crc ^ 0xffffffffu;
}

[[nodiscard]] inline std::uint32_t adler32(const std::vector<std::uint8_t> &data)
{
  constexpr std::uint32_t mod = 65521u;
  std::uint32_t a = 1;
  std::uint32_t b = 0;
  for (const std::uint8_t byte : data)
    {
      a = (a + byte) % mod;
      b = (b + a) % mod;
    }
  return (b << 16) | a;
}

[[nodiscard]] inline std::vector<std::uint8_t> zlib_store(const std::vector<std::uint8_t> &raw)
{
  std::vector<std::uint8_t> out;
  out.reserve(raw.size() + raw.size() / 65535 + 16);
  out.push_back(0x78);
  out.push_back(0x01);

  std::size_t pos = 0;
  do
    {
      const std::size_t remaining = raw.size() - pos;
      const std::uint16_t block = static_cast<std::uint16_t>(remaining > 65535u ? 65535u : remaining);
      const bool final = pos + block == raw.size();
      out.push_back(final ? 0x01 : 0x00);
      out.push_back(static_cast<std::uint8_t>(block & 0xffu));
      out.push_back(static_cast<std::uint8_t>((block >> 8) & 0xffu));
      const std::uint16_t nlen = static_cast<std::uint16_t>(~block);
      out.push_back(static_cast<std::uint8_t>(nlen & 0xffu));
      out.push_back(static_cast<std::uint8_t>((nlen >> 8) & 0xffu));
      out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos),
                 raw.begin() + static_cast<std::ptrdiff_t>(pos + block));
      pos += block;
  } while (pos < raw.size());

  append_be32(out, adler32(raw));
  return out;
}

inline void write_chunk(std::ostream &out, const char type[4], const std::vector<std::uint8_t> &payload)
{
  write_be32(out, static_cast<std::uint32_t>(payload.size()));
  out.write(type, 4);
  if (not payload.empty())
    out.write(reinterpret_cast<const char *>(payload.data()),
              static_cast<std::streamsize>(payload.size()));

  std::vector<std::uint8_t> crc_data;
  crc_data.reserve(4 + payload.size());
  for (int i = 0; i < 4; ++i)
    crc_data.push_back(static_cast<std::uint8_t>(type[i]));
  crc_data.insert(crc_data.end(), payload.begin(), payload.end());
  write_be32(out, crc32(crc_data.data(), crc_data.size()));
}

}  // namespace ca_png_detail

/** @brief Write a rank-2 frame as an 8-bit RGB PNG image.
 *
 *  `mapper(cell)` must return an `RGB8` colour. The function writes a
 *  complete PNG stream to `out` without closing it.
 *
 *  @tparam Lattice 2D lattice type.
 *  @tparam Mapper invocable `state_type -> RGB8`.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to encode.
 *  @param[in] mapper state-to-RGB mapper.
 *  @throws std::domain_error if the frame has zero width or height.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(rows * cols).
 */
template <typename Lattice, typename Mapper>
inline void write_png(std::ostream &out, const Lattice &frame, Mapper &&mapper)
  requires requires(Mapper &&m, const typename Lattice::state_type &v) {
    { m(v) } -> std::convertible_to<RGB8>;
  }
{
  static_assert(LatticeLike<Lattice>, "write_png requires a LatticeLike frame");
  static_assert(Lattice::rank == 2, "write_png requires a rank-2 frame");
  using coord_t = typename Lattice::coord_type;

  const ca_size_t rows = frame.size(0);
  const ca_size_t cols = frame.size(1);
  ah_domain_error_if(rows == 0 or cols == 0) << "write_png: PNG dimensions must be non-zero";

  std::vector<std::uint8_t> raw;
  raw.reserve(rows * (1 + cols * 3));
  for (ca_size_t r = 0; r < rows; ++r)
    {
      raw.push_back(0);
      for (ca_size_t c = 0; c < cols; ++c)
        {
          const RGB8 rgb
            = mapper(frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
          raw.push_back(rgb.r);
          raw.push_back(rgb.g);
          raw.push_back(rgb.b);
        }
    }

  static constexpr unsigned char signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  out.write(reinterpret_cast<const char *>(signature), 8);

  std::vector<std::uint8_t> ihdr;
  ca_png_detail::append_be32(ihdr, static_cast<std::uint32_t>(cols));
  ca_png_detail::append_be32(ihdr, static_cast<std::uint32_t>(rows));
  ihdr.push_back(8);  // bit depth
  ihdr.push_back(2);  // truecolour RGB
  ihdr.push_back(0);  // deflate
  ihdr.push_back(0);  // adaptive filtering
  ihdr.push_back(0);  // no interlace
  ca_png_detail::write_chunk(out, "IHDR", ihdr);

  const std::vector<std::uint8_t> idat = ca_png_detail::zlib_store(raw);
  ca_png_detail::write_chunk(out, "IDAT", idat);
  ca_png_detail::write_chunk(out, "IEND", {});

  ah_runtime_error_if(not out) << "write_png: output stream failed";
}

/** @brief Write a frame as PNG using the default binary RGB mapper.
 *
 *  @tparam Lattice 2D lattice type.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to encode.
 */
template <typename Lattice>
inline void write_png(std::ostream &out, const Lattice &frame)
{
  write_png(out, frame, Binary_RGB_Mapper<typename Lattice::state_type>{});
}

/** @brief Path-pattern sink that writes one PNG file per accepted frame.
 *
 *  The path pattern may contain `{step}` or `{}`; the placeholder is
 *  replaced with a zero-padded step number. Parent directories are
 *  created lazily before each write.
 *
 *  @tparam Mapper invocable `state_type -> RGB8`.
 */
template <typename Mapper>
class Png_Frame_Sink
{
  std::filesystem::path pattern_;
  Mapper mapper_;
  std::size_t zero_pad_ = 6;

public:
  /** @brief Build a PNG frame sink.
   *  @param[in] path_pattern output path pattern.
   *  @param[in] mapper state-to-RGB mapper.
   *  @param[in] zero_pad minimum width for `{step}`.
   */
  Png_Frame_Sink(std::filesystem::path path_pattern, Mapper mapper, const std::size_t zero_pad = 6)
    : pattern_(std::move(path_pattern)), mapper_(std::move(mapper)), zero_pad_(zero_pad)
  {}

  /** @brief Write one PNG frame.
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
    std::ofstream out(path, std::ios::binary);
    ah_runtime_error_if(not out) << "Png_Frame_Sink::accept: cannot open '" << path.string() << "'";
    write_png(out, frame, mapper_);
  }

  /** @brief PNG frames are written eagerly; this is a no-op. */
  void flush() {}
};

template <typename Mapper>
Png_Frame_Sink(std::filesystem::path, Mapper, std::size_t = 6) -> Png_Frame_Sink<Mapper>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_PNG_H
