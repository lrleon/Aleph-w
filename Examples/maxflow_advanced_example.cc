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
 * @file maxflow_advanced_example.cc
 * @brief Advanced Maximum Flow Algorithms Comparison
 * 
 * This example demonstrates and compares different maximum flow algorithms
 * available in Aleph-w:
 * 
 * ## Algorithms Covered
 * 
 * 1. **Edmonds-Karp** (Ford-Fulkerson with BFS): O(VE²)
 *    - Simple to understand and implement
 *    - Good for sparse graphs
 * 
 * 2. **Dinic's Algorithm**: O(V²E)
 *    - Uses level graphs and blocking flows
 *    - Better for dense graphs
 * 
 * 3. **Capacity Scaling**: O(VE log U)
 *    - Works with large capacities efficiently
 *    - U = maximum capacity
 * 
 * 4. **HLPP (Highest Label Preflow-Push)**: O(V²√E)
 *    - Push-relabel variant
 *    - Best theoretical complexity for dense graphs
 * 
 * ## Applications
 * 
 * - Network bandwidth optimization
 * - Supply chain logistics
 * - Image segmentation (via min-cut)
 * - Bipartite matching
 * - Baseball elimination
 * 
 * ## When to Use Each Algorithm
 * 
 * - **Small graphs**: Any algorithm works
 * - **Sparse graphs**: Edmonds-Karp or Dinic
 * - **Dense graphs**: HLPP or Dinic
 * - **Large capacities**: Capacity Scaling
 * 
 * @see tpl_maxflow.H for algorithm implementations
 * @see tpl_net.H for network structures
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <tpl_net.H>
#include <tpl_maxflow.H>

using namespace std;
using namespace Aleph;

// Type definitions
using FlowType = double;
using Net = Net_Graph<Net_Node<string>, Net_Arc<Empty_Class, FlowType>>;
using Node = Net::Node;

/**
 * @brief Build a supply chain network
 * 
 *        [Factory1]----10---->[Warehouse1]----8----+
 *          |                       |               |
 *          12                      5               |
 *          |                       v               v
 *       [Source]                [Hub]---------->[Sink]
 *          |                       ^               ^
 *          15                      6               |
 *          |                       |               |
 *        [Factory2]----12---->[Warehouse2]----9---+
 */
Net build_supply_chain()
{
  Net net;
  
  auto source = net.insert_node("Source");
  auto f1 = net.insert_node("Factory1");
  auto f2 = net.insert_node("Factory2");
  auto w1 = net.insert_node("Warehouse1");
  auto w2 = net.insert_node("Warehouse2");
  auto hub = net.insert_node("Hub");
  auto sink = net.insert_node("Sink");
  
  // From source to factories
  net.insert_arc(source, f1, 12);
  net.insert_arc(source, f2, 15);
  
  // Factory to warehouses
  net.insert_arc(f1, w1, 10);
  net.insert_arc(f2, w2, 12);
  
  // Warehouses to hub and sink
  net.insert_arc(w1, hub, 5);
  net.insert_arc(w2, hub, 6);
  net.insert_arc(w1, sink, 8);
  net.insert_arc(w2, sink, 9);
  
  // Hub to sink
  net.insert_arc(hub, sink, 15);
  
  return net;
}

/**
 * @brief Build a grid network for stress testing
 * 
 * Creates a rows x cols grid with diagonal connections.
 * Source is top-left corner, sink is bottom-right.
 */
Net build_grid_network(int rows, int cols, FlowType base_cap = 10.0)
{
  Net net;
  
  vector<vector<Node*>> nodes(rows, vector<Node*>(cols));
  
  // Create nodes
  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      nodes[i][j] = net.insert_node("N" + to_string(i) + "_" + to_string(j));
  
  // Create edges (right and down) with varying capacities
  for (int i = 0; i < rows; ++i)
  {
    for (int j = 0; j < cols; ++j)
    {
      // Right edge
      if (j + 1 < cols)
        net.insert_arc(nodes[i][j], nodes[i][j+1], base_cap + (i % 3));
      
      // Down edge
      if (i + 1 < rows)
        net.insert_arc(nodes[i][j], nodes[i+1][j], base_cap + (j % 3));
    }
  }
  
  return net;
}

