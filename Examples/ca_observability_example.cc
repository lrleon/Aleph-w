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
 * @file ca_observability_example.cc
 * @brief Phase-7 illustration: every observer running together.
 *
 * Drives a 64x64 toroidal Game of Life seeded with a random pattern
 * for 200 steps while a `Composite_Observer` collects density,
 * activity, entropy, frame snapshots and short-cycle detection.
 *
 * Then re-runs a small blinker for 8 steps to showcase the same
 * pipeline producing the canonical Phase-7 numbers (activity = 4,
 * cycle length = 2).
 */

#include <cstdint>
#include <cstdio>
#include <random>

#include <ca-engine-utils.H>
#include <ca-metrics.H>
#include <ca-observer.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace {

using Lat = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
using Engine = Synchronous_Engine<Lat, Game_Of_Life_Rule, Moore<2, 1>>;

Lat make_random_seed(std::size_t n, std::uint32_t seed, double density)
{
  Lat lat({n, n}, 0);
  std::mt19937 rng(seed);
  std::bernoulli_distribution flip(density);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j)
      lat.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              flip(rng) ? 1 : 0);
  return lat;
}

}  // namespace

int main()
{
  std::printf("Aleph::CA Phase-7 example: full observability pipeline\n");
  std::printf("------------------------------------------------------\n");

  // ---- Random GoL run ---------------------------------------------------
  Lat random_seed = make_random_seed(64, /*seed=*/0xCAFEu, /*density=*/0.4);

  Density_Observer<int> dens(1);
  Activity_Observer<Lat> act;
  Entropy_Observer<2> ent;
  Stationary_Detector det(64);
  Sampling_Observer<Lat> samp(50);

  Composite_Observer<Density_Observer<int> &, Activity_Observer<Lat> &,
                     Entropy_Observer<2> &, Stationary_Detector &,
                     Sampling_Observer<Lat> &>
    comp(dens, act, ent, det, samp);

  Engine eng(random_seed, make_game_of_life_rule(), Moore<2, 1>{});
  attach_observer(eng, comp);
  eng.run(200);

  std::printf("\nRandom GoL 64x64, 200 steps\n");
  std::printf("  density samples       : %zu\n", dens.size());
  std::printf("  activity samples      : %zu\n", act.size());
  std::printf("  entropy samples       : %zu\n", ent.size());
  std::printf("  snapshot count        : %zu\n", samp.size());
  std::printf("  density (initial)     : %.4f\n", dens.density_at(0));
  std::printf("  density (final)       : %.4f\n",
              dens.density_at(dens.size() - 1));

  double avg_density = 0.0;
  for (std::size_t i = dens.size() - 50; i < dens.size(); ++i)
    avg_density += dens.density_at(i);
  avg_density /= 50.0;
  std::printf("  density (last-50 avg) : %.4f\n", avg_density);

  if (act.size() > 0)
    {
      double avg_act = 0.0;
      for (auto a : act.activity())
        avg_act += static_cast<double>(a);
      avg_act /= static_cast<double>(act.size());
      std::printf("  activity (avg)        : %.2f cells/step\n", avg_act);
    }
  if (ent.size() > 0)
    std::printf("  entropy (final)       : %.4f nats\n",
                ent.entropy().back());
  if (det.cycle_detected())
    std::printf("  short cycle detected  : length=%zu starting at step=%zu\n",
                *det.cycle_length(), *det.cycle_start());
  else
    std::printf("  short cycle detected  : none within window=%zu\n",
                det.buffered_hashes());

  // ---- Blinker reference run -------------------------------------------
  Lat blinker({5, 5}, 0);
  blinker.set({2, 1}, 1);
  blinker.set({2, 2}, 1);
  blinker.set({2, 3}, 1);

  Activity_Observer<Lat> b_act;
  Stationary_Detector b_det(8);
  Composite_Observer<Activity_Observer<Lat> &, Stationary_Detector &> b_comp(
    b_act, b_det);

  Engine b_eng(blinker, make_game_of_life_rule(), Moore<2, 1>{});
  attach_observer(b_eng, b_comp);
  b_eng.run(8);

  std::printf("\nBlinker reference run (8 steps)\n");
  std::printf("  activity per step :");
  for (auto a : b_act.activity())
    std::printf(" %zu", a);
  std::putchar('\n');
  std::printf("  cycle length      : %zu\n",
              b_det.cycle_detected() ? *b_det.cycle_length() : 0);

  return 0;
}