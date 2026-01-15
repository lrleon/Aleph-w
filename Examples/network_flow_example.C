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
 * @file network_flow_example.C
 * @brief Maximum Flow in Network Graphs (Ford-Fulkerson/Edmonds-Karp)
 * 
 * This example demonstrates the maximum flow problem, one of the most
 * important problems in combinatorial optimization. It shows how to find
 * the maximum amount of flow that can be sent from a source to a sink
 * through a capacitated network.
 *
 * ## The Maximum Flow Problem
 *
 * Given a directed graph (network) where:
 * - Each edge has a **capacity** (maximum flow allowed)
 * - There is a **source** node (s) that produces flow
 * - There is a **sink** node (t) that consumes flow
 *
 * **Goal**: Find the maximum flow from source to sink.
 *
### Constraints
 *
 * 1. **Capacity constraint**: Flow on edge ≤ capacity
 * 2. **Flow conservation**: For each node (except s, t):
 *    - Inflow = Outflow
 *    - (Flow entering = Flow leaving)
 *
### Example
 *
 * ```
 *     [10]      [5]
 *  s -----> A -----> t
 *     [5]       [8]
 *  s -----> B -----> t
 * ```
 *
 * Maximum flow = 13 (10+3 from s→A→t, 5 from s→B→t)
 *
## Algorithm: Ford-Fulkerson / Edmonds-Karp
 *
### Ford-Fulkerson Method
 *
 * The general approach:
 * 1. Start with zero flow
 * 2. While there exists an **augmenting path** s → t in residual graph:
 *    a. Find augmenting path (any path with available capacity)
 *    b. Find **bottleneck capacity** (minimum capacity along path)
 *    c. **Augment** flow along path by bottleneck amount
 * 3. Maximum flow = sum of flows leaving source
 *
### Edmonds-Karp Variant
 *
 * Uses **BFS** (breadth-first search) to find augmenting paths:
 * - Always finds **shortest** augmenting path
 * - Guarantees polynomial time: O(V × E²)
 * - More efficient than DFS-based Ford-Fulkerson
 *
### Residual Graph
 *
 * The residual graph represents remaining capacity:
 * - **Forward edge**: Remaining capacity = capacity - flow
 * - **Backward edge**: Can "undo" flow = current flow
 * - Allows algorithm to find better paths by reversing flow
 *
## Complexity
 *
 * | Algorithm | Time Complexity | Notes |
 * |-----------|----------------|------|
 * | Ford-Fulkerson (DFS) | O(E × max_flow) | Can be exponential! |
 * | Edmonds-Karp (BFS) | O(V × E²) | Polynomial, this example |
 * | Dinic's | O(V² × E) | Better for dense graphs |
 * | Push-Relabel | O(V² × E) | Alternative approach |
 *
 * **Note**: Ford-Fulkerson with DFS can be exponential if capacities
 * are large integers. Edmonds-Karp guarantees polynomial time.
 *
## Applications
 *
### Transportation Networks
 * - **Highway systems**: Maximum vehicles through road network
 * - **Railway networks**: Maximum trains through tracks
 * - **Shipping routes**: Maximum cargo through shipping lanes
 *
### Communication Networks
 * - **Network bandwidth**: Maximum data through network
 * - **Internet routing**: Maximum packets through routers
 * - **Telecommunications**: Maximum calls through switches
 *
### Matching Problems
 * - **Bipartite matching**: Match workers to jobs
 * - **Dating apps**: Match users based on preferences
 * - **Resource allocation**: Assign resources to tasks
 *
### Game Theory
 * - **Baseball elimination**: Which teams can still win?
 * - **Tournament scheduling**: Determine possible outcomes
 *
### Computer Vision
 * - **Image segmentation**: Separate foreground/background
 * - **Min-cut**: Find minimum cut (dual of max-flow)
 *
