
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


/** @file net_apps.H
 *  @brief Network flow applications.
 *
 *  This file provides solutions to classic problems that can be
 *  modeled and solved using network flow algorithms.
 *
 *  ## Problems Included
 *
 *  ### Circulation with Demands
 *  Find feasible flow satisfying node supplies/demands and edge lower bounds.
 *
 *  ### Project Selection
 *  Select projects to maximize profit given dependencies (max-flow min-cut).
 *
 *  ### Baseball Elimination
 *  Determine if a team can still win the championship.
 *
 *  ### Image Segmentation
 *  Binary image segmentation using graph cuts.
 *
 *  ### Survey Design
 *  Assign respondents to questions respecting constraints.
 *
 *  @see tpl_net.H Network flow structures
 *  @see tpl_maxflow.H Maximum flow algorithms
 *  @see tpl_mincost.H Minimum cost flow
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef NET_APPS_H
#define NET_APPS_H

#include <vector>
#include <string>
#include <functional>
#include <tpl_net.H>
#include <tpl_maxflow.H>
#include <tpl_netcost.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynListQueue.H>

namespace Aleph
{

//==============================================================================
// CIRCULATION WITH DEMANDS
//==============================================================================

/** @brief Result of a circulation problem.
 *  @ingroup Networks
 */
template <typename Flow_Type>
struct CirculationResult
{
  bool feasible{false};              ///< Is there a feasible circulation?
  Flow_Type excess_flow{0};          ///< Flow needed to satisfy demands
  DynMapTree<void*, Flow_Type> flow; ///< Flow on each edge (arc pointer -> flow)
};

/** @brief Solve a circulation problem with demands.
 *
 *  Given a network where:
 *  - Each node has a demand d(v): positive = needs flow, negative = produces flow
 *  - Each edge has lower bound l(e) and upper bound u(e)
 *
 *  Find a circulation f such that:
 *  - l(e) ≤ f(e) ≤ u(e) for all edges
 *  - For each node: in_flow - out_flow = demand
 *
 *  ## Algorithm
 *  1. Transform to max-flow problem:
 *     - Create super-source connected to nodes with negative demand
 *     - Create super-sink connected from nodes with positive demand
 *     - Adjust capacities for lower bounds
 *  2. Solve max-flow
 *  3. Check if flow saturates all super-source/sink edges
 *
 *  @tparam Net Network type.
 *  @tparam GetDemand Functor returning node demand.
 *  @tparam GetLower Functor returning edge lower bound.
 *  @tparam Maxflow Max-flow algorithm to use.
 *  @param net Network (modified during computation).
 *  @param get_demand Returns demand for each node.
 *  @param get_lower Returns the lower bound for each arc.
 *  @return CirculationResult with feasibility and flow values.
 *
 *  @par Example
 *  ```cpp
 *  Net_Graph<> net;
 *  // ... build network ...
 *
 *  auto result = solve_circulation(net,
 *    [](Node* n) { return node_demand[n]; },
 *    [](Arc* a) { return arc_lower[a]; });
 *
 *  if (result.feasible)
 *    std::cout << "Feasible circulation exists!\n";
 *  ```
 *
 *  @ingroup Networks
 */
template <class Net, class GetDemand, class GetLower,
          template <class> class Maxflow = Dinic_Maximum_Flow>
