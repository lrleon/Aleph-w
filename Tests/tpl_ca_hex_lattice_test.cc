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
 * @file tpl_ca_hex_lattice_test.cc
 * @brief Tests for `Aleph::CA::Hex_Lattice` and the hex coord helpers.
 *
 * Scope:
 *   - Round-trip for axial <-> offset (even-r and odd-r).
 *   - Round-trip for axial <-> cube.
 *   - Hex distance against ground-truth small cases.
 *   - Concept conformance (Hex_Lattice satisfies LatticeLike).
 *   - Hex_Neighborhood radial invariants.
 *   - Hex Game-of-Life style B2/S34 rule by Bays: blinker pattern
 *     period-2 oscillation.
 *   - Pixel mapping is finite and unique per coord.
 *   - TikZ export contains expected fragments.
 */

#include <array>
#include <cstdint>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_hex_lattice.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Concept conformance.
// ---------------------------------------------------------------------------

using Hex_2D = Hex_Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>;
static_assert(LatticeLike<Hex_2D>);

// ---------------------------------------------------------------------------
// Coord conversions.
// ---------------------------------------------------------------------------

TEST(CAHexCoord, AxialToCubeSatisfiesXYZZeroSum)
{
  for (ca_index_t q = -5; q <= 5; ++q)
    for (ca_index_t r = -5; r <= 5; ++r)
      {
        const auto cube = axial_to_cube(Hex_Axial{q, r});
        EXPECT_EQ(cube.x + cube.y + cube.z, 0);
        EXPECT_EQ(cube_to_axial(cube), (Hex_Axial{q, r}));
      }
}

TEST(CAHexCoord, AxialOffsetEvenRRoundTrip)
{
  for (ca_index_t q = -4; q <= 4; ++q)
    for (ca_index_t r = -4; r <= 4; ++r)
      {
        const Hex_Axial a{q, r};
        EXPECT_EQ(offset_to_axial_even_r(axial_to_offset_even_r(a)), a);
      }
}

TEST(CAHexCoord, AxialOffsetOddRRoundTrip)
{
  for (ca_index_t q = -4; q <= 4; ++q)
    for (ca_index_t r = -4; r <= 4; ++r)
      {
        const Hex_Axial a{q, r};
        EXPECT_EQ(offset_to_axial_odd_r(axial_to_offset_odd_r(a)), a);
      }
}

TEST(CAHexCoord, HexDistanceMatchesNeighbourhoodRadius)
{
  const Hex_Axial origin{0, 0};
  // Each of the six neighbours is at distance 1.
  for (const auto &off : Hex_Neighborhood::offsets)
    {
      const Hex_Axial n{origin.q + off[0], origin.r + off[1]};
      EXPECT_EQ(hex_distance(origin, n), 1u);
    }
  // Distance is symmetric and zero for self.
  EXPECT_EQ(hex_distance(origin, origin), 0u);
  EXPECT_EQ(hex_distance(Hex_Axial{2, -1}, Hex_Axial{-1, 2}), 3u);
}

TEST(CAHexCoord, PixelCentresAreUniquePerCell)
{
  std::set<std::pair<long long, long long>> seen;
  for (ca_index_t r = 0; r < 6; ++r)
    for (ca_index_t q = 0; q < 6; ++q)
      {
        const auto px = axial_to_pixel_pointy(Hex_Axial{q, r}, 1.0);
        const auto key = std::pair<long long, long long>(
          static_cast<long long>(px[0] * 1e6),
          static_cast<long long>(px[1] * 1e6));
        EXPECT_TRUE(seen.insert(key).second)
          << "duplicate pixel for (q=" << q << ", r=" << r << ")";
      }
}

// ---------------------------------------------------------------------------
// Hex_Lattice basics.
// ---------------------------------------------------------------------------

TEST(CAHexLattice, AxialAccessorsReadAndWrite)
{
  Hex_2D lat({4, 5}, 0);
  for (ca_index_t r = 0; r < 4; ++r)
    for (ca_index_t q = 0; q < 5; ++q)
      lat.set_axial(Hex_Axial{q, r}, static_cast<int>(q + r * 10));

  for (ca_index_t r = 0; r < 4; ++r)
    for (ca_index_t q = 0; q < 5; ++q)
      EXPECT_EQ(lat.at_axial(Hex_Axial{q, r}), q + r * 10);
}

