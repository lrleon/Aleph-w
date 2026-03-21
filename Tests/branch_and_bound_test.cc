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

#include <string>

#include <gtest/gtest.h>

#include <Knapsack.H>
#include <State_Search.H>

using namespace Aleph;

namespace
{

struct ArtificialState
{
  size_t depth = 0;
  int code = 1;
  int value = 0;
};

struct ArtificialMove
{
  int target_code = 1;
  int delta = 0;
  char label = 'L';
};

struct ArtificialMaxDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 9;
      case 2: return 9;
      case 3: return 5;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
    state.code /= 2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= 2)
      return true;

    switch (state.code)
      {
      case 1:
        if (not visit(Move{2, 0, 'L'}))
          return false;
        return visit(Move{3, 0, 'R'});

      case 2:
        if (not visit(Move{4, 7, 'L'}))
          return false;
        return visit(Move{5, 9, 'R'});

      case 3:
        if (not visit(Move{6, 5, 'L'}))
          return false;
        return visit(Move{7, 4, 'R'});

      default:
        return true;
      }
  }
};

struct ArtificialMaxBadOrderDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 9;
      case 2: return 9;
      case 3: return 5;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
    state.code /= 2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= 2)
      return true;

    switch (state.code)
      {
      case 1:
        if (not visit(Move{3, 0, 'R'}))
          return false;
        return visit(Move{2, 0, 'L'});

      case 2:
        if (not visit(Move{4, 7, 'L'}))
          return false;
        return visit(Move{5, 9, 'R'});

      case 3:
        if (not visit(Move{6, 5, 'L'}))
          return false;
        return visit(Move{7, 4, 'R'});

      default:
        return true;
      }
  }
};

struct ArtificialMinDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 6;
      case 2: return 6;
      case 3: return 7;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
    state.code /= 2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= 2)
      return true;

    switch (state.code)
      {
      case 1:
        if (not visit(Move{2, 0, 'L'}))
          return false;
        return visit(Move{3, 0, 'R'});

      case 2:
        if (not visit(Move{4, 8, 'L'}))
          return false;
        return visit(Move{5, 6, 'R'});

      case 3:
        if (not visit(Move{6, 7, 'L'}))
          return false;
        return visit(Move{7, 9, 'R'});

      default:
        return true;
      }
  }
};

struct KnapsackState
{
  size_t index = 0;
  int weight = 0;
  double value = 0;
  Array<unsigned char> chosen;

  explicit KnapsackState(const size_t n = 0)
    : index(0), weight(0), value(0), chosen(n, 0)
  {
    // empty
  }
};

class KnapsackBBDomain
{
public:
  using State = KnapsackState;

  struct Move
  {
    bool take = false;
  };

  using Objective = double;

  KnapsackBBDomain(const Array<Knapsack_Item<int, double>> &items,
                   const int capacity,
                   const bool fractional_bound)
    : items_(items),
      suffix_values_(items.size() + 1, 0.0),
      capacity_(capacity),
      use_fractional_bound_(fractional_bound)
  {
    for (size_t i = items_.size(); i > 0; --i)
      suffix_values_[i - 1] = suffix_values_[i] + items_[i - 1].value;
  }

  bool is_complete(const State &state) const
  {
    return state.index == items_.size();
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    if (not use_fractional_bound_)
      return state.value + suffix_values_[state.index];

    Objective optimistic = state.value;
    int remaining = capacity_ - state.weight;

    for (size_t i = state.index; i < items_.size() and remaining > 0; ++i)
      if (items_[i].weight <= remaining)
        {
          optimistic += items_[i].value;
          remaining -= items_[i].weight;
        }
      else
        {
          optimistic += items_[i].value * (static_cast<Objective>(remaining)/
                                           static_cast<Objective>(items_[i].weight));
          break;
        }

    return optimistic;
  }

  void apply(State &state, const Move &move) const
  {
    if (move.take)
      {
        state.weight += items_[state.index].weight;
        state.value += items_[state.index].value;
        state.chosen[state.index] = 1;
      }
    else
      state.chosen[state.index] = 0;

    ++state.index;
  }

