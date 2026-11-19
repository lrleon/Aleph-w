
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
 * @file tpl_dynMapTree_test.cc
 * @brief Tests for Tpl Dynmaptree
 */

#include <gtest/gtest.h>

#include <tpl_array.H>
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

namespace
{
// A tree for the typed tests of the search by the key
template <template <typename, class> class T>
struct Tree_Of
{
  template <class K, class D>
  using Map = DynMapTree<K, D, T>;
};

// The key i, of len characters (len >= 4), distinct for i < 9000: with a
// prefix, len - 4 equal letters and four digits; without one, the digits
// first. A std::string of 60 characters keeps them out of the node
std::string string_key(const int i, const size_t len, const bool prefix)
{
  const std::string digits = std::to_string(1000 + i);
  return prefix ? std::string(len - 4, 'k') + digits : digits + std::string(len - 4, 'x');
}

template <class Kind>
class DynMapTreeSearchByKey : public ::testing::Test {};

using Search_Trees =
  ::testing::Types<Tree_Of<Avl_Tree>, Tree_Of<Avl_Tree_Rk>, Tree_Of<Rb_Tree>, Tree_Of<TdRbTree>,
                   Tree_Of<Treap>, Tree_Of<BinTree>, Tree_Of<Rand_Tree>, Tree_Of<Splay_Tree>>;
TYPED_TEST_SUITE(DynMapTreeSearchByKey, Search_Trees);
} // namespace

// The search by a key that is not a scalar finds every pair, at any depth
// of the tree, and no key between two of them, before them or after them,
// with short and long string keys, with and without a long common prefix
TYPED_TEST(DynMapTreeSearchByKey, FindsEveryStringKeyAndNoOther)
{
  using Map = typename TypeParam::template Map<std::string, int>;
  constexpr int N = 300;
  for (const size_t len : {size_t(7), size_t(16), size_t(60)})
    for (const bool prefix : {false, true})
      {
        SCOPED_TRACE("length " + std::to_string(len) + (prefix ? ", with a prefix" : ""));
        Map map;
        for (int i = 0; i < N; ++i)  // out of order, so that the tree has any shape
          ASSERT_NE(map.insert(string_key(i * 37 % N, len, prefix), i * 37 % N), nullptr);
        Array<std::string> absent = {std::string(), std::string(len + 1, '~')};
        for (int i = 0; i < N; ++i)
          {
            const std::string key = string_key(i, len, prefix);
            const auto * p = map.search(key);
            ASSERT_NE(p, nullptr) << key;
            EXPECT_EQ(p->first, key);
            EXPECT_EQ(p->second, i);
            absent.append(key + '!');  // after key and before the next one
          }
        for (const auto & key : absent)
          EXPECT_EQ(map.search(key), nullptr) << key;
        EXPECT_EQ(map.size(), static_cast<size_t>(N));
        EXPECT_TRUE(map.verify());
      }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
