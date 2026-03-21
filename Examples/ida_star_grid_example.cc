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

/** @file ida_star_grid_example.cc
 *  @brief Reference example: IDA* pathfinding on a 2-D grid.
 *
 *  Model summary:
 *
 *  - State: current (row, col) position on a grid with obstacles.
 *  - Moves: up / down / left / right with unit cost.
 *  - Heuristic: Manhattan distance to the goal — admissible and consistent.
 *  - Goal: reach the target cell.
 *
 *  IDA* finds the shortest path (minimum number of steps) from a source cell
 *  to a goal cell, avoiding obstacle cells. Because the heuristic is
 *  admissible, the first complete solution is guaranteed optimal.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target ida_star_grid_example`
 *  - `./build/Examples/ida_star_grid_example`
 */

# include <array>
# include <iostream>

# include <State_Search.H>

using Aleph::Array;

namespace
{

namespace Search = Aleph::Search;

// ---------------------------------------------------------------------------
// Grid layout
// ---------------------------------------------------------------------------

// 0 = free, 1 = obstacle
// clang-format off
static constexpr int GRID_ROWS = 6;
static constexpr int GRID_COLS = 8;
static constexpr std::array<std::array<int, GRID_COLS>, GRID_ROWS> GRID = {{
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 1, 1, 0, 1, 0 },
  { 0, 1, 0, 0, 0, 0, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
}};
// clang-format on

static constexpr int START_ROW = 0;
static constexpr int START_COL = 0;
static constexpr int GOAL_ROW  = 5;
static constexpr int GOAL_COL  = 7;

// ---------------------------------------------------------------------------
// Domain
// ---------------------------------------------------------------------------

struct GridState
{
  int row = 0;
  int col = 0;

  bool operator==(const GridState &) const = default;
};

struct GridDomain
{
  struct Move
  {
    int dr = 0; ///< Row delta (-1, 0, +1).
    int dc = 0; ///< Column delta (-1, 0, +1).
  };

  using State    = GridState;
  using Distance = int;

  const int goal_row;
  const int goal_col;

  explicit GridDomain(const int gr = GOAL_ROW, const int gc = GOAL_COL)
    : goal_row(gr), goal_col(gc)
  {
    // empty
  }

  bool is_goal(const State &s) const noexcept
  {
    return s.row == goal_row and s.col == goal_col;
  }

  Distance heuristic(const State &s) const noexcept
  {
    return std::abs(s.row - goal_row) + std::abs(s.col - goal_col);
  }

  Distance cost(const State &, const Move &) const noexcept
  {
    return 1;
  }

  void apply(State &s, const Move &m) const noexcept
  {
    s.row += m.dr;
    s.col += m.dc;
  }

  void undo(State &s, const Move &m) const noexcept
  {
    s.row -= m.dr;
    s.col -= m.dc;
  }

  template <typename Visitor>
  bool for_each_successor(const State &s, Visitor visit) const
  {
    static constexpr std::array<Move, 4> dirs = {{
      {-1,  0},  // up
      { 1,  0},  // down
      { 0, -1},  // left
      { 0,  1},  // right
    }};

    for (const auto &d : dirs)
      {
        const int nr = s.row + d.dr;
        const int nc = s.col + d.dc;

        if (nr < 0 or nr >= GRID_ROWS or nc < 0 or nc >= GRID_COLS)
          continue;
        if (GRID[static_cast<size_t>(nr)][static_cast<size_t>(nc)] != 0)
          continue;

        if (not visit(d))
          return false;
      }

    return true;
  }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

char cell_char(const int r, const int c)
{
  if (r == START_ROW and c == START_COL)
    return 'S';
  if (r == GOAL_ROW and c == GOAL_COL)
    return 'G';
  return GRID[static_cast<size_t>(r)][static_cast<size_t>(c)] ? '#' : '.';
}

void print_grid()
{
  std::cout << "Grid (" << GRID_ROWS << 'x' << GRID_COLS
            << ", S=start, G=goal, #=obstacle):\n";
  for (int r = 0; r < GRID_ROWS; ++r)
    {
      std::cout << "  ";
      for (int c = 0; c < GRID_COLS; ++c)
        std::cout << cell_char(r, c);
      std::cout << '\n';
    }
}

void print_path(const Aleph::SearchPath<GridDomain::Move> &path,
                const GridState &start)
{
  GridState pos = start;
  std::cout << "  (" << pos.row << ',' << pos.col << ')';
  for (const auto &m : path)
    {
      pos.row += m.dr;
      pos.col += m.dc;
      std::cout << " -> (" << pos.row << ',' << pos.col << ')';
    }
  std::cout << '\n';
}

} // end namespace

int main()
{
  std::cout << "IDA* grid pathfinding example\n\n";
  print_grid();
  std::cout << '\n';

  GridDomain domain(GOAL_ROW, GOAL_COL);
  const GridState root{START_ROW, START_COL};

  auto result = Search::ida_star_search(domain, root);

  std::cout << "Status: ";
  switch (result.status)
    {
    case Aleph::SearchStatus::StoppedOnSolution: std::cout << "solution found\n"; break;
    case Aleph::SearchStatus::Exhausted:         std::cout << "exhausted (no path)\n"; break;
    case Aleph::SearchStatus::LimitReached:      std::cout << "limit reached\n"; break;
    default:                                     std::cout << "not started\n"; break;
    }

  std::cout << "IDA* iterations: " << result.iterations.size() << '\n';
  std::cout << "Total visited states: " << result.stats.visited_states << '\n';
  std::cout << "Total expanded states: " << result.stats.expanded_states << '\n';

  for (size_t i = 0; i < result.iterations.size(); ++i)
    {
      const auto &it = result.iterations[i];
      std::cout << "  Iteration " << i + 1
                << ": threshold=" << it.threshold
                << " visited=" << it.visited_states
                << (it.found_solution ? " FOUND" : "") << '\n';
    }

  if (result.found_solution())
    {
      const auto &sol = result.best_solution.get();
      std::cout << "\nOptimal path cost: " << result.total_cost << " steps\n";
      std::cout << "Path:\n";
      print_path(sol.path, root);
    }
  else
    {
      std::cout << "\nNo path found from ("
                << START_ROW << ',' << START_COL << ") to ("
                << GOAL_ROW << ',' << GOAL_COL << ")\n";
    }

  return 0;
}
