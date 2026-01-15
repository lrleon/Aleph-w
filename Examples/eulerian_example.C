/**
 * @file eulerian_example.C
 * @brief Example demonstrating Eulerian path and cycle detection in Aleph-w
 *
 * This program demonstrates Eulerian path and cycle detection using
 * `eulerian.H`. An Eulerian path/cycle visits every **edge** exactly once,
 * which is different from Hamiltonian paths (which visit every **vertex**).
 *
 * ## What is an Eulerian Path/Cycle?
 *
### Eulerian Path
 *
 * An **Eulerian path** is a path that visits every edge exactly once.
 * The path may start and end at different vertices.
 *
### Eulerian Cycle
 *
 * An **Eulerian cycle** (or circuit) is an Eulerian path that starts and
 * ends at the same vertex, forming a cycle.
 *
### Key Difference from Hamiltonian
 *
 * | Property | Eulerian | Hamiltonian |
 * |----------|----------|------------|
 * | Visits | Every **edge** once | Every **vertex** once |
 * | Complexity | Polynomial O(V+E) | NP-complete |
 * | Test | Exact conditions | Only sufficiency |
 *
## Eulerian Conditions
 *
### Undirected Graphs
 *
#### Eulerian Cycle
 * **Condition**: All vertices have **EVEN** degree
 *
 * **Why**: To form a cycle, you must enter and leave each vertex equally.
 * Even degree ensures this is possible.
 *
#### Eulerian Path
 * **Condition**: Exactly **0 or 2** vertices have **ODD** degree
 *
 * **Why**: 
 * - 0 odd vertices: Cycle exists (start = end)
 * - 2 odd vertices: Path exists (start and end are the odd-degree vertices)
 * - More than 2 odd vertices: Impossible (can't have more than 2 endpoints)
 *
### Directed Graphs
 *
#### Eulerian Cycle
 * **Condition**: For **ALL** vertices: `in-degree = out-degree`
 *
 * **Why**: Must enter and leave each vertex equally.
 *
#### Eulerian Path
 * **Condition**: 
 * - At most **1** vertex with `out-degree - in-degree = 1` (start)
 * - At most **1** vertex with `in-degree - out-degree = 1` (end)
 * - All other vertices: `in-degree = out-degree`
 *
## Finding Eulerian Path/Cycle
 *
### Algorithm: Hierholzer's Algorithm
 *
 * ```
 * Find_Eulerian_Path(G):
 *   1. Check if Eulerian path/cycle exists (conditions above)
 *   2. Choose start vertex:
 *      - Cycle: any vertex
 *      - Path: vertex with odd degree (or out > in for directed)
 *   3. DFS to find a cycle starting from start
 *   4. While unvisited edges remain:
 *      - Find vertex in current path with unvisited edges
 *      - Find cycle starting from that vertex
 *      - Merge cycles
 *   5. Return Eulerian path/cycle
 * ```
 *
 * **Time complexity**: O(E) - visit each edge once
 *
## Historical Context: Königsberg Bridges
 *
### The Problem (1736)
 *
 * The city of Königsberg had 7 bridges connecting 4 land areas:
 * ```
 *    A
 *   /|\
 *  / | \
 * B--+--C
 *  \ | /
 *   \|/
 *    D
 * ```
 *
 * **Question**: Can you walk through the city crossing each bridge exactly once?
 *
### Euler's Solution
 *
 * Leonhard Euler proved this is **impossible** by:
 * 1. Modeling as a graph (land areas = vertices, bridges = edges)
 * 2. Showing all vertices have odd degree (3, 3, 3, 3)
 * 3. Since more than 2 vertices have odd degree, no Eulerian path exists
 *
 * **This was the birth of graph theory!**
 *
## Applications
 *
### Route Planning
 * - **Postal routes**: Deliver mail efficiently (Chinese Postman Problem)
 * - **Garbage collection**: Collect garbage from all streets
 * - **Snow plowing**: Plow all streets efficiently
 *
### Network Design
 * - **Circuit design**: Design circuits visiting all connections
 * - **Network testing**: Test all network links
 * - **Traffic flow**: Optimize traffic routes
 *
### DNA Sequencing
 * - **Genome assembly**: Reconstruct genome from fragments
 * - **Read alignment**: Align sequencing reads
 *
### Puzzle Solving
 * - **Mazes**: Find path visiting all passages
 * - **Puzzle games**: Solve path-finding puzzles
 *
## Complexity
 *
 * | Operation | Time | Space |
 * |-----------|------|-------|
 * | Test Eulerian | O(V + E) | O(V) |
 * | Find Eulerian path | O(E) | O(E) |
 * | Find Eulerian cycle | O(E) | O(E) |
 *
## Example: Undirected Graph
 *
 * ```
 * Graph:
 *   A---B
 *   |\ /|
 *   | X |
 *   |/ \|
 *   C---D
 *
 * Degrees: A=3, B=3, C=3, D=3 (all odd!)
 * Result: No Eulerian path (4 odd vertices > 2)
 * ```
 *
 * **Modify**: Add one edge to make two vertices even:
 * ```
 *   A---B---E
 *   |\ /|
 *   | X |
 *   |/ \|
 *   C---D
 *
 * Degrees: A=3, B=4, C=3, D=3, E=1
 * Result: Eulerian path exists (A and E are odd)
 * ```
 *
## Usage
 *
 * ```bash
 * # Run all Eulerian demonstrations
 * ./eulerian_example
 *
 * # Run specific demo
 * ./eulerian_example -s cycle    # Cycle detection
 * ./eulerian_example -s path    # Path detection
 * ./eulerian_example -s konigsberg  # Historical example
 * ```
 *
 * @see eulerian.H Eulerian graph algorithms
 * @see hamiltonian_example.C Hamiltonian path/cycle (visits vertices)
 * @see bfs_dfs_example.C Graph traversal (used in algorithms)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

#include <tclap/CmdLine.h>

#include <tpl_graph.H>
#include <eulerian.H>

using namespace std;
using namespace Aleph;

// Graph types
using SNode = Graph_Node<string>;
using IArc = Graph_Arc<int>;
using UGraph = List_Graph<SNode, IArc>;
using DGraph = List_Digraph<SNode, IArc>;

// =============================================================================
// Helper functions
// =============================================================================

void print_section(const string& title)
{
  cout << "\n" << string(60, '=') << "\n";
  cout << "  " << title << "\n";
  cout << string(60, '=') << "\n\n";
}

void print_subsection(const string& title)
{
  cout << "\n--- " << title << " ---\n";
}

template <typename G>
void print_graph(const string& label, G& g)
{
  cout << label << ":" << endl;
  cout << "  Nodes: " << g.get_num_nodes() << endl;
  cout << "  Arcs: " << g.get_num_arcs() << endl;
  
  cout << "  Vertices:" << endl;
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    auto node = it.get_curr();
    cout << "    " << node->get_info();
    
    // Count degree
    size_t degree = 0;
    for (Node_Arc_Iterator<G> ait(node); ait.has_curr(); ait.next())
      degree++;
    
    cout << " (degree=" << degree << ")" << endl;
  }
  
  cout << "  Edges:" << endl;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    cout << "    " << g.get_src_node(arc)->get_info() 
         << " -- " << g.get_tgt_node(arc)->get_info() << endl;
  }
}

// =============================================================================
// 1. Eulerian Cycle Detection
// =============================================================================

void demo_eulerian_cycle()
{
  print_section("EULERIAN CYCLE DETECTION");
  
  cout << "An Eulerian CYCLE visits every edge exactly once and returns to start.\n";
  cout << "Condition (undirected): ALL vertices must have EVEN degree.\n\n";
  
  // Graph 1: Triangle (Eulerian)
  print_subsection("Example 1: Triangle (Eulerian)");
  
  UGraph triangle;
  auto a = triangle.insert_node("A");
  auto b = triangle.insert_node("B");
  auto c = triangle.insert_node("C");
  triangle.insert_arc(a, b, 1);
  triangle.insert_arc(b, c, 1);
  triangle.insert_arc(c, a, 1);
  
  print_graph("Triangle", triangle);
  cout << "\nAll vertices have degree 2 (even)." << endl;
  
  Test_Eulerian<UGraph> test1;
  cout << "Is Eulerian? " << (test1(triangle) ? "YES" : "NO") << endl;
  cout << "Eulerian cycle: A -> B -> C -> A" << endl;
  
  // Graph 2: Square with diagonals (Eulerian)
  print_subsection("Example 2: Square with diagonals (Eulerian)");
  
  UGraph square;
  auto s1 = square.insert_node("1");
  auto s2 = square.insert_node("2");
  auto s3 = square.insert_node("3");
  auto s4 = square.insert_node("4");
  // Square edges
  square.insert_arc(s1, s2, 1);
  square.insert_arc(s2, s3, 1);
  square.insert_arc(s3, s4, 1);
  square.insert_arc(s4, s1, 1);
  // Diagonals
  square.insert_arc(s1, s3, 1);
  square.insert_arc(s2, s4, 1);
  
  print_graph("Square with diagonals", square);
  cout << "\nAll vertices have degree 4 (even)." << endl;
  
  Test_Eulerian<UGraph> test2;
  cout << "Is Eulerian? " << (test2(square) ? "YES" : "NO") << endl;
  
  // Graph 3: Path (NOT Eulerian)
  print_subsection("Example 3: Simple Path (NOT Eulerian)");
  
  UGraph path;
  auto p1 = path.insert_node("X");
  auto p2 = path.insert_node("Y");
  auto p3 = path.insert_node("Z");
  path.insert_arc(p1, p2, 1);
  path.insert_arc(p2, p3, 1);
  
  print_graph("Simple path", path);
  cout << "\nX has degree 1 (odd), Z has degree 1 (odd)." << endl;
  
  Test_Eulerian<UGraph> test3;
  cout << "Is Eulerian? " << (test3(path) ? "YES" : "NO") << endl;
  cout << "Cannot return to start without reusing edges." << endl;
}

// =============================================================================
// 2. The Seven Bridges of Königsberg
// =============================================================================

void demo_konigsberg()
{
  print_section("THE SEVEN BRIDGES OF KÖNIGSBERG");
  
  cout << "The famous problem that started graph theory (Euler, 1736).\n\n";
  cout << "Can you cross all 7 bridges exactly once and return to start?\n\n";
  
  cout << "The city of Königsberg (now Kaliningrad) had:\n";
  cout << "  - 4 land masses (A, B, C, D)\n";
  cout << "  - 7 bridges connecting them\n\n";
  
  /*
   *        A
   *       /|\
   *      / | \
   *     /  |  \     (2 bridges A-B, 2 bridges A-C)
   *    B---+---C    (1 bridge A-D, 1 bridge B-D, 1 bridge C-D)
   *     \  |  /
   *      \ | /
   *       \|/
   *        D
   */
  
  UGraph konigsberg;
  auto kA = konigsberg.insert_node("A");  // North bank
  auto kB = konigsberg.insert_node("B");  // West island
  auto kC = konigsberg.insert_node("C");  // East island  
  auto kD = konigsberg.insert_node("D");  // South bank
  
  // 7 bridges
  konigsberg.insert_arc(kA, kB, 1);  // Bridge 1: A-B
  konigsberg.insert_arc(kA, kB, 2);  // Bridge 2: A-B (second)
  konigsberg.insert_arc(kA, kC, 1);  // Bridge 3: A-C
  konigsberg.insert_arc(kA, kC, 2);  // Bridge 4: A-C (second)
  konigsberg.insert_arc(kA, kD, 1);  // Bridge 5: A-D
  konigsberg.insert_arc(kB, kD, 1);  // Bridge 6: B-D
  konigsberg.insert_arc(kC, kD, 1);  // Bridge 7: C-D
  
  cout << "Graph representation:" << endl;
  cout << "  Vertices (land masses): A, B, C, D" << endl;
  cout << "  Edges (bridges): 7" << endl;
  cout << "\nDegrees:" << endl;
  cout << "  A: degree 5 (ODD)" << endl;
  cout << "  B: degree 3 (ODD)" << endl;
  cout << "  C: degree 3 (ODD)" << endl;
  cout << "  D: degree 3 (ODD)" << endl;
  
  Test_Eulerian<UGraph> test;
  cout << "\nIs Eulerian (can return to start)? " << (test(konigsberg) ? "YES" : "NO") << endl;
  
  cout << "\nEuler proved: With 4 odd-degree vertices, it's IMPOSSIBLE!\n";
  cout << "For an Eulerian cycle, ALL vertices must have even degree.\n";
  cout << "For an Eulerian path, exactly 0 or 2 vertices can have odd degree.\n";
}

