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
 * @file graph_coloring_example.cc
 * @brief Graph coloring algorithms in Aleph-w: greedy, Welsh-Powell,
 *        DSatur, and exact chromatic number.
 *
 * ## Overview
 *
 * Graph coloring assigns a color to each vertex such that no two adjacent
 * vertices share the same color. The minimum number of colors needed is the
 * **chromatic number** χ(G).
 *
 * This example demonstrates Aleph-w's `Graph_Coloring.H` module on four
 * progressively richer scenarios:
 *
 * 1. **Map coloring** — South American countries as an adjacency graph.
 * 2. **Exam scheduling** — courses with shared students cannot overlap.
 * 3. **Register allocation** — variables with overlapping lifetimes need
 *    different CPU registers.
 * 4. **Algorithm comparison** — Greedy vs Welsh-Powell vs DSatur on a
 *    classic benchmark graph (Petersen).
 *
 * ## Algorithms provided by `Graph_Coloring.H`
 *
 * | Function                | Strategy                                   | Complexity        |
 * |-------------------------|--------------------------------------------|-------------------|
 * | `greedy_coloring`       | First-fit in iteration order               | O((V+E) log V)    |
 * | `welsh_powell_coloring` | First-fit, decreasing degree order         | O((V+E) log V)    |
 * | `dsatur_coloring`       | Most-saturated vertex first (adaptive)     | O(V^2 + E log V)  |
 * | `is_valid_coloring`     | Validation                                 | O(E log V)        |
 * | `chromatic_number`      | Exact (branch-and-bound, max 64 nodes)     | Exponential       |
 *
 * ## State safety
 *
 * Coloring algorithms temporarily reuse `NODE_COOKIE` internally while
 * running, and restore any pre-existing node cookies on exit. However, they
 * are not thread-safe or reentrant with respect to cookie usage: concurrent
 * executions, nested cookie users, or other simultaneous cookie-based
 * algorithms require external synchronization.
 *
 * ## Usage
 *
 * ```bash
 * ./graph_coloring_example
 * ```
 *
 * No command-line options; all scenarios are hard-coded.
 * For Graphviz output, see `graph_coloring_graphviz_example`.
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <iomanip>
# include <string>
# include <cstdio>

# include <Graph_Coloring.H>
# include <tpl_graph.H>

using namespace Aleph;
using namespace std;

// ============================================================================
// Graph type used throughout this example
// ============================================================================

using G = List_Graph<Graph_Node<string>, Graph_Arc<Empty_Class>>;
using Node = G::Node;
using ColorMap = DynMapTree<Node *, size_t>;

// ============================================================================
// Display helpers
// ============================================================================

static const char * color_names[] = {
  "Red", "Blue", "Green", "Yellow", "Orange",
  "Purple", "Cyan", "Magenta", "Brown", "Gray"
};

static const char * color_name(size_t c)
{
  constexpr size_t N = sizeof(color_names) / sizeof(color_names[0]);
  if (c < N) return color_names[c];
  static char buf[32];
  std::snprintf(buf, sizeof(buf), "Color%zu", c);
  return buf;
}

static void print_coloring(const G & g, const ColorMap & colors,
                            size_t num_colors)
{
  cout << "  Colors used: " << num_colors << "\n";
  cout << "  Assignment:\n";
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *v = it.get_curr();
      size_t c = colors.find(v);
      cout << "    " << left << setw(20) << v->get_info()
           << " -> " << color_name(c) << " (" << c << ")\n";
    }
  cout << "  Valid: " << (is_valid_coloring(g, colors) ? "YES" : "NO") << "\n";
}

