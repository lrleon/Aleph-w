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

/** @file Negamax.H
 *  @brief Negamax search for two-player zero-sum turn-based games.
 *
 *  This module introduces the adversarial-search layer of the framework while
 *  keeping it separate from optimization search:
 *
 *  - games are modeled as implicit state spaces with `apply()` / `undo()`,
 *  - evaluation is always interpreted from the side-to-move perspective,
 *  - results expose one principal variation instead of "solutions",
 *  - limits and baseline traversal statistics are reused from
 *    @ref state_search_common.H.
 *
 *  The implementation targets deterministic, perfect-information, alternating-
 *  turn, zero-sum games. It is intentionally small so Alpha-Beta can reuse the
 *  same contract without pulling game-specific semantics into branch and bound.
 *
 *  @author Leandro Rabindranath León
 */

# ifndef NEGAMAX_H
# define NEGAMAX_H

#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <state_search_common.H>
#include <Transposition_Table.H>

namespace Aleph {

/** @brief Score type accepted by adversarial search engines.
 *
 *  Negamax and Alpha-Beta require an order relation and sign inversion, so the
 *  framework constrains scores to signed integral or floating-point types.
 */
template <typename T>
concept AdversarialScore = std::signed_integral<T> or std::floating_point<T>;

/** @brief Statistics collected by adversarial search engines. */
struct AdversarialSearchStats : SearchStats
{
  size_t heuristic_evaluations = 0;   ///< Number of calls to `evaluate(state)`.
  size_t alpha_beta_cutoffs = 0;      ///< Number of cutoffs performed by Alpha-Beta.
  TranspositionStats transpositions;  ///< Transposition-table usage during this run.
};

/** @brief Cached adversarial-search entry stored in transposition tables. */
template <SearchMove Move, AdversarialScore Score>
struct AdversarialTranspositionEntry
{
  using Move_Type = Move;
  using Score_Type = Score;

  Score value = Score{};  ///< Cached search value.
  size_t depth = 0;       ///< Remaining depth searched for this entry.
  TranspositionBound bound = TranspositionBound::Exact;  ///< Bound classification.
  SearchPath<Move> principal_variation;                  ///< Best line known for this entry.
};

/** @brief Replacement policy for adversarial transposition entries.
 *
 *  Entries are keyed by `(state_key, remaining_depth)`, so depth mismatches are
 *  already separated at the key level. Replacement therefore only needs to
 *  prefer stronger information for the same keyed search configuration:
 *
 *  - exact values replace lower/upper bounds,
 *  - otherwise the existing entry is kept.
 */
template <SearchMove Move, AdversarialScore Score>
struct Prefer_Exact_Adversarial_Entry
{
  using Entry = AdversarialTranspositionEntry<Move, Score>;

  bool operator()(const Entry &candidate, const Entry &current) const noexcept
  {
    if (candidate.depth != current.depth)
      return candidate.depth > current.depth;

    if (candidate.bound != current.bound)
      return candidate.bound == TranspositionBound::Exact;

    return false;
  }
};

/** @brief Convenience alias for adversarial transposition tables.
 *
 *  The key includes both `state_key` and `remaining_depth` so the memoized
 *  value remains consistent with the configured horizon of the current search.
 */
template <typename StateKey>
struct AdversarialTranspositionKey
{
  StateKey state_key = StateKey{};
  size_t depth = 0;

  [[nodiscard]] bool operator==(const AdversarialTranspositionKey &) const noexcept = default;
};

template <typename StateKey>
[[nodiscard]] inline size_t aleph_hash_value(const AdversarialTranspositionKey<StateKey> &key) noexcept
{
  size_t seed = dft_hash_fct(key.state_key);
  hash_combine(seed, dft_hash_fct(key.depth));
  return seed;
}

template <typename StateKey,
          SearchMove Move,
          AdversarialScore Score,
          template <typename, typename, class> class HashMapTable = MapOLhash,
          class Cmp = Aleph::equal_to<AdversarialTranspositionKey<StateKey>>>
using AdversarialTranspositionTable
  = Transposition_Table<AdversarialTranspositionKey<StateKey>,
                        AdversarialTranspositionEntry<Move, Score>,
                        HashMapTable,
                        Cmp,
                        Prefer_Exact_Adversarial_Entry<Move, Score>>;

/** @brief Concept for memo tables storing adversarial-search entries. */
template <typename Table, typename StateKey, typename Move, typename Score>
concept AdversarialTranspositionMemo
  = SearchMove<Move> and AdversarialScore<Score>
and MemoTable<Table, AdversarialTranspositionKey<StateKey>, AdversarialTranspositionEntry<Move, Score>>;

/** @brief Result of one adversarial search execution.
 *
 *  The returned score is interpreted from the perspective of the side to move
 *  at the root state. `principal_variation` stores the best line found from
 *  that root under the configured horizon and limits.
 */
template <SearchMove Move, AdversarialScore Score>
struct AdversarialSearchResult
{
  using Move_Type = Move;
  using Score_Type = Score;

