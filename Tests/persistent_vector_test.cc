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
 * @file persistent_vector_test.cc
 * @brief Tests for Aleph::PersistentVector.
 */

#include <gtest/gtest.h>

#include <tpl_persistent_vector.H>

#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

using namespace Aleph;

namespace
{
  struct CopyTrackedValue
  {
    static inline int copy_assignments = 0;
    static inline int moves = 0;

    int value = 0;

    CopyTrackedValue() = default;
    explicit CopyTrackedValue(const int v) noexcept : value(v) {}
    CopyTrackedValue(const CopyTrackedValue &) = default;

    CopyTrackedValue &operator = (const CopyTrackedValue &other) noexcept
    {
      value = other.value;
      ++copy_assignments;
      return *this;
    }

    CopyTrackedValue(CopyTrackedValue &&other) noexcept : value(other.value)
    {
      ++moves;
    }

    CopyTrackedValue &operator = (CopyTrackedValue &&other) noexcept
    {
      value = other.value;
      ++moves;
      return *this;
    }

    bool operator == (const CopyTrackedValue &other) const noexcept
    {
      return value == other.value;
    }

    static void reset_counters() noexcept
    {
      copy_assignments = 0;
      moves = 0;
    }
  };

  template <typename T>
  std::vector<T> to_vector(const PersistentVector<T> &vector)
  {
    std::vector<T> out;
    out.reserve(vector.size());
    for (size_t i = 0; i < vector.size(); ++i)
      out.push_back(vector.get(i));
    return out;
  }
}

TEST(PersistentVector, EmptyVectorBasics)
{
  PersistentVector<int> vector;

  EXPECT_TRUE(vector.is_empty());
  EXPECT_EQ(vector.size(), 0u);
  EXPECT_EQ(PersistentVector<int>::branching_factor(), 32u);
  EXPECT_TRUE(vector.verify());
  EXPECT_THROW(vector.get(0), std::out_of_range);
  EXPECT_THROW(vector.pop_back(), std::underflow_error);
}

TEST(PersistentVector, PushBackAndOldVersionsAreIndependent)
{
  PersistentVector<int> empty;
  auto one = empty.push_back(10);
  auto two = one.push_back(20);
  auto three = two.push_back(30);

  EXPECT_TRUE(empty.is_empty());
  EXPECT_EQ(one.size(), 1u);
  EXPECT_EQ(two.size(), 2u);
  EXPECT_EQ(three.size(), 3u);

  EXPECT_EQ(one.get(0), 10);
  EXPECT_EQ(two.get(0), 10);
  EXPECT_EQ(two.get(1), 20);
  EXPECT_EQ(three.get(2), 30);
  EXPECT_TRUE(empty.verify());
  EXPECT_TRUE(one.verify());
  EXPECT_TRUE(two.verify());
  EXPECT_TRUE(three.verify());
}

