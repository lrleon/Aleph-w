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

struct AssignmentState
{
  size_t row = 0;
  int total_cost = 0;
  Array<unsigned char> used_columns;
  Array<int> assigned_column;

  explicit AssignmentState(const size_t n = 0)
    : row(0), total_cost(0), used_columns(n, 0), assigned_column(n, -1)
  {
    // empty
  }
};

class AssignmentBBDomain
{
public:
  struct Move
  {
    size_t col = 0;
  };

  using State = AssignmentState;
  using Objective = int;

  explicit AssignmentBBDomain(const Array<Array<int>> &costs)
    : costs_(costs)
  {
    // empty
  }

  bool is_complete(const State &state) const
  {
    return state.row == costs_.size();
  }

  Objective objective_value(const State &state) const
  {
    return state.total_cost;
  }

  Objective bound(const State &state) const
  {
    Objective optimistic = state.total_cost;

    for (size_t row = state.row; row < costs_.size(); ++row)
      {
        int best = -1;
        for (size_t col = 0; col < costs_[row].size(); ++col)
          if (not state.used_columns[col] and (best < 0 or costs_[row][col] < best))
            best = costs_[row][col];

        optimistic += best;
      }

    return optimistic;
  }

  void apply(State &state, const Move &move) const
  {
    state.total_cost += costs_[state.row][move.col];
    state.used_columns[move.col] = 1;
    state.assigned_column[state.row] = static_cast<int>(move.col);
    ++state.row;
  }

  void undo(State &state, const Move &move) const
  {
    --state.row;
    state.total_cost -= costs_[state.row][move.col];
    state.used_columns[move.col] = 0;
    state.assigned_column[state.row] = -1;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.row >= costs_.size())
      return true;

    for (size_t col = 0; col < costs_[state.row].size(); ++col)
      if (not state.used_columns[col] and not visit(Move{col}))
        return false;

    return true;
  }

private:
  Array<Array<int>> costs_;
};

void print_assignment(const AssignmentState &state)
{
  std::cout << "Assignment:";
  for (size_t row = 0; row < state.assigned_column.size(); ++row)
    std::cout << " row" << row << "->col" << state.assigned_column[row];
  std::cout << '\n';
}

} // end namespace

int main()
{
  const Array<Array<int>> costs = {
    Array<int>{9, 2, 7, 8},
    Array<int>{6, 4, 3, 7},
    Array<int>{5, 8, 1, 8},
    Array<int>{7, 6, 9, 4}
  };

  using Engine = Branch_And_Bound<AssignmentBBDomain, Minimize_Objective<int>>;

  ExplorationPolicy policy = Engine::default_policy();
  policy.strategy = ExplorationPolicy::Strategy::Best_First;

  Engine engine(AssignmentBBDomain(costs), policy, {}, Minimize_Objective<int>{});
  auto result = engine.search(AssignmentState(costs.size()));

  std::cout << "Optimal assignment cost: " << result.incumbent.best_value() << '\n';
  std::cout << "Visited states: " << result.stats.visited_states << '\n';
  std::cout << "Pruned by bound: " << result.stats.pruned_by_bound << '\n';
  print_assignment(result.incumbent.get().state);

  return 0;
}
