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
 * @file graph_components_example.C
 * @brief Graph connectivity: components and spanning trees
 *
 * This example demonstrates fundamental algorithms for analyzing graph
 * connectivity, which is crucial for understanding graph structure and
 * designing efficient algorithms. Connectivity analysis helps identify
 * isolated groups, understand graph structure, and design robust systems.
 *
 * ## Connected Components (Undirected Graphs)
 *
 * ### Definition
 *
 * A **connected component** is a maximal set of vertices where every
 * vertex can reach every other vertex through a path.
 *
 * **Key property**: In undirected graphs, connectivity is **symmetric**:
 * if u can reach v, then v can reach u.
 *
 * ### Algorithm
 *
 * Uses DFS or BFS traversal:
 * ```
 * Find_Components(G):
 *   visited = all false
 *   components = []
 *   For each unvisited vertex v:
 *     component = []
 *     DFS(v, visited, component)
 *     components.append(component)
 *   Return components
 *
 * DFS(v, visited, component):
 *   visited[v] = true
 *   component.append(v)
 *   For each neighbor w of v:
 *     If not visited[w]:
 *       DFS(w, visited, component)
 * ```
 *
 * **Time Complexity**: O(V + E) - visits each vertex and edge once
 *
 * ### Applications
 *
 * - **Social networks**: Finding friend groups (connected communities)
 * - **Network analysis**: Identifying isolated subnetworks
 * - **Image processing**: Connected pixel regions (blob detection)
 * - **Circuit design**: Identifying disconnected circuits
 * - **Ecology**: Habitat connectivity analysis
 *
 * ## Strongly Connected Components (Directed Graphs)
 *
 * ### Definition
 *
 * A **strongly connected component** (SCC) is a maximal set of vertices
 * in a directed graph where every vertex can reach every other vertex
 * through directed paths.
 *
 * **Key difference**: In directed graphs, connectivity is **NOT symmetric**:
 * u → v doesn't imply v → u.
 *
 * ### Algorithm: Tarjan's Algorithm
 *
 * Uses a single DFS pass with:
 * - **`index[v]`**: Discovery time (order of first visit)
 * - **`lowlink[v]`**: Lowest index reachable from v's DFS subtree
 *
 * **Key insight**: When `lowlink[v] == index[v]`, v is the root of an SCC.
 *
 * **Time Complexity**: O(V + E) - single DFS traversal
 *
 * ### Applications
 *
 * - **Web analysis**: Finding communities of mutually linked pages
 * - **Compiler optimization**: Detecting cyclic dependencies
 * - **Social networks**: Finding tightly-knit groups
 * - **Deadlock detection**: Identifying circular wait conditions
 * - **2-SAT solving**: Reducing to SCC finding
 *
 * ## Spanning Tree
 *
 * ### Definition
 *
 * A **spanning tree** is a subgraph that:
 * - Contains **all vertices**
 * - Is a **tree** (connected, acyclic)
 * - Has exactly **V-1 edges**
 *
 * ### Finding a Spanning Tree
 *
 * **Algorithm**: Use DFS or BFS to traverse the graph, keeping track
 * of tree edges (edges used in traversal).
 *
 * ```
 * Find_Spanning_Tree(G, root):
 *   visited = all false
 *   tree_edges = []
 *   DFS(root, visited, tree_edges)
 *   Return tree_edges
 *
 * DFS(v, visited, tree_edges):
 *   visited[v] = true
 *   For each neighbor w of v:
 *     If not visited[w]:
 *       tree_edges.append((v, w))
 *       DFS(w, visited, tree_edges)
 * ```
 *
 * **Time Complexity**: O(V + E)
 *
 * ### Applications
 *
 * - **Network design**: Minimum cost to connect all nodes (MST)
 * - **Broadcast trees**: Efficient message distribution
 * - **Graph simplification**: Reduce graph while maintaining connectivity
 * - **Routing**: Find paths in networks
 *
 * ## Comparison: Components vs SCCs
 *
 * | Aspect | Connected Components | Strongly Connected Components |
 * |--------|---------------------|------------------------------|
 * | Graph type | Undirected | Directed |
 * | Connectivity | Symmetric | Not symmetric |
 * | Algorithm | Simple DFS/BFS | Tarjan/Kosaraju |
 * | Complexity | O(V + E) | O(V + E) |
 *
 * ## Usage
 *
 * ```bash
 * # Analyze graph connectivity
 * ./graph_components_example
 *
 * # Find components
 * ./graph_components_example --components
 *
 * # Find SCCs
 * ./graph_components_example --scc
 *
 * # Find spanning tree
 * ./graph_components_example --spanning-tree
 *
 * # Network analysis demo
 * ./graph_components_example --network-analysis
 *
 * # Show help
 * ./graph_components_example --help
 * ```
 *
 * @see bfs_dfs_example.C Graph traversal (used by algorithms)
 * @see tarjan_example.C Tarjan's SCC algorithm
 * @see kosaraju_example.cc Kosaraju's SCC algorithm
 * @see mst_example.C Minimum spanning tree (weighted spanning tree)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <string>
