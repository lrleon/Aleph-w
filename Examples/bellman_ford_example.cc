
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
 * @file bellman_ford_example.cc
 * @brief Bellman-Ford shortest paths in Aleph-w (negative weights, negative-cycle detection, SPFA).
 *
 * ## Overview
 *
 * This example demonstrates Aleph-w's Bellman-Ford implementation for
 * single-source shortest paths on directed graphs that may contain
 * **negative arc weights**. It also demonstrates:
 *
 * - **Negative-cycle detection** (reachable from the source).
 * - A queue-based relaxation strategy often referred to as **SPFA**
 *   (Shortest Path Faster Algorithm).
 *
 * Bellman-Ford is the “safe default” when you cannot guarantee non-negative
 * weights, and it is a key building block for Johnson's all-pairs algorithm.
 *
 * ## Data model used by this example
 *
 * - **Graph type**: `WeightedDigraph = List_Digraph<Graph_Node<string>, Graph_Arc<double>>`
 * - **Node info**: label/name (`string`)
 * - **Arc info**: weight/cost (`double`)
 *
 * ## Usage
 *
 * ```bash
 * # Run the full demo suite (default)
 * ./bellman_ford_example
 *
 * # Run the negative-cycle demo
 * ./bellman_ford_example --negative-cycles
 *
 * # Run the SPFA comparison demo
 * ./bellman_ford_example --spfa
 *
 * # Show help
 * ./bellman_ford_example --help
 * ```
 *
 * If no flags are given, or if you pass no “specific” flags, the program runs
 * all demos.
 *
 * ## Algorithms
 *
 * ### Standard Bellman-Ford
 *
 * Bellman-Ford repeatedly relaxes all edges. In a graph with no negative cycles
 * reachable from the source, shortest paths have at most `|V|-1` edges, so
 * `|V|-1` relaxation rounds suffice.
 *
 * ### Negative-cycle detection
 *
 * After the `|V|-1` rounds, if any edge can still be relaxed, there exists a
 * negative cycle **reachable from the source**, and shortest paths are not
 * well-defined (cost can be decreased indefinitely by looping).
 *
 * ### SPFA (queue-based relaxation)
 *
 * The example also shows a queue-driven approach that only relaxes outgoing
 * edges of nodes whose distance changed. This is often faster in practice,
 * but retains Bellman-Ford's worst-case behavior.
 *
 * ## Complexity
 *
 * Let **V** be the number of nodes and **E** the number of arcs.
 *
 * - **Standard Bellman-Ford**: `O(V * E)` time, `O(V)` extra space.
 * - **SPFA (typical)**: often close to `O(E)` on many inputs (no guarantee).
 * - **SPFA (worst case)**: `O(V * E)`.
 *
 * ## Pitfalls and edge cases
 *
 * - **Dijkstra incompatibility**: Dijkstra is invalid if any arc has negative weight.
 * - **Cycle reachability**: a negative cycle that is not reachable from the chosen
 *   source does not affect shortest paths from that source.
 * - **Floating point**: with `double` weights, comparisons can be sensitive to
 *   rounding; be careful if you adapt this to real-world numeric data.
 * - **Unreachable nodes**: distances remain infinite; paths to those nodes are empty.
 *
 * ## References / see also
 *
 * - `Bellman_Ford.H` (implementation)
 * - `dijkstra_example.cc` / `Dijkstra.H` (faster when all weights are non-negative)
 * - `johnson_example.cc` (all-pairs shortest paths using Bellman-Ford + Dijkstra)
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>

#include <tpl_graph.H>
#include <Bellman_Ford.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Graph Type Definitions
// =============================================================================

using WeightedDigraph = List_Digraph<Graph_Node<string>, Graph_Arc<double>>;
using Node = WeightedDigraph::Node;
using Arc = WeightedDigraph::Arc;

/// Distance accessor
struct Distance
{
  using Distance_Type = double;

  static void set_zero(Arc* a) { a->get_info() = 0.0; }

  Distance_Type operator()(Arc* a) const { return a->get_info(); }
};

// =============================================================================
// Utility Functions
// =============================================================================

