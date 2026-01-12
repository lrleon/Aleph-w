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
 * @file mst_example.C
 * @brief Minimum Spanning Tree algorithms: Kruskal and Prim
 * 
 * This example demonstrates two classic algorithms for finding the
 * Minimum Spanning Tree (MST) of a weighted undirected graph.
 * 
 * ## Algorithms Compared
 * 
 * ### Kruskal's Algorithm
 * - Sorts all edges by weight
 * - Greedily adds edges that don't create cycles
 * - Uses Union-Find data structure
 * - Time: O(E log E) = O(E log V)
 * - Best for: Sparse graphs
 * 
 * ### Prim's Algorithm
 * - Grows MST from a starting vertex
 * - Always adds the minimum weight edge to the tree
 * - Uses priority queue
 * - Time: O(E log V) with binary heap
 * - Best for: Dense graphs
 * 
 * ## Applications
 * 
 * - Network design (minimum cost to connect all nodes)
 * - Cluster analysis
 * - Approximation algorithms for NP-hard problems
 * - Image segmentation
 * 
 * @see Kruskal.H Prim.H
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <random>
#include <tpl_graph.H>
#include <Kruskal.H>
#include <Prim.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Node: location name
using LocationNode = Graph_Node<string>;

// Arc: cable length (cost)
using CableArc = Graph_Arc<double>;

// Undirected graph for network design
using NetworkGraph = List_Graph<LocationNode, CableArc>;

// Distance accessor
struct CableLength
{
  using Distance_Type = double;
  static constexpr double Max_Distance = numeric_limits<double>::infinity();
  static constexpr double Zero_Distance = 0.0;
  
  double operator()(NetworkGraph::Arc* arc) const
  {
    return arc->get_info();
  }
};

/**
 * @brief Build a sample network graph
 * 
 * Represents buildings that need to be connected with network cables.
 * Edge weights represent cable lengths (cost).
 * 
 *     Library ----15---- AdminBldg ----20---- Gym
 *        |                  |                  |
 *       12                  8                 25
 *        |                  |                  |
 *     SciLab -----10----- MainHall ----18---- Dorm
 *        |                  |                  |
 *       22                 14                 16
 *        |                  |                  |
 *     ArtStudio ---9---- Cafeteria ---11--- Theater
 */
NetworkGraph build_campus_network()
{
  NetworkGraph g;
  
  // Buildings
  auto library   = g.insert_node("Library");
  auto admin     = g.insert_node("AdminBldg");
  auto gym       = g.insert_node("Gym");
  auto scilab    = g.insert_node("SciLab");
  auto mainhall  = g.insert_node("MainHall");
  auto dorm      = g.insert_node("Dorm");
  auto artstudio = g.insert_node("ArtStudio");
  auto cafeteria = g.insert_node("Cafeteria");
  auto theater   = g.insert_node("Theater");
  
  // Potential cable routes (with lengths in meters)
  g.insert_arc(library, admin, 15);
  g.insert_arc(admin, gym, 20);
  g.insert_arc(library, scilab, 12);
  g.insert_arc(admin, mainhall, 8);
  g.insert_arc(gym, dorm, 25);
  g.insert_arc(scilab, mainhall, 10);
  g.insert_arc(mainhall, dorm, 18);
  g.insert_arc(scilab, artstudio, 22);
  g.insert_arc(mainhall, cafeteria, 14);
  g.insert_arc(dorm, theater, 16);
  g.insert_arc(artstudio, cafeteria, 9);
  g.insert_arc(cafeteria, theater, 11);
  
  // Additional cross-connections for more interesting MST
  g.insert_arc(library, mainhall, 17);
  g.insert_arc(scilab, cafeteria, 19);
  g.insert_arc(admin, dorm, 30);
  
  return g;
}

/**
 * @brief Generate a random graph for performance testing
 */
