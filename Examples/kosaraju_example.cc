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
 * @file kosaraju_example.cc
 * @brief Comprehensive example of Kosaraju's algorithm for SCCs.
 *
 * This example demonstrates Kosaraju's algorithm for finding Strongly
 * Connected Components (SCCs) in a directed graph.
 *
 * ## What is a Strongly Connected Component?
 *
 * In a directed graph, a strongly connected component (SCC) is a maximal
 * set of vertices such that there is a path from every vertex to every
 * other vertex in the set.
 *
 * ## Algorithm Overview
 *
 * Kosaraju's algorithm works in two phases:
 *
 * ```
 * Phase 1: Compute finish times
 *   - Run DFS on the original graph
 *   - Record nodes in order they finish (postorder)
 *
 * Phase 2: Find SCCs
 *   - Create the transposed graph (reverse all edges)
 *   - Process nodes in decreasing order of finish time
 *   - Each DFS tree in this phase is one SCC
 * ```
 *
 * ## Why Does It Work?
 *
 * The key insight is that if u can reach v in G, then v can reach u in G^T
 * (transposed graph). By processing nodes in decreasing finish order, we
 * ensure that when we start a new DFS tree, we only reach nodes in the
 * same SCC.
 *
 * ## Applications
 *
 * - **2-SAT solver**: Can be reduced to finding SCCs
 * - **Dependency analysis**: Finding circular dependencies
 * - **Social networks**: Finding tightly-knit communities
 * - **Compiler optimization**: Data flow analysis
 * - **Web crawling**: Identifying website clusters
 *
 * ## Complexity
 *
 * - Time: O(V + E)
 * - Space: O(V + E) for the transposed graph
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <tpl_graph.H>
#include <kosaraju.H>
#include <Tarjan.H>  // For comparison

using namespace std;
using namespace Aleph;

// =============================================================================
// Graph Type Definitions
// =============================================================================

using Graph = List_Digraph<Graph_Node<string>, Graph_Arc<int>>;
using Node = Graph::Node;
using Arc = Graph::Arc;

// =============================================================================
// Utility Functions
// =============================================================================

Node* find_node(Graph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

void print_graph(Graph& g)
{
  cout << "Graph structure (" << g.get_num_nodes() << " nodes, "
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
          cout << tgt->get_info();
          first = false;
        }
      if (first) cout << "(none)";
      cout << "\n";
    }
}

// =============================================================================
// Example 1: Basic SCC Detection
// =============================================================================

void example_basic_sccs()
{
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 1: Basic Strongly Connected Components\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  /*
   * Graph with 3 SCCs:
   *
   *   SCC1: {A, B, C}      SCC2: {D, E}      SCC3: {F}
   *
   *       A ←──── B            D ←── E
   *       │       ↑            │     ↑
   *       └──→ C ─┘            └──→ ─┘
   *             │                    │
   *             └────────────→───────┘
   *                                  │
   *                                  ↓
   *                                  F
   *
   *   Arcs between SCCs: C→E, E→F
   */

  Graph g;

  // SCC 1: A, B, C form a cycle
  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");

  g.insert_arc(A, C);
  g.insert_arc(C, B);
  g.insert_arc(B, A);

  // SCC 2: D, E form a cycle
  auto* D = g.insert_node("D");
  auto* E = g.insert_node("E");

  g.insert_arc(D, E);
  g.insert_arc(E, D);

  // SCC 3: F is alone
  auto* F = g.insert_node("F");

  // Cross-component edges
  g.insert_arc(C, E);  // SCC1 → SCC2
  g.insert_arc(E, F);  // SCC2 → SCC3

  print_graph(g);

  cout << "\n▶ Running Kosaraju's Algorithm:\n\n";

  DynList<Graph> sccs;
  DynList<Arc*> cross_arcs;

  kosaraju_connected_components(g, sccs, cross_arcs);

  cout << "  Found " << sccs.size() << " strongly connected components:\n\n";

  int scc_num = 1;
  for (auto& scc : sccs)
    {
      cout << "  SCC " << scc_num++ << ": { ";
      bool first = true;
      for (auto it = scc.get_node_it(); it.has_curr(); it.next())
        {
          if (!first) cout << ", ";
          // Get the original node via cookie mapping
          auto scc_node = it.get_curr();
          auto orig_node = static_cast<Node*>(NODE_COOKIE(scc_node));
          cout << orig_node->get_info();
          first = false;
        }
      cout << " }\n";
      cout << "       Internal arcs: " << scc.get_num_arcs() << "\n\n";
    }

  cout << "  Cross-component arcs (" << cross_arcs.size() << "):\n";
  for (auto* arc : cross_arcs)
    {
      auto* src = g.get_src_node(arc);
      auto* tgt = g.get_tgt_node(arc);
      cout << "    " << src->get_info() << " → " << tgt->get_info() << "\n";
    }
}

