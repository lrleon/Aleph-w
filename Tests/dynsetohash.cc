
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
 * @file dynsetohash.cc
 * @brief Tests for Dynsetohash
 */
# include <gtest/gtest.h>

# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>

using namespace std;
using namespace testing;
using namespace Aleph;

using P = pair<size_t, string>;

template <class HashTbl>
struct OHashTest : public ::testing::Test
{
  HashTbl tbl;
};

TYPED_TEST_CASE_P(OHashTest);

TYPED_TEST_P(OHashTest, basic)
{
  TypeParam tbl = this->tbl;
  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());

  for (size_t i = 0; i < 100; ++i)
    {
      EXPECT_EQ(tbl.size(), i);
      tbl.emplace(i, to_string(i));
      EXPECT_EQ(tbl.size(), i + 1);
      auto ptr = tbl.search(i);
      EXPECT_NE(ptr, nullptr);
      EXPECT_EQ(ptr->first, i);
      EXPECT_EQ(ptr->second, to_string(i));
    }

  for (size_t i = 0, n = tbl.size(); i < n; ++i)
    {
      auto ptr = tbl.search(i);
      ASSERT_NE(ptr, nullptr);
      EXPECT_EQ(ptr->first, i);
      tbl.remove(i);
      EXPECT_EQ(tbl.size(), n - i - 1);
      EXPECT_EQ(tbl.search(i), nullptr);
      EXPECT_FALSE(tbl.contains(i));
    }
}

REGISTER_TYPED_TEST_CASE_P(OHashTest, basic);

typedef Types<MapODhash<size_t, string>,
	      MapOLhash<size_t, string>,
	      DynMapLinHash<size_t, string>,
	      DynMapHash<size_t, string>> HashTypes;

INSTANTIATE_TYPED_TEST_CASE_P(Open, OHashTest, HashTypes);


