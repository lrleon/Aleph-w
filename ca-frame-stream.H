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

/** @file ca-frame-stream.H
 *  @brief Directory-backed frame sinks for CA trajectories.
 *
 *  `Directory_Frame_Sink` writes selected simulation frames into a directory
 *  using the Phase-11 exporters from `ca-io.H` and `ca-tikz.H`. It is small
 *  enough to be used directly in an engine post-step hook or in an explicit
 *  simulation loop.
 *
 *  No compression or video dependency is pulled into Aleph-w. The sink writes
 *  a deterministic manifest and exposes `montage_command_hint()` so callers
 *  can hand the frame sequence to external tools such as ffmpeg or NetPBM.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_FRAME_STREAM_H
#define CA_FRAME_STREAM_H

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-io.H>
#include <ca-tikz.H>

namespace Aleph {
namespace CA {

/** @brief Concept implemented by sinks that consume CA frames.
 *
 *  A frame sink accepts immutable frames tagged with their simulation
 *  step and exposes `flush()` to finish buffered output. The concept is
 *  intentionally minimal so file sinks, live viewers and in-memory
 *  collectors can share the same call surface.
 *
 *  @tparam Sink candidate sink type.
 *  @tparam Lattice frame type accepted by the sink.
 */
template <typename Sink, typename Lattice>
concept Frame_Sink = requires(Sink &sink, std::size_t step, const Lattice &frame)
{
  sink.accept(step, frame);
  sink.flush();
};

/** @brief File format used by `Directory_Frame_Sink`.
 *
 *  @par Complexity
 *  O(1) storage.
 */
enum class Frame_Output_Format
{
  PGM,   ///< binary NetPBM grayscale (`.pgm`)
  PPM,   ///< binary NetPBM RGB (`.ppm`)
  RLE,   ///< Conway RLE (`.rle`)
  CSV,   ///< numeric CSV (`.csv`)
  JSON,  ///< JSON snapshot (`.json`)
  TikZ   ///< TikZ picture (`.tex`)
};

/** @brief Options for directory-backed frame streams.
 *
 *  `stride` controls sampling: `stride == 1` writes every accepted frame,
 *  `stride == 10` writes steps 0, 10, 20, ...
 */
struct Frame_Stream_Options
{
  std::filesystem::path directory = "ca_frames";          ///< destination directory
  std::string prefix = "frame";                           ///< filename prefix
  Frame_Output_Format format = Frame_Output_Format::PPM;  ///< exporter format
  std::size_t stride = 1;                                 ///< write every Nth step
  std::size_t zero_pad = 6;                               ///< step field width in filenames
  bool write_manifest = true;                             ///< write `manifest.csv`
  RLE_Write_Options rle_options{};                        ///< RLE options when format is RLE
  CSV_Options csv_options{};                              ///< CSV options when format is CSV
  JSON_Write_Options json_options{};                      ///< JSON options when format is JSON
  Rect_Tikz_Options tikz_options{};                       ///< TikZ options when format is TikZ
  NetPBM_Write_Options netpbm_options{};                  ///< PGM/PPM options
};

namespace ca_frame_stream_detail {

inline const char *extension(Frame_Output_Format format) noexcept
{
  switch (format)
    {
    case Frame_Output_Format::PGM:
      return ".pgm";
    case Frame_Output_Format::PPM:
      return ".ppm";
    case Frame_Output_Format::RLE:
      return ".rle";
    case Frame_Output_Format::CSV:
      return ".csv";
    case Frame_Output_Format::JSON:
      return ".json";
    case Frame_Output_Format::TikZ:
      return ".tex";
    }
  return ".frame";
}

inline const char *format_name(Frame_Output_Format format) noexcept
{
  switch (format)
    {
    case Frame_Output_Format::PGM:
      return "pgm";
    case Frame_Output_Format::PPM:
      return "ppm";
    case Frame_Output_Format::RLE:
      return "rle";
    case Frame_Output_Format::CSV:
      return "csv";
    case Frame_Output_Format::JSON:
      return "json";
    case Frame_Output_Format::TikZ:
      return "tikz";
    }
  return "unknown";
}

inline std::string padded_step(std::size_t step, std::size_t width)
{
  std::ostringstream out;
  out << std::setw(static_cast<int>(width)) << std::setfill('0') << step;
  return out.str();
}

inline std::filesystem::path format_step_path(const std::filesystem::path &pattern,
                                              const std::size_t step,
                                              const std::size_t zero_pad)
{
  std::string text = pattern.string();
  const std::string value = padded_step(step, zero_pad);
  auto replace_all = [&](const std::string &needle)
  {
    std::size_t pos = 0;
    bool changed = false;
    while ((pos = text.find(needle, pos)) != std::string::npos)
      {
        text.replace(pos, needle.size(), value);
        pos += value.size();
        changed = true;
      }
    return changed;
  };

  if (replace_all("{step}"))
    return std::filesystem::path(text);
  if (replace_all("{}"))
    return std::filesystem::path(text);
  return pattern;
}

}  // namespace ca_frame_stream_detail