  void undo(State &state, const Move &move) const
  {
    --state.index;
    if (move.take)
      {
        state.weight -= items_[state.index].weight;
        state.value -= items_[state.index].value;
      }

    state.chosen[state.index] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.index >= items_.size())
      return true;

    if (state.weight + items_[state.index].weight <= capacity_)
      if (not visit(Move{true}))
        return false;

    return visit(Move{false});
  }

private:
  Array<Knapsack_Item<int, double>> items_;
  Array<double> suffix_values_;
  int capacity_ = 0;
  bool use_fractional_bound_ = true;
};

struct AssignmentState
{
  size_t row = 0;
  int total_cost = 0;
  Array<unsigned char> used_columns;
  Array<int> assigned_column;

  explicit AssignmentState(const size_t n = 0)
    : row(0), total_cost(0), used_columns(n, 0), assigned_column(n, -1)
  {
    // empty
  }
};

class AssignmentBBDomain
{
public:
  struct Move
  {
    size_t col = 0;
  };

  using State = AssignmentState;
  using Objective = int;

  explicit AssignmentBBDomain(const Array<Array<int>> &costs)
    : costs_(costs)
  {
    // empty
  }

  bool is_complete(const State &state) const
  {
    return state.row == costs_.size();
  }

  Objective objective_value(const State &state) const
  {
    return state.total_cost;
  }

  Objective bound(const State &state) const
  {
    Objective optimistic = state.total_cost;

    for (size_t row = state.row; row < costs_.size(); ++row)
      {
        int best = -1;
        for (size_t col = 0; col < costs_[row].size(); ++col)
          if (not state.used_columns[col] and (best < 0 or costs_[row][col] < best))
            best = costs_[row][col];

        optimistic += best;
      }

    return optimistic;
  }

  void apply(State &state, const Move &move) const
  {
    state.total_cost += costs_[state.row][move.col];
    state.used_columns[move.col] = 1;
    state.assigned_column[state.row] = static_cast<int>(move.col);
    ++state.row;
  }

  void undo(State &state, const Move &move) const
  {
    --state.row;
    state.total_cost -= costs_[state.row][move.col];
    state.used_columns[move.col] = 0;
    state.assigned_column[state.row] = -1;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.row >= costs_.size())
      return true;

    for (size_t col = 0; col < costs_[state.row].size(); ++col)
      if (not state.used_columns[col] and not visit(Move{col}))
        return false;

    return true;
  }

private:
  Array<Array<int>> costs_;
};

static_assert(BranchAndBoundDomain<ArtificialMaxDomain>);
static_assert(BranchAndBoundDomain<ArtificialMinDomain>);
static_assert(BranchAndBoundDomain<KnapsackBBDomain>);
static_assert(BranchAndBoundDomain<AssignmentBBDomain>);
static_assert(BranchAndBoundObjectivePolicy<Maximize_Objective<int>, int>);
static_assert(BranchAndBoundObjectivePolicy<Minimize_Objective<int>, int>);

std::string artificial_signature(const SearchPath<ArtificialMove> &path)
{
  std::string out;
  for (const auto &move : path)
    out.push_back(move.label);
  return out;
}

int brute_assignment_cost(const Array<Array<int>> &costs,
                          const size_t row,
                          Array<unsigned char> &used)
{
  if (row == costs.size())
    return 0;

  int best = -1;
  for (size_t col = 0; col < costs[row].size(); ++col)
    if (not used[col])
      {
        used[col] = 1;
        const int candidate = costs[row][col] + brute_assignment_cost(costs, row + 1, used);
        used[col] = 0;
        if (best < 0 or candidate < best)
          best = candidate;
      }

  return best;
}

} // end namespace

TEST(BranchAndBoundFramework, ObjectivePoliciesAndIncumbentConstruct)
{
  using Sol = OptimizationSolution<int, int, int>;

  ObjectiveIncumbent<Sol, Maximize_Objective<int>> incumbent(Maximize_Objective<int>{});
  EXPECT_FALSE(incumbent.has_value());
  EXPECT_TRUE(incumbent.can_improve(10));

  Sol first;
  first.objective_value = 5;
  EXPECT_TRUE(incumbent.consider(first));
  EXPECT_TRUE(incumbent.has_value());
  EXPECT_EQ(incumbent.best_value(), 5);
  EXPECT_FALSE(incumbent.can_improve(5));
  EXPECT_TRUE(incumbent.can_improve(6));
}

