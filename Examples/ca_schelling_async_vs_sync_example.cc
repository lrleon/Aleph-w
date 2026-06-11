/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_schelling_async_vs_sync_example.cc
 * @brief Phase 13 example: how the update scheme reshapes Schelling
 *        segregation dynamics.
 *
 * Schelling's local CA segregation model is sensitive to the update
 * scheme: synchronous updates can produce oscillating ghosts (an
 * agent leaves a cell that another agent moves into in the same
 * step), while asynchronous updates evolve more like the original
 * non-local model and converge to stable clusters. This example
 * runs the same initial configuration under three schemes side by
 * side and prints a "segregation" metric (fraction of same-type
 * neighbours among occupied cells) so the difference is visible at
 * a glance.
 *
 * Run:
 *   ./ca_schelling_async_vs_sync_example [steps] [side] [seed]
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_stochastic_rules.H>
#include <tpl_ca_update_scheme.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
using Schelling = Schelling_Rule<>;

void seed_grid(Grid &g, std::uint32_t seed_value)
{
  std::mt19937 rng(seed_value);
  std::discrete_distribution<int> pick({0.20, 0.40, 0.40});
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            pick(rng));
}

/// Fraction of same-type neighbours among occupied cells (0..1).
double segregation_index(const Grid &g)
{
  double accum = 0.0;
  std::size_t agents = 0;
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      {
        const auto self = g.at({static_cast<ca_index_t>(i),
                                static_cast<ca_index_t>(j)});
        if (self == 0)
          continue;
        std::size_t same = 0;
        std::size_t other = 0;
        for (ca_index_t di = -1; di <= 1; ++di)
          for (ca_index_t dj = -1; dj <= 1; ++dj)
            {
              if (di == 0 and dj == 0)
                continue;
              const auto v = g.at_safe({static_cast<ca_index_t>(i) + di,
                                        static_cast<ca_index_t>(j) + dj});
              if (v == 0)
                continue;
              if (v == self)
                ++same;
              else
                ++other;
            }
        const std::size_t total = same + other;
        if (total > 0)
          {
            accum += static_cast<double>(same) / static_cast<double>(total);
            ++agents;
          }
      }
  return agents == 0 ? 0.0 : accum / static_cast<double>(agents);
}

/// Render a frame as ASCII glyphs.
void render(const Grid &g, std::ostream &os)
{
  for (ca_size_t i = 0; i < g.size(0); ++i)
    {
      for (ca_size_t j = 0; j < g.size(1); ++j)
        {
          const auto v = g.at({static_cast<ca_index_t>(i),
                               static_cast<ca_index_t>(j)});
          os << (v == 0 ? "  " : v == 1 ? "AA" : "BB");
        }
      os << '\n';
    }
}

}  // namespace

int main(int argc, char **argv)
{
  std::size_t steps = 30;
  ca_size_t side = 16;
  std::uint32_t seed_value = 0xBADC0DEu;
  if (argc >= 2) steps = static_cast<std::size_t>(std::stoul(argv[1]));
  if (argc >= 3) side = static_cast<ca_size_t>(std::stoul(argv[2]));
  if (argc >= 4) seed_value = static_cast<std::uint32_t>(std::stoul(argv[3]));

  Grid initial({side, side}, 0);
  seed_grid(initial, seed_value);

  std::cout << "Schelling segregation: same rule, three schemes\n";
  std::cout << "  side = " << side << ", steps = " << steps
            << ", seed = 0x" << std::hex << seed_value << std::dec << "\n\n";
  std::cout << "Initial frame (segregation index "
            << segregation_index(initial) << "):\n";
  render(initial, std::cout);
  std::cout << "\n";

  // --- Synchronous double-buffer ---------------------------------------
  Async_Engine<Grid, Schelling, Moore<2, 1>, Synchronous_Update<RowMajor>>
    sync(initial, Schelling{0.5, 1.0, 1.0, 1u}, Moore<2, 1>{});
  sync.run(steps);
  std::cout << "Synchronous_Update (double-buffer) — segregation "
            << segregation_index(sync.frame()) << ":\n";
  render(sync.frame(), std::cout);
  std::cout << "\n";

  // --- Sequential in-place ---------------------------------------------
  Async_Engine<Grid, Schelling, Moore<2, 1>, Sequential_Update<RowMajor>>
    seq(initial, Schelling{0.5, 1.0, 1.0, 2u}, Moore<2, 1>{});
  seq.run(steps);
  std::cout << "Sequential_Update (in-place) — segregation "
            << segregation_index(seq.frame()) << ":\n";
  render(seq.frame(), std::cout);
  std::cout << "\n";

  // --- Random asynchronous ---------------------------------------------
  Async_Engine<Grid, Schelling, Moore<2, 1>,
               Random_Asynchronous_Update<>>
    async(std::move(initial), Schelling{0.5, 1.0, 1.0, 3u}, Moore<2, 1>{},
          Random_Asynchronous_Update<>{0xC0FFEE,
                                       static_cast<std::size_t>(side * side)});
  async.run(steps);
  std::cout << "Random_Asynchronous_Update — segregation "
            << segregation_index(async.frame()) << ":\n";
  render(async.frame(), std::cout);
  std::cout << "\n";

  std::cout
    << "Compare segregation indices: typically the asynchronous schemes\n"
    << "converge more smoothly to higher segregation than synchronous\n"
    << "ones, because synchronous updates can swap occupants in lockstep\n"
    << "and oscillate.\n";
  return 0;
}