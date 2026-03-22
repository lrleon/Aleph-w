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

/** @file backtracking_subset_sum_example.cc
 *  @brief Reference example: subset-sum enumeration with domain-side pruning.
 *
 *  Build and run:
 *
 *  - `cmake --build build --target backtracking_subset_sum_example`
 *  - `./build/Examples/backtracking_subset_sum_example`
 */

# include <iostream>
# include <string>

# include <State_Search.H>

namespace
{

namespace Search = Aleph::Search;
using Aleph::Array;

struct SubsetSumState
{
  size_t index = 0;
  int sum = 0;
  Array<unsigned char> chosen;

  explicit SubsetSumState(const size_t n = 0)
    : index(0), sum(0), chosen(n, 0)
  {
    // empty
  }
};

class SubsetSumDomain
{
public:
  struct Move
  {
    bool take = false;
  };

  using State = SubsetSumState;

  explicit SubsetSumDomain(const Array<int> &values, const int target)
    : values_(values),
      suffix_remaining_(values.size() + 1, 0),
      target_(target)
  {
    for (size_t i = values_.size(); i > 0; --i)
      suffix_remaining_[i - 1] = suffix_remaining_[i] + values_[i - 1];
  }

  bool is_goal(const State &state) const
  {
    return state.sum == target_;
  }

  bool is_terminal(const State &state) const
  {
    return state.index == values_.size();
  }

  bool should_prune(const State &state, const size_t) const
  {
    return state.sum > target_ or state.sum + suffix_remaining_[state.index] < target_;
  }

  void apply(State &state, const Move &move) const
  {
    if (move.take)
      state.sum += values_[state.index];

    state.chosen[state.index] = move.take ? 1 : 0;
    ++state.index;
  }

  void undo(State &state, const Move &move) const
  {
    --state.index;
    if (move.take)
      state.sum -= values_[state.index];

    state.chosen[state.index] = 0;
  }

  template <typename Visitor>
  bool for_each_successor(const State &state, Visitor visit) const
  {
    if (state.index >= values_.size())
      return true;

    if (not visit(Move{true}))
      return false;

    return visit(Move{false});
  }

  const Array<int> &values() const noexcept
  {
    return values_;
  }

private:
  Array<int> values_;
  Array<int> suffix_remaining_;
  int target_ = 0;
};

std::string signature(const SubsetSumState &state)
{
  std::string s;
  for (const auto pick : state.chosen)
    s.push_back(pick ? '1' : '0');
  return s;
}

void print_subset(const Array<int> &values, const SubsetSumState &state)
{
  std::cout << "{";
  bool first = true;
  for (size_t i = 0; i < values.size(); ++i)
    if (state.chosen[i])
      {
        if (not first)
          std::cout << ", ";
        std::cout << values[i];
        first = false;
      }
  std::cout << "}";
}

} // end namespace

int main()
{
  const Array<int> values = {4, 1, 1, 2};
  constexpr int target = 2;

  SubsetSumDomain domain(values, target);
  Search::ExplorationPolicy policy;
  policy.stop_at_first_solution = false;

  using Engine = Search::Backtracking<SubsetSumDomain>;
  Engine engine(domain, policy);
  Search::Collector<Engine::Solution> collector;

  auto result = engine.search(SubsetSumState(values.size()), collector);

  std::cout << "Subset sum target: " << target << '\n';
  std::cout << "Solutions found: " << result.stats.solutions_found << '\n';
  std::cout << "Pruned states: " << result.stats.pruned_by_domain << '\n';

  for (auto it = collector.solutions().get_it(); it.has_curr(); it.next_ne())
    {
      const auto &solution = it.get_curr();
      std::cout << "Choice mask: " << signature(solution.state) << " -> ";
      print_subset(values, solution.state);
      std::cout << '\n';
    }

  return 0;
}
