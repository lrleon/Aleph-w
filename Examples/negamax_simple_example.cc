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

/** @file negamax_simple_example.cc
 *  @brief Minimal Negamax example: solve Tic-Tac-Toe from the empty board.
 *
 *  This is the simplest possible adversarial-search example.  It defines a
 *  Tic-Tac-Toe domain, runs plain Negamax without transposition tables,
 *  iterative deepening or move ordering, and prints the game-theoretic value
 *  and the principal variation.
 *
 *  For advanced features (TT, iterative deepening, aspiration windows, move
 *  ordering, tracing) see `negamax_tictactoe_example.cc`.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target negamax_simple_example`
 *  - `./build/Examples/negamax_simple_example`
 */

# include <iostream>

# include <State_Search.H>
# include <tpl_array.H>

using Aleph::Array;

namespace
{

namespace Search = Aleph::Search;

// --- State -----------------------------------------------------------------

struct TicTacToeState
{
  Array<int> board;   // 0 = empty, +1 = X, -1 = O
  int player = 1;     // side to move (+1 or -1)
  size_t moves_played = 0;

  TicTacToeState() : board(size_t(9), 0) {}
};

// --- Domain ----------------------------------------------------------------

class TicTacToeDomain
{
public:
  struct Move
  {
    size_t cell = 0;
    bool operator==(const Move &) const noexcept = default;
  };

  using State = TicTacToeState;
  using Score = int;

  bool is_terminal(const State &state) const
  {
    return winner(state) != 0 or state.moves_played == 9;
  }

  /// Evaluate from the perspective of the player to move.
  Score evaluate(const State &state) const
  {
    const int w = winner(state);
    if (w != 0)
      return w == state.player ? 100 : -100;
    return 0;  // draw
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
    for (size_t i = 0; i < 9; ++i)
      if (state.board[i] == 0 and not visit(Move{i}))
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
};

// --- Display ---------------------------------------------------------------

void print_board(const TicTacToeState &state)
{
  for (size_t row = 0; row < 3; ++row)
    {
      for (size_t col = 0; col < 3; ++col)
        {
          const int cell = state.board[row * 3 + col];
          std::cout << (cell == 1 ? 'X' : cell == -1 ? 'O' : '.');
        }
      std::cout << '\n';
    }
}

} // end anonymous namespace

int main()
{
  TicTacToeDomain domain;
  TicTacToeState root;

  // Run plain Negamax to full depth (no limits needed for a 9-ply game).
  auto result = Search::negamax_search(domain, root);

  std::cout << "Tic-Tac-Toe — simple Negamax example\n\n";

  print_board(root);
  std::cout << '\n';

  std::cout << "Game-theoretic value: " << result.value
            << " (0 = draw)\n";
  if (result.has_principal_variation())
    std::cout << "Best opening move:   cell " << result.first_move().cell << '\n';
  else
    std::cout << "Best opening move:   no principal variation\n";
  std::cout << "Visited states:      " << result.stats.visited_states
            << '\n';
  std::cout << "Heuristic evals:     " << result.stats.heuristic_evaluations
            << '\n';

  // Replay the principal variation.
  std::cout << "\nPrincipal variation (" << result.principal_variation.size()
            << " moves):\n";
  TicTacToeState replay = root;
  for (size_t i = 0; i < result.principal_variation.size(); ++i)
    {
      const auto &move = result.principal_variation[i];
      domain.apply(replay, move);
      std::cout << "  Move " << i + 1 << ": cell " << move.cell
                << " by " << (replay.player == -1 ? 'X' : 'O') << '\n';
    }

  std::cout << "\nFinal board:\n";
  print_board(replay);

  return 0;
}