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
 * @file tpl_ca_rule_test.cc
 * @brief Phase 2 tests for the rule catalogue.
 *
 * The Phase 3 engine is not yet available, so these tests advance the
 * automaton manually: read all cells through the current lattice,
 * compute the next state with the rule under test, swap into the next
 * lattice, repeat. This exercises both `gather_neighbors` and the rule
 * combinators.
 */

# include <array>
# include <cstdint>
# include <utility>
# include <vector>

# include <gtest/gtest.h>

# include <ca-traits.H>
# include <tpl_ca_concepts.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_neighborhood.H>
# include <tpl_ca_rule.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  /// Apply a rule to every cell of a 2D `int` lattice once.
  template <typename Rule, typename Boundary>
  Lattice<Dense_Cell_Storage<int, 2>, Boundary>
  step_2d(const Lattice<Dense_Cell_Storage<int, 2>, Boundary> & cur,
          const Rule & rule)
  {
    Lattice<Dense_Cell_Storage<int, 2>, Boundary> nxt(cur.extents());
    Moore<2, 1> nh;
    std::array<int, Moore<2, 1>::size_v> buf { };
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(cur.size(0)); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(cur.size(1)); ++j)
        {
          gather_neighbors(nh, cur, Coord_Vec<2>{ i, j }, buf.data());
          nxt.set({ i, j },
                  rule(cur.at({ i, j }),
                       Neighbor_View<int>(buf.data(), buf.size())));
        }
    return nxt;
  }

  /// Apply a 1D rule with a 2-neighbour `{-1, +1}` neighborhood to a
  /// length-`L` lattice once.
  template <typename Rule, typename Boundary>
  Lattice<Dense_Cell_Storage<int, 1>, Boundary>
  step_1d(const Lattice<Dense_Cell_Storage<int, 1>, Boundary> & cur,
          const Rule & rule)
  {
    Lattice<Dense_Cell_Storage<int, 1>, Boundary> nxt(cur.extents());
    Custom_Neighborhood<1, 2> nh({{ Offset_Vec<1>{ -1 },
                                    Offset_Vec<1>{  1 } }});
    std::array<int, 2> buf { };
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(cur.size(0)); ++i)
      {
        gather_neighbors(nh, cur, Coord_Vec<1>{ i }, buf.data());
        nxt.set({ i },
                rule(cur.at({ i }),
                     Neighbor_View<int>(buf.data(), buf.size())));
      }
    return nxt;
  }

  /// Convenience: build a 2D lattice from a row-major initialiser list.
  template <typename Boundary>
  Lattice<Dense_Cell_Storage<int, 2>, Boundary>
  make_grid(std::array<ca_size_t, 2> ext, const std::vector<int> & data)
  {
    Lattice<Dense_Cell_Storage<int, 2>, Boundary> lat(ext);
    for (ca_size_t i = 0; i < ext[0]; ++i)
      for (ca_size_t j = 0; j < ext[1]; ++j)
        lat.set({ static_cast<ca_index_t>(i),
                  static_cast<ca_index_t>(j) },
                data[i * ext[1] + j]);
    return lat;
  }

  /// Compare contents of two 2D lattices.
  template <typename B1, typename B2>
  bool same(const Lattice<Dense_Cell_Storage<int, 2>, B1> & a,
            const Lattice<Dense_Cell_Storage<int, 2>, B2> & b)
  {
    if (a.size(0) != b.size(0) or a.size(1) != b.size(1))
      return false;
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(a.size(0)); ++i)
      for (ca_index_t j = 0; j < static_cast<ca_index_t>(a.size(1)); ++j)
        if (a.at({ i, j }) != b.at({ i, j }))
          return false;
    return true;
  }
}

// ---------------------------------------------------------------------------
// Game of Life — canonical patterns.
// ---------------------------------------------------------------------------

