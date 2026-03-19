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


/** @file state_search_common.H
 *  @brief Common infrastructure for implicit state-space search.
 *
 *  This module defines the small set of reusable types needed by the first
 *  generation of search engines over implicit trees and DAGs:
 *
 *  - search status, limits and statistics,
 *  - incumbent/best-solution tracking,
 *  - path and storage aliases based on Aleph containers,
 *  - minimal concepts for states, moves and lazy successor generation.
 *
 *  The design intentionally starts with a small contract centered on
 *  `State + Move + make/unmake`, leaving room for later phases to add
 *  branch-and-bound, negamax and memoization on top of the same base.
 *
 *  @author Leandro Rabindranath León
 */

#ifndef STATE_SEARCH_COMMON_H
#define STATE_SEARCH_COMMON_H

#include <chrono>
#include <concepts>
#include <cstddef>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include <ah-concepts.H>
#include <ah-errors.H>
#include <htlist.H>
#include <search_move_ordering.H>
#include <tpl_array.H>
#include <tpl_hash.H>

namespace Aleph
{

/** @brief Sentinel used by @ref SearchLimits to mean "no bound". */
inline constexpr size_t Search_Unlimited = std::numeric_limits<size_t>::max();

/** @brief Final state of a search execution. */
enum class SearchStatus
{
  NotStarted,        ///< Search object exists but no traversal has run yet.
  Exhausted,         ///< Search space within the configured bounds was exhausted.
  StoppedOnSolution, ///< Search stopped because solution handling requested it.
  LimitReached       ///< Search stopped because an external hard limit was hit.
};

/** @brief Minimal requirement for mutable search states.
 *
 *  States are copied only when a solution snapshot must be retained.
 *  Traversal itself is expected to mutate the same state in-place through
 *  `apply()` / `undo()`.
 */
template <typename T>
concept SearchState = std::movable<T> and std::copy_constructible<T>;

/** @brief Minimal requirement for search moves.
 *
 *  Moves are stored in Aleph arrays to preserve a reconstruction path and to
 *  support future move-ordering policies.
 */
template <typename T>
concept SearchMove = std::movable<T> and std::copy_constructible<T>;

/** @brief Default Aleph container used to store a move path. */
template <SearchMove Move>
using SearchPath = Array<Move>;

using SearchClock = std::chrono::steady_clock; ///< Monotonic clock used for search timings.

[[nodiscard]] inline double
search_elapsed_ms(const SearchClock::duration &duration) noexcept
{
  return std::chrono::duration<double, std::milli>(duration).count();
}

/** @brief Aleph hash map alias for memoization/transposition storage. */
template <typename Key, typename Data,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<Key>>
using SearchStorageMap = DynHashMap<Key, Data, HashMapTable, Cmp>;

/** @brief Aleph hash set alias for visited-state storage. */
template <typename Key,
          template <typename, class> class HashSetTable = OLhashTable,
          class Cmp = Aleph::equal_to<Key>>
using SearchStorageSet = DynHashSet<Key, HashSetTable, Cmp>;

/** @brief Counters collected during a search run. */
struct SearchStats
{
  size_t visited_states = 0;       ///< Number of states entered by the engine.
  size_t expanded_states = 0;      ///< Number of non-terminal states expanded.
  size_t generated_successors = 0; ///< Number of successor moves emitted.
  size_t solutions_found = 0;      ///< Number of goal states accepted.
  size_t terminal_states = 0;      ///< Non-solution terminal states cut by the domain.
  size_t pruned_by_depth = 0;      ///< States not expanded due to max depth.
  size_t pruned_by_domain = 0;     ///< States discarded by domain-side pruning.
  size_t limit_hits = 0;           ///< Number of hard-limit stops triggered.
  size_t max_depth_reached = 0;    ///< Deepest path depth visited.
  double elapsed_ms = 0.0;         ///< Wall-clock time spent inside the search call.
  MoveOrderingStats move_ordering; ///< Successor-ordering activity for this run.
};

/** @brief Hard bounds applied by the search engine.
 *
 *  `max_depth` is treated as part of the explored region: states at depth
 *  `max_depth` are still checked for goal, but not expanded. The other bounds
 *  (`max_expansions`, `max_solutions`) are hard stops and yield
 *  @ref SearchStatus::LimitReached when hit.
 */
struct SearchLimits
{
  size_t max_depth = Search_Unlimited;      ///< Maximum expansion depth.
  size_t max_expansions = Search_Unlimited; ///< Maximum expanded states.
  size_t max_solutions = Search_Unlimited;  ///< Maximum accepted solutions.
};

template <SearchMove Move>
inline void reserve_search_path(SearchPath<Move> &path,
                                const SearchLimits &limits)
{
  if (limits.max_depth != Search_Unlimited)
    path.reserve(limits.max_depth + 1);
}

/** @brief Exploration controls shared across engines.
 *
 *  Only depth-first traversal is enabled in this initial phase. The struct is
 *  intentionally small so that later phases can extend it without forcing a
 *  large hierarchy today.
 */
struct ExplorationPolicy
{
  /** @brief Traversal strategy supported by the engine. */
  enum class Strategy
  {
    Depth_First, ///< Recursive depth-first traversal.
    Best_First   ///< Priority-guided expansion of the most promising frontier node.
  };

