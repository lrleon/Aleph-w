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
 * @file tpl_ca_neighborhood_test.cc
 * @brief Phase 2 tests for the neighborhoods catalogue.
 */

# include <array>
# include <cstdint>
# include <set>

# include <gtest/gtest.h>

# include <ca-traits.H>
# include <tpl_ca_concepts.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_neighborhood.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Compile-time concept conformance.
// ---------------------------------------------------------------------------

static_assert(NeighborhoodLike<Moore<2, 1>>);
static_assert(NeighborhoodLike<Moore<2, 2>>);
static_assert(NeighborhoodLike<Moore<3, 1>>);
static_assert(NeighborhoodLike<Von_Neumann<2, 1>>);
static_assert(NeighborhoodLike<Von_Neumann<3, 1>>);
static_assert(NeighborhoodLike<Hex_Neighborhood>);
static_assert(NeighborhoodLike<Triangular_Neighborhood>);
static_assert(NeighborhoodLike<Custom_Neighborhood<2, 4>>);

// ---------------------------------------------------------------------------
// Compile-time count checks.
// ---------------------------------------------------------------------------

static_assert(Moore<1, 1>::size_v == 2);
static_assert(Moore<1, 2>::size_v == 4);
static_assert(Moore<2, 1>::size_v == 8);
static_assert(Moore<2, 2>::size_v == 24);
static_assert(Moore<3, 1>::size_v == 26);

static_assert(Von_Neumann<1, 1>::size_v == 2);
static_assert(Von_Neumann<2, 1>::size_v == 4);
static_assert(Von_Neumann<2, 2>::size_v == 12);
static_assert(Von_Neumann<3, 1>::size_v == 6);
static_assert(Von_Neumann<3, 2>::size_v == 24);

static_assert(Hex_Neighborhood::size_v == 6);
static_assert(Triangular_Neighborhood::size_v == 3);

// ---------------------------------------------------------------------------
// Moore — every offset must be unique, non-zero, and inside the radius.
// ---------------------------------------------------------------------------

TEST(CAMoore, OffsetsAreUniqueNonZeroAndBounded)
{
  Moore<2, 1> nh;
  EXPECT_EQ(nh.size(), 8u);
  EXPECT_EQ(nh.radius(), 1u);

  std::set<std::pair<int, int>> seen;
  std::size_t calls = 0;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o)
    {
      ++calls;
      EXPECT_FALSE(o[0] == 0 and o[1] == 0);
      EXPECT_GE(o[0], -1); EXPECT_LE(o[0], 1);
      EXPECT_GE(o[1], -1); EXPECT_LE(o[1], 1);
      seen.emplace(static_cast<int>(o[0]), static_cast<int>(o[1]));
    });
  EXPECT_EQ(calls, 8u);
  EXPECT_EQ(seen.size(), 8u);
}

TEST(CAMoore, R2In2DProduces24Offsets)
{
  Moore<2, 2> nh;
  std::size_t calls = 0;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    ++calls;
    EXPECT_FALSE(o[0] == 0 and o[1] == 0);
    EXPECT_LE(std::max(std::abs((int)o[0]), std::abs((int)o[1])), 2);
  });
  EXPECT_EQ(calls, 24u);
}

// ---------------------------------------------------------------------------
// Von Neumann — offsets satisfy L1 ≤ R and ≥ 1.
// ---------------------------------------------------------------------------

TEST(CAVonNeumann, R1In2DGivesFourCardinalOffsets)
{
  Von_Neumann<2, 1> nh;
  EXPECT_EQ(nh.size(), 4u);

  std::set<std::pair<int, int>> seen;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    seen.emplace(static_cast<int>(o[0]), static_cast<int>(o[1]));
  });
  EXPECT_EQ(seen.count({ -1,  0 }), 1u);
  EXPECT_EQ(seen.count({  1,  0 }), 1u);
  EXPECT_EQ(seen.count({  0, -1 }), 1u);
  EXPECT_EQ(seen.count({  0,  1 }), 1u);
}

TEST(CAVonNeumann, R2In2DOffsetsRespectL1Bound)
{
  Von_Neumann<2, 2> nh;
  std::size_t calls = 0;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    ++calls;
    const int s = std::abs((int)o[0]) + std::abs((int)o[1]);
    EXPECT_GE(s, 1); EXPECT_LE(s, 2);
  });
  EXPECT_EQ(calls, 12u);
}

// ---------------------------------------------------------------------------
// Hex
// ---------------------------------------------------------------------------

