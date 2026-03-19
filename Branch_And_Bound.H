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


/** @file Branch_And_Bound.H
 *  @brief Reusable branch-and-bound engine for implicit optimization spaces.
 *
 *  This module builds on the common state-space search vocabulary and adds the
 *  minimal machinery specific to optimization:
 *
 *  - optimization sense (minimize/maximize),
 *  - optimistic bounds,
 *  - global incumbent management,
 *  - pruning when a node cannot improve the incumbent,
 *  - depth-first and best-first exploration strategies,
 *  - optional bound-guided child ordering for depth-first exploration.
 *
 *  @author Leandro Rabindranath León
 */

#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

#include <concepts>
#include <utility>

#include <ah-errors.H>
#include <state_search_common.H>
#include <tpl_dynBinHeap.H>

namespace Aleph
{

/** @brief Optimization direction supported by branch and bound. */
enum class OptimizationSense
{
  Minimize, ///< Smaller objective values are better.
  Maximize  ///< Larger objective values are better.
};

/** @brief Objective policy for maximization problems. */
template <std::totally_ordered Value>
struct Maximize_Objective
{
  using Value_Type = Value;
  static constexpr OptimizationSense sense = OptimizationSense::Maximize;

  bool better(const Value &candidate, const Value &incumbent) const noexcept
  {
    return candidate > incumbent;
  }

  bool can_improve(const Value &bound, const Value &incumbent) const noexcept
  {
    return bound > incumbent;
  }

  bool more_promising(const Value &lhs, const Value &rhs) const noexcept
  {
    return lhs > rhs;
  }
};

/** @brief Objective policy for minimization problems. */
template <std::totally_ordered Value>
struct Minimize_Objective
{
  using Value_Type = Value;
  static constexpr OptimizationSense sense = OptimizationSense::Minimize;

  bool better(const Value &candidate, const Value &incumbent) const noexcept
  {
    return candidate < incumbent;
  }

  bool can_improve(const Value &bound, const Value &incumbent) const noexcept
  {
    return bound < incumbent;
  }

  bool more_promising(const Value &lhs, const Value &rhs) const noexcept
  {
    return lhs < rhs;
  }
};

/** @brief Concept for objective policies used by branch and bound. */
template <typename Policy, typename Value>
concept BranchAndBoundObjectivePolicy =
  requires(const Policy &policy, const Value &a, const Value &b)
  {
    { policy.better(a, b) } -> std::convertible_to<bool>;
    { policy.can_improve(a, b) } -> std::convertible_to<bool>;
    { policy.more_promising(a, b) } -> std::convertible_to<bool>;
  };

/** @brief Snapshot of a complete optimization solution. */
template <SearchState State, SearchMove Move, std::totally_ordered Objective>
struct OptimizationSolution : SearchSolution<State, Move>
{
  using Objective_Type = Objective;

  Objective objective_value = Objective{}; ///< Objective value of the complete solution.
};

/** @brief Compare two optimization solutions by objective quality. */
template <typename Solution, typename ObjectivePolicy>
struct PreferBetterObjective
{
  ObjectivePolicy objective;

  bool operator()(const Solution &lhs, const Solution &rhs) const noexcept
  {
    return objective.better(lhs.objective_value, rhs.objective_value);
  }
};

/** @brief Global incumbent manager for branch and bound. */
template <typename Solution, typename ObjectivePolicy>
class ObjectiveIncumbent
{
public:
  using Solution_Type = Solution;
  using Objective_Type = typename Solution::Objective_Type;
  using Objective_Policy = ObjectivePolicy;
  using Compare_Type = PreferBetterObjective<Solution, ObjectivePolicy>;

  ObjectiveIncumbent() = default;

  explicit ObjectiveIncumbent(ObjectivePolicy objective)
    : objective_(std::move(objective)),
      best_(Compare_Type{objective_})
  {
    // empty
  }

  [[nodiscard]] bool has_value() const noexcept
  {
    return best_.has_value();
  }

  [[nodiscard]] const Solution &get() const
  {
    return best_.get();
  }

  [[nodiscard]] const Objective_Type &best_value() const
  {
    return get().objective_value;
  }