  SearchStatus status = SearchStatus::NotStarted;  ///< Final execution state.
  ExplorationPolicy policy;                        ///< Exploration policy used during the run.
  SearchLimits limits;                             ///< Limits used for the run.
  AdversarialSearchStats stats;                    ///< Collected adversarial-search statistics.
  Score value = Score{};                 ///< Root score from the current player's perspective.
  SearchPath<Move> principal_variation;  ///< Best line found from the root.

  [[nodiscard]] bool exhausted() const noexcept
  {
    return status == SearchStatus::Exhausted;
  }

  [[nodiscard]] bool limit_reached() const noexcept
  {
    return status == SearchStatus::LimitReached;
  }

  [[nodiscard]] bool has_principal_variation() const noexcept
  {
    return not principal_variation.is_empty();
  }

  /** @brief First move of the principal variation.
   *  @throws std::runtime_error if no move is available at the root.
   */
  [[nodiscard]] const Move &first_move() const
  {
    ah_runtime_error_unless(has_principal_variation())
      << "AdversarialSearchResult::first_move: no principal variation available";
    return principal_variation[0];
  }
};

/** @brief Trace event kinds emitted by adversarial search engines. */
enum class AdversarialTraceEventKind
{
  Enter_Node,         ///< One search node has just been entered.
  Exit_Node,          ///< One search node finished with an exact local result.
  Terminal_Node,      ///< A terminal state was evaluated.
  Depth_Cutoff,       ///< Search stopped at the configured horizon.
  Domain_Prune,       ///< Domain-side pruning hook discarded the state.
  Expansion_Limit,    ///< Search stopped because the expansion limit was hit.
  Alpha_Beta_Cutoff,  ///< One Alpha-Beta fail-high cutoff occurred.
  Transposition_Hit,  ///< One cached transposition entry was reused.
  Iteration_Begin,    ///< One iterative-deepening iteration started.
  Iteration_End,      ///< One iterative-deepening iteration completed.
  Aspiration_Retry    ///< An aspiration window failed and was widened.
};

/** @brief One trace event produced by an adversarial search. */
template <SearchMove Move, AdversarialScore Score>
struct AdversarialTraceEvent
{
  using Move_Type = Move;
  using Score_Type = Score;

  AdversarialTraceEventKind kind = AdversarialTraceEventKind::Enter_Node;
  size_t depth = 0;                      ///< Current node depth from the root.
  size_t remaining_depth = 0;            ///< Remaining horizon from this node.
  size_t iteration = 0;                  ///< Iteration index in iterative deepening.
  size_t horizon = 0;                    ///< Horizon of the current iterative iteration.
  size_t aspiration_retries = 0;         ///< Number of aspiration retries so far.
  Score value = Score{};                 ///< Value known at the event point, if any.
  Score alpha = Score{};                 ///< Current Alpha-Beta lower window bound, if any.
  Score beta = Score{};                  ///< Current Alpha-Beta upper window bound, if any.
  std::optional<Move> move;              ///< Best or cutoff move when meaningful.
  SearchPath<Move> principal_variation;  ///< PV snapshot for iteration-end events.
};

/** @brief Concept for adversarial-search trace sinks. */
template <typename Tracer, typename Move, typename Score>
concept AdversarialSearchTracer
  = SearchMove<Move> and AdversarialScore<Score>
and requires(Tracer &tracer, const AdversarialTraceEvent<Move, Score> &event) {
      { tracer(event) } -> std::same_as<void>;
    };

/** @brief Concept for explicit state-key providers used by adversarial TT overloads. */
template <typename Table, typename Keyer, typename State, typename Move, typename Score>
concept AdversarialSearchKeyer
  = SearchState<State> and SearchMove<Move> and AdversarialScore<Score>
and (not AdversarialSearchTracer<Keyer, Move, Score>) and requires(Keyer &keyer, const State &state) {
      typename std::invoke_result_t<Keyer &, const State &>;
      requires AdversarialTranspositionMemo<Table, std::invoke_result_t<Keyer &, const State &>, Move, Score>;
    };

/** @brief Default no-op tracer used when the caller does not request tracing. */
struct Null_Adversarial_Search_Tracer
{
  template <typename Event>
  void operator()(const Event &) const noexcept
  {
    // empty
  }
};

/** @brief Collector that stores trace events in an Aleph list. */
template <SearchMove Move, AdversarialScore Score>
class AdversarialTraceCollector
{
public:
  using Event = AdversarialTraceEvent<Move, Score>;
  using Container_Type = DynList<Event>;

  void operator()(const Event &event)
  {
    events_.append(event);
  }

  void clear() noexcept
  {
    events_.clear();
  }

