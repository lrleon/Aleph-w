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
 * @file tpl_ca_concepts_test.cc
 * @brief Compile-time and runtime checks for `Aleph::CA` Phase 0 concepts.
 *
 * Phase 0 only ships the concept scaffolding. These tests validate that:
 *   1. Common cell-state types satisfy `CellState`.
 *   2. `Coord_Vec<N>` satisfies `Coord<C, N>`.
 *   3. A minimal mock lattice satisfies `LatticeLike`.
 *   4. A minimal mock neighborhood satisfies `NeighborhoodLike`.
 *   5. A minimal rule satisfies `RuleLike<R, L>`.
 */

# include <array>
# include <cstdint>
# include <gtest/gtest.h>

# include <ca-traits.H>
# include <tpl_ca_concepts.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{

  // ---------------------------------------------------------------------
  // Mock types used to validate the Phase 0 concepts. These are kept
  // purposely minimal: they implement nothing beyond what the concepts
  // require, so a regression that tightens a concept will surface here
  // immediately.
  // ---------------------------------------------------------------------

  /// Minimal 2D dense lattice over `T`.
  template <typename T>
  struct Mock_Lattice
  {
    using state_type = T;
    using coord_type = Coord_Vec<2>;

    std::array<std::size_t, 2> extents { 0, 0 };
    std::array<T, 16> cells { };  // up to 4x4 fits

    std::size_t dimension() const { return 2; }

    std::size_t size(std::size_t d) const { return extents[d]; }

    T at(const coord_type & c) const
    {
      return cells[c[0] * extents[1] + c[1]];
    }

    void set(const coord_type & c, const T & v)
    {
      cells[c[0] * extents[1] + c[1]] = v;
    }
  };

  /// Minimal Moore-like neighborhood placeholder (no enumeration yet).
  struct Mock_Neighborhood
  {
    std::size_t radius() const { return 1; }
    std::size_t size()   const { return 8; }
  };

  /// Minimal rule: returns the current state unchanged.
  struct Identity_Rule
  {
    template <typename S>
    S operator()(S s) const { return s; }
  };

  // ---------------------------------------------------------------------
  // Compile-time checks. If any of these fails the file does not compile,
  // which is exactly the signal we want from the acceptance criteria.
  // ---------------------------------------------------------------------

  static_assert(CellState<bool>);
  static_assert(CellState<std::uint8_t>);
  static_assert(CellState<int>);

  static_assert(Coord<Coord_Vec<1>, 1>);
  static_assert(Coord<Coord_Vec<2>, 2>);
  static_assert(Coord<Coord_Vec<3>, 3>);

  static_assert(LatticeLike<Mock_Lattice<bool>>);
  static_assert(LatticeLike<Mock_Lattice<std::uint8_t>>);

  static_assert(NeighborhoodLike<Mock_Neighborhood>);

  static_assert(RuleLike<Identity_Rule, Mock_Lattice<bool>>);
  static_assert(RuleLike<Identity_Rule, Mock_Lattice<std::uint8_t>>);

  // Negative checks: things that must NOT satisfy the concepts.
  struct Not_A_Lattice {};
  static_assert(not LatticeLike<Not_A_Lattice>);

  struct Not_A_Neighborhood {};
  static_assert(not NeighborhoodLike<Not_A_Neighborhood>);

} // namespace


TEST(CACellularAutomataConcepts, MockLatticeReadWrite)
{
  Mock_Lattice<std::uint8_t> lat;
  lat.extents = { 4, 4 };
  for (std::size_t i = 0; i < lat.cells.size(); ++i)
    lat.cells[i] = 0;

  const Coord_Vec<2> c { 1, 2 };
  lat.set(c, 7u);
  EXPECT_EQ(lat.at(c), 7u);
  EXPECT_EQ(lat.dimension(), 2u);
  EXPECT_EQ(lat.size(0), 4u);
  EXPECT_EQ(lat.size(1), 4u);
}

TEST(CACellularAutomataConcepts, MockNeighborhoodInterface)
{
  Mock_Neighborhood h;
  EXPECT_EQ(h.radius(), 1u);
  EXPECT_EQ(h.size(), 8u);
}

TEST(CACellularAutomataConcepts, IdentityRuleProducesSameState)
{
  Identity_Rule r;
  EXPECT_EQ(r(static_cast<std::uint8_t>(42)), static_cast<std::uint8_t>(42));
  EXPECT_TRUE(r(true));
  EXPECT_FALSE(r(false));
}

TEST(CACellularAutomataConcepts, BoundaryTagsAreEmpty)
{
  // The boundary tags from ca-traits.H should be zero-sized so they can
  // be passed as template tags without runtime cost.
  EXPECT_EQ(sizeof(OpenBoundary), 1u);
  EXPECT_EQ(sizeof(ToroidalBoundary), 1u);
  EXPECT_EQ(sizeof(ReflectiveBoundary), 1u);
  EXPECT_EQ(sizeof(RowMajor), 1u);
  EXPECT_EQ(sizeof(ColumnMajor), 1u);

  using B = ConstantBoundary<std::uint8_t, 0>;
  EXPECT_EQ(B::value, 0u);
}