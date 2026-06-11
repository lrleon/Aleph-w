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
 * @file ca_hex_life_example.cc
 * @brief Phase-6 illustration: Bays' B2/S34 hexagonal Game of Life.
 *
 * Carter Bays' rule on a hex lattice (B2/S34) is the canonical
 * "interesting" hex Life: a cell is born with exactly two alive
 * neighbours and survives with three or four. This example seeds a
 * small symmetric pattern, runs it for a few generations on a 16x16
 * toroidal hex lattice, prints ASCII snapshots in offset-coords and
 * dumps a TikZ render of the final state to stdout for inclusion in
 * a LaTeX document.
 */

#include <cstdio>

#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace {

struct Bays_B2_S34_Functor
{
  template <typename State>
  [[nodiscard]] constexpr State operator()(const State &current,
                                           const std::size_t alive) const noexcept
  {
    const bool alive_now = current != State{};
    const bool next_alive = (alive_now and (alive == 3 or alive == 4))
                            or (not alive_now and alive == 2);
    return next_alive ? static_cast<State>(1) : static_cast<State>(0);
  }
};

using L = Hex_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

void print_offset_view(const L &lat, std::size_t step)
{
  std::size_t alive = 0;
  for (ca_size_t r = 0; r < lat.rows(); ++r)
    for (ca_size_t q = 0; q < lat.cols(); ++q)
      if (lat.at_axial(Hex_Axial{static_cast<ca_index_t>(q), static_cast<ca_index_t>(r)})
          != 0)
        ++alive;

  std::printf("\nStep %2zu  alive=%zu\n", step, alive);
  for (ca_size_t r = 0; r < lat.rows(); ++r)
    {
      // Indent odd rows by one space to visually approximate the hex
      // offset-r staggered layout.
      if ((r & 1) != 0)
        std::putchar(' ');
      for (ca_size_t q = 0; q < lat.cols(); ++q)
        {
          const auto v = lat.at_axial(
            Hex_Axial{static_cast<ca_index_t>(q), static_cast<ca_index_t>(r)});
          std::printf("%c ", v != 0 ? '#' : '.');
        }
      std::putchar('\n');
    }
}

}  // namespace

int main()
{
  L seed({16, 16}, 0);

  // Seed a small symmetric "ring" pattern (alive cells on the
  // hex distance-2 circle around (8, 8)).
  const Hex_Axial centre{8, 8};
  for (ca_index_t dq = -2; dq <= 2; ++dq)
    for (ca_index_t dr = -2; dr <= 2; ++dr)
      {
        const Hex_Axial p{centre.q + dq, centre.r + dr};
        if (hex_distance(p, centre) == 2)
          seed.set_axial(p, 1);
      }

  std::printf("Aleph::CA Phase-6 example: Bays' B2/S34 hex life on a 16x16 toroidal lattice\n");
  print_offset_view(seed, 0);

  using Rule = Outer_Totalistic_Rule<Bays_B2_S34_Functor>;
  Synchronous_Engine<L, Rule, Hex_Neighborhood> eng(
    seed, Rule{Bays_B2_S34_Functor{}}, Hex_Neighborhood{});

  for (std::size_t s = 1; s <= 6; ++s)
    {
      eng.step();
      print_offset_view(eng.frame(), s);
    }

  std::printf("\n--- TikZ render of step 6 ---\n");
  std::fputs(render_hex_lattice_tikz(eng.frame(),
                                     [](int v) {
                                       return v != 0 ? "black" : "white";
                                     },
                                     /*radius=*/0.25)
               .c_str(),
             stdout);

  return 0;
}