CirculationResult<typename Net::Flow_Type>
solve_circulation(Net& net, GetDemand get_demand, GetLower get_lower)
{
  using Node = typename Net::Node;
  using Arc = typename Net::Arc;
  using Flow_Type = typename Net::Flow_Type;

  CirculationResult<Flow_Type> result;

  // Collect original nodes (before adding super nodes)
  DynList<Node*> original_nodes;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    original_nodes.append(it.get_curr());

  // Store original capacities to restore later
  DynMapTree<Arc*, Flow_Type> original_caps;
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      original_caps[arc] = arc->cap;
    }

  // Create super-source and super-sink for the transformed network
  Node* super_source = net.insert_node();
  Node* super_sink = net.insert_node();

  // Track arcs we add so we can remove them later
  DynList<Arc*> added_arcs;

  // Calculate total positive ADJUSTED demand
  // (This must be computed from adjusted demands, not original demands)
  Flow_Type total_demand{0};

  // Add edges from super-source/to super-sink based on adjusted demands
  for (auto* v : original_nodes)
    {
      // Calculate adjusted demand: d'(v) = d(v) + l_out(v) - l_in(v)
      // This comes from the circulation reduction with lower bounds:
      // - For each arc (u,v) with lower bound l: the mandatory l units
      //   count as outflow from u and inflow to v.
      // - d'(u) = d(u) + l (gains "debt" from mandatory outflow)
      // - d'(v) = d(v) - l (satisfied by mandatory inflow)
      Flow_Type lower_in{0}, lower_out{0};
      for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
        {
          Arc* arc = ait.get_curr();
          Flow_Type lb = get_lower(arc);
          if (net.get_tgt_node(arc) == v)
            lower_in += lb;
          if (net.get_src_node(arc) == v)
            lower_out += lb;
        }

      Flow_Type adjusted_demand = get_demand(v) + lower_out - lower_in;

      if (adjusted_demand > 0)
        {
          total_demand += adjusted_demand;
          added_arcs.append(net.insert_arc(super_source, v, adjusted_demand));
        }
      else if (adjusted_demand < 0)
        added_arcs.append(net.insert_arc(v, super_sink, -adjusted_demand));
    }

  // Trivial case: no adjusted demands means circulation with lower bounds is feasible
  if (total_demand == Flow_Type{0})
    {
      result.feasible = true;
      // Set all flows to their lower bounds
      for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
        {
          Arc* arc = it.get_curr();
          if (original_caps.contains(arc))
            result.flow[arc] = get_lower(arc);
        }
      // Cleanup: remove added arcs and nodes
      for (Arc* arc : added_arcs)
        net.remove_arc(arc);
      net.remove_node(super_source);
      net.remove_node(super_sink);
      return result;
    }

  // Adjust arc capacities: cap' = cap - lower_bound
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* src = net.get_src_node(arc);
      Node* tgt = net.get_tgt_node(arc);

      // Skip super-source/sink arcs (they don't have lower bounds)
      if (src == super_source or tgt == super_sink)
        continue;

      Flow_Type lb = get_lower(arc);
      arc->cap -= lb;
    }

  // Ensure super_source is the ONLY source and super_sink is the ONLY sink.
  // For the max-flow to correctly compute feasibility, we need paths from s* to t*.
  //
  // For original sinks (nodes with no outgoing arcs), flow arriving at them has
  // nowhere to go. We add arcs to super_sink. The capacity should be the incoming
  // capacity from ORIGINAL arcs only (not demand arcs), since we're measuring
  // how much flow can reach this sink through the original network.
  //
  // For original sources (nodes with no incoming arcs), we add zero-capacity
  // arcs just to make them non-sources in the network topology.
  for (auto* v : original_nodes)
    {
      // Check if v is still a source (no incoming arcs)
      if (net.get_in_degree(v) == 0)
        added_arcs.append(net.insert_arc(super_source, v, Flow_Type{0}));

      // Check if v is still a sink (no outgoing arcs): add arc to super_sink
      // Compute incoming capacity from ORIGINAL arcs only
      if (net.get_out_degree(v) == 0)
        {
          Flow_Type in_cap_orig{0};
          for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
            {
              Arc* arc = ait.get_curr();
              if (net.get_tgt_node(arc) == v and original_caps.contains(arc))
                in_cap_orig += arc->cap;  // Use modified cap (original - lower)
            }
          added_arcs.append(net.insert_arc(v, super_sink, in_cap_orig));
        }
    }

  // Solve max-flow on the transformed network
  Flow_Type max_flow = Maxflow<Net>()(net);

  result.feasible = (max_flow == total_demand);
  result.excess_flow = max_flow;

  // Compute actual circulation flow on original arcs.
  // The max-flow determines feasibility but may route through auxiliary arcs.
  // We need to compute flows on original arcs using flow conservation equations.
  //
  // For each original arc, start with lower bound and adjust based on demands.
  // First, initialize flows to lower bounds.
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      if (original_caps.contains(arc))
        result.flow[arc] = get_lower(arc);
    }

  // If feasible, compute the actual circulation flows.
  // For each node, we need: inflow - outflow = demand
  // Use an iterative approach: repeatedly process nodes until all balances are satisfied.
  if (result.feasible)
    {
      // Iterate until all nodes have correct flow balance
      bool changed = true;
      int max_iterations = original_nodes.size() * 2;  // Safety limit
      while (changed and max_iterations-- > 0)
        {
          changed = false;
          for (auto* v : original_nodes)
            {
              // Compute current balance at v
              Flow_Type current_in{0}, current_out{0};
              for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
                {
                  Arc* arc = ait.get_curr();
                  if (not original_caps.contains(arc))
                    continue;
                  if (net.get_tgt_node(arc) == v)
                    current_in += result.flow[arc];
                  if (net.get_src_node(arc) == v)
                    current_out += result.flow[arc];
                }

              // imbalance = (current_in - current_out) - demand
              Flow_Type imbalance = (current_in - current_out) - get_demand(v);

              if (imbalance < Flow_Type{0})
                {
                  // Node needs more inflow - increase flow on incoming arcs
                  Flow_Type deficit = -imbalance;
                  for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
                    {
                      Arc* arc = ait.get_curr();
                      if (not original_caps.contains(arc))
                        continue;
                      if (net.get_tgt_node(arc) != v)
                        continue;

                      Flow_Type slack = original_caps[arc] - result.flow[arc];
                      Flow_Type increase = (slack < deficit) ? slack : deficit;
                      if (increase > Flow_Type{0})
                        {
                          result.flow[arc] += increase;
                          deficit -= increase;
                          changed = true;
                        }
                      if (deficit <= Flow_Type{0})
                        break;
                    }
                }
              else if (imbalance > Flow_Type{0})
                {
                  // Node has excess inflow - increase flow on outgoing arcs
                  Flow_Type excess = imbalance;
                  for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
                    {
                      Arc* arc = ait.get_curr();
                      if (not original_caps.contains(arc))
                        continue;
                      if (net.get_src_node(arc) != v)
                        continue;

                      Flow_Type slack = original_caps[arc] - result.flow[arc];
                      Flow_Type increase = (slack < excess) ? slack : excess;
                      if (increase > Flow_Type{0})
                        {
                          result.flow[arc] += increase;
                          excess -= increase;
                          changed = true;
                        }
                      if (excess <= Flow_Type{0})
                        break;
                    }
                }
            }
        }
    }

  // Cleanup: restore original capacities and reset flows
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      if (original_caps.contains(arc))
        {
          arc->cap = original_caps[arc];
          arc->flow = Flow_Type{0};  // Reset flow
        }
    }

  // Cleanup: remove added arcs (must be done before removing nodes)
  for (Arc* arc : added_arcs)
    net.remove_arc(arc);

  // Cleanup: remove super nodes we created
  net.remove_node(super_source);
  net.remove_node(super_sink);

  return result;
}


