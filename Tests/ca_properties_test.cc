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
 * @file ca_properties_test.cc
 * @brief Phase 26 — property-based tests for the cellular-automata core.
 *
 * Unlike the case-designed GoogleTests, these checks generate many random
 * initial grids and assert *invariants* that must hold for every input:
 *
 *  - `BBM_Rule` and `TM_Gas_Rule` under `Margolus_Update` conserve the
 *    whole-lattice particle count for any initial grid.
 *  - `Margolus_Update` with an involutive block rule is reversible:
 *    `forward(N)` followed by `backward(N)` restores the initial state
 *    bit-for-bit, for any initial grid.
 *  - `Synchronous_Engine` is deterministic: the same initial state yields
 *    the same trajectory.
 *  - `Lattice::at_safe(c)` is well-defined for any (possibly far
 *    out-of-range or negative) coordinate: it never reads out of bounds
 *    (validated under ASan/UBSan in CI) and agrees with the boundary
 *    policy.
 *
 * Each property sweeps a grid of `{seed} x {side}` random instances via the
 * `for_random_grids` helper.
 */

#include <cstdint>
#include <random>

#include <gtest/gtest.h>

#include <ca-traits.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_update_scheme.H>
#include <tpl_ca_block_rule.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

Grid make_random_grid(ca_size_t rows, ca_size_t cols,
                      double p_alive, std::uint32_t seed)
{
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  Grid g({rows, cols}, 0);
  for (ca_size_t i = 0; i < rows; ++i)
    for (ca_size_t j = 0; j < cols; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < p_alive ? 1 : 0);
  return g;
}

std::size_t alive_count(const Grid &g)
{
  std::size_t n = 0;
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      if (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0)
        ++n;
  return n;
}

bool grids_equal(const Grid &a, const Grid &b)
{
  if (a.size(0) != b.size(0) or a.size(1) != b.size(1))
    return false;
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

/// Sweep `body(seed, side)` over the cartesian product of a small set of
/// master seeds and grid sides. Keeps every property test reproducible
/// (fixed seeds) while still covering ~dozens of distinct random grids.
template <typename F>
void for_random_grids(F &&body)
{
  static const std::uint32_t seeds[] = {
      0x00000001u, 0xC0FFEEu,   0xDEADBEEFu, 0xFEEDFACEu,
      0xABCDEF01u, 0x13572468u, 0x9E3779B9u, 0x5BD1E995u};
  static const ca_size_t sides[] = {12, 16, 24};
  for (std::uint32_t s : seeds)
    for (ca_size_t side : sides)
      body(s, side);
}

/// Toroidal wrap of a single axis index into `[0, n)`.
ca_index_t wrap(ca_index_t v, ca_size_t n)
{
  const ca_index_t m = static_cast<ca_index_t>(n);
  ca_index_t r = v % m;
  if (r < 0)
    r += m;
  return r;
}

}  // namespace

// ===================================================================
// Property: particle-conserving block rules conserve population
// ===================================================================

TEST(CAProperties, BBMConservesParticleCountAcrossRandomGrids)
{
  int grids = 0;
  for_random_grids([&](std::uint32_t seed, ca_size_t side)
  {
    Grid g = make_random_grid(side, side, 0.20, seed);
    const auto initial = alive_count(g);
    Async_Engine<Grid, BBM_Rule, Null_Neighborhood<2>, Margolus_Update> eng(
        std::move(g), BBM_Rule{}, Null_Neighborhood<2>{});
    for (std::size_t t = 0; t < 60; ++t)
      {
        eng.step();
        ASSERT_EQ(alive_count(eng.frame()), initial)
            << "BBM lost particles (seed=" << seed << ", side=" << side
            << ", step=" << t << ")";
      }
    ++grids;
  });
  EXPECT_GE(grids, 20) << "property must sweep enough random grids";
}

TEST(CAProperties, TMGasConservesParticleCountAcrossRandomGrids)
{
  for_random_grids([&](std::uint32_t seed, ca_size_t side)
  {
    Grid g = make_random_grid(side, side, 0.20, seed);
    const auto initial = alive_count(g);
    Async_Engine<Grid, TM_Gas_Rule, Null_Neighborhood<2>, Margolus_Update> eng(
        std::move(g), TM_Gas_Rule{}, Null_Neighborhood<2>{});
    for (std::size_t t = 0; t < 60; ++t)
      {
        eng.step();
        ASSERT_EQ(alive_count(eng.frame()), initial)
            << "TM Gas lost particles (seed=" << seed << ", side=" << side
            << ", step=" << t << ")";
      }
  });
}