  [[nodiscard]] const ObjectivePolicy &objective() const noexcept
  {
    return objective_;
  }

  bool consider(const Solution &solution)
  {
    return best_.consider(solution);
  }

  bool consider(Solution &&solution)
  {
    return best_.consider(std::move(solution));
  }

  [[nodiscard]] bool can_improve(const Objective_Type &bound) const noexcept
  {
    return not has_value() or objective_.can_improve(bound, best_value());
  }

private:
  ObjectivePolicy objective_ = {};
  BestSolution<Solution, Compare_Type> best_;
};

/** @brief Branch-and-bound specific statistics. */
struct BranchAndBoundStats : SearchStats
{
  size_t pruned_by_bound = 0;  ///< Nodes pruned because their bound cannot beat the incumbent.
  size_t incumbent_updates = 0; ///< Number of times the incumbent improved.
};

/** @brief Result of a branch-and-bound execution. */
template <typename Solution, typename ObjectivePolicy>
struct BranchAndBoundResult
{
  using Solution_Type = Solution;
  using Objective_Policy = ObjectivePolicy;
  using Incumbent_Type = ObjectiveIncumbent<Solution, ObjectivePolicy>;

  SearchStatus status = SearchStatus::NotStarted; ///< Final execution status.
  ExplorationPolicy policy; ///< Exploration policy used during the run.
  SearchLimits limits;      ///< Hard limits used during the run.
  BranchAndBoundStats stats; ///< Collected branch-and-bound statistics.
  Incumbent_Type incumbent; ///< Global incumbent for the run.

  BranchAndBoundResult() = default;

  explicit BranchAndBoundResult(ObjectivePolicy objective)
    : incumbent(std::move(objective))
  {
    // empty
  }

  [[nodiscard]] bool found_solution() const noexcept
  {
    return incumbent.has_value();
  }

  [[nodiscard]] bool exhausted() const noexcept
  {
    return status == SearchStatus::Exhausted;
  }

  [[nodiscard]] bool stopped_on_solution() const noexcept
  {
    return status == SearchStatus::StoppedOnSolution;
  }

  [[nodiscard]] bool limit_reached() const noexcept
  {
    return status == SearchStatus::LimitReached;
  }
};

/** @brief Concept for complete-solution predicates in optimization domains. */
template <typename Domain>
concept CompleteSolutionPredicate =
  requires(Domain &domain, const typename Domain::State &state)
  {
    { domain.is_complete(state) } -> std::convertible_to<bool>;
  };

/** @brief Concept for domains that provide an objective and an optimistic bound. */
template <typename Domain>
concept OptimizationEvaluator =
  requires(Domain &domain, const typename Domain::State &state)
  {
    typename Domain::Objective;
    { domain.objective_value(state) } -> std::convertible_to<typename Domain::Objective>;
    { domain.bound(state) } -> std::convertible_to<typename Domain::Objective>;
  };

/** @brief Minimal contract for branch-and-bound domains. */
template <typename Domain>
concept BranchAndBoundDomain =
  SuccessorGenerator<Domain> and CompleteSolutionPredicate<Domain> and
  OptimizationEvaluator<Domain> and
  requires(Domain &domain,
           typename Domain::State &state,
           const typename Domain::Move &move)
  {
    requires SearchState<typename Domain::State>;
    requires SearchMove<typename Domain::Move>;
    { domain.apply(state, move) } -> std::same_as<void>;
    { domain.undo(state, move) } -> std::same_as<void>;
  };

namespace branch_and_bound_detail
{

template <typename Domain>
[[nodiscard]] bool is_terminal_state(const Domain &domain,
                                     const typename Domain::State &state)
{
  if constexpr (TerminalPredicate<Domain>)
    return domain.is_terminal(state);
  else
    return false;
}

template <typename Domain>
[[nodiscard]] bool should_prune_state(Domain &domain,
                                      const typename Domain::State &state,
                                      const size_t depth)
{
  if constexpr (DomainPruner<Domain>)
    return domain.should_prune(state, depth);
  else
    return false;
}

} // end namespace branch_and_bound_detail

/** @brief Reusable branch-and-bound engine over implicit state spaces. */
template <BranchAndBoundDomain Domain,
          typename ObjectivePolicy = Maximize_Objective<typename Domain::Objective>>
  requires BranchAndBoundObjectivePolicy<ObjectivePolicy, typename Domain::Objective>
class Branch_And_Bound
{
public:
  using Domain_Type = Domain;
  using State = typename Domain::State;
  using Move = typename Domain::Move;
  using Objective = typename Domain::Objective;
  using Solution = OptimizationSolution<State, Move, Objective>;
  using Incumbent = ObjectiveIncumbent<Solution, ObjectivePolicy>;
  using Result = BranchAndBoundResult<Solution, ObjectivePolicy>;

private:
  struct FrontierNode
  {
    State state;
    SearchPath<Move> path;
    size_t depth = 0;
    Objective bound_value = Objective{};
  };

