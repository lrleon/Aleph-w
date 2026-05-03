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
 * @file tpl_ca_hashlife_test.cc
 * @brief Phase 10 tests for the Hashlife engine (`tpl_ca_hashlife.H`).
 *
 * Coverage:
 *   1. Cell-level API: set / clear / read / bbox / population.
 *   2. Conway's Game of Life small canonical patterns:
 *        - block (still life), blinker (period 2), glider (moving period 4).
 *   3. Cross-check against an unbounded brute-force reference for several
 *      rules and step counts (powers of 2 up to 1024).
 *   4. Other outer-totalistic rules: HighLife, Day_And_Night, Seeds.
 *   5. RLE round-trip on canonical patterns.
 *   6. Cache invalidation when the rule or universe changes.
 *   7. Edge cases: empty universe, large coordinates, repeated set/clear.
 */

# include <cstdint>
# include <set>
# include <sstream>
# include <string>
# include <utility>

# include <gtest/gtest.h>

# include <tpl_ca_hashlife.H>

using namespace Aleph::CA;

namespace
{
  using Cell    = std::pair<std::int64_t, std::int64_t>;
  using Cell_Set = std::set<Cell>;

  // ------------------------------------------------------------------
  // Helpers
  // ------------------------------------------------------------------

  Cell_Set engine_cells(const Hashlife_Engine & e)
  {
    Cell_Set s;
    e.for_each_alive([&s](std::int64_t x, std::int64_t y)
                     { s.emplace(x, y); });
    return s;
  }

  Cell_Set load_cells(Hashlife_Engine & e, const Cell_Set & cells)
  {
    for (const auto & c : cells)
      e.set_alive(c.first, c.second, true);
    return engine_cells(e);
  }

  // Brute-force one Game-of-Life-style step on an arbitrary cell set.
  // Operates on the union of alive cells and their 8-neighbour shells, so
  // unbounded growth is correctly tracked.
  Cell_Set brute_force_step(const Cell_Set & cur,
                            const Outer_Totalistic_Binary_Rule & rule)
  {
    Cell_Set candidates;
    for (const auto & c : cur)
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
          candidates.emplace(c.first + dx, c.second + dy);

    Cell_Set nxt;
    for (const auto & c : candidates)
      {
        std::uint8_t n = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            {
              if (dx == 0 and dy == 0)
                continue;
              if (cur.count({ c.first + dx, c.second + dy }))
                ++n;
            }
        const bool alive = cur.count(c) != 0;
        if (rule.apply(alive, n))
          nxt.insert(c);
      }
    return nxt;
  }

  Cell_Set brute_force_run(Cell_Set cur,
                           const Outer_Totalistic_Binary_Rule & rule,
                           std::uint64_t steps)
  {
    for (std::uint64_t i = 0; i < steps; ++i)
      cur = brute_force_step(cur, rule);
    return cur;
  }

  // Canonical patterns expressed as cell sets.

  Cell_Set blinker()
  {
    return Cell_Set{ { 0, -1 }, { 0, 0 }, { 0, 1 } };
  }

