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
 * @file johnson_example.cc
 * @brief Johnson all-pairs shortest paths in Aleph-w (negative weights, Floyd comparison, benchmark flags).
 *
 * ## Overview
 *
 * This example demonstrates **Johnson's algorithm** for all-pairs shortest paths
 * (APSP) on a weighted directed graph.
 *
 * Johnson is especially useful when:
 *
 * - the graph can contain **negative edge weights**, but
 * - there are **no negative cycles**, and
 * - the graph is **sparse** (E is closer to V than to V²).
 *
 * Johnson combines Bellman-Ford (for reweighting) and Dijkstra (for efficient
 * shortest paths once all weights are non-negative).
 *
 * ## Data model used by this example
 *
 * - **Graph type**: `WeightedDigraph = List_Digraph<Graph_Node<string>, Graph_Arc<double>>`
 * - **Node info**: label/name (`string`)
 * - **Arc info**: weight/cost (`double`)
 *
 * ## Usage / CLI
 *
 * ```bash
 * # Run built-in demos
 * ./johnson_example
 *
 * # Compare Johnson vs Floyd-Warshall on a small demo graph
 * ./johnson_example --compare
 *
 * # Random graph benchmark (non-negative weights)
 * ./johnson_example -n 1000 -e 5000
 *
 * # Benchmark + compare (Floyd only runs automatically for n <= 200)
 * ./johnson_example --compare -n 150 -e 1000
 *
 * # Show help
 * ./johnson_example --help
 * ./johnson_example -h
 * ```
 *
 * Notes:
 * - If you pass `-n` without `-e`, the program uses `e = 5*n`.
 * - The generated benchmark graph uses **positive weights** in `[1, 10]`.
 *
 * ## Algorithm (Johnson)
 *
 * 1. Add a dummy source `q` and connect `q -> v` with weight 0 for all vertices `v`.
 * 2. Run Bellman-Ford from `q` to compute potentials `h(v)`.
 *    - If a negative cycle is detected, APSP is undefined.
 * 3. Reweight edges:
 *    - `w'(u,v) = w(u,v) + h(u) - h(v)`
 *    - All `w'` become non-negative.
 * 4. Run Dijkstra as needed on the reweighted graph.
 * 5. Convert distances back:
 *    - `d(u,v) = d'(u,v) - h(u) + h(v)`
 *
 * In Aleph-w, constructing `Johnson<...>` performs the Bellman-Ford step and
 * reweighting internally.
 *
 * ## Complexity
 *
 * Let **V** be the number of nodes and **E** the number of arcs.
 *
 * - **Classic Johnson** (one Dijkstra per source):
 *   - `O(V*E + V*(E+V) log V)` with a binary heap.
 * - **This example's Floyd comparison** computes a full distance table by repeatedly
 *   calling `johnson.get_distance(src,tgt)` for every pair, which is significantly
 *   more expensive if each `get_distance()` triggers a Dijkstra run.
 * - **Benchmark mode (`-n/-e`)** intentionally performs only a single sample query
 *   to avoid an `O(V^2)` all-pairs loop.
 *
 * ## Pitfalls and edge cases
 *
 * - **Negative cycles**: if a negative cycle exists, shortest paths are undefined.
 *   In this example, `Johnson<...>` construction throws `std::domain_error`.
 * - **Dense graphs**: for E ≈ V², Floyd-Warshall (`O(V^3)`) can be competitive or
 *   simpler to reason about.
 * - **Benchmarking**: avoid computing a full V×V matrix by naïvely calling
 *   `get_distance(src,tgt)` for all pairs on large graphs.
 *
 * ## References / see also
 *
 * - `Johnson.H` (implementation)
 * - `Bellman_Ford.H` (used internally by Johnson)
 * - `Dijkstra.H` / `dijkstra_example.cc` (used after reweighting)
 * - `Floyd_Warshall.H` / `write_floyd.C` (APSP alternative)
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <chrono>
#include <cmath>
#include <random>
#include <unordered_set>
#include <cstring>

#include <tpl_graph.H>
#include <Johnson.H>
#include <Floyd_Warshall.H>

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

template <typename Func>
double measure_ms(Func&& f);

