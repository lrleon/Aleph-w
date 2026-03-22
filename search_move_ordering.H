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

/** @file search_move_ordering.H
 *  @brief Shared support for configurable successor ordering heuristics.
 *
 *  This module keeps move ordering intentionally lightweight:
 *
 *  - a small enum describing the ordering mode requested by the user,
 *  - per-run ordering statistics,
 *  - optional killer/history heuristic tables,
 *  - helpers to sort materialized successor batches with Aleph sort utilities.
 *
 *  Search engines remain responsible for deciding when ordering is meaningful
 *  and how priorities are computed (for example, shallow evaluation in
 *  Alpha-Beta vs optimistic bounds in branch and bound).
 *
 *  @author Leandro Rabindranath León
 */

# ifndef SEARCH_MOVE_ORDERING_H
# define SEARCH_MOVE_ORDERING_H

#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>

#include <tpl_array.H>
#include <tpl_hash.H>
#include <tpl_sort_utils.H>

namespace Aleph {

/** @brief Built-in move-ordering modes supported by the framework. */
enum class MoveOrderingMode
{
  Domain,           ///< Preserve the order emitted by `for_each_successor()`.
  Estimated_Score,  ///< Rank successors by a cheap heuristic score estimate.
  Estimated_Bound   ///< Rank successors by an optimistic child bound.
};

/** @brief Statistics collected by engines that reorder successor batches. */
struct MoveOrderingStats
{
  size_t ordered_batches = 0;     ///< Number of successor batches materialized and ordered.
  size_t ordered_moves = 0;       ///< Number of individual moves considered by ordering.
  size_t priority_estimates = 0;  ///< Number of score/bound estimates computed for ordering.
  size_t killer_hits = 0;         ///< Moves promoted because they matched a killer slot.
  size_t history_hits = 0;        ///< Moves promoted because they had non-zero history.
};

/** @brief One move plus the metadata used by ordering comparators.
 *
 *  @tparam Move Concrete move type.
 *  @tparam Priority Numeric type used for sorting (score or bound).
 */
template <typename Move, typename Priority>
struct RankedMove
{
  Move move;  ///< The candidate move.
  Priority priority
    = Priority{};            ///< Cheap heuristic priority (lower or higher depending on engine).
  size_t ordinal = 0;        ///< Original order from successor generator (tie-breaker).
  bool killer = false;       ///< True if this move matched a killer slot.
  size_t history_score = 0;  ///< Accumulated history heuristic score.
};

/** @brief Two-slot killer heuristic table indexed by search depth.
 *
 *  This implementation is intentionally small: each depth keeps two recent
 *  cutoff-producing moves. Engines can query the table while ordering a batch
 *  and record a move when it triggers a cutoff.
 *
 *  @tparam Move Move type.
 *  @tparam Supported Internal marker for equality support.
 */
template <typename Move, bool Supported = std::equality_comparable<Move>>
class Killer_Move_Table;

/** @brief Specialized killer table for moves without equality support (no-op). */
template <typename Move>
class Killer_Move_Table<Move, false>
{
public:
  /// Marker indicating that this table does nothing.
  static constexpr bool supported = false;

  /** @brief No-op. */
  static void clear() noexcept
  {
    // empty
  }

  /** @brief Always returns false.
   *  @return Always false.
   */
  [[nodiscard]] static bool is_killer(const size_t, const Move &) noexcept
  {
    return false;
  }

  /** @brief No-op. */
  static void record(const size_t, const Move &) noexcept
  {
    // empty
  }
};

/** @brief Killer heuristic table for equality-comparable moves. */
template <typename Move>
class Killer_Move_Table<Move, true>
{
public:
  /// Marker indicating that this table tracks moves.
  static constexpr bool supported = true;

  /** @brief Remove all recorded killer moves. */
  void clear() noexcept
  {
    primary_.empty();
    secondary_.empty();
  }

  /** @brief Return true if `move` is in a killer slot for `depth`.
   *  @param depth Path depth from root.
   *  @param move Candidate move to check.
   *  @return true if move was found in either slot for depth.
   */
  [[nodiscard]] bool is_killer(const size_t depth, const Move &move) const noexcept
  {
    return depth < primary_.size()
       and ((primary_[depth].has_value() and *primary_[depth] == move)
            or (secondary_[depth].has_value() and *secondary_[depth] == move));
  }

