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
 * @file tpl_ca_parallel_engine_test.cc
 * @brief Coverage tests for `Aleph::CA::Parallel_Synchronous_Engine` (Phase 5).
 *
 * Scope:
 *   - Tiling primitives (`split_range_balanced`, `Row_Partition`,
 *     `Block_Partition_2D`, Morton helpers).
 *   - Bit-for-bit equivalence between the parallel and sequential
 *     engines under multiple thread counts (1, 2, 4, 8) for:
 *       * 2D Game of Life on `Lattice` (toroidal, open, reflective,
 *         constant, neumann boundaries).
 *       * 2D Game of Life on `Ghost_Lattice` (toroidal & reflective).
 *       * 2D totalistic rule with `Von_Neumann<2,2>` neighbourhood.
 *       * 2D `Tile<W,H>` iteration order.
 *       * 1D Wolfram rule 30 / 110 evolution.
 *       * 3D outer-totalistic rule.
 *   - Determinism across repeated runs and across pool sizes.
 *   - Sequential fallback when the workload is below the threshold.
 *   - Pre / post hook firing semantics.
 *   - Custom `ThreadPool` injection.
 */

#include <array>
#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <thread_pool.H>

#include <ca-tiling.H>
#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_bit_storage.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_ghost_lattice.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Tiling primitives.
// ---------------------------------------------------------------------------

TEST(CATiling, SplitRangeBalancedExactDivision)
{
  // Five threads dividing 50 indices: every range gets ten, no remainder.
  for (std::size_t i = 0; i < 5; ++i)
    {
      const auto r = split_range_balanced(50, 5, i);
      EXPECT_EQ(r.size(), 10u);
      EXPECT_EQ(r.begin, i * 10u);
      EXPECT_EQ(r.end, (i + 1) * 10u);
    }
}

TEST(CATiling, SplitRangeBalancedRemainder)
{
  // Three threads dividing 10 indices: 4, 3, 3.
  const auto a = split_range_balanced(10, 3, 0);
  const auto b = split_range_balanced(10, 3, 1);
  const auto c = split_range_balanced(10, 3, 2);
  EXPECT_EQ(a.size(), 4u);
  EXPECT_EQ(b.size(), 3u);
  EXPECT_EQ(c.size(), 3u);
  EXPECT_EQ(a.begin, 0u);
  EXPECT_EQ(b.begin, a.end);
  EXPECT_EQ(c.begin, b.end);
  EXPECT_EQ(c.end, 10u);
}

TEST(CATiling, SplitRangeBalancedEdgeCases)
{
  // Zero parts collapses to the full range.
  EXPECT_EQ(split_range_balanced(7, 0, 0).end, 7u);
  // Out-of-range index produces an empty trailing range at `n`.
  const auto out = split_range_balanced(7, 3, 5);
  EXPECT_TRUE(out.empty());
  EXPECT_EQ(out.begin, 7u);
}

TEST(CATiling, RowPartitionCoversEveryIndexExactlyOnce)
{
  std::array<ca_size_t, 2> ext{17, 9};
  std::vector<int> hits(ext[0], 0);
  for (std::size_t p = 0; p < 5; ++p)
    {
      const auto r = Row_Partition<2>::slab(ext, 5, p);
      for (ca_size_t i = r.begin; i < r.end; ++i)
        ++hits[i];
    }
  for (int h : hits)
    EXPECT_EQ(h, 1);
}

TEST(CATiling, BlockPartition2DCoversEveryCellOnce)
{
  std::array<ca_size_t, 2> ext{12, 18};
  const auto factor = Block_Partition_2D::factor_partitions(ext, 6);
  EXPECT_EQ(factor[0] * factor[1], 6u);

  std::vector<std::vector<int>> hits(ext[0], std::vector<int>(ext[1], 0));
  for (std::size_t t = 0; t < 6; ++t)
    {
      const auto tile = Block_Partition_2D::tile(ext, factor[0], factor[1], t);
      for (ca_size_t i = tile.rows.begin; i < tile.rows.end; ++i)
        for (ca_size_t j = tile.cols.begin; j < tile.cols.end; ++j)
          ++hits[i][j];
    }
  for (const auto &row : hits)
    for (int h : row)
      EXPECT_EQ(h, 1);
}

TEST(CATiling, MortonEncodeRoundTripBits)
{
  // Bit interleaving: morton_2d(0b101, 0b010) == 0b011001.
  EXPECT_EQ(morton_encode_2d(0b101u, 0b010u), 0b011001u);
  // Sanity: identical inputs put each bit pair as 11 or 00.
  EXPECT_EQ(morton_encode_2d(0b1111u, 0b1111u), 0b11111111u);
  // Pure-x and pure-y inputs occupy alternating bits.
  EXPECT_EQ(morton_encode_2d(0xFFu, 0u), 0x5555u);
  EXPECT_EQ(morton_encode_2d(0u, 0xFFu), 0xAAAAu);
}

