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

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include <State_Search.H>

using namespace Aleph;

namespace
{

struct ArtificialGameState
{
  size_t depth = 0;
  size_t code = 1;
  int player = 1;
};

struct ArtificialGameMove
{
  size_t next_code = 1;
  char label = 'A';
};

class ArtificialGameDomain
{
public:
  using State = ArtificialGameState;
  using Move = ArtificialGameMove;
  using Score = int;
  using State_Key = std::uint64_t;

  bool is_terminal(const State &state) const
  {
    return state.depth == 2;
  }

  Score evaluate(const State &state) const
  {
    return root_score(state.code)*state.player;
  }

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    return (static_cast<State_Key>(state.code) << 1) |
           static_cast<State_Key>(state.player > 0 ? 1 : 0);
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.next_code;
    state.player = -state.player;
    ++state.depth;
  }

  void undo(State &state, const Move&) const
  {
    --state.depth;
    state.player = -state.player;
    state.code /= 2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= 2)
      return true;

    switch (state.code)
      {
      case 1:
        if (not visit(Move{2, 'A'}))
          return false;
        return visit(Move{3, 'B'});

      case 2:
        if (not visit(Move{4, 'a'}))
          return false;
        return visit(Move{5, 'b'});

      case 3:
        if (not visit(Move{6, 'a'}))
          return false;
        return visit(Move{7, 'b'});

      default:
        return true;
      }
  }

private:
  [[nodiscard]] static Score root_score(const size_t code) noexcept
  {
    switch (code)
      {
      case 2: return 6;
      case 3: return 1;
      case 4: return 3;
      case 5: return 5;
      case 6: return 2;
      case 7: return 4;
      default: return 0;
      }
  }
};

struct TicTacToeState
{
  Array<int> board;
  int player = 1;
  size_t moves_played = 0;

  TicTacToeState() : board(size_t(9), 0)
  {
    // empty
  }
};

class TicTacToeDomain
{
public:
  struct Move
  {
    size_t cell = 0;

    [[nodiscard]] bool operator == (const Move &) const noexcept = default;
  };

  using State = TicTacToeState;
  using Score = int;
  using State_Key = std::uint64_t;
  using Move_Key = size_t;

  bool is_terminal(const State &state) const
  {
    return winner(state) != 0 or state.moves_played == 9;
  }

  Score evaluate(const State &state) const
  {
    const int win = winner(state);
    if (win != 0)
      return win == state.player ? 100 : -100;

    if (state.moves_played == 9)
      return 0;

    return heuristic(state, state.player) - heuristic(state, -state.player);
  }

  [[nodiscard]] State_Key state_key(const State &state) const noexcept
  {
    State_Key key = static_cast<State_Key>(state.player > 0 ? 1 : 2);
    for (size_t i = 0; i < state.board.size(); ++i)
      {
        key *= 3;
        if (state.board[i] == 1)
          key += 1;
        else if (state.board[i] == -1)
          key += 2;
      }

    return key;
  }

  [[nodiscard]] Move_Key move_key(const Move &move) const noexcept
  {
    return move.cell;
  }

  void apply(State &state, const Move &move) const
  {
    state.board[move.cell] = state.player;
    state.player = -state.player;
    ++state.moves_played;
  }

