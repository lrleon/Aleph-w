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

/** @file branch_and_bound_df_vs_bf_example.cc
 *  @brief Reference example: comparing Depth-First and Best-First strategies
 *         in Branch-and-Bound on a 0/1 knapsack problem.
 *
 *  The 0/1 knapsack problem assigns a binary decision (include / exclude)
 *  to each item.  Branch-and-Bound prunes subtrees whose optimistic upper
 *  bound cannot beat the current incumbent.
 *
 *  Depth-First (the default) quickly finds a first feasible solution and
 *  tightens the bound early, but may explore many nodes before proving
 *  optimality.
 *
 *  Best-First expands nodes in decreasing order of their optimistic bound,
 *  so it tends to reach the optimum with fewer expansions — at the cost of
 *  maintaining a priority queue.
 *
 *  This example runs both strategies on the same instance, prints the
 *  results side-by-side, and verifies that they agree on the optimum value.
 *
 *  Build and run:
 *  - `cmake --build build --target branch_and_bound_df_vs_bf_example`
 *  - `./build/Examples/branch_and_bound_df_vs_bf_example`
 */

#include <cstddef>
#include <iostream>
#include <string>

#include <Branch_And_Bound.H>
#include <tpl_array.H>

using namespace Aleph;

// ---------------------------------------------------------------------------
// 0/1 knapsack domain
// ---------------------------------------------------------------------------

struct Item
{
  std::string name;
  double      weight = 0.0;
  double      value  = 0.0;
};

struct KnapsackState
{
  size_t next_item  = 0;   ///< Index of the next item to decide.
  double weight_so_far = 0.0;
  double value_so_far  = 0.0;
};

class KnapsackDomain
{
public:
  struct Move
  {
    bool include = false;
  };

  using State     = KnapsackState;
  using Objective = double;

  explicit KnapsackDomain(const Array<Item> &items, double capacity)
    : items_(items), capacity_(capacity)
  {
    // empty
  }

  /** @brief True when all items have been decided. */
  [[nodiscard]] bool is_complete(const State &s) const noexcept
  {
    return s.next_item == static_cast<size_t>(items_.size());
  }

  /** @brief Objective value for a complete assignment. */
  [[nodiscard]] Objective objective_value(const State &s) const noexcept
  {
    return s.value_so_far;
  }

  /** @brief Greedy fractional upper bound on the remaining items.
   *
   *  Takes remaining items in value-density order and fills the remaining
   *  capacity fractionally.  This is an admissible upper bound for B&B.
   */
  [[nodiscard]] Objective bound(const State &s) const noexcept
  {
    double remaining = capacity_ - s.weight_so_far;
    double ub        = s.value_so_far;
    size_t i         = s.next_item;
    while (i < static_cast<size_t>(items_.size()) and remaining > 0.0)
      {
        const Item &item = items_[i];
        if (item.weight <= remaining)
          {
            ub        += item.value;
            remaining -= item.weight;
          }
        else
          {
            ub += item.value * (remaining / item.weight);
            remaining = 0.0;
          }
        ++i;
      }

    return ub;
  }

  void apply(State &s, const Move &m) const noexcept
  {
    if (m.include)
      {
        const Item &item = items_[s.next_item];
        s.weight_so_far += item.weight;
        s.value_so_far  += item.value;
      }
    ++s.next_item;
  }

  void undo(State &s, const Move &m) const noexcept
  {
    --s.next_item;
    if (m.include)
      {
        const Item &item = items_[s.next_item];
        s.weight_so_far -= item.weight;
        s.value_so_far  -= item.value;
      }
  }

  [[nodiscard]] bool is_feasible(const State &s) const noexcept
  {
    return s.weight_so_far <= capacity_;
  }

  template <typename Visitor>
  bool for_each_successor(const State &s, Visitor visit) const
  {
    if (s.next_item >= static_cast<size_t>(items_.size()))
      return true;

    // Try including the item first (often finds better solutions sooner).
    const Item &item = items_[s.next_item];
    if (s.weight_so_far + item.weight <= capacity_)
      {
        if (not visit(Move{true}))
          return false;
      }

    return visit(Move{false});
  }

private:
  Array<Item> items_;
  double        capacity_;
};

// ---------------------------------------------------------------------------
// Helper to run and print one strategy
// ---------------------------------------------------------------------------

static void run_strategy(const Array<Item> &items,
                          double capacity,
                          ExplorationPolicy::Strategy strategy,
                          const std::string &label)
{
  KnapsackDomain domain(items, capacity);
  ExplorationPolicy policy;
  policy.strategy = strategy;

  Branch_And_Bound<KnapsackDomain> engine{domain, policy};
  const auto result = engine.search(KnapsackState{});

  std::cout << label << ":\n";
  if (result.found_solution())
    {
      std::cout << "  Optimum value  : " << result.incumbent.best_value() << "\n";
      std::cout << "  Expanded states: " << result.stats.expanded_states << "\n";
      std::cout << "  Pruned by bound: " << result.stats.pruned_by_bound << "\n";
    }
  else
    {
      std::cout << "  No solution found.\n";
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
  // Items sorted by value/weight ratio (required for the fractional bound).
  const Array<Item> items = {
    {"watch",   1.0, 6.0},
    {"book",    2.0, 10.0},
    {"camera",  3.0, 12.0},
    {"laptop",  5.0, 16.0},
    {"tablet",  4.0, 13.0},
  };

  const double capacity = 7.0;

  run_strategy(items, capacity, ExplorationPolicy::Strategy::Depth_First, "Depth-First B&B");
  std::cout << "\n";
  run_strategy(items, capacity, ExplorationPolicy::Strategy::Best_First,  "Best-First B&B");

  return 0;
}