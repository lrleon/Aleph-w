/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_forest_fire_example.cc
 * @brief Phase 8 example: Drossel–Schwabl forest-fire automaton.
 *
 * Builds a 32×32 toroidal forest-fire CA seeded with a few burning
 * cells and runs it for 60 steps with a fixed master seed. After each
 * step the example prints an ASCII frame plus a tiny tally
 * (#empty / #trees / #burning). The same `(seed, init)` pair must
 * always produce the same ASCII trace, regardless of the build mode.
 *
 *  Glyphs:  '.' empty  'T' tree  '#' burning
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>

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

using Forest_Lattice = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

void seed_initial_frame(Forest_Lattice &lat, std::uint32_t rng_seed)
{
  std::mt19937 rng(rng_seed);
  std::discrete_distribution<int> pick({0.40, 0.55, 0.05});
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      lat.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
              pick(rng));
}

char glyph_for(int v)
{
  switch (static_cast<Forest_Cell>(v))
    {
    case Forest_Cell::EMPTY:   return '.';
    case Forest_Cell::TREE:    return 'T';
    case Forest_Cell::BURNING: return '#';
    }
  return '?';
}

void render(const Forest_Lattice &lat, std::ostream &os)
{
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    {
      for (ca_size_t j = 0; j < lat.size(1); ++j)
        os << glyph_for(lat.at({static_cast<ca_index_t>(i),
                                static_cast<ca_index_t>(j)}));
      os << '\n';
    }
}

struct Tally
{
  std::size_t empty = 0;
  std::size_t tree = 0;
  std::size_t burning = 0;
};

Tally count(const Forest_Lattice &lat)
{
  Tally t;
  for (ca_size_t i = 0; i < lat.size(0); ++i)
    for (ca_size_t j = 0; j < lat.size(1); ++j)
      {
        const int v = lat.at({static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j)});
        if (v == static_cast<int>(Forest_Cell::EMPTY))   ++t.empty;
        else if (v == static_cast<int>(Forest_Cell::TREE)) ++t.tree;
        else ++t.burning;
      }
  return t;
}

}  // namespace

int main()
{
  constexpr ca_size_t rows = 32;
  constexpr ca_size_t cols = 32;
  constexpr std::uint64_t master_seed = 0xC0FFEE0042Aull;
  constexpr std::size_t total_steps = 60;
  constexpr std::size_t snapshot_every = 10;

  std::cout << "Forest-fire reference example\n";
  std::cout << "  size=" << rows << "x" << cols
            << " master_seed=0x" << std::hex << master_seed << std::dec
            << " p_growth=0.02 p_lightning=0.001\n";

  Forest_Lattice lat({rows, cols});
  seed_initial_frame(lat, /*rng_seed=*/0xACE0u);
  // Sprinkle two extra burning cells so the dynamics are immediately
  // interesting.
  lat.set({rows / 2, cols / 2}, static_cast<int>(Forest_Cell::BURNING));
  lat.set({1, 1}, static_cast<int>(Forest_Cell::BURNING));

  Forest_Fire_Rule<> rule(/*p_growth=*/0.02, /*p_lightning=*/0.001,
                          master_seed);
  Synchronous_Engine<Forest_Lattice, Forest_Fire_Rule<>, Moore<2, 1>>
    engine(lat, rule);

  std::cout << "\n=== step 0 (initial) ===\n";
  render(engine.frame(), std::cout);
  Tally t = count(engine.frame());
  std::cout << "tally: empty=" << t.empty << " trees=" << t.tree
            << " burning=" << t.burning << '\n';

  for (std::size_t step = 1; step <= total_steps; ++step)
    {
      engine.step();
      if (step % snapshot_every == 0 or step == total_steps)
        {
          std::cout << "\n=== step " << step << " ===\n";
          render(engine.frame(), std::cout);
          t = count(engine.frame());
          std::cout << "tally: empty=" << t.empty << " trees=" << t.tree
                    << " burning=" << t.burning << '\n';
        }
    }

  std::cout << "\nDone after " << total_steps << " steps.\n";
  return 0;
}
