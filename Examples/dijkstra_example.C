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
 * @file dijkstra_example.C
 * @brief Demonstration of Dijkstra's shortest path algorithm
 * 
 * This example demonstrates Dijkstra's algorithm for finding shortest
 * paths from a source vertex to all other vertices in a weighted graph.
 * 
 * ## Features Demonstrated
 * 
 * 1. **Graph Construction**: Building a weighted directed graph
 * 2. **Shortest Path Tree**: Computing all shortest paths from a source
 * 3. **Path Retrieval**: Getting the actual path to a destination
 * 4. **Distance Queries**: Finding distances between nodes
 * 
 * ## Algorithm Overview
 * 
 * Dijkstra's algorithm maintains a priority queue of nodes ordered by
 * their tentative distance from the source. At each step, it:
 * 1. Extracts the node with minimum tentative distance
 * 2. Relaxes all outgoing edges from that node
 * 3. Updates the priority queue with new distances
 * 
 * Time complexity: O((V + E) log V) with binary heap
 * 
 * ## Example Graph
 * 
 * The example builds a graph representing cities connected by roads
 * with distances (in km), then finds shortest paths between them.
 * 
 * @note Dijkstra's algorithm requires non-negative edge weights.
 *       For graphs with negative weights, use Bellman-Ford.
 * 
 * @see Dijkstra.H for the implementation
 * @author Leandro Rabindranath León
 * @ingroup Ejemplos
 * @see Bellman_Ford.H for negative weight handling
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <tpl_graph.H>
#include <Dijkstra.H>
#include <tclap/CmdLine.h>

using namespace std;
using namespace Aleph;

// Node type: city name
using CityNode = Graph_Node<string>;

// Arc type: distance in km
using RoadArc = Graph_Arc<double>;

// Graph type: directed graph of cities connected by roads
using CityGraph = List_Digraph<CityNode, RoadArc>;

// Distance accessor for Dijkstra
struct RoadDistance
{
  using Distance_Type = double;
  static constexpr double Max_Distance = numeric_limits<double>::infinity();
  static constexpr double Zero_Distance = 0.0;
  
  double operator()(CityGraph::Arc* arc) const
  {
    return arc->get_info();
  }
};

/**
 * @brief Build a sample graph of cities
 * 
 * Creates a graph representing cities in a fictional region:
 * 
 *     Alpha --100-- Beta --150-- Gamma
 *       |            |            |
 *      200          50           100
 *       |            |            |
 *     Delta --80-- Epsilon --120-- Zeta
 *       |                          |
 *      300                        90
 *       |                          |
 *     Eta --------250----------- Theta
 */
CityGraph build_city_graph()
{
  CityGraph g;
  
  // Insert cities (nodes)
  auto alpha   = g.insert_node("Alpha");
  auto beta    = g.insert_node("Beta");
  auto gamma   = g.insert_node("Gamma");
  auto delta   = g.insert_node("Delta");
  auto epsilon = g.insert_node("Epsilon");
  auto zeta    = g.insert_node("Zeta");
  auto eta     = g.insert_node("Eta");
  auto theta   = g.insert_node("Theta");
  
  // Insert roads (arcs) - bidirectional roads as two directed arcs
  auto add_road = [&](auto* from, auto* to, double dist) {
    g.insert_arc(from, to, dist);
    g.insert_arc(to, from, dist);
  };
  
  add_road(alpha, beta, 100);
  add_road(beta, gamma, 150);
  add_road(alpha, delta, 200);
  add_road(beta, epsilon, 50);
  add_road(gamma, zeta, 100);
  add_road(delta, epsilon, 80);
  add_road(epsilon, zeta, 120);
  add_road(delta, eta, 300);
  add_road(zeta, theta, 90);
  add_road(eta, theta, 250);
  
  return g;
}

/**
 * @brief Find a node by its name
 */
CityGraph::Node* find_city(CityGraph& g, const string& name)
{
  for (auto it = g.get_node_it(); it.has_curr(); it.next())
    if (it.get_curr()->get_info() == name)
      return it.get_curr();
  return nullptr;
}

/**
 * @brief Print the graph structure
 */
void print_graph(CityGraph& g)
{
  cout << "\n=== City Road Network ===" << endl;
  cout << "Cities: " << g.get_num_nodes() << endl;
  cout << "Roads:  " << g.get_num_arcs() / 2 << " (bidirectional)" << endl;
  cout << "\nConnections:" << endl;
  
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* node = nit.get_curr();
    cout << "  " << node->get_info() << " -> ";
    bool first = true;
    for (auto ait = g.get_out_it(node); ait.has_curr(); ait.next())
    {
      auto* arc = ait.get_curr();
      auto* tgt = g.get_tgt_node(arc);
      if (!first) cout << ", ";
      cout << tgt->get_info() << "(" << arc->get_info() << "km)";
      first = false;
    }
    cout << endl;
  }
}