//==============================================================================
// PROJECT SELECTION (MAX PROFIT / CLOSURE)
//==============================================================================

/** @brief Project with profit and dependencies.
 *  @ingroup Networks
 */
template <typename Value_Type>
struct Project
{
  size_t id;                           ///< Unique project ID
  Value_Type profit;                   ///< Profit (positive) or cost (negative)
  std::vector<size_t> prerequisites;   ///< IDs of prerequisite projects
  std::string name;                    ///< Optional name

  Project(size_t id_, Value_Type profit_,
          const std::vector<size_t>& prereqs = {},
          const std::string& name_ = "")
    : id(id_), profit(profit_), prerequisites(prereqs), name(name_) {}
};

/** @brief Result of project selection.
 *  @ingroup Networks
 */
template <typename Value_Type>
struct ProjectSelectionResult
{
  Value_Type max_profit{0};            ///< Maximum achievable profit
  std::vector<size_t> selected;        ///< IDs of selected projects
  Value_Type total_revenue{0};         ///< Sum of positive profits
  Value_Type total_cost{0};            ///< Sum of negative profits (costs)
};

/** @brief Solve project selection problem using max-flow.
 *
 *  Given a set of projects with profits/costs and prerequisite constraints,
 *  select a subset of projects to maximize total profit.
 *
 *  Constraint: If project A is selected, all its prerequisites must also
 *  be selected.
 *
 *  ## Problem Formulation
 *  This is the "closure" problem, solved by min-cut:
 *  - Create source connected to all positive-profit projects (capacity = profit)
 *  - Create sink connected from all negative-profit projects (capacity = -profit)
 *  - Create infinite-capacity edges for prerequisites
 *  - Max profit = sum of positive profits - min cut
 *
 *  @tparam Value_Type Type for profit values.
 *  @param projects Vector of projects.
 *  @return ProjectSelectionResult with optimal selection.
 *
 *  @par Example
 *  ```cpp
 *  std::vector<Project<double>> projects = {
 *    {0, 100, {}, "Project A"},     // Profit 100, no prerequisites
 *    {1, 50, {0}, "Project B"},     // Profit 50, requires A
 *    {2, -70, {}, "Infrastructure"} // Cost 70, no prerequisites
 *  };
 *
 *  auto result = solve_project_selection(projects);
 *  std::cout << "Max profit: " << result.max_profit << "\n";
 *  ```
 *
 *  @ingroup Networks
 */