// ===================================================================
// Property: Margolus with an involutive rule is reversible
// ===================================================================

namespace
{

template <typename Block_Rule>
void check_reversible(Block_Rule rule, const char *label)
{
  for_random_grids([&](std::uint32_t seed, ca_size_t side)
  {
    // Margolus partitions act on even side lengths.
    const ca_size_t s = (side % 2 == 0) ? side : side + 1;
    Grid initial = make_random_grid(s, s, 0.45, seed);
    Grid baseline = initial;

    Async_Engine<Grid, Block_Rule, Null_Neighborhood<2>, Margolus_Update>
        eng(std::move(initial), rule, Null_Neighborhood<2>{}, Margolus_Update{});

    const std::size_t N = 300;
    ASSERT_NO_THROW(eng.run(N))
        << label << " forward threw (seed=" << seed << ")";
    ASSERT_NO_THROW(eng.run_back(N))
        << label << " backward threw (seed=" << seed << ")";

    ASSERT_TRUE(grids_equal(eng.frame(), baseline))
        << label << ": forward(" << N << ") then backward(" << N
        << ") must restore the initial state (seed=" << seed
        << ", side=" << s << ")";
    EXPECT_EQ(eng.steps_run(), 0u);
  });
}

}  // namespace

TEST(CAProperties, MargolusBBMIsReversibleAcrossRandomGrids)
{
  check_reversible(BBM_Rule{}, "BBM");
}

TEST(CAProperties, MargolusTMGasIsReversibleAcrossRandomGrids)
{
  check_reversible(TM_Gas_Rule{}, "TM Gas");
}

TEST(CAProperties, MargolusCrittersIsReversibleAcrossRandomGrids)
{
  check_reversible(Critters_Rule{}, "Critters");
}

// ===================================================================
// Property: the synchronous engine is deterministic
// ===================================================================

TEST(CAProperties, SynchronousEngineIsDeterministic)
{
  for_random_grids([&](std::uint32_t seed, ca_size_t side)
  {
    Grid initial = make_random_grid(side, side, 0.35, seed);

    Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>> a(
        initial, make_game_of_life_rule(), Moore<2, 1>{});
    Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>> b(
        initial, make_game_of_life_rule(), Moore<2, 1>{});

    for (std::size_t t = 0; t < 40; ++t)
      {
        ASSERT_TRUE(grids_equal(a.frame(), b.frame()))
            << "non-deterministic frame (seed=" << seed << ", step=" << t << ")";
        a.step();
        b.step();
      }
    EXPECT_TRUE(grids_equal(a.frame(), b.frame()));
  });
}

// ===================================================================
// Property: at_safe is well-defined for any coordinate
// ===================================================================

TEST(CAProperties, AtSafeIsWellDefinedForAnyCoordinate)
{
  for_random_grids([&](std::uint32_t seed, ca_size_t side)
  {
    Grid g = make_random_grid(side, side, 0.50, seed);
    std::mt19937 rng(seed ^ 0xA5A5A5A5u);
    // Range deliberately spans far past the grid bounds, including large
    // negative indices, to exercise every fold in the toroidal policy.
    std::uniform_int_distribution<int> coord(-4 * static_cast<int>(side),
                                             4 * static_cast<int>(side));
    for (int k = 0; k < 200; ++k)
      {
        const ca_index_t r = static_cast<ca_index_t>(coord(rng));
        const ca_index_t c = static_cast<ca_index_t>(coord(rng));
        const int v = g.at_safe({r, c});
        // Binary grid: the resolved value must stay in-domain (no UB / no
        // garbage read; ASan/UBSan in CI catches any OOB access here).
        ASSERT_TRUE(v == 0 or v == 1)
            << "at_safe returned out-of-domain value " << v
            << " at (" << r << ", " << c << ")";
        // Toroidal boundary: at_safe must agree with the wrapped in-range
        // read.
        const int folded = g.at({wrap(r, g.size(0)), wrap(c, g.size(1))});
        ASSERT_EQ(v, folded)
            << "at_safe disagrees with toroidal wrap at (" << r << ", " << c
            << ")";
      }
  });
}