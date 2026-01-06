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
 * @file tarjan_example.C
 * @brief Tarjan's Algorithm: Finding Strongly Connected Components
 * 
 * This example demonstrates Tarjan's algorithm for finding strongly
 * connected components (SCCs) in directed graphs.
 * 
 * ## What is a Strongly Connected Component?
 * 
 * An SCC is a maximal subset of vertices where every vertex is
 * reachable from every other vertex in the subset. In other words,
 * there's a path from any vertex to any other vertex within the SCC.
 * 
 * ## Real-World Applications
 * 
 * - **Social networks**: Finding cohesive groups where everyone knows everyone
 * - **Web analysis**: Identifying communities of mutually linked pages
 * - **Compiler optimization**: Detecting cyclic dependencies
 * - **2-SAT satisfiability**: Boolean formula solving
 * - **Deadlock detection**: Finding circular wait conditions
 * 
 * ## Algorithm
 * 
 * Tarjan's algorithm uses a single DFS pass with:
 * - **index**: Order of first visit
 * - **lowlink**: Lowest index reachable from the DFS subtree
 * 
 * When lowlink[v] == index[v], v is the root of an SCC.
 * 
 * Time complexity: O(V + E)
 * 
 * @see Tarjan.H
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tpl_graph.H>
#include <Tarjan.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Graph types
using SNode = Graph_Node<string>;
using SArc = Graph_Arc<int>;
using SDigraph = List_Digraph<SNode, SArc>;

/**
 * @brief Build a sample web page link graph
 * 
 * Represents web pages and their hyperlinks:
 * 
 *   Homepage <---> About
 *      |            |
 *      v            v
 *   Products --> Services
 *      ^            |
 *      |            v
 *      +--------- Contact
 *      
 *   Blog <--> Article1
 *     |          |
 *     v          v
 *   Article2 <-- Article3
 */
SDigraph build_web_graph()
{
  SDigraph g;
  
  // Main site pages (form one SCC)
  auto home = g.insert_node("Homepage");
  auto about = g.insert_node("About");
  auto products = g.insert_node("Products");
  auto services = g.insert_node("Services");
  auto contact = g.insert_node("Contact");
  
  // Blog pages (form another SCC)
  auto blog = g.insert_node("Blog");
  auto art1 = g.insert_node("Article1");
  auto art2 = g.insert_node("Article2");
  auto art3 = g.insert_node("Article3");
  
  // Main site links (creates a cycle)
  g.insert_arc(home, about);
  g.insert_arc(about, home);     // Back link
  g.insert_arc(home, products);
  g.insert_arc(about, services);
  g.insert_arc(products, services);
  g.insert_arc(services, contact);
  g.insert_arc(contact, products);  // Creates cycle
  
  // Blog links (creates another cycle)
  g.insert_arc(blog, art1);
  g.insert_arc(art1, blog);      // Back link
  g.insert_arc(blog, art2);
  g.insert_arc(art1, art3);
  g.insert_arc(art3, art2);
  g.insert_arc(art2, blog);      // Creates cycle
  
  return g;
}

/**
 * @brief Build a software module dependency graph
 * 
 *   Core <--> Utils
 *    |         |
 *    v         v
 *   DB <--> Cache <--> Logger
 *            
 *   API --> Auth --> Session
 *           ^            |
 *           +------------+
 */
SDigraph build_module_graph()
{
  SDigraph g;
  
  // Core modules (SCC 1)
  auto core = g.insert_node("Core");
  auto utils = g.insert_node("Utils");
  
  // Data modules (SCC 2)
  auto db = g.insert_node("Database");
  auto cache = g.insert_node("Cache");
  auto logger = g.insert_node("Logger");
  
  // Auth modules (SCC 3)
  auto api = g.insert_node("API");
  auto auth = g.insert_node("Auth");
  auto session = g.insert_node("Session");
  
  // Core dependencies (bidirectional = SCC)
  g.insert_arc(core, utils);
  g.insert_arc(utils, core);
  
  // Data dependencies
  g.insert_arc(core, db);
  g.insert_arc(utils, cache);
  g.insert_arc(db, cache);
  g.insert_arc(cache, db);       // Cycle
  g.insert_arc(cache, logger);
  g.insert_arc(logger, cache);   // Cycle
  
  // Auth dependencies
  g.insert_arc(api, auth);
  g.insert_arc(auth, session);
  g.insert_arc(session, auth);   // Cycle
  
  return g;
}

/**
 * @brief Build a simple graph with no cycles (DAG)
 */
SDigraph build_dag()
{
  SDigraph g;
  
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  
  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(c, d);
  g.insert_arc(d, e);
  
  return g;
}

/**
 * @brief Find a node by name
 */
SDigraph::Node* find_node(SDigraph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

/**
 * @brief Print the graph structure
 */
void print_graph(SDigraph& g, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Nodes: " << g.get_num_nodes() << endl;
  cout << "Edges: " << g.get_num_arcs() << endl;
  
  cout << "\nAdjacency structure:" << endl;
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* node = nit.get_curr();
    cout << "  " << node->get_info() << " -> ";
    
    bool first = true;
    for (auto ait = g.get_out_it(node); ait.has_curr(); ait.next())
    {
      if (not first) cout << ", ";
      cout << ait.get_tgt_node()->get_info();
      first = false;
    }
    if (first) cout << "(none)";
    cout << endl;
  }
}

/**
 * @brief Demonstrate finding SCCs with Tarjan's algorithm
 */