template <typename Value_Type>
ProjectSelectionResult<Value_Type>
solve_project_selection(const std::vector<Project<Value_Type>>& projects)
{
  using Net = Net_Graph<Net_Node<Empty_Class>,
                        Net_Arc<Empty_Class, Value_Type>>;
  using Node = typename Net::Node;

  ProjectSelectionResult<Value_Type> result;

  if (projects.empty())
    return result;

  Net net;
  const Value_Type INF = std::numeric_limits<Value_Type>::max() / 2;

  // Create source and sink
  Node* source = net.insert_node();
  Node* sink = net.insert_node();

  // Create project nodes
  std::vector<Node*> project_nodes(projects.size());
  for (size_t i = 0; i < projects.size(); ++i)
    project_nodes[i] = net.insert_node();

  // Connect based on profits
  Value_Type sum_positive{0};

  for (size_t i = 0; i < projects.size(); ++i)
    {
      Value_Type p = projects[i].profit;

      // Source to all projects (capacity = profit if positive, 0 otherwise)
      // This ensures all project nodes are not sources
      Value_Type source_cap = (p > 0) ? p : Value_Type{0};
      net.insert_arc(source, project_nodes[i], source_cap);
      if (p > 0)
        sum_positive += p;

      // All projects connect to sink (cost projects with capacity = -profit,
      // others with capacity 0 to ensure proper network structure)
      Value_Type sink_cap = (p < 0) ? -p : Value_Type{0};
      net.insert_arc(project_nodes[i], sink, sink_cap);

      // Add prerequisite edges (infinite capacity)
      for (size_t prereq : projects[i].prerequisites)
        {
          if (prereq < projects.size())
            net.insert_arc(project_nodes[i], project_nodes[prereq], INF);
        }
    }

  // Solve min-cut (via max-flow)
  Value_Type min_cut = dinic_maximum_flow(net);

  result.max_profit = sum_positive - min_cut;

  // Find selected projects (reachable from source in residual graph)
  DynSetTree<Node*> reachable;

  // BFS from source in residual graph
  DynListQueue<Node*> queue;
  queue.put(source);
  reachable.insert(source);

  while (not queue.is_empty())
    {
      Node* u = queue.front();
      queue.get();

      for (typename Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto v = net.get_connected_node(arc, u);

          if (reachable.contains(v))
            continue;

          // Check residual capacity
          Value_Type residual;
          if (net.get_src_node(arc) == u)
            residual = arc->cap - arc->flow;
          else
            residual = arc->flow;

          if (residual > Value_Type{0})
            {
              reachable.insert(v);
              queue.put(v);
            }
        }
    }

  // Projects reachable from source are selected
  for (size_t i = 0; i < projects.size(); ++i)
    {
      if (reachable.contains(project_nodes[i]))
        {
          result.selected.push_back(i);
          if (projects[i].profit > 0)
            result.total_revenue += projects[i].profit;
          else
            result.total_cost += -projects[i].profit;
        }
    }

  return result;
}


//==============================================================================
// BASEBALL ELIMINATION
//==============================================================================

/** @brief Team information for baseball elimination.
 *  @ingroup Networks
 */
struct Team
{
  std::string name;
  int wins;
  int losses;
  int remaining;                      ///< Games remaining
  std::vector<int> against;           ///< Games remaining against each team

  Team(const std::string& n = "", int w = 0, int l = 0, int r = 0)
    : name(n), wins(w), losses(l), remaining(r) {}
};

/** @brief Result of baseball elimination check.
 *  @ingroup Networks
 */
struct BaseballEliminationResult
{
  bool eliminated{false};             ///< Is the team mathematically eliminated?
  std::vector<size_t> certificate;    ///< Teams that form elimination certificate
  int max_possible_wins{0};           ///< Maximum wins team can achieve
};

