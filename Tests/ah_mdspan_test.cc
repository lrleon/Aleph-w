/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file ah_mdspan_test.cc
 * @brief Tests for the multidimensional view type (ah-mdspan.H).
 *
 * Exercises `Aleph::mdspan` through the same surface regardless of whether
 * it resolves to `std::mdspan` (C++23) or the in-house C++20 polyfill.
 */

#include <array>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include <ah-mdspan.H>

using Aleph::dextents;
using Aleph::dynamic_extent;
using Aleph::extents;
using Aleph::layout_left;
using Aleph::layout_right;
using Aleph::mdspan;

TEST(AhMdspan, DetectionMacroIsDefined)
{
  // Must always be defined to 0 or 1, regardless of -std.
  constexpr int v = ALEPH_HAS_STD_MDSPAN;
  EXPECT_TRUE(v == 0 or v == 1);
}

TEST(AhMdspan, DextentsRankAndRankDynamic)
{
  using Ext = dextents<size_t, 3>;
  static_assert(Ext::rank() == 3);
  static_assert(Ext::rank_dynamic() == 3);
  Ext e(2, 3, 4);
  EXPECT_EQ(e.extent(0), 2u);
  EXPECT_EQ(e.extent(1), 3u);
  EXPECT_EQ(e.extent(2), 4u);
}

TEST(AhMdspan, MixedStaticAndDynamicExtents)
{
  // Axis 0 dynamic, axis 1 fixed to 4.
  using Ext = extents<size_t, dynamic_extent, 4>;
  static_assert(Ext::rank() == 2);
  static_assert(Ext::rank_dynamic() == 1);
  static_assert(Ext::static_extent(1) == 4);
  static_assert(Ext::static_extent(0) == dynamic_extent);

  Ext e(7);  // only the one dynamic axis needs a value
  EXPECT_EQ(e.extent(0), 7u);
  EXPECT_EQ(e.extent(1), 4u);
}

TEST(AhMdspan, ExtentsFromArray)
{
  std::array<size_t, 2> shape{3, 5};
  dextents<size_t, 2> e(shape);
  EXPECT_EQ(e.extent(0), 3u);
  EXPECT_EQ(e.extent(1), 5u);
}

