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
 * @file blossom_example.cc
 * @brief General graph maximum matching with Edmonds-Blossom.
 *
 * Demonstrates:
 * - Maximum cardinality matching in non-bipartite graphs
 * - Same algorithm over three Aleph graph backends
 * - TikZ export with highlighted matching arcs
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target blossom_example
 *   ./build/Examples/blossom_example
 *
 * Generated files:
 * - /tmp/blossom_pentagon_stems.tex
 * - /tmp/blossom_double_flower.tex
 */

# include <algorithm>
# include <fstream>
# include <iomanip>
# include <iostream>
# include <set>
# include <string>
# include <utility>
# include <vector>

# include <Blossom.H>
# include <tpl_graph.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace std;
using namespace Aleph;

namespace
{
  struct Node_Pos
  {
    double x = 0;
    double y = 0;
    string label;
  };

  struct Scenario
  {
    string slug;
    string title;
    vector<Node_Pos> nodes;
    vector<pair<size_t, size_t>> edges;
  };

  template <class GT>
  /**
   * @brief Constructs a graph of type `GT` that represents the given scenario.
   *
   * Builds a graph containing one node per entry in `s.nodes` (inserted with integer
   * ids 0..n-1) and one directed arc for each pair in `s.edges` with weight 1.
   *
   * @tparam GT Graph type; must provide `using Node`, `Node*`, `insert_node(int)` and
   *            `insert_arc(Node*, Node*, int)` operations.
   * @param s Scenario describing vertices (positions and labels) and an edge list.
   * @return GT Graph instance populated with the scenario's nodes and arcs.
   */
  GT build_graph(const Scenario & s)
  {
    GT g;
    vector<typename GT::Node *> nodes;
    nodes.reserve(s.nodes.size());

    for (size_t i = 0; i < s.nodes.size(); ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v] : s.edges)
      {
        ah_range_error_if(u >= s.nodes.size() or v >= s.nodes.size())
          << "Edge endpoint out of range";
        g.insert_arc(nodes[u], nodes[v], 1);
      }

