/** @file adaptive_huffman_visual_example.cc
 *  @brief Generate an offline, interactive gallery of real FGK tree states.
 *
 *  Build: cmake --build build --target adaptive_huffman_visual_example
 *  Run:   ./build/Examples/adaptive_huffman_visual_example [output.html] [text]
 *
 *  Without custom text, three scenarios illustrate first appearances,
 *  repeated words, and a changing source distribution. The generated HTML
 *  includes SVG trees, a byte-by-byte timeline, current codes and actual bit
 *  costs. It needs no server, network resources, or extra dependencies.
 *  Every frame comes from a verified encoding/decoding step of the public API.
 */

#include <Adaptive_Huffman.H>
#include <ah-errors.H>
#include <tpl_array.H>
#include "adaptive_huffman_demo_support.H"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <locale>
#include <ostream>
#include <string>
#include <string_view>

namespace
{
  using adaptive_huffman_demo::byte_label;
  using adaptive_huffman_demo::Frame;
  using adaptive_huffman_demo::Snapshot;

  /** @brief Escape text used in both HTML and SVG text or attributes.
   *  @param text Untrusted text to escape.
   *  @return Text with markup-significant ASCII characters replaced.
   */
  std::string escape(const std::string_view text)
  {
    std::string result;
    for (const char c : text)
      switch (c)
        {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        case '"': result += "&quot;"; break;
        case '\'': result += "&#39;"; break;
        default: result += c;
        }
    return result;
  }

  /** @brief Draw an exact tree snapshot, highlighting the latest byte's new path.
   *  @param out Destination HTML stream.
   *  @param tree Tree after the update.
   *  @param current Latest byte value, or -2 for the initial state.
   */
  void draw_tree(std::ostream &out, const Snapshot &tree, const int current)
  {
    const double actual_width = 120 + 108 * (tree.leaf_count - 1);
    const double width = std::max(540.0, actual_width);
    const double height = 100 + 86 * tree.depth;
    std::string current_code;
    for (const auto &node : tree.nodes)
      if (node.symbol == current)
        current_code = node.code;
    out << "<svg class='tree' role='img' aria-label='Huffman tree after the update' "
        << "viewBox='0 0 " << width << ' ' << height << "' style='min-width:"
        << std::min(width, 660.0) << "px'><title>Current adaptive Huffman tree</title>"
        << "<desc>Left edges are zero, right edges are one. Node weights are frequencies. "
        << "NYT represents unseen bytes. The last byte's new path is highlighted.</desc>"
        << "<g transform='translate(" << (width - actual_width) / 2 << ",0)'>";
    for (const auto &parent : tree.nodes)
      for (int bit = 0; bit < 2; ++bit)
        if (parent.children[bit] != -1)
          {
            const auto &child = tree.nodes[parent.children[bit]];
            const bool active = not current_code.empty() and current_code.starts_with(child.code);
            const double mid_y = (parent.y + child.y) / 2;
            out << "<path class='edge" << (active ? " active" : "") << "' d='M"
                << parent.x << ' ' << parent.y + 18 << " C" << parent.x << ' ' << mid_y
                << ' ' << child.x << ' ' << mid_y << ' ' << child.x << ' ' << child.y - 22
                << "'/><text class='edge-label' x='" << (parent.x + child.x) / 2 + (bit ? 10 : -10)
                << "' y='" << mid_y - 4 << "'>" << bit << "</text>";
          }
    for (const auto &node : tree.nodes)
      {
        const bool leaf = node.symbol != -2;
        const auto label = node.symbol == -1 ? std::string("NYT") :
          leaf ? byte_label(static_cast<unsigned char>(node.symbol)) : std::to_string(node.weight);
        out << "<g class='node" << (leaf ? " leaf" : "") << (node.symbol == -1 ? " nyt" : "")
            << (leaf and node.symbol == current ? " current" : "")
            << "' transform='translate(" << node.x << ',' << node.y << ")'>"
            << "<title>" << escape(label) << ": frequency " << node.weight
            << ", path " << (node.code.empty() ? "root" : node.code) << "</title>";
        if (leaf)
          out << "<rect x='-35' y='-23' width='70' height='51' rx='9'/>"
              << "<text class='symbol' y='-2'>" << escape(label) << "</text>"
              << "<text class='weight' y='17'>" << node.weight << "</text>";
        else
          out << "<circle r='19'/><text class='sum' y='5'>" << label << "</text>";
        out << "</g>";
      }
    out << "</g></svg>";
  }

