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
#include <Transposition_Table.H>
#include <tpl_dynBinHeap.H>

namespace Aleph {

/** @brief Optimization direction supported by branch and bound. */
enum class OptimizationSense
{
  Minimize,  ///< Smaller objective values are better.
  Maximize   ///< Larger objective values are better.
};

/** @brief Objective policy for maximization problems. */
template <std::totally_ordered Value>
struct Maximize_Objective
{
  using Value_Type = Value;
  static constexpr OptimizationSense sense = OptimizationSense::Maximize;

  static bool better(const Value &candidate, const Value &incumbent) noexcept
  {
    return candidate > incumbent;
  }

  static bool can_improve(const Value &bound, const Value &incumbent) noexcept
  {
    return bound > incumbent;
  }

  static bool more_promising(const Value &lhs, const Value &rhs) noexcept
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

  static bool better(const Value &candidate, const Value &incumbent) noexcept
  {
    return candidate < incumbent;
  }

  static bool can_improve(const Value &bound, const Value &incumbent) noexcept
  {
    return bound < incumbent;
  }

  static bool more_promising(const Value &lhs, const Value &rhs) noexcept
  {
    return lhs < rhs;
  }
};

/** @brief Concept for objective policies used by branch and bound. */
template <typename Policy, typename Value>
concept BranchAndBoundObjectivePolicy
  = requires(const Policy &policy, const Value &a, const Value &b) {
      { policy.better(a, b) } -> std::convertible_to<bool>;
      { policy.can_improve(a, b) } -> std::convertible_to<bool>;
      { policy.more_promising(a, b) } -> std::convertible_to<bool>;
    };

/** @brief Snapshot of a complete optimization solution. */
template <SearchState State, SearchMove Move, std::totally_ordered Objective>
struct OptimizationSolution : SearchSolution<State, Move>
{
  using Objective_Type = Objective;

  Objective objective_value = Objective{};  ///< Objective value of the complete solution.
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
    : objective_(std::move(objective)), best_(Compare_Type{objective_})
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
  size_t pruned_by_bound = 0;    ///< Nodes pruned because their bound cannot beat the incumbent.
  size_t incumbent_updates = 0;  ///< Number of times the incumbent improved.
};

/** @brief Result of a branch-and-bound execution. */
template <typename Solution, typename ObjectivePolicy>
struct BranchAndBoundResult
{
  using Solution_Type = Solution;
  using Objective_Policy = ObjectivePolicy;
  using Incumbent_Type = ObjectiveIncumbent<Solution, ObjectivePolicy>;

  SearchStatus status = SearchStatus::NotStarted;  ///< Final execution status.
  ExplorationPolicy policy;                        ///< Exploration policy used during the run.
  SearchLimits limits;                             ///< Hard limits used during the run.
  BranchAndBoundStats stats;                       ///< Collected branch-and-bound statistics.
  Incumbent_Type incumbent;                        ///< Global incumbent for the run.

  BranchAndBoundResult() = default;

