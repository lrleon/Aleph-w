/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file main.cc
 * @brief External-consumer smoke test: run a Game of Life blinker.
 *
 * This program is intentionally tiny. It exists only to prove that an
 * installed Aleph-w can be consumed through `find_package(Aleph)` and that the
 * cellular-automata headers compile and link against the installed archive.
 * A period-2 blinker is the simplest oscillator whose evolution is trivial to
 * assert, so it doubles as a correctness check.
 */

#include <cstdlib>
#include <iostream>

#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

/** @brief Count live cells in a binary frame.
 * @param frame Game-of-Life frame.
 * @return number of non-zero cells.
 */
std::size_t population(const Grid &frame)
{
  std::size_t count = 0;
  for (ca_size_t r = 0; r < frame.size(0); ++r)
    for (ca_size_t c = 0; c < frame.size(1); ++c)
      if (frame.at({static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}) != 0)
        ++count;
  return count;
}

}  // namespace

int main()
{
  // Horizontal blinker on a 5x5 torus: three cells in the middle row.
  Grid seed({5, 5}, 0);
  seed.set({2, 1}, 1);
  seed.set({2, 2}, 1);
  seed.set({2, 3}, 1);

  Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>
    engine(seed, make_game_of_life_rule(), Moore<2, 1>{});

  const std::size_t start = population(engine.frame());
  engine.run(1);  // blinker turns vertical, population stays 3.
  const std::size_t after_one = population(engine.frame());
  engine.run(1);  // returns to the horizontal phase.
  const std::size_t after_two = population(engine.frame());

  std::cout << "Aleph-w consumer: Game of Life blinker\n"
            << "  population: " << start << " -> " << after_one
            << " -> " << after_two << '\n';

  const bool ok = start == 3 and after_one == 3 and after_two == 3
                  and engine.frame().at({2, 1}) == 1
                  and engine.frame().at({2, 3}) == 1;
  if (not ok)
    {
      std::cerr << "consumer: blinker did not behave as a period-2 oscillator\n";
      return EXIT_FAILURE;
    }
  std::cout << "  find_package(Aleph) integration OK\n";
  return EXIT_SUCCESS;
}