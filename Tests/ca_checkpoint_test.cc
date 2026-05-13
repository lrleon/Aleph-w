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
 * @file ca_checkpoint_test.cc
 * @brief Phase 15 tests for binary checkpoints, resume tokens,
 *        the Periodic_Checkpoint_Observer and the Tile_Cache.
 *
 *  Coverage:
 *   - Round-trip: `save → load → save'` produces bit-identical files.
 *   - Resume reproducibility: `run(N) → save → load → run(N)` yields
 *     the same final frame as `run(2N)`.
 *   - Stochastic resume: master RNG seed is restored so a stochastic
 *     rule's continuation is identical to the uninterrupted run.
 *   - `inspect_checkpoint` returns the validated header.
 *   - `Periodic_Checkpoint_Observer` writes one snapshot every k steps.
 *   - `Tile_Cache` survives more touches than its capacity (LRU
 *     eviction works) and round-trips a checkerboard pattern.
 */

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ca-checkpoint.H>
#include <ca-rng.H>
#include <ca-streaming-storage.H>
#include <ca-traits.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_stochastic_rules.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;

std::filesystem::path tmp_path(const std::string &name)
{
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path()
         / ("aleph_ca_chk_" + name + "_" + std::to_string(static_cast<long long>(tick)));
}

Grid random_grid(ca_size_t side, std::uint32_t seed, double p_alive = 0.4)
{
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  Grid g({side, side}, 0);
  for (ca_size_t i = 0; i < side; ++i)
    for (ca_size_t j = 0; j < side; ++j)
      g.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
            u(rng) < p_alive ? 1 : 0);
  return g;
}

bool grids_equal(const Grid &a, const Grid &b)
{
  if (a.extents() != b.extents())
    return false;
  for (ca_size_t i = 0; i < a.size(0); ++i)
    for (ca_size_t j = 0; j < a.size(1); ++j)
      if (a.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)})
          != b.at({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}))
        return false;
  return true;
}

