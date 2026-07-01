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
 * @file ca_io_expected_test.cc
 * @brief Tests for the non-throwing `try_read_*` parser overloads (ca-io.H).
 *
 * Each try_read_* must return a value on valid input and an error message on
 * invalid input, while the corresponding throwing reader keeps throwing.
 */

#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <ca-io.H>

using namespace Aleph;
using namespace Aleph::CA;

// --- RLE -------------------------------------------------------------------

TEST(CaIoExpected, RleSuccess)
{
  auto r = try_read_rle_string("x = 3, y = 3, rule = B3/S23\nbob$2bo$3o!\n");
  ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error());
  EXPECT_EQ(r->width, 3u);
  EXPECT_EQ(r->height, 3u);
  EXPECT_EQ(r->alive.size(), 5u);  // glider
}

TEST(CaIoExpected, RleErrorCarriesMessage)
{
  auto r = try_read_rle_string("this is not an RLE file @@@");
  ASSERT_FALSE(r.has_value());
  EXPECT_FALSE(r.error().empty());
}

TEST(CaIoExpected, RleIstreamOverload)
{
  std::istringstream in("x = 3, y = 1, rule = B3/S23\n3o!\n");
  auto r = try_read_rle(in);
  ASSERT_TRUE(r.has_value()) << (r.has_value() ? "" : r.error());
  EXPECT_EQ(r->alive.size(), 3u);
}

// --- Life ------------------------------------------------------------------

TEST(CaIoExpected, Life105SuccessAndError)
{
  std::istringstream ok("#Life 1.05\n#P 0 0\n***\n");
  auto a = try_read_life_105(ok);
  ASSERT_TRUE(a.has_value()) << (a.has_value() ? "" : a.error());
  EXPECT_EQ(a->alive.size(), 3u);

  std::istringstream bad("garbage without a header\n");
  auto b = try_read_life_105(bad);
  ASSERT_FALSE(b.has_value());
  EXPECT_FALSE(b.error().empty());
}

TEST(CaIoExpected, Life106SuccessAndError)
{
  std::istringstream ok("#Life 1.06\n0 0\n1 0\n2 0\n");
  auto a = try_read_life_106(ok);
  ASSERT_TRUE(a.has_value()) << (a.has_value() ? "" : a.error());
  EXPECT_EQ(a->alive.size(), 3u);

  std::istringstream bad("#Life 1.06\nnot two integers\n");
  auto b = try_read_life_106(bad);
  ASSERT_FALSE(b.has_value());
  EXPECT_FALSE(b.error().empty());
}

// --- CSV -------------------------------------------------------------------

TEST(CaIoExpected, CsvSuccessAndError)
{
  std::istringstream ok("0,1,0\n0,1,0\n0,1,0\n");
  auto a = try_read_csv_snapshot<int>(ok);
  ASSERT_TRUE(a.has_value()) << (a.has_value() ? "" : a.error());
  EXPECT_EQ(a->height, 3u);
  EXPECT_EQ(a->width, 3u);

  std::istringstream ragged("0,1,0\n1,1\n");  // inconsistent width
  auto b = try_read_csv_snapshot<int>(ragged);
  ASSERT_FALSE(b.has_value());
  EXPECT_FALSE(b.error().empty());
}

// --- The throwing readers are unchanged ------------------------------------

TEST(CaIoExpected, ThrowingReadersStillThrow)
{
  EXPECT_ANY_THROW((void) read_rle_string("not an RLE @@@"));

  std::istringstream bad("garbage\n");
  EXPECT_ANY_THROW((void) read_life_105(bad));
}