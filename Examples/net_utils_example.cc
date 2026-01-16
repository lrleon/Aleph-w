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
 * @file net_utils_example.cc
 * @brief Network Utilities: Generation, Visualization, Serialization
 * 
 * This example demonstrates utility functions for working with
 * flow networks in Aleph-w. These utilities help with network
 * generation, visualization, serialization, and benchmarking,
 * making it easier to work with flow networks in practice.
 *
 * ## Why Network Utilities?
 *
 * Working with flow networks often requires:
 * - **Generating test networks**: For algorithm development and testing
 * - **Visualizing networks**: To understand structure and debug
 * - **Saving/loading networks**: For persistence and sharing
 * - **Benchmarking**: To compare algorithm performance
 *
 * ## Features
 *
 * ### 1. Network Generation
 *
 * #### Random Networks
 * - **Approximate density**: Choose an edge count from a target density
 * - **Random capacities**: Assign random capacities to edges
 * - **Configurable size**: Control number of vertices and edges
 *
 * #### Grid Networks
 * - **Regular structure**: Grid-based networks for benchmarking
 * - **Predictable**: Known properties for testing
 * - **Scalable**: Easy to generate large grids
 *
 * #### Layered Networks
 * - **DAG structure**: Directed acyclic graphs
 * - **Layered layout**: Vertices organized in layers
 * - **Applications**: Perfect for DAG-based flow problems
 *
 * #### Bipartite Networks
 * - **Two sets**: Vertices divided into two sets
 * - **Matching problems**: Ideal for bipartite matching
 * - **Applications**: Job assignment, resource allocation
 *
 * ### 2. Visualization
 *
 * #### DOT Format Export
 * - **GraphViz compatibility**: Export to DOT format
 * - **Visualization tools**: Use GraphViz, dot, neato, etc.
 * - **Customizable**: Show capacities, flows, labels
 *
 * ### 3. Serialization
 *
 * #### JSON Export
 * - **Human-readable**: Easy to inspect and edit
 * - **Structured data**: Preserve network structure
 * - **Interoperability**: Work with other tools
 *
 * #### DIMACS Format
 * - **Standard format**: DIMACS challenge format
 * - **Compatibility**: Work with other max-flow solvers
 * - **Benchmarking**: Use standard test cases
 *
 * ### 4. Benchmarking
 *    - Compare algorithm performance
 *    - Generate test cases
 * 
 * @see net_utils.H
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>
#include <limits>
#include <tpl_net.H>
#include <tpl_maxflow.H>
#include <net_utils.H>

using namespace std;
using namespace Aleph;

// Type definitions
using FlowType = double;
using Net = Net_Graph<Net_Node<string>, Net_Arc<Empty_Class, FlowType>>;
using Node = Net::Node;

static size_t density_to_num_arcs(size_t n, double density)
{
  if (n < 2)
    return 0;

  const double max_arcs = static_cast<double>(n) * (n - 1);
  size_t m = static_cast<size_t>(density * max_arcs);
  if (m < n - 1)
    m = n - 1;
  return m;
}

static void add_super_source_and_sink(Net& net)
{
  vector<Node*> nodes;
  for (auto it = net.get_node_it(); it.has_curr(); it.next())
    nodes.push_back(it.get_curr());

  FlowType total_cap = 0;
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
    total_cap += it.get_curr()->cap;
  if (total_cap <= FlowType{0})
    total_cap = FlowType{1};

  const FlowType cap = total_cap;
  auto s = net.insert_node("SuperSource");
  auto t = net.insert_node("SuperSink");

  for (Node* v : nodes)
    {
      net.insert_arc(s, v, cap);
      net.insert_arc(v, t, cap);
    }
}

/**
 * @brief Print network statistics
 */
void print_network_stats(Net& net, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Nodes: " << net.get_num_nodes() << endl;
  cout << "Arcs:  " << net.get_num_arcs() << endl;
  
  // Calculate density
  size_t n = net.get_num_nodes();
  size_t m = net.get_num_arcs();
  double max_arcs = n * (n - 1);  // Directed graph
  double density = (max_arcs > 0) ? 100.0 * m / max_arcs : 0;
  
  cout << "Density: " << fixed << setprecision(1) << density << "%" << endl;
  
  // Calculate total capacity
  FlowType total_cap = 0;
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
    total_cap += it.get_curr()->cap;
  
  cout << "Total capacity: " << total_cap << endl;
}

/**
 * @brief Demo 1: Random Network Generation
 */
