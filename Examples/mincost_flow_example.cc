
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
 * @file mincost_flow_example.cc
 * @brief Minimum Cost Maximum Flow: Optimization with Cost Constraints
 * 
 * This example demonstrates the minimum cost flow problem, a fundamental
 * optimization problem that combines maximum flow with cost minimization.
 * Unlike basic max-flow (which only maximizes flow), min-cost max-flow
 * finds the cheapest way to achieve maximum flow.
 *
 * ## The Min-Cost Max-Flow Problem
 *
 * ### Problem Statement
 *
 * Given a directed network where each edge has:
 * - **Capacity**: Maximum flow allowed (c(e))
 * - **Cost**: Cost per unit of flow (w(e))
 *
 * Find a flow that:
 * 1. **Maximizes** total flow from source to sink
 * 2. **Minimizes** total cost among all maximum flows
 *
 * ### Mathematical Formulation
 *
 * ```
 * Minimize: Σ (flow(e) × cost(e)) for all edges e
 * 
 * Subject to:
 *   - Flow conservation: Σ flow into v = Σ flow out of v (for all v ≠ s,t)
 *   - Capacity constraints: 0 ≤ flow(e) ≤ capacity(e) for all edges e
 *   - Flow maximization: Total flow is maximum possible
 * ```
 *
 * ## Algorithms Demonstrated
 *
 * ### 1. Cycle Canceling Algorithm
 *
 * **Strategy**: Start with max-flow, then reduce cost by canceling negative cycles
 *
 * **Algorithm**:
 * ```
 * 1. Find any maximum flow (using Ford-Fulkerson, Dinic, etc.)
 * 2. Build residual graph with costs:
 *    - Forward edge: cost = original cost
 *    - Backward edge: cost = -original cost (can "undo" flow)
 * 3. While negative-cost cycle exists in residual graph:
 *    a. Find negative-cost cycle (using Bellman-Ford)
 *    b. Push flow around cycle (minimum residual capacity)
 *    c. Cost decreases by cycle_cost × flow_pushed
 * 4. Return min-cost max-flow
 * ```
 *
 * **Key insight**: Negative cycles in residual graph indicate we can
 * reduce cost by rerouting flow.
 *
 * **Complexity**: O(V × E² × C × U) where C = max absolute cost and U = max capacity
 * - May need many cycle cancellations
 *
 * **Best for**: Understanding the concept, small networks
 *
 * ### 2. Network Simplex
 *
 * **Strategy**: Specialized linear programming for networks
 *
 * **How it works**:
 * - Maintains a spanning tree structure (basis)
 * - Uses network structure for efficiency
 * - Pivots between spanning trees
 * - Much faster than general simplex
 *
 * **Complexity**: Often polynomial in practice, exponential worst case
 * - Usually faster than cycle canceling
 *
 * **Best for**: Large networks, production use
 *
 * ## Comparison with Max-Flow
 *
 * | Aspect | Max-Flow | Min-Cost Max-Flow |
 * |--------|----------|-------------------|
 * | Goal | Maximize flow | Maximize flow + minimize cost |
 * | Edge info | Capacity only | Capacity + cost |
 * | Complexity | O(VE²) | O(VE² × U) or higher |
 * | Applications | Simple routing | Cost optimization |
 *
 * ## Applications
 *
 * ### Transportation & Logistics
 * - **Package delivery**: Deliver maximum packages at minimum cost
 * - **Shipping**: Route goods through cheapest paths
 * - **Vehicle routing**: Optimize delivery routes
 *
 * ### Supply Chain
 * - **Production planning**: Optimize production and distribution
 * - **Inventory management**: Minimize storage and transport costs
 * - **Resource allocation**: Assign resources efficiently
 *
 * ### Telecommunications
 * - **Network routing**: Route data through cheapest paths
 * - **Bandwidth allocation**: Maximize throughput, minimize cost
 * - **Service provisioning**: Optimize service delivery
 *
 * ### Economics & Finance
 * - **Market clearing**: Clear markets with transaction costs
 * - **Portfolio optimization**: Maximize returns, minimize costs
 * - **Resource trading**: Optimize resource exchanges
 *
 * ### Energy Systems
 * - **Power distribution**: Minimize transmission costs
 * - **Gas pipelines**: Optimize gas flow and costs
 *
 * ## Example Scenario: Logistics Network
 *
 * ```
 * Network:
 *   Source → Warehouse A (capacity: 10, cost: 2/unit)
 *   Source → Warehouse B (capacity: 8, cost: 3/unit)
 *   Warehouse A → Warehouse B (capacity: 5, cost: 1/unit)
 *   Warehouse A → Sink (capacity: 12, cost: 1/unit)
 *   Warehouse B → Sink (capacity: 10, cost: 2/unit)
 * ```
 *
 * **Problem**: Maximize flow while minimizing total shipping cost.
 *
 * **Solution**: Find optimal flow distribution:
 * - Use cheaper paths when possible
 * - Balance flow to minimize total cost
 * - Still achieve maximum flow
 *
 * ## Complexity Analysis
 *
 * | Algorithm | Time Complexity | Notes |
 * |-----------|----------------|-------|
 * | Cycle Canceling | O(VE² × U) | U = max capacity, many cycles |
 * | Network Simplex | Exponential worst, polynomial average | Fast in practice |
 * | Successive Shortest Path | O(V × E × max_flow) | Alternative approach |
 *
 * ## When to Use
 *
 * ✅ **Use min-cost max-flow when**:
 * - Both flow and cost matter
 * - Need optimal cost solution
 * - Network has cost information
 *
 * ❌ **Use simple max-flow when**:
 * - Only flow matters (cost irrelevant)
 * - Simpler problem
 * - Faster solution needed
 *
 * ## Usage
 *
 * ```bash
 * # Run min-cost max-flow demo
 * ./mincost_flow_example
 *
 * # Compare algorithms
 * ./mincost_flow_example --compare
 *
 * # Test on specific network
 * ./mincost_flow_example --network logistics
 * ./mincost_flow_example --network assignment
 * ./mincost_flow_example --network transportation
 * ./mincost_flow_example --network all
 *
 * # Show help
 * ./mincost_flow_example --help
 * ```
 *
 * @note In Aleph-w, `max_flow_min_cost_by_cycle_canceling()` returns
 *       `(cycles_cancelled, it_factor)`; the maximum flow and the resulting
 *       minimum cost are read from the modified network (e.g.
 *       `net.get_out_flow(net.get_source())` and `net.flow_cost()`).
 *
 * @see tpl_netcost.H Network with cost structures
 * @see tpl_mincost.H Minimum cost flow algorithms
 * @see network_flow_example.C Basic max-flow (no cost)
 * @see maxflow_advanced_example.cc Advanced max-flow algorithms
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <tpl_netcost.H>
#include <tpl_mincost.H>