TEST(PersistentVector, SetReturnsNewVersion)
{
  auto base = PersistentVector<int>().push_back(1).push_back(2).push_back(3);
  auto changed = base.set(1, 99);

  EXPECT_EQ(to_vector(base), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(to_vector(changed), (std::vector<int>{1, 99, 3}));
  EXPECT_TRUE(base.verify());
  EXPECT_TRUE(changed.verify());
  EXPECT_THROW(base.set(3, 10), std::out_of_range);
}

TEST(PersistentVector, PopBackReturnsNewVersionAndShrinksLevels)
{
  PersistentVector<int> vector;
  for (int i = 0; i < 40; ++i)
    vector = vector.push_back(i);

  auto popped = vector.pop_back();
  for (int i = 0; i < 7; ++i)
    popped = popped.pop_back();

  EXPECT_EQ(vector.size(), 40u);
  EXPECT_EQ(popped.size(), 32u);
  EXPECT_EQ(vector.get(39), 39);
  EXPECT_EQ(popped.get(31), 31);
  EXPECT_THROW(popped.get(32), std::out_of_range);
  EXPECT_TRUE(vector.verify());
  EXPECT_TRUE(popped.verify());
}

TEST(PersistentVector, GrowsBeyondTwoTrieLevels)
{
  PersistentVector<int> vector;
  for (int i = 0; i < 1100; ++i)
    vector = vector.push_back(i * 3);

  ASSERT_EQ(vector.size(), 1100u);
  EXPECT_EQ(vector.get(0), 0);
  EXPECT_EQ(vector.get(31), 93);
  EXPECT_EQ(vector.get(32), 96);
  EXPECT_EQ(vector.get(1023), 3069);
  EXPECT_EQ(vector.get(1024), 3072);
  EXPECT_EQ(vector.get(1099), 3297);

  auto changed = vector.set(1024, -1);
  EXPECT_EQ(vector.get(1024), 3072);
  EXPECT_EQ(changed.get(1024), -1);
  EXPECT_TRUE(vector.verify());
  EXPECT_TRUE(changed.verify());
}

TEST(PersistentVector, ToArrayCopiesValues)
{
  PersistentVector<int> vector;
  for (int i = 0; i < 10; ++i)
    vector = vector.push_back(i);

  auto array = vector.to_array();
  ASSERT_EQ(array.size(), 10u);
  for (size_t i = 0; i < array.size(); ++i)
    EXPECT_EQ(array[i], static_cast<int>(i));
}

TEST(PersistentVector, ToArrayCopiesWithoutMovingFromTemporaries)
{
  PersistentVector<CopyTrackedValue> vector;
  const CopyTrackedValue one{1};
  const CopyTrackedValue two{2};

  vector = vector.push_back(one).push_back(two);

  CopyTrackedValue::reset_counters();
  auto array = vector.to_array();
  ASSERT_EQ(array.size(), 2u);
  EXPECT_EQ(array[0].value, 1);
  EXPECT_EQ(array[1].value, 2);
  EXPECT_EQ(CopyTrackedValue::copy_assignments, 2);
  EXPECT_EQ(CopyTrackedValue::moves, 0);
  EXPECT_TRUE(vector.verify());
}

TEST(PersistentVector, SupportsMoveOnlyValues)
{
  PersistentVector<std::unique_ptr<int>> vector;
  auto one = vector.push_back(std::make_unique<int>(10));
  auto two = one.push_back(std::make_unique<int>(20));
  auto changed = two.set(0, std::make_unique<int>(99));

  EXPECT_EQ(*one.get(0), 10);
  EXPECT_EQ(*two.get(0), 10);
  EXPECT_EQ(*two.get(1), 20);
  EXPECT_EQ(*changed.get(0), 99);
  EXPECT_EQ(*changed.get(1), 20);
  EXPECT_TRUE(one.verify());
  EXPECT_TRUE(two.verify());
  EXPECT_TRUE(changed.verify());
}

TEST(PersistentVector, RandomizedTraceMatchesStdVectorAndKeepsOldVersions)
{
  std::mt19937 rng(0xFACEB00Cu);
  std::uniform_int_distribution<int> op_dist(0, 2);
  std::uniform_int_distribution<int> value_dist(-5000, 5000);

  PersistentVector<int> subject;
  std::vector<int> reference;
  std::vector<PersistentVector<int>> old_subjects;
  std::vector<std::vector<int>> old_references;

  for (int iter = 0; iter < 1800; ++iter)
    {
      if (iter % 53 == 0)
        {
          old_subjects.push_back(subject);
          old_references.push_back(reference);
        }

      const int op = reference.empty() ? 0 : op_dist(rng);
      if (op == 0)
        {
          const int value = value_dist(rng);
          subject = subject.push_back(value);
          reference.push_back(value);
        }
      else if (op == 1)
        {
          std::uniform_int_distribution<size_t> index_dist(0, reference.size() - 1);
          const size_t index = index_dist(rng);
          const int value = value_dist(rng);
          subject = subject.set(index, value);
          reference[index] = value;
        }
      else
        {
          subject = subject.pop_back();
          reference.pop_back();
        }

      ASSERT_TRUE(subject.verify());
      ASSERT_EQ(subject.size(), reference.size());
      for (size_t i = 0; i < reference.size(); ++i)
        ASSERT_EQ(subject.get(i), reference[i]) << "index " << i;
    }

  for (size_t i = 0; i < old_subjects.size(); ++i)
    EXPECT_EQ(to_vector(old_subjects[i]), old_references[i]);
}
