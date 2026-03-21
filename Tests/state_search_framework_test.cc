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

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include <State_Search.H>
#include <State_Search_IDA_Star.H>

using namespace Aleph;

namespace
{

struct ArtificialTreeState
{
  size_t depth = 0;
  size_t code = 1;
};

struct ArtificialDecisionTreeDomain
{
  struct Move
  {
    char label = 'L';
  };

  using State = ArtificialTreeState;
  using State_Key = std::uint64_t;

  size_t leaf_depth = 2;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return (static_cast<State_Key>(state.depth) << 32)
           ^ static_cast<State_Key>(state.code);
  }

  bool is_goal(const State &state) const
  {
    return state.depth == leaf_depth and (state.code == 5 or state.code == 6);
  }

  bool is_terminal(const State &state) const
  {
    return state.depth == leaf_depth;
  }

  void apply(State &state, const Move &move) const
  {
    state.code = state.code*2 + (move.label == 'R' ? 1u : 0u);
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.code = (state.code - (move.label == 'R' ? 1u : 0u))/2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= leaf_depth)
      return true;

    if (not visit(Move{'L'}))
      return false;

    return visit(Move{'R'});
  }
};

struct NQueensState
{
  size_t n = 0;
  size_t row = 0;
  Array<int> queens;
  Array<unsigned char> used_columns;
  Array<unsigned char> used_diag_down;
  Array<unsigned char> used_diag_up;

  explicit NQueensState(const size_t size = 0)
    : n(size),
      row(0),
      queens(size, -1),
      used_columns(size, 0),
      used_diag_down(size == 0 ? 0 : 2*size - 1, 0),
      used_diag_up(size == 0 ? 0 : 2*size - 1, 0)
  {
    // empty
  }
};

struct NQueensDomain
{
  struct Move
  {
    size_t col = 0;
  };

  using State = NQueensState;
  using State_Key = std::uint64_t;

  size_t n = 0;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    State_Key key = static_cast<State_Key>(state.row);
    for (size_t row = 0; row < state.n; ++row)
      {
        const int col = state.queens[row];
        key = key * static_cast<State_Key>(1315423911u)
              + static_cast<State_Key>(col + 2);
      }

    return key;
  }

  bool is_goal(const State &state) const
  {
    return state.row == n;
  }

  void apply(State &state, const Move &move) const
  {
    const size_t row = state.row;
    const size_t down = row + state.n - 1 - move.col;
    const size_t up = row + move.col;

    state.queens[row] = static_cast<int>(move.col);
    state.used_columns[move.col] = 1;
    state.used_diag_down[down] = 1;
    state.used_diag_up[up] = 1;
    ++state.row;
  }

  void undo(State &state, const Move &move) const
  {
    --state.row;
    const size_t row = state.row;
    const size_t down = row + state.n - 1 - move.col;
    const size_t up = row + move.col;

    state.queens[row] = -1;
    state.used_columns[move.col] = 0;
    state.used_diag_down[down] = 0;
    state.used_diag_up[up] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.row >= n)
      return true;

    for (size_t col = 0; col < n; ++col)
      {
        const size_t down = state.row + state.n - 1 - col;
        const size_t up = state.row + col;
        if (state.used_columns[col] or state.used_diag_down[down] or state.used_diag_up[up])
          continue;

        if (not visit(Move{col}))
          return false;
      }

    return true;
  }
};

struct SubsetSumState
{
  size_t index = 0;
  int sum = 0;
  Array<unsigned char> chosen;

  explicit SubsetSumState(const size_t n = 0)
    : index(0), sum(0), chosen(n, 0)
  {
    // empty
  }
};

class SubsetSumDomain
{
public:
  struct Move
  {
    bool take = false;
  };

  using State = SubsetSumState;
  using State_Key = std::uint64_t;