static void separator(const string & title)
{
  cout << "\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << title << "\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

// ============================================================================
// Part 1: Map coloring — South America
// ============================================================================

/**
 * Demonstrates graph coloring as a map-coloring problem.
 *
 * Each node is a country; each edge means the two countries share a border.
 * The Four Color Theorem guarantees that every planar map needs at most 4 colors.
 */
static void demo_map_coloring()
{
  separator("Part 1: Map Coloring — South America");

  cout << "Each node is a country; edges represent shared borders.\n";
  cout << "Goal: color the map so no two adjacent countries share a color.\n";
  cout << "The Four Color Theorem guarantees χ(G) ≤ 4 for any planar map.\n\n";

  G g;
  auto *br = g.insert_node("Brazil");
  auto *ar = g.insert_node("Argentina");
  auto *uy = g.insert_node("Uruguay");
  auto *py = g.insert_node("Paraguay");
  auto *bo = g.insert_node("Bolivia");
  auto *pe = g.insert_node("Peru");
  auto *cl = g.insert_node("Chile");
  auto *ec = g.insert_node("Ecuador");
  auto *co = g.insert_node("Colombia");
  auto *ve = g.insert_node("Venezuela");
  auto *gy = g.insert_node("Guyana");
  auto *sr = g.insert_node("Suriname");
  auto *gf = g.insert_node("Fr. Guiana");

  // Borders (undirected)
  g.insert_arc(br, uy);  g.insert_arc(br, ar);
  g.insert_arc(br, py);  g.insert_arc(br, bo);
  g.insert_arc(br, pe);  g.insert_arc(br, co);
  g.insert_arc(br, ve);  g.insert_arc(br, gy);
  g.insert_arc(br, sr);  g.insert_arc(br, gf);
  g.insert_arc(ar, uy);  g.insert_arc(ar, py);
  g.insert_arc(ar, bo);  g.insert_arc(ar, cl);
  g.insert_arc(py, bo);  g.insert_arc(bo, pe);
  g.insert_arc(bo, cl);  g.insert_arc(pe, cl);
  g.insert_arc(pe, ec);  g.insert_arc(pe, co);
  g.insert_arc(ec, co);  g.insert_arc(co, ve);
  g.insert_arc(ve, gy);  g.insert_arc(gy, sr);
  g.insert_arc(sr, gf);

  cout << "Graph: " << g.get_num_nodes() << " countries, "
       << g.get_num_arcs() << " borders.\n\n";

  ColorMap colors;

  cout << "▶ DSatur (best heuristic):\n";
  size_t k = dsatur_coloring(g, colors);
  print_coloring(g, colors, k);
}

// ============================================================================
// Part 2: Exam scheduling
// ============================================================================

/**
 * Models exam scheduling as a graph coloring problem.
 *
 * - Nodes  = courses
 * - Edges  = two courses share at least one student (cannot be scheduled at
 *             the same time)
 * - Colors = exam time slots
 *
 * Minimum coloring = fewest time slots needed.
 */
static void demo_exam_scheduling()
{
  separator("Part 2: Exam Scheduling");

  cout << "Courses that share students cannot be scheduled at the same time.\n";
  cout << "Each color represents an exam time slot.\n\n";

  G g;
  auto *math  = g.insert_node("Mathematics");
  auto *phys  = g.insert_node("Physics");
  auto *cs    = g.insert_node("Comp. Science");
  auto *chem  = g.insert_node("Chemistry");
  auto *bio   = g.insert_node("Biology");
  auto *eng   = g.insert_node("English");
  auto *hist  = g.insert_node("History");
  auto *econ  = g.insert_node("Economics");

  // Conflicts (shared students)
  g.insert_arc(math, phys);   // Math & Physics share students
  g.insert_arc(math, cs);     // Math & CS share students
  g.insert_arc(math, econ);
  g.insert_arc(phys, cs);     // Physics & CS share students
  g.insert_arc(phys, chem);
  g.insert_arc(cs, econ);
  g.insert_arc(chem, bio);
  g.insert_arc(bio, eng);
  g.insert_arc(eng, hist);
  g.insert_arc(hist, econ);

  cout << "Courses: " << g.get_num_nodes()
       << ", Conflicts: " << g.get_num_arcs() << "\n\n";

  ColorMap colors;

  cout << "▶ Greedy (fast, may not be optimal):\n";
  size_t k1 = greedy_coloring(g, colors);
  print_coloring(g, colors, k1);

  cout << "\n▶ Welsh-Powell (sort by degree first):\n";
  size_t k2 = welsh_powell_coloring(g, colors);
  print_coloring(g, colors, k2);

  cout << "\n▶ DSatur (adaptive, usually optimal):\n";
  size_t k3 = dsatur_coloring(g, colors);
  print_coloring(g, colors, k3);

  cout << "\n▶ Exact chromatic number:\n";
  size_t chi = chromatic_number(g, colors);
  cout << "  χ(G) = " << chi << " time slot(s) minimum\n";
  print_coloring(g, colors, chi);
}

// ============================================================================
// Part 3: Register allocation
// ============================================================================

/**
 * Models register allocation as graph coloring.
 *
 * In compiler register allocation:
 * - Nodes  = program variables (live ranges)
 * - Edges  = two variables are simultaneously live (cannot share a register)
 * - Colors = CPU registers
 *
 * If the chromatic number ≤ number of available registers, no spilling is needed.
 */
static void demo_register_allocation()
{
  separator("Part 3: Register Allocation (Compiler Optimization)");

  cout << "Variables that are simultaneously live cannot share a register.\n";
  cout << "Colors represent CPU registers (R0, R1, R2, ...).\n\n";

  // Simulate interference graph from a small function body
  //
  //   a = ...
  //   b = a + 1     (a and b live simultaneously)
  //   c = a * 2     (a, b, c live simultaneously)
  //   d = b + c     (b, c, d live simultaneously)
  //   e = d - 1     (d, e live simultaneously)
  //   return e
  //
  G g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  auto *c = g.insert_node("c");
  auto *d = g.insert_node("d");
  auto *e = g.insert_node("e");

  // Interference edges (simultaneously live)
  g.insert_arc(a, b);  // a and b overlap
  g.insert_arc(a, c);  // a and c overlap
  g.insert_arc(b, c);  // b and c overlap
  g.insert_arc(b, d);  // b and d overlap
  g.insert_arc(c, d);  // c and d overlap
  g.insert_arc(d, e);  // d and e overlap

  cout << "Variables: " << g.get_num_nodes()
       << ", Interferences: " << g.get_num_arcs() << "\n\n";

  ColorMap colors;

  cout << "▶ DSatur coloring (minimum registers):\n";
  size_t k = dsatur_coloring(g, colors);
  cout << "  Registers needed: " << k << "\n";
  cout << "  Assignment:\n";
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    {
      Node *v = it.get_curr();
      cout << "    " << left << setw(4) << v->get_info()
           << " -> R" << colors.find(v) << "\n";
    }
  cout << "  Valid (no interference): "
       << (is_valid_coloring(g, colors) ? "YES" : "NO") << "\n";

  cout << "\n▶ Exact chromatic number:\n";
  size_t chi = chromatic_number(g, colors);
  cout << "  Minimum registers needed: " << chi << "\n";

  if (chi <= 3)
    cout << "  ✓ Fits in 3 registers — no spilling required!\n";
  else
    cout << "  ✗ Needs more than 3 registers — some variables must be spilled.\n";
}

// ============================================================================
// Part 4: Algorithm comparison on the Petersen graph
// ============================================================================

/**
 * The Petersen graph is a well-known benchmark in graph theory.
 * It is 3-regular, vertex-transitive, and has chromatic number 3.
 *
 * It is a hard case for greedy algorithms because the result depends on
 * the vertex order.
 */
static void demo_algorithm_comparison()
{
  separator("Part 4: Algorithm Comparison — Petersen Graph");

  cout << "The Petersen graph is 3-regular and has χ(G) = 3.\n";
  cout << "It is a well-known benchmark for coloring heuristics.\n\n";

  G g;
  // Outer ring: 0..4, inner pentagram: 5..9
  Node *nodes[10];
  for (int i = 0; i < 10; ++i)
    nodes[i] = g.insert_node("v" + to_string(i));

  // Outer cycle: 0-1-2-3-4-0
  for (int i = 0; i < 5; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % 5]);

  // Inner pentagram (5-connected skipping one): 5-7-9-6-8-5
  g.insert_arc(nodes[5], nodes[7]);
  g.insert_arc(nodes[7], nodes[9]);
  g.insert_arc(nodes[9], nodes[6]);
  g.insert_arc(nodes[6], nodes[8]);
  g.insert_arc(nodes[8], nodes[5]);

  // Spokes: i -- i+5
  for (int i = 0; i < 5; ++i)
    g.insert_arc(nodes[i], nodes[i + 5]);

  cout << "Graph: " << g.get_num_nodes() << " nodes, "
       << g.get_num_arcs() << " edges (3-regular).\n\n";

  ColorMap colors;

  cout << "▶ Greedy (order-dependent):\n";
  size_t k1 = greedy_coloring(g, colors);
  cout << "  Colors: " << k1 << "  Valid: "
       << (is_valid_coloring(g, colors) ? "yes" : "no") << "\n";

  cout << "\n▶ Welsh-Powell (decreasing degree):\n";
  size_t k2 = welsh_powell_coloring(g, colors);
  cout << "  Colors: " << k2 << "  Valid: "
       << (is_valid_coloring(g, colors) ? "yes" : "no") << "\n";

  cout << "\n▶ DSatur (adaptive saturation):\n";
  size_t k3 = dsatur_coloring(g, colors);
  cout << "  Colors: " << k3 << "  Valid: "
       << (is_valid_coloring(g, colors) ? "yes" : "no") << "\n";
  print_coloring(g, colors, k3);

  cout << "\n▶ Exact chromatic number (branch-and-bound):\n";
  size_t chi = chromatic_number(g, colors);
  cout << "  χ(Petersen) = " << chi << "\n";
  cout << "  Valid: " << (is_valid_coloring(g, colors) ? "yes" : "no") << "\n";

  cout << "\n  Summary:\n";
  cout << "    Greedy:        " << k1 << " color(s)\n";
  cout << "    Welsh-Powell:  " << k2 << " color(s)\n";
  cout << "    DSatur:        " << k3 << " color(s)\n";
  cout << "    Exact (χ):     " << chi << " color(s)  ← optimal\n";
}