// =============================================================================
// Example 2: Using the Lightweight Version
// =============================================================================

void example_lightweight_version()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 2: Lightweight SCC Detection (Node Lists Only)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "When you only need to know which nodes belong to which component,\n";
  cout << "the lightweight version is more efficient (no subgraph construction).\n\n";

  // Same graph as before
  Graph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");
  auto* E = g.insert_node("E");
  auto* F = g.insert_node("F");

  g.insert_arc(A, C);
  g.insert_arc(C, B);
  g.insert_arc(B, A);
  g.insert_arc(D, E);
  g.insert_arc(E, D);
  g.insert_arc(C, E);
  g.insert_arc(E, F);

  cout << "▶ Running lightweight Kosaraju:\n\n";

  auto sccs = kosaraju_connected_components(g);

  cout << "  Found " << sccs.size() << " components:\n\n";

  int scc_num = 1;
  for (auto& component : sccs)
    {
      cout << "  Component " << scc_num++ << ": { ";
      bool first = true;
      for (auto* node : component)
        {
          if (!first) cout << ", ";
          cout << node->get_info();
          first = false;
        }
      cout << " }\n";
    }
}

// =============================================================================
// Example 3: Strongly Connected Graph
// =============================================================================

void example_strongly_connected()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 3: Fully Strongly Connected Graph\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  /*
   * A graph is strongly connected if there is exactly ONE SCC
   * containing all vertices.
   *
   *       A ←───── B
   *       │↘     ↗│
   *       │  ↘  ↗ │
   *       ↓   ✕   ↓
   *       │  ↗ ↘  │
   *       │↗    ↘↓│
   *       C ─────→ D
   */

  Graph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");

  // Create edges so that every vertex can reach every other
  g.insert_arc(A, C);
  g.insert_arc(A, D);
  g.insert_arc(B, A);
  g.insert_arc(B, D);
  g.insert_arc(C, B);
  g.insert_arc(D, C);

  print_graph(g);

  auto sccs = kosaraju_connected_components(g);

  cout << "\n▶ Result:\n\n";
  cout << "  Number of SCCs: " << sccs.size() << "\n";

  if (sccs.size() == 1)
    cout << "  ✓ The graph is STRONGLY CONNECTED\n";
  else
    cout << "  ✗ The graph is NOT strongly connected\n";
}

// =============================================================================
// Example 4: DAG (No SCCs with more than one node)
// =============================================================================

void example_dag()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 4: Directed Acyclic Graph (DAG)\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << "In a DAG, every SCC contains exactly one node (no cycles).\n\n";

  /*
   *       A ────→ B ────→ D
   *       │       │       │
   *       └──→ C ←┘       ↓
   *                       E
   */

  Graph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");
  auto* E = g.insert_node("E");

  g.insert_arc(A, B);
  g.insert_arc(A, C);
  g.insert_arc(B, C);
  g.insert_arc(B, D);
  g.insert_arc(D, E);

  print_graph(g);

  auto sccs = kosaraju_connected_components(g);

  cout << "\n▶ Result:\n\n";
  cout << "  Number of SCCs: " << sccs.size() << "\n";
  cout << "  Number of nodes: " << g.get_num_nodes() << "\n";

  if (sccs.size() == g.get_num_nodes())
    cout << "\n  ✓ This is a DAG (each node is its own SCC)\n";
}