  [[nodiscard]] size_t size() const noexcept
  {
    return events_.size();
  }

  [[nodiscard]] bool is_empty() const noexcept
  {
    return events_.is_empty();
  }

  [[nodiscard]] const Container_Type &events() const noexcept
  {
    return events_;
  }

private:
  Container_Type events_;
};

/** @brief Aspiration-window configuration for iterative deepening. */
template <AdversarialScore Score>
struct AspirationWindow
{
  Score half_window = Score{};  ///< Initial half-width around the previous root value.
  Score growth = Score{};       ///< Extra widening applied after each failed attempt.

  [[nodiscard]] bool enabled() const noexcept
  {
    return half_window > Score{};
  }
};

/** @brief Controls for adversarial iterative deepening. */
template <AdversarialScore Score>
struct AdversarialIterativeDeepeningOptions
{
  size_t initial_depth = 1;            ///< First horizon to search.
  size_t depth_step = 1;               ///< Increment applied after each completed iteration.
  AspirationWindow<Score> aspiration;  ///< Optional aspiration-window policy.
};

/** @brief Result of one iterative-deepening iteration. */
template <SearchMove Move, AdversarialScore Score>
struct AdversarialIterativeDeepeningIteration
{
  size_t depth = 0;                     ///< Horizon reached by this iteration.
  bool used_aspiration_window = false;  ///< Whether the iteration started from a bounded window.
  size_t aspiration_researches = 0;     ///< Number of retries triggered by aspiration failure.
  Score aspiration_alpha = Score{};     ///< Final lower bound used in the last search attempt.
  Score aspiration_beta = Score{};      ///< Final upper bound used in the last search attempt.
  AdversarialSearchResult<Move, Score> result;  ///< Final result returned for this depth.
  AdversarialSearchStats total_stats;           ///< Aggregate cost of all attempts at this depth.
};

/** @brief Aggregate result of adversarial iterative deepening. */
template <SearchMove Move, AdversarialScore Score>
struct AdversarialIterativeDeepeningResult
{
  AdversarialSearchResult<Move, Score> result;  ///< Deepest completed iteration result.
  Array<AdversarialIterativeDeepeningIteration<Move, Score>> iterations;  ///< Per-depth results.
  AdversarialSearchStats total_stats;  ///< Aggregate cost across all iterations and retries.
  size_t completed_iterations = 0;     ///< Number of iterations stored in `iterations`.
  size_t aspiration_researches = 0;    ///< Total number of aspiration retries.

  [[nodiscard]] bool has_iterations() const noexcept
  {
    return not iterations.is_empty();
  }
};

/** @brief Required evaluator contract for zero-sum games.
 *
 *  `evaluate(state)` must return a score from the perspective of the player to
 *  move in `state`. This is the key convention that keeps Negamax and
 *  Alpha-Beta compact and symmetric.
 */
template <typename Domain>
concept GameEvaluator = requires(Domain &domain, const typename Domain::State &state) {
  typename Domain::Score;
  requires AdversarialScore<typename Domain::Score>;
  { domain.is_terminal(state) } -> std::convertible_to<bool>;
  { domain.evaluate(state) } -> std::convertible_to<typename Domain::Score>;
};

/** @brief Minimal contract for alternating-turn zero-sum games.
 *
 *  A game domain must:
 *
 *  - expose nested `State`, `Move` and `Score` types,
 *  - lazily generate legal moves through `for_each_successor()`,
 *  - mutate the current state with `apply()` / `undo()`,
 *  - recognize terminal positions with `is_terminal()`,
 *  - evaluate any state with `evaluate()`, always from the current player's
 *    perspective.
 *
 *  The engine assumes `apply()` / `undo()` switch turns implicitly through the
 *  state itself; no separate player parameter is carried by the search code.
 */
template <typename Domain>
concept AdversarialGameDomain
  = SuccessorGenerator<Domain> and GameEvaluator<Domain>
and requires(Domain &domain, typename Domain::State &state, const typename Domain::Move &move) {
      requires SearchState<typename Domain::State>;
      requires SearchMove<typename Domain::Move>;
      { domain.apply(state, move) } -> std::same_as<void>;
      { domain.undo(state, move) } -> std::same_as<void>;
    };

namespace adversarial_search_detail {

template <typename Entry>
struct Null_Transposition_Table
{
  static constexpr bool enabled = false;

  template <typename Key>
  [[nodiscard]] Entry *probe(const Key &) noexcept
  {
    return nullptr;
  }