  explicit SubsetSumDomain(const Array<int> &values, const int target)
    : values_(values),
      suffix_remaining_(values.size() + 1, 0),
      target_(target)
  {
    for (size_t i = values_.size(); i > 0; --i)
      suffix_remaining_[i - 1] = suffix_remaining_[i] + values_[i - 1];
  }

  bool is_goal(const State &state) const
  {
    return state.sum == target_;
  }

  bool is_terminal(const State &state) const
  {
    return state.index == values_.size();
  }

  bool should_prune(const State &state, const size_t) const
  {
    return state.sum > target_ or state.sum + suffix_remaining_[state.index] < target_;
  }

  void apply(State &state, const Move &move) const
  {
    if (move.take)
      state.sum += values_[state.index];

    state.chosen[state.index] = move.take ? 1 : 0;
    ++state.index;
  }

  void undo(State &state, const Move &move) const
  {
    --state.index;
    if (move.take)
      state.sum -= values_[state.index];

    state.chosen[state.index] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.index >= values_.size())
      return true;

    if (not visit(Move{true}))
      return false;

    return visit(Move{false});
  }

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    State_Key key = static_cast<State_Key>(state.index);
    key = key * static_cast<State_Key>(2654435761u)
          + static_cast<State_Key>(state.sum ^ 0x9e3779b9);
    for (const auto pick : state.chosen)
      key = (key << 1) ^ static_cast<State_Key>(pick);
    return key;
  }

private:
  Array<int> values_;
  Array<int> suffix_remaining_;
  int target_ = 0;
};

struct TinyIDAStarState
{
  size_t node = 0;
  Array<size_t> history;
};

struct TinyIDAStarDomain
{
  struct Move
  {
    size_t to = 0;
    int cost = 0;
  };

  using State = TinyIDAStarState;
  using State_Key = std::uint64_t;
  using Distance = int;

  TinyIDAStarDomain()
    : adjacency_{
        Array<Move>{Move{1, 4}, Move{2, 1}},
        Array<Move>{Move{4, 2}},
        Array<Move>{Move{3, 1}},
        Array<Move>{Move{4, 5}},
        Array<Move>{}},
      heuristic_{6, 2, 6, 5, 0}
  {
    // empty
  }

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return (static_cast<State_Key>(state.node) << 32) | static_cast<State_Key>(state.history.size());
  }

  [[nodiscard]] bool is_goal(const State &state) const noexcept
  {
    return state.node == goal_;
  }

  [[nodiscard]] bool is_terminal(const State &state) const noexcept
  {
    return adjacency_[state.node].is_empty();
  }

  void apply(State &state, const Move &move) const
  {
    state.history.append(state.node);
    state.node = move.to;
  }

  void undo(State &state, const Move &) const
  {
    if (state.history.is_empty())
      {
        state.node = 0;
        return;
      }

    const size_t previous = state.history[state.history.size() - 1];
    (void) state.history.remove_last();
    state.node = previous;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    for (const auto &move : adjacency_[state.node])
      {
        if (not visit(move))
          return false;
      }

    return true;
  }

  [[nodiscard]] Distance heuristic(const State &state) const noexcept
  {
    return heuristic_[state.node];
  }

  [[nodiscard]] Distance cost(const State &, const Move &move) const noexcept
  {
    return move.cost;
  }

private:
  Array<Array<Move>> adjacency_;
  Array<Distance> heuristic_;
  size_t goal_ = 4;
};

struct ThrowingApplyIDAState
{
  std::shared_ptr<bool> undo_called;
  size_t node = 0;
};

struct ThrowingApplyIDADomain
{
  struct Move
  {
    size_t to = 1;
    int cost = 1;
  };

  using State = ThrowingApplyIDAState;
  using State_Key = std::uint64_t;
  using Distance = int;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.node;
  }

  [[nodiscard]] bool is_goal(const State &) const noexcept
  {
    return false;
  }

  [[nodiscard]] bool is_terminal(const State &state) const noexcept
  {
    return state.node != 0;
  }

  void apply(State &state, const Move &) const
  {
    (void) state;
    throw std::runtime_error("apply failed");
  }

  void undo(State &state, const Move &) const
  {
    *state.undo_called = true;
    state.node = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.node != 0)
      return true;

    return visit(Move{1, 1});
  }

  [[nodiscard]] Distance heuristic(const State &) const noexcept
  {
    return 0;
  }

  [[nodiscard]] Distance cost(const State &, const Move &move) const noexcept
  {
    return move.cost;
  }
};

