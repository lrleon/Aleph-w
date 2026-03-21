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

/** @file Alpha_Beta.H
 *  @brief Alpha-Beta pruning over the adversarial-search contract.
 *
 *  This module keeps the same game adapter required by @ref Negamax.H but adds
 *  Alpha-Beta cutoffs on top of the same depth-first recurrence and result
 *  vocabulary. Through @ref Aleph::ExplorationPolicy it also supports opt-in move
 *  ordering by shallow score estimate plus lightweight killer/history hooks.
 *
 *  @author Leandro Rabindranath León
 */

# ifndef ALPHA_BETA_H
# define ALPHA_BETA_H

#include <utility>

#include <ah-errors.H>
#include <Negamax.H>

namespace Aleph {

/** @brief Negamax with Alpha-Beta pruning.
 *
 *  The domain contract and score semantics are identical to @ref Negamax:
 *  `evaluate(state)` must always be expressed from the side-to-move
 *  perspective, while `apply()` / `undo()` alternate turns through the state.
 *
 *  If `policy.move_ordering == MoveOrderingMode::Estimated_Score`, the engine
 *  materializes each successor batch, scores children by a one-ply estimate,
 *  and reorders them before search. Optional killer/history hooks can further
 *  bias that ordering when explicitly enabled.
 */
/** @brief Adversarial search engine with Alpha-Beta pruning.
 *
 *  Alpha-Beta improves upon pure Negamax by pruning branches that are
 *  guaranteed to be worse than a previously discovered line of play.
 *
 *  @tparam Domain game adapter exposing adversarial-search hooks.
 */
template <AdversarialGameDomain Domain>
class Alpha_Beta
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
  /// Outcome of an Alpha-Beta search execution.
  using Result = AdversarialSearchResult<Move, Score>;
  using Killer_Table = Killer_Move_Table<Move>;
  using History_Table = typename adversarial_search_detail::History_Table_Selector<Domain>::Type;

  /** @brief Return the default exploration policy for Alpha-Beta. */
  [[nodiscard]] static ExplorationPolicy default_policy() noexcept
  {
    return Negamax<Domain>::default_policy();
  }

  /** @brief Build an Alpha-Beta engine bound to one game domain. */
  explicit Alpha_Beta(Domain domain,
                      ExplorationPolicy policy = default_policy(),
                      SearchLimits limits = {})
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

