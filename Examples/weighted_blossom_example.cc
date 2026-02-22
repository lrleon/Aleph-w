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
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <Blossom_Weighted.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

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
    vector<tuple<size_t, size_t, long long>> edges;
  };


  struct Solve_Output
  {
    long long total_weight = 0;
    size_t cardinality = 0;
    vector<pair<size_t, size_t>> matched_pairs;
  };


  template <class GT>
  GT build_graph(const Scenario & s)
  {
    GT g;
    vector<typename GT::Node *> nodes;
    nodes.reserve(s.nodes.size());

    for (size_t i = 0; i < s.nodes.size(); ++i)
      nodes.push_back(g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : s.edges)
      g.insert_arc(nodes[u], nodes[v], w);

    return g;
  }


  template <class GT>
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

    return Solve_Output{result.total_weight, result.cardinality, pairs};
  }


  string format_pairs(const vector<pair<size_t, size_t>> & pairs)
  {
    if (pairs.empty())
      return "(empty)";

    string out;
    for (size_t i = 0; i < pairs.size(); ++i)
      {
        out += "(" + to_string(pairs[i].first) + "," + to_string(pairs[i].second) + ")";
        if (i + 1 < pairs.size())
          out += " ";
      }
    return out;
  }


  void write_tikz(const Scenario & s,
                  const Solve_Output & output,
                  bool max_cardinality,
                  const string & file_path)
  {
    set<pair<size_t, size_t>> matched_set(output.matched_pairs.begin(),
                                          output.matched_pairs.end());

    ofstream out(file_path);
    if (not out)
      {
        cerr << "Cannot write file: " << file_path << "\n";
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
        const auto & p = s.nodes[i];
        out << "  \\node[vertex] (n" << i << ") at ("
            << fixed << setprecision(2) << p.x << "," << p.y << ") {"
            << p.label << "};\n";
      }

    for (const auto & [u_raw, v_raw, w] : s.edges)
      {
        size_t u = u_raw;
        size_t v = v_raw;
        size_t a = min(u, v);
        size_t b = max(u, v);
        const bool matched = (matched_set.find(make_pair(a, b)) != matched_set.end());

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
  void print_backend(const string & backend,
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

    cout << "  " << backend
         << " -> card " << got.cardinality
         << ", weight " << got.total_weight
         << " | " << format_pairs(got.matched_pairs) << "\n";

    if (got.cardinality != reference.cardinality
        or got.total_weight != reference.total_weight)
      {
        cerr << "  Warning: backend objective mismatch\n";
      }
  }


  void run_scenario(const Scenario & s)
  {
    cout << "\n" << s.title << "\n";
    cout << string(s.title.size(), '-') << "\n";

    for (bool max_cardinality : {false, true})
      {
        cout << (max_cardinality
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

        const string suffix = max_cardinality ? "max_cardinality" : "max_weight";
        const string tex_path = "/tmp/weighted_blossom_" + s.slug + "_" + suffix + ".tex";

        write_tikz(s, canonical, max_cardinality, tex_path);
        cout << "  TikZ export: " << tex_path << "\n";
      }
  }

} // namespace


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

  cout << "\nDone. Compile generated .tex files with pdflatex to visualize the matchings.\n";

  return 0;
}