static_assert(SearchState<ArtificialDecisionTreeDomain::State>);
static_assert(SearchMove<ArtificialDecisionTreeDomain::Move>);
static_assert(BacktrackingDomain<ArtificialDecisionTreeDomain>);
static_assert(TerminalPredicate<ArtificialDecisionTreeDomain>);

static_assert(SearchState<NQueensDomain::State>);
static_assert(SearchMove<NQueensDomain::Move>);
static_assert(BacktrackingDomain<NQueensDomain>);

static_assert(SearchState<SubsetSumDomain::State>);
static_assert(SearchMove<SubsetSumDomain::Move>);
static_assert(BacktrackingDomain<SubsetSumDomain>);
static_assert(TerminalPredicate<SubsetSumDomain>);
static_assert(DomainPruner<SubsetSumDomain>);

static_assert(IDAStarDomain<TinyIDAStarDomain>);
static_assert(IDAStarDomain<ThrowingApplyIDADomain>);

template <typename Move>
std::string path_signature(const SearchPath<Move> &path)
{
  std::string signature;
  for (const auto &move : path)
    signature.push_back(move.label);
  return signature;
}

std::string nqueens_signature(const NQueensState &state)
{
  std::string signature;
  for (size_t row = 0; row < state.n; ++row)
    signature.push_back(static_cast<char>('0' + state.queens[row]));
  return signature;
}

std::string subset_sum_signature(const SubsetSumState &state)
{
  std::string signature;
  for (const auto pick : state.chosen)
    signature.push_back(pick ? '1' : '0');
  return signature;
}

std::string ida_star_path_signature(const SearchPath<TinyIDAStarDomain::Move> &path)
{
  std::string signature;
  size_t node = 0;
  signature.push_back(static_cast<char>('0' + node));
  for (const auto &move : path)
    {
      node = move.to;
      signature.push_back(static_cast<char>('0' + node));
    }
  return signature;
}

template <typename SolutionList, typename Projector>
bool contains_signature(const SolutionList &solutions,
                        const std::string &expected,
                        Projector projector)
{
  for (auto it = solutions.get_it(); it.has_curr(); it.next_ne())
    if (projector(it.get_curr()) == expected)
      return true;

  return false;
}

} // end namespace

TEST(StateSearchFramework, CommonTypesAndCollectorConstruct)
{
  SearchStats stats;
  SearchLimits limits;
  ExplorationPolicy policy;

  EXPECT_EQ(stats.visited_states, 0u);
  EXPECT_EQ(stats.terminal_states, 0u);
  EXPECT_EQ(stats.pruned_by_domain, 0u);
  EXPECT_EQ(limits.max_depth, Search_Unlimited);
  EXPECT_EQ(policy.strategy, ExplorationPolicy::Strategy::Depth_First);
  EXPECT_TRUE(policy.stop_at_first_solution);

  BestSolution<int> incumbent;
  EXPECT_FALSE(incumbent.has_value());
  EXPECT_TRUE(incumbent.consider(7));
  EXPECT_EQ(incumbent.get(), 7);
  EXPECT_FALSE(incumbent.consider(9));

  SearchSolutionCollector<int> collector(2);
  EXPECT_TRUE(collector.is_empty());
  EXPECT_TRUE(collector(10));
  EXPECT_FALSE(collector.is_empty());
  EXPECT_FALSE(collector(20));
  EXPECT_EQ(collector.size(), 2u);

  SearchPath<int> path;
  path.append(1);
  path.append(2);
  EXPECT_EQ(path.size(), 2u);

  SearchStorageMap<int, int> memo;
  auto *entry = memo.insert(1, 11);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->first, 1);
  EXPECT_EQ(entry->second, 11);
}

