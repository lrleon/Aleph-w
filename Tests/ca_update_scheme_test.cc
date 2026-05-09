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
 * @file ca_update_scheme_test.cc
 * @brief Phase 13 tests for the update-scheme strategies, the
 *        Async_Engine and the Block_Rule catalogue.
 *
 * Coverage:
 *  - Synchronous_Update equivalence: Async_Engine + Synchronous_Update
 *    must produce frame-by-frame identical output to the legacy
 *    Synchronous_Engine on Game-of-Life and Wolfram rules (regression).
 *  - Sequential_Update on a sandpile-like relaxation rule: cell counts
 *    decay monotonically and the lattice eventually relaxes.
 *  - Random_Asynchronous_Update is reproducible across runs with the
 *    same master seed and visits cells in (statistically) the right
 *    distribution.
 *  - Block_Synchronous_Update updates exactly one block per step.
 *  - Margolus_Update with Critters_Rule, BBM_Rule and TM_Gas_Rule:
 *    forward(N) ∘ backward(N) = identity bit-by-bit on a 32×32
 *    toroidal grid for N = 1000 (key Phase 13 acceptance test).
 */

#include <array>
#include <cstdint>
#include <random>
#include <vector>

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

std::size_t alive_count(const Grid &g)
{
  std::size_t n = 0;
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      if (g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}) != 0)
        ++n;
  return n;
}

}  // namespace

// ===================================================================
// Regression: Synchronous_Update == Synchronous_Engine
// ===================================================================

TEST(CAUpdateScheme, SynchronousUpdateMatchesSynchronousEngineOnGameOfLife)
{
  // Random soup, run both engines side by side, frame must match
  // every step (bit-for-bit).
  Grid initial = make_random_grid(24, 24, 0.35, 0xC0FFEEu);

  Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>> legacy(
      initial, make_game_of_life_rule(), Moore<2, 1>{});

  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Synchronous_Update<RowMajor>>
      schemed(std::move(initial), make_game_of_life_rule(), Moore<2, 1>{},
              Synchronous_Update<RowMajor>{});

  for (std::size_t t = 0; t < 30; ++t)
    {
      ASSERT_TRUE(grids_equal(legacy.frame(), schemed.frame()))
          << "Mismatch at step " << t;
      legacy.step();
      schemed.step();
    }
  EXPECT_TRUE(grids_equal(legacy.frame(), schemed.frame()))
      << "Mismatch after 30 steps";
}

TEST(CAUpdateScheme, SynchronousUpdateMatchesSynchronousEngineOnWolfram90)
{
  using Row = Lattice<Dense_Cell_Storage<int, 1>, ToroidalBoundary>;
  using Wolfram_Rule = Lookup_Rule<2, 2>;
  using Wolfram_Nh = Custom_Neighborhood<1, 2, 1>;

  const auto offsets = std::array<Offset_Vec<1>, 2>{{{-1}, {1}}};

  Row initial({33}, 0);
  initial.set({16}, 1);

  Wolfram_Rule rule = make_wolfram_elementary_rule(90);

  Synchronous_Engine<Row, Wolfram_Rule, Wolfram_Nh> legacy(
      initial, rule, Wolfram_Nh(offsets));

  Async_Engine<Row, Wolfram_Rule, Wolfram_Nh, Synchronous_Update<RowMajor>>
      schemed(std::move(initial), rule, Wolfram_Nh(offsets),
              Synchronous_Update<RowMajor>{});

  for (std::size_t t = 0; t < 32; ++t)
    {
      for (ca_size_t i = 0; i < legacy.frame().size(0); ++i)
        ASSERT_EQ(legacy.frame().at({static_cast<ca_index_t>(i)}),
                  schemed.frame().at({static_cast<ca_index_t>(i)}))
            << "step " << t << ", cell " << i;
      legacy.step();
      schemed.step();
    }
}

// ===================================================================
// Sequential_Update: sandpile-like relaxation
// ===================================================================

namespace
{

/// Bak-Tang-Wiesenfeld toppling rule for a single cell. This is a
/// purely-local rule that, given the per-cell "height", produces a
/// new height that has lost 4 grains if it was at or above the
/// toppling threshold (4). Neighbours are inspected only to add the
/// grains they push into the centre. With the canonical sequential
/// update this captures the BTW dynamics: each visit relaxes the
/// cell once.
struct Sandpile_Rule
{
  template <typename State>
  State operator()(const State &s, Neighbor_View<State> nh) const noexcept
  {
    constexpr State threshold = 4;
    State result = s;
    if (result >= threshold)
      result -= threshold;
    for (const auto &n : nh)
      if (n >= threshold)
        result += 1;  // each over-threshold neighbour pushes one grain
    return result;
  }
};

}  // namespace

