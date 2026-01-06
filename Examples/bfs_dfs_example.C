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
 * @file bfs_dfs_example.C
 * @brief Graph Traversal: BFS vs DFS Comparison
 * 
 * This example demonstrates the two fundamental graph traversal algorithms:
 * - **BFS (Breadth-First Search)**: Explores level by level (uses queue)
 * - **DFS (Depth-First Search)**: Explores as deep as possible first (uses stack)
 * 
 * ## Key Differences
 * 
 * | Aspect | BFS | DFS |
 * |--------|-----|-----|
 * | Data structure | Queue | Stack |
 * | Path found | Shortest | Any path |
 * | Memory | O(width) | O(depth) |
 * | Complete | Yes | Yes (finite graphs) |
 * 
 * ## Applications
 * 
 * ### BFS Applications
 * - Finding shortest path (unweighted)
 * - Level-order traversal
 * - Finding connected components
 * - Social network friends at distance k
 * 
 * ### DFS Applications
 * - Topological sorting
 * - Cycle detection
 * - Path finding (any path)
 * - Maze generation/solving
 * - Finding strongly connected components
 * 
 * @see graph-traverse.H
 * @see tpl_find_path.H
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tpl_graph.H>
#include <graph-traverse.H>
#include <tpl_find_path.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Graph types
using Node = Graph_Node<string>;
using Arc = Graph_Arc<int>;
using Graph = List_Graph<Node, Arc>;

/**
 * @brief Build a sample social network graph
 * 
 *         Alice
 *        /     \
 *     Bob --- Charlie
 *      |         |
 *    Diana --- Eve
 *      |
 *    Frank --- Grace
 *              |
 *            Henry
 */
Graph build_social_network()
{
  Graph g;
  
  auto alice = g.insert_node("Alice");
  auto bob = g.insert_node("Bob");
  auto charlie = g.insert_node("Charlie");
  auto diana = g.insert_node("Diana");
  auto eve = g.insert_node("Eve");
  auto frank = g.insert_node("Frank");
  auto grace = g.insert_node("Grace");
  auto henry = g.insert_node("Henry");
  
  // Friendships (undirected)
  g.insert_arc(alice, bob);
  g.insert_arc(alice, charlie);
  g.insert_arc(bob, charlie);
  g.insert_arc(bob, diana);
  g.insert_arc(charlie, eve);
  g.insert_arc(diana, eve);
  g.insert_arc(diana, frank);
  g.insert_arc(frank, grace);
  g.insert_arc(grace, henry);
  
  return g;
}

/**
 * @brief Build a tree-like graph for clear traversal comparison
 *
 *              1
 *            / | \
 *           2  3  4
 *          /|     |
 *         5 6     7
 *        /|\
 *       8 9 10
 */