TEST(StateSearchFramework, ArtificialTreeStopsAtFirstSolutionWithExpectedCounts)
{
  ArtificialDecisionTreeDomain domain;
  Depth_First_Backtracking<ArtificialDecisionTreeDomain> engine(domain);

  auto result = engine.search(ArtificialTreeState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.stopped_on_solution());
  EXPECT_EQ(result.stats.visited_states, 4u);
  EXPECT_EQ(result.stats.expanded_states, 2u);
  EXPECT_EQ(result.stats.generated_successors, 3u);
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(result.stats.terminal_states, 1u);
  EXPECT_EQ(result.stats.max_depth_reached, 2u);

  EXPECT_EQ(path_signature(result.best_solution.get().path), "LR");
}

TEST(StateSearchFramework, ArtificialTreeEnumeratesAllSolutions)
{
  ArtificialDecisionTreeDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<ArtificialDecisionTreeDomain> engine(domain, policy);
  SearchSolutionCollector<Depth_First_Backtracking<ArtificialDecisionTreeDomain>::Solution> collector;

  auto result = engine.search(ArtificialTreeState{}, collector);

  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.visited_states, 7u);
  EXPECT_EQ(result.stats.expanded_states, 3u);
  EXPECT_EQ(result.stats.generated_successors, 6u);
  EXPECT_EQ(result.stats.solutions_found, 2u);
  EXPECT_EQ(result.stats.terminal_states, 2u);
  EXPECT_EQ(collector.size(), 2u);
  EXPECT_TRUE(contains_signature(collector.solutions(), "LR",
                                 [](const auto &solution)
                                 {
                                   return path_signature(solution.path);
                                 }));
  EXPECT_TRUE(contains_signature(collector.solutions(), "RL",
                                 [](const auto &solution)
                                 {
                                   return path_signature(solution.path);
                                 }));
}

TEST(StateSearchFramework, ArtificialTreeRespectsDepthLimit)
{
  ArtificialDecisionTreeDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  SearchLimits limits;
  limits.max_depth = 1;

  Depth_First_Backtracking<ArtificialDecisionTreeDomain> engine(domain, policy, limits);
  SearchSolutionCollector<Depth_First_Backtracking<ArtificialDecisionTreeDomain>::Solution> collector;

  auto result = engine.search(ArtificialTreeState{}, collector);

  EXPECT_FALSE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.visited_states, 3u);
  EXPECT_EQ(result.stats.expanded_states, 1u);
  EXPECT_EQ(result.stats.pruned_by_depth, 2u);
  EXPECT_EQ(collector.size(), 0u);
}

TEST(StateSearchFramework, NQueensEnumeratesExpectedSolutions)
{
  NQueensDomain domain{4};
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<NQueensDomain> engine(domain, policy);
  SearchSolutionCollector<Depth_First_Backtracking<NQueensDomain>::Solution> collector;

  auto result = engine.search(NQueensState(4), collector);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.solutions_found, 2u);
  EXPECT_EQ(collector.size(), 2u);
  EXPECT_EQ(result.best_solution.get().depth, 4u);
  EXPECT_TRUE(contains_signature(collector.solutions(), "1302",
                                 [](const auto &solution)
                                 {
                                   return nqueens_signature(solution.state);
                                 }));
  EXPECT_TRUE(contains_signature(collector.solutions(), "2031",
                                 [](const auto &solution)
                                 {
                                   return nqueens_signature(solution.state);
                                 }));
}

TEST(StateSearchFramework, NQueensDepthLimitPreventsSolutions)
{
  NQueensDomain domain{4};
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  SearchLimits limits;
  limits.max_depth = 3;

  Depth_First_Backtracking<NQueensDomain> engine(domain, policy, limits);
  auto result = engine.search(NQueensState(4));

  EXPECT_FALSE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_GT(result.stats.pruned_by_depth, 0u);
}