static void usage(const char* prog)
{
  cout << "Usage: " << prog << " [--compare] [-n <nodes>] [-e <edges>] [--help]\n";
  cout << "\nIf no flags are given, all demos are executed.\n";
  cout << "\nIf -n/-e are provided, a random non-negative weighted graph is generated.\n";
  cout << "(If --compare is also given and the graph is small enough, Floyd-Warshall is run too.)\n";
}

static WeightedDigraph build_random_graph(int n, int e, unsigned seed)
{
  WeightedDigraph g;

  vector<Node*> nodes;
  nodes.reserve(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i)
    nodes.push_back(g.insert_node("V" + to_string(i)));

  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> node_dist(0, n - 1);
  std::uniform_real_distribution<double> weight_dist(1.0, 10.0);

  std::unordered_set<long long> used;
  used.reserve(static_cast<size_t>(e) * 2);

  // Ensure basic connectivity from V0 to V(n-1)
  int edges_added = 0;
  for (int i = 0; i + 1 < n && edges_added < e; ++i)
    {
      g.insert_arc(nodes[i], nodes[i + 1], weight_dist(rng));
      used.insert(static_cast<long long>(i) * n + (i + 1));
      ++edges_added;
    }

  while (edges_added < e)
    {
      const int u = node_dist(rng);
      const int v = node_dist(rng);
      if (u == v)
        continue;
      const long long key = static_cast<long long>(u) * n + v;
      if (used.find(key) != used.end())
        continue;
      used.insert(key);
      g.insert_arc(nodes[u], nodes[v], weight_dist(rng));
      ++edges_added;
    }

  return g;
}

static void compare_with_floyd(WeightedDigraph& g)
{
  vector<Node*> nodes;
  nodes.reserve(static_cast<size_t>(g.get_num_nodes()));
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    nodes.push_back(it.get_curr());

  double johnson_ms = 0.0;
  vector<double> jdists;
  jdists.reserve(nodes.size() * nodes.size());

  johnson_ms = measure_ms([&]() {
    Johnson<WeightedDigraph, Distance> johnson(g);
    for (auto* src : nodes)
      for (auto* tgt : nodes)
        jdists.push_back(johnson.get_distance(src, tgt));
  });

  double floyd_ms = 0.0;
  Floyd_All_Shortest_Paths<WeightedDigraph, Distance> *floyd_ptr = nullptr;
  floyd_ms = measure_ms([&]() {
    floyd_ptr = new Floyd_All_Shortest_Paths<WeightedDigraph, Distance>(g);
  });
  auto& floyd = *floyd_ptr;

  const auto& dist = floyd.get_dist_mat();
  const double Inf = std::numeric_limits<double>::max();

  size_t mismatches = 0;
  size_t idx = 0;
  for (auto* src : nodes)
    {
      const long isrc = floyd.index_node(src);
      for (auto* tgt : nodes)
        {
          const long itgt = floyd.index_node(tgt);
          const double fd = dist(isrc, itgt);
          const double jd = jdists[idx++];

          const bool f_inf = (fd == Inf);
          const bool j_inf = (jd == std::numeric_limits<double>::infinity());
          if (f_inf != j_inf)
            {
              ++mismatches;
              continue;
            }
          if (!f_inf && std::abs(fd - jd) > 1e-9)
            ++mismatches;
        }
    }

  delete floyd_ptr;

  cout << "\nComparison results:\n";
  cout << "  Johnson (multiple Dijkstra calls): " << fixed << setprecision(3) << johnson_ms << " ms\n";
  cout << "  Floyd-Warshall:                  " << fixed << setprecision(3) << floyd_ms << " ms\n";
  cout << "  Distance mismatches:             " << mismatches << "\n";
}

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
  cout << "Graph (" << g.get_num_nodes() << " nodes, "
       << g.get_num_arcs() << " arcs):\n";
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
          cout << tgt->get_info() << "(" << showpos << arc->get_info()
               << noshowpos << ")";
          first = false;
        }
      if (first) cout << "(none)";
      cout << "\n";
    }
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
// Example 1: Basic All-Pairs Shortest Paths
// =============================================================================