  explicit BranchAndBoundResult(ObjectivePolicy objective) : incumbent(std::move(objective))
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
concept CompleteSolutionPredicate = requires(Domain &domain, const typename Domain::State &state) {
  { domain.is_complete(state) } -> std::convertible_to<bool>;
};

/** @brief Concept for domains that provide an objective and an optimistic bound. */
template <typename Domain>
concept OptimizationEvaluator = requires(Domain &domain, const typename Domain::State &state) {
  typename Domain::Objective;
  { domain.objective_value(state) } -> std::convertible_to<typename Domain::Objective>;
  { domain.bound(state) } -> std::convertible_to<typename Domain::Objective>;
};

/** @brief Minimal contract for branch-and-bound domains.
 *
 *  The engine assumes a mutable search state and reversible moves:
 *
 *  - `for_each_successor(state, visitor)` lazily emits candidate moves,
 *  - `apply(state, move)` advances the state.
 *    @post If `apply()` throws, the state must remain unchanged (strong exception safety).
 *  - `undo(state, move)` restores the previous state.
 *    @post `undo()` should not throw exceptions; it is typically called during
 *          normal backtracking or exception stack unwinding.
 *  - `is_complete(state)` marks an accepted solution,
 *  - `bound(state)` returns an optimistic estimate of the final objective.
 */
template <typename Domain>
concept BranchAndBoundDomain
  = SuccessorGenerator<Domain> and CompleteSolutionPredicate<Domain> and OptimizationEvaluator<Domain>
and requires(Domain &domain, typename Domain::State &state, const typename Domain::Move &move) {
      requires SearchState<typename Domain::State>;
      requires SearchMove<typename Domain::Move>;
      { domain.apply(state, move) } -> std::same_as<void>;
      { domain.undo(state, move) } -> std::same_as<void>;
    };

/** @brief Reusable branch-and-bound engine over implicit state spaces.
 *
 *  @tparam Domain Problem adapter exposing `bound(state)`, `is_complete(state)`
 *          and `objective_value(state)`.
 *  @tparam ObjectivePolicy Policy used to compare solutions (@ref Maximize_Objective
 *          or @ref Minimize_Objective).
 */
template <BranchAndBoundDomain Domain,
          typename ObjectivePolicy = Maximize_Objective<typename Domain::Objective>>
  requires BranchAndBoundObjectivePolicy<ObjectivePolicy, typename Domain::Objective>
class Branch_And_Bound
{
public:
  /// Type of the problem domain.
  using Domain_Type = Domain;
  /// Concrete search state type.
  using State = typename Domain::State;
  /// Move type.
  using Move = typename Domain::Move;
  /// Type of the metric being optimized.
  using Objective = typename Domain::Objective;
  /// Concrete solution type with objective value.
  using Solution = OptimizationSolution<State, Move, Objective>;
  /// Tracker for the best solution seen so far.
  using Incumbent = ObjectiveIncumbent<Solution, ObjectivePolicy>;
  /// Aggregated result type for optimization runs.
  using Result = BranchAndBoundResult<Solution, ObjectivePolicy>;

  /** @brief Compile-time marker: Branch_And_Bound supports both Depth_First
   *         and Best_First strategies.
   *
   *  Unlike @ref Aleph::Negamax and @ref Aleph::Alpha_Beta (which only
   *  support Depth_First), this engine also accepts
   *  `ExplorationPolicy::Strategy::Best_First`.  Check this constant
   *  alongside those engines' `supports_best_first = false` to detect
   *  strategy mismatches at compile time.
   */
  static constexpr bool supports_best_first = true;

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
                            const SearchLimits &limits = {},
                            ObjectivePolicy objective = {})
    : domain_(std::move(domain)), policy_(policy), limits_(limits), objective_(std::move(objective))
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

  /** @brief Branch and bound with a visited-bound map (DFS only).
   *
   *  Before expanding a node, checks whether the same state was already
   *  visited with an equal-or-better optimistic bound. If so, the current
   *  branch is pruned; otherwise the stored bound is updated. This eliminates
   *  redundant sub-tree explorations when the same state is reachable via
   *  multiple paths.
   *
   *  The domain must expose `state_key(state)` (see @ref Aleph::SearchStateKeyProvider).
   *
   *  @param[in] initial_state Mutable root state.
   *  @param[in,out] visited_map Caller-owned map from state key to best bound.
   *  @return Search result with statistics and best retained solution.
   */
  template <typename VisitedMap>
    requires SearchStateKeyProvider<Domain>
  [[nodiscard]] Result search(State initial_state, VisitedMap &visited_map)
  {
    using State_Key = typename Domain::State_Key;
    static_assert(VisitedBoundMap<VisitedMap, State_Key, Objective>,
                  "Visited map must satisfy VisitedBoundMap concept");

    ContinueSearch on_solution;
    return search(std::move(initial_state), visited_map, on_solution);
  }