/** @brief Check if a team is mathematically eliminated from winning.
 *
 *  A team x is eliminated if there is no possible outcome of remaining
 *  games where x finishes first (or tied for first).
 *
 *  ## Algorithm
 *  Construct a flow network:
 *  - For each game between teams i and j (i≠x, j≠x), create game node
 *  - Source connects to game nodes with capacity = games remaining
 *  - Game nodes connect to both team nodes with infinite capacity
 *  - Team nodes connect to sink with capacity = max_wins_x - current_wins_i
 *
 *  Team x is eliminated iff max-flow < total remaining games among others.
 *
 *  @param teams Vector of team information.
 *  @param team_idx Index of team to check.
 *  @return BaseballEliminationResult.
 *
 *  @par Example
 *  ```cpp
 *  std::vector<Team> teams = {
 *    {"Atlanta", 83, 71, 8, {0, 1, 6, 1}},
 *    {"Philly", 80, 79, 3, {1, 0, 0, 2}},
 *    // ...
 *  };
 *
 *  auto result = check_baseball_elimination(teams, 3);  // Check team 3
 *  if (result.eliminated)
 *    std::cout << "Team is eliminated!\n";
 *  ```
 *
 *  @ingroup Networks
 */
inline BaseballEliminationResult
check_baseball_elimination(const std::vector<Team>& teams, size_t team_idx)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, int>>;
  using Node = Net::Node;

  BaseballEliminationResult result;

  if (team_idx >= teams.size())
    return result;

  const Team& x = teams[team_idx];
  result.max_possible_wins = x.wins + x.remaining;

  // Trivial elimination check
  for (size_t i = 0; i < teams.size(); ++i)
    {
      if (i != team_idx and teams[i].wins > result.max_possible_wins)
        {
          result.eliminated = true;
          result.certificate.push_back(i);
          return result;
        }
    }

  Net net;
  const int INF = 1000000;

  Node* source = net.insert_node();
  Node* sink = net.insert_node();

  // Create team nodes (excluding team x)
  std::vector<Node*> team_nodes(teams.size(), nullptr);
  for (size_t i = 0; i < teams.size(); ++i)
    {
      if (i != team_idx)
        {
          team_nodes[i] = net.insert_node();

          // Team to sink: capacity = max wins x can have - current wins of i
          int cap = result.max_possible_wins - teams[i].wins;
          if (cap > 0)
            net.insert_arc(team_nodes[i], sink, cap);
        }
    }

  // Create game nodes
  int total_games = 0;
  for (size_t i = 0; i < teams.size(); ++i)
    {
      if (i == team_idx)
        continue;

      for (size_t j = i + 1; j < teams.size(); ++j)
        {
          if (j == team_idx)
            continue;

          int games = teams[i].against[j];
          if (games > 0)
            {
              total_games += games;

              Node* game_node = net.insert_node();
              net.insert_arc(source, game_node, games);
              net.insert_arc(game_node, team_nodes[i], INF);
              net.insert_arc(game_node, team_nodes[j], INF);
            }
        }
    }

  // Ensure network has single source and sink
  net.make_super_source();
  net.make_super_sink();

  int max_flow = dinic_maximum_flow(net);

  result.eliminated = (max_flow < total_games);

  // If eliminated, find certificate (teams in source side of min-cut)
  if (result.eliminated)
    {
      // BFS from source in residual graph
      DynSetTree<Node*> reachable;
      DynListQueue<Node*> queue;
      queue.put(source);
      reachable.insert(source);

      while (not queue.is_empty())
        {
          Node* u = queue.front();
          queue.get();

          for (Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_curr();
              auto v = net.get_connected_node(arc, u);

              if (reachable.contains(v))
                continue;

              int residual = (net.get_src_node(arc) == u) ?
                  (arc->cap - arc->flow) : arc->flow;

              if (residual > 0)
                {
                  reachable.insert(v);
                  queue.put(v);
                }
            }
        }

      for (size_t i = 0; i < teams.size(); ++i)
        if (i != team_idx and team_nodes[i] != nullptr and
            reachable.contains(team_nodes[i]))
          result.certificate.push_back(i);
    }

  return result;
}


//==============================================================================
// IMAGE SEGMENTATION (BINARY LABELING)
//==============================================================================

/** @brief Result of binary image segmentation.
 *  @ingroup Networks
 */
