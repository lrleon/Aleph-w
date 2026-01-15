/**
 * @file bipartite_example.C
 * @brief Example demonstrating bipartite graphs in Aleph-w
 *
 * A bipartite graph is a graph whose vertices can be divided into two
 * disjoint sets such that every edge connects a vertex from one set to
 * a vertex in the other set. No edges exist within the same set.
 *
 * ## What is a Bipartite Graph?
 *
 * ### Definition
 *
 * A graph G = (V, E) is **bipartite** if V can be partitioned into
 * two sets U and W such that:
 * - Every edge connects a vertex in U to a vertex in W
 * - No edge connects two vertices in the same set
 *
 * ### Visual Example
 *
 * ```
 * Set U:    A --- B
 *           |     |
 * Set W:    C --- D
 *
 * Edges: A-C, A-D, B-C, B-D (all cross between sets)
 * ```
 *
 * ## Key Properties
 *
 * ### 2-Coloring
 *
 * **Bipartite ⟺ 2-colorable**
 *
 * A graph is bipartite if and only if it can be colored with 2 colors
 * such that no two adjacent vertices have the same color.
 *
 * **Algorithm**: Use BFS/DFS to assign colors, check for conflicts
 *
 * ### No Odd Cycles
 *
 * **Bipartite ⟺ No cycles of odd length**
 *
 * A graph is bipartite if and only if it contains no cycle of odd length.
 *
 * **Proof idea**: In a bipartite graph, any cycle must alternate between
 * the two sets, requiring even length.
 *
 * ### Matching
 *
 * A **matching** is a set of edges with no shared vertices.
 *
 * - **Maximum matching**: Largest possible matching
 * - **Perfect matching**: Every vertex is matched (requires |U| = |W|)
 *
 * ## Testing Bipartiteness
 *
 * ### Algorithm
 *
 * ```
 * Test_Bipartite(G):
 *   1. Color all vertices as uncolored
 *   2. For each uncolored vertex v:
 *      Color v as 0
 *      Queue.enqueue(v)
 *      While queue not empty:
 *        u = queue.dequeue()
 *        For each neighbor w of u:
 *          If w has same color as u:
 *            Return false  // Not bipartite!
 *          If w uncolored:
 *            Color w with opposite color of u
 *            Queue.enqueue(w)
 *   3. Return true  // Graph is bipartite
 * ```
 *
 * **Time complexity**: O(V + E) - single BFS/DFS pass
 *
 * ## Maximum Matching
 *
 * ### Problem
 *
 * Find the largest set of edges such that no two edges share a vertex.
 *
 * ### Algorithm: Augmenting Paths
 *
 * Uses the concept of **augmenting paths**:
 * 1. Start with empty matching
 * 2. While augmenting path exists:
 *    - Flip edges along path (matched ↔ unmatched)
 *    - Matching size increases by 1
 * 3. Return maximum matching
 *
 * **Time complexity**: O(V × E) using BFS
 *
 * ### Applications of Matching
 *
 * - **Job assignment**: Match workers to tasks
 * - **Dating apps**: Match users based on preferences
 * - **Course scheduling**: Match students to courses
 * - **Resource allocation**: Match resources to tasks
 *
 * ## Complexity
 *
 * | Operation | Time Complexity | Notes |
 * |-----------|----------------|-------|
 * | Test bipartite | O(V + E) | Single BFS/DFS |
 * | Find partition | O(V + E) | Same as testing |
 * | Maximum matching | O(V × E) | Augmenting paths |
 * | Perfect matching check | O(V × E) | Same as max matching |
 *
 * ## Applications
 *
 * ### Job Assignment
 * - **Workers ↔ Tasks**: Match workers to tasks they can do
 * - **Skills matching**: Match based on required skills
 * - **Scheduling**: Assign shifts to workers
 *
 * ### Course Scheduling
 * - **Students ↔ Courses**: Match students to courses
 * - **Prerequisites**: Respect course prerequisites
 * - **Capacity**: Match within course capacity limits
 *
 * ### Dating/Matching Services
 * - **Users ↔ Users**: Match compatible users
 * - **Preferences**: Match based on preferences
 * - **Stable matching**: Find stable matchings
 *
 * ### Resource Allocation
 * - **Resources ↔ Tasks**: Allocate resources to tasks
 * - **Optimization**: Maximize resource utilization
 * - **Constraints**: Respect capacity constraints
 *
 * ### Network Flow
 * - **Reduction**: Many problems reduce to max-flow
 * - **Bipartite matching**: Special case of max-flow
 * - **Min-cost matching**: Weighted matching problems
 *
 * ## Example: Job Assignment
 *
 * ```
 * Workers:  Alice, Bob, Charlie
 * Tasks:    Task1, Task2, Task3
 *
 * Bipartite graph:
 *   Alice   → Task1, Task2
 *   Bob     → Task2, Task3
 *   Charlie → Task1, Task3
 *
 * Maximum matching:
 *   Alice → Task1
 *   Bob → Task3
 *   Charlie → (unmatched)
 *
 * Or:
 *   Alice → Task2
 *   Bob → Task3
 *   Charlie → Task1
 * ```
 *
 * ## Comparison with General Graphs
 *
 * | Property | Bipartite | General |
 * |-----------|-----------|---------|
 * | Cycles | Only even length | Any length |
 * | Coloring | 2 colors | May need more |
 * | Matching | Polynomial | NP-hard (general) |
 * | Structure | Two sets | Arbitrary |
 *
 * ## Usage
 *
 * ```bash
 * # Run all bipartite demonstrations
 * ./bipartite_example
 *
 * # Run specific demo
 * ./bipartite_example -s def      # Definition and examples
 * ./bipartite_example -s test     # Bipartite testing
 * ./bipartite_example -s match    # Maximum matching demo
 * ./bipartite_example -s dating   # Dating / pairing example
 * ./bipartite_example -s hall     # Hall's theorem
 * ```
 *
 * @see tpl_bipartite.H Bipartite graph algorithms
 * @see network_flow_example.C Maximum flow (related to matching)
 * @see bfs_dfs_example.C Graph traversal (used for testing)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>

#include <tpl_graph.H>
#include <tpl_bipartite.H>

using namespace std;
using namespace Aleph;

// Graph types
using Node = Graph_Node<string>;
using Arc = Graph_Arc<int>;
using Graph = List_Graph<Node, Arc>;

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

void print_partition(const string& label, DynDlist<Graph::Node*>& nodes)
{
  cout << label << ": {";
  bool first = true;
  for (auto it = nodes.get_it(); it.has_curr(); it.next())
  {
    if (not first) cout << ", ";
    cout << it.get_curr()->get_info();
    first = false;
  }
  cout << "}" << endl;
}

void print_matching(Graph& g, DynDlist<Graph::Arc*>& matching)
{
  cout << "Matching (" << matching.size() << " edges):" << endl;
  for (auto it = matching.get_it(); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    cout << "  " << g.get_src_node(arc)->get_info() 
         << " -- " << g.get_tgt_node(arc)->get_info() << endl;
  }
}

// =============================================================================
// 1. What is a Bipartite Graph?
// =============================================================================

void demo_definition()
{
  print_section("WHAT IS A BIPARTITE GRAPH?");
  
  cout << "A bipartite graph has two disjoint vertex sets L and R,\n";
  cout << "where every edge connects a vertex in L to a vertex in R.\n";
  cout << "No edges exist within L or within R.\n\n";
  
  // Example: Simple bipartite graph
  print_subsection("Example: Workers and Tasks");
  
  Graph g;
  
  // Workers (left partition)
  auto w1 = g.insert_node("Juan");
  auto w2 = g.insert_node("Maria");
  auto w3 = g.insert_node("Carlos");
  
  // Tasks (right partition)
  auto t1 = g.insert_node("Cocinar");
  auto t2 = g.insert_node("Limpiar");
  auto t3 = g.insert_node("Comprar");
  
  // Skills (edges): who can do what
  g.insert_arc(w1, t1, 1);  // Juan can cook
  g.insert_arc(w1, t2, 1);  // Juan can clean
  g.insert_arc(w2, t2, 1);  // Maria can clean
  g.insert_arc(w2, t3, 1);  // Maria can buy
  g.insert_arc(w3, t1, 1);  // Carlos can cook
  g.insert_arc(w3, t3, 1);  // Carlos can buy
  
  cout << "Workers: {Juan, Maria, Carlos}\n";
  cout << "Tasks:   {Cocinar, Limpiar, Comprar}\n\n";
  cout << "Skills (edges):\n";
  cout << "  Juan   -- Cocinar, Limpiar\n";
  cout << "  Maria  -- Limpiar, Comprar\n";
  cout << "  Carlos -- Cocinar, Comprar\n\n";
  
  // Compute bipartition
  print_subsection("Compute bipartition");
  
  DynDlist<Graph::Node*> left, right;
  
  try
  {
    compute_bipartite(g, left, right);
    cout << "Graph IS bipartite!\n\n";
    print_partition("Left partition", left);
    print_partition("Right partition", right);
  }
  catch (const exception& e)
  {
    cout << "Graph is NOT bipartite: " << e.what() << endl;
  }
}

// =============================================================================
// 2. Testing Bipartiteness
// =============================================================================

void demo_testing()
{
  print_section("TESTING BIPARTITENESS");
  
  cout << "A graph is bipartite ⟺ it has no odd-length cycles.\n";
  cout << "Equivalently: it can be 2-colored.\n\n";
  
  // Bipartite example: Even cycle
  print_subsection("Example 1: Square (C4) - Bipartite");
  
  Graph square;
  auto a = square.insert_node("A");
  auto b = square.insert_node("B");
  auto c = square.insert_node("C");
  auto d = square.insert_node("D");
  square.insert_arc(a, b, 1);
  square.insert_arc(b, c, 1);
  square.insert_arc(c, d, 1);
  square.insert_arc(d, a, 1);
  
  cout << "Square: A-B-C-D-A (cycle of length 4 = even)\n";
  
  DynDlist<Graph::Node*> l1, r1;
  try
  {
    compute_bipartite(square, l1, r1);
    cout << "Result: BIPARTITE\n";
    print_partition("  Red vertices", l1);
    print_partition("  Blue vertices", r1);
  }
  catch (...)
  {
    cout << "Result: NOT bipartite\n";
  }
  
  // Non-bipartite example: Triangle (odd cycle)
  print_subsection("Example 2: Triangle (C3) - NOT Bipartite");
  
  Graph triangle;
  auto t1 = triangle.insert_node("X");
  auto t2 = triangle.insert_node("Y");
  auto t3 = triangle.insert_node("Z");
  triangle.insert_arc(t1, t2, 1);
  triangle.insert_arc(t2, t3, 1);
  triangle.insert_arc(t3, t1, 1);
  
  cout << "Triangle: X-Y-Z-X (cycle of length 3 = odd)\n";
  
  DynDlist<Graph::Node*> l2, r2;
  try
  {
    compute_bipartite(triangle, l2, r2);
    cout << "Result: BIPARTITE\n";
  }
  catch (const exception& e)
  {
    cout << "Result: NOT bipartite\n";
    cout << "Reason: Cannot 2-color an odd cycle!\n";
  }
  
  // Complete bipartite K3,3
  print_subsection("Example 3: Complete Bipartite K3,3");
  
  Graph k33;
  auto u1 = k33.insert_node("U1");
  auto u2 = k33.insert_node("U2");
  auto u3 = k33.insert_node("U3");
  auto v1 = k33.insert_node("V1");
  auto v2 = k33.insert_node("V2");
  auto v3 = k33.insert_node("V3");
  
  // Connect all U's to all V's
  k33.insert_arc(u1, v1, 1); k33.insert_arc(u1, v2, 1); k33.insert_arc(u1, v3, 1);
  k33.insert_arc(u2, v1, 1); k33.insert_arc(u2, v2, 1); k33.insert_arc(u2, v3, 1);
  k33.insert_arc(u3, v1, 1); k33.insert_arc(u3, v2, 1); k33.insert_arc(u3, v3, 1);
  
  cout << "K3,3: Complete bipartite (all U's connected to all V's)\n";
  cout << "  9 edges, 6 vertices\n";
  
  DynDlist<Graph::Node*> l3, r3;
  try
  {
    compute_bipartite(k33, l3, r3);
    cout << "Result: BIPARTITE\n";
    print_partition("  Set U", l3);
    print_partition("  Set V", r3);
  }
  catch (...)
  {
    cout << "Result: NOT bipartite\n";
  }
}

// =============================================================================
// 3. Maximum Matching (Concept)
// =============================================================================

void demo_matching()
{
  print_section("MAXIMUM MATCHING");
  
  cout << "A matching is a set of edges with no shared vertices.\n";
  cout << "Maximum matching = largest possible matching.\n\n";
  
  cout << "Application: Assign workers to tasks (one task per worker).\n\n";
  
  // Job assignment problem
  print_subsection("Job Assignment Problem");
  
  cout << "Workers: Ana, Bob, Cam, Dan\n";
  cout << "Tasks: Programar, Diseñar, Testear, Documentar\n\n";
  cout << "Skills (edges in bipartite graph):\n";
  cout << "  Ana: Programar, Testear\n";
  cout << "  Bob: Programar, Diseñar\n";
  cout << "  Cam: Diseñar, Documentar\n";
  cout << "  Dan: Testear, Documentar\n\n";
  
  cout << "Maximum matching algorithm:\n";
  cout << "1. Build flow network with source -> L, R -> sink\n";
  cout << "2. Each edge has capacity 1\n";
  cout << "3. Maximum flow = maximum matching size\n\n";
  
  cout << "Optimal assignment for this example:\n";
  cout << "  Ana  -- Programar   (or Testear)\n";
  cout << "  Bob  -- Diseñar     (or Programar)\n";
  cout << "  Cam  -- Documentar  (or Diseñar)\n";
  cout << "  Dan  -- Testear     (or Documentar)\n\n";
  
  cout << "Result: All 4 workers can be assigned!\n";
  
  // Another example with imperfect matching
  print_subsection("Example with Imperfect Matching");
  
  cout << "3 students, 2 courses:\n";
  cout << "  Student1 wants CourseA only\n";
  cout << "  Student2 wants CourseA only\n";
  cout << "  Student3 wants CourseB only\n\n";
  
  cout << "Maximum matching: 2 students get assigned\n";
  cout << "  Student1 -- CourseA  (or Student2)\n";
  cout << "  Student3 -- CourseB\n\n";
  
  cout << "One student without a course (conflict over CourseA).\n";
  
  // Demonstrate bipartition detection
  print_subsection("Verify graph is bipartite");
  
  Graph jobs;
  auto ana = jobs.insert_node("Ana");
  auto bob = jobs.insert_node("Bob");
  auto prog = jobs.insert_node("Programar");
  auto dise = jobs.insert_node("Diseñar");
  
  jobs.insert_arc(ana, prog, 1);
  jobs.insert_arc(ana, dise, 1);
  jobs.insert_arc(bob, prog, 1);
  
  DynDlist<Graph::Node*> l, r;
  try
  {
    compute_bipartite(jobs, l, r);
    cout << "Small graph verified as bipartite:\n";
    print_partition("  Workers", l);
    print_partition("  Tasks", r);
  }
  catch (...)
  {
    cout << "Graph is not bipartite\n";
  }
}

// =============================================================================
// 4. Practical Application: Dating Service
// =============================================================================

void demo_dating()
{
  print_section("PRACTICAL: Dating Service Matching");
  
  cout << "Match compatible people maximizing the number of dates.\n\n";
  
  cout << "Compatibility graph:\n";
  cout << "  Sofia  <-> Andres, Miguel\n";
  cout << "  Lucia  <-> Miguel, David\n";
  cout << "  Camila <-> Andres, David\n\n";
  
  // Verify this is bipartite
  Graph dating;
  auto p1 = dating.insert_node("Sofia");
  auto p2 = dating.insert_node("Lucia");
  auto p3 = dating.insert_node("Camila");
  auto q1 = dating.insert_node("Andres");
  auto q2 = dating.insert_node("Miguel");
  auto q3 = dating.insert_node("David");
  
  dating.insert_arc(p1, q1, 1);
  dating.insert_arc(p1, q2, 1);
  dating.insert_arc(p2, q2, 1);
  dating.insert_arc(p2, q3, 1);
  dating.insert_arc(p3, q1, 1);
  dating.insert_arc(p3, q3, 1);
  
  DynDlist<Graph::Node*> group_a, group_b;
  compute_bipartite(dating, group_a, group_b);
  
  print_partition("Group A", group_a);
  print_partition("Group B", group_b);
  
  cout << "\nMaximum matching analysis:\n";
  cout << "  Each person in Group A has 2 compatible matches\n";
  cout << "  Hall's condition: every subset has enough neighbors\n\n";
  
  cout << "Optimal pairing (found via max-flow):\n";
  cout << "  Sofia  <3 Miguel\n";
  cout << "  Lucia  <3 David\n";
  cout << "  Camila <3 Andres\n\n";
  
  cout << "All 3 people get a date! (Perfect matching exists)\n";
}

// =============================================================================
// 5. Hall's Marriage Theorem
// =============================================================================

void demo_halls_theorem()
{
  print_section("HALL'S MARRIAGE THEOREM");
  
  cout << "Hall's Theorem: A bipartite graph G=(L,R,E) has a matching\n";
  cout << "covering all of L if and only if for every subset S of L,\n";
  cout << "|N(S)| >= |S| where N(S) = neighbors of S.\n\n";
  
  cout << "In other words: Every subset of L must have 'enough' neighbors.\n\n";
  
  // Example satisfying Hall's condition
  print_subsection("Example satisfying Hall's condition");
  
  cout << "Graph: Each H has 2 choices among W's\n";
  cout << "  H1 <-> W1, W2\n";
  cout << "  H2 <-> W2, W3\n";
  cout << "  H3 <-> W1, W3\n\n";
  
  cout << "Check Hall's condition:\n";
  cout << "  |{H1}| = 1 <= |{W1, W2}| = 2  OK\n";
  cout << "  |{H2}| = 1 <= |{W2, W3}| = 2  OK\n";
  cout << "  |{H3}| = 1 <= |{W1, W3}| = 2  OK\n";
  cout << "  |{H1, H2}| = 2 <= |{W1, W2, W3}| = 3  OK\n";
  cout << "  |{H1, H3}| = 2 <= |{W1, W2, W3}| = 3  OK\n";
  cout << "  |{H2, H3}| = 2 <= |{W1, W2, W3}| = 3  OK\n";
  cout << "  |{H1, H2, H3}| = 3 <= |{W1, W2, W3}| = 3  OK\n\n";
  
  cout << "Hall's condition SATISFIED => Perfect matching exists:\n";
  cout << "  H1 -- W2\n";
  cout << "  H2 -- W3\n";
  cout << "  H3 -- W1\n";
  
  // Example violating Hall's condition
  print_subsection("Example violating Hall's condition");
  
  cout << "Graph: 3 A's but only 2 B's as neighbors\n";
  cout << "  A1, A2, A3 all <-> B1, B2 only (not B3)\n\n";
  
  cout << "Check Hall's condition:\n";
  cout << "  |{A1, A2, A3}| = 3  but  |N({A1, A2, A3})| = |{B1, B2}| = 2\n";
  cout << "  3 > 2 => VIOLATED!\n\n";
  
  cout << "Hall's condition VIOLATED => NO perfect matching!\n";
  cout << "Maximum matching size = 2 (one A left unmatched)\n";
  
  // Verify with compute_bipartite
  print_subsection("Verify graphs are bipartite");
  
  Graph hall_ok;
  auto h1 = hall_ok.insert_node("H1");
  auto h2 = hall_ok.insert_node("H2");
  auto h3 = hall_ok.insert_node("H3");
  auto w1 = hall_ok.insert_node("W1");
  auto w2 = hall_ok.insert_node("W2");
  auto w3 = hall_ok.insert_node("W3");
  
  hall_ok.insert_arc(h1, w1, 1);
  hall_ok.insert_arc(h1, w2, 1);
  hall_ok.insert_arc(h2, w2, 1);
  hall_ok.insert_arc(h2, w3, 1);
  hall_ok.insert_arc(h3, w1, 1);
  hall_ok.insert_arc(h3, w3, 1);
  
  DynDlist<Graph::Node*> l1, r1;
  compute_bipartite(hall_ok, l1, r1);
  print_partition("Left (H's)", l1);
  print_partition("Right (W's)", r1);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Bipartite graph example for Aleph-w.\n"
      "Demonstrates bipartition and maximum matching.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: def, test, match, dating, hall, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W BIPARTITE GRAPHS EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "def")
      demo_definition();
    
    if (section == "all" or section == "test")
      demo_testing();
    
    if (section == "all" or section == "match")
      demo_matching();
    
    if (section == "all" or section == "dating")
      demo_dating();
    
    if (section == "all" or section == "hall")
      demo_halls_theorem();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Bipartite graphs demo completed!\n";
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

