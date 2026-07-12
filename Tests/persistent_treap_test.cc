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
 * @file persistent_treap_test.cc
 * @brief Tests for Aleph::PersistentTreapSet and Aleph::PersistentTreapMap.
 */

#include <gtest/gtest.h>

#include <tpl_persistent_treap.H>

#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace Aleph;

namespace
{
  template <class ArrayType>
  std::vector<typename ArrayType::Item_Type> to_vector(const ArrayType &array)
  {
    std::vector<typename ArrayType::Item_Type> out;
    out.reserve(array.size());
    for (size_t i = 0; i < array.size(); ++i)
      out.push_back(array[i]);
    return out;
  }
}

TEST(PersistentTreapSet, InsertEraseAndVersionsAreIndependent)
{
  PersistentTreapSet<int> empty;
  auto one = empty.insert(10);
  auto two = one.insert(5).insert(20);
  auto erased = two.erase(10);

  EXPECT_TRUE(empty.is_empty());
  EXPECT_FALSE(empty.contains(10));

  EXPECT_EQ(one.size(), 1u);
  EXPECT_TRUE(one.contains(10));
  EXPECT_FALSE(one.contains(5));

  EXPECT_EQ(two.size(), 3u);
  EXPECT_TRUE(two.contains(5));
  EXPECT_TRUE(two.contains(10));
  EXPECT_TRUE(two.contains(20));

  EXPECT_EQ(erased.size(), 2u);
  EXPECT_FALSE(erased.contains(10));
  EXPECT_TRUE(two.contains(10));  // old version unchanged

  EXPECT_TRUE(empty.verify());
  EXPECT_TRUE(one.verify());
  EXPECT_TRUE(two.verify());
  EXPECT_TRUE(erased.verify());
}

TEST(PersistentTreapSet, DuplicateInsertAndMissingEraseShareLogicalVersion)
{
  PersistentTreapSet<int> set;
  auto one = set.insert(7);
  auto duplicate = one.insert(7);
  auto missing = duplicate.erase(99);

  EXPECT_EQ(one.size(), 1u);
  EXPECT_EQ(duplicate.size(), 1u);
  EXPECT_EQ(missing.size(), 1u);
  EXPECT_TRUE(missing.contains(7));
  EXPECT_TRUE(missing.verify());
}

TEST(PersistentTreapSet, KeysAreSorted)
{
  PersistentTreapSet<int> set;
  for (int x : {4, 1, 7, 3, 9, 2})
    set = set.insert(x);

  EXPECT_EQ(to_vector(set.keys()), (std::vector<int>{1, 2, 3, 4, 7, 9}));
  EXPECT_TRUE(set.verify());
}