TEST(CAGameOfLife, BlockIsStill)
{
  // 4x4 grid, 2x2 block at rows/cols [1..2].
  auto lat = make_grid<OpenBoundary>(
    { 4, 4 },
    { 0, 0, 0, 0,
      0, 1, 1, 0,
      0, 1, 1, 0,
      0, 0, 0, 0 });
  auto rule = make_game_of_life_rule();
  for (int t = 0; t < 5; ++t)
    {
      auto nxt = step_2d(lat, rule);
      ASSERT_TRUE(same(lat, nxt)) << "Block must be stationary at t=" << t;
      lat = std::move(nxt);
    }
}

TEST(CAGameOfLife, BlinkerOscillatesPeriod2)
{
  // 5x5 grid, horizontal blinker at row 2, cols 1..3.
  auto horiz = make_grid<OpenBoundary>(
    { 5, 5 },
    { 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 1, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0 });
  auto vert = make_grid<OpenBoundary>(
    { 5, 5 },
    { 0, 0, 0, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 0, 0, 0 });
  auto rule = make_game_of_life_rule();

  auto t1 = step_2d(horiz, rule);
  EXPECT_TRUE(same(t1, vert));
  auto t2 = step_2d(t1, rule);
  EXPECT_TRUE(same(t2, horiz));
  auto t3 = step_2d(t2, rule);
  EXPECT_TRUE(same(t3, vert));
}

TEST(CAGameOfLife, GliderMovesByOneOneEveryFourSteps)
{
  // 8x8 toroidal so the glider can travel without falling off the edge.
  auto lat = make_grid<ToroidalBoundary>(
    { 8, 8 },
    { 0, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 0, 0, 0, 0, 0,
      1, 1, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 });
  // After 4 steps a glider translates by (1, 1).
  auto expected = make_grid<ToroidalBoundary>(
    { 8, 8 },
    { 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 0, 0, 0, 0,
      0, 1, 1, 1, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 });

  auto rule = make_game_of_life_rule();
  for (int t = 0; t < 4; ++t)
    lat = step_2d(lat, rule);
  EXPECT_TRUE(same(lat, expected)) << "Glider must shift by (1,1) in 4 steps";
}

// ---------------------------------------------------------------------------
// Wolfram elementary rules — spot-checked outputs.
// ---------------------------------------------------------------------------

namespace
{
  /// Reference next-state computation for elementary Wolfram rule `r`
  /// applied to (left, self, right). This mirrors the standard
  /// definition and is used as ground truth for our `Lookup_Rule`.
  int wolfram_next(std::uint8_t r, int l, int s, int rg)
  {
    const std::size_t idx = (l << 2) | (s << 1) | rg;
    return (r >> idx) & 1;
  }
}

TEST(CAWolfram1D, AllRulesMatchReferenceFor10Patterns)
{
  // For every elementary rule and every 3-bit pattern, the precomputed
  // table built by `make_wolfram_elementary_rule` must produce the
  // same next-state as the textbook formula.
  for (int r = 0; r < 256; ++r)
    {
      auto rule = make_wolfram_elementary_rule(static_cast<std::uint8_t>(r));
      for (int l = 0; l < 2; ++l)
        for (int s = 0; s < 2; ++s)
          for (int rg = 0; rg < 2; ++rg)
            {
              std::array<int, 2> nb { l, rg };
              const int got = rule(s, Neighbor_View<int>(nb.data(), 2));
              const int want = wolfram_next(static_cast<std::uint8_t>(r),
                                            l, s, rg);
              ASSERT_EQ(got, want)
                << "rule=" << r << " (l,s,r)=(" << l << "," << s << ","
                << rg << ")";
            }
    }
}

