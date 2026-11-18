
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
 * @file hash_function_change_test.cc
 * @brief Pattern test: every hash container keeps finding its keys after
 *        set_hash_fct().
 *
 * A container that keeps a hash function of its own for its searches, as
 * the maps of tpl_dynSetHash.H that search by the key alone, or that leaves
 * its keys in the lists of the old function, stops finding them after
 * set_hash_fct(). Every hash container of the library goes through here,
 * empty and with keys: the chained and the open tables place their keys
 * again, and the linear ones, which cannot, refuse the function.
 */

# include <gtest/gtest.h>

# include <stdexcept>
# include <string>
# include <utility>

# include <tpl_dynLhash.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_lhash.H>
# include <tpl_linHash.H>
# include <tpl_odhash.H>
# include <tpl_olhash.H>

using namespace Aleph;

namespace
{
  constexpr int Keys = 300;  // enough to make the tables of 101 lists grow

  // Hash functions other than the default ones, of the keys and of the
  // pairs of the maps: a search by a stale function misses
  size_t new_hash(const int & k) noexcept
  {
    return static_cast<size_t>(k) * 40503u + 17;
  }

  size_t new_second_hash(const int & k) noexcept
  {
    return static_cast<size_t>(k) * 3 + 11;
  }

  size_t new_pair_hash(const std::pair<int, int> & p) noexcept
  {
    return new_hash(p.first);
  }

  size_t new_second_pair_hash(const std::pair<int, int> & p) noexcept
  {
    return new_second_hash(p.first);
  }

  // Each container: how it inserts the key k, whether it holds k, and how
  // it takes the new hash functions. refuses_keys: whether set_hash_fct()
  // refuses a container with keys, as the linear hash tables do.

  struct Chained_Table
  {
    using Container = LhashTable<int>;
    static constexpr bool refuses_keys = false;
    static void insert(Container & c, int k) { c.insert(new LhashBucket<int>(k)); }
    static bool holds(Container & c, int k) { return c.search(k) != nullptr; }
    static void set(Container & c) { c.set_hash_fct(new_hash); }
  };

  struct Linear_Table
  {
    using Container = LinearHashTable<int>;
    static constexpr bool refuses_keys = true;
    static void insert(Container & c, int k) { c.insert(new Container::Bucket(k)); }
    static bool holds(Container & c, int k) { return c.search(k) != nullptr; }
    static void set(Container & c) { c.set_hash_fct(new_hash); }
  };

  struct Linear_Probing_Table
  {
    using Container = OLhashTable<int>;
    static constexpr bool refuses_keys = false;
    static void insert(Container & c, int k) { (void) c.insert(k); }
    static bool holds(Container & c, int k) { return c.search(k) != nullptr; }
    static void set(Container & c) { c.set_hash_fct(new_hash); }
  };

  struct Double_Hashing_Table
  {
    using Container = ODhashTable<int>;
    static constexpr bool refuses_keys = false;
    static void insert(Container & c, int k) { (void) c.insert(k); }
    static bool holds(Container & c, int k) { return c.search(k) != nullptr; }
    static void set(Container & c)
    {
      c.set_hash_fct(new_hash);
      c.set_second_hash_fct(new_second_hash);
    }
  };

  template <class Set, bool Linear>
  struct A_Set
  {
    using Container = Set;
    static constexpr bool refuses_keys = Linear;
    static void insert(Container & c, int k) { (void) c.insert(k); }
    static bool holds(Container & c, int k) { return c.search(k) != nullptr; }
    static void set(Container & c) { c.set_hash_fct(new_hash); }
  };

  struct Chained_Records
  {
    using Container = DynLhashTable<int, int>;
    static constexpr bool refuses_keys = false;
    static void insert(Container & c, int k) { c.insert(k, 10 * k); }
    static bool holds(Container & c, int k)
    {
      const int * r = c.search(k);
      return r != nullptr and *r == 10 * k;
    }
    static void set(Container & c) { c.set_hash_fct(new_hash); }
  };

  // A map of tpl_dynSetHash.H, given a function of the keys or of the pairs
  template <class Map, bool Linear, bool By_Pairs>
  struct A_Hash_Map
  {
    using Container = Map;
    static constexpr bool refuses_keys = Linear;
    static void insert(Container & c, int k) { c.insert(k, 10 * k); }
    static bool holds(Container & c, int k)
    {
      const auto * p = c.search(k);
      return p != nullptr and p->second == 10 * k and c.has(k);
    }
    static void set(Container & c)
    {
      if constexpr (By_Pairs)
        c.set_hash_fct(new_pair_hash);
      else
        c.set_hash_fct(new_hash);
    }
  };

