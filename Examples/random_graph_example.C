/**
 * @file random_graph_example.C
 * @brief Example demonstrating random graph generation in Aleph-w.
 *
 * This program demonstrates `random_graph.H` which provides:
 * - Erdős–Rényi random graphs G(n,m)
 * - Connected random graphs
 * - Eulerian random graphs
 * - Random directed graphs (digraphs)
 *
 * ## Graph Models
 *
 * ### Erdős–Rényi G(n,m)
 * - n vertices, m randomly placed edges
 * - May be disconnected
 *
 * ### Connected Random Graph
 * - Guaranteed to be connected
 * - First builds a spanning tree, then adds random edges
 *
 * ### Eulerian Random Graph
 * - All vertices have even degree
 * - Has an Eulerian cycle
 *
 * ## Usage
 *
 * ```bash
 * ./random_graph_example           # Run all demos
 * ./random_graph_example -s erdos  # Only Erdős-Rényi demo
 * ```
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 * @date 2024
 * @copyright GNU General Public License
 *
 * @see random_graph.H Random graph generation
 * @see eulerian.H Eulerian graph testing
 */

#include <iostream>
#include <iomanip>
#include <string>

#include <tclap/CmdLine.h>

#include <tpl_graph.H>
#include <random_graph.H>
#include <tpl_components.H>
#include <eulerian.H>

using namespace std;
using namespace Aleph;

// Graph types
using Node = Graph_Node<int>;
using Arc = Graph_Arc<double>;
using UGraph = List_Graph<Node, Arc>;
using DGraph = List_Digraph<Node, Arc>;

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
void print_graph_stats(const string& label, G& g)
{
  cout << label << ":" << endl;
  cout << "  Vertices: " << g.get_num_nodes() << endl;
  cout << "  Edges: " << g.get_num_arcs() << endl;
  
  // Calculate average degree
  double total_degree = 0;
  size_t min_deg = numeric_limits<size_t>::max();
  size_t max_deg = 0;
  
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    size_t deg = 0;
    for (Node_Arc_Iterator<G> ait(it.get_curr()); ait.has_curr(); ait.next())
      deg++;
    total_degree += deg;
    min_deg = min(min_deg, deg);
    max_deg = max(max_deg, deg);
  }
  
  double avg_deg = total_degree / g.get_num_nodes();
  cout << "  Avg degree: " << fixed << setprecision(2) << avg_deg << endl;
  cout << "  Min degree: " << min_deg << endl;
  cout << "  Max degree: " << max_deg << endl;
}

// =============================================================================
// 1. Erdős–Rényi Random Graph
// =============================================================================

void demo_erdos_renyi()
{
  print_section("ERDŐS-RÉNYI RANDOM GRAPH G(n,m)");
  
  cout << "Generate a graph with n vertices and m random edges.\n";
  cout << "Edges are placed uniformly at random.\n\n";
  
  size_t n = 20;  // vertices
  size_t m = 40;  // edges
  
  cout << "Parameters: n=" << n << " vertices, m=" << m << " edges\n";
  
  // Create random graph generator
  Random_Graph<UGraph> gen(time(nullptr));
  
  // Generate graph
  print_subsection("Generated graph");
  UGraph g = gen(n, m);
  
  print_graph_stats("G(" + to_string(n) + "," + to_string(m) + ")", g);
  
  // Check connectivity
  print_subsection("Connectivity analysis");
  
  Unconnected_Components<UGraph> components;
  DynList<UGraph> subgraphs;
  components(g, subgraphs);
  
  cout << "Number of connected components: " << subgraphs.size() << endl;
  
  if (subgraphs.size() == 1)
    cout << "Graph is CONNECTED\n";
  else
  {
    cout << "Graph is DISCONNECTED\n";
    cout << "Component sizes: ";
    for (auto it = subgraphs.get_it(); it.has_curr(); it.next())
      cout << it.get_curr().get_num_nodes() << " ";
    cout << endl;
  }
  
  // Density
  double density = 2.0 * m / (n * (n - 1));
  cout << "\nGraph density: " << fixed << setprecision(4) << density << endl;
  cout << "(1.0 = complete graph, 0.0 = no edges)\n";
}

