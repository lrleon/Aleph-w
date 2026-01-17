
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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