  /** @brief Execute Alpha-Beta from `initial_state`.
   *
   *  @param[in] initial_state Root position.
   *  @return Root score and one principal variation.
   *
   *  @throws std::invalid_argument if unsupported common-policy values are
   *          requested.
   */
  [[nodiscard]] Result search(State initial_state)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search(std::move(initial_state), tracer);
  }

  /** @brief Execute Alpha-Beta from `initial_state` while emitting trace events. */
  template <typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search(State initial_state, Tracer &tracer)
  {
    adversarial_search_detail::Null_Transposition_Table<AdversarialTranspositionEntry<Move, Score>> table;
    adversarial_search_detail::Dummy_State_Key keyer;
    return search_impl(std::move(initial_state),
                       table,
                       keyer,
                       tracer,
                       adversarial_search_detail::score_floor<Score>(),
                       adversarial_search_detail::score_ceiling<Score>());
  }

  /** @brief Execute Alpha-Beta with an explicit root window. */
  [[nodiscard]] Result search_with_window(State initial_state, const Score alpha, const Score beta)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search_with_window(std::move(initial_state), alpha, beta, tracer);
  }

  /** @brief Execute Alpha-Beta with an explicit root window and tracing. */
  template <typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search_with_window(State initial_state,
                                          const Score alpha,
                                          const Score beta,
                                          Tracer &tracer)
  {
    adversarial_search_detail::Null_Transposition_Table<AdversarialTranspositionEntry<Move, Score>> table;
    adversarial_search_detail::Dummy_State_Key keyer;
    return search_impl(std::move(initial_state), table, keyer, tracer, alpha, beta);
  }

  /** @brief Execute Alpha-Beta with a transposition table and explicit keyer. */
  template <typename Table, typename Keyer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table, Keyer keyer)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search(std::move(initial_state), table, keyer, tracer);
  }

  /** @brief Execute Alpha-Beta with TT/keyer and tracing. */
  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
         and AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search(State initial_state, Table &table, Keyer keyer, Tracer &tracer)
  {
    return search_impl(std::move(initial_state),
                       table,
                       keyer,
                       tracer,
                       adversarial_search_detail::score_floor<Score>(),
                       adversarial_search_detail::score_ceiling<Score>());
  }

  /** @brief Execute Alpha-Beta with TT/keyer and an explicit root window. */
  template <typename Table, typename Keyer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
  [[nodiscard]] Result search_with_window(
    State initial_state, Table &table, Keyer keyer, const Score alpha, const Score beta)
  {
    Null_Adversarial_Search_Tracer tracer;
    return search_with_window(std::move(initial_state), table, keyer, alpha, beta, tracer);
  }

  /** @brief Execute Alpha-Beta with TT/keyer, root window and tracing. */
  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchKeyer<Table, Keyer, State, Move, Score>
         and AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search_with_window(State initial_state,
                                          Table &table,
                                          Keyer keyer,
                                          const Score alpha,
                                          const Score beta,
                                          Tracer &tracer)
  {
    return search_impl(std::move(initial_state), table, keyer, tracer, alpha, beta);
  }

  /** @brief Execute Alpha-Beta with a transposition table using `domain().state_key()`. */
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

    return search_impl(std::move(initial_state),
                       table,
                       keyer,
                       tracer,
                       adversarial_search_detail::score_floor<Score>(),
                       adversarial_search_detail::score_ceiling<Score>());
  }

  /** @brief Execute Alpha-Beta with domain-provided key and tracing. */
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

    return search_impl(std::move(initial_state),
                       table,
                       keyer,
                       tracer,
                       adversarial_search_detail::score_floor<Score>(),
                       adversarial_search_detail::score_ceiling<Score>());
  }

  /** @brief Execute Alpha-Beta with domain-provided key and explicit root window. */
  template <typename Table>
    requires SearchStateKeyProvider<Domain>
         and AdversarialTranspositionMemo<Table, typename Domain::State_Key, Move, Score>
  [[nodiscard]] Result search_with_window(State initial_state,
                                          Table &table,
                                          const Score alpha,
                                          const Score beta)
  {
    Null_Adversarial_Search_Tracer tracer;
    auto keyer = [this](const State &state)
    {
      return domain_.state_key(state);
    };

    return search_impl(std::move(initial_state), table, keyer, tracer, alpha, beta);
  }

  /** @brief Execute Alpha-Beta with domain-provided key, root window and tracing. */
  template <typename Table, typename Tracer>
    requires SearchStateKeyProvider<Domain>
         and AdversarialTranspositionMemo<Table, typename Domain::State_Key, Move, Score>
         and AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search_with_window(
    State initial_state, Table &table, const Score alpha, const Score beta, Tracer &tracer)
  {
    auto keyer = [this](const State &state)
    {
      return domain_.state_key(state);
    };

    return search_impl(std::move(initial_state), table, keyer, tracer, alpha, beta);
  }