#include <cstring>
#include <tpl_graph.H>
#include <tpl_components.H>
#include <tpl_spanning_tree.H>
#include <tpl_test_connectivity.H>
#include <Tarjan.H>

using namespace std;
using namespace Aleph;

static bool has_flag(int argc, char* argv[], const char* flag)
{
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], flag) == 0)
      return true;
  return false;
}

static void usage(const char* prog)
{
  cout << "Usage: " << prog
       << " [--components] [--scc] [--spanning-tree] [--network-analysis] [--all] [--help]\n";
  cout << "\nIf no flags are given, all demos are executed.\n";
}

// Graph types
using UGraph = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
using DGraph = List_Digraph<Graph_Node<string>, Graph_Arc<int>>;

// =============================================================================
// Helper functions
// =============================================================================

template <class GT>
typename GT::Node* find_or_create(GT& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return g.insert_node(name);
}

template <class GT>
void add_edge(GT& g, const string& src, const string& tgt, int weight = 1)
{
  auto s = find_or_create(g, src);
  auto t = find_or_create(g, tgt);
  g.insert_arc(s, t, weight);
}

template <class GT>
void print_graph(GT& g, const string& title)
{
  cout << title << ":\n";
  cout << "  Vertices: ";
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    cout << it.get_curr()->get_info() << " ";
  cout << "\n  Edges:\n";
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne()) {
    auto arc = it.get_curr();
    cout << "    " << g.get_src_node(arc)->get_info() 
         << " → " << g.get_tgt_node(arc)->get_info() << "\n";
  }
  cout << "\n";
}

// =============================================================================
// Example 1: Connected Components (Undirected Graph)
// =============================================================================

void demo_connected_components()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 1: Connected Components (Undirected)                ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "A connected component is a maximal set of vertices where\n";
  cout << "every pair of vertices is connected by a path.\n\n";

  UGraph g;

  // Component 1: A-B-C
  add_edge(g, "A", "B");
  add_edge(g, "B", "C");
  add_edge(g, "A", "C");

  // Component 2: D-E
  add_edge(g, "D", "E");

  // Component 3: F (isolated)
  find_or_create(g, "F");

  // Component 4: G-H-I
  add_edge(g, "G", "H");
  add_edge(g, "H", "I");

  print_graph(g, "Graph with 4 components");

  // Find connected components using Unconnected_Components
  DynList<UGraph> components;
  Unconnected_Components<UGraph> uc;
  uc(g, components);

  cout << "Found " << components.size() << " connected components:\n\n";

  int i = 1;
  for (auto& comp : components) {
    cout << "  Component " << i++ << ": ";
    for (auto it = comp.get_node_it(); it.has_curr(); it.next_ne())
      cout << it.get_curr()->get_info() << " ";
    cout << "(size: " << comp.get_num_nodes() << ")\n";
  }

  cout << "\n--- Testing connectivity ---\n\n";

  // Test connectivity using test_connectivity function
  Test_Connectivity<UGraph> test_conn;
  bool is_connected = test_conn(g);
  
  cout << "  Graph is fully connected: " << (is_connected ? "YES" : "NO") << "\n";
  cout << "  Number of components: " << components.size() << "\n";
}