TEST(BranchAndBoundFramework, ArtificialMaximizationPrunesByBound)
{
  ArtificialMaxDomain domain;
  SearchSolutionCollector<Branch_And_Bound<ArtificialMaxDomain>::Solution> collector;

  auto result = branch_and_bound_search(domain, ArtificialState{}, collector);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.incumbent.best_value(), 9);
  EXPECT_EQ(artificial_signature(result.incumbent.get().path), "LR");
  EXPECT_EQ(result.stats.solutions_found, 2u);
  EXPECT_EQ(result.stats.pruned_by_bound, 1u);
  EXPECT_EQ(result.stats.incumbent_updates, 2u);
  EXPECT_EQ(collector.size(), 2u);
}

TEST(BranchAndBoundFramework, ArtificialMinimizationWorksWithBestFirst)
{
  ArtificialMinDomain domain;
  ExplorationPolicy policy = Branch_And_Bound<ArtificialMinDomain,
                                              Minimize_Objective<int>>::default_policy();
  policy.strategy = ExplorationPolicy::Strategy::Best_First;

  Branch_And_Bound<ArtificialMinDomain, Minimize_Objective<int>> engine(domain, policy);
  auto result = engine.search(ArtificialState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.incumbent.best_value(), 6);
  EXPECT_EQ(artificial_signature(result.incumbent.get().path), "LR");
  EXPECT_GT(result.stats.pruned_by_bound, 0u);
}

TEST(BranchAndBoundFramework, BoundOrderedDepthFirstAndBestFirstBeatPlainBadOrder)
{
  ArtificialMaxBadOrderDomain domain;

  auto plain_result = branch_and_bound_search(domain, ArtificialState{});

  ExplorationPolicy ordered_policy = Branch_And_Bound<ArtificialMaxBadOrderDomain>::default_policy();
  ordered_policy.move_ordering = MoveOrderingMode::Estimated_Bound;
  auto ordered_result = branch_and_bound_search(domain, ArtificialState{}, ordered_policy);

  ExplorationPolicy best_policy = Branch_And_Bound<ArtificialMaxBadOrderDomain>::default_policy();
  best_policy.strategy = ExplorationPolicy::Strategy::Best_First;
  auto best_result = branch_and_bound_search(domain, ArtificialState{}, best_policy);

  ASSERT_TRUE(plain_result.found_solution());
  ASSERT_TRUE(ordered_result.found_solution());
  ASSERT_TRUE(best_result.found_solution());
  EXPECT_EQ(plain_result.incumbent.best_value(), 9);
  EXPECT_EQ(ordered_result.incumbent.best_value(), plain_result.incumbent.best_value());
  EXPECT_EQ(best_result.incumbent.best_value(), plain_result.incumbent.best_value());
  EXPECT_EQ(artificial_signature(ordered_result.incumbent.get().path), "LR");
  EXPECT_LT(ordered_result.stats.visited_states, plain_result.stats.visited_states);
  EXPECT_GT(ordered_result.stats.move_ordering.ordered_batches, 0u);
  EXPECT_GT(ordered_result.stats.move_ordering.priority_estimates, 0u);
  EXPECT_LT(best_result.stats.visited_states, plain_result.stats.visited_states);
}

TEST(BranchAndBoundFramework, StopAtFirstSolutionUsesCommonExplorationPolicy)
{
  ArtificialMaxDomain domain;
  ExplorationPolicy policy = Branch_And_Bound<ArtificialMaxDomain>::default_policy();
  policy.stop_at_first_solution = true;

  auto result = branch_and_bound_search(domain, ArtificialState{}, policy);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.stopped_on_solution());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(result.incumbent.best_value(), 7);
  EXPECT_EQ(artificial_signature(result.incumbent.get().path), "LL");
}

TEST(BranchAndBoundFramework, KnapsackFractionalBoundFindsOptimal)
{
  const Array<Knapsack_Item<int, double>> items = {
    {2, 40.0}, {5, 30.0}, {10, 50.0}, {5, 10.0}
  };
  constexpr int capacity = 16;

  KnapsackBBDomain domain(items, capacity, true);
  auto result = branch_and_bound_search(domain, KnapsackState(items.size()));
  const double optimum = knapsack_01_value<double, int>(items, capacity);

  ASSERT_TRUE(result.found_solution());
  EXPECT_DOUBLE_EQ(result.incumbent.best_value(), 90.0);
  EXPECT_DOUBLE_EQ(result.incumbent.best_value(), optimum);
}