  /** @brief Write one navigable frame, with before-update bits and after-update codes.
   *  @param out Destination stream.
   *  @param frame Verified event and resulting tree.
   *  @param step Number of consumed bytes.
   *  @param initial_step Frame initially visible in this scenario.
   */
  void write_frame(std::ostream &out, const Frame &frame, const size_t step,
                    const size_t initial_step)
  {
    out << "<article class='frame' data-step='" << step << "'"
        << (step == initial_step ? "" : " hidden") << "><div class='metrics'>"
        << "<div><span>Bytes processed</span><strong>" << step << "</strong></div>"
        << "<div><span>Original bits</span><strong>" << 8 * step << "</strong></div>"
        << "<div><span>Encoded bits</span><strong>" << frame.total_bits << "</strong></div>"
        << "<div><span>Known bytes</span><strong>" << frame.tree.leaf_count - 1
        << "</strong></div></div><div class='workspace'><section class='tree-panel'>"
        << "<div class='panel-heading'><h2>Tree after the update</h2><span class='step-tag'>"
        << (step == 0 ? "Initial model" : "Byte " + std::to_string(step))
        << "</span></div><div class='tree-wrap'>";
    draw_tree(out, frame.tree, step == 0 ? -2 : frame.byte);
    out << "</div><p class='caption'>0 = left &nbsp; 1 = right &nbsp; Weight = frequency"
        << "<br>Highlight: the latest byte&#39;s path in the updated tree.</p></section>"
        << "<aside><section class='event'><span class='eyebrow'>";
    if (step == 0)
      out << "BEFORE THE FIRST BYTE</span><h2>A single NYT leaf.</h2>"
          << "<p>No frequencies are known yet. The first byte will travel as eight literal bits.</p>";
    else
      {
        out << (frame.first_appearance ? "FIRST APPEARANCE" : "KNOWN BYTE")
            << "</span><h2>Byte <code>" << escape(byte_label(frame.byte)) << "</code></h2>"
            << "<p>Bits sent <em>before</em> the tree update:</p>"
            << "<div class='bit-fields'><div><span>"
            << (frame.first_appearance ? "NYT path" : "Leaf path") << "</span><code>"
            << (frame.path.empty() ? "root" : frame.path) << "</code></div>";
        if (frame.first_appearance)
          out << "<div class='literal'><span>8 literal bits</span><code>" << frame.literal
              << "</code></div>";
        out << "</div><p class='event-total'>" << frame.path.size() + frame.literal.size()
            << " bits transmitted for this byte.</p>";
      }
    out << "</section><section class='codes'><h2>Codes now</h2>"
        << "<table><thead><tr><th>Byte</th><th>Count</th><th>Path</th></tr></thead><tbody>";
    for (const auto &node : frame.tree.nodes)
      if (node.symbol != -2)
        out << "<tr" << (step != 0 and node.symbol == frame.byte ? " class='selected'" : "")
            << "><td>" << escape(node.symbol == -1 ? "NYT" :
                                  byte_label(static_cast<unsigned char>(node.symbol)))
            << "</td><td>" << node.weight << "</td><td><code>"
            << (node.code.empty() ? "root" : node.code) << "</code></td></tr>";
    out << "</tbody></table></section></aside></div></article>";
  }