std::string read_file(const std::filesystem::path &p)
{
  std::ifstream in(p, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

// =====================================================================
// Round-trip: save → load → save' is bit-identical
// =====================================================================

TEST(CACheckpoint, RoundTripProducesIdenticalFiles)
{
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  const auto dir = tmp_path("roundtrip");
  std::filesystem::create_directories(dir);
  const auto path1 = dir / "snap1.bin";
  const auto path2 = dir / "snap2.bin";

  Engine eng1(random_grid(20, 0x1234), make_game_of_life_rule(), Moore<2, 1>{});
  eng1.run(7);
  save_checkpoint(eng1, path1);

  // Construct an engine of the same shape and load the snapshot.
  Engine eng2(Grid({20, 20}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  const Resume_Token token = load_checkpoint_into(eng2, path1);
  EXPECT_EQ(token.header.step_count, 7u);

  save_checkpoint(eng2, path2);

  EXPECT_EQ(read_file(path1), read_file(path2))
    << "save → load → save must be bit-identical";

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Resume reproducibility: run(N) + save + load + run(N) == run(2N)
// =====================================================================

TEST(CACheckpoint, ResumeReproducesUninterruptedRun)
{
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  const auto dir = tmp_path("resume");
  std::filesystem::create_directories(dir);
  const auto path = dir / "midpoint.bin";

  // Reference: uninterrupted run for 2N steps.
  Engine ref(random_grid(20, 0x9999), make_game_of_life_rule(), Moore<2, 1>{});
  ref.run(40);

  // Resume run: N steps, save, fresh engine, load, N more steps.
  Engine resumed(random_grid(20, 0x9999), make_game_of_life_rule(), Moore<2, 1>{});
  resumed.run(20);
  save_checkpoint(resumed, path);

  Engine restored(Grid({20, 20}, 0), make_game_of_life_rule(), Moore<2, 1>{});
  const Resume_Token token = load_checkpoint_into(restored, path);
  EXPECT_EQ(token.header.step_count, 20u);
  EXPECT_EQ(restored.steps_run(), 20u);
  restored.run(20);

  EXPECT_EQ(restored.steps_run(), 40u);
  EXPECT_TRUE(grids_equal(restored.frame(), ref.frame()))
    << "restored run must match uninterrupted run frame-by-frame";

  std::filesystem::remove_all(dir);
}

// =====================================================================
// inspect_checkpoint surfaces the validated header
// =====================================================================

TEST(CACheckpoint, InspectReturnsValidatedHeader)
{
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  const auto dir = tmp_path("inspect");
  std::filesystem::create_directories(dir);
  const auto path = dir / "snap.bin";

  Engine eng(random_grid(8, 17), make_game_of_life_rule(), Moore<2, 1>{});
  eng.run(3);
  save_checkpoint(eng, path);

  const Checkpoint_Header h = inspect_checkpoint(path);
  EXPECT_EQ(h.format_version, 1u);
  EXPECT_EQ(h.rank, 2u);
  EXPECT_EQ(h.extents[0], 8u);
  EXPECT_EQ(h.extents[1], 8u);
  EXPECT_EQ(h.step_count, 3u);
  EXPECT_EQ(h.cell_count, 64u);
  EXPECT_EQ(h.has_rng, 0u);  // Game_Of_Life_Rule is deterministic

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Stochastic rule: master_seed is restored so the future trajectory
// is identical to the uninterrupted run.
// =====================================================================

TEST(CACheckpoint, StochasticRuleResumesBitIdentically)
{
  using Engine = Synchronous_Engine<Grid, Schelling_Rule<>, Moore<2, 1>>;

  const auto dir = tmp_path("stoch");
  std::filesystem::create_directories(dir);
  const auto path = dir / "stoch.bin";

  // Build a Schelling-style stochastic rule (carries a master seed).
  auto make_rule = []
  { return Schelling_Rule<>{0.5, 1.0, 1.0, /*master=*/0xCAFEBABE}; };

  // Reference run for 30 steps.
  Grid initial({16, 16}, 0);
  for (ca_size_t i = 0; i < 16; ++i)
    for (ca_size_t j = 0; j < 16; ++j)
      initial.set({static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)},
                  static_cast<int>((i + j) % 3));
  Engine ref(initial, make_rule(), Moore<2, 1>{});
  ref.run(30);

  // Resume run: 15 steps, save, new engine, load, 15 more.
  Engine partial(initial, make_rule(), Moore<2, 1>{});
  partial.run(15);
  save_checkpoint(partial, path);

  // Build resumed engine with a *wrong* master seed to prove the
  // restore actually overwrites it.
  Engine resumed(Grid({16, 16}, 0), Schelling_Rule<>{0.5, 1.0, 1.0, 0xDEADBEEF},
                 Moore<2, 1>{});
  load_checkpoint_into(resumed, path);
  EXPECT_EQ(resumed.rule().master_seed(), 0xCAFEBABEu)
    << "master seed must be restored from the snapshot";
  resumed.run(15);

  EXPECT_TRUE(grids_equal(resumed.frame(), ref.frame()))
    << "stochastic resume must be bit-identical to uninterrupted run";

  std::filesystem::remove_all(dir);
}

// =====================================================================
// load_checkpoint_into rejects type-hash mismatches
// =====================================================================

TEST(CACheckpoint, LoadRejectsTypeHashMismatch)
{
  using Engine_GoL = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;
  using Wolfram_Rule = Lookup_Rule<2, 2>;
  using Engine_Wolf = Synchronous_Engine<
    Lattice<Dense_Cell_Storage<int, 1>, ToroidalBoundary>,
    Wolfram_Rule, Custom_Neighborhood<1, 2, 1>>;

  const auto dir = tmp_path("badtype");
  std::filesystem::create_directories(dir);
  const auto path = dir / "snap.bin";

  Engine_GoL eng(random_grid(8, 1), make_game_of_life_rule(), Moore<2, 1>{});
  save_checkpoint(eng, path);

  Lattice<Dense_Cell_Storage<int, 1>, ToroidalBoundary> row({8}, 0);
  const auto offsets = std::array<Offset_Vec<1>, 2>{{{-1}, {1}}};
  Engine_Wolf wrong(std::move(row), make_wolfram_elementary_rule(90),
                    Custom_Neighborhood<1, 2, 1>(offsets));
  EXPECT_THROW(load_checkpoint_into(wrong, path), std::runtime_error);

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Periodic_Checkpoint_Observer
// =====================================================================

TEST(CACheckpoint, PeriodicObserverWritesEveryKSteps)
{
  using Engine = Synchronous_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>>;

  const auto dir = tmp_path("periodic");
  std::filesystem::create_directories(dir);

  Engine eng(random_grid(10, 5), make_game_of_life_rule(), Moore<2, 1>{});
  Periodic_Checkpoint_Observer obs(eng, /*every=*/3,
                                   (dir / "snap_{step}.bin").string(),
                                   /*zero_pad=*/4);

  eng.on_post_step([&](std::size_t s, const Grid &f) { obs.on_step_end(s, f); });
  eng.run(10);

  // Steps 3, 6, 9 must have produced files.
  EXPECT_TRUE(std::filesystem::exists(dir / "snap_0003.bin"));
  EXPECT_TRUE(std::filesystem::exists(dir / "snap_0006.bin"));
  EXPECT_TRUE(std::filesystem::exists(dir / "snap_0009.bin"));
  // Steps 1, 2, 4, 5, 7, 8, 10 must not.
  EXPECT_FALSE(std::filesystem::exists(dir / "snap_0001.bin"));
  EXPECT_FALSE(std::filesystem::exists(dir / "snap_0002.bin"));
  EXPECT_FALSE(std::filesystem::exists(dir / "snap_0010.bin"));

  // Headers of the three files agree on extents/version.
  const auto h3 = inspect_checkpoint(dir / "snap_0003.bin");
  const auto h6 = inspect_checkpoint(dir / "snap_0006.bin");
  EXPECT_EQ(h3.step_count, 3u);
  EXPECT_EQ(h6.step_count, 6u);
  EXPECT_EQ(h3.extents, h6.extents);

  std::filesystem::remove_all(dir);
}

// =====================================================================
// Tile_Cache: LRU eviction + disk round-trip
// =====================================================================

TEST(CACheckpointStream, TileCacheLruEvictionAndPersistence)
{
  const auto dir = tmp_path("tile");
  std::filesystem::remove_all(dir);

  using Tiles = Tile_Cache<int, 4>;
  // 16×16 grid → 4×4 = 16 tiles total. Capacity 4 forces evictions.
  Tiles tiles({16, 16}, dir, /*capacity=*/4);

  // Stamp a checkerboard pattern across the whole grid: forces every
  // tile to be touched at least once.
  for (ca_size_t i = 0; i < 16; ++i)
    for (ca_size_t j = 0; j < 16; ++j)
      tiles.set(i, j, static_cast<int>((i + j) & 1));

  EXPECT_LE(tiles.resident(), 4u) << "cache must respect capacity";
  EXPECT_GE(tiles.stats().evictions, 12u)
    << "16 distinct tiles touched, capacity 4 → >= 12 evictions";

  // Persist any leftover dirty tiles, then re-read every cell. With
  // the working set far exceeding capacity we will keep paging in
  // and out — but every read must return the original checkerboard
  // value.
  tiles.flush();
  for (ca_size_t i = 0; i < 16; ++i)
    for (ca_size_t j = 0; j < 16; ++j)
      EXPECT_EQ(tiles.at(i, j), static_cast<int>((i + j) & 1))
        << "checkerboard mismatch at (" << i << "," << j << ")";

  // On-disk file count: every distinct tile that was touched should
  // have a corresponding file.
  std::size_t file_count = 0;
  for (auto &p : std::filesystem::directory_iterator(dir))
    if (p.path().extension() == ".bin")
      ++file_count;
  EXPECT_EQ(file_count, 16u);

  std::filesystem::remove_all(dir);
}

TEST(CACheckpointStream, TileCacheRejectsBadExtents)
{
  using Tiles = Tile_Cache<int, 4>;
  const auto dir = tmp_path("tile_bad");
  std::filesystem::create_directories(dir);
  EXPECT_THROW((Tiles({15, 16}, dir, 4)), std::domain_error);
  EXPECT_THROW((Tiles({16, 16}, dir, 0)), std::domain_error);
  std::filesystem::remove_all(dir);
}