void demo_random_networks()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 1: Random Network Generation" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nGenerating random networks with different parameters..." << endl;
  
  // Small sparse network
  {
    Net net = generate_random_network<Net>(10, density_to_num_arcs(10, 0.2), 1.0, 10.0);
    print_network_stats(net, "Small Sparse (n=10, density=20%)");

    add_super_source_and_sink(net);
    FlowType flow = dinic_maximum_flow(net);
    cout << "Max flow: " << flow << endl;
  }
  
  // Medium network
  {
    Net net = generate_random_network<Net>(20, density_to_num_arcs(20, 0.3), 5.0, 50.0);
    print_network_stats(net, "Medium (n=20, density=30%)");

    add_super_source_and_sink(net);
    FlowType flow = dinic_maximum_flow(net);
    cout << "Max flow: " << flow << endl;
  }
  
  // Dense network
  {
    Net net = generate_random_network<Net>(15, density_to_num_arcs(15, 0.6), 1.0, 100.0);
    print_network_stats(net, "Dense (n=15, density=60%)");

    add_super_source_and_sink(net);
    FlowType flow = dinic_maximum_flow(net);
    cout << "Max flow: " << flow << endl;
  }
}

/**
 * @brief Demo 2: Grid Network Generation
 */
void demo_grid_networks()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 2: Grid Network Generation" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nGrid networks are useful for benchmarking." << endl;
  cout << "Source is top-left, sink is bottom-right.\n" << endl;
  
  // Different grid sizes
  for (int size : {3, 5, 8})
  {
    Net net = generate_grid_network<Net>(size, size, 10.0, false);
    
    stringstream title;
    title << size << "x" << size << " Grid";
    print_network_stats(net, title.str());
    
    auto start = chrono::high_resolution_clock::now();
    FlowType flow = dinic_maximum_flow(net);
    auto end = chrono::high_resolution_clock::now();
    
    double ms = chrono::duration<double, milli>(end - start).count();
    cout << "Max flow: " << flow << " (computed in " 
         << fixed << setprecision(3) << ms << " ms)" << endl;
  }
}

/**
 * @brief Demo 3: Layered Network Generation
 */
void demo_layered_networks()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 3: Layered Network Generation" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nLayered networks have nodes in discrete layers." << endl;
  cout << "Edges only go from one layer to the next (DAG structure).\n" << endl;
  
  vector<size_t> layers = {1, 3, 4, 3, 1};  // Source, 3, 4, 3, Sink
  
  Net net = generate_layered_network<Net>(layers, 20.0, 0.7);
  
  cout << "Layer structure: ";
  for (size_t l : layers)
    cout << l << " ";
  cout << endl;
  
  print_network_stats(net, "Layered Network");
  
  FlowType flow = dinic_maximum_flow(net);
  cout << "Max flow: " << flow << endl;
  
  cout << "\nLayered networks model:" << endl;
  cout << "  - Assembly lines (stages of production)" << endl;
  cout << "  - Communication protocols (network layers)" << endl;
  cout << "  - Project scheduling (phases)" << endl;
}

/**
 * @brief Demo 4: DOT Export for Visualization
 */
void demo_dot_export()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 4: GraphViz DOT Export" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nCreating a small network and exporting to DOT format..." << endl;
  
  // Create a simple network
  Net net;
  
  auto s = net.insert_node("Source");
  auto a = net.insert_node("A");
  auto b = net.insert_node("B");
  auto c = net.insert_node("C");
  auto t = net.insert_node("Sink");
  
  net.insert_arc(s, a, 10);
  net.insert_arc(s, b, 8);
  net.insert_arc(a, b, 5);
  net.insert_arc(a, c, 7);
  net.insert_arc(b, c, 6);
  net.insert_arc(b, t, 9);
  net.insert_arc(c, t, 12);
  
  // Compute max flow first
  FlowType flow = dinic_maximum_flow(net);
  cout << "Max flow computed: " << flow << endl;
  
  // Export to DOT string
  DotExportOptions opts;
  opts.graph_name = "sample_network";
  opts.show_capacity = true;
  opts.show_flow = true;
  
  string dot = network_to_dot_string(net, opts);
  
  cout << "\nDOT output:" << endl;
  cout << string(40, '-') << endl;
  cout << dot;
  cout << string(40, '-') << endl;
  
  cout << "\nTo visualize, save to .dot file and run:" << endl;
  cout << "  dot -Tpng network.dot -o network.png" << endl;
  
  cout << "\nCommands to visualize DOT files:" << endl;
  cout << "  dot -Tpng network.dot -o network.png" << endl;
  cout << "  dot -Tsvg network.dot -o network.svg" << endl;
}

/**
 * @brief Demo 5: JSON Serialization
 */