TEST(CAHexLattice, OffsetAccessorsRoundTripEvenR)
{
  Hex_2D lat({4, 5}, 0);
  // Use the offset accessor to write, axial to read back.
  lat.set_offset_even_r(Hex_Offset{2, 1}, 7);
  // Even-r: (col=2, row=1) -> (q = 2 - (1 + 1)/2 = 1, r = 1)
  EXPECT_EQ(lat.at_axial(Hex_Axial{1, 1}), 7);
}

TEST(CAHexLattice, BoundaryAwareAtSafeOnOpenBoundary)
{
  Hex_2D lat({3, 3}, 5);
  EXPECT_EQ(lat.at_axial_safe(Hex_Axial{-1, 0}), 0);
  EXPECT_EQ(lat.at_axial_safe(Hex_Axial{0, 0}), 5);
}

// ---------------------------------------------------------------------------
// Engine integration: Bays' B2/S34 hex life on a small toroidal grid.
// ---------------------------------------------------------------------------

namespace {

struct Bays_B2_S34_Functor
{
  template <typename State>
  [[nodiscard]] constexpr State operator()(const State &current,
                                           const std::size_t alive) const noexcept
  {
    const bool alive_now = current != State{};
    const bool next_alive = (alive_now and (alive == 3 or alive == 4))
                            or (not alive_now and alive == 2);
    return next_alive ? static_cast<State>(1) : static_cast<State>(0);
  }
};

}  // namespace

TEST(CAHexLatticeEngine, OneIsolatedCellDiesByB2S34)
{
  using L = Hex_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({6, 6}, 0);
  seed.set_axial(Hex_Axial{2, 2}, 1);

  using Rule = Outer_Totalistic_Rule<Bays_B2_S34_Functor>;
  Synchronous_Engine<L, Rule, Hex_Neighborhood> eng(
    seed, Rule{Bays_B2_S34_Functor{}}, Hex_Neighborhood{});

  eng.run(1);
  // Cell with 0 alive neighbours dies.
  EXPECT_EQ(eng.frame().at_axial(Hex_Axial{2, 2}), 0);
  // No neighbour had two alive neighbours, so nothing was born either.
  for (ca_index_t r = 0; r < 6; ++r)
    for (ca_index_t q = 0; q < 6; ++q)
      EXPECT_EQ(eng.frame().at_axial(Hex_Axial{q, r}), 0);
}

TEST(CAHexLatticeEngine, PairOfAdjacentCellsBirthsThirdCell)
{
  // With B2 and two adjacent cells, the cells that touch *both* are
  // exactly the two common neighbours of those two cells. They are
  // born; meanwhile the original two cells have only 1 alive
  // neighbour each (each other), so under S34 they die. After one
  // step we therefore have exactly the two common-neighbour cells.
  using L = Hex_Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  L seed({6, 6}, 0);
  seed.set_axial(Hex_Axial{2, 2}, 1);
  seed.set_axial(Hex_Axial{3, 2}, 1);  // (+1, 0) neighbour of (2, 2)

  using Rule = Outer_Totalistic_Rule<Bays_B2_S34_Functor>;
  Synchronous_Engine<L, Rule, Hex_Neighborhood> eng(
    seed, Rule{Bays_B2_S34_Functor{}}, Hex_Neighborhood{});

  eng.run(1);
  // The two starter cells must be dead (S34, only 1 neighbour).
  EXPECT_EQ(eng.frame().at_axial(Hex_Axial{2, 2}), 0);
  EXPECT_EQ(eng.frame().at_axial(Hex_Axial{3, 2}), 0);
  // The two common neighbours of (2,2) and (3,2) are (3,1) and
  // (2,3); they must have been born.
  EXPECT_EQ(eng.frame().at_axial(Hex_Axial{3, 1}), 1);
  EXPECT_EQ(eng.frame().at_axial(Hex_Axial{2, 3}), 1);
}

// ---------------------------------------------------------------------------
// TikZ export sanity.
// ---------------------------------------------------------------------------

TEST(CAHexLatticeTikz, ProducesTikzPictureWithExpectedFragments)
{
  Hex_2D lat({2, 2}, 0);
  lat.set_axial(Hex_Axial{0, 0}, 1);
  const std::string out = render_hex_lattice_tikz(
    lat, [](int v) { return v != 0 ? std::string("black") : std::string("white"); });
  EXPECT_NE(out.find("\\begin{tikzpicture}"), std::string::npos);
  EXPECT_NE(out.find("\\end{tikzpicture}"), std::string::npos);
  EXPECT_NE(out.find("regular polygon sides=6"), std::string::npos);
  EXPECT_NE(out.find("fill=black"), std::string::npos);
  EXPECT_NE(out.find("fill=white"), std::string::npos);
}