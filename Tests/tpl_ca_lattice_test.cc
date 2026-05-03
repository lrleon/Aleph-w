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
 * @file tpl_ca_lattice_test.cc
 * @brief Boundary-policy tests for `Aleph::CA::Lattice<Storage, Boundary>`.
 *
 * For each policy (`OpenBoundary`, `ToroidalBoundary`, `ReflectiveBoundary`,
 * `ConstantBoundary<T, V>`) we verify that `at_safe(coord)` returns the
 * expected value at the borders, the corners, and outside the lattice.
 */

# include <cstdint>
# include <stdexcept>

# include <gtest/gtest.h>

# include <ca-traits.H>
# include <tpl_ca_concepts.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_bit_storage.H>
# include <tpl_ca_lattice.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  /// Build a 3x3 lattice whose interior cells encode i*10 + j so we can
  /// recognise from which cell the boundary policy mapped any coordinate.
  template <typename Boundary>
  Lattice<Dense_Cell_Storage<int, 2>, Boundary>
  make_3x3_int_lattice()
  {
    Lattice<Dense_Cell_Storage<int, 2>, Boundary> lat({ 3, 3 }, 0);
    for (ca_index_t i = 0; i < 3; ++i)
      for (ca_index_t j = 0; j < 3; ++j)
        lat.set({ i, j }, static_cast<int>(i * 10 + j));
    return lat;
  }
}

// ---------------------------------------------------------------------------
// Concept conformance: a fully-formed Lattice still satisfies LatticeLike.
// ---------------------------------------------------------------------------

static_assert(LatticeLike<Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary>>);
static_assert(LatticeLike<Lattice<Dense_Cell_Storage<int, 2>,
                                  ToroidalBoundary>>);
static_assert(LatticeLike<Lattice<Bit_Cell_Storage<2>, ReflectiveBoundary>>);

// ---------------------------------------------------------------------------
// OpenBoundary: out-of-range cells read as state_type{} (zero).
// ---------------------------------------------------------------------------

TEST(CALatticeOpenBoundary, InRangeMatchesStorage)
{
  auto lat = make_3x3_int_lattice<OpenBoundary>();
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      EXPECT_EQ(lat.at_safe({ i, j }), i * 10 + j);
}

TEST(CALatticeOpenBoundary, OutOfRangeReturnsZero)
{
  auto lat = make_3x3_int_lattice<OpenBoundary>();
  EXPECT_EQ(lat.at_safe({ -1, 0 }), 0);
  EXPECT_EQ(lat.at_safe({ 0, -1 }), 0);
  EXPECT_EQ(lat.at_safe({ 3,  0 }), 0);
  EXPECT_EQ(lat.at_safe({ 0,  3 }), 0);
  EXPECT_EQ(lat.at_safe({ -1, -1 }), 0);
  EXPECT_EQ(lat.at_safe({  3,  3 }), 0);
}

// ---------------------------------------------------------------------------
// ToroidalBoundary: out-of-range cells wrap around on every axis.
// ---------------------------------------------------------------------------

TEST(CALatticeToroidalBoundary, WrapsOnEachAxis)
{
  auto lat = make_3x3_int_lattice<ToroidalBoundary>();

  // Single-axis wrap: -1 → 2 on i.
  EXPECT_EQ(lat.at_safe({ -1, 1 }), lat.at({ 2, 1 }));
  // Single-axis wrap: 3 → 0 on j.
  EXPECT_EQ(lat.at_safe({ 1,  3 }), lat.at({ 1, 0 }));
  // Both axes wrap simultaneously (corner).
  EXPECT_EQ(lat.at_safe({ -1, -1 }), lat.at({ 2, 2 }));
  EXPECT_EQ(lat.at_safe({  3,  3 }), lat.at({ 0, 0 }));
  // Multi-period wrap: -4 ≡ -1 (mod 3) ≡ 2.
  EXPECT_EQ(lat.at_safe({ -4, 1 }), lat.at({ 2, 1 }));
  EXPECT_EQ(lat.at_safe({  7, 1 }), lat.at({ 1, 1 }));
}

// ---------------------------------------------------------------------------
// ReflectiveBoundary: -1 → 0, -2 → 1, n → n-1, n+1 → n-2.
// ---------------------------------------------------------------------------

TEST(CALatticeReflectiveBoundary, ReflectIntoRangeCoversBoundaryAndPeriods)
{
  using ca_lattice_detail::reflect_into_range;

  EXPECT_EQ(reflect_into_range(-1, 3), 0u);
  EXPECT_EQ(reflect_into_range(-2, 3), 1u);
  EXPECT_EQ(reflect_into_range(3, 3), 2u);
  EXPECT_EQ(reflect_into_range(4, 3), 1u);
  EXPECT_EQ(reflect_into_range(-3, 3), 2u);
  EXPECT_EQ(reflect_into_range(5, 3), 0u);
  EXPECT_EQ(reflect_into_range(6, 3), 0u);
  EXPECT_EQ(reflect_into_range(-7, 3), 0u);

  EXPECT_EQ(reflect_into_range(-1, 5), 0u);
  EXPECT_EQ(reflect_into_range(-2, 5), 1u);
  EXPECT_EQ(reflect_into_range(5, 5), 4u);
  EXPECT_EQ(reflect_into_range(6, 5), 3u);
  EXPECT_EQ(reflect_into_range(19, 5), 0u);
  EXPECT_EQ(reflect_into_range(-19, 5), 1u);
}