  /** @brief Plot the actual per-byte encoded length and the eight-bit baseline.
   *  @param out Destination HTML stream.
   *  @param frames Initial state and successive verified events.
   */
  void write_cost_chart(std::ostream &out, const Aleph::Array<Frame> &frames)
  {
    if (frames.size() == 1)
      return;
    const size_t count = frames.size() - 1;
    size_t maximum = 8;
    for (size_t i = 1; i < frames.size(); ++i)
      maximum = std::max(maximum, frames[i].path.size() + frames[i].literal.size());
    const double width = std::max(600.0, 60.0 + count * 32.0);
    const double pitch = (width - 80) / count;
    const double scale = 106.0 / maximum;
    out << "<section class='costs'><h2>Cost of each byte</h2>"
        << "<div class='chart-wrap'><svg class='cost-chart' role='img' "
        << "aria-label='Transmitted bits per byte compared with an eight-bit literal' viewBox='0 0 "
        << width << " 180'><title>Actual encoded bits per byte</title>"
        << "<text class='axis-label' x='8' y='15'>Bits / byte</text>"
        << "<line class='baseline' x1='44' x2='" << width - 10 << "' y1='"
        << 135 - 8 * scale << "' y2='" << 135 - 8 * scale << "'/>"
        << "<text class='axis-label' x='18' y='" << 139 - 8 * scale << "'>8</text>";
    for (size_t i = 1; i < frames.size(); ++i)
      {
        const auto &frame = frames[i];
        const size_t cost = frame.path.size() + frame.literal.size();
        const double x = 50 + (i - 1) * pitch;
        const double height = cost * scale;
        out << "<g class='cost-bar' data-index='" << i << "'><title>Byte " << i << ": "
            << escape(byte_label(frame.byte)) << ", " << cost << " bits</title>"
            << "<rect x='" << x << "' y='" << 135 - height << "' width='"
            << pitch * .58 << "' height='" << height << "' rx='2'/>"
            << "<text class='cost-value' x='" << x + pitch * .29 << "' y='"
            << 128 - height << "'>" << cost << "</text><text class='cost-byte' x='"
            << x + pitch * .29 << "' y='152'>" << escape(byte_label(frame.byte))
            << "</text></g>";
      }
    out << "<text class='axis-label' x='" << width / 2 << "' y='176'>Byte position in the message</text>"
        << "</svg></div><p class='caption'>Dashed line: 8-bit literal. Bright bars: bytes already processed. "
        << "Source: actual Aleph FGK encoding of this message.</p></section>";
  }

  /** @brief Write one independently navigable scenario.
   *  @param out Destination stream.
   *  @param input Message, limited by main() to 32 bytes.
   *  @param index Scenario number, zero-based.
   */
  void write_scenario(std::ostream &out, const std::string_view input, const size_t index)
  {
    const auto frames = adaptive_huffman_demo::trace(input);
    const size_t initial = std::min<size_t>(4, input.size());
    out << "<div class='scenario' data-case='" << index << "' data-length='" << input.size()
        << "' data-current='" << initial << "'" << (index == 0 ? "" : " hidden")
        << "><div class='message' aria-label='Jump to a byte'><button type='button' data-jump='0' "
        << "aria-label='Show the initial tree'>Start</button>";
    for (size_t i = 0; i < input.size(); ++i)
      out << "<button type='button' data-jump='" << i + 1 << "' aria-label='Show byte " << i + 1
          << "'>" << escape(byte_label(static_cast<unsigned char>(input[i]))) << "</button>";
    out << "</div>";
    for (size_t i = 0; i < frames.size(); ++i)
      write_frame(out, frames[i], i, initial);
    write_cost_chart(out, frames);
    out << "</div>";
  }

