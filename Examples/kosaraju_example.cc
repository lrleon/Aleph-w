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
 * @brief Comprehensive example of Kosaraju's algorithm for SCCs
 *
 * This example demonstrates Kosaraju's algorithm for finding Strongly
 * Connected Components (SCCs) in a directed graph. Kosaraju's algorithm
 * is conceptually simpler than Tarjan's, using two DFS passes instead
 * of one, but requiring a graph transpose.
 *
 * ## What is a Strongly Connected Component?
 *
 * In a directed graph, a **strongly connected component** (SCC) is a
 * maximal set of vertices such that there is a path from every vertex
 * to every other vertex in the set.
 *
 * **Key property**: In an SCC, you can travel from any vertex to any
 * other vertex (and back).
 *
 * ### Example
 *
 * Graph: A → B → C → A, D → E → D
 *
 * SCCs:
 * - {A, B, C} - form a cycle
 * - {D, E} - form a cycle
 *
 * ## Kosaraju's Algorithm
 *
 * ### Overview
 *
 * Kosaraju's algorithm works in **two phases**:
 *
 * ### Phase 1: Compute Finish Times
 *
 * ```
 * 1. Run DFS on the original graph G
 * 2. Record nodes in order they finish (postorder)
 * 3. Store finish times
 * ```
 *
 * **Key**: We record when DFS finishes exploring a vertex (post-order).
 *
 * ### Phase 2: Find SCCs
 *
 * ```
 * 1. Create transposed graph G^T (reverse all edges)
 * 2. Process nodes in DECREASING order of finish time
 * 3. Run DFS on G^T starting from highest finish time
 * 4. Each DFS tree found is one SCC
 * ```
 *
 * **Key**: Process in reverse finish order, on reversed graph.
 *
 * ## Why Does It Work?
 *
 * ### Key Insight
 *
 * If vertex u can reach vertex v in graph G, then v can reach u in the
 * transposed graph G^T (where all edges are reversed).
 *
 * ### Why Reverse Finish Order?
 *
 * By processing vertices in **decreasing finish order**:
 * - We start with vertices that finished LAST in Phase 1
 * - These are "sink" vertices (end of paths)
 * - In G^T, they become "source" vertices
 * - DFS from them only reaches vertices in the same SCC
 *
 * ### Why Transpose?
 *
 * - In G: If u → v, then u can reach v
 * - In G^T: If v → u (reversed), then v can reach u
 * - Together: u and v can reach each other ⟺ same SCC
 *
 * ## Algorithm Pseudocode
 *
 * ```
 * Kosaraju_SCC(G):
 *   // Phase 1: Compute finish times
 *   stack = empty
 *   visited = all false
 *   For each vertex v in G:
 *     If not visited[v]:
 *       DFS_Phase1(v, G, visited, stack)
 *
 *   // Phase 2: Find SCCs on transposed graph
 *   G_transpose = transpose(G)
 *   visited = all false
 *   While stack not empty:
 *     v = stack.pop()
 *     If not visited[v]:
 *       SCC = DFS_Phase2(v, G_transpose, visited)
 *       Output SCC
 *
 * DFS_Phase1(v, G, visited, stack):
 *   visited[v] = true
 *   For each neighbor w of v in G:
 *     If not visited[w]:
 *       DFS_Phase1(w, G, visited, stack)
 *   stack.push(v)  // Post-order: push after exploring
 *
 * DFS_Phase2(v, G_T, visited):
 *   visited[v] = true
 *   SCC = {v}
 *   For each neighbor w of v in G_T:
 *     If not visited[w]:
 *       SCC += DFS_Phase2(w, G_T, visited)
 *   Return SCC
 * ```
 *
 * ## Complexity
 *
 * - **Time**: O(V + E) - two DFS passes
 * - **Space**: O(V + E) - for transposed graph
 *
 * **Note**: Tarjan's algorithm is more space-efficient (no transpose needed)
 *
 * ## Comparison with Tarjan's Algorithm
 *
 * | Aspect | Kosaraju's | Tarjan's |
 * |--------|-----------|----------|
 * | Passes | 2 DFS | 1 DFS |
 * | Graph transpose | Required | Not needed |
 * | Space | O(V+E) for transpose | O(V) |
 * | Implementation | Simpler | More complex |
 * | Performance | Slightly slower | Faster |
 * | Best for | Learning, simplicity | Production, efficiency |
 *
 * ## Applications
 *
 * ### 2-SAT Satisfiability
 * - **Boolean formulas**: Reduce 2-SAT to SCC finding
 * - **Constraint satisfaction**: Solve logical constraints
 * - **Circuit design**: Verify satisfiability
 *
 * ### Dependency Analysis
 * - **Circular dependencies**: Find cycles in dependency graphs
 * - **Build systems**: Detect circular build dependencies
 * - **Package managers**: Find circular package dependencies
 *
 * ### Social Networks
 * - **Communities**: Find tightly-knit groups
 * - **Influence**: Identify influential clusters
 * - **Recommendations**: Suggest connections in same SCC
 *
 * ### Compiler Optimization
 * - **Data flow**: Analyze variable dependencies
 * - **Dead code**: Identify unreachable code
 * - **Optimization**: Find optimization opportunities
 *
 * ### Web Crawling
 * - **Site clusters**: Identify website communities
 * - **Link analysis**: Understand web structure
 * - **SEO**: Analyze site connectivity
 *
 * ## Example: Dependency Graph
 *
 * ```
 * Modules: A → B → C → A  (circular!)
 *          D → E → D      (circular!)
 *          F → G
 *
 * SCCs:
 * - {A, B, C} - circular dependency
 * - {D, E} - circular dependency
 * - {F} - single module
 * - {G} - single module
 * ```
 *
 * This helps identify problematic circular dependencies.
 *
 * ## Usage
 *
 * ```bash
 * # Run Kosaraju's algorithm demo
 * ./kosaraju_example
 *
 * # Compare with Tarjan's
 * ./kosaraju_example --compare
 *
 * # Show help
 * ./kosaraju_example --help
 * ```
 *
 * @see kosaraju.H Kosaraju's algorithm implementation
 * @see tarjan_example.C Tarjan's algorithm (more efficient)
 * @see graph_components_example.C Connected components (undirected)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>

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

static void usage(const char* prog)
{
  cout << "Usage: " << prog << " [--compare] [--help]\n";
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
  if (has_flag(argc, argv, "--help"))
    {
      usage(argv[0]);
      return 0;
    }

  const bool compare = has_flag(argc, argv, "--compare");

  cout << "╔══════════════════════════════════════════════════════════════════════╗\n";
  cout << "║      Kosaraju's Algorithm for Strongly Connected Components          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

  if (compare)
    {
      example_comparison_tarjan();
      cout << "\nDone.\n";
      return 0;
    }

  example_basic_sccs();
  example_lightweight_version();
  example_strongly_connected();
  example_dag();
  example_comparison_tarjan();
  example_applications();

  cout << "\nDone.\n";
  return 0;
}
