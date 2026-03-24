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

/** @file backtracking_cyclic_graph_example.cc
 *  @brief Reference example: DFS/backtracking on a cyclic directed graph
 *         using a depth-aware visited map (`SearchStorageMap`) to avoid loops.
 *
 *  Without cycle detection, a DFS on a graph with back-edges would loop
 *  indefinitely.  Passing a `SearchStorageMap<Key, size_t>` to
 *  `Depth_First_Backtracking::search()` turns the tree-search into a
 *  graph-search: each state is recorded with the depth at which it was first
 *  reached.  When the same state is encountered again via a longer path, it is
 *  pruned.  Via a shorter path the stored depth is updated and exploration
 *  continues.
 *
 *  Graph used in this example (directed):
 *
 *  @code
 *    0 ──► 1 ──► 0  (back-edge, creates a cycle)
 *          │
 *          ▼
 *          2 ──► 3  (goal)
 *  @endcode
 *
 *  Without visited tracking the engine would loop between nodes 0 and 1
 *  forever.  With visited tracking it prunes the 1→0 back-edge and finds
 *  the goal at node 3 via the path 0→1→2→3.
 *
 *  Build and run:
 *  - `cmake --build build --target backtracking_cyclic_graph_example`
 *  - `./build/Examples/backtracking_cyclic_graph_example`
 */

#include <cstddef>
#include <iostream>

#include <State_Search.H>

using namespace Aleph;

// ---------------------------------------------------------------------------
// Domain definition
// ---------------------------------------------------------------------------

struct GraphState
{
  size_t node = 0;
};

struct CyclicGraphDomain
{
  struct Move
  {
    size_t from = 0;
    size_t to   = 0;
  };

  using State     = GraphState;
  using State_Key = size_t;

  [[nodiscard]] State_Key state_key(const State &s) const noexcept
  {
    return s.node;
  }

  [[nodiscard]] bool is_goal(const State &s) const noexcept
  {
    return s.node == 3;
  }

  void apply(State &s, const Move &m) const noexcept
  {
    s.node = m.to;
  }

  void undo(State &s, const Move &m) const noexcept
  {
    s.node = m.from;
  }

  template <typename Visitor>
  bool for_each_successor(const State &s, Visitor visit) const
  {
    switch (s.node)
      {
      case 0:
        return visit(Move{0, 1});
      case 1:
        if (not visit(Move{1, 0}))  // back-edge (cycle)
          return false;
        return visit(Move{1, 2});
      case 2:
        return visit(Move{2, 3});
      default:
        return true;
      }
  }
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
  CyclicGraphDomain domain;
  Depth_First_Backtracking<CyclicGraphDomain> engine(domain);

  // Depth-aware visited map: key=node id, value=minimum depth seen (updated if a shorter path is found).
  SearchStorageMap<size_t, size_t> visited;

  auto result = engine.search(GraphState{}, visited);

  if (result.found_solution())
    {
      std::cout << "Goal reached at node "
                << result.best_solution.get().state.node << "\n";
      std::cout << "Visited states   : " << result.stats.visited_states << "\n";
      std::cout << "Pruned by visited: " << result.stats.pruned_by_visited << "\n";
    }
  else
    {
      std::cout << "No solution found.\n";
    }

  return 0;
}