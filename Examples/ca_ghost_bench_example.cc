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
 * @file ca_ghost_bench_example.cc
 * @brief Microbench: GoL 1024x1024 on `Lattice` (Phase 3) vs `Ghost_Lattice` (Phase 4).
 *
 * Runs Conway's Game of Life on a `1024 x 1024` toroidal grid for a
 * fixed number of steps using the two lattice flavours defined by
 * the CA module, and prints the wall-clock time and steps-per-second
 * for each. Phase 4 targets a >= 30% speedup over Phase 3 — this
 * example is the reproducible rig to measure it.
 *
 * Usage:
 *   ./build/Examples/ca_ghost_bench_example [size] [steps]
 *
 * Defaults:  size = 1024,  steps = 50.
 *
 * The bench stamps a deterministic pseudo-random seed so both
 * engines start from the exact same state. After the runs it also
 * asserts that both engines produce the same final frame, which
 * doubles as a correctness smoke test.
 */

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <utility>

#include <ca-traits.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_ghost_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_engine.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  constexpr std::uint64_t SEED = 0xBEEFCAFE123456ULL;

  /// xorshift64* — tiny, deterministic, good enough to scatter bits.
  inline std::uint64_t next(std::uint64_t &s)
  {
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    return s * 0x2545F4914F6CDD1DULL;
  }

  template <typename L>
  void seed_lattice(L &lat, ca_size_t rows, ca_size_t cols)
  {
    std::uint64_t s = SEED;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(rows); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(cols); ++j)
        lat.set({ i, j }, (next(s) & 0xF) == 0 ? 1 : 0);  // ~6% alive
  }

  template <typename A, typename B>
  bool frames_match(const A &a, const B &b)
  {
    const ca_size_t rows = a.size(0);
    const ca_size_t cols = a.size(1);
    if (rows != b.size(0) or cols != b.size(1))
      return false;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(rows); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(cols); ++j)
        if (a.at({ i, j }) != b.at({ i, j }))
          return false;
    return true;
  }

  /// Counts alive cells in the user-visible region.
  template <typename L>
  std::size_t population(const L &lat)
  {
    std::size_t pop = 0;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(lat.size(0)); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(lat.size(1)); ++j)
        pop += static_cast<std::size_t>(lat.at({ i, j }) != 0);
    return pop;
  }

  struct Bench_Result
  {
    double wall_ms;
    double steps_per_sec;
    std::size_t final_population;
  };

  template <typename Engine>
  Bench_Result run_bench(Engine &engine, std::size_t steps)
  {
    const auto t0 = std::chrono::steady_clock::now();
    engine.run(steps);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms
      = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return { ms, 1000.0 * steps / ms, population(engine.frame()) };
  }
}

int main(int argc, char *argv[])
{
  ca_size_t size = 1024;
  std::size_t steps = 50;
  if (argc > 1)
    size = static_cast<ca_size_t>(std::atol(argv[1]));
  if (argc > 2)
    steps = static_cast<std::size_t>(std::atol(argv[2]));

  using Storage = Dense_Cell_Storage<int, 2>;
  using Boundary = ToroidalBoundary;
  using ClassicL = Lattice<Storage, Boundary>;
  using GhostL = Ghost_Lattice<Storage, Boundary, 1>;
  using ClassicEngine
    = Synchronous_Engine<ClassicL, Game_Of_Life_Rule, Moore<2, 1>>;
  using GhostEngine
    = Synchronous_Engine<GhostL, Game_Of_Life_Rule, Moore<2, 1>>;

  std::cout << "GoL " << size << "x" << size << " for " << steps
            << " steps (toroidal).\n";

  // Seed both lattices identically so the final frames must match.
  ClassicL classic_init({ size, size }, 0);
  GhostL ghost_init({ size, size }, 0);
  seed_lattice(classic_init, size, size);
  seed_lattice(ghost_init, size, size);

  ClassicEngine classic_engine(std::move(classic_init),
                               make_game_of_life_rule(),
                               Moore<2, 1>{});
  GhostEngine ghost_engine(std::move(ghost_init),
                           make_game_of_life_rule(),
                           Moore<2, 1>{});

  std::cout << std::fixed << std::setprecision(2);
  const auto classic_res = run_bench(classic_engine, steps);
  std::cout << "Phase 3 Lattice         : "
            << std::setw(9) << classic_res.wall_ms << " ms  ("
            << classic_res.steps_per_sec << " steps/s)  "
            << "population=" << classic_res.final_population << '\n';

  const auto ghost_res = run_bench(ghost_engine, steps);
  std::cout << "Phase 4 Ghost_Lattice   : "
            << std::setw(9) << ghost_res.wall_ms << " ms  ("
            << ghost_res.steps_per_sec << " steps/s)  "
            << "population=" << ghost_res.final_population << '\n';

  const double speedup
    = classic_res.wall_ms == 0.0 ? 0.0 : classic_res.wall_ms / ghost_res.wall_ms;
  std::cout << "Speedup                 : " << speedup << "x\n";

  if (not frames_match(classic_engine.frame(), ghost_engine.frame()))
    {
      std::cerr << "ERROR: final frames diverge — halo refresh is broken.\n";
      return 1;
    }
  std::cout << "Final frames bit-identical: OK\n";
  return 0;
}
