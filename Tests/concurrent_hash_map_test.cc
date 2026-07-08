
/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file concurrent_hash_map_test.cc
 * @brief Tests for Aleph::ConcurrentHashMap (tpl_concurrent_hash_map.H).
 */

#include <gtest/gtest.h>

#include "concurrency_test_utils.H"

#include <tpl_concurrent_hash_map.H>

#include <atomic>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace Aleph;
using namespace Aleph::Testing;

namespace
{
  struct Throwing_Copy
  {
    static bool should_throw;
    int value;

    explicit Throwing_Copy(int v) : value(v) {}
    Throwing_Copy(const Throwing_Copy &other) : value(other.value)
    {
      if (should_throw)
        throw std::runtime_error("Throwing_Copy: copy failed");
    }
    Throwing_Copy(Throwing_Copy &&) = default;
    Throwing_Copy &operator=(const Throwing_Copy &) = default;
    Throwing_Copy &operator=(Throwing_Copy &&) = default;
  };

  bool Throwing_Copy::should_throw = false;
}  // namespace

// `run_workers(thread_count, worker)` (launch N independently-indexed
// threads with a deterministic simultaneous start, exception-safe join,
// and first-exception rethrow) is provided by concurrency_test_utils.H
// itself -- Aleph::Testing::run_workers -- and used unqualified here via
// the `using namespace Aleph::Testing;` above.

TEST(ConcurrentHashMap, DefaultConstructedMapIsEmpty)
{
  ConcurrentHashMap<std::string, int> m;
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0u);
}

TEST(ConcurrentHashMap, InsertRejectsDuplicateKeys)
{
  ConcurrentHashMap<std::string, int> m;
  EXPECT_TRUE(m.insert("alpha", 1));
  EXPECT_FALSE(m.insert("alpha", 99));
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(*m.find_copy("alpha"), 1);  // unchanged by the rejected insert
}

TEST(ConcurrentHashMap, InsertMoveOverloadIsSupported)
{
  ConcurrentHashMap<std::string, std::unique_ptr<int>> m;
  EXPECT_TRUE(m.insert("alpha", std::make_unique<int>(10)));
  bool saw = m.with_value("alpha", [](const std::unique_ptr<int> &p)
  {
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 10);
  });
  EXPECT_TRUE(saw);
}

TEST(ConcurrentHashMap, ContainsAndFindCopyReflectState)
{
  ConcurrentHashMap<std::string, int> m;
  EXPECT_FALSE(m.contains("alpha"));
  EXPECT_FALSE(m.find_copy("alpha").has_value());

  m.insert("alpha", 42);
  EXPECT_TRUE(m.contains("alpha"));
  auto v = m.find_copy("alpha");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(*v, 42);
}

TEST(ConcurrentHashMap, InsertOrAssignInsertsThenOverwrites)
{
  ConcurrentHashMap<std::string, int> m;
  m.insert_or_assign("alpha", 1);
  EXPECT_EQ(*m.find_copy("alpha"), 1);
  m.insert_or_assign("alpha", 2);
  EXPECT_EQ(*m.find_copy("alpha"), 2);
  EXPECT_EQ(m.size(), 1u);
}

TEST(ConcurrentHashMap, EraseReturnsFalseForMissingKey)
{
  ConcurrentHashMap<std::string, int> m;
  EXPECT_FALSE(m.erase("nope"));
  m.insert("alpha", 1);
  EXPECT_TRUE(m.erase("alpha"));
  EXPECT_FALSE(m.erase("alpha"));
  EXPECT_TRUE(m.is_empty());
}

TEST(ConcurrentHashMap, WithValueSeesCurrentValueAndSkipsMissingKeys)
{
  ConcurrentHashMap<std::string, int> m;
  m.insert("alpha", 7);

  int seen = -1;
  EXPECT_TRUE(m.with_value("alpha", [&](const int &v) { seen = v; }));
  EXPECT_EQ(seen, 7);

  bool called = false;
  EXPECT_FALSE(m.with_value("missing", [&](const int &) { called = true; }));
  EXPECT_FALSE(called);
}

TEST(ConcurrentHashMap, WithValueMutMutatesInPlace)
{
  ConcurrentHashMap<std::string, int> m;
  m.insert("alpha", 10);
  EXPECT_TRUE(m.with_value_mut("alpha", [](int &v) { v += 5; }));
  EXPECT_EQ(*m.find_copy("alpha"), 15);

  EXPECT_FALSE(m.with_value_mut("missing", [](int &v) { v = 999; }));
}

TEST(ConcurrentHashMap, ClearRemovesEveryEntry)
{
  ConcurrentHashMap<std::string, int> m;
  for (int i = 0; i < 100; ++i)
    m.insert("key" + std::to_string(i), i);
  EXPECT_EQ(m.size(), 100u);

  m.clear();
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0u);
}

TEST(ConcurrentHashMap, SnapshotReturnsIndependentCopy)
{
  ConcurrentHashMap<std::string, int> m;
  m.insert("alpha", 1);
  m.insert("beta", 2);

  auto snap = m.snapshot();
  ASSERT_EQ(snap.size(), 2u);

  // Mutating the map afterward must not affect the already-taken snapshot.
  m.insert_or_assign("alpha", 999);
  m.erase("beta");

  std::map<std::string, int> as_map;
  for (const auto &entry : snap)
    as_map.emplace(entry.first, entry.second);
  ASSERT_EQ(as_map.size(), 2u);
  EXPECT_EQ(as_map.at("alpha"), 1);
  EXPECT_EQ(as_map.at("beta"), 2);
}

