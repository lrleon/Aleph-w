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

/** @file state_search_framework_example.cc
 *  @brief Reference example: minimal artificial decision tree with DFS.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target state_search_framework_example`
 *  - `./build/Examples/state_search_framework_example`
 */

# include <iostream>
# include <string>

# include <State_Search.H>

using namespace Aleph;

namespace
{

struct ArtificialTreeState
{
  size_t depth = 0;
  size_t code = 1;
};

struct ArtificialDecisionTreeDomain
{
  struct Move
  {
    char label = 'L';
  };

  using State = ArtificialTreeState;

  size_t leaf_depth = 2;

  bool is_goal(const State &state) const
  {
    return state.depth == leaf_depth and (state.code == 5 or state.code == 6);
  }

  bool is_terminal(const State &state) const
  {
    return state.depth == leaf_depth;
  }

  void apply(State &state, const Move &move) const
  {
    state.code = state.code*2 + (move.label == 'R' ? 1u : 0u);
    ++state.depth;
  }

  void undo(State &state, const Move &move) const
  {
    --state.depth;
    state.code = (state.code - (move.label == 'R' ? 1u : 0u))/2;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.depth >= leaf_depth)
      return true;

    if (not visit(Move{'L'}))
      return false;

    return visit(Move{'R'});
  }
};

std::string path_signature(const SearchPath<ArtificialDecisionTreeDomain::Move> &path)
{
  std::string signature;
  for (const auto &move : path)
    signature.push_back(move.label);
  return signature;
}

} // end namespace

int main()
{
  ArtificialDecisionTreeDomain domain;
  ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  using Engine = Depth_First_Backtracking<ArtificialDecisionTreeDomain>;
  Engine engine(domain, policy);
  SearchSolutionCollector<Engine::Solution> collector;

  auto result = engine.search(ArtificialTreeState{}, collector);

  std::cout << "Visited states: " << result.stats.visited_states << '\n';
  std::cout << "Expanded states: " << result.stats.expanded_states << '\n';
  std::cout << "Solutions found: " << result.stats.solutions_found << '\n';

  for (auto it = collector.solutions().get_it(); it.has_curr(); it.next_ne())
    std::cout << "Solution path: " << path_signature(it.get_curr().path) << '\n';

  return 0;
}
