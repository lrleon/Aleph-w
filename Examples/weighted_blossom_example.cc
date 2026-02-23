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


/**
 * @file weighted_blossom_example.cc
 * @brief Maximum-weight matching in general graphs (weighted blossom).
 *
 * Demonstrates:
 * - Weighted blossom matching on non-bipartite graphs
 * - Pure maximum-weight objective vs maximum-cardinality mode
 * - Cross-backend consistency on List/SGraph/Array graph types
 * - TikZ exports with weighted labels and highlighted matched arcs
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target weighted_blossom_example
 *   ./build/Examples/weighted_blossom_example
 *
 * Generated files (examples):
 * - /tmp/weighted_blossom_odd_cycle_bridge_max_weight.tex
 * - /tmp/weighted_blossom_odd_cycle_bridge_max_cardinality.tex
 * - /tmp/weighted_blossom_tradeoff_max_weight.tex
 * - /tmp/weighted_blossom_tradeoff_max_cardinality.tex
 */

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#include <Blossom_Weighted.H>
#include <tpl_agraph.H>
#include <tpl_dynArray.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  struct Node_Pos
  {
    double x = 0;
    double y = 0;
    std::string label;
  };


  struct Weighted_Edge
  {
    size_t u = 0;
    size_t v = 0;
    long long w = 0;
  };


  struct Scenario
  {
    std::string slug;
    std::string title;
    DynArray<Node_Pos> nodes;
    DynArray<Weighted_Edge> edges;
  };


  struct Solve_Output
  {
    long long total_weight = 0;
    size_t cardinality = 0;
    DynArray<std::pair<size_t, size_t>> matched_pairs;
  };


  template <class GT>
  /**
   * @brief Construct a graph of type GT from a Scenario.
   *
   * Builds a GT instance with one node per Scenario node and inserts arcs for
   * each Scenario edge using the provided weights.
   *
   * @tparam GT Graph type to construct; must provide insert_node(int) and
   *            insert_arc(node_ptr, node_ptr, weight) APIs.
   * @param s Scenario describing node positions and weighted undirected edges.
   * @return GT Constructed graph whose nodes were created with their Scenario
   *            indices as the integer payload and whose arcs reflect Scenario.edges.
   */
  GT build_graph(const Scenario & s)
  {
    GT g;
    DynArray<typename GT::Node *> nodes;

    for (size_t i = 0; i < s.nodes.size(); ++i)
      nodes.append(g.insert_node(static_cast<int>(i)));

    for (const auto & e : s.edges)
      g.insert_arc(nodes(e.u), nodes(e.v), e.w);

    return g;
  }


  template <class GT>
  /**
   * @brief Compute a maximum-weight matching for the given scenario and return its solution.
   *
   * Builds a graph from the provided Scenario, solves for a maximum-weight matching
   * (optionally preferring larger cardinality when weights tie), and returns the
   * matching details. Matched pairs are reported as unique, sorted (u,v) tuples
   * with u <= v in deterministic order using the node indices from the Scenario.
   *
   * @param s The scenario describing nodes, edges, and weights to solve.
   * @param max_cardinality If `true`, prefer matchings with larger cardinality when total weight ties.
   * @return Solve_Output Contains `total_weight` (sum of weights in the matching),
   * `cardinality` (number of matched edges), and `matched_pairs` (array of `(u,v)` pairs with `u <= v`).
   */
  Solve_Output solve_case(const Scenario & s, bool max_cardinality)
  {
    GT g = build_graph<GT>(s);

    DynDlist<typename GT::Arc *> matching;
    const auto result = blossom_maximum_weight_matching<GT>(
        g,
        matching,
        Dft_Dist<GT>(),
        Dft_Show_Arc<GT>(),
        max_cardinality);

    DynSetTree<std::pair<size_t, size_t>> sorted_pairs;
    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        size_t u = static_cast<size_t>(g.get_src_node(arc)->get_info());
        size_t v = static_cast<size_t>(g.get_tgt_node(arc)->get_info());
        if (u > v)
          std::swap(u, v);
        sorted_pairs.insert(std::make_pair(u, v));
      }

    DynArray<std::pair<size_t, size_t>> pairs;
    for (auto it = sorted_pairs.get_it(); it.has_curr(); it.next_ne())
      pairs.append(it.get_curr());

    return Solve_Output{result.total_weight, result.cardinality, pairs};
  }


  /**
   * @brief Formats an array of index pairs into a readable sequence.
   *
   * Produces a space-separated sequence of pair tokens in the form "(u,v) (x,y) ..."
   * or the literal "(empty)" when the input array contains no pairs.
   *
   * @param pairs Array of index pairs to format.
   * @return string A string containing the formatted pairs or "(empty)" if no pairs.
   */
  std::string format_pairs(const DynArray<std::pair<size_t, size_t>> & pairs)
  {
    if (pairs.is_empty())
      return "(empty)";

    std::string out;
    for (size_t i = 0; i < pairs.size(); ++i)
      {
        const auto & p = pairs(i);
        out += "(" + std::to_string(p.first) + "," + std::to_string(p.second) + ")";
        if (i + 1 < pairs.size())
          out += " ";
      }
    return out;
  }


  /**
   * @brief Write a LaTeX/TikZ document that visualizes a graph scenario and its matching.
   *
   * Generates a standalone TikZ/LaTeX file at the given path containing:
   * - plotted vertices at coordinates from the scenario with labels,
   * - edges with weight labels,
   * - matched edges highlighted,
   * - a header describing the objective mode and a footer summarizing matching size and total weight.
   *
   * @param s Scenario describing node positions, labels, and weighted undirected edges.
   * @param output Solve_Output containing matched pairs, matching cardinality, and total weight to visualize.
   * @param max_cardinality If true, the document's objective description will indicate "max-cardinality then max-weight"; otherwise "pure max-weight".
   * @param file_path Filesystem path where the generated .tex file will be written; the file is overwritten if it exists. If the file cannot be opened, the function writes a diagnostic to stderr and returns without producing output.
   */
  void write_tikz(const Scenario & s,
                  const Solve_Output & output,
                  bool max_cardinality,
                  const std::string & file_path)
  {
    DynSetTree<std::pair<size_t, size_t>> matched_set(output.matched_pairs);

    std::ofstream out(file_path);
    if (not out)
      {
        std::cerr << "Cannot write file: " << file_path << "\n";
        return;
      }

    out << "\\documentclass[tikz,border=10pt]{standalone}\n";
    out << "\\usepackage{tikz}\n";
    out << "\\usetikzlibrary{arrows.meta,calc}\n";
    out << "\\begin{document}\n";
    out << "\\begin{tikzpicture}[x=1cm,y=1cm]\n";

    out << "  \\path[shade, left color=cyan!6, right color=blue!5] "
        << "(-4.6,-3.4) rectangle (4.6,3.4);\n";

    out << "  \\node[font=\\bfseries\\large] at (0,3.05) {"
        << s.title << "};\n";

    out << "  \\node[font=\\small] at (0,2.65) {objective: "
        << (max_cardinality ? "max-cardinality then max-weight" : "pure max-weight")
        << "};\n";

    out << "  \\tikzset{"
        << "edge/.style={draw=black!58, line width=0.95pt, shorten <=2pt, shorten >=2pt},"
        << "match/.style={draw=orange!92!black, line width=2.7pt},"
        << "wlabel/.style={fill=white, inner sep=1.2pt, font=\\scriptsize},"
        << "vertex/.style={circle, draw=black!70, fill=white, minimum size=9mm, inner sep=1pt, font=\\small\\bfseries}"
        << "}\n";

    // Emit vertex node definitions first so \draw commands can reference them.
    for (size_t i = 0; i < s.nodes.size(); ++i)
      {
        const auto & p = s.nodes(i);
        out << "  \\node[vertex] (n" << i << ") at ("
            << std::fixed << std::setprecision(2) << p.x << "," << p.y << ") {"
            << p.label << "};\n";
      }

    for (const auto & [u_raw, v_raw, w] : s.edges)
      {
        size_t u = u_raw;
        size_t v = v_raw;
        size_t a = std::min(u, v);
        size_t b = std::max(u, v);
        const bool matched = matched_set.contains(std::make_pair(a, b));

        out << "  \\draw[" << (matched ? "match" : "edge") << "] (n"
            << u << ") -- (n" << v << ");\n";

        out << "  \\node[wlabel] at ($(n" << u << ")!0.5!(n" << v << ")$) {"
            << w << "};\n";
      }

    out << "  \\node[font=\\small] at (0,-3.05) {matching size = "
        << output.cardinality
        << ", total weight = " << output.total_weight << "};\n";

    out << "\\end{tikzpicture}\n";
    out << "\\end{document}\n";
  }


  template <class GT>
  /**
   * @brief Runs the solver for a specific graph backend, prints its result, and checks consistency against a canonical reference.
   *
   * Executes the matching solver for the backend identified by `backend` on scenario `s` using the
   * `max_cardinality` mode, prints the backend name, matching cardinality, total weight, and matched pairs,
   * and emits a warning to stderr if the cardinality or total weight differs from `reference`.
   *
   * @param backend Human-readable name of the graph backend being tested (printed to stdout).
   * @param s Scenario describing nodes and weighted edges to solve.
   * @param max_cardinality If `true`, prefer maximum cardinality before maximizing weight; otherwise use pure max-weight objective.
   * @param reference On first invocation for a given mode, updated to the backend's result and subsequently used as the canonical result to compare against.
   * @param first On entry, indicates whether this backend result should initialize `reference`; set to `false` when initialization occurs.
   */
  void print_backend(const std::string & backend,
                     const Scenario & s,
                     bool max_cardinality,
                     Solve_Output & reference,
                     bool & first)
  {
    const Solve_Output got = solve_case<GT>(s, max_cardinality);

    if (first)
      {
        reference = got;
        first = false;
      }

    std::cout << "  " << backend
              << " -> card " << got.cardinality
              << ", weight " << got.total_weight
              << " | " << format_pairs(got.matched_pairs) << "\n";

    if (got.cardinality != reference.cardinality
        or got.total_weight != reference.total_weight)
      {
        std::cerr << "  Warning: backend objective mismatch\n";
      }
  }


  /**
   * Execute the given Scenario: solve maximum-weight matchings (in two modes) across multiple backends, print per-backend results, and produce TikZ visualizations.
   *
   * For both modes — pure maximum-weight and maximum-cardinality-then-maximum-weight — this function runs the solver on three graph backends, prints each backend's cardinality, total weight, and matched pairs (and reports objective mismatches), and writes a TikZ/LaTeX file summarizing the matching to /tmp using the scenario slug and mode in the filename.
   *
   * @param s Scenario to run; its title is printed and its slug is used to name the generated TikZ files.
   */
  void run_scenario(const Scenario & s)
  {
    std::cout << "\n" << s.title << "\n";
    std::cout << std::string(s.title.size(), '-') << "\n";

    for (bool max_cardinality : {false, true})
      {
        std::cout << (max_cardinality
                      ? "\nMode: maximum-cardinality then maximum-weight\n"
                      : "\nMode: pure maximum-weight\n");

        Solve_Output canonical;
        bool first = true;

        print_backend<List_Graph<Graph_Node<int>, Graph_Arc<long long>>>(
            "List_Graph", s, max_cardinality, canonical, first);

        print_backend<List_SGraph<Graph_Snode<int>, Graph_Sarc<long long>>>(
            "List_SGraph", s, max_cardinality, canonical, first);

        print_backend<Array_Graph<Graph_Anode<int>, Graph_Aarc<long long>>>(
            "Array_Graph", s, max_cardinality, canonical, first);

        const std::string suffix = max_cardinality ? "max_cardinality" : "max_weight";
        const std::string tex_path = "/tmp/weighted_blossom_" + s.slug + "_" + suffix + ".tex";

        write_tikz(s, canonical, max_cardinality, tex_path);
        std::cout << "  TikZ export: " << tex_path << "\n";
      }
  }

} /**
 * @brief Runs example scenarios demonstrating maximum-weight matching and writes TikZ visualizations.
 *
 * Executes two predefined graph scenarios that exercise the weighted blossom algorithm in both
 * pure max-weight and max-cardinality-then-max-weight modes across multiple graph backends, prints
 * per-backend results and warnings on objective mismatches, and emits .tex files visualizing each
 * computed matching.
 *
 * @return int Exit status code; `0` on successful completion.
 */