struct SegmentationResult
{
  std::vector<std::vector<int>> labels;  ///< 0 or 1 for each pixel
  double energy{0};                       ///< Total energy of segmentation
};

/** @brief Segment image using graph cuts.
 *
 *  Solves the binary labeling problem for image segmentation:
 *  - Each pixel can be labeled 0 (background) or 1 (foreground)
 *  - Data term: cost of assigning each label to each pixel
 *  - Smoothness term: cost of adjacent pixels having different labels
 *
 *  The energy to minimize is:
 *  E(L) = Σ D_p(L_p) + Σ V_{p,q}(L_p, L_q)
 *
 *  where D is data cost and V is smoothness cost.
 *
 *  @tparam Value_Type Cost type.
 *  @param rows Image height.
 *  @param cols Image width.
 *  @param data_cost data_cost[i][j][k] = cost of pixel (i,j) having label k.
 *  @param smoothness Cost of adjacent pixels having different labels.
 *  @return SegmentationResult with optimal labeling.
 *
 *  @par Example
 *  ```cpp
 *  // 3x3 image with data costs
 *  std::vector<std::vector<std::array<double,2>>> data(3,
 *    std::vector<std::array<double,2>>(3));
 *
 *  // Set data costs based on pixel intensities
 *  for (int i = 0; i < 3; ++i)
 *    for (int j = 0; j < 3; ++j) {
 *      data[i][j][0] = intensity[i][j];        // Cost of background
 *      data[i][j][1] = 255 - intensity[i][j];  // Cost of foreground
 *    }
 *
 *  auto result = segment_image(3, 3, data, 10.0);
 *  ```
 *
 *  @ingroup Networks
 */
template <typename Value_Type>
SegmentationResult segment_image(
    size_t rows, size_t cols,
    const std::vector<std::vector<std::array<Value_Type, 2>>>& data_cost,
    Value_Type smoothness)
{
  using Net = Net_Graph<Net_Node<Empty_Class>,
                        Net_Arc<Empty_Class, Value_Type>>;
  using Node = typename Net::Node;

  SegmentationResult result;
  result.labels.resize(rows, std::vector<int>(cols, 0));

  if (rows == 0 or cols == 0)
    return result;

  Net net;

  // Create source (label 0) and sink (label 1)
  Node* source = net.insert_node();
  Node* sink = net.insert_node();

  // Create pixel nodes
  std::vector<std::vector<Node*>> pixels(rows, std::vector<Node*>(cols));
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      pixels[i][j] = net.insert_node();

  // Add data term edges (standard graph cut model for binary labeling)
  // See: Boykov, Kolmogorov "An Experimental Comparison of Min-Cut/Max-Flow Algorithms"
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      {
        // Source to pixel: capacity = cost of background (label 0)
        // If this arc is cut, pixel goes to sink side = background
        net.insert_arc(source, pixels[i][j], data_cost[i][j][0]);

        // Pixel to sink: capacity = cost of foreground (label 1)
        // If this arc is cut, pixel stays on source side = foreground
        net.insert_arc(pixels[i][j], sink, data_cost[i][j][1]);
      }

  // Add smoothness term edges (4-connected)
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      {
        // Right neighbor
        if (j + 1 < cols)
          {
            net.insert_arc(pixels[i][j], pixels[i][j+1], smoothness);
            net.insert_arc(pixels[i][j+1], pixels[i][j], smoothness);
          }

        // Bottom neighbor
        if (i + 1 < rows)
          {
            net.insert_arc(pixels[i][j], pixels[i+1][j], smoothness);
            net.insert_arc(pixels[i+1][j], pixels[i][j], smoothness);
          }
      }

  // Ensure network has single source and sink
  net.make_super_source();
  net.make_super_sink();

  // Compute min-cut
  Value_Type min_cut = dinic_maximum_flow(net);
  result.energy = static_cast<double>(min_cut);

  // Extract labels (BFS from source in residual graph)
  DynSetTree<Node*> foreground;  // Reachable = foreground (label 1)

  DynListQueue<Node*> queue;
  queue.put(source);
  foreground.insert(source);

  while (not queue.is_empty())
    {
      Node* u = queue.front();
      queue.get();

      for (typename Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto v = net.get_connected_node(arc, u);

          if (foreground.contains(v))
            continue;

          Value_Type residual = (net.get_src_node(arc) == u) ?
              (arc->cap - arc->flow) : arc->flow;

          if (residual > Value_Type{0})
            {
              foreground.insert(v);
              queue.put(v);
            }
        }
    }

  // Set labels
  for (size_t i = 0; i < rows; ++i)
    for (size_t j = 0; j < cols; ++j)
      result.labels[i][j] = foreground.contains(pixels[i][j]) ? 1 : 0;

  return result;
}