void example_basic_all_pairs()
{
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 1: All-Pairs Shortest Paths with Negative Weights\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  /*
   * Graph with negative edges but no negative cycles:
   *
   *       A ─(3)──→ B ─(1)─→ D
   *       │↖       ↑        │
   *     (8) (10) (-4)     (-3)
   *       │   ↖   │         ↓
   *       └─────→ C ─(2)──→ E
   *
   * Negative edges: C→B (-4), D→E (-3)
   * No negative cycle: A→C→B→D→E→A = 8+(-4)+1+(-3)+10 = 12 > 0
   */

  WeightedDigraph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");
  auto* E = g.insert_node("E");

  g.insert_arc(A, B, 3.0);
  g.insert_arc(A, C, 8.0);
  g.insert_arc(B, D, 1.0);
  g.insert_arc(C, B, -4.0);   // Negative edge (but no cycle: A→C→B, not back)
  g.insert_arc(C, E, 2.0);
  g.insert_arc(D, E, -3.0);   // Negative edge
  g.insert_arc(E, A, 10.0);   // Back edge but total cycle is positive

  print_graph(g);

  cout << "\n▶ Running Johnson's Algorithm:\n\n";

  try
    {
      Johnson<WeightedDigraph, Distance> johnson(g);

      // Print node potentials (h values)
      cout << "  Node potentials (from Bellman-Ford):\n";
      vector<Node*> nodes = {A, B, C, D, E};
      for (auto* node : nodes)
        {
          double h = johnson.get_potential(node);
          cout << "    h(" << node->get_info() << ") = " << showpos << h
               << noshowpos << "\n";
        }

      // Print all-pairs distances
      cout << "\n  Shortest distances (all pairs):\n\n";
      cout << "       ";
      for (auto* tgt : nodes)
        cout << setw(6) << tgt->get_info();
      cout << "\n       ";
      for (size_t i = 0; i < nodes.size(); ++i)
        cout << "──────";
      cout << "\n";

      for (auto* src : nodes)
        {
          cout << "  " << src->get_info() << " │ ";
          for (auto* tgt : nodes)
            {
              double dist = johnson.get_distance(src, tgt);
              if (dist == numeric_limits<double>::infinity())
                cout << setw(6) << "∞";
              else
                cout << setw(6) << fixed << setprecision(0) << dist;
            }
          cout << "\n";
        }

      // Show a specific path
      cout << "\n  Example path A → D:\n";
      double dist_ad = johnson.get_distance(A, D);
      cout << "    Distance: " << dist_ad << "\n";
    }
  catch (const domain_error& e)
    {
      cout << "  ERROR: " << e.what() << "\n";
    }
}

// =============================================================================
// Example 2: Negative Cycle Detection
// =============================================================================

void example_negative_cycle()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 2: Negative Cycle Detection\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "Johnson's algorithm uses Bellman-Ford internally, so it can\n";
  cout << "detect negative cycles. If one exists, construction throws.\n\n";

  /*
   * Graph with a negative cycle: A → B → C → A has weight -1
   *
   *       A ─(2)─→ B
   *       ↑       │
   *     (-5)    (2)
   *       │       ↓
   *       └───── C
   */

  WeightedDigraph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");

  g.insert_arc(A, B, 2.0);
  g.insert_arc(B, C, 2.0);
  g.insert_arc(C, A, -5.0);  // Creates negative cycle: 2 + 2 - 5 = -1

  print_graph(g);

  cout << "\n▶ Attempting to run Johnson's Algorithm:\n\n";

  try
    {
      Johnson<WeightedDigraph, Distance> johnson(g);
      cout << "  Unexpected: No exception thrown!\n";
    }
  catch (const domain_error& e)
    {
      cout << "  ⚠ EXCEPTION: " << e.what() << "\n";
      cout << "\n  Johnson cannot compute shortest paths when negative cycles exist\n";
      cout << "  because shortest paths become undefined (can always improve by\n";
      cout << "  going around the negative cycle one more time).\n";
    }
}

// =============================================================================
// Example 3: Understanding Reweighting
// =============================================================================