// =============================================================================
// Example 2: Strongly Connected Components (Directed Graph)
// =============================================================================

void demo_strongly_connected()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 2: Strongly Connected Components (Directed)         ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "A strongly connected component (SCC) is a maximal set where\n";
  cout << "every vertex can reach every other vertex (bidirectional paths).\n\n";

  DGraph g;

  // SCC 1: A ↔ B ↔ C (cycle)
  add_edge(g, "A", "B");
  add_edge(g, "B", "C");
  add_edge(g, "C", "A");

  // SCC 2: D ↔ E (cycle)
  add_edge(g, "D", "E");
  add_edge(g, "E", "D");

  // SCC 3: F (single node)
  find_or_create(g, "F");

  // Cross-component edges
  add_edge(g, "C", "D");
  add_edge(g, "E", "F");

  print_graph(g, "Directed graph");

  // Using Tarjan's algorithm
  Tarjan_Connected_Components<DGraph> tarjan;
  auto sccs = tarjan(g);  // Returns DynList<DynList<Node*>>

  cout << "Tarjan's Algorithm found " << sccs.size() << " SCCs:\n\n";

  int i = 1;
  for (auto& scc : sccs) {
    cout << "  SCC " << i++ << ": ";
    for (auto node : scc)
      cout << node->get_info() << " ";
    cout << "\n";
  }

  // Check if graph has cycles
  cout << "\n--- Cycle analysis ---\n\n";
  cout << "  Graph has cycles: " << (tarjan.has_cycle(g) ? "YES" : "NO") << "\n";
  cout << "  Graph is a DAG: " << (tarjan.is_dag(g) ? "YES" : "NO") << "\n";

  cout << "\n--- Why SCCs matter ---\n\n";
  cout << "  • Compiler optimization: basic blocks\n";
  cout << "  • Web crawling: identify tightly connected pages\n";
  cout << "  • Social networks: find communities\n";
  cout << "  • 2-SAT problem solving\n";
}

// =============================================================================
// Example 3: Spanning Tree
// =============================================================================

void demo_spanning_tree()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 3: Spanning Tree                                    ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "A spanning tree connects all vertices using exactly V-1 edges.\n";
  cout << "It contains no cycles and forms a tree structure.\n\n";

  UGraph g;

  // Create a connected graph
  add_edge(g, "A", "B");
  add_edge(g, "A", "C");
  add_edge(g, "B", "C");
  add_edge(g, "B", "D");
  add_edge(g, "C", "D");
  add_edge(g, "C", "E");
  add_edge(g, "D", "E");

  cout << "Original graph:\n";
  cout << "  Vertices: " << g.get_num_nodes() << "\n";
  cout << "  Edges: " << g.get_num_arcs() << "\n\n";

  // Build spanning tree using DFS
  UGraph tree;
  Find_Depth_First_Spanning_Tree<UGraph> dfs_tree;
  
  auto start = find_or_create(g, "A");
  dfs_tree(g, start, tree);

  cout << "DFS Spanning Tree:\n";
  cout << "  Vertices: " << tree.get_num_nodes() << "\n";
  cout << "  Edges: " << tree.get_num_arcs() << "\n";
  cout << "  Tree edges:\n";
  for (auto it = tree.get_arc_it(); it.has_curr(); it.next_ne()) {
    auto arc = it.get_curr();
    cout << "    " << tree.get_src_node(arc)->get_info() 
         << " — " << tree.get_tgt_node(arc)->get_info() << "\n";
  }

  cout << "\n--- Properties of spanning tree ---\n\n";
  cout << "  • Connects all " << tree.get_num_nodes() << " vertices\n";
  cout << "  • Uses exactly " << tree.get_num_arcs() << " edges (V-1)\n";
  cout << "  • Contains no cycles\n";
  cout << "  • Unique path between any two vertices\n";
}