//==============================================================================
// SURVEY DESIGN
//==============================================================================

/** @brief Survey question with constraints.
 *  @ingroup Networks
 */
struct SurveyQuestion
{
  size_t id;
  int min_responses;   ///< Minimum number of responses needed
  int max_responses;   ///< Maximum responses accepted
};

/** @brief Survey respondent with constraints.
 *  @ingroup Networks
 */
struct SurveyRespondent
{
  size_t id;
  int min_questions;   ///< Minimum questions to answer
  int max_questions;   ///< Maximum questions can answer
  std::vector<size_t> eligible_questions;  ///< Questions this respondent can answer
};

/** @brief Result of survey design.
 *  @ingroup Networks
 */
struct SurveyDesignResult
{
  bool feasible{false};
  std::vector<std::pair<size_t, size_t>> assignments;  ///< (respondent, question)
};

/** @brief Design survey assignment using network flow.
 *
 *  Assign respondents to questions such that:
 *  - Each question gets [min, max] responses
 *  - Each respondent answers [min, max] questions
 *  - Respondents only answer eligible questions
 *
 *  @param questions Vector of questions with constraints.
 *  @param respondents Vector of respondents with constraints and eligibility.
 *  @return SurveyDesignResult with feasibility and assignments.
 *
 *  @ingroup Networks
 */
inline SurveyDesignResult
design_survey(const std::vector<SurveyQuestion>& questions,
              const std::vector<SurveyRespondent>& respondents)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, int>>;
  using Node = Net::Node;

  SurveyDesignResult result;

  if (questions.empty() or respondents.empty())
    return result;

  Net net;

  Node* source = net.insert_node();
  Node* sink = net.insert_node();

  // Create question nodes
  std::vector<Node*> question_nodes(questions.size());
  for (size_t i = 0; i < questions.size(); ++i)
    {
      question_nodes[i] = net.insert_node();

      // Question to sink: [min, max] responses
      // For feasibility check, use max capacity
      net.insert_arc(question_nodes[i], sink, questions[i].max_responses);
    }

  // Create respondent nodes
  std::vector<Node*> respondent_nodes(respondents.size());
  for (size_t i = 0; i < respondents.size(); ++i)
    {
      respondent_nodes[i] = net.insert_node();

      // Source to respondent: [min, max] questions
      net.insert_arc(source, respondent_nodes[i], respondents[i].max_questions);

      // Respondent to eligible questions
      for (size_t q : respondents[i].eligible_questions)
        if (q < questions.size())
          net.insert_arc(respondent_nodes[i], question_nodes[q], 1);
    }

  // Ensure network has single source and sink
  net.make_super_source();
  net.make_super_sink();

  // Compute max flow
  dinic_maximum_flow(net);

  // Check if minimum constraints are satisfied
  result.feasible = true;

  for (size_t i = 0; i < questions.size(); ++i)
    {
      int responses = 0;
      for (Net::Node_Arc_Iterator it(question_nodes[i]); it.has_curr();
           it.next_ne())
        {
          auto arc = it.get_curr();
          if (net.get_tgt_node(arc) == sink)
            responses = arc->flow;
        }

      if (responses < questions[i].min_responses)
        {
          result.feasible = false;
          break;
        }
    }

  // Extract assignments
  if (result.feasible)
    {
      for (size_t r = 0; r < respondents.size(); ++r)
        {
          for (Net::Node_Arc_Iterator it(respondent_nodes[r]);
               it.has_curr(); it.next_ne())
            {
              auto arc = it.get_curr();
              if (arc->flow > 0)
                {
                  // Find which question this is
                  auto tgt = net.get_tgt_node(arc);
                  for (size_t q = 0; q < questions.size(); ++q)
                    if (question_nodes[q] == tgt)
                      {
                        result.assignments.push_back({r, q});
                        break;
                      }
                }
            }
        }
    }

  return result;
}

} // namespace Aleph

#endif // NET_APPS_H