// =============================================================================
// 3. Directed Graph Eulerian
// =============================================================================

void demo_directed()
{
  print_section("DIRECTED GRAPH EULERIAN");
  
  cout << "For directed graphs, the condition is different:\n";
  cout << "  in-degree = out-degree for EVERY vertex\n\n";
  
  // Eulerian directed graph
  print_subsection("Example 1: Directed cycle (Eulerian)");
  
  DGraph dcycle;
  auto d1 = dcycle.insert_node("1");
  auto d2 = dcycle.insert_node("2");
  auto d3 = dcycle.insert_node("3");
  dcycle.insert_arc(d1, d2, 1);
  dcycle.insert_arc(d2, d3, 1);
  dcycle.insert_arc(d3, d1, 1);
  
  cout << "Directed cycle: 1 -> 2 -> 3 -> 1" << endl;
  cout << "  Node 1: in=1, out=1" << endl;
  cout << "  Node 2: in=1, out=1" << endl;
  cout << "  Node 3: in=1, out=1" << endl;
  
  Test_Eulerian<DGraph> dtest1;
  cout << "\nIs Eulerian? " << (dtest1(dcycle) ? "YES" : "NO") << endl;
  
  // Non-Eulerian directed graph
  print_subsection("Example 2: Directed path (NOT Eulerian)");
  
  DGraph dpath;
  auto dp1 = dpath.insert_node("A");
  auto dp2 = dpath.insert_node("B");
  auto dp3 = dpath.insert_node("C");
  dpath.insert_arc(dp1, dp2, 1);
  dpath.insert_arc(dp2, dp3, 1);
  
  cout << "Directed path: A -> B -> C" << endl;
  cout << "  Node A: in=0, out=1 (UNBALANCED)" << endl;
  cout << "  Node B: in=1, out=1" << endl;
  cout << "  Node C: in=1, out=0 (UNBALANCED)" << endl;
  
  Test_Eulerian<DGraph> dtest2;
  cout << "\nIs Eulerian? " << (dtest2(dpath) ? "YES" : "NO") << endl;
  
  // Complex Eulerian digraph
  print_subsection("Example 3: Figure-8 (Eulerian)");
  
  DGraph fig8;
  auto f1 = fig8.insert_node("Center");
  auto f2 = fig8.insert_node("Top");
  auto f3 = fig8.insert_node("Bottom");
  // Upper loop
  fig8.insert_arc(f1, f2, 1);
  fig8.insert_arc(f2, f1, 1);
  // Lower loop
  fig8.insert_arc(f1, f3, 1);
  fig8.insert_arc(f3, f1, 1);
  
  cout << "Figure-8 shape:" << endl;
  cout << "  Center: in=2, out=2" << endl;
  cout << "  Top: in=1, out=1" << endl;
  cout << "  Bottom: in=1, out=1" << endl;
  
  Test_Eulerian<DGraph> dtest3;
  cout << "\nIs Eulerian? " << (dtest3(fig8) ? "YES" : "NO") << endl;
}