/** @brief Format a step-indexed file path.
 *
 *  Replaces either `{step}` or `{}` in `pattern` with a zero-padded
 *  decimal representation of `step`. If no placeholder is present the
 *  pattern is returned unchanged, which is useful for single-file sinks.
 *
 *  @param[in] pattern path pattern.
 *  @param[in] step simulation step.
 *  @param[in] zero_pad minimum width of the step field.
 *  @return formatted path.
 *
 *  @par Complexity
 *  O(pattern length).
 */
[[nodiscard]] inline std::filesystem::path format_step_path(const std::filesystem::path &pattern,
                                                            const std::size_t step,
                                                            const std::size_t zero_pad = 6)
{
  return ca_frame_stream_detail::format_step_path(pattern, step, zero_pad);
}

/** @brief Fan-out sink that forwards every frame to several sinks.
 *
 *  `Composite_Frame_Sink` owns its child sinks and forwards `accept()`
 *  and `flush()` in tuple order. It is useful when a simulation should
 *  emit, for example, PNG frames, an HTML player and a terminal preview
 *  from one traversal.
 *
 *  @tparam Sinks concrete sink types.
 *
 *  @par Thread-safety
 *  Not thread-safe unless every child sink is externally synchronised.
 */
template <typename... Sinks>
class Composite_Frame_Sink
{
  std::tuple<Sinks...> sinks_;

public:
  /** @brief Construct the composite sink.
   *  @param[in] sinks child sinks moved into the composite.
   */
  explicit Composite_Frame_Sink(Sinks... sinks) : sinks_(std::move(sinks)...) {}

  /** @brief Return child sink `I`.
   *  @tparam I tuple index.
   *  @return mutable sink reference.
   */
  template <std::size_t I>
  [[nodiscard]] auto &get() noexcept
  {
    return std::get<I>(sinks_);
  }

  /** @brief Return child sink `I`.
   *  @tparam I tuple index.
   *  @return immutable sink reference.
   */
  template <std::size_t I>
  [[nodiscard]] const auto &get() const noexcept
  {
    return std::get<I>(sinks_);
  }

  /** @brief Forward a frame to every child sink.
   *  @tparam Lattice frame type.
   *  @param[in] step simulation step.
   *  @param[in] frame frame to forward.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    std::apply([&](auto &...sink)
    {
      (sink.accept(step, frame), ...);
    }, sinks_);
  }

  /** @brief Flush every child sink in tuple order. */
  void flush()
  {
    std::apply([](auto &...sink)
    {
      (sink.flush(), ...);
    }, sinks_);
  }
};

/** @brief Directory-backed sink for CA frames.
 *
 *  The sink is format-agnostic: `accept(step, frame)` picks the exporter
 *  configured in `Frame_Stream_Options`. A CSV manifest is kept in memory
 *  and written on `flush()`; it lists step, format and path for every frame.
 *
 *  @par Thread-safety
 *  Not thread-safe. Use one sink per simulation thread or synchronise calls
 *  externally.
 */
class Directory_Frame_Sink
{
  Frame_Stream_Options opts_;
  std::vector<std::pair<std::size_t, std::filesystem::path>> written_;

  [[nodiscard]] std::filesystem::path frame_path(std::size_t step) const
  {
    return opts_.directory
         / (opts_.prefix + "_" + ca_frame_stream_detail::padded_step(step, opts_.zero_pad)
            + ca_frame_stream_detail::extension(opts_.format));
  }