TEST(ConcurrentHashMap, FailedInsertCopyLeavesMapUnchanged)
{
  ConcurrentHashMap<int, Throwing_Copy, Aleph::equal_to<int>, 4> m;
  Throwing_Copy value(1);
  Throwing_Copy::should_throw = false;
  EXPECT_TRUE(m.insert(1, value));

  Throwing_Copy::should_throw = true;
  Throwing_Copy other(2);
  EXPECT_THROW(m.insert(2, other), std::runtime_error);
  Throwing_Copy::should_throw = false;

  EXPECT_EQ(m.size(), 1u);
  EXPECT_TRUE(m.contains(1));
  EXPECT_FALSE(m.contains(2));
}

TEST(ConcurrentHashMap, DifferentShardsAllowConcurrentWriters)
{
  // Two keys deliberately chosen to land in different shards (with 16
  // shards and Aleph's default hash, consecutive small integers spread
  // across shards); the point of this test is exercising real concurrent
  // writers below under TSan, not the exact shard placement.
  constexpr size_t shards = 16;
  ConcurrentHashMap<int, int, Aleph::equal_to<int>, shards> m;

  run_workers(shards, [&](size_t idx)
  {
    for (int i = 0; i < 200; ++i)
      {
        const int key = static_cast<int>(idx) * 1000 + i;
        m.insert(key, i);
      }
  });

  EXPECT_EQ(m.size(), shards * 200);
}

TEST(ConcurrentHashMap, ConcurrentInsertFindMutateEraseIsRaceFree)
{
  constexpr int thread_count = 8;
  constexpr int keys_per_thread = 2000;
  ConcurrentHashMap<int, int, Aleph::equal_to<int>, 16> m;

  run_workers(thread_count, [&](size_t t)
  {
    const int base = static_cast<int>(t) * keys_per_thread;

    for (int i = 0; i < keys_per_thread; ++i)
      {
        const int key = base + i;
        EXPECT_TRUE(m.insert(key, i));
        EXPECT_TRUE(m.contains(key));
        auto v = m.find_copy(key);
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
      }

    for (int i = 0; i < keys_per_thread; ++i)
      {
        const int key = base + i;
        EXPECT_TRUE(m.with_value_mut(key, [](int &v) { v *= 2; }));
      }

    for (int i = 0; i < keys_per_thread; i += 2)  // erase every other key
      EXPECT_TRUE(m.erase(base + i));
  });

  const size_t expected = static_cast<size_t>(thread_count) * (keys_per_thread / 2);
  EXPECT_EQ(m.size(), expected);

  for (int t = 0; t < thread_count; ++t)
    for (int i = 0; i < keys_per_thread; ++i)
      {
        const int key = t * keys_per_thread + i;
        auto v = m.find_copy(key);
        if (i % 2 == 0)
          EXPECT_FALSE(v.has_value());
        else
          {
            ASSERT_TRUE(v.has_value());
            EXPECT_EQ(*v, i * 2);
          }
      }
}

TEST(ConcurrentHashMap, RandomizedOperationTraceMatchesSequentialReferenceModel)
{
  const auto trace = make_random_operation_trace(
    4000, 0xBADC0FFEu, 200,
    {
      Trace_Operation_Kind::insert,
      Trace_Operation_Kind::erase,
      Trace_Operation_Kind::contains
    });

  std::map<size_t, size_t> reference;
  ConcurrentHashMap<size_t, size_t> subject;

  auto apply_reference = [](std::map<size_t, size_t> &ref, const Trace_Operation &op) -> bool
  {
    switch (op.kind)
      {
      case Trace_Operation_Kind::insert:
        return ref.emplace(op.key, op.value).second;
      case Trace_Operation_Kind::erase:
        return ref.erase(op.key) != 0;
      case Trace_Operation_Kind::contains:
        return ref.find(op.key) != ref.end();
      default:
        return false;
      }
  };

  auto apply_subject = [](ConcurrentHashMap<size_t, size_t> &s, const Trace_Operation &op) -> bool
  {
    switch (op.kind)
      {
      case Trace_Operation_Kind::insert:
        return s.insert(op.key, op.value);
      case Trace_Operation_Kind::erase:
        return s.erase(op.key);
      case Trace_Operation_Kind::contains:
        return s.contains(op.key);
      default:
        return false;
      }
  };

  for (size_t i = 0; i < trace.size(); ++i)
    {
      const bool ref_result = apply_reference(reference, trace[i]);
      const bool subj_result = apply_subject(subject, trace[i]);
      ASSERT_EQ(ref_result, subj_result) << "mismatch at operation " << i;
    }

  ASSERT_EQ(subject.size(), reference.size());
  for (const auto &[key, value] : reference)
    {
      auto v = subject.find_copy(key);
      ASSERT_TRUE(v.has_value()) << "missing key " << key;
      EXPECT_EQ(*v, value);
    }
}