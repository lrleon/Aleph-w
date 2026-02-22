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
  pair<size_t, vector<pair<size_t, size_t>>> solve_case(const Scenario & s)
  {
    GT g = build_graph<GT>(s);
    DynDlist<typename GT::Arc *> matching;
    const size_t cardinality = blossom_maximum_cardinality_matching<GT>(g, matching);
    return std::make_pair(cardinality, extract_matching_pairs(g, matching));
  }

  template <class GT>
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
