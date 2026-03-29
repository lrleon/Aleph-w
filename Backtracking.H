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

# ifndef BACKTRACKING_H
# define BACKTRACKING_H

#include <utility>

#include <ah-errors.H>
#include <state_search_common.H>

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
  /// Type of the problem domain.
  using Domain_Type = Domain;
  /// Concrete search state type.
  using State = typename Domain::State;
  /// Move type.
  using Move = typename Domain::Move;
  /// Concrete solution type.
  using Solution = SearchSolution<State, Move>;
  /// Internal solution comparator.
  using Solution_Compare = state_search_detail::PreferShallowerSolution<State, Move>;
  /// Aggregated result type.
  using Result = SearchResult<Solution, Solution_Compare>;

  /** @brief Compile-time marker: Depth_First_Backtracking only supports
   *         Depth_First strategy.
   *
   *  Passing `ExplorationPolicy::Strategy::Best_First` to this engine raises
   *  `std::invalid_argument` at runtime.  Check this constant before
   *  constructing a policy to detect the mismatch at compile time.
   */
  static constexpr bool supports_best_first = false;

  /** @brief Build an engine bound to one domain adapter.
   *
   *  @param domain Problem adapter captured by value. Ownership is transferred
   *                into the engine; move-only domains are accepted and remain
   *                owned for the lifetime of the `Depth_First_Backtracking`.
   *  @param policy Exploration policy copied into the engine. Only
   *                `Depth_First` traversal is honored during search.
   *  @param limits Search constraints copied and enforced on every run
   *                (such as depth, expansions and solutions).
   *  @param compare Functor used to rank solutions kept by @ref Result; moved
   *                 into the result objects returned by @ref search.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Not thread-safe. Guard the engine externally when sharing across threads.
   */
  explicit Depth_First_Backtracking(Domain domain,
                                    ExplorationPolicy policy = {},
                                    const SearchLimits &limits = {},
                                    Solution_Compare compare = {})
    : domain_(std::move(domain)), policy_(policy), limits_(limits), compare_(std::move(compare))
  {
    // empty
  }

  /** @brief Read-only access to the bound domain adapter.
   *  @return const Domain& view of the currently configured domain adapter.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Caller must synchronize concurrent accesses to the engine.
   */
  [[nodiscard]] const Domain &domain() const noexcept
  {
    return domain_;
  }

  /** @brief Mutable access to the bound domain adapter.
   *  @return Domain& reference that permits in-place configuration changes.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Not thread-safe. Synchronize externally before mutating the domain.
   */
  [[nodiscard]] Domain &domain() noexcept
  {
    return domain_;
  }

  /** @brief Current exploration policy.
   *  @return const ExplorationPolicy& describing how successors are explored.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Caller must synchronize concurrent observations.
   */
  [[nodiscard]] const ExplorationPolicy &policy() const noexcept
  {
    return policy_;
  }

  /** @brief Current hard limits.
   *  @return const SearchLimits& currently enforced during search.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Caller must synchronize concurrent observations.
   */
  [[nodiscard]] const SearchLimits &limits() const noexcept
  {
    return limits_;
  }

  /** @brief Replace the exploration policy for future runs.
   *  @param policy Exploration policy copied and used by subsequent @ref search calls.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Synchronize externally when mutating shared engines.
   */
  void set_policy(const ExplorationPolicy &policy) noexcept
  {
    policy_ = policy;
  }

  /** @brief Replace the hard limits for future runs.
   *  @param limits New @ref SearchLimits copied verbatim into the engine.
   *  @throws None.
   *  @par Complexity
   *  Time O(1); space O(1).
   *  @par Thread Safety
   *  Synchronize externally when mutating shared engines.
   */
  void set_limits(const SearchLimits &limits) noexcept
  {
    limits_ = limits;
  }

  /** @brief Execute a depth-first backtracking search from `initial_state`.
   *
   *  @param initial_state Mutable root state provided by value; the engine
   *         restores it before returning so callers retain ownership.
   *  @return Result containing the final search status, aggregated statistics,
   *          effective limits/policy, and the best solution kept according to
   *          @ref Solution_Compare.
   *  @throws std::invalid_argument if `policy().strategy` is not
   *          `ExplorationPolicy::Strategy::Depth_First` or if
   *          `limits().max_solutions == 0`.
   *  @par Complexity
   *  Time proportional to the number of visited states until termination;
   *  auxiliary space proportional to the maximum path depth (one stack of
   *  states plus bookkeeping in @ref SearchPath).
   *  @par Thread Safety
   *  Not thread-safe. Serialize concurrent invocations on the same engine.
   */
  [[nodiscard]] Result search(State initial_state)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), on_solution);
  }

  /** @brief Execute DFS/backtracking and invoke `on_solution` on each solution.
   *
   *  @param initial_state Mutable root state explored by value.
   *  @param on_solution Functor satisfying @ref Aleph::SearchSolutionVisitor that receives
   *         immutable @ref Solution snapshots and returns `true` to continue or
   *         `false` to stop immediately.
   *  @return Result populated with policy, limits, statistics and the best
   *          solution according to @ref Solution_Compare.
   *  @throws std::invalid_argument if the exploration policy is not depth-first
   *          or if `limits().max_solutions == 0`.
   *  @par Complexity
   *  Same as the overload without a callback: time proportional to explored
   *  states, auxiliary space proportional to max depth.
   *  @par Thread Safety
   *  External synchronization required.
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

  /** @brief Rvalue-friendly overload for temporary solution visitors.
   *  @param initial_state Mutable root state explored by value.
   *  @param on_solution Temporary visitor consumed (moved) by the call.
   *  @return Result identical to the lvalue overload.
   *  @throws std::invalid_argument under the same conditions as the lvalue overload.
   *  @par Complexity
   *  Same as other @ref search overloads.
   *  @par Thread Safety
   *  External synchronization required.
   */
  template <typename OnSolution>
    requires SearchSolutionVisitor<OnSolution, Solution>
  [[nodiscard]] Result search(State initial_state, OnSolution &&on_solution)
  {
    auto handler = std::forward<OnSolution>(on_solution);
    return search(std::move(initial_state), handler);
  }

  /** @brief DFS/backtracking with a depth-aware visited-state map.
   *
   *  Converts tree-search to graph-search: states are marked globally after
   *  passing limit/pruning checks. A state is pruned only when a previous
   *  expansion reached it at an equal or shallower depth (i.e., with at least
   *  as much remaining budget); if the current path is shallower the stored
   *  depth is updated and exploration continues. Requires domains exposing
   *  `state_key(state)`.
   *
   *  @param initial_state Mutable root state explored by value.
   *  @param visited Caller-managed depth map that accumulates visited keys
   *                 across calls; must satisfy @ref Aleph::VisitedBoundMap
   *                 with `size_t` as the bound type (e.g. `SearchStorageMap<Key, size_t>`).
   *  @return Result with final status, statistics, limits/policy snapshots and
   *          best solution backed by @ref Solution_Compare.
   *  @throws std::invalid_argument if the exploration policy is not depth-first
   *          or if `limits().max_solutions == 0`.
   *  @par Complexity
   *  Time proportional to explored states; auxiliary space proportional to the
   *  maximum search depth and the size of `visited`.
   *  @par Thread Safety
   *  Caller must synchronize both the engine and the provided `visited` map.
   */
  template <typename VisitedSet>
    requires SearchStateKeyProvider<Domain>
         and VisitedBoundMap<VisitedSet, typename Domain::State_Key, size_t>
  [[nodiscard]] Result search(State initial_state, VisitedSet &visited)
  {
    ContinueSearch on_solution;
    return search(std::move(initial_state), visited, on_solution);
  }

  /** @brief DFS/backtracking with depth-aware visited-state tracking and a solution callback.
   *
   *  @param initial_state Mutable root state explored by value.
   *  @param visited Caller-managed depth map updated at every state expansion;
   *                 must satisfy @ref Aleph::VisitedBoundMap with `size_t` as
   *                 bound type (e.g. `SearchStorageMap<Key, size_t>`).
   *  @param on_solution Solution visitor invoked for each solution; returning
   *         `false` stops the search immediately.
   *  @return Result capturing the final status, stats, limits, policy and best
   *          solution selected by @ref Solution_Compare.
   *  @throws std::invalid_argument if the policy is not depth-first or if the
   *          configured maximum number of solutions is zero.
   *  @par Complexity
   *  Time proportional to explored states, auxiliary space proportional to the
   *  maximum depth plus storage required by `visited`.
   *  @par Thread Safety
   *  Not thread-safe. Synchronize external access to both the engine and the
   *  provided depth map.
   */
  template <typename VisitedSet, typename OnSolution>
    requires SearchStateKeyProvider<Domain>
         and VisitedBoundMap<VisitedSet, typename Domain::State_Key, size_t>
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
    return search_engine_detail::stop_after_solution(result, policy_, limits_);
  }

  [[nodiscard]] bool expansion_limit_reached(Result &result) const
  {
    return search_engine_detail::expansion_limit_reached(result, limits_);
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
      try
        {
          stop = dfs(state, path, depth + 1, result, on_solution);
        }
      catch (...)
        {
          domain_.undo(state, move);
          (void) path.remove_last();
          throw;
        }
      domain_.undo(state, move);
      (void) path.remove_last();
      return not stop;
    });

    return stop;
  }

  template <typename OnSolution, typename VisitedSet>
    requires SearchSolutionVisitor<OnSolution, Solution>
         and VisitedBoundMap<VisitedSet, typename Domain::State_Key, size_t>
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

    const auto key = domain_.state_key(state);
    if (auto *entry = visited.search(key);
        entry != nullptr and entry->second <= depth)
      {
        ++result.stats.pruned_by_visited;
        return false;
      }

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

    if (expansion_limit_reached(result))
      return true;

    if (auto *entry = visited.search(key); entry != nullptr)
      entry->second = depth;
    else
      visited.insert(key, depth);

    ++result.stats.expanded_states;

    bool stop = false;
    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      ++result.stats.generated_successors;
      path.append(move);
      domain_.apply(state, move);
      try
        {
          stop = dfs_visited(state, path, depth + 1, result, on_solution, visited);
        }
      catch (...)
        {
          domain_.undo(state, move);
          (void) path.remove_last();
          throw;
        }
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

# endif  // BACKTRACKING_H
