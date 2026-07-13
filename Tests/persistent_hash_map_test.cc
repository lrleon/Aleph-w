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
 * @file persistent_hash_map_test.cc
 * @brief Tests for Aleph::PersistentHashMap.
 */

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <random>

#include <tpl_persistent_hash_map.H>

using namespace Aleph;

namespace
{

size_t bad_hash(const std::string &)
{
  return 0;
}

char lower_ascii(const char c)
{
  return c >= 'A' and c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

size_t case_hash(const std::string &s)
{
  size_t h = 1469598103934665603ULL;
  for (const char c : s)
    {
      h ^= static_cast<unsigned char>(lower_ascii(c));
      h *= 1099511628211ULL;
    }
  return h;
}

struct CaseEqual
{
  bool operator () (const std::string &a, const std::string &b) const noexcept
  {
    if (a.size() != b.size())
      return false;
    for (size_t i = 0; i < a.size(); ++i)
      if (lower_ascii(a[i]) != lower_ascii(b[i]))
        return false;
    return true;
  }
};

struct MoveTracked
{
  int value = 0;
  bool moved_from = false;

  explicit MoveTracked(const int v = 0) noexcept
    : value(v)
  {
    // Empty.
  }

  MoveTracked(const MoveTracked &rhs) noexcept
    : value(rhs.value)
  {
    // Empty.
  }

  MoveTracked(MoveTracked &&rhs) noexcept
    : value(rhs.value)
  {
    rhs.moved_from = true;
  }

  MoveTracked &operator = (const MoveTracked &rhs) noexcept
  {
    value = rhs.value;
    moved_from = false;
    return *this;
  }

  MoveTracked &operator = (MoveTracked &&rhs) noexcept
  {
    value = rhs.value;
    moved_from = false;
    rhs.moved_from = true;
    return *this;
  }
};

} // namespace

TEST(PersistentHashMap, Empty)
{
  PersistentHashMap<std::string, int> map;
  EXPECT_TRUE(map.is_empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.keys().size(), 0);
  EXPECT_EQ(map.items().size(), 0);
  EXPECT_TRUE(map.verify());
}

TEST(PersistentHashMap, InsertBasic)
{
  PersistentHashMap<std::string, int> m1;
  auto m2 = m1.insert("foo", 42);

  EXPECT_EQ(m1.size(), 0);
  EXPECT_FALSE(m1.contains("foo"));

  EXPECT_EQ(m2.size(), 1);
  EXPECT_TRUE(m2.contains("foo"));
  ASSERT_NE(m2.find("foo"), nullptr);
  EXPECT_EQ(*m2.find("foo"), 42);

  auto m3 = m2.insert("bar", 99);
  EXPECT_EQ(m2.size(), 1);
  EXPECT_EQ(m3.size(), 2);
  EXPECT_TRUE(m3.contains("foo"));
  EXPECT_TRUE(m3.contains("bar"));
  ASSERT_NE(m3.find("bar"), nullptr);
  EXPECT_EQ(*m3.find("bar"), 99);

  EXPECT_TRUE(m3.verify());
}

TEST(PersistentHashMap, InsertDoesNotOverwriteAndAssignDoes)
{
  PersistentHashMap<std::string, int> m1;
  auto m2 = m1.insert("foo", 42);
  auto duplicate = m2.insert("foo", 99);

  EXPECT_EQ(duplicate.size(), 1);
  ASSERT_NE(duplicate.find("foo"), nullptr);
  EXPECT_EQ(*duplicate.find("foo"), 42);

  auto updated = m2.insert_or_assign("foo", 99);
  EXPECT_EQ(updated.size(), 1);
  ASSERT_NE(updated.find("foo"), nullptr);
  EXPECT_EQ(*updated.find("foo"), 99);

  ASSERT_NE(m2.find("foo"), nullptr);
  EXPECT_EQ(*m2.find("foo"), 42);
  EXPECT_TRUE(updated.verify());
}

TEST(PersistentHashMap, DuplicateInsertDoesNotMoveRvalue)
{
  PersistentHashMap<std::string, MoveTracked> m1;
  auto m2 = m1.insert("foo", MoveTracked{42});

  MoveTracked replacement{99};
  auto duplicate = m2.insert("foo", std::move(replacement));

  EXPECT_FALSE(replacement.moved_from);
  EXPECT_EQ(duplicate.size(), 1);
  ASSERT_NE(duplicate.find("foo"), nullptr);
  EXPECT_EQ(duplicate.find("foo")->value, 42);
}

TEST(PersistentHashMap, KeysAndItemsExposeEveryBinding)
{
  PersistentHashMap<std::string, int> map;
  map = map.insert("a", 1).insert("b", 2).insert("c", 3);

  auto keys = map.keys();
  auto items = map.items();
  EXPECT_EQ(keys.size(), 3);
  EXPECT_EQ(items.size(), 3);

  std::unordered_map<std::string, int> seen;
  for (size_t i = 0; i < items.size(); ++i)
    seen.emplace(items[i].first, items[i].second);

  EXPECT_EQ(seen.size(), 3);
  EXPECT_EQ(seen["a"], 1);
  EXPECT_EQ(seen["b"], 2);
  EXPECT_EQ(seen["c"], 3);
}

TEST(PersistentHashMap, EraseBasic)
{
  PersistentHashMap<std::string, int> m1;
  auto m2 = m1.insert("a", 1).insert("b", 2).insert("c", 3);

  EXPECT_EQ(m2.size(), 3);
  auto missing = m2.erase("missing");
  EXPECT_EQ(missing.size(), 3);
  EXPECT_TRUE(missing.verify());

  auto m3 = m2.erase("b");

  EXPECT_EQ(m2.size(), 3);
  EXPECT_TRUE(m2.contains("b"));

  EXPECT_EQ(m3.size(), 2);
  EXPECT_FALSE(m3.contains("b"));
  EXPECT_TRUE(m3.contains("a"));
  EXPECT_TRUE(m3.contains("c"));

  auto m4 = m3.erase("a").erase("c");
  EXPECT_TRUE(m4.is_empty());
  EXPECT_TRUE(m4.verify());
}

TEST(PersistentHashMap, AdversarialCollisions)
{
  PersistentHashMap<std::string, int> map(bad_hash);
  std::vector<PersistentHashMap<std::string, int>> versions;
  versions.push_back(map);

  for (int i = 0; i < 100; ++i)
    {
      map = map.insert(std::to_string(i), i * 10);
      versions.push_back(map);
      EXPECT_TRUE(map.verify());
    }

  EXPECT_EQ(map.size(), 100);
  for (int i = 0; i < 100; ++i)
    {
      ASSERT_NE(map.find(std::to_string(i)), nullptr);
      EXPECT_EQ(*map.find(std::to_string(i)), i * 10);
    }

  auto keys = map.keys();
  auto items = map.items();
  EXPECT_EQ(keys.size(), 100u);
  EXPECT_EQ(items.size(), 100u);

  std::unordered_map<std::string, bool> exported_keys;
  for (size_t i = 0; i < keys.size(); ++i)
    exported_keys.emplace(keys[i], true);

  std::unordered_map<std::string, int> exported_items;
  for (size_t i = 0; i < items.size(); ++i)
    exported_items.emplace(items[i].first, items[i].second);

  EXPECT_EQ(exported_keys.size(), 100u);
  EXPECT_EQ(exported_items.size(), 100u);
  for (int i = 0; i < 100; ++i)
    {
      const std::string key = std::to_string(i);
      EXPECT_NE(exported_keys.find(key), exported_keys.end());
      auto item = exported_items.find(key);
      ASSERT_NE(item, exported_items.end());
      EXPECT_EQ(item->second, i * 10);
    }

  for (int i = 0; i <= 100; ++i)
    EXPECT_EQ(versions[i].size(), i);

  for (int i = 0; i < 100; ++i)
    {
      map = map.erase(std::to_string(i));
      EXPECT_TRUE(map.verify());
    }
  EXPECT_TRUE(map.is_empty());
}

TEST(PersistentHashMap, CollisionInsertDoesNotOverwriteAndAssignDoes)
{
  PersistentHashMap<std::string, int> map(bad_hash);
  auto base = map.insert("a", 1).insert("b", 2);

  auto duplicate = base.insert("a", 99);
  EXPECT_EQ(duplicate.size(), 2);
  ASSERT_NE(duplicate.find("a"), nullptr);
  EXPECT_EQ(*duplicate.find("a"), 1);

  auto updated = base.insert_or_assign("a", 99);
  EXPECT_EQ(updated.size(), 2);
  ASSERT_NE(updated.find("a"), nullptr);
  ASSERT_NE(updated.find("b"), nullptr);
  EXPECT_EQ(*updated.find("a"), 99);
  EXPECT_EQ(*updated.find("b"), 2);
  EXPECT_TRUE(updated.verify());

  ASSERT_NE(base.find("a"), nullptr);
  EXPECT_EQ(*base.find("a"), 1);
}

TEST(PersistentHashMap, CustomComparatorAndCompatibleHash)
{
  PersistentHashMap<std::string, int, CaseEqual> map(case_hash, CaseEqual{});
  auto m1 = map.insert("Aleph", 1);
  auto duplicate = m1.insert("aleph", 2);
  auto updated = m1.insert_or_assign("aleph", 2);

  EXPECT_EQ(duplicate.size(), 1);
  ASSERT_NE(duplicate.find("ALEPH"), nullptr);
  EXPECT_EQ(*duplicate.find("ALEPH"), 1);

  EXPECT_EQ(updated.size(), 1);
  ASSERT_NE(updated.find("ALEPH"), nullptr);
  EXPECT_EQ(*updated.find("ALEPH"), 2);
  EXPECT_TRUE(updated.verify());
}

TEST(PersistentHashMap, RandomizedParity)
{
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist_op(0, 2);
  std::uniform_int_distribution<int> dist_key(0, 500);

  PersistentHashMap<int, std::string> pmap;
  std::unordered_map<int, std::string> stdmap;
  std::vector<PersistentHashMap<int, std::string>> versions;

  versions.push_back(pmap);

  for (int i = 0; i < 5000; ++i)
    {
      const int op = dist_op(rng);
      const int key = dist_key(rng);

      if (op == 0)
        {
          const std::string val = "insert" + std::to_string(key) + "_" + std::to_string(i);
          pmap = pmap.insert(key, val);
          stdmap.emplace(key, val);
        }
      else if (op == 1)
        {
          const std::string val = "assign" + std::to_string(key) + "_" + std::to_string(i);
          pmap = pmap.insert_or_assign(key, val);
          stdmap[key] = val;
        }
      else
        {
          pmap = pmap.erase(key);
          stdmap.erase(key);
        }

      EXPECT_EQ(pmap.size(), stdmap.size());
      EXPECT_TRUE(pmap.verify());

      if (i % 100 == 0)
        versions.push_back(pmap);
    }

  for (const auto &[k, v] : stdmap)
    {
      EXPECT_TRUE(pmap.contains(k));
      ASSERT_NE(pmap.find(k), nullptr);
      EXPECT_EQ(*pmap.find(k), v);
    }

  for (const auto &v_map : versions)
    EXPECT_TRUE(v_map.verify());
}