## Max-Flow Min-Cut Theorem
 *
 * **Fundamental result**: Maximum flow = Minimum cut capacity
 *
 * A **cut** is a partition of nodes into two sets (S containing s, T containing t).
 * Cut capacity = sum of capacities of edges from S to T.
 *
 * This theorem is the foundation of many max-flow applications.
 *
## Network Graph Structure in Aleph-w
 *
### Net_Graph Features
 *
 * In Aleph-w's `Net_Graph`:
 * - **Automatic source detection**: Nodes with no incoming arcs are sources
 * - **Automatic sink detection**: Nodes with no outgoing arcs are sinks
 * - **Single source/sink**: Ford-Fulkerson requires exactly one of each
 * - **Capacity storage**: Each arc stores its capacity
 *
### Building a Network
 *
 * ```cpp
 * Net_Graph g;
 * auto s = g.insert_node("Source");
 * auto t = g.insert_node("Sink");
 * auto a = g.insert_node("A");
 *
 * g.insert_arc(s, a, 10);  // Capacity 10
 * g.insert_arc(a, t, 5);   // Capacity 5
 * ```
 *
## Usage
 *
 * ```bash
 * # Run maximum flow example
 * ./network_flow_example
 * ```
 *
 * @see tpl_net.H Network graph structures
 * @see maxflow_advanced_example.cc Advanced max-flow algorithms (Dinic, HLPP)
 * @see mincost_flow_example.cc Minimum cost flow (extension)
 * @see net_apps_example.cc Real-world applications
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <tpl_net.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Node info type
using NodeInfo = string;

// Arc info type (empty - capacity/flow handled by Net_Arc)
using ArcInfo = Empty_Class;

// Flow type
using FlowType = int;

// Network type
using FlowNet = Net_Graph<Net_Node<NodeInfo>, Net_Arc<ArcInfo, FlowType>>;

/**
 * @brief Build a sample flow network
 * 
 * Water distribution network:
 * 
 *                    [B]
 *                   / | \
 *                 10  4   8
 *                /    |    \
 *     [Source]--     [D]    --[Sink]
 *               \    /|\   /
 *                8  5 2  6
 *                 \ | | /
 *                   [C]
 * 
 * The topology ensures Source has no incoming arcs and Sink has no outgoing arcs.
 */
FlowNet build_water_network()
{
  FlowNet net;
  
  // Create nodes
  auto source = net.insert_node("Source");
  auto b = net.insert_node("PumpB");
  auto c = net.insert_node("PumpC");
  auto d = net.insert_node("PumpD");
  auto sink = net.insert_node("Sink");
  
  // Add edges with capacities (in liters/second)
  // Note: In Net_Graph, source/sink status is automatic based on topology
  // - Source will have no incoming arcs
  // - Sink will have no outgoing arcs
  
  // From source
  net.insert_arc(source, b, 10);
  net.insert_arc(source, c, 8);
  
  // Internal
  net.insert_arc(b, d, 4);
  net.insert_arc(c, d, 5);
  
  // To sink
  net.insert_arc(b, sink, 8);
  net.insert_arc(c, sink, 2);
  net.insert_arc(d, sink, 6);
  
  return net;
}

/**
 * @brief Build a more complex network
 */
FlowNet build_datacenter_network()
{
  FlowNet net;
  
  auto source = net.insert_node("Source");
  auto r1 = net.insert_node("Router1");
  auto r2 = net.insert_node("Router2");
  auto r3 = net.insert_node("Router3");
  auto r4 = net.insert_node("Router4");
  auto s1 = net.insert_node("Switch1");
  auto s2 = net.insert_node("Switch2");
  auto sink = net.insert_node("Sink");
  
  // Source connections
  net.insert_arc(source, r1, 15);
  net.insert_arc(source, s1, 20);
  
  // Internal connections
  net.insert_arc(r1, r2, 5);
  net.insert_arc(r1, r3, 10);
  net.insert_arc(r2, s1, 7);
  net.insert_arc(r3, r4, 8);
  net.insert_arc(s1, s2, 6);
  net.insert_arc(s2, r4, 9);
  
  // Sink connections
  net.insert_arc(r3, sink, 12);
  net.insert_arc(r4, sink, 15);
  
  return net;
}