  /** @brief Generate the complete self-contained gallery from real encoder traces.
   *  @param out Destination HTML stream.
   *  @param messages Built-in scenarios or one user-supplied message.
   */
  void write_gallery(std::ostream &out, const Aleph::Array<std::string> &messages)
  {
    out << R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Aleph / Adaptive Huffman, one byte at a time</title>
<style>
:root{color-scheme:light;--bg:#f6f5f1;--paper:#fff;--ink:#24342f;--muted:#62716a;--line:#dbe1da;--accent:#147764;--soft:#e5f3ec;--literal:#9b601b;--literal-bg:#f9efdf;--node:#f0f2ed}
@media(prefers-color-scheme:dark){:root{color-scheme:dark;--bg:#16211d;--paper:#1e2c26;--ink:#e3ebe4;--muted:#afbeb3;--line:#42554b;--accent:#80d3b3;--soft:#243f34;--literal:#f2c387;--literal-bg:#403322;--node:#2b3931}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.5 system-ui,-apple-system,sans-serif}main{max-width:1200px;margin:auto;padding:44px 32px}h1,h2,p{margin:0}h1{font-size:clamp(28px,4vw,42px);font-weight:650;letter-spacing:-1.4px;line-height:1.15;margin:12px 0}h2{font-size:16px;font-weight:650}p{color:var(--muted)}.brand,.eyebrow{font-size:11px;letter-spacing:2px;font-weight:750;color:var(--accent)}.intro{max-width:700px;font-size:16px}.hero{display:flex;justify-content:space-between;gap:30px;align-items:flex-start;margin-bottom:30px}.verified{white-space:nowrap;border:1px solid var(--line);border-radius:24px;padding:7px 12px;font-size:12px;color:var(--accent);margin-top:4px}button,select,input{font:inherit}button,select{color:var(--ink);background:var(--paper);border:1px solid var(--line);border-radius:6px;padding:8px 12px}button{cursor:pointer}button:hover{background:var(--soft)}button:disabled{opacity:.35;cursor:default}button:focus-visible,select:focus-visible,input:focus-visible{outline:2px solid var(--accent);outline-offset:3px}.controls{display:flex;align-items:center;gap:10px;flex-wrap:wrap;border-block:1px solid var(--line);padding:16px 0}.controls label{font-size:12px;color:var(--muted)}.controls select{max-width:100%}.transport{display:flex;gap:6px;margin-left:auto}#play{background:var(--accent);color:var(--paper);border-color:var(--accent);min-width:70px}.slider-row{display:flex;align-items:center;gap:16px;margin:18px 0 14px}.slider-row label{font-size:13px;white-space:nowrap}input[type=range]{flex:1;min-width:60px;accent-color:var(--accent)}output{font:13px ui-monospace,monospace;min-width:62px;text-align:right}.message{display:flex;gap:5px;overflow-x:auto;padding:6px 0 20px}.message button{font:14px ui-monospace,monospace;min-width:30px;padding:7px 9px;white-space:nowrap}.message button.past{background:var(--soft)}.message button[aria-pressed=true]{background:var(--accent);color:var(--paper);border-color:var(--accent)}.metrics{display:flex;gap:32px;padding:12px 0 23px}.metrics div{min-width:90px}.metrics span{display:block;font-size:12px;color:var(--muted)}.metrics strong{font-size:28px;font-weight:600;font-variant-numeric:tabular-nums}.workspace{display:grid;grid-template-columns:minmax(0,1fr) 265px;gap:22px;align-items:start}.tree-panel{background:var(--paper);border:1px solid var(--line);border-radius:12px;overflow:hidden}.panel-heading{display:flex;justify-content:space-between;gap:16px;align-items:center;padding:20px 22px}.step-tag{font-size:12px;color:var(--muted)}.tree-wrap{overflow:auto;padding:4px 12px}.tree{width:100%;display:block;max-height:650px}.edge{fill:none;stroke:var(--line);stroke-width:1.5}.edge.active{stroke:var(--accent);stroke-width:2.4}.edge-label,.tree text{font-family:ui-monospace,monospace;text-anchor:middle;fill:var(--muted);font-size:12px}.node circle,.node rect{fill:var(--node);stroke:var(--line);stroke-width:1}.node .symbol{fill:var(--ink);font-weight:650;font-size:14px}.node .sum{fill:var(--ink);font-size:14px}.node .weight{font-size:11px}.node.current rect{fill:var(--soft);stroke:var(--accent);stroke-width:1.7}.node.nyt rect{fill:var(--literal-bg);stroke:var(--literal);stroke-dasharray:4 3}.caption{font-size:11px;line-height:1.6;padding:14px 22px 18px}.event{padding:0 0 22px;border-bottom:1px solid var(--line)}.event h2{font-size:24px;margin:8px 0}.event p{font-size:13px}.event code,.codes code{font-family:ui-monospace,monospace}.bit-fields{display:flex;flex-wrap:wrap;gap:8px;margin:14px 0}.bit-fields div{padding:10px 12px;background:var(--soft);border-radius:6px}.bit-fields span{display:block;font-size:10px;color:var(--muted);margin-bottom:3px}.bit-fields code{font-size:16px;color:var(--accent);overflow-wrap:anywhere}.bit-fields .literal{background:var(--literal-bg)}.literal code{color:var(--literal)}.event-total{font-size:12px!important}.codes{padding-top:20px}.codes h2{margin-bottom:9px}table{width:100%;border-collapse:collapse;font-size:12px}th,td{padding:7px 5px;text-align:left;border-bottom:1px solid var(--line)}th{font-weight:500;color:var(--muted)}th:nth-child(2),td:nth-child(2){text-align:right;padding-right:14px}td:last-child{overflow-wrap:anywhere;max-width:130px}.selected td{background:var(--soft)}.costs{margin-top:25px;border-top:1px solid var(--line);padding-top:20px}.costs h2{margin-bottom:12px}.chart-wrap{overflow:auto}.cost-chart{width:100%;min-width:550px;max-height:220px;display:block}.baseline{stroke:var(--muted);stroke-width:1;stroke-dasharray:5 4}.cost-bar rect{fill:var(--accent)}.cost-bar.future{opacity:.25}.cost-value,.cost-byte,.axis-label{font:11px ui-monospace,monospace;fill:var(--muted)}.cost-value,.cost-byte{text-anchor:middle}.cost-value{fill:var(--ink)}.costs .caption{padding-left:0}footer{border-top:1px solid var(--line);padding-top:18px;margin-top:18px;font-size:12px;color:var(--muted)}[hidden]{display:none!important}
@media(max-width:800px){main{padding:26px 16px}.hero{display:block}.verified{display:inline-block;margin-top:18px}.workspace{grid-template-columns:1fr}aside{display:grid;grid-template-columns:1fr 1fr;gap:22px}.event{border:0}.codes{padding:0}.metrics{gap:18px;flex-wrap:wrap}.metrics div{min-width:80px}.controls{align-items:flex-end}.transport{margin-left:0}.tree{max-height:550px}}
@media(max-width:430px){aside{grid-template-columns:1fr}.metrics strong{font-size:24px}.codes{padding-bottom:10px}.hero h1{letter-spacing:-.8px}}
</style></head><body><main id="huffman-gallery">
<header class="hero"><div><div class="brand">ALEPH / ALGORITHM EXPLORER</div>
<h1>Watch the code learn.</h1><p class="intro">One byte arrives. A code is sent. The tree changes. Step through adaptive Huffman coding and see exactly what both peers know.</p></div>
<span class="verified">Every step round-trip verified</span></header>
<div class="controls"><label for="scenario">Message</label><select id="scenario">
)HTML";
    for (size_t i = 0; i < messages.size(); ++i)
      out << "<option value='" << i << "'>"
          << (messages[i].empty() ? "Empty message" : escape(messages[i])) << "</option>";
    out << R"HTML(</select><div class="transport"><button id="previous" type="button" aria-label="Previous byte">Previous</button><button id="play" type="button" aria-pressed="false">Play</button><button id="next" type="button" aria-label="Next byte">Next</button></div></div>
<div class="slider-row"><label for="step">Processed bytes</label><input id="step" type="range" min="0" value="0" max="0"><output id="position" for="step" aria-live="polite"></output></div>
)HTML";
    for (size_t i = 0; i < messages.size(); ++i)
      write_scenario(out, messages[i], i);
    out << R"HTML(<footer>FGK / Faller–Gallager–Knuth. First appearance = NYT path + 8 literal bits; known byte = current leaf path. Tree and code table show the state <strong>after</strong> the update. There is no tree header or end marker: preserve the exact bit count.</footer>
</main><script>
(() => {
  'use strict';
  const root = document.getElementById('huffman-gallery');
  const cases = [...root.querySelectorAll('.scenario')];
  const select = root.querySelector('#scenario');
  const slider = root.querySelector('#step');
  const position = root.querySelector('#position');
  const previous = root.querySelector('#previous');
  const next = root.querySelector('#next');
  const play = root.querySelector('#play');
  let timer = null;
  const currentCase = () => cases[Number(select.value)];
  function stop() {
    if (timer !== null) clearInterval(timer);
    timer = null;
    play.textContent = 'Play';
    play.setAttribute('aria-pressed', 'false');
  }
  function show(value) {
    const current = currentCase();
    const length = Number(current.dataset.length);
    const step = Math.max(0, Math.min(length, Number(value)));
    current.dataset.current = String(step);
    cases.forEach(item => { item.hidden = item !== current; });
    current.querySelectorAll('.frame').forEach(frame => {
      frame.hidden = Number(frame.dataset.step) !== step;
    });
    current.querySelectorAll('[data-jump]').forEach(button => {
      const at = Number(button.dataset.jump);
      button.setAttribute('aria-pressed', String(at === step));
      button.classList.toggle('past', at > 0 && at < step);
    });
    current.querySelectorAll('.cost-bar').forEach(bar => {
      bar.classList.toggle('future', Number(bar.dataset.index) > step);
    });
    slider.max = String(length);
    slider.value = String(step);
    position.textContent = `${step} / ${length}`;
    previous.disabled = step === 0;
    next.disabled = step === length;
    play.disabled = length === 0;
    if (step === length) stop();
  }
  slider.addEventListener('input', () => { stop(); show(slider.value); });
  select.addEventListener('change', () => { stop(); show(currentCase().dataset.current); });
  previous.addEventListener('click', () => { stop(); show(Number(slider.value) - 1); });
  next.addEventListener('click', () => { stop(); show(Number(slider.value) + 1); });
  root.addEventListener('click', event => {
    const button = event.target.closest('[data-jump]');
    if (button) { stop(); show(button.dataset.jump); }
  });
  play.addEventListener('click', () => {
    if (timer !== null) { stop(); return; }
    if (Number(slider.value) === Number(slider.max)) show(0);
    play.textContent = 'Pause';
    play.setAttribute('aria-pressed', 'true');
    timer = setInterval(() => show(Number(slider.value) + 1), 950);
  });
  document.addEventListener('visibilitychange', () => { if (document.hidden) stop(); });
  show(currentCase().dataset.current);
})();
</script></body></html>
)HTML";
  }
}

