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
 * @file tpl_ca_storage_test.cc
 * @brief Unit tests for Phase 1 cell storages
 *        (`Dense_Cell_Storage`, `Bit_Cell_Storage`).
 *
 * Verifies shape, indexing, copy/move semantics, swap and fill, plus a
 * pair of `static_assert` checks confirming both storages still satisfy
 * the Phase 0 `LatticeLike` concept.
 */

# include <cstdint>
# include <limits>
# include <stdexcept>
# include <utility>

# include <gtest/gtest.h>

# include <tpl_ca_concepts.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_bit_storage.H>

using namespace Aleph;
using namespace Aleph::CA;

// ---------------------------------------------------------------------------
// Concept conformance — these are compile-time checks; if any fails the
// test binary will not link.
// ---------------------------------------------------------------------------

static_assert(LatticeLike<Dense_Cell_Storage<std::uint8_t, 1>>);
static_assert(LatticeLike<Dense_Cell_Storage<std::uint8_t, 2>>);
static_assert(LatticeLike<Dense_Cell_Storage<std::uint8_t, 3>>);
static_assert(LatticeLike<Bit_Cell_Storage<1>>);
static_assert(LatticeLike<Bit_Cell_Storage<2>>);
static_assert(LatticeLike<Bit_Cell_Storage<3>>);

// ---------------------------------------------------------------------------
// Dense_Cell_Storage
// ---------------------------------------------------------------------------

TEST(CADenseCellStorage, DefaultConstructedIsEmpty)
{
  Dense_Cell_Storage<std::uint8_t, 2> s;
  EXPECT_EQ(s.size(), 0u);
  EXPECT_EQ(s.dimension(), 2u);
  EXPECT_EQ(s.extents()[0], 0u);
  EXPECT_EQ(s.extents()[1], 0u);
  EXPECT_EQ(s.data(), nullptr);
}

TEST(CADenseCellStorage, ConstructorAllocatesAndInitialises)
{
  Dense_Cell_Storage<int, 2> s({ 3, 4 }, 7);
  EXPECT_EQ(s.size(), 12u);
  EXPECT_EQ(s.size(0), 3u);
  EXPECT_EQ(s.size(1), 4u);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      EXPECT_EQ(s.at(i, j), 7);
}

TEST(CADenseCellStorage, RowMajorLinearIndex)
{
  Dense_Cell_Storage<int, 2> s({ 3, 4 });
  // Row-major: idx = i*4 + j
  EXPECT_EQ(s.linear_index({ 0, 0 }), 0u);
  EXPECT_EQ(s.linear_index({ 0, 3 }), 3u);
  EXPECT_EQ(s.linear_index({ 1, 0 }), 4u);
  EXPECT_EQ(s.linear_index({ 2, 3 }), 11u);
}

TEST(CADenseCellStorage, SetAndAtRoundTrip2D)
{
  Dense_Cell_Storage<int, 2> s({ 3, 4 }, 0);
  s.set(1, 2, 42);
  EXPECT_EQ(s.at(1, 2), 42);
  EXPECT_EQ(s.at(0, 0), 0);
  // Underlying buffer respects row-major:
  EXPECT_EQ(s.data()[1 * 4 + 2], 42);
}

TEST(CADenseCellStorage, SetAndAt3D)
{
  Dense_Cell_Storage<std::uint8_t, 3> s({ 2, 2, 2 }, 0);
  s.set(1, 0, 1, 9);
  s.set(0, 1, 1, 5);
  EXPECT_EQ(s.at(1, 0, 1), 9);
  EXPECT_EQ(s.at(0, 1, 1), 5);
  EXPECT_EQ(s.size(), 8u);
}

TEST(CADenseCellStorage, OneDimensional)
{
  Dense_Cell_Storage<int, 1> s({ 5 }, -1);
  for (ca_index_t i = 0; i < 5; ++i)
    EXPECT_EQ(s.at(i), -1);
  s.set(2, 100);
  EXPECT_EQ(s.at(2), 100);
}

TEST(CADenseCellStorage, OutOfRangeThrows)
{
  Dense_Cell_Storage<int, 2> s({ 3, 4 }, 0);
  EXPECT_THROW(s.at(3, 0), std::out_of_range);
  EXPECT_THROW(s.at(0, 4), std::out_of_range);
  EXPECT_THROW(s.at(-1, 0), std::out_of_range);
  EXPECT_THROW(s.at(0, -1), std::out_of_range);
  EXPECT_THROW(s.set({ -1, 0 }, 1), std::out_of_range);
  EXPECT_THROW(s.size(2), std::out_of_range);
}

TEST(CADenseCellStorage, FillResetsAllCells)
{
  Dense_Cell_Storage<int, 2> s({ 3, 3 }, 0);
  s.set(0, 0, 1); s.set(1, 1, 2); s.set(2, 2, 3);
  s.fill(99);
  for (ca_index_t i = 0; i < 3; ++i)
    for (ca_index_t j = 0; j < 3; ++j)
      EXPECT_EQ(s.at(i, j), 99);
}

