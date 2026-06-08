/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_gol_1024.cc
 * @brief Perf-gate anchor: dense Game of Life, 1024x1024 toroidal, 100 steps.
 */

#include <cstddef>
#include <cstdint>
#include <random>

#include <benchmarks/ca/bench_support.H>
#include <ca-engine-utils.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Bench;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

/** @brief Build a deterministic ~30%-alive square frame.
 * @param side grid side length.
 * @param seed PRNG seed for reproducible occupancy.
 * @return the seeded toroidal frame.
 */
Grid make_seed(const ca_size_t side, const std::uint64_t seed)
{
  Grid frame({side, side}, 0);
  std::mt19937_64 rng(seed);
  std::bernoulli_distribution alive(0.3);
  for (ca_size_t r = 0; r < side; ++r)
    for (ca_size_t c = 0; c < side; ++c)
      if (alive(rng))
        frame.set({static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}, 1);
  return frame;
}

}  // namespace

int main()
{
  constexpr ca_size_t side = 1024;
  constexpr std::size_t steps = 100;
  const Grid seed = make_seed(side, 0xB1772A11FE1234ull);
  const double processed_cells
    = static_cast<double>(side) * static_cast<double>(side) * static_cast<double>(steps);

  const Bench_Result result = measure("gol_1024", processed_cells, resolve_repeats(3), [&]
  {
    Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>
      engine(seed, make_game_of_life_rule(), Moore<2, 1>{});
    engine.run(steps);
  });

  emit_json(result);
  return 0;
}