  template <typename Lattice>
  void write_one(std::ostream &out, const Lattice &frame) const
  {
    switch (opts_.format)
      {
      case Frame_Output_Format::PGM:
        write_pgm(out, frame, opts_.netpbm_options);
        break;
      case Frame_Output_Format::PPM:
        write_ppm(out, frame, opts_.netpbm_options);
        break;
      case Frame_Output_Format::RLE:
        write_rle(out, frame, opts_.rle_options);
        break;
      case Frame_Output_Format::CSV:
        write_csv(out, frame, opts_.csv_options);
        break;
      case Frame_Output_Format::JSON:
        write_json(out, frame, opts_.json_options);
        break;
      case Frame_Output_Format::TikZ:
        render_lattice_tikz(out, frame, opts_.tikz_options);
        break;
      }
  }

public:
  /** @brief Build a frame sink.
   *
   *  The destination directory is created immediately.
   *
   *  @param[in] opts sink options.
   *  @throws std::domain_error if `stride == 0`.
   *  @throws std::runtime_error if the directory cannot be created.
   *
   *  @par Complexity
   *  O(path length).
   */
  explicit Directory_Frame_Sink(Frame_Stream_Options opts) : opts_(std::move(opts))
  {
    ah_domain_error_if(opts_.stride == 0) << "Directory_Frame_Sink: stride must be positive";
    std::error_code ec;
    std::filesystem::create_directories(opts_.directory, ec);
    ah_runtime_error_if(ec) << "Directory_Frame_Sink: cannot create directory '"
                            << opts_.directory.string() << "': " << ec.message();
  }

  /** @brief Return the immutable sink options.
   *  @return sink options.
   */
  [[nodiscard]] const Frame_Stream_Options &options() const noexcept
  {
    return opts_;
  }

  /** @brief Return the frame files written so far.
   *  @return vector of `(step, path)` entries.
   */
  [[nodiscard]] const std::vector<std::pair<std::size_t, std::filesystem::path>> &written_frames() const noexcept
  {
    return written_;
  }

  /** @brief Accept a frame and write it if it passes the stride filter.
   *
   *  @tparam Lattice 2D lattice type.
   *  @param[in] step simulation step.
   *  @param[in] frame frame to export.
   *  @throws std::runtime_error on file or exporter failure.
   *
   *  @par Complexity
   *  O(rows * cols) when written, O(1) when skipped by stride.
   */
  template <typename Lattice>
  void accept(std::size_t step, const Lattice &frame)
  {
    if (step % opts_.stride != 0)
      return;
    const std::filesystem::path path = frame_path(step);
    std::ofstream out(path, std::ios::binary);
    ah_runtime_error_if(not out) << "Directory_Frame_Sink::accept: cannot open '" << path.string()
                                 << "'";
    write_one(out, frame);
    ah_runtime_error_if(not out) << "Directory_Frame_Sink::accept: write failed for '"
                                 << path.string() << "'";
    written_.push_back({step, path});
  }

  /** @brief Write the optional manifest.
   *
   *  The manifest is a CSV file named `manifest.csv` with columns
   *  `step,format,path`.
   *
   *  @throws std::runtime_error on write failure.
   *
   *  @par Complexity
   *  O(number of written frames).
   */
  void flush() const
  {
    if (not opts_.write_manifest)
      return;
    const std::filesystem::path path = opts_.directory / "manifest.csv";
    std::ofstream out(path);
    ah_runtime_error_if(not out) << "Directory_Frame_Sink::flush: cannot open '" << path.string()
                                 << "'";
    out << "step,format,path\n";
    for (const auto &[step, frame] : written_)
      out << step << ',' << ca_frame_stream_detail::format_name(opts_.format) << ','
          << frame.string() << '\n';
    ah_runtime_error_if(not out) << "Directory_Frame_Sink::flush: write failed for '"
                                 << path.string() << "'";
  }

  /** @brief Return an ffmpeg command hint for the current frame sequence.
   *
   *  The command is only a string; Aleph-w does not execute external
   *  encoders. It is meaningful for image formats (PGM/PPM).
   *
   *  @param[in] output_file video filename to place in the command.
   *  @param[in] fps frames per second.
   *  @return shell command hint.
   */
  [[nodiscard]] std::string montage_command_hint(const std::string &output_file = "ca.mp4",
                                                 unsigned fps = 24) const
  {
    std::ostringstream cmd;
    cmd << "ffmpeg -y -framerate " << fps << " -i "
        << (opts_.directory
            / (opts_.prefix + "_%0" + std::to_string(opts_.zero_pad) + "d"
               + ca_frame_stream_detail::extension(opts_.format)))
             .string()
        << " -pix_fmt yuv420p " << output_file;
    return cmd.str();
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // CA_FRAME_STREAM_H