TEST(StateSearchFramework, SubsetSumEnumeratesExpectedSolutionsAndPrunes)
{
  SubsetSumDomain domain(Array<int>{4, 1, 1, 2}, 2);
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<SubsetSumDomain> engine(domain, policy);
  SearchSolutionCollector<Depth_First_Backtracking<SubsetSumDomain>::Solution> collector;

  auto result = engine.search(SubsetSumState(4), collector);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.solutions_found, 2u);
  EXPECT_EQ(collector.size(), 2u);
  EXPECT_GT(result.stats.pruned_by_domain, 0u);
  EXPECT_TRUE(contains_signature(collector.solutions(), "0110",
                                 [](const auto &solution)
                                 {
                                   return subset_sum_signature(solution.state);
                                 }));
  EXPECT_TRUE(contains_signature(collector.solutions(), "0001",
                                 [](const auto &solution)
                                 {
                                   return subset_sum_signature(solution.state);
                                 }));
}

TEST(StateSearchFramework, SubsetSumMaxSolutionsLimitStopsEnumeration)
{
  SubsetSumDomain domain(Array<int>{4, 1, 1, 2}, 2);
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  SearchLimits limits;
  limits.max_solutions = 1;

  Depth_First_Backtracking<SubsetSumDomain> engine(domain, policy, limits);
  SearchSolutionCollector<Depth_First_Backtracking<SubsetSumDomain>::Solution> collector;

  auto result = engine.search(SubsetSumState(4), collector);

  EXPECT_TRUE(result.limit_reached());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(collector.size(), 1u);
}

TEST(StateSearchFramework, IDAStarFindsOptimalSolution)
{
  TinyIDAStarDomain domain;
  IDA_Star_State_Search<TinyIDAStarDomain> engine(domain);

  auto result = engine.search(TinyIDAStarDomain::State{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.total_cost, 6);
  ASSERT_TRUE(result.best_solution.has_value());
  EXPECT_EQ(result.best_solution.get().path.size(), 2u);
  EXPECT_EQ(ida_star_path_signature(result.best_solution.get().path), "014");
  ASSERT_FALSE(result.iterations.is_empty());
  EXPECT_EQ(result.iterations[0].threshold, 6);
}

TEST(StateSearchFramework, IDAStarRespectsDepthLimit)
{
  TinyIDAStarDomain domain;
  SearchLimits limits;
  limits.max_depth = 1;

  IDA_Star_State_Search<TinyIDAStarDomain> engine(domain, ExplorationPolicy{}, limits);
  auto result = engine.search(TinyIDAStarDomain::State{});

  EXPECT_FALSE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_GT(result.stats.pruned_by_depth, 0u);
}

TEST(StateSearchFramework, IDAStarCallbackStopsAfterFirstGoal)
{
  TinyIDAStarDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  size_t callbacks = 0;
  auto on_solution = [&](const auto &solution) {
    (void) solution;
    ++callbacks;
    return callbacks == 1 ? false : true;
  };

  IDA_Star_State_Search<TinyIDAStarDomain> engine(domain, policy);
  auto result = engine.search(TinyIDAStarDomain::State{}, on_solution);

  EXPECT_TRUE(result.stopped_on_solution());
  EXPECT_EQ(callbacks, 1u);
  EXPECT_EQ(result.stats.solutions_found, 1u);
}

TEST(StateSearchFramework, IDAStarApplyExceptionDoesNotCallUndo)
{
  auto undo_called = std::make_shared<bool>(false);
  ThrowingApplyIDADomain domain;
  IDA_Star_State_Search<ThrowingApplyIDADomain> engine(domain);

  EXPECT_THROW((void) engine.search(ThrowingApplyIDAState{undo_called, 0}), std::runtime_error);
  EXPECT_FALSE(*undo_called);
}

// ---------------------------------------------------------------------------
// Edge-case tests (Recommendation 5)
// ---------------------------------------------------------------------------

// max_depth=0: root is visited but never expanded — no solutions possible
// in a tree where goals live at depth 2.
TEST(StateSearchFramework, MaxDepthZeroVisitsRootButNeverExpands)
{
  ArtificialDecisionTreeDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  SearchLimits limits;
  limits.max_depth = 0;

  Depth_First_Backtracking<ArtificialDecisionTreeDomain> engine(domain, policy, limits);
  auto result = engine.search(ArtificialTreeState{});

  EXPECT_FALSE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.visited_states, 1u);
  EXPECT_EQ(result.stats.expanded_states, 0u);
  EXPECT_EQ(result.stats.pruned_by_depth, 1u);
  EXPECT_EQ(result.stats.generated_successors, 0u);
}