// =============================================================================
// Example 4: Practical Application - Network Analysis
// =============================================================================

void demo_network_analysis()
{
  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║      EXAMPLE 4: Network Analysis Application                     ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  cout << "Scenario: Analyzing a computer network for redundancy.\n\n";

  UGraph network;

  // Core network (highly connected)
  add_edge(network, "Server1", "Server2");
  add_edge(network, "Server2", "Server3");
  add_edge(network, "Server3", "Server1");
  add_edge(network, "Server1", "Router");
  add_edge(network, "Server2", "Router");

  // Branch office (separate component)
  add_edge(network, "Branch1", "Branch2");

  // Remote worker (isolated)
  find_or_create(network, "Remote");

  // Analyze components
  DynList<UGraph> components;
  Unconnected_Components<UGraph> uc;
  uc(network, components);

  cout << "Network analysis results:\n\n";
  cout << "  Total devices: " << network.get_num_nodes() << "\n";
  cout << "  Network segments: " << components.size() << "\n\n";

  int segment = 1;
  for (auto& comp : components) {
    cout << "  Segment " << segment++ << ":\n";
    cout << "    Devices: ";
    for (auto it = comp.get_node_it(); it.has_curr(); it.next_ne())
      cout << it.get_curr()->get_info() << " ";
    cout << "\n";
    cout << "    Connections: " << comp.get_num_arcs() << "\n";

    // Check if segment is a single point of failure
    if (comp.get_num_nodes() > 1 && comp.get_num_arcs() == comp.get_num_nodes() - 1)
      cout << "    ⚠️  No redundancy (tree structure)\n";
    else if (comp.get_num_arcs() > comp.get_num_nodes() - 1)
      cout << "    ✓ Has redundancy (multiple paths)\n";
    else
      cout << "    • Single device\n";
    cout << "\n";
  }

  cout << "Recommendation: Connect segments for full network connectivity.\n";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  if (has_flag(argc, argv, "--help"))
    {
      usage(argv[0]);
      return 0;
    }

  const bool run_all = has_flag(argc, argv, "--all") || argc == 1;
  const bool run_components = run_all || has_flag(argc, argv, "--components");
  const bool run_scc = run_all || has_flag(argc, argv, "--scc");
  const bool run_spanning = run_all || has_flag(argc, argv, "--spanning-tree");
  const bool run_network = run_all || has_flag(argc, argv, "--network-analysis");

  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║        Graph Connectivity Analysis in Aleph-w Library            ║\n";
  cout << "║                                                                  ║\n";
  cout << "║     Aleph-w Library - https://github.com/lrleon/Aleph-w          ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n";

  if (run_components)
    demo_connected_components();
  if (run_scc)
    demo_strongly_connected();
  if (run_spanning)
    demo_spanning_tree();
  if (run_network)
    demo_network_analysis();

  if (!(run_components || run_scc || run_spanning || run_network))
    {
      usage(argv[0]);
      return 1;
    }

  cout << "\n";
  cout << "╔══════════════════════════════════════════════════════════════════╗\n";
  cout << "║                         Summary                                  ║\n";
  cout << "╠══════════════════════════════════════════════════════════════════╣\n";
  cout << "║  Unconnected_Components: Find connected components (undirected) ║\n";
  cout << "║  Tarjan_Connected:       Find SCCs (directed) - O(V+E)          ║\n";
  cout << "║  Find_DFS_Spanning_Tree: Build spanning tree via DFS            ║\n";
  cout << "║  Find_BFS_Spanning_Tree: Build spanning tree via BFS            ║\n";
  cout << "║  Test_Connectivity:      Check if graph is connected            ║\n";
  cout << "║                                                                  ║\n";
  cout << "║  All algorithms run in O(V + E) time complexity.                ║\n";
  cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

  return 0;
}
