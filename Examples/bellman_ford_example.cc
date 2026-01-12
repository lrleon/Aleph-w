/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file bellman_ford_example.cc
 * @brief Comprehensive example of Bellman-Ford shortest path algorithm.
 *
 * This example demonstrates the Bellman-Ford algorithm for finding shortest
 * paths from a single source in graphs with potentially negative edge weights.
 *
 * ## Key Features Demonstrated
 *
 * 1. **Handling Negative Weights**: Unlike Dijkstra, Bellman-Ford works correctly
 *    with negative edge weights.
 *
 * 2. **Negative Cycle Detection**: The algorithm can detect and report negative
 *    weight cycles, which would make shortest paths undefined.
 *
 * 3. **SPFA Optimization**: Shows the faster queue-based variant (Shortest Path
 *    Faster Algorithm).
 *
 * ## Algorithm Overview
 *
 * ```
 * Bellman-Ford(G, s):
 *   1. Initialize: d[s] = 0, d[v] = ∞ for all other v
 *   2. Repeat |V| - 1 times:
 *      For each edge (u, v) with weight w:
 *        If d[u] + w < d[v]:
 *          d[v] = d[u] + w
 *          predecessor[v] = u
 *   3. Check for negative cycles:
 *      For each edge (u, v) with weight w:
 *        If d[u] + w < d[v]:
 *          Report negative cycle
 * ```
 *
 * ## Complexity
 *
 * - Time: O(V × E) standard, O(E) average for SPFA
 * - Space: O(V)
 *
 * ## When to Use
 *
 * | Scenario | Algorithm |
 * |----------|-----------|
 * | Non-negative weights only | Dijkstra (faster) |
 * | Negative weights, no negative cycles | Bellman-Ford |
 * | Need to detect negative cycles | Bellman-Ford |
 * | All-pairs with negative weights | Johnson (uses B-F) |
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>

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

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
  cout << "║          Bellman-Ford Algorithm - Comprehensive Example              ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

  example_basic_negative_weights();
  example_negative_cycle();
  example_spfa_comparison();
  example_comparison_dijkstra();

  cout << "\nDone.\n";
  return 0;
}
