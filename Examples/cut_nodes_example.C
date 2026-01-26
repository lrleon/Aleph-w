
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
 * @brief Cut nodes (articulation points) and biconnected components in Aleph-w.
 *
 * ## Overview
 *
 * This example demonstrates algorithms for analyzing connectivity in undirected
 * graphs:
 *
 * - **cut nodes** (articulation points): vertices whose removal disconnects the graph
 * - **biconnected components**: maximal edge blocks not separable by removing one vertex
 *
 * The implementation uses a DFS-based approach with discovery/low-link values
 * (Tarjan-style).
 *
 * ## Data model used by this example
 *
 * - **Graph type**: `Graph = List_Graph<Graph_Node<string>, Graph_Arc<int>>`
 * - **Node info**: label (`string`)
 * - **Arc info**: integer value (`int`) used by the demo
 *
 * ## Usage / CLI
 *
 * This example uses TCLAP. Options:
 *
 * - `--basic` / `-b`: basic cut nodes demo.
 * - `--network` / `-n`: network vulnerability analysis.
 * - `--biconnected` / `-c`: biconnected components demo.
 * - `--resilience` / `-r`: resilience comparison demo.
 * - `--fix` / `-f`: show how adding edges can remove articulation points.
 * - `--all` / `-a`: run all demos.
 * - `--help`: show help.
 *
 * Behavior:
 * - If no demo-selection flags are provided, the program defaults to running **all** demos.
 *
 * ```bash
 * ./cut_nodes_example
 * ./cut_nodes_example --basic
 * ./cut_nodes_example --network
 * ./cut_nodes_example --biconnected
 * ./cut_nodes_example --resilience
 * ./cut_nodes_example --fix
 * ./cut_nodes_example --help
 * ```
 *
 * ## Algorithms
 *
 * The core idea is DFS with:
 *
 * - `df[v]`: discovery time
 * - `low[v]`: smallest discovery time reachable from v via tree edges + at most one back edge
 *
 * A vertex `u` is an articulation point if:
 *
 * - root case: `u` is DFS root and has ≥ 2 children
 * - non-root case: there exists a child `v` with `low[v] >= df[u]`
 *
 * This example focuses on cut nodes and biconnected blocks (bridges are discussed
 * conceptually but not printed as a primary output).
 *
 * ## Complexity
 *
 * Let **V** be the number of vertices and **E** the number of edges.
 *
 * - Time: `O(V + E)`
 * - Extra space: `O(V)`
 *
 * ## Pitfalls and edge cases
 *
 * - Disconnected graphs require running DFS from each unvisited node.
 * - DFS recursion depth may be large on deep graphs.
 *
 * ## References / see also
 *
 * - `tpl_cut_nodes.H`
 * - `graph_components_example.C` (components)
 * - `tarjan_example.C` (DFS-based decomposition)
 *
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
  
  cout << "\n--- Biconnected Components ---" << endl;
  cout << "Number of components: " << num_colors << endl;
  
  cout << "\nNodes by component (color):" << endl;
  for (long color = 1; color <= num_colors; ++color)
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
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(basicArg);
    cmd.add(netArg);
    cmd.add(biconArg);
    cmd.add(compareArg);
    cmd.add(fixArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    bool runBasic = basicArg.getValue();
    bool runNet = netArg.getValue();
    bool runBicon = biconArg.getValue();
    bool runCompare = compareArg.getValue();
    bool runFix = fixArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runBasic and not runNet and not runBicon and 
        not runCompare and not runFix)
      runAll = true;
    
    cout << "=== Cut Nodes (Articulation Points) and Biconnected Components ===" << endl;
    cout << "A cut node's removal disconnects the graph." << endl;
    
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
    
    cout << "\n=== Summary ===" << endl;
    cout << "Cut nodes are critical points in network topology." << endl;
    cout << "Uses: Network reliability, infrastructure planning, graph analysis" << endl;
    cout << "Algorithm: DFS with low-link values, O(V + E)" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