  void undo(State &state, const Move &move) const
  {
    --state.moves_played;
    state.player = -state.player;
    state.board[move.cell] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    static constexpr size_t order[] = {1, 3, 5, 7, 0, 2, 6, 8, 4};

    for (const size_t cell : order)
      if (state.board[cell] == 0 and not visit(Move{cell}))
        return false;

    return true;
  }

private:
  [[nodiscard]] static int winner(const State &state) noexcept
  {
    static constexpr size_t lines[8][3] = {
      {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
      {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
      {0, 4, 8}, {2, 4, 6}
    };

    for (const auto &line : lines)
      {
        const int a = state.board[line[0]];
        if (a != 0 and a == state.board[line[1]] and a == state.board[line[2]])
          return a;
      }

    return 0;
  }

  [[nodiscard]] static Score heuristic(const State &state, const int player) noexcept
  {
    static constexpr size_t lines[8][3] = {
      {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
      {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
      {0, 4, 8}, {2, 4, 6}
    };

    Score total = 0;
    for (const auto &line : lines)
      {
        size_t mine = 0;
        size_t empty = 0;
        bool blocked = false;

        for (const size_t cell : line)
          if (state.board[cell] == player)
            ++mine;
          else if (state.board[cell] == 0)
            ++empty;
          else
            {
              blocked = true;
              break;
            }

        if (blocked)
          continue;

        if (mine == 2 and empty == 1)
          total += 10;
        else if (mine == 1 and empty == 2)
          total += 1;
      }

    return total;
  }
};

static_assert(AdversarialGameDomain<ArtificialGameDomain>);
static_assert(AdversarialGameDomain<TicTacToeDomain>);

std::string path_signature(const SearchPath<ArtificialGameMove> &path)
{
  std::string out;
  for (const auto &move : path)
    out.push_back(move.label);
  return out;
}

template <typename Move, typename Score>
size_t count_trace_events(const AdversarialTraceCollector<Move, Score> &collector,
                          const AdversarialTraceEventKind kind)
{
  size_t count = 0;
  collector.events().for_each(
    [&](const AdversarialTraceEvent<Move, Score> &event)
    {
      if (event.kind == kind)
        ++count;
    });
  return count;
}

TicTacToeState make_tictactoe_state(const char *layout, const int player)
{
  TicTacToeState state;
  state.player = player;

  for (size_t i = 0; i < 9; ++i)
    switch (layout[i])
      {
      case 'X':
        state.board[i] = 1;
        ++state.moves_played;
        break;

      case 'O':
        state.board[i] = -1;
        ++state.moves_played;
        break;

      default:
        state.board[i] = 0;
      }

  return state;
}

} // end namespace

TEST(AdversarialSearchFramework, ArtificialGameNegamaxFindsKnownValueAndPath)
{
  auto result = negamax_search(ArtificialGameDomain{}, ArtificialGameState{});

  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.value, 3);
  EXPECT_EQ(path_signature(result.principal_variation), "Aa");
  EXPECT_EQ(result.stats.visited_states, 7u);
  EXPECT_EQ(result.stats.expanded_states, 3u);
}

TEST(AdversarialSearchFramework, ArtificialGameAlphaBetaMatchesNegamaxAndPrunes)
{
  auto negamax = negamax_search(ArtificialGameDomain{}, ArtificialGameState{});
  auto alpha_beta = alpha_beta_search(ArtificialGameDomain{}, ArtificialGameState{});

  EXPECT_EQ(alpha_beta.value, negamax.value);
  EXPECT_EQ(path_signature(alpha_beta.principal_variation), "Aa");
  EXPECT_LT(alpha_beta.stats.visited_states, negamax.stats.visited_states);
  EXPECT_GT(alpha_beta.stats.alpha_beta_cutoffs, 0u);
}

TEST(AdversarialSearchFramework, ArtificialGameDepthLimitUsesHeuristicEvaluation)
{
  SearchLimits limits;
  limits.max_depth = 1;

  auto result = negamax_search(ArtificialGameDomain{}, ArtificialGameState{}, {}, limits);

  EXPECT_TRUE(result.exhausted());
  EXPECT_EQ(result.value, 6);
  EXPECT_EQ(path_signature(result.principal_variation), "A");
  EXPECT_EQ(result.stats.pruned_by_depth, 2u);
}

TEST(AdversarialSearchFramework, TicTacToeFindsImmediateWinningMove)
{
  const TicTacToeState state = make_tictactoe_state("XX.OO....", 1);

  auto negamax = negamax_search(TicTacToeDomain{}, state);
  auto alpha_beta = alpha_beta_search(TicTacToeDomain{}, state);

  EXPECT_EQ(negamax.value, 100);
  EXPECT_EQ(alpha_beta.value, 100);
  ASSERT_TRUE(alpha_beta.has_principal_variation());
  EXPECT_EQ(alpha_beta.first_move().cell, 2u);
  EXPECT_EQ(negamax.first_move().cell, alpha_beta.first_move().cell);
}

TEST(AdversarialSearchFramework, TicTacToeAlphaBetaMatchesNegamaxAndVisitsFewerNodes)
{
  TicTacToeState state;

  auto negamax = negamax_search(TicTacToeDomain{}, state);
  auto alpha_beta = alpha_beta_search(TicTacToeDomain{}, state);

  EXPECT_EQ(negamax.value, 0);
  EXPECT_EQ(alpha_beta.value, negamax.value);
  ASSERT_TRUE(negamax.has_principal_variation());
  ASSERT_TRUE(alpha_beta.has_principal_variation());
  EXPECT_EQ(alpha_beta.first_move().cell, negamax.first_move().cell);
  EXPECT_LT(alpha_beta.stats.visited_states, negamax.stats.visited_states);
  EXPECT_GT(alpha_beta.stats.alpha_beta_cutoffs, 0u);
}

TEST(AdversarialSearchFramework, InvalidBestFirstPolicyIsRejected)
{
  ExplorationPolicy policy = Negamax<ArtificialGameDomain>::default_policy();
  policy.strategy = ExplorationPolicy::Strategy::Best_First;

  Negamax<ArtificialGameDomain> negamax(ArtificialGameDomain{}, policy);
  Alpha_Beta<ArtificialGameDomain> alpha_beta(ArtificialGameDomain{}, policy);

  EXPECT_THROW((void) negamax.search(ArtificialGameState{}), std::invalid_argument);
  EXPECT_THROW((void) alpha_beta.search(ArtificialGameState{}), std::invalid_argument);
}

TEST(AdversarialSearchFramework, AlphaBetaHeuristicMoveOrderingReducesNodes)
{
  TicTacToeState state;

  Alpha_Beta<TicTacToeDomain> plain_engine(TicTacToeDomain{});
  auto plain = plain_engine.search(state);

  ExplorationPolicy ordered_policy = Alpha_Beta<TicTacToeDomain>::default_policy();
  ordered_policy.move_ordering = MoveOrderingMode::Estimated_Score;
  ordered_policy.use_killer_moves = true;
  ordered_policy.use_history_heuristic = true;

  Alpha_Beta<TicTacToeDomain> ordered_engine(TicTacToeDomain{}, ordered_policy);
  auto ordered = ordered_engine.search(state);

  EXPECT_EQ(ordered.value, plain.value);
  ASSERT_TRUE(plain.has_principal_variation());
  ASSERT_TRUE(ordered.has_principal_variation());
  EXPECT_LT(ordered.stats.visited_states, plain.stats.visited_states);
  EXPECT_EQ(ordered.first_move().cell, 4u);
  EXPECT_GT(ordered.stats.alpha_beta_cutoffs, 0u);
  EXPECT_GT(ordered.stats.move_ordering.ordered_batches, 0u);
  EXPECT_GT(ordered.stats.move_ordering.ordered_moves, 0u);
  EXPECT_GT(ordered.stats.move_ordering.priority_estimates, 0u);
}

TEST(AdversarialSearchFramework, NegamaxTranspositionTablePreservesResultAndReducesNodes)
{
  TicTacToeState state;
  Negamax<TicTacToeDomain> engine(TicTacToeDomain{});

  auto without_tt = engine.search(state);

  using TT = AdversarialTranspositionTable<TicTacToeDomain::State_Key,
                                           TicTacToeDomain::Move,
                                           TicTacToeDomain::Score>;
  TT table;
  auto with_tt = engine.search(state, table);

  EXPECT_EQ(with_tt.value, without_tt.value);
  ASSERT_TRUE(with_tt.has_principal_variation());
  ASSERT_TRUE(without_tt.has_principal_variation());
  EXPECT_EQ(with_tt.first_move().cell, without_tt.first_move().cell);
  EXPECT_LT(with_tt.stats.visited_states, without_tt.stats.visited_states);
  EXPECT_GT(with_tt.stats.transpositions.hits, 0u);
  EXPECT_GT(with_tt.stats.transpositions.cutoffs, 0u);
  EXPECT_GT(table.stats().stores, 0u);
}

TEST(AdversarialSearchFramework, AlphaBetaTranspositionTablePreservesResultAndReducesNodes)
{
  TicTacToeState state;
  Alpha_Beta<TicTacToeDomain> engine(TicTacToeDomain{});

  auto without_tt = engine.search(state);

  using TT = AdversarialTranspositionTable<TicTacToeDomain::State_Key,
                                           TicTacToeDomain::Move,
                                           TicTacToeDomain::Score>;
  TT table;
  auto with_tt = engine.search(state, table);

  EXPECT_EQ(with_tt.value, without_tt.value);
  ASSERT_TRUE(with_tt.has_principal_variation());
  ASSERT_TRUE(without_tt.has_principal_variation());
  EXPECT_EQ(with_tt.first_move().cell, without_tt.first_move().cell);
  EXPECT_LT(with_tt.stats.visited_states, without_tt.stats.visited_states);
  EXPECT_GT(with_tt.stats.transpositions.hits, 0u);
  EXPECT_GT(with_tt.stats.transpositions.cutoffs, 0u);
  EXPECT_GT(table.stats().stores, 0u);
}

TEST(AdversarialSearchFramework, NegamaxIterativeDeepeningCollectsPerDepthPV)
{
  SearchLimits limits;
  limits.max_depth = 2;

  auto fixed = negamax_search(ArtificialGameDomain{}, ArtificialGameState{}, {}, limits);
  auto iterative = iterative_deepening_negamax_search(ArtificialGameDomain{},
                                                      ArtificialGameState{},
                                                      {},
                                                      limits);

  ASSERT_TRUE(iterative.has_iterations());
  ASSERT_EQ(iterative.iterations.size(), 2u);
  EXPECT_EQ(iterative.completed_iterations, 2u);

  EXPECT_EQ(iterative.iterations[0].depth, 1u);
  EXPECT_EQ(iterative.iterations[0].result.value, 6);
  EXPECT_EQ(path_signature(iterative.iterations[0].result.principal_variation), "A");

  EXPECT_EQ(iterative.iterations[1].depth, 2u);
  EXPECT_EQ(iterative.iterations[1].result.value, fixed.value);
  EXPECT_EQ(path_signature(iterative.iterations[1].result.principal_variation), "Aa");

  EXPECT_EQ(iterative.result.value, fixed.value);
  EXPECT_EQ(path_signature(iterative.result.principal_variation), "Aa");
  EXPECT_GT(iterative.total_stats.visited_states, fixed.stats.visited_states);
}

TEST(AdversarialSearchFramework, AlphaBetaIterativeDeepeningAspirationPreservesValue)
{
  SearchLimits limits;
  limits.max_depth = 2;

  AdversarialIterativeDeepeningOptions<int> options;
  options.initial_depth = 1;
  options.depth_step = 1;
  options.aspiration.half_window = 1;
  options.aspiration.growth = 1;

  auto fixed = alpha_beta_search(ArtificialGameDomain{}, ArtificialGameState{}, {}, limits);
  auto iterative = iterative_deepening_alpha_beta_search(ArtificialGameDomain{},
                                                         ArtificialGameState{},
                                                         {},
                                                         limits,
                                                         options);

  ASSERT_EQ(iterative.iterations.size(), 2u);
  EXPECT_EQ(iterative.result.value, fixed.value);
  EXPECT_EQ(path_signature(iterative.result.principal_variation), "Aa");
  EXPECT_TRUE(iterative.iterations[1].used_aspiration_window);
  EXPECT_GT(iterative.iterations[1].aspiration_researches, 0u);
  EXPECT_GT(iterative.aspiration_researches, 0u);
}

TEST(AdversarialSearchFramework, AlphaBetaTracingRecordsIterationsAndCutoffs)
{
  SearchLimits limits;
  limits.max_depth = 2;

  AdversarialIterativeDeepeningOptions<int> options;
  options.initial_depth = 1;
  options.depth_step = 1;
  options.aspiration.half_window = 1;
  options.aspiration.growth = 1;

  AdversarialTraceCollector<ArtificialGameMove, int> trace;
  auto iterative = iterative_deepening_alpha_beta_search(ArtificialGameDomain{},
                                                         ArtificialGameState{},
                                                         trace,
                                                         {},
                                                         limits,
                                                         options);

  EXPECT_EQ(iterative.result.value, 3);
  EXPECT_GT(trace.size(), 0u);
  EXPECT_EQ(count_trace_events(trace, AdversarialTraceEventKind::Iteration_Begin), 2u);
  EXPECT_EQ(count_trace_events(trace, AdversarialTraceEventKind::Iteration_End), 2u);
  EXPECT_GT(count_trace_events(trace, AdversarialTraceEventKind::Alpha_Beta_Cutoff), 0u);
  EXPECT_GT(count_trace_events(trace, AdversarialTraceEventKind::Aspiration_Retry), 0u);
}

TEST(AdversarialSearchFramework, AlphaBetaIterativeDeepeningTTPreservesValueAndReducesNodes)
{
  TicTacToeState state;

  SearchLimits limits;
  limits.max_depth = 9;

  ExplorationPolicy ordered_policy = Alpha_Beta<TicTacToeDomain>::default_policy();
  ordered_policy.move_ordering = MoveOrderingMode::Estimated_Score;
  ordered_policy.use_killer_moves = true;
  ordered_policy.use_history_heuristic = true;

  AdversarialIterativeDeepeningOptions<int> options;
  options.initial_depth = 1;
  options.depth_step = 1;
  options.aspiration.half_window = 4;
  options.aspiration.growth = 4;

  auto without_tt = iterative_deepening_alpha_beta_search(TicTacToeDomain{},
                                                          state,
                                                          ordered_policy,
                                                          limits,
                                                          options);

  using TT = AdversarialTranspositionTable<TicTacToeDomain::State_Key,
                                           TicTacToeDomain::Move,
                                           TicTacToeDomain::Score>;
  TT table;
  auto with_tt = iterative_deepening_alpha_beta_search(TicTacToeDomain{},
                                                       state,
                                                       table,
                                                       ordered_policy,
                                                       limits,
                                                       options);

  EXPECT_EQ(with_tt.result.value, without_tt.result.value);
  ASSERT_TRUE(with_tt.result.has_principal_variation());
  ASSERT_TRUE(without_tt.result.has_principal_variation());
  EXPECT_EQ(with_tt.result.first_move().cell, without_tt.result.first_move().cell);
  EXPECT_LT(with_tt.total_stats.visited_states, without_tt.total_stats.visited_states);
  EXPECT_GT(table.stats().stores, 0u);
}
