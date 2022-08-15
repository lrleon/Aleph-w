
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

# include <tpl_odhash.H>

TEST(ODhashTable, Simplest)
{
  ODhashTable<int> tbl(100);

  EXPECT_TRUE(tbl.is_empty());
  EXPECT_EQ(tbl.size(), 0);

  const size_t cap = tbl.capacity();
  for (size_t i = 0; i < cap; ++i)
    {
      EXPECT_EQ(tbl.size(), i);
      tbl.insert(i);
      EXPECT_EQ(tbl.size(), i + 1);
      EXPECT_FALSE(tbl.is_empty());
    }

  for (size_t i = 0; i < tbl.size(); ++i)
    {
      ASSERT_NE(tbl.search(i), nullptr);
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      EXPECT_FALSE(tbl.is_empty());
    }

  for (size_t i = 0, n = tbl.size(); i < n; ++i)
    {
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      tbl.remove(*ptr);
      EXPECT_EQ(tbl.size(), n - i - 1);
      EXPECT_EQ(tbl.search(i), nullptr);
      EXPECT_FALSE(tbl.contains(i));
    }

  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());
}

struct MyRecord
{
  size_t key;
  string value;
  MyRecord() {}
  MyRecord(size_t k, const string & v) : key(k), value(v) {}
  MyRecord(size_t k) : key(k) {}
  struct Eq
  {
    bool operator () (const MyRecord & r1, const MyRecord & r2) const noexcept
    {
      return r1.key == r2.key;
    }
  };
  bool operator == (const MyRecord & r) const noexcept { return Eq()(*this, r); }
};

inline size_t fst_hast(const MyRecord & r) noexcept
{
  return dft_hash_fct(r.key);
}

inline size_t snd_hash(const MyRecord & r) noexcept
{
  return snd_hash_fct(r.key);
}


TEST(ODhashTable, Map)
{
  ODhashTable<MyRecord, MyRecord::Eq> tbl(10, fst_hast, snd_hash, MyRecord::Eq());

  EXPECT_EQ(tbl.size(), 0);
  EXPECT_TRUE(tbl.is_empty());

  for (size_t i = 0; i < 100; ++i)
    {
      EXPECT_EQ(tbl.size(), i);
      tbl.emplace(i, to_string(i));
      EXPECT_EQ(tbl.size(), i + 1);
      auto ptr = tbl.search(MyRecord(i));
      EXPECT_NE(ptr, nullptr);
      EXPECT_EQ(ptr->key, i);
      EXPECT_EQ(ptr->value, to_string(i));
    }

  for (size_t i = 0, n = tbl.size(); i < n; ++i)
    {
      auto ptr = tbl.search(i);
      EXPECT_EQ(*ptr, i);
      tbl.remove(MyRecord(ptr->key));
      EXPECT_EQ(tbl.size(), n - i - 1);
      EXPECT_EQ(tbl.search(i), nullptr);
      EXPECT_FALSE(tbl.contains(MyRecord(i)));
    }
}