private:
  Domain domain_;
  ExplorationPolicy policy_;
  SearchLimits limits_;
  bool stop_ = false;
  Killer_Table killer_moves_;
  History_Table history_moves_;

  [[nodiscard]] bool ordering_active() const noexcept
  {
    return policy_.move_ordering != MoveOrderingMode::Domain or policy_.use_killer_moves
        or policy_.use_history_heuristic;
  }

  void validate_ordering_configuration() const
  {
    ah_invalid_argument_if(policy_.move_ordering == MoveOrderingMode::Estimated_Bound)
      << "Alpha_Beta does not support MoveOrderingMode::Estimated_Bound";

    if constexpr (not Killer_Table::supported)
      ah_invalid_argument_if(policy_.use_killer_moves)
        << "Alpha_Beta killer heuristic requires equality-comparable Move";

    if constexpr (not MoveKeyProvider<Domain>)
      ah_invalid_argument_if(policy_.use_history_heuristic)
        << "Alpha_Beta history heuristic requires domain.move_key(move)";
  }

  void clear_ordering_heuristics()
  {
    killer_moves_.clear();
    history_moves_.clear();
  }

  [[nodiscard]] static size_t history_bonus(const size_t depth, const size_t remaining) noexcept
  {
    const size_t base = remaining == Search_Unlimited ? depth + 1 : remaining + 1;
    return base * base;
  }

  [[nodiscard]] size_t move_history_score(const Move &move) const noexcept
  {
    if constexpr (MoveKeyProvider<Domain>)
      if (policy_.use_history_heuristic)
        return history_moves_.score(domain_.move_key(move));

    return 0;
  }

  void record_cutoff_move(const size_t depth, const size_t remaining, const Move &move)
  {
    if (policy_.use_killer_moves)
      killer_moves_.record(depth, move);

    if constexpr (MoveKeyProvider<Domain>)
      if (policy_.use_history_heuristic)
        history_moves_.record(domain_.move_key(move), history_bonus(depth, remaining));
  }

  [[nodiscard]] Array<RankedMove<Move, Score>> collect_ordered_moves(State &state,
                                                                     const size_t depth,
                                                                     Result &result)
  {
    Array<RankedMove<Move, Score>> moves;
    size_t ordinal = 0;

    (void) domain_.for_each_successor(state,
                                      [&](const Move &move) -> bool
    {
      RankedMove<Move, Score> ranked;
      ranked.move = move;
      ranked.ordinal = ordinal++;

      if (policy_.use_killer_moves)
        {
          ranked.killer = killer_moves_.is_killer(depth, move);
          if (ranked.killer)
            ++result.stats.move_ordering.killer_hits;
        }

      if (policy_.use_history_heuristic)
        {
          ranked.history_score = move_history_score(move);
          if (ranked.history_score > 0)
            ++result.stats.move_ordering.history_hits;
        }

      if (policy_.move_ordering == MoveOrderingMode::Estimated_Score)
        {
          domain_.apply(state, move);
          ranked.priority = -domain_.evaluate(state);
          domain_.undo(state, move);
          ++result.stats.heuristic_evaluations;
          ++result.stats.move_ordering.priority_estimates;
        }

      moves.append(std::move(ranked));
      return true;
    });

    if (not moves.is_empty())
      {
        ++result.stats.move_ordering.ordered_batches;
        result.stats.move_ordering.ordered_moves += moves.size();
      }

    sort_ranked_moves(moves,
                      [](const Score &lhs, const Score &rhs) noexcept
    {
      return lhs > rhs;
    },
                      policy_.use_killer_moves,
                      policy_.use_history_heuristic);

    return moves;
  }

  template <typename Table, typename Keyer, typename Tracer>
    requires AdversarialSearchTracer<Tracer, Move, Score>
  [[nodiscard]] Result search_impl(State initial_state,
                                   Table &table,
                                   Keyer &keyer,
                                   Tracer &tracer,
                                   const Score alpha,
                                   const Score beta)
  {
    ah_invalid_argument_if(policy_.strategy != ExplorationPolicy::Strategy::Depth_First)
      << "Alpha_Beta only supports depth-first exploration";
    ah_invalid_argument_if(limits_.max_solutions == 0)
      << "SearchLimits::max_solutions must be positive or Search_Unlimited";
    ah_invalid_argument_if(alpha >= beta) << "Alpha_Beta root window requires alpha < beta";
    validate_ordering_configuration();

    Result result;
    result.policy = policy_;
    result.limits = limits_;
    const auto start_time = SearchClock::now();

    stop_ = false;
    clear_ordering_heuristics();
    auto root = search_node(initial_state, 0, result, table, keyer, tracer, alpha, beta);
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
                                         Score &alpha,
                                         Score &beta,
                                         adversarial_search_detail::NodeEvaluation<Move, Score> &out)
  {
    if constexpr (Table::enabled)
      {
        ++result.stats.transpositions.probes;

        if (auto *entry = table.probe(typename Table::Key_Type{keyer(state), remaining});
            entry != nullptr)
          {
            ++result.stats.transpositions.hits;

            switch (entry->bound)
              {
              case TranspositionBound::Exact:
                ++result.stats.transpositions.cutoffs;
                out.value = entry->value;
                out.principal_variation = entry->principal_variation;
                return true;

              case TranspositionBound::Lower_Bound:
                if (entry->value > alpha)
                  alpha = entry->value;
                break;

              case TranspositionBound::Upper_Bound:
                if (entry->value < beta)
                  beta = entry->value;
                break;
              }

            if (alpha >= beta)
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
  [[nodiscard]] adversarial_search_detail::NodeEvaluation<Move, Score> search_node(State &state,
                                                                                   const size_t depth,
                                                                                   Result &result,
                                                                                   Table &table,
                                                                                   Keyer &keyer,
                                                                                   Tracer &tracer,
                                                                                   const Score alpha,
                                                                                   const Score beta)
  {
    using adversarial_search_detail::emit_trace;
    using adversarial_search_detail::evaluate_leaf;
    using adversarial_search_detail::first_move_of;
    using adversarial_search_detail::prepend_move;
    using adversarial_search_detail::register_visit;
    using adversarial_search_detail::remaining_depth;

    register_visit(depth, result);

    const size_t remaining = remaining_depth(limits_, depth);
    emit_trace<Move, Score>(
      tracer, AdversarialTraceEventKind::Enter_Node, depth, remaining, 0, 0, 0, Score{}, alpha, beta);
    Score local_alpha = alpha;
    Score local_beta = beta;
    const Score alpha_orig = alpha;
    const Score beta_orig = beta;
    adversarial_search_detail::NodeEvaluation<Move, Score> cached;
    if (probe_transposition(state, remaining, result, table, keyer, local_alpha, local_beta, cached))
      {
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Transposition_Hit,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                cached.value,
                                local_alpha,
                                local_beta,
                                first_move_of(cached.principal_variation));
        return cached;
      }

    if (domain_.is_terminal(state))
      {
        ++result.stats.terminal_states;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Terminal_Node,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                leaf.value,
                                local_alpha,
                                local_beta);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (depth >= limits_.max_depth)
      {
        ++result.stats.pruned_by_depth;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Depth_Cutoff,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                leaf.value,
                                local_alpha,
                                local_beta);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (adversarial_search_detail::should_prune_state(domain_, state, depth))
      {
        ++result.stats.pruned_by_domain;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Domain_Prune,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                leaf.value,
                                local_alpha,
                                local_beta);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    if (expansion_limit_reached(result))
      {
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Expansion_Limit,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                leaf.value,
                                local_alpha,
                                local_beta);
        return leaf;
      }

    adversarial_search_detail::NodeEvaluation<Move, Score> best;
    bool has_move = false;
    bool has_best = false;
    bool counted_expansion = false;

    auto explore_move = [&](const Move &move) -> bool
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
      auto child
        = search_node(state, depth + 1, result, table, keyer, tracer, -local_beta, -local_alpha);
      domain_.undo(state, move);

      const Score candidate = -child.value;
      if (not has_best or candidate > best.value)
        {
          best.value = candidate;
          best.principal_variation = prepend_move(move, child.principal_variation);
          has_best = true;
        }

      if (candidate > local_alpha)
        local_alpha = candidate;

      if (local_alpha >= local_beta)
        {
          ++result.stats.alpha_beta_cutoffs;
          record_cutoff_move(depth, remaining, move);
          emit_trace<Move, Score>(tracer,
                                  AdversarialTraceEventKind::Alpha_Beta_Cutoff,
                                  depth,
                                  remaining,
                                  0,
                                  0,
                                  0,
                                  best.value,
                                  local_alpha,
                                  local_beta,
                                  std::optional<Move>{move});
          return false;
        }

      return not stop_;
    };

    if (ordering_active())
      {
        for (auto ordered_moves = collect_ordered_moves(state, depth, result);
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

    if (not has_move)
      {
        ++result.stats.terminal_states;
        auto leaf = evaluate_leaf(domain_, state, result);
        emit_trace<Move, Score>(tracer,
                                AdversarialTraceEventKind::Terminal_Node,
                                depth,
                                remaining,
                                0,
                                0,
                                0,
                                leaf.value,
                                local_alpha,
                                local_beta);
        store_transposition(state, remaining, result, table, keyer, leaf, TranspositionBound::Exact);
        return leaf;
      }

    auto bound = TranspositionBound::Exact;
    if (best.value <= alpha_orig)
      bound = TranspositionBound::Upper_Bound;
    else if (best.value >= beta_orig)
      bound = TranspositionBound::Lower_Bound;

    emit_trace<Move, Score>(tracer,
                            AdversarialTraceEventKind::Exit_Node,
                            depth,
                            remaining,
                            0,
                            0,
                            0,
                            best.value,
                            local_alpha,
                            local_beta,
                            first_move_of(best.principal_variation));

    store_transposition(state, remaining, result, table, keyer, best, bound);
    return best;
  }
};

/** @brief Convenience wrapper for one-shot Alpha-Beta search. */
template <AdversarialGameDomain Domain>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state));
}

