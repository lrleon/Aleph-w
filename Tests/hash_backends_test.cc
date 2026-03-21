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

/** @file hash_backends_test.cc
 *  @brief Regression tests for HashSet/HashMap default backends.
 *
 *  These tests pin the default hash backend for HashSet and HashMap
 *  (OLhashTable / MapOLhash, changed from OD in commit 323aeb7b) and verify
 *  that both OL and OD backends are functionally equivalent. If the default is
 *  intentionally changed in the future the static_asserts below will catch it
 *  at compile time, forcing an explicit acknowledgment of the breaking change.
 */

# include <string>
# include <type_traits>
# include <unordered_set>

# include <gtest/gtest.h>

# include <tpl_hash.H>

using namespace Aleph;

namespace
{

// ============================================================================
// Compile-time default checks
// ============================================================================

// HashSet<Key> must default to OLhashTable (linear probing).
// This was changed from ODhashTable in commit 323aeb7b for consistency with
// DynHashSet and better cache locality. If you intentionally change this
// default again, update this assert and document it as a breaking change.
static_assert(std::is_same_v<HashSet<int>, HashSet<int, OLhashTable>>,
              "HashSet<Key> default backend must be OLhashTable. "
              "If this changed intentionally, document it as a breaking change.");

// HashMap<Key,Data> must default to MapOLhash.
static_assert(std::is_same_v<HashMap<int, int>, HashMap<int, int, MapOLhash>>,
              "HashMap<Key,Data> default backend must be MapOLhash. "
              "If this changed intentionally, document it as a breaking change.");

// DynHashSet<Key> must default to OLhashTable (unchanged historical default).
static_assert(std::is_same_v<DynHashSet<int>, DynHashSet<int, OLhashTable>>,
              "DynHashSet<Key> default backend must be OLhashTable.");

// DynHashMap<Key,Data> must default to MapOLhash (unchanged historical default).
static_assert(std::is_same_v<DynHashMap<int, int>,
                              DynHashMap<int, int, MapOLhash>>,
              "DynHashMap<Key,Data> default backend must be MapOLhash.");

// ============================================================================
// Helpers
// ============================================================================

constexpr int N = 200;

// Collect all keys in a HashSet into a std::unordered_set for comparison.
template <typename Set>
std::unordered_set<int> collect(Set &s)
{
  std::unordered_set<int> result;
  s.for_each([&](const int k) { result.insert(k); });
  return result;
}

template <typename Map>
std::unordered_set<int> collect_keys(Map &m)
{
  std::unordered_set<int> result;
  m.for_each([&](const std::pair<int, std::string> &p) { result.insert(p.first); });
  return result;
}

// ============================================================================
// HashSet — OL backend (default)
// ============================================================================

struct HashSetOLTest : ::testing::Test
{
  HashSet<int, OLhashTable> s;
};

TEST_F(HashSetOLTest, InsertAndContains)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(s.contains(i)) << "key " << i << " missing after insert";
}

TEST_F(HashSetOLTest, ContainsReturnsFalseForAbsent)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  EXPECT_FALSE(s.contains(N));
  EXPECT_FALSE(s.contains(-1));
}

TEST_F(HashSetOLTest, IterationCoversAllKeys)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  auto found = collect(s);
  EXPECT_EQ(found.size(), (size_t) N);
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(found.count(i)) << "key " << i << " missing from iteration";
}

TEST_F(HashSetOLTest, RemoveKey)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  s.remove(42);
  EXPECT_FALSE(s.contains(42));

  // All other keys still present.
  for (int i = 0; i < N; ++i)
    {
      if (i != 42)
        EXPECT_TRUE(s.contains(i)) << "key " << i << " missing after remove(42)";
    }
}

TEST_F(HashSetOLTest, DuplicateInsertIsNoOp)
{
  s.insert(7);
  s.insert(7);
  size_t count = 0;
  s.for_each([&](const int k) { if (k == 7) ++count; });
  EXPECT_EQ(count, 1u);
}

// ============================================================================
// HashSet — OD backend (explicit)
// ============================================================================

struct HashSetODTest : ::testing::Test
{
  HashSet<int, ODhashTable> s;
};

TEST_F(HashSetODTest, InsertAndContains)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(s.contains(i)) << "key " << i << " missing after insert";
}

TEST_F(HashSetODTest, ContainsReturnsFalseForAbsent)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  EXPECT_FALSE(s.contains(N));
  EXPECT_FALSE(s.contains(-1));
}