void example_reweighting()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 3: Understanding Edge Reweighting\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
  The key insight of Johnson's algorithm is REWEIGHTING:

    w'(u,v) = w(u,v) + h(u) - h(v)

  where h(v) is the shortest distance from a dummy source to v.

  Why does this work?
  ───────────────────
  For ANY path p from s to t, the reweighted length is:

    w'(p) = Σ w'(u,v)
          = Σ [w(u,v) + h(u) - h(v)]
          = Σ w(u,v) + h(s) - h(t)     (telescoping sum!)
          = w(p) + h(s) - h(t)

  Since h(s) and h(t) are constants, minimizing w'(p) also minimizes w(p)!

  Why are reweighted edges non-negative?
  ─────────────────────────────────────
  Since h is computed by Bellman-Ford:
    h(v) ≤ h(u) + w(u,v)    (triangle inequality)

  Rearranging:
    w(u,v) + h(u) - h(v) ≥ 0
    w'(u,v) ≥ 0  ✓
)";

  // Demonstrate with a simple example
  WeightedDigraph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");

  g.insert_arc(A, B, 5.0);
  g.insert_arc(B, C, -3.0);  // Negative!
  g.insert_arc(A, C, 4.0);

  cout << "\n  Original graph:\n";
  print_graph(g);

  try
    {
      Johnson<WeightedDigraph, Distance> johnson(g);

      cout << "\n  Node potentials:\n";
      cout << "    h(A) = " << johnson.get_potential(A) << "\n";
      cout << "    h(B) = " << johnson.get_potential(B) << "\n";
      cout << "    h(C) = " << johnson.get_potential(C) << "\n";

      cout << "\n  After reweighting, all edges become non-negative,\n";
      cout << "  allowing Dijkstra to be used!\n";
    }
  catch (const domain_error& e)
    {
      cout << "  Error: " << e.what() << "\n";
    }
}

// =============================================================================
// Example 4: Complexity Comparison
// =============================================================================

void example_complexity_comparison()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 4: When to Use Johnson vs Floyd-Warshall\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
┌───────────────────────────────────────────────────────────────────────────┐
│           All-Pairs Shortest Paths Algorithm Selection                    │
├───────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  Algorithm       │ Time Complexity    │ Best For                         │
│  ────────────────┼────────────────────┼──────────────────────────────────│
│  Floyd-Warshall  │ O(V³)              │ Dense graphs (E ≈ V²)            │
│                  │                    │ Simple implementation            │
│                  │                    │ Works with negative edges        │
│  ────────────────┼────────────────────┼──────────────────────────────────│
│  Johnson         │ O(V² log V + VE)   │ Sparse graphs (E ≈ V)            │
│                  │                    │ = O(V² log V) when sparse        │
│                  │                    │ Works with negative edges        │
│  ────────────────┼────────────────────┼──────────────────────────────────│
│  V × Dijkstra    │ O(V(V log V + E))  │ Non-negative edges only          │
│                  │                    │ Simpler than Johnson             │
│                                                                           │
├───────────────────────────────────────────────────────────────────────────┤
│  Sparsity Rule of Thumb:                                                  │
│  ─────────────────────────                                                │
│  • E < V² / log V  →  Use Johnson                                         │
│  • E > V² / log V  →  Use Floyd-Warshall                                  │
│                                                                           │
│  Example: V = 1000                                                        │
│  • E < 100,000    →  Johnson is faster                                    │
│  • E > 100,000    →  Floyd-Warshall may be faster                         │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
)";
}

// =============================================================================
// Example 5: Practical Application
// =============================================================================