TEST(BranchAndBoundFramework, KnapsackBestFirstMatchesDepthFirst)
{
  const Array<Knapsack_Item<int, double>> items = {
    {2, 40.0}, {5, 30.0}, {10, 50.0}, {5, 10.0}
  };
  constexpr int capacity = 16;

  KnapsackBBDomain domain(items, capacity, true);
  auto depth_result = branch_and_bound_search(domain, KnapsackState(items.size()));

  ExplorationPolicy policy = Branch_And_Bound<KnapsackBBDomain>::default_policy();
  policy.strategy = ExplorationPolicy::Strategy::Best_First;
  Branch_And_Bound<KnapsackBBDomain> best_first(domain, policy);
  auto best_result = best_first.search(KnapsackState(items.size()));

  EXPECT_DOUBLE_EQ(depth_result.incumbent.best_value(), best_result.incumbent.best_value());
  EXPECT_GT(best_result.stats.pruned_by_bound, 0u);
}

TEST(BranchAndBoundFramework, KnapsackFractionalBoundPrunesMoreThanWeakBound)
{
  const Array<Knapsack_Item<int, double>> items = {
    {10, 100.0}, {9, 80.0}, {9, 80.0}, {9, 80.0}
  };
  constexpr int capacity = 10;

  auto tight_result = branch_and_bound_search(KnapsackBBDomain(items, capacity, true),
                                              KnapsackState(items.size()));
  auto weak_result = branch_and_bound_search(KnapsackBBDomain(items, capacity, false),
                                             KnapsackState(items.size()));

  EXPECT_DOUBLE_EQ(tight_result.incumbent.best_value(), weak_result.incumbent.best_value());
  EXPECT_GT(tight_result.stats.pruned_by_bound, 0u);
  EXPECT_LT(tight_result.stats.visited_states, weak_result.stats.visited_states);
  EXPECT_LT(tight_result.stats.expanded_states, weak_result.stats.expanded_states);
}

TEST(BranchAndBoundFramework, AssignmentMinimizationFindsExpectedOptimum)
{
  const Array<Array<int>> costs = {
    Array<int>{9, 2, 7, 8},
    Array<int>{6, 4, 3, 7},
    Array<int>{5, 8, 1, 8},
    Array<int>{7, 6, 9, 4}
  };

  AssignmentBBDomain domain(costs);
  Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>> engine(
    domain,
    Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>>::default_policy(),
    {},
    Minimize_Objective<int>{});

  auto result = engine.search(AssignmentState(costs.size()));

  Array<unsigned char> used(costs.size(), 0);
  const int brute = brute_assignment_cost(costs, 0, used);

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.incumbent.best_value(), brute);
  EXPECT_EQ(result.incumbent.best_value(), 13);
}

TEST(BranchAndBoundFramework, AssignmentBestFirstMatchesDepthFirst)
{
  const Array<Array<int>> costs = {
    Array<int>{9, 2, 7, 8},
    Array<int>{6, 4, 3, 7},
    Array<int>{5, 8, 1, 8},
    Array<int>{7, 6, 9, 4}
  };

  AssignmentBBDomain domain(costs);
  Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>> depth_engine(
    domain,
    Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>>::default_policy(),
    {},
    Minimize_Objective<int>{});
  auto depth_result = depth_engine.search(AssignmentState(costs.size()));

  ExplorationPolicy policy = Branch_And_Bound<AssignmentBBDomain,
                                              Minimize_Objective<int>>::default_policy();
  policy.strategy = ExplorationPolicy::Strategy::Best_First;
  Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>> best_engine(
    domain, policy, {}, Minimize_Objective<int>{});
  auto best_result = best_engine.search(AssignmentState(costs.size()));

  EXPECT_EQ(depth_result.incumbent.best_value(), best_result.incumbent.best_value());
  EXPECT_GE(best_result.stats.pruned_by_bound, 0u);
}