  template <typename Key>
  [[nodiscard]] TranspositionStoreStatus store(const Key &, const Entry &) noexcept
  {
    return TranspositionStoreStatus::Rejected;
  }
};

struct Dummy_State_Key
{
  template <typename State>
  [[nodiscard]] constexpr size_t operator()(const State &) const noexcept
  {
    return 0;
  }
};

template <typename Domain, bool Supported = MoveKeyProvider<Domain>>
struct History_Table_Selector
{
  using Type = Null_History_Heuristic_Table;
};

template <typename Domain>
struct History_Table_Selector<Domain, true>
{
  using Type = History_Heuristic_Table<typename Domain::Move_Key>;
};

template <AdversarialScore Score>
[[nodiscard]] constexpr Score score_floor() noexcept
{
  return std::numeric_limits<Score>::lowest() / Score(2);
}

template <AdversarialScore Score>
[[nodiscard]] constexpr Score score_ceiling() noexcept
{
  return std::numeric_limits<Score>::max() / Score(2);
}

template <typename Result>
void mark_limit_reached(Result &result)
{
  if (result.status != SearchStatus::LimitReached)
    {
      ++result.stats.limit_hits;
      result.status = SearchStatus::LimitReached;
    }
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

template <SearchMove Move>
[[nodiscard]] SearchPath<Move> prepend_move(const Move &move, const SearchPath<Move> &tail)
{
  SearchPath<Move> path;
  path.reserve(tail.size() + 1);
  path.append(move);
  for (const auto &item : tail)
    path.append(item);
  return path;
}

template <SearchMove Move, AdversarialScore Score>
struct NodeEvaluation
{
  Score value = Score{};
  SearchPath<Move> principal_variation;
};

template <typename Result>
void register_visit(const size_t depth, Result &result)
{
  ++result.stats.visited_states;
  if (depth > result.stats.max_depth_reached)
    result.stats.max_depth_reached = depth;
}

template <typename Domain, typename Result>
[[nodiscard]] auto evaluate_leaf(Domain &domain, const typename Domain::State &state, Result &result)
{
  using Move = typename Domain::Move;
  using Score = typename Domain::Score;

  ++result.stats.heuristic_evaluations;

  NodeEvaluation<Move, Score> out;
  out.value = domain.evaluate(state);
  return out;
}

[[nodiscard]] inline constexpr size_t remaining_depth(const SearchLimits &limits,
                                                      const size_t depth) noexcept
{
  return limits.max_depth == Search_Unlimited ? Search_Unlimited : limits.max_depth - depth;
}

inline void accumulate_move_ordering_stats(MoveOrderingStats &dst, const MoveOrderingStats &src) noexcept
{
  dst.ordered_batches += src.ordered_batches;
  dst.ordered_moves += src.ordered_moves;
  dst.priority_estimates += src.priority_estimates;
  dst.killer_hits += src.killer_hits;
  dst.history_hits += src.history_hits;
}

inline void accumulate_transposition_stats(TranspositionStats &dst,
                                           const TranspositionStats &src) noexcept
{
  dst.probes += src.probes;
  dst.hits += src.hits;
  dst.misses += src.misses;
  dst.cutoffs += src.cutoffs;
  dst.stores += src.stores;
  dst.replacements += src.replacements;
  dst.rejected_updates += src.rejected_updates;
}

inline void accumulate_search_stats(SearchStats &dst, const SearchStats &src) noexcept
{
  dst.visited_states += src.visited_states;
  dst.expanded_states += src.expanded_states;
  dst.generated_successors += src.generated_successors;
  dst.solutions_found += src.solutions_found;
  dst.terminal_states += src.terminal_states;
  dst.pruned_by_depth += src.pruned_by_depth;
  dst.pruned_by_domain += src.pruned_by_domain;
  dst.pruned_by_visited += src.pruned_by_visited;
  dst.limit_hits += src.limit_hits;
  if (src.max_depth_reached > dst.max_depth_reached)
    dst.max_depth_reached = src.max_depth_reached;
  dst.elapsed_ms += src.elapsed_ms;
  accumulate_move_ordering_stats(dst.move_ordering, src.move_ordering);
}

inline void accumulate_adversarial_stats(AdversarialSearchStats &dst,
                                         const AdversarialSearchStats &src) noexcept
{
  accumulate_search_stats(dst, src);
  dst.heuristic_evaluations += src.heuristic_evaluations;
  dst.alpha_beta_cutoffs += src.alpha_beta_cutoffs;
  accumulate_transposition_stats(dst.transpositions, src.transpositions);
}

template <SearchMove Move, AdversarialScore Score, typename Tracer>
  requires AdversarialSearchTracer<Tracer, Move, Score>
void emit_trace(Tracer &tracer,
                const AdversarialTraceEventKind kind,
                const size_t depth,
                const size_t remaining,
                const size_t iteration = 0,
                const size_t horizon = 0,
                const size_t aspiration_retries = 0,
                const Score value = Score{},
                const Score alpha = Score{},
                const Score beta = Score{},
                const std::optional<Move> &move = std::nullopt,
                const SearchPath<Move> *principal_variation = nullptr)
{
  AdversarialTraceEvent<Move, Score> event;
  event.kind = kind;
  event.depth = depth;
  event.remaining_depth = remaining;
  event.iteration = iteration;
  event.horizon = horizon;
  event.aspiration_retries = aspiration_retries;
  event.value = value;
  event.alpha = alpha;
  event.beta = beta;
  event.move = move;
  if (principal_variation != nullptr)
    event.principal_variation = *principal_variation;
  tracer(event);
}

template <SearchMove Move>
[[nodiscard]] std::optional<Move> first_move_of(const SearchPath<Move> &path)
{
  if (path.is_empty())
    return std::nullopt;

  return std::optional<Move>{path[0]};
}

template <AdversarialScore Score>
[[nodiscard]] constexpr Score saturating_subtract(const Score value, const Score delta) noexcept
{
  const Score floor = score_floor<Score>();
  if (delta <= Score{})
    return value;
  if (value <= floor + delta)
    return floor;
  return value - delta;
}

template <AdversarialScore Score>
[[nodiscard]] constexpr Score saturating_add(const Score value, const Score delta) noexcept
{
  const Score ceiling = score_ceiling<Score>();
  if (delta <= Score{})
    return value;
  if (value >= ceiling - delta)
    return ceiling;
  return value + delta;
}

template <AdversarialScore Score>
[[nodiscard]] constexpr Score grow_aspiration_half_window(const Score current,
                                                          const AspirationWindow<Score> &window) noexcept
{
  const Score step
    = window.growth > Score{} ? window.growth : (current > Score{} ? current : Score{1});
  return saturating_add(current, step);
}

}  // end namespace adversarial_search_detail

/** @brief Pure Negamax search over an implicit game tree.
 *
 *  Negamax is the canonical base for two-player zero-sum search because a
 *  single maximization recurrence is enough once evaluation is defined from the
 *  side-to-move perspective.
 *
 *  @tparam Domain game adapter exposing adversarial-search hooks.
 */
template <AdversarialGameDomain Domain>
class Negamax
{
public:
  /// Type of the problem domain.
  using Domain_Type = Domain;
  /// Concrete search state type.
  using State = typename Domain::State;
  /// Move type.
  using Move = typename Domain::Move;
  /// Score type used for board evaluation.
  using Score = typename Domain::Score;
  /// Outcome of an adversarial search execution.
  using Result = AdversarialSearchResult<Move, Score>;

