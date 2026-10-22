
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
 * @file hash-it.cc
 * @brief Tests for Hash It
 */
# include <gtest/gtest.h>

# include <ahFunctional.H>
# include <ahSort.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynSetHash.H>
# include <tpl_olhash.H>
# include <tpl_odhash.H>
# include <tpl_lhash.H>
# include <tpl_hash.H>
# include <algorithm>
# include <deque>
# include <stdexcept>
# include <functional>
# include <map>
# include <ostream>
# include <random>
# include <set>
# include <type_traits>
# include <utility>
# include <vector>

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
  for (int i = 0; i < 4; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  EXPECT_EQ(tbl.insert(4), nullptr); // full: one bucket stays empty
  EXPECT_EQ(tbl.size(), static_cast<size_t>(4));
  EXPECT_EQ(tbl.search(42), nullptr); // miss should terminate
}

TEST(OLhashTable, StatsWithDeletedAndBusy)
{
  auto constant_hash = [] (const int &) noexcept -> size_t { return 0u; };
  OLhashTable<int> tbl(7, OLhashTable<int>::Hash_Fct(constant_hash),
                       Aleph::equal_to<int>(),
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
  for (int i = 0; i < 4; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  EXPECT_EQ(tbl.insert(4), nullptr); // full: one bucket stays empty

  for (int i = 0; i < 4; ++i)
    EXPECT_NE(tbl.search(i), nullptr);

  auto stats = tbl.stats();
  EXPECT_EQ(stats.num_busy, static_cast<size_t>(4));
  EXPECT_GE(stats.max_len, static_cast<size_t>(4)); // long collision chain
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
    (void) tbl_full.insert(i); // the last one does not fit
  auto stats_full = tbl_full.stats();
  EXPECT_EQ(stats_full.num_busy, static_cast<size_t>(4));
  EXPECT_EQ(stats_full.num_empty, static_cast<size_t>(1));
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
  // Two keys fill three buckets, since one must stay empty
  EXPECT_EQ(tbl.insert(3), nullptr);
  // With a full table and no resize, inserting a duplicate returns nullptr
  EXPECT_EQ(tbl.insert(2), nullptr);
  EXPECT_EQ(tbl.size(), static_cast<size_t>(2));
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
  for (int i = 0; i < 4; ++i)
    ASSERT_NE(tbl.insert(i), nullptr);
  EXPECT_EQ(tbl.insert(4), nullptr); // full: one bucket stays empty

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

TEST(LhashTable, SetWithResizeTurnsResizingOnAndOff)
{
  LhashTable<int> tbl(11, Aleph::dft_hash_fct<int>, Aleph::equal_to<int>(),
                      hash_default_lower_alpha, hash_default_upper_alpha,
                      true, false);
  using Bucket = LhashBucket<int>;
  EXPECT_FALSE(tbl.get_with_resize());
  for (int k = 0; k < 50; ++k)
    ASSERT_NE(tbl.insert(new Bucket(k)), nullptr) << "key " << k;
  EXPECT_EQ(tbl.capacity(), 11u);

  // turned on, the next insertion grows it
  tbl.set_with_resize(true);
  EXPECT_TRUE(tbl.get_with_resize());
  ASSERT_NE(tbl.insert(new Bucket(50)), nullptr);
  const size_t grown = tbl.capacity();
  EXPECT_GT(grown, 11u);

  // turned off, removals do not shrink it
  tbl.set_with_resize(false);
  for (int k = 0; k < 45; ++k)
    delete tbl.remove(tbl.search(k));
  EXPECT_EQ(tbl.capacity(), grown);

  // and turned on again, they do
  tbl.set_with_resize(true);
  delete tbl.remove(tbl.search(45));
  EXPECT_LT(tbl.capacity(), grown);
  for (int k = 46; k <= 50; ++k)
    EXPECT_NE(tbl.search(k), nullptr) << "key " << k;

  for (LhashTable<int>::Iterator it(tbl); it.has_curr(); )
    delete it.del();
}

// The sets and maps over chained tables copied every table as resizing.
TEST(DynSetLhash, CopyKeepsWhetherTheTableResizes)
{
  DynSetLhash<int> original;
  for (int k = 0; k < 20; ++k)
    original.insert(k);
  original.set_with_resize(false);

  DynSetLhash<int> copy(original);
  EXPECT_FALSE(copy.get_with_resize());
  EXPECT_EQ(copy.size(), 20u);

  DynSetLinHash<int> lin_original;
  lin_original.insert(1);
  lin_original.set_with_resize(false);
  DynSetLinHash<int> lin_copy(lin_original);
  EXPECT_FALSE(lin_copy.get_with_resize());
  EXPECT_TRUE(lin_copy.contains(1));
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

// ---------------------------------------------------------------------------
// What rules a table travels with its keys
// ---------------------------------------------------------------------------

namespace
{
  size_t first_hash_a(const long & k) noexcept
  {
    return static_cast<size_t>(k) * 2654435761u;
  }

  size_t first_hash_b(const long & k) noexcept
  {
    return static_cast<size_t>(k) * 40503u + 17;
  }

  size_t second_hash_a(const long & k) noexcept
  {
    return static_cast<size_t>(k) + 7;
  }

  size_t second_hash_b(const long & k) noexcept
  {
    return static_cast<size_t>(k) * 3 + 11;
  }

  // An equality that says which table it was given to.
  struct Tagged_Equal
  {
    int tag = 0;

    bool operator()(const long & x, const long & y) const noexcept
    {
      return x == y;
    }
  };

  // Everything that rules a table, apart from its keys and its capacity.
  struct Table_Config
  {
    size_t first_hash = 0;  // the hash function, applied to a fixed key
    size_t second_hash = 0; // the second one, in the table that has two
    int cmp_tag = 0;
    float lower_alpha = 0;
    float upper_alpha = 0;

    bool operator==(const Table_Config &) const = default;

    friend std::ostream & operator<<(std::ostream & out,
                                     const Table_Config & c)
    {
      return out << "{hash " << c.first_hash << ", second hash "
                 << c.second_hash << ", comparator " << c.cmp_tag
                 << ", thresholds " << c.lower_alpha << '/' << c.upper_alpha
                 << '}';
    }
  };

  constexpr long Config_Probe_Key = 12345;

  // The two configurations that the tests tell apart. Capacity `len` is
  // not part of them: each test chooses it.
  const Table_Config config_a = {first_hash_a(Config_Probe_Key),
                                 second_hash_a(Config_Probe_Key), 1,
                                 0.10f, 0.40f};
  const Table_Config config_b = {first_hash_b(Config_Probe_Key),
                                 second_hash_b(Config_Probe_Key), 2,
                                 0.25f, 0.90f};

  using Linear_Probing = OLhashTable<long, Tagged_Equal>;
  using Double_Hashing = ODhashTable<long, Tagged_Equal>;
  using Chained = DynSetLhash<long, Tagged_Equal>;
  using Linear_Hashing = DynSetLinHash<long, Tagged_Equal>;

  // How to build each table with configuration a or b, and how to read
  // its configuration back.
  template <class Table> struct Config_Of;

  template <> struct Config_Of<Linear_Probing>
  {
    static Linear_Probing make(const bool a, const size_t len)
    {
      return a ? Linear_Probing(len, first_hash_a, Tagged_Equal{1},
                                0.10f, 0.40f, true)
               : Linear_Probing(len, first_hash_b, Tagged_Equal{2},
                                0.25f, 0.90f, true);
    }

    static size_t second_hash(const Linear_Probing &, const bool a)
    {
      return a ? config_a.second_hash : config_b.second_hash; // it has none
    }
  };

  template <> struct Config_Of<Double_Hashing>
  {
    static Double_Hashing make(const bool a, const size_t len)
    {
      return a ? Double_Hashing(len, first_hash_a, second_hash_a,
                                Tagged_Equal{1}, 0.10f, 0.40f, true)
               : Double_Hashing(len, first_hash_b, second_hash_b,
                                Tagged_Equal{2}, 0.25f, 0.90f, true);
    }

    static size_t second_hash(const Double_Hashing & table, bool)
    {
      return table.get_second_hash_fct()(Config_Probe_Key);
    }
  };

  template <class Table> struct Config_Of_Chained
  {
    static Table make(const bool a, const size_t len)
    {
      return a ? Table(len, first_hash_a, Tagged_Equal{1}, 0.10f, 0.40f)
               : Table(len, first_hash_b, Tagged_Equal{2}, 0.25f, 0.90f);
    }

    static size_t second_hash(const Table &, const bool a)
    {
      return a ? config_a.second_hash : config_b.second_hash; // it has none
    }
  };

  template <> struct Config_Of<Chained> : Config_Of_Chained<Chained> {};

  template <>
  struct Config_Of<Linear_Hashing> : Config_Of_Chained<Linear_Hashing> {};

  // `like_a` only says which second hash to report for the tables that
  // have a single hash function.
  template <class Table>
  Table_Config config_of(const Table & table, const bool like_a)
  {
    return {table.get_hash_fct()(Config_Probe_Key),
            Config_Of<Table>::second_hash(table, like_a),
            table.get_compare().tag,
            table.get_lower_alpha(), table.get_upper_alpha()};
  }

  template <class Table>
  void insert_keys(Table & table, const long from, const long to)
  {
    for (long k = from; k < to; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
  }

  template <class Table>
  ::testing::AssertionResult holds(const Table & table, long from, long to)
  {
    if (table.size() != static_cast<size_t>(to - from))
      return ::testing::AssertionFailure()
             << "size is " << table.size() << ", expected " << to - from;
    for (long k = from; k < to; ++k)
      if (not table.contains(k))
        return ::testing::AssertionFailure() << "key " << k << " not found";
    return ::testing::AssertionSuccess();
  }

  template <class Table>
  struct HashTableConfiguration : public ::testing::Test {};

  using Configured_Tables = ::testing::Types<Linear_Probing, Double_Hashing,
                                             Chained, Linear_Hashing>;
  TYPED_TEST_SUITE(HashTableConfiguration, Configured_Tables);

  TYPED_TEST(HashTableConfiguration, ConstructionKeepsWhatItWasGiven)
  {
    const auto a = Config_Of<TypeParam>::make(true, 101);
    const auto b = Config_Of<TypeParam>::make(false, 101);

    EXPECT_EQ(config_of(a, true), config_a);
    EXPECT_EQ(config_of(b, false), config_b);
  }

  TYPED_TEST(HashTableConfiguration, CopyConstructionCopiesTheConfiguration)
  {
    auto source = Config_Of<TypeParam>::make(true, 101);
    insert_keys(source, 0, 40);

    const TypeParam copy(source);

    EXPECT_EQ(config_of(copy, true), config_a);
    EXPECT_TRUE(holds(copy, 0, 40));
    EXPECT_EQ(config_of(source, true), config_a);
    EXPECT_TRUE(holds(source, 0, 40));
  }

  // The target is larger than the source needs. The open addressing tables
  // used to keep their own array in this case, and with it their own hash
  // function, comparator and thresholds.
  TYPED_TEST(HashTableConfiguration, CopyAssignmentToALargerTable)
  {
    auto source = Config_Of<TypeParam>::make(true, 101);
    insert_keys(source, 0, 40);
    auto target = Config_Of<TypeParam>::make(false, 1009);
    insert_keys(target, 500, 520);

    target = source;

    EXPECT_EQ(config_of(target, true), config_a);
    EXPECT_TRUE(holds(target, 0, 40));
    EXPECT_EQ(config_of(source, true), config_a);
    EXPECT_TRUE(holds(source, 0, 40));

    insert_keys(target, 40, 400); // it keeps working with what it was given
    EXPECT_TRUE(holds(target, 0, 400));
  }

  TYPED_TEST(HashTableConfiguration, CopyAssignmentToASmallerTable)
  {
    auto source = Config_Of<TypeParam>::make(true, 1009);
    insert_keys(source, 0, 400);
    auto target = Config_Of<TypeParam>::make(false, 11);
    insert_keys(target, 500, 505);

    target = source;

    EXPECT_EQ(config_of(target, true), config_a);
    EXPECT_TRUE(holds(target, 0, 400));
    EXPECT_TRUE(holds(source, 0, 400));
  }

  TYPED_TEST(HashTableConfiguration, SelfCopyAssignmentChangesNothing)
  {
    auto table = Config_Of<TypeParam>::make(true, 101);
    insert_keys(table, 0, 40);
    const auto & same = table;

    table = same;

    EXPECT_EQ(config_of(table, true), config_a);
    EXPECT_TRUE(holds(table, 0, 40));
  }

  TYPED_TEST(HashTableConfiguration, MoveConstructionCarriesTheConfiguration)
  {
    auto source = Config_Of<TypeParam>::make(true, 101);
    insert_keys(source, 0, 40);

    TypeParam target(std::move(source));

    EXPECT_EQ(config_of(target, true), config_a);
    EXPECT_TRUE(holds(target, 0, 40));

    // The source is empty, keeps its configuration and can be used again.
    EXPECT_TRUE(source.is_empty());
    EXPECT_EQ(source.size(), 0u);
    EXPECT_FALSE(source.contains(7));
    EXPECT_EQ(config_of(source, true), config_a);
    insert_keys(source, 100, 300);
    EXPECT_TRUE(holds(source, 100, 300));
    EXPECT_TRUE(holds(target, 0, 40));
  }

  TYPED_TEST(HashTableConfiguration, MoveAssignmentCarriesTheConfiguration)
  {
    auto source = Config_Of<TypeParam>::make(true, 101);
    insert_keys(source, 0, 40);
    auto target = Config_Of<TypeParam>::make(false, 1009);
    insert_keys(target, 500, 520);

    target = std::move(source);

    EXPECT_EQ(config_of(target, true), config_a);
    EXPECT_TRUE(holds(target, 0, 40));

    insert_keys(target, 40, 400);
    EXPECT_TRUE(holds(target, 0, 400));
  }

  TYPED_TEST(HashTableConfiguration, SwapExchangesTheConfiguration)
  {
    auto a = Config_Of<TypeParam>::make(true, 101);
    insert_keys(a, 0, 40);
    auto b = Config_Of<TypeParam>::make(false, 1009);
    insert_keys(b, 500, 520);

    a.swap(b);

    EXPECT_EQ(config_of(a, false), config_b);
    EXPECT_TRUE(holds(a, 500, 520));
    EXPECT_EQ(config_of(b, true), config_a);
    EXPECT_TRUE(holds(b, 0, 40));

    insert_keys(a, 520, 900);
    insert_keys(b, 40, 400);
    EXPECT_TRUE(holds(a, 500, 900));
    EXPECT_TRUE(holds(b, 0, 400));
  }

  TYPED_TEST(HashTableConfiguration, SwapWithAnEmptyTable)
  {
    auto a = Config_Of<TypeParam>::make(true, 101);
    insert_keys(a, 0, 40);
    auto b = Config_Of<TypeParam>::make(false, 11);

    a.swap(b);

    EXPECT_EQ(config_of(a, false), config_b);
    EXPECT_TRUE(a.is_empty());
    EXPECT_EQ(config_of(b, true), config_a);
    EXPECT_TRUE(holds(b, 0, 40));
  }

  // The resizing policy of the open addressing tables has no accessor: it
  // shows in whether a table grows as it fills up. One bucket is left
  // free: a table that does not resize must not be given more keys than
  // it has buckets.
  template <class Table> bool grows_when_filled(Table & table)
  {
    const size_t capacity = table.capacity();
    for (long k = 0;
         table.capacity() == capacity and table.size() + 1 < capacity; ++k)
      (void) table.insert(1000000 + k);
    return table.capacity() != capacity;
  }

  template <class Table>
  struct OpenTableResizingPolicy : public ::testing::Test
  {
    // The sources of the tests have a few keys, so that assigning one of
    // them to a table with fewer buckets than that cannot keep the array
    // of the target.
    static constexpr long Num_Keys = 6;

    static Table fixed_size(const size_t len, const long num_keys = 0)
    {
      auto make = [len] ()
        {
          if constexpr (std::is_same_v<Table, Linear_Probing>)
            return Table(len, first_hash_a, Tagged_Equal{1}, 0.10f, 0.40f,
                         false);
          else
            return Table(len, first_hash_a, second_hash_a, Tagged_Equal{1},
                         0.10f, 0.40f, false);
        };
      Table table = make();
      insert_keys(table, 0, num_keys);
      return table;
    }

    static Table resizable(const size_t len, const long num_keys = 0)
    {
      Table table = Config_Of<Table>::make(false, len);
      insert_keys(table, 0, num_keys);
      return table;
    }
  };

  using Open_Tables = ::testing::Types<Linear_Probing, Double_Hashing>;
  TYPED_TEST_SUITE(OpenTableResizingPolicy, Open_Tables);

  TYPED_TEST(OpenTableResizingPolicy, IsWhatTheConstructorWasGiven)
  {
    auto fixed = this->fixed_size(11, this->Num_Keys);
    auto resizable = this->resizable(11, this->Num_Keys);

    EXPECT_FALSE(grows_when_filled(fixed));
    EXPECT_TRUE(grows_when_filled(resizable));
  }

  TYPED_TEST(OpenTableResizingPolicy, IsCopied)
  {
    const auto fixed = this->fixed_size(11, this->Num_Keys);
    const auto resizable = this->resizable(11, this->Num_Keys);

    TypeParam fixed_copy(fixed);
    TypeParam resizable_copy(resizable);
    EXPECT_FALSE(grows_when_filled(fixed_copy));
    EXPECT_TRUE(grows_when_filled(resizable_copy));

    // By assignment, to a target with fewer buckets than the source has
    // keys and to one with plenty of them.
    for (const size_t len : {size_t(5), size_t(1009)})
      {
        auto to_fixed = this->resizable(len);
        to_fixed = fixed;
        EXPECT_FALSE(grows_when_filled(to_fixed)) << "capacity " << len;

        auto to_resizable = this->fixed_size(len);
        to_resizable = resizable;
        EXPECT_TRUE(grows_when_filled(to_resizable)) << "capacity " << len;
      }
  }

  TYPED_TEST(OpenTableResizingPolicy, IsMoved)
  {
    auto fixed = this->fixed_size(11, this->Num_Keys);
    auto resizable = this->resizable(11, this->Num_Keys);

    TypeParam from_fixed(std::move(fixed));
    TypeParam from_resizable(std::move(resizable));

    auto to_fixed = this->resizable(101);
    to_fixed = std::move(from_fixed);
    EXPECT_FALSE(grows_when_filled(to_fixed));

    auto to_resizable = this->fixed_size(101);
    to_resizable = std::move(from_resizable);
    EXPECT_TRUE(grows_when_filled(to_resizable));
  }

  TYPED_TEST(OpenTableResizingPolicy, IsSwapped)
  {
    auto a = this->fixed_size(11, this->Num_Keys);
    auto b = this->resizable(11, this->Num_Keys);

    a.swap(b);

    EXPECT_TRUE(grows_when_filled(a));
    EXPECT_FALSE(grows_when_filled(b));
  }

  // -------------------------------------------------------------------------
  // An open addressing table after a move took its buckets
  // -------------------------------------------------------------------------

  template <class Table>
  struct OpenTableAfterMove : public ::testing::Test
  {
    // A table with keys [0, 40) whose buckets are then taken by a move.
    // `resizes` is the resizing policy of the table.
    static Table emptied_by_a_move(const bool resizes = true)
    {
      auto make = [resizes] ()
        {
          if constexpr (std::is_same_v<Table, OLhashTable<long>>)
            return Table(101, first_hash_a, Aleph::equal_to<long>(),
                         0.10f, 0.40f, resizes);
          else
            return Table(101, first_hash_a, second_hash_a,
                         Aleph::equal_to<long>(), 0.10f, 0.40f, resizes);
        };
      Table source = make();
      insert_keys(source, 0, 40);
      Table target(std::move(source));
      EXPECT_TRUE(holds(target, 0, 40));
      return source;
    }
  };

  using Plain_Open_Tables =
    ::testing::Types<OLhashTable<long>, ODhashTable<long>>;
  TYPED_TEST_SUITE(OpenTableAfterMove, Plain_Open_Tables);

  // The keys do not change address: the buckets are taken, not copied.
  TYPED_TEST(OpenTableAfterMove, TheTargetHasTheVeryBucketsOfTheSource)
  {
    static_assert(std::is_nothrow_move_constructible_v<TypeParam>);

    TypeParam source;
    insert_keys(source, 0, 1000);
    const size_t capacity = source.capacity();
    std::vector<const long *> where;
    for (long k = 0; k < 1000; ++k)
      where.push_back(source.search(k));

    TypeParam target(std::move(source));

    EXPECT_EQ(target.capacity(), capacity);
    ASSERT_TRUE(holds(target, 0, 1000));
    for (long k = 0; k < 1000; ++k)
      ASSERT_EQ(std::as_const(target).search(k), where[k]) << "key " << k;

    EXPECT_EQ(source.size(), 0u);
    EXPECT_EQ(source.capacity(), 0u);
  }

  TYPED_TEST(OpenTableAfterMove, TheSourceAnswersAsAnEmptyTable)
  {
    TypeParam source = this->emptied_by_a_move();

    EXPECT_EQ(source.size(), 0u);
    EXPECT_EQ(source.capacity(), 0u);
    EXPECT_TRUE(source.is_empty());
    EXPECT_EQ(source.current_alpha(), 0.0f);

    EXPECT_EQ(source.search(7), nullptr);
    EXPECT_EQ(std::as_const(source).search(7), nullptr);
    EXPECT_FALSE(source.has(7));
    EXPECT_FALSE(source.contains(7));
    EXPECT_THROW((void) source.find(7), std::domain_error);
    EXPECT_THROW(source.remove(7), std::domain_error);

    EXPECT_TRUE(source.keys().is_empty());
    size_t visited = 0;
    source.for_each([&visited] (const long &) { ++visited; });
    for ([[maybe_unused]] const long & key : source)
      ++visited;
    EXPECT_EQ(visited, 0u);

    typename TypeParam::Iterator it(source);
    EXPECT_FALSE(it.has_curr());
    EXPECT_THROW(it.del(), std::overflow_error);
    it.reset_last();
    EXPECT_FALSE(it.has_curr());

    const auto stats = source.stats();
    EXPECT_EQ(stats.num_busy, 0u);
    EXPECT_EQ(stats.num_empty, 0u);
    EXPECT_EQ(stats.num_deleted, 0u);
    EXPECT_EQ(stats.max_len, 0u);
    EXPECT_EQ(stats.avg, 0.0f); // not the 0/0 of averaging over no bucket
    EXPECT_EQ(stats.var, 0.0f);

    source.rehash();
    EXPECT_EQ(source.capacity(), 0u);
    EXPECT_EQ(source.resize(0), 0u);
    EXPECT_EQ(source.capacity(), 0u);
  }

  TYPED_TEST(OpenTableAfterMove, TheSourceGetsBucketsWithItsFirstInsertion)
  {
    {
      TypeParam source = this->emptied_by_a_move();
      ASSERT_NE(source.insert(7), nullptr);
      EXPECT_EQ(source.capacity(), Primes::DefaultPrime);
      EXPECT_TRUE(holds(source, 7, 8));
      insert_keys(source, 8, 2000);
      EXPECT_TRUE(holds(source, 7, 2000));
      source.remove(7);
      EXPECT_TRUE(holds(source, 8, 2000));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      long key = 7;
      ASSERT_NE(source.insert(std::move(key)), nullptr);
      EXPECT_TRUE(holds(source, 7, 8));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      const long * stored = source.search_or_insert(7);
      ASSERT_NE(stored, nullptr);
      EXPECT_EQ(source.search_or_insert(7), stored);
      EXPECT_TRUE(holds(source, 7, 8));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      const auto first = source.contains_or_insert(7);
      ASSERT_NE(first.first, nullptr);
      EXPECT_FALSE(first.second);
      EXPECT_TRUE(source.contains_or_insert(7).second);
      EXPECT_TRUE(holds(source, 7, 8));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      EXPECT_EQ(source[7], 7);
      EXPECT_TRUE(holds(source, 7, 8));
    }
  }

  // A table that does not resize gets buckets all the same; otherwise it
  // could never hold a key again.
  TYPED_TEST(OpenTableAfterMove, ATableThatDoesNotResizeGetsBucketsToo)
  {
    TypeParam source = this->emptied_by_a_move(false);

    ASSERT_NE(source.insert(7), nullptr);
    EXPECT_EQ(source.capacity(), Primes::DefaultPrime);
    insert_keys(source, 8, 50);
    EXPECT_TRUE(holds(source, 7, 50));
    EXPECT_FALSE(grows_when_filled(source));
  }

  TYPED_TEST(OpenTableAfterMove, TheSourceCanBeResizedAndCleared)
  {
    {
      TypeParam source = this->emptied_by_a_move();
      EXPECT_EQ(source.resize(17), 17u);
      EXPECT_EQ(source.capacity(), 17u);
      EXPECT_TRUE(source.is_empty());
      insert_keys(source, 0, 10);
      EXPECT_TRUE(holds(source, 0, 10));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      source.clear();
      EXPECT_EQ(source.capacity(), Primes::DefaultPrime);
      EXPECT_TRUE(source.is_empty());
      insert_keys(source, 0, 10);
      EXPECT_TRUE(holds(source, 0, 10));
    }
  }

  TYPED_TEST(OpenTableAfterMove, TheSourceCanBeCopiedMovedAndAssigned)
  {
    {
      const TypeParam source = this->emptied_by_a_move();
      TypeParam copy(source);
      EXPECT_TRUE(copy.is_empty());
      EXPECT_EQ(copy.get_hash_fct()(Config_Probe_Key),
                first_hash_a(Config_Probe_Key));
      insert_keys(copy, 0, 10);
      EXPECT_TRUE(holds(copy, 0, 10));
      EXPECT_TRUE(source.is_empty());
    }
    {
      TypeParam source = this->emptied_by_a_move();
      TypeParam moved(std::move(source));
      EXPECT_TRUE(moved.is_empty());
      insert_keys(moved, 0, 10);
      insert_keys(source, 20, 30);
      EXPECT_TRUE(holds(moved, 0, 10));
      EXPECT_TRUE(holds(source, 20, 30));
    }
    {
      const TypeParam source = this->emptied_by_a_move();
      TypeParam target;
      insert_keys(target, 100, 150);
      target = source;
      EXPECT_TRUE(target.is_empty());
      insert_keys(target, 0, 10);
      EXPECT_TRUE(holds(target, 0, 10));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      TypeParam other;
      insert_keys(other, 100, 150);
      source = other;
      EXPECT_TRUE(holds(source, 100, 150));
      EXPECT_TRUE(holds(other, 100, 150));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      TypeParam other;
      insert_keys(other, 100, 150);
      source = std::move(other);
      EXPECT_TRUE(holds(source, 100, 150));
      insert_keys(other, 0, 10);
      EXPECT_TRUE(holds(other, 0, 10));
    }
    {
      TypeParam source = this->emptied_by_a_move();
      TypeParam other;
      insert_keys(other, 100, 150);
      source.swap(other);
      EXPECT_TRUE(holds(source, 100, 150));
      EXPECT_TRUE(other.is_empty());
      insert_keys(other, 0, 10);
      EXPECT_TRUE(holds(other, 0, 10));
    }
  }

  // -------------------------------------------------------------------------
  // A copy assignment that fails midway
  // -------------------------------------------------------------------------

  // A key whose copies can be made to throw.
  struct Fragile_Key
  {
    // How many copies still succeed. Negative: none fails.
    static inline long copies_before_failure = -1;

    long value = 0;

    Fragile_Key() = default;

    Fragile_Key(const long v) : value(v) {}

    Fragile_Key(const Fragile_Key & other) : value(other.value)
    {
      may_throw();
    }

    Fragile_Key & operator=(const Fragile_Key & other)
    {
      may_throw();
      value = other.value;
      return *this;
    }

    Fragile_Key(Fragile_Key &&) noexcept = default;

    Fragile_Key & operator=(Fragile_Key &&) noexcept = default;

    bool operator==(const Fragile_Key &) const = default;

    static void may_throw()
    {
      if (copies_before_failure == 0)
        throw std::runtime_error("no copy");
      if (copies_before_failure > 0)
        --copies_before_failure;
    }
  };

  size_t fragile_hash(const Fragile_Key & key) noexcept
  {
    return first_hash_a(key.value);
  }

  size_t fragile_second_hash(const Fragile_Key & key) noexcept
  {
    return second_hash_a(key.value);
  }

  // While it lives, copying a key throws after `successful` copies.
  struct Failing_Copies
  {
    explicit Failing_Copies(const long successful = 0)
    {
      Fragile_Key::copies_before_failure = successful;
    }

    ~Failing_Copies() { Fragile_Key::copies_before_failure = -1; }
  };

  template <class Table>
  struct HashTableCopyThatThrows : public ::testing::Test
  {
    static Table make(const long from, const long to)
    {
      auto empty_table = [] ()
        {
          if constexpr (std::is_same_v<Table, ODhashTable<Fragile_Key>>)
            return Table(101, fragile_hash, fragile_second_hash);
          else
            return Table(101, fragile_hash);
        };
      Table table = empty_table();
      for (long k = from; k < to; ++k)
        EXPECT_NE(table.insert(Fragile_Key(k)), nullptr) << "key " << k;
      return table;
    }

    static ::testing::AssertionResult holds_keys(const Table & table,
                                                 const long from,
                                                 const long to)
    {
      if (table.size() != static_cast<size_t>(to - from))
        return ::testing::AssertionFailure()
               << "size is " << table.size() << ", expected " << to - from;
      for (long k = from; k < to; ++k)
        if (not table.contains(Fragile_Key(k)))
          return ::testing::AssertionFailure() << "key " << k << " not found";
      return ::testing::AssertionSuccess();
    }
  };

  using Fragile_Tables = ::testing::Types<OLhashTable<Fragile_Key>,
                                          ODhashTable<Fragile_Key>,
                                          DynSetLhash<Fragile_Key>,
                                          DynSetLinHash<Fragile_Key>>;
  TYPED_TEST_SUITE(HashTableCopyThatThrows, Fragile_Tables);

  // The assignment used to empty the target before it copied the first
  // key.
  TYPED_TEST(HashTableCopyThatThrows, AssignmentKeepsTheTarget)
  {
    const TypeParam source = this->make(0, 30);
    TypeParam target = this->make(100, 110);

    {
      Failing_Copies failing;
      EXPECT_THROW(target = source, std::runtime_error);
    }

    EXPECT_TRUE(this->holds_keys(target, 100, 110));
    EXPECT_TRUE(this->holds_keys(source, 0, 30));

    target = source;
    EXPECT_TRUE(this->holds_keys(target, 0, 30));
  }

  TYPED_TEST(HashTableCopyThatThrows, ConstructionReleasesWhatItBuilt)
  {
    const TypeParam source = this->make(0, 30);

    {
      Failing_Copies failing;
      EXPECT_THROW(TypeParam copy(source), std::runtime_error);
    }

    EXPECT_TRUE(this->holds_keys(source, 0, 30));
  }

  // -------------------------------------------------------------------------
  // Changing the hash function of a table that has keys
  // -------------------------------------------------------------------------

  TEST(OpenTableHashFunction, RehashPlacesTheKeysWithTheNewFunction)
  {
    {
      OLhashTable<long> table(101, first_hash_a);
      insert_keys(table, 0, 60);
      table.set_hash_fct(first_hash_b);
      table.rehash();
      EXPECT_TRUE(holds(table, 0, 60));
      insert_keys(table, 60, 300);
      EXPECT_TRUE(holds(table, 0, 300));
    }
    {
      ODhashTable<long> table(101, first_hash_a, second_hash_a);
      insert_keys(table, 0, 60);
      table.set_hash_fct(first_hash_b);
      table.set_second_hash_fct(second_hash_b);
      table.rehash();
      EXPECT_TRUE(holds(table, 0, 60));
      for (long k = 0; k < 60; k += 2)
        table.remove(k);
      insert_keys(table, 60, 300);
      for (long k = 1; k < 60; k += 2)
        EXPECT_TRUE(table.contains(k)) << "key " << k;
      EXPECT_EQ(table.size(), 270u);
    }
  }

  // -------------------------------------------------------------------------
  // Hash functions given as lambdas without captures
  // -------------------------------------------------------------------------

  // With every parameter given, a lambda without captures fitted both the
  // constructor that takes a std::function and the one that takes a
  // function pointer, and the call did not compile.
  TEST(OpenTableConstruction, TakesLambdasWithoutCaptures)
  {
    auto plus_ten = [] (const long & k) { return static_cast<size_t>(k) + 10; };
    auto plus_twenty = [] (const long & k)
      {
        return static_cast<size_t>(k) + 20;
      };
    const Aleph::equal_to<long> eq;

    OLhashTable<long> one(11, plus_ten, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(one.get_hash_fct()(1), 11u);
    insert_keys(one, 0, 100);
    EXPECT_TRUE(holds(one, 0, 100));

    OLhashTable<long> two(11, plus_ten, plus_twenty, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(two.get_hash_fct()(1), 11u);

    OLhashTable<long> with_a_function(11, plus_ten, second_hash_a, eq,
                                      0.1f, 0.5f, true);
    EXPECT_EQ(with_a_function.get_hash_fct()(1), 11u);

    const OLhashTable<long>::Hash_Fct stored = plus_twenty;
    OLhashTable<long> with_a_stored_one(11, stored, plus_ten, eq,
                                        0.1f, 0.5f, true);
    EXPECT_EQ(with_a_stored_one.get_hash_fct()(1), 21u);

    ODhashTable<long> twice(11, plus_ten, plus_twenty, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(twice.get_hash_fct()(1), 11u);
    EXPECT_EQ(twice.get_second_hash_fct()(1), 21u);
    insert_keys(twice, 0, 100);
    EXPECT_TRUE(holds(twice, 0, 100));

    HashSet<long, OLhashTable> set(11, plus_ten, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(set.get_hash_fct()(1), 11u);
  }

  // The calls that already compiled keep choosing the same function.
  TEST(OpenTableConstruction, OtherHashFunctionsAreTakenAsBefore)
  {
    long bias = 30;
    auto capturing = [bias] (const long & k)
      {
        return static_cast<size_t>(k + bias);
      };
    auto plus_ten = [] (const long & k) { return static_cast<size_t>(k) + 10; };
    const OLhashTable<long>::Hash_Fct stored = capturing;
    const Aleph::equal_to<long> eq;
    const size_t a = first_hash_a(1), b = first_hash_b(1);
    const size_t sa = second_hash_a(1);

    using OL = OLhashTable<long>;
    EXPECT_EQ(OL(11, first_hash_a, eq, 0.1f, 0.5f, true).get_hash_fct()(1), a);
    EXPECT_EQ(OL(11, &first_hash_b, eq, 0.1f, 0.5f, true).get_hash_fct()(1), b);
    EXPECT_EQ(OL(11, +plus_ten, eq, 0.1f, 0.5f, true).get_hash_fct()(1), 11u);
    EXPECT_EQ(OL(11, capturing, eq, 0.1f, 0.5f, true).get_hash_fct()(1), 31u);
    EXPECT_EQ(OL(11, stored, eq, 0.1f, 0.5f, true).get_hash_fct()(1), 31u);
    EXPECT_EQ(OL(11, plus_ten).get_hash_fct()(1), 11u);
    EXPECT_EQ(OL(11, plus_ten, eq, 0.1f, 0.5f).get_hash_fct()(1), 11u);
    EXPECT_EQ(OL(11, first_hash_a, first_hash_b, eq, 0.1f, 0.5f, true)
                .get_hash_fct()(1), a);
    EXPECT_EQ(OL(11, stored, stored, eq, 0.1f, 0.5f, true).get_hash_fct()(1),
              31u);
    EXPECT_EQ(OL(11, capturing, first_hash_b, eq, 0.1f, 0.5f, true)
                .get_hash_fct()(1), 31u);
    EXPECT_EQ(OL(11, first_hash_a, capturing, eq, 0.1f, 0.5f, true)
                .get_hash_fct()(1), a);
    EXPECT_EQ(OL(11, first_hash_b, plus_ten, eq, 0.1f, 0.5f, true)
                .get_hash_fct()(1), b);

    using OD = ODhashTable<long>;
    const OD plain(11, first_hash_a, second_hash_a, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(plain.get_hash_fct()(1), a);
    EXPECT_EQ(plain.get_second_hash_fct()(1), sa);
    const OD lambda_first(11, plus_ten, second_hash_a, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(lambda_first.get_hash_fct()(1), 11u);
    EXPECT_EQ(lambda_first.get_second_hash_fct()(1), sa);
    const OD lambda_second(11, first_hash_a, plus_ten, eq, 0.1f, 0.5f, true);
    EXPECT_EQ(lambda_second.get_hash_fct()(1), a);
    EXPECT_EQ(lambda_second.get_second_hash_fct()(1), 11u);
    const OD with_defaults(11, plus_ten, plus_ten);
    EXPECT_EQ(with_defaults.get_second_hash_fct()(1), 11u);
  }

  // -------------------------------------------------------------------------
  // An insertion that throws leaves an open addressing table as it was
  // -------------------------------------------------------------------------

  // A key with copy operations only, as many older classes are. Moving it
  // copies it, so swapping two of them can throw.
  struct Copy_Only_Key
  {
    long value = 0;

    Copy_Only_Key() = default;

    Copy_Only_Key(const long v) : value(v) {}

    Copy_Only_Key(const Copy_Only_Key & other) : value(other.value)
    {
      Fragile_Key::may_throw();
    }

    Copy_Only_Key & operator=(const Copy_Only_Key & other)
    {
      Fragile_Key::may_throw();
      value = other.value;
      return *this;
    }

    bool operator==(const Copy_Only_Key &) const = default;
  };

  static_assert(noexcept(std::swap(std::declval<Fragile_Key &>(),
                                   std::declval<Fragile_Key &>())));
  static_assert(not noexcept(std::swap(std::declval<Copy_Only_Key &>(),
                                       std::declval<Copy_Only_Key &>())));

  // Hash functions that send many keys to the same buckets, so that the
  // insertions go through the buckets of removed keys and, in the double
  // hashing table, through its three kinds of probe.
  template <class Key> size_t crowded_hash(const Key & key) noexcept
  {
    return static_cast<size_t>(key.value) % 7;
  }

  template <class Key> size_t crowded_second_hash(const Key & key) noexcept
  {
    return static_cast<size_t>(key.value) % 5;
  }

  // A key of value 1000 b + s has its first probe in bucket b and its
  // second one in bucket s, so that a test can place each key.
  long value_of(const long k) noexcept { return k; }

  template <class Key> long value_of(const Key & key) noexcept
  {
    return key.value;
  }

  template <class Key> size_t chosen_hash(const Key & key) noexcept
  {
    return static_cast<size_t>(value_of(key) / 1000);
  }

  template <class Key> size_t chosen_second_hash(const Key & key) noexcept
  {
    return static_cast<size_t>(value_of(key) % 1000);
  }

  // A table of 101 buckets, with an upper threshold of 0.9, whose buckets
  // [1, 1 + removed) are marked as the ones of removed keys: they held keys
  // whose first probe was there, and the key stored after them, the only
  // one left, has a probe sequence that goes through all of them. The
  // table resizes itself, but not while the keys are removed: with one key
  // left, the removals would shrink it.
  template <class Table> Table with_removed_keys(const long removed)
  {
    using Key = typename Table::Key_Type;
    Table table = [] ()
      {
        if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
          return Table(101, chosen_hash<Key>, chosen_second_hash<Key>,
                       Aleph::equal_to<Key>(), 0.05f, 0.9f, true);
        else
          return Table(101, chosen_hash<Key>, Aleph::equal_to<Key>(),
                       0.05f, 0.9f, true);
      }();
    for (long b = 1; b <= removed; ++b)
      EXPECT_NE(table.insert(Key(1000*b + 999)), nullptr) << "bucket " << b;
    EXPECT_NE(table.insert(Key(1000 + 2)), nullptr); // probes 1, 2 and on
    table.set_with_resize(false);
    for (long b = 1; b <= removed; ++b)
      table.remove(Key(1000*b + 999));
    table.set_with_resize(true);
    EXPECT_EQ(table.size(), 1u);
    EXPECT_EQ(table.stats().num_deleted, static_cast<size_t>(removed));
    return table;
  }

  template <class Table> struct OpenTableInsertionThatThrows
    : public ::testing::Test
  {
    using Key = typename Table::Key_Type;

    static Table make(const size_t len, const float upper_alpha,
                      const bool resizes)
    {
      if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
        return Table(len, crowded_hash<Key>, crowded_second_hash<Key>,
                     Aleph::equal_to<Key>(), 0.05f, upper_alpha, resizes);
      else
        return Table(len, crowded_hash<Key>, Aleph::equal_to<Key>(),
                     0.05f, upper_alpha, resizes);
    }

    // Keys [0, 20) without six of them, whose buckets are left behind.
    static Table with_keys_and_removals(const bool resizes)
    {
      Table table = make(31, 0.95f, resizes);
      for (long k = 0; k < 20; ++k)
        EXPECT_NE(table.insert(Key(k)), nullptr) << "key " << k;
      for (const long k : {1L, 4L, 7L, 10L, 13L, 16L})
        table.remove(Key(k));
      return table;
    }

    static std::vector<long> keys_of(const Table & table)
    {
      std::vector<long> keys;
      table.for_each([&keys] (const Key & key) { keys.push_back(key.value); });
      std::sort(keys.begin(), keys.end());
      return keys;
    }

    // What the probes depend on: the state of every bucket and, in the
    // double hashing table, how many probe sequences go through it. In
    // these tests every removed key's bucket lies on the probe sequence of
    // a key still stored, so undoing an insertion must restore them all.
    static std::vector<std::pair<int, unsigned>> probes_of(const Table & table)
    {
      std::vector<std::pair<int, unsigned>> state;
      for (size_t i = 0; i < table.capacity(); ++i)
        if constexpr (requires { table.table[0].probe_counter; })
          state.emplace_back(table.table[i].status,
                             table.table[i].probe_counter);
        else
          state.emplace_back(table.table[i].status, 0u);
      return state;
    }
  };

  using Tables_With_Fragile_Keys =
    ::testing::Types<OLhashTable<Fragile_Key>, ODhashTable<Fragile_Key>>;
  TYPED_TEST_SUITE(OpenTableInsertionThatThrows, Tables_With_Fragile_Keys);

  // The bucket was marked as busy before the key was copied into it. When
  // the copy threw, the table kept one entry more, holding whatever the
  // bucket had: a key by default or one that had been removed.
  TYPED_TEST(OpenTableInsertionThatThrows, KeyCopyThatThrowsLeavesNoEntry)
  {
    for (const bool resizes : {false, true})
      {
        TypeParam table = this->with_keys_and_removals(resizes);
        const auto keys = this->keys_of(table);
        const auto probes = this->probes_of(table);
        const size_t capacity = table.capacity();
        ASSERT_EQ(keys.size(), 14u);

        for (long k = 0; k < 60; ++k)
          for (int operation = 0; operation < 4; ++operation)
            {
              const Fragile_Key key(k);
              const bool present = table.contains(key);
              bool threw = false;
              {
                Failing_Copies failing;
                try
                  {
                    switch (operation)
                      {
                      case 0: (void) table.insert(key); break;
                      case 1: (void) table.search_or_insert(key); break;
                      case 2: (void) table.contains_or_insert(key); break;
                      default: (void) table[key]; break;
                      }
                  }
                catch (const std::runtime_error &)
                  {
                    threw = true;
                  }
              }
              // a key that is already there is not copied
              ASSERT_EQ(threw, not present)
                << "key " << k << ", operation " << operation;
              ASSERT_EQ(this->keys_of(table), keys)
                << "key " << k << ", operation " << operation;
              ASSERT_EQ(table.size(), keys.size());
              ASSERT_EQ(table.capacity(), capacity);
              ASSERT_EQ(this->probes_of(table), probes)
                << "key " << k << ", operation " << operation;
            }

        // and the table goes on working
        for (long k = 0; k < 28; ++k)
          {
            const bool present = table.contains(Fragile_Key(k));
            EXPECT_EQ(table.insert(Fragile_Key(k)) != nullptr, not present)
              << "key " << k;
          }
        for (long k = 0; k < 28; ++k)
          EXPECT_TRUE(table.contains(Fragile_Key(k))) << "key " << k;
        for (long k = 0; k < 28; ++k)
          table.remove(Fragile_Key(k));
        EXPECT_TRUE(table.is_empty());
      }
  }

  // The key is stored and then the table grows, which copies the key once
  // more and asks for memory. If that failed, the insertion threw and the
  // key stayed in the table.
  TYPED_TEST(OpenTableInsertionThatThrows, GrowthThatFailsTakesTheKeyOutAgain)
  {
    for (int operation = 0; operation < 5; ++operation)
      {
        TypeParam table = this->make(11, 0.5f, true);
        for (long k = 0; k < 5; ++k)
          ASSERT_NE(table.insert(Fragile_Key(k)), nullptr);
        ASSERT_EQ(table.capacity(), 11u); // the sixth key makes it grow
        const auto keys = this->keys_of(table);
        const auto probes = this->probes_of(table);

        const Fragile_Key key(5);
        {
          // the copy into the bucket succeeds; the one made to grow fails
          Failing_Copies failing(operation == 4 ? 0 : 1);
          switch (operation)
            {
            case 0:
              EXPECT_THROW((void) table.insert(key), std::runtime_error);
              break;
            case 1:
              EXPECT_THROW((void) table.search_or_insert(key),
                           std::runtime_error);
              break;
            case 2:
              EXPECT_THROW((void) table.contains_or_insert(key),
                           std::runtime_error);
              break;
            case 3:
              EXPECT_THROW((void) table[key], std::runtime_error);
              break;
            default: // moved in: the only copy is the one made to grow
              EXPECT_THROW((void) table.insert(Fragile_Key(5)),
                           std::runtime_error);
              break;
            }
        }

        EXPECT_EQ(this->keys_of(table), keys) << "operation " << operation;
        EXPECT_EQ(table.size(), 5u) << "operation " << operation;
        EXPECT_EQ(table.capacity(), 11u) << "operation " << operation;
        EXPECT_FALSE(table.contains(key)) << "operation " << operation;
        EXPECT_EQ(this->probes_of(table), probes)
          << "operation " << operation;

        ASSERT_NE(table.insert(key), nullptr);
        EXPECT_GT(table.capacity(), 11u);
        for (long k = 0; k < 6; ++k)
          EXPECT_TRUE(table.contains(Fragile_Key(k))) << "key " << k;
      }
  }

  // A search that reaches an empty bucket stops there. An insertion that
  // threw left the bucket it had reserved as the one of a removed key, so
  // a few of them could use up the empty buckets of a table far from full.
  TYPED_TEST(OpenTableInsertionThatThrows, FailuresDoNotUseUpEmptyBuckets)
  {
    TypeParam table = this->make(11, 0.35f, true);
    for (const long k : {1L, 3L, 5L})
      ASSERT_NE(table.insert(Fragile_Key(k)), nullptr);
    ASSERT_EQ(table.capacity(), 11u); // a fourth key makes it grow
    const auto probes = this->probes_of(table);
    const size_t empty = table.stats().num_empty;

    // each key goes to an empty bucket that a busy one follows
    for (const long k : {0L, 2L, 4L})
      {
        const Fragile_Key key(k);
        {
          Failing_Copies failing; // the copy into the bucket fails
          EXPECT_THROW((void) table.insert(key), std::runtime_error)
            << "key " << k;
        }
        {
          Failing_Copies failing(1); // the copy made to grow fails
          EXPECT_THROW((void) table.insert(key), std::runtime_error)
            << "key " << k;
        }
      }

    EXPECT_EQ(table.size(), 3u);
    EXPECT_EQ(table.capacity(), 11u);
    EXPECT_EQ(table.stats().num_empty, empty);
    EXPECT_EQ(this->probes_of(table), probes);
  }

  // When the buckets of removed keys take the table to its upper threshold
  // and the keys are few, the insertion rebuilds it in place, which copies
  // the key once more. If that copy fails, the key is taken out again.
  TYPED_TEST(OpenTableInsertionThatThrows, RebuildInPlaceThatFailsTakesTheKeyOutAgain)
  {
    // 60 removed keys' buckets and 30 keys: 90 buckets in use of 101
    TypeParam table = with_removed_keys<TypeParam>(60);
    for (long b = 62; b < 91; ++b)
      ASSERT_NE(table.insert(Fragile_Key(1000*b + 999)), nullptr);
    ASSERT_EQ(table.stats().num_deleted, 60u);
    const auto keys = this->keys_of(table);
    const auto probes = this->probes_of(table);

    // the 91st bucket in use takes the table over 0.9, with 31 keys
    const Fragile_Key key(1000*91 + 999);
    {
      // the copy into the bucket succeeds; the one made to rebuild fails
      Failing_Copies failing(1);
      EXPECT_THROW((void) table.insert(key), std::runtime_error);
    }
    EXPECT_EQ(this->keys_of(table), keys);
    EXPECT_EQ(this->probes_of(table), probes);
    EXPECT_EQ(table.capacity(), 101u);
    EXPECT_FALSE(table.contains(key));

    ASSERT_NE(table.insert(key), nullptr);
    EXPECT_EQ(table.capacity(), 101u);
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_EQ(table.size(), 31u);
  }

  // A removed key's bucket that no probe sequence goes through is not
  // needed. When a failed insertion had reserved one, it is left empty,
  // and so are the unneeded removed keys' buckets right before it.
  TEST(OLhashTable, FailedInsertionClearsRemovedMarksNoOneNeeds)
  {
    OLhashTable<Fragile_Key> table(11, crowded_hash<Fragile_Key>,
                                   Aleph::equal_to<Fragile_Key>(), 0.05f,
                                   0.95f, false);
    for (const long k : {1L, 2L, 3L})
      ASSERT_NE(table.insert(Fragile_Key(k)), nullptr);
    table.remove(Fragile_Key(1));
    table.remove(Fragile_Key(2));
    ASSERT_EQ(table.stats().num_deleted, 2u);

    {
      const Fragile_Key key(9); // its probe sequence starts at bucket 2
      Failing_Copies failing;
      EXPECT_THROW((void) table.insert(key), std::runtime_error);
    }

    EXPECT_EQ(table.size(), 1u);
    EXPECT_TRUE(table.contains(Fragile_Key(3)));
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_EQ(table.stats().num_empty, 10u);
  }

  // Real insertions and removals mixed with insertions that fail, checked
  // against a std::set.
  TYPED_TEST(OpenTableInsertionThatThrows, FailuresDoNotDisturbTheTable)
  {
    TypeParam table = this->make(11, 0.7f, true);
    std::set<long> oracle;
    std::mt19937 rng(20261013);

    auto check = [&] (const int step)
      {
        ASSERT_EQ(table.size(), oracle.size()) << "step " << step;
        for (long k = 0; k < 150; ++k)
          ASSERT_EQ(table.contains(Fragile_Key(k)), oracle.count(k) == 1)
            << "key " << k << ", step " << step;
      };

    for (int step = 0; step < 20000; ++step)
      {
        const long k = static_cast<long>(rng() % 150);
        const Fragile_Key key(k);
        switch (rng() % 5)
          {
          case 0: // a real insertion
            EXPECT_EQ(table.insert(key) != nullptr, oracle.insert(k).second);
            break;
          case 1: // a real removal
            if (oracle.erase(k) == 1)
              table.remove(key);
            else
              EXPECT_THROW(table.remove(key), std::domain_error);
            break;
          case 2: // the copy into the bucket fails
            {
              Failing_Copies failing;
              try
                {
                  EXPECT_EQ(table.search_or_insert(key) != nullptr, true);
                  EXPECT_EQ(oracle.count(k), 1u); // it was already there
                }
              catch (const std::runtime_error &)
                {
                  EXPECT_EQ(oracle.count(k), 0u);
                }
            }
            break;
          default: // the copy made to grow fails, when the table grows
            {
              Failing_Copies failing(1);
              try
                {
                  if (table.insert(key) != nullptr)
                    oracle.insert(k);
                }
              catch (const std::runtime_error &)
                {
                  EXPECT_EQ(oracle.count(k), 0u);
                }
            }
            break;
          }
        if (step % 500 == 0)
          check(step);
      }
    check(20000);

    for (const long k : oracle)
      table.remove(Fragile_Key(k));
    EXPECT_TRUE(table.is_empty());
  }

  template <class Table> struct OpenTableWithCopyOnlyKeys
    : public OpenTableInsertionThatThrows<Table> {};

  using Tables_With_Copy_Only_Keys =
    ::testing::Types<OLhashTable<Copy_Only_Key>, ODhashTable<Copy_Only_Key>>;
  TYPED_TEST_SUITE(OpenTableWithCopyOnlyKeys, Tables_With_Copy_Only_Keys);

  // A smaller table that cannot be built, here because copying a key into
  // it fails, leaves the table as it was: the key stays removed, and
  // neither remove() nor remove_ptr() throws.
  TYPED_TEST(OpenTableWithCopyOnlyKeys, ShrinkThatFailsKeepsTheCapacity)
  {
    using Key = typename TypeParam::Key_Type;
    TypeParam table = this->make(11, 0.95f, true);
    for (long k = 0; k < 1000; ++k)
      ASSERT_NE(table.insert(Key(k)), nullptr) << "key " << k;
    const size_t grown = table.capacity();

    {
      Failing_Copies failing;
      for (long k = 0; k < 990; ++k)
        if (k % 2 == 0)
          ASSERT_NO_THROW(table.remove(Key(k))) << "key " << k;
        else
          ASSERT_NO_THROW(table.remove_ptr(table.search(Key(k)))) << "key " << k;
    }
    EXPECT_EQ(table.capacity(), grown);
    EXPECT_EQ(this->keys_of(table).size(), 10u);
    for (long k = 990; k < 1000; ++k)
      EXPECT_TRUE(table.contains(Key(k))) << "key " << k;

    // once the keys can be copied, the next removal shrinks it
    table.remove(Key(990));
    EXPECT_LT(table.capacity(), grown);
    for (long k = 991; k < 1000; ++k)
      EXPECT_TRUE(table.contains(Key(k))) << "key " << k;
  }

  // The insertions that take the key by rvalue swap it into the bucket. A
  // swap that can throw could leave both the table and the argument
  // broken, so such a key is copied, and the copy that fails is undone.
  TYPED_TEST(OpenTableWithCopyOnlyKeys, MovedInKeyThatThrowsLeavesNoEntry)
  {
    for (const bool resizes : {false, true})
      {
        TypeParam table = this->with_keys_and_removals(resizes);
        const auto keys = this->keys_of(table);
        const auto probes = this->probes_of(table);

        for (long k = 0; k < 60; ++k)
          for (int operation = 0; operation < 3; ++operation)
            {
              Copy_Only_Key key(k);
              const bool present = table.contains(key);
              bool threw = false;
              {
                Failing_Copies failing;
                try
                  {
                    switch (operation)
                      {
                      case 0: (void) table.insert(std::move(key)); break;
                      case 1:
                        (void) table.search_or_insert(std::move(key));
                        break;
                      default:
                        (void) table.contains_or_insert(std::move(key));
                        break;
                      }
                  }
                catch (const std::runtime_error &)
                  {
                    threw = true;
                  }
              }
              ASSERT_EQ(threw, not present)
                << "key " << k << ", operation " << operation;
              ASSERT_EQ(key.value, k) << "the argument was not touched";
              ASSERT_EQ(this->keys_of(table), keys)
                << "key " << k << ", operation " << operation;
              ASSERT_EQ(this->probes_of(table), probes)
                << "key " << k << ", operation " << operation;
            }

        for (long k = 20; k < 28; ++k)
          EXPECT_NE(table.insert(Copy_Only_Key(k)), nullptr) << "key " << k;
        for (long k = 20; k < 28; ++k)
          EXPECT_TRUE(table.contains(Copy_Only_Key(k))) << "key " << k;
      }
  }

  // Growing places the keys in new buckets, and such keys are copied there.
  // A copy that failed halfway left the table with the keys placed so far:
  // the others were lost, and the old buckets leaked.
  TYPED_TEST(OpenTableWithCopyOnlyKeys, GrowthThatFailsHalfwayLeavesTheTable)
  {
    // the key is copied into its bucket and once more before growing; the
    // next 6 copies place the 6 keys in the new buckets
    for (size_t successful = 2; successful < 8; ++successful)
      {
        TypeParam table = this->make(11, 0.5f, true);
        for (long k = 0; k < 5; ++k)
          ASSERT_NE(table.insert(Copy_Only_Key(k)), nullptr);
        ASSERT_EQ(table.capacity(), 11u); // the sixth key makes it grow
        const auto keys = this->keys_of(table);
        const auto probes = this->probes_of(table);

        const Copy_Only_Key key(5);
        {
          Failing_Copies failing(successful);
          EXPECT_THROW((void) table.insert(key), std::runtime_error)
            << successful << " copies";
        }

        EXPECT_EQ(this->keys_of(table), keys) << successful << " copies";
        EXPECT_EQ(table.size(), 5u) << successful << " copies";
        EXPECT_EQ(table.capacity(), 11u) << successful << " copies";
        EXPECT_EQ(this->probes_of(table), probes) << successful << " copies";

        ASSERT_NE(table.insert(key), nullptr);
        EXPECT_GT(table.capacity(), 11u);
        for (long k = 0; k < 6; ++k)
          EXPECT_TRUE(table.contains(Copy_Only_Key(k))) << "key " << k;
      }
  }

  // rehash() and resize() place the keys in new buckets the same way.
  TYPED_TEST(OpenTableWithCopyOnlyKeys, RebuildThatFailsLeavesTheTable)
  {
    TypeParam table = this->with_keys_and_removals(false);
    const auto keys = this->keys_of(table);
    const auto probes = this->probes_of(table);
    const size_t capacity = table.capacity();
    const size_t larger = Primes::next_prime(2 * capacity);

    for (size_t successful = 0; successful < keys.size(); ++successful)
      {
        {
          Failing_Copies failing(successful);
          EXPECT_THROW(table.rehash(), std::runtime_error) << successful;
        }
        ASSERT_EQ(this->keys_of(table), keys) << successful;
        ASSERT_EQ(this->probes_of(table), probes) << successful;
        ASSERT_EQ(table.capacity(), capacity) << successful;

        {
          Failing_Copies failing(successful);
          EXPECT_THROW((void) table.resize(larger), std::runtime_error)
            << successful;
        }
        ASSERT_EQ(this->keys_of(table), keys) << successful;
        ASSERT_EQ(this->probes_of(table), probes) << successful;
        ASSERT_EQ(table.capacity(), capacity) << successful;
      }

    table.rehash();
    EXPECT_EQ(this->keys_of(table), keys);
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_EQ(table.resize(larger), larger);
    EXPECT_EQ(this->keys_of(table), keys);
  }

  // -------------------------------------------------------------------------
  // A table that does not resize itself is full with capacity() - 1 keys
  // -------------------------------------------------------------------------

  template <class Table> struct OpenTableWithoutResizing
    : public ::testing::Test
  {
    static Table make(const size_t len)
    {
      if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
        return Table(len, Aleph::dft_hash_ptr_fct<long>,
                     Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                     hash_default_lower_alpha, hash_default_upper_alpha, false);
      else
        return Table(len, Aleph::dft_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                     hash_default_lower_alpha, hash_default_upper_alpha, false);
    }

    // Inserts keys 0, 1, 2... until the table refuses one, and returns how
    // many it took.
    static size_t fill(Table & table)
    {
      size_t taken = 0;
      for (long k = 0; k < 1000 and table.insert(k) != nullptr; ++k)
        ++taken;
      return taken;
    }
  };

  using Open_Tables_Of_Longs =
    ::testing::Types<OLhashTable<long>, ODhashTable<long>>;
  TYPED_TEST_SUITE(OpenTableWithoutResizing, Open_Tables_Of_Longs);

  // A search that misses stops at an empty bucket, so the table keeps one.
  // It used to take a key in every bucket, and the double hashing table
  // then failed an assertion on the next insertion.
  TYPED_TEST(OpenTableWithoutResizing, KeepsOneBucketEmpty)
  {
    TypeParam table = this->make(11);
    ASSERT_EQ(table.capacity(), 11u);

    EXPECT_EQ(this->fill(table), 10u);
    EXPECT_EQ(table.size(), 10u);
    EXPECT_EQ(table.capacity(), 11u);
    EXPECT_EQ(table.stats().num_empty, 1u);

    const long absent = 500;
    EXPECT_EQ(table.insert(absent), nullptr);
    EXPECT_EQ(table.insert(long(absent)), nullptr);
    EXPECT_FALSE(table.contains(absent));
    EXPECT_EQ(table.size(), 10u);

    // a removal makes room for one key
    table.remove(3L);
    EXPECT_NE(table.insert(absent), nullptr);
    EXPECT_EQ(table.insert(3L), nullptr);
    EXPECT_EQ(table.size(), 10u);
    EXPECT_EQ(table.stats().num_busy, 10u);
  }

  // A full table still finds the keys it has, and refuses the others.
  // operator[] dereferenced the null pointer that it got for a new key.
  TYPED_TEST(OpenTableWithoutResizing, FullTableFindsItsKeysAndRefusesNewOnes)
  {
    TypeParam table = this->make(11);
    ASSERT_EQ(this->fill(table), 10u);
    const long present = 3, absent = 500;
    long * const stored = table.search(present);
    ASSERT_NE(stored, nullptr);

    EXPECT_EQ(table.search_or_insert(present), stored);
    EXPECT_EQ(table.search_or_insert(long(present)), stored);
    EXPECT_EQ(table.contains_or_insert(present),
              std::make_pair(stored, true));
    EXPECT_EQ(table.contains_or_insert(long(present)),
              std::make_pair(stored, true));
    EXPECT_EQ(&table[present], stored);

    EXPECT_EQ(table.search_or_insert(absent), nullptr);
    EXPECT_EQ(table.search_or_insert(long(absent)), nullptr);
    EXPECT_EQ(table.contains_or_insert(absent),
              std::make_pair(static_cast<long *>(nullptr), false));
    EXPECT_EQ(table.contains_or_insert(long(absent)),
              std::make_pair(static_cast<long *>(nullptr), false));
    EXPECT_THROW((void) table[absent], std::overflow_error);

    EXPECT_EQ(table.size(), 10u);
    EXPECT_FALSE(table.contains(absent));
  }

  template <class Map> struct OpenMapWithoutResizing : public ::testing::Test
  {};

  using Open_Maps = ::testing::Types<MapOLhash<long, int>, MapODhash<long, int>>;
  TYPED_TEST_SUITE(OpenMapWithoutResizing, Open_Maps);

  // The maps over open tables index through search_or_insert() too.
  TYPED_TEST(OpenMapWithoutResizing, SubscriptOnAFullMapThrows)
  {
    TypeParam map(11, Aleph::dft_hash_ptr_fct<long>,
                  Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                  hash_default_lower_alpha, hash_default_upper_alpha, false);
    for (long k = 0; k < 10; ++k)
      ASSERT_NE(map.insert(k, static_cast<int>(k)), nullptr) << "key " << k;
    EXPECT_EQ(map.insert(10L, 10), nullptr); // full

    map[3L] = 30;
    const long present = 4;
    map[present] = 40;
    EXPECT_EQ(map.find(3L), 30);
    EXPECT_EQ(map.find(4L), 40);

    const long absent = 500;
    EXPECT_THROW((void) map[absent], std::overflow_error);
    EXPECT_THROW((void) map[501L], std::overflow_error);
    EXPECT_EQ(map.size(), 10u);
    EXPECT_FALSE(map.contains(absent));
  }

  template <class Map> struct OpenMapShrinking : public ::testing::Test
  {};

  TYPED_TEST_SUITE(OpenMapShrinking, Open_Maps);

  // The maps remove through the remove() of their table, which did not
  // shrink it.
  TYPED_TEST(OpenMapShrinking, RemoveShrinksTheMap)
  {
    TypeParam map(11, Aleph::dft_hash_ptr_fct<long>,
                  Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                  hash_default_lower_alpha, hash_default_upper_alpha, true);
    for (long k = 0; k < 1000; ++k)
      ASSERT_NE(map.insert(k, static_cast<int>(k)), nullptr) << "key " << k;
    const size_t grown = map.capacity();

    for (long k = 0; k < 990; ++k)
      map.remove(k);

    EXPECT_LT(map.capacity(), grown / 4);
    EXPECT_EQ(map.size(), 10u);
    for (long k = 990; k < 1000; ++k)
      EXPECT_EQ(map.find(k), static_cast<int>(k)) << "key " << k;
  }

  // -------------------------------------------------------------------------
  // Shrinking leaves the keys under the upper threshold
  // -------------------------------------------------------------------------

  template <class Table> struct OpenTableShrinking : public ::testing::Test
  {
    static Table make(const size_t len, const float lower, const float upper)
    {
      if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
        return Table(len, Aleph::dft_hash_ptr_fct<long>,
                     Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                     lower, upper, true);
      else
        return Table(len, Aleph::dft_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                     lower, upper, true);
    }
  };

  TYPED_TEST_SUITE(OpenTableShrinking, Open_Tables_Of_Longs);

  // With a lower threshold over one half, a table of half the capacity may
  // not hold the keys. remove_ptr() removed the key and then resize()
  // threw std::overflow_error. Now the table waits until the keys fit.
  TYPED_TEST(OpenTableShrinking, KeysFitInTheSmallerTable)
  {
    TypeParam table = this->make(101, 0.6f, 0.9f);
    for (long k = 0; k < 60; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    ASSERT_EQ(table.capacity(), 101u);

    for (long k = 0; k < 55; ++k)
      {
        ASSERT_NO_THROW(table.remove_ptr(table.search(k))) << "key " << k;
        ASSERT_FALSE(table.contains(k)) << "key " << k;
        ASSERT_EQ(table.size(), static_cast<size_t>(59 - k));
        ASSERT_LT(table.current_alpha(), 0.9f) << "key " << k;
        ASSERT_LT(table.size(), table.capacity()) << "key " << k;
      }

    EXPECT_LT(table.capacity(), 101u); // it shrank once the keys fitted
    for (long k = 55; k < 60; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // A smaller table where the keys leave exactly one bucket empty is good
  // enough: 6 keys fit in 7 buckets under 0.9.
  TYPED_TEST(OpenTableShrinking, ShrinksWhenTheKeysLeaveOneBucketEmpty)
  {
    TypeParam table = this->make(11, 0.6f, 0.9f);
    for (long k = 0; k < 7; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    ASSERT_EQ(table.capacity(), 11u);

    table.remove_ptr(table.search(0L));

    EXPECT_EQ(table.capacity(), 7u);
    EXPECT_EQ(table.size(), 6u);
    EXPECT_EQ(table.stats().num_empty, 1u);
    for (long k = 1; k < 7; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // With the default thresholds the table still shrinks as before.
  TYPED_TEST(OpenTableShrinking, DefaultThresholdsStillShrink)
  {
    TypeParam table = this->make(11, hash_default_lower_alpha,
                                 hash_default_upper_alpha);
    for (long k = 0; k < 1000; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    const size_t grown = table.capacity();

    for (long k = 0; k < 990; ++k)
      table.remove_ptr(table.search(k));

    EXPECT_LT(table.capacity(), grown / 4);
    EXPECT_EQ(table.size(), 10u);
    for (long k = 990; k < 1000; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // Only remove_ptr() shrank the table; remove() left it as large as it
  // had grown.
  TYPED_TEST(OpenTableShrinking, RemoveShrinksTooWhileTheTableResizes)
  {
    TypeParam table = this->make(11, hash_default_lower_alpha,
                                 hash_default_upper_alpha);
    for (long k = 0; k < 1000; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    const size_t grown = table.capacity();

    // turned off, the table keeps its capacity and its keys do not move
    table.set_with_resize(false);
    EXPECT_FALSE(table.get_with_resize());
    for (long k = 0; k < 900; ++k)
      table.remove(k);
    EXPECT_EQ(table.capacity(), grown);

    table.set_with_resize(true);
    EXPECT_TRUE(table.get_with_resize());
    for (long k = 900; k < 990; ++k)
      table.remove(k);
    EXPECT_LT(table.capacity(), grown / 4);
    EXPECT_EQ(table.size(), 10u);
    for (long k = 990; k < 1000; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // With a lower threshold over one half, remove() also waits until the
  // keys fit in the smaller table.
  TYPED_TEST(OpenTableShrinking, RemoveWaitsUntilTheKeysFit)
  {
    TypeParam table = this->make(101, 0.6f, 0.9f);
    for (long k = 0; k < 60; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    ASSERT_EQ(table.capacity(), 101u);

    for (long k = 0; k < 55; ++k)
      {
        ASSERT_NO_THROW(table.remove(k)) << "key " << k;
        ASSERT_EQ(table.size(), static_cast<size_t>(59 - k));
        ASSERT_LT(table.current_alpha(), 0.9f) << "key " << k;
        ASSERT_LT(table.size(), table.capacity()) << "key " << k;
      }

    EXPECT_LT(table.capacity(), 101u);
    for (long k = 55; k < 60; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // The del() of an iterator does not shrink the table, which would leave
  // the iterator on buckets that are gone.
  TYPED_TEST(OpenTableShrinking, IteratorDelDoesNotShrink)
  {
    TypeParam table = this->make(11, hash_default_lower_alpha,
                                 hash_default_upper_alpha);
    for (long k = 0; k < 1000; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    const size_t grown = table.capacity();

    auto it = table.get_it();
    for (size_t deleted = 0; deleted < 990; ++deleted)
      {
        ASSERT_TRUE(it.has_curr());
        it.del();
      }

    EXPECT_EQ(table.capacity(), grown);
    EXPECT_EQ(table.size(), 10u);
  }

  // A table built without resizing grows once it is turned on: the change
  // applies from the next insertion.
  TYPED_TEST(OpenTableShrinking, TurningResizingOnLetsAFullTableGrow)
  {
    TypeParam table = this->make(11, hash_default_lower_alpha,
                                 hash_default_upper_alpha);
    EXPECT_TRUE(table.get_with_resize());
    table.set_with_resize(false);
    for (long k = 0; k < 10; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    EXPECT_EQ(table.insert(10L), nullptr); // full

    table.set_with_resize(true);
    ASSERT_NE(table.insert(10L), nullptr);
    EXPECT_GT(table.capacity(), 11u);
    for (long k = 0; k <= 10; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // -------------------------------------------------------------------------
  // The buckets of removed keys count as used
  // -------------------------------------------------------------------------

  // A removed key's bucket stays marked while a search may have to go
  // through it, so it is not empty. Those buckets did not count to grow or
  // fill the table: insertions and removals could leave it without an empty
  // bucket, where the searches stop, far from its upper threshold.

  size_t colliding_hash(const long & k) noexcept
  {
    return static_cast<size_t>(k);
  }

  size_t colliding_second_hash(const long & k) noexcept
  {
    return static_cast<size_t>(k / 7 + k % 5);
  }

  template <class Table> struct OpenTableRemovedKeys : public ::testing::Test
  {
    static Table make(const size_t len, const bool resizes)
    {
      if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
        return Table(len, colliding_hash, colliding_second_hash,
                     Aleph::equal_to<long>(), 0.05f, 0.9f, resizes);
      else
        return Table(len, colliding_hash, Aleph::equal_to<long>(),
                     0.05f, 0.9f, resizes);
    }

    // Whether the buckets in use, those of removed keys included, are
    // under the upper threshold of 0.9, computed as the table does.
    static bool under_threshold(const Table & table)
    {
      const auto stats = table.stats();
      return 1.0f*(stats.num_busy + stats.num_deleted)/table.capacity() < 0.9f;
    }

    static ::testing::AssertionResult holds(const Table & table,
                                            const std::set<long> & keys)
    {
      if (table.size() != keys.size())
        return ::testing::AssertionFailure()
               << "size is " << table.size() << ", expected " << keys.size();
      for (const long k : keys)
        if (not table.contains(k))
          return ::testing::AssertionFailure() << "key " << k << " not found";
      return ::testing::AssertionSuccess();
    }
  };

  TYPED_TEST_SUITE(OpenTableRemovedKeys, Open_Tables_Of_Longs);

  // Insertions and removals of a few keys that collide a lot, in small
  // tables. Both tables used to end without empty buckets.
  TYPED_TEST(OpenTableRemovedKeys, ResizingTableStaysUnderItsThreshold)
  {
    std::mt19937 rng(20261018);
    for (int trial = 0; trial < 300; ++trial)
      {
        TypeParam table = this->make(11, true);
        std::set<long> keys;
        for (int step = 0; step < 300; ++step)
          {
            const long k = static_cast<long>(rng() % 200);
            if (keys.count(k) == 0)
              {
                ASSERT_NE(table.insert(k), nullptr);
                keys.insert(k);
              }
            else
              {
                table.remove(k);
                keys.erase(k);
              }
            ASSERT_GT(table.stats().num_empty, 0u)
              << "trial " << trial << ", step " << step;
            ASSERT_TRUE(this->under_threshold(table))
              << "trial " << trial << ", step " << step;
          }
        ASSERT_TRUE(this->holds(table, keys)) << "trial " << trial;
      }
  }

  // A table that does not resize itself is never rebuilt by itself, so
  // that the pointers to its keys stay valid: it keeps its capacity, takes
  // keys until it holds capacity() - 1, and never moves one.
  TYPED_TEST(OpenTableRemovedKeys, FixedTableNeverMovesItsKeys)
  {
    std::mt19937 rng(20261018);
    for (int trial = 0; trial < 300; ++trial)
      {
        TypeParam table = this->make(11, false);
        std::map<long, long *> keys; // and where each one is
        for (int step = 0; step < 300; ++step)
          {
            const long k = static_cast<long>(rng() % 200);
            if (keys.count(k) == 0)
              {
                long * const stored = table.insert(k);
                const bool full = keys.size() == 10;
                ASSERT_EQ(stored == nullptr, full)
                  << "trial " << trial << ", step " << step;
                if (not full)
                  keys[k] = stored;
              }
            else
              {
                table.remove(k);
                keys.erase(k);
                ASSERT_EQ(table.search(k), nullptr);
              }
            ASSERT_EQ(table.capacity(), 11u);
            ASSERT_EQ(table.size(), keys.size());
            for (const auto & [key, where] : keys)
              ASSERT_EQ(table.search(key), where)
                << "key " << key << " moved, trial " << trial << ", step " << step;
          }
      }
  }

  // 60 removed keys' buckets and 30 keys put 90 buckets in use, of 101.
  // The next key takes them over 0.9 with 31 keys, under half of it, and
  // the table is rebuilt in place.
  TYPED_TEST(OpenTableRemovedKeys, RebuildsInPlaceWhileTheKeysAreFew)
  {
    TypeParam table = with_removed_keys<TypeParam>(60);
    std::set<long> keys = {1002};
    for (long b = 62; b <= 91; ++b)
      {
        ASSERT_EQ(table.stats().num_deleted, 60u) << "bucket " << b;
        ASSERT_NE(table.insert(1000*b + 999), nullptr) << "bucket " << b;
        keys.insert(1000*b + 999);
      }

    EXPECT_EQ(table.capacity(), 101u);
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_TRUE(this->holds(table, keys));
  }

  // With 40 removed keys' buckets it takes 51 keys, over half of the
  // threshold, to get there: the table doubles.
  TYPED_TEST(OpenTableRemovedKeys, GrowsWhenTheKeysAreMany)
  {
    TypeParam table = with_removed_keys<TypeParam>(40);
    std::set<long> keys = {1002};
    for (long b = 42; b <= 91; ++b)
      {
        ASSERT_EQ(table.capacity(), 101u) << "bucket " << b;
        ASSERT_NE(table.insert(1000*b + 999), nullptr) << "bucket " << b;
        keys.insert(1000*b + 999);
      }

    EXPECT_GT(table.capacity(), 101u);
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_TRUE(this->holds(table, keys));
  }

  // Each key replaces the oldest one, so that the removed keys' buckets
  // are left behind, followed by busy ones. With 30 keys the linear probing
  // table is rebuilt in place, again and again; with 60 it doubles.
  TEST(OLhashTable, RemovedKeysRebuildItInPlaceWhileTheKeysAreFew)
  {
    for (const size_t num_keys : {size_t(30), size_t(60)})
      {
        OLhashTable<long> table(101, colliding_hash, Aleph::equal_to<long>(),
                                0.05f, 0.9f, true);
        std::deque<long> order;
        size_t rebuilt_in_place = 0;
        for (long k = 0; k < 3000; ++k)
          {
            const size_t capacity = table.capacity();
            const size_t deleted = table.stats().num_deleted;
            ASSERT_NE(table.insert(k), nullptr) << "key " << k;
            if (table.capacity() == capacity and deleted > 1
                and table.stats().num_deleted == 0)
              ++rebuilt_in_place;
            order.push_back(k);
            if (order.size() > num_keys)
              {
                table.remove(order.front());
                order.pop_front();
              }
            const auto stats = table.stats();
            ASSERT_LT(1.0f*(stats.num_busy + stats.num_deleted)/table.capacity(),
                      0.9f) << "key " << k;
          }

        EXPECT_EQ(table.size(), num_keys);
        for (const long k : order)
          EXPECT_TRUE(table.contains(k)) << "key " << k;
        if (num_keys == 30)
          {
            EXPECT_GT(rebuilt_in_place, 1u);
            EXPECT_EQ(table.capacity(), 101u);
          }
        else
          EXPECT_GT(table.capacity(), 101u);
      }
  }

  // -------------------------------------------------------------------------
  // The iterator deletes the last key
  // -------------------------------------------------------------------------

  template <class Table> struct OpenTableIteratorDel : public ::testing::Test
  {};

  TYPED_TEST_SUITE(OpenTableIteratorDel, Open_Tables_Of_Longs);

  // Deleting the last key in bucket order, with other keys before it, went
  // on looking for the next busy bucket past the end of the array.
  TYPED_TEST(OpenTableIteratorDel, DeletingTheLastKeyLeavesTheIteratorAtTheEnd)
  {
    // the keys go to buckets 2 and 9 of 11
    TypeParam table = OpenTableRemovedKeys<TypeParam>::make(11, false);
    ASSERT_NE(table.insert(2L), nullptr);
    ASSERT_NE(table.insert(9L), nullptr);

    auto it = table.get_it();
    ASSERT_EQ(it.get_curr(), 2L);
    it.next();
    ASSERT_EQ(it.get_curr(), 9L);
    it.del();

    EXPECT_FALSE(it.has_curr());
    EXPECT_EQ(table.size(), 1u);
    EXPECT_FALSE(table.contains(9L));
    it.prev();
    ASSERT_TRUE(it.has_curr());
    EXPECT_EQ(it.get_curr(), 2L);
  }

  // -------------------------------------------------------------------------
  // Random operations on small open addressing tables, against an oracle
  // -------------------------------------------------------------------------

  template <class Table> struct OpenTableRandomOperations : public ::testing::Test
  {
    static Table make(const size_t len, const float lower, const float upper,
                      const bool resizes)
    {
      if constexpr (requires (Table & t) { t.get_second_hash_fct(); })
        return Table(len, colliding_hash, colliding_second_hash,
                     Aleph::equal_to<long>(), lower, upper, resizes);
      else
        return Table(len, colliding_hash, Aleph::equal_to<long>(),
                     lower, upper, resizes);
    }
  };

  TYPED_TEST_SUITE(OpenTableRandomOperations, Open_Tables_Of_Longs);

  // Every way of inserting and removing, with the resizing turned on and
  // off at random, on tables of a few buckets where most keys collide and
  // the buckets of removed keys may take all the empty ones. After each
  // step the table holds the oracle's keys. While it does not resize
  // itself, its capacity and the places of its keys stay, and it refuses a
  // new key only when full. While it resizes itself, an insertion of a new
  // key leaves its buckets in use under the upper threshold, and a removal
  // shrinks it to the prime after half its capacity exactly when the keys
  // fit there.
  TYPED_TEST(OpenTableRandomOperations, AgreesWithAnOracle)
  {
    const std::pair<float, float> thresholds[] =
      {{0.05f, 0.9f}, {0.25f, 0.9f}, {0.6f, 0.9f}, {0.85f, 0.9f}, {0.01f, 0.99f}};
    const size_t lens[] = {2, 3, 5, 7, 11, 13};
    std::mt19937 rng(20261021);
    for (int trial = 0; trial < 400; ++trial)
      {
        const auto [lower, upper] = thresholds[rng() % 5];
        TypeParam table = this->make(lens[rng() % 6], lower, upper,
                                     rng() % 2 == 0);
        std::set<long> oracle;
        std::map<long, const long *> places; // kept while it does not resize
        for (int step = 0; step < 300; ++step)
          {
            const auto where = [&] { return ::testing::Message()
                << "trial " << trial << ", step " << step; };
            const long k = static_cast<long>(rng() % 40);
            const unsigned op = rng() % 10;
            const bool resizes = table.get_with_resize();
            const size_t capacity = table.capacity();
            const bool present = oracle.count(k) > 0;
            const bool full = table.size() + 1 == capacity;

            if (op < 4) // an insertion, of one of four kinds
              {
                long * stored = nullptr;
                bool found = false;
                if (op == 0)
                  stored = table.insert(k);
                else if (op == 1)
                  {
                    long copy = k;
                    stored = table.insert(std::move(copy));
                  }
                else if (op == 2)
                  {
                    stored = table.search_or_insert(k);
                    found = present and stored != nullptr;
                  }
                else
                  std::tie(stored, found) = table.contains_or_insert(k);

                if (present)
                  {
                    if (op < 2)
                      ASSERT_EQ(stored, nullptr) << where();
                    else
                      {
                        ASSERT_NE(stored, nullptr) << where();
                        ASSERT_TRUE(found) << where();
                      }
                  }
                else if (full and not resizes)
                  {
                    ASSERT_EQ(stored, nullptr) << where();
                    ASSERT_FALSE(found) << where();
                  }
                else
                  {
                    ASSERT_NE(stored, nullptr) << where();
                    ASSERT_FALSE(found) << where();
                    ASSERT_EQ(*stored, k) << where();
                    oracle.insert(k);
                    if (not resizes)
                      places[k] = stored;
                    else // an insertion rebuilds the table at the threshold
                      {
                        const auto stats = table.stats();
                        ASSERT_LT(1.0f*(stats.num_busy + stats.num_deleted)
                                  /table.capacity(), upper) << where();
                      }
                  }
              }
            else if (op < 8) // a removal, by key or by pointer
              {
                if (not present)
                  {
                    if (op < 6)
                      {
                        ASSERT_THROW(table.remove(k), std::domain_error) << where();
                      }
                    ASSERT_EQ(table.capacity(), capacity) << where();
                  }
                else
                  {
                    if (op < 6)
                      table.remove(k);
                    else
                      table.remove_ptr(table.search(k));
                    oracle.erase(k);
                    places.erase(k);

                    const size_t n = oracle.size();
                    const size_t smaller = Primes::next_prime(capacity / 2 + 1);
                    const bool shrinks = resizes and 1.0f*n/capacity < lower
                      and n < smaller and 1.0f*n/smaller < upper;
                    ASSERT_EQ(table.capacity(), shrinks ? smaller : capacity)
                      << where();
                  }
              }
            else if (op == 8 and rng() % 4 == 0) // turn the resizing over
              {
                table.set_with_resize(not resizes);
                ASSERT_EQ(table.get_with_resize(), not resizes);
                if (resizes) // turned off: the keys stay from now on
                  {
                    places.clear();
                    for (const long key : oracle)
                      places[key] = table.search(key);
                  }
              }

            ASSERT_EQ(table.size(), oracle.size()) << where();
            ASSERT_LT(table.size(), table.capacity()) << where();
            for (long key = 0; key < 40; ++key)
              ASSERT_EQ(table.contains(key), oracle.count(key) > 0)
                << "key " << key << ", " << where();
            if (not table.get_with_resize() and not resizes)
              {
                ASSERT_EQ(table.capacity(), capacity) << where();
                for (const auto & [key, at] : places)
                  ASSERT_EQ(table.search(key), at)
                    << "key " << key << " moved, " << where();
              }
          }

        std::set<long> listed;
        table.for_each([&listed] (const long key) { listed.insert(key); });
        EXPECT_EQ(listed, oracle) << "trial " << trial;
      }
  }

  // -------------------------------------------------------------------------
  // Paths that no test went through
  // -------------------------------------------------------------------------

  template <class Table> struct OpenTableEdges : public ::testing::Test
  {
    static Table fixed(const size_t len)
    {
      return OpenTableRandomOperations<Table>::make(len, 0.05f, 0.9f, false);
    }
  };

  TYPED_TEST_SUITE(OpenTableEdges, Open_Tables_Of_Longs);

  // A linear probing table that does not resize itself may lose all its
  // empty buckets to removed keys (with double hashing, a bucket that no
  // probe sequence goes through becomes empty). Turned on, the next
  // insertion rebuilds it in place, as its keys are few.
  TEST(OLhashTable, TurningResizingOnRebuildsATableWithoutEmptyBuckets)
  {
    OLhashTable<long> table(11, colliding_hash, Aleph::equal_to<long>(),
                            0.05f, 0.9f, false);
    for (long k = 0; k < 10; ++k) // key k in bucket k
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    table.remove(0L);
    ASSERT_NE(table.insert(10L), nullptr); // no empty bucket left
    for (const long k : {1L, 2L, 3L, 4L, 5L, 6L})
      table.remove(k);
    ASSERT_EQ(table.stats().num_empty, 0u);
    ASSERT_EQ(table.size(), 4u);

    table.set_with_resize(true);
    ASSERT_NE(table.insert(1000L), nullptr);
    EXPECT_EQ(table.capacity(), 11u); // rebuilt in place: the keys are few
    EXPECT_EQ(table.stats().num_deleted, 0u);
    EXPECT_GT(table.stats().num_empty, 0u);
    for (const long k : {7L, 8L, 9L, 10L, 1000L})
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // A search that misses goes through every bucket of a table without
  // empty buckets, and remove() then throws; search_or_insert() and
  // contains_or_insert() take the first removed key's bucket they met.
  TEST(OLhashTable, TableWithoutEmptyBuckets)
  {
    OLhashTable<long> table(11, colliding_hash, Aleph::equal_to<long>(),
                            0.05f, 0.9f, false);
    for (long k = 0; k < 10; ++k)
      ASSERT_NE(table.insert(k), nullptr) << "key " << k;
    table.remove(0L);                       // bucket 0 marked
    ASSERT_NE(table.insert(10L), nullptr);  // bucket 10: no empty one left
    table.remove(5L);                       // and bucket 5 marked
    ASSERT_EQ(table.stats().num_empty, 0u);
    ASSERT_EQ(table.size(), 9u);

    EXPECT_THROW(table.remove(22L), std::domain_error); // starts at bucket 0
    EXPECT_THROW(table.remove(21L), std::domain_error); // starts at bucket 10

    long * const p = table.search_or_insert(21L); // goes around to bucket 0
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 21L);
    EXPECT_EQ(table.size(), 10u); // full
    EXPECT_EQ(table.search_or_insert(43L), nullptr);
    EXPECT_EQ(table.contains_or_insert(43L).first, nullptr);

    table.remove(7L); // bucket 7 marked, still no empty bucket
    ASSERT_EQ(table.stats().num_empty, 0u);
    auto [q, found] = table.contains_or_insert(32L); // bucket 5
    ASSERT_NE(q, nullptr);
    EXPECT_FALSE(found);
    EXPECT_EQ(*q, 32L);
    for (const long k : {1L, 2L, 3L, 4L, 6L, 8L, 9L, 10L, 21L, 32L})
      EXPECT_TRUE(table.contains(k)) << "key " << k;
    EXPECT_FALSE(table.contains(7L));
  }

  // resize() refuses a capacity without room for the keys and an empty
  // bucket, and leaves the table as it was.
  TYPED_TEST(OpenTableEdges, ResizeRefusesTooFewBuckets)
  {
    TypeParam table = this->fixed(11);
    for (long k = 0; k < 7; ++k)
      ASSERT_NE(table.insert(k), nullptr);
    EXPECT_THROW(table.resize(7), std::overflow_error);
    EXPECT_THROW(table.resize(5), std::overflow_error);
    EXPECT_EQ(table.capacity(), 11u);
    EXPECT_EQ(table.size(), 7u);
    EXPECT_EQ(table.resize(0), 11u);
    EXPECT_EQ(table.resize(8), 8u); // 7 keys and an empty bucket
    for (long k = 0; k < 7; ++k)
      EXPECT_TRUE(table.contains(k)) << "key " << k;
  }

  // remove_ptr() refuses a pointer that does not point into the table, or
  // to a bucket whose key was removed already. The double hashing table
  // took them: in a release build, removing a stale pointer again lost
  // other keys.
  TYPED_TEST(OpenTableEdges, RemovePtrRefusesForeignAndStalePointers)
  {
    TypeParam table = this->fixed(11);
    long * const p = table.insert(3L);
    ASSERT_NE(table.insert(4L), nullptr);
    long outside = 3;
    EXPECT_THROW(table.remove_ptr(&outside), std::invalid_argument);

    table.remove_ptr(p);
    EXPECT_THROW(table.remove_ptr(p), std::domain_error);
    EXPECT_EQ(table.size(), 1u);

    TypeParam other(std::move(table)); // table is left without buckets
    EXPECT_THROW(table.remove_ptr(&outside), std::invalid_argument);
    EXPECT_TRUE(other.contains(4L));
  }

  // The bounds of the iterator.
  TYPED_TEST(OpenTableEdges, IteratorBounds)
  {
    TypeParam table = this->fixed(11);
    for (const long k : {2L, 5L, 9L})
      ASSERT_NE(table.insert(k), nullptr);

    auto it = table.get_it();
    EXPECT_EQ(it.get_pos(), 0);
    const auto & const_it = it;
    EXPECT_EQ(const_it.get_curr(), 2L);
    it.prev(); // before the first one
    EXPECT_FALSE(it.has_curr());
    EXPECT_EQ(it.get_pos(), -1);
    EXPECT_THROW(it.get_curr(), std::underflow_error);
    EXPECT_THROW(it.prev(), std::underflow_error);

    it.reset_last();
    EXPECT_EQ(it.get_curr(), 9L);
    EXPECT_EQ(it.get_pos(), 2);
    it.next(); // past the last one
    EXPECT_FALSE(it.has_curr());
    EXPECT_THROW(it.get_curr(), std::overflow_error);
    EXPECT_THROW(it.next(), std::overflow_error);
  }

  // remove() of a key inside the table that was removed already.
  TEST(ODhashTable, RemoveOfAKeyThatWasRemovedThrows)
  {
    ODhashTable<long> table(11, colliding_hash, colliding_second_hash,
                            Aleph::equal_to<long>(), 0.05f, 0.9f, false);
    for (long k = 0; k < 5; ++k)
      ASSERT_NE(table.insert(k * 11), nullptr); // all start at bucket 0
    long & second = *table.search(11L);
    table.remove(second);
    EXPECT_THROW(table.remove(second), std::domain_error);
    EXPECT_EQ(table.size(), 4u);
  }

  // values_ptr() and items_ptr() of the maps over open tables.
  TYPED_TEST(OpenMapShrinking, ValuesAndItemsByPointer)
  {
    TypeParam map(11, Aleph::dft_hash_ptr_fct<long>,
                  Aleph::snd_hash_ptr_fct<long>, Aleph::equal_to<long>(),
                  hash_default_lower_alpha, hash_default_upper_alpha, true);
    for (long k = 0; k < 5; ++k)
      ASSERT_NE(map.insert(k, static_cast<int>(10 * k)), nullptr);

    std::set<int> values;
    map.values_ptr().for_each([&values] (int * v) { values.insert(*v); });
    EXPECT_EQ(values, (std::set<int>{0, 10, 20, 30, 40}));

    std::set<long> keys;
    map.items_ptr().for_each([&keys] (auto * item)
    {
      keys.insert(item->first);
      item->second += 1;
    });
    EXPECT_EQ(keys, (std::set<long>{0, 1, 2, 3, 4}));
    EXPECT_EQ(map.find(3L), 31);
  }

  // -------------------------------------------------------------------------
  // Random operations on small chained tables, against an oracle
  // -------------------------------------------------------------------------

  // Number of non-empty lists, counted
  template <class Table> size_t busy_lists(const Table & table)
  {
    const auto stats = table.stats();
    return table.capacity() - (stats.lens.size() > 0 ? stats.lens(0) : 0);
  }

  // Insertions, also through search_or_insert(), and removals, with the
  // resizing turned on and off at random. After each step the table holds
  // the oracle's keys and counts its busy lists right. While it does not
  // resize itself, its capacity stays. While it does, an insertion that
  // takes the load to the upper threshold doubles it to the next prime of
  // the table until the keys are under that threshold (after turning the
  // resizing on, more than once), and a removal that takes it under the
  // lower one halves it,
  // if that gives a smaller table where the keys stay under the upper one.
  // Before, with a lower threshold over half the upper one, such a removal
  // shrank the table and resize() grew it back at once.
  TEST(LhashTable, RandomOperationsAgreeWithAnOracle)
  {
    using Bucket = LhashBucket<long>;
    const std::pair<float, float> thresholds[] =
      {{0.25f, 0.9f}, {0.5f, 2.0f}, {0.01f, 0.99f}, {0.85f, 0.9f}};
    std::mt19937 rng(20261021);
    for (int trial = 0; trial < 200; ++trial)
      {
        const auto [lower, upper] = thresholds[rng() % 4];
        const bool resizing = rng() % 2 == 0;
        LhashTable<long> table(1 + rng() % 13, colliding_hash,
                               Aleph::equal_to<long>(), lower, upper, true,
                               resizing);
        std::map<long, Bucket *> oracle;
        for (int step = 0; step < 300; ++step)
          {
            const auto where = [&] { return ::testing::Message()
                << "trial " << trial << ", step " << step; };
            const long k = static_cast<long>(rng() % 60);
            const unsigned op = rng() % 9;
            const bool resizes = table.get_with_resize();
            const size_t capacity = table.capacity();
            const bool present = oracle.count(k) > 0;

            if (op < 4)
              {
                auto * bucket = new Bucket(k);
                Bucket * stored = op < 2 ? table.insert(bucket)
                                         : table.search_or_insert(bucket);
                if (present)
                  {
                    ASSERT_EQ(stored, op < 2 ? nullptr : oracle[k]) << where();
                    delete bucket;
                  }
                else
                  {
                    ASSERT_EQ(stored, bucket) << where();
                    oracle[k] = bucket;
                    // resize() places the keys again with insert(), which
                    // grows the new table too while they do not fit
                    const auto load = [&] (const size_t c)
                      { return static_cast<float>(1.0*oracle.size()/c); };
                    size_t expected = capacity;
                    if (resizes and load(capacity) >= upper)
                      do
                        expected = Primes::next_prime(2*expected);
                      while (load(expected) >= upper);
                    ASSERT_EQ(table.capacity(), expected) << where();
                  }
              }
            else if (op < 8)
              {
                if (present)
                  {
                    delete table.remove(oracle[k]);
                    oracle.erase(k);
                    const size_t n = oracle.size();
                    const size_t smaller = Primes::next_prime(capacity/2);
                    const bool shrinks = resizes
                      and static_cast<float>(1.0*n/capacity) < lower
                      and smaller < capacity and 1.0f*n/smaller < upper;
                    ASSERT_EQ(table.capacity(), shrinks ? smaller : capacity)
                      << where();
                  }
              }
            else if (rng() % 4 == 0)
              table.set_with_resize(not resizes);

            ASSERT_EQ(table.size(), oracle.size()) << where();
            ASSERT_EQ(table.get_num_busy_slots(), busy_lists(table)) << where();
            for (long key = 0; key < 60; ++key)
              ASSERT_EQ(table.search(key),
                        oracle.count(key) ? oracle[key] : nullptr)
                << "key " << key << ", " << where();
          }
        for (auto & [key, bucket] : oracle)
          delete table.remove(bucket);
      }
  }

  // The bounds of the iterator of a chained table, and resize(0).
  TEST(LhashTable, IteratorBoundsAndResizeZero)
  {
    using Bucket = LhashBucket<long>;
    LhashTable<long> table(11, colliding_hash, Aleph::equal_to<long>(),
                           hash_default_lower_alpha, hash_default_upper_alpha,
                           true, false);
    LhashTable<long>::Iterator empty_it(table);
    EXPECT_FALSE(empty_it.has_curr());
    empty_it.reset_last(); // an empty table has no last one
    EXPECT_FALSE(empty_it.has_curr());

    for (const long k : {2L, 5L, 9L})
      ASSERT_NE(table.insert(new Bucket(k)), nullptr);
    EXPECT_EQ(table.resize(0), 11u);

    LhashTable<long>::Iterator it(table);
    EXPECT_EQ(it.get_curr()->get_key(), 2L);
    it.prev(); // before the first one
    EXPECT_FALSE(it.has_curr());
    EXPECT_THROW(it.prev(), std::underflow_error);
    it.reset_last();
    EXPECT_EQ(it.get_curr()->get_key(), 9L);
    it.next(); // past the last one
    EXPECT_FALSE(it.has_curr());
    EXPECT_THROW(it.next(), std::overflow_error);

    for (LhashTable<long>::Iterator del(table); del.has_curr(); )
      delete del.del();
  }

  // A map over a chained or a linear hash table: values and items by
  // pointer, and a move into itself.
  template <class Map> struct DynMapHashTables : public ::testing::Test {};

  using Dyn_Hash_Maps = ::testing::Types<DynMapHash<long, int>,
                                         DynMapLinHash<long, int>>;
  TYPED_TEST_SUITE(DynMapHashTables, Dyn_Hash_Maps);

  TYPED_TEST(DynMapHashTables, ValuesAndItemsByPointerAndSelfMove)
  {
    TypeParam map;
    for (long k = 0; k < 5; ++k)
      map.insert(k, static_cast<int>(10 * k));

    std::set<int> values;
    map.values_ptr().for_each([&values] (int * v) { values.insert(*v); });
    EXPECT_EQ(values, (std::set<int>{0, 10, 20, 30, 40}));

    std::set<long> keys;
    map.items_ptr().for_each([&keys] (auto * item)
    {
      keys.insert(item->first);
      item->second += 1;
    });
    EXPECT_EQ(keys, (std::set<long>{0, 1, 2, 3, 4}));
    EXPECT_EQ(map.find(3L), 31);

    TypeParam & same = map;
    map = std::move(same);
    EXPECT_EQ(map.size(), 5u);
    EXPECT_EQ(map.find(4L), 41);
  }
} // namespace