Node* find_node(WeightedDigraph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

void print_graph(WeightedDigraph& g)
{
  cout << "Graph structure:\n";
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
    {
      auto* node = nit.get_curr();
      cout << "  " << node->get_info() << " → ";
      bool first = true;
      for (auto ait = g.get_out_it(node); ait.has_curr(); ait.next())
        {
          auto* arc = ait.get_curr();
          auto* tgt = g.get_tgt_node(arc);
          if (!first) cout << ", ";
          cout << tgt->get_info() << "(" << arc->get_info() << ")";
          first = false;
        }
      cout << "\n";
    }
}

void print_path(Path<WeightedDigraph>& path, WeightedDigraph& g)
{
  (void)g;
  if (path.size() == 0)
    {
      cout << "(no path)\n";
      return;
    }

  double total = 0;
  bool first = true;
  path.for_each_node([&first](Node* n) {
    if (!first) cout << " → ";
    cout << n->get_info();
    first = false;
  });
  cout << "\n  Edges: ";

  first = true;
  path.for_each_arc([&](Arc* a) {
    if (!first) cout << " + ";
    cout << a->get_info();
    total += a->get_info();
    first = false;
  });
  cout << " = " << total << "\n";
}

template <typename Func>
double measure_ms(Func&& f)
{
  auto start = chrono::high_resolution_clock::now();
  f();
  auto end = chrono::high_resolution_clock::now();
  return chrono::duration<double, milli>(end - start).count();
}

// =============================================================================
// Example 1: Basic Shortest Paths (with negative weights)
// =============================================================================

void example_basic_negative_weights()
{
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 1: Shortest Paths with Negative Weights\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  /*
   * Build a graph where the shortest path requires taking a negative edge:
   *
   *       A ──(4)──→ B ──(3)──→ E
   *       │         │↗
   *       │       (-2)
   *      (2)        │
   *       │         ↓
   *       └───────→ C ──(1)──→ D
   *
   * Shortest path A→E: A→C→B→E (cost = 2 + (-2) + 3 = 3)
   * NOT A→B→E (cost = 4 + 3 = 7)
   */

  WeightedDigraph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");
  auto* E = g.insert_node("E");

  g.insert_arc(A, B, 4.0);
  g.insert_arc(A, C, 2.0);
  g.insert_arc(B, E, 3.0);
  g.insert_arc(C, B, -2.0);   // Negative weight!
  g.insert_arc(C, D, 1.0);

  cout << "Graph with negative edge C→B (-2):\n\n";
  print_graph(g);

  cout << "\n▶ Running Bellman-Ford from A:\n\n";

  Bellman_Ford<WeightedDigraph, Distance> bf(g);
  bool has_negative_cycle = bf.paint_spanning_tree(A);

  if (has_negative_cycle)
    {
      cout << "  ERROR: Negative cycle detected!\n";
      return;
    }

  cout << "  No negative cycle detected.\n\n";

  // Get distances and paths to all nodes
  cout << "  Distances from A:\n";
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    {
      auto* node = it.get_curr();
      double dist = bf.get_distance(node);
      cout << "    A → " << node->get_info() << ": ";

      if (dist == numeric_limits<double>::max())
        cout << "∞ (unreachable)\n";
      else
        cout << dist << "\n";
    }

  // Show specific path A→E
  cout << "\n  Path A → E:\n    ";
  Path<WeightedDigraph> path(g);
  bf.get_min_path(E, path);
  print_path(path, g);

  cout << "\n  Note: Dijkstra would fail here because of the negative edge!\n";
}

// =============================================================================
// Example 2: Negative Cycle Detection
// =============================================================================

void example_negative_cycle()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 2: Negative Cycle Detection\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  /*
   * Build a graph with a negative cycle:
   *
   *       A ──(1)──→ B
   *       ↑         │
   *       │        (2)
   *      (-5)       │
   *       │         ↓
   *       └───(1)── C
   *
   * Cycle B→C→A→B has weight: 2 + (-5) + 1 = -2
   * This is a negative cycle - shortest paths are undefined!
   */

  WeightedDigraph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");

  g.insert_arc(A, B, 1.0);
  g.insert_arc(B, C, 2.0);
  g.insert_arc(C, A, -5.0);   // Creates negative cycle!

  cout << "Graph with negative cycle (A→B→C→A has weight -2):\n\n";
  print_graph(g);

  cout << "\n▶ Running Bellman-Ford from A:\n\n";

  Bellman_Ford<WeightedDigraph, Distance> bf(g);
  bool has_negative_cycle = bf.paint_spanning_tree(A);

  if (has_negative_cycle)
    {
      cout << "  ⚠ NEGATIVE CYCLE DETECTED!\n\n";
      cout << "  When a negative cycle exists, shortest paths are undefined\n";
      cout << "  because you can always go around the cycle to decrease the cost.\n";

      // Build and print the negative cycle
      Path<WeightedDigraph> cycle = bf.test_negative_cycle();

      cout << "\n  Negative cycle: ";
      cycle.for_each_node([](Node* n) {
        cout << n->get_info() << " → ";
      });
      cout << "...\n";

      double cycle_weight = 0;
      cycle.for_each_arc([&cycle_weight](Arc* a) {
        cycle_weight += a->get_info();
      });
      cout << "  Cycle weight: " << cycle_weight << "\n";
    }
  else
    {
      cout << "  No negative cycle detected.\n";
    }
}

// =============================================================================
// Example 3: SPFA (Faster Variant)
// =============================================================================

