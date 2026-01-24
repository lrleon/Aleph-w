/**
 * @file tpl_kgraph_example.cc
 * @brief Educational examples for k-connected graphs and connectivity analysis
 * 
 * WHAT IS K-CONNECTIVITY?
 * =======================
 * A graph is k-connected if it remains connected after removing any k-1 vertices
 * Measures robustness: Higher k = more fault-tolerant network
 * 
 * TYPES OF CONNECTIVITY:
 * ======================
 * - 0-connected: Disconnected graph
 * - 1-connected: Has bridges (removing 1 edge disconnects)
 * - 2-connected: No bridges, but has cut vertices
 * - 3-connected: No single point can disconnect graph
 * 
 * EDGE CONNECTIVITY:
 * ==================
 * Minimum number of edges to remove to disconnect graph
 * Computed using maximum flow algorithms
 * 
 * WHY IT MATTERS:
 * ===============
 * Network reliability, fault tolerance, redundancy analysis
 * Critical for designing robust communication/transportation networks
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_kgraph_example tpl_kgraph_example.cc
 * ./tpl_kgraph_example
 */

#include <iostream>
#include <tpl_graph.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== K-Connected Graphs: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Understanding 1-Connectivity (Bridges)
  // =========================================================================
  {
    cout << "--- Example 1: 1-Connected Graph (Has Bridges) ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT network;
    
    cout << "SCENARIO: Simple network with weak link\n";
    cout << "=======================================\n\n";
    
    // Build network with a bridge
    auto cluster1_a = network.insert_node("A");
    auto cluster1_b = network.insert_node("B");
    auto cluster2_c = network.insert_node("C");
    auto cluster2_d = network.insert_node("D");
    
    cout << "Network topology:\n";
    cout << "  Cluster 1: A --- B\n";
    cout << "                   |\n";
    cout << "                BRIDGE (single connection)\n";
    cout << "                   |\n";
    cout << "  Cluster 2: C --- D\n\n";
    
    // Connections within clusters
    network.insert_arc(cluster1_a, cluster1_b);
    network.insert_arc(cluster1_b, cluster1_a);
    
    network.insert_arc(cluster2_c, cluster2_d);
    network.insert_arc(cluster2_d, cluster2_c);
    
    // BRIDGE: Single connection between clusters
    network.insert_arc(cluster1_b, cluster2_c);
    network.insert_arc(cluster2_c, cluster1_b);
    
    cout << "CONNECTIVITY ANALYSIS:\n";
    cout << "  * Edge connectivity: 1\n";
    cout << "  * Meaning: Removing 1 edge (the bridge) disconnects graph\n";
    cout << "  * Risk: Single point of failure!\n\n";
    
    cout << "WHAT HAPPENS IF BRIDGE FAILS?\n";
    cout << "  Cluster 1 (A,B) is isolated from Cluster 2 (C,D)\n";
    cout << "  Network splits into two components\n";
    cout << "  No redundancy - catastrophic failure!\n\n";
    
    cout << "REAL-WORLD: Internet backbone with single link between regions\n";
    cout << "              One cable cut = complete regional isolation\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: 2-Connected Graph (More Robust)
  // =========================================================================
  {
    cout << "--- Example 2: 2-Connected Graph (No Bridges) ---\n\n";
    
    using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
    GT network;
    
    cout << "SCENARIO: Improved network with redundancy\n";
    cout << "==========================================\n\n";
    
    auto a = network.insert_node("A");
    auto b = network.insert_node("B");
    auto c = network.insert_node("C");
    auto d = network.insert_node("D");
    
    cout << "Network topology (cycle with diameter):\n";
    cout << "    A --- B\n";
    cout << "    |  \\  |\n";
    cout << "    |   \\ |  (diagonal A-C adds redundancy)\n";
    cout << "    D --- C\n\n";
    
    // Create cycle
    network.insert_arc(a, b);
    network.insert_arc(b, c);
    network.insert_arc(c, d);
    network.insert_arc(d, a);
    
    // Add diagonal for 2-connectivity
    network.insert_arc(a, c);
    
    // Make undirected
    network.insert_arc(b, a);
    network.insert_arc(c, b);
    network.insert_arc(d, c);
    network.insert_arc(a, d);
    network.insert_arc(c, a);
    
    cout << "CONNECTIVITY ANALYSIS:\n";
    cout << "  * Edge connectivity: 2\n";
    cout << "  * Meaning: Need to remove 2 edges to disconnect\n";
    cout << "  * Benefit: Can survive any single edge failure!\n\n";
    
    cout << "FAILURE SCENARIOS:\n";
    cout << "  1. Edge A-B fails:\n";
    cout << "     Alternative path: A → D → C → B\n";
    cout << "     Or: A → C → B\n";
    cout << "     Network stays connected ✓\n\n";
    
    cout << "  2. Edge B-C fails:\n";
    cout << "     Alternative: B → A → C (via diagonal)\n";
    cout << "     Or: B → A → D → C\n";
    cout << "     Network stays connected ✓\n\n";
    
    cout << "KEY INSIGHT: 2-connectivity = no single point of failure\n";
    cout << "             Every node reachable via multiple paths\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Comparing Connectivity Levels
  // =========================================================================
  {
    cout << "--- Example 3: Connectivity Level Comparison ---\n\n";
    
    cout << "CONNECTIVITY LEVELS:\n";
    cout << "===================\n\n";
    
    cout << "k=0 (DISCONNECTED):\n";
    cout << "  A   B     C   D\n";
    cout << "  Completely separate components\n";
    cout << "  Fault tolerance: None\n";
    cout << "  Use case: Independent subsystems\n\n";
    
    cout << "k=1 (TREE/BRIDGE):\n";
    cout << "  A---B     C---D\n";
    cout << "       \\   /\n";
    cout << "        BRIDGE\n";
    cout << "  Minimal connectivity\n";
    cout << "  Fault tolerance: Very low (any edge failure = disconnect)\n";
    cout << "  Use case: Hierarchical networks, cost-critical designs\n\n";
    
    cout << "k=2 (CYCLE):\n";
    cout << "  A---B\n";
    cout << "  |   |\n";
    cout << "  D---C\n";
    cout << "  Survives 1 edge failure\n";
    cout << "  Fault tolerance: Good (backup paths exist)\n";
    cout << "  Use case: Most practical networks\n\n";
    
    cout << "k=3 (HIGHLY CONNECTED):\n";
    cout << "  A---B\n";
    cout << "  |\\ /|\n";
    cout << "  |X X|  (many cross-connections)\n";
    cout << "  |/ \\|\n";
    cout << "  D---C\n";
    cout << "  Survives 2 edge failures\n";
    cout << "  Fault tolerance: Excellent\n";
    cout << "  Use case: Critical infrastructure, data centers\n\n";
    
    cout << "TRADE-OFF:\n";
    cout << "  Higher k → More reliable BUT More expensive\n";
    cout << "  More edges → Higher cost (cables, maintenance)\n";
    cout << "  Design choice depends on criticality vs budget\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Calculating Edge Connectivity
  // =========================================================================
  {
    cout << "--- Example 4: Computing Edge Connectivity ---\n\n";
    
    using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
    GT g;
    
    cout << "ALGORITHM: Uses maximum flow\n";
    cout << "============================\n\n";
    
    // Create a simple graph
    auto n1 = g.insert_node(1);
    auto n2 = g.insert_node(2);
    auto n3 = g.insert_node(3);
    auto n4 = g.insert_node(4);
    
    // Create 2-connected graph
    g.insert_arc(n1, n2);
    g.insert_arc(n2, n3);
    g.insert_arc(n3, n4);
    g.insert_arc(n4, n1);
    g.insert_arc(n1, n3); // diagonal
    
    cout << "Graph: Square with diagonal\n";
    cout << "  1---2\n";
    cout << "  |\\  |\n";
    cout << "  | \\ |\n";
    cout << "  4---3\n\n";
    
    cout << "COMPUTATION STEPS:\n";
    cout << "1. Pick a source node (say, node 1)\n";
    cout << "2. For each other node as sink:\n";
    cout << "   a. Build unit-capacity network\n";
    cout << "   b. Compute max-flow from source to sink\n";
    cout << "   c. Max-flow = number of edge-disjoint paths\n";
    cout << "3. Minimum over all sinks = edge connectivity\n\n";
    
    cout << "EXAMPLE CALCULATION:\n";
    cout << "  Node 1 to Node 2: max-flow = 2 (direct + via 4 or 3)\n";
    cout << "  Node 1 to Node 3: max-flow = 2 (direct + via 2 or 4)\n";
    cout << "  Node 1 to Node 4: max-flow = 2 (direct + via 2-3)\n";
    cout << "  Minimum = 2\n";
    cout << "  Edge connectivity = 2 ✓\n\n";
    
    cout << "COMPLEXITY:\n";
    cout << "  * Need O(V) max-flow computations\n";
    cout << "  * Each max-flow: O(V * E^2) with Edmonds-Karp\n";
    cout << "  * Total: O(V^2 * E^2)\n";
    cout << "  * Practical for graphs with thousands of nodes\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Real-World Network Design
  // =========================================================================
  {
    cout << "--- Example 5: Designing Reliable Networks ---\n\n";
    
    cout << "CASE STUDY: Data Center Network\n";
    cout << "===============================\n\n";
    
    cout << "REQUIREMENTS:\n";
    cout << "  * 100 servers must stay connected\n";
    cout << "  * Network must survive any 2 simultaneous failures\n";
    cout << "  * Minimize number of switches (cost)\n\n";
    
    cout << "SOLUTION: 3-Connected Topology\n";
    cout << "  * Each server connects to 3 switches\n";
    cout << "  * Switches form highly connected mesh\n";
    cout << "  * Any 2 links can fail, connectivity preserved\n\n";
    
    cout << "DESIGN CHOICES:\n\n";
    
    cout << "Option A: Tree (k=1)\n";
    cout << "  Pros: Minimal cost (n-1 edges)\n";
    cout << "  Cons: No fault tolerance\n";
    cout << "  Decision: ✗ Too risky for data center\n\n";
    
    cout << "Option B: Ring (k=2)\n";
    cout << "  Pros: Moderate cost, survives 1 failure\n";
    cout << "  Cons: Still vulnerable to 2 failures\n";
    cout << "  Decision: ✗ Insufficient for requirements\n\n";
    
    cout << "Option C: 3-Connected Mesh (k=3)\n";
    cout << "  Pros: Survives 2 failures (meets requirement!)\n";
    cout << "  Cons: Higher cost (more cables)\n";
    cout << "  Decision: ✓ Best fit for critical infrastructure\n\n";
    
    cout << "COST-BENEFIT ANALYSIS:\n";
    cout << "  Extra cost: ~50% more cables than ring\n";
    cout << "  Benefit: Can survive 2 simultaneous failures\n";
    cout << "  ROI: Downtime costs far exceed cable costs\n";
    cout << "  Conclusion: Worth the investment\n\n";
  }
  
  cout << "=== SUMMARY: K-Connected Graphs ===\n";
  cout << "\n1. DEFINITION:\n";
  cout << "   k-connected: Removing any k-1 vertices keeps graph connected\n";
  cout << "   Edge connectivity: Min edges to remove to disconnect\n";
  cout << "   Higher k = more robust network\n";
  cout << "\n2. CONNECTIVITY LEVELS:\n";
  cout << "   k=1: Tree-like, has bridges (weak)\n";
  cout << "   k=2: No bridges, survives 1 failure (good)\n";
  cout << "   k=3+: Highly redundant, very robust (excellent)\n";
  cout << "\n3. HOW TO COMPUTE:\n";
  cout << "   Use maximum flow algorithm\n";
  cout << "   Build unit-capacity network\n";
  cout << "   Compute min-cut = edge connectivity\n";
  cout << "   Time: O(V^2 * E^2)\n";
  cout << "\n4. DESIGN PRINCIPLES:\n";
  cout << "   * Critical systems: k ≥ 3 (data centers, hospitals)\n";
  cout << "   * Important systems: k = 2 (corporate networks)\n";
  cout << "   * Non-critical: k = 1 acceptable (home networks)\n";
  cout << "   * Always consider cost vs reliability trade-off\n";
  cout << "\n5. REAL-WORLD APPLICATIONS:\n";
  cout << "   ✓ Internet backbone design\n";
  cout << "   ✓ Power grid planning\n";
  cout << "   ✓ Transportation networks\n";
  cout << "   ✓ Data center topologies\n";
  cout << "   ✓ Telecommunications infrastructure\n";
  cout << "\n6. FAILURE ANALYSIS:\n";
  cout << "   k=1: Any edge failure → disconnect\n";
  cout << "   k=2: Survives 1 edge failure\n";
  cout << "   k=3: Survives 2 edge failures\n";
  cout << "   General: Survives k-1 failures\n";
  cout << "\n7. COST CONSIDERATIONS:\n";
  cout << "   Tree (k=1): n-1 edges (minimum)\n";
  cout << "   Cycle (k=2): n edges (+1)\n";
  cout << "   Complete (k=n-1): n*(n-1)/2 edges (maximum)\n";
  cout << "   Practical: k=2 or k=3 for most applications\n";
  
  return 0;
}