  /** @brief Branch and bound with visited-bound map and solution callback. */
  template <typename VisitedMap, typename OnSolution>
    requires SearchStateKeyProvider<Domain> and SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, VisitedMap &visited_map, OnSolution &on_solution)
  {
    using State_Key = typename Domain::State_Key;
    static_assert(VisitedBoundMap<VisitedMap, State_Key, Objective>,
                  "Visited map must satisfy VisitedBoundMap concept");

    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "Visited-map search only supports Depth_First strategy";
    validate_ordering_configuration();

    Result result(objective_);
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    SearchPath<Move> path;
    reserve_search_path(path, limits_);
    (void) dfs_visited(initial_state, path, 0, result, on_solution, visited_map);

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
  ObjectivePolicy objective_;

  /** @brief Apply one move, recurse, then undo — shared by `dfs` and `dfs_visited`.
   *
   *  Captures the common try/catch pattern that protects `undo` and the path
   *  rollback against exceptions thrown during `apply`, recursion or tracing.
   *
   *  @tparam Recurse  Callable `() -> bool` that performs the recursive descent
   *                   after the move has been applied and appended to `path`.
   *  @param  move     Move to apply and undo.
   *  @param  state    Mutable search state (modified in-place).
   *  @param  path     Current move path (extended and restored in-place).
   *  @param  result   Result object whose statistics are updated in-place.
   *  @param  stop     Out-parameter set by the recursive call; `true` signals
   *                   an early stop propagated upward.
   *  @param  recurse  Callable invoked after `apply`; its return value is
   *                   stored in `stop`.
   *  @return `true` to continue sibling enumeration; `false` to stop.
   */
  template <typename Recurse>
  [[nodiscard]] bool apply_move_and_recurse(const Move &move,
                                            State &state,
                                            SearchPath<Move> &path,
                                            Result &result,
                                            bool &stop,
                                            Recurse &&recurse)
  {
    ++result.stats.generated_successors;
    bool applied = false;
    bool path_appended = false;
    try
      {
        domain_.apply(state, move);
        applied = true;
        path.append(move);
        path_appended = true;
        stop = std::forward<Recurse>(recurse)();
      }
    catch (...)
      {
        if (path_appended)
          (void) path.remove_last();
        if (applied)
          domain_.undo(state, move);
        throw;
      }
    (void) path.remove_last();
    domain_.undo(state, move);
    return not stop;
  }

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
    return search_engine_detail::stop_after_solution(result, policy_, limits_);
  }

  [[nodiscard]] bool expansion_limit_reached(Result &result) const
  {
    return search_engine_detail::expansion_limit_reached(result, limits_);
  }

  static void register_visit(const size_t depth, Result &result)
  {
    search_engine_detail::register_visit(depth, result);
  }

