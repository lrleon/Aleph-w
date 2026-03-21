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

/** @file state_search_benchmark.cc
 *  @brief Reproducible benchmark suite for the state-space search framework.
 *
 *  Included cases:
 *
 *  - DFS/backtracking on N-Queens,
 *  - branch and bound on a deterministic knapsack instance,
 *  - Alpha-Beta on Tic-Tac-Toe with optional move ordering and TT.
 *
 *  The suite is deterministic and supports `--validate` for invariant checks,
 *  plus `--json` for tooling-friendly output.
 */

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <Knapsack.H>
#include <State_Search.H>
#include <tpl_array.H>
#include <tpl_sort_utils.H>

using Aleph::Array;
using Aleph::Knapsack_Item;
namespace Search = Aleph::Search;

namespace
{

volatile long double benchmark_sink = 0.0L;

struct Options
{
  bool validate = false;
  bool json = false;
  size_t repetitions = 32;
  size_t warmup = 1;
  size_t samples = 7;
};

struct TimingStats
{
  double mean_ms = 0.0;
  double median_ms = 0.0;
  double min_ms = 0.0;
  double max_ms = 0.0;
  double stddev_ms = 0.0;
};

struct BenchmarkRow
{
  std::string family;
  std::string scenario;
  std::string variant;
  TimingStats timing;

  size_t visited_states = 0;
  size_t expanded_states = 0;
  size_t generated_successors = 0;
  size_t solutions_found = 0;
  size_t terminal_states = 0;
  size_t pruned_by_depth = 0;
  size_t pruned_by_domain = 0;
  size_t pruned_by_bound = 0;
  size_t incumbent_updates = 0;
  size_t alpha_beta_cutoffs = 0;
  size_t tt_hits = 0;
  size_t tt_cutoffs = 0;
  size_t tt_stores = 0;
  size_t ordered_batches = 0;
  size_t priority_estimates = 0;
  size_t killer_hits = 0;
  size_t history_hits = 0;

  double result_value = 0.0;
  bool has_best_move = false;
  size_t best_move = 0;
};

[[nodiscard]] Options
parse_options(const int argc, char **argv)
{
  Options options;

  for (int i = 1; i < argc; ++i)
    {
      const std::string arg = argv[i];
      if (arg == "--validate")
        options.validate = true;
      else if (arg == "--json")
        options.json = true;
      else if (arg == "--repetitions" and i + 1 < argc)
        options.repetitions = static_cast<size_t>(std::stoull(argv[++i]));
      else if (arg == "--warmup" and i + 1 < argc)
        options.warmup = static_cast<size_t>(std::stoull(argv[++i]));
      else if (arg == "--samples" and i + 1 < argc)
        options.samples = static_cast<size_t>(std::stoull(argv[++i]));
      else if (arg == "--help")
        {
          std::cout
            << "Usage: state_search_benchmark [--validate] [--json]"
            << " [--repetitions N] [--warmup N] [--samples N]\n";
          std::exit(0);
        }
      else
        {
          std::cerr << "Unknown option: " << arg << '\n';
          std::exit(1);
        }
    }

  if (options.repetitions == 0 or options.samples == 0)
    {
      std::cerr << "Repetitions and samples must be positive\n";
      std::exit(1);
    }

  return options;
}

[[nodiscard]] TimingStats
compute_timing_stats(const Array<double> &sample_ms)
{
  TimingStats stats;
  if (sample_ms.is_empty())
    return stats;

  Array<double> sorted = sample_ms;
  Aleph::introsort(sorted);

  double sum = 0.0;
  for (const double value : sample_ms)
    sum += value;

  stats.mean_ms = sum/static_cast<double>(sample_ms.size());
  stats.min_ms = sorted[0];
  stats.max_ms = sorted[sorted.size() - 1];

  if (sorted.size() % 2 == 0)
    stats.median_ms = (sorted[sorted.size()/2 - 1] + sorted[sorted.size()/2])/2.0;
  else
    stats.median_ms = sorted[sorted.size()/2];

  if (sample_ms.size() > 1)
    {
      double sq = 0.0;
      for (const double value : sample_ms)
        {
          const double delta = value - stats.mean_ms;
          sq += delta*delta;
        }

      stats.stddev_ms = std::sqrt(sq/static_cast<double>(sample_ms.size() - 1));
    }

  return stats;
}

template <typename Runner, typename Capture>
[[nodiscard]] BenchmarkRow
measure_case(const std::string &family,
             const std::string &scenario,
             const std::string &variant,
             const Options &options,
             Runner runner,
             Capture capture)
{
  for (size_t i = 0; i < options.warmup; ++i)
    (void) runner();

  BenchmarkRow row;
  row.family = family;
  row.scenario = scenario;
  row.variant = variant;

  Array<double> sample_ms;
  sample_ms.reserve(options.samples);

  for (size_t sample = 0; sample < options.samples; ++sample)
    {
      double total_ms = 0.0;
      for (size_t rep = 0; rep < options.repetitions; ++rep)
        {
          auto result = runner();
          capture(row, result);
          total_ms += result.stats.elapsed_ms;
          benchmark_sink += static_cast<long double>(result.stats.visited_states +
                                                     result.stats.generated_successors +
                                                     result.stats.expanded_states);
        }

      sample_ms.append(total_ms/static_cast<double>(options.repetitions));
    }

  row.timing = compute_timing_stats(sample_ms);
  return row;
}

[[nodiscard]] const BenchmarkRow *
find_row(const Array<BenchmarkRow> &rows,
         const std::string &scenario,
         const std::string &variant)
{
  for (size_t i = 0; i < rows.size(); ++i)
    if (rows[i].scenario == scenario and rows[i].variant == variant)
      return &rows[i];

  return nullptr;
}

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