void demo_find_sccs(SDigraph& g, const string& description)
{
  cout << "\n--- Finding Strongly Connected Components ---" << endl;
  cout << "Graph: " << description << endl;
  
  Tarjan_Connected_Components<SDigraph> tarjan;
  
  // Get SCCs as lists of nodes
  DynList<DynList<SDigraph::Node*>> sccs;
  tarjan.connected_components(g, sccs);
  
  cout << "\nFound " << sccs.size() << " strongly connected components:" << endl;
  
  int scc_num = 1;
  for (auto sit = sccs.get_it(); sit.has_curr(); sit.next(), ++scc_num)
  {
    auto& scc = sit.get_curr();
    cout << "\n  SCC " << scc_num << " (" << scc.size() << " node(s)): ";
    
    bool first = true;
    for (auto nit = scc.get_it(); nit.has_curr(); nit.next())
    {
      if (not first) cout << ", ";
      cout << nit.get_curr()->get_info();
      first = false;
    }
    
    if (scc.size() > 1)
      cout << " [cycle exists]";
    else
      cout << " [single node]";
  }
  cout << endl;
}

/**
 * @brief Demonstrate cycle detection
 */
void demo_cycle_detection(SDigraph& g, const string& description)
{
  cout << "\n--- Cycle Detection ---" << endl;
  cout << "Graph: " << description << endl;
  
  Tarjan_Connected_Components<SDigraph> tarjan;
  
  if (tarjan.has_cycle(g))
  {
    cout << "Result: Graph CONTAINS cycles" << endl;
    
    // Find and display a cycle
    Path<SDigraph> cycle(g);
    tarjan.compute_cycle(g, cycle);
    
    if (cycle.size() > 0)
    {
      cout << "A cycle found: ";
      bool first = true;
      for (auto it = cycle.get_it(); it.has_curr(); it.next())
      {
        if (not first) cout << " -> ";
        cout << it.get_curr()->get_info();
        first = false;
      }
      cout << " -> " << cycle.get_first_node()->get_info() << " (back to start)" << endl;
    }
  }
  else
  {
    cout << "Result: Graph is a DAG (no cycles)" << endl;
    cout << "This graph can be topologically sorted." << endl;
  }
}

/**
 * @brief Demonstrate practical application: web communities
 */
void demo_web_communities()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example: Web Page Community Detection" << endl;
  cout << string(60, '=') << endl;
  
  SDigraph g = build_web_graph();
  print_graph(g, "Web Page Link Graph");
  
  demo_find_sccs(g, "Web pages with hyperlinks");
  
  cout << "\n--- Analysis ---" << endl;
  cout << "Pages in the same SCC form a 'community' - they mutually" << endl;
  cout << "link to each other (directly or through intermediate pages)." << endl;
  cout << "This is useful for:" << endl;
  cout << "  - Detecting website structure" << endl;
  cout << "  - Finding related content clusters" << endl;
  cout << "  - SEO analysis" << endl;
}

/**
 * @brief Demonstrate practical application: dependency analysis
 */
void demo_dependency_analysis()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example: Software Module Dependency Analysis" << endl;
  cout << string(60, '=') << endl;
  
  SDigraph g = build_module_graph();
  print_graph(g, "Module Dependency Graph");
  
  demo_find_sccs(g, "Software modules with dependencies");
  demo_cycle_detection(g, "Software modules");
  
  cout << "\n--- Analysis ---" << endl;
  cout << "Modules in the same SCC have circular dependencies." << endl;
  cout << "This indicates:" << endl;
  cout << "  - These modules must be built together" << endl;
  cout << "  - Changes to one may affect others" << endl;
  cout << "  - Consider refactoring to break cycles" << endl;
}

/**
 * @brief Demonstrate DAG detection
 */
void demo_dag_detection()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example: DAG Detection" << endl;
  cout << string(60, '=') << endl;
  
  SDigraph g = build_dag();
  print_graph(g, "Directed Acyclic Graph");
  
  demo_find_sccs(g, "DAG structure");
  demo_cycle_detection(g, "DAG (no cycles)");
  
  cout << "\n--- Analysis ---" << endl;
  cout << "Each SCC contains only one node, confirming this is a DAG." << endl;
  cout << "DAGs can be topologically sorted and processed in order." << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Tarjan's SCC Algorithm Example", ' ', "1.0");
    
    TCLAP::SwitchArg webArg("w", "web",
      "Show web community detection example", false);
    TCLAP::SwitchArg modArg("m", "modules",
      "Show module dependency analysis example", false);
    TCLAP::SwitchArg dagArg("d", "dag",
      "Show DAG detection example", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(webArg);
    cmd.add(modArg);
    cmd.add(dagArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    bool runWeb = webArg.getValue();
    bool runMod = modArg.getValue();
    bool runDag = dagArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runWeb and not runMod and not runDag)
      runAll = true;
    
    cout << "=== Tarjan's Algorithm: Strongly Connected Components ===" << endl;
    cout << "An SCC is a maximal set where every vertex reaches every other." << endl;
    
    if (runAll or runWeb)
      demo_web_communities();
    
    if (runAll or runMod)
      demo_dependency_analysis();
    
    if (runAll or runDag)
      demo_dag_detection();
    
    cout << "\n=== Algorithm Summary ===" << endl;
    cout << "Tarjan's Algorithm: O(V + E) time, single DFS pass" << endl;
    cout << "Uses index (visit order) and lowlink (lowest reachable index)" << endl;
    cout << "When lowlink[v] == index[v], v is root of an SCC" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