TEST(CATiling, ShouldRunSequentialBelowThreshold)
{
  EXPECT_TRUE(should_run_sequential(/*cells=*/0, /*parts=*/8, /*min=*/100));
  EXPECT_TRUE(should_run_sequential(50, 8, 100));
  EXPECT_TRUE(should_run_sequential(1000, 1, 100));
  EXPECT_FALSE(should_run_sequential(1000, 8, 100));
}

// ---------------------------------------------------------------------------
// Helpers for the engine equivalence tests.
// ---------------------------------------------------------------------------

template <typename L>
static void seed_random(L &lat, std::uint32_t seed, double density = 0.4)
{
  std::mt19937 rng(seed);
  std::bernoulli_distribution flip(density);
  using coord_t = typename L::coord_type;
  if constexpr (L::rank == 1)
    {
      const ca_size_t n0 = lat.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        lat.set(coord_t{static_cast<ca_index_t>(i)}, flip(rng) ? 1 : 0);
    }
  else if constexpr (L::rank == 2)
    {
      const ca_size_t n0 = lat.size(0);
      const ca_size_t n1 = lat.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          lat.set(coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
                  flip(rng) ? 1 : 0);
    }
  else if constexpr (L::rank == 3)
    {
      const ca_size_t n0 = lat.size(0);
      const ca_size_t n1 = lat.size(1);
      const ca_size_t n2 = lat.size(2);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          for (ca_size_t k = 0; k < n2; ++k)
            lat.set(coord_t{static_cast<ca_index_t>(i),
                            static_cast<ca_index_t>(j),
                            static_cast<ca_index_t>(k)},
                    flip(rng) ? 1 : 0);
    }
}

template <typename L>
static bool frames_equal(const L &a, const L &b)
{
  if (a.extents() != b.extents())
    return false;
  using coord_t = typename L::coord_type;
  if constexpr (L::rank == 1)
    {
      const ca_size_t n0 = a.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        if (a.at(coord_t{static_cast<ca_index_t>(i)})
            != b.at(coord_t{static_cast<ca_index_t>(i)}))
          return false;
    }
  else if constexpr (L::rank == 2)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          {
            const coord_t c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
            if (a.at(c) != b.at(c))
              return false;
          }
    }
  else if constexpr (L::rank == 3)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      const ca_size_t n2 = a.size(2);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          for (ca_size_t k = 0; k < n2; ++k)
            {
              const coord_t c{static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j),
                              static_cast<ca_index_t>(k)};
              if (a.at(c) != b.at(c))
                return false;
            }
    }
  return true;
}

// Helper to drive both engines through `steps` iterations and assert
// the resulting frames are bit-equal.
template <typename Lattice, typename Rule, typename Neighborhood>
static void
expect_engine_equivalence(const Lattice &seed,
                          Rule rule,
                          Neighborhood nh,
                          std::size_t steps,
                          std::size_t partitions,
                          std::size_t min_cells = 0)
{
  Synchronous_Engine<Lattice, Rule, Neighborhood> seq(seed, rule, nh);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = partitions;
  cfg.min_parallel_cells = min_cells;
  Parallel_Synchronous_Engine<Lattice, Rule, Neighborhood> par(seed, rule, nh, cfg);

  seq.run(steps);
  par.run(steps);

  EXPECT_EQ(seq.steps_run(), steps);
  EXPECT_EQ(par.steps_run(), steps);
  EXPECT_TRUE(frames_equal(seq.frame(), par.frame()))
    << "Parallel engine diverged from sequential at " << steps << " steps with "
    << partitions << " partitions";
}

// ---------------------------------------------------------------------------
// Equivalence with the sequential engine.
// ---------------------------------------------------------------------------

