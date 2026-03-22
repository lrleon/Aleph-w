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

/** @file State_Search_IDA_Star.H
 *  @brief IDA* (Iterative Deepening A*) over implicit state spaces.
 *
 *  IDA* combines the memory efficiency of DFS with the optimality guarantee of
 *  A* for admissible heuristics. Each iteration performs a depth-limited DFS
 *  that prunes branches whose estimated total cost f = g + h exceeds the
 *  current threshold. The threshold is set to the minimum f-value that
 *  exceeded the limit in the previous pass. The process repeats until a goal
 *  is found or the space is exhausted.
 *
 *  The domain contract is given by @ref Aleph::IDAStarDomain, which extends
 *  @ref Aleph::BacktrackingDomain with @ref Aleph::HeuristicEvaluator and
 *  @ref Aleph::ActionCostProvider. Admissibility requires `heuristic(state) <= true
 *  cost to goal` for all states.
 *
 *  @author Leandro Rabindranath León
 */

# ifndef STATE_SEARCH_IDA_STAR_H
# define STATE_SEARCH_IDA_STAR_H

#include <limits>
#include <utility>

#include <ah-errors.H>
#include <state_search_common.H>
#include <Backtracking.H>

namespace Aleph {

/** @brief Minimal contract for IDA* domains.
 *
 *  Extends @ref Aleph::BacktrackingDomain with:
 *
 *  - `heuristic(state)` — admissible cost estimate to the nearest goal,
 *  - `cost(state, move)` — non-negative step cost.
 *
 *  Both must expose a `Distance` type (numeric, typically `int` or `double`).
 *  The heuristic must be admissible (never overestimates the true cost) for
 *  IDA\* to return an optimal solution.
 */
template <typename Domain>
concept IDAStarDomain
  = BacktrackingDomain<Domain> and HeuristicEvaluator<Domain> and ActionCostProvider<Domain>;

/** @brief Result of one threshold pass of IDA*.
 *
 *  Each pass is a depth-limited DFS with a fixed f-cost threshold. This struct
 *  records how many states were visited, whether a solution was found and what
 *  the next threshold should be.
 *
 *  @tparam Distance numeric cost type from the domain.
 */
template <typename Distance>
struct IDAStarIteration
{
  Distance threshold = Distance{};       ///< F-cost threshold used for this pass.
  size_t visited_states = 0;             ///< States visited during this pass.
  bool found_solution = false;           ///< True if a goal was reached at or below threshold.
  Distance next_threshold = Distance{};  ///< Minimum f-value that exceeded `threshold`.
};

/** @brief Aggregated result of a complete IDA* run.
 *
 *  Extends @ref SearchResult with per-iteration data and the total optimal
 *  cost once a solution is found.
 *
 *  @tparam Solution solution snapshot type (see @ref SearchSolution).
 *  @tparam Distance numeric cost type from the domain.
 */
template <typename Solution, typename Distance>
struct IDAStarResult : SearchResult<Solution>
{
  using Base = SearchResult<Solution>;
  using Distance_Type = Distance;

  Distance total_cost = Distance{};  ///< Optimal path cost (valid when found_solution()).
  Array<IDAStarIteration<Distance>> iterations;  ///< One entry per completed threshold pass.

