/**
 * @file tpl_netcapgraph_example.cc
 * @brief Educational examples for capacity-constrained flow networks
 * 
 * WHAT IS A CAPACITY NETWORK?
 * ============================
 * Network where arcs have capacity limits (max flow through arc)
 * Foundation for maximum flow and minimum cut problems
 * Core structure for many optimization algorithms
 * 
 * MAXIMUM FLOW PROBLEM:
 * ====================
 * Given: Source node, sink node, arc capacities
 * Find: Maximum amount of "flow" from source to sink
 * Subject to:
 *   1. Flow conservation (in = out at each node)
 *   2. Capacity constraints (flow ≤ capacity)
 * 
 * KEY ALGORITHMS:
 * ===============
 * - Ford-Fulkerson: Augmenting paths, O(E * max_flow)
 * - Edmonds-Karp: BFS for paths, O(V * E^2)
 * - Dinic: Level graphs, O(V^2 * E)
 * - Push-Relabel: O(V^3) or O(V^2 * sqrt(E))
 * 
 * COMPILE & RUN:
 * ==============
 * g++ -std=c++20 -I.. -o tpl_netcapgraph_example tpl_netcapgraph_example.cc
 * ./tpl_netcapgraph_example
 */

#include <iostream>
#include <tpl_netcapgraph.H>
#include <tpl_net.H>

using namespace std;
using namespace Aleph;