/** @brief Convenience wrapper for one-shot Alpha-Beta search with tracing. */
template <AdversarialGameDomain Domain, typename Tracer>
  requires AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     Tracer &tracer,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), tracer);
}

/** @brief Convenience wrapper for Alpha-Beta search with transposition table. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     Table &table,
                                     Keyer keyer,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, keyer);
}

/** @brief Convenience wrapper for Alpha-Beta search with TT/keyer and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer, typename Tracer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     Table &table,
                                     Keyer keyer,
                                     Tracer &tracer,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, keyer, tracer);
}

/** @brief Convenience wrapper for Alpha-Beta search with domain-provided state key. */
template <AdversarialGameDomain Domain, typename Table>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     Table &table,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table);
}

/** @brief Convenience wrapper for Alpha-Beta search with domain-provided key and tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Tracer>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto alpha_beta_search(Domain domain,
                                     typename Domain::State initial_state,
                                     Table &table,
                                     Tracer &tracer,
                                     ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
                                     SearchLimits limits = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return engine.search(std::move(initial_state), table, tracer);
}

/** @brief Iterative deepening over Alpha-Beta with optional aspiration windows. */
template <AdversarialGameDomain Domain, typename Tracer>
  requires AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  Tracer &tracer,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  using Move = typename Domain::Move;
  using Score = typename Domain::Score;
  using adversarial_search_detail::emit_trace;
  using adversarial_search_detail::first_move_of;
  using adversarial_search_detail::grow_aspiration_half_window;
  using adversarial_search_detail::saturating_add;
  using adversarial_search_detail::saturating_subtract;

  ah_invalid_argument_if(limits.max_depth == Search_Unlimited)
    << "iterative_deepening_alpha_beta_search requires a finite SearchLimits::max_depth";
  ah_invalid_argument_if(options.depth_step == 0)
    << "AdversarialIterativeDeepeningOptions::depth_step must be positive";
  ah_invalid_argument_if(options.initial_depth > limits.max_depth)
    << "AdversarialIterativeDeepeningOptions::initial_depth exceeds SearchLimits::max_depth";

  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  AdversarialIterativeDeepeningResult<Move, Score> out;
  out.result.policy = policy;
  out.result.limits = limits;
  out.iterations.reserve((limits.max_depth - options.initial_depth) / options.depth_step + 1);

  const Score full_alpha = adversarial_search_detail::score_floor<Score>();
  const Score full_beta = adversarial_search_detail::score_ceiling<Score>();

  size_t depth = options.initial_depth;
  size_t iteration_index = 0;
  for (;;)
    {
      emit_trace<Move, Score>(
        tracer, AdversarialTraceEventKind::Iteration_Begin, 0, depth, iteration_index, depth);

      SearchLimits iteration_limits = limits;
      iteration_limits.max_depth = depth;
      engine.set_limits(iteration_limits);

      AdversarialIterativeDeepeningIteration<Move, Score> iteration;
      iteration.depth = depth;

      const bool use_aspiration = options.aspiration.enabled() and out.has_iterations();
      iteration.used_aspiration_window = use_aspiration;

      Score center = Score{};
      Score half_window = options.aspiration.half_window;
      if (use_aspiration)
        center = out.result.value;

      for (;;)
        {
          const Score alpha = use_aspiration ? saturating_subtract(center, half_window) : full_alpha;
          const Score beta = use_aspiration ? saturating_add(center, half_window) : full_beta;

          iteration.aspiration_alpha = alpha;
          iteration.aspiration_beta = beta;
          iteration.result = engine.search_with_window(initial_state, alpha, beta, tracer);
          adversarial_search_detail::accumulate_adversarial_stats(iteration.total_stats,
                                                                  iteration.result.stats);

          if (not use_aspiration or iteration.result.limit_reached())
            break;

          if (iteration.result.value <= alpha and alpha != full_alpha)
            {
              ++iteration.aspiration_researches;
              ++out.aspiration_researches;
              half_window = grow_aspiration_half_window(half_window, options.aspiration);
              emit_trace<Move, Score>(tracer,
                                      AdversarialTraceEventKind::Aspiration_Retry,
                                      0,
                                      depth,
                                      iteration_index,
                                      depth,
                                      iteration.aspiration_researches,
                                      iteration.result.value,
                                      saturating_subtract(center, half_window),
                                      saturating_add(center, half_window),
                                      first_move_of(iteration.result.principal_variation));
              continue;
            }

          if (iteration.result.value >= beta and beta != full_beta)
            {
              ++iteration.aspiration_researches;
              ++out.aspiration_researches;
              half_window = grow_aspiration_half_window(half_window, options.aspiration);
              emit_trace<Move, Score>(tracer,
                                      AdversarialTraceEventKind::Aspiration_Retry,
                                      0,
                                      depth,
                                      iteration_index,
                                      depth,
                                      iteration.aspiration_researches,
                                      iteration.result.value,
                                      saturating_subtract(center, half_window),
                                      saturating_add(center, half_window),
                                      first_move_of(iteration.result.principal_variation));
              continue;
            }

          break;
        }

      adversarial_search_detail::accumulate_adversarial_stats(out.total_stats, iteration.total_stats);
      out.result = iteration.result;
      out.iterations.append(iteration);
      ++out.completed_iterations;

      emit_trace<Move, Score>(tracer,
                              AdversarialTraceEventKind::Iteration_End,
                              0,
                              depth,
                              iteration_index,
                              depth,
                              iteration.aspiration_researches,
                              iteration.result.value,
                              iteration.aspiration_alpha,
                              iteration.aspiration_beta,
                              first_move_of(iteration.result.principal_variation),
                              &iteration.result.principal_variation);

      if (iteration.result.limit_reached() or depth == limits.max_depth)
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

/** @brief Iterative deepening over Alpha-Beta with TT/keyer reuse. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer, typename Tracer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Keyer keyer,
  Tracer &tracer,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  using Move = typename Domain::Move;
  using Score = typename Domain::Score;
  using adversarial_search_detail::emit_trace;
  using adversarial_search_detail::first_move_of;
  using adversarial_search_detail::grow_aspiration_half_window;
  using adversarial_search_detail::saturating_add;
  using adversarial_search_detail::saturating_subtract;

  ah_invalid_argument_if(limits.max_depth == Search_Unlimited)
    << "iterative_deepening_alpha_beta_search requires a finite SearchLimits::max_depth";
  ah_invalid_argument_if(options.depth_step == 0)
    << "AdversarialIterativeDeepeningOptions::depth_step must be positive";
  ah_invalid_argument_if(options.initial_depth > limits.max_depth)
    << "AdversarialIterativeDeepeningOptions::initial_depth exceeds SearchLimits::max_depth";

  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  AdversarialIterativeDeepeningResult<Move, Score> out;
  out.result.policy = policy;
  out.result.limits = limits;
  out.iterations.reserve((limits.max_depth - options.initial_depth) / options.depth_step + 1);

  const Score full_alpha = adversarial_search_detail::score_floor<Score>();
  const Score full_beta = adversarial_search_detail::score_ceiling<Score>();

  size_t depth = options.initial_depth;
  size_t iteration_index = 0;
  for (;;)
    {
      emit_trace<Move, Score>(
        tracer, AdversarialTraceEventKind::Iteration_Begin, 0, depth, iteration_index, depth);

      SearchLimits iteration_limits = limits;
      iteration_limits.max_depth = depth;
      engine.set_limits(iteration_limits);

      AdversarialIterativeDeepeningIteration<Move, Score> iteration;
      iteration.depth = depth;

      const bool use_aspiration = options.aspiration.enabled() and out.has_iterations();
      iteration.used_aspiration_window = use_aspiration;

      Score center = Score{};
      Score half_window = options.aspiration.half_window;
      if (use_aspiration)
        center = out.result.value;

      for (;;)
        {
          const Score alpha = use_aspiration ? saturating_subtract(center, half_window) : full_alpha;
          const Score beta = use_aspiration ? saturating_add(center, half_window) : full_beta;

          iteration.aspiration_alpha = alpha;
          iteration.aspiration_beta = beta;
          iteration.result
            = engine.search_with_window(initial_state, table, keyer, alpha, beta, tracer);
          adversarial_search_detail::accumulate_adversarial_stats(iteration.total_stats,
                                                                  iteration.result.stats);

          if (not use_aspiration or iteration.result.limit_reached())
            break;

          if (iteration.result.value <= alpha and alpha != full_alpha)
            {
              ++iteration.aspiration_researches;
              ++out.aspiration_researches;
              half_window = grow_aspiration_half_window(half_window, options.aspiration);
              emit_trace<Move, Score>(tracer,
                                      AdversarialTraceEventKind::Aspiration_Retry,
                                      0,
                                      depth,
                                      iteration_index,
                                      depth,
                                      iteration.aspiration_researches,
                                      iteration.result.value,
                                      saturating_subtract(center, half_window),
                                      saturating_add(center, half_window),
                                      first_move_of(iteration.result.principal_variation));
              continue;
            }

          if (iteration.result.value >= beta and beta != full_beta)
            {
              ++iteration.aspiration_researches;
              ++out.aspiration_researches;
              half_window = grow_aspiration_half_window(half_window, options.aspiration);
              emit_trace<Move, Score>(tracer,
                                      AdversarialTraceEventKind::Aspiration_Retry,
                                      0,
                                      depth,
                                      iteration_index,
                                      depth,
                                      iteration.aspiration_researches,
                                      iteration.result.value,
                                      saturating_subtract(center, half_window),
                                      saturating_add(center, half_window),
                                      first_move_of(iteration.result.principal_variation));
              continue;
            }

          break;
        }

      adversarial_search_detail::accumulate_adversarial_stats(out.total_stats, iteration.total_stats);
      out.result = iteration.result;
      out.iterations.append(iteration);
      ++out.completed_iterations;

      emit_trace<Move, Score>(tracer,
                              AdversarialTraceEventKind::Iteration_End,
                              0,
                              depth,
                              iteration_index,
                              depth,
                              iteration.aspiration_researches,
                              iteration.result.value,
                              iteration.aspiration_alpha,
                              iteration.aspiration_beta,
                              first_move_of(iteration.result.principal_variation),
                              &iteration.result.principal_variation);

      if (iteration.result.limit_reached() or depth == limits.max_depth)
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

/** @brief Iterative deepening over Alpha-Beta using `domain.state_key()`. */
template <AdversarialGameDomain Domain, typename Table, typename Tracer>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
       and AdversarialSearchTracer<Tracer, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Tracer &tracer,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  Alpha_Beta<Domain> engine(std::move(domain), policy, limits);
  return iterative_deepening_alpha_beta_search(engine.domain(),
                                               std::move(initial_state),
                                               table,
                                               [&engine](const typename Domain::State &state)
  {
    return engine.domain().state_key(state);
  },
                                               tracer,
                                               policy,
                                               limits,
                                               options);
}