// max_expansions=1: only the root is expanded — its two children are visited
// but never expanded themselves, so no solutions at depth 2.
TEST(StateSearchFramework, MaxExpansionsOneExpandsOnlyRoot)
{
  ArtificialDecisionTreeDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  SearchLimits limits;
  limits.max_expansions = 1;

  Depth_First_Backtracking<ArtificialDecisionTreeDomain> engine(domain, policy, limits);
  auto result = engine.search(ArtificialTreeState{});

  EXPECT_TRUE(result.limit_reached());
  EXPECT_EQ(result.stats.expanded_states, 1u);
  EXPECT_GE(result.stats.generated_successors, 1u);
  EXPECT_LE(result.stats.generated_successors, 2u);
  EXPECT_EQ(result.stats.solutions_found, 0u);
}

// Root is goal: a domain where the initial state already satisfies is_goal().
// The engine must find a solution with an empty path at depth 0.
namespace
{

struct RootGoalDomain
{
  struct Move
  {
    int id = 0;
  };

  using State = ArtificialTreeState;
  using State_Key = size_t;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.code;
  }

  bool is_goal(const State &) const
  {
    return true;
  }

  void apply(State &state, const Move &) const
  {
    ++state.depth;
  }

  void undo(State &state, const Move &) const
  {
    --state.depth;
  }

  template <typename Visitor>
  bool for_each_successor(const State &, Visitor visit) const
  {
    return visit(Move{1});
  }
};

} // end namespace

TEST(StateSearchFramework, RootStateIsGoalFindsImmediateSolution)
{
  RootGoalDomain domain;
  Depth_First_Backtracking<RootGoalDomain> engine(domain);

  auto result = engine.search(ArtificialTreeState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.stopped_on_solution());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(result.stats.visited_states, 1u);
  EXPECT_EQ(result.stats.expanded_states, 0u);
  EXPECT_EQ(result.best_solution.get().depth, 0u);
  EXPECT_TRUE(result.best_solution.get().path.is_empty());
}

// Root is goal with max_depth=0: even at depth 0, the root is checked for
// goal before the depth limit prunes expansion — it should still find a solution.
TEST(StateSearchFramework, RootGoalFoundEvenWithMaxDepthZero)
{
  RootGoalDomain domain;
  SearchLimits limits;
  limits.max_depth = 0;

  Depth_First_Backtracking<RootGoalDomain> engine(domain, ExplorationPolicy{}, limits);

  auto result = engine.search(ArtificialTreeState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(result.best_solution.get().depth, 0u);
}

// Cycles in the state space: a directed graph with cycles that would cause
// infinite recursion without a visited set.  The graph is:
//
//   0 → 1 → 2 → 3 (goal)
//         ↘ 0 (cycle back)
//
// Without visited-set, the cycle 0→1→0→1→... causes infinite recursion.
// With visited-set (search_visited), the engine detects the repeated state
// and finds the goal via the non-cyclic path.
namespace
{

struct CyclicGraphState
{
  size_t node = 0;
};

struct CyclicGraphDomain
{
  struct Move
  {
    size_t from = 0;
    size_t to = 0;
  };

  using State = CyclicGraphState;
  using State_Key = size_t;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.node;
  }

  bool is_goal(const State &state) const
  {
    return state.node == 3;
  }

  void apply(State &state, const Move &move) const
  {
    state.node = move.to;
  }

  void undo(State &state, const Move &move) const
  {
    state.node = move.from;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    switch (state.node)
      {
      case 0:
        return visit(Move{0, 1});
      case 1:
        if (not visit(Move{1, 0}))  // cycle back to 0 (tried first)
          return false;
        return visit(Move{1, 2});
      case 2:
        return visit(Move{2, 3});
      default:
        return true;
      }
  }
};

} // end namespace

