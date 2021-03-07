
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

# include <ahFunctional.H>
# include <ahSort.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>

using namespace std;
using namespace testing;

using P = pair<size_t, string>;

constexpr size_t N = 1000;

template <class HashTbl>
struct OHashTest : public ::testing::Test
{
  HashTbl tbl;
  DynList<size_t> items;
  OHashTest() : items(range(size_t(0), N - 1))
  {
    items.for_each([this] (auto i) { tbl.emplace(i, to_string(i)); });
  }
};

template <class HashTbl>
struct EmptyOHashTest : public ::testing::Test
{
  HashTbl tbl;
};

TYPED_TEST_CASE_P(OHashTest);

TYPED_TEST_CASE_P(EmptyOHashTest);

TYPED_TEST_P(EmptyOHashTest, With_exception)
{
  TypeParam tbl = this->tbl;
  auto it = tbl.get_it();
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), std::overflow_error);
  ASSERT_THROW(it.next(), std::overflow_error);
}

TYPED_TEST_P(OHashTest, Without_exception)
{
  TypeParam tbl = this->tbl;

  ASSERT_EQ(tbl.size(), N);

  auto it = tbl.get_it();
  EXPECT_NO_THROW(it.get_curr());
  EXPECT_NO_THROW(it.next());
  EXPECT_NO_THROW(it.reset_first());
  DynList<P> l;
  for (; it.has_curr(); it.next_ne())
    l.append(it.get_curr_ne());
  EXPECT_FALSE(it.has_curr());
  auto ll = l.maps<size_t>([] (auto & p) { return p.first; });
  EXPECT_EQ(sort(ll), this->items);

  l.empty();
  EXPECT_NO_THROW(it.reset_last());
  for (; it.has_curr(); it.prev_ne())
    l.append(it.get_curr_ne());
  ll = l.maps<size_t>([] (auto & p) { return p.first; });
  EXPECT_EQ(sort(ll), this->items);
}

REGISTER_TYPED_TEST_CASE_P(EmptyOHashTest, With_exception);
REGISTER_TYPED_TEST_CASE_P(OHashTest, Without_exception);

typedef Types<MapODhash<size_t, string>,
              MapOLhash<size_t, string>,
              DynMapLinHash<size_t, string>,
              DynMapHash<size_t, string>> HashTypes;

INSTANTIATE_TYPED_TEST_CASE_P(Empty, EmptyOHashTest, HashTypes);
INSTANTIATE_TYPED_TEST_CASE_P(NoEmpty, OHashTest, HashTypes);