  bool is_goal(const State &state) const
  {
    return state.row == n;
  }

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

struct KnapsackState
{
  size_t index = 0;
  int weight = 0;
  double value = 0.0;
  Array<unsigned char> chosen;

  explicit KnapsackState(const size_t n = 0)
    : chosen(n, 0)
  {
    // empty
  }
};

class KnapsackDomain
{
public:
  struct Move
  {
    bool take = false;
  };

  using State = KnapsackState;
  using Objective = double;

  KnapsackDomain(const Array<Knapsack_Item<int, double>> &items,
                 const int capacity,
                 const bool take_first = true)
    : items_(items), capacity_(capacity), take_first_(take_first)
  {
    // empty
  }

  bool is_complete(const State &state) const
  {
    return state.index == items_.size();
  }

  Objective objective_value(const State &state) const
  {
    return state.value;
  }

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
          optimistic += items_[i].value*
            (static_cast<Objective>(remaining)/static_cast<Objective>(items_[i].weight));
          break;
        }

    return optimistic;
  }

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

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.index >= items_.size())
      return true;

    const bool can_take = state.weight + items_[state.index].weight <= capacity_;

    if (take_first_)
      {
        if (can_take and not visit(Move{true}))
          return false;

        return visit(Move{false});
      }

    if (not visit(Move{false}))
      return false;

    if (can_take)
      return visit(Move{true});

    return true;
  }

private:
  Array<Knapsack_Item<int, double>> items_;
  int capacity_ = 0;
  bool take_first_ = true;
};

struct TicTacToeState
{
  Array<int> board;
  int player = 1;
  size_t moves_played = 0;