  /** @brief Record a move that caused a cutoff at `depth`.
   *  @param depth Path depth from root.
   *  @param move Cutoff-producing move.
   */
  void record(const size_t depth, const Move &move)
  {
    ensure_depth(depth + 1);
    if (primary_[depth].has_value() and *primary_[depth] == move)
      return;

    secondary_[depth] = primary_[depth];
    primary_[depth] = move;
  }

private:
  void ensure_depth(const size_t required)
  {
    while (primary_.size() < required)
      {
        primary_.append(std::optional<Move>{});
        secondary_.append(std::optional<Move>{});
      }
  }

  Array<std::optional<Move>> primary_;
  Array<std::optional<Move>> secondary_;
};

/** @brief Null history heuristic hook used when a domain has no move key. */
struct Null_History_Heuristic_Table
{
  static constexpr bool supported = false;

  static void clear() noexcept
  {
    // empty
  }

  template <typename Key>
  [[nodiscard]] size_t score(const Key &) const noexcept
  {
    return 0;
  }

  template <typename Key>
  static void record(const Key &, const size_t = 1) noexcept
  {
    // empty
  }
};

/** @brief Sparse history heuristic table over Aleph hash maps.
 *
 *  @tparam Key Key type used to identify moves globally.
 *  @tparam HashMapTable Aleph hash-table implementation.
 *  @tparam Cmp Key equality comparator.
 */
template <typename Key,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<Key>>
class History_Heuristic_Table
{
public:
  /// Marker indicating that this table tracks history.
  static constexpr bool supported = true;
  /// Type of move keys.
  using Key_Type = Key;
  /// Internal map type.
  using Map_Type = DynHashMap<Key, size_t, HashMapTable, Cmp>;

  /** @brief Reset all history scores to zero. */
  void clear() noexcept
  {
    table_.empty();
  }

  /** @brief Read the current score for a move key.
   *  @param key Move key.
   *  @return Cumulative history score.
   */
  [[nodiscard]] size_t score(const Key &key) const noexcept
  {
    if (auto *entry = table_.search(key); entry != nullptr)
      return entry->second;

    return 0;
  }

  /** @brief Increment the history score for a move key.
   *  @param key Move key.
   *  @param bonus Amount to add to the score.
   */
  void record(const Key &key, const size_t bonus = 1)
  {
    if (auto *entry = table_.search(key); entry != nullptr)
      {
        entry->second += bonus;
        return;
      }

    (void) table_.insert(key, bonus);
  }

private:
  Map_Type table_;
};

/** @brief Sort one materialized move batch using priority and optional hooks.
 *
 *  The ordering is:
 *
 *  1. killer moves first, if enabled,
 *  2. greater history score first, if enabled,
 *  3. better priority first according to `better_priority`,
 *  4. original domain order as deterministic tie-breaker.
 *
 *  @tparam Move Concrete move type.
 *  @tparam Priority Numeric priority type.
 *  @tparam BetterPriority Binary predicate `(Priority, Priority) -> bool`.
 *  @param moves Batch of ranked moves to sort in-place.
 *  @param better_priority Comparator where `true` means the first priority is better.
 *  @param prefer_killer If true, moves with `.killer == true` are promoted.
 *  @param prefer_history If true, moves are ordered by descending `.history_score`.
 */
template <typename Move, typename Priority, typename BetterPriority>
void sort_ranked_moves(Array<RankedMove<Move, Priority>> &moves,
                       BetterPriority better_priority,
                       const bool prefer_killer,
                       const bool prefer_history)
{
  if (moves.size() < 2)
    return;

  introsort(moves,
            [=](const RankedMove<Move, Priority> &lhs, const RankedMove<Move, Priority> &rhs) noexcept
  {
    if (prefer_killer and lhs.killer != rhs.killer)
      return lhs.killer and not rhs.killer;

    if (prefer_history and lhs.history_score != rhs.history_score)
      return lhs.history_score > rhs.history_score;

    if (better_priority(lhs.priority, rhs.priority))
      return true;

    if (better_priority(rhs.priority, lhs.priority))
      return false;

    return lhs.ordinal < rhs.ordinal;
  });
}

}  // end namespace Aleph

# endif  // SEARCH_MOVE_ORDERING_H
