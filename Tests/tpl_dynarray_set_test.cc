
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
 * @file tpl_dynarray_set_test.cc
 * @brief Tests for Tpl Dynarray Set
 */
#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

#include <tpl_dynarray_set.H>

using namespace Aleph;

namespace
{
  struct AbsEqual
  {
    bool operator()(int a, int b) const noexcept
    {
      auto aa = a < 0 ? -a : a;
      auto bb = b < 0 ? -b : b;
      return aa == bb;
    }
  };

  struct ModEqual
  {
    int mod = 1;

    bool operator()(int a, int b) const noexcept
    {
      auto aa = a % mod;
      auto bb = b % mod;
      if (aa < 0)
        aa += mod;
      if (bb < 0)
        bb += mod;
      return aa == bb;
    }
  };
}

TEST(DynArraySet, DefaultConstructEmpty)
{
  DynArray_Set<int> s;
  EXPECT_EQ(s.size(), 0u);
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.search(1), nullptr);
  EXPECT_FALSE(s.contains(1));
  EXPECT_EQ(s.count(1), 0u);
}

TEST(DynArraySet, InsertSearchAndContains)
{
  DynArray_Set<int> s;

  auto *p1 = s.insert(10);
  ASSERT_NE(p1, nullptr);
  EXPECT_EQ(*p1, 10);
  EXPECT_EQ(s.size(), 1u);

  auto *p2 = s.insert(20);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(*p2, 20);
  EXPECT_EQ(s.size(), 2u);

  auto *f10 = s.search(10);
  ASSERT_NE(f10, nullptr);
  EXPECT_EQ(*f10, 10);
  EXPECT_TRUE(s.contains(20));
  EXPECT_FALSE(s.contains(30));
}

TEST(DynArraySet, DuplicatesAreAllowed)
{
  DynArray_Set<int> s;
  s.insert(7);
  s.insert(7);
  s.insert(7);

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.count(7), 3u);
  EXPECT_NE(s.search(7), nullptr);
}

TEST(DynArraySet, FindThrowsWhenMissing)
{
  DynArray_Set<int> s;
  s.insert(1);

  EXPECT_NO_THROW((void)s.find(1));
  EXPECT_THROW((void)s.find(2), std::domain_error);
}

TEST(DynArraySet, RemoveOne)
{
  DynArray_Set<int> s;
  s.insert(1);
  s.insert(2);
  s.insert(1);

  EXPECT_EQ(s.count(1), 2u);
  EXPECT_TRUE(s.remove_one(1));
  EXPECT_EQ(s.count(1), 1u);
  EXPECT_EQ(s.size(), 2u);

  EXPECT_TRUE(s.remove_one(1));
  EXPECT_EQ(s.count(1), 0u);
  EXPECT_EQ(s.size(), 1u);

  EXPECT_FALSE(s.remove_one(1));
  EXPECT_EQ(s.size(), 1u);
}

TEST(DynArraySet, RemoveAll)
{
  DynArray_Set<int> s;
  s.insert(5);
  s.insert(6);
  s.insert(5);
  s.insert(7);
  s.insert(5);

  EXPECT_EQ(s.count(5), 3u);
  auto removed = s.remove_all(5);
  EXPECT_EQ(removed, 3u);
  EXPECT_EQ(s.count(5), 0u);
  EXPECT_EQ(s.size(), 2u);
  EXPECT_TRUE(s.contains(6));
  EXPECT_TRUE(s.contains(7));
}

TEST(DynArraySet, CustomEqualityPredicate)
{
  DynArray_Set<int, AbsEqual> s(AbsEqual{});

  s.insert(10);
  s.insert(-10);
  s.insert(20);

  EXPECT_TRUE(s.contains(10));
  EXPECT_TRUE(s.contains(-10));

  // Under AbsEqual, both 10 and -10 match the key 10.
  EXPECT_EQ(s.count(10), 2u);

  // search should return a pointer to some matching element.
  auto *p = s.search(10);
  ASSERT_NE(p, nullptr);
  EXPECT_TRUE(*p == 10 || *p == -10);
}

TEST(DynArraySet, SetEqualChangesSearchSemantics)
{
  DynArray_Set<int, ModEqual> s(ModEqual{10});

  s.insert(10);
  s.insert(11);

  // With mod=10, 10 and 20 are equivalent.
  EXPECT_TRUE(s.contains(20));
  EXPECT_EQ(s.count(20), 1u);

  s.set_equal(ModEqual{11});

  // With mod=11, 10 and 20 are no longer equivalent.
  EXPECT_FALSE(s.contains(20));
}

TEST(DynArraySet, InheritedDynArrayApiStillWorks)
{
  DynArray_Set<int> s(12, 10, 4);
  s.append(1);
  s.append(2);
  s.append(3);

  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.get_first(), 1);
  EXPECT_EQ(s.get_last(), 3);

  // removal-by-reference (swap-with-last) is inherited.
  auto &ref = s.access(0);
  s.remove(ref);
  EXPECT_EQ(s.size(), 2u);
}
