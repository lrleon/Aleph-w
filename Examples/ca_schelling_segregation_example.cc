/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_schelling_segregation_example.cc
 * @brief Phase 8 example: local Schelling segregation on a 2-D lattice.
 *
 * Builds a 48x48 toroidal lattice seeded with two agent types (A, B)
 * and a small fraction of empty cells, and runs the local Schelling
 * approximation:
 *
 *   - An occupied cell looks at its Moore-radius-1 occupied neighbours.
 *     If the same-type fraction is below `threshold`, the cell vacates
 *     with probability `p_move`.
 *   - An empty cell with a strict majority of one type fills with that
 *     type with probability `p_fill`.
 *
 * The example sweeps three intolerance thresholds (0.30, 0.50, 0.70).
 * For each one we report a cluster-quality proxy — the fraction of
 * same-type pairs among occupied edges of the lattice graph — at the
 * initial frame and at the end of the simulation. The visualisation
 * renders the final configuration with three glyphs.
 *
 *  Glyphs:  '.' empty  'A' type A  'B' type B
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include <ca-rng.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Schelling_Lattice
  = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

void seed_population(Schelling_Lattice &lat, std::uint32_t rng_seed)
{
  std::mt19937 rng(rng_seed);
  std::discrete_distribution<int> pick({0.10, 0.45, 0.45});  // empty, A, B
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      lat.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              pick(rng));
}

char glyph_for(int v)
{
  switch (static_cast<Schelling_Cell>(v))
    {
    case Schelling_Cell::EMPTY:  return '.';
    case Schelling_Cell::TYPE_A: return 'A';
    case Schelling_Cell::TYPE_B: return 'B';
    }
  return '?';
}

void render(const Schelling_Lattice &lat, std::ostream &os)
{
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    {
      for (ca_size_t j = 0; j < lat.size(1); ++j)
        os << glyph_for(lat.at({static_cast<ca_index_t>(i),
                                static_cast<ca_index_t>(j)}));
      os << '\n';
    }
}

/// Return the fraction of same-type horizontal+vertical edges among
/// edges connecting two occupied cells. 1.0 = perfect segregation,
/// 0.5 = perfectly mixed.
double same_type_edge_fraction(const Schelling_Lattice &lat)
{
  std::size_t same = 0;
  std::size_t total = 0;
  const ca_size_t n0 = lat.size(0);
  const ca_size_t n1 = lat.size(1);
  for (ca_size_t i = 0; i < n0; ++i)
    for (ca_size_t j = 0; j < n1; ++j)
      {
        const int c = lat.at({static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j)});
        if (c == static_cast<int>(Schelling_Cell::EMPTY))
          continue;

        const int right
          = lat.at({static_cast<ca_index_t>(i),
                    static_cast<ca_index_t>((j + 1) % n1)});
        const int down
          = lat.at({static_cast<ca_index_t>((i + 1) % n0),
                    static_cast<ca_index_t>(j)});

        if (right != static_cast<int>(Schelling_Cell::EMPTY))
          {
            ++total;
            if (right == c) ++same;
          }
        if (down != static_cast<int>(Schelling_Cell::EMPTY))
          {
            ++total;
            if (down == c) ++same;
          }
      }
  return total == 0 ? 0.0
                    : static_cast<double>(same) / static_cast<double>(total);
}

std::string bar(double v, std::size_t width = 40)
{
  const std::size_t k
    = std::min(width, static_cast<std::size_t>(v * static_cast<double>(width)));
  return std::string(k, '|') + std::string(width - k, ' ');
}

void run_scenario(const Schelling_Lattice &seed,
                  double threshold,
                  std::uint64_t master_seed,
                  std::size_t total_steps)
{
  std::cout << "\n========================================\n";
  std::cout << "  Schelling threshold = "
            << std::fixed << std::setprecision(2) << threshold
            << std::defaultfloat
            << "  steps = " << total_steps
            << "  seed = 0x" << std::hex << master_seed << std::dec << '\n';
  std::cout << "========================================\n";

  Schelling_Rule<> rule(threshold, /*p_move=*/0.7, /*p_fill=*/0.7,
                        master_seed);
  Synchronous_Engine<Schelling_Lattice, Schelling_Rule<>, Moore<2, 1>>
    engine(seed, rule);

  const double q0 = same_type_edge_fraction(engine.frame());
  std::cout << "  same-type edge fraction (initial)  = "
            << std::fixed << std::setprecision(3) << q0 << std::defaultfloat
            << "  [" << bar(q0) << "]\n";

  engine.run(total_steps);

  const double q1 = same_type_edge_fraction(engine.frame());
  std::cout << "  same-type edge fraction (final)    = "
            << std::fixed << std::setprecision(3) << q1 << std::defaultfloat
            << "  [" << bar(q1) << "]\n";

  std::cout << "\nFinal configuration:\n";
  render(engine.frame(), std::cout);
}

}  // namespace

int main()
{
  constexpr ca_size_t rows = 48;
  constexpr ca_size_t cols = 48;
  constexpr std::uint64_t master_seed = 0x5C4E11Aull;
  constexpr std::size_t total_steps = 80;

  std::cout << "Schelling-style local segregation on a 48x48 toroidal lattice\n";
  std::cout << "  Moore radius 1, two types of agents + 10% vacancy\n";
  std::cout << "  same-type edge fraction is a cluster-quality proxy:\n";
  std::cout << "  ~0.50 = mixed   ~1.00 = perfectly segregated\n";

  Schelling_Lattice seed({rows, cols});
  seed_population(seed, /*rng_seed=*/0xACE0u);

  std::cout << "\nInitial configuration (shared by every scenario):\n";
  render(seed, std::cout);
  const double q_seed = same_type_edge_fraction(seed);
  std::cout << "same-type edge fraction = "
            << std::fixed << std::setprecision(3) << q_seed << std::defaultfloat
            << "  [" << bar(q_seed) << "]\n";

  for (const double threshold : {0.30, 0.50, 0.70})
    run_scenario(seed, threshold, master_seed, total_steps);

  std::cout << "\nNote: master_seed and the initial frame are shared across\n"
               "      threshold scenarios, so the trace is reproducible\n"
               "      bit-for-bit on any thread count.\n";
  return 0;
}