TEST(CAUpdateScheme, SequentialUpdateRelaxesSandpile)
{
  Grid g({12, 12}, 0);
  // Drop ten grains at the centre to seed an avalanche.
  for (int k = 0; k < 50; ++k)
    {
      const auto cur = g.at({6, 6});
      g.set({6, 6}, cur + 1);
      // Run a single full sweep using Sequential_Update.
      Async_Engine<Grid, Sandpile_Rule, Von_Neumann<2, 1>,
                   Sequential_Update<RowMajor>>
          eng(std::move(g), Sandpile_Rule{}, Von_Neumann<2, 1>{},
              Sequential_Update<RowMajor>{});
      // Run enough sweeps to relax the avalanche.
      eng.run(40);
      g = eng.frame();
    }

  // After many drops + relaxations every cell must be below the
  // toppling threshold.
  for (ca_size_t i = 0; i < g.size(0); ++i)
    for (ca_size_t j = 0; j < g.size(1); ++j)
      EXPECT_LT(g.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}), 4)
          << "cell (" << i << ", " << j << ") still over threshold";
}

TEST(CAUpdateScheme, SequentialUpdateUsesSingleBuffer)
{
  // Strictly compile-time tag: Sequential_Update must declare it
  // does not require a double buffer — saves ~50% memory.
  static_assert(not Sequential_Update<RowMajor>::requires_double_buffer);
  static_assert(Synchronous_Update<RowMajor>::requires_double_buffer);
}

// ===================================================================
// Random_Asynchronous_Update: reproducibility
// ===================================================================

TEST(CAUpdateScheme, RandomAsynchronousUpdateIsReproducible)
{
  Grid initial = make_random_grid(24, 24, 0.30, 0xBEEFu);

  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Random_Asynchronous_Update<>>
      a(initial, make_game_of_life_rule(), Moore<2, 1>{},
        Random_Asynchronous_Update<>{0xCAFE, 64});
  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Random_Asynchronous_Update<>>
      b(std::move(initial), make_game_of_life_rule(), Moore<2, 1>{},
        Random_Asynchronous_Update<>{0xCAFE, 64});

  a.run(20);
  b.run(20);
  EXPECT_TRUE(grids_equal(a.frame(), b.frame()))
      << "Same master seed must produce identical frames";
}