TEST_F(HashSetODTest, IterationCoversAllKeys)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  auto found = collect(s);
  EXPECT_EQ(found.size(), (size_t) N);
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(found.count(i)) << "key " << i << " missing from iteration";
}

TEST_F(HashSetODTest, RemoveKey)
{
  for (int i = 0; i < N; ++i)
    s.insert(i);

  s.remove(42);
  EXPECT_FALSE(s.contains(42));

  for (int i = 0; i < N; ++i)
    {
      if (i != 42)
        EXPECT_TRUE(s.contains(i)) << "key " << i << " missing after remove(42)";
    }
}

// ============================================================================
// HashSet — both backends produce the same key set
// ============================================================================

TEST(HashSetBackendsEquivalent, SameKeysAfterInsert)
{
  HashSet<int, OLhashTable> ol;
  HashSet<int, ODhashTable> od;

  for (int i = 0; i < N; ++i)
    {
      ol.insert(i);
      od.insert(i);
    }

  EXPECT_EQ(collect(ol), collect(od));
}

// ============================================================================
// HashMap — OL backend (default)
// ============================================================================

struct HashMapOLTest : ::testing::Test
{
  HashMap<int, std::string, MapOLhash> m;
};

TEST_F(HashMapOLTest, InsertAndSearch)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  for (int i = 0; i < N; ++i)
    {
      auto *p = m.search(i);
      ASSERT_NE(p, nullptr) << "key " << i << " not found";
      EXPECT_EQ(p->second, std::to_string(i));
    }
}

TEST_F(HashMapOLTest, SearchMissingKeyReturnsNull)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  EXPECT_EQ(m.search(N), nullptr);
  EXPECT_EQ(m.search(-1), nullptr);
}

TEST_F(HashMapOLTest, IterationCoversAllKeys)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  auto keys = collect_keys(m);
  EXPECT_EQ(keys.size(), (size_t) N);
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(keys.count(i)) << "key " << i << " missing from iteration";
}

// ============================================================================
// HashMap — OD backend (explicit)
// ============================================================================

struct HashMapODTest : ::testing::Test
{
  HashMap<int, std::string, MapODhash> m;
};

TEST_F(HashMapODTest, InsertAndSearch)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  for (int i = 0; i < N; ++i)
    {
      auto *p = m.search(i);
      ASSERT_NE(p, nullptr) << "key " << i << " not found";
      EXPECT_EQ(p->second, std::to_string(i));
    }
}

TEST_F(HashMapODTest, SearchMissingKeyReturnsNull)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  EXPECT_EQ(m.search(N), nullptr);
  EXPECT_EQ(m.search(-1), nullptr);
}

TEST_F(HashMapODTest, IterationCoversAllKeys)
{
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  auto keys = collect_keys(m);
  EXPECT_EQ(keys.size(), (size_t) N);
  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(keys.count(i)) << "key " << i << " missing from iteration";
}

// ============================================================================
// HashMap — both backends produce the same key/value set
// ============================================================================

TEST(HashMapBackendsEquivalent, SameEntriesAfterInsert)
{
  HashMap<int, std::string, MapOLhash> ol;
  HashMap<int, std::string, MapODhash> od;

  for (int i = 0; i < N; ++i)
    {
      ol.insert(i, std::to_string(i));
      od.insert(i, std::to_string(i));
    }

  EXPECT_EQ(collect_keys(ol), collect_keys(od));

  for (int i = 0; i < N; ++i)
    {
      auto *pol = ol.search(i);
      auto *pod = od.search(i);
      ASSERT_NE(pol, nullptr);
      ASSERT_NE(pod, nullptr);
      EXPECT_EQ(pol->second, pod->second);
    }
}

// ============================================================================
// DynHashSet / DynHashMap (aliases) — smoke test with default backends
// ============================================================================

TEST(DynHashSetTest, DefaultBackendWorks)
{
  DynHashSet<int> s;
  for (int i = 0; i < N; ++i)
    s.insert(i);

  auto found = collect(s);
  EXPECT_EQ(found.size(), (size_t) N);
}

TEST(DynHashMapTest, DefaultBackendWorks)
{
  DynHashMap<int, std::string> m;
  for (int i = 0; i < N; ++i)
    m.insert(i, std::to_string(i));

  auto keys = collect_keys(m);
  EXPECT_EQ(keys.size(), (size_t) N);
}

}  // namespace