TEST(CAWolfram1D, Rule90StepFromCenterImpulse)
{
  // Rule 90 from a single 1 at the centre produces the Sierpinski pattern.
  // After one step, only the cells adjacent to the impulse become 1.
  auto lat = make_grid<OpenBoundary>(
    { 1, 7 },
    { 0, 0, 0, 1, 0, 0, 0 });
  auto rule = make_wolfram_elementary_rule(90);
  // Adapt 1D step using the 2D infrastructure with size(0)=1:
  // we just iterate j over the single row.
  Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary> row({ 7 });
  for (ca_index_t j = 0; j < 7; ++j) row.set({ j }, lat.at({ 0, j }));

  auto nxt = step_1d(row, rule);
  // Expected after rule 90 from impulse at index 3:
  // (l,s,r)=(0,0,0)→0, (0,0,1)→1, (0,1,0)→0, (1,0,0)→1, others → 0
  // Centred impulse at j=3:
  // j=2: l=0,s=0,r=1 → 1
  // j=3: l=0,s=1,r=0 → 0
  // j=4: l=1,s=0,r=0 → 1
  // others stay 0.
  EXPECT_EQ(nxt.at({ 0 }), 0);
  EXPECT_EQ(nxt.at({ 1 }), 0);
  EXPECT_EQ(nxt.at({ 2 }), 1);
  EXPECT_EQ(nxt.at({ 3 }), 0);
  EXPECT_EQ(nxt.at({ 4 }), 1);
  EXPECT_EQ(nxt.at({ 5 }), 0);
  EXPECT_EQ(nxt.at({ 6 }), 0);
}

TEST(CAWolfram1D, Rule30FirstFiveStepsFromImpulse)
{
  // Lattice large enough to avoid hitting OpenBoundary in 5 steps.
  Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary> row({ 21 });
  row.set({ 10 }, 1);  // central impulse
  auto rule = make_wolfram_elementary_rule(30);

  // Reference rows (rule 30 is the canonical Wolfram example).
  // Source: Wolfram MathWorld "Rule 30".
  std::vector<std::vector<int>> expected = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // t=0
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // t=1
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // t=2
    { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 }, // t=3
    { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, // t=4
    { 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 }, // t=5
  };

  for (std::size_t t = 0; t < expected.size(); ++t)
    {
      for (ca_index_t j = 0; j < 21; ++j)
        ASSERT_EQ(row.at({ j }), expected[t][j])
          << "rule 30 t=" << t << " j=" << j;
      row = step_1d(row, rule);
    }
}

// ---------------------------------------------------------------------------
// Composite_Rule
// ---------------------------------------------------------------------------

TEST(CACompositeRule, AppliesRulesInSequence)
{
  // Build two rules over a tiny state machine. Rule A: x -> x + 1.
  // Rule B: x -> x * 2. Composite(A, B): x -> 2(x+1).
  struct Add_One {
    int operator()(int x, Neighbor_View<int>) const { return x + 1; }
  };
  struct Times_Two {
    int operator()(int x, Neighbor_View<int>) const { return x * 2; }
  };
  Composite_Rule<Add_One, Times_Two> r{ Add_One{}, Times_Two{} };
  std::array<int, 0> nb { };
  EXPECT_EQ(r(3, Neighbor_View<int>(nb.data(), 0)), 8);
  EXPECT_EQ(r(0, Neighbor_View<int>(nb.data(), 0)), 2);
}

// ---------------------------------------------------------------------------
// Totalistic_Rule
// ---------------------------------------------------------------------------

TEST(CATotalisticRule, SumsCellAndNeighbours)
{
  // Identity-on-sum: f(s) = s. The rule then returns
  //   current + sum(neighbours) for any state type.
  auto identity = [](int s) { return s; };
  Totalistic_Rule<decltype(identity)> r(identity);
  std::array<int, 4> nb { 1, 2, 3, 4 };
  EXPECT_EQ(r(10, Neighbor_View<int>(nb.data(), 4)), 20);
}

// ---------------------------------------------------------------------------
// Outer_Totalistic_Rule with a non-bool state type.
// ---------------------------------------------------------------------------

TEST(CAOuterTotalisticRule, AliveCountIgnoresMagnitude)
{
  // F: returns the number of non-zero neighbours regardless of `current`.
  auto f = [](int /*current*/, std::size_t alive) -> int
  { return static_cast<int>(alive); };
  Outer_Totalistic_Rule<decltype(f)> r(f);

  std::array<int, 5> nb { 0, 7, 0, -3, 0 };
  EXPECT_EQ(r(99, Neighbor_View<int>(nb.data(), 5)), 2);
}