using namespace std;
using namespace Aleph;

static void usage(const char* prog)
{
  cout << "Usage: " << prog << " [--compare] [--network <logistics|assignment|transportation|all>] [--help]\n";
  cout << "\nIf no flags are given, all demos are executed.\n";
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

static void demo_compare_algorithms_on_logistics()
{
  cout << "\n" << string(60, '=') << endl;
  cout << "Comparison: Cycle Canceling vs Network Simplex (Logistics Network)" << endl;
  cout << string(60, '=') << endl;

  {
    CostNet net = build_simple_network();
    auto r = max_flow_min_cost_by_cycle_canceling(net);
    auto flow = net.get_out_flow(net.get_source());
    auto cost = net.flow_cost();
    cout << "Cycle canceling:\n";
    cout << "  Max flow: " << flow << "\n";
    cout << "  Total cost: $" << fixed << setprecision(2) << cost << "\n";
    cout << "  Cycles cancelled: " << get<0>(r) << "\n";
  }

  {
    CostNet net = build_simple_network();
    size_t pivots = max_flow_min_cost_by_network_simplex(net);
    auto flow = net.get_out_flow(net.get_source());
    auto cost = net.flow_cost();
    cout << "Network simplex:\n";
    cout << "  Max flow: " << flow << "\n";
    cout << "  Total cost: $" << fixed << setprecision(2) << cost << "\n";
    cout << "  Pivots: " << pivots << "\n";
  }
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
  auto flow = net.get_out_flow(net.get_source());
  auto cost = net.flow_cost();
  
  print_cost_network(net, "After Optimization");
  
  cout << "\n*** Results ***" << endl;
  cout << "Maximum flow: " << flow << endl;
  cout << "Minimum cost: $" << fixed << setprecision(2) << cost << endl;
  cout << "Cycles cancelled: " << get<0>(result) << endl;
}

int main(int argc, char* argv[])
{
  cout << "=== Minimum Cost Maximum Flow ===" << endl;
  cout << "Optimize flow networks with costs per unit\n" << endl;

  if (has_flag(argc, argv, "--help"))
    {
      usage(argv[0]);
      return 0;
    }

  const bool compare = has_flag(argc, argv, "--compare");
  const char* network = get_opt_value(argc, argv, "--network");
  const string sel = network ? string(network) : string("all");

  if (compare)
    {
      demo_compare_algorithms_on_logistics();
      return 0;
    }

  if (argc == 1 || sel == "all" || sel == "logistics")
    demo_mincost_maxflow();
  if (argc == 1 || sel == "all" || sel == "assignment")
    demo_assignment_problem();
  if (argc == 1 || sel == "all" || sel == "transportation")
    demo_transportation_problem();

  if (!(argc == 1 || sel == "all" || sel == "logistics" || sel == "assignment" || sel == "transportation"))
    {
      cout << "Unknown --network value: " << sel << "\n";
      usage(argv[0]);
      return 1;
    }
  
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
