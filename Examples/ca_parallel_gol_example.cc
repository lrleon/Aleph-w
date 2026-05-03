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
 * @file ca_parallel_gol_example.cc
 * @brief Phase-5 illustration: a glider gun on a parallel GoL engine.
 *
 * Sets up a 60×80 toroidal Game of Life with Gosper's glider gun and
 * runs it for several hundred steps using the parallel engine. Every
 * 30 steps it prints an ASCII snapshot of the grid plus the live cell
 * count; this makes the per-step progress visible and shows how a
 * canonical pattern evolves under the same multithreaded engine that
 * powers larger HPC simulations.
 *
 * The example is also a smoke test: at the end it asserts that the
 * parallel and sequential engines have produced bit-identical frames.
 */

#include <cstdio>
#include <cstdlib>

#include <ca-engine-utils.H>
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

void stamp_glider_gun(Lat_t &lat, ca_index_t row0, ca_index_t col0)
{
  // Coordinates of every alive cell in Gosper's classic 36-cell gun,
  // relative to the top-left of the bounding box.
  static constexpr int gun[][2] = {
    {0, 24}, {1, 22}, {1, 24}, {2, 12}, {2, 13}, {2, 20}, {2, 21}, {2, 34},
    {2, 35}, {3, 11}, {3, 15}, {3, 20}, {3, 21}, {3, 34}, {3, 35}, {4, 0},
    {4, 1},  {4, 10}, {4, 16}, {4, 20}, {4, 21}, {5, 0},  {5, 1},  {5, 10},
    {5, 14}, {5, 16}, {5, 17}, {5, 22}, {5, 24}, {6, 10}, {6, 16}, {6, 24},
    {7, 11}, {7, 15}, {8, 12}, {8, 13},
  };
  for (const auto &p : gun)
    lat.set({row0 + p[0], col0 + p[1]}, 1);
}

void print_frame(const Lat_t &lat, std::size_t step, bool sparse_only = false)
{
  std::size_t alive = 0;
  for (std::size_t i = 0; i < lat.size(0); ++i)
    for (std::size_t j = 0; j < lat.size(1); ++j)
      if (lat.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0)
        ++alive;

  std::printf("\nStep %4zu  alive=%zu\n", step, alive);
  if (sparse_only)
    return;

  for (std::size_t i = 0; i < lat.size(0); ++i)
    {
      for (std::size_t j = 0; j < lat.size(1); ++j)
        std::putchar(
          lat.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0
            ? '#'
            : '.');
      std::putchar('\n');
    }
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

}  // namespace

int main()
{
  constexpr std::size_t rows = 60;
  constexpr std::size_t cols = 80;
  constexpr std::size_t steps = 240;

  Lat_t seed({rows, cols}, 0);
  stamp_glider_gun(seed, /*row0=*/2, /*col0=*/2);

  std::printf("Aleph::CA Phase-5 example: parallel Game of Life with Gosper's gun\n");
  std::printf("Grid %zux%zu, toroidal, %zu steps, hardware_concurrency=%u\n",
              rows,
              cols,
              steps,
              static_cast<unsigned>(std::thread::hardware_concurrency()));

  // Parallel engine: use the global default pool, four partitions.
  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<Lat_t, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  // Snapshot every 30 steps for visual inspection. We print the very
  // first frame, then a dense view every quarter of the run, and
  // sparse summaries for the rest so the terminal is not overwhelmed.
  print_frame(par.frame(), 0);
  for (std::size_t s = 30; s <= steps; s += 30)
    {
      par.run(30);
      const bool dense = (s == steps) or (s == steps / 2);
      print_frame(par.frame(), s, /*sparse_only=*/not dense);
    }

  // Cross-check the parallel run against a sequential baseline.
  Synchronous_Engine<Lat_t, Game_Of_Life_Rule, Moore<2, 1>> seq(
    seed, make_game_of_life_rule(), Moore<2, 1>{});
  seq.run(steps);

  if (not frames_equal(par.frame(), seq.frame()))
    {
      std::printf("\n*** parallel and sequential frames diverged ***\n");
      return EXIT_FAILURE;
    }
  std::printf("\nParallel run matched the sequential reference frame "
              "(%zu cells / step).\n",
              static_cast<std::size_t>(rows) * cols);
  return EXIT_SUCCESS;
}