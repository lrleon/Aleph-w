/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_gray_scott_512.cc
 * @brief Perf-gate anchor: Gray-Scott reaction-diffusion, 512x512, 1000 steps.
 */

#include <cstddef>
#include <cstdint>

#include <benchmarks/ca/bench_support.H>
#include <tpl_ca_continuous_rules.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Bench;

namespace
{

using Cell = Reaction_Diffusion_Cell<double>;
using Grid = Lattice<Dense_Cell_Storage<Cell, 2>, NeumannBoundary>;

/** @brief Build the canonical centre-patch Gray-Scott perturbation.
 * @param side square lattice side length.
 * @return the seeded frame with a clean background and a perturbed core.
 */
Grid make_seed(const ca_size_t side)
{
  Grid frame({side, side}, Cell{1.0, 0.0});
  const ca_size_t patch = side / 5;
  const ca_size_t begin = side / 2 - patch / 2;
  for (ca_size_t r = begin; r < begin + patch; ++r)
    for (ca_size_t c = begin; c < begin + patch; ++c)
      frame.set({static_cast<ca_index_t>(r), static_cast<ca_index_t>(c)}, Cell{0.5, 0.25});
  return frame;
}

}  // namespace

int main()
{
  constexpr ca_size_t side = 512;
  constexpr std::size_t steps = 1'000;
  const Grid seed = make_seed(side);
  const double processed_cells
    = static_cast<double>(side) * static_cast<double>(side) * static_cast<double>(steps);

  const Bench_Result result = measure("gray_scott_512", processed_cells, resolve_repeats(3), [&]
  {
    Gray_Scott_Rule<double> rule(0.0350, 0.0650, 0.16, 0.08);
    Synchronous_Engine<Grid, Gray_Scott_Rule<double>, Moore<2, 1>> engine(seed, rule);
    engine.run(steps);
  });

  emit_json(result);
  return 0;
}