  IDAStarResult() = default;
};

namespace ida_star_detail {

/** @brief Outcome of one recursive DFS step inside IDA*. */
template <typename Distance>
struct DFSResult
{
  bool found = false;
  Distance next_bound = std::numeric_limits<Distance>::max();
};

/** @brief Core recursive DFS used by IDA* for a single threshold pass.
 *
 *  Returns `found = true` immediately when a goal is reached. Otherwise
 *  propagates the minimum f-value that exceeded `threshold` upward so the
 *  caller can set the next iteration's threshold.
 */
template <IDAStarDomain Domain, typename Solution, typename OnSolution>
  requires SearchSolutionVisitor<OnSolution, Solution>
DFSResult<typename Domain::Distance> dfs(Domain &domain,
                                         typename Domain::State &state,
                                         SearchPath<typename Domain::Move> &path,
                                         const typename Domain::Distance g,
                                         const typename Domain::Distance threshold,
                                         const size_t depth,
                                         IDAStarResult<Solution, typename Domain::Distance> &result,
                                         OnSolution &on_solution)
{
  using Distance = typename Domain::Distance;

  ++result.stats.visited_states;
  if (depth > result.stats.max_depth_reached)
    result.stats.max_depth_reached = depth;

  const Distance h = domain.heuristic(state);
  const Distance f = g + h;

  if (f > threshold)
    return {false, f};

  if (domain.is_goal(state))
    {
      ++result.stats.solutions_found;
      Solution solution{state, path, depth};
      if (result.best_solution.consider(solution))
        result.total_cost = g;
      const bool keep_searching = on_solution(solution);
      if (not keep_searching)
        result.status = SearchStatus::StoppedOnSolution;
      return {true, threshold};
    }

  if (search_engine_detail::is_terminal_state(domain, state))
    {
      ++result.stats.terminal_states;
      return {false, std::numeric_limits<Distance>::max()};
    }

  if (search_engine_detail::should_prune_state(domain, state, depth))
    {
      ++result.stats.pruned_by_domain;
      return {false, std::numeric_limits<Distance>::max()};
    }

  if (depth >= result.limits.max_depth)
    {
      ++result.stats.pruned_by_depth;
      return {false, std::numeric_limits<Distance>::max()};
    }

  if (result.stats.expanded_states >= result.limits.max_expansions)
    {
      ++result.stats.limit_hits;
      result.status = SearchStatus::LimitReached;
      return {false, std::numeric_limits<Distance>::max()};
    }

  ++result.stats.expanded_states;

  using Move = typename Domain::Move;
  DFSResult<Distance> out{false, std::numeric_limits<Distance>::max()};

  if (depth + 1 > result.limits.max_depth)
    return out;

  (void) domain.for_each_successor(state,
                                   [&](const Move &move) -> bool
  {
    if (result.status == SearchStatus::LimitReached
        or result.status == SearchStatus::StoppedOnSolution)
      return false;

    ++result.stats.generated_successors;
    const Distance step = domain.cost(state, move);
    bool applied = false;
    bool path_appended = false;
    DFSResult<Distance> child;
    try
      {
        domain.apply(state, move);
        applied = true;
        path.append(move);
        path_appended = true;
        child = dfs(domain, state, path, g + step, threshold, depth + 1, result, on_solution);
      }
    catch (...)
      {
        if (path_appended)
          (void) path.remove_last();
        if (applied)
          domain.undo(state, move);
        throw;
      }

    (void) path.remove_last();
    domain.undo(state, move);

    if (child.found)
      {
        out.found = true;
        return false;
      }

    if (result.status == SearchStatus::LimitReached
        or result.status == SearchStatus::StoppedOnSolution)
      return false;

    if (child.next_bound < out.next_bound)
      out.next_bound = child.next_bound;

    return true;
  });

  return out;
}

}  // end namespace ida_star_detail

/** @brief IDA* engine for implicit state spaces with an admissible heuristic.
 *
 *  The engine performs iterative threshold-bounded DFS. Each pass prunes
 *  branches whose estimated total cost f = g + h exceeds the current
 *  threshold. The threshold advances to the minimum cost that exceeded it in
 *  the previous pass.
 *
 *  ### Optimality
 *  With an admissible heuristic, IDA\* guarantees an optimal solution. The
 *  path returned in `result.best_solution.get().path` reaches the goal in
 *  exactly `result.total_cost` cost.
 *
 *  ### Memory
 *  Only the current path from root to the node under expansion is stored
 *  (one `SearchPath` of depth at most `max_depth`). No open/closed lists.
 *
 *  @tparam Domain problem adapter satisfying @ref Aleph::IDAStarDomain.
 */
template <IDAStarDomain Domain>
class IDA_Star_State_Search
{
public:
  /// Type of the problem domain.
  using Domain_Type = Domain;
  /// Concrete search state type.
  using State = typename Domain::State;
  /// Move type.
  using Move = typename Domain::Move;
  /// Numeric distance/cost type.
  using Distance = typename Domain::Distance;
  /// Solution type containing state and path.
  using Solution = SearchSolution<State, Move>;
  /// Aggregated result type for IDA* search.
  using Result = IDAStarResult<Solution, Distance>;

  /** @brief Construct an engine bound to one domain adapter.
   *  @param domain Problem adapter.
   *  @param policy Exploration settings.
   *  @param limits Resource constraints.
   */
  explicit IDA_Star_State_Search(Domain domain, ExplorationPolicy policy = {}, SearchLimits limits = {})
    : domain_(std::move(domain)), policy_(policy), limits_(limits)
  {
    // empty
  }

