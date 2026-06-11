/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_wolfram_30_1d.cc
 * @brief Perf-gate anchor: elementary Wolfram rule 30, 1M cells, 1000 steps.
 */

#include <cstddef>
#include <cstdint>

#include <benchmarks/ca/bench_support.H>
#include <ca-engine-utils.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Bench;

namespace
{

using Row = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;

}  // namespace

int main()
{
  constexpr ca_size_t width = 1'000'000;
  constexpr std::size_t steps = 1'000;
  constexpr std::uint8_t rule_no = 30;

  Row seed({width}, 0);
  seed.set({static_cast<ca_index_t>(width / 2)}, 1);
  const double processed_cells
    = static_cast<double>(width) * static_cast<double>(steps);

  const Bench_Result result = measure("wolfram_30_1d", processed_cells, resolve_repeats(3), [&]
  {
    Wolfram_1D_Engine engine = make_wolfram_engine(rule_no, seed);
    engine.run(steps);
  });

  emit_json(result);
  return 0;
}