// ============================================================================
// Part 5: State safety demonstration
// ============================================================================

/**
 * Shows that running one coloring algorithm does not alter cookies that
 * were set previously by user code.
 */
static void demo_cookie_safety()
{
  separator("Part 5: State Safety — Cookies Are Restored on Completion");

  cout << "This implementation temporarily reuses NODE_COOKIE for coloring state,\n";
  cout << "but restores any pre-existing cookie values when it finishes.\n";
  cout << "It is not thread-safe, reentrant, or safe for nested/concurrent\n";
  cout << "cookie users without external synchronization.\n\n";

  G g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  auto *c = g.insert_node("c");
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, a);

  // Store sentinel values in cookies (simulating another algorithm's data)
  int sentinel_a = 100, sentinel_b = 200, sentinel_c = 300;
  NODE_COOKIE(a) = &sentinel_a;
  NODE_COOKIE(b) = &sentinel_b;
  NODE_COOKIE(c) = &sentinel_c;

  cout << "Before coloring:\n";
  cout << "  cookie(a) = " << NODE_COOKIE(a) << " (expected " << &sentinel_a << ")\n";
  cout << "  cookie(b) = " << NODE_COOKIE(b) << " (expected " << &sentinel_b << ")\n";
  cout << "  cookie(c) = " << NODE_COOKIE(c) << " (expected " << &sentinel_c << ")\n\n";

  ColorMap colors;
  size_t k = dsatur_coloring(g, colors);
  cout << "After dsatur_coloring (" << k << " colors):\n";
  cout << "  cookie(a) = " << NODE_COOKIE(a)
       << (NODE_COOKIE(a) == &sentinel_a ? "  ✓ preserved" : "  ✗ corrupted!") << "\n";
  cout << "  cookie(b) = " << NODE_COOKIE(b)
       << (NODE_COOKIE(b) == &sentinel_b ? "  ✓ preserved" : "  ✗ corrupted!") << "\n";
  cout << "  cookie(c) = " << NODE_COOKIE(c)
       << (NODE_COOKIE(c) == &sentinel_c ? "  ✓ preserved" : "  ✗ corrupted!") << "\n";
  cout << "  Coloring valid: " << (is_valid_coloring(g, colors) ? "yes" : "no") << "\n";
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║           Graph Coloring Algorithms — Aleph-w Example            ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  demo_map_coloring();
  demo_exam_scheduling();
  demo_register_allocation();
  demo_algorithm_comparison();
  demo_cookie_safety();

  cout << "\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Summary of algorithms in Graph_Coloring.H\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
  cout << "┌─────────────────────────┬──────────────────────┬─────────────────┐\n";
  cout << "│ Function                │ Complexity           │ Quality         │\n";
  cout << "├─────────────────────────┼──────────────────────┼─────────────────┤\n";
  cout << "│ greedy_coloring         │ O((V+E) log V)       │ ≤ Δ+1 colors   │\n";
  cout << "│ welsh_powell_coloring   │ O((V+E) log V)       │ Often better   │\n";
  cout << "│ dsatur_coloring         │ O(V^2 + E log V)     │ Near-optimal   │\n";
  cout << "│ is_valid_coloring       │ O(E log V)           │ Validation     │\n";
  cout << "│ chromatic_number        │ Exponential (≤64 V)  │ Exact χ(G)     │\n";
  cout << "└─────────────────────────┴──────────────────────┴─────────────────┘\n\n";
  cout << "All algorithms:\n";
  cout << "  • Return colors as DynMapTree<Node*, size_t> (0-based indices)\n";
  cout << "  • Temporarily reuse NODE_COOKIE, then restore prior values on completion\n";
  cout << "  • Are not thread-safe/reentrant for concurrent or nested cookie users\n";
  cout << "  • Work on List_Graph, List_SGraph, Array_Graph and digraph variants\n";
  cout << "  • Accept any arc filter via template parameter SA\n\n";

  return 0;
}
