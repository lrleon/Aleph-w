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

/** @file negamax_tictactoe_example.cc
 *  @brief Reference example: Tic-Tac-Toe with Negamax and Alpha-Beta.
 *
 *  Model summary:
 *
 *  - State: board cells, side to move and ply count.
 *  - Successor generator: every legal move in a fixed exploration order.
 *  - Terminal criterion: win for either side or full board.
 *  - Evaluator: score from the side-to-move perspective.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target negamax_tictactoe_example`
 *  - `./build/Examples/negamax_tictactoe_example`
 */

#include <cstdint>
#include <iostream>

#include <State_Search.H>
#include <tpl_array.H>

using Aleph::Array;

namespace
{

namespace Search = Aleph::Search;

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

  // The game stops on a win or when no empty cells remain.
  bool is_terminal(const State &state) const
  {
    return winner(state) != 0 or state.moves_played == 9;
  }

  // Evaluate from the perspective of the player that must move in `state`.
  Score evaluate(const State &state) const
  {
    const int win = winner(state);
    if (win != 0)
      return win == state.player ? 100 : -100;

    if (state.moves_played == 9)
      return 0;

    return heuristic(state, state.player) - heuristic(state, -state.player);
  }

  // Compact ternary encoding used by the optional transposition table.
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

  // Apply one legal move and flip the side to move.
  void apply(State &state, const Move &move) const
  {
    state.board[move.cell] = state.player;
    state.player = -state.player;
    ++state.moves_played;
  }

  // Restore the previous position exactly.
  void undo(State &state, const Move &move) const
  {
    --state.moves_played;
    state.player = -state.player;
    state.board[move.cell] = 0;
  }

  // Generate legal moves lazily in a fixed domain order.
  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    static constexpr size_t order[] = {4, 0, 2, 6, 8, 1, 3, 5, 7};

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

void print_board(const TicTacToeState &state)
{
  for (size_t row = 0; row < 3; ++row)
    {
      for (size_t col = 0; col < 3; ++col)
        {
          const int cell = state.board[row*3 + col];
          std::cout << (cell == 1 ? 'X' : cell == -1 ? 'O' : '.');
        }
      std::cout << '\n';
    }
}

} // end namespace

int main()
{
  TicTacToeState root;
  using TT = Search::GameTT<TicTacToeDomain::State_Key,
                            TicTacToeDomain::Move,
                            TicTacToeDomain::Score>;
  using Trace = Search::TraceCollector<TicTacToeDomain::Move,
                                       TicTacToeDomain::Score>;

  auto negamax = Search::negamax_search(TicTacToeDomain{}, root);
  TT negamax_tt_cache;
  auto negamax_tt = Search::negamax_search(TicTacToeDomain{}, root, negamax_tt_cache);

  auto alpha_beta = Search::alpha_beta_search(TicTacToeDomain{}, root);
  TT alpha_beta_tt_cache;
  auto alpha_beta_tt = Search::alpha_beta_search(TicTacToeDomain{}, root, alpha_beta_tt_cache);

  Search::ExplorationPolicy ordered_policy = Search::AlphaBeta<TicTacToeDomain>::default_policy();
  ordered_policy.move_ordering = Search::MoveOrderingMode::Estimated_Score;
  ordered_policy.use_killer_moves = true;

  Search::SearchLimits iterative_limits;
  iterative_limits.max_depth = 9;

  Search::IterativeDeepeningOptions<int> iterative_options;
  iterative_options.initial_depth = 1;
  iterative_options.depth_step = 1;
  iterative_options.aspiration.half_window = 4;
  iterative_options.aspiration.growth = 4;

  TT iterative_tt_cache;
  Trace trace;
  auto iterative = Search::iterative_deepening_alpha_beta_search(TicTacToeDomain{},
                                                                 root,
                                                                 iterative_tt_cache,
                                                                 trace,
                                                                 ordered_policy,
                                                                 iterative_limits,
                                                                 iterative_options);

  std::cout << "Tic-Tac-Toe reference example\n";
  std::cout << "Root position:\n";
  print_board(root);
  std::cout << '\n';

  std::cout << "Negamax root value: " << negamax.value << '\n';
  std::cout << "Negamax best move: " << negamax.first_move().cell << '\n';
  std::cout << "Negamax visited states: " << negamax.stats.visited_states << '\n';
  std::cout << "Negamax + TT visited states: "
            << negamax_tt.stats.visited_states << '\n';
  std::cout << "Negamax + TT hits: "
            << negamax_tt.stats.transpositions.hits << '\n';
  std::cout << "Negamax + TT cutoffs: "
            << negamax_tt.stats.transpositions.cutoffs << '\n';

  std::cout << '\n';
  std::cout << "Alpha-Beta root value: " << alpha_beta.value << '\n';
  std::cout << "Alpha-Beta best move: " << alpha_beta.first_move().cell << '\n';
  std::cout << "Alpha-Beta visited states: " << alpha_beta.stats.visited_states << '\n';
  std::cout << "Alpha-Beta cutoffs: " << alpha_beta.stats.alpha_beta_cutoffs << '\n';
  std::cout << "Alpha-Beta + TT visited states: "
            << alpha_beta_tt.stats.visited_states << '\n';
  std::cout << "Alpha-Beta + TT hits: "
            << alpha_beta_tt.stats.transpositions.hits << '\n';
  std::cout << "Alpha-Beta + TT cutoffs: "
            << alpha_beta_tt.stats.transpositions.cutoffs << '\n';

  std::cout << '\n';
  std::cout << "Negamax + TT table stores: "
            << negamax_tt_cache.stats().stores << '\n';
  std::cout << "Alpha-Beta + TT table stores: "
            << alpha_beta_tt_cache.stats().stores << '\n';
  std::cout << "Iterative Alpha-Beta best move: "
            << iterative.result.first_move().cell << '\n';
  std::cout << "Iterative Alpha-Beta iterations: "
            << iterative.completed_iterations << '\n';
  std::cout << "Iterative Alpha-Beta aspiration retries: "
            << iterative.aspiration_researches << '\n';
  std::cout << "Iterative Alpha-Beta visited states: "
            << iterative.total_stats.visited_states << '\n';
  std::cout << "Iterative Alpha-Beta TT stores: "
            << iterative_tt_cache.stats().stores << '\n';
  std::cout << "Trace events captured: " << trace.size() << '\n';

  return 0;
}