  /** @brief Return the default exploration policy for adversarial search. */
  [[nodiscard]] static ExplorationPolicy default_policy() noexcept
  {
    ExplorationPolicy policy;
    policy.strategy = ExplorationPolicy::Strategy::Depth_First;
    policy.stop_at_first_solution = false;
    return policy;
  }

  /** @brief Build a Negamax engine bound to one game domain. */
  explicit Negamax(Domain domain, ExplorationPolicy policy = default_policy(), SearchLimits limits = {})
    : domain_(std::move(domain)), policy_(policy), limits_(limits)
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

  void set_policy(const ExplorationPolicy &policy) noexcept
  {
    policy_ = policy;
  }

  void set_limits(const SearchLimits &limits) noexcept
  {
    limits_ = limits;
  }

  /** @brief Execute Negamax from `initial_state`.
   *
   *  @param[in] initial_state Root position. Its side to move defines the
   *             perspective of the returned score.
   *  @return One principal variation and its root score.
   *
   *  @throws std::invalid_argument if a non-depth-first exploration strategy
   *          is requested or common limits are malformed.
   */
  [[nodiscard]] Result search(State initial_state)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search(std::move(initial_state), tracer);
  }

  /** @brief Execute Negamax from `initial_state` while emitting trace events. */
  template <typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search(State initial_state, Tracer &tracer)
  {
    adversarial_search_detail::Null_Transposition_Table<AdversarialTranspositionEntry<Move, Score>> table;
    adversarial_search_detail::Dummy_State_Key keyer;
    return search_impl(std::move(initial_state), table, keyer, tracer);
  }

