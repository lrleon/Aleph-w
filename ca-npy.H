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

/** @file ca-npy.H
 *  @brief NumPy `.npy` writer for CA frames.
 *
 *  The backend writes NPY version 1.0 arrays in C order. Numeric cell
 *  states are emitted as their natural fixed-width dtype, making the
 *  output directly loadable with `numpy.load()`.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_NPY_H
#define CA_NPY_H

#include <bit>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_npy_detail {

template <typename T>
[[nodiscard]] inline std::string dtype_descr()
{
  using U = std::remove_cv_t<T>;
  static_assert(std::is_arithmetic_v<U>, "write_npy requires an arithmetic cell state");

  if constexpr (std::is_same_v<U, bool>)
    return "|b1";
  else if constexpr (std::is_floating_point_v<U>)
    return (std::endian::native == std::endian::little ? "<f" : ">f") + std::to_string(sizeof(U));
  else if constexpr (std::is_signed_v<U>)
    return (sizeof(U) == 1 ? "|i" : (std::endian::native == std::endian::little ? "<i" : ">i"))
         + std::to_string(sizeof(U));
  else
    return (sizeof(U) == 1 ? "|u" : (std::endian::native == std::endian::little ? "<u" : ">u"))
         + std::to_string(sizeof(U));
}

template <typename T>
inline void write_scalar(std::ostream &out, const T &value)
{
  using U = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<U, bool>)
    {
      const std::uint8_t byte = value ? 1u : 0u;
      out.write(reinterpret_cast<const char *>(&byte), 1);
    }
  else
    {
      out.write(reinterpret_cast<const char *>(&value), static_cast<std::streamsize>(sizeof(U)));
    }
}

template <std::size_t Dim, typename Lattice>
inline void write_payload_rec(std::ostream &out,
                              const Lattice &frame,
                              typename Lattice::coord_type &coord)
{
  if constexpr (Dim == Lattice::rank)
    {
      write_scalar(out, frame.at(coord));
    }
  else
    {
      for (ca_size_t i = 0; i < frame.size(Dim); ++i)
        {
          coord[Dim] = static_cast<ca_index_t>(i);
          write_payload_rec<Dim + 1>(out, frame, coord);
        }
    }
}

template <typename Lattice>
[[nodiscard]] inline std::string shape_tuple(const Lattice &frame)
{
  std::ostringstream out;
  out << '(';
  for (std::size_t d = 0; d < Lattice::rank; ++d)
    {
      if (d != 0)
        out << ", ";
      out << frame.size(d);
    }
  if constexpr (Lattice::rank == 1)
    out << ',';
  out << ')';
  return out.str();
}

}  // namespace ca_npy_detail

/** @brief Write a lattice frame as a NumPy `.npy` array.
 *
 *  The lattice rank becomes the NumPy array rank and the payload is
 *  emitted in row-major order.
 *
 *  @tparam Lattice lattice type with an arithmetic state type.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to write.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(frame.size()).
 */
template <typename Lattice>
inline void write_npy(std::ostream &out, const Lattice &frame)
{
  static_assert(LatticeLike<Lattice>, "write_npy requires a LatticeLike frame");
  using state_t = typename Lattice::state_type;
  static_assert(std::is_arithmetic_v<state_t>, "write_npy requires arithmetic cell states");

  std::string header = "{'descr': '" + ca_npy_detail::dtype_descr<state_t>()
                     + "', 'fortran_order': False, 'shape': " + ca_npy_detail::shape_tuple(frame)
                     + ", }";
  constexpr std::size_t prelude = 10;
  const std::size_t rem = (prelude + header.size() + 1) % 16;
  const std::size_t padding = rem == 0 ? 0 : 16 - rem;
  header.append(padding, ' ');
  header.push_back('\n');

  ah_domain_error_if(header.size() > 65535u) << "write_npy: NPY v1.0 header too large";

  static constexpr unsigned char magic[] = {0x93, 'N', 'U', 'M', 'P', 'Y'};
  out.write(reinterpret_cast<const char *>(magic), 6);
  out.put(static_cast<char>(1));
  out.put(static_cast<char>(0));
  const std::uint16_t len = static_cast<std::uint16_t>(header.size());
  out.put(static_cast<char>(len & 0xffu));
  out.put(static_cast<char>((len >> 8) & 0xffu));
  out.write(header.data(), static_cast<std::streamsize>(header.size()));

  typename Lattice::coord_type coord{};
  ca_npy_detail::write_payload_rec<0>(out, frame, coord);
  ah_runtime_error_if(not out) << "write_npy: output stream failed";
}

/** @brief Path-pattern sink that writes one `.npy` file per frame. */
class Npy_Frame_Sink
{
  std::filesystem::path pattern_;
  std::size_t zero_pad_ = 6;

public:
  /** @brief Build an NPY frame sink.
   *  @param[in] path_pattern output path pattern.
   *  @param[in] zero_pad minimum width for `{step}`.
   */
  explicit Npy_Frame_Sink(std::filesystem::path path_pattern, const std::size_t zero_pad = 6)
    : pattern_(std::move(path_pattern)), zero_pad_(zero_pad)
  {}

  /** @brief Write one NPY frame.
   *  @tparam Lattice lattice type.
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
    ah_runtime_error_if(not out) << "Npy_Frame_Sink::accept: cannot open '" << path.string() << "'";
    write_npy(out, frame);
  }

  /** @brief NPY frames are written eagerly; this is a no-op. */
  void flush() {}
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_NPY_H