    return g;
  }

  template <class GT>
  /**
   * @brief Extracts sorted, deduplicated matching vertex pairs from a graph's matching.
   *
   * Returns each matched edge as a pair (u, v) with u <= v, sorted lexicographically
   * and with duplicates removed.
   *
   * @param g Graph instance whose nodes' info fields contain vertex indices.
   * @param matching List of arc pointers representing the matching.
   * @return std::vector<std::pair<size_t, size_t>> Vector of unique, sorted vertex-index
   *         pairs corresponding to matched edges; each pair is ordered with the smaller
   *         index first.
   */
  vector<pair<size_t, size_t>> extract_matching_pairs(
      const GT & g,
      const DynDlist<typename GT::Arc *> & matching)
  {
    vector<pair<size_t, size_t>> pairs;

    for (auto it = matching.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        size_t u = static_cast<size_t>(g.get_src_node(arc)->get_info());
        size_t v = static_cast<size_t>(g.get_tgt_node(arc)->get_info());
        if (u > v)
          swap(u, v);
        pairs.emplace_back(u, v);
      }

    sort(pairs.begin(), pairs.end());
    pairs.erase(unique(pairs.begin(), pairs.end()), pairs.end());

    return pairs;
  }

  /**
   * Format a list of vertex-index pairs into a human-readable string.
   *
   * Each pair is formatted as `(u,v)` and pairs are separated by single spaces.
   *
   * @param pairs Vector of vertex index pairs to format.
   * @return `"(empty)"` if `pairs` is empty; otherwise a space-separated list of `(u,v)` entries.
   */
  string format_pairs(const vector<pair<size_t, size_t>> & pairs)
  {
    if (pairs.empty())
      return "(empty)";

    string out;
    for (size_t i = 0; i < pairs.size(); ++i)
      {
        const auto & [u, v] = pairs[i];
        out += "(" + to_string(u) + "," + to_string(v) + ")";
        if (i + 1 < pairs.size())
          out += " ";
      }
    return out;
  }

  /**
   * @brief Write a standalone TikZ/LaTeX document visualizing a graph scenario and highlighting matched edges.
   *
   * Generates a .tex file at the provided path containing a TikZ picture that draws all vertices and edges
   * from the scenario, renders matched edges with a distinct style, and annotates the matching size.
   *
   * @param s Scenario describing vertex positions, labels, and edge list.
   * @param matching_pairs Vector of matched vertex-index pairs to highlight in the output.
   * @param file_path Filesystem path where the generated .tex file will be written.
   *
   * If the file cannot be opened for writing, an error message is written to stderr and the function returns
   * without producing output.
   */
  void write_tikz(const Scenario & s,
                  const vector<pair<size_t, size_t>> & matching_pairs,
                  const string & file_path)
  {
    set<pair<size_t, size_t>> matched_set;
    for (auto [u, v] : matching_pairs)
      {
        if (u > v)
          swap(u, v);
        matched_set.emplace(u, v);
      }

    ofstream out(file_path);
    if (not out)
      {
        cerr << "Cannot write file: " << file_path << endl;
        return;
      }

    out << "\\documentclass[tikz,border=10pt]{standalone}\n";
    out << "\\usepackage{tikz}\n";
    out << "\\usetikzlibrary{arrows.meta}\n";
    out << "\\begin{document}\n";
    out << "\\begin{tikzpicture}[x=1cm,y=1cm]\n";

    out << "  \\fill[blue!2] (-4.2,-3.2) rectangle (4.2,3.2);\n";
    out << "  \\node[font=\\bfseries\\large] at (0,2.95) {"
        << s.title << "};\n";

    out << "  \\tikzset{"
        << "edge/.style={line width=0.9pt, draw=black!65},"
        << "match/.style={line width=2.6pt, draw=orange!90!black},"
        << "vertex/.style={circle, draw=black!70, fill=white, minimum size=9mm, inner sep=1pt, font=\\small\\bfseries}"
        << "}\n";

    for (size_t i = 0; i < s.nodes.size(); ++i)
      {
        const auto & node = s.nodes[i];
        out << "  \\node[vertex] (n" << i << ") at ("
            << fixed << setprecision(2) << node.x << "," << node.y << ") {"
            << node.label << "};\n";
      }

    for (const auto & [u, v] : s.edges)
      {
        ah_range_error_if(u >= s.nodes.size() or v >= s.nodes.size())
          << "Edge endpoint out of range";

        size_t a = u;
        size_t b = v;
        if (a > b)
          swap(a, b);

        if (matched_set.find(std::make_pair(a, b)) != matched_set.end())
          continue;

        out << "  \\draw[edge] (n" << u << ") -- (n" << v << ");\n";
      }

    for (auto [u, v] : matching_pairs)
      out << "  \\draw[match] (n" << u << ") -- (n" << v << ");\n";

    out << "  \\node[font=\\small] at (0,-2.95) {matching size = "
        << matching_pairs.size() << "};\n";

    out << "\\end{tikzpicture}\n";
    out << "\\end{document}\n";
  }

  template <class GT>
  /**
   * Compute the maximum-cardinality matching for the given scenario and return its size and matched vertex pairs.
   *
   * @param s Scenario describing the graph (nodes, positions and edges) to build and solve.
   * @return pair whose first element is the matching cardinality (number of matched vertex pairs) and whose second element is a vector of unique, sorted vertex-index pairs `(u, v)` with `u <= v` representing the matching.
   */
  pair<size_t, vector<pair<size_t, size_t>>> solve_case(const Scenario & s)
  {
    GT g = build_graph<GT>(s);
    DynDlist<typename GT::Arc *> matching;
    const size_t cardinality = blossom_maximum_cardinality_matching<GT>(g, matching);
    return std::make_pair(cardinality, extract_matching_pairs(g, matching));
  }

  template <class GT>
  /**
   * @brief Run the matching backend for a scenario, print its result, and validate consistency.
   *
   * Executes the maximum-cardinality matching for graph type `GT` on scenario `s`, prints
   * the backend name with the matching size and pairs, records the first backend's result
   * into the provided canonical references, and emits a warning if the matching size
   * differs from the canonical cardinality.
   *
   * @tparam GT Graph type used as the backend.
   * @param backend_name Human-readable name of the backend for logging.
   * @param s Scenario describing nodes and edges to solve.
   * @param cardinality_ref Reference to store the canonical matching cardinality from the first backend.
   * @param pairs_ref Reference to store the canonical matching pairs from the first backend.
   * @param is_first Flag indicating whether this is the first backend being run; set to false after storing canonical results.
   */
  void print_backend_result(const string & backend_name,
                            const Scenario & s,
                            size_t & cardinality_ref,
                            vector<pair<size_t, size_t>> & pairs_ref,
                            bool & is_first)
  {
    auto [cardinality, pairs] = solve_case<GT>(s);

    if (is_first)
      {
        cardinality_ref = cardinality;
        pairs_ref = pairs;
        is_first = false;
      }

    cout << "  " << backend_name << " -> size " << cardinality
         << " | " << format_pairs(pairs) << "\n";

    if (cardinality != cardinality_ref)
      cerr << "  Warning: cardinality mismatch across backends\n";
  }

  /**
   * @brief Execute a scenario: run matching backends, print results, and export a TikZ visualization.
   *
   * Runs the maximum-cardinality matching on the scenario's graph using multiple backend graph
   * implementations, prints each backend's matching size and pairs to stdout, and writes a TikZ
   * file representing the graph with matched edges highlighted.
   *
   * @param s Scenario describing the graph topology and node layout (slug, title, nodes, edges).
   *
   * @note Produces console output and writes the TikZ file to /tmp/blossom_<slug>.tex.
   */
  void run_scenario(const Scenario & s)
  {
    cout << "\n" << s.title << "\n";
    cout << string(s.title.size(), '-') << "\n";

    size_t cardinality = 0;
    vector<pair<size_t, size_t>> canonical_pairs;
    bool first = true;

    print_backend_result<List_Graph<Graph_Node<int>, Graph_Arc<int>>>(
        "List_Graph", s, cardinality, canonical_pairs, first);

    print_backend_result<List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>>(
        "List_SGraph", s, cardinality, canonical_pairs, first);

    print_backend_result<Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>(
        "Array_Graph", s, cardinality, canonical_pairs, first);

    const string tex_path = "/tmp/blossom_" + s.slug + ".tex";
    write_tikz(s, canonical_pairs, tex_path);

    cout << "  TikZ export: " << tex_path << "\n";
  }
}