Graph build_tree_graph()
{
  Graph g;
  
  vector<Graph::Node*> nodes;
  for (int i = 1; i <= 10; ++i)
    nodes.push_back(g.insert_node(to_string(i)));
  
  // Tree structure
  g.insert_arc(nodes[0], nodes[1]);  // 1-2
  g.insert_arc(nodes[0], nodes[2]);  // 1-3
  g.insert_arc(nodes[0], nodes[3]);  // 1-4
  g.insert_arc(nodes[1], nodes[4]);  // 2-5
  g.insert_arc(nodes[1], nodes[5]);  // 2-6
  g.insert_arc(nodes[3], nodes[6]);  // 4-7
  g.insert_arc(nodes[4], nodes[7]);  // 5-8
  g.insert_arc(nodes[4], nodes[8]);  // 5-9
  g.insert_arc(nodes[4], nodes[9]);  // 5-10
  
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
  
  cout << "\nAdjacency list:" << endl;
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
 * @brief Demonstrate BFS traversal
 */
void demo_bfs(Graph& g, Graph::Node* start)
{
  cout << "\n--- BFS (Breadth-First Search) ---" << endl;
  cout << "Uses: Queue (FIFO)" << endl;
  cout << "Explores: Level by level" << endl;
  cout << "Starting from: " << start->get_info() << endl;
  
  cout << "\nVisit order: ";
  
  int visit_num = 0;
  auto visitor = [&visit_num](Graph::Node* node) {
    if (visit_num > 0) cout << " -> ";
    cout << node->get_info();
    ++visit_num;
    return true;  // Continue traversal
  };
  
  // BFS uses DynListQueue
  Graph_Traverse<Graph, Node_Arc_Iterator<Graph>, DynListQueue> bfs(g);
  size_t visited = bfs(start, visitor);
  
  cout << endl;
  cout << "Total nodes visited: " << visited << endl;
}

/**
 * @brief Demonstrate DFS traversal
 */
void demo_dfs(Graph& g, Graph::Node* start)
{
  cout << "\n--- DFS (Depth-First Search) ---" << endl;
  cout << "Uses: Stack (LIFO)" << endl;
  cout << "Explores: As deep as possible first" << endl;
  cout << "Starting from: " << start->get_info() << endl;
  
  cout << "\nVisit order: ";
  
  int visit_num = 0;
  auto visitor = [&visit_num](Graph::Node* node) {
    if (visit_num > 0) cout << " -> ";
    cout << node->get_info();
    ++visit_num;
    return true;
  };
  
  // DFS uses DynListStack
  Graph_Traverse<Graph, Node_Arc_Iterator<Graph>, DynListStack> dfs(g);
  size_t visited = dfs(start, visitor);
  
  cout << endl;
  cout << "Total nodes visited: " << visited << endl;
}

/**
 * @brief Compare BFS and DFS on the same graph
 */
void demo_comparison()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "BFS vs DFS: Side-by-Side Comparison" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_tree_graph();
  print_graph(g, "Tree Graph");
  
  auto* root = find_node(g, "1");
  
  demo_bfs(g, root);
  demo_dfs(g, root);
  
  cout << "\n--- Analysis ---" << endl;
  cout << "BFS visits nodes level by level: 1, then 2-3-4, then 5-6-7, etc." << endl;
  cout << "DFS explores one branch completely before backtracking." << endl;
}

/**
 * @brief Practical example: Finding degrees of separation
 */
void demo_degrees_of_separation()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Degrees of Separation (BFS)" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_social_network();
  print_graph(g, "Social Network");
  
  auto* alice = find_node(g, "Alice");
  auto* henry = find_node(g, "Henry");
  
  cout << "\nFinding shortest path from Alice to Henry..." << endl;
  
  // Use BFS to find shortest path
  Find_Path_Breadth_First<Graph> path_finder;
  Path<Graph> path = path_finder(g, alice, henry);
  
  if (path.size() > 0)
  {
    cout << "Path found! Degrees of separation: " << (path.size() - 1) << endl;
    cout << "Connection: ";
    
    bool first = true;
    for (auto it = path.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << " -> ";
      cout << it.get_curr()->get_info();
      first = false;
    }
    cout << endl;
  }
  else
  {
    cout << "No path found!" << endl;
  }
  
  cout << "\nNote: BFS guarantees finding the shortest path (fewest edges)." << endl;
}

/**
 * @brief Practical example: Finding any path (DFS)
 */
void demo_any_path()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Finding Any Path (DFS)" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_social_network();
  
  auto* alice = find_node(g, "Alice");
  auto* henry = find_node(g, "Henry");
  
  cout << "\nFinding a path (any path) from Alice to Henry using DFS..." << endl;
  
  Find_Path_Depth_First<Graph> path_finder;
  Path<Graph> path = path_finder(g, alice, henry);
  
  if (path.size() > 0)
  {
    cout << "Path found (may not be shortest): " << endl;
    cout << "Length: " << (path.size() - 1) << " edges" << endl;
    cout << "Path: ";
    
    bool first = true;
    for (auto it = path.get_it(); it.has_curr(); it.next())
    {
      if (not first) cout << " -> ";
      cout << it.get_curr()->get_info();
      first = false;
    }
    cout << endl;
  }
  
  cout << "\nNote: DFS doesn't guarantee shortest path, but uses less memory" << endl;
  cout << "      on deep graphs and can be useful for exploring all possibilities." << endl;
}

/**
 * @brief Demonstrate early termination
 */
