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
 * @file dijkstra_example.cc
 * @brief Dijkstra shortest paths in Aleph-w (single path, shortest-path tree, and heap trade-offs).
 *
 * ## Overview
 *
 * This example demonstrates how to compute shortest paths with Aleph-w's
 * `Dijkstra_Min_Paths` on a weighted graph with **non-negative** arc weights.
 * It focuses on two common usage modes:
 *
 * - **Single-destination query**: compute one shortest path from a source to a destination.
 * - **Many queries from one source**: compute a shortest-paths tree once and then query
 *   multiple destinations efficiently.
 *
 * It also compares two priority-queue backends used internally by Dijkstra:
 * `ArcHeap` (binary heap) vs `ArcFibonacciHeap`.
 *
 * ## Data model used by this example
 *
 * - **Graph type**: `CityGraph = List_Digraph<Graph_Node<string>, Graph_Arc<double>>`
 * - **Node info**: city name (`string`)
 * - **Arc info**: distance in km (`double`)
 *
 * Note: The demo builds *bidirectional roads* by inserting arcs in both directions,
 * even though the container type is a directed graph.
 *
 * ## Usage
 *
 * ```bash
 * ./dijkstra_example
 * ```
 *
 * This example has no command-line options; all parameters (graph sizes, densities)
 * are hard-coded.
 *
 * ## Algorithms and Aleph-w API
 *
 * - **Single shortest path**:
 *   - `find_min_path(g, src, dst, path)` computes a shortest path and writes it into `path`.
 * - **Shortest-paths tree (many queries)**:
 *   - `compute_min_paths_tree(g, src, tree)` builds an explicit shortest-path tree graph.
 *   - `paint_min_paths_tree(g, src)` marks the original graph so you can query later.
 *   - `get_min_path(tree, dst, path)` extracts the path to `dst` from a previously built tree.
 *   - `get_min_path(dst, path)` extracts the path to `dst` after `paint_min_paths_tree()`.
 *
 * ## Complexity
 *
 * Let **V** be the number of nodes and **E** the number of arcs.
 *
 * - **Binary heap (`ArcHeap`)**: `O((V + E) log V)`
 * - **Fibonacci heap (`ArcFibonacciHeap`)**: `O(E + V log V)` (amortized)
 *
 * Notes:
 * - Fibonacci heaps can win on *dense* graphs due to cheaper decrease-key operations,
 *   but have higher constant factors.
 * - In practice, binary heaps are often the default choice.
 *
 * ## Pitfalls and edge cases
 *
 * - **Negative weights**: Dijkstra is invalid if any arc weight is negative.
 * - **Disconnected graphs**: unreachable nodes will not appear in the computed tree.
 * - **Source == destination**: this example documents Aleph-w's observed behavior where
 *   `find_min_path(g, s, s, path)` may return `Inf` and an empty path; handle the trivial
 *   case explicitly if you need distance 0 and path `[s]`.
 * - **Directed vs undirected modeling**: for undirected graphs you must insert both
 *   directions, or use an undirected graph container.
 *
 * ## References / see also
 *
 * - `Dijkstra.H` (implementation)
 * - `bellman_ford_example.cc` / `Bellman_Ford.H` (negative weights)
 * - `johnson_example.cc` (all-pairs shortest paths with negative weights but no negative cycles)
 * - `astar_example.cc` / `AStar.H` (heuristic-guided shortest path)
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <limits>

#include <tpl_graph.H>
#include <Dijkstra.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Type Definitions
// =============================================================================

/// Node type: city name
using CityNode = Graph_Node<string>;

/// Arc type: distance in km
using RoadArc = Graph_Arc<double>;

/// Graph type: directed graph of cities connected by roads
using CityGraph = List_Digraph<CityNode, RoadArc>;

// =============================================================================
// Distance Accessor
// =============================================================================

/**
 * @brief Distance accessor functor for Dijkstra algorithm.
 *
 * This functor tells Dijkstra how to read the weight from an arc.
 * It must define:
 * - Distance_Type: the numeric type for distances
 * - operator()(Arc*): returns the arc's weight
 */
