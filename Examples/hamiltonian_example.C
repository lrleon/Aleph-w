/**
 * @file hamiltonian_example.C
 * @brief Example demonstrating Hamiltonian graph testing in Aleph-w.
 *
 * A Hamiltonian cycle visits every VERTEX exactly once (vs Eulerian
 * which visits every EDGE exactly once).
 *
 * ## Hamiltonian vs Eulerian
 *
 * | Property | Hamiltonian | Eulerian |
 * |----------|-------------|----------|
 * | Visits | Every vertex once | Every edge once |
 * | Complexity | NP-complete | Polynomial O(V+E) |
 * | Test | Sufficiency only | Exact |
 *
 * ## Ore's Theorem (Sufficiency)
 *
 * For a graph with n ≥ 3 vertices:
 * If for every pair of NON-ADJACENT vertices u, v:
 *   deg(u) + deg(v) ≥ n
 * Then the graph has a Hamiltonian cycle.
 *
 * **Important**: This is sufficient but NOT necessary!
 * A graph may be Hamiltonian without satisfying Ore's condition.
 *
 * ## Usage
 *
 * ```bash
 * ./hamiltonian_example           # Run all demos
 * ./hamiltonian_example -s ore    # Only Ore's theorem demo
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see hamiltonian.H Hamiltonian sufficiency testing
 * @see eulerian.H Eulerian path/cycle detection
 */

#include <iostream>
#include <iomanip>
#include <string>

#include <tclap/CmdLine.h>

#include <tpl_graph.H>
#include <hamiltonian.H>

using namespace std;
using namespace Aleph;

// Graph types
using SNode = Graph_Node<string>;
using IArc = Graph_Arc<int>;
using UGraph = List_Graph<SNode, IArc>;

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

void print_degrees(UGraph& g)
{
  cout << "Vertex degrees:" << endl;
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    auto node = it.get_curr();
    size_t degree = 0;
    for (Node_Arc_Iterator<UGraph> ait(node); ait.has_curr(); ait.next())
      degree++;
    cout << "  " << node->get_info() << ": degree " << degree << endl;
  }
}

// Build complete graph Kn
void build_complete_graph(UGraph& g, size_t n)
{
  vector<UGraph::Node*> nodes;
  
  // Create nodes
  for (size_t i = 0; i < n; i++)
    nodes.push_back(g.insert_node(to_string(i)));
  
  // Connect all pairs
  for (size_t i = 0; i < n; i++)
    for (size_t j = i + 1; j < n; j++)
      g.insert_arc(nodes[i], nodes[j], 1);
}

// =============================================================================
// 1. Hamiltonian vs Eulerian
// =============================================================================

void demo_comparison()
{
  print_section("HAMILTONIAN VS EULERIAN");
  
  cout << "Hamiltonian: Visit every VERTEX exactly once\n";
  cout << "Eulerian:    Visit every EDGE exactly once\n\n";
  
  // Example: Triangle
  print_subsection("Example: Triangle (K3)");
  
  UGraph triangle;
  auto a = triangle.insert_node("A");
  auto b = triangle.insert_node("B");
  auto c = triangle.insert_node("C");
  triangle.insert_arc(a, b, 1);
  triangle.insert_arc(b, c, 1);
  triangle.insert_arc(c, a, 1);
  
  cout << "Triangle: A-B-C-A\n";
  cout << "  Vertices: 3, Edges: 3\n";
  cout << "  Each vertex has degree 2\n\n";
  
  cout << "Hamiltonian cycle: A -> B -> C -> A (visits each vertex once)\n";
  cout << "Eulerian cycle:    A -> B -> C -> A (visits each edge once)\n";
  cout << "\nTriangle is BOTH Hamiltonian AND Eulerian!\n";
  
  // Example: Star graph
  print_subsection("Example: Star graph (K1,4)");
  
  UGraph star;
  auto center = star.insert_node("Center");
  auto p1 = star.insert_node("P1");
  auto p2 = star.insert_node("P2");
  auto p3 = star.insert_node("P3");
  auto p4 = star.insert_node("P4");
  star.insert_arc(center, p1, 1);
  star.insert_arc(center, p2, 1);
  star.insert_arc(center, p3, 1);
  star.insert_arc(center, p4, 1);
  
  cout << "Star: Center connected to P1, P2, P3, P4\n";
  cout << "  Center degree: 4\n";
  cout << "  P1-P4 degrees: 1 each\n\n";
  
  cout << "Hamiltonian? NO - Can't visit all without repeating Center\n";
  cout << "Eulerian?    NO - Peripheral vertices have odd degree\n";
}