TEST(CALatticeReflectiveBoundary, MirrorsAtEachEdge)
{
  auto lat = make_3x3_int_lattice<ReflectiveBoundary>();

  EXPECT_EQ(lat.at_safe({ -1, 1 }), lat.at({ 0, 1 }));
  EXPECT_EQ(lat.at_safe({ -2, 1 }), lat.at({ 1, 1 }));
  EXPECT_EQ(lat.at_safe({  3, 1 }), lat.at({ 2, 1 }));
  EXPECT_EQ(lat.at_safe({  4, 1 }), lat.at({ 1, 1 }));
  EXPECT_EQ(lat.at_safe({  1, -1 }), lat.at({ 1, 0 }));
  EXPECT_EQ(lat.at_safe({  1,  3 }), lat.at({ 1, 2 }));
}

TEST(CALatticeReflectiveBoundary, MirrorsAtCorners)
{
  auto lat = make_3x3_int_lattice<ReflectiveBoundary>();

  EXPECT_EQ(lat.at_safe({ -1, -1 }), lat.at({ 0, 0 }));
  EXPECT_EQ(lat.at_safe({ -1,  3 }), lat.at({ 0, 2 }));
  EXPECT_EQ(lat.at_safe({  3, -1 }), lat.at({ 2, 0 }));
  EXPECT_EQ(lat.at_safe({  3,  3 }), lat.at({ 2, 2 }));
}

// ---------------------------------------------------------------------------
// ConstantBoundary<T, V>: out-of-range cells read as V.
// ---------------------------------------------------------------------------

TEST(CALatticeConstantBoundary, OutOfRangeReturnsConstantValue)
{
  using Boundary = ConstantBoundary<int, 99>;
  Lattice<Dense_Cell_Storage<int, 2>, Boundary> lat({ 3, 3 }, 0);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      lat.set({ i, j }, static_cast<int>(i * 10 + j));

  EXPECT_EQ(lat.at_safe({  0,  0 }), 0);
  EXPECT_EQ(lat.at_safe({ -1,  0 }), 99);
  EXPECT_EQ(lat.at_safe({  0,  3 }), 99);
  EXPECT_EQ(lat.at_safe({ -2, -2 }), 99);
}

TEST(CALatticeConstantBoundary, BitLatticeWithConstantTrue)
{
  using Boundary = ConstantBoundary<bool, true>;
  Lattice<Bit_Cell_Storage<2>, Boundary> lat({ 3, 3 }, false);
  // Inner cells are false, outside cells must read true.
  EXPECT_FALSE(lat.at_safe({ 0, 0 }));
  EXPECT_TRUE(lat.at_safe({ -1, 0 }));
  EXPECT_TRUE(lat.at_safe({  3, 3 }));
}

// ---------------------------------------------------------------------------
// Common surface: extents, swap, fill, strict at/set semantics.
// ---------------------------------------------------------------------------

TEST(CALattice, ExtentsAndDimensionForwardToStorage)
{
  Lattice<Dense_Cell_Storage<int, 3>, ToroidalBoundary> lat({ 2, 3, 4 });
  EXPECT_EQ(lat.dimension(), 3u);
  EXPECT_EQ(lat.size(), 24u);
  EXPECT_EQ(lat.size(0), 2u);
  EXPECT_EQ(lat.size(1), 3u);
  EXPECT_EQ(lat.size(2), 4u);
  EXPECT_EQ(lat.extents()[2], 4u);
}

TEST(CALattice, FillResetsEveryCell)
{
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> lat({ 3, 3 }, 0);
  lat.set({ 1, 1 }, 7);
  lat.fill(42);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      EXPECT_EQ(lat.at({ i, j }), 42);
}

TEST(CALattice, SwapExchangesContents)
{
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> a({ 2, 2 }, 1);
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> b({ 3, 3 }, 9);
  a.swap(b);
  EXPECT_EQ(a.size(0), 3u);
  EXPECT_EQ(b.size(0), 2u);
  EXPECT_EQ(a.at({ 0, 0 }), 9);
  EXPECT_EQ(b.at({ 0, 0 }), 1);
}

TEST(CALattice, StrictAtThrowsOnOutOfRange)
{
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> lat({ 3, 3 }, 0);
  EXPECT_THROW(lat.at({ -1, 0 }), std::out_of_range);
  EXPECT_THROW(lat.set({ 3, 0 }, 1), std::out_of_range);
}

TEST(CALattice, EmptyExtentsHandlesZeroAxis)
{
  Lattice<Dense_Cell_Storage<int, 2>, OpenBoundary> lat({ 0, 3 }, 0);
  EXPECT_EQ(lat.dimension(), 2u);
  EXPECT_EQ(lat.size(), 0u);
  EXPECT_EQ(lat.size(0), 0u);
  EXPECT_EQ(lat.size(1), 3u);
  EXPECT_THROW(lat.at({ 0, 0 }), std::out_of_range);
  EXPECT_THROW(lat.set({ 0, 0 }, 1), std::out_of_range);
  EXPECT_NO_THROW((void) lat.at_safe({ 0, 0 }));
  EXPECT_EQ(lat.at_safe({ 0, 0 }), 0);

  Lattice<Dense_Cell_Storage<int, 3>, OpenBoundary> cube({ 0, 0, 2 }, 0);
  EXPECT_EQ(cube.dimension(), 3u);
  EXPECT_EQ(cube.size(), 0u);
  EXPECT_EQ(cube.size(0), 0u);
  EXPECT_EQ(cube.size(1), 0u);
  EXPECT_THROW(cube.at({ 0, 0, 0 }), std::out_of_range);
  EXPECT_NO_THROW((void) cube.at_safe({ 0, 0, 0 }));
}
