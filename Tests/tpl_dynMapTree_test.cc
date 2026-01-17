
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
 * @file tpl_dynMapTree_test.cc
 * @brief Tests for Tpl Dynmaptree
 */

#include <gtest/gtest.h>

#include <tpl_dynMapTree.H>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Aleph;

namespace
{
DynMapTree<int, std::string> make_sample()
{
  DynMapTree<int, std::string> m;
  m.insert(1, "one");
  m.insert(2, "two");
  m.insert(3, "three");
  return m;
}

struct ReverseInt
{
  bool operator()(int a, int b) const noexcept { return a > b; }
};
} // namespace

TEST(DynMapTree, DefaultConstruction)
{
  DynMapTree<int, int> m;
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0u);
}

TEST(DynMapTree, ConstructFromKeys)
{
  DynList<int> keys;
  keys.append(3);
  keys.append(1);
  keys.append(2);

  DynMapTree<int, int> m(keys);
  EXPECT_EQ(m.size(), 3u);
  EXPECT_TRUE(m.has(1));
  EXPECT_TRUE(m.has(2));
  EXPECT_TRUE(m.has(3));

  // Default-constructed values.
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 0);
  EXPECT_EQ(m[3], 0);
}

TEST(DynMapTree, InsertReturnsNullptrOnDuplicate)
{
  DynMapTree<int, int> m;
  auto * p1 = m.insert(1, 10);
  ASSERT_NE(p1, nullptr);
  auto * p2 = m.insert(1, 20);
  EXPECT_EQ(p2, nullptr);
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.find(1), 10);
}

TEST(DynMapTree, AppendAndPutAreAliases)
{
  DynMapTree<int, int> m;
  EXPECT_NE(m.append(1, 10), nullptr);
  EXPECT_EQ(m.put(2, 20)->second, 20);
  EXPECT_EQ(m.size(), 2u);
}

TEST(DynMapTree, SearchHasContains)
{
  auto m = make_sample();

  auto * p = m.search(2);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(p->second, "two");

  EXPECT_TRUE(m.has(1));
  EXPECT_TRUE(m.contains(3));
  EXPECT_FALSE(m.has(99));
}

TEST(DynMapTree, FindThrowsOnMissing)
{
  DynMapTree<int, int> m;
  EXPECT_THROW((void)m.find(1), std::domain_error);
}

TEST(DynMapTree, OperatorBracketsInsertsDefault)
{
  DynMapTree<int, int> m;
  int & v = m[42];
  EXPECT_EQ(v, 0);
  v = 7;
  EXPECT_EQ(m.find(42), 7);
  EXPECT_EQ(m.size(), 1u);
}

TEST(DynMapTree, ConstOperatorBracketsThrowsOnMissing)
{
  DynMapTree<int, int> m;
  const auto & cm = m;
  EXPECT_THROW((void)cm[1], std::domain_error);
}

TEST(DynMapTree, RemoveReturnsData)
{
  auto m = make_sample();
  EXPECT_EQ(m.size(), 3u);

  const auto removed = m.remove(2);
  EXPECT_EQ(removed, "two");
  EXPECT_FALSE(m.has(2));
  EXPECT_EQ(m.size(), 2u);
}

TEST(DynMapTree, RemoveKeyThrowsOnMissing)
{
  DynMapTree<int, int> m;
  EXPECT_THROW(m.remove_key(1), std::domain_error);
}

TEST(DynMapTree, KeysAndValues)
{
  auto m = make_sample();

  auto keys = m.keys();
  auto values = m.values();

  EXPECT_EQ(keys.size(), 3u);
  EXPECT_EQ(values.size(), 3u);
}

TEST(DynMapTree, ValuesPtrAndItemsPtr)
{
  auto m = make_sample();

  auto vp = m.values_ptr();
  auto ip = m.items_ptr();

  EXPECT_EQ(vp.size(), 3u);
  EXPECT_EQ(ip.size(), 3u);

  // Mutate through a value pointer.
  *vp.get_first() = "ONE";
  EXPECT_EQ(m.find(1), "ONE");
}

TEST(DynMapTree, GetDataAndGetKeyHelpers)
{
  auto m = make_sample();
  auto * p = m.search(3);
  ASSERT_NE(p, nullptr);

  // get_data via key reference
  std::string & data = DynMapTree<int, std::string>::get_data(p->first);
  EXPECT_EQ(data, "three");
  data = "THREE";
  EXPECT_EQ(m.find(3), "THREE");

  // get_key via data pointer
  const int & key = DynMapTree<int, std::string>::get_key(&p->second);
  EXPECT_EQ(key, 3);
}

TEST(DynMapTree, CustomComparatorAffectsTraversalOrder)
{
  DynMapTree<int, int, Avl_Tree, ReverseInt> m;
  m.insert(1, 1);
  m.insert(2, 2);
  m.insert(3, 3);

  // The exact traversal order depends on the tree implementation, but for a
  // strict reverse comparator, the first key visited should be the largest.
  DynMapTree<int, int, Avl_Tree, ReverseInt>::Iterator it(m);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr().first, 3);
}

TEST(DynMapTree, TypeTraits)
{
  static_assert(std::is_same_v<DynMapTree<int, int>::Key_Type, int>);
  static_assert(std::is_same_v<DynMapTree<int, int>::Value_Type, int>);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
