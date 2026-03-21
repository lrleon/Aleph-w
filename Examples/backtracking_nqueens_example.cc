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

/** @file backtracking_nqueens_example.cc
 *  @brief Reference example: N-Queens with DFS/backtracking.
 *
 *  Model summary:
 *
 *  - State: partial queen placement, one row at a time.
 *  - Successor generator: every safe column in the next row.
 *  - Terminal criterion: all rows are filled.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target backtracking_nqueens_example`
 *  - `./build/Examples/backtracking_nqueens_example`
 */

#include <iostream>
#include <string>

#include <State_Search.H>
#include <tpl_array.H>

using Aleph::Array;

namespace
{

namespace Search = Aleph::Search;

struct NQueensState
{
  size_t n = 0;
  size_t row = 0;
  Array<int> queens;
  Array<unsigned char> used_columns;
  Array<unsigned char> used_diag_down;
  Array<unsigned char> used_diag_up;

  explicit NQueensState(const size_t size = 0)
    : n(size),
      row(0),
      queens(size, -1),
      used_columns(size, 0),
      used_diag_down(size == 0 ? 0 : 2*size - 1, 0),
      used_diag_up(size == 0 ? 0 : 2*size - 1, 0)
  {
    // empty
  }
};

struct NQueensDomain
{
  struct Move
  {
    size_t col = 0;
  };

  using State = NQueensState;

  size_t n = 0;

  // A complete placement is found when all rows already contain a queen.
  bool is_goal(const State &state) const
  {
    return state.row == n;
  }

  // Apply one decision: place one queen in the current row.
  void apply(State &state, const Move &move) const
  {
    const size_t row = state.row;
    const size_t down = row + state.n - 1 - move.col;
    const size_t up = row + move.col;

    state.queens[row] = static_cast<int>(move.col);
    state.used_columns[move.col] = 1;
    state.used_diag_down[down] = 1;
    state.used_diag_up[up] = 1;
    ++state.row;
  }

  // Undo exactly the last placement.
  void undo(State &state, const Move &move) const
  {
    --state.row;
    const size_t row = state.row;
    const size_t down = row + state.n - 1 - move.col;
    const size_t up = row + move.col;

    state.queens[row] = -1;
    state.used_columns[move.col] = 0;
    state.used_diag_down[down] = 0;
    state.used_diag_up[up] = 0;
  }

  // Generate safe columns for the next row without materializing child states.
  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.row >= n)
      return true;

    for (size_t col = 0; col < n; ++col)
      {
        const size_t down = state.row + state.n - 1 - col;
        const size_t up = state.row + col;
        if (state.used_columns[col] or state.used_diag_down[down] or state.used_diag_up[up])
          continue;

        if (not visit(Move{col}))
          return false;
      }

    return true;
  }
};

std::string signature(const NQueensState &state)
{
  std::string s;
  for (size_t row = 0; row < state.n; ++row)
    s.push_back(static_cast<char>('0' + state.queens[row]));
  return s;
}

void print_board(const NQueensState &state)
{
  for (size_t row = 0; row < state.n; ++row)
    {
      for (size_t col = 0; col < state.n; ++col)
        std::cout << (state.queens[row] == static_cast<int>(col) ? 'Q' : '.');
      std::cout << '\n';
    }
}

} // end namespace

int main()
{
  constexpr size_t n = 4;

  Search::ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  using Engine = Search::Backtracking<NQueensDomain>;
  Search::Collector<Engine::Solution> collector;
  auto result = Search::backtracking_search(NQueensDomain{n},
                                            NQueensState(n),
                                            collector,
                                            policy);

  std::cout << "N-Queens reference example\n";
  std::cout << "Board size: " << n << '\n';
  std::cout << "Solutions found: " << result.stats.solutions_found << '\n';
  std::cout << "Visited states: " << result.stats.visited_states << '\n';
  std::cout << "Solution signatures:";
  for (auto it = collector.solutions().get_it(); it.has_curr(); it.next_ne())
    std::cout << ' ' << signature(it.get_curr().state);
  std::cout << '\n';

  if (result.found_solution())
    {
      std::cout << "First solution signature: "
                << signature(result.best_solution.get().state) << '\n';
      std::cout << "First solution board:\n";
      print_board(result.best_solution.get().state);
    }

  return 0;
}