// =============================================================================
// 4. Practical Applications
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL APPLICATIONS");
  
  // 1. Chinese Postman Problem (simplified)
  print_subsection("Application 1: Mail Delivery Route");
  
  cout << "A mail carrier wants to visit every street exactly once.\n";
  cout << "This is the Eulerian path/cycle problem!\n\n";
  
  UGraph streets;
  auto h1 = streets.insert_node("Casa1");
  auto h2 = streets.insert_node("Casa2");
  auto h3 = streets.insert_node("Casa3");
  auto h4 = streets.insert_node("Casa4");
  auto office = streets.insert_node("Correo");
  
  // Create a grid-like structure
  streets.insert_arc(office, h1, 1);
  streets.insert_arc(office, h2, 1);
  streets.insert_arc(h1, h2, 1);
  streets.insert_arc(h1, h3, 1);
  streets.insert_arc(h2, h4, 1);
  streets.insert_arc(h3, h4, 1);
  streets.insert_arc(h3, office, 1);
  streets.insert_arc(h4, office, 1);
  
  print_graph("Mail route", streets);
  
  Test_Eulerian<UGraph> mail_test;
  if (mail_test(streets))
    cout << "\nPerfect! The mail carrier can visit every street exactly once\n"
         << "and return to the post office!\n";
  else
    cout << "\nSome streets must be visited more than once.\n";
  
  // 2. Circuit Board Design
  print_subsection("Application 2: Circuit Board Routing");
  
  cout << "Draw all connections without lifting the pen?\n";
  cout << "This is an Eulerian path problem!\n\n";
  
  UGraph circuit;
  auto pin1 = circuit.insert_node("Pin1");
  auto pin2 = circuit.insert_node("Pin2");
  auto pin3 = circuit.insert_node("Pin3");
  auto pin4 = circuit.insert_node("Pin4");
  
  // Non-Eulerian connections
  circuit.insert_arc(pin1, pin2, 1);
  circuit.insert_arc(pin2, pin3, 1);
  circuit.insert_arc(pin3, pin4, 1);
  circuit.insert_arc(pin4, pin1, 1);
  circuit.insert_arc(pin1, pin3, 1);  // Diagonal
  
  cout << "Circuit with 5 connections:" << endl;
  cout << "  Pin1-Pin2, Pin2-Pin3, Pin3-Pin4, Pin4-Pin1, Pin1-Pin3" << endl;
  
  // Count odd-degree vertices
  size_t odd_count = 0;
  for (auto it = circuit.get_node_it(); it.has_curr(); it.next())
  {
    size_t degree = 0;
    for (Node_Arc_Iterator<UGraph> ait(it.get_curr()); ait.has_curr(); ait.next())
      degree++;
    if (degree % 2 == 1) odd_count++;
  }
  
  cout << "Vertices with odd degree: " << odd_count << endl;
  
  if (odd_count == 0)
    cout << "Can draw all connections returning to start (Eulerian cycle)!\n";
  else if (odd_count == 2)
    cout << "Can draw all connections but not return to start (Eulerian path).\n";
  else
    cout << "Cannot draw without lifting pen - need " << (odd_count/2) << " extra strokes.\n";
}