  struct FrontierCompare
  {
    ObjectivePolicy objective;

    bool operator()(const FrontierNode &lhs, const FrontierNode &rhs) const noexcept
    {
      if (objective.more_promising(lhs.bound_value, rhs.bound_value))
        return true;

      if (objective.more_promising(rhs.bound_value, lhs.bound_value))
        return false;

      return lhs.depth < rhs.depth;
    }
  };

  using Frontier = DynBinHeap<FrontierNode, FrontierCompare>;

public:
  /** @brief Return the default branch-and-bound exploration policy. */
  [[nodiscard]] static ExplorationPolicy default_policy() noexcept
  {
    ExplorationPolicy policy;
    policy.strategy = ExplorationPolicy::Strategy::Depth_First;
    policy.stop_at_first_solution = false;
    return policy;
  }

  /** @brief Build a branch-and-bound engine bound to one optimization domain. */
  explicit Branch_And_Bound(Domain domain,
                            ExplorationPolicy policy = default_policy(),
                            SearchLimits limits = {},
                            ObjectivePolicy objective = {})
    : domain_(std::move(domain)),
      policy_(policy),
      limits_(limits),
      objective_(std::move(objective))
  {
    // empty
  }

  [[nodiscard]] const Domain &domain() const noexcept
  {
    return domain_;
  }

  [[nodiscard]] Domain &domain() noexcept
  {
    return domain_;
  }

  [[nodiscard]] const ExplorationPolicy &policy() const noexcept
  {
    return policy_;
  }

  [[nodiscard]] const SearchLimits &limits() const noexcept
  {
    return limits_;
  }

  [[nodiscard]] const ObjectivePolicy &objective_policy() const noexcept
  {
    return objective_;
  }

  void set_policy(const ExplorationPolicy &policy) noexcept
  {
    policy_ = policy;
  }

  void set_limits(const SearchLimits &limits) noexcept
  {
    limits_ = limits;
  }