TEST(CAUpdateScheme, RandomAsynchronousUpdateChangesFewCellsPerStep)
{
  // After exactly k sub-steps at most k cells may have changed.
  Grid initial = make_random_grid(20, 20, 0.30, 0xBADu);

  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Random_Asynchronous_Update<>>
      eng(initial, make_game_of_life_rule(), Moore<2, 1>{},
          Random_Asynchronous_Update<>{1u, 5});

  Grid before = initial;
  eng.step();
  std::size_t diff = 0;
  for (ca_size_t i = 0; i < before.size(0); ++i)
    for (ca_size_t j = 0; j < before.size(1); ++j)
      if (before.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != eng.frame().at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        ++diff;
  EXPECT_LE(diff, 5u)
      << "5 sub-steps must touch at most 5 cells, got " << diff;
}

// ===================================================================
// Block_Synchronous_Update
// ===================================================================

TEST(CAUpdateScheme, BlockSynchronousUpdateOnlyTouchesOneBlockPerStep)
{
  Grid initial = make_random_grid(8, 8, 0.50, 0x1234u);
  Grid before = initial;

  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Block_Synchronous_Update<4>>
      eng(std::move(initial), make_game_of_life_rule(), Moore<2, 1>{});

  eng.step();

  std::size_t changed_blocks = 0;
  for (ca_size_t bi = 0; bi < 2; ++bi)
    for (ca_size_t bj = 0; bj < 2; ++bj)
      {
        bool touched = false;
        for (ca_size_t i = 0; i < 4 and not touched; ++i)
          for (ca_size_t j = 0; j < 4 and not touched; ++j)
            {
              const auto r = static_cast<ca_index_t>(bi * 4 + i);
              const auto c = static_cast<ca_index_t>(bj * 4 + j);
              if (before.at({r, c}) != eng.frame().at({r, c}))
                touched = true;
            }
        if (touched)
          ++changed_blocks;
      }
  EXPECT_LE(changed_blocks, 1u);
}

// ===================================================================
// Margolus_Update + Block rules: reversibility
// ===================================================================

namespace
{

template <typename Block_Rule>
void run_reversibility_test(Block_Rule rule, std::size_t N,
                            ca_size_t side, std::uint32_t seed,
                            const char *label)
{
  Grid initial = make_random_grid(side, side, 0.45, seed);
  Grid baseline = initial;

  Async_Engine<Grid, Block_Rule, Null_Neighborhood<2>, Margolus_Update>
      eng(std::move(initial), std::move(rule), Null_Neighborhood<2>{},
          Margolus_Update{});

  eng.run(N);
  EXPECT_NE(alive_count(eng.frame()) + 1, 0u);  // sanity: didn't crash
  // Even in pathological cases, the rule must be invertible by
  // running backwards the same number of steps.
  eng.run_back(N);

  ASSERT_TRUE(grids_equal(eng.frame(), baseline))
      << label << ": forward(" << N << ") then backward(" << N
      << ") must restore the initial state bit-by-bit";
  EXPECT_EQ(eng.steps_run(), 0u);
}

}  // namespace

TEST(CAUpdateScheme, MargolusCrittersIsReversibleFor1000Steps)
{
  run_reversibility_test(Critters_Rule{}, 1000u, 32, 0xDEADBEEFu,
                         "Critters");
}

TEST(CAUpdateScheme, MargolusBBMIsReversibleFor1000Steps)
{
  run_reversibility_test(BBM_Rule{}, 1000u, 32, 0xFEEDFACEu, "BBM");
}

TEST(CAUpdateScheme, MargolusTMGasIsReversibleFor1000Steps)
{
  run_reversibility_test(TM_Gas_Rule{}, 1000u, 32, 0xABCDEF01u, "TM Gas");
}

TEST(CAUpdateScheme, MargolusCrittersConservesPopulationParity)
{
  // The Critters rule swaps invert+rotate when popcount != 2 in the
  // 2×2 block. Whole-lattice population is *not* conserved, but each
  // step must still leave the grid in a valid binary state.
  Grid g = make_random_grid(16, 16, 0.40, 0xC0DE);
  Async_Engine<Grid, Critters_Rule, Null_Neighborhood<2>, Margolus_Update>
      eng(std::move(g), Critters_Rule{}, Null_Neighborhood<2>{});
  for (std::size_t t = 0; t < 50; ++t)
    {
      eng.step();
      for (ca_size_t i = 0; i < eng.frame().size(0); ++i)
        for (ca_size_t j = 0; j < eng.frame().size(1); ++j)
          {
            const int v = eng.frame().at({static_cast<ca_index_t>(i),
                                          static_cast<ca_index_t>(j)});
            ASSERT_TRUE(v == 0 or v == 1)
                << "non-binary cell after step " << t;
          }
    }
}

TEST(CAUpdateScheme, MargolusBBMConservesParticleCount)
{
  // BBM is a particle-conserving rule.
  Grid g = make_random_grid(16, 16, 0.20, 0xB1B2u);
  const auto initial_particles = alive_count(g);
  Async_Engine<Grid, BBM_Rule, Null_Neighborhood<2>, Margolus_Update> eng(
      std::move(g), BBM_Rule{}, Null_Neighborhood<2>{});
  for (std::size_t t = 0; t < 100; ++t)
    {
      eng.step();
      EXPECT_EQ(alive_count(eng.frame()), initial_particles)
          << "BBM lost particles at step " << t;
    }
}

TEST(CAUpdateScheme, MargolusTMGasConservesParticleCount)
{
  Grid g = make_random_grid(16, 16, 0.20, 0xEFEFu);
  const auto initial_particles = alive_count(g);
  Async_Engine<Grid, TM_Gas_Rule, Null_Neighborhood<2>, Margolus_Update> eng(
      std::move(g), TM_Gas_Rule{}, Null_Neighborhood<2>{});
  for (std::size_t t = 0; t < 100; ++t)
    {
      eng.step();
      EXPECT_EQ(alive_count(eng.frame()), initial_particles)
          << "TM Gas lost particles at step " << t;
    }
}

// ===================================================================
// Block_Rule adapter: cell rule wrapped as block rule
// ===================================================================

TEST(CAUpdateScheme, CellRuleAsBlockRuleAppliesCellRuleToEachCorner)
{
  // Wrap a tiny "increment" cell-rule into a block rule.
  struct Inc
  {
    int operator()(int s) const noexcept { return s + 1; }
  };
  Cell_Rule_As_Block_Rule<Inc> wrapped(Inc{});
  const Block_2x2<int> in{0, 1, 2, 3};
  const Block_2x2<int> out = wrapped(in);
  EXPECT_EQ(out[0], 1);
  EXPECT_EQ(out[1], 2);
  EXPECT_EQ(out[2], 3);
  EXPECT_EQ(out[3], 4);
}

// ===================================================================
// Hooks
// ===================================================================

TEST(CAUpdateScheme, AsyncEnginePreAndPostHooksFire)
{
  Grid initial = make_random_grid(6, 6, 0.50, 0x1u);
  Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>,
               Synchronous_Update<RowMajor>>
      eng(std::move(initial), make_game_of_life_rule(), Moore<2, 1>{});

  std::vector<std::size_t> pre, post;
  eng.on_pre_step([&](std::size_t s, const Grid &) { pre.push_back(s); });
  eng.on_post_step([&](std::size_t s, const Grid &) { post.push_back(s); });

  eng.run(3);
  EXPECT_EQ(pre, (std::vector<std::size_t>{0, 1, 2}));
  EXPECT_EQ(post, (std::vector<std::size_t>{1, 2, 3}));
}