// =============================================================================
// 5. Finding Eulerian Paths with Hierholzer's Algorithm
// =============================================================================

void demo_hierholzer()
{
  print_section("HIERHOLZER'S ALGORITHM: Finding Eulerian Paths");
  
  cout << "Hierholzer's algorithm constructs an Eulerian path/cycle in O(E) time.\n";
  cout << "Instead of just testing existence, it finds the actual path!\n\n";
  
  // Triangle - Eulerian cycle
  print_subsection("Example 1: Triangle (find the cycle)");
  
  UGraph triangle;
  auto a = triangle.insert_node("A");
  auto b = triangle.insert_node("B");
  auto c = triangle.insert_node("C");
  triangle.insert_arc(a, b, 1);
  triangle.insert_arc(b, c, 1);
  triangle.insert_arc(c, a, 1);
  
  Find_Eulerian_Path<UGraph> finder1;
  auto result1 = finder1(triangle);
  
  cout << "Triangle graph: A-B-C\n";
  cout << "Classification: ";
  switch (result1.type) {
    case Eulerian_Type::Cycle: cout << "EULERIAN CYCLE\n"; break;
    case Eulerian_Type::Path:  cout << "EULERIAN PATH\n"; break;
    case Eulerian_Type::None:  cout << "NOT EULERIAN\n"; break;
  }
  
  cout << "Path found (" << result1.path.size() << " edges):\n  ";
  
  auto nodes1 = finder1.find_node_sequence(triangle);
  bool first = true;
  for (auto node : nodes1) {
    if (!first) cout << " -> ";
    cout << node->get_info();
    first = false;
  }
  cout << endl;
  
  // Path graph - Eulerian path (not cycle)
  print_subsection("Example 2: Path graph (Eulerian path, not cycle)");
  
  UGraph path;
  auto p1 = path.insert_node("1");
  auto p2 = path.insert_node("2");
  auto p3 = path.insert_node("3");
  auto p4 = path.insert_node("4");
  path.insert_arc(p1, p2, 1);
  path.insert_arc(p2, p3, 1);
  path.insert_arc(p3, p4, 1);
  
  Find_Eulerian_Path<UGraph> finder2;
  auto result2 = finder2(path);
  
  cout << "Linear path: 1-2-3-4\n";
  cout << "Classification: ";
  switch (result2.type) {
    case Eulerian_Type::Cycle: cout << "EULERIAN CYCLE\n"; break;
    case Eulerian_Type::Path:  cout << "EULERIAN PATH\n"; break;
    case Eulerian_Type::None:  cout << "NOT EULERIAN\n"; break;
  }
  
  if (result2.type != Eulerian_Type::None) {
    cout << "Path found (" << result2.path.size() << " edges):\n  ";
    auto nodes2 = finder2.find_node_sequence(path);
    first = true;
    for (auto node : nodes2) {
      if (!first) cout << " -> ";
      cout << node->get_info();
      first = false;
    }
    cout << endl;
  }
  
  // Bow-tie graph - complex Eulerian cycle
  print_subsection("Example 3: Bow-tie graph (two triangles sharing a vertex)");
  
  UGraph bowtie;
  auto center = bowtie.insert_node("Center");
  auto top1 = bowtie.insert_node("Top1");
  auto top2 = bowtie.insert_node("Top2");
  auto bot1 = bowtie.insert_node("Bot1");
  auto bot2 = bowtie.insert_node("Bot2");
  
  // Upper triangle
  bowtie.insert_arc(center, top1, 1);
  bowtie.insert_arc(top1, top2, 1);
  bowtie.insert_arc(top2, center, 1);
  // Lower triangle
  bowtie.insert_arc(center, bot1, 1);
  bowtie.insert_arc(bot1, bot2, 1);
  bowtie.insert_arc(bot2, center, 1);
  
  Find_Eulerian_Path<UGraph> finder3;
  auto result3 = finder3(bowtie);
  
  cout << "Bow-tie: Two triangles sharing 'Center'\n";
  cout << "  Center has degree 4 (even)\n";
  cout << "  All others have degree 2 (even)\n";
  cout << "Classification: ";
  switch (result3.type) {
    case Eulerian_Type::Cycle: cout << "EULERIAN CYCLE\n"; break;
    case Eulerian_Type::Path:  cout << "EULERIAN PATH\n"; break;
    case Eulerian_Type::None:  cout << "NOT EULERIAN\n"; break;
  }
  
  cout << "Path found (" << result3.path.size() << " edges):\n  ";
  auto nodes3 = finder3.find_node_sequence(bowtie);
  first = true;
  for (auto node : nodes3) {
    if (!first) cout << " -> ";
    cout << node->get_info();
    first = false;
  }
  cout << endl;
  
  cout << "\nHierholzer's algorithm visits both triangles, returning to start!\n";
}