TEST(StateSearchFramework, CyclicGraphWithVisitedSetFindsSolution)
{
  CyclicGraphDomain domain;
  Depth_First_Backtracking<CyclicGraphDomain> engine(domain);

  SearchStorageMap<size_t, size_t> visited;
  auto result = engine.search(CyclicGraphState{}, visited);

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.best_solution.get().state.node, 3u);
  EXPECT_GT(result.stats.pruned_by_visited, 0u);
}

// Cycle with multiple paths of different depth: state S is reachable via a
// short path (depth 1) and a long path (depth 3).  With the depth-aware
// visited map, the shorter path records depth 1 for S. When the longer path
// reaches S at depth 3, it should be pruned.  Goal G is reachable only
// through S.
//
//   0 → S → G          (short: depth 2)
//   0 → A → B → S → G  (long: depth 4, pruned at S)
//
namespace
{

struct MultiPathState
{
  int node = 0;
};

struct MultiPathDomain
{
  struct Move
  {
    int from = 0;
    int to = 0;
  };

  using State = MultiPathState;
  using State_Key = int;

  static constexpr int S = 1;
  static constexpr int G = 2;
  static constexpr int A = 3;
  static constexpr int B = 4;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.node;
  }

  bool is_goal(const State &state) const
  {
    return state.node == G;
  }

  void apply(State &state, const Move &move) const
  {
    state.node = move.to;
  }

  void undo(State &state, const Move &move) const
  {
    state.node = move.from;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    switch (state.node)
      {
      case 0:
        if (not visit(Move{0, S}))  // short path: 0 → S
          return false;
        return visit(Move{0, A});   // long path: 0 → A → B → S
      case S:
        return visit(Move{S, G});
      case A:
        return visit(Move{A, B});
      case B:
        return visit(Move{B, S});   // reaches S at depth 3 (pruned)
      default:
        return true;
      }
  }
};

} // end namespace

TEST(StateSearchFramework, MultiPathDepthAwareVisitedPrunesLongerPath)
{
  MultiPathDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<MultiPathDomain> engine(domain, policy);

  SearchStorageMap<int, size_t> visited;
  auto result = engine.search(MultiPathState{}, visited);

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(result.stats.pruned_by_visited, 1u);
  EXPECT_EQ(result.best_solution.get().state.node, MultiPathDomain::G);
}

// IDA* with max_depth=0: root is visited but not expanded.
TEST(StateSearchFramework, IDAStarMaxDepthZeroDoesNotExpand)
{
  TinyIDAStarDomain domain;
  SearchLimits limits;
  limits.max_depth = 0;

  IDA_Star_State_Search<TinyIDAStarDomain> engine(domain, ExplorationPolicy{}, limits);
  auto result = engine.search(TinyIDAStarDomain::State{});

  EXPECT_FALSE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_GT(result.stats.pruned_by_depth, 0u);
  EXPECT_EQ(result.stats.expanded_states, 0u);
}

// IDA* with root as goal: the initial state has node==goal.
namespace
{

struct IDAStarRootGoalDomain
{
  struct Move
  {
    int to = 0;
    int cost = 1;
  };

  using State = TinyIDAStarState;
  using State_Key = size_t;
  using Distance = int;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.node;
  }

  bool is_goal(const State &state) const
  {
    return state.node == 0;
  }