  /** @brief Execute Negamax with a transposition table and explicit keyer.
   *
   *  `keyer(state)` must return a stable key that identifies search-equivalent
   *  states. The engine internally combines it with remaining depth to keep the
   *  cache consistent with the configured horizon.
   */
  template <typename Table, typename Keyer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table, Keyer keyer)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search(std::move(initial_state), table, keyer, tracer);
  }

  /** @brief Execute Negamax with TT/keyer and a trace sink. */
  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
         and AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table, Keyer keyer, Tracer &tracer)
  {
    return search_impl(std::move(initial_state), table, keyer, tracer);
  }

  /** @brief Execute Negamax with a transposition table using `domain().state_key()`. */
  template <typename Table>
    requires SearchStateKeyProvider<Domain>
         and AdversarialTranspositionMemo<Table, typename Domain::State_Key, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table)
  {
    Null_Adversarial_Search_Tracer tracer;
    auto keyer = [this](const State &state)
    {
      return domain_.state_key(state);
    };

    return search_impl(std::move(initial_state), table, keyer, tracer);
  }

  /** @brief Execute Negamax with domain-provided key and a trace sink. */
  template <typename Table, typename Tracer>
    requires SearchStateKeyProvider<Domain>
         and AdversarialTranspositionMemo<Table, typename Domain::State_Key, Move, Score>
         and AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table, Tracer &tracer)
  {
    auto keyer = [this](const State &state)
    {
      return domain_.state_key(state);
    };

    return search_impl(std::move(initial_state), table, keyer, tracer);
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
  bool stop_ = false;

  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search_impl(State initial_state, Table &table, Keyer &keyer, Tracer &tracer)
  {
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "Negamax only supports depth-first exploration";
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";
    ah_invalid_argument_if(policy_.move_ordering != MoveOrderingMode::Domain
                           or policy_.use_killer_moves or policy_.use_history_heuristic)
      << "Negamax currently preserves domain successor order; move ordering is "
         "implemented in Alpha_Beta";

    Result result;
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    stop_ = false;
    auto root = search_node(initial_state, 0, result, table, keyer, tracer);
    result.value = root.value;
    result.principal_variation = std::move(root.principal_variation);

    if (result.status == SearchStatus::NotStarted)
      result.status = SearchStatus::Exhausted;

    result.stats.elapsed_ms = search_elapsed_ms(SearchClock::now() - start_time);

    return result;
  }

  [[nodiscard]] bool expansion_limit_reached(Result &result)
  {
    if (result.stats.expanded_states >= limits_.max_expansions)
      {
        adversarial_search_detail::mark_limit_reached(result);
        stop_ = true;
        return true;
      }

    return false;
  }

  template <typename Table, typename Keyer>
  void store_transposition(State &state,
                           const size_t remaining,
                           Result &result,
                           Table &table,
                           Keyer &keyer,
                           const adversarial_search_detail::NodeEvaluation<Move, Score> &value,
                           const TranspositionBound bound)
  {
    if constexpr (Table::enabled)
      {
        if (stop_ or result.limit_reached())
          return;

        AdversarialTranspositionEntry<Move, Score> entry;
        entry.value = value.value;
        entry.depth = remaining;
        entry.bound = bound;
        entry.principal_variation = value.principal_variation;

        const auto status = table.store(typename Table::Key_Type{keyer(state), remaining}, entry);
        if (status != TranspositionStoreStatus::Rejected)
          ++result.stats.transpositions.stores;
        if (status == TranspositionStoreStatus::Replaced)
          ++result.stats.transpositions.replacements;
      }
  }

  template <typename Table, typename Keyer>
  [[nodiscard]] bool probe_transposition(State &state,
                                         const size_t remaining,
                                         Result &result,
                                         Table &table,
                                         Keyer &keyer,
                                         adversarial_search_detail::NodeEvaluation<Move, Score> &out)
  {
    if constexpr (Table::enabled)
      {
        ++result.stats.transpositions.probes;

        if (auto *entry = table.probe(typename Table::Key_Type{keyer(state), remaining});
            entry != nullptr)
          {
            ++result.stats.transpositions.hits;
            if (entry->bound == TranspositionBound::Exact)
              {
                ++result.stats.transpositions.cutoffs;
                out.value = entry->value;
                out.principal_variation = entry->principal_variation;
                return true;
              }
          }
        else
          ++result.stats.transpositions.misses;
      }

    return false;
  }

  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] adversarial_search_detail::NodeEvaluation<Move, Score> search_node(
    State &state, const size_t depth, Result &result, Table &table, Keyer &keyer, Tracer &tracer)
  {
    using adversarial_search_detail::emit_trace;
    using adversarial_search_detail::evaluate_leaf;
    using adversarial_search_detail::first_move_of;
    using adversarial_search_detail::prepend_move;
    using adversarial_search_detail::register_visit;
    using adversarial_search_detail::remaining_depth;

    register_visit(depth, result);

    const size_t remaining = remaining_depth(limits_, depth);
    emit_trace<Move, Score>(tracer, AdversarialTraceEventKind::Enter_Node, depth, remaining);

    adversarial_search_detail::NodeEvaluation<Move, Score> cached;
    if (probe_transposition(state, remaining, result, table, keyer, cached))
      {
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Transposition_Hit,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                cached.value,
                                Score{},
                                Score{},
                                first_move_of(cached.principal_variation));
        return cached;
      }

    if (domain_.is_terminal(state))
      {
        ++result.stats.terminal_states;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(
          tracer, AdversarialTraceEventKind::Terminal_Node, depth, remaining, 0, 0, 0, leaf.value);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(
          tracer, AdversarialTraceEventKind::Depth_Cutoff, depth, remaining, 0, 0, 0, leaf.value);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (adversarial_search_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(
          tracer, AdversarialTraceEventKind::Domain_Prune, depth, remaining, 0, 0, 0, leaf.value);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (expansion_limit_reached(result))
      {
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(
          tracer, AdversarialTraceEventKind::Expansion_Limit, depth, remaining, 0, 0, 0, leaf.value);
        return leaf;
      }

    adversarial_search_detail::NodeEvaluation<Move, Score> best;
    bool has_move = false;
    bool has_best = false;
    bool counted_expansion = false;

    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      if (stop_)
        return false;

      if (not counted_expansion)
        {
          ++result.stats.expanded_states;
          counted_expansion = true;
        }

      has_move = true;
      ++result.stats.generated_successors;
      domain_.apply(state, move);
      auto child = search_node(state, depth + 1, result, table, keyer, tracer);
      domain_.undo(state, move);

      // Negate child score to switch perspective. lowest() is the engine's
      // internal sentinel (never returned by domain evaluators, which are
      // bounded by score_floor()/score_ceiling()); negating lowest() would
      // overflow signed types, so it maps to max() instead.
      const Score candidate = (child.value == std::numeric_limits<Score>::lowest())
                              ? std::numeric_limits<Score>::max()
                              : -child.value;
      if (not has_best or candidate > best.value)
        {
          best.value = candidate;
          best.principal_variation = prepend_move(move, child.principal_variation);
          has_best = true;
        }

      return not stop_;
    });

    if (not has_move)
      {
        ++result.stats.terminal_states;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(
          tracer, AdversarialTraceEventKind::Terminal_Node, depth, remaining, 0, 0, 0, leaf.value);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    emit_trace<Move, Score>(tracer,
                            AdversarialTraceEventKind::Exit_Node,
                            depth,
                            remaining,
                            0,
                            0,
                            0,
                            best.value,
                            Score{},
                            Score{},
                            first_move_of(best.principal_variation));

    store_transposition(state, remaining, result, table, keyer, best, TranspositionBound::Exact);
    return best;
  }
};

