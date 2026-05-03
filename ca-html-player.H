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

/** @file ca-html-player.H
 *  @brief Self-contained HTML trajectory player for CA frames.
 *
 *  Frames are embedded as JSON RGB pixel arrays and rendered with a
 *  tiny canvas script. The generated file has no external resources, so
 *  it can be shared as a single artifact and opened directly in a
 *  browser.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_HTML_PLAYER_H
#define CA_HTML_PLAYER_H

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <ca-io.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

/** @brief Options for generated HTML players. */
struct HTML_Player_Options
{
  std::string title = "Aleph::CA trajectory"; ///< document title
  unsigned fps = 12;                           ///< playback frames per second
  unsigned cell_size = 8;                      ///< canvas pixel size per CA cell
  bool show_grid = false;                      ///< draw faint cell grid
};

namespace ca_html_detail {

[[nodiscard]] inline std::string html_escape(const std::string &s)
{
  std::string out;
  for (const char c : s)
    switch (c)
      {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out.push_back(c); break;
      }
  return out;
}

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

struct Frame
{
  std::size_t step = 0;
  ca_size_t width = 0;
  ca_size_t height = 0;
  std::vector<RGB8> pixels;
};

inline void write_frame_json(std::ostream &out, const Frame &f)
{
  out << "{\"step\":" << f.step << ",\"width\":" << f.width
      << ",\"height\":" << f.height << ",\"pixels\":[";
  for (std::size_t i = 0; i < f.pixels.size(); ++i)
    {
      if (i != 0)
        out << ',';
      out << '"' << rgb_hex(f.pixels[i]) << '"';
    }
  out << "]}";
}

}  // namespace ca_html_detail

/** @brief Sink that writes a self-contained HTML playback file.
 *
 *  @tparam Mapper invocable `state_type -> RGB8`.
 */
template <typename Mapper>
class HTML_Player_Frame_Sink
{
  std::filesystem::path path_;
  Mapper mapper_;
  HTML_Player_Options opts_{};
  std::vector<ca_html_detail::Frame> frames_;

public:
  /** @brief Build an HTML player sink.
   *  @param[in] path output `.html` file.
   *  @param[in] mapper state-to-RGB mapper.
   *  @param[in] opts player options.
   */
  HTML_Player_Frame_Sink(std::filesystem::path path,
                         Mapper mapper,
                         HTML_Player_Options opts = {})
    : path_(std::move(path)), mapper_(std::move(mapper)), opts_(std::move(opts))
  {}

  /** @brief Append one frame to the embedded trajectory.
   *  @tparam Lattice rank-2 lattice type.
   *  @param[in] step simulation step.
   *  @param[in] frame frame to collect.
   */
  template <typename Lattice>
  void accept(const std::size_t step, const Lattice &frame)
  {
    static_assert(LatticeLike<Lattice>, "HTML_Player_Frame_Sink requires LatticeLike frames");
    static_assert(Lattice::rank == 2, "HTML_Player_Frame_Sink requires rank-2 frames");
    using coord_t = typename Lattice::coord_type;

    ca_html_detail::Frame f;
    f.step = step;
    f.width = frame.size(1);
    f.height = frame.size(0);
    f.pixels.reserve(f.width * f.height);
    for (ca_size_t r = 0; r < frame.size(0); ++r)
      for (ca_size_t c = 0; c < frame.size(1); ++c)
        f.pixels.push_back(mapper_(frame.at(coord_t{static_cast<ca_index_t>(r),
                                                    static_cast<ca_index_t>(c)})));
    frames_.push_back(std::move(f));
  }