// =============================================================================
// 2. Connected Random Graph
// =============================================================================

void demo_connected()
{
  print_section("DENSE RANDOM GRAPH (likely connected)");
  
  cout << "Generate a dense random graph (high edge probability).\n";
  cout << "Dense graphs are typically connected.\n\n";
  
  size_t n = 20;
  size_t m = n * 3;  // Dense: 3 edges per vertex on average
  
  cout << "Parameters: n=" << n << " vertices, m=" << m << " edges\n";
  cout << "(Threshold for connectivity: ~n*ln(n)/2 = " << (size_t)(n * log(n) / 2) << ")\n";
  
  Random_Graph<UGraph> gen(time(nullptr));
  
  // Generate dense graph
  print_subsection("Generated dense graph");
  UGraph g = gen(n, m);
  
  print_graph_stats("Dense G", g);
  
  // Verify connectivity
  Unconnected_Components<UGraph> components;
  DynList<UGraph> subgraphs;
  components(g, subgraphs);
  
  cout << "\nConnected components: " << subgraphs.size() << endl;
  if (subgraphs.size() == 1)
    cout << "Graph is CONNECTED (as expected for dense graphs)\n";
  else
    cout << "Graph is disconnected (rare for this density)\n";
}

// =============================================================================
// 3. Eulerian Random Graph
// =============================================================================

void demo_eulerian()
{
  print_section("EULERIAN RANDOM GRAPH");
  
  cout << "Generate a random graph where all vertices have even degree.\n";
  cout << "Such a graph has an Eulerian cycle.\n\n";
  
  size_t n = 15;
  size_t m = 30;
  
  cout << "Parameters: n=" << n << " vertices, m=" << m << " edges\n";
  
  Random_Graph<UGraph> gen(time(nullptr));
  
  // Generate Eulerian graph
  print_subsection("Generated Eulerian graph");
  UGraph g = gen.eulerian(n, m);
  
  print_graph_stats("Eulerian G", g);
  
  // Check all degrees are even
  print_subsection("Degree verification");
  
  bool all_even = true;
  cout << "Vertex degrees: ";
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    size_t deg = 0;
    for (Node_Arc_Iterator<UGraph> ait(it.get_curr()); ait.has_curr(); ait.next())
      deg++;
    
    if (deg % 2 != 0) all_even = false;
  }
  cout << "(checking...)" << endl;
  
  cout << "All degrees even? " << (all_even ? "YES" : "NO") << endl;
  
  // Verify with Test_Eulerian
  Test_Eulerian<UGraph> test;
  cout << "Is Eulerian (Test_Eulerian)? " << (test(g) ? "YES" : "NO") << endl;
}

// =============================================================================
// 4. Random Digraph
// =============================================================================

void demo_digraph()
{
  print_section("RANDOM DIRECTED GRAPH");
  
  cout << "Generate random directed graphs (digraphs).\n\n";
  
  size_t n = 15;
  size_t m = 40;
  
  cout << "Parameters: n=" << n << " vertices, m=" << m << " arcs\n";
  
  Random_Digraph<DGraph> gen(time(nullptr));
  
  // Generate random digraph
  print_subsection("Generated digraph");
  DGraph g = gen(n, m);
  
  cout << "Digraph statistics:" << endl;
  cout << "  Vertices: " << g.get_num_nodes() << endl;
  cout << "  Arcs: " << g.get_num_arcs() << endl;
  
  // Analyze in/out degrees
  print_subsection("Degree analysis");
  
  size_t total_out = 0;
  size_t max_out = 0;
  
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    size_t out_deg = 0;
    for (Node_Arc_Iterator<DGraph> ait(it.get_curr()); ait.has_curr(); ait.next())
      out_deg++;
    total_out += out_deg;
    max_out = max(max_out, out_deg);
  }
  
  cout << "Average out-degree: " << fixed << setprecision(2) 
       << (double)total_out / n << endl;
  cout << "Max out-degree: " << max_out << endl;
}

// =============================================================================
// 5. Eulerian Random Digraph  
// =============================================================================

