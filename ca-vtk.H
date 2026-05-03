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

/** @file ca-vtk.H
 *  @brief VTK legacy writer for scientific CA visualisation.
 *
 *  The writer emits ASCII `STRUCTURED_POINTS` datasets for 2D and 3D
 *  lattices. Files can be opened directly in ParaView and VisIt.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_VTK_H
#define CA_VTK_H

#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Options for VTK legacy output. */
struct VTK_Write_Options
{
  std::string title = "Aleph::CA frame";  ///< VTK header title
  std::string scalar_name = "state";      ///< scalar field name
  double spacing[3] = {1.0, 1.0, 1.0};    ///< grid spacing in x, y, z
  double origin[3] = {0.0, 0.0, 0.0};     ///< grid origin in x, y, z
};

namespace ca_vtk_detail {

template <typename T>
[[nodiscard]] inline const char *vtk_scalar_type()
{
  using U = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<U, bool>)
    return "unsigned_char";
  else if constexpr (std::is_floating_point_v<U> and sizeof(U) <= sizeof(float))
    return "float";
  else if constexpr (std::is_floating_point_v<U>)
    return "double";
  else if constexpr (std::is_signed_v<U> and sizeof(U) <= sizeof(int))
    return "int";
  else if constexpr (std::is_signed_v<U>)
    return "long";
  else if constexpr (sizeof(U) <= sizeof(unsigned int))
    return "unsigned_int";
  else
    return "unsigned_long";
}

template <typename T>
inline void write_scalar(std::ostream &out, const T &value)
{
  if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>)
    out << (value ? 1 : 0);
  else if constexpr (std::is_integral_v<T> and sizeof(T) == 1)
    out << static_cast<int>(value);
  else
    out << value;
}

}  // namespace ca_vtk_detail

/** @brief Write a 2D or 3D lattice as VTK legacy `STRUCTURED_POINTS`.
 *
 *  Axis mapping follows VTK convention: CA axis 0 maps to Y in 2D
 *  and Z in 3D, while the last CA axis maps to X.
 *
 *  @tparam Lattice rank-2 or rank-3 lattice type with arithmetic state.
 *  @param[in,out] out output stream.
 *  @param[in] frame frame to write.
 *  @param[in] opts VTK options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(frame.size()).
 */
template <typename Lattice>
inline void write_vtk_legacy(std::ostream &out,
                             const Lattice &frame,
                             const VTK_Write_Options &opts = {})
{
  static_assert(LatticeLike<Lattice>, "write_vtk_legacy requires a LatticeLike frame");
  static_assert(Lattice::rank == 2 or Lattice::rank == 3,
                "write_vtk_legacy supports rank-2 and rank-3 frames");
  using state_t = typename Lattice::state_type;
  static_assert(std::is_arithmetic_v<state_t>, "write_vtk_legacy requires arithmetic states");
  using coord_t = typename Lattice::coord_type;

  const ca_size_t nx = Lattice::rank == 2 ? frame.size(1) : frame.size(2);
  const ca_size_t ny = Lattice::rank == 2 ? frame.size(0) : frame.size(1);
  const ca_size_t nz = Lattice::rank == 2 ? 1 : frame.size(0);
  const ca_size_t points = nx * ny * nz;

  out << "# vtk DataFile Version 3.0\n";
  out << opts.title << '\n';
  out << "ASCII\n";
  out << "DATASET STRUCTURED_POINTS\n";
  out << "DIMENSIONS " << nx << ' ' << ny << ' ' << nz << '\n';
  out << "ORIGIN " << opts.origin[0] << ' ' << opts.origin[1] << ' ' << opts.origin[2] << '\n';
  out << "SPACING " << opts.spacing[0] << ' ' << opts.spacing[1] << ' ' << opts.spacing[2] << '\n';
  out << "POINT_DATA " << points << '\n';
  out << "SCALARS " << opts.scalar_name << ' ' << ca_vtk_detail::vtk_scalar_type<state_t>() << " 1\n";
  out << "LOOKUP_TABLE default\n";

  if constexpr (Lattice::rank == 2)
    {
      for (ca_size_t r = 0; r < frame.size(0); ++r)
        for (ca_size_t c = 0; c < frame.size(1); ++c)
          {
            ca_vtk_detail::write_scalar(
              out, frame.at(coord_t{static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}));
            out << '\n';
          }
    }
  else
    {
      for (ca_size_t z = 0; z < frame.size(0); ++z)
        for (ca_size_t y = 0; y < frame.size(1); ++y)
          for (ca_size_t x = 0; x < frame.size(2); ++x)
            {
              ca_vtk_detail::write_scalar(
                out, frame.at(coord_t{static_cast<ca_index_t>(z), static_cast<ca_index_t>(y),
                                      static_cast<ca_index_t>(x)}));
              out << '\n';
            }
    }

  ah_runtime_error_if(not out) << "write_vtk_legacy: output stream failed";
}

/** @brief Path-pattern sink that writes VTK legacy frames. */
class VTK_Frame_Sink
{
  std::filesystem::path pattern_;
  VTK_Write_Options opts_{};
  std::size_t zero_pad_ = 6;

public:
  /** @brief Build a VTK frame sink.
   *  @param[in] path_pattern output path pattern.
   *  @param[in] opts writer options.
   *  @param[in] zero_pad minimum width for `{step}`.
   */
  explicit VTK_Frame_Sink(std::filesystem::path path_pattern,
                          VTK_Write_Options opts = {},
                          const std::size_t zero_pad = 6)
    : pattern_(std::move(path_pattern)), opts_(std::move(opts)), zero_pad_(zero_pad)
  {}

  /** @brief Write one VTK frame.
   *  @tparam Lattice rank-2 or rank-3 lattice type.
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
    ah_runtime_error_if(not out) << "VTK_Frame_Sink::accept: cannot open '" << path.string() << "'";
    write_vtk_legacy(out, frame, opts_);
  }

  /** @brief VTK frames are written eagerly; this is a no-op. */
  void flush() {}
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_VTK_H
