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

/**
 * @file ca_triangular_example.cc
 * @brief Phase-6 illustration: a triangular lattice CA with up/down
 *        parity-aware neighbours.
 *
 * Drives a small (8x8) triangular lattice with a "spread once"
 * totalistic rule: a cell becomes alive if it has at least one alive
 * edge-neighbour, and stays alive thereafter. The example prints
 * ASCII snapshots and dumps a TikZ render of the final frame.
 */

#include <cstdio>

#include <tpl_ca_engine.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_triangular_lattice.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace {

struct Spread_Once_Functor
{
  template <typename State>
  [[nodiscard]] constexpr State operator()(const State &current,
                                           const std::size_t alive) const noexcept
  {
    return (current != State{} or alive >= 1) ? static_cast<State>(1)
                                              : static_cast<State>(0);
  }
};

using L = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

void print(const L &lat, std::size_t step)
{
  std::size_t alive = 0;
  for (ca_size_t i = 0; i < lat.rows(); ++i)
    for (ca_size_t j = 0; j < lat.cols(); ++j)
      if (lat.at_tri(Tri_Coord{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != 0)
        ++alive;
  std::printf("\nStep %2zu  alive=%zu\n", step, alive);
  for (ca_size_t i = 0; i < lat.rows(); ++i)
    {
      for (ca_size_t j = 0; j < lat.cols(); ++j)
        {
          const Tri_Coord c{static_cast<ca_index_t>(i),
                            static_cast<ca_index_t>(j)};
          const char glyph
            = lat.at_tri(c) != 0 ? (tri_is_up(c) ? '^' : 'v') : '.';
          std::printf("%c ", glyph);
        }
      std::putchar('\n');
    }
}

}  // namespace

int main()
{
  L seed({8, 8}, 0);
  // Seed a single up-triangle at (3, 3) and a down-triangle far from it.
  seed.set_tri(Tri_Coord{3, 3}, 1);
  seed.set_tri(Tri_Coord{6, 0}, 1);

  std::printf(
    "Aleph::CA Phase-6 example: triangular spread-once on an 8x8 lattice\n");
  print(seed, 0);

  using Rule = Outer_Totalistic_Rule<Spread_Once_Functor>;
  Synchronous_Engine<L, Rule, Triangular_Neighborhood> eng(
    seed, Rule{Spread_Once_Functor{}}, Triangular_Neighborhood{});
  for (std::size_t s = 1; s <= 6; ++s)
    {
      eng.step();
      print(eng.frame(), s);
    }

  std::printf("\n--- TikZ render of step 6 ---\n");
  std::fputs(render_triangular_lattice_tikz(
               eng.frame(),
               [](int v) { return v != 0 ? "blue" : "white"; },
               /*side=*/0.6)
               .c_str(),
             stdout);
  return 0;
}