void example_currency_arbitrage()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 5: Currency Arbitrage Detection\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
  Currency arbitrage occurs when you can profit by converting currencies
  in a cycle, ending up with more than you started with.

  To detect this using shortest paths:
  - Create edge (A→B) with weight = -log(exchange_rate(A→B))
  - A negative cycle means: product of rates > 1 → arbitrage opportunity!

  Example rates:
    USD → EUR: 0.85
    EUR → GBP: 0.88
    GBP → USD: 1.40

  Product: 0.85 × 0.88 × 1.40 = 1.0472 > 1  →  Arbitrage exists!
  
  As -log values:
    -log(0.85) = 0.163
    -log(0.88) = 0.128
    -log(1.40) = -0.336
  
  Sum: 0.163 + 0.128 - 0.336 = -0.045 < 0  →  Negative cycle!
)";

  WeightedDigraph g;

  auto* USD = g.insert_node("USD");
  auto* EUR = g.insert_node("EUR");
  auto* GBP = g.insert_node("GBP");

  // Convert exchange rates to -log values
  g.insert_arc(USD, EUR, -log(0.85));
  g.insert_arc(EUR, GBP, -log(0.88));
  g.insert_arc(GBP, USD, -log(1.40));

  // Add some reverse edges
  g.insert_arc(EUR, USD, -log(1.0 / 0.85));
  g.insert_arc(GBP, EUR, -log(1.0 / 0.88));
  g.insert_arc(USD, GBP, -log(1.0 / 1.40));

  cout << "\n▶ Checking for arbitrage opportunity:\n\n";

  try
    {
      Johnson<WeightedDigraph, Distance> johnson(g);
      cout << "  No arbitrage opportunity found (no negative cycles).\n";
    }
  catch (const domain_error& e)
    {
      cout << "  ⚠ ARBITRAGE OPPORTUNITY DETECTED!\n";
      cout << "  Negative cycle exists in the exchange rate graph.\n";
      cout << "\n  Profit path: USD → EUR → GBP → USD\n";
      cout << "  Starting with $1000: end with $" << fixed << setprecision(2)
           << 1000 * 0.85 * 0.88 * 1.40 << "\n";
    }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  bool compare = false;
  int n = -1;
  int e = -1;

  for (int i = 1; i < argc; ++i)
    {
      const string arg = argv[i];
      if (arg == "--help" || arg == "-h")
        {
          usage(argv[0]);
          return 0;
        }
      if (arg == "--compare")
        {
          compare = true;
          continue;
        }
      if (arg == "-n")
        {
          if (i + 1 >= argc)
            {
              usage(argv[0]);
              return 1;
            }
          n = std::stoi(argv[++i]);
          continue;
        }
      if (arg == "-e")
        {
          if (i + 1 >= argc)
            {
              usage(argv[0]);
              return 1;
            }
          e = std::stoi(argv[++i]);
          continue;
        }

      cout << "Unknown argument: " << arg << "\n";
      usage(argv[0]);
      return 1;
    }

  if (n != -1 || e != -1)
    {
      if (n == -1)
        n = 100;
      if (e == -1)
        e = 5 * n;
      if (n <= 0 || e < 0)
        {
          usage(argv[0]);
          return 1;
        }

      const int max_edges = n * (n - 1);
      if (e > max_edges)
        e = max_edges;

      cout << "Random graph benchmark: n=" << n << ", e=" << e << "\n";
      auto g = build_random_graph(n, e, 42);

      // Run a single sample query to avoid O(n^2) repeated Dijkstra calls.
      auto* src = find_node(g, "V0");
      auto* tgt = find_node(g, "V" + to_string(n - 1));

      try
        {
          Johnson<WeightedDigraph, Distance> johnson(g);
          double ms = measure_ms([&]() { (void)johnson.get_distance(src, tgt); });
          cout << "Sample query V0 -> V" << (n - 1) << " computed in " << fixed << setprecision(3)
               << ms << " ms\n";
        }
      catch (const std::exception& ex)
        {
          cout << "ERROR: " << ex.what() << "\n";
          return 1;
        }

      if (compare && n <= 200)
        compare_with_floyd(g);
      else if (compare)
        cout << "\nSkipping Floyd-Warshall comparison for n > 200.\n";

      return 0;
    }

  if (compare)
    {
      WeightedDigraph g;
      auto* A = g.insert_node("A");
      auto* B = g.insert_node("B");
      auto* C = g.insert_node("C");
      auto* D = g.insert_node("D");
      auto* E = g.insert_node("E");
      g.insert_arc(A, B, 3.0);
      g.insert_arc(A, C, 8.0);
      g.insert_arc(B, D, 1.0);
      g.insert_arc(C, B, -4.0);
      g.insert_arc(C, E, 2.0);
      g.insert_arc(D, E, -3.0);
      g.insert_arc(E, A, 10.0);
      compare_with_floyd(g);
      return 0;
    }

  cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
  cout << "║      Johnson's Algorithm for All-Pairs Shortest Paths                ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

  example_basic_all_pairs();
  example_negative_cycle();
  example_reweighting();
  example_complexity_comparison();
  example_currency_arbitrage();

  cout << "\nDone.\n";
  return 0;
}