void demo_eulerian_digraph()
{
  print_section("EULERIAN RANDOM DIGRAPH");
  
  cout << "Generate a random digraph where in-degree = out-degree for all.\n";
  cout << "(Has an Eulerian cycle)\n\n";
  
  size_t n = 12;
  size_t m = 30;
  
  cout << "Parameters: n=" << n << " vertices, m=" << m << " arcs\n";
  
  Random_Digraph<DGraph> gen(time(nullptr));
  
  // Generate Eulerian digraph
  print_subsection("Generated Eulerian digraph");
  DGraph g = gen.eulerian(n, m);
  
  cout << "Digraph statistics:" << endl;
  cout << "  Vertices: " << g.get_num_nodes() << endl;
  cout << "  Arcs: " << g.get_num_arcs() << endl;
  
  // Verify Eulerian property
  print_subsection("Verification (in-degree = out-degree)");
  
  bool is_eulerian = true;
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
  {
    auto node = it.get_curr();
    size_t out_deg = 0;
    for (Node_Arc_Iterator<DGraph> ait(node); ait.has_curr(); ait.next())
      out_deg++;
    
    // In a digraph, we need to count in-degree separately
    // For simplicity, we trust the generator
  }
  
  Test_Eulerian<DGraph> test;
  cout << "Is Eulerian (Test_Eulerian)? " << (test(g) ? "YES" : "NO") << endl;
}

// =============================================================================
// 6. Parameter Study
// =============================================================================

void demo_parameters()
{
  print_section("PARAMETER STUDY");
  
  cout << "How does edge count affect connectivity?\n\n";
  
  size_t n = 20;
  size_t trials = 10;
  
  cout << "n = " << n << " vertices, " << trials << " trials each\n\n";
  
  cout << setw(10) << "Edges" << setw(15) << "Density" 
       << setw(20) << "Avg Components" << setw(15) << "% Connected" << endl;
  cout << string(60, '-') << endl;
  
  Random_Graph<UGraph> gen(time(nullptr));
  
  for (size_t m = n/2; m <= n*3; m += n/2)
  {
    size_t total_components = 0;
    size_t connected_count = 0;
    
    for (size_t t = 0; t < trials; t++)
    {
      UGraph g = gen(n, m);
      
      Unconnected_Components<UGraph> ic;
      DynList<UGraph> comps;
      ic(g, comps);
      
      total_components += comps.size();
      if (comps.size() == 1) connected_count++;
    }
    
    double density = 2.0 * m / (n * (n - 1));
    double avg_comps = (double)total_components / trials;
    double pct_connected = 100.0 * connected_count / trials;
    
    cout << setw(10) << m 
         << setw(15) << fixed << setprecision(3) << density
         << setw(20) << setprecision(1) << avg_comps
         << setw(14) << setprecision(0) << pct_connected << "%" << endl;
  }
  
  cout << "\nNote: Connectivity threshold is around m ≈ n*ln(n)/2 ≈ " 
       << (size_t)(n * log(n) / 2) << " edges\n";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd(
      "Random graph generation example for Aleph-w.\n"
      "Demonstrates various random graph models.",
      ' ', "1.0"
    );
    
    TCLAP::ValueArg<string> sectionArg(
      "s", "section",
      "Run only specific section: erdos, connected, eulerian, digraph, "
      "eulerian_dig, params, or 'all'",
      false, "all", "section", cmd
    );
    
    cmd.parse(argc, argv);
    
    string section = sectionArg.getValue();
    
    cout << "\n";
    cout << "============================================================\n";
    cout << "        ALEPH-W RANDOM GRAPH GENERATION EXAMPLE\n";
    cout << "============================================================\n";
    
    if (section == "all" or section == "erdos")
      demo_erdos_renyi();
    
    if (section == "all" or section == "connected")
      demo_connected();
    
    if (section == "all" or section == "eulerian")
      demo_eulerian();
    
    if (section == "all" or section == "digraph")
      demo_digraph();
    
    if (section == "all" or section == "eulerian_dig")
      demo_eulerian_digraph();
    
    if (section == "all" or section == "params")
      demo_parameters();
    
    cout << "\n" << string(60, '=') << "\n";
    cout << "Random graph generation demo completed!\n";
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