TEST(CAParallelEngine, GameOfLifeToroidalEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({64, 64}, 0);
  seed_random(seed, /*seed=*/0xC0FFEEu);

  for (std::size_t parts : {1u, 2u, 4u, 8u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 100, parts);
}

TEST(CAParallelEngine, GameOfLifeOpenBoundaryEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  L seed({40, 80}, 0);
  seed_random(seed, /*seed=*/123u);

  for (std::size_t parts : {1u, 2u, 4u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 60, parts);
}

TEST(CAParallelEngine, GameOfLifeReflectiveEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ReflectiveBoundary>;
  L seed({32, 48}, 0);
  seed_random(seed, /*seed=*/7777u);

  for (std::size_t parts : {1u, 3u, 8u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 75, parts);
}

TEST(CAParallelEngine, GameOfLifeConstantBoundaryEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ConstantBoundary<int, 1>>;
  L seed({24, 24}, 0);
  seed_random(seed, /*seed=*/42u, /*density=*/0.25);

  for (std::size_t parts : {1u, 2u, 4u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 50, parts);
}

TEST(CAParallelEngine, GameOfLifeNeumannEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, NeumannBoundary>;
  L seed({36, 28}, 0);
  seed_random(seed, /*seed=*/1u);

  for (std::size_t parts : {1u, 4u, 8u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 60, parts);
}

TEST(CAParallelEngine, GhostLatticeToroidalEquivalence)
{
  using L = Ghost_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary, 1>;
  L seed({48, 48});
  seed_random(seed, /*seed=*/9090u);

  for (std::size_t parts : {1u, 2u, 4u, 8u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 100, parts);
}

TEST(CAParallelEngine, GhostLatticeReflectiveEquivalence)
{
  using L = Ghost_Lattice<Dense_Cell_Storage<int, 2>, ReflectiveBoundary, 1>;
  L seed({40, 60});
  seed_random(seed, /*seed=*/55555u);

  for (std::size_t parts : {1u, 4u})
    expect_engine_equivalence(seed, make_game_of_life_rule(), Moore<2, 1>{}, 80, parts);
}

TEST(CAParallelEngine, TotalisticVonNeumannRadius2Equivalence)
{
  // A simple totalistic rule that depends on a non-trivial neighbourhood.
  // Sum of the centre and 12 Von-Neumann radius-2 neighbours: keep the
  // last bit. Plays nicely with our `int` cells.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({32, 32}, 0);
  seed_random(seed, /*seed=*/24601u);

  auto totalistic = Totalistic_Rule<int (*)(int)>(+[](int sum) -> int { return sum & 1; });
  for (std::size_t parts : {1u, 2u, 8u})
    expect_engine_equivalence(seed, totalistic, Von_Neumann<2, 2>{}, 50, parts);
}

TEST(CAParallelEngine, TileOrderEquivalence)
{
  // The parallel engine respects the `Order` template just like the
  // sequential one; equivalence must hold under tiled iteration too.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Order = Tile<8, 8>;
  L seed({64, 64}, 0);
  seed_random(seed, /*seed=*/42424242u);

  Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>, Order> seq(
    seed, make_game_of_life_rule(), Moore<2, 1>{});

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>, Order> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  seq.run(100);
  par.run(100);
  EXPECT_TRUE(frames_equal(seq.frame(), par.frame()));
}

TEST(CAParallelEngine, Wolfram1DEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;
  Custom_Neighborhood<1, 2> nh(
    std::array<Offset_Vec<1>, 2>{Offset_Vec<1>{-1}, Offset_Vec<1>{1}});

  for (std::uint8_t rule_no : {std::uint8_t{30}, std::uint8_t{90}, std::uint8_t{110}})
    {
      L seed({257}, 0);
      seed.set(typename L::coord_type{128}, 1);
      auto rule = make_wolfram_elementary_rule(rule_no);

      for (std::size_t parts : {1u, 2u, 4u, 8u})
        expect_engine_equivalence(seed, rule, nh, 60, parts);
    }
}

TEST(CAParallelEngine, ThreeDOuterTotalisticEquivalence)
{
  using L = Lattice<Dense_Cell_Storage<int, 3>, ToroidalBoundary>;
  L seed({12, 12, 12}, 0);
  seed_random(seed, /*seed=*/2024u, /*density=*/0.3);

  // 3D Life-like rule: B5/S45.
  auto rule = Outer_Totalistic_Rule<int (*)(int, std::size_t)>(
    +[](int current, std::size_t alive) -> int {
      const bool alive_now = current != 0;
      const bool next_alive = (alive_now and (alive == 4 or alive == 5))
                              or (not alive_now and alive == 5);
      return next_alive ? 1 : 0;
    });

  for (std::size_t parts : {1u, 2u, 4u})
    expect_engine_equivalence(seed, rule, Moore<3, 1>{}, 25, parts);
}

// ---------------------------------------------------------------------------
// Engine-level behaviour.
// ---------------------------------------------------------------------------

TEST(CAParallelEngine, SequentialFallbackBelowThreshold)
{
  // A very small lattice with a high threshold should bypass the pool
  // entirely and produce the same trajectory as the sequential engine.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({4, 4}, 0);
  seed.set({0, 1}, 1);
  seed.set({1, 2}, 1);
  seed.set({2, 0}, 1);
  seed.set({2, 1}, 1);
  seed.set({2, 2}, 1);  // glider

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 1024;  // threshold above the 16-cell lattice
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> seq(
    seed, make_game_of_life_rule(), Moore<2, 1>{});

  par.run(20);
  seq.run(20);
  EXPECT_TRUE(frames_equal(seq.frame(), par.frame()));
}

TEST(CAParallelEngine, HookFiringSemantics)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({16, 16}, 0);
  seed_random(seed, /*seed=*/1234u);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  std::size_t pre_count = 0;
  std::size_t post_count = 0;
  std::size_t last_pre_idx = 0;
  std::size_t last_post_idx = 0;
  par.on_pre_step([&](std::size_t step_idx, const L &) {
    ++pre_count;
    last_pre_idx = step_idx;
  });
  par.on_post_step([&](std::size_t step_idx, const L &) {
    ++post_count;
    last_post_idx = step_idx;
  });

  par.run(7);
  EXPECT_EQ(pre_count, 7u);
  EXPECT_EQ(post_count, 7u);
  EXPECT_EQ(last_pre_idx, 6u);   // pre fires before swap, so last index is 6
  EXPECT_EQ(last_post_idx, 7u);  // post fires after swap, so last index is 7
}

TEST(CAParallelEngine, CustomThreadPoolInjection)
{
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({32, 32}, 0);
  seed_random(seed, /*seed=*/8675309u);

  ThreadPool custom_pool(2);

  Parallel_Engine_Config cfg;
  cfg.pool = &custom_pool;
  cfg.num_partitions = 4;  // oversubscribe the 2-worker pool
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> seq(
    seed, make_game_of_life_rule(), Moore<2, 1>{});

  par.run(50);
  seq.run(50);
  EXPECT_TRUE(frames_equal(seq.frame(), par.frame()));
}

TEST(CAParallelEngine, DeterminismAcrossRepeatedRuns)
{
  // Same seed, same configuration, multiple independent runs: identical
  // final frames.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({40, 40}, 0);
  seed_random(seed, /*seed=*/0xDEADBEEFu);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 8;
  cfg.min_parallel_cells = 0;

  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> a(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> b(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  a.run(80);
  b.run(80);
  EXPECT_TRUE(frames_equal(a.frame(), b.frame()));
}

TEST(CAParallelEngine, GliderTrajectoryIsExact)
{
  // A glider on an 8x8 toroidal grid returns to a translated copy of
  // itself after exactly four steps, both sequentially and in parallel.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({8, 8}, 0);
  seed.set({0, 1}, 1);
  seed.set({1, 2}, 1);
  seed.set({2, 0}, 1);
  seed.set({2, 1}, 1);
  seed.set({2, 2}, 1);

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 4;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  par.run(4);

  // After 4 steps the glider has moved by (+1, +1).
  L expected({8, 8}, 0);
  expected.set({1, 2}, 1);
  expected.set({2, 3}, 1);
  expected.set({3, 1}, 1);
  expected.set({3, 2}, 1);
  expected.set({3, 3}, 1);
  EXPECT_TRUE(frames_equal(par.frame(), expected));
}

TEST(CAParallelEngine, BlinkerOscillatesUnderEveryPartitionCount)
{
  // A blinker has period 2: B at step 0, B' at step 1, B at step 2.
  using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({5, 5}, 0);
  seed.set({2, 1}, 1);
  seed.set({2, 2}, 1);
  seed.set({2, 3}, 1);

  for (std::size_t parts : {1u, 2u, 5u, 8u})
    {
      Parallel_Engine_Config cfg;
      cfg.num_partitions = parts;
      cfg.min_parallel_cells = 0;
      Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
        seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);
      par.run(2);
      EXPECT_TRUE(frames_equal(par.frame(), seed))
        << "Blinker did not return to seed with " << parts << " partitions";
    }
}

TEST(CAParallelEngine, RankOneEquivalenceSmallLattice)
{
  // A small 1D lattice still works correctly when partitions exceed
  // the number of cells along axis 0; any extra partitions should be
  // empty no-ops.
  using L = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;
  Custom_Neighborhood<1, 2> nh(
    std::array<Offset_Vec<1>, 2>{Offset_Vec<1>{-1}, Offset_Vec<1>{1}});

  L seed({5}, 0);
  seed.set({2}, 1);

  expect_engine_equivalence(seed, make_wolfram_elementary_rule(30), nh, 3, 16);
}

TEST(CAParallelEngine, BitCellStorageRunsConcurrentlyWithoutDataRaces)
{
  using L = Lattice<Bit_Cell_Storage<2>, ToroidalBoundary>;
  L seed({32, 32}, false);
  seed_random(seed, 0xB17u, 0.35);

  Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> seq(
    seed, make_game_of_life_rule(), Moore<2, 1>{});

  Parallel_Engine_Config cfg;
  cfg.num_partitions = 8;
  cfg.min_parallel_cells = 0;
  Parallel_Synchronous_Engine<L, Game_Of_Life_Rule, Moore<2, 1>> par(
    seed, make_game_of_life_rule(), Moore<2, 1>{}, cfg);

  seq.run(30);
  par.run(30);

  EXPECT_TRUE(frames_equal(seq.frame(), par.frame()));
}