struct RoadDistance
{
  using Distance_Type = double;

  double operator()(CityGraph::Arc* arc) const
  {
    return arc->get_info();
  }
};

// =============================================================================
// Graph Building Utilities
// =============================================================================

/**
 * @brief Build a sample graph representing a city road network.
 *
 * Creates the following network:
 *
 * ```
 *     Alpha ──100── Beta ──150── Gamma
 *       │           │            │
 *      200         50          100
 *       │           │            │
 *     Delta ──80── Epsilon ─120─ Zeta
 *       │                        │
 *      300                      90
 *       │                        │
 *      Eta ────────250──────── Theta
 * ```
 *
 * @param[out] nodes Vector to store node pointers for later reference
 * @return The constructed graph
 */
CityGraph build_city_graph(vector<CityGraph::Node*>& nodes)
{
  CityGraph g;

  // Insert cities (nodes)
  nodes.push_back(g.insert_node("Alpha"));    // 0
  nodes.push_back(g.insert_node("Beta"));     // 1
  nodes.push_back(g.insert_node("Gamma"));    // 2
  nodes.push_back(g.insert_node("Delta"));    // 3
  nodes.push_back(g.insert_node("Epsilon"));  // 4
  nodes.push_back(g.insert_node("Zeta"));     // 5
  nodes.push_back(g.insert_node("Eta"));      // 6
  nodes.push_back(g.insert_node("Theta"));    // 7

  // Lambda to add bidirectional roads
  auto add_road = [&](int from, int to, double dist) {
    g.insert_arc(nodes[from], nodes[to], dist);
    g.insert_arc(nodes[to], nodes[from], dist);
  };

  // Add roads
  add_road(0, 1, 100);  // Alpha - Beta
  add_road(1, 2, 150);  // Beta - Gamma
  add_road(0, 3, 200);  // Alpha - Delta
  add_road(1, 4, 50);   // Beta - Epsilon
  add_road(2, 5, 100);  // Gamma - Zeta
  add_road(3, 4, 80);   // Delta - Epsilon
  add_road(4, 5, 120);  // Epsilon - Zeta
  add_road(3, 6, 300);  // Delta - Eta
  add_road(5, 7, 90);   // Zeta - Theta
  add_road(6, 7, 250);  // Eta - Theta

  return g;
}

/**
 * @brief Build a random graph for performance testing.
 *
 * @param num_nodes Number of nodes
 * @param edge_probability Probability of edge between any two nodes (0.0 to 1.0)
 * @param max_weight Maximum edge weight
 * @param seed Random seed for reproducibility
 * @return The constructed graph
 */
CityGraph build_random_graph(int num_nodes, double edge_probability,
                             double max_weight, unsigned seed = 42)
{
  CityGraph g;
  vector<CityGraph::Node*> nodes;

  mt19937 rng(seed);
  uniform_real_distribution<double> prob_dist(0.0, 1.0);
  uniform_real_distribution<double> weight_dist(1.0, max_weight);

  // Create nodes
  for (int i = 0; i < num_nodes; ++i)
    nodes.push_back(g.insert_node("N" + to_string(i)));

  // Create edges with given probability
  for (int i = 0; i < num_nodes; ++i)
    for (int j = i + 1; j < num_nodes; ++j)
      if (prob_dist(rng) < edge_probability)
        {
          double w = weight_dist(rng);
          g.insert_arc(nodes[i], nodes[j], w);
          g.insert_arc(nodes[j], nodes[i], w);
        }

  return g;
}

// =============================================================================
// Visualization Utilities
// =============================================================================

/**
 * @brief Print the graph structure.
 */