  // A map of tpl_dynMapOhash.H, whose functions are of the pairs
  template <class Map, bool Double>
  struct An_Open_Map
  {
    using Container = Map;
    static constexpr bool refuses_keys = false;
    static void insert(Container & c, int k) { (void) c.insert(k, 10 * k); }
    static bool holds(Container & c, int k)
    {
      const auto * p = c.search(k);
      return p != nullptr and p->second == 10 * k;
    }
    static void set(Container & c)
    {
      c.set_hash_fct(new_pair_hash);
      if constexpr (Double)
        c.set_second_hash_fct(new_second_pair_hash);
    }
  };

  using Hash_Containers = ::testing::Types<
    Chained_Table, Linear_Table, Linear_Probing_Table, Double_Hashing_Table,
    A_Set<DynSetLhash<int>, false>, A_Set<DynSetHash<int>, false>,
    A_Set<DynSetLinHash<int>, true>, Chained_Records,
    A_Hash_Map<DynMapHash<int, int>, false, false>,
    A_Hash_Map<DynMapHash<int, int>, false, true>,
    A_Hash_Map<DynMapLinHash<int, int>, true, false>,
    A_Hash_Map<DynMapLinHash<int, int>, true, true>,
    An_Open_Map<MapOLhash<int, int>, false>,
    An_Open_Map<MapODhash<int, int>, true>>;

  template <class T>
  class Hash_Function_Change : public ::testing::Test
  {
  };

  TYPED_TEST_SUITE(Hash_Function_Change, Hash_Containers);

  // A function set on an empty container rules where its keys go and
  // where its searches look for them
  TYPED_TEST(Hash_Function_Change, OnAnEmptyContainer)
  {
    typename TypeParam::Container c;
    TypeParam::set(c);
    for (int k = 0; k < Keys; ++k)
      TypeParam::insert(c, k);

    for (int k = 0; k < Keys; ++k)
      ASSERT_TRUE(TypeParam::holds(c, k)) << "key " << k;
    EXPECT_FALSE(TypeParam::holds(c, Keys));
  }

  // On a container with keys, set_hash_fct() places them again, so they
  // are still found, and the new ones go by the new function; a linear
  // table, which cannot place its keys again, refuses the function and
  // keeps its own. The keys are looked for before the container grows,
  // which would place them again by itself.
  TYPED_TEST(Hash_Function_Change, OnAContainerWithKeys)
  {
    typename TypeParam::Container c;
    for (int k = 0; k < Keys; ++k)
      TypeParam::insert(c, k);

    if constexpr (TypeParam::refuses_keys)
      EXPECT_THROW(TypeParam::set(c), std::domain_error);
    else
      TypeParam::set(c);

    for (int k = 0; k < Keys; ++k)
      ASSERT_TRUE(TypeParam::holds(c, k)) << "key " << k << ", before the container grows";
    for (int k = Keys; k < 2 * Keys; ++k)
      TypeParam::insert(c, k);
    for (int k = 0; k < 2 * Keys; ++k)
      ASSERT_TRUE(TypeParam::holds(c, k)) << "key " << k;
    EXPECT_FALSE(TypeParam::holds(c, 2 * Keys));
  }

  // =========================================================================
  // The maps of tpl_dynSetHash.H search by the hash of the keys, which they
  // keep apart from the function of the table. After a function of pairs
  // they search by the pair; a function of keys brings the search that
  // builds nothing back. Copies, moves and swaps keep the function of each
  // map.
  // =========================================================================

  template <class Map>
  class Map_Hash_Function : public ::testing::Test
  {
  };

  using Hash_Maps = ::testing::Types<DynMapHash<int, int>, DynMapLinHash<int, int>>;

  TYPED_TEST_SUITE(Map_Hash_Function, Hash_Maps);

  // Whether map holds the pairs (k, 10 k) for k in [from, to), and no other
  // key of [from - 1, to], through every way of searching
  template <class Map>
  void expect_pairs(Map & map, const int from, const int to)
  {
    const Map & const_map = map;
    for (int k = from; k < to; ++k)
      {
        auto * p = map.search(k);
        ASSERT_NE(p, nullptr) << "key " << k;
        EXPECT_EQ(p->second, 10 * k);
        EXPECT_EQ(const_map.search(k), p);
        int key = k;
        EXPECT_EQ(map.search(std::move(key)), p);
        EXPECT_TRUE(map.has(k));
        EXPECT_TRUE(map.contains(k));
        EXPECT_EQ(map.find(k), 10 * k);
      }
    EXPECT_FALSE(map.has(from - 1));
    EXPECT_FALSE(map.has(to));
  }

