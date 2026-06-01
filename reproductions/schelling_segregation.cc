/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file schelling_segregation.cc
 * @brief Quantitative local Schelling segregation reproduction.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

#include <reproductions/ca_reproduction_support.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Reproductions;

#ifndef ALEPH_REPRODUCTIONS_SOURCE_DIR
# define ALEPH_REPRODUCTIONS_SOURCE_DIR "reproductions"
#endif

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

/** @brief Fill a Schelling grid with 10% vacancies and balanced populations.
 * @param grid output frame.
 * @param seed standard-engine seed for the initial frame.
 */
void seed_population(Grid &grid, const std::uint32_t seed)
{
  std::mt19937 rng(seed);
  std::discrete_distribution<int> pick({0.10, 0.45, 0.45});
  for (ca_size_t r = 0; r < grid.size(0); ++r)
    for (ca_size_t c = 0; c < grid.size(1); ++c)
      grid.set({static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}, pick(rng));
}

}  // namespace

int main()
{
  constexpr ca_size_t side = 100;
  constexpr std::size_t steps = 200;
  constexpr double threshold = 0.5;
  constexpr std::uint64_t master_seed = 0x5C4E111971ull;

  Grid frame({side, side});
  seed_population(frame, 0xACE0u);
  const double initial = morans_i_binary(
    frame,
    static_cast<int>(Schelling_Cell::EMPTY),
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_B));

  Schelling_Rule<> rule(threshold, /*p_move=*/0.7, /*p_fill=*/0.7, master_seed);
  Synchronous_Engine<Grid, Schelling_Rule<>, Moore<2, 1>> engine(std::move(frame), rule);
  engine.run(steps);

  const double final = morans_i_binary(
    engine.frame(),
    static_cast<int>(Schelling_Cell::EMPTY),
    static_cast<int>(Schelling_Cell::TYPE_A),
    static_cast<int>(Schelling_Cell::TYPE_B));
  const std::filesystem::path root = ALEPH_REPRODUCTIONS_SOURCE_DIR;
  std::filesystem::create_directories(root / "results");
  std::ofstream summary(root / "results" / "schelling_summary.csv");
  if (not summary)
    {
      std::cerr << "Cannot write Schelling summary\n";
      return 1;
    }
  summary << "side,steps,threshold,initial_morans_i,final_morans_i\n"
          << side << ',' << steps << ',' << threshold << ','
          << std::setprecision(12) << initial << ',' << final << '\n';

  std::cout << "Schelling segregation: initial Moran's I=" << std::fixed
            << std::setprecision(4) << initial << " final=" << final << '\n';
  if (final <= 0.7)
    {
      std::cerr << "Schelling final Moran's I must be greater than 0.7\n";
      return 2;
    }
  return 0;
}
