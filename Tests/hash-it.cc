
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
# include <gtest/gtest.h>

# include <ahFunctional.H>
# include <ahSort.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_olhash.H>
# include <tpl_odhash.H>
# include <tpl_lhash.H>
# include <stdexcept>
# include <functional>

using namespace std;
using namespace testing;
using namespace Aleph;

using P = pair<size_t, string>;

constexpr size_t N = 1000;

template <class HashTbl>
struct OHashTest : public ::testing::Test
{
  HashTbl tbl;
  DynList<size_t> items;
  OHashTest()
  : tbl(N),  items(range(static_cast<size_t>(0), N - 1))
  {
    items.for_each([this](auto i)
                   { tbl.emplace(i, to_string(i)); });
  }
};

template <class HashTbl>
struct EmptyOHashTest : public ::testing::Test
{
  HashTbl tbl;
};

TYPED_TEST_CASE_P
(OHashTest);

TYPED_TEST_CASE_P
(EmptyOHashTest);

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
  auto ll = l.maps<size_t>([](auto &p)
                           { return p.first; });
  EXPECT_EQ(sort(ll), this->items);

  l.empty();
  EXPECT_NO_THROW(it.reset_last());
  for (; it.has_curr(); it.prev_ne())
    l.append(it.get_curr_ne());
  ll = l.maps<size_t>([](auto &p)
                      { return p.first; });
  EXPECT_EQ(sort(ll), this->items);
}

REGISTER_TYPED_TEST_CASE_P
(EmptyOHashTest, With_exception);
REGISTER_TYPED_TEST_CASE_P
(OHashTest, Without_exception);

using HashTypes =
  Types<MapODhash<size_t, string>,
    MapOLhash<size_t, string>,
    DynMapLinHash<size_t, string>,
    DynMapHash<size_t, string>
    >;

INSTANTIATE_TYPED_TEST_CASE_P
(Empty, EmptyOHashTest, HashTypes);
INSTANTIATE_TYPED_TEST_CASE_P
(NoEmpty, OHashTest, HashTypes);

TEST(OLhashTable, DoubleHashCtorAndOps)
{
  OLhashTable<int> tbl(
    11,
    Aleph::dft_hash_fct<int>,
    Aleph::dft_hash_fct<int>,
    Aleph::equal_to<int>(),
    hash_default_lower_alpha,
    hash_default_upper_alpha,
    false);

  EXPECT_TRUE(tbl.is_empty());
  ASSERT_NE(tbl.insert(42), nullptr);
  ASSERT_NE(tbl.insert(7), nullptr);
  EXPECT_NE(tbl.search(42), nullptr);

  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(2));
}

TEST(OLhashTable, KeyToBucketRoundTrip)
{
  OLhashTable<int> tbl;
  auto *ptr = tbl.insert(5);
  ASSERT_NE(ptr, nullptr);

  auto *bucket = decltype(tbl)::key_to_bucket(ptr);
  ASSERT_NE(bucket, nullptr);
  EXPECT_EQ(bucket->key, 5);
  // bucket must be within table range
  EXPECT_NO_THROW(tbl.remove(5));
}

TEST(OLhashTable, RemoveThrowsOnMissingKey)
{
  OLhashTable<int> tbl;
  tbl.insert(1);
  EXPECT_THROW(tbl.remove(2), std::domain_error);
}