/**
 * @brief Print network structure and flow
 */
void print_network(FlowNet& net, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Nodes: " << net.get_num_nodes() << endl;
  cout << "Arcs:  " << net.get_num_arcs() << endl;
  
  // Show source and sink
  cout << "Source: " << net.get_source()->get_info() << endl;
  cout << "Sink:   " << net.get_sink()->get_info() << endl;
  
  cout << "\nEdge flows (flow/capacity):" << endl;
  
  FlowType total_out = 0;
  FlowType total_in = 0;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = net.get_src_node(arc);
    auto* tgt = net.get_tgt_node(arc);
    
    cout << "  " << setw(10) << left << src->get_info()
         << " ---> " << setw(10) << left << tgt->get_info()
         << right << " : " << setw(3) << arc->flow 
         << " / " << setw(3) << arc->cap;
    
    // Show utilization
    if (arc->cap > 0)
    {
      double util = 100.0 * arc->flow / arc->cap;
      cout << "  (" << fixed << setprecision(0) << util << "%)";
      if (arc->flow == arc->cap)
        cout << " [SATURATED]";
    }
    cout << endl;
    
    if (net.is_source(src))
      total_out += arc->flow;
    if (net.is_sink(tgt))
      total_in += arc->flow;
  }
  
  cout << "\nFlow out of source: " << total_out << endl;
  cout << "Flow into sink:     " << total_in << endl;
}

/**
 * @brief Demonstrate min-cut (dual of max-flow)
 */
void demonstrate_min_cut(FlowNet& net)
{
  cout << "\n=== Min-Cut (Dual of Max-Flow) ===" << endl;
  cout << "\nBy the Max-Flow Min-Cut Theorem:" << endl;
  cout << "  Maximum flow value = Minimum cut capacity" << endl;
  
  cout << "\nSaturated edges (part of min-cut):" << endl;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    if (arc->flow == arc->cap && arc->cap > 0)
    {
      auto* src = net.get_src_node(arc);
      auto* tgt = net.get_tgt_node(arc);
      cout << "  " << src->get_info() << " -> " 
           << tgt->get_info() << " (capacity " << arc->cap << ")" << endl;
    }
  }
  
  cout << "\nNote: The min-cut separates source from sink." << endl;
  cout << "Cutting these edges disconnects source from sink." << endl;
}

/**
 * @brief Demonstrate bipartite matching as max-flow
 */