/**
 * @brief Demonstrate Dijkstra's algorithm
 */
void demonstrate_dijkstra(CityGraph& g, const string& source_name, 
                          const string& dest_name, bool verbose)
{
  auto* source = find_city(g, source_name);
  auto* dest = find_city(g, dest_name);
  
  if (!source || !dest)
  {
    cerr << "Error: City not found!" << endl;
    return;
  }
  
  cout << "\n=== Dijkstra's Algorithm ===" << endl;
  cout << "Source: " << source_name << endl;
  cout << "Destination: " << dest_name << endl;
  
  // Create Dijkstra solver
  Dijkstra_Min_Paths<CityGraph, RoadDistance> dijkstra;
  
  // Method 1: Find shortest path directly
  cout << "\n--- Finding Shortest Path ---" << endl;
  
  Path<CityGraph> path(g);
  auto distance = dijkstra.find_min_path(g, source, dest, path);
  
  if (distance == RoadDistance::Max_Distance)
  {
    cout << "No path exists from " << source_name << " to " << dest_name << endl;
    return;
  }
  
  cout << "Total distance: " << distance << " km" << endl;
  cout << "Path: ";
  
  // Iterate through path nodes
  bool first = true;
  for (auto it = path.get_it(); it.has_curr(); it.next())
  {
    auto* node = it.get_current_node();
    if (!first) cout << " -> ";
    cout << node->get_info();
    first = false;
  }
  cout << endl;
  
  // Show step-by-step distances
  if (verbose)
  {
    cout << "\nStep-by-step:" << endl;
    double cumulative = 0;
    for (auto it = path.get_it(); it.has_curr(); it.next())
    {
      auto* arc = it.get_current_arc();
      if (arc)
      {
        cumulative += arc->get_info();
        cout << "  " << setw(10) << g.get_src_node(arc)->get_info() 
             << " --" << setw(4) << arc->get_info() << "km--> "
             << setw(10) << left << g.get_tgt_node(arc)->get_info()
             << right << " (cumulative: " << cumulative << " km)" << endl;
      }
    }
  }
  
  // Method 2: Compute entire shortest paths tree
  if (verbose)
  {
    cout << "\n--- Computing Full Shortest Path Tree ---" << endl;
    
    CityGraph tree;
    dijkstra.compute_min_paths_tree(g, source, tree);
    
    cout << "Shortest paths tree has " << tree.get_num_nodes() 
         << " nodes (reachable from " << source_name << ")" << endl;
  }
}

/**
 * @brief Show distances from source to all cities
 */
void show_all_distances(CityGraph& g, const string& source_name)
{
  auto* source = find_city(g, source_name);
  if (!source)
  {
    cerr << "Error: City " << source_name << " not found!" << endl;
    return;
  }
  
  cout << "\n=== Distances from " << source_name << " ===" << endl;
  
  Dijkstra_Min_Paths<CityGraph, RoadDistance> dijkstra;
  
  for (auto nit = g.get_node_it(); nit.has_curr(); nit.next())
  {
    auto* dest = nit.get_curr();
    if (dest == source) continue;
    
    Path<CityGraph> path(g);
    auto dist = dijkstra.find_min_path(g, source, dest, path);
    
    cout << "  " << setw(10) << left << dest->get_info() << right << ": ";
    if (dist == RoadDistance::Max_Distance)
      cout << "unreachable" << endl;
    else
      cout << setw(6) << dist << " km" << endl;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    TCLAP::CmdLine cmd("Dijkstra's Algorithm Example", ' ', "1.0");
    
    TCLAP::ValueArg<string> sourceArg("s", "source", 
      "Source city", false, "Alpha", "string");
    TCLAP::ValueArg<string> destArg("d", "dest",
      "Destination city", false, "Theta", "string");
    TCLAP::SwitchArg verboseArg("v", "verbose",
      "Show detailed output", false);
    TCLAP::SwitchArg allArg("a", "all",
      "Show distances to all cities", false);
    
    cmd.add(sourceArg);
    cmd.add(destArg);
    cmd.add(verboseArg);
    cmd.add(allArg);
    
    cmd.parse(argc, argv);
    
    string source = sourceArg.getValue();
    string dest = destArg.getValue();
    bool verbose = verboseArg.getValue();
    bool showAll = allArg.getValue();
    
    // Build the graph
    CityGraph g = build_city_graph();
    
    // Print graph structure
    print_graph(g);
    
    // Run Dijkstra
    demonstrate_dijkstra(g, source, dest, verbose);
    
    // Optionally show all distances
    if (showAll)
      show_all_distances(g, source);
    
    cout << "\n=== Algorithm Properties ===" << endl;
    cout << "Time complexity:  O((V + E) log V)" << endl;
    cout << "Space complexity: O(V)" << endl;
    cout << "Requirement:      Non-negative edge weights" << endl;
    
    return 0;
  }
  catch (TCLAP::ArgException& e)
  {
    cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
    return 1;
  }
}