TEST(CAHexNeighborhood, SixAxialOffsets)
{
  Hex_Neighborhood nh;
  EXPECT_EQ(nh.size(), 6u);

  std::set<std::pair<int, int>> seen;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    seen.emplace(static_cast<int>(o[0]), static_cast<int>(o[1]));
  });
  EXPECT_EQ(seen.size(), 6u);
  EXPECT_EQ(seen.count({  1,  0 }), 1u);
  EXPECT_EQ(seen.count({ -1,  0 }), 1u);
  EXPECT_EQ(seen.count({  0,  1 }), 1u);
  EXPECT_EQ(seen.count({  0, -1 }), 1u);
  EXPECT_EQ(seen.count({  1, -1 }), 1u);
  EXPECT_EQ(seen.count({ -1,  1 }), 1u);
}

// ---------------------------------------------------------------------------
// Triangular — third offset is parity-dependent.
// ---------------------------------------------------------------------------

TEST(CATriangularNeighborhood, ParityDependentThirdOffset)
{
  Triangular_Neighborhood nh;
  EXPECT_EQ(nh.size(), 3u);

  // Even parity (i+j even) → third offset is (1, 0).
  std::set<std::pair<int, int>> seen_even;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    seen_even.emplace(static_cast<int>(o[0]), static_cast<int>(o[1]));
  });
  EXPECT_EQ(seen_even.count({  0, -1 }), 1u);
  EXPECT_EQ(seen_even.count({  0,  1 }), 1u);
  EXPECT_EQ(seen_even.count({  1,  0 }), 1u);

  // Odd parity → third offset is (-1, 0).
  std::set<std::pair<int, int>> seen_odd;
  nh.for_each_offset({ 0, 1 }, [&](const auto & o) {
    seen_odd.emplace(static_cast<int>(o[0]), static_cast<int>(o[1]));
  });
  EXPECT_EQ(seen_odd.count({  0, -1 }), 1u);
  EXPECT_EQ(seen_odd.count({  0,  1 }), 1u);
  EXPECT_EQ(seen_odd.count({ -1,  0 }), 1u);
}

// ---------------------------------------------------------------------------
// Custom
// ---------------------------------------------------------------------------

TEST(CACustomNeighborhood, EnumeratesProvidedOffsets)
{
  Custom_Neighborhood<2, 3> nh({{ Offset_Vec<2>{ 2, 0 },
                                  Offset_Vec<2>{ 0, 2 },
                                  Offset_Vec<2>{ -3, 1 } }});
  EXPECT_EQ(nh.size(), 3u);
  EXPECT_EQ(nh.radius(), 3u);  // largest |component| is 3

  std::vector<std::pair<int, int>> seen;
  nh.for_each_offset({ 0, 0 }, [&](const auto & o) {
    seen.emplace_back(static_cast<int>(o[0]), static_cast<int>(o[1]));
  });
  ASSERT_EQ(seen.size(), 3u);
  EXPECT_EQ(seen[0].first, 2);  EXPECT_EQ(seen[0].second, 0);
  EXPECT_EQ(seen[1].first, 0);  EXPECT_EQ(seen[1].second, 2);
  EXPECT_EQ(seen[2].first, -3); EXPECT_EQ(seen[2].second, 1);
}

// ---------------------------------------------------------------------------
// gather_neighbors — boundary policy is honoured via at_safe.
// ---------------------------------------------------------------------------

TEST(CAGatherNeighbors, MooreOnToroidalLattice)
{
  Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary> lat({ 3, 3 }, 0);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      lat.set({ i, j }, static_cast<int>(i * 10 + j));

  Moore<2, 1> nh;
  std::array<int, Moore<2, 1>::size_v> buf { };
  gather_neighbors(nh, lat, Coord_Vec<2>{ 0, 0 }, buf.data());

  // Sum of all 8 neighbours of (0,0) under toroidal wrap on a 3x3
  // lattice equals (sum of all cells) - cell(0,0). All cells: 0+1+2+
  // 10+11+12+20+21+22 = 99. Minus cell(0,0) = 99.
  int total = 0;
  for (int v : buf) total += v;
  EXPECT_EQ(total, 99);
}

TEST(CAGatherNeighbors, MooreOnOpenLatticeReadsZeroOutsideBorders)
{
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> lat({ 3, 3 }, 1);
  Moore<2, 1> nh;
  std::array<int, Moore<2, 1>::size_v> buf { };
  gather_neighbors(nh, lat, Coord_Vec<2>{ 0, 0 }, buf.data());

  // (0,0) has only 3 in-range neighbours: (0,1), (1,0), (1,1). The
  // remaining 5 read as 0 (open boundary).
  int alive = 0;
  for (int v : buf) if (v != 0) ++alive;
  EXPECT_EQ(alive, 3);
}