/**
 * @brief Time a max-flow algorithm execution
 */
template <typename Algorithm>
pair<FlowType, double> time_algorithm(Net net)
{
  // Reset flows
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
    it.get_curr()->flow = 0;
  
  auto start = chrono::high_resolution_clock::now();
  FlowType flow = Algorithm()(net);
  auto end = chrono::high_resolution_clock::now();
  
  double ms = chrono::duration<double, milli>(end - start).count();
  
  return {flow, ms};
}

/**
 * @brief Print network flow statistics
 */
void print_flow_stats(Net& net, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  
  FlowType total_cap = 0, total_flow = 0;
  int saturated = 0, zero_flow = 0;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    total_cap += arc->cap;
    total_flow += arc->flow;
    if (arc->flow == arc->cap && arc->cap > 0) saturated++;
    if (arc->flow == 0) zero_flow++;
  }
  
  cout << "Total capacity:    " << total_cap << endl;
  cout << "Total flow:        " << total_flow << endl;
  cout << "Saturated arcs:    " << saturated << endl;
  cout << "Zero-flow arcs:    " << zero_flow << endl;
  cout << "Utilization:       " << fixed << setprecision(1) 
       << (100.0 * total_flow / total_cap) << "%" << endl;
}

/**
 * @brief Demonstrate flow decomposition into paths
 */
void demonstrate_flow_decomposition(Net& net)
{
  cout << "\n=== Flow Decomposition ===" << endl;
  cout << "Breaking down max-flow into individual paths:\n" << endl;
  
  auto decomp = decompose_flow(net);
  
  int path_num = 1;
  int total_paths = 0;
  
  for (auto it = decomp.paths.get_it(); it.has_curr(); it.next())
  {
    const auto& fp = it.get_curr();
    cout << "Path " << path_num++ << " (flow = " << fp.flow << "): ";
    cout << net.get_source()->get_info();
    
    for (auto ait = fp.arcs.get_it(); ait.has_curr(); ait.next())
    {
      auto* arc = ait.get_curr();
      auto* tgt = net.get_tgt_node(arc);
      cout << " -> " << tgt->get_info();
    }
    cout << endl;
    ++total_paths;
  }
  
  cout << "\nTotal paths: " << total_paths << endl;
  cout << "Total flow: " << decomp.total_flow() << endl;
}

/**
 * @brief Compare all algorithms on the same network
 */
void compare_algorithms(int grid_size)
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Algorithm Comparison on " << grid_size << "x" << grid_size 
       << " Grid Network" << endl;
  cout << string(60, '=') << endl;
  
  Net net = build_grid_network(grid_size, grid_size);
  
  cout << "\nNetwork: " << net.get_num_nodes() << " nodes, " 
       << net.get_num_arcs() << " arcs\n" << endl;
  
  cout << setw(20) << left << "Algorithm" 
       << setw(12) << right << "Flow"
       << setw(15) << "Time (ms)" << endl;
  cout << string(47, '-') << endl;
  
  // Edmonds-Karp
  {
    auto [flow, time] = time_algorithm<Ford_Fulkerson_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "Edmonds-Karp"
         << setw(12) << right << flow
         << setw(15) << fixed << setprecision(3) << time << endl;
  }
  
  // Dinic
  {
    auto [flow, time] = time_algorithm<Dinic_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "Dinic"
         << setw(12) << right << flow
         << setw(15) << fixed << setprecision(3) << time << endl;
  }
  
  // Capacity Scaling
  {
    auto [flow, time] = time_algorithm<Capacity_Scaling_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "Capacity Scaling"
         << setw(12) << right << flow
         << setw(15) << fixed << setprecision(3) << time << endl;
  }
  
  // HLPP
  {
    auto [flow, time] = time_algorithm<HLPP_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "HLPP"
         << setw(12) << right << flow
         << setw(15) << fixed << setprecision(3) << time << endl;
  }
}

/**
 * @brief Demonstrate min-cut duality
 */