/**
 * @brief Program entry that runs two Edmonds–Blossom example scenarios and emits TikZ visualizations.
 *
 * Executes two predefined Scenario instances (pentagon_stems and double_flower), prints per-backend
 * matching results to stdout, writes corresponding TikZ files to /tmp, and prints instructions to
 * convert those TeX files to PDF.
 *
 * @return int Exit code: `0` on success.
 */
int main()
{
  const Scenario pentagon_stems{
    .slug = "pentagon_stems",
    .title = "Blossom demo: odd cycle + stems",
    .nodes = {
      {-1.90,  1.10, "0"},
      { 0.00,  2.20, "1"},
      { 1.90,  1.10, "2"},
      { 1.30, -1.10, "3"},
      {-1.30, -1.10, "4"},
      {-3.10,  2.35, "5"},
      { 3.10,  2.35, "6"},
      { 2.70, -2.05, "7"}
    },
    .edges = {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0},
      {1, 5}, {2, 6}, {3, 7}
    }
  };

  const Scenario double_flower{
    .slug = "double_flower",
    .title = "Blossom demo: two odd flowers",
    .nodes = {
      {-3.00,  1.40, "0"},
      {-1.60,  2.40, "1"},
      {-0.40,  1.20, "2"},
      {-0.70, -0.60, "3"},
      {-2.40, -0.90, "4"},
      { 0.80,  0.10, "5"},
      { 2.10,  1.60, "6"},
      { 3.20,  0.20, "7"},
      { 2.40, -1.50, "8"},
      { 0.30,  2.55, "9"}
    },
    .edges = {
      {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0},
      {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 5},
      {2, 5}, {3, 7}, {1, 9}, {6, 9}
    }
  };

  cout << "Edmonds-Blossom maximum matching examples\n";
  cout << "========================================\n";

  run_scenario(pentagon_stems);
  run_scenario(double_flower);

  cout << "\nTo compile a TikZ file to PDF (optional):\n";
  cout << "  cd /tmp && pdflatex blossom_pentagon_stems.tex\n";
  cout << "  cd /tmp && pdflatex blossom_double_flower.tex\n";

  return 0;
}