  bool is_terminal(const State &) const
  {
    return false;
  }

  void apply(State &state, const Move &move) const
  {
    state.history.append(state.node);
    state.node = static_cast<size_t>(move.to);
  }

  void undo(State &state, const Move &) const
  {
    if (state.history.is_empty())
      return;

    const size_t prev = state.history[state.history.size() - 1];
    (void) state.history.remove_last();
    state.node = prev;
  }

  template <typename Visitor>
  bool for_each_successor(const State &, Visitor visit) const
  {
    return visit(Move{1, 5});
  }

  [[nodiscard]] Distance heuristic(const State &) const noexcept
  {
    return 0;
  }

  [[nodiscard]] Distance cost(const State &, const Move &move) const noexcept
  {
    return move.cost;
  }
};

} // end namespace

TEST(StateSearchFramework, IDAStarRootIsGoalFindsImmediateSolution)
{
  IDAStarRootGoalDomain domain;
  IDA_Star_State_Search<IDAStarRootGoalDomain> engine(domain);

  auto result = engine.search(TinyIDAStarState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.total_cost, 0);
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_TRUE(result.best_solution.get().path.is_empty());
}

// IDA* with zero-cost edges: all edges cost 0.  The heuristic is 0 everywhere.
// This means f = g + h = 0 for all states and the initial threshold suffices.
namespace
{

struct ZeroCostIDADomain
{
  struct Move
  {
    size_t to = 0;
    int cost = 0;
  };

  using State = TinyIDAStarState;
  using State_Key = size_t;
  using Distance = int;

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return state.node;
  }

  bool is_goal(const State &state) const
  {
    return state.node == 2;
  }

  void apply(State &state, const Move &move) const
  {
    state.history.append(state.node);
    state.node = move.to;
  }

  void undo(State &state, const Move &) const
  {
    if (state.history.is_empty())
      return;

    const size_t prev = state.history[state.history.size() - 1];
    (void) state.history.remove_last();
    state.node = prev;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.node == 0)
      return visit(Move{1, 0});
    if (state.node == 1)
      return visit(Move{2, 0});
    return true;
  }

  [[nodiscard]] Distance heuristic(const State &) const noexcept
  {
    return 0;
  }

  [[nodiscard]] Distance cost(const State &, const Move &move) const noexcept
  {
    return move.cost;
  }
};

} // end namespace

TEST(StateSearchFramework, IDAStarZeroCostEdgesFindsGoal)
{
  ZeroCostIDADomain domain;
  IDA_Star_State_Search<ZeroCostIDADomain> engine(domain);

  auto result = engine.search(TinyIDAStarState{});

  ASSERT_TRUE(result.found_solution());
  EXPECT_EQ(result.total_cost, 0);
  EXPECT_EQ(result.stats.solutions_found, 1u);
}

// NQueens n=1: trivial single-queen problem — should find exactly 1 solution.
TEST(StateSearchFramework, NQueensN1TrivialSingleSolution)
{
  NQueensDomain domain{1};
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<NQueensDomain> engine(domain, policy);
  SearchSolutionCollector<Depth_First_Backtracking<NQueensDomain>::Solution> collector;

  auto result = engine.search(NQueensState(1), collector);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.solutions_found, 1u);
  EXPECT_EQ(collector.size(), 1u);
  EXPECT_EQ(nqueens_signature(result.best_solution.get().state), "0");
}

// NQueens n=8: realistic problem — should find all 92 solutions.
TEST(StateSearchFramework, NQueensN8FindsAll92Solutions)
{
  NQueensDomain domain{8};
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Depth_First_Backtracking<NQueensDomain> engine(domain, policy);
  SearchSolutionCollector<Depth_First_Backtracking<NQueensDomain>::Solution> collector;

  auto result = engine.search(NQueensState(8), collector);

  ASSERT_TRUE(result.found_solution());
  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.stats.solutions_found, 92u);
  EXPECT_EQ(collector.size(), 92u);
}
