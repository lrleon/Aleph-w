
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

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>
#include <string>
#include <functional>
#include <tpl_net.H>
#include <tpl_maxflow.H>
#include <tpl_netcost.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynListQueue.H>

namespace Aleph {

//==============================================================================
// CIRCULATION WITH DEMANDS
//==============================================================================

/** @brief Result of a circulation problem.
 *  @ingroup Networks
 */
template <typename Flow_Type>
struct CirculationResult
{
  bool feasible{false};                ///< Is there a feasible circulation?
  Flow_Type excess_flow{0};            ///< Flow needed to satisfy demands
  DynMapTree<void *, Flow_Type> flow;  ///< Flow on each edge (arc pointer -> flow)
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
 *     - Take the lower bounds as a mandatory flow, which adjusts the
 *       demands: d'(v) = d(v) - l_in(v) + l_out(v)
 *     - Create super-source connected to nodes with negative demand
 *     - Create super-sink connected from nodes with positive demand
 *     - Adjust capacities for lower bounds
 *  2. Solve max-flow
 *  3. Check if flow saturates all super-source/sink edges
 *
 *  There is no circulation if a lower bound is negative or above the
 *  capacity of its arc, or if the demands do not add up to 0. With a
 *  floating flow, an adjusted demand within the rounding of its sum of
 *  the demand and the lower bounds of its node, an epsilon of the sum of
 *  their magnitudes by each of them and two more, is 0, so that bounds
 *  that balance but for rounding are a circulation.
 *
 *  @tparam Net Network type.
 *  @tparam GetDemand Functor returning node demand.
 *  @tparam GetLower Functor returning edge lower bound.
 *  @tparam Maxflow Max-flow algorithm to use.
 *  @param net Network (modified during computation).
 *  @param get_demand Returns demand for each node.
 *  @param get_lower Returns the lower bound for each arc.
 *  @return CirculationResult with feasibility and flow values: the flow of
 *          the circulation, if there is one, else the lower bounds.
 *  @throw std::bad_alloc if there is not enough memory. Then the network is
 *         as it was, but for its flows, which are 0, unless the removal of
 *         the super nodes, which allocates, fails too.
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
template <class Net, class GetDemand, class GetLower, template <class> class Maxflow = Dinic_Maximum_Flow>
CirculationResult<typename Net::Flow_Type> solve_circulation(Net &net, GetDemand get_demand,
                                                             GetLower get_lower)
{
  using Node = typename Net::Node;
  using Arc = typename Net::Arc;
  using Flow_Type = typename Net::Flow_Type;

  // Whether two totals are the same: exactly, or, for a floating flow,
  // but for rounding
  auto same = [](const Flow_Type &a, const Flow_Type &b)
    {
      if constexpr (std::is_floating_point_v<Flow_Type>)
        return std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) * 1e-9;
      else
        return a == b;
    };

  CirculationResult<Flow_Type> result;

  // The nodes of the network, before the super nodes, and its arcs, with
  // their capacities and lower bounds; the flows start at the bounds
  DynList<Node *> original_nodes;
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    original_nodes.append(it.get_curr());

  struct Arc_Bounds
  {
    Arc *arc;
    Flow_Type cap;
    Flow_Type lower;
  };
  DynList<Arc_Bounds> original_arcs;
  bool bounded = true;  // whether 0 <= l(e) <= u(e) on every arc
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc *arc = it.get_curr();
      const Flow_Type lb = get_lower(arc);
      bounded = bounded and lb >= Flow_Type{0} and lb <= arc->cap;
      original_arcs.append(Arc_Bounds{arc, arc->cap, lb});
      result.flow[arc] = lb;
    }

  // An arc u -> v carries its lower bound l at least, which v receives
  // and u sends: what is left is a flow f' = f - l, within u(e) - l, where
  // in'(v) - out'(v) = d'(v) = d(v) - l_in(v) + l_out(v). For a floating
  // flow, an adjusted demand within the rounding of the numbers that
  // make it, the demand and the lower bounds of the arcs of v, is 0:
  // lower bounds that balance but for rounding are a circulation. The
  // capacities do not make d'(v), and a large one beside small bounds
  // would take a real demand for a residue.
  DynMapTree<Node *, Flow_Type> adjusted;
  Flow_Type need{0}, give{0};  // what the nodes need, and what they give
  for (Node *v : original_nodes)
    {
      Flow_Type d = get_demand(v);
      [[maybe_unused]] Flow_Type scale = d < Flow_Type{0} ? -d : d;
      [[maybe_unused]] size_t terms = 1;
      for (typename Net::Node_Arc_Iterator ait(v); ait.has_curr(); ait.next_ne())
        {
          Arc *arc = ait.get_curr();
          const Flow_Type lb = get_lower(arc);
          if (net.get_tgt_node(arc) == v)
            d -= lb;
          if (net.get_src_node(arc) == v)
            d += lb;
          if constexpr (std::is_floating_point_v<Flow_Type>)
            {
              scale += lb < Flow_Type{0} ? -lb : lb;
              ++terms;
            }
        }
      if constexpr (std::is_floating_point_v<Flow_Type>)
        if (std::abs(d) <= std::numeric_limits<Flow_Type>::epsilon() * Flow_Type(terms + 2) * scale)
          d = Flow_Type{0};
      adjusted[v] = d;
      if (d > Flow_Type{0})
        need += d;
      else
        give -= d;
    }