  /** @brief Write the final self-contained HTML file.
   *  @throws std::runtime_error on file or stream failure.
   */
  void flush()
  {
    if (const auto parent = path_.parent_path(); not parent.empty())
      std::filesystem::create_directories(parent);
    std::ofstream out(path_);
    ah_runtime_error_if(not out)
      << "HTML_Player_Frame_Sink::flush: cannot open '" << path_.string() << "'";

    const unsigned interval = opts_.fps == 0 ? 1000u : 1000u / opts_.fps;
    out << "<!doctype html>\n<html lang=\"en\">\n<head>\n";
    out << "<meta charset=\"utf-8\">\n<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n";
    out << "<title>" << ca_html_detail::html_escape(opts_.title) << "</title>\n";
    out << "<style>body{margin:0;font:14px system-ui;background:#111;color:#eee;display:grid;"
           "place-items:center;min-height:100vh}main{width:min(96vw,980px)}canvas{image-rendering:pixelated;"
           "background:#fff;max-width:100%;height:auto;border:1px solid #444}button{margin:.35rem .2rem;"
           "padding:.35rem .6rem;background:#222;color:#eee;border:1px solid #555}output{margin-left:.7rem}</style>\n";
    out << "</head>\n<body><main>\n<h1>"
        << ca_html_detail::html_escape(opts_.title) << "</h1>\n";
    out << "<canvas id=\"view\"></canvas><div><button id=\"play\">Play</button>"
           "<button id=\"prev\">Prev</button><button id=\"next\">Next</button><output id=\"status\"></output></div>\n";
    out << "<script>\nconst frames = [";
    for (std::size_t i = 0; i < frames_.size(); ++i)
      {
        if (i != 0)
          out << ',';
        ca_html_detail::write_frame_json(out, frames_[i]);
      }
    out << "];\n";
    out << "const cellSize=" << opts_.cell_size << ", showGrid="
        << (opts_.show_grid ? "true" : "false") << ", delay=" << interval << ";\n";
    out << R"JS(
const canvas=document.getElementById('view'),ctx=canvas.getContext('2d');
const status=document.getElementById('status'),play=document.getElementById('play');
let idx=0,timer=null;
function draw(){
  if(frames.length===0){status.value='no frames';return;}
  const f=frames[idx]; canvas.width=f.width*cellSize; canvas.height=f.height*cellSize;
  for(let r=0;r<f.height;r++) for(let c=0;c<f.width;c++){
    ctx.fillStyle=f.pixels[r*f.width+c];
    ctx.fillRect(c*cellSize,r*cellSize,cellSize,cellSize);
  }
  if(showGrid){ctx.strokeStyle='rgba(0,0,0,.18)';ctx.lineWidth=1;
    for(let x=0;x<=f.width;x++){ctx.beginPath();ctx.moveTo(x*cellSize,0);ctx.lineTo(x*cellSize,canvas.height);ctx.stroke();}
    for(let y=0;y<=f.height;y++){ctx.beginPath();ctx.moveTo(0,y*cellSize);ctx.lineTo(canvas.width,y*cellSize);ctx.stroke();}
  }
  status.value=`frame ${idx+1}/${frames.length} · step ${f.step}`;
}
function stop(){if(timer!==null){clearInterval(timer);timer=null;play.textContent='Play';}}
play.onclick=()=>{if(timer===null){timer=setInterval(()=>{idx=(idx+1)%frames.length;draw();},delay);play.textContent='Pause';}else stop();};
document.getElementById('prev').onclick=()=>{stop();idx=(idx+frames.length-1)%frames.length;draw();};
document.getElementById('next').onclick=()=>{stop();idx=(idx+1)%frames.length;draw();};
draw();
)JS";
    out << "</script>\n</main></body>\n</html>\n";
    ah_runtime_error_if(not out) << "HTML_Player_Frame_Sink::flush: output stream failed";
  }

  /** @brief Return collected frame count.
   *  @return number of frames currently buffered.
   */
  [[nodiscard]] std::size_t size() const noexcept
  {
    return frames_.size();
  }
};

template <typename Mapper>
HTML_Player_Frame_Sink(std::filesystem::path, Mapper, HTML_Player_Options = {})
  -> HTML_Player_Frame_Sink<Mapper>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_HTML_PLAYER_H
