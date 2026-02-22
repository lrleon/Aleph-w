
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
 * @file cut_nodes_example.C
 * @brief Cut nodes (articulation points), bridges, and biconnected components.
 *
 * ## Overview
 *
 * Demonstrates algorithms for analyzing connectivity vulnerabilities in
 * undirected graphs using Aleph-w:
 *
 * - **Cut nodes** (articulation points): vertices whose removal disconnects
 *   the graph (`Compute_Cut_Nodes`, `compute_cut_nodes()`).
 * - **Bridges** (cut edges / isthmuses): edges whose removal disconnects the
 *   graph (`Compute_Bridges`, `find_bridges()`).
 * - **Biconnected components**: maximal 2-edge-connected blocks
 *   (`Compute_Cut_Nodes::paint_subgraphs()`).
 *
 * Both algorithms use Tarjan's DFS low-link technique.
 *
 * ## Key concepts
 *
 * Given a DFS, let `df[v]` = discovery time and `low[v]` = minimum discovery
 * time reachable from v's subtree via back-edges.
 *
 * - **Articulation point** `u`: DFS root with ≥ 2 children, OR non-root where
 *   `low[child] >= df[u]`.
 * - **Bridge** `(u, v)` (tree edge): `low[v] > df[u]` — strictly greater,
 *   meaning no back-edge from v's subtree escapes above u.
 *
 * ## Data model
 *
 * - `Graph = List_Graph<Graph_Node<string>, Graph_Arc<int>>`
 * - Node info: label (`string`), arc info: int (unused in most demos).
 *
 * ## CLI options (TCLAP)
 *
 * | Flag | Long      | Description |
 * |------|-----------|-------------|
 * | `-b` | `--basic`       | Basic cut nodes demo |
 * | `-n` | `--network`     | Network vulnerability analysis |
 * | `-c` | `--biconnected` | Biconnected components demo |
 * | `-r` | `--resilience`  | Resilience comparison |
 * | `-f` | `--fix`         | Fixing vulnerabilities by adding edges |
 * | `-d` | `--bridges`     | Bridge edges demo |
 * | `-a` | `--all`         | Run all demos (default if no flag given) |
 *
 * ```bash
 * ./cut_nodes_example             # all demos
 * ./cut_nodes_example --bridges   # bridge demo only
 * ./cut_nodes_example --basic --bridges
 * ```
 *
 * ## Complexity
 *
 * - Time: O(V + E) — single DFS for both cut nodes and bridges.
 * - Space: O(V) extra.
 *
 * @see tpl_cut_nodes.H
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tpl_graph.H>
#include <tpl_cut_nodes.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Graph types
using Node = Graph_Node<string>;
using Arc = Graph_Arc<int>;
using Graph = List_Graph<Node, Arc>;

/**
 * @brief Build a network with clear cut nodes
 * 
 *       A --- B --- C
 *       |     |
 *       D --- E --- F --- G
 *                   |     |
 *                   +--H--+
 * 
 * Cut nodes: B, E, F (removing any disconnects the graph)
 */
Graph build_network_graph()
{
  Graph g;
  
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  auto f = g.insert_node("F");
  auto gg = g.insert_node("G");
  auto h = g.insert_node("H");
  
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(a, d);
  g.insert_arc(b, e);
  g.insert_arc(d, e);
  g.insert_arc(e, f);
  g.insert_arc(f, gg);
  g.insert_arc(f, h);
  g.insert_arc(gg, h);
  
  return g;
}

/**
 * @brief Build a cyclic graph with fewer cut nodes
 * 
 *     A --- B
 *    /|     |\
 *   / |     | \
 *  E  |     |  C
 *   \ |     | /
 *    \|     |/
 *     D --- F --- G
 * 
 * Only F is a cut node (the cycle A-B-C-F-D-E makes others resilient)
 */
Graph build_cyclic_graph()
{
  Graph g;
  
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  auto f = g.insert_node("F");
  auto gg = g.insert_node("G");
  
  // Main cycle
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, f);
  g.insert_arc(f, d);
  g.insert_arc(d, e);
  g.insert_arc(e, a);
  
  // Cross connections
  g.insert_arc(a, d);
  g.insert_arc(b, f);
  
  // Pendant node
  g.insert_arc(f, gg);
  
  return g;
}

