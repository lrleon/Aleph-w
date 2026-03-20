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

/** @file Backtracking.H
 *  @brief Minimal depth-first backtracking engine for implicit state spaces.
 *
 *  This first version focuses on a single reusable kernel:
 *
 *  - mutable state plus `apply()` / `undo()`,
 *  - lazy successor generation via `for_each_successor()`,
 *  - optional early stop on the first solution,
 *  - reusable limits and statistics from @ref state_search_common.H.
 *
 *  The engine is intentionally small, but already usable as a base layer for
 *  exact search problems and for future branch-and-bound extensions.
 *
 *  @author Leandro Rabindranath León
 */

#ifndef BACKTRACKING_H
#define BACKTRACKING_H

#include <utility>

#include <ah-errors.H>
#include <state_search_common.H>
#include <Transposition_Table.H>

namespace Aleph {

namespace state_search_detail {

/** @brief Default incumbent ordering for DFS solutions.
 *
 *  The first criterion is minimum depth. Ties keep the earlier incumbent.
 */
template <SearchState State, SearchMove Move>
struct PreferShallowerSolution
{
  bool operator()(const SearchSolution<State, Move> &lhs,
                  const SearchSolution<State, Move> &rhs) const noexcept
  {
    return lhs.depth < rhs.depth;
  }
};

/** @brief Dispatch helper for the optional `is_terminal` hook.
 *
 *  Returns `true` if `Domain` satisfies @ref Aleph::TerminalPredicate and the domain
 *  reports the state as terminal (a non-goal dead end). Returns `false`
 *  unconditionally when the domain does not expose `is_terminal`.
 */
template <typename Domain>
[[nodiscard]] bool is_terminal_state(const Domain &domain, const typename Domain::State &state)
{
  if constexpr (TerminalPredicate<Domain>)
    return domain.is_terminal(state);
  else
    return false;
}

/** @brief Dispatch helper for the optional `should_prune` hook.
 *
 *  Returns `true` if `Domain` satisfies @ref Aleph::DomainPruner and the domain
 *  requests early pruning of the current state. Returns `false` when the
 *  domain does not expose `should_prune`.
 */
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

}  // end namespace state_search_detail

/** @brief Recursive depth-first backtracking over an implicit state space.
 *
 *  The domain contract is given by @ref Aleph::BacktrackingDomain. Goal states are
 *  treated as accepted solutions and are not expanded further. Domains may
 *  also expose optional `is_terminal()` and `should_prune()` hooks.
 *
 *  @tparam Domain problem adapter exposing `State`, `Move`, successor
 *          generation and `apply()` / `undo()`.
 */
template <BacktrackingDomain Domain>
class Depth_First_Backtracking
{
public:
  using Domain_Type = Domain;
  using State = typename Domain::State;
  using Move = typename Domain::Move;
  using Solution = SearchSolution<State, Move>;
  using Solution_Compare = state_search_detail::PreferShallowerSolution<State, Move>;
  using Result = SearchResult<Solution, Solution_Compare>;

  /** @brief Build an engine bound to one domain adapter. */
  explicit Depth_First_Backtracking(Domain domain,
                                    ExplorationPolicy policy = {},
                                    SearchLimits limits = {},
                                    Solution_Compare compare = {})
    : domain_(std::move(domain)), policy_(policy), limits_(limits), compare_(std::move(compare))
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

