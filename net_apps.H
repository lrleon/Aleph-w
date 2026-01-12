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

namespace Aleph
{

//==============================================================================
// CIRCULATION WITH DEMANDS
//==============================================================================

/** @brief Result of circulation problem.
 *  @ingroup Networks
 */
template <typename Flow_Type>
struct CirculationResult
{
  bool feasible{false};              ///< Is there a feasible circulation?
  Flow_Type excess_flow{0};          ///< Flow needed to satisfy demands
  DynMapTree<void*, Flow_Type> flow; ///< Flow on each edge (arc pointer -> flow)
};

/** @brief Solve circulation problem with demands.
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
 *  @param get_lower Returns lower bound for each arc.
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

  // Calculate total positive and negative demands
  Flow_Type total_demand{0};  // sum of positive demands

  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Flow_Type d = get_demand(it.get_curr());
      if (d > 0)
        total_demand += d;
    }

  if (total_demand == Flow_Type{0})
    {
      result.feasible = true;
      return result;
    }

  // Create super-source and super-sink
  Node* super_source = net.insert_node();
  Node* super_sink = net.insert_node();

  // Add edges from super-source/to super-sink based on demands
  Flow_Type total_lower{0};

  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Node* v = it.get_curr();
      if (v == super_source or v == super_sink)
        continue;

      // Adjust for lower bounds: sum of lower bounds on incoming/outgoing edges
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

      Flow_Type d = get_demand(v) + lower_in - lower_out;

      if (d > 0)
        net.insert_arc(super_source, v, d);
      else if (d < 0)
        net.insert_arc(v, super_sink, -d);

      total_lower += lower_out;  // Each lower bound counted once
    }

  // Adjust arc capacities for lower bounds
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* src = net.get_src_node(arc);
      Node* tgt = net.get_tgt_node(arc);

      // Skip super-source/sink arcs
      if (src == super_source or tgt == super_sink)
        continue;

      Flow_Type lb = get_lower(arc);
      arc->cap -= lb;  // Reduce capacity by lower bound
    }

  // Ensure network has single source and sink for max-flow
  net.make_super_source();
  net.make_super_sink();

  // Solve max-flow
  Flow_Type max_flow = Maxflow<Net>()(net);

  result.feasible = (max_flow == total_demand);
  result.excess_flow = max_flow;

  // Extract flow values (add back lower bounds)
  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* src = net.get_src_node(arc);
      Node* tgt = net.get_tgt_node(arc);

      if (src == super_source or tgt == super_sink)
        continue;

      Flow_Type lb = get_lower(arc);
      result.flow[arc] = arc->flow + lb;
    }

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
  std::queue<Node*> queue;
  queue.push(source);
  reachable.insert(source);

  while (not queue.empty())
    {
      Node* u = queue.front();
      queue.pop();

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
              queue.push(v);
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
      std::queue<Node*> queue;
      queue.push(source);
      reachable.insert(source);

      while (not queue.empty())
        {
          Node* u = queue.front();
          queue.pop();

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
                  queue.push(v);
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

  std::queue<Node*> queue;
  queue.push(source);
  foreground.insert(source);

  while (not queue.empty())
    {
      Node* u = queue.front();
      queue.pop();

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
              queue.push(v);
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
