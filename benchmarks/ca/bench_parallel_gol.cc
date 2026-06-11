/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_parallel_gol.cc
 * @brief Perf-gate anchor: parallel Game of Life, 2048x2048 toroidal, 100 steps.
 */

#include <cstddef>
#include <cstdint>
#include <random>
#include <thread>

#include <benchmarks/ca/bench_support.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
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
  constexpr ca_size_t side = 2048;
  constexpr std::size_t steps = 100;
  const Grid seed = make_seed(side, 0x9011A55E1CE9011ull);
  const double processed_cells
    = static_cast<double>(side) * static_cast<double>(side) * static_cast<double>(steps);

  Parallel_Engine_Config cfg;
  const unsigned hw = std::thread::hardware_concurrency();
  cfg.num_partitions = hw == 0 ? 4 : hw;
  cfg.min_parallel_cells = 0;

  const Bench_Result result = measure("parallel_gol_2048", processed_cells, resolve_repeats(3), [&]
  {
    Parallel_Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>
      engine(seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);
    engine.run(steps);
  });

  emit_json(result);
  return 0;
}