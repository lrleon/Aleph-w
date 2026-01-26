
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file maxflow_advanced_example.cc
 * @brief Advanced Maximum Flow Algorithms Comparison
 * 
 * This example demonstrates and compares different maximum flow algorithms
 * available in Aleph-w. While basic Ford-Fulkerson/Edmonds-Karp works well
 * for many cases, advanced algorithms offer better performance for specific
 * graph types and scenarios.
 *
 * ## Why Multiple Algorithms?
 *
 * Different max-flow algorithms excel in different scenarios:
 * - **Graph density**: Sparse vs dense graphs
 * - **Capacity size**: Small vs large capacities
 * - **Graph structure**: Special properties
 * - **Performance requirements**: Speed vs simplicity
 *
 * ## Algorithms Covered
 *
 * ### 1. Edmonds-Karp (Ford-Fulkerson with BFS)
 *
 * **Strategy**: Use BFS to find shortest augmenting paths
 *
 * **Complexity**: O(V × E²)
 * - Each BFS: O(E)
 * - At most O(VE) augmentations
 *
 * **Pros**:
 * - Simple to understand and implement
 * - Predictable performance
 * - Good for sparse graphs
 *
 * **Cons**:
 * - Slower for dense graphs
 * - May do many augmentations
 *
 * ### 2. Dinic's Algorithm
 *
 * **Strategy**: Use level graphs and blocking flows
 *
 * **How it works**:
 * 1. Build level graph (BFS layers)
 * 2. Find blocking flow (saturate all paths in level graph)
 * 3. Repeat until no augmenting path
 *
 * **Complexity**: O(V² × E)
 * - O(V) blocking flow computations
 * - Each blocking flow: O(VE)
 *
 * **Pros**:
 * - Faster than Edmonds-Karp
 * - Good for both sparse and dense graphs
 * - Practical performance often better than worst case
 *
 * **Cons**:
 * - More complex implementation
 *
 * ### 3. Capacity Scaling
 *
 * **Strategy**: Process edges in rounds by capacity threshold
 *
 * **How it works**:
 * 1. Start with large capacity threshold Δ
 * 2. Only consider edges with capacity ≥ Δ
 * 3. Find augmenting paths in this subgraph
 * 4. Reduce Δ and repeat
 *
 * **Complexity**: O(V × E × log U)
 * - U = maximum capacity
 * - log U rounds
 * - O(VE) work per round
 *
 * **Pros**:
 * - Efficient for large capacities
 * - Good when capacities vary widely
 *
 * **Cons**:
 * - Overhead for small capacities
 *
 * ### 4. HLPP (Highest Label Preflow-Push)
 *
 * **Strategy**: Push-relabel with highest label selection
 *
 * **How it works**:
 * 1. Push flow from active vertices
 * 2. Relabel vertices when stuck
 * 3. Always process highest label vertex
 *
 * **Complexity**: O(V² × √E)
 * - Best theoretical for dense graphs
 *
 * **Pros**:
 * - Best complexity for dense graphs
 * - Efficient in practice
 *
 * **Cons**:
 * - Most complex implementation
 * - May be slower for sparse graphs
 *
 * ## Complexity Comparison
 *
 * | Algorithm | Time Complexity | Best For |
 * |-----------|-----------------|----------|
 * | Edmonds-Karp | O(V × E²) | Sparse graphs, simplicity |
 * | Dinic | O(V² × E) | General purpose |
 * | Capacity Scaling | O(V × E × log U) | Large capacities |
 * | HLPP | O(V² × √E) | Dense graphs |
 *
 * **Note**: Actual performance depends heavily on graph structure!
 *
 * ## When to Use Each Algorithm
 *
 * ### Small Graphs (< 100 vertices)
 * - **Any algorithm works**: Performance difference negligible
 * - **Recommendation**: Edmonds-Karp (simplest)
 *
 * ### Sparse Graphs (E ≈ V)
 * - **Edmonds-Karp**: Simple, O(V³) effective
 * - **Dinic**: Better worst-case, often faster
 * - **Recommendation**: Dinic (best balance)
 *
 * ### Dense Graphs (E ≈ V²)
 * - **Dinic**: O(V⁴) but practical
 * - **HLPP**: O(V² × √E) = O(V³) theoretical best
 * - **Recommendation**: HLPP for large graphs, Dinic for medium
 *
 * ### Large Capacities (U >> V)
 * - **Capacity Scaling**: O(V × E × log U) efficient
 * - **Others**: May be slower
 * - **Recommendation**: Capacity Scaling
 *
 * ### General Purpose
 * - **Dinic**: Good balance of speed and simplicity
 * - **Recommendation**: Default choice
 *
 * ## Performance Characteristics
 *
 * ### Sparse Graph (E = O(V))
 *
 * | Algorithm | Complexity | Relative Speed |
 * |-----------|-----------|----------------|
 * | Edmonds-Karp | O(V³) | 1× |
 * | Dinic | O(V³) | 2-5× faster |
 * | HLPP | O(V².5) | 3-10× faster |
 *
 * ### Dense Graph (E = O(V²))
 *
 * | Algorithm | Complexity | Relative Speed |
 * |-----------|-----------|----------------|
 * | Edmonds-Karp | O(V⁵) | 1× |
 * | Dinic | O(V⁴) | 10-100× faster |
 * | HLPP | O(V³) | 100-1000× faster |
 *
 * ## Applications
 *
 * ### Network Bandwidth Optimization
 * - **Internet routing**: Maximize data flow
 * - **Content delivery**: Distribute content efficiently
 *
 * ### Supply Chain Logistics
 * - **Transportation**: Maximize goods flow
 * - **Resource allocation**: Optimize resource usage
 *
 * ### Image Segmentation
 * - **Min-cut**: Find optimal segmentation
 * - **Computer vision**: Separate foreground/background
 *
 * ### Matching Problems
 * - **Bipartite matching**: Reduce to max-flow
 * - **Job assignment**: Match workers to tasks
 *
 * ### Game Theory
 * - **Baseball elimination**: Determine if team can win
 * - **Tournament analysis**: Analyze possible outcomes
 *
 * ## Usage
 *
 * ```bash
 * # Run all demos (supply chain + algorithm comparisons + large capacity demo)
 * ./maxflow_advanced_example
 *
 * # Choose the algorithm used for the supply chain demo
 * ./maxflow_advanced_example --algorithm dinic
 * ./maxflow_advanced_example --algorithm hlpp
 *
 * # Run the benchmark comparison on a grid network
 * ./maxflow_advanced_example --sparse
 * ./maxflow_advanced_example --dense
 * ```
 *
 * @see tpl_maxflow.H Advanced max-flow algorithm implementations
 * @see network_flow_example.C Basic max-flow example (Ford-Fulkerson)
 * @see tpl_net.H Network graph structures
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <tpl_net.H>
#include <tpl_maxflow.H>

