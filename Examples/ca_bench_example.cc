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
 * @file ca_bench_example.cc
 * @brief Phase-5 microbenchmark: parallel vs sequential GoL throughput.
 *
 * Sweeps a grid of `(N, threads)` configurations, runs Conway's Game
 * of Life with toroidal borders for a fixed number of steps, and
 * prints a table with wall-clock time, throughput and the speedup vs
 * the single-thread baseline.
 *
 * The example asserts that the parallel and sequential engines
 * produce bit-identical final frames for every configuration. This
 * doubles as a sanity check: any divergence is a bug to fix before
 * looking at the timings.
 */

#include <array>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include <thread_pool.H>

#include <ca-bench.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

using Lat_t = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

namespace {

Lat_t make_random_seed(std::size_t n, std::uint32_t seed)
{
  Lat_t lat({n, n}, 0);
  std::mt19937 rng(seed);
  std::bernoulli_distribution flip(0.4);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j)
      lat.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              flip(rng) ? 1 : 0);
  return lat;
}

bool frames_equal(const Lat_t &a, const Lat_t &b)
{
  if (a.extents() != b.extents())
    return false;
  for (std::size_t i = 0; i < a.size(0); ++i)
    for (std::size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

double run_sequential(const Lat_t &seed, std::size_t steps)
{
  Synchronous_Engine<Lat_t, Game_Of_Life_Rule, Moore<2, 1>> eng(
    seed, make_game_of_life_rule(), Moore<2, 1>{});
  return bench_seconds([&] { eng.run(steps); });
}

double run_parallel(const Lat_t &seed,
                    std::size_t steps,
                    std::size_t threads,
                    Lat_t &out_frame)
{
  ThreadPool pool(threads);
  Parallel_Engine_Config cfg;
  cfg.pool = &pool;
  cfg.num_partitions = threads;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<Lat_t, Game_Of_Life_Rule, Moore<2, 1>> eng(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  const double secs = bench_seconds([&] { eng.run(steps); });
  out_frame = eng.frame();
  return secs;
}

}  // namespace

int main()
{
  std::printf("Aleph::CA Phase-5 microbench (Game of Life, toroidal)\n");
  std::printf("------------------------------------------------------\n");
  std::printf("hardware_concurrency = %u\n",
              static_cast<unsigned>(std::thread::hardware_concurrency()));

  // Modest defaults so the bench finishes quickly when run as part of
  // the example suite. Bump these for serious profiling work.
  const std::array<std::size_t, 3> sizes{256, 512, 1024};
  const std::array<std::size_t, 5> thread_counts{1, 2, 4, 8, 16};
  const std::size_t steps = 100;

  std::printf(
    "\n%-7s %-9s %-9s %-18s %-9s\n", "N", "threads", "wall (s)", "throughput", "speedup");
  std::printf("%-7s %-9s %-9s %-18s %-9s\n", "-------", "-------", "--------",
              "-----------------", "-------");

  for (std::size_t N : sizes)
    {
      const Lat_t seed = make_random_seed(N, /*seed=*/0xCAFEu ^ N);

      // Compute a reference sequential frame for validation and a
      // baseline time for speedup calculations.
      Synchronous_Engine<Lat_t, Game_Of_Life_Rule, Moore<2, 1>> ref(
        seed, make_game_of_life_rule(), Moore<2, 1>{});
      const double seq_seconds = bench_seconds([&] { ref.run(steps); });
      const double seq_throughput
        = static_cast<double>(N) * N * steps / seq_seconds;

      std::printf("%-7zu %-9s %-9.4f %-18s %-9s\n",
                  N,
                  "seq",
                  seq_seconds,
                  format_throughput(seq_throughput),
                  "1.00x");

      for (std::size_t T : thread_counts)
        {
          Lat_t par_frame;
          const double par_seconds = run_parallel(seed, steps, T, par_frame);

          if (not frames_equal(ref.frame(), par_frame))
            {
              std::printf("    *** divergence at N=%zu T=%zu ***\n", N, T);
              return 1;
            }

          const double throughput
            = static_cast<double>(N) * N * steps / par_seconds;
          const double speedup = seq_seconds / par_seconds;
          std::printf("%-7zu %-9zu %-9.4f %-18s %4.2fx\n",
                      N,
                      T,
                      par_seconds,
                      format_throughput(throughput),
                      speedup);
        }
      std::printf("\n");
    }

  // Touch the lazy default pool to make sure later examples don't pay
  // its construction cost; sanity check only.
  (void)Aleph::default_pool();

  // Throwaway sequential baseline to satisfy the seq_seconds reference
  // path used above (kept as a hint that a single sequential reference
  // is enough to validate every parallel configuration).
  (void)run_sequential;

  std::printf("Microbench finished. All parallel runs matched the sequential frame.\n");
  return 0;
}