  /** @brief Execute a depth-first backtracking search from `initial_state`.
   *
   *  @param[in] initial_state Mutable root state to explore.
   *  @return Search result with limits, stats and best retained solution.
   *
   *  @throws std::invalid_argument if unsupported policy values are requested.
   */
  [[nodiscard]] Result search(State initial_state)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), on_solution);
  }

  /** @brief Execute DFS/backtracking and invoke `on_solution` on each solution.
   *
   *  The callback receives a snapshot of the accepted solution and returns
   *  `true` to continue enumeration or `false` to stop immediately.
   */
  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &on_solution)
  {
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "Depth_First_Backtracking only supports depth-first strategy";
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";

    Result result(compare_);
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    SearchPath<Move> path;
    reserve_search_path(path, limits_);
    (void) dfs(initial_state, path, 0, result, on_solution);

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

  /** @brief Rvalue-friendly overload for temporary solution visitors. */
  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &&on_solution)
  {
    auto handler = std::forward<OnSolution>(on_solution);
    return search(std::move(initial_state), handler);
  }

  /** @brief DFS/backtracking with a global visited-state set.
   *
   *  Converts tree-search to graph-search: states are marked globally when
   *  first entered. Any state whose key is already in `visited` is pruned,
   *  preventing duplicate expansions via longer paths.
   *
   *  The domain must expose `state_key(state)` (see @ref Aleph::SearchStateKeyProvider).
   *
   *  @param[in] initial_state Mutable root state.
   *  @param[in,out] visited Caller-owned set that accumulates visited state keys.
   *  @return Search result with statistics and best retained solution.
   */
  template <typename VisitedSet>
    requires SearchStateKeyProvider<Domain>
         and VisitedStateSet<VisitedSet, typename Domain::State_Key>
  [[nodiscard]] Result search(State initial_state, VisitedSet &visited)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), visited, on_solution);
  }

  /** @brief DFS/backtracking with visited-state tracking and a solution callback.
   *
   *  Combines graph-search pruning via `visited` with per-solution callbacks.
   */
  template <typename VisitedSet, typename OnSolution>
    requires SearchStateKeyProvider<Domain>
         and VisitedStateSet<VisitedSet, typename Domain::State_Key>
         and SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, VisitedSet &visited, OnSolution &on_solution)
  {
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "Depth_First_Backtracking only supports depth-first strategy";
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";

    Result result(compare_);
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    SearchPath<Move> path;
    reserve_search_path(path, limits_);
    (void) dfs_visited(initial_state, path, 0, result, on_solution, visited);

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
  Solution_Compare compare_;

  [[nodiscard]] bool stop_after_solution(Result &result) const
  {
    if (policy_.stop_at_first_solution)
      {
        result.status = SearchStatus::StoppedOnSolution;
        return true;
      }

    if (limits_.max_solutions != Search_Unlimited
        and result.stats.solutions_found >= limits_.max_solutions)
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

  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] bool dfs(State &state,
                         SearchPath<Move> &path,
                         const size_t depth,
                         Result &result,
                         OnSolution &on_solution)
  {
    ++result.stats.visited_states;
    if (depth > result.stats.max_depth_reached)
      result.stats.max_depth_reached = depth;

    if (domain_.is_goal(state))
      {
        ++result.stats.solutions_found;
        Solution solution{state, path, depth};
        result.best_solution.consider(solution);

        if (not on_solution(solution))
          {
            result.status = SearchStatus::StoppedOnSolution;
            return true;
          }

        return stop_after_solution(result);
      }

    if (state_search_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (state_search_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }

    if (expansion_limit_reached(result))
      return true;

    ++result.stats.expanded_states;

    bool stop = false;
    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      ++result.stats.generated_successors;
      path.append(move);
      domain_.apply(state, move);
      stop = dfs(state, path, depth + 1, result, on_solution);
      domain_.undo(state, move);
      (void) path.remove_last();
      return not stop;
    });

    return stop;
  }

  template <typename OnSolution, typename VisitedSet>
    requires SearchSolutionVisitor<OnSolution, Solution>
         and VisitedStateSet<VisitedSet, typename Domain::State_Key>
  [[nodiscard]] bool dfs_visited(State &state,
                                 SearchPath<Move> &path,
                                 const size_t depth,
                                 Result &result,
                                 OnSolution &on_solution,
                                 VisitedSet &visited)
  {
    ++result.stats.visited_states;
    if (depth > result.stats.max_depth_reached)
      result.stats.max_depth_reached = depth;

    if (visited.contains(domain_.state_key(state)))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }
    visited.insert(domain_.state_key(state));

    if (domain_.is_goal(state))
      {
        ++result.stats.solutions_found;
        Solution solution{state, path, depth};
        result.best_solution.consider(solution);

        if (not on_solution(solution))
          {
            result.status = SearchStatus::StoppedOnSolution;
            return true;
          }

        return stop_after_solution(result);
      }

    if (state_search_detail::is_terminal_state(domain_, state))
      {
        ++result.stats.terminal_states;
        return false;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        return false;
      }

    if (state_search_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        return false;
      }

    if (expansion_limit_reached(result))
      return true;

    ++result.stats.expanded_states;

    bool stop = false;
    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      ++result.stats.generated_successors;
      path.append(move);
      domain_.apply(state, move);
      stop = dfs_visited(state, path, depth + 1, result, on_solution, visited);
      domain_.undo(state, move);
      (void) path.remove_last();
      return not stop;
    });

    return stop;
  }
};

/** @brief Convenience wrapper that runs a one-shot DFS/backtracking search. */
template <BacktrackingDomain Domain>
[[nodiscard]] auto backtracking_search(Domain domain,
                                       typename Domain::State initial_state,
                                       ExplorationPolicy policy = {},
                                       SearchLimits limits = {})
{
  Depth_First_Backtracking<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state));
}

/** @brief Convenience wrapper with a solution callback/collector. */
template <BacktrackingDomain Domain, typename OnSolution>
  requires SearchSolutionVisitor<OnSolution, SearchSolution<typename Domain::State, typename Domain::Move>>
[[nodiscard]] auto backtracking_search(Domain domain,
                                       typename Domain::State initial_state,
                                       OnSolution &on_solution,
                                       ExplorationPolicy policy = {},
                                       SearchLimits limits = {})
{
  Depth_First_Backtracking<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), on_solution);
}

}  // end namespace Aleph

#endif  // BACKTRACKING_H
