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

/** @file ca-ffmpeg-sink.H
 *  @brief Runtime ffmpeg sink for publication-quality CA video.
 *
 *  The sink writes PPM frames to an `ffmpeg` process through stdin.
 *  ffmpeg is an optional runtime dependency: when it is not available
 *  and `require_ffmpeg` is false, the sink becomes a no-op.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_FFMPEG_SINK_H
#define CA_FFMPEG_SINK_H

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

#include <ah-errors.H>

#include <ca-io.H>

namespace Aleph {
namespace CA {

/** @brief Options for `Ffmpeg_Frame_Sink`. */
struct Ffmpeg_Options
{
  unsigned fps = 24;                     ///< input and output frame rate
  std::string codec = "libx264";         ///< ffmpeg encoder name
  std::string pixel_format = "yuv420p";  ///< output pixel format
  bool auto_start = true;                ///< start ffmpeg in the constructor
  bool require_ffmpeg = false;           ///< throw if ffmpeg cannot be started
};

namespace ca_ffmpeg_detail {

[[nodiscard]] inline std::string shell_quote(const std::string &s)
{
  std::string out = "'";
  for (const char c : s)
    {
      if (c == '\'')
        out += "'\\''";
      else
        out.push_back(c);
    }
  out.push_back('\'');
  return out;
}

[[nodiscard]] inline bool ffmpeg_available()
{
  return std::system("ffmpeg -version >/dev/null 2>&1") == 0;
}

}  // namespace ca_ffmpeg_detail

/** @brief Frame sink that streams PPM frames into ffmpeg.
 *
 *  @tparam Mapper invocable `state_type -> RGB8`.
 */
template <typename Mapper>
class Ffmpeg_Frame_Sink
{
  std::filesystem::path output_;
  Mapper mapper_;
  Ffmpeg_Options opts_{};
  std::string command_;
  FILE *pipe_ = nullptr;

  void start()
  {
    command_ = "ffmpeg -y -loglevel error -f image2pipe -vcodec ppm -r " + std::to_string(opts_.fps)
             + " -i - -an -vcodec " + opts_.codec + " -pix_fmt " + opts_.pixel_format + ' '
             + ca_ffmpeg_detail::shell_quote(output_.string());
    if (not opts_.auto_start)
      return;
    if (not ca_ffmpeg_detail::ffmpeg_available())
      {
        ah_runtime_error_if(opts_.require_ffmpeg)
          << "Ffmpeg_Frame_Sink: ffmpeg is not available in PATH";
        return;
      }
    pipe_ = popen(command_.c_str(), "w");
    ah_runtime_error_if(pipe_ == nullptr and opts_.require_ffmpeg)
      << "Ffmpeg_Frame_Sink: cannot start ffmpeg";
  }

public:
  /** @brief Build an ffmpeg frame sink.
   *  @param[in] output output video path.
   *  @param[in] mapper state-to-RGB mapper.
   *  @param[in] opts sink options.
   */
  Ffmpeg_Frame_Sink(std::filesystem::path output, Mapper mapper, Ffmpeg_Options opts = {})
    : output_(std::move(output)), mapper_(std::move(mapper)), opts_(std::move(opts))
  {
    start();
  }

  Ffmpeg_Frame_Sink(const Ffmpeg_Frame_Sink &) = delete;
  Ffmpeg_Frame_Sink &operator=(const Ffmpeg_Frame_Sink &) = delete;

  /** @brief Move-construct a sink, transferring the process handle.
   *  @param[in,out] other sink to move from.
   */
  Ffmpeg_Frame_Sink(Ffmpeg_Frame_Sink &&other) noexcept
    : output_(std::move(other.output_)), mapper_(std::move(other.mapper_)),
      opts_(std::move(other.opts_)), command_(std::move(other.command_)), pipe_(other.pipe_)
  {
    other.pipe_ = nullptr;
  }

  /** @brief Close ffmpeg best-effort. */
  ~Ffmpeg_Frame_Sink()
  {
    if (pipe_ != nullptr)
      pclose(pipe_);
  }

  /** @brief Return the ffmpeg command used by this sink.
   *  @return command string.
   */
  [[nodiscard]] const std::string &command() const noexcept
  {
    return command_;
  }

  /** @brief Return true when a live ffmpeg process is attached.
   *  @return process state.
   */
  [[nodiscard]] bool active() const noexcept
  {
    return pipe_ != nullptr;
  }

  /** @brief Send one frame to ffmpeg, or no-op when inactive.
   *  @tparam Lattice rank-2 lattice type.
   *  @param[in] step simulation step, accepted for interface symmetry.
   *  @param[in] frame frame to encode.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    (void) step;
    if (pipe_ == nullptr)
      return;
    std::ostringstream ppm(std::ios::binary);
    write_ppm(ppm, frame, mapper_);
    const std::string bytes = ppm.str();
    const std::size_t written = std::fwrite(bytes.data(), 1, bytes.size(), pipe_);
    ah_runtime_error_if(written != bytes.size())
      << "Ffmpeg_Frame_Sink::accept: ffmpeg pipe write failed";
  }

  /** @brief Flush and close the ffmpeg process if one was started. */
  void flush()
  {
    if (pipe_ == nullptr)
      return;
    std::fflush(pipe_);
    const int status = pclose(pipe_);
    pipe_ = nullptr;
    ah_runtime_error_if(status != 0 and opts_.require_ffmpeg)
      << "Ffmpeg_Frame_Sink::flush: ffmpeg exited with status " << status;
  }
};

template <typename Mapper>
Ffmpeg_Frame_Sink(std::filesystem::path, Mapper, Ffmpeg_Options = {}) -> Ffmpeg_Frame_Sink<Mapper>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_FFMPEG_SINK_H
