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
#include <string>

#include <State_Search.H>

using namespace Aleph;

namespace
{

struct ArtificialGameState
{
  size_t depth = 0;
  size_t code = 1;
  int player = 1;
};

struct ArtificialGameMove
{
  size_t next_code = 1;
  char label = 'A';
};

class ArtificialGameDomain
{
public:
  using State = ArtificialGameState;
  using Move = ArtificialGameMove;
  using Score = int;

  bool is_terminal(const State &state) const
  {
    return state.depth == 2;
  }

  Score evaluate(const State &state) const
  {
    return root_score(state.code)*state.player;
  }

  void apply(State &state, const Move &move) const
  {
    state.code = move.next_code;
    state.player = -state.player;
    ++state.depth;
  }

  void undo(State &state, const Move&) const
  {
    --state.depth;
    state.player = -state.player;
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
        if (not visit(Move{2, 'A'}))
          return false;
        return visit(Move{3, 'B'});

      case 2:
        if (not visit(Move{4, 'a'}))
          return false;
        return visit(Move{5, 'b'});

      case 3:
        if (not visit(Move{6, 'a'}))
          return false;
        return visit(Move{7, 'b'});

      default:
        return true;
      }
  }

private:
  [[nodiscard]] static Score root_score(const size_t code) noexcept
  {
    switch (code)
      {
      case 2: return 6;
      case 3: return 1;
      case 4: return 3;
      case 5: return 5;
      case 6: return 2;
      case 7: return 4;
      default: return 0;
      }
  }
};

std::string signature(const SearchPath<ArtificialGameMove> &path)
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
  std::cout << "  value: " << result.value << '\n';
  std::cout << "  pv: " << signature(result.principal_variation) << '\n';
  std::cout << "  visited: " << result.stats.visited_states << '\n';
  std::cout << "  cutoffs: " << result.stats.alpha_beta_cutoffs << '\n';
}

} // end namespace

int main()
{
  const ArtificialGameState root;

  auto negamax = negamax_search(ArtificialGameDomain{}, root);
  auto alpha_beta = alpha_beta_search(ArtificialGameDomain{}, root);

  print_summary("Negamax", negamax);
  print_summary("Alpha-Beta", alpha_beta);

  return 0;
}