void example_spfa_comparison()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 3: Standard vs SPFA (Faster) Variant\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "SPFA (Shortest Path Faster Algorithm) is a queue-based optimization\n";
  cout << "of Bellman-Ford. It only relaxes edges from nodes whose distances\n";
  cout << "have been updated, which is often much faster in practice.\n\n";

  // Build a larger graph for meaningful comparison
  WeightedDigraph g;
  const int N = 100;
  vector<Node*> nodes;

  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node("N" + to_string(i)));

  // Create a sparse graph with some negative edges
  for (int i = 0; i < N; ++i)
    {
      // Forward edges (mostly positive)
      if (i + 1 < N) g.insert_arc(nodes[i], nodes[i + 1], 1.0);
      if (i + 2 < N) g.insert_arc(nodes[i], nodes[i + 2], 3.0);
      if (i + 5 < N) g.insert_arc(nodes[i], nodes[i + 5], 4.0);

      // Some negative edges (backwards, but not creating cycles)
      if (i > 0 && i % 10 == 0)
        g.insert_arc(nodes[i], nodes[i - 1], -0.5);
    }

  cout << "Graph: " << g.get_num_nodes() << " nodes, " << g.get_num_arcs() << " arcs\n\n";

  // Standard Bellman-Ford
  double time_standard;
  {
    Bellman_Ford<WeightedDigraph, Distance> bf(g);
    time_standard = measure_ms([&]() {
      bf.paint_spanning_tree(nodes[0]);
    });
    cout << "▶ Standard Bellman-Ford: " << fixed << setprecision(3)
         << time_standard << " ms\n";
  }

  // SPFA variant
  double time_spfa;
  {
    Bellman_Ford<WeightedDigraph, Distance> bf(g);
    time_spfa = measure_ms([&]() {
      bf.faster_paint_spanning_tree(nodes[0]);
    });
    cout << "▶ SPFA (Faster) variant: " << fixed << setprecision(3)
         << time_spfa << " ms\n";
  }

  double speedup = time_standard / time_spfa;
  cout << "\n  Speedup: " << fixed << setprecision(1) << speedup << "x\n";
  cout << "\n  Note: SPFA is usually faster on sparse graphs, but has the\n";
  cout << "  same worst-case complexity O(V*E) as standard Bellman-Ford.\n";
}

// =============================================================================
// Example 4: Comparison with Dijkstra
// =============================================================================

void example_comparison_dijkstra()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 4: When to Use Bellman-Ford vs Dijkstra\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
┌──────────────────────────────────────────────────────────────────────┐
│                    Algorithm Selection Guide                         │
├──────────────────────────────────────────────────────────────────────┤
│ Criterion              │ Dijkstra        │ Bellman-Ford             │
├────────────────────────┼─────────────────┼──────────────────────────┤
│ Edge weights           │ Non-negative    │ Any (incl. negative)     │
│ Time complexity        │ O((V+E) log V)  │ O(V × E)                 │
│ Negative cycle detect  │ No              │ Yes                      │
│ Best for               │ Road networks   │ Currency exchange,       │
│                        │ GPS routing     │ game AI, financial       │
├────────────────────────┴─────────────────┴──────────────────────────┤
│ Use Bellman-Ford when:                                               │
│   • Graph has negative edge weights                                  │
│   • Need to detect negative cycles                                   │
│   • Correctness more important than speed                            │
│                                                                      │
│ Use Dijkstra when:                                                   │
│   • All edges are non-negative                                       │
│   • Performance is critical                                          │
│   • Working with large road/network graphs                           │
└──────────────────────────────────────────────────────────────────────┘
)";
}

// =============================================================================
// Main
// =============================================================================

static void usage(const char* prog)
{
  cout << "Usage: " << prog << " [--negative-cycles] [--spfa] [--help]\n";
  cout << "\nIf no flags are given, all demos are executed.\n";
}

static bool has_flag(int argc, char* argv[], const char* flag)
{
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], flag) == 0)
      return true;
  return false;
}

int main(int argc, char* argv[])
{
  cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
  cout << "║          Bellman-Ford Algorithm - Comprehensive Example              ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

  if (has_flag(argc, argv, "--help"))
    {
      usage(argv[0]);
      return 0;
    }

  const bool any_specific =
    has_flag(argc, argv, "--negative-cycles") ||
    has_flag(argc, argv, "--spfa");

  const bool run_all = (argc == 1) || !any_specific;

  if (run_all)
    {
      example_basic_negative_weights();
      example_negative_cycle();
      example_spfa_comparison();
      example_comparison_dijkstra();
    }
  else
    {
      // Keep the basic example as context for specialized runs.
      example_basic_negative_weights();

      if (has_flag(argc, argv, "--negative-cycles"))
        example_negative_cycle();

      if (has_flag(argc, argv, "--spfa"))
        example_spfa_comparison();
    }

  cout << "\nDone.\n";
  return 0;
}