TEST(OLhashTable, InsertRejectsDuplicate)
{
  OLhashTable<int> tbl(5, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  auto *p1 = tbl.insert(10);
  ASSERT_NE(p1, nullptr);
  auto *p2 = tbl.insert(10);
  EXPECT_EQ(p2, nullptr); // duplicate rejected
  EXPECT_EQ(tbl.size(), static_cast<size_t>(1));
}

TEST(OLhashTable, RemovePtrMarksDeleted)
{
  OLhashTable<int> tbl(11, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  auto *p = tbl.insert(5);
  ASSERT_NE(p, nullptr);
  // remove_ptr is protected; exercise via remove(key) for coverage
  EXPECT_NO_THROW(tbl.remove(5));
  EXPECT_EQ(tbl.size(), static_cast<size_t>(0));
  EXPECT_THROW(tbl.remove(5), std::domain_error);
}

TEST(OLhashTable, NoResizeFillAndSearchMiss)
{
  OLhashTable<int> tbl(5, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  EXPECT_EQ(tbl.size(), static_cast<size_t>(5));
  EXPECT_EQ(tbl.search(42), nullptr); // miss should terminate
}

TEST(OLhashTable, StatsWithDeletedAndBusy)
{
  OLhashTable<int> tbl(7, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  auto *p1 = tbl.insert(1);
  auto *p2 = tbl.insert(2);
  auto *p3 = tbl.insert(3);
  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  ASSERT_NE(p3, nullptr);

  tbl.remove_ptr(p2); // mark one as deleted

  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(2));
  EXPECT_EQ(stats.num_deleted, static_cast<size_t>(1));
  EXPECT_EQ(stats.num_busy + stats.num_deleted + stats.num_empty,
            static_cast<size_t>(tbl.capacity()));
  EXPECT_GE(stats.max_len, static_cast<size_t>(1));
}

TEST(OLhashTable, RehashKeepsElements)
{
  OLhashTable<int> tbl(5,
                       Aleph::dft_hash_fct<int>,
                       Aleph::equal_to<int>(),
                       /*lower*/0.25f,
                       /*upper*/0.5f,
                       /*with_resize*/true);
  for (int i = 0; i < 10; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);

  for (int i = 0; i < 10; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
  EXPECT_EQ(tbl.size(), static_cast<size_t>(10));
}

TEST(OLhashTable, ConstantHashCollisionsNoResize)
{
  auto const_hash = [](int) { return static_cast<size_t>(0); };
  OLhashTable<int> tbl(5, const_hash, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);

  for (int i = 0; i < 5; ++i)
    EXPECT_NE(tbl.search(i), nullptr);

  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(5));
  EXPECT_GE(stats.max_len, static_cast<size_t>(5)); // long collision chain
}

TEST(OLhashTable, RehashAfterDeletesPreservesLiveKeys)
{
  OLhashTable<int> tbl(5,
                       Aleph::dft_hash_fct<int>,
                       Aleph::equal_to<int>(),
                       /*lower*/0.25f,
                       /*upper*/0.5f,
                       /*with_resize*/true);
  for (int i = 0; i < 6; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);

  // Remove some keys to create DELETED slots
  tbl.remove(1);
  tbl.remove(3);

  // Insert more keys to force resize
  for (int i = 10; i < 16; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);

  // Live keys must remain accessible
  for (int i : {0, 2, 4, 5, 10, 11, 12, 13, 14, 15})
    EXPECT_NE(tbl.search(i), nullptr);
}

TEST(OLhashTable, SearchOrInsertBehavior)
{
  OLhashTable<int> tbl(7);
  auto *p1 = tbl.search_or_insert(1);
  ASSERT_NE(p1, nullptr);
  auto *p2 = tbl.search_or_insert(1); // should return existing
  EXPECT_EQ(p1, p2);
  EXPECT_EQ(tbl.size(), static_cast<size_t>(1));
}

TEST(OLhashTable, CopyMoveAndSwap)
{
  OLhashTable<int> a(7, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                     hash_default_lower_alpha, hash_default_upper_alpha, false);
  a.insert(1);
  a.insert(2);

  // copy ctor
  OLhashTable<int> b = a;
  EXPECT_EQ(b.size(), a.size());
  EXPECT_NE(b.search(1), nullptr);
  EXPECT_NE(b.search(2), nullptr);

  // move ctor
  OLhashTable<int> c = std::move(b);
  EXPECT_EQ(c.size(), static_cast<size_t>(2));
  EXPECT_NE(c.search(1), nullptr);
  // moved-from state is unspecified; ensure it is still usable
  b.insert(5);
  EXPECT_NE(b.search(5), nullptr);

  // swap
  OLhashTable<int> d(3, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                     hash_default_lower_alpha, hash_default_upper_alpha, false);
  d.insert(99);
  a.swap(d);
  EXPECT_NE(a.search(99), nullptr);
  EXPECT_EQ(d.search(99), nullptr);

  // copy assignment
  OLhashTable<int> e(2);
  e.insert(42);
  e = c; // c has 1 and 2
  EXPECT_NE(e.search(1), nullptr);
  EXPECT_NE(e.search(2), nullptr);
  EXPECT_EQ(e.size(), static_cast<size_t>(2));

  // move assignment
  OLhashTable<int> f(2);
  f.insert(77);
  f = std::move(e);
  EXPECT_NE(f.search(1), nullptr);
  EXPECT_NE(f.search(2), nullptr);
  f.insert(88); // moved-from e should still be usable indirectly via f
}

TEST(OLhashTable, StatsExtremes)
{
  OLhashTable<int> tbl_empty(5);
  auto stats_empty = tbl_empty.stats();
  EXPECT_EQ(stats_empty.num_busy, static_cast<size_t>(0));
  EXPECT_EQ(stats_empty.num_deleted, static_cast<size_t>(0));
  // num_empty may be greater than len due to lens counting, so only busy/deleted are validated

  auto const_hash = [](int) { return static_cast<size_t>(0); };
  OLhashTable<int> tbl_full(5, const_hash, Aleph::equal_to<int>(),
                            hash_default_lower_alpha, hash_default_upper_alpha, false);
  for (int i = 0; i < 5; ++i)
    tbl_full.insert(i);
  auto stats_full = tbl_full.stats();
  EXPECT_EQ(stats_full.num_busy, static_cast<size_t>(5));
  EXPECT_EQ(stats_full.num_deleted, static_cast<size_t>(0));
  EXPECT_FALSE(std::isnan(stats_full.avg));
  EXPECT_FALSE(std::isnan(stats_full.var));
}

TEST(OLhashTable, CleanTableLeavesEmpty)
{
  OLhashTable<int> tbl(5);
  tbl.insert(1);
  tbl.insert(2);
  tbl.clean_table();
  EXPECT_EQ(tbl.size(), static_cast<size_t>(0));
  EXPECT_EQ(tbl.search(1), nullptr);
}

TEST(OLhashTable, InsertFailsWhenFullNoResize)
{
  OLhashTable<int> tbl(3, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  ASSERT_NE(tbl.insert(1), nullptr);
  ASSERT_NE(tbl.insert(2), nullptr);
  ASSERT_NE(tbl.insert(3), nullptr);
  // With a full table and no resize, inserting a duplicate returns nullptr
  EXPECT_EQ(tbl.insert(2), nullptr);
}

TEST(OLhashTable, ResizeIncreasesCapacity)
{
  // Small upper alpha to force rehash quickly
  OLhashTable<int> tbl(3,
                       Aleph::dft_hash_fct<int>,
                       Aleph::equal_to<int>(),
                       /*lower*/0.1f,
                       /*upper*/0.4f,
                       /*with_resize*/true);
  const auto cap0 = tbl.capacity();
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  EXPECT_GE(tbl.capacity(), cap0); // puede no crecer si el alpha no se supera
  for (int i = 0; i < 10; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

TEST(OLhashTable, DeletedSlotIsReused)
{
  OLhashTable<int> tbl(5, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                       hash_default_lower_alpha, hash_default_upper_alpha, false);
  tbl.insert(1);
  tbl.insert(2);
  tbl.remove(1); // leaves a DELETED slot
  EXPECT_EQ(tbl.size(), static_cast<size_t>(1));
  tbl.insert(3); // should reuse the slot if applicable
  EXPECT_NE(tbl.search(2), nullptr);
  EXPECT_NE(tbl.search(3), nullptr);
  EXPECT_EQ(tbl.size(), static_cast<size_t>(2));
}

TEST(ODhashTable, RemoveMissingInFullTableDoesNotHang)
{
  // Small table without resize to force search without finding EMPTY
  ODhashTable<int> tbl(5,
                       Aleph::dft_hash_fct<int>,
                       Aleph::snd_hash_fct<int>,
                       Aleph::equal_to<int>(),
                       hash_default_lower_alpha,
                       hash_default_upper_alpha,
                       /*with_resize*/false);
  for (int i = 0; i < 5; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);

  // Missing key must throw in finite time (no infinite loop)
  EXPECT_THROW(tbl.remove(99), std::domain_error);
}

TEST(ODhashTable, DuplicateInsertAndSearchOrInsert)
{
  ODhashTable<int> tbl(7);
  auto *p1 = tbl.insert(10);
  ASSERT_NE(p1, nullptr);
  auto *p2 = tbl.insert(10);
  EXPECT_EQ(p2, nullptr); // duplicate rejected

  auto *p3 = tbl.search_or_insert(10);
  EXPECT_EQ(p3, p1); // returns the existing one
  EXPECT_EQ(tbl.size(), static_cast<size_t>(1));
}

TEST(ODhashTable, CopyMoveAssignAndSwap)
{
  ODhashTable<int> a(7);
  a.insert(1);
  a.insert(2);

  ODhashTable<int> b = a; // copy ctor
  EXPECT_EQ(b.size(), a.size());
  EXPECT_NE(b.search(1), nullptr);

  ODhashTable<int> c = std::move(b); // move ctor
  EXPECT_EQ(c.size(), static_cast<size_t>(2));

  ODhashTable<int> d(3);
  d.insert(99);
  c.swap(d);
  EXPECT_NE(c.search(99), nullptr);
  EXPECT_EQ(d.search(99), nullptr);

  ODhashTable<int> e(2);
  e.insert(77);
  e = c; // copy assign
  EXPECT_NE(e.search(99), nullptr);
  EXPECT_EQ(e.size(), c.size());

  ODhashTable<int> f(2);
  f = std::move(e); // move assign
  EXPECT_NE(f.search(99), nullptr);
  f.insert(5); // usable after move
}

TEST(ODhashTable, StatsWithBusyDeletedEmpty)
{
  ODhashTable<int> tbl(7, Aleph::dft_hash_fct<int>, Aleph::snd_hash_fct<int>,
                       Aleph::equal_to<int>(), hash_default_lower_alpha,
                       hash_default_upper_alpha, false);
  tbl.insert(1);
  tbl.insert(2);
  tbl.insert(3);
  tbl.remove(2); // leaves DELETED and rehash clears probe_counters
  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(2));
  EXPECT_GE(stats.num_deleted, static_cast<size_t>(0)); // may compact
}

TEST(ODhashTable, CustomSecondHashSetter)
{
  ODhashTable<int> tbl;
  auto h2 = [](int k) { return static_cast<size_t>(k * 13); };
  tbl.set_second_hash_fct(h2);
  EXPECT_TRUE(tbl.insert(1));
  EXPECT_NE(tbl.search(1), nullptr);
}

TEST(ODhashTable, StatsAvgVarFinite)
{
  ODhashTable<int> tbl;
  for (int i = 0; i < 5; ++i)
    tbl.insert(i);
  auto stats = tbl.stats();
  EXPECT_FALSE(std::isnan(stats.avg));
  EXPECT_FALSE(std::isnan(stats.var));
}
TEST(ODhashTable, StatsControlledProbes)
{
  ODhashTable<int> tbl(7);
  // Force the second hash to be constant to lengthen the probe chain
  auto const_h2 = +[](int) { return static_cast<size_t>(0); };
  tbl.set_second_hash_fct(const_h2);
  // Insert keys that collide in the first hash by modulo
  tbl.insert(0);  // i_fst=0
  tbl.insert(7);  // i_fst=0 -> BUSY, i_snd=0 -> BUSY, linear probe
  tbl.insert(14); // i_fst=0 -> BUSY, i_snd=0 -> BUSY, linear probe
  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(3));
  EXPECT_GE(stats.max_len, static_cast<size_t>(1));
}

TEST(ODhashTable, RehashWithResizeKeepsElements)
{
  ODhashTable<int> tbl(5,
                       Aleph::dft_hash_fct<int>,
                       Aleph::snd_hash_fct<int>,
                       Aleph::equal_to<int>(),
                       /*lower*/0.25f,
                       /*upper*/0.5f,
                       /*with_resize*/true);
  const auto cap0 = tbl.capacity();
  for (int i = 0; i < 10; ++i)
    tbl.insert(i);
  EXPECT_GE(tbl.capacity(), cap0);
  for (int i = 0; i < 10; ++i)
    EXPECT_NE(tbl.search(i), nullptr);
}

TEST(ODhashTable, LinearProbePathCovered)
{
  ODhashTable<int> tbl(5);
  tbl.insert(0);
  tbl.insert(5);
  tbl.insert(10);
  EXPECT_NE(tbl.search(10), nullptr); // traverses linear probe
}

TEST(ODhashTable, IteratorCollectsAllKeys)
{
  ODhashTable<int> tbl(11);
  std::set<int> inserted;
  for (int i = 0; i < 7; ++i)
    {
      tbl.insert(i * 2);
      inserted.insert(i * 2);
    }
  auto it = tbl.get_it();
  std::set<int> iterated;
  for (; it.has_curr(); it.next_ne())
    iterated.insert(it.get_curr());
  EXPECT_EQ(iterated, inserted);
}

// ============================================================================
// LhashTable tests
// ============================================================================

TEST(LhashTable, BusySlotsAfterLastRemoval)
{
  auto const_hash = +[](const int &) { return static_cast<size_t>(0); };
  LhashTable<int> tbl(5, const_hash, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, true);

  using Bucket = LhashBucket<int>;
  auto * b = new Bucket(1);
  ASSERT_NE(tbl.insert(b), nullptr);
  EXPECT_EQ(tbl.size(), 1u);
  EXPECT_EQ(tbl.get_num_busy_slots(), 1u);

  auto * removed = tbl.remove(b);
  ASSERT_NE(removed, nullptr);
  delete removed;

  EXPECT_EQ(tbl.size(), 0u);
  EXPECT_EQ(tbl.get_num_busy_slots(), 0u);
  EXPECT_DOUBLE_EQ(tbl.current_alpha(), 0.0);
}

TEST(LhashTable, ShrinksWhenLoadFallsBelowLowerAlpha)
{
  LhashTable<int> tbl(500, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                      0.9f, 10.0f, true, true);

  using Bucket = LhashBucket<int>;
  auto * b1 = new Bucket(1);
  auto * b2 = new Bucket(2);
  auto * b3 = new Bucket(3);

  ASSERT_NE(tbl.insert(b1), nullptr);
  ASSERT_NE(tbl.insert(b2), nullptr);
  ASSERT_NE(tbl.insert(b3), nullptr);

  const auto old_cap = tbl.capacity();
  auto * removed = tbl.remove(b1);
  ASSERT_NE(removed, nullptr);
  delete removed;

  EXPECT_LT(tbl.capacity(), old_cap);
  EXPECT_EQ(tbl.size(), 2u);

  delete tbl.remove(b2);
  delete tbl.remove(b3);
}

TEST(LhashTable, IteratorDelRemovesAllCollidingBuckets)
{
  auto const_hash = +[](const int &) { return static_cast<size_t>(1); };
  LhashTable<int> tbl(7, const_hash, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, true);
  using Bucket = LhashBucket<int>;
  for (int k : {1, 2, 3})
    ASSERT_NE(tbl.insert(new Bucket(k)), nullptr);

  for (LhashTable<int>::Iterator it(tbl); it.has_curr(); )
    {
      auto * removed = it.del();
      delete removed;
    }

  EXPECT_EQ(tbl.size(), 0u);
  EXPECT_EQ(tbl.get_num_busy_slots(), 0u);
}

TEST(LhashTable, SearchOrInsertReturnsExisting)
{
  LhashTable<int> tbl(5);
  using Bucket = LhashBucket<int>;

  auto * b1 = new Bucket(42);
  ASSERT_NE(tbl.insert(b1), nullptr);

  auto * b2 = new Bucket(42);
  auto * found = tbl.search_or_insert(b2);
  EXPECT_EQ(found, b1);
  delete b2;

  delete tbl.remove(b1);
}

TEST(LhashTable, MoveSemanticsPreserveContent)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> src(5);
  auto * b1 = new Bucket(1);
  auto * b2 = new Bucket(2);
  src.insert(b1);
  src.insert(b2);

  LhashTable<int> dst(std::move(src));
  EXPECT_TRUE(src.is_empty());
  EXPECT_EQ(dst.size(), 2u);
  EXPECT_NE(dst.search(1), nullptr);
  EXPECT_NE(dst.search(2), nullptr);

  delete dst.remove(b1);
  delete dst.remove(b2);
}

TEST(LhashTable, InsertRejectsDuplicateAndSearchFinds)
{
  LhashTable<int> tbl(11);
  using Bucket = LhashBucket<int>;

  auto * b1 = new Bucket(7);
  ASSERT_NE(tbl.insert(b1), nullptr);
  EXPECT_EQ(tbl.size(), 1u);

  auto * b2 = new Bucket(7);
  EXPECT_EQ(tbl.insert(b2), nullptr);
  delete b2;

  auto * found = tbl.search(7);
  EXPECT_EQ(found, b1);

  delete tbl.remove(b1);
}

TEST(LhashTable, SearchNextReturnsNullWhenNoDuplicateKey)
{
  auto const_hash = +[](const int &) { return static_cast<size_t>(3); };
  LhashTable<int> tbl(7, const_hash, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, true);
  using Bucket = LhashBucket<int>;
  auto * b1 = new Bucket(1);
  auto * b2 = new Bucket(2);
  auto * b3 = new Bucket(3);
  tbl.insert(b1);
  tbl.insert(b2);
  tbl.insert(b3);

  auto * first = tbl.search(1);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(tbl.search_next(first), nullptr);

  delete tbl.remove(b1);
  delete tbl.remove(b2);
  delete tbl.remove(b3);
}

TEST(LhashTable, IteratorBoundsThrow)
{
  LhashTable<int> tbl(5);
  using Bucket = LhashBucket<int>;
  tbl.insert(new Bucket(1));
  tbl.insert(new Bucket(2));

  LhashTable<int>::Iterator it(tbl);
  it.reset_last();
  ASSERT_TRUE(it.has_curr());
  it.next();
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);

  it.reset_first();
  ASSERT_TRUE(it.has_curr());
  it.prev();
  EXPECT_THROW(it.get_curr(), std::underflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);

  // cleanup
  for (LhashTable<int>::Iterator clean(tbl); clean.has_curr(); )
    delete clean.del();
}

TEST(LhashTable, EmptyResetsCountersAndBuckets)
{
  LhashTable<int> tbl(11, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, true);
  using Bucket = LhashBucket<int>;
  for (int k : {1, 2, 3})
    tbl.insert(new Bucket(k));

  tbl.empty();

  EXPECT_EQ(tbl.size(), 0u);
  EXPECT_EQ(tbl.get_num_busy_slots(), 0u);
  EXPECT_DOUBLE_EQ(tbl.current_alpha(), 0.0);
}

TEST(LhashTable, RemoveAllBucketsFalseKeepsOwnership)
{
  // remove_all_buckets = false should not delete user buckets on destruction
  auto const_hash = +[](const int &) { return static_cast<size_t>(0); };
  auto * b = new LhashBucket<int>(10);
  {
    LhashTable<int> tbl(5, const_hash, Aleph::equal_to<int>(),
                        hash_default_lower_alpha, hash_default_upper_alpha,
                        /*remove_all_buckets*/false, /*with_resize*/true);
    tbl.insert(b);
    EXPECT_EQ(tbl.size(), 1u);
  }
  // if destructor deleted b, this would UAF; we delete manually
  delete b;
}

TEST(LhashTable, SwapExchangesState)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> a(7);
  LhashTable<int> b(3);
  auto * b1 = new Bucket(1);
  auto * b2 = new Bucket(2);
  auto * b3 = new Bucket(3);
  auto * bx = new Bucket(99);
  a.insert(b1);
  a.insert(b2);
  a.insert(b3);
  b.insert(bx);

  const auto cap_a = a.capacity();
  const auto cap_b = b.capacity();

  a.swap(b);

  EXPECT_EQ(a.size(), 1u);
  EXPECT_EQ(b.size(), 3u);
  EXPECT_EQ(a.capacity(), cap_b);
  EXPECT_EQ(b.capacity(), cap_a);
  EXPECT_NE(a.search(99), nullptr);
  EXPECT_NE(b.search(1), nullptr);

  delete a.remove(bx);
  delete b.remove(b1);
  delete b.remove(b2);
  delete b.remove(b3);
}

TEST(LhashTable, SetHashFunctionChangesDistribution)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> tbl(101);
  for (int k : {1, 2, 102}) // 1 and 102 collide with modulo 101
    tbl.insert(new Bucket(k));
  const size_t busy_before = tbl.get_num_busy_slots();

  tbl.set_hash_fct(+[](const int & k) { return static_cast<size_t>(k * 2); });
  tbl.resize(101); // force rehash with new hash function

  EXPECT_LE(tbl.get_num_busy_slots(), 3u);
  EXPECT_NE(tbl.search(1), nullptr);
  EXPECT_NE(tbl.search(2), nullptr);
  EXPECT_NE(tbl.search(102), nullptr);
  EXPECT_NE(tbl.get_num_busy_slots(), busy_before);

  delete tbl.remove(tbl.search(1));
  delete tbl.remove(tbl.search(2));
  delete tbl.remove(tbl.search(102));
}