// =============================================================================
// Example 5: Comparison with Tarjan's Algorithm
// =============================================================================

void example_comparison_tarjan()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 5: Kosaraju vs Tarjan's Algorithm\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
┌────────────────────────────────────────────────────────────────────────┐
│                    SCC Algorithm Comparison                            │
├────────────────────────────────────────────────────────────────────────┤
│ Aspect             │ Kosaraju           │ Tarjan                       │
├────────────────────┼────────────────────┼──────────────────────────────┤
│ DFS passes         │ 2                  │ 1                            │
│ Extra space        │ O(V+E) for G^T     │ O(V) for stack               │
│ Time complexity    │ O(V + E)           │ O(V + E)                     │
│ Implementation     │ Simpler            │ More complex                 │
│ Order of SCCs      │ Reverse topo order │ Any order                    │
├────────────────────┴────────────────────┴──────────────────────────────┤
│ When to use Kosaraju:                                                  │
│   • Need SCCs in reverse topological order                             │
│   • Simpler implementation preferred                                   │
│   • Memory not critical (need space for transposed graph)              │
│                                                                        │
│ When to use Tarjan:                                                    │
│   • Memory is critical (no transposed graph needed)                    │
│   • Only one DFS pass preferred                                        │
│   • Already have Tarjan's implementation for other purposes            │
└────────────────────────────────────────────────────────────────────────┘
)";

  // Quick verification that both give same number of components
  Graph g;

  auto* A = g.insert_node("A");
  auto* B = g.insert_node("B");
  auto* C = g.insert_node("C");
  auto* D = g.insert_node("D");

  g.insert_arc(A, B);
  g.insert_arc(B, C);
  g.insert_arc(C, A);  // Creates cycle A-B-C
  g.insert_arc(C, D);

  auto kosaraju_sccs = kosaraju_connected_components(g);

  DynList<Graph> tarjan_blks;
  DynList<Arc*> tarjan_arcs;
  Tarjan_Connected_Components<Graph>()(g, tarjan_blks, tarjan_arcs);

  cout << "\n  Verification on sample graph:\n";
  cout << "    Kosaraju found: " << kosaraju_sccs.size() << " SCCs\n";
  cout << "    Tarjan found:   " << tarjan_blks.size() << " SCCs\n";

  if (kosaraju_sccs.size() == tarjan_blks.size())
    cout << "    ✓ Both algorithms agree!\n";
}

// =============================================================================
// Example 6: Applications
// =============================================================================

void example_applications()
{
  cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
  cout << "Example 6: Real-World Applications of SCCs\n";
  cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

  cout << R"(
  1. CIRCULAR DEPENDENCY DETECTION
     ─────────────────────────────
     In build systems (Make, CMake) or package managers (npm, pip),
     an SCC with more than one node indicates circular dependencies.

  2. 2-SAT SOLVER
     ────────────
     Boolean satisfiability with clauses of 2 literals can be solved
     in O(V+E) using SCC decomposition of the implication graph.

  3. SOCIAL NETWORK ANALYSIS
     ───────────────────────
     SCCs identify tightly-knit communities where information flows
     freely in both directions between all members.

  4. WEB PAGE RANKING
     ────────────────
     SCCs help identify clusters of web pages that link to each other,
     useful in understanding website structure.

  5. COMPILER OPTIMIZATION
     ─────────────────────
     SCCs in data flow graphs help identify loops and enable
     optimizations like loop-invariant code motion.

  6. DATABASE QUERY OPTIMIZATION
     ──────────────────────────
     Finding cycles in query dependency graphs helps detect
     and handle recursive queries.
)";
}

// =============================================================================
// Main
// =============================================================================

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
  cout << "║      Kosaraju's Algorithm for Strongly Connected Components          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

  example_basic_sccs();
  example_lightweight_version();
  example_strongly_connected();
  example_dag();
  example_comparison_tarjan();
  example_applications();

  cout << "\nDone.\n";
  return 0;
}
