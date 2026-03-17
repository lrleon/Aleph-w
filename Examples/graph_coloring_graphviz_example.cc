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

/** @file graph_coloring_graphviz_example.cc
 *  @brief Export colored graphs to Graphviz DOT for visual comparison.
 *
 *  This example builds a crown graph `H_5`, colors it with several
 *  algorithms from `Graph_Coloring.H`, and writes one DOT file per
 *  algorithm. The node order is intentionally adversarial for greedy
 *  coloring, so the visual output shows why DSatur and the exact solver
 *  can be much better than a naive first-fit pass.
 *
 *  Generated files:
 *  - `<prefix>_greedy.dot`
 *  - `<prefix>_welsh_powell.dot`
 *  - `<prefix>_dsatur.dot`
 *  - `<prefix>_exact.dot`
 *
 *  Usage:
 *  @code
 *  ./graph_coloring_graphviz_example
 *  ./graph_coloring_graphviz_example /tmp/crown
 *  neato -n -Tsvg /tmp/crown_dsatur.dot -o /tmp/crown_dsatur.svg
 *  @endcode
 *
 *  @ingroup Examples
 */

#include <fstream>
#include <iostream>
#include <string>

#include <ah-errors.H>
#include <Graph_Coloring.H>
#include <tpl_graph.H>

using namespace Aleph;
using namespace std;

using Graph    = List_Graph<Graph_Node<string>, Graph_Arc<Empty_Class>>;
using Node     = Graph::Node;
using ColorMap = DynMapTree<Node *, size_t>;

static constexpr size_t Crown_Size = 5;

static const char *fill_palette[] = {"#f94144", "#277da1", "#90be6d", "#f8961e", "#7b2cbf", "#577590"};

static const char *font_palette[] = {"#ffffff", "#ffffff", "#111111", "#111111", "#ffffff", "#ffffff"};

static const char *palette_names[] = {"coral", "ocean", "sage", "amber", "violet", "slate"};

static const char *left_positions[Crown_Size] = {"0,8!", "0,6!", "0,4!", "0,2!", "0,0!"};

static const char *right_positions[Crown_Size] = {"5,8!", "5,6!", "5,4!", "5,2!", "5,0!"};

static constexpr size_t palette_size()
{
  return sizeof(fill_palette) / sizeof(fill_palette[0]);
}

static string color_name(size_t c)
{
  return palette_names[c % palette_size()];
}

static string color_legend(size_t num_colors)
{
  string legend;

  for (size_t c = 0; c < num_colors; ++c)
    {
      if (not legend.empty())
        legend += ", ";

      legend += "c" + to_string(c) + "=" + color_name(c);
    }

  return legend;
}

static void build_crown_graph(Graph &g, Node *left[Crown_Size], Node *right[Crown_Size])
{
  for (size_t i = 0; i < Crown_Size; ++i)
    {
      left[i]  = g.insert_node("u" + to_string(i));
      right[i] = g.insert_node("v" + to_string(i));
    }

  for (size_t i = 0; i < Crown_Size; ++i)
    for (size_t j = 0; j < Crown_Size; ++j)
      if (i != j)
        g.insert_arc(left[i], right[j]);
}

static void locate_node(Node *node, Node *left[Crown_Size], Node *right[Crown_Size], bool &is_left, size_t &index)
{
  for (size_t i = 0; i < Crown_Size; ++i)
    {
      if (left[i] == node)
        {
          is_left = true;
          index   = i;
          return;
        }

      if (right[i] == node)
        {
          is_left = false;
          index   = i;
          return;
        }
    }

  ah_runtime_error() << "node not found in crown graph export";
}

static void write_colored_node(ostream &out, const string &dot_id, const string &label, const char *pos, size_t color)
{
  const size_t palette_index = color % palette_size();

  out << "  " << dot_id << " [label=\"" << label << "\\nc" << color << "\""
      << ", fillcolor=\"" << fill_palette[palette_index] << "\""
      << ", fontcolor=\"" << font_palette[palette_index] << "\""
      << ", pos=\"" << pos << "\"];\n";
}