  [[nodiscard]] Array<RankedMove<Move, Objective>> collect_ordered_moves(State &state, Result &result)
  {
    Array<RankedMove<Move, Objective>> moves;
    size_t ordinal = 0;

    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      RankedMove<Move, Objective> ranked;
      ranked.move = move;
      ranked.ordinal = ordinal++;

      if constexpr (IncrementalBoundProvider<Domain>)
        {
          // Fast path: compute the ordering bound without apply/undo.
          ranked.priority = domain_.bound_after(state, move);
        }
      else
        {
          bool applied = false;
          try
            {
              domain_.apply(state, move);
              applied = true;
              ranked.priority = domain_.bound(state);
            }
          catch (...)
            {
              if (applied)
                domain_.undo(state, move);
              throw;
            }
          domain_.undo(state, move);
        }

      ++result.stats.move_ordering.priority_estimates;
      moves.append(std::move(ranked));
      return true;
    });

    if (not moves.is_empty())
      {
        ++result.stats.move_ordering.ordered_batches;
        result.stats.move_ordering.ordered_moves += moves.size();
      }

    sort_ranked_moves(moves,
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

    if (search_engine_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (search_engine_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }

    if (const Objective bound_value = domain_.bound(state);
        not result.incumbent.can_improve(bound_value))
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
      return apply_move_and_recurse(move,
                                    state,
                                    path,
                                    result,
                                    stop,
                                    [&]()
      {
        return dfs(state, path, depth + 1, result, on_solution);
      });
    };

    if (ordering_active_for_depth_first())
      {
        for (auto ordered_moves = collect_ordered_moves(state, result);
             const auto &ranked : ordered_moves)
          if (not explore_move(ranked.move))
            break;
      }
    else
      (void) domain_.for_each_successor(state,
                                        [&](const Move &move) -> bool
      {
        return explore_move(move);
      });

    return stop;
  }

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  void search_depth_first(State &initial_state, Result &result, OnSolution &on_solution)
  {
    SearchPath<Move> path;
    reserve_search_path(path, limits_);
    (void) dfs(initial_state, path, 0, result, on_solution);
  }

  template <typename OnSolution, typename VisitedMap>
    requires SearchSolutionVisitor<OnSolution, Solution> and SearchStateKeyProvider<Domain>
  [[nodiscard]] bool dfs_visited(State &state,
                                 SearchPath<Move> &path,
                                 const size_t depth,
                                 Result &result,
                                 OnSolution &on_solution,
                                 VisitedMap &visited)
  {
    using State_Key = typename Domain::State_Key;
    static_assert(VisitedBoundMap<VisitedMap, State_Key, Objective>,
                  "Visited map must satisfy VisitedBoundMap concept");

    register_visit(depth, result);

    if (domain_.is_complete(state))
      return handle_complete_solution(state, path, depth, result, on_solution);

    if (search_engine_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (search_engine_detail::should_prune_state(domain_, state, depth))
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

    const auto key = domain_.state_key(state);
    Objective old_visited_bound;
    bool was_inserted = false;
    if (auto *pair = visited.search(key); pair != nullptr)
      {
        if (not objective_.more_promising(bound_value, pair->second))
          {
            ++result.stats.pruned_by_bound;
            return false;
          }
        old_visited_bound = pair->second;
        pair->second = bound_value;
      }
    else
      {
        visited.insert(key, bound_value);
        was_inserted = true;
      }

    auto rollback_visited = [&]()
    {
      if (was_inserted)
        visited.remove(key);
      else if (auto *pair = visited.search(key); pair != nullptr)
        pair->second = old_visited_bound;
    };

    if (expansion_limit_reached(result))
      return true;

    ++result.stats.expanded_states;

    bool stop = false;

    try
      {
        auto explore_move = [&](const Move &move) -> bool
        {
          return apply_move_and_recurse(move,
                                        state,
                                        path,
                                        result,
                                        stop,
                                        [&]()
          {
            return dfs_visited(state, path, depth + 1, result, on_solution, visited);
          });
        };

        if (ordering_active_for_depth_first())
          {
            for (auto ordered_moves = collect_ordered_moves(state, result);
                 const auto &ranked : ordered_moves)
              if (not explore_move(ranked.move))
                break;
          }
        else
          (void) domain_.for_each_successor(state,
                                            [&](const Move &move) -> bool
          {
            return explore_move(move);
          });
      }
    catch (...)
      {
        rollback_visited();
        throw;
      }

    return stop;
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

    if (search_engine_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (search_engine_detail::should_prune_state(domain_, state, depth))
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
  void search_best_first(State initial_state, Result &result, OnSolution &on_solution)
  {
    Frontier frontier(FrontierCompare{objective_});
    SearchPath<Move> root_path;
    reserve_search_path(root_path, limits_);
    if (process_best_first_candidate(
          std::move(initial_state), std::move(root_path), 0, result, frontier, on_solution))
      return;

    while (not frontier.is_empty())
      {
        if (result.incumbent.has_value()
            and not result.incumbent.can_improve(frontier.top().bound_value))
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
        (void) domain_.for_each_successor(node.state,
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
[[nodiscard]] auto branch_and_bound_search(
  Domain domain,
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
  requires BranchAndBoundObjectivePolicy<ObjectivePolicy, typename Domain::Objective>
       and SearchSolutionVisitor<
             OnSolution,
             OptimizationSolution<typename Domain::State, typename Domain::Move, typename Domain::Objective>>
[[nodiscard]] auto branch_and_bound_search(
  Domain domain,
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

}  // end namespace Aleph

#endif  // BRANCH_AND_BOUND_H