/** @brief Convenience wrapper for one-shot Negamax search. */
template <AdversarialGameDomain Domain>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state));
}

/** @brief Convenience wrapper for one-shot Negamax search with tracing. */
template <AdversarialGameDomain Domain, typename Tracer>
  requires AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  Tracer &tracer,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), tracer);
}

/** @brief Convenience wrapper for Negamax search with transposition table. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  Table &table,
                                  Keyer keyer,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, keyer);
}

/** @brief Convenience wrapper for Negamax search with TT/keyer and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer, typename Tracer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  Table &table,
                                  Keyer keyer,
                                  Tracer &tracer,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, keyer, tracer);
}

/** @brief Convenience wrapper for Negamax search with domain-provided state key. */
template <AdversarialGameDomain Domain, typename Table>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  Table &table,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table);
}

/** @brief Convenience wrapper for Negamax search with domain-provided key and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Tracer>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto negamax_search(Domain domain,
                                  typename Domain::State initial_state,
                                  Table &table,
                                  Tracer &tracer,
                                  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
                                  SearchLimits limits = {})
{
  Negamax<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, tracer);
}

/** @brief Iterative deepening over Negamax without transposition table. */
template <AdversarialGameDomain Domain>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  static Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_negamax_search(
    std::move(domain), std::move(initial_state), tracer, policy, limits, options);
}

/** @brief Iterative deepening over Negamax with tracing. */
template <AdversarialGameDomain Domain, typename Tracer>
  requires AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  Tracer &tracer,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  using Move = typename Domain::Move;
  using Score = typename Domain::Score;

  ah_invalid_argument_if(limits.max_depth == Search_Unlimited)
    << "iterative_deepening_negamax_search requires a finite SearchLimits::max_depth";
  ah_invalid_argument_if(options.depth_step == 0)
    << "AdversarialIterativeDeepeningOptions::depth_step must be positive";
  ah_invalid_argument_if(options.initial_depth > limits.max_depth)
    << "AdversarialIterativeDeepeningOptions::initial_depth exceeds SearchLimits::max_depth";
  ah_invalid_argument_if(options.aspiration.enabled())
    << "Aspiration windows are only available through iterative_deepening_alpha_beta_search";

  Negamax<Domain> engine(std::move(domain), policy, limits);
  AdversarialIterativeDeepeningResult<Move, Score> out;
  out.result.policy = policy;
  out.result.limits = limits;
  out.iterations.reserve((limits.max_depth - options.initial_depth) / options.depth_step + 1);

  size_t depth = options.initial_depth;
  size_t iteration_index = 0;
  for (;;)
    {
      adversarial_search_detail::emit_trace<Move, Score>(
        tracer, AdversarialTraceEventKind::Iteration_Begin, 0, depth, iteration_index, depth);

      SearchLimits iteration_limits = limits;
      iteration_limits.max_depth = depth;
      engine.set_limits(iteration_limits);
      auto result = engine.search(initial_state, tracer);

      AdversarialIterativeDeepeningIteration<Move, Score> iteration;
      iteration.depth = depth;
      iteration.result = result;
      iteration.total_stats = result.stats;

      adversarial_search_detail::accumulate_adversarial_stats(out.total_stats, result.stats);
      out.result = iteration.result;
      out.iterations.append(iteration);
      ++out.completed_iterations;

      adversarial_search_detail::emit_trace<Move, Score>(tracer,
                                                         AdversarialTraceEventKind::Iteration_End,
                                                         0,
                                                         depth,
                                                         iteration_index,
                                                         depth,
                                                         0,
                                                         result.value,
                                                         Score{},
                                                         Score{},
                                                         adversarial_search_detail::first_move_of(
                                                           result.principal_variation),
                                                         &result.principal_variation);

      if (result.limit_reached() or depth == limits.max_depth)
        break;

      const size_t next_depth = depth > limits.max_depth - options.depth_step
                                ? limits.max_depth
                                : depth + options.depth_step;
      if (next_depth == depth)
        break;

      depth = next_depth;
      ++iteration_index;
    }

  return out;
}