  // No flow fits bounds out of order, nor demands whose total is not 0;
  // without demands left, the lower bounds are the circulation
  if (not bounded or not same(need, give))
    return result;
  if (need == Flow_Type{0})
    {
      result.feasible = true;
      return result;
    }

  // The network as it was, but for its flows, which are 0; the super
  // nodes go, with their arcs, outside a destructor, as a removal
  // allocates in a network
  Node *super_source = nullptr;
  Node *super_sink = nullptr;
  auto restore = [&]
    {
      for (auto it = original_arcs.get_it(); it.has_curr(); it.next_ne())
        {
          const Arc_Bounds &b = it.get_curr();
          b.arc->cap = b.cap;
          b.arc->flow = Flow_Type{0};
        }
      if (super_sink != nullptr)
        {
          net.remove_node(super_sink);
          super_sink = nullptr;
        }
      if (super_source != nullptr)
        {
          net.remove_node(super_source);
          super_source = nullptr;
        }
    };

  try
    {
      super_source = net.insert_node();
      super_sink = net.insert_node();

      // A node that needs flow gets it from the super-sink side, and one
      // that gives it, from the super-source
      for (Node *v : original_nodes)
        if (const Flow_Type d = adjusted[v]; d > Flow_Type{0})
          net.insert_arc(v, super_sink, d);
        else if (d < Flow_Type{0})
          net.insert_arc(super_source, v, -d);

      // The other sources and sinks, through arcs of capacity 0, so that
      // the network has one source and one sink
      DynList<Node *> sources, sinks;
      for (auto it = net.get_src_nodes().get_it(); it.has_curr(); it.next_ne())
        if (it.get_curr() != super_source)
          sources.append(it.get_curr());
      for (auto it = net.get_sink_nodes().get_it(); it.has_curr(); it.next_ne())
        if (it.get_curr() != super_sink)
          sinks.append(it.get_curr());
      for (Node *v : sources)
        net.insert_arc(super_source, v, Flow_Type{0});
      for (Node *v : sinks)
        net.insert_arc(v, super_sink, Flow_Type{0});

      for (auto it = original_arcs.get_it(); it.has_curr(); it.next_ne())
        {
          const Arc_Bounds &b = it.get_curr();
          b.arc->cap = b.cap - b.lower;
          b.arc->flow = Flow_Type{0};
        }

      const Flow_Type got = Maxflow<Net>()(net);
      result.excess_flow = got;
      result.feasible = same(got, need);
      if (result.feasible)
        for (auto it = original_arcs.get_it(); it.has_curr(); it.next_ne())
          {
            const Arc_Bounds &b = it.get_curr();
            result.flow[b.arc] = b.arc->flow + b.lower;
          }
    }
  catch (...)
    {
      // If the removal fails too, the super nodes stay, and the first
      // exception goes on
      try
        {
          restore();
        }
      catch (...)
        {
          // empty
        }
      throw;
    }

  restore();
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
  size_t id;                          ///< Unique project ID
  Value_Type profit;                  ///< Profit (positive) or cost (negative)
  std::vector<size_t> prerequisites;  ///< IDs of prerequisite projects
  std::string name;                   ///< Optional name

  Project(size_t id_, Value_Type profit_, const std::vector<size_t> &prereqs = {},
          const std::string &name_ = "")
    : id(id_), profit(profit_), prerequisites(prereqs), name(name_)
  {}
};

/** @brief Result of project selection.
 *  @ingroup Networks
 */
template <typename Value_Type>
struct ProjectSelectionResult
{
  Value_Type max_profit{0};      ///< Maximum achievable profit
  std::vector<size_t> selected;  ///< IDs of selected projects
  Value_Type total_revenue{0};   ///< Sum of positive profits
  Value_Type total_cost{0};      ///< Sum of negative profits (costs)
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
ProjectSelectionResult<Value_Type> solve_project_selection(const std::vector<Project<Value_Type>> &projects)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, Value_Type>>;
  using Node = typename Net::Node;

  ProjectSelectionResult<Value_Type> result;

  if (projects.empty())
    return result;

  Net net;
  const Value_Type INF = std::numeric_limits<Value_Type>::max() / 2;