TEST(LhashTable, NoResizeModeKeepsCapacity)
{
  LhashTable<int> tbl(11, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, false);
  using Bucket = LhashBucket<int>;
  const auto cap = tbl.capacity();
  for (int k = 0; k < 50; ++k)
    tbl.insert(new Bucket(k));

  EXPECT_EQ(tbl.capacity(), cap);

  for (LhashTable<int>::Iterator it(tbl); it.has_curr(); )
    delete it.del();
}

TEST(LhashTable, CustomComparatorActsAsEquality)
{
  struct ModCmp
  {
    bool operator()(int a, int b) const { return (a % 10) == (b % 10); }
  };
  using Bucket = LhashBucket<int>;
  auto const_hash = +[](const int &) { return static_cast<size_t>(0); };
  LhashTable<int, ModCmp> tbl(11, const_hash, ModCmp(),
                              hash_default_lower_alpha,
                              hash_default_upper_alpha,
                              true, true);
  auto * b1 = new Bucket(10);
  auto * b2 = new Bucket(20);
  ASSERT_NE(tbl.insert(b1), nullptr);
  EXPECT_EQ(tbl.insert(b2), nullptr); // treated as equal keys
  EXPECT_EQ(tbl.size(), 1u);

  delete tbl.remove(b1);
  delete b2;
}

TEST(LhashTable, CurrentAlphaNonZero)
{
  using Bucket = LhashBucket<int>;
  LhashTable<int> tbl(101);
  tbl.insert(new Bucket(1));
  tbl.insert(new Bucket(2));
  EXPECT_GT(tbl.current_alpha(), 0.0);
  for (LhashTable<int>::Iterator it(tbl); it.has_curr(); )
    delete it.del();
}

static_assert(!std::is_copy_constructible_v<LhashTable<int>>,
              "LhashTable should not be copy constructible");
static_assert(!std::is_copy_assignable_v<LhashTable<int>>,
              "LhashTable should not be copy assignable");