TEST(AhMdspan, ExtentsEquality)
{
  dextents<size_t, 2> a(2, 3);
  dextents<size_t, 2> b(2, 3);
  dextents<size_t, 2> c(3, 2);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(AhMdspan, BasicConstructionAndShape)
{
  std::vector<int> buf(2 * 3, 0);
  mdspan<int, dextents<size_t, 2>> m(buf.data(), 2, 3);

  EXPECT_EQ(m.rank(), 2u);
  EXPECT_EQ(m.extent(0), 2u);
  EXPECT_EQ(m.extent(1), 3u);
  EXPECT_EQ(m.size(), 6u);
  EXPECT_FALSE(m.empty());
  EXPECT_EQ(m.data_handle(), buf.data());
}

TEST(AhMdspan, ConstructFromArrayOfDynamicExtents)
{
  std::vector<int> buf(6, 0);
  std::array<size_t, 2> shape{2, 3};
  mdspan<int, dextents<size_t, 2>> m(buf.data(), shape);
  EXPECT_EQ(m.extent(0), 2u);
  EXPECT_EQ(m.extent(1), 3u);
}

TEST(AhMdspan, RowMajorIsDefaultAndMatchesFlatBufferOrder)
{
  // layout_right (row-major): the LAST axis varies fastest, i.e. m(i, j)
  // maps to buf[i * ncols + j] -- exactly Aleph's own Array/C convention.
  std::vector<int> buf(2 * 3, 0);
  mdspan<int, dextents<size_t, 2>> m(buf.data(), 2, 3);

  int next = 0;
  for (size_t i = 0; i < m.extent(0); ++i)
    for (size_t j = 0; j < m.extent(1); ++j)
      m(i, j) = next++;

  EXPECT_EQ(buf, (std::vector<int>{0, 1, 2, 3, 4, 5}));
}

TEST(AhMdspan, LayoutLeftIsColumnMajor)
{
  // layout_left (column-major): the FIRST axis varies fastest.
  std::vector<int> buf(2 * 3, 0);
  mdspan<int, dextents<size_t, 2>, layout_left> m(buf.data(), 2, 3);

  int next = 0;
  for (size_t j = 0; j < m.extent(1); ++j)
    for (size_t i = 0; i < m.extent(0); ++i)
      m(i, j) = next++;

  EXPECT_EQ(buf, (std::vector<int>{0, 1, 2, 3, 4, 5}));
}

TEST(AhMdspan, WriteThroughViewIsVisibleThroughBuffer)
{
  std::vector<double> buf(4, 0.0);
  mdspan<double, dextents<size_t, 2>> m(buf.data(), 2, 2);
  m(0, 0) = 1.0;
  m(0, 1) = 2.0;
  m(1, 0) = 3.0;
  m(1, 1) = 4.0;
  EXPECT_EQ(buf, (std::vector<double>{1.0, 2.0, 3.0, 4.0}));
}

TEST(AhMdspan, ReadOnlyViewOverConstElement)
{
  const std::vector<int> buf{9, 8, 7, 6};
  mdspan<const int, dextents<size_t, 1>> m(buf.data(), 4);
  EXPECT_EQ(m(0), 9);
  EXPECT_EQ(m(2), 7);
  static_assert(std::is_const_v<std::remove_reference_t<decltype(m(0))>>);
}

TEST(AhMdspan, AtThrowsOnOutOfRangeIndex)
{
  std::vector<int> buf(6, 0);
  mdspan<int, dextents<size_t, 2>> m(buf.data(), 2, 3);
  EXPECT_NO_THROW((void) m.at(1, 2));
  EXPECT_THROW((void) m.at(2, 0), std::out_of_range);
  EXPECT_THROW((void) m.at(0, 3), std::out_of_range);
}

TEST(AhMdspan, OneDimensionalView)
{
  std::vector<int> buf{10, 20, 30};
  mdspan<int, dextents<size_t, 1>> m(buf.data(), 3);
  EXPECT_EQ(m.rank(), 1u);
  EXPECT_EQ(m(0), 10);
  EXPECT_EQ(m(2), 30);
}

TEST(AhMdspan, ThreeDimensionalRowMajorRoundTrip)
{
  std::vector<int> buf(2 * 2 * 2, -1);
  mdspan<int, dextents<size_t, 3>> m(buf.data(), 2, 2, 2);

  int next = 0;
  for (size_t i = 0; i < 2; ++i)
    for (size_t j = 0; j < 2; ++j)
      for (size_t k = 0; k < 2; ++k)
        m(i, j, k) = next++;

  for (int v : buf)
    EXPECT_NE(v, -1);
  EXPECT_EQ(m(1, 1, 1), 7);
  EXPECT_EQ(buf.back(), 7);
}

TEST(AhMdspan, DefaultConstructedIsEmptyWithNullData)
{
  mdspan<int, dextents<size_t, 1>> m;
  EXPECT_EQ(m.data_handle(), nullptr);
  EXPECT_TRUE(m.empty());
  EXPECT_EQ(m.size(), 0u);
}

TEST(AhMdspan, MultipleIndependentViewsOverSameBufferDoNotInterfere)
{
  std::vector<int> buf(4, 0);
  mdspan<int, dextents<size_t, 2>> m1(buf.data(), 2, 2);
  mdspan<int, dextents<size_t, 2>> m2(buf.data(), 2, 2);
  m1(0, 0) = 42;
  EXPECT_EQ(m2(0, 0), 42);  // same underlying buffer: aliasing is intentional
}

TEST(AhMdspan, MutableViewConvertsImplicitlyToConstView)
{
  // Mirrors T* -> const T* implicit conversion.
  int buf[4] = {1, 2, 3, 4};
  mdspan<int, dextents<size_t, 2>> m(buf, 2, 2);

  auto reader = [](mdspan<const int, dextents<size_t, 2>> v) { return v(1, 1); };
  EXPECT_EQ(reader(m), 4);
}