/**
 * @brief Build a graph representing a computer network
 * 
 *    Server1 --- Router1 --- Switch1 --- PC1
 *                   |           |
 *                   |        Switch2 --- PC2
 *                   |           |
 *                Router2 --- Switch3 --- PC3
 *                   |
 *                Server2
 */
Graph build_computer_network()
{
  Graph g;
  
  auto server1 = g.insert_node("Server1");
  auto server2 = g.insert_node("Server2");
  auto router1 = g.insert_node("Router1");
  auto router2 = g.insert_node("Router2");
  auto switch1 = g.insert_node("Switch1");
  auto switch2 = g.insert_node("Switch2");
  auto switch3 = g.insert_node("Switch3");
  auto pc1 = g.insert_node("PC1");
  auto pc2 = g.insert_node("PC2");
  auto pc3 = g.insert_node("PC3");
  
  g.insert_arc(server1, router1);
  g.insert_arc(router1, switch1);
  g.insert_arc(switch1, pc1);
  g.insert_arc(switch1, switch2);
  g.insert_arc(switch2, pc2);
  g.insert_arc(router1, router2);
  g.insert_arc(switch2, switch3);
  g.insert_arc(router2, switch3);
  g.insert_arc(switch3, pc3);
  g.insert_arc(router2, server2);
  
  return g;
}

/**
 * @brief Find a node by name
 */
Graph::Node* find_node(Graph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

/**
 * @brief Print graph structure
 */
void print_graph(Graph& g, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Nodes: " << g.get_num_nodes() << endl;
  cout << "Edges: " << g.get_num_arcs() << endl;
  
  cout << "\nConnections:" << endl;
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* node = nit.get_curr();
    cout << "  " << node->get_info() << " -- ";
    
    bool first = true;
    for (Node_Arc_Iterator<Graph> ait(node); ait.has_curr(); ait.next())
    {
      auto* arc = ait.get_curr();
      auto* neighbor = g.get_connected_node(arc, node);
      if (not first) cout << ", ";
      cout << neighbor->get_info();
      first = false;
    }
    cout << endl;
  }
}

/**
 * @brief Demonstrate finding cut nodes
 */
void demo_cut_nodes(Graph& g, const string& description)
{
  cout << "\n--- Finding Cut Nodes (Articulation Points) ---" << endl;
  cout << "Graph: " << description << endl;
  
  Compute_Cut_Nodes<Graph> compute(g);
  DynDlist<Graph::Node*> cut_nodes;
  
  compute(g.get_first_node(), cut_nodes);
  
  if (cut_nodes.is_empty())
  {
    cout << "\nNo cut nodes found - graph is biconnected!" << endl;
    cout << "Removing any single node won't disconnect the graph." << endl;
  }
  else
  {
    cout << "\nCut nodes found: " << cut_nodes.size() << endl;
    cout << "Cut nodes: ";
    bool first = true;
    for (auto it = cut_nodes.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << ", ";
      cout << it.get_curr()->get_info();
      first = false;
    }
    cout << endl;
    
    cout << "\nImpact: Removing any of these nodes disconnects the graph." << endl;
  }
}

/**
 * @brief Practical example: Network vulnerability analysis
 */