  Strategy strategy = Strategy::Depth_First; ///< Traversal strategy.
  MoveOrderingMode move_ordering = MoveOrderingMode::Domain; ///< Successor-ordering mode.
  bool stop_at_first_solution = true;        ///< Stop when the first goal is found.
  bool use_killer_moves = false;             ///< Enable experimental killer heuristic where supported.
  bool use_history_heuristic = false;        ///< Enable experimental history heuristic where supported.
};

/** @brief Snapshot of a concrete solution encountered during the traversal.
 *
 *  The engine copies the terminal state and the move path that produced it,
 *  allowing users to inspect or replay the result after the mutable search
 *  state has been restored by `undo()`.
 */
template <SearchState State, SearchMove Move>
struct SearchSolution
{
  State state;         ///< Snapshot of the terminal state.
  SearchPath<Move> path; ///< Move sequence leading to `state`.
  size_t depth = 0;    ///< Path depth of the solution.
};

/** @brief Comparison policy that keeps the first solution seen.
 *
 *  It is the default policy for @ref BestSolution when the caller does not
 *  want to impose an ordering among solutions.
 */
template <typename Solution>
struct KeepFirstSolution
{
  bool operator()(const Solution &, const Solution &) const noexcept
  {
    return false;
  }
};

/** @brief Default solution visitor that always continues the search. */
struct ContinueSearch
{
  template <typename Solution>
  bool operator()(const Solution &) const noexcept
  {
    return true;
  }
};

/** @brief Concept for callbacks invoked on each accepted solution. */
template <typename Visitor, typename Solution>
concept SearchSolutionVisitor =
  requires(Visitor &visitor, const Solution &solution)
  {
    { visitor(solution) } -> std::convertible_to<bool>;
  };

/** @brief Collector that stores accepted solutions in an Aleph list.
 *
 *  This is a small adapter for DFS/backtracking enumeration. It appends each
 *  accepted solution to an internal @ref DynList and can optionally stop after
 *  a user-specified number of collected solutions.
 */
template <typename Solution>
class SearchSolutionCollector
{
public:
  using Solution_Type = Solution;
  using Container_Type = DynList<Solution>;

  SearchSolutionCollector() = default;

  explicit SearchSolutionCollector(const size_t max_solutions)
    : limit_(max_solutions)
  {
    ah_invalid_argument_if(limit_ == 0)
      << "SearchSolutionCollector: max_solutions must be positive";
  }

  /** @brief Append one solution and report whether enumeration should continue. */
  bool operator()(const Solution &solution)
  {
    solutions_.append(solution);
    ++count_;
    return count_ < limit_;
  }

