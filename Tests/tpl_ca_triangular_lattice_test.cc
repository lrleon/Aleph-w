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
 * @file tpl_ca_triangular_lattice_test.cc
 * @brief Tests for `Aleph::CA::Triangular_Lattice` and helpers.
 *
 * Scope:
 *   - Parity helpers: `tri_parity`, `tri_is_up`, `tri_is_down`.
 *   - Pixel mapping: distinct centres for distinct cells.
 *   - Concept conformance.
 *   - Engine integration with `Triangular_Neighborhood`: a known
 *     totalistic rule advances the lattice as expected.
 *   - TikZ export sanity check.
 */

#include <set>
#include <string>

#include <gtest/gtest.h>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>
#include <tpl_ca_triangular_lattice.H>

using namespace Aleph;
using namespace Aleph::CA;

using Tri_2D = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
static_assert(LatticeLike<Tri_2D>);

// ---------------------------------------------------------------------------
// Parity helpers.
// ---------------------------------------------------------------------------

TEST(CATriangularCoord, ParityAlternatesByCoordSum)
{
  EXPECT_TRUE(tri_is_up(Tri_Coord{0, 0}));
  EXPECT_TRUE(tri_is_down(Tri_Coord{0, 1}));
  EXPECT_TRUE(tri_is_up(Tri_Coord{1, 1}));
  EXPECT_TRUE(tri_is_down(Tri_Coord{2, 3}));
  for (ca_index_t i = -3; i <= 3; ++i)
    for (ca_index_t j = -3; j <= 3; ++j)
      EXPECT_EQ(tri_parity(Tri_Coord{i, j}),
                static_cast<int>(((i + j) % 2 + 2) % 2));
}

TEST(CATriangularPixel, CentresAreUniquePerCell)
{
  std::set<std::pair<long long, long long>> seen;
  for (ca_index_t i = 0; i < 6; ++i)
    for (ca_index_t j = 0; j < 6; ++j)
      {
        const auto px = tri_pixel_centre(Tri_Coord{i, j}, 1.0);
        const auto key = std::pair<long long, long long>(
          static_cast<long long>(px[0] * 1e6),
          static_cast<long long>(px[1] * 1e6));
        EXPECT_TRUE(seen.insert(key).second)
          << "duplicate pixel for (i=" << i << ", j=" << j << ")";
      }
}

// ---------------------------------------------------------------------------
// Lattice accessors.
// ---------------------------------------------------------------------------

TEST(CATriangularLattice, AccessorsReadAndWrite)
{
  Tri_2D lat({3, 4}, 0);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      lat.set_tri(Tri_Coord{i, j}, static_cast<int>(i * 10 + j));
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      EXPECT_EQ(lat.at_tri(Tri_Coord{i, j}), i * 10 + j);
}

TEST(CATriangularLattice, AtSafeRespectsOpenBoundary)
{
  Tri_2D lat({3, 3}, 9);
  EXPECT_EQ(lat.at_tri_safe(Tri_Coord{-1, 0}), 0);
  EXPECT_EQ(lat.at_tri_safe(Tri_Coord{0, 3}), 0);
  EXPECT_EQ(lat.at_tri_safe(Tri_Coord{1, 1}), 9);
}

// ---------------------------------------------------------------------------
// Engine integration.
// ---------------------------------------------------------------------------

namespace {

// Toy rule: cell becomes alive iff it has *at least one* alive
// neighbour (parity-aware via `Triangular_Neighborhood`). Once alive
// it stays alive. This is enough to verify that the engine routes
// through the parity-dependent neighbourhood correctly.
struct Spread_Once_Functor
{
  template <typename State>
  [[nodiscard]] constexpr State operator()(const State &current,
                                           const std::size_t alive) const noexcept
  {
    return (current != State{} or alive >= 1) ? static_cast<State>(1)
                                              : static_cast<State>(0);
  }
};

}  // namespace

TEST(CATriangularEngine, SpreadFromSingleSeedReachesNeighbours)
{
  using L = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  L seed({3, 3}, 0);
  // Place the seed at an "up" triangle.
  seed.set_tri(Tri_Coord{1, 1}, 1);

  using Rule = Outer_Totalistic_Rule<Spread_Once_Functor>;
  Synchronous_Engine<L, Rule, Triangular_Neighborhood> eng(
    seed, Rule{Spread_Once_Functor{}}, Triangular_Neighborhood{});
  eng.run(1);

  // Expected three alive neighbours of (1, 1) under the up triangle
  // parity rule of `Triangular_Neighborhood`: (1,0), (1,2), (2,1).
  EXPECT_EQ(eng.frame().at_tri(Tri_Coord{1, 0}), 1);
  EXPECT_EQ(eng.frame().at_tri(Tri_Coord{1, 2}), 1);
  EXPECT_EQ(eng.frame().at_tri(Tri_Coord{2, 1}), 1);
  // The seed itself stays alive.
  EXPECT_EQ(eng.frame().at_tri(Tri_Coord{1, 1}), 1);
  // A non-edge-neighbour (different parity, no shared edge) stays dead.
  EXPECT_EQ(eng.frame().at_tri(Tri_Coord{0, 0}), 0);
}

TEST(CATriangularEngine, FullCoverageAfterEnoughSpreadSteps)
{
  using L = Triangular_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
  L seed({4, 4}, 0);
  seed.set_tri(Tri_Coord{0, 0}, 1);

  using Rule = Outer_Totalistic_Rule<Spread_Once_Functor>;
  Synchronous_Engine<L, Rule, Triangular_Neighborhood> eng(
    seed, Rule{Spread_Once_Functor{}}, Triangular_Neighborhood{});
  eng.run(20);  // ample to saturate a 4x4 connected lattice

  for (ca_index_t i = 0; i < 4; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      EXPECT_EQ(eng.frame().at_tri(Tri_Coord{i, j}), 1);
}

// ---------------------------------------------------------------------------
// TikZ sanity.
// ---------------------------------------------------------------------------

TEST(CATriangularTikz, RenderContainsTriangleNodes)
{
  Tri_2D lat({2, 2}, 0);
  lat.set_tri(Tri_Coord{0, 1}, 1);
  const std::string out = render_triangular_lattice_tikz(
    lat, [](int v) { return v != 0 ? std::string("blue") : std::string("white"); });
  EXPECT_NE(out.find("regular polygon sides=3"), std::string::npos);
  EXPECT_NE(out.find("fill=blue"), std::string::npos);
  EXPECT_NE(out.find("rotate=180"), std::string::npos);  // down triangle
  EXPECT_NE(out.find("rotate=0"), std::string::npos);    // up triangle
}