  TYPED_TEST(Map_Hash_Function, FunctionsOfPairsAndOfKeys)
  {
    TypeParam by_pairs, by_keys, back_to_keys;
    by_pairs.set_hash_fct(new_pair_hash);
    by_keys.set_hash_fct(new_hash);
    back_to_keys.set_hash_fct(new_pair_hash);
    back_to_keys.set_hash_fct(new_hash);
    for (int k = 0; k < Keys; ++k)
      {
        by_pairs.insert(k, 10 * k);
        by_keys.insert(k, 10 * k);
        back_to_keys.insert(k, 10 * k);
      }

    expect_pairs(by_pairs, 0, Keys);
    expect_pairs(by_keys, 0, Keys);
    expect_pairs(back_to_keys, 0, Keys);

    EXPECT_EQ(by_pairs.remove(7), 70);
    EXPECT_FALSE(by_pairs.has(7));
    EXPECT_EQ(by_keys.remove(7), 70);
    EXPECT_FALSE(by_keys.has(7));
    EXPECT_EQ(by_pairs[8], 80);
    by_pairs[Keys] = 10 * Keys;
    EXPECT_EQ(by_pairs.find(Keys), 10 * Keys);
  }

  TYPED_TEST(Map_Hash_Function, CopiesMovesAndSwapsKeepTheFunction)
  {
    TypeParam by_pairs, by_keys;
    by_pairs.set_hash_fct(new_pair_hash);
    by_keys.set_hash_fct(new_hash);
    for (int k = 0; k < Keys; ++k)
      {
        by_pairs.insert(k, 10 * k);
        by_keys.insert(Keys + k, 10 * (Keys + k));
      }

    TypeParam copy = by_pairs;
    expect_pairs(copy, 0, Keys);
    TypeParam moved = std::move(copy);
    expect_pairs(moved, 0, Keys);
    TypeParam assigned;
    assigned = by_keys;
    expect_pairs(assigned, Keys, 2 * Keys);
    TypeParam move_assigned;
    move_assigned = std::move(assigned);
    expect_pairs(move_assigned, Keys, 2 * Keys);

    by_pairs.swap(by_keys);
    expect_pairs(by_pairs, Keys, 2 * Keys);
    expect_pairs(by_keys, 0, Keys);
  }

  // The search by the hash of the keys builds nothing, so it does not throw
  // with keys of any type. After a function of pairs it builds one, from a
  // copy or a move of the key and a Data built by default: the search is
  // noexcept as they are. A Data with no default constructor takes no
  // function of pairs, and leaves the first search alone.
  struct No_Default
  {
    int value;
    No_Default(int v) noexcept : value(v) {}
  };

  template <class Key, class Data>
  concept Takes_A_Function_Of_Pairs =
    requires (DynMapHash<Key, Data> & m, size_t (*f)(const std::pair<Key, Data> &))
    {
      m.set_hash_fct(f);
    };

  static_assert(noexcept(std::declval<DynMapHash<int, int> &>().search(1)));
  static_assert(noexcept(std::declval<DynMapLinHash<int, int> &>().has(1)));
  static_assert(not noexcept(std::declval<DynMapHash<std::string, int> &>()
                             .search(std::declval<const std::string &>())));
  static_assert(not noexcept(std::declval<const DynMapLinHash<std::string, int> &>()
                             .has(std::declval<const std::string &>())));
  static_assert(noexcept(std::declval<DynMapHash<std::string, int> &>()
                         .search(std::declval<std::string>())));
  static_assert(noexcept(std::declval<DynMapHash<std::string, No_Default> &>()
                         .search(std::declval<const std::string &>())));
  static_assert(Takes_A_Function_Of_Pairs<std::string, int>);
  static_assert(not Takes_A_Function_Of_Pairs<std::string, No_Default>);

  TEST(Map_Hash_Function, ADataWithNoDefaultConstructorStillSearchesByTheKey)
  {
    DynMapHash<int, No_Default> map;
    map.set_hash_fct(new_hash);
    for (int k = 0; k < Keys; ++k)
      map.insert(k, No_Default(10 * k));
    for (int k = 0; k < Keys; ++k)
      {
        const auto * p = map.search(k);
        ASSERT_NE(p, nullptr) << "key " << k;
        EXPECT_EQ(p->second.value, 10 * k);
      }
  }
} // namespace