/** @brief Generate a self-contained, self-checking adaptive Huffman gallery.
 *  @param argc Number of command-line arguments.
 *  @param argv Optional output path, then optional custom text of at most 32 bytes.
 *  @return Zero on success; one for invalid arguments, I/O or verification errors.
 */
int main(int argc, char *argv[])
{
  try
    {
      if (argc > 1 and std::string_view(argv[1]) == "--help")
        {
          std::cout << "Usage: adaptive_huffman_visual_example [output.html] [text]\n"
                    << "Text may contain at most 32 bytes; default: three illustrated scenarios.\n";
          return 0;
        }
      ah_invalid_argument_if(argc > 3) << "Use adaptive_huffman_visual_example [output.html] [text]";
      Aleph::Array<std::string> messages;
      if (argc > 2)
        {
          const std::string_view input(argv[2]);
          ah_invalid_argument_if(input.size() > 32) << "Use at most 32 bytes for a readable gallery";
          messages.append(std::string(input));
        }
      else
        {
          messages.append("ABRACADABRA");
          messages.append("BANANA BANDANA");
          messages.append("AAAAAAAABBBBBBBBBBBBBBBB");
        }
      const std::string path = argc > 1 ? argv[1] : "adaptive_huffman_gallery.html";
      std::ofstream output(path);
      ah_runtime_error_unless(output) << "Cannot open gallery output: " << path;
      output.imbue(std::locale::classic());
      write_gallery(output, messages);
      output.close();
      ah_runtime_error_unless(output) << "Cannot finish writing gallery: " << path;
      std::cout << "Generated adaptive Huffman gallery: " << path << '\n'
                << "Scenarios: " << messages.size() << '\n'
                << "All frames verified against the real online encoder and decoder.\n"
                << "Open the HTML file in a browser; it works entirely offline.\n";
      return 0;
    }
  catch (const std::exception &error)
    {
      std::cerr << error.what() << '\n';
      return 1;
    }
}