void demonstrate_bipartite_matching()
{
  cout << "\n=== Bipartite Matching via Max-Flow ===" << endl;
  
  cout << "\nProblem: Assign workers to jobs (each worker can do some jobs)" << endl;
  
  // Build bipartite matching network
  FlowNet net;
  
  auto source = net.insert_node("Source");
  
  // Workers
  auto alice = net.insert_node("Alice");
  auto bob = net.insert_node("Bob");
  auto carol = net.insert_node("Carol");
  
  // Jobs
  auto coding = net.insert_node("Coding");
  auto design = net.insert_node("Design");
  auto testing = net.insert_node("Testing");
  
  auto sink = net.insert_node("Sink");
  
  // Each worker can be assigned to at most 1 job (capacity 1 from source)
  net.insert_arc(source, alice, 1);
  net.insert_arc(source, bob, 1);
  net.insert_arc(source, carol, 1);
  
  // Worker-job compatibility (edges with capacity 1)
  net.insert_arc(alice, coding, 1);   // Alice can code
  net.insert_arc(alice, design, 1);   // Alice can design
  net.insert_arc(bob, coding, 1);     // Bob can code
  net.insert_arc(bob, testing, 1);    // Bob can test
  net.insert_arc(carol, design, 1);   // Carol can design
  net.insert_arc(carol, testing, 1);  // Carol can test
  
  // Each job needs at most 1 worker (capacity 1 to sink)
  net.insert_arc(coding, sink, 1);
  net.insert_arc(design, sink, 1);
  net.insert_arc(testing, sink, 1);
  
  cout << "\nWorker skills:" << endl;
  cout << "  Alice: Coding, Design" << endl;
  cout << "  Bob:   Coding, Testing" << endl;
  cout << "  Carol: Design, Testing" << endl;
  
  // Compute max flow = max matching using Ford-Fulkerson
  FlowType max_matching = ford_fulkerson_maximum_flow(net);
  
  cout << "\nMaximum matching size: " << max_matching << endl;
  cout << "\nOptimal assignment:" << endl;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = net.get_src_node(arc);
    auto* tgt = net.get_tgt_node(arc);
    
    // Skip source and sink edges
    if (net.is_source(src) || net.is_sink(tgt))
      continue;
    
    if (arc->flow == 1)
    {
      cout << "  " << src->get_info() << " -> " 
           << tgt->get_info() << endl;
    }
  }
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Network Flow Example", ' ', "1.0");
    
    TCLAP::SwitchArg simpleArg("s", "simple",
      "Use simple water network", false);
    TCLAP::SwitchArg complexArg("c", "complex",
      "Use complex datacenter network", false);
    TCLAP::SwitchArg matchArg("m", "matching",
      "Demonstrate bipartite matching", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Run all demos", false);
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    
    cmd.add(simpleArg);
    cmd.add(complexArg);
    cmd.add(matchArg);
    cmd.add(allArg);
    cmd.add(verboseArg);
    
    cmd.parse(argc, argv);
    
    bool runSimple = simpleArg.getValue();
    bool runComplex = complexArg.getValue();
    bool runMatch = matchArg.getValue();
    bool runAll = allArg.getValue();
    (void)verboseArg.getValue();  // Unused but available
    
    // Default: run simple demo
    if (!runSimple && !runComplex && !runMatch)
      runAll = true;
    
    cout << "=== Maximum Flow Problem ===" << endl;
    cout << "Algorithm: Ford-Fulkerson / Edmonds-Karp (BFS)" << endl;
    
    if (runAll || runSimple)
    {
      cout << "\n" << string(50, '=') << endl;
      cout << "Water Distribution Network" << endl;
      cout << string(50, '=') << endl;
      
      FlowNet water = build_water_network();
      print_network(water, "Initial Network (zero flow)");
      
      cout << "\n--- Computing Maximum Flow ---" << endl;
      FlowType max_flow = ford_fulkerson_maximum_flow(water);
      
      print_network(water, "After Max-Flow Computation");
      cout << "\n*** MAXIMUM FLOW: " << max_flow << " units ***" << endl;
      
      demonstrate_min_cut(water);
    }
    
    if (runAll || runComplex)
    {
      cout << "\n" << string(50, '=') << endl;
      cout << "Data Center Network" << endl;
      cout << string(50, '=') << endl;
      
      FlowNet dc = build_datacenter_network();
      print_network(dc, "Initial Network (zero flow)");
      
      cout << "\n--- Computing Maximum Flow ---" << endl;
      FlowType max_flow = ford_fulkerson_maximum_flow(dc);
      
      print_network(dc, "After Max-Flow Computation");
      cout << "\n*** MAXIMUM FLOW: " << max_flow << " units ***" << endl;
    }
    
    if (runAll || runMatch)
    {
      cout << "\n" << string(50, '=') << endl;
      demonstrate_bipartite_matching();
    }
    
    cout << "\n=== Algorithm Summary ===" << endl;
    cout << "Ford-Fulkerson: O(E * max_flow)" << endl;
    cout << "Edmonds-Karp:   O(V * E²) - uses BFS" << endl;
    cout << "Dinic:          O(V² * E) - uses level graphs" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}