void demo_json_export()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 5: JSON Serialization" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nExporting network to JSON format..." << endl;
  
  // Create a small network
  Net net = generate_random_network<Net>(5, density_to_num_arcs(5, 0.4), 1.0, 10.0);
  add_super_source_and_sink(net);
  dinic_maximum_flow(net);
  
  string json = network_to_json_string(net);
  
  cout << "\nJSON output:" << endl;
  cout << string(40, '-') << endl;
  cout << json << endl;
  cout << string(40, '-') << endl;
  
  cout << "\nJSON format is useful for:" << endl;
  cout << "  - Web visualization (D3.js, vis.js)" << endl;
  cout << "  - Data exchange between systems" << endl;
  cout << "  - Storing network configurations" << endl;
}

/**
 * @brief Demo 6: Benchmarking
 */
void demo_benchmarking()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Demo 6: Algorithm Benchmarking" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nComparing max-flow algorithms on different network types...\n" << endl;
  
  cout << setw(20) << left << "Network Type"
       << setw(10) << "Nodes"
       << setw(10) << "Arcs"
       << setw(16) << "F-F (DFS) (ms)"
       << setw(12) << "Dinic (ms)"
       << setw(10) << "Flow" << endl;
  cout << string(74, '-') << endl;
  
  auto benchmark = [](Net& net, const string& name) {
    size_t n = net.get_num_nodes();
    size_t m = net.get_num_arcs();
    
    // Ford-Fulkerson (DFS augmenting paths)
    Net net1 = net;  // Copy
    add_super_source_and_sink(net1);
    auto t1 = chrono::high_resolution_clock::now();
    FlowType f1 = ford_fulkerson_maximum_flow(net1);
    auto t2 = chrono::high_resolution_clock::now();
    double ek_ms = chrono::duration<double, milli>(t2 - t1).count();
    
    // Dinic
    Net net2 = net;
    add_super_source_and_sink(net2);
    t1 = chrono::high_resolution_clock::now();
    dinic_maximum_flow(net2);
    t2 = chrono::high_resolution_clock::now();
    double dinic_ms = chrono::duration<double, milli>(t2 - t1).count();
    
    cout << setw(20) << left << name
         << setw(10) << n
         << setw(10) << m
         << setw(16) << fixed << setprecision(3) << ek_ms
         << setw(12) << dinic_ms
         << setw(10) << setprecision(0) << f1 << endl;
  };
  
  // Random sparse
  {
    Net net = generate_random_network<Net>(30, density_to_num_arcs(30, 0.15), 1.0, 100.0);
    benchmark(net, "Random Sparse");
  }
  
  // Random dense
  {
    Net net = generate_random_network<Net>(20, density_to_num_arcs(20, 0.5), 1.0, 100.0);
    benchmark(net, "Random Dense");
  }
  
  // Grid
  {
    Net net = generate_grid_network<Net>(8, 8, 50.0, false);
    benchmark(net, "Grid 8x8");
  }
  
  // Layered
  {
    vector<size_t> layers = {1, 5, 8, 8, 5, 1};
    Net net = generate_layered_network<Net>(layers, 50.0, 0.6);
    benchmark(net, "Layered (6 layers)");
  }
  
  cout << "\nNote: Times may vary based on random network structure." << endl;
  cout << "Dinic is generally faster, especially on dense networks." << endl;
}

int main()
{
  cout << "=== Network Utilities ===" << endl;
  cout << "Generation, Visualization, and Serialization\n" << endl;
  
  demo_random_networks();
  demo_grid_networks();
  demo_layered_networks();
  demo_dot_export();
  demo_json_export();
  demo_benchmarking();
  
  // Summary
  cout << "\n" << string(60, '=') << endl;
  cout << "Summary" << endl;
  cout << string(60, '=') << endl;
  
  cout << R"(
Network Utilities in Aleph-w:

Generation Functions:
  - generate_random_network(n, m, min_cap, max_cap)
  - generate_grid_network(rows, cols, capacity, bidirectional)
  - generate_layered_network(layers, capacity, edge_prob)
  - generate_bipartite_network(left, right, edge_prob)

Visualization (DOT/GraphViz):
  - export_network_to_dot(net, filename, options)
  - network_to_dot_string(net, options)
  
  Visualize with: dot -Tpng network.dot -o network.png

Serialization:
  - network_to_json_string(net)
  - export_network_to_dimacs(net, filename)
  - import_network_from_dimacs<Net>(filename)

Use Cases:
  - Algorithm testing and benchmarking
  - Educational demonstrations
  - Network visualization
  - Data exchange between systems
)" << endl;
  
  return 0;
}