  TicTacToeState()
    : board(size_t(9), 0)
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

[[nodiscard]] Array<Knapsack_Item<int, double>>
make_knapsack_items()
{
  return {
    {2, 30.0}, {3, 42.0}, {4, 52.0}, {5, 60.0},
    {6, 66.0}, {7, 70.0}, {8, 72.0}, {9, 72.0},
    {10, 70.0}, {11, 66.0}, {12, 60.0}, {13, 52.0}
  };
}

[[nodiscard]] Array<BenchmarkRow>
benchmark_nqueens(const Options &options)
{
  constexpr size_t n = 8;
  Search::ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  Array<BenchmarkRow> rows;
  rows.append(measure_case(
    "backtracking",
    "nqueens-8",
    "depth_first",
    options,
    [&]()
    {
      Search::SearchLimits limits;
      limits.max_depth = n;
      return Search::backtracking_search(NQueensDomain{n}, NQueensState(n), policy, limits);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.solutions_found = result.stats.solutions_found;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.result_value = static_cast<double>(result.stats.solutions_found);
    }));

  return rows;
}

[[nodiscard]] Array<BenchmarkRow>
benchmark_knapsack(const Options &options)
{
  const auto items = make_knapsack_items();
  constexpr int capacity = 35;
  const double optimum = Aleph::knapsack_01_value<double, int>(items, capacity);

  Array<BenchmarkRow> rows;

  rows.append(measure_case(
    "branch_and_bound",
    "knapsack-medium",
    "depth_first",
    options,
    [&]()
    {
      auto policy = Search::BranchAndBound<KnapsackDomain>::default_policy();
      Search::SearchLimits limits;
      limits.max_depth = items.size();
      return Search::branch_and_bound_search(KnapsackDomain(items, capacity, false),
                                             KnapsackState(items.size()),
                                             policy,
                                             limits);
    },
    [optimum](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.solutions_found = result.stats.solutions_found;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.pruned_by_bound = result.stats.pruned_by_bound;
      row.incumbent_updates = result.stats.incumbent_updates;
      row.result_value = result.incumbent.best_value();
      benchmark_sink += row.result_value + optimum;
    }));

  rows.append(measure_case(
    "branch_and_bound",
    "knapsack-medium",
    "depth_first_bound_ordered",
    options,
    [&]()
    {
      Search::ExplorationPolicy policy;
      policy.move_ordering = Aleph::MoveOrderingMode::Estimated_Bound;
      policy.stop_at_first_solution = false;

      Search::SearchLimits limits;
      limits.max_depth = items.size();
      return Search::branch_and_bound_search(KnapsackDomain(items, capacity, false),
                                             KnapsackState(items.size()),
                                             policy,
                                             limits);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.solutions_found = result.stats.solutions_found;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.pruned_by_bound = result.stats.pruned_by_bound;
      row.incumbent_updates = result.stats.incumbent_updates;
      row.ordered_batches = result.stats.move_ordering.ordered_batches;
      row.priority_estimates = result.stats.move_ordering.priority_estimates;
      row.result_value = result.incumbent.best_value();
    }));

  rows.append(measure_case(
    "branch_and_bound",
    "knapsack-medium",
    "best_first",
    options,
    [&]()
    {
      Search::ExplorationPolicy policy;
      policy.strategy = Search::ExplorationPolicy::Strategy::Best_First;
      policy.stop_at_first_solution = false;

      Search::SearchLimits limits;
      limits.max_depth = items.size();
      return Search::branch_and_bound_search(KnapsackDomain(items, capacity, false),
                                             KnapsackState(items.size()),
                                             policy,
                                             limits);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.solutions_found = result.stats.solutions_found;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.pruned_by_bound = result.stats.pruned_by_bound;
      row.incumbent_updates = result.stats.incumbent_updates;
      row.result_value = result.incumbent.best_value();
    }));

  return rows;
}

[[nodiscard]] Array<BenchmarkRow>
benchmark_tictactoe(const Options &options)
{
  TicTacToeState root;
  using TT = Search::GameTT<TicTacToeDomain::State_Key,
                            TicTacToeDomain::Move,
                            TicTacToeDomain::Score>;

  Array<BenchmarkRow> rows;

  rows.append(measure_case(
    "adversarial",
    "tictactoe-full",
    "alpha_beta_plain",
    options,
    [&]()
    {
      return Search::alpha_beta_search(TicTacToeDomain{}, root);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.alpha_beta_cutoffs = result.stats.alpha_beta_cutoffs;
      row.result_value = static_cast<double>(result.value);
      row.has_best_move = result.has_principal_variation();
      row.best_move = result.first_move().cell;
    }));

  rows.append(measure_case(
    "adversarial",
    "tictactoe-full",
    "alpha_beta_ordered",
    options,
    [&]()
    {
      Search::ExplorationPolicy policy = Search::AlphaBeta<TicTacToeDomain>::default_policy();
      policy.move_ordering = Aleph::MoveOrderingMode::Estimated_Score;
      policy.use_killer_moves = true;
      policy.use_history_heuristic = true;
      return Search::alpha_beta_search(TicTacToeDomain{}, root, policy);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.alpha_beta_cutoffs = result.stats.alpha_beta_cutoffs;
      row.ordered_batches = result.stats.move_ordering.ordered_batches;
      row.priority_estimates = result.stats.move_ordering.priority_estimates;
      row.killer_hits = result.stats.move_ordering.killer_hits;
      row.history_hits = result.stats.move_ordering.history_hits;
      row.result_value = static_cast<double>(result.value);
      row.has_best_move = result.has_principal_variation();
      row.best_move = result.first_move().cell;
    }));

  rows.append(measure_case(
    "adversarial",
    "tictactoe-full",
    "alpha_beta_tt",
    options,
    [&]()
    {
      TT table;
      return Search::alpha_beta_search(TicTacToeDomain{}, root, table);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.alpha_beta_cutoffs = result.stats.alpha_beta_cutoffs;
      row.tt_hits = result.stats.transpositions.hits;
      row.tt_cutoffs = result.stats.transpositions.cutoffs;
      row.tt_stores = result.stats.transpositions.stores;
      row.result_value = static_cast<double>(result.value);
      row.has_best_move = result.has_principal_variation();
      row.best_move = result.first_move().cell;
    }));

  rows.append(measure_case(
    "adversarial",
    "tictactoe-full",
    "alpha_beta_ordered_tt",
    options,
    [&]()
    {
      Search::ExplorationPolicy policy = Search::AlphaBeta<TicTacToeDomain>::default_policy();
      policy.move_ordering = Aleph::MoveOrderingMode::Estimated_Score;
      policy.use_killer_moves = true;
      policy.use_history_heuristic = true;
      TT table;
      return Search::alpha_beta_search(TicTacToeDomain{}, root, table, policy);
    },
    [](BenchmarkRow &row, const auto &result)
    {
      row.visited_states = result.stats.visited_states;
      row.expanded_states = result.stats.expanded_states;
      row.generated_successors = result.stats.generated_successors;
      row.terminal_states = result.stats.terminal_states;
      row.pruned_by_depth = result.stats.pruned_by_depth;
      row.pruned_by_domain = result.stats.pruned_by_domain;
      row.alpha_beta_cutoffs = result.stats.alpha_beta_cutoffs;
      row.tt_hits = result.stats.transpositions.hits;
      row.tt_cutoffs = result.stats.transpositions.cutoffs;
      row.tt_stores = result.stats.transpositions.stores;
      row.ordered_batches = result.stats.move_ordering.ordered_batches;
      row.priority_estimates = result.stats.move_ordering.priority_estimates;
      row.killer_hits = result.stats.move_ordering.killer_hits;
      row.history_hits = result.stats.move_ordering.history_hits;
      row.result_value = static_cast<double>(result.value);
      row.has_best_move = result.has_principal_variation();
      row.best_move = result.first_move().cell;
    }));

  return rows;
}

void validate_rows(const Array<BenchmarkRow> &rows)
{
  const auto *nqueens = find_row(rows, "nqueens-8", "depth_first");
  const auto *knapsack_plain = find_row(rows, "knapsack-medium", "depth_first");
  const auto *knapsack_ordered = find_row(rows, "knapsack-medium", "depth_first_bound_ordered");
  const auto *knapsack_best = find_row(rows, "knapsack-medium", "best_first");
  const auto *plain = find_row(rows, "tictactoe-full", "alpha_beta_plain");
  const auto *ordered = find_row(rows, "tictactoe-full", "alpha_beta_ordered");
  const auto *tt = find_row(rows, "tictactoe-full", "alpha_beta_tt");
  const auto *ordered_tt = find_row(rows, "tictactoe-full", "alpha_beta_ordered_tt");

  if (nqueens == nullptr or knapsack_plain == nullptr or knapsack_ordered == nullptr or
      knapsack_best == nullptr or plain == nullptr or ordered == nullptr or
      tt == nullptr or ordered_tt == nullptr)
    throw std::runtime_error("Benchmark validation failed: missing benchmark row");

  if (nqueens->solutions_found != 92u)
    throw std::runtime_error("N-Queens validation failed: expected 92 solutions");

  if (knapsack_plain->result_value != knapsack_ordered->result_value or
      knapsack_plain->result_value != knapsack_best->result_value)
    throw std::runtime_error("Knapsack validation failed: inconsistent optimum");
  if (knapsack_ordered->visited_states > knapsack_plain->visited_states)
    throw std::runtime_error("Knapsack validation failed: bound ordering did not help");
  if (knapsack_best->pruned_by_bound == 0)
    throw std::runtime_error("Knapsack validation failed: best-first did not prune");

  if (plain->result_value != ordered->result_value or
      plain->result_value != tt->result_value or
      plain->result_value != ordered_tt->result_value)
    throw std::runtime_error("Alpha-Beta validation failed: inconsistent root value");
  if (ordered->visited_states >= plain->visited_states)
    throw std::runtime_error("Alpha-Beta validation failed: move ordering did not reduce nodes");
  if (tt->visited_states >= plain->visited_states)
    throw std::runtime_error("Alpha-Beta validation failed: TT did not reduce nodes");
}

void print_row_text(const BenchmarkRow &row)
{
  std::cout << row.family << " / " << row.scenario << " / " << row.variant << '\n';
  std::cout << "  mean_ms: " << row.timing.mean_ms
            << " median_ms: " << row.timing.median_ms
            << " min_ms: " << row.timing.min_ms
            << " max_ms: " << row.timing.max_ms << '\n';
  std::cout << "  visited: " << row.visited_states
            << " expanded: " << row.expanded_states
            << " generated: " << row.generated_successors << '\n';

  if (row.solutions_found > 0)
    std::cout << "  solutions: " << row.solutions_found << '\n';

  if (row.pruned_by_bound > 0)
    std::cout << "  pruned_by_bound: " << row.pruned_by_bound
              << " incumbent_updates: " << row.incumbent_updates << '\n';

  if (row.alpha_beta_cutoffs > 0 or row.tt_hits > 0)
    std::cout << "  alpha_beta_cutoffs: " << row.alpha_beta_cutoffs
              << " tt_hits: " << row.tt_hits
              << " tt_cutoffs: " << row.tt_cutoffs << '\n';

  if (row.ordered_batches > 0 or row.priority_estimates > 0)
    std::cout << "  ordered_batches: " << row.ordered_batches
              << " priority_estimates: " << row.priority_estimates
              << " killer_hits: " << row.killer_hits
              << " history_hits: " << row.history_hits << '\n';

  std::cout << "  result_value: " << row.result_value;
  if (row.has_best_move)
    std::cout << " best_move: " << row.best_move;
  std::cout << '\n';
}

void print_json(const Array<BenchmarkRow> &rows, const Options &options)
{
  std::cout << "{\n";
  std::cout << "  \"repetitions\": " << options.repetitions << ",\n";
  std::cout << "  \"warmup\": " << options.warmup << ",\n";
  std::cout << "  \"samples\": " << options.samples << ",\n";
  std::cout << "  \"rows\": [\n";

  for (size_t i = 0; i < rows.size(); ++i)
    {
      const auto &row = rows[i];
      std::cout
        << "    {\n"
        << "      \"family\": \"" << row.family << "\",\n"
        << "      \"scenario\": \"" << row.scenario << "\",\n"
        << "      \"variant\": \"" << row.variant << "\",\n"
        << "      \"mean_ms\": " << row.timing.mean_ms << ",\n"
        << "      \"median_ms\": " << row.timing.median_ms << ",\n"
        << "      \"min_ms\": " << row.timing.min_ms << ",\n"
        << "      \"max_ms\": " << row.timing.max_ms << ",\n"
        << "      \"stddev_ms\": " << row.timing.stddev_ms << ",\n"
        << "      \"visited_states\": " << row.visited_states << ",\n"
        << "      \"expanded_states\": " << row.expanded_states << ",\n"
        << "      \"generated_successors\": " << row.generated_successors << ",\n"
        << "      \"solutions_found\": " << row.solutions_found << ",\n"
        << "      \"pruned_by_bound\": " << row.pruned_by_bound << ",\n"
        << "      \"alpha_beta_cutoffs\": " << row.alpha_beta_cutoffs << ",\n"
        << "      \"tt_hits\": " << row.tt_hits << ",\n"
        << "      \"tt_cutoffs\": " << row.tt_cutoffs << ",\n"
        << "      \"ordered_batches\": " << row.ordered_batches << ",\n"
        << "      \"priority_estimates\": " << row.priority_estimates << ",\n"
        << "      \"result_value\": " << row.result_value << ",\n"
        << "      \"has_best_move\": " << (row.has_best_move ? "true" : "false") << ",\n"
        << "      \"best_move\": " << row.best_move << '\n'
        << "    }";
      if (i + 1 != rows.size())
        std::cout << ',';
      std::cout << '\n';
    }

  std::cout << "  ]\n}\n";
}

} // end namespace

int main(const int argc, char **argv)
{
  const Options options = parse_options(argc, argv);

  Array<BenchmarkRow> rows;
  auto nqueens_rows = benchmark_nqueens(options);
  auto knapsack_rows = benchmark_knapsack(options);
  auto tictactoe_rows = benchmark_tictactoe(options);

  rows.append(nqueens_rows);
  rows.append(knapsack_rows);
  rows.append(tictactoe_rows);

  if (options.validate)
    validate_rows(rows);

  std::cout << std::fixed << std::setprecision(3);

  if (options.json)
    print_json(rows, options);
  else
    {
      std::cout << "State-space search benchmark suite\n";
      std::cout << "repetitions=" << options.repetitions
                << " warmup=" << options.warmup
                << " samples=" << options.samples << "\n\n";

      for (const auto &row : rows)
        {
          print_row_text(row);
          std::cout << '\n';
        }

      if (options.validate)
        std::cout << "validation: ok\n";
    }

  return 0;
}