  // Create source and sink
  Node *source = net.insert_node();
  Node *sink = net.insert_node();

  // Create project nodes
  std::vector<Node *> project_nodes(projects.size());
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
        if (prereq < projects.size())
          net.insert_arc(project_nodes[i], project_nodes[prereq], INF);
    }

  // Solve min-cut (via max-flow)
  Value_Type min_cut = dinic_maximum_flow(net);

  result.max_profit = sum_positive - min_cut;

  // Find selected projects (reachable from source in residual graph)
  DynSetTree<Node *> reachable;

  // BFS from source in residual graph
  DynListQueue<Node *> queue;
  queue.put(source);
  reachable.insert(source);

  while (not queue.is_empty())
    {
      Node *u = queue.front();
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
    if (reachable.contains(project_nodes[i]))
      {
        result.selected.push_back(i);
        if (projects[i].profit > 0)
          result.total_revenue += projects[i].profit;
        else
          result.total_cost += -projects[i].profit;
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
  int remaining;             ///< Games remaining
  std::vector<int> against;  ///< Games remaining against each team

  Team(const std::string &n = "", int w = 0, int l = 0, int r = 0)
    : name(n), wins(w), losses(l), remaining(r)
  {}
};

/** @brief Result of baseball elimination check.
 *  @ingroup Networks
 */
struct BaseballEliminationResult
{
  bool eliminated{false};           ///< Is the team mathematically eliminated?
  std::vector<size_t> certificate;  ///< Teams that form elimination certificate
  int max_possible_wins{0};         ///< Maximum wins team can achieve
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
inline BaseballEliminationResult check_baseball_elimination(const std::vector<Team> &teams,
                                                            size_t team_idx)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, int>>;
  using Node = Net::Node;

  BaseballEliminationResult result;

  if (team_idx >= teams.size())
    return result;

  const Team &x = teams[team_idx];
  result.max_possible_wins = x.wins + x.remaining;

  // Trivial elimination check
  for (size_t i = 0; i < teams.size(); ++i)
    if (i != team_idx and teams[i].wins > result.max_possible_wins)
      {
        result.eliminated = true;
        result.certificate.push_back(i);
        return result;
      }

  Net net;

  Node *source = net.insert_node();
  Node *sink = net.insert_node();

  // Create team nodes (excluding team x)
  std::vector<Node *> team_nodes(teams.size(), nullptr);
  for (size_t i = 0; i < teams.size(); ++i)
    if (i != team_idx)
      {
        team_nodes[i] = net.insert_node();

        // Team to sink: capacity = max wins x can have - current wins of
        // i, which is 0 for a team without room: the arc stays, so that
        // the team is not a sink
        net.insert_arc(team_nodes[i], sink, result.max_possible_wins - teams[i].wins);
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
              constexpr int INF = 1000000;
              total_games += games;

              Node *game_node = net.insert_node();
              net.insert_arc(source, game_node, games);
              net.insert_arc(game_node, team_nodes[i], INF);
              net.insert_arc(game_node, team_nodes[j], INF);
            }
        }
    }

  // Without games among the others, x is not eliminated
  if (total_games == 0)
    return result;

  // A team without games gets none from the source, by an arc of capacity
  // 0, so that the network has one source: make_super_source() would feed
  // it with the capacity of its arc to the sink
  for (Node *p : team_nodes)
    if (p != nullptr and net.get_in_degree(p) == 0)
      net.insert_arc(source, p, 0);

  const int max_flow = dinic_maximum_flow(net);

  result.eliminated = (max_flow < total_games);

  // If eliminated, find certificate (teams in source side of min-cut)
  if (result.eliminated)
    {
      // BFS from source in residual graph
      DynSetTree<Node *> reachable;
      DynListQueue<Node *> queue;
      queue.put(source);
      reachable.insert(source);

      while (not queue.is_empty())
        {
          Node *u = queue.front();
          queue.get();

          for (Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
            {
              const auto arc = it.get_curr();
              auto v = net.get_connected_node(arc, u);

              if (reachable.contains(v))
                continue;

              if (const int residual =
                    (net.get_src_node(arc) == u) ? (arc->cap - arc->flow) : arc->flow;
                  residual > 0)
                {
                  reachable.insert(v);
                  queue.put(v);
                }
            }
        }

      for (size_t i = 0; i < teams.size(); ++i)
        if (i != team_idx and team_nodes[i] != nullptr and reachable.contains(team_nodes[i]))
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
  double energy{0};                      ///< Total energy of segmentation
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
SegmentationResult segment_image(size_t rows, size_t cols,
                                 const std::vector<std::vector<std::array<Value_Type, 2>>> &data_cost,
                                 Value_Type smoothness)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, Value_Type>>;
  using Node = typename Net::Node;

  SegmentationResult result;
  result.labels.resize(rows, std::vector<int>(cols, 0));

  if (rows == 0 or cols == 0)
    return result;

  Net net;

  // Create source (label 0) and sink (label 1)
  Node *source = net.insert_node();
  Node *sink = net.insert_node();

  // Create pixel nodes
  std::vector<std::vector<Node *>> pixels(rows, std::vector<Node *>(cols));
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
            net.insert_arc(pixels[i][j], pixels[i][j + 1], smoothness);
            net.insert_arc(pixels[i][j + 1], pixels[i][j], smoothness);
          }

        // Bottom neighbor
        if (i + 1 < rows)
          {
            net.insert_arc(pixels[i][j], pixels[i + 1][j], smoothness);
            net.insert_arc(pixels[i + 1][j], pixels[i][j], smoothness);
          }
      }

  // Ensure network has single source and sink
  net.make_super_source();
  net.make_super_sink();

  // Compute min-cut
  Value_Type min_cut = dinic_maximum_flow(net);
  result.energy = static_cast<double>(min_cut);

  // Extract labels (BFS from source in residual graph)
  DynSetTree<Node *> foreground;  // Reachable = foreground (label 1)

  DynListQueue<Node *> queue;
  queue.put(source);
  foreground.insert(source);

  while (not queue.is_empty())
    {
      Node *u = queue.front();
      queue.get();

      for (typename Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto v = net.get_connected_node(arc, u);

          if (foreground.contains(v))
            continue;

          Value_Type residual = (net.get_src_node(arc) == u) ? (arc->cap - arc->flow) : arc->flow;

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
  int min_responses;  ///< Minimum number of responses needed
  int max_responses;  ///< Maximum responses accepted
};

/** @brief Survey respondent with constraints.
 *  @ingroup Networks
 */
struct SurveyRespondent
{
  size_t id;
  int min_questions;                       ///< Minimum questions to answer
  int max_questions;                       ///< Maximum questions can answer
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
 *  The minimums are the lower bounds of a circulation, which
 *  solve_circulation() finds. There is none if a minimum is negative or
 *  above its maximum.
 *
 *  @param questions Vector of questions with constraints.
 *  @param respondents Vector of respondents with constraints and eligibility.
 *  @return SurveyDesignResult with feasibility and assignments.
 *
 *  @ingroup Networks
 */
inline SurveyDesignResult design_survey(const std::vector<SurveyQuestion> &questions,
                                        const std::vector<SurveyRespondent> &respondents)
{
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, int>>;
  using Node = Net::Node;
  using Arc = Net::Arc;

  SurveyDesignResult result;

  if (questions.empty() or respondents.empty())
    return result;

  // The bounds must be in order
  for (const auto &q : questions)
    if (q.min_responses < 0 or q.min_responses > q.max_responses)
      return result;
  for (const auto &r : respondents)
    if (r.min_questions < 0 or r.min_questions > r.max_questions)
      return result;

  // A circulation, with the minimums as lower bounds: from the source to
  // each respondent, to the questions it may answer, to the sink, and
  // back to the source
  Net net;
  DynMapTree<Arc *, int> lower;

  Node *source = net.insert_node();
  Node *sink = net.insert_node();

  std::vector<Node *> question_nodes(questions.size());
  for (size_t i = 0; i < questions.size(); ++i)
    {
      question_nodes[i] = net.insert_node();
      Arc *a = net.insert_arc(question_nodes[i], sink, questions[i].max_responses);
      lower.insert(a, questions[i].min_responses);
    }

  int most = 0;  // the most answers there may be
  std::vector<std::vector<std::pair<size_t, Arc *>>> answers(respondents.size());
  for (size_t i = 0; i < respondents.size(); ++i)
    {
      Node *r = net.insert_node();
      Arc *a = net.insert_arc(source, r, respondents[i].max_questions);
      lower.insert(a, respondents[i].min_questions);
      most += respondents[i].max_questions;

      for (size_t q : respondents[i].eligible_questions)
        if (q < questions.size())
          answers[i].push_back({q, net.insert_arc(r, question_nodes[q], 1)});
    }

  net.insert_arc(sink, source, most);

  auto circulation = solve_circulation(net,
                                       [](Node *) { return 0; },
                                       [&lower](Arc *a)
                                         {
                                           auto p = lower.search(a);
                                           return p == nullptr ? 0 : p->second;
                                         });

  result.feasible = circulation.feasible;
  if (result.feasible)
    for (size_t r = 0; r < respondents.size(); ++r)
      for (const auto &[q, a] : answers[r])
        if (circulation.flow.find(a) > 0)
          result.assignments.push_back({r, q});

  return result;
}

}  // namespace Aleph

#endif  // NET_APPS_H