  /** @brief Remove all collected solutions. */
  void clear() noexcept
  {
    solutions_.empty();
    count_ = 0;
  }

  /** @brief Number of collected solutions. */
  [[nodiscard]] size_t size() const noexcept
  {
    return count_;
  }

  /** @brief Return `true` if no solution has been collected. */
  [[nodiscard]] bool is_empty() const noexcept
  {
    return count_ == 0;
  }

  /** @brief Access the collected solutions. */
  [[nodiscard]] const Container_Type &solutions() const noexcept
  {
    return solutions_;
  }

  /** @brief Mutable access to the collected solutions. */
  [[nodiscard]] Container_Type &solutions() noexcept
  {
    return solutions_;
  }

private:
  Container_Type solutions_;
  size_t count_ = 0;
  size_t limit_ = Search_Unlimited;
};

/** @brief Tracks the best solution seen so far according to a comparator.
 *
 *  The first candidate is always accepted. Later candidates replace the stored
 *  incumbent only if `compare(candidate, incumbent)` returns `true`.
 */
template <typename Solution, typename Compare = KeepFirstSolution<Solution>>
  requires BinaryPredicate<Compare, Solution>
class BestSolution
{
public:
  using Solution_Type = Solution;
  using Compare_Type = Compare;

  BestSolution() = default;

  explicit BestSolution(Compare compare) : compare_(std::move(compare))
  {
    // empty
  }

  /** @brief Return `true` if an incumbent exists. */
  [[nodiscard]] bool has_value() const noexcept
  {
    return current_.has_value();
  }

  /** @brief Remove the stored incumbent, if any. */
  void clear() noexcept
  {
    current_.reset();
  }

  /** @brief Read the current incumbent.
   *  @throws std::runtime_error if no incumbent has been stored yet.
   */
  [[nodiscard]] const Solution &get() const
  {
    ah_runtime_error_unless(current_.has_value())
      << "BestSolution::get: no incumbent solution available";
    return *current_;
  }

  /** @brief Mutable access to the current incumbent.
   *  @throws std::runtime_error if no incumbent has been stored yet.
   */
  [[nodiscard]] Solution &get()
  {
    ah_runtime_error_unless(current_.has_value())
      << "BestSolution::get: no incumbent solution available";
    return *current_;
  }

  /** @brief Access the ordering functor used by this incumbent. */
  [[nodiscard]] const Compare &compare() const noexcept
  {
    return compare_;
  }

  /** @brief Consider a candidate by copy.
   *  @return `true` if the incumbent was updated.
   */
  bool consider(const Solution &candidate)
  {
    if (not current_.has_value() or compare_(candidate, *current_))
      {
        current_ = candidate;
        return true;
      }

    return false;
  }