NetworkGraph generate_random_graph(size_t num_nodes, size_t num_edges, 
                                   unsigned seed)
{
  NetworkGraph g;
  mt19937 rng(seed);
  uniform_real_distribution<double> weight_dist(1.0, 100.0);
  
  // Create nodes
  vector<NetworkGraph::Node*> nodes;
  for (size_t i = 0; i < num_nodes; i++)
    nodes.push_back(g.insert_node("N" + to_string(i)));
  
  // First, create a spanning tree to ensure connectivity
  for (size_t i = 1; i < num_nodes; i++)
  {
    uniform_int_distribution<size_t> parent_dist(0, i - 1);
    size_t parent = parent_dist(rng);
    g.insert_arc(nodes[parent], nodes[i], weight_dist(rng));
  }
  
  // Add remaining edges
  size_t edges_to_add = num_edges - (num_nodes - 1);
  uniform_int_distribution<size_t> node_dist(0, num_nodes - 1);
  
  for (size_t i = 0; i < edges_to_add; i++)
  {
    size_t a = node_dist(rng);
    size_t b = node_dist(rng);
    if (a != b)
      g.insert_arc(nodes[a], nodes[b], weight_dist(rng));
  }
  
  return g;
}

/**
 * @brief Print MST result
 */
void print_mst(NetworkGraph& tree, const string& algorithm)
{
  double total_weight = 0;
  
  cout << "\n" << algorithm << " MST Edges:" << endl;
  for (auto ait = tree.get_arc_it(); ait.has_curr(); ait.next())
  {
    auto* arc = ait.get_curr();
    auto* src = tree.get_src_node(arc);
    auto* tgt = tree.get_tgt_node(arc);
    double weight = arc->get_info();
    total_weight += weight;
    
    cout << "  " << setw(12) << left << src->get_info() 
         << " --- " << setw(5) << right << weight 
         << " --- " << tgt->get_info() << endl;
  }
  
  cout << "Total weight: " << total_weight << endl;
  cout << "Edges in MST: " << tree.get_num_arcs() << endl;
}

/**
 * @brief Run Kruskal's algorithm
 */
double run_kruskal(NetworkGraph& g, NetworkGraph& tree, bool verbose)
{
  auto start = chrono::high_resolution_clock::now();
  
  Kruskal_Min_Spanning_Tree<NetworkGraph, CableLength> kruskal;
  kruskal(g, tree);
  
  auto end = chrono::high_resolution_clock::now();
  double ms = chrono::duration<double, milli>(end - start).count();
  
  if (verbose)
    print_mst(tree, "Kruskal's");
  
  return ms;
}

/**
 * @brief Run Prim's algorithm
 */
double run_prim(NetworkGraph& g, NetworkGraph& tree, bool verbose)
{
  auto start = chrono::high_resolution_clock::now();
  
  Prim_Min_Spanning_Tree<NetworkGraph, CableLength> prim;
  prim(g, tree);
  
  auto end = chrono::high_resolution_clock::now();
  double ms = chrono::duration<double, milli>(end - start).count();
  
  if (verbose)
    print_mst(tree, "Prim's");
  
  return ms;
}

/**
 * @brief Calculate total MST weight
 */