/** @brief Iterative deepening over Negamax with TT/keyer reuse across iterations. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Keyer keyer,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  static Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_negamax_search(
    std::move(domain), std::move(initial_state), table, keyer, tracer, policy, limits, options);
}

/** @brief Iterative deepening over Negamax with TT/keyer reuse and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer, typename Tracer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Keyer keyer,
  Tracer &tracer,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  using Move = typename Domain::Move;
  using Score = typename Domain::Score;

  ah_invalid_argument_if(limits.max_depth == Search_Unlimited)
    << "iterative_deepening_negamax_search requires a finite SearchLimits::max_depth";
  ah_invalid_argument_if(options.depth_step == 0)
    << "AdversarialIterativeDeepeningOptions::depth_step must be positive";
  ah_invalid_argument_if(options.initial_depth > limits.max_depth)
    << "AdversarialIterativeDeepeningOptions::initial_depth exceeds SearchLimits::max_depth";
  ah_invalid_argument_if(options.aspiration.enabled())
    << "Aspiration windows are only available through iterative_deepening_alpha_beta_search";

  Negamax<Domain> engine(std::move(domain), policy, limits);
  AdversarialIterativeDeepeningResult<Move, Score> out;
  out.result.policy = policy;
  out.result.limits = limits;
  out.iterations.reserve((limits.max_depth - options.initial_depth) / options.depth_step + 1);

  size_t depth = options.initial_depth;
  size_t iteration_index = 0;
  for (;;)
    {
      adversarial_search_detail::emit_trace<Move, Score>(
        tracer, AdversarialTraceEventKind::Iteration_Begin, 0, depth, iteration_index, depth);

      SearchLimits iteration_limits = limits;
      iteration_limits.max_depth = depth;
      engine.set_limits(iteration_limits);
      auto result = engine.search(initial_state, table, keyer, tracer);

      AdversarialIterativeDeepeningIteration<Move, Score> iteration;
      iteration.depth = depth;
      iteration.result = result;
      iteration.total_stats = result.stats;

      adversarial_search_detail::accumulate_adversarial_stats(out.total_stats, result.stats);
      out.result = iteration.result;
      out.iterations.append(iteration);
      ++out.completed_iterations;

      adversarial_search_detail::emit_trace<Move, Score>(tracer,
                                                         AdversarialTraceEventKind::Iteration_End,
                                                         0,
                                                         depth,
                                                         iteration_index,
                                                         depth,
                                                         0,
                                                         result.value,
                                                         Score{},
                                                         Score{},
                                                         adversarial_search_detail::first_move_of(
                                                           result.principal_variation),
                                                         &result.principal_variation);

      if (result.limit_reached() or depth == limits.max_depth)
        break;

      const size_t next_depth = depth > limits.max_depth - options.depth_step
                                ? limits.max_depth
                                : depth + options.depth_step;
      if (next_depth == depth)
        break;

      depth = next_depth;
      ++iteration_index;
    }

  return out;
}

/** @brief Iterative deepening over Negamax using `domain.state_key()`. */
template <AdversarialGameDomain Domain, typename Table>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  static Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_negamax_search(
    std::move(domain), std::move(initial_state), table, tracer, policy, limits, options);
}

/** @brief Iterative deepening over Negamax using `domain.state_key()` and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Tracer>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_negamax_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Tracer &tracer,
  ExplorationPolicy policy = Negamax<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  auto key_domain = domain;
  auto keyer = [key_domain = std::move(key_domain)](const typename Domain::State &state) mutable
  {
    return key_domain.state_key(state);
  };

  return iterative_deepening_negamax_search(
    std::move(domain), std::move(initial_state), table, keyer, tracer, policy, limits, options);
}

}  // end namespace Aleph

# endif  // NEGAMAX_H
