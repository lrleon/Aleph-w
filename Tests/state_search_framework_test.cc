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

#include <State_Search.H>

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

  size_t leaf_depth = 2;

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

  size_t n = 0;

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

private:
  Array<int> values_;
  Array<int> suffix_remaining_;
  int target_ = 0;
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