void print_graph(CityGraph& g)
{
  cout << "\n┌─────────────────────────────────────────┐\n";
  cout << "│         City Road Network               │\n";
  cout << "├─────────────────────────────────────────┤\n";
  cout << "│ Cities: " << setw(3) << g.get_num_nodes()
       << "                              │\n";
  cout << "│ Roads:  " << setw(3) << g.get_num_arcs() / 2
       << " (bidirectional)            │\n";
  cout << "└─────────────────────────────────────────┘\n\n";

  cout << "Connections:\n";
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
    {
      auto* node = nit.get_curr();
      cout << "  " << setw(8) << left << node->get_info() << right << " → ";
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

/**
 * @brief Print a path with detailed step-by-step information.
 */
void print_path_detailed(CityGraph& g, Path<CityGraph>& path)
{
  if (path.size() == 0)
    {
      cout << "  (empty path)\n";
      return;
    }

  // Print route using for_each_node
  cout << "\n  Route: ";
  bool first = true;
  path.for_each_node([&first](CityGraph::Node* node) {
    if (!first) cout << " → ";
    cout << node->get_info();
    first = false;
  });
  cout << "\n\n";

  // Step by step using for_each_arc (safe iteration)
  if (path.size() <= 1)
    return;

  cout << "  Step-by-step:\n";
  cout << "  ┌──────────────────────────────────────────────────┐\n";
  cout << "  │  From        Distance      To          Cumulative│\n";
  cout << "  ├──────────────────────────────────────────────────┤\n";

  double cumulative = 0;
  path.for_each_arc([&](CityGraph::Arc* arc) {
    cumulative += arc->get_info();
    cout << "  │  " << setw(8) << left << g.get_src_node(arc)->get_info()
         << "  ──" << setw(5) << right << arc->get_info() << " km──▶  "
         << setw(8) << left << g.get_tgt_node(arc)->get_info()
         << right << setw(7) << cumulative << " km │\n";
  });
  cout << "  └──────────────────────────────────────────────────┘\n";
}

// =============================================================================
// Timing Utility
// =============================================================================

template <typename Func>
double measure_time_ms(Func&& f)
{
  auto start = chrono::high_resolution_clock::now();
  f();
  auto end = chrono::high_resolution_clock::now();
  return chrono::duration<double, milli>(end - start).count();
}

// =============================================================================
// Main Demonstration
// =============================================================================

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║        Dijkstra's Shortest Path Algorithm - Example              ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  // =========================================================================
  // Part 1: Basic Usage
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 1: Basic Usage - Finding Shortest Path\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  vector<CityGraph::Node*> nodes;
  CityGraph g = build_city_graph(nodes);

  print_graph(g);

  // Define source and destination
  auto* source = nodes[0];  // Alpha
  auto* dest = nodes[7];    // Theta

  cout << "\n▶ Finding shortest path from " << source->get_info()
       << " to " << dest->get_info() << ":\n";

  // Create Dijkstra solver with default Binary Heap
  Dijkstra_Min_Paths<CityGraph, RoadDistance> dijkstra;

  // Find shortest path
  Path<CityGraph> path(g);
  double distance = dijkstra.find_min_path(g, source, dest, path);

  cout << "\n  Total distance: " << distance << " km\n";
  cout << "  Path length: " << path.size() << " cities\n";

  print_path_detailed(g, path);

  // =========================================================================
  // Part 2: Advanced Operations
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 2: Computing Complete Shortest Paths Tree\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  cout << "\nWhen you need shortest paths to MULTIPLE destinations, compute\n";
  cout << "the full tree once, then query efficiently.\n\n";

  // Method A: compute_min_paths_tree() - builds an actual tree graph
  cout << "▶ Method A: compute_min_paths_tree()\n";
  {
    CityGraph tree;
    double time = measure_time_ms([&]() {
      dijkstra.compute_min_paths_tree(g, source, tree);
    });

    cout << "  Tree nodes: " << tree.get_num_nodes() << "\n";
    cout << "  Tree edges: " << tree.get_num_arcs() << "\n";
    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n\n";

    // Query distances using the tree
    cout << "  Distances from " << source->get_info() << ":\n";
    for (size_t i = 1; i < nodes.size(); ++i)
      {
        Path<CityGraph> p(g);
        double d = dijkstra.get_min_path(tree, nodes[i], p);
        cout << "    → " << setw(8) << left << nodes[i]->get_info()
             << right << ": " << setw(6) << d << " km\n";
      }
  }

  // Method B: paint_min_paths_tree() - marks the graph in-place
  cout << "\n▶ Method B: paint_min_paths_tree()\n";
  cout << "  (More memory-efficient, marks graph directly)\n";
  {
    double time = measure_time_ms([&]() {
      dijkstra.paint_min_paths_tree(g, source);
    });

    cout << "  Time: " << fixed << setprecision(3) << time << " ms\n";

    // Query using get_min_path() after painting
    Path<CityGraph> p(g);
    double d = dijkstra.get_min_path(dest, p);
    cout << "  Distance to " << dest->get_info() << ": " << d << " km\n";
  }

  // =========================================================================
  // Part 3: Performance Comparison - Binary Heap vs Fibonacci Heap
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 3: Performance Comparison - Heap Types\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  cout << "\nDijkstra can use different priority queue implementations:\n";
  cout << "  • Binary Heap (default): O((V+E) log V) - good for sparse graphs\n";
  cout << "  • Fibonacci Heap: O(E + V log V) - better for dense graphs\n\n";

  // Test on larger random graphs
  const int SPARSE_NODES = 500;
  const double SPARSE_PROB = 0.02;  // ~2% edge probability

  const int DENSE_NODES = 200;
  const double DENSE_PROB = 0.3;    // ~30% edge probability

  // Test sparse graph
  cout << "▶ Sparse Graph (" << SPARSE_NODES << " nodes, ~"
       << int(SPARSE_PROB * 100) << "% edge density):\n";
  {
    CityGraph sparse = build_random_graph(SPARSE_NODES, SPARSE_PROB, 100.0);
    auto* s = sparse.get_first_node();

    // Binary Heap
    using DijkstraBin = Dijkstra_Min_Paths<CityGraph, RoadDistance,
                                           Node_Arc_Iterator,
                                           Dft_Show_Arc<CityGraph>,
                                           ArcHeap>;
    DijkstraBin dbin;
    CityGraph tree_bin;
    double time_bin = measure_time_ms([&]() {
      dbin.compute_min_paths_tree(sparse, s, tree_bin);
    });

    // Fibonacci Heap
    using DijkstraFib = Dijkstra_Min_Paths<CityGraph, RoadDistance,
                                           Node_Arc_Iterator,
                                           Dft_Show_Arc<CityGraph>,
                                           ArcFibonacciHeap>;
    DijkstraFib dfib;
    CityGraph tree_fib;
    double time_fib = measure_time_ms([&]() {
      dfib.compute_min_paths_tree(sparse, s, tree_fib);
    });

    cout << "  Binary Heap:    " << fixed << setprecision(3) << time_bin << " ms\n";
    cout << "  Fibonacci Heap: " << fixed << setprecision(3) << time_fib << " ms\n";
    cout << "  Edges: " << sparse.get_num_arcs() << "\n";
  }

  // Test dense graph
  cout << "\n▶ Dense Graph (" << DENSE_NODES << " nodes, ~"
       << int(DENSE_PROB * 100) << "% edge density):\n";
  {
    CityGraph dense = build_random_graph(DENSE_NODES, DENSE_PROB, 100.0);
    auto* s = dense.get_first_node();

    // Binary Heap
    using DijkstraBin = Dijkstra_Min_Paths<CityGraph, RoadDistance,
                                           Node_Arc_Iterator,
                                           Dft_Show_Arc<CityGraph>,
                                           ArcHeap>;
    DijkstraBin dbin;
    CityGraph tree_bin;
    double time_bin = measure_time_ms([&]() {
      dbin.compute_min_paths_tree(dense, s, tree_bin);
    });

    // Fibonacci Heap
    using DijkstraFib = Dijkstra_Min_Paths<CityGraph, RoadDistance,
                                           Node_Arc_Iterator,
                                           Dft_Show_Arc<CityGraph>,
                                           ArcFibonacciHeap>;
    DijkstraFib dfib;
    CityGraph tree_fib;
    double time_fib = measure_time_ms([&]() {
      dfib.compute_min_paths_tree(dense, s, tree_fib);
    });

    cout << "  Binary Heap:    " << fixed << setprecision(3) << time_bin << " ms\n";
    cout << "  Fibonacci Heap: " << fixed << setprecision(3) << time_fib << " ms\n";
    cout << "  Edges: " << dense.get_num_arcs() << "\n";
  }

  // =========================================================================
  // Part 4: Special Cases
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Part 4: Special Cases\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  // Case A: Source equals destination
  cout << "\n▶ Case A: Source = Destination (same node)\n";
  {
    Path<CityGraph> p(g);
    double d = dijkstra.find_min_path(g, source, source, p);

    if (d == std::numeric_limits<double>::max() && p.size() == 0)
      {
        cout << "  Aleph-w behavior: start==end returns Inf and an empty path.\n";
        cout << "  If you want a trivial path, handle it explicitly as distance=0 and path=[start].\n";
      }
    else
      {
        cout << "  Distance: " << d << " km\n";
        cout << "  Path length: " << p.size() << " cities\n";
      }
  }

  // Case B: Unreachable destination
  cout << "\n▶ Case B: Disconnected graph (unreachable node)\n";
  {
    CityGraph disconnected;
    auto* island_a = disconnected.insert_node("Island_A");
    auto* island_b = disconnected.insert_node("Island_B");
    disconnected.insert_node("Island_C");  // isolated

    disconnected.insert_arc(island_a, island_b, 10.0);
    disconnected.insert_arc(island_b, island_a, 10.0);

    // Try to reach isolated node
    Dijkstra_Min_Paths<CityGraph, RoadDistance> d;
    CityGraph tree;
    d.compute_min_paths_tree(disconnected, island_a, tree);

    cout << "  Graph nodes: " << disconnected.get_num_nodes() << "\n";
    cout << "  Reachable nodes (in tree): " << tree.get_num_nodes() << "\n";
  }

  // Case C: Single node graph
  cout << "\n▶ Case C: Single-node graph\n";
  {
    CityGraph single;
    auto* only_node = single.insert_node("Lonely");

    Dijkstra_Min_Paths<CityGraph, RoadDistance> d;
    CityGraph tree;
    d.compute_min_paths_tree(single, only_node, tree);

    cout << "  Tree nodes: " << tree.get_num_nodes() << "\n";
  }

  // =========================================================================
  // Summary
  // =========================================================================

  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Summary\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

  cout << R"(
┌─────────────────────────────────────────────────────────────────────┐
│ Dijkstra's Algorithm Properties                                     │
├─────────────────────────────────────────────────────────────────────┤
│ Time Complexity:                                                    │
│   • Binary Heap:    O((V + E) log V)                               │
│   • Fibonacci Heap: O(E + V log V)                                 │
│                                                                     │
│ Space Complexity: O(V)                                              │
│                                                                     │
│ Requirements:                                                       │
│   • All edge weights must be non-negative                          │
│   • For negative weights, use Bellman-Ford                         │
├─────────────────────────────────────────────────────────────────────┤
│ Key Methods:                                                        │
│   • find_min_path(g, src, dst, path) - single destination          │
│   • compute_min_paths_tree(g, src, tree) - build full tree         │
│   • paint_min_paths_tree(g, src) - mark graph in-place             │
│   • get_min_path(dst, path) - query after paint                    │
│   • get_min_path(tree, dst, path) - query from tree                │
│   • get_distance(dst) - just the distance after paint              │
├─────────────────────────────────────────────────────────────────────┤
│ When to Use:                                                        │
│   • Single shortest path: find_min_path()                          │
│   • Multiple queries from same source: compute/paint tree first    │
│   • Memory-constrained: paint_min_paths_tree()                     │
│   • Need actual tree structure: compute_min_paths_tree()           │
└─────────────────────────────────────────────────────────────────────┘
)";

  cout << "\nFor graphs with heuristic information, consider using A* (AStar.H)\n";
  cout << "which can be significantly faster for single-destination queries.\n\n";

  return 0;
}