/** @brief Iterative deepening over Alpha-Beta without tracing. */
template <AdversarialGameDomain Domain>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_alpha_beta_search(
    std::move(domain), std::move(initial_state), tracer, policy, limits, options);
}

/** @brief Iterative deepening over Alpha-Beta with TT/keyer reuse and no tracing. */
template <AdversarialGameDomain Domain, typename Table, typename Keyer>
  requires AdversarialSearchKeyer<Table, Keyer, typename Domain::State, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  Keyer keyer,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_alpha_beta_search(
    std::move(domain), std::move(initial_state), table, keyer, tracer, policy, limits, options);
}

/** @brief Iterative deepening over Alpha-Beta using `domain.state_key()` and no tracing. */
template <AdversarialGameDomain Domain, typename Table>
  requires SearchStateKeyProvider<Domain>
       and AdversarialTranspositionMemo<Table, typename Domain::State_Key, typename Domain::Move, typename Domain::Score>
[[nodiscard]] auto iterative_deepening_alpha_beta_search(
  Domain domain,
  typename Domain::State initial_state,
  Table &table,
  ExplorationPolicy policy = Alpha_Beta<Domain>::default_policy(),
  SearchLimits limits = {},
  AdversarialIterativeDeepeningOptions<typename Domain::Score> options = {})
{
  Null_Adversarial_Search_Tracer tracer;
  return iterative_deepening_alpha_beta_search(
    std::move(domain), std::move(initial_state), table, tracer, policy, limits, options);
}

}  // end namespace Aleph

# endif  // ALPHA_BETA_H
