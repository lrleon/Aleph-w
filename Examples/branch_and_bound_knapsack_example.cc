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

/** @file branch_and_bound_knapsack_example.cc
 *  @brief Reference example: 0/1 Knapsack with branch and bound.
 *
 *  Model summary:
 *
 *  - State: partial packing decision, cumulative weight and value.
 *  - Successor generator: take or skip the next item.
 *  - Complete solution: all items have been decided.
 *  - Bound: fractional-knapsack optimistic upper bound.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target branch_and_bound_knapsack_example`
 *  - `./build/Examples/branch_and_bound_knapsack_example`
 */

#include <iostream>

#include <Knapsack.H>
#include <State_Search.H>
#include <tpl_array.H>

using Aleph::Array;
using Aleph::Knapsack_Item;

namespace
{

namespace Search = Aleph::Search;

struct KnapsackState
{
  size_t index = 0;
  int weight = 0;
  double value = 0;
  Array<unsigned char> chosen;

  explicit KnapsackState(const size_t n = 0)
    : index(0), weight(0), value(0), chosen(n, 0)
  {
    // empty
  }
};

class KnapsackBBDomain
{
public:
  struct Move
  {
    bool take = false;
  };

  using State = KnapsackState;
  using Objective = double;

  KnapsackBBDomain(const Array<Knapsack_Item<int, double>> &items,
                   const int capacity)
    : items_(items), capacity_(capacity)
  {
    // empty
  }

  // A solution is complete once every item is fixed to take/skip.
  bool is_complete(const State &state) const
  {
    return state.index == items_.size();
  }

  // Exact value of a complete solution.
  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  // Fractional upper bound used to prune branches that cannot beat the incumbent.
  Objective bound(const State &state) const
  {
    Objective optimistic = state.value;
    int remaining = capacity_ - state.weight;

    for (size_t i = state.index; i < items_.size() and remaining > 0; ++i)
      if (items_[i].weight <= remaining)
        {
          optimistic += items_[i].value;
          remaining -= items_[i].weight;
        }
      else
        {
          optimistic += items_[i].value*(static_cast<Objective>(remaining)/
                                         static_cast<Objective>(items_[i].weight));
          break;
        }

    return optimistic;
  }

  // Apply one branch decision.
  void apply(State &state, const Move &move) const
  {
    if (move.take)
      {
        state.weight += items_[state.index].weight;
        state.value += items_[state.index].value;
        state.chosen[state.index] = 1;
      }
    else
      state.chosen[state.index] = 0;

    ++state.index;
  }

  // Undo exactly the last branch decision.
  void undo(State &state, const Move &move) const
  {
    --state.index;
    if (move.take)
      {
        state.weight -= items_[state.index].weight;
        state.value -= items_[state.index].value;
      }

    state.chosen[state.index] = 0;
  }

  // Branch lazily: try "take" first when it is feasible, then "skip".
  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.index >= items_.size())
      return true;

    if (state.weight + items_[state.index].weight <= capacity_)
      if (not visit(Move{true}))
        return false;

    return visit(Move{false});
  }

private:
  Array<Knapsack_Item<int, double>> items_;
  int capacity_ = 0;
};

void print_solution(const KnapsackState &state)
{
  std::cout << "Chosen items:";
  for (size_t i = 0; i < state.chosen.size(); ++i)
    if (state.chosen[i])
      std::cout << ' ' << i;
  std::cout << '\n';
}

} // end namespace

int main()
{
  const Array<Knapsack_Item<int, double>> items = {
    {2, 40.0}, {5, 30.0}, {10, 50.0}, {5, 10.0}
  };
  constexpr int capacity = 16;

  using Engine = Search::BranchAndBound<KnapsackBBDomain>;
  Search::ExplorationPolicy policy = Engine::default_policy();
  policy.strategy = Search::ExplorationPolicy::Strategy::Best_First;

  auto result = Search::branch_and_bound_search(KnapsackBBDomain(items, capacity),
                                                KnapsackState(items.size()),
                                                policy);

  std::cout << "0/1 Knapsack reference example\n";
  std::cout << "Capacity: " << capacity << '\n';
  std::cout << "Strategy: best-first\n";
  std::cout << "Optimal value: " << result.incumbent.best_value() << '\n';
  std::cout << "Reference dynamic-programming value: "
            << Aleph::knapsack_01_value<double, int>(items, capacity) << '\n';
  std::cout << "Visited states: " << result.stats.visited_states << '\n';
  std::cout << "Pruned by bound: " << result.stats.pruned_by_bound << '\n';
  print_solution(result.incumbent.get().state);

  return 0;
}