void demo_network_vulnerability()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Network Vulnerability Analysis" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_computer_network();
  print_graph(g, "Computer Network");
  
  Compute_Cut_Nodes<Graph> compute(g);
  DynDlist<Graph::Node*> cut_nodes;
  
  compute(g.get_first_node(), cut_nodes);
  
  cout << "\n--- Vulnerability Analysis ---" << endl;
  
  if (cut_nodes.is_empty())
  {
    cout << "Network is fully redundant - no single point of failure!" << endl;
  }
  else
  {
    cout << "Single points of failure identified:" << endl;
    for (auto it = cut_nodes.get_it(); it.has_curr(); it.next())
    {
      auto* node = it.get_curr();
      cout << "\n  * " << node->get_info() << endl;
      
      // Count connections
      int connections = 0;
      for (Node_Arc_Iterator<Graph> ait(node); ait.has_curr(); ait.next())
        ++connections;
      
      cout << "    Connections: " << connections << endl;
      cout << "    Risk: CRITICAL - failure would partition the network" << endl;
    }
  }
  
  cout << "\n--- Recommendations ---" << endl;
  cout << "1. Add redundant links to eliminate cut nodes" << endl;
  cout << "2. Prioritize backup for critical equipment" << endl;
  cout << "3. Monitor cut nodes for failures" << endl;
}

/**
 * @brief Demonstrate biconnected components
 */
void demo_biconnected_components()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Biconnected Components" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_network_graph();
  print_graph(g, "Network Graph");
  
  Compute_Cut_Nodes<Graph> compute(g);
  DynDlist<Graph::Node*> cut_nodes;
  
  compute(g.get_first_node(), cut_nodes);
  
  cout << "\nCut nodes: ";
  for (auto it = cut_nodes.get_it(); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << " ";
  cout << endl;
  
  // Paint subgraphs (components)
  long num_colors = compute.paint_subgraphs();
  
  // paint_subgraphs() returns the next unused color (N+1 for N components).
  cout << "\n--- Biconnected Components ---" << endl;
  cout << "Number of components: " << (num_colors - 1) << endl;

  cout << "\nNodes by component (color):" << endl;
  for (long color = 1; color < num_colors; ++color)
  {
    cout << "  Component " << color << ": ";
    bool first = true;
    for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
    {
      auto* node = nit.get_curr();
      if (g.get_counter(node) == color)
      {
        if (not first) cout << ", ";
        cout << node->get_info();
        first = false;
      }
    }
    cout << endl;
  }
  
  cout << "\n--- Analysis ---" << endl;
  cout << "A biconnected component has no cut nodes within it." << endl;
  cout << "Components are connected through cut nodes." << endl;
}

/**
 * @brief Compare resilient vs fragile networks
 */
void demo_resilience_comparison()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Network Resilience Comparison" << endl;
  cout << string(60, '=') << endl;
  
  // Fragile network (tree-like)
  cout << "\n--- Fragile Network (Tree-like) ---" << endl;
  Graph fragile = build_network_graph();
  print_graph(fragile, "Fragile Network");
  
  {
    Compute_Cut_Nodes<Graph> compute(fragile);
    DynDlist<Graph::Node*> cut_nodes;
    compute(fragile.get_first_node(), cut_nodes);
    
    cout << "Cut nodes: " << cut_nodes.size() << " out of " 
         << fragile.get_num_nodes() << " nodes" << endl;
    double fragility = 100.0 * cut_nodes.size() / fragile.get_num_nodes();
    cout << "Fragility score: " << fixed << setprecision(1) << fragility << "%" << endl;
  }
  
  // Resilient network (with cycles)
  cout << "\n--- Resilient Network (With Cycles) ---" << endl;
  Graph resilient = build_cyclic_graph();
  print_graph(resilient, "Resilient Network");
  
  {
    Compute_Cut_Nodes<Graph> compute(resilient);
    DynDlist<Graph::Node*> cut_nodes;
    compute(resilient.get_first_node(), cut_nodes);
    
    cout << "Cut nodes: " << cut_nodes.size() << " out of " 
         << resilient.get_num_nodes() << " nodes" << endl;
    double fragility = 100.0 * cut_nodes.size() / resilient.get_num_nodes();
    cout << "Fragility score: " << fixed << setprecision(1) << fragility << "%" << endl;
  }
  
  cout << "\n--- Key Insight ---" << endl;
  cout << "Adding redundant connections (creating cycles) reduces fragility" << endl;
  cout << "by eliminating articulation points." << endl;
}

/**
 * @brief Demonstrate bridge finding with Compute_Bridges / find_bridges().
 *
 * Shows three scenarios:
 *  1. A graph with several bridges (the computer network).
 *  2. A fully 2-edge-connected graph (cycle → no bridges).
 *  3. The relationship between bridges and articulation points.
 */