int main()
{
  cout << "=== Capacity Networks: Educational Examples ===\n\n";
  
  // =========================================================================
  // EXAMPLE 1: Basic Maximum Flow
  // =========================================================================
  {
    cout << "--- Example 1: Simple Max Flow Problem ---\n\n";
    
    // STEP 1: Define network type
    using Net = Net_Graph<Net_Node<string>, Net_Arc<Empty_Class, int>>;
    Net network;
    
    cout << "PROBLEM: Pipeline network\n";
    cout << "========================\n\n";
    
    // STEP 2: Create nodes
    auto source = network.insert_node("Source");
    auto a = network.insert_node("A");
    auto b = network.insert_node("B");
    auto sink = network.insert_node("Sink");
    
    cout << "Network topology:\n";
    cout << "     Source\n";
    cout << "      / \\\n";
    cout << "     A   B\n";
    cout << "      \\ /\n";
    cout << "      Sink\n\n";
    
    // STEP 3: Add arcs with capacities
    cout << "Arc capacities (gallons/minute):\n";
    network.insert_arc(source, a, 10);  // capacity 10
    network.insert_arc(source, b, 8);   // capacity 8
    network.insert_arc(a, sink, 12);    // capacity 12
    network.insert_arc(b, sink, 9);     // capacity 9
    network.insert_arc(a, b, 5);        // capacity 5 (crossover)
    
    cout << "  Source -> A: 10\n";
    cout << "  Source -> B: 8\n";
    cout << "  A -> Sink: 12\n";
    cout << "  B -> Sink: 9\n";
    cout << "  A -> B: 5 (allows redistribution)\n\n";
    
    cout << "QUESTION: What is maximum flow from Source to Sink?\n\n";
    
    cout << "SOLUTION PATHS:\n";
    cout << "  Path 1: Source -> A -> Sink (limited by 10)\n";
    cout << "  Path 2: Source -> B -> Sink (limited by 8)\n";
    cout << "  Path 3: Source -> A -> B -> Sink (limited by 5)\n\n";
    
    cout << "MAXIMUM FLOW CALCULATION:\n";
    cout << "  1. Send 10 through Source -> A -> Sink\n";
    cout << "     BUT: A -> Sink has capacity 12, can handle it\n";
    cout << "  2. Send 8 through Source -> B -> Sink\n";
    cout << "  3. Could send more through A -> B -> Sink\n";
    cout << "     But Source -> A already at capacity (10)\n\n";
    
    cout << "EXPECTED MAX FLOW: 17 units\n";
    cout << "  (Limited by source output: 10 + 8 = 18, but)\n";
    cout << "  (Actually limited by sink input: 12 + 9 = 21)\n";
    cout << "  (Real limit: 10 + 7 = 17, using A->B path)\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 2: Bottleneck Analysis
  // =========================================================================
  {
    cout << "--- Example 2: Finding Bottlenecks ---\n\n";
    
    using Net = Net_Graph<Net_Node<string>, Net_Arc<Empty_Class, int>>;
    Net network;
    
    cout << "SCENARIO: Data center network\n";
    cout << "============================\n\n";
    
    auto internet = network.insert_node("Internet");
    auto router = network.insert_node("Router");
    auto switch1 = network.insert_node("Switch1");
    auto switch2 = network.insert_node("Switch2");
    auto server_cluster = network.insert_node("Servers");
    
    cout << "Network path: Internet -> Router -> Switches -> Servers\n\n";
    
    // Capacities in Gbps
    network.insert_arc(internet, router, 100);       // 100 Gbps
    network.insert_arc(router, switch1, 40);         // 40 Gbps
    network.insert_arc(router, switch2, 40);         // 40 Gbps
    network.insert_arc(switch1, server_cluster, 50); // 50 Gbps
    network.insert_arc(switch2, server_cluster, 50); // 50 Gbps
    
    cout << "Link capacities:\n";
    cout << "  Internet -> Router: 100 Gbps\n";
    cout << "  Router -> Switch1: 40 Gbps\n";
    cout << "  Router -> Switch2: 40 Gbps\n";
    cout << "  Switch1 -> Servers: 50 Gbps\n";
    cout << "  Switch2 -> Servers: 50 Gbps\n\n";
    
    cout << "BOTTLENECK ANALYSIS:\n";
    cout << "  1. Internet connection: 100 Gbps (not a bottleneck)\n";
    cout << "  2. Router output: 40 + 40 = 80 Gbps (BOTTLENECK!)\n";
    cout << "  3. Server input: 50 + 50 = 100 Gbps (not a bottleneck)\n\n";
    
    cout << "MAXIMUM THROUGHPUT: 80 Gbps\n";
    cout << "  Limited by router's total output capacity\n";
    cout << "  Even though internet and servers can handle 100 Gbps\n\n";
    
    cout << "UPGRADE RECOMMENDATION:\n";
    cout << "  Add another 40 Gbps link from router to switches\n";
    cout << "  This would increase capacity to 100 Gbps\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 3: Minimum Cut (Max-Flow Min-Cut Theorem)
  // =========================================================================
  {
    cout << "--- Example 3: Minimum Cut Problem ---\n\n";
    
    using Net = Net_Graph<Net_Node<int>, Net_Arc<Empty_Class, int>>;
    Net network;
    
    cout << "MAX-FLOW MIN-CUT THEOREM:\n";
    cout << "=========================\n";
    cout << "The maximum flow equals the capacity of the minimum cut\n";
    cout << "Cut = partition of nodes into two sets (S and T)\n";
    cout << "Cut capacity = sum of arc capacities from S to T\n\n";
    
    // Create simple network
    auto s = network.insert_node(1);
    auto a = network.insert_node(2);
    auto b = network.insert_node(3);
    auto t = network.insert_node(4);
    
    network.insert_arc(s, a, 10);
    network.insert_arc(s, b, 10);
    network.insert_arc(a, b, 2);
    network.insert_arc(a, t, 4);
    network.insert_arc(b, t, 8);
    
    cout << "Network:\n";
    cout << "  s -> a: 10\n";
    cout << "  s -> b: 10\n";
    cout << "  a -> b: 2\n";
    cout << "  a -> t: 4\n";
    cout << "  b -> t: 8\n\n";
    
    cout << "POSSIBLE CUTS:\n\n";
    
    cout << "Cut 1: S={s}, T={a,b,t}\n";
    cout << "  Capacity: 10 + 10 = 20\n\n";
    
    cout << "Cut 2: S={s,a}, T={b,t}\n";
    cout << "  Capacity: 10 + 4 = 14\n\n";
    
    cout << "Cut 3: S={s,b}, T={a,t}\n";
    cout << "  Capacity: 10 + 8 = 18\n\n";
    
    cout << "Cut 4: S={s,a,b}, T={t}\n";
    cout << "  Capacity: 4 + 8 = 12 (MINIMUM CUT!)\n\n";
    
    cout << "CONCLUSION:\n";
    cout << "  Minimum cut capacity: 12\n";
    cout << "  Maximum flow: 12 (by theorem)\n";
    cout << "  The cut identifies the bottleneck!\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 4: Residual Graph and Augmenting Paths
  // =========================================================================
  {
    cout << "--- Example 4: Ford-Fulkerson Algorithm Concept ---\n\n";
    
    cout << "RESIDUAL GRAPH:\n";
    cout << "===============\n";
    cout << "Shows remaining capacity after current flow\n\n";
    
    cout << "Original arc: A --10--> B\n";
    cout << "Current flow: 7 units flowing\n\n";
    
    cout << "Residual graph has TWO arcs:\n";
    cout << "  1. A --3--> B (forward: remaining capacity = 10 - 7 = 3)\n";
    cout << "  2. B --7--> A (backward: can 'undo' flow = 7)\n\n";
    
    cout << "WHY BACKWARD ARCS?\n";
    cout << "  Allow algorithm to 'change its mind'\n";
    cout << "  Redirect flow along better paths\n";
    cout << "  Essential for finding optimal solution\n\n";
    
    cout << "AUGMENTING PATH:\n";
    cout << "================\n";
    cout << "Path from source to sink in residual graph\n\n";
    
    cout << "FORD-FULKERSON ALGORITHM:\n";
    cout << "1. Start with zero flow\n";
    cout << "2. While augmenting path exists in residual graph:\n";
    cout << "   a. Find path from source to sink\n";
    cout << "   b. Determine bottleneck (min capacity on path)\n";
    cout << "   c. Augment flow by bottleneck amount\n";
    cout << "   d. Update residual graph\n";
    cout << "3. No more augmenting paths → maximum flow found!\n\n";
    
    cout << "COMPLEXITY:\n";
    cout << "  Time: O(E * |max_flow|)\n";
    cout << "  Can be slow for large flows\n";
    cout << "  Edmonds-Karp improves this to O(V * E^2) using BFS\n\n";
  }
  
  // =========================================================================
  // EXAMPLE 5: Real-World Applications
  // =========================================================================
  {
    cout << "--- Example 5: Practical Applications ---\n\n";
    
    cout << "1. NETWORK ROUTING:\n";
    cout << "   ==================\n";
    cout << "   * Internet traffic routing\n";
    cout << "   * Telecommunication networks\n";
    cout << "   * Data center load balancing\n";
    cout << "   Goal: Maximize throughput\n\n";
    
    cout << "2. TRANSPORTATION:\n";
    cout << "   ===============\n";
    cout << "   * Road traffic management\n";
    cout << "   * Railway scheduling\n";
    cout << "   * Airline route planning\n";
    cout << "   Goal: Maximize vehicles/passengers moved\n\n";
    
    cout << "3. SUPPLY CHAIN:\n";
    cout << "   =============\n";
    cout << "   * Distribution networks\n";
    cout << "   * Manufacturing pipelines\n";
    cout << "   * Inventory management\n";
    cout << "   Goal: Maximize delivery capacity\n\n";
    
    cout << "4. BIPARTITE MATCHING:\n";
    cout << "   ====================\n";
    cout << "   * Job assignment (workers to tasks)\n";
    cout << "   * Stable marriage problem\n";
    cout << "   * Resource allocation\n";
    cout << "   Goal: Maximum matching size\n\n";
    
    cout << "5. IMAGE SEGMENTATION:\n";
    cout << "   ===================\n";
    cout << "   * Computer vision\n";
    cout << "   * Medical imaging\n";
    cout << "   * Object detection\n";
    cout << "   Goal: Optimal foreground/background separation\n\n";
  }
  
  cout << "=== SUMMARY: Capacity Networks ===\n";
  cout << "\n1. CORE CONCEPTS:\n";
  cout << "   * Capacity: Maximum flow through arc\n";
  cout << "   * Flow: Actual amount flowing\n";
  cout << "   * Conservation: Flow in = Flow out (except source/sink)\n";
  cout << "\n2. MAXIMUM FLOW PROBLEM:\n";
  cout << "   Input:  Network with capacities, source, sink\n";
  cout << "   Output: Maximum amount from source to sink\n";
  cout << "   Time:   O(V * E^2) with Edmonds-Karp\n";
  cout << "\n3. MINIMUM CUT PROBLEM:\n";
  cout << "   Input:  Same as max flow\n";
  cout << "   Output: Partition with minimum cut capacity\n";
  cout << "   Result: Max flow = Min cut (famous theorem!)\n";
  cout << "\n4. KEY ALGORITHMS:\n";
  cout << "   Ford-Fulkerson:   O(E * max_flow), simple\n";
  cout << "   Edmonds-Karp:     O(V * E^2), uses BFS\n";
  cout << "   Dinic:            O(V^2 * E), level graphs\n";
  cout << "   Push-Relabel:     O(V^2 * sqrt(E)), fastest\n";
  cout << "\n5. RESIDUAL GRAPH:\n";
  cout << "   * Forward arcs: Remaining capacity\n";
  cout << "   * Backward arcs: Can reduce flow\n";
  cout << "   * Essential for finding optimal solution\n";
  cout << "\n6. APPLICATIONS:\n";
  cout << "   ✓ Network design and optimization\n";
  cout << "   ✓ Resource allocation\n";
  cout << "   ✓ Scheduling problems\n";
  cout << "   ✓ Image processing\n";
  cout << "   ✓ Bipartite matching\n";
  cout << "\n7. DESIGN PRINCIPLES:\n";
  cout << "   * Add capacity where needed most\n";
  cout << "   * Identify bottlenecks with min-cut\n";
  cout << "   * Redundancy improves robustness\n";
  cout << "   * Balance capacities across network\n";
  
  return 0;
}