static void write_coloring_dot(const string &filename, const string &title, const Graph &g, Node *left[Crown_Size],
                               Node *right[Crown_Size], const ColorMap &colors, size_t num_colors)
{
  ofstream out(filename);
  ah_runtime_error_if(not out) << "cannot open output file " << filename;

  out << "graph CrownColoring {\n"
      << "  layout=neato;\n"
      << "  overlap=false;\n"
      << "  splines=true;\n"
      << "  outputorder=edgesfirst;\n"
      << "  graph [label=\"" << title << "\\n"
      << "Crown graph H_" << Crown_Size << " (K_" << Crown_Size << "," << Crown_Size << " minus a perfect matching)\\n"
      << num_colors << " color(s): " << color_legend(num_colors)
      << "\", labelloc=t, fontsize=20, fontname=\"Helvetica\"];\n"
      << "  node [shape=circle, style=filled, fixedsize=true,\n"
      << "        width=0.95, height=0.95, penwidth=1.8,\n"
      << "        color=\"#1f2937\", fontname=\"Helvetica\"];\n"
      << "  edge [color=\"#94a3b8\", penwidth=1.5];\n"
      << "  left_partition [shape=plaintext, fixedsize=false,"
      << " label=\"Left partition U\", pos=\"0,9.5!\"];\n"
      << "  right_partition [shape=plaintext, fixedsize=false,"
      << " label=\"Right partition V\", pos=\"5,9.5!\"];\n";

  for (size_t i = 0; i < Crown_Size; ++i)
    {
      write_colored_node(out, "u" + to_string(i), left[i]->get_info(), left_positions[i], colors.find(left[i]));
      write_colored_node(out, "v" + to_string(i), right[i]->get_info(), right_positions[i], colors.find(right[i]));
    }

  out << "\n";

  for (Graph::Arc_Iterator it(g); it.has_curr(); it.next_ne())
    {
      Node *src = it.get_src_node();
      Node *tgt = it.get_tgt_node();

      bool   src_is_left = false;
      bool   tgt_is_left = false;
      size_t src_index   = 0;
      size_t tgt_index   = 0;

      locate_node(src, left, right, src_is_left, src_index);
      locate_node(tgt, left, right, tgt_is_left, tgt_index);

      out << "  " << (src_is_left ? "u" : "v") << src_index << " -- " << (tgt_is_left ? "u" : "v") << tgt_index
          << ";\n";
    }

  out << "}\n";
}

template <class Coloring>
static void export_variant(const string &prefix, const string &suffix, const string &title, const Graph &g,
                           Node *left[Crown_Size], Node *right[Crown_Size], Coloring coloring)
{
  ColorMap     colors;
  const size_t num_colors = coloring(g, colors);
  const string filename   = prefix + "_" + suffix + ".dot";

  write_coloring_dot(filename, title, g, left, right, colors, num_colors);

  cout << "  " << suffix << ": " << num_colors << " color(s) -> " << filename << '\n';
}

int main(int argc, char **argv)
try
  {
    const string prefix = argc > 1 ? argv[1] : "graph_coloring_crown";

    Graph g;
    Node *left[Crown_Size];
    Node *right[Crown_Size];
    build_crown_graph(g, left, right);

    cout << "Graph coloring Graphviz example\n";
    cout << "==============================\n";
    cout << "Graph: crown H_" << Crown_Size << " with " << g.get_num_nodes() << " nodes and " << g.get_num_arcs()
         << " edges\n";
    cout << "Node order is adversarial for greedy coloring: "
         << "u0, v0, u1, v1, ...\n\n";

    export_variant(prefix, "greedy", "Greedy coloring", g, left, right,
                   [](const Graph &graph, ColorMap &colors) { return greedy_coloring(graph, colors); });

    export_variant(prefix, "welsh_powell", "Welsh-Powell coloring", g, left, right,
                   [](const Graph &graph, ColorMap &colors) { return welsh_powell_coloring(graph, colors); });

    export_variant(prefix, "dsatur", "DSatur coloring", g, left, right,
                   [](const Graph &graph, ColorMap &colors) { return dsatur_coloring(graph, colors); });

    export_variant(prefix, "exact", "Exact chromatic number", g, left, right,
                   [](const Graph &graph, ColorMap &colors) { return chromatic_number(graph, colors); });

    cout << "\nRender any DOT file with Graphviz, for example:\n";
    cout << "  neato -n -Tsvg " << prefix << "_dsatur.dot -o " << prefix << "_dsatur.svg\n";
    cout << "  neato -n -Tpng " << prefix << "_exact.dot -o " << prefix << "_exact.png\n";

    return 0;
  }
catch (const exception &e)
  {
    cerr << "Error: " << e.what() << '\n';
    return 1;
  }