void demo_bridges()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Bridge Edges (Isthmuses / Cut Edges)" << endl;
  cout << string(60, '=') << endl;
  cout << "A bridge is an edge whose removal disconnects the graph." << endl;
  cout << "Algorithm: Tarjan low-link DFS — O(V + E)." << endl;

  // ── Scenario 1: computer network (has bridges) ──────────────────────────
  {
    cout << "\n--- Scenario 1: Computer Network ---" << endl;

    Graph g = build_computer_network();
    print_graph(g, "Computer Network");

    // Free-function form
    auto bridges = find_bridges(g);

    cout << "\nBridge edges (" << bridges.size() << " found):" << endl;
    if (bridges.is_empty())
      cout << "  (none — graph is 2-edge-connected)" << endl;
    else
      bridges.for_each([&g](Graph::Arc * a)
        {
          cout << "  " << g.get_src_node(a)->get_info()
               << " ══ " << g.get_tgt_node(a)->get_info()
               << "  ← bridge" << endl;
        });

    cout << "\nImpact: severing a bridge immediately splits the network." << endl;
  }

  // ── Scenario 2: pure cycle (no bridges) ─────────────────────────────────
  {
    cout << "\n--- Scenario 2: Ring Topology (no bridges) ---" << endl;

    Graph g;
    auto a = g.insert_node("A");
    auto b = g.insert_node("B");
    auto c = g.insert_node("C");
    auto d = g.insert_node("D");
    auto e = g.insert_node("E");
    g.insert_arc(a, b); g.insert_arc(b, c);
    g.insert_arc(c, d); g.insert_arc(d, e);
    g.insert_arc(e, a);

    // Class form — useful when calling find_bridges() multiple times
    Compute_Bridges<Graph> cb(g);
    auto bridges = cb.find_bridges();

    cout << "Ring A-B-C-D-E-A: ";
    if (bridges.is_empty())
      cout << "no bridges (fully 2-edge-connected)." << endl;
    else
      cout << bridges.size() << " bridge(s) found." << endl;
  }

  // ── Scenario 3: two triangles joined by a bridge ────────────────────────
  {
    cout << "\n--- Scenario 3: Two Triangles + Bridge ---" << endl;
    cout << "  Triangle1: A-B-C-A    Bridge: C--D    Triangle2: D-E-F-D" << endl;

    Graph g;
    auto a = g.insert_node("A"), b = g.insert_node("B"),
         c = g.insert_node("C"), d = g.insert_node("D"),
         e = g.insert_node("E"), f = g.insert_node("F");
    g.insert_arc(a, b); g.insert_arc(b, c); g.insert_arc(c, a);
    auto br = g.insert_arc(c, d);            // the bridge
    g.insert_arc(d, e); g.insert_arc(e, f); g.insert_arc(f, d);

    auto bridges = find_bridges(g, a);

    cout << "\nBridges found: " << bridges.size() << endl;
    bridges.for_each([&g](Graph::Arc * arc)
      {
        cout << "  " << g.get_src_node(arc)->get_info()
             << " ══ " << g.get_tgt_node(arc)->get_info() << endl;
      });
    (void)br; // used above

    // Show relationship: bridge endpoints are also articulation points
    auto cut_nodes = compute_cut_nodes(g);
    cout << "\nArticulation points (cut nodes): ";
    cut_nodes.for_each([](Graph::Node * p)
      { cout << p->get_info() << " "; });
    cout << endl;
    cout << "Note: bridge endpoints C and D are also articulation points." << endl;
  }
}

/**
 * @brief Demonstrate fixing network vulnerabilities
 */
