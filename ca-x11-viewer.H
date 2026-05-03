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

/** @file ca-x11-viewer.H
 *  @brief Optional live X11 viewer sink for CA simulations.
 *
 *  The viewer opens a simple X11 window on first accepted frame. When
 *  no display is available and `require_display` is false, it becomes
 *  a no-op so examples and tests remain usable in headless CI.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_X11_VIEWER_H
#define CA_X11_VIEWER_H

#include <chrono>
#include <concepts>
#include <cstdlib>
#include <string>
#include <thread>
#include <utility>

#if defined(__has_include)
#  if __has_include(<X11/Xlib.h>)
#    include <X11/Xlib.h>
#    define ALEPH_CA_HAVE_X11 1
#    ifdef None
#      undef None
#    endif
#  endif
#endif

#include <ah-errors.H>

#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Options for the live X11 viewer. */
struct X11_Viewer_Options
{
  unsigned cell_size = 8;             ///< pixels per CA cell
  unsigned delay_ms = 25;             ///< sleep after each drawn frame
  std::string title = "Aleph::CA";    ///< window title
  bool enabled = true;                ///< disable to force no-op behaviour
  bool require_display = false;       ///< throw instead of no-op without X11
};

/** @brief Live X11 frame sink.
 *
 *  @tparam Mapper invocable `state_type -> RGB8`.
 */
template <typename Mapper>
class X11_Frame_Sink
{
  Mapper mapper_;
  X11_Viewer_Options opts_{};
  bool disabled_ = false;

#if defined(ALEPH_CA_HAVE_X11)
  Display *display_ = nullptr;
  Window window_{};
  GC gc_{};
  bool ready_ = false;
  ca_size_t rows_ = 0;
  ca_size_t cols_ = 0;

  void close_display() noexcept
  {
    if (display_ != nullptr)
      {
        if (ready_)
          {
            XFreeGC(display_, gc_);
            XDestroyWindow(display_, window_);
          }
        XCloseDisplay(display_);
      }
    display_ = nullptr;
    ready_ = false;
  }

  void ensure_window(const ca_size_t rows, const ca_size_t cols)
  {
    if (disabled_ or ready_)
      return;
    if (not opts_.enabled)
      {
        disabled_ = true;
        return;
      }
    if (std::getenv("DISPLAY") == nullptr)
      {
        ah_runtime_error_if(opts_.require_display)
          << "X11_Frame_Sink: DISPLAY is not set";
        disabled_ = true;
        return;
      }
    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr)
      {
        ah_runtime_error_if(opts_.require_display)
          << "X11_Frame_Sink: cannot open X display";
        disabled_ = true;
        return;
      }
    rows_ = rows;
    cols_ = cols;
    const int screen = DefaultScreen(display_);
    const unsigned width = static_cast<unsigned>(cols * opts_.cell_size);
    const unsigned height = static_cast<unsigned>(rows * opts_.cell_size);
    window_ = XCreateSimpleWindow(display_,
                                  RootWindow(display_, screen),
                                  0,
                                  0,
                                  width,
                                  height,
                                  1,
                                  BlackPixel(display_, screen),
                                  WhitePixel(display_, screen));
    XStoreName(display_, window_, opts_.title.c_str());
    XMapWindow(display_, window_);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XFlush(display_);
    ready_ = true;
  }

  [[nodiscard]] unsigned long pixel(const RGB8 c) const noexcept
  {
    return (static_cast<unsigned long>(c.r) << 16)
           | (static_cast<unsigned long>(c.g) << 8)
           | static_cast<unsigned long>(c.b);
  }
#else
  void ensure_window(ca_size_t, ca_size_t)
  {
    ah_runtime_error_if(opts_.require_display)
      << "X11_Frame_Sink: X11 headers are not available";
    disabled_ = true;
  }
#endif

public:
  /** @brief Build an X11 viewer sink.
   *  @param[in] mapper state-to-RGB mapper.
   *  @param[in] opts viewer options.
   */
  explicit X11_Frame_Sink(Mapper mapper, X11_Viewer_Options opts = {})
    : mapper_(std::move(mapper)), opts_(std::move(opts))
  {}

  X11_Frame_Sink(const X11_Frame_Sink &) = delete;
  X11_Frame_Sink &operator=(const X11_Frame_Sink &) = delete;

  /** @brief Move-construct a viewer, transferring the window handle.
   *  @param[in,out] other sink to move from.
   */
  X11_Frame_Sink(X11_Frame_Sink &&other) noexcept
    : mapper_(std::move(other.mapper_))
    , opts_(std::move(other.opts_))
    , disabled_(other.disabled_)
#if defined(ALEPH_CA_HAVE_X11)
    , display_(other.display_)
    , window_(other.window_)
    , gc_(other.gc_)
    , ready_(other.ready_)
    , rows_(other.rows_)
    , cols_(other.cols_)
#endif
  {
#if defined(ALEPH_CA_HAVE_X11)
    other.display_ = nullptr;
    other.ready_ = false;
#endif
    other.disabled_ = true;
  }

  /** @brief Close the X11 window best-effort. */
  ~X11_Frame_Sink()
  {
#if defined(ALEPH_CA_HAVE_X11)
    close_display();
#endif
  }

  /** @brief Return whether the viewer is actively drawing.
   *  @return true when a display window exists.
   */
  [[nodiscard]] bool active() const noexcept
  {
#if defined(ALEPH_CA_HAVE_X11)
    return ready_;
#else
    return false;
#endif
  }

  /** @brief Draw one frame, or no-op when disabled/headless.
   *  @tparam Lattice rank-2 lattice type.
   *  @param[in] step simulation step, accepted for interface symmetry.
   *  @param[in] frame frame to draw.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    (void) step;
    static_assert(LatticeLike<Lattice>, "X11_Frame_Sink requires LatticeLike frames");
    static_assert(Lattice::rank == 2, "X11_Frame_Sink requires rank-2 frames");
    using coord_t = typename Lattice::coord_type;

    ensure_window(frame.size(0), frame.size(1));
#if defined(ALEPH_CA_HAVE_X11)
    if (not ready_)
      return;
    if (frame.size(0) != rows_ or frame.size(1) != cols_)
      {
        close_display();
        ensure_window(frame.size(0), frame.size(1));
      }
    for (ca_size_t r = 0; r < frame.size(0); ++r)
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        {
          const RGB8 colour = mapper_(frame.at(coord_t{static_cast<ca_index_t>(r),
                                                       static_cast<ca_index_t>(c)}));
          XSetForeground(display_, gc_, pixel(colour));
          XFillRectangle(display_,
                         window_,
                         gc_,
                         static_cast<int>(c * opts_.cell_size),
                         static_cast<int>(r * opts_.cell_size),
                         opts_.cell_size,
                         opts_.cell_size);
        }
    XFlush(display_);
    if (opts_.delay_ms != 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(opts_.delay_ms));
#endif
  }

  /** @brief Flush pending X11 requests. */
  void flush()
  {
#if defined(ALEPH_CA_HAVE_X11)
    if (ready_)
      XFlush(display_);
#endif
  }
};

template <typename Mapper>
X11_Frame_Sink(Mapper, X11_Viewer_Options = {}) -> X11_Frame_Sink<Mapper>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_X11_VIEWER_H
