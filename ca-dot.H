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

/** @file ca-dot.H
 *  @brief GraphViz DOT renderer for graph cellular automata.
 *
 *  `Graph_Lattice` frames are rendered as undirected DOT graphs by
 *  default, with one filled node per cell and edges from the precomputed
 *  adjacency list.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_DOT_H
#define CA_DOT_H

#include <concepts>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <ah-errors.H>

#include <ca-frame-stream.H>
#include <tpl_ca_graph_automaton.H>

namespace Aleph {
namespace CA {

/** @brief DOT rendering options. */
struct DOT_Render_Options
{
  std::string graph_name = "CA";  ///< DOT graph identifier
  bool undirected = true;         ///< use `graph`/`--` and suppress duplicate edges
  bool include_labels = true;     ///< label nodes with their numeric id
};

/** @brief Default binary DOT fill-colour palette.
 *
 *  @tparam State cell state type.
 */
template <typename State>
struct Binary_DOT_Palette
{
  State dead{};                        ///< state considered dead
  std::string dead_colour = "white";   ///< fill colour for dead cells
  std::string alive_colour = "black";  ///< fill colour for live cells

  /** @brief Map a cell state to a GraphViz colour.
   *  @param[in] value cell value.
   *  @return DOT colour string.
   */
  [[nodiscard]] std::string operator()(const State &value) const
  {
    return value == dead ? dead_colour : alive_colour;
  }
};

namespace ca_dot_detail {

inline std::string quote(const std::string &s)
{
  std::ostringstream out;
  out << '"';
  for (const char c : s)
    {
      if (c == '"' or c == '\\')
        out << '\\';
      out << c;
    }
  out << '"';
  return out.str();
}

}  // namespace ca_dot_detail

/** @brief Render a graph CA frame as GraphViz DOT.
 *
 *  @tparam Graph graph lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in,out] out output stream.
 *  @param[in] graph graph lattice frame.
 *  @param[in] palette state-to-fill-colour mapper.
 *  @param[in] opts rendering options.
 *  @throws std::runtime_error on stream write failure.
 *
 *  @par Complexity
 *  O(nodes + edges).
 */
template <typename Graph, typename Palette>
inline void render_graph_lattice_dot(std::ostream &out,
                                     const Graph &graph,
                                     Palette &&palette,
                                     const DOT_Render_Options &opts = {})
  requires requires(const Graph &g, std::size_t n, Palette &&p, const typename Graph::state_type &v) {
    g.size();
    g.neighbours(n);
    g.at_node(n);
    { p(v) } -> std::convertible_to<std::string>;
  }
{
  const char *kind = opts.undirected ? "graph" : "digraph";
  const char *edge = opts.undirected ? "--" : "->";
  out << kind << ' ' << opts.graph_name << " {\n";
  out << "  node [shape=circle, style=filled, fixedsize=true, width=0.42];\n";

  for (std::size_t n = 0; n < graph.size(); ++n)
    {
      const std::string fill = palette(graph.at_node(n));
      out << "  n" << n << " [fillcolor=" << ca_dot_detail::quote(fill);
      if (opts.include_labels)
        out << ", label=" << ca_dot_detail::quote(std::to_string(n));
      else
        out << ", label=\"\"";
      if (fill == "black" or fill == "#000000")
        out << ", fontcolor=\"white\"";
      out << "];\n";
    }

  for (std::size_t n = 0; n < graph.size(); ++n)
    {
      const auto &row = graph.neighbours(n);
      for (std::size_t k = 0; k < row.size(); ++k)
        {
          const std::size_t m = row[k];
          if (opts.undirected and m < n)
            continue;
          out << "  n" << n << ' ' << edge << " n" << m << ";\n";
        }
    }
  out << "}\n";
  ah_runtime_error_if(not out) << "render_graph_lattice_dot: output stream failed";
}

/** @brief Return a graph frame rendered as DOT.
 *
 *  @tparam Graph graph lattice type.
 *  @tparam Palette invocable `state_type -> std::string`.
 *  @param[in] graph graph lattice frame.
 *  @param[in] palette state-to-fill-colour mapper.
 *  @param[in] opts rendering options.
 *  @return DOT document.
 */
template <typename Graph, typename Palette>
[[nodiscard]] inline std::string render_graph_lattice_dot_string(const Graph &graph,
                                                                 Palette &&palette,
                                                                 const DOT_Render_Options &opts = {})
{
  std::ostringstream out;
  render_graph_lattice_dot(out, graph, std::forward<Palette>(palette), opts);
  return out.str();
}

/** @brief Path-pattern sink that writes GraphViz DOT frames.
 *
 *  @tparam Palette invocable `state_type -> std::string`.
 */
template <typename Palette>
class Dot_Frame_Sink
{
  std::filesystem::path pattern_;
  Palette palette_;
  DOT_Render_Options opts_{};
  std::size_t zero_pad_ = 6;

public:
  /** @brief Build a DOT frame sink.
   *  @param[in] path_pattern output path pattern.
   *  @param[in] palette state-to-fill-colour mapper.
   *  @param[in] opts rendering options.
   *  @param[in] zero_pad minimum width for `{step}`.
   */
  Dot_Frame_Sink(std::filesystem::path path_pattern,
                 Palette palette,
                 DOT_Render_Options opts = {},
                 const std::size_t zero_pad = 6)
    : pattern_(std::move(path_pattern)), palette_(std::move(palette)), opts_(std::move(opts)),
      zero_pad_(zero_pad)
  {}

  /** @brief Write one DOT frame.
   *  @tparam Graph graph lattice type.
   *  @param[in] step simulation step.
   *  @param[in] graph graph frame to write.
   */
  template <typename Graph>
  void accept(const std::size_t step, const Graph &graph)
  {
    const std::filesystem::path path = format_step_path(pattern_, step, zero_pad_);
    if (const auto parent = path.parent_path(); not parent.empty())
      std::filesystem::create_directories(parent);
    std::ofstream out(path);
    ah_runtime_error_if(not out) << "Dot_Frame_Sink::accept: cannot open '" << path.string() << "'";
    render_graph_lattice_dot(out, graph, palette_, opts_);
  }

  /** @brief DOT frames are written eagerly; this is a no-op. */
  static void flush() {}
};

template <typename Palette>
Dot_Frame_Sink(std::filesystem::path, Palette, DOT_Render_Options = {}, std::size_t = 6)
  -> Dot_Frame_Sink<Palette>;

}  // namespace CA
}  // namespace Aleph

#endif  // CA_DOT_H