void demo_fixing_vulnerabilities()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Fixing Network Vulnerabilities" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_network_graph();
  
  cout << "\n--- Before: Original Network ---" << endl;
  
  {
    Compute_Cut_Nodes<Graph> compute(g);
    DynDlist<Graph::Node*> cut_nodes;
    compute(g.get_first_node(), cut_nodes);
    
    cout << "Cut nodes: ";
    for (auto it = cut_nodes.get_it(); it.has_curr(); it.next())
      cout << it.get_curr()->get_info() << " ";
    cout << endl;
  }
  
  cout << "\n--- Adding Redundant Links ---" << endl;
  
  // Add redundant links to eliminate cut nodes
  auto* c = find_node(g, "C");
  auto* d = find_node(g, "D");
  auto* a = find_node(g, "A");
  auto* f = find_node(g, "F");
  
  cout << "Adding link: C -- D" << endl;
  g.insert_arc(c, d);
  
  cout << "Adding link: A -- F" << endl;
  g.insert_arc(a, f);
  
  cout << "\n--- After: Reinforced Network ---" << endl;
  
  {
    Compute_Cut_Nodes<Graph> compute(g);
    DynDlist<Graph::Node*> cut_nodes;
    compute(g.get_first_node(), cut_nodes);
    
    if (cut_nodes.is_empty())
      cout << "No cut nodes! Network is now more resilient." << endl;
    else
    {
      cout << "Remaining cut nodes: ";
      for (auto it = cut_nodes.get_it(); it.has_curr(); it.next())
        cout << it.get_curr()->get_info() << " ";
      cout << endl;
    }
  }
  
  cout << "\n--- Lesson ---" << endl;
  cout << "Strategic addition of edges can eliminate articulation points" << endl;
  cout << "and improve network reliability." << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Cut Nodes (Articulation Points) Example", ' ', "1.0");
    
    TCLAP::SwitchArg basicArg("b", "basic",
      "Show basic cut nodes demo", false);
    TCLAP::SwitchArg netArg("n", "network",
      "Show network vulnerability analysis", false);
    TCLAP::SwitchArg biconArg("c", "biconnected",
      "Show biconnected components", false);
    TCLAP::SwitchArg compareArg("r", "resilience",
      "Compare network resilience", false);
    TCLAP::SwitchArg fixArg("f", "fix",
      "Show fixing vulnerabilities", false);
    TCLAP::SwitchArg bridgesArg("d", "bridges",
      "Show bridge edges demo", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);

    cmd.add(basicArg);
    cmd.add(netArg);
    cmd.add(biconArg);
    cmd.add(compareArg);
    cmd.add(fixArg);
    cmd.add(bridgesArg);
    cmd.add(allArg);

    cmd.parse(argc, argv);

    bool runBasic   = basicArg.getValue();
    bool runNet     = netArg.getValue();
    bool runBicon   = biconArg.getValue();
    bool runCompare = compareArg.getValue();
    bool runFix     = fixArg.getValue();
    bool runBridges = bridgesArg.getValue();
    bool runAll     = allArg.getValue();

    if (not runBasic and not runNet and not runBicon and
        not runCompare and not runFix and not runBridges)
      runAll = true;

    cout << "=== Cut Nodes, Bridges, and Biconnected Components ===" << endl;
    cout << "Cut node removal / bridge removal disconnects the graph." << endl;

    if (runAll or runBasic)
      {
        Graph g = build_network_graph();
        print_graph(g, "Sample Network");
        demo_cut_nodes(g, "Sample network");
      }

    if (runAll or runNet)
      demo_network_vulnerability();

    if (runAll or runBicon)
      demo_biconnected_components();

    if (runAll or runCompare)
      demo_resilience_comparison();

    if (runAll or runFix)
      demo_fixing_vulnerabilities();

    if (runAll or runBridges)
      demo_bridges();

    cout << "\n=== Summary ===" << endl;
    cout << "Cut nodes and bridges are dual notions of graph vulnerability:" << endl;
    cout << "  cut node  = vertex whose removal disconnects the graph" << endl;
    cout << "  bridge    = edge  whose removal disconnects the graph" << endl;
    cout << "Both detected by Tarjan low-link DFS in O(V + E)." << endl;
    cout << "Headers: tpl_cut_nodes.H (Compute_Cut_Nodes, Compute_Bridges," << endl;
    cout << "         compute_cut_nodes(), find_bridges())" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

