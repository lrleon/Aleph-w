/**
 * @file min_cut_example.cc
 * @brief Educational examples for minimum cut algorithms
 *
 * This file provides comprehensive, well-documented examples of using
 * the Karger-Stein and Stoer-Wagner min-cut algorithms in Aleph-w.
 *
 * ============================================================================
 * WHAT IS A MINIMUM CUT?
 * ============================================================================
 *
 * A minimum cut (min-cut) of a graph is a partition of vertices into two
 * non-empty sets S and T such that the total weight of edges crossing
 * between S and T is minimized.
 *
 * Key Properties:
 * - Every graph with n vertices has at least one min-cut
 * - A graph can have multiple min-cuts with the same weight
 * - Min-cut ≤ min degree of any vertex
 * - For complete graph Kn: min-cut = n-1
 *
 * ============================================================================
 * ALGORITHMS COMPARED
 * ============================================================================
 *
 * 1. KARGER-STEIN (Randomized)
 *    - Time: O(n² log³ n)
 *    - Space: O(n + m)
 *    - Pros: Fast for large graphs, simple concept
 *    - Cons: Probabilistic (may need multiple runs)
 *    - Best for: Large sparse graphs, approximate solutions OK
 *
 * 2. STOER-WAGNER (Deterministic)
 *    - Time: O(nm + n² log n)
 *    - Space: O(n²)
 *    - Pros: Always correct, handles weights naturally
 *    - Cons: O(n²) space for adjacency matrix
 *    - Best for: Weighted graphs, exact solution required
 *
 * ============================================================================
 * COMPILE & RUN
 * ============================================================================
 *
 *   g++ -std=c++20 -O2 -I.. -o min_cut_example min_cut_example.cc \
 *       -lgsl -lgslcblas -lpthread
 *   ./min_cut_example
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

#include <Karger_Stein.H>
#include <Stoer_Wagner.H>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Helper: Print partition
// ============================================================================
template <typename GT>
void print_partition(const string& name,
                     const DynList<typename GT::Node*>& partition)
{
  cout << "  " << name << ": { ";
  bool first = true;
  for (auto node : partition)
  {
    if (!first) cout << ", ";
    cout << node->get_info();
    first = false;
  }
  cout << " }\n";
}

// ============================================================================
// Helper: Print cut edges
// ============================================================================
template <typename GT>
void print_cut_edges(GT& g, const DynList<typename GT::Arc*>& cut)
{
  cout << "  Cut edges:\n";
  for (auto arc : cut)
  {
    cout << "    " << g.get_src_node(arc)->get_info()
         << " --(" << arc->get_info() << ")-- "
         << g.get_tgt_node(arc)->get_info() << "\n";
  }
}

// ============================================================================
// EXAMPLE 1: Network Reliability Analysis
// ============================================================================
// Use case: Find the weakest point in a network topology
// Real-world: Data center, power grid, social network analysis
// ============================================================================

void example_network_reliability()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 1: Network Reliability Analysis                       ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  cout << "SCENARIO: A company has 6 offices connected by network links.\n";
  cout << "          We need to find the minimum number of links that,\n";
  cout << "          if cut, would split the network into two parts.\n\n";

  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  GT network;

  // Create offices
  cout << "STEP 1: Building network topology\n";
  cout << "        Offices: HQ, Branch1, Branch2, Branch3, Remote1, Remote2\n\n";

  auto hq      = network.insert_node("HQ");
  auto branch1 = network.insert_node("Branch1");
  auto branch2 = network.insert_node("Branch2");
  auto branch3 = network.insert_node("Branch3");
  auto remote1 = network.insert_node("Remote1");
  auto remote2 = network.insert_node("Remote2");

  // HQ is well-connected (hub topology)
  network.insert_arc(hq, branch1, 1); network.insert_arc(branch1, hq, 1);
  network.insert_arc(hq, branch2, 1); network.insert_arc(branch2, hq, 1);
  network.insert_arc(hq, branch3, 1); network.insert_arc(branch3, hq, 1);

  // Branches have some redundancy
  network.insert_arc(branch1, branch2, 1); network.insert_arc(branch2, branch1, 1);
  network.insert_arc(branch2, branch3, 1); network.insert_arc(branch3, branch2, 1);

  // Remote offices only connected to one branch each (vulnerability!)
  network.insert_arc(branch1, remote1, 1); network.insert_arc(remote1, branch1, 1);
  network.insert_arc(branch3, remote2, 1); network.insert_arc(remote2, branch3, 1);

  cout << "        Network structure:\n";
  cout << "                     Remote1\n";
  cout << "                       |\n";
  cout << "              Branch1--+--HQ--Branch3--Remote2\n";
  cout << "                 |          |\n";
  cout << "              Branch2-------+\n\n";

  cout << "        Total: " << network.get_num_nodes() << " offices, "
       << network.get_num_arcs() / 2 << " unique links\n\n";

  // Find min-cut using Karger-Stein
  cout << "STEP 2: Finding minimum cut with Karger-Stein\n";
  cout << "        (Running 20 iterations for accuracy)\n\n";

  Karger_Stein_Min_Cut<GT> ks;
  DynList<GT::Node*> S, T;
  DynList<GT::Arc*> cut;

  size_t min_cut = ks(network, S, T, cut, 20);

  cout << "RESULT:\n";
  cout << "  Minimum cut size: " << min_cut / 2 << " links\n";  // Divide by 2 for undirected
  print_partition<GT>("Partition 1", S);
  print_partition<GT>("Partition 2", T);

  cout << "\nINTERPRETATION:\n";
  cout << "  The network can be split by cutting just " << min_cut / 2 << " link(s).\n";
  cout << "  RECOMMENDATION: Add redundant links to vulnerable offices.\n\n";
}

// ============================================================================
// EXAMPLE 2: Weighted Network - Bandwidth Optimization
// ============================================================================
// Use case: Find the bottleneck in a network with different link capacities
// Real-world: Network capacity planning, traffic analysis
// ============================================================================

void example_weighted_bandwidth()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 2: Weighted Network - Bandwidth Analysis              ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  cout << "SCENARIO: A data center has servers connected with varying bandwidth.\n";
  cout << "          Find the minimum total bandwidth that could bottleneck.\n\n";

  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  GT datacenter;

  // Create servers
  auto web1 = datacenter.insert_node("Web1");
  auto web2 = datacenter.insert_node("Web2");
  auto app  = datacenter.insert_node("App");
  auto db   = datacenter.insert_node("Database");
  auto cache = datacenter.insert_node("Cache");

  cout << "STEP 1: Network topology with bandwidth (Gbps)\n\n";

  // High bandwidth core connections
  datacenter.insert_arc(web1, app, 10);   datacenter.insert_arc(app, web1, 10);
  datacenter.insert_arc(web2, app, 10);   datacenter.insert_arc(app, web2, 10);
  datacenter.insert_arc(app, cache, 20);  datacenter.insert_arc(cache, app, 20);
  datacenter.insert_arc(cache, db, 5);    datacenter.insert_arc(db, cache, 5);  // Bottleneck!
  datacenter.insert_arc(app, db, 2);      datacenter.insert_arc(db, app, 2);    // Backup link

  cout << "        Web1 --10-- App --20-- Cache --5-- Database\n";
  cout << "        Web2 --10--  |                 |\n";
  cout << "                     +-------2---------+\n\n";

  // Find min-cut using Stoer-Wagner (handles weights properly)
  cout << "STEP 2: Finding minimum cut with Stoer-Wagner\n\n";

  Stoer_Wagner_Min_Cut<GT> sw;
  DynList<GT::Node*> S, T;
  DynList<GT::Arc*> cut;

  int min_bandwidth = sw(datacenter, S, T, cut);

  cout << "RESULT:\n";
  cout << "  Minimum cut bandwidth: " << min_bandwidth / 2 << " Gbps\n";
  print_partition<GT>("Partition 1", S);
  print_partition<GT>("Partition 2", T);
  cout << "\n  Bottleneck links:\n";
  for (auto arc : cut)
  {
    cout << "    " << datacenter.get_src_node(arc)->get_info()
         << " <-> " << datacenter.get_tgt_node(arc)->get_info()
         << " (" << arc->get_info() << " Gbps)\n";
  }

  cout << "\nINTERPRETATION:\n";
  cout << "  The database access is the bottleneck at " << min_bandwidth / 2 << " Gbps.\n";
  cout << "  RECOMMENDATION: Upgrade Cache-Database link or add more paths.\n\n";
}

// ============================================================================
// EXAMPLE 3: Community Detection
// ============================================================================
// Use case: Find natural divisions in a social network
// Real-world: Social media analysis, market segmentation
// ============================================================================

void example_community_detection()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 3: Community Detection in Social Network              ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  cout << "SCENARIO: A social network with two friend groups loosely connected.\n";
  cout << "          Find the natural division between communities.\n\n";

  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  GT social;

  // Group 1: Tech enthusiasts
  auto alice = social.insert_node("Alice");
  auto bob   = social.insert_node("Bob");
  auto carol = social.insert_node("Carol");

  // Group 2: Sports fans
  auto dave  = social.insert_node("Dave");
  auto eve   = social.insert_node("Eve");
  auto frank = social.insert_node("Frank");

  cout << "STEP 1: Building social connections\n\n";

  // Dense connections within Group 1
  social.insert_arc(alice, bob, 1);   social.insert_arc(bob, alice, 1);
  social.insert_arc(alice, carol, 1); social.insert_arc(carol, alice, 1);
  social.insert_arc(bob, carol, 1);   social.insert_arc(carol, bob, 1);

  // Dense connections within Group 2
  social.insert_arc(dave, eve, 1);    social.insert_arc(eve, dave, 1);
  social.insert_arc(dave, frank, 1);  social.insert_arc(frank, dave, 1);
  social.insert_arc(eve, frank, 1);   social.insert_arc(frank, eve, 1);

  // Sparse connections between groups (just 1 link)
  social.insert_arc(carol, dave, 1);  social.insert_arc(dave, carol, 1);

  cout << "        Group 1 (Tech):     Group 2 (Sports):\n";
  cout << "        Alice--Bob          Dave--Eve\n";
  cout << "           \\  /               \\  /\n";
  cout << "           Carol ---- Dave   Frank\n";
  cout << "              (weak link)\n\n";

  // Run both algorithms and compare
  cout << "STEP 2: Running both algorithms\n\n";

  // Karger-Stein
  Karger_Stein_Min_Cut<GT> ks;
  DynList<GT::Node*> ks_S, ks_T;
  DynList<GT::Arc*> ks_cut;
  size_t ks_result = ks(social, ks_S, ks_T, ks_cut, 30);

  // Stoer-Wagner
  Stoer_Wagner_Min_Cut<GT> sw;
  DynList<GT::Node*> sw_S, sw_T;
  DynList<GT::Arc*> sw_cut;
  int sw_result = sw(social, sw_S, sw_T, sw_cut);

  cout << "KARGER-STEIN RESULT:\n";
  cout << "  Min-cut size: " << ks_result / 2 << "\n";
  print_partition<GT>("Community 1", ks_S);
  print_partition<GT>("Community 2", ks_T);

  cout << "\nSTOER-WAGNER RESULT:\n";
  cout << "  Min-cut size: " << sw_result / 2 << "\n";
  print_partition<GT>("Community 1", sw_S);
  print_partition<GT>("Community 2", sw_T);

  cout << "\nINTERPRETATION:\n";
  cout << "  Both algorithms identify the two communities correctly.\n";
  cout << "  The Carol-Dave connection is the bridge between groups.\n\n";
}

// ============================================================================
// EXAMPLE 4: Algorithm Comparison - Accuracy vs Speed
// ============================================================================

void example_algorithm_comparison()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 4: Algorithm Comparison                               ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  cout << "SCENARIO: Compare Karger-Stein vs Stoer-Wagner on the same graph.\n\n";

  using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  GT g;

  // Build two 5-cliques connected by 3 edges
  const int CLIQUE_SIZE = 5;
  const int BRIDGE_COUNT = 3;

  vector<GT::Node*> left(CLIQUE_SIZE), right(CLIQUE_SIZE);

  for (int i = 0; i < CLIQUE_SIZE; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(CLIQUE_SIZE + i);
  }

  // Left clique
  for (int i = 0; i < CLIQUE_SIZE; ++i)
    for (int j = i + 1; j < CLIQUE_SIZE; ++j)
    {
      g.insert_arc(left[i], left[j], 1);
      g.insert_arc(left[j], left[i], 1);
    }

  // Right clique
  for (int i = 0; i < CLIQUE_SIZE; ++i)
    for (int j = i + 1; j < CLIQUE_SIZE; ++j)
    {
      g.insert_arc(right[i], right[j], 1);
      g.insert_arc(right[j], right[i], 1);
    }

  // Bridges
  for (int i = 0; i < BRIDGE_COUNT; ++i)
  {
    g.insert_arc(left[i], right[i], 1);
    g.insert_arc(right[i], left[i], 1);
  }

  cout << "GRAPH: Two K" << CLIQUE_SIZE << " cliques connected by "
       << BRIDGE_COUNT << " edges\n";
  cout << "       Expected min-cut: " << BRIDGE_COUNT << "\n";
  cout << "       Nodes: " << g.get_num_nodes() << ", Edges: "
       << g.get_num_arcs() / 2 << "\n\n";

  // Run Karger-Stein with varying iterations
  cout << "KARGER-STEIN (varying iterations):\n";
  Karger_Stein_Min_Cut<GT> ks;

  for (int iters : {1, 5, 10, 20, 50})
  {
    DynList<GT::Node*> S, T;
    DynList<GT::Arc*> cut;
    size_t result = ks(g, S, T, cut, iters);
    cout << "  " << setw(2) << iters << " iterations: min-cut = "
         << result / 2 << "\n";
  }

  // Run Stoer-Wagner
  cout << "\nSTOER-WAGNER (deterministic):\n";
  Stoer_Wagner_Min_Cut<GT> sw;
  DynList<GT::Node*> S, T;
  DynList<GT::Arc*> cut;
  int result = sw(g, S, T, cut);
  cout << "  Result: min-cut = " << result / 2 << "\n";

  cout << "\nCONCLUSION:\n";
  cout << "  - Karger-Stein converges to correct answer with more iterations\n";
  cout << "  - Stoer-Wagner always gives exact answer in one run\n";
  cout << "  - Choose based on: graph size, accuracy needs, weighted/unweighted\n\n";
}

// ============================================================================
// EXAMPLE 5: Practical API Usage Patterns
// ============================================================================

void example_api_patterns()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 5: API Usage Patterns                                 ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  GT g;

  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b, 1); g.insert_arc(b, a, 1);
  g.insert_arc(b, c, 1); g.insert_arc(c, b, 1);
  g.insert_arc(c, d, 1); g.insert_arc(d, c, 1);
  g.insert_arc(a, d, 1); g.insert_arc(d, a, 1);

  cout << "PATTERN 1: Basic single-run (Karger-Stein)\n";
  cout << "-------------------------------------------\n";
  cout << R"(
  Karger_Stein_Min_Cut<GT> ks;
  DynList<GT::Node*> S, T;
  DynList<GT::Arc*> cut;
  size_t result = ks(g, S, T, cut);
)";

  {
    Karger_Stein_Min_Cut<GT> ks;
    DynList<GT::Node*> S, T;
    DynList<GT::Arc*> cut;
    size_t result = ks(g, S, T, cut);
    cout << "  Result: " << result << "\n\n";
  }

  cout << "PATTERN 2: Multiple iterations for accuracy (Karger-Stein)\n";
  cout << "-----------------------------------------------------------\n";
  cout << R"(
  size_t result = ks(g, S, T, cut, 20);
)";

  {
    Karger_Stein_Min_Cut<GT> ks;
    DynList<GT::Node*> S, T;
    DynList<GT::Arc*> cut;
    size_t result = ks(g, S, T, cut, 20);
    cout << "  Result: " << result << "\n\n";
  }

  cout << "PATTERN 3: Reproducible results with seed\n";
  cout << "------------------------------------------\n";
  cout << R"(
  Karger_Stein_Min_Cut<GT> ks(12345);  // Seed = 12345
  // Or: ks.set_seed(12345);
)";

  {
    Karger_Stein_Min_Cut<GT> ks1(12345);
    Karger_Stein_Min_Cut<GT> ks2(12345);
    DynList<GT::Node*> S1, T1, S2, T2;
    DynList<GT::Arc*> cut1, cut2;
    size_t r1 = ks1(g, S1, T1, cut1);
    size_t r2 = ks2(g, S2, T2, cut2);
    cout << "  Same seed → same result: " << (r1 == r2 ? "YES" : "NO") << "\n\n";
  }

  cout << "PATTERN 4: Weighted graph (Stoer-Wagner)\n";
  cout << "-----------------------------------------\n";
  cout << R"(
  Stoer_Wagner_Min_Cut<GT> sw;
  int weight = sw(g, S, T, cut);  // Returns total weight of cut edges
)";

  {
    Stoer_Wagner_Min_Cut<GT> sw;
    DynList<GT::Node*> S, T;
    DynList<GT::Arc*> cut;
    int weight = sw(g, S, T, cut);
    cout << "  Result: " << weight << "\n\n";
  }

  cout << "PATTERN 5: Just the weight, no partition (Stoer-Wagner)\n";
  cout << "--------------------------------------------------------\n";
  cout << R"(
  int weight = sw.min_cut_weight(g);  // Slightly faster
)";

  {
    Stoer_Wagner_Min_Cut<GT> sw;
    int weight = sw.min_cut_weight(g);
    cout << "  Result: " << weight << "\n\n";
  }

  cout << "PATTERN 6: Unweighted graph with Stoer-Wagner\n";
  cout << "----------------------------------------------\n";
  cout << R"(
  Stoer_Wagner_Min_Cut<GT, Unit_Weight<GT>> sw;
  size_t num_edges = sw(g, S, T, cut);  // Counts edges, ignores weights
)";

  {
    Stoer_Wagner_Min_Cut<GT, Unit_Weight<GT>> sw;
    DynList<GT::Node*> S, T;
    DynList<GT::Arc*> cut;
    size_t num_edges = sw(g, S, T, cut);
    cout << "  Result: " << num_edges << " edges\n\n";
  }
}

// ============================================================================
// EXAMPLE 6: When to Use Which Algorithm
// ============================================================================

void example_algorithm_selection()
{
  cout << "╔════════════════════════════════════════════════════════════════╗\n";
  cout << "║  EXAMPLE 6: Algorithm Selection Guide                          ║\n";
  cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

  cout << "DECISION FLOWCHART:\n";
  cout << "═══════════════════\n\n";

  cout << "  ┌─────────────────────────────────────────┐\n";
  cout << "  │ Do you need the EXACT minimum cut?      │\n";
  cout << "  └────────────────┬────────────────────────┘\n";
  cout << "                   │\n";
  cout << "         ┌─────────┴─────────┐\n";
  cout << "         │                   │\n";
  cout << "        YES                  NO\n";
  cout << "         │                   │\n";
  cout << "         ▼                   ▼\n";
  cout << "  ┌──────────────┐    ┌──────────────────────┐\n";
  cout << "  │ STOER-WAGNER │    │ Is the graph large?  │\n";
  cout << "  │ Deterministic│    │ (n > 100)            │\n";
  cout << "  └──────────────┘    └──────────┬───────────┘\n";
  cout << "                                 │\n";
  cout << "                       ┌─────────┴─────────┐\n";
  cout << "                       │                   │\n";
  cout << "                      YES                  NO\n";
  cout << "                       │                   │\n";
  cout << "                       ▼                   ▼\n";
  cout << "                ┌──────────────┐    ┌──────────────┐\n";
  cout << "                │ KARGER-STEIN │    │ STOER-WAGNER │\n";
  cout << "                │ O(n² log³ n) │    │ Simple cases │\n";
  cout << "                └──────────────┘    └──────────────┘\n\n";

  cout << "COMPARISON TABLE:\n";
  cout << "═════════════════\n\n";

  cout << "  ┌────────────────┬───────────────────┬───────────────────┐\n";
  cout << "  │ Criterion      │ Karger-Stein      │ Stoer-Wagner      │\n";
  cout << "  ├────────────────┼───────────────────┼───────────────────┤\n";
  cout << "  │ Time           │ O(n² log³ n)      │ O(nm + n² log n)  │\n";
  cout << "  │ Space          │ O(n + m)          │ O(n²)             │\n";
  cout << "  │ Deterministic? │ No (randomized)   │ Yes               │\n";
  cout << "  │ Weighted?      │ No                │ Yes               │\n";
  cout << "  │ Exact?         │ High probability  │ Always            │\n";
  cout << "  │ Large graphs   │ ✓ Better          │ OK                │\n";
  cout << "  │ Small graphs   │ OK                │ ✓ Better          │\n";
  cout << "  └────────────────┴───────────────────┴───────────────────┘\n\n";

  cout << "PRACTICAL RECOMMENDATIONS:\n";
  cout << "══════════════════════════\n\n";

  cout << "  1. NETWORK RELIABILITY → Stoer-Wagner (exact answer matters)\n";
  cout << "  2. BANDWIDTH ANALYSIS  → Stoer-Wagner (needs weights)\n";
  cout << "  3. COMMUNITY DETECTION → Either (approximate OK)\n";
  cout << "  4. LARGE SOCIAL GRAPH  → Karger-Stein (faster)\n";
  cout << "  5. VLSI DESIGN         → Stoer-Wagner (precision critical)\n\n";
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "\n";
  cout << "████████████████████████████████████████████████████████████████████\n";
  cout << "██                                                                ██\n";
  cout << "██   MINIMUM CUT ALGORITHMS - EDUCATIONAL EXAMPLES                ██\n";
  cout << "██   Karger-Stein (Randomized) & Stoer-Wagner (Deterministic)    ██\n";
  cout << "██                                                                ██\n";
  cout << "████████████████████████████████████████████████████████████████████\n";
  cout << "\n";

  example_network_reliability();
  example_weighted_bandwidth();
  example_community_detection();
  example_algorithm_comparison();
  example_api_patterns();
  example_algorithm_selection();

  cout << "════════════════════════════════════════════════════════════════════\n";
  cout << "  All examples completed successfully!\n";
  cout << "════════════════════════════════════════════════════════════════════\n\n";

  return 0;
}