TEST(PersistentTreapSet, SplitAndJoin)
{
  PersistentTreapSet<int> set;
  for (int i = 0; i < 10; ++i)
    set = set.insert(i);

  auto [left, right] = set.split(5);
  EXPECT_EQ(to_vector(left.keys()), (std::vector<int>{0, 1, 2, 3, 4}));
  EXPECT_EQ(to_vector(right.keys()), (std::vector<int>{5, 6, 7, 8, 9}));

  auto joined = left.join(right);
  EXPECT_EQ(to_vector(joined.keys()), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
  EXPECT_TRUE(left.verify());
  EXPECT_TRUE(right.verify());
  EXPECT_TRUE(joined.verify());
}

TEST(PersistentTreapSet, JoinRejectsOverlappingRanges)
{
  auto left = PersistentTreapSet<int>().insert(1).insert(3);
  auto right = PersistentTreapSet<int>().insert(3).insert(5);

  EXPECT_THROW((void)left.join(right), std::domain_error);
}

TEST(PersistentTreapSet, RandomizedTraceMatchesStdSetAndKeepsOldVersions)
{
  std::mt19937 rng(0x5EEDu);
  std::uniform_int_distribution<int> key_dist(0, 80);
  std::uniform_int_distribution<int> op_dist(0, 1);

  PersistentTreapSet<int> subject;
  std::set<int> reference;
  std::vector<PersistentTreapSet<int>> old_subjects;
  std::vector<std::set<int>> old_references;

  for (int iter = 0; iter < 1200; ++iter)
    {
      if (iter % 37 == 0)
        {
          old_subjects.push_back(subject);
          old_references.push_back(reference);
        }

      const int key = key_dist(rng);
      if (op_dist(rng) == 0)
        {
          subject = subject.insert(key);
          reference.insert(key);
        }
      else
        {
          subject = subject.erase(key);
          reference.erase(key);
        }

      ASSERT_TRUE(subject.verify());
      EXPECT_EQ(subject.size(), reference.size());
      EXPECT_EQ(to_vector(subject.keys()),
                std::vector<int>(reference.begin(), reference.end()));
    }

  for (size_t i = 0; i < old_subjects.size(); ++i)
    EXPECT_EQ(to_vector(old_subjects[i].keys()),
              std::vector<int>(old_references[i].begin(), old_references[i].end()));
}

TEST(PersistentTreapMap, InsertAssignEraseAndVersionsAreIndependent)
{
  PersistentTreapMap<int, std::string> empty;
  auto one = empty.insert(1, std::string("one"));
  auto two = one.insert(2, std::string("two"));
  auto reassigned = two.insert_or_assign(1, std::string("uno"));
  auto erased = reassigned.erase(2);

  ASSERT_NE(one.find(1), nullptr);
  EXPECT_EQ(*one.find(1), "one");
  EXPECT_EQ(one.find(2), nullptr);

  ASSERT_NE(two.find(2), nullptr);
  EXPECT_EQ(*two.find(2), "two");
  EXPECT_EQ(*two.find(1), "one");

  EXPECT_EQ(*reassigned.find(1), "uno");
  EXPECT_EQ(*two.find(1), "one");  // old version unchanged

  EXPECT_FALSE(erased.contains(2));
  EXPECT_TRUE(reassigned.contains(2));
  EXPECT_TRUE(empty.verify());
  EXPECT_TRUE(one.verify());
  EXPECT_TRUE(two.verify());
  EXPECT_TRUE(reassigned.verify());
  EXPECT_TRUE(erased.verify());
}

TEST(PersistentTreapMap, DuplicateInsertDoesNotOverwrite)
{
  auto map = PersistentTreapMap<int, std::string>()
               .insert(4, std::string("four"))
               .insert(4, std::string("cuatro"));

  ASSERT_NE(map.find(4), nullptr);
  EXPECT_EQ(*map.find(4), "four");
  EXPECT_EQ(map.size(), 1u);
  EXPECT_TRUE(map.verify());
}

TEST(PersistentTreapMap, SplitJoinAndItems)
{
  PersistentTreapMap<int, std::string> map;
  for (int i = 0; i < 6; ++i)
    map = map.insert(i, std::to_string(i));

  auto [left, right] = map.split(3);
  EXPECT_EQ(to_vector(left.keys()), (std::vector<int>{0, 1, 2}));
  EXPECT_EQ(to_vector(right.keys()), (std::vector<int>{3, 4, 5}));

  auto joined = PersistentTreapMap<int, std::string>::join(left, right);
  auto items = joined.items();
  ASSERT_EQ(items.size(), 6u);
  for (size_t i = 0; i < items.size(); ++i)
    {
      EXPECT_EQ(items[i].first, static_cast<int>(i));
      EXPECT_EQ(items[i].second, std::to_string(i));
    }
  EXPECT_TRUE(joined.verify());
}

TEST(PersistentTreapMap, SupportsMoveOnlyMappedValues)
{
  PersistentTreapMap<int, std::unique_ptr<int>> map;
  auto one = map.insert(1, std::make_unique<int>(10));
  auto two = one.insert_or_assign(1, std::make_unique<int>(20));
  auto three = two.insert(2, std::make_unique<int>(30));

  ASSERT_NE(one.find(1), nullptr);
  ASSERT_NE(two.find(1), nullptr);
  ASSERT_NE(three.find(2), nullptr);
  EXPECT_EQ(**one.find(1), 10);
  EXPECT_EQ(**two.find(1), 20);
  EXPECT_EQ(**three.find(2), 30);
  EXPECT_TRUE(one.verify());
  EXPECT_TRUE(two.verify());
  EXPECT_TRUE(three.verify());
}

TEST(PersistentTreapMap, RandomizedTraceMatchesStdMapAndKeepsOldVersions)
{
  std::mt19937 rng(0xBADC0DEu);
  std::uniform_int_distribution<int> key_dist(0, 60);
  std::uniform_int_distribution<int> value_dist(-1000, 1000);
  std::uniform_int_distribution<int> op_dist(0, 2);

  PersistentTreapMap<int, int> subject;
  std::map<int, int> reference;
  std::vector<PersistentTreapMap<int, int>> old_subjects;
  std::vector<std::map<int, int>> old_references;

  for (int iter = 0; iter < 1500; ++iter)
    {
      if (iter % 41 == 0)
        {
          old_subjects.push_back(subject);
          old_references.push_back(reference);
        }

      const int key = key_dist(rng);
      const int op = op_dist(rng);
      if (op == 0)
        {
          const int value = value_dist(rng);
          subject = subject.insert(key, value);
          reference.insert({key, value});
        }
      else if (op == 1)
        {
          const int value = value_dist(rng);
          subject = subject.insert_or_assign(key, value);
          reference[key] = value;
        }
      else
        {
          subject = subject.erase(key);
          reference.erase(key);
        }

      ASSERT_TRUE(subject.verify());
      EXPECT_EQ(subject.size(), reference.size());
      for (const auto &[k, v] : reference)
        {
          ASSERT_NE(subject.find(k), nullptr);
          EXPECT_EQ(*subject.find(k), v);
        }
    }

  for (size_t i = 0; i < old_subjects.size(); ++i)
    {
      EXPECT_EQ(old_subjects[i].size(), old_references[i].size());
      for (const auto &[k, v] : old_references[i])
        {
          ASSERT_NE(old_subjects[i].find(k), nullptr);
          EXPECT_EQ(*old_subjects[i].find(k), v);
        }
    }
}