// =============================================================================
// 2. Ore's Theorem
// =============================================================================

void demo_ore_theorem()
{
  print_section("ORE'S THEOREM (Sufficiency Condition)");
  
  cout << "Ore's Theorem states:\n";
  cout << "For a graph G with n >= 3 vertices, if for every pair of\n";
  cout << "NON-ADJACENT vertices u, v: deg(u) + deg(v) >= n,\n";
  cout << "then G has a Hamiltonian cycle.\n\n";
  
  // Complete graph K5 - satisfies Ore's condition
  print_subsection("Example 1: Complete graph K5");
  
  UGraph k5;
  build_complete_graph(k5, 5);
  
  cout << "K5: Complete graph with 5 vertices\n";
  cout << "  All vertices connected to all others\n";
  cout << "  Each vertex has degree 4\n\n";
  
  cout << "Check Ore's condition:\n";
  cout << "  In K5, every pair IS adjacent (no non-adjacent pairs)\n";
  cout << "  Condition is trivially satisfied!\n\n";
  
  Test_Hamiltonian_Sufficiency<UGraph> test1;
  cout << "Satisfies Ore's condition? " << (test1(k5) ? "YES" : "NO") << endl;
  cout << "=> K5 is Hamiltonian\n";
  
  // Cycle graph C5 - does NOT satisfy Ore's but IS Hamiltonian
  print_subsection("Example 2: Cycle C5 (Pentagon)");
  
  UGraph c5;
  auto n0 = c5.insert_node("0");
  auto n1 = c5.insert_node("1");
  auto n2 = c5.insert_node("2");
  auto n3 = c5.insert_node("3");
  auto n4 = c5.insert_node("4");
  c5.insert_arc(n0, n1, 1);
  c5.insert_arc(n1, n2, 1);
  c5.insert_arc(n2, n3, 1);
  c5.insert_arc(n3, n4, 1);
  c5.insert_arc(n4, n0, 1);
  
  cout << "C5: Cycle 0-1-2-3-4-0\n";
  cout << "  Each vertex has degree 2\n\n";
  
  cout << "Check Ore's condition:\n";
  cout << "  Non-adjacent pair (0, 2): deg(0) + deg(2) = 2 + 2 = 4\n";
  cout << "  Need >= n = 5, but only have 4\n";
  cout << "  Condition NOT satisfied!\n\n";
  
  Test_Hamiltonian_Sufficiency<UGraph> test2;
  cout << "Satisfies Ore's condition? " << (test2(c5) ? "YES" : "NO") << endl;
  cout << "\nBUT: C5 IS Hamiltonian! (The cycle itself is Hamiltonian)\n";
  cout << "=> Ore's is SUFFICIENT but not NECESSARY\n";
}

// =============================================================================
// 3. Practical Examples
// =============================================================================

