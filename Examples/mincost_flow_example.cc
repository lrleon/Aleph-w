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
 * @file mincost_flow_example.cc
 * @brief Minimum Cost Maximum Flow Examples
 * 
 * This example demonstrates the minimum cost flow problem and its solutions.
 * 
 * ## The Min-Cost Max-Flow Problem
 * 
 * Given a network with capacities and costs per unit:
 * - Maximize total flow from source to sink
 * - Among all maximum flows, minimize total cost
 * 
 * ## Algorithms
 * 
 * 1. **Cycle Canceling**: Find max-flow, then cancel negative-cost cycles
 * 2. **Network Simplex**: Specialized simplex for network flow problems
 * 
 * @see tpl_netcost.H for min-cost flow algorithms
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <tpl_netcost.H>
#include <tpl_mincost.H>

using namespace std;
using namespace Aleph;

// Type definitions
using Flow_Type = double;
using CostNet = Net_Cost_Graph<Net_Cost_Node<string>, Net_Cost_Arc<Empty_Class, Flow_Type>>;
using Node = CostNet::Node;
using Arc = CostNet::Arc;

/**
 * @brief Build a simple logistics network
 */
CostNet build_simple_network()
{
  CostNet net;
  
  auto s = net.insert_node("Source");
  auto a = net.insert_node("A");
  auto b = net.insert_node("B");
  auto t = net.insert_node("Sink");
  
  // insert_arc(src, tgt, cap, cost)
  net.insert_arc(s, a, 10, 2.0);  // cap=10, cost=2
  net.insert_arc(s, b, 8, 3.0);   // cap=8, cost=3
  net.insert_arc(a, b, 5, 1.0);   // cap=5, cost=1
  net.insert_arc(a, t, 7, 4.0);   // cap=7, cost=4
  net.insert_arc(b, t, 10, 2.0);  // cap=10, cost=2
  
  return net;
}

/**
 * @brief Print network with flows and costs
 */
void print_cost_network(CostNet& net, const string& title)
{
  cout << "\n=== " << title << " ===" << endl;
  cout << "Nodes: " << net.get_num_nodes() << ", Arcs: " << net.get_num_arcs() << endl;
  
  Flow_Type total_cost = 0;
  Flow_Type total_flow = 0;
  
  cout << "\nArc flows:" << endl;
  
  for (auto it = net.get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = net.get_src_node(arc);
    auto* tgt = net.get_tgt_node(arc);
    
    Flow_Type arc_cost = arc->flow * arc->cost;
    total_cost += arc_cost;
    
    if (net.is_source(src))
      total_flow += arc->flow;
    
    cout << "  " << setw(8) << left << src->get_info()
         << " -> " << setw(8) << left << tgt->get_info()
         << right << " : " << setw(4) << arc->flow 
         << " / " << setw(4) << arc->cap
         << " @ $" << arc->cost << endl;
  }
  
  cout << "\nTotal flow: " << total_flow << endl;
  cout << "Total cost: $" << fixed << setprecision(2) << total_cost << endl;
}

/**
 * @brief Demonstrate the assignment problem
 */
void demo_assignment_problem()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 2: Assignment Problem" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nProblem: Assign 3 workers to 3 jobs minimizing total cost." << endl;
  cout << "\nCost matrix:" << endl;
  cout << "          Job1  Job2  Job3" << endl;
  cout << "Worker1:   $9    $2    $7" << endl;
  cout << "Worker2:   $6    $4    $3" << endl;
  cout << "Worker3:   $5    $8    $1" << endl;
  
  vector<vector<double>> costs = {
    {9, 2, 7},  // Worker 1
    {6, 4, 3},  // Worker 2
    {5, 8, 1}   // Worker 3
  };
  
  auto result = solve_assignment<double>(costs);
  
  cout << "\nOptimal assignment:" << endl;
  
  if (result.feasible)
  {
    for (auto it = result.assignments.get_it(); it.has_curr(); it.next())
    {
      auto [w, j] = it.get_curr();
      cout << "  Worker" << (w+1) << " -> Job" << (j+1)
           << " (cost = $" << costs[w][j] << ")" << endl;
    }
    cout << "\nMinimum total cost: $" << result.total_cost << endl;
  }
  else
  {
    cout << "No feasible assignment found." << endl;
  }
}

/**
 * @brief Demonstrate the transportation problem
 */
void demo_transportation_problem()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 3: Transportation Problem" << endl;
  cout << string(60, '=') << endl;
  
  cout << "\nProblem: Ship goods from 2 warehouses to 3 stores." << endl;
  
  vector<double> supply = {30, 20};
  vector<double> demand = {15, 20, 15};
  vector<vector<double>> costs = {
    {4, 8, 8},  // Warehouse 1 to stores
    {6, 2, 4}   // Warehouse 2 to stores
  };
  
  auto result = solve_transportation<double>(supply, demand, costs);
  
  if (result.feasible)
  {
    cout << "\nOptimal shipments:" << endl;
    for (size_t i = 0; i < result.shipments.size(); ++i)
    {
      cout << "  Warehouse" << (i+1) << ": ";
      for (size_t j = 0; j < result.shipments[i].size(); ++j)
        cout << setw(6) << result.shipments[i][j] << " ";
      cout << endl;
    }
    cout << "\nMinimum cost: $" << result.total_cost << endl;
  }
  else
  {
    cout << "No feasible solution found." << endl;
  }
}

/**
 * @brief Demonstrate min-cost max-flow
 */
void demo_mincost_maxflow()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Example 1: Min-Cost Max-Flow" << endl;
  cout << string(60, '=') << endl;
  
  CostNet net = build_simple_network();
  
  print_cost_network(net, "Initial Network");
  
  cout << "\n--- Computing Min-Cost Max-Flow ---" << endl;
  
  // Using cycle canceling
  auto result = max_flow_min_cost_by_cycle_canceling(net);
  
  print_cost_network(net, "After Optimization");
  
  cout << "\n*** Results ***" << endl;
  cout << "Maximum flow: " << get<0>(result) << endl;
  cout << "Minimum cost: $" << fixed << setprecision(2) << get<1>(result) << endl;
}

int main()
{
  cout << "=== Minimum Cost Maximum Flow ===" << endl;
  cout << "Optimize flow networks with costs per unit\n" << endl;
  
  demo_mincost_maxflow();
  demo_assignment_problem();
  demo_transportation_problem();
  
  cout << "\n" << string(60, '=') << endl;
  cout << "Summary" << endl;
  cout << string(60, '=') << endl;
  
  cout << R"(
Min-Cost Max-Flow Problem:
  
  Given: Network with capacities and per-unit costs
  Find: Flow maximizing total flow at minimum cost
  
Algorithms:
  - Cycle Canceling: Simple, cancel negative-cost cycles
  - Network Simplex: Efficient, maintains spanning tree
  
Applications:
  - Transportation: Ship goods at minimum cost
  - Assignment: Match entities minimizing total cost
  - Supply Chain: Optimize logistics networks
)" << endl;
  
  return 0;
}