void demonstrate_min_cut(Net& net)
{
  cout << "\n=== Min-Cut / Max-Flow Duality ===" << endl;
  cout << "\nThe Max-Flow Min-Cut Theorem states:" << endl;
  cout << "  max_flow = min_cut_capacity" << endl;
  
  // Find saturated edges (potential min-cut edges)
  cout << "\nSaturated edges (candidates for min-cut):" << endl;
  
  FlowType cut_capacity = 0;
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    if (arc->flow == arc->cap && arc->cap > 0)
    {
      auto* src = net.get_src_node(arc);
      auto* tgt = net.get_tgt_node(arc);
      cout << "  " << src->get_info() << " -> " << tgt->get_info()
           << " (cap = " << arc->cap << ")" << endl;
      cut_capacity += arc->cap;
    }
  }
  
  cout << "\nNote: Not all saturated edges are necessarily in the min-cut," << endl;
  cout << "but the min-cut consists only of saturated edges." << endl;
}

int main()
{
  cout << "=== Advanced Maximum Flow Algorithms ===" << endl;
  cout << "Comparing Edmonds-Karp, Dinic, Capacity Scaling, and HLPP\n" << endl;
  
  // Demo 1: Supply chain example
  cout << string(60, '=') << endl;
  cout << "Demo 1: Supply Chain Network" << endl;
  cout << string(60, '=') << endl;
  
  Net supply = build_supply_chain();
  
  cout << "\nNetwork structure:" << endl;
  cout << "  Nodes: " << supply.get_num_nodes() << endl;
  cout << "  Arcs:  " << supply.get_num_arcs() << endl;
  
  cout << "\nComputing max-flow with Dinic's algorithm..." << endl;
  FlowType max_flow = dinic_maximum_flow(supply);
  
  cout << "\n*** Maximum Flow: " << max_flow << " units ***" << endl;
  
  print_flow_stats(supply, "Flow Statistics");
  demonstrate_flow_decomposition(supply);
  demonstrate_min_cut(supply);
  
  // Demo 2: Algorithm comparison
  compare_algorithms(5);
  compare_algorithms(10);
  compare_algorithms(15);
  
  // Demo 3: Large capacity handling
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 3: Large Capacities (Capacity Scaling Advantage)" << endl;
  cout << string(60, '=') << endl;
  
  Net large_cap = build_grid_network(8, 8, 1000000.0);
  
  cout << "\nNetwork with capacities around 1,000,000:" << endl;
  
  cout << setw(20) << left << "Algorithm" 
       << setw(15) << right << "Flow"
       << setw(15) << "Time (ms)" << endl;
  cout << string(50, '-') << endl;
  
  {
    auto [flow, time] = time_algorithm<Ford_Fulkerson_Maximum_Flow<Net>>(large_cap);
    cout << setw(20) << left << "Edmonds-Karp"
         << setw(15) << right << fixed << setprecision(0) << flow
         << setw(15) << setprecision(3) << time << endl;
  }
  
  {
    auto [flow, time] = time_algorithm<Capacity_Scaling_Maximum_Flow<Net>>(large_cap);
    cout << setw(20) << left << "Capacity Scaling"
         << setw(15) << right << fixed << setprecision(0) << flow
         << setw(15) << setprecision(3) << time << endl;
  }
  
  cout << "\nCapacity Scaling excels with large integer capacities!" << endl;
  
  // Summary
  cout << "\n" << string(60, '=') << endl;
  cout << "Summary" << endl;
  cout << string(60, '=') << endl;
  
  cout << R"(
Algorithm Selection Guide:
  
  1. Edmonds-Karp: O(VE²)
     - Simple, good for small/sparse graphs
     - Polynomial in graph size
  
  2. Dinic: O(V²E)
     - Excellent all-around choice
     - Works well on most networks
  
  3. Capacity Scaling: O(VE log U)
     - Best for large integer capacities
     - Scales logarithmically with max capacity
  
  4. HLPP: O(V²√E)
     - Push-relabel method
     - Often fastest in practice for dense graphs

Recommendation:
  - Start with Dinic (good balance)
  - Use Capacity Scaling for very large capacities
  - Use HLPP for dense graphs if Dinic is slow
)" << endl;
  
  return 0;
}
