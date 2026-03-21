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

/** @file branch_and_bound_artificial_example.cc
 *  @brief Reference example: artificial maximization and minimization trees.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target branch_and_bound_artificial_example`
 *  - `./build/Examples/branch_and_bound_artificial_example`
 */

# include <iostream>
# include <string>

# include <State_Search.H>

using namespace Aleph;

namespace
{

struct ArtificialState
{
  size_t depth = 0;
  int code = 1;
  int value = 0;
};

struct ArtificialMove
{
  int target_code = 1;
  int delta = 0;
  char label = 'L';
};

struct ArtificialMaxDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 9;
      case 2: return 9;
      case 3: return 5;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
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
        if (not visit(Move{2, 0, 'L'}))
          return false;
        return visit(Move{3, 0, 'R'});

      case 2:
        if (not visit(Move{4, 7, 'L'}))
          return false;
        return visit(Move{5, 9, 'R'});

      case 3:
        if (not visit(Move{6, 5, 'L'}))
          return false;
        return visit(Move{7, 4, 'R'});

      default:
        return true;
      }
  }
};

struct ArtificialMinDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 6;
      case 2: return 6;
      case 3: return 7;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
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
        if (not visit(Move{2, 0, 'L'}))
          return false;
        return visit(Move{3, 0, 'R'});

      case 2:
        if (not visit(Move{4, 8, 'L'}))
          return false;
        return visit(Move{5, 6, 'R'});

      case 3:
        if (not visit(Move{6, 7, 'L'}))
          return false;
        return visit(Move{7, 9, 'R'});

      default:
        return true;
      }
  }
};

struct ArtificialMaxBadOrderDomain
{
  using State = ArtificialState;
  using Move = ArtificialMove;
  using Objective = int;

  bool is_complete(const State &state) const
  {
    return state.depth == 2;
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

  Objective bound(const State &state) const
  {
    switch (state.code)
      {
      case 1: return 9;
      case 2: return 9;
      case 3: return 5;
      default: return state.value;
      }
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.target_code;
    state.value += move.delta;
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.value -= move.delta;
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
        if (not visit(Move{3, 0, 'R'}))
          return false;
        return visit(Move{2, 0, 'L'});

      case 2:
        if (not visit(Move{4, 7, 'L'}))
          return false;
        return visit(Move{5, 9, 'R'});

      case 3:
        if (not visit(Move{6, 5, 'L'}))
          return false;
        return visit(Move{7, 4, 'R'});

      default:
        return true;
      }
  }
};

std::string signature(const SearchPath<ArtificialMove> &path)
{
  std::string out;
  for (const auto &move : path)
    out.push_back(move.label);
  return out;
}

template <typename Result>
void print_summary(const char *title, const Result &result)
{
  std::cout << title << '\n';
  std::cout << "  objective: " << result.incumbent.best_value() << '\n';
  std::cout << "  path: " << signature(result.incumbent.get().path) << '\n';
  std::cout << "  visited: " << result.stats.visited_states << '\n';
  std::cout << "  pruned by bound: " << result.stats.pruned_by_bound << '\n';
  std::cout << "  ordered batches: " << result.stats.move_ordering.ordered_batches << '\n';
}

} // end namespace

int main()
{
  auto max_plain = branch_and_bound_search(ArtificialMaxBadOrderDomain{}, ArtificialState{});
  print_summary("Maximization (depth-first, domain order)", max_plain);

  ExplorationPolicy ordered_policy =
    Branch_And_Bound<ArtificialMaxBadOrderDomain>::default_policy();
  ordered_policy.move_ordering = MoveOrderingMode::Estimated_Bound;
  auto max_ordered = branch_and_bound_search(ArtificialMaxBadOrderDomain{},
                                             ArtificialState{},
                                             ordered_policy);
  print_summary("Maximization (depth-first, bound-ordered)", max_ordered);

  ExplorationPolicy max_best_policy =
    Branch_And_Bound<ArtificialMaxBadOrderDomain>::default_policy();
  max_best_policy.strategy = ExplorationPolicy::Strategy::Best_First;
  auto max_best = branch_and_bound_search(ArtificialMaxBadOrderDomain{},
                                          ArtificialState{},
                                          max_best_policy);
  print_summary("Maximization (best-first)", max_best);

  ExplorationPolicy min_policy =
    Branch_And_Bound<ArtificialMinDomain, Minimize_Objective<int>>::default_policy();
  min_policy.strategy = ExplorationPolicy::Strategy::Best_First;

  Branch_And_Bound<ArtificialMinDomain, Minimize_Objective<int>> min_engine(
    ArtificialMinDomain{}, min_policy, {}, Minimize_Objective<int>{});
  auto min_result = min_engine.search(ArtificialState{});
  print_summary("Minimization (best-first)", min_result);

  return 0;
}