// =============================================================================
// 6. Using Eulerian_Type enum
// =============================================================================

void demo_eulerian_type()
{
  print_section("EULERIAN CLASSIFICATION WITH compute()");
  
  cout << "The compute() method returns detailed classification:\n";
  cout << "  - Eulerian_Type::Cycle - Has Eulerian cycle\n";
  cout << "  - Eulerian_Type::Path  - Has Eulerian path but not cycle\n";
  cout << "  - Eulerian_Type::None  - Not Eulerian\n\n";
  
  // Test various graphs
  struct TestCase {
    string name;
    UGraph g;
    vector<pair<string, string>> edges;
  };
  
  vector<TestCase> tests;
  
  // Triangle (cycle)
  TestCase t1{"Triangle", {}, {{"A","B"},{"B","C"},{"C","A"}}};
  // Path (path only)
  TestCase t2{"Path 1-2-3", {}, {{"1","2"},{"2","3"}}};
  // Star (none)
  TestCase t3{"Star", {}, {{"C","1"},{"C","2"},{"C","3"},{"C","4"}}};
  
  tests.push_back(t1);
  tests.push_back(t2);
  tests.push_back(t3);
  
  cout << setw(20) << "Graph" << setw(15) << "Result" << setw(20) << "has_eulerian_path()" << endl;
  cout << string(55, '-') << endl;
  
  for (auto& tc : tests) {
    // Build graph
    map<string, UGraph::Node*> node_map;
    for (auto& [u, v] : tc.edges) {
      if (node_map.find(u) == node_map.end())
        node_map[u] = tc.g.insert_node(u);
      if (node_map.find(v) == node_map.end())
        node_map[v] = tc.g.insert_node(v);
      tc.g.insert_arc(node_map[u], node_map[v], 1);
    }
    
    Test_Eulerian<UGraph> tester;
    auto result = tester.compute(tc.g);
    
    string result_str;
    switch (result) {
      case Eulerian_Type::Cycle: result_str = "CYCLE"; break;
      case Eulerian_Type::Path:  result_str = "PATH"; break;
      case Eulerian_Type::None:  result_str = "NONE"; break;
    }
    
    cout << setw(20) << tc.name 
         << setw(15) << result_str 
         << setw(20) << (tester.has_eulerian_path(tc.g) ? "true" : "false") << endl;
  }
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Eulerian graph example for Aleph-w.\n"
      "Demonstrates Eulerian path and cycle detection.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run specific section: cycle, konigsberg, directed, practical, hierholzer, types, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W EULERIAN GRAPHS EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "cycle")
      demo_eulerian_cycle();
    
    if (section == "all" or section == "konigsberg")
      demo_konigsberg();
    
    if (section == "all" or section == "directed")
      demo_directed();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    if (section == "all" or section == "hierholzer")
      demo_hierholzer();
    
    if (section == "all" or section == "types")
      demo_eulerian_type();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Eulerian graphs demo completed!\n";
    cout << string(60, '=') << "\n\n";
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
    return 1;
  }
  catch (exception& e)
  {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
}