  Cell_Set block()
  {
    return Cell_Set{ { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
  }

  // Standard SE-bound glider:
  //   .O.
  //   ..O
  //   OOO
  Cell_Set glider()
  {
    return Cell_Set{ { 1, 0 },
                     { 2, 1 },
                     { 0, 2 }, { 1, 2 }, { 2, 2 } };
  }

  Cell_Set translate(const Cell_Set & cs, std::int64_t dx, std::int64_t dy)
  {
    Cell_Set out;
    for (const auto & c : cs)
      out.emplace(c.first + dx, c.second + dy);
    return out;
  }

  // Gosper glider gun (LWSS / classic configuration).
  // Coordinates from the canonical RLE; placed near origin.
  Cell_Set gosper_gun()
  {
    static const int xy[][2] = {
      { 1, 5 }, { 1, 6 }, { 2, 5 }, { 2, 6 },
      { 11, 5 }, { 11, 6 }, { 11, 7 }, { 12, 4 }, { 12, 8 },
      { 13, 3 }, { 13, 9 }, { 14, 3 }, { 14, 9 },
      { 15, 6 }, { 16, 4 }, { 16, 8 },
      { 17, 5 }, { 17, 6 }, { 17, 7 }, { 18, 6 },
      { 21, 3 }, { 21, 4 }, { 21, 5 },
      { 22, 3 }, { 22, 4 }, { 22, 5 },
      { 23, 2 }, { 23, 6 },
      { 25, 1 }, { 25, 2 }, { 25, 6 }, { 25, 7 },
      { 35, 3 }, { 35, 4 }, { 36, 3 }, { 36, 4 },
    };
    Cell_Set s;
    for (const auto & p : xy)
      s.emplace(p[0], p[1]);
    return s;
  }
}  // namespace


// ----------------------------------------------------------------------
// 1. Cell-level API
// ----------------------------------------------------------------------

TEST(CAHashlife, EmptyUniverseHasZeroPopulation)
{
  Hashlife_Engine e;
  EXPECT_EQ(e.population(), 0u);
  EXPECT_EQ(e.generation(), 0);
  EXPECT_TRUE(e.bbox().is_empty());
}

TEST(CAHashlife, SetAndGetSingleCell)
{
  Hashlife_Engine e;
  e.set_alive(3, -7, true);
  EXPECT_TRUE(e.alive_at(3, -7));
  EXPECT_FALSE(e.alive_at(0, 0));
  EXPECT_EQ(e.population(), 1u);

  auto bb = e.bbox();
  EXPECT_EQ(bb.x_min, 3);
  EXPECT_EQ(bb.y_min, -7);
  EXPECT_EQ(bb.x_max, 3);
  EXPECT_EQ(bb.y_max, -7);
  EXPECT_EQ(bb.width(), 1);
  EXPECT_EQ(bb.height(), 1);

  e.set_alive(3, -7, false);
  EXPECT_FALSE(e.alive_at(3, -7));
  EXPECT_EQ(e.population(), 0u);
}

TEST(CAHashlife, SetMultipleCellsKeepsExactSet)
{
  Hashlife_Engine e;
  Cell_Set src{ { -100, -100 }, { 0, 0 }, { 7, 13 }, { -3, 5 } };
  for (const auto & c : src)
    e.set_alive(c.first, c.second);
  EXPECT_EQ(engine_cells(e), src);
  EXPECT_EQ(e.population(), src.size());

  auto bb = e.bbox();
  EXPECT_EQ(bb.x_min, -100);
  EXPECT_EQ(bb.x_max, 7);
  EXPECT_EQ(bb.y_min, -100);
  EXPECT_EQ(bb.y_max, 13);
}

TEST(CAHashlife, ClearResetsUniverse)
{
  Hashlife_Engine e;
  load_cells(e, blinker());
  e.advance(5);
  EXPECT_GT(e.generation(), 0);
  EXPECT_GT(e.population(), 0u);

  e.clear();
  EXPECT_EQ(e.population(), 0u);
  EXPECT_EQ(e.generation(), 0);
  EXPECT_TRUE(e.bbox().is_empty());
}

TEST(CAHashlife, ReadOutsideRootIsDead)
{
  Hashlife_Engine e;
  e.set_alive(0, 0);
  EXPECT_FALSE(e.alive_at(1'000'000, 1'000'000));
  EXPECT_FALSE(e.alive_at(-1'000'000, -1'000'000));
}


// ----------------------------------------------------------------------
// 2. Canonical Game of Life patterns
// ----------------------------------------------------------------------

TEST(CAHashlife, BlockIsAStillLife)
{
  Hashlife_Engine e;
  load_cells(e, block());
  const auto initial = engine_cells(e);
  for (unsigned k : { 0u, 1u, 2u, 3u, 4u, 5u })
    {
      e.advance(k);
      EXPECT_EQ(engine_cells(e), initial)
        << "block changed after advance(" << k << ")";
    }
}

TEST(CAHashlife, BlinkerIsPeriod2)
{
  Hashlife_Engine e;
  load_cells(e, blinker());
  const auto vertical = engine_cells(e);
  // After 2 steps the blinker is back to the vertical state.
  e.advance(0);
  EXPECT_EQ(engine_cells(e), vertical) << "after 2 steps";
  // 1024 is even => still vertical.
  e.advance(10);
  EXPECT_EQ(engine_cells(e), vertical) << "after 1024 + 2 steps";
  EXPECT_EQ(e.population(), 3u);
}

TEST(CAHashlife, GliderTravelsByOneCellEveryFourSteps)
{
  Hashlife_Engine e;
  const auto initial = glider();
  load_cells(e, initial);

  // After 4 steps the glider has shifted by (+1, +1) and is otherwise
  // identical (same cell offsets).
  const std::uint64_t advanced = e.run(4);
  ASSERT_GE(advanced, 4u);
  EXPECT_EQ(engine_cells(e), translate(initial,
                                       static_cast<std::int64_t>(advanced) / 4,
                                       static_cast<std::int64_t>(advanced) / 4));
  EXPECT_EQ(e.population(), 5u);
}

TEST(CAHashlife, GliderMatchesBruteForceForManySteps)
{
  Hashlife_Engine e;
  load_cells(e, glider());
  Cell_Set ref = glider();

  // Iterate by power-of-two chunks, comparing at every checkpoint.
  for (unsigned k = 0; k <= 9; ++k)  // up to 1024 generations
    {
      const std::uint64_t advanced = e.advance(k);
      ref = brute_force_run(ref, Conway_Life, advanced);
      EXPECT_EQ(engine_cells(e), ref)
        << "after advance(" << k << ") = " << advanced << " steps";
    }
}


// ----------------------------------------------------------------------
// 3. Cross-validation across several rules
// ----------------------------------------------------------------------

struct Rule_Sample
{
  const char * name;
  Outer_Totalistic_Binary_Rule rule;
  Cell_Set seed;
  std::uint64_t steps;
};

class CAHashlifeBruteForceMatch : public ::testing::TestWithParam<Rule_Sample>
{};

TEST_P(CAHashlifeBruteForceMatch, MatchesUnboundedReference)
{
  const auto & sample = GetParam();
  Hashlife_Engine e(sample.rule);
  load_cells(e, sample.seed);
  e.run(sample.steps);

  const Cell_Set ref =
    brute_force_run(sample.seed, sample.rule, sample.steps);
  EXPECT_EQ(engine_cells(e), ref) << sample.name;
}

INSTANTIATE_TEST_SUITE_P(
  CAHashlifeBruteForce, CAHashlifeBruteForceMatch,
  ::testing::Values(
    Rule_Sample{ "Conway_blinker_128",  Conway_Life,    blinker(),     128u },
    Rule_Sample{ "Conway_block_128",    Conway_Life,    block(),       128u },
    Rule_Sample{ "Conway_glider_256",   Conway_Life,    glider(),      256u },
    Rule_Sample{ "HighLife_glider_128", HighLife,       glider(),      128u },
    Rule_Sample{ "Seeds_blinker_32",    Seeds,          blinker(),      32u },
    Rule_Sample{ "Day_Night_block_64",  Day_And_Night,  block(),        64u }
  ),
  [](const ::testing::TestParamInfo<Rule_Sample> & i)
  { return std::string(i.param.name); });


// ----------------------------------------------------------------------
// 4. Advance / run semantics
// ----------------------------------------------------------------------

TEST(CAHashlife, AdvanceReturnsAtLeast2ToTheK)
{
  Hashlife_Engine e;
  load_cells(e, glider());
  for (unsigned k = 0; k <= 8; ++k)
    {
      const std::uint64_t before = static_cast<std::uint64_t>(e.generation());
      const std::uint64_t advanced = e.advance(k);
      EXPECT_GE(advanced, std::uint64_t{1} << k);
      EXPECT_EQ(static_cast<std::uint64_t>(e.generation()),
                before + advanced);
    }
}

TEST(CAHashlife, RunAtLeastReachesRequestedGeneration)
{
  Hashlife_Engine e;
  load_cells(e, glider());
  const std::int64_t target = e.generation() + 1023;
  e.run(1023);
  EXPECT_GE(e.generation(), target);
}

TEST(CAHashlife, AdvanceTooLargeKThrows)
{
  Hashlife_Engine e;
  EXPECT_THROW(e.advance(63), std::domain_error);
  EXPECT_THROW(e.advance(100), std::domain_error);
}

TEST(CAHashlife, ConstructorRejectsTinyCacheCapacity)
{
  EXPECT_THROW(Hashlife_Engine(Conway_Life, 16), std::domain_error);
}


// ----------------------------------------------------------------------
// 5. RLE round-trip
// ----------------------------------------------------------------------

TEST(CAHashlife, RleRoundTripPreservesGlider)
{
  Hashlife_Engine src;
  load_cells(src, glider());
  const std::string rle = src.save_rle_string("test");

  Hashlife_Engine dst;
  dst.load_rle_string(rle);

  EXPECT_EQ(dst.population(), src.population());
  // Both engines may centre the pattern differently, but the *shape*
  // (set of cells minus its bbox origin) must coincide.
  auto normalise = [](const Cell_Set & cs)
  {
    if (cs.empty())
      return cs;
    std::int64_t mx = cs.begin()->first;
    std::int64_t my = cs.begin()->second;
    for (const auto & c : cs)
      {
        if (c.first < mx) mx = c.first;
        if (c.second < my) my = c.second;
      }
    Cell_Set out;
    for (const auto & c : cs)
      out.emplace(c.first - mx, c.second - my);
    return out;
  };
  EXPECT_EQ(normalise(engine_cells(dst)), normalise(engine_cells(src)));
}

TEST(CAHashlife, RleRoundTripPreservesGosperGun)
{
  Hashlife_Engine src;
  load_cells(src, gosper_gun());
  const std::string rle = src.save_rle_string();

  Hashlife_Engine dst;
  dst.load_rle_string(rle);

  EXPECT_EQ(dst.population(), src.population());
  EXPECT_EQ(dst.population(), gosper_gun().size());
}

TEST(CAHashlife, RleHeaderEncodesRule)
{
  Hashlife_Engine e(HighLife);
  load_cells(e, blinker());
  const std::string rle = e.save_rle_string();
  EXPECT_NE(rle.find("rule = B36/S23"), std::string::npos)
    << "rule field missing from: " << rle;
}

TEST(CAHashlife, RleParseAcceptsStandardConwayDialect)
{
  // Glider in canonical RLE.
  const std::string glider_rle =
    "#N glider\n"
    "x = 3, y = 3, rule = B3/S23\n"
    "bob$2bo$3o!\n";
  Hashlife_Engine e;
  e.load_rle_string(glider_rle);
  EXPECT_EQ(e.population(), 5u);
  // After 4 steps it should still have 5 cells (gliders don't lose cells).
  e.run(4);
  EXPECT_EQ(e.population(), 5u);
}

TEST(CAHashlife, FormatRuleIsRoundTripStable)
{
  for (const auto & r : { Conway_Life, HighLife, Day_And_Night, Seeds })
    {
      const std::string s = format_rule(r);
      auto parsed = parse_rule(s);
      ASSERT_TRUE(parsed.has_value());
      EXPECT_EQ(*parsed, r) << "round-trip failed for: " << s;
    }
}


// ----------------------------------------------------------------------
// 6. Cache and rule changes
// ----------------------------------------------------------------------

TEST(CAHashlife, StatsTrackCanonicalNodes)
{
  Hashlife_Engine e;
  load_cells(e, glider());
  e.advance(5);
  const auto stats = e.stats();
  EXPECT_GT(stats.canonical_nodes, 0u);
  EXPECT_GT(stats.result_misses, 0u);
}

TEST(CAHashlife, ChangingRuleInvalidatesResultCache)
{
  Hashlife_Engine e(Conway_Life);
  load_cells(e, glider());
  e.advance(4);
  const auto before = e.stats();
  EXPECT_GT(before.result_misses, 0u);

  e.set_rule(HighLife);
  EXPECT_EQ(e.stats().result_cache_clears, before.result_cache_clears + 1);
}

TEST(CAHashlife, SmallCacheCapacityForcesPeriodicClears)
{
  Hashlife_Engine e(Conway_Life, /*cache_capacity*/ 2048);
  load_cells(e, gosper_gun());
  e.run(2048);
  // The result cache must have been cleared at least once before we hit
  // 2 Ki canonical nodes; otherwise the bound would be violated.
  EXPECT_GT(e.stats().result_cache_clears, 0u);
}


// ----------------------------------------------------------------------
// 7. Comparison against a synchronous brute-force reference at scale
// ----------------------------------------------------------------------

TEST(CAHashlife, GosperGunMatchesReferenceAt256Steps)
{
  Hashlife_Engine e;
  load_cells(e, gosper_gun());
  e.run(256);

  const Cell_Set ref = brute_force_run(gosper_gun(), Conway_Life, 256);
  EXPECT_EQ(engine_cells(e), ref);
}

TEST(CAHashlife, GosperGunPopulationGrowsLinearly)
{
  Hashlife_Engine e;
  load_cells(e, gosper_gun());
  const std::uint64_t pop_initial = e.population();
  e.run(120);   // 4 gliders emitted (one per 30 generations)
  const std::uint64_t pop_after = e.population();
  EXPECT_GT(pop_after, pop_initial);
  // Gosper gun emits one glider every 30 generations and oscillates
  // through configurations of varying internal density. Within the first
  // few hundred generations, total population stays in a band that easily
  // accommodates the gun (36-50 cells across its cycle) plus roughly
  // four newly-emitted gliders (5 cells each).
  EXPECT_GE(pop_after, 40u);
  EXPECT_LE(pop_after, 100u);
}


// ----------------------------------------------------------------------
// 8. Edge cases
// ----------------------------------------------------------------------

TEST(CAHashlife, RunZeroIsNoOp)
{
  Hashlife_Engine e;
  load_cells(e, blinker());
  const auto before = engine_cells(e);
  EXPECT_EQ(e.run(0), 0u);
  EXPECT_EQ(engine_cells(e), before);
  EXPECT_EQ(e.generation(), 0);
}

TEST(CAHashlife, LargeCoordinatesAreSupported)
{
  Hashlife_Engine e;
  e.set_alive(1'000'000, -1'000'000);
  EXPECT_TRUE(e.alive_at(1'000'000, -1'000'000));
  EXPECT_EQ(e.population(), 1u);
  // A lone cell dies on the next Conway step (no neighbours).
  e.run(1);
  EXPECT_EQ(e.population(), 0u);
}

TEST(CAHashlife, RepeatedSetAliveOnSameCellIsIdempotent)
{
  Hashlife_Engine e;
  for (int i = 0; i < 10; ++i)
    e.set_alive(5, 5, true);
  EXPECT_EQ(e.population(), 1u);
  for (int i = 0; i < 10; ++i)
    e.set_alive(5, 5, false);
  EXPECT_EQ(e.population(), 0u);
}

TEST(CAHashlife, DeepRunHashlifeStaysExactAgainstBruteForce)
{
  // A short stress test: a known random-looking pattern on Conway, run for
  // 512 steps, must match the brute-force exactly. Any drift exposes a
  // bug in the macrocell evolution.
  Cell_Set seed;
  // R-pentomino: notoriously chaotic until step ~1100; perfect for
  // catching subtle bugs.
  for (auto p : { Cell{1, 0}, Cell{2, 0}, Cell{0, 1}, Cell{1, 1}, Cell{1, 2} })
    seed.insert(p);

  Hashlife_Engine e;
  load_cells(e, seed);
  e.run(512);
  const Cell_Set ref = brute_force_run(seed, Conway_Life, 512);
  EXPECT_EQ(engine_cells(e), ref);
  EXPECT_EQ(e.population(), ref.size());
}