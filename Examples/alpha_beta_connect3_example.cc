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

#include <iostream>

#include <State_Search.H>
#include <tpl_array.H>

using namespace Aleph;

namespace
{

struct Connect3State
{
  Array<int> board;
  Array<size_t> heights;
  int player = 1;
  size_t moves_played = 0;

  Connect3State()
    : board(size_t(16), 0), heights(size_t(4), size_t(0))
  {
    // empty
  }
};

class Connect3Domain
{
public:
  struct Move
  {
    size_t column = 0;

    [[nodiscard]] bool operator == (const Move &) const noexcept = default;
  };

  using State = Connect3State;
  using Score = int;
  using Move_Key = size_t;

  static constexpr size_t Rows = 4;
  static constexpr size_t Cols = 4;
  static constexpr size_t Align = 3;

  bool is_terminal(const State &state) const
  {
    return winner(state) != 0 or state.moves_played == Rows*Cols;
  }

  Score evaluate(const State &state) const
  {
    const int win = winner(state);
    if (win != 0)
      return win == state.player ? 1000 : -1000;

    if (state.moves_played == Rows*Cols)
      return 0;

    return heuristic(state, state.player) - heuristic(state, -state.player);
  }

  [[nodiscard]] Move_Key move_key(const Move &move) const noexcept
  {
    return move.column;
  }

  void apply(State &state, const Move &move) const
  {
    const size_t row = state.heights[move.column];
    state.board[index(row, move.column)] = state.player;
    ++state.heights[move.column];
    state.player = -state.player;
    ++state.moves_played;
  }

  void undo(State &state, const Move &move) const
  {
    --state.moves_played;
    state.player = -state.player;
    --state.heights[move.column];
    const size_t row = state.heights[move.column];
    state.board[index(row, move.column)] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    static constexpr size_t order[] = {0, 3, 1, 2};

    for (const size_t col : order)
      if (state.heights[col] < Rows and not visit(Move{col}))
        return false;

    return true;
  }

private:
  [[nodiscard]] static constexpr size_t index(const size_t row,
                                              const size_t col) noexcept
  {
    return row*Cols + col;
  }

  [[nodiscard]] static int winner(const State &state) noexcept
  {
    for (size_t row = 0; row < Rows; ++row)
      for (size_t col = 0; col < Cols; ++col)
        {
          const int cell = state.board[index(row, col)];
          if (cell == 0)
            continue;

          if (col + Align <= Cols)
            {
              bool ok = true;
              for (size_t k = 1; k < Align; ++k)
                ok = ok and state.board[index(row, col + k)] == cell;
              if (ok)
                return cell;
            }

          if (row + Align <= Rows)
            {
              bool ok = true;
              for (size_t k = 1; k < Align; ++k)
                ok = ok and state.board[index(row + k, col)] == cell;
              if (ok)
                return cell;
            }

          if (row + Align <= Rows and col + Align <= Cols)
            {
              bool ok = true;
              for (size_t k = 1; k < Align; ++k)
                ok = ok and state.board[index(row + k, col + k)] == cell;
              if (ok)
                return cell;
            }

          if (row + Align <= Rows and col + 1 >= Align)
            {
              bool ok = true;
              for (size_t k = 1; k < Align; ++k)
                ok = ok and state.board[index(row + k, col - k)] == cell;
              if (ok)
                return cell;
            }
        }

    return 0;
  }

  [[nodiscard]] static Score heuristic(const State &state, const int player) noexcept
  {
    Score total = 0;

    auto score_window = [&](const size_t r0,
                            const size_t c0,
                            const int dr,
                            const int dc) noexcept
    {
      size_t mine = 0;
      size_t empty = 0;

      for (size_t k = 0; k < Align; ++k)
        {
          const size_t row = static_cast<size_t>(static_cast<int>(r0) +
                                                 dr*static_cast<int>(k));
          const size_t col = static_cast<size_t>(static_cast<int>(c0) +
                                                 dc*static_cast<int>(k));
          const int cell = state.board[index(row, col)];
          if (cell == player)
            ++mine;
          else if (cell == 0)
            ++empty;
          else
            return;
        }

      if (mine == 2 and empty == 1)
        total += 20;
      else if (mine == 1 and empty == 2)
        total += 2;
    };

    for (size_t row = 0; row < Rows; ++row)
      for (size_t col = 0; col + Align <= Cols; ++col)
        score_window(row, col, 0, 1);

    for (size_t row = 0; row + Align <= Rows; ++row)
      for (size_t col = 0; col < Cols; ++col)
        score_window(row, col, 1, 0);

    for (size_t row = 0; row + Align <= Rows; ++row)
      for (size_t col = 0; col + Align <= Cols; ++col)
        score_window(row, col, 1, 1);

    for (size_t row = 0; row + Align <= Rows; ++row)
      for (size_t col = Align - 1; col < Cols; ++col)
        score_window(row, col, 1, -1);

    return total;
  }
};

void print_board(const Connect3State &state)
{
  for (size_t row = Connect3Domain::Rows; row > 0; --row)
    {
      const size_t current = row - 1;
      for (size_t col = 0; col < Connect3Domain::Cols; ++col)
        {
          const int cell = state.board[current*Connect3Domain::Cols + col];
          std::cout << (cell == 1 ? 'X' : cell == -1 ? 'O' : '.');
        }
      std::cout << '\n';
    }
}

void drop(Connect3Domain &domain, Connect3State &state, const size_t column)
{
  domain.apply(state, Connect3Domain::Move{column});
}

} // end namespace

int main()
{
  Connect3Domain domain;
  Connect3State state;

  drop(domain, state, 1);
  drop(domain, state, 2);
  drop(domain, state, 1);
  drop(domain, state, 2);
  drop(domain, state, 0);

  SearchLimits limits;
  limits.max_depth = 6;

  ExplorationPolicy plain_policy = Alpha_Beta<Connect3Domain>::default_policy();
  Alpha_Beta<Connect3Domain> plain_engine(domain, plain_policy, limits);
  auto plain = plain_engine.search(state);

  ExplorationPolicy ordered_policy = Alpha_Beta<Connect3Domain>::default_policy();
  ordered_policy.move_ordering = MoveOrderingMode::Estimated_Score;
  ordered_policy.use_killer_moves = true;
  ordered_policy.use_history_heuristic = true;
  Alpha_Beta<Connect3Domain> ordered_engine(domain, ordered_policy, limits);
  auto ordered = ordered_engine.search(state);

  std::cout << "Reduced Connect-3 position:\n";
  print_board(state);
  std::cout << '\n';
  std::cout << "Alpha-Beta value: " << plain.value << '\n';
  std::cout << "Best column: " << plain.first_move().column << '\n';
  std::cout << "Visited states (domain order): " << plain.stats.visited_states << '\n';
  std::cout << "Cutoffs (domain order): " << plain.stats.alpha_beta_cutoffs << '\n';
  std::cout << '\n';
  std::cout << "Best column with ordering: " << ordered.first_move().column << '\n';
  std::cout << "Visited states (ordered): " << ordered.stats.visited_states << '\n';
  std::cout << "Cutoffs (ordered): " << ordered.stats.alpha_beta_cutoffs << '\n';
  std::cout << "Ordered batches: " << ordered.stats.move_ordering.ordered_batches << '\n';
  std::cout << "Priority estimates: " << ordered.stats.move_ordering.priority_estimates << '\n';
  std::cout << "Killer hits: " << ordered.stats.move_ordering.killer_hits << '\n';
  std::cout << "History hits: " << ordered.stats.move_ordering.history_hits << '\n';

  return 0;
}