TEST(CADenseCellStorage, SwapIsConstantTimeAndExchanges)
{
  Dense_Cell_Storage<int, 2> a({ 3, 3 }, 1);
  Dense_Cell_Storage<int, 2> b({ 4, 4 }, 2);
  const int * pa = a.data();
  const int * pb = b.data();

  a.swap(b);

  EXPECT_EQ(a.size(0), 4u);
  EXPECT_EQ(a.size(1), 4u);
  EXPECT_EQ(b.size(0), 3u);
  EXPECT_EQ(b.size(1), 3u);
  // Pointers should follow the data, not the object slot.
  EXPECT_EQ(a.data(), pb);
  EXPECT_EQ(b.data(), pa);
  EXPECT_EQ(a.at(0, 0), 2);
  EXPECT_EQ(b.at(0, 0), 1);
}

TEST(CADenseCellStorage, FreeFunctionSwap)
{
  Dense_Cell_Storage<int, 2> a({ 2, 2 }, 1);
  Dense_Cell_Storage<int, 2> b({ 2, 2 }, 9);
  swap(a, b);
  EXPECT_EQ(a.at(0, 0), 9);
  EXPECT_EQ(b.at(0, 0), 1);
}

TEST(CADenseCellStorage, MoveTransfersOwnership)
{
  Dense_Cell_Storage<int, 2> a({ 3, 3 }, 4);
  const int * p = a.data();
  Dense_Cell_Storage<int, 2> b(std::move(a));
  EXPECT_EQ(b.size(), 9u);
  EXPECT_EQ(b.data(), p);
  EXPECT_EQ(b.at(0, 0), 4);
}

// ---------------------------------------------------------------------------
// Bit_Cell_Storage
// ---------------------------------------------------------------------------

TEST(CABitCellStorage, DefaultConstructedIsEmpty)
{
  Bit_Cell_Storage<2> s;
  EXPECT_EQ(s.size(), 0u);
  EXPECT_EQ(s.dimension(), 2u);
}

TEST(CABitCellStorage, ConstructorInitialisesValue)
{
  Bit_Cell_Storage<2> s({ 5, 7 }, true);
  EXPECT_EQ(s.size(), 35u);
  for (ca_index_t i = 0; i < 5; ++i)
    for (ca_index_t j = 0; j < 7; ++j)
      EXPECT_TRUE(s.at(i, j));
}

TEST(CABitCellStorage, SetAndAtRoundTrip)
{
  Bit_Cell_Storage<2> s({ 4, 4 }, false);
  s.set(1, 2, true);
  s.set(3, 3, true);
  for (ca_index_t i = 0; i < 4; ++i)
    for (ca_index_t j = 0; j < 4; ++j)
      EXPECT_EQ(s.at(i, j),
                (i == 1 and j == 2) or (i == 3 and j == 3));
}

TEST(CABitCellStorage, RowMajorLinearIndex)
{
  Bit_Cell_Storage<2> s({ 3, 5 }, false);
  EXPECT_EQ(s.linear_index({ 0, 0 }), 0u);
  EXPECT_EQ(s.linear_index({ 0, 4 }), 4u);
  EXPECT_EQ(s.linear_index({ 2, 4 }), 14u);
}

TEST(CABitCellStorage, FillToggles)
{
  Bit_Cell_Storage<1> s({ 17 }, false);  // not byte-aligned
  s.fill(true);
  for (ca_index_t i = 0; i < 17; ++i)
    EXPECT_TRUE(s.at(i));
  s.fill(false);
  for (ca_index_t i = 0; i < 17; ++i)
    EXPECT_FALSE(s.at(i));
}

TEST(CABitCellStorage, SwapExchangesContentsAndExtents)
{
  Bit_Cell_Storage<2> a({ 3, 3 }, true);
  Bit_Cell_Storage<2> b({ 4, 4 }, false);
  a.swap(b);
  EXPECT_EQ(a.size(0), 4u);
  EXPECT_EQ(b.size(0), 3u);
  EXPECT_FALSE(a.at(0, 0));
  EXPECT_TRUE(b.at(0, 0));
}

TEST(CABitCellStorage, OutOfRangeThrows)
{
  Bit_Cell_Storage<2> s({ 3, 3 }, false);
  EXPECT_THROW(s.at(3, 0), std::out_of_range);
  EXPECT_THROW(s.at(-1, 0), std::out_of_range);
  EXPECT_THROW(s.set({ 0, 3 }, true), std::out_of_range);
}

TEST(CABitCellStorage, ThreeDimensional)
{
  Bit_Cell_Storage<3> s({ 2, 2, 2 }, false);
  s.set(1, 1, 1, true);
  EXPECT_TRUE(s.at(1, 1, 1));
  EXPECT_FALSE(s.at(0, 0, 0));
  EXPECT_EQ(s.size(), 8u);
}

TEST(CABitCellStorage, ExtentsProductOverflowThrows)
{
  constexpr ca_size_t max = std::numeric_limits<ca_size_t>::max();
  EXPECT_THROW((Bit_Cell_Storage<2>({ max, 2 }, false)),
               std::overflow_error);
}
