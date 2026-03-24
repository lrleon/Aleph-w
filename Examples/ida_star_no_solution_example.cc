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

/** @file ida_star_no_solution_example.cc
 *  @brief Reference example: IDA* on a DAG where no path to the goal exists.
 *
 *  This example demonstrates the behaviour of IDA* when the goal is
 *  completely unreachable.  The search exhausts all reachable states and
 *  returns `SearchStatus::Exhausted` with `found_solution() == false`.
 *
 *  Graph (directed, acyclic):
 *  @code
 *    0 ──► 1 ──► 2
 *               (dead end)
 *    Goal: node 9 (disconnected from the reachable component)
 *  @endcode
 *
 *  Because the graph is a DAG, IDA* terminates without any depth limit:
 *  once all paths from the root are exhausted, `next_bound` reaches
 *  `numeric_limits<Distance>::max()` and the engine reports `Exhausted`.
 *
 *  A second scenario below shows an inadmissible (over-estimating) heuristic
 *  on a reachable goal: IDA* still finds a solution, though not necessarily
 *  the optimal one.
 *
 *  Build and run:
 *  - `cmake --build build --target ida_star_no_solution_example`
 *  - `./build/Examples/ida_star_no_solution_example`
 */

#include <cstddef>
#include <iostream>
#include <limits>

#include <State_Search_IDA_Star.H>

using namespace Aleph;

// ---------------------------------------------------------------------------
// Shared state type
// ---------------------------------------------------------------------------

struct NodeState
{
  size_t node = 0;
};

// ---------------------------------------------------------------------------
// Scenario A: unreachable goal on a small DAG
// ---------------------------------------------------------------------------

class UnreachableGoalDomain
{
public:
  struct Move
  {
    size_t to   = 0;
    int    cost = 1;
  };

  using State     = NodeState;
  using State_Key = size_t;
  using Distance  = int;

  [[nodiscard]] State_Key state_key(const State &s) const noexcept
  {
    return s.node;
  }

  [[nodiscard]] bool is_goal(const State &s) const noexcept
  {
    return s.node == 9;  // node 9 has no incoming edges
  }

  void apply(State &s, const Move &m) const noexcept
  {
    s.node = m.to;
  }

  void undo(State &s, const Move &m) const noexcept
  {
    // Restore the parent node using the move's destination (DAG: to==1 ⟹ came from 0).
    s.node = (m.to == 1) ? 0 : (m.to == 2) ? 1 : s.node;
  }

  template <typename Visitor>
  bool for_each_successor(const State &s, Visitor visit) const
  {
    if (s.node == 0)
      return visit(Move{1, 1});
    if (s.node == 1)
      return visit(Move{2, 1});
    return true;  // node 2 is a dead end; node 9 is unreachable
  }

  [[nodiscard]] Distance heuristic(const State &) const noexcept
  {
    return 1;  // constant — admissible (never overestimates for an unreachable goal)
  }

  [[nodiscard]] Distance cost(const State &, const Move &m) const noexcept
  {
    return m.cost;
  }
};

// ---------------------------------------------------------------------------
// Scenario B: inadmissible heuristic on a reachable goal
//
// Graph:  0 ──(1)──► 1 ──(1)──► 2   (goal)
// Heuristic h(0) = h(1) = 100, h(2) = 0  (massively overestimates)
// Optimal cost = 2; because h is inadmissible IDA* is NOT guaranteed
// to find the optimum, but it MUST find some solution.
// ---------------------------------------------------------------------------

class InadmissibleDomain
{
public:
  struct Move
  {
    size_t to   = 0;
    int    cost = 1;
  };

  using State     = NodeState;
  using State_Key = size_t;
  using Distance  = int;

  [[nodiscard]] State_Key state_key(const State &s) const noexcept
  {
    return s.node;
  }

  [[nodiscard]] bool is_goal(const State &s) const noexcept
  {
    return s.node == 2;
  }

  void apply(State &s, const Move &m) const noexcept
  {
    s.node = m.to;
  }

  void undo(State &s, const Move &m) const noexcept
  {
    s.node = (m.to == 1) ? 0 : 1;
  }

  template <typename Visitor>
  bool for_each_successor(const State &s, Visitor visit) const
  {
    if (s.node == 0)
      return visit(Move{1, 1});
    if (s.node == 1)
      return visit(Move{2, 1});
    return true;
  }

  [[nodiscard]] Distance heuristic(const State &s) const noexcept
  {
    return s.node == 2 ? 0 : 100;  // inadmissible overestimate
  }

  [[nodiscard]] Distance cost(const State &, const Move &m) const noexcept
  {
    return m.cost;
  }
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
  // --- Scenario A ---
  {
    UnreachableGoalDomain domain;
    IDA_Star_State_Search<UnreachableGoalDomain> engine(domain);
    const auto result = engine.search(NodeState{});

    std::cout << "=== Scenario A: unreachable goal ===\n";
    std::cout << "Found solution : " << (result.found_solution() ? "yes" : "no") << "\n";
    std::cout << "Status         : " << (result.exhausted() ? "Exhausted" : "other") << "\n";
    std::cout << "States visited : " << result.stats.visited_states << "\n";
    std::cout << "IDA* iterations: " << result.iterations.size() << "\n";
  }

  std::cout << "\n";

  // --- Scenario B ---
  {
    InadmissibleDomain domain;
    IDA_Star_State_Search<InadmissibleDomain> engine(domain);
    const auto result = engine.search(NodeState{});

    std::cout << "=== Scenario B: inadmissible heuristic ===\n";
    std::cout << "Found solution : " << (result.found_solution() ? "yes" : "no") << "\n";
    std::cout << "Total cost     : " << result.total_cost << "\n";
    std::cout << "Note: optimal cost is 2; inadmissible h may or may not match it.\n";
  }

  return 0;
}