double mst_total_weight(NetworkGraph& tree)
{
  double total = 0;
  for (auto ait = tree.get_arc_it(); ait.has_curr(); ait.next())
    total += ait.get_curr()->get_info();
  return total;
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Minimum Spanning Tree Example", ' ', "1.0");
    
    TCLAP::SwitchArg benchArg("b", "benchmark",
      "Run performance benchmark", false);
    TCLAP::ValueArg<size_t> nodesArg("n", "nodes",
      "Number of nodes for benchmark", false, 1000, "size_t");
    TCLAP::ValueArg<size_t> edgesArg("e", "edges",
      "Number of edges for benchmark", false, 5000, "size_t");
    TCLAP::ValueArg<unsigned> seedArg("s", "seed",
      "Random seed", false, 42, "unsigned");
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    
    cmd.add(benchArg);
    cmd.add(nodesArg);
    cmd.add(edgesArg);
    cmd.add(seedArg);
    cmd.add(verboseArg);
    
    cmd.parse(argc, argv);
    
    bool benchmark = benchArg.getValue();
    size_t num_nodes = nodesArg.getValue();
    size_t num_edges = edgesArg.getValue();
    unsigned seed = seedArg.getValue();
    bool verbose = verboseArg.getValue();
    
    cout << "=== Minimum Spanning Tree Algorithms ===" << endl;
    
    if (!benchmark)
    {
      // Demo with small graph
      cout << "\nBuilding campus network..." << endl;
      NetworkGraph g = build_campus_network();
      
      cout << "Network has " << g.get_num_nodes() << " buildings and "
           << g.get_num_arcs() << " potential cable routes." << endl;
      
      // Run Kruskal
      cout << "\n--- Kruskal's Algorithm ---" << endl;
      cout << "Strategy: Sort edges, add if no cycle (uses Union-Find)" << endl;
      
      NetworkGraph kruskal_tree;
      double kruskal_time = run_kruskal(g, kruskal_tree, true);
      cout << "Time: " << fixed << setprecision(3) << kruskal_time << " ms" << endl;
      
      // Run Prim
      cout << "\n--- Prim's Algorithm ---" << endl;
      cout << "Strategy: Grow tree from start, always add cheapest edge" << endl;
      
      NetworkGraph prim_tree;
      double prim_time = run_prim(g, prim_tree, true);
      cout << "Time: " << fixed << setprecision(3) << prim_time << " ms" << endl;
      
      // Verify both give same total weight
      double kruskal_weight = mst_total_weight(kruskal_tree);
      double prim_weight = mst_total_weight(prim_tree);
      
      cout << "\n--- Verification ---" << endl;
      cout << "Kruskal MST weight: " << kruskal_weight << endl;
      cout << "Prim MST weight:    " << prim_weight << endl;
      
      if (abs(kruskal_weight - prim_weight) < 1e-9)
        cout << "Both algorithms found optimal MST!" << endl;
      else
        cout << "Warning: Weights differ (may have multiple optimal MSTs)" << endl;
    }
    else
    {
      // Performance benchmark
      cout << "\nGenerating random graph with " << num_nodes << " nodes and "
           << num_edges << " edges..." << endl;
      
      NetworkGraph g = generate_random_graph(num_nodes, num_edges, seed);
      cout << "Graph created: " << g.get_num_nodes() << " nodes, "
           << g.get_num_arcs() << " edges" << endl;
      
      // Benchmark Kruskal
      NetworkGraph kruskal_tree;
      double kruskal_time = run_kruskal(g, kruskal_tree, verbose);
      
      // Benchmark Prim
      NetworkGraph prim_tree;
      double prim_time = run_prim(g, prim_tree, verbose);
      
      cout << "\n--- Performance Results ---" << endl;
      cout << setw(15) << "Algorithm" << setw(15) << "Time (ms)" 
           << setw(15) << "MST Weight" << endl;
      cout << string(45, '-') << endl;
      cout << setw(15) << "Kruskal" 
           << setw(15) << fixed << setprecision(3) << kruskal_time
           << setw(15) << setprecision(2) << mst_total_weight(kruskal_tree) << endl;
      cout << setw(15) << "Prim"
           << setw(15) << fixed << setprecision(3) << prim_time
           << setw(15) << setprecision(2) << mst_total_weight(prim_tree) << endl;
      
      cout << "\n--- Complexity Analysis ---" << endl;
      cout << "V = " << num_nodes << ", E = " << num_edges << endl;
      cout << "E/V ratio: " << fixed << setprecision(2) 
           << (double)num_edges / num_nodes << " (";
      
      double ratio = (double)num_edges / num_nodes;
      if (ratio < 2)
        cout << "sparse graph -> Kruskal recommended";
      else if (ratio > 10)
        cout << "dense graph -> Prim recommended";
      else
        cout << "medium density -> similar performance";
      cout << ")" << endl;
    }
    
    cout << "\n=== Algorithm Summary ===" << endl;
    cout << "Kruskal: O(E log E), best for sparse graphs" << endl;
    cout << "Prim:    O(E log V), best for dense graphs" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}

