/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_critters_reversible_example.cc
 * @brief Phase 13 example: Toffoli & Margolus' Critters reversible CA.
 *
 * Critters is the canonical Margolus block-rule CA. The 2×2 partition
 * alternates origin between (0,0) and (1,1) on consecutive steps, and
 * the rule is its own inverse: applying it twice to the same block is
 * the identity. Combined with the alternating partition, the CA is
 * fully reversible.
 *
 * The example runs the rule N steps forward, prints the final frame,
 * runs it N steps backward, and verifies that the recovered frame is
 * bit-identical to the initial random soup.
 *
 * Run:
 *   ./ca_critters_reversible_example [steps] [side] [seed]
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include <ca-traits.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_block_rule.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_update_scheme.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

void seed(Grid &g, std::uint32_t seed_value, double p_alive)
{
  std::mt19937 rng(seed_value);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < p_alive ? 1 : 0);
}

void render(const Grid &g, std::ostream &os)
{
  for (ca_size_t i = 0; i < g.size(0); ++i)
    {
      for (ca_size_t j = 0; j < g.size(1); ++j)
        os << (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
                 ? "##"
                 : " .");
      os << '\n';
    }
}

bool grids_equal(const Grid &a, const Grid &b)
{
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

std::size_t alive(const Grid &g)
{
  std::size_t n = 0;
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      if (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0)
        ++n;
  return n;
}

}  // namespace

int main(int argc, char **argv)
{
  std::size_t steps = 1000;
  ca_size_t side = 32;
  std::uint32_t seed_value = 0xC0DEFEEDu;
  if (argc >= 2) steps = static_cast<std::size_t>(std::stoul(argv[1]));
  if (argc >= 3) side = static_cast<ca_size_t>(std::stoul(argv[2]));
  if (argc >= 4) seed_value = static_cast<std::uint32_t>(std::stoul(argv[3]));

  Grid initial({side, side}, 0);
  seed(initial, seed_value, 0.45);
  Grid baseline = initial;

  std::cout << "Critters reversible CA — " << side << "×" << side
            << ", " << steps << " steps, seed 0x" << std::hex << seed_value
            << std::dec << "\n\n";
  std::cout << "Initial frame (" << alive(initial) << " alive):\n";
  render(initial, std::cout);

  using Engine = Async_Engine<Grid, Critters_Rule, Null_Neighborhood<2>,
                              Margolus_Update>;
  Engine engine(std::move(initial), Critters_Rule{}, Null_Neighborhood<2>{});
  engine.run(steps);

  std::cout << "\nAfter forward(" << steps << ") steps ("
            << alive(engine.frame()) << " alive):\n";
  render(engine.frame(), std::cout);

  engine.run_back(steps);

  std::cout << "\nAfter backward(" << steps << ") steps ("
            << alive(engine.frame()) << " alive):\n";
  render(engine.frame(), std::cout);

  const bool ok = grids_equal(engine.frame(), baseline);
  std::cout << "\nReversibility check: "
            << (ok ? "PASS — bit-exact identity recovered"
                   : "FAIL — recovered frame differs from initial")
            << "\n";
  return ok ? 0 : 1;
}