void demo_early_termination()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Early Termination: Stop When Target Found" << endl;
  cout << string(60, '=') << endl;
  
  Graph g = build_social_network();
  
  auto* alice = find_node(g, "Alice");
  string target = "Eve";
  
  cout << "\nSearching for '" << target << "' starting from 'Alice'..." << endl;
  
  Graph::Node* found_node = nullptr;
  int nodes_visited = 0;
  
  auto search_visitor = [&](Graph::Node* node) {
    ++nodes_visited;
    cout << "  Visiting: " << node->get_info() << endl;
    
    if (node->get_info() == target)
    {
      found_node = node;
      return false;  // Stop traversal
    }
    return true;  // Continue
  };
  
  cout << "\nUsing BFS:" << endl;
  found_node = nullptr;
  nodes_visited = 0;
  Graph_Traverse<Graph, Node_Arc_Iterator<Graph>, DynListQueue> bfs(g);
  bfs(alice, search_visitor);
  cout << "Nodes visited before finding '" << target << "': " << nodes_visited << endl;
  
  cout << "\nUsing DFS:" << endl;
  found_node = nullptr;
  nodes_visited = 0;
  Graph_Traverse<Graph, Node_Arc_Iterator<Graph>, DynListStack> dfs(g);
  dfs(alice, search_visitor);
  cout << "Nodes visited before finding '" << target << "': " << nodes_visited << endl;
  
  cout << "\nNote: BFS may find closer targets faster, DFS may explore more." << endl;
}

/**
 * @brief Demonstrate finding connected components
 */
void demo_connected_components()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Practical Example: Finding Connected Components" << endl;
  cout << string(60, '=') << endl;
  
  Graph g;
  
  // Component 1: A-B-C
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  
  // Component 2: D-E
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  g.insert_arc(d, e);
  
  // Component 3: F (isolated)
  g.insert_node("F");
  
  print_graph(g, "Graph with Multiple Components");
  
  cout << "\nFinding connected components using BFS..." << endl;
  
  // Mark all nodes as unvisited
  g.reset_nodes();
  
  int component_num = 0;
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* node = nit.get_curr();
    if (NODE_BITS(node).get_bit(Aleph::Depth_First))
      continue;  // Already visited
    
    ++component_num;
    cout << "\nComponent " << component_num << ": ";
    
    bool first = true;
    auto visitor = [&first](Graph::Node* n) {
      if (not first) cout << ", ";
      cout << n->get_info();
      NODE_BITS(n).set_bit(Aleph::Depth_First, true);
      first = false;
      return true;
    };
    
    Graph_Traverse<Graph, Node_Arc_Iterator<Graph>, DynListQueue> bfs(g);
    bfs(node, visitor);
    cout << endl;
  }
  
  cout << "\nTotal components: " << component_num << endl;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("BFS/DFS Graph Traversal Example", ' ', "1.0");
    
    TCLAP::SwitchArg compareArg("c", "compare",
      "Compare BFS and DFS on same graph", false);
    TCLAP::SwitchArg degreesArg("d", "degrees",
      "Show degrees of separation example", false);
    TCLAP::SwitchArg pathArg("p", "path",
      "Show any path example (DFS)", false);
    TCLAP::SwitchArg earlyArg("e", "early",
      "Show early termination example", false);
    TCLAP::SwitchArg compArg("o", "components",
      "Show connected components example", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    
    cmd.add(compareArg);
    cmd.add(degreesArg);
    cmd.add(pathArg);
    cmd.add(earlyArg);
    cmd.add(compArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    bool runCompare = compareArg.getValue();
    bool runDegrees = degreesArg.getValue();
    bool runPath = pathArg.getValue();
    bool runEarly = earlyArg.getValue();
    bool runComp = compArg.getValue();
    bool runAll = allArg.getValue();
    
    if (not runCompare and not runDegrees and not runPath and 
        not runEarly and not runComp)
      runAll = true;
    
    cout << "=== Graph Traversal: BFS vs DFS ===" << endl;
    cout << "BFS: Breadth-First (Queue) - Finds shortest paths" << endl;
    cout << "DFS: Depth-First (Stack) - Explores deeply first" << endl;
    
    if (runAll or runCompare)
      demo_comparison();
    
    if (runAll or runDegrees)
      demo_degrees_of_separation();
    
    if (runAll or runPath)
      demo_any_path();
    
    if (runAll or runEarly)
      demo_early_termination();
    
    if (runAll or runComp)
      demo_connected_components();
    
    cout << "\n=== Summary ===" << endl;
    cout << "BFS: Use when shortest path matters (unweighted graphs)" << endl;
    cout << "DFS: Use for topological sort, cycle detection, or when any path suffices" << endl;
    cout << "Both: O(V + E) time complexity" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