int main()
{
  const Scenario odd_cycle_bridge{
    .slug = "odd_cycle_bridge",
    .title = "Weighted blossom demo: odd cycle + bridge",
    .nodes = {
      {-2.20,  1.30, "0"},
      {-0.50,  2.30, "1"},
      { 1.30,  1.65, "2"},
      { 1.55, -0.35, "3"},
      {-0.20, -1.50, "4"},
      {-2.05, -0.70, "5"},
      { 3.35,  1.10, "6"},
      { 3.05, -1.05, "7"}
    },
    .edges = {
      {0, 1, 9},
      {1, 2, 10},
      {2, 3, 8},
      {3, 4, 7},
      {4, 5, 9},
      {5, 0, 8},
      {1, 5, 6},
      {2, 6, 12},
      {3, 7, 11},
      {6, 7, 5},
      {0, 4, 4}
    }
  };

  const Scenario tradeoff{
    .slug = "tradeoff",
    .title = "Weighted blossom demo: weight/cardinality trade-off",
    .nodes = {
      {-2.35,  0.95, "0"},
      {-0.85,  2.05, "1"},
      { 0.95,  2.00, "2"},
      { 2.35,  0.85, "3"},
      { 1.50, -1.25, "4"},
      {-0.40, -1.95, "5"}
    },
    .edges = {
      {0, 1, 2},
      {0, 4, 3},
      {1, 2, 7},
      {1, 5, 2},
      {2, 3, 9},
      {2, 5, 4},
      {3, 4, 8},
      {3, 5, 4}
    }
  };

  run_scenario(odd_cycle_bridge);
  run_scenario(tradeoff);

  std::cout << "\nDone. Compile generated .tex files with pdflatex to visualize the matchings.\n";

  return 0;
}
