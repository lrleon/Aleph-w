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
 * @file ca_ghost_gol_toroidal_example.cc
 * @brief Game of Life on a Ghost_Lattice with toroidal borders.
 *
 * Drops the canonical 5-cell glider onto a 12x12 Ghost_Lattice with
 * `PeriodicBoundary` (halo radius = 1), runs Conway's Game of Life
 * for 48 steps and prints every 4th frame. The glider walks
 * diagonally and seamlessly re-enters from the opposite edge when it
 * crosses the border, proving that the halo refresh is correct every
 * step.
 *
 * Run it to see a glider travel across the torus:
 *
 *     $ ./build/Examples/ca_ghost_gol_toroidal_example
 */

#include <iostream>
#include <utility>

#include <ca-traits.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_ghost_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_engine.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  constexpr ca_size_t N = 12;
  constexpr std::size_t TOTAL_STEPS = 48;
  constexpr std::size_t PRINT_EVERY = 4;

  using Storage = Dense_Cell_Storage<int, 2>;
  using GhostL = Ghost_Lattice<Storage, PeriodicBoundary, 1>;
  using Engine = Synchronous_Engine<GhostL, Game_Of_Life_Rule, Moore<2, 1>>;

  /// Stamp the canonical Conway glider near coordinate (0, 0).
  void stamp_glider(GhostL &lat)
  {
    // Canonical 5-cell glider:
    //   . X .
    //   . . X
    //   X X X
    lat.set({ 0, 1 }, 1);
    lat.set({ 1, 2 }, 1);
    lat.set({ 2, 0 }, 1);
    lat.set({ 2, 1 }, 1);
    lat.set({ 2, 2 }, 1);
  }

  void render_frame(const GhostL &lat, std::size_t step)
  {
    std::cout << "\nStep " << step << '\n';
    std::cout << "+";
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      std::cout << '-';
    std::cout << "+\n";
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(lat.size(0)); ++i)
      {
        std::cout << '|';
        for (ca_index_t j = 0; j < static_cast<ca_index_t>(lat.size(1)); ++j)
          std::cout << (lat.at({ i, j }) ? '#' : ' ');
        std::cout << "|\n";
      }
    std::cout << "+";
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      std::cout << '-';
    std::cout << "+\n";
  }
}

int main()
{
  std::cout <<
    "Game of Life on a 12x12 Ghost_Lattice (PeriodicBoundary, halo=1).\n"
    "A glider is stamped near the top-left corner. It walks diagonally\n"
    "at one cell every four generations and wraps cleanly across the\n"
    "torus because the engine refreshes the halo before each step.\n";

  GhostL lat({ N, N }, 0);
  stamp_glider(lat);
  Engine engine(std::move(lat), make_game_of_life_rule(), Moore<2, 1>{});

  render_frame(engine.frame(), 0);
  for (std::size_t s = 1; s <= TOTAL_STEPS; ++s)
    {
      engine.step();
      if (s % PRINT_EVERY == 0)
        render_frame(engine.frame(), s);
    }
  return 0;
}
