
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include <gmock/gmock.h>

# include <tpl_dynSetOhash.H>
# include <tpl_dynSetHash.H>

using namespace std;
using namespace testing;

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