using namespace std;
using namespace Aleph;

static void usage(const char* prog)
{
  cout << "Usage: " << prog
       << " [--algorithm <edmonds-karp|dinic|capacity-scaling|hlpp>]"
       << " [--sparse] [--dense] [--help]\n";
  cout << "\nIf no flags are given, all demos are executed.\n";
  cout << "If any flags are given, the program always runs the supply chain demo\n";
  cout << "(using --algorithm if provided) and the large capacities demo.\n";
  cout << "The grid benchmark comparison is run when --sparse or --dense is set.\n";
}

static bool has_flag(int argc, char* argv[], const char* flag)
{
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], flag) == 0)
      return true;
  return false;
}

static const char* get_opt_value(int argc, char* argv[], const char* opt)
{
  for (int i = 1; i + 1 < argc; ++i)
    if (std::strcmp(argv[i], opt) == 0)
      return argv[i + 1];
  return nullptr;
}

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
 * Creates a rows x cols grid with right and down connections.
 * Source is automatically detected as the top-left corner, and sink as the
 * bottom-right corner.
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
  
  FlowType total_cap = 0;
  int saturated = 0, zero_flow = 0;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    total_cap += arc->cap;
    if (arc->flow == arc->cap && arc->cap > 0) saturated++;
    if (arc->flow == 0) zero_flow++;
  }

  const FlowType max_flow = net.flow_value();
  
  cout << "Total capacity:    " << total_cap << endl;
  cout << "Max flow value:    " << max_flow << endl;
  cout << "Saturated arcs:    " << saturated << endl;
  cout << "Zero-flow arcs:    " << zero_flow << endl;
  cout << "Utilization:       " << fixed << setprecision(1)
       << ((total_cap > 0) ? (100.0 * max_flow / total_cap) : 0.0) << "%" << endl;
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

  // Edmonds-Karp (Ford-Fulkerson with BFS)
  {
    auto [flow, time] = time_algorithm<Edmonds_Karp_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "Edmonds-Karp"
         << setw(12) << right << flow
         << setw(15) << fixed << setprecision(3) << time << endl;
  }
  
  // Ford-Fulkerson (augmenting paths with DFS)
  {
    auto [flow, time] = time_algorithm<Ford_Fulkerson_Maximum_Flow<Net>>(net);
    cout << setw(20) << left << "Ford-Fulkerson"
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

int main(int argc, char* argv[])
{
  cout << "=== Advanced Maximum Flow Algorithms ===" << endl;
  cout << "Comparing Edmonds-Karp, Ford-Fulkerson, Dinic, Capacity Scaling, and HLPP\n" << endl;

  if (has_flag(argc, argv, "--help"))
    {
      usage(argv[0]);
      return 0;
    }

  const char* algo = get_opt_value(argc, argv, "--algorithm");
  const bool sparse = has_flag(argc, argv, "--sparse");
  const bool dense = has_flag(argc, argv, "--dense");

  const bool has_cli = (argc > 1);
  const bool default_run_all = !has_cli;

  
  // Demo 1: Supply chain example
  cout << string(60, '=') << endl;
  cout << "Demo 1: Supply Chain Network" << endl;
  cout << string(60, '=') << endl;
  
  Net supply = build_supply_chain();
  
  cout << "\nNetwork structure:" << endl;
  cout << "  Nodes: " << supply.get_num_nodes() << endl;
  cout << "  Arcs:  " << supply.get_num_arcs() << endl;
  
  FlowType max_flow = 0;
  const char* chosen = algo ? algo : "dinic";
  if (std::strcmp(chosen, "dinic") == 0)
    {
      cout << "\nComputing max-flow with Dinic's algorithm..." << endl;
      max_flow = dinic_maximum_flow(supply);
    }
  else if (std::strcmp(chosen, "edmonds-karp") == 0)
    {
      cout << "\nComputing max-flow with Edmonds-Karp..." << endl;
      max_flow = edmonds_karp_maximum_flow(supply);
    }
  else if (std::strcmp(chosen, "capacity-scaling") == 0)
    {
      cout << "\nComputing max-flow with Capacity Scaling..." << endl;
      max_flow = capacity_scaling_maximum_flow(supply);
    }
  else if (std::strcmp(chosen, "hlpp") == 0)
    {
      cout << "\nComputing max-flow with HLPP..." << endl;
      max_flow = hlpp_maximum_flow(supply);
    }
  else
    {
      cout << "Unknown --algorithm value: " << chosen << "\n";
      usage(argv[0]);
      return 1;
    }
  
  cout << "\n*** Maximum Flow: " << max_flow << " units ***" << endl;
  
  print_flow_stats(supply, "Flow Statistics");
  demonstrate_flow_decomposition(supply);
  demonstrate_min_cut(supply);
  
  if (default_run_all)
    {
      compare_algorithms(5);
      compare_algorithms(10);
      compare_algorithms(15);
    }
  else if (sparse || dense)
    {
      const int grid = dense ? 20 : 8;
      cout << "\nBenchmarking on " << grid << "x" << grid
           << " grid network\n";
      compare_algorithms(grid);
    }
  
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
    cout << setw(20) << left << "Ford-Fulkerson"
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
