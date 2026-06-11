/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file game_of_life_bench.cc
 * @brief Dense-step versus Hashlife Gosper-gun benchmark.
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <reproductions/ca_reproduction_support.H>
#include <tpl_ca_hashlife.H>
#include <tpl_ca_rule.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

#ifndef ALEPH_REPRODUCTIONS_SOURCE_DIR
# define ALEPH_REPRODUCTIONS_SOURCE_DIR "reproductions"
#endif

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;

/** @brief Measure one callable with a steady clock.
 * @tparam F nullary callable type.
 * @param operation work to measure.
 * @return elapsed seconds.
 */
template <typename F>
double seconds_for(F &&operation)
{
  const auto start = std::chrono::steady_clock::now();
  operation();
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

/** @brief Count alive cells in a dense binary frame.
 * @param frame Game-of-Life frame.
 * @return number of non-zero cells.
 */
std::size_t population(const Grid &frame)
{
  std::size_t count = 0;
  for (ca_size_t row = 0; row < frame.size(0); ++row)
    for (ca_size_t column = 0; column < frame.size(1); ++column)
      if (frame.at({static_cast<ca_index_t>(row), static_cast<ca_index_t>(column)}) != 0)
        ++count;
  return count;
}

}  // namespace

int main()
{
  constexpr ca_size_t side = 384;
  constexpr std::size_t generations = 1000;
  constexpr std::int64_t offset = 32;

  Grid seed({side, side}, 0);
  for_each_gosper_gun_cell([&](const std::int64_t x, const std::int64_t y)
  {
    seed.set({offset + y, offset + x}, 1);
  });
  Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>
    dense(seed, make_game_of_life_rule());
  const double dense_seconds = seconds_for([&] { dense.run(generations); });

  Hashlife_Engine hashlife;
  for_each_gosper_gun_cell([&](const std::int64_t x, const std::int64_t y)
  {
    hashlife.set_alive(x, y);
  });
  std::uint64_t actual_generations = 0;
  const double hashlife_seconds
    = seconds_for([&] { actual_generations = hashlife.run(generations); });

  const std::filesystem::path root = ALEPH_REPRODUCTIONS_SOURCE_DIR;
  std::filesystem::create_directories(root / "results");
  std::ofstream summary(root / "results" / "game_of_life_bench.csv");
  if (not summary)
    {
      std::cerr << "Cannot write Game of Life benchmark\n";
      return 1;
    }
  summary << "engine,requested_generations,actual_generations,population,seconds\n"
          << "dense_step," << generations << ',' << generations << ','
          << population(dense.frame()) << ',' << std::setprecision(12) << dense_seconds << '\n'
          << "hashlife," << generations << ',' << actual_generations << ','
          << hashlife.population() << ',' << hashlife_seconds << '\n';

  std::cout << "Game of Life Gosper gun benchmark (informational only)\n"
            << "  dense step: " << std::fixed << std::setprecision(6)
            << dense_seconds << " s\n"
            << "  Hashlife:   " << hashlife_seconds << " s"
            << " (actual generations " << actual_generations << ")\n";
  if (actual_generations == generations)
    std::cout << "  population match: "
              << (population(dense.frame()) == hashlife.population() ? "yes" : "no")
              << '\n';
  return 0;
}