  /** @brief Read-only access to the bound domain adapter. */
  [[nodiscard]] const Domain &domain() const noexcept
  {
    return domain_;
  }

  /** @brief Mutable access to the bound domain adapter. */
  [[nodiscard]] Domain &domain() noexcept
  {
    return domain_;
  }

  /** @brief Current exploration policy. */
  [[nodiscard]] const ExplorationPolicy &policy() const noexcept
  {
    return policy_;
  }

  /** @brief Current hard limits. */
  [[nodiscard]] const SearchLimits &limits() const noexcept
  {
    return limits_;
  }

  /** @brief Replace the exploration policy for future runs. */
  void set_policy(const ExplorationPolicy &policy) noexcept
  {
    policy_ = policy;
  }

  /** @brief Replace the hard limits for future runs. */
  void set_limits(const SearchLimits &limits) noexcept
  {
    limits_ = limits;
  }

  /** @brief Run IDA* from `initial_state`.
   *
   *  Iterates threshold passes until a goal is found, the space is exhausted
   *  or a hard limit is reached.
   *
   *  @param[in] initial_state Root state to expand. Mutated during search and
   *             restored to its original value before this function returns.
   *  @return @ref IDAStarResult with solution, stats and per-iteration data.
   *
   *  @throws std::invalid_argument if the policy requests Best_First strategy
   *          (IDA\* is DFS-only) or if max_solutions is zero.
   *  @note @p max_depth in @ref SearchLimits limits the depth of each pass.
   */
  [[nodiscard]] Result search(State initial_state)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), on_solution);
  }

  /** @brief Run IDA* and invoke `on_solution` when the goal is reached.
   *
   *  The callback receives a solution snapshot and returns `true` to allow the
   *  search to continue according to the exploration policy and limits, or
   *  `false` to stop immediately. IDA\* is typically used with
   *  stop-at-first-solution. This implementation stops at the first goal
   *  reached within each threshold pass; with non-negative step costs and an
   *  admissible heuristic the first solution encountered is already optimal,
   *  so raising the threshold in later passes cannot produce a cheaper
   *  solution, only potentially explore alternative states subject to the
   *  configured limits.
   */
  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &on_solution)
  {
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "IDA_Star_State_Search only supports depth-first strategy";
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";

    Result result;
    result.policy = policy_;
    result.limits = limits_;

    const auto start_time = SearchClock::now();

    SearchPath<Move> path;
    reserve_search_path(path, limits_);

    Distance threshold = domain_.heuristic(initial_state);
    const Distance inf = std::numeric_limits<Distance>::max();

    for (;;)
      {
        IDAStarIteration<Distance> iteration;
        iteration.threshold = threshold;

        const size_t stats_before = result.stats.visited_states;

        auto [found, next_bound] = ida_star_detail::dfs(
          domain_, initial_state, path, Distance{}, threshold, 0, result, on_solution);

        iteration.visited_states = result.stats.visited_states - stats_before;
        iteration.found_solution = found;
        iteration.next_threshold = found ? threshold : next_bound;
        result.iterations.append(iteration);

        if (result.status == SearchStatus::StoppedOnSolution)
          break;

        if (found)
          {
            if (result.status != SearchStatus::LimitReached)
              result.status = SearchStatus::StoppedOnSolution;
            break;
          }

        if (result.status == SearchStatus::LimitReached)
          break;

        if (next_bound >= inf)
          {
            result.status = SearchStatus::Exhausted;
            break;
          }

        threshold = next_bound;
      }

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
};

/** @brief Convenience wrapper for a one-shot IDA* search. */
template <IDAStarDomain Domain>
[[nodiscard]] auto ida_star_search(Domain domain,
                                   typename Domain::State initial_state,
                                   ExplorationPolicy policy = {},
                                   SearchLimits limits = {})
{
  IDA_Star_State_Search<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state));
}

/** @brief Convenience wrapper for IDA* with a solution callback. */
template <IDAStarDomain Domain, typename OnSolution>
  requires SearchSolutionVisitor<OnSolution, SearchSolution<typename Domain::State, typename Domain::Move>>
[[nodiscard]] auto ida_star_search(Domain domain,
                                   typename Domain::State initial_state,
                                   OnSolution &on_solution,
                                   ExplorationPolicy policy = {},
                                   SearchLimits limits = {})
{
  IDA_Star_State_Search<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), on_solution);
}

}  // end namespace Aleph

# endif  // STATE_SEARCH_IDA_STAR_H