void demo_practical()
{
  print_section("PRACTICAL: Traveling Salesman Setup");
  
  cout << "The Hamiltonian cycle problem is the foundation of TSP.\n";
  cout << "TSP asks: What's the shortest Hamiltonian cycle?\n\n";
  
  // Colombian cities as vertices
  print_subsection("Colombian cities tour");
  
  UGraph colombia;
  auto bog = colombia.insert_node("Bogota");
  auto med = colombia.insert_node("Medellin");
  auto cal = colombia.insert_node("Cali");
  auto bar = colombia.insert_node("Barranquilla");
  auto car = colombia.insert_node("Cartagena");
  
  // Create some connections (not all cities directly connected)
  colombia.insert_arc(bog, med, 1);  // Bogota-Medellin
  colombia.insert_arc(bog, cal, 1);  // Bogota-Cali
  colombia.insert_arc(med, cal, 1);  // Medellin-Cali
  colombia.insert_arc(med, bar, 1);  // Medellin-Barranquilla
  colombia.insert_arc(bar, car, 1);  // Barranquilla-Cartagena
  colombia.insert_arc(bog, bar, 1);  // Bogota-Barranquilla
  
  cout << "Cities: Bogota, Medellin, Cali, Barranquilla, Cartagena\n";
  cout << "Connections:\n";
  cout << "  Bogota-Medellin, Bogota-Cali, Bogota-Barranquilla\n";
  cout << "  Medellin-Cali, Medellin-Barranquilla\n";
  cout << "  Barranquilla-Cartagena\n\n";
  
  print_degrees(colombia);
  
  cout << "\nNon-adjacent pairs check:\n";
  cout << "  (Bogota, Cartagena): 3 + 1 = 4 < 5 - FAILS\n";
  cout << "  (Cali, Barranquilla): 2 + 2 = 4 < 5 - FAILS\n";
  cout << "  (Cali, Cartagena): 2 + 1 = 3 < 5 - FAILS\n";
  
  Test_Hamiltonian_Sufficiency<UGraph> test;
  cout << "\nSatisfies Ore's condition? " << (test(colombia) ? "YES" : "NO") << endl;
  
  cout << "\nThis doesn't mean no Hamiltonian cycle exists!\n";
  cout << "Let's check manually:\n";
  cout << "  Bogota -> Medellin -> Barranquilla -> Cartagena -> ?\n";
  cout << "  Cartagena only connects to Barranquilla - STUCK!\n";
  cout << "\nNeed to add Cali-Cartagena or Bogota-Cartagena connection.\n";
  
  // Add missing connection
  print_subsection("Adding Cali-Cartagena connection");
  
  colombia.insert_arc(cal, car, 1);
  
  cout << "Added: Cali-Cartagena\n\n";
  print_degrees(colombia);
  
  cout << "\nNow we can find a Hamiltonian cycle:\n";
  cout << "  Bogota -> Cali -> Cartagena -> Barranquilla -> Medellin -> Bogota\n";
  cout << "  (Visits each city exactly once and returns to start)\n";
}

// =============================================================================
// 4. Dense vs Sparse Graphs
// =============================================================================

void demo_density()
{
  print_section("GRAPH DENSITY AND HAMILTONICITY");
  
  cout << "Dense graphs are more likely to satisfy Ore's condition.\n\n";
  
  // Compare different densities
  vector<tuple<string, size_t, size_t>> configs = {
    {"Sparse (n edges)", 8, 8},
    {"Medium (2n edges)", 8, 16},
    {"Dense (3n edges)", 8, 24},
    {"Complete (n(n-1)/2)", 8, 28}
  };
  
  cout << setw(25) << "Configuration" << setw(15) << "Satisfies Ore?" << endl;
  cout << string(40, '-') << endl;
  
  for (auto& [name, n, target_edges] : configs)
  {
    UGraph g;
    vector<UGraph::Node*> nodes;
    
    // Create nodes
    for (size_t i = 0; i < n; i++)
      nodes.push_back(g.insert_node(to_string(i)));
    
    // Add edges up to target
    size_t edges = 0;
    for (size_t i = 0; i < n and edges < target_edges; i++)
    {
      for (size_t j = i + 1; j < n and edges < target_edges; j++)
      {
        g.insert_arc(nodes[i], nodes[j], 1);
        edges++;
      }
    }
    
    Test_Hamiltonian_Sufficiency<UGraph> test;
    cout << setw(25) << name << setw(15) << (test(g) ? "YES" : "NO") << endl;
  }
  
  cout << "\nConclusion: Denser graphs more likely to be Hamiltonian.\n";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Hamiltonian graph example for Aleph-w.\n"
      "Demonstrates Ore's sufficiency condition for Hamiltonian cycles.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: compare, ore, practical, density, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "          ALEPH-W HAMILTONIAN GRAPHS EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "compare")
      demo_comparison();
    
    if (section == "all" or section == "ore")
      demo_ore_theorem();
    
    if (section == "all" or section == "practical")
      demo_practical();
    
    if (section == "all" or section == "density")
      demo_density();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Hamiltonian graphs demo completed!\n";
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