  /** @brief Consider a candidate by move.
   *  @return `true` if the incumbent was updated.
   */
  bool consider(Solution &&candidate)
  {
    if (not current_.has_value() or compare_(candidate, *current_))
      {
        current_ = std::move(candidate);
        return true;
      }

    return false;
  }

private:
  Compare compare_ = {};
  std::optional<Solution> current_;
};

/** @brief Alias used by optimization/search engines for their current best. */
template <typename Solution, typename Compare = KeepFirstSolution<Solution>>
using Incumbent = BestSolution<Solution, Compare>;

/** @brief Concept for lazy successor generation.
 *
 *  A generator exposes nested `State` and `Move` types plus
 *  `for_each_successor(state, visitor)`. The visitor receives each move and
 *  returns `true` to continue or `false` to stop early. The generator should
 *  propagate that early stop and return a boolean indicating whether traversal
 *  completed (`true`) or was interrupted by the visitor (`false`).
 */
template <typename Domain>
concept SuccessorGenerator =
  requires(Domain &d, const typename Domain::State &state)
  {
    typename Domain::State;
    typename Domain::Move;
    requires SearchState<typename Domain::State>;
    requires SearchMove<typename Domain::Move>;
    { d.for_each_successor(state,
                           [](const typename Domain::Move &) -> bool
                           {
                             return true;
                           }) } -> std::convertible_to<bool>;
  };

/** @brief Concept for domains that can recognize goal states. */
template <typename Domain>
concept GoalPredicate =
  requires(Domain &d, const typename Domain::State &state)
  {
    { d.is_goal(state) } -> std::convertible_to<bool>;
  };

/** @brief Optional concept for explicit non-solution terminal states. */
template <typename Domain>
concept TerminalPredicate =
  requires(Domain &d, const typename Domain::State &state)
  {
    { d.is_terminal(state) } -> std::convertible_to<bool>;
  };

/** @brief Optional concept for domain-side pruning hooks.
 *
 *  This is intentionally small: a pure DFS engine can already cut dead states,
 *  while later phases can layer stronger incumbent-aware pruning on top of the
 *  same traversal kernel.
 */
template <typename Domain>
concept DomainPruner =
  requires(Domain &d, const typename Domain::State &state, const size_t depth)
  {
    { d.should_prune(state, depth) } -> std::convertible_to<bool>;
  };

/** @brief Optional concept for domains that can key moves for history heuristics. */
template <typename Domain>
concept MoveKeyProvider =
  requires(Domain &d, const typename Domain::Move &move)
  {
    typename Domain::Move_Key;
    { d.move_key(move) } -> std::convertible_to<typename Domain::Move_Key>;
  };

/** @brief Minimal contract for DFS/backtracking domains.
 *
 *  The engine assumes a mutable search state and reversible moves:
 *
 *  - `for_each_successor(state, visitor)` lazily emits candidate moves,
 *  - `apply(state, move)` advances the state,
 *  - `undo(state, move)` restores the previous state,
 *  - `is_goal(state)` marks an accepted solution,
 *  - optionally `is_terminal(state)` marks dead-end terminal states,
 *  - optionally `should_prune(state, depth)` requests an early cutoff.
 */
template <typename Domain>
concept BacktrackingDomain =
  SuccessorGenerator<Domain> and GoalPredicate<Domain> and
  requires(Domain &d,
           typename Domain::State &state,
           const typename Domain::Move &move)
  {
    { d.apply(state, move) } -> std::same_as<void>;
    { d.undo(state, move) } -> std::same_as<void>;
  };

/** @brief Aggregates the outcome of one search execution. */
template <typename Solution, typename Compare = KeepFirstSolution<Solution>>
  requires BinaryPredicate<Compare, Solution>
struct SearchResult
{
  using Solution_Type = Solution;
  using Compare_Type = Compare;
  using Incumbent_Type = BestSolution<Solution, Compare>;

  SearchStatus status = SearchStatus::NotStarted; ///< Final execution state.
  ExplorationPolicy policy; ///< Exploration policy used for the run.
  SearchLimits limits;      ///< Limits used for the run.
  SearchStats stats;        ///< Statistics collected during the run.
  Incumbent_Type best_solution; ///< Best incumbent retained by the engine.

  SearchResult() = default;

  explicit SearchResult(Compare compare) : best_solution(std::move(compare))
  {
    // empty
  }

  /** @brief Return `true` if at least one solution was retained. */
  [[nodiscard]] bool found_solution() const noexcept
  {
    return best_solution.has_value();
  }

  /** @brief Return `true` if the search exhausted the configured region. */
  [[nodiscard]] bool exhausted() const noexcept
  {
    return status == SearchStatus::Exhausted;
  }

  /** @brief Return `true` if search stopped because solution handling requested it. */
  [[nodiscard]] bool stopped_on_solution() const noexcept
  {
    return status == SearchStatus::StoppedOnSolution;
  }

  /** @brief Return `true` if a hard search limit stopped the traversal. */
  [[nodiscard]] bool limit_reached() const noexcept
  {
    return status == SearchStatus::LimitReached;
  }
};

} // end namespace Aleph

#endif // STATE_SEARCH_COMMON_H