  /** @brief Execute branch and bound and keep only the incumbent. */
  [[nodiscard]] Result search(State initial_state)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), on_solution);
  }

  /** @brief Execute branch and bound with a callback/collector per solution. */
  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &on_solution)
  {
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";
    validate_ordering_configuration();

    Result result(objective_);
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    switch (policy_.strategy)
      {
      case ExplorationPolicy::Strategy::Depth_First:
        search_depth_first(initial_state, result, on_solution);
        break;

      case ExplorationPolicy::Strategy::Best_First:
        search_best_first(std::move(initial_state), result, on_solution);
        break;

      default:
        ah_invalid_argument_if(true)
          << "Branch_And_Bound received an unsupported exploration strategy";
      }

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &&on_solution)
  {
    auto handler = std::forward<OnSolution>(on_solution);
    return search(std::move(initial_state), handler);
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
  ObjectivePolicy objective_;

  [[nodiscard]] bool ordering_active_for_depth_first() const noexcept
  {
    return policy_.move_ordering == MoveOrderingMode::Estimated_Bound;
  }

  void validate_ordering_configuration() const
  {
    ah_invalid_argument_if(policy_.move_ordering == MoveOrderingMode::Estimated_Score)
      << "Branch_And_Bound does not support MoveOrderingMode::Estimated_Score";
    ah_invalid_argument_if(policy_.use_killer_moves)
      << "Branch_And_Bound does not use killer heuristics";
    ah_invalid_argument_if(policy_.use_history_heuristic)
      << "Branch_And_Bound does not use history heuristics";
  }

  [[nodiscard]] bool stop_after_solution(Result &result) const
  {
    if (policy_.stop_at_first_solution)
      {
        result.status = SearchStatus::StoppedOnSolution;
        return true;
      }

    if (limits_.max_solutions != Search_Unlimited and
        result.stats.solutions_found >= limits_.max_solutions)
      {
        ++result.stats.limit_hits;
        result.status = SearchStatus::LimitReached;
        return true;
      }

    return false;
  }

  [[nodiscard]] bool expansion_limit_reached(Result &result) const
  {
    if (result.stats.expanded_states >= limits_.max_expansions)
      {
        ++result.stats.limit_hits;
        result.status = SearchStatus::LimitReached;
        return true;
      }

    return false;
  }

  static void register_visit(const size_t depth, Result &result)
  {
    ++result.stats.visited_states;
    if (depth > result.stats.max_depth_reached)
      result.stats.max_depth_reached = depth;
  }

  [[nodiscard]] Array<RankedMove<Move, Objective>>
  collect_ordered_moves(State &state,
                        Result &result)
  {
    Array<RankedMove<Move, Objective>> moves;
    size_t ordinal = 0;

    (void) domain_.for_each_successor(
      state,
      [&](const Move &move) -> bool
      {
        RankedMove<Move, Objective> ranked;
        ranked.move = move;
        ranked.ordinal = ordinal++;
        domain_.apply(state, move);
        ranked.priority = domain_.bound(state);
        domain_.undo(state, move);
        ++result.stats.move_ordering.priority_estimates;
        moves.append(std::move(ranked));
        return true;
      });

    if (not moves.is_empty())
      {
        ++result.stats.move_ordering.ordered_batches;
        result.stats.move_ordering.ordered_moves += moves.size();
      }

    sort_ranked_moves(
      moves,
      [this](const Objective &lhs, const Objective &rhs) noexcept
      {
        return objective_.more_promising(lhs, rhs);
      },
      false,
      false);

    return moves;
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] bool handle_complete_solution(const State &state,
                                              const SearchPath<Move> &path,
                                              const size_t depth,
                                              Result &result,
                                              OnSolution &on_solution)
  {
    ++result.stats.solutions_found;
    Solution solution;
    solution.state = state;
    solution.path = path;
    solution.depth = depth;
    solution.objective_value = domain_.objective_value(state);

    if (result.incumbent.consider(solution))
      ++result.stats.incumbent_updates;

    if (not on_solution(solution))
      {
        result.status = SearchStatus::StoppedOnSolution;
        return true;
      }

    return stop_after_solution(result);
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] bool dfs(State &state,
                         SearchPath<Move> &path,
                         const size_t depth,
                         Result &result,
                         OnSolution &on_solution)
  {
    register_visit(depth, result);

    if (domain_.is_complete(state))
      return handle_complete_solution(state, path, depth, result, on_solution);

    if (branch_and_bound_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (branch_and_bound_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }

    const Objective bound_value = domain_.bound(state);
    if (not result.incumbent.can_improve(bound_value))
      {
        ++result.stats.pruned_by_bound;
        return false;
      }

    if (expansion_limit_reached(result))
      return true;

    ++result.stats.expanded_states;

    bool stop = false;

    auto explore_move = [&](const Move &move) -> bool
    {
      ++result.stats.generated_successors;
      path.append(move);
      domain_.apply(state, move);
      stop = dfs(state, path, depth + 1, result, on_solution);
      domain_.undo(state, move);
      (void) path.remove_last();
      return not stop;
    };

    if (ordering_active_for_depth_first())
      {
        auto ordered_moves = collect_ordered_moves(state, result);
        for (const auto &ranked : ordered_moves)
          if (not explore_move(ranked.move))
            break;
      }
    else
      {
        (void) domain_.for_each_successor(
          state,
          [&](const Move &move) -> bool
          {
            return explore_move(move);
          });
      }

    return stop;
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  void search_depth_first(State &initial_state,
                          Result &result,
                          OnSolution &on_solution)
  {
    SearchPath<Move> path;
    reserve_search_path(path, limits_);
    (void) dfs(initial_state, path, 0, result, on_solution);
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] bool process_best_first_candidate(State state,
                                                  SearchPath<Move> path,
                                                  const size_t depth,
                                                  Result &result,
                                                  Frontier &frontier,
                                                  OnSolution &on_solution)
  {
    register_visit(depth, result);

    if (domain_.is_complete(state))
      return handle_complete_solution(state, path, depth, result, on_solution);

    if (branch_and_bound_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (branch_and_bound_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }

    const Objective bound_value = domain_.bound(state);
    if (not result.incumbent.can_improve(bound_value))
      {
        ++result.stats.pruned_by_bound;
        return false;
      }

    frontier.put(FrontierNode{std::move(state), std::move(path), depth, bound_value});
    return false;
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  void search_best_first(State initial_state,
                         Result &result,
                         OnSolution &on_solution)
  {
    Frontier frontier(FrontierCompare{objective_});
    SearchPath<Move> root_path;
    reserve_search_path(root_path, limits_);
    if (process_best_first_candidate(std::move(initial_state), std::move(root_path),
                                     0, result, frontier, on_solution))
      return;

    while (not frontier.is_empty())
      {
        if (result.incumbent.has_value() and
            not result.incumbent.can_improve(frontier.top().bound_value))
          {
            result.stats.pruned_by_bound += frontier.size();
            frontier.empty();
            return;
          }

        if (expansion_limit_reached(result))
          return;

        FrontierNode node = frontier.getMin();
        ++result.stats.expanded_states;

        bool stop = false;
        (void) domain_.for_each_successor(
          node.state,
          [&](const Move &move) -> bool
          {
            ++result.stats.generated_successors;
            State child_state = node.state;
            SearchPath<Move> child_path = node.path;
            child_path.reserve(node.path.size() + 1);
            child_path.append(move);
            domain_.apply(child_state, move);
            stop = process_best_first_candidate(std::move(child_state),
                                                std::move(child_path),
                                                node.depth + 1,
                                                result,
                                                frontier,
                                                on_solution);
            return not stop;
          });

        if (stop)
          return;
      }
  }
};

/** @brief Convenience wrapper for one-shot branch and bound. */
template <BranchAndBoundDomain Domain,
          typename ObjectivePolicy = Maximize_Objective<typename Domain::Objective>>
  requires BranchAndBoundObjectivePolicy<ObjectivePolicy, typename Domain::Objective>
[[nodiscard]] auto branch_and_bound_search(Domain domain,
                                           typename Domain::State initial_state,
                                           ExplorationPolicy policy = Branch_And_Bound<Domain, ObjectivePolicy>::default_policy(),
                                           SearchLimits limits = {},
                                           ObjectivePolicy objective = {})
{
  Branch_And_Bound<Domain, ObjectivePolicy> engine(std::move(domain),
                                                   policy,
                                                   limits,
                                                   std::move(objective));
  return engine.search(std::move(initial_state));
}

/** @brief Convenience wrapper for branch and bound with a solution callback. */
template <BranchAndBoundDomain Domain,
          typename OnSolution,
          typename ObjectivePolicy = Maximize_Objective<typename Domain::Objective>>
  requires BranchAndBoundObjectivePolicy<ObjectivePolicy, typename Domain::Objective> and
           SearchSolutionVisitor<OnSolution,
                                 OptimizationSolution<typename Domain::State,
                                                      typename Domain::Move,
                                                      typename Domain::Objective>>
[[nodiscard]] auto branch_and_bound_search(Domain domain,
                                           typename Domain::State initial_state,
                                           OnSolution &on_solution,
                                           ExplorationPolicy policy = Branch_And_Bound<Domain, ObjectivePolicy>::default_policy(),
                                           SearchLimits limits = {},
                                           ObjectivePolicy objective = {})
{
  Branch_And_Bound<Domain, ObjectivePolicy> engine(std::move(domain),
                                                   policy,
                                                   limits,
                                                   std::move(objective));
  return engine.search(std::move(initial_state), on_solution);
}

} // end namespace Aleph

#endif // BRANCH_AND_BOUND_H
