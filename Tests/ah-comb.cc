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
 * @file ah-comb.cc
 * @brief Tests for Ah Comb
 */

# include <gtest/gtest.h>

# include <ah-comb.H>
# include <ahFunctional.H>

# include <algorithm>
# include <array>
# include <set>
# include <sstream>
# include <vector>

using namespace Aleph;

namespace
{
/**
 * @brief Converts a DynList<int> to a comma-separated string of its elements.
 *
 * @param l Source list whose elements will be concatenated in order.
 * @return std::string The elements of `l` joined by commas (e.g., "1,2,3"); returns an empty string if `l` is empty.
 */
std::string list_to_string(const DynList<int> & l)
{
  std::ostringstream oss;
  bool first = true;
  l.for_each([&](int v)
  {
    if (not first)
      oss << ',';
    first = false;
    oss << v;
  });
  return oss.str();
}

template <class Seq>
/**
 * @brief Converts a sequence-like object to a comma-separated string.
 *
 * @tparam Seq Sequence-like type that provides size() and operator()(size_t).
 * @param s Sequence whose elements will be converted via stream insertion and joined with commas.
 * @return std::string Comma-separated representation of the sequence's elements (empty string when the sequence is empty).
 */
std::string seq_to_string(const Seq & s)
{
  std::ostringstream oss;
  for (size_t i = 0; i < s.size(); ++i)
    {
      if (i > 0)
        oss << ',';
      oss << s(i);
    }
  return oss.str();
}

/**
 * @brief Converts a vector of integers to a comma-separated string.
 *
 * @param v Vector of integers to stringify.
 * @return std::string Comma-separated sequence of the vector's elements; empty string if the vector is empty.
 */
std::string vector_to_string(const std::vector<int> & v)
{
  std::ostringstream oss;
  for (size_t i = 0; i < v.size(); ++i)
    {
      if (i > 0)
        oss << ',';
      oss << v[i];
    }
  return oss.str();
}
}

TEST(AhComb, TransposeEmpty)
{
  DynList<DynList<int>> m;
  EXPECT_TRUE(transpose(m).is_empty());

  in_place_transpose(m);
  EXPECT_TRUE(m.is_empty());
}

TEST(AhComb, TransposeRectangular)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<int> r2 = {4, 5, 6};
  DynList<DynList<int>> m = {r1, r2};

  DynList<int> c1 = {1, 4};
  DynList<int> c2 = {2, 5};
  DynList<int> c3 = {3, 6};
  DynList<DynList<int>> expected = {c1, c2, c3};

  EXPECT_EQ(transpose(m), expected);

  auto m2 = m;
  in_place_transpose(m2);
  EXPECT_EQ(m2, expected);
}

TEST(AhComb, InPlaceTransposeDynArray)
{
  DynArray<DynArray<int>> m;

  DynArray<int> r1;
  r1.reserve(3);
  r1(0) = 1;
  r1(1) = 2;
  r1(2) = 3;

  DynArray<int> r2;
  r2.reserve(3);
  r2(0) = 4;
  r2(1) = 5;
  r2(2) = 6;

  m.reserve(2);
  m(0) = r1;
  m(1) = r2;

  in_place_transpose(m);

  ASSERT_EQ(m.size(), 3u);
  ASSERT_EQ(m(0).size(), 2u);
  EXPECT_EQ(m(0)(0), 1);
  EXPECT_EQ(m(0)(1), 4);
  EXPECT_EQ(m(1)(0), 2);
  EXPECT_EQ(m(1)(1), 5);
  EXPECT_EQ(m(2)(0), 3);
  EXPECT_EQ(m(2)(1), 6);
}

TEST(AhComb, TraversePermEnumeratesAll)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  std::set<std::string> perms;
  bool ok = traverse_perm(l, [&](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_TRUE(ok);
  EXPECT_EQ(perms.size(), 4u);
  EXPECT_TRUE(perms.contains("1,10"));
  EXPECT_TRUE(perms.contains("2,10"));
  EXPECT_TRUE(perms.contains("1,20"));
  EXPECT_TRUE(perms.contains("2,20"));
}

TEST(AhComb, TraversePermEarlyStop)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  size_t seen = 0;
  bool ok = traverse_perm(l, [&](const DynList<int> &)
  {
    ++seen;
    return seen < 3; // stop on third call
  });

  EXPECT_FALSE(ok);
  EXPECT_EQ(seen, 3u);
}

TEST(AhComb, BuildPerms)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto perms = build_perms(l);
  EXPECT_EQ(perms.size(), 2u);

  std::set<std::string> s;
  perms.for_each([&](const DynList<int> & p) { s.insert(list_to_string(p)); });
  EXPECT_TRUE(s.contains("1,10"));
  EXPECT_TRUE(s.contains("2,10"));
}

TEST(AhComb, BuildCombsDeduplicates)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {2, 1};
  DynList<DynList<int>> l = {l1, l2};

  auto combs = build_combs(l);

  // Possible permutations are (1,2), (1,1), (2,2), (2,1).
  // After sorting: (1,2), (1,1), (2,2), (1,2) => unique: 3
  EXPECT_EQ(combs.size(), 3u);

  std::set<std::string> s;
  combs.for_each([&](const DynList<int> & c) { s.insert(list_to_string(c)); });
  EXPECT_TRUE(s.contains("1,1"));
  EXPECT_TRUE(s.contains("1,2"));
  EXPECT_TRUE(s.contains("2,2"));
}

TEST(AhComb, FoldPermAccumulates)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  // Sum of first elements across permutations:
  // (1,10), (2,10), (1,20), (2,20) => sum = 6
  auto sum = fold_perm<size_t, int>(0u, l, [](size_t a, const DynList<int> & p)
  {
    return a + p.get_first();
  });

  EXPECT_EQ(sum, 6u);
}

TEST(AhComb, TraversePermEmptyInputCallsOnce)
{
  DynList<DynList<int>> l;
  size_t calls = 0;
  bool ok = traverse_perm(l, [&](const DynList<int> & p)
  {
    ++calls;
    EXPECT_TRUE(p.is_empty());
    return true;
  });
  EXPECT_TRUE(ok);
  EXPECT_EQ(calls, 1u);
}

TEST(AhComb, BuildPermsEmptyInputReturnsOneEmptyPermutation)
{
  DynList<DynList<int>> l;
  auto perms = build_perms(l);
  ASSERT_EQ(perms.size(), 1u);
  EXPECT_TRUE(perms.get_first().is_empty());
}

TEST(AhComb, BuildCombsEmptyInputReturnsOneEmptyCombination)
{
  DynList<DynList<int>> l;
  auto combs = build_combs(l);
  ASSERT_EQ(combs.size(), 1u);
  EXPECT_TRUE(combs.get_first().is_empty());
}

TEST(AhComb, TransposeWithZeroColumns)
{
  DynList<int> r1;
  DynList<int> r2;
  DynList<DynList<int>> m = {r1, r2};

  EXPECT_TRUE(transpose(m).is_empty());

  auto m2 = m;
  in_place_transpose(m2);
  EXPECT_TRUE(m2.is_empty());
}

#ifndef NDEBUG
TEST(AhComb, NonRectangularTransposeDies)
{
  DynList<int> r1 = {1, 2};
  DynList<int> r2 = {3};
  DynList<DynList<int>> m = {r1, r2};

  EXPECT_DEATH((void)transpose(m), "");

  auto m2 = m;
  EXPECT_DEATH(in_place_transpose(m2), "");
}
#endif

// ==== New tests for additional functions ====

TEST(AhComb, PermCountBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20, 30};
  DynList<DynList<int>> l = {l1, l2};

  EXPECT_EQ(perm_count(l), 6u); // 2 * 3
}

TEST(AhComb, PermCountEmpty)
{
  DynList<DynList<int>> l;
  EXPECT_EQ(perm_count(l), 1u); // One empty permutation
}

TEST(AhComb, PermCountWithEmptyList)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2; // empty
  DynList<DynList<int>> l = {l1, l2};

  EXPECT_EQ(perm_count(l), 0u); // Any empty list = 0 permutations
}

TEST(AhComb, PermCountSingleList)
{
  DynList<int> l1 = {1, 2, 3, 4, 5};
  DynList<DynList<int>> l = {l1};

  EXPECT_EQ(perm_count(l), 5u);
}

TEST(AhComb, ExistsPermFindsMatch)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool found = exists_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 2 && p.get_last() == 20;
  });

  EXPECT_TRUE(found);
}

TEST(AhComb, ExistsPermNoMatch)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool found = exists_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 100; // Never true
  });

  EXPECT_FALSE(found);
}

TEST(AhComb, ExistsPermStopsEarly)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20, 30};
  DynList<DynList<int>> l = {l1, l2};

  size_t calls = 0;
  (void)exists_perm(l, [&calls](const DynList<int> & p)
  {
    ++calls;
    return p.get_first() == 1 && p.get_last() == 10; // First one matches
  });

  EXPECT_EQ(calls, 1u); // Should stop after first match
}

TEST(AhComb, AllPermAllSatisfy)
{
  DynList<int> l1 = {2, 4};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  // All sums are >= 12
  bool all = all_perm(l, [](const DynList<int> & p)
  {
    int sum = 0;
    p.for_each([&sum](int v) { sum += v; });
    return sum >= 12;
  });

  EXPECT_TRUE(all);
}

TEST(AhComb, AllPermSomeFail)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool all = all_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 2; // Only half satisfy this
  });

  EXPECT_FALSE(all);
}

TEST(AhComb, AllPermStopsEarly)
{
  DynList<int> l1 = {1, 2, 3, 4};
  DynList<int> l2 = {10, 20, 30, 40};
  DynList<DynList<int>> l = {l1, l2};

  size_t calls = 0;
  (void)all_perm(l, [&calls](const DynList<int> & p)
  {
    ++calls;
    return p.get_first() != 1; // First permutation fails
  });

  EXPECT_EQ(calls, 1u); // Should stop after first failure
}

TEST(AhComb, NonePermNoneSatisfy)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool none = none_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 100; // Never true
  });

  EXPECT_TRUE(none);
}

TEST(AhComb, NonePermSomeSatisfy)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  bool none = none_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() == 1; // Some satisfy
  });

  EXPECT_FALSE(none);
}

TEST(AhComb, FilterPermBasic)
{
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> & p)
  {
    return p.get_first() >= 2; // Only 2,10  2,20  3,10  3,20
  });

  EXPECT_EQ(filtered.size(), 4u);
}

TEST(AhComb, FilterPermEmpty)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> &)
  {
    return false; // None pass
  });

  EXPECT_TRUE(filtered.is_empty());
}

TEST(AhComb, FilterPermAll)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto filtered = filter_perm(l, [](const DynList<int> &)
  {
    return true; // All pass
  });

  EXPECT_EQ(filtered.size(), 2u);
}

TEST(AhComb, MapPermBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<DynList<int>> l = {l1, l2};

  auto sums = map_perm<int>(l, [](const DynList<int> & p)
  {
    int sum = 0;
    p.for_each([&sum](int v) { sum += v; });
    return sum;
  });

  EXPECT_EQ(sums.size(), 4u);

  std::set<int> s;
  sums.for_each([&s](int v) { s.insert(v); });
  // (1,10)=11, (2,10)=12, (1,20)=21, (2,20)=22
  EXPECT_TRUE(s.contains(11));
  EXPECT_TRUE(s.contains(12));
  EXPECT_TRUE(s.contains(21));
  EXPECT_TRUE(s.contains(22));
}

TEST(AhComb, MapPermToString)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  auto strs = map_perm<std::string>(l, [](const DynList<int> & p)
  {
    return list_to_string(p);
  });

  EXPECT_EQ(strs.size(), 2u);

  std::set<std::string> s;
  strs.for_each([&s](const std::string & v) { s.insert(v); });
  EXPECT_TRUE(s.contains("1,10"));
  EXPECT_TRUE(s.contains("2,10"));
}

TEST(AhComb, ForEachPermBasic)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  std::set<std::string> perms;
  for_each_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
  });

  EXPECT_EQ(perms.size(), 2u);
  EXPECT_TRUE(perms.contains("1,10"));
  EXPECT_TRUE(perms.contains("2,10"));
}

TEST(AhComb, ForEachPermEmpty)
{
  DynList<DynList<int>> l;

  size_t calls = 0;
  for_each_perm(l, [&calls](const DynList<int> & p)
  {
    EXPECT_TRUE(p.is_empty());
    ++calls;
  });

  EXPECT_EQ(calls, 1u);
}

TEST(AhComb, TraversePermThreeLists)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {10, 20};
  DynList<int> l3 = {100, 200};
  DynList<DynList<int>> l = {l1, l2, l3};

  std::set<std::string> perms;
  traverse_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_EQ(perms.size(), 8u); // 2 * 2 * 2
}

TEST(AhComb, TraversePermSingleElement)
{
  DynList<int> l1 = {42};
  DynList<DynList<int>> l = {l1};

  std::set<std::string> perms;
  traverse_perm(l, [&perms](const DynList<int> & p)
  {
    perms.insert(list_to_string(p));
    return true;
  });

  EXPECT_EQ(perms.size(), 1u);
  EXPECT_TRUE(perms.contains("42"));
}

TEST(AhComb, TransposeSingleRow)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<DynList<int>> m = {r1};

  auto t = transpose(m);
  EXPECT_EQ(t.size(), 3u); // 3 columns => 3 rows

  size_t idx = 1;
  for (auto it = t.get_it(); it.has_curr(); it.next_ne(), ++idx)
    {
      EXPECT_EQ(it.get_curr().size(), 1u);
      EXPECT_EQ(it.get_curr().get_first(), static_cast<int>(idx));
    }
}

TEST(AhComb, TransposeSingleColumn)
{
  DynList<int> r1 = {1};
  DynList<int> r2 = {2};
  DynList<int> r3 = {3};
  DynList<DynList<int>> m = {r1, r2, r3};

  auto t = transpose(m);
  EXPECT_EQ(t.size(), 1u); // 1 column => 1 row
  EXPECT_EQ(t.get_first().size(), 3u); // 3 rows => 3 columns

  DynList<int> expected = {1, 2, 3};
  EXPECT_EQ(t.get_first(), expected);
}

TEST(AhComb, InPlaceTransposeSingleRow)
{
  DynList<int> r1 = {1, 2, 3};
  DynList<DynList<int>> m = {r1};

  in_place_transpose(m);
  EXPECT_EQ(m.size(), 3u);

  auto it = m.get_it();
  EXPECT_EQ(it.get_curr().get_first(), 1);
  it.next_ne();
  EXPECT_EQ(it.get_curr().get_first(), 2);
  it.next_ne();
  EXPECT_EQ(it.get_curr().get_first(), 3);
}

TEST(AhComb, TransposeSquare)
{
  DynList<int> r1 = {1, 2};
  DynList<int> r2 = {3, 4};
  DynList<DynList<int>> m = {r1, r2};

  auto t = transpose(m);

  DynList<int> c1 = {1, 3};
  DynList<int> c2 = {2, 4};
  DynList<DynList<int>> expected = {c1, c2};

  EXPECT_EQ(t, expected);
}

TEST(AhComb, BuildCombsAllSame)
{
  DynList<int> l1 = {1};
  DynList<int> l2 = {1};
  DynList<DynList<int>> l = {l1, l2};

  auto combs = build_combs(l);
  EXPECT_EQ(combs.size(), 1u);

  DynList<int> expected = {1, 1};
  EXPECT_EQ(sort(combs.get_first()), expected);
}

TEST(AhComb, BuildCombsThreeLists)
{
  DynList<int> l1 = {1, 2};
  DynList<int> l2 = {2, 1};
  DynList<int> l3 = {1, 2};
  DynList<DynList<int>> l = {l1, l2, l3};

  auto combs = build_combs(l);

  // Possible combinations after sorting:
  // (1,1,1), (1,1,2), (1,2,2), (2,2,2)
  EXPECT_EQ(combs.size(), 4u);
}

TEST(AhComb, FoldPermMultiplyFirstElements)
{
  DynList<int> l1 = {2, 3};
  DynList<int> l2 = {10};
  DynList<DynList<int>> l = {l1, l2};

  // (2,10) and (3,10) => product of first elements = 2 * 3 = 6
  auto product = fold_perm<int, int>(1, l, [](int acu, const DynList<int> & p)
  {
    return acu * p.get_first();
  });

  EXPECT_EQ(product, 6);
}

TEST(AhComb, StressTestManyPermutations)
{
  // 3^5 = 243 permutations
  DynList<int> l1 = {1, 2, 3};
  DynList<int> l2 = {10, 20, 30};
  DynList<int> l3 = {100, 200, 300};
  DynList<int> l4 = {1000, 2000, 3000};
  DynList<int> l5 = {10000, 20000, 30000};
  DynList<DynList<int>> l = {l1, l2, l3, l4, l5};

  EXPECT_EQ(perm_count(l), 243u);

  size_t count = 0;
  traverse_perm(l, [&count](const DynList<int> & p)
  {
    EXPECT_EQ(p.size(), 5u);
    ++count;
    return true;
  });

  EXPECT_EQ(count, 243u);
}

TEST(AhComb, NodiscardBuildPerms)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto perms = build_perms(l);
  EXPECT_EQ(perms.size(), 2u);
}

TEST(AhComb, NodiscardBuildCombs)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto combs = build_combs(l);
  EXPECT_EQ(combs.size(), 2u);
}

TEST(AhComb, NodiscardTranspose)
{
  DynList<int> r1 = {1, 2};
  DynList<DynList<int>> m = {r1};

  // Should not trigger nodiscard warning - use result
  auto t = transpose(m);
  EXPECT_EQ(t.size(), 2u);
}

TEST(AhComb, NodiscardFoldPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto sum = fold_perm<int, int>(0, l, [](int a, const DynList<int> & p)
  {
    return a + p.get_first();
  });
  EXPECT_EQ(sum, 3);
}

TEST(AhComb, NodiscardPermCount)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto count = perm_count(l);
  EXPECT_EQ(count, 2u);
}

TEST(AhComb, NodiscardExistsPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto exists = exists_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_TRUE(exists);
}

TEST(AhComb, NextPermutationArrayBasicLexicographicOrder)
{
  Array<int> a = {1, 2, 3};

  std::vector<std::string> seen;
  seen.push_back(seq_to_string(a));
  while (next_permutation(a))
    seen.push_back(seq_to_string(a));

  const std::vector<std::string> expected = {
    "1,2,3", "1,3,2", "2,1,3", "2,3,1", "3,1,2", "3,2,1"
  };
  EXPECT_EQ(seen, expected);

  // Default behavior resets to first permutation at the end.
  EXPECT_EQ(seq_to_string(a), "1,2,3");
}

TEST(AhComb, NextPermutationArrayHandlesDuplicatesWithoutRepeats)
{
  Array<int> a = {1, 1, 2};

  std::vector<std::string> seen;
  seen.push_back(seq_to_string(a));
  while (next_permutation(a))
    seen.push_back(seq_to_string(a));

  const std::vector<std::string> expected = {
    "1,1,2", "1,2,1", "2,1,1"
  };
  EXPECT_EQ(seen, expected);
  EXPECT_EQ(seq_to_string(a), "1,1,2");
}

TEST(AhComb, NextPermutationArrayMatchesStdWithCustomComparator)
{
  Array<int> a = {3, 2, 1};
  std::vector<int> v = {3, 2, 1};

  while (true)
    {
      EXPECT_EQ(seq_to_string(a), vector_to_string(v));

      const bool ha = Aleph::next_permutation(a, Aleph::greater<int>());
      const bool hv = std::next_permutation(v.begin(), v.end(),
                                            std::greater<int>());
      EXPECT_EQ(ha, hv);

      if (not ha)
        break;
    }
}

TEST(AhComb, NextPermutationResetOnLastCanBeDisabled)
{
  Array<int> a = {3, 2, 1};
  EXPECT_FALSE(next_permutation(a, Aleph::less<int>(), false));
  EXPECT_EQ(seq_to_string(a), "3,2,1");
}

TEST(AhComb, NextPermutationDynArray)
{
  DynArray<int> a;
  a.reserve(3);
  a(0) = 1;
  a(1) = 2;
  a(2) = 3;

  size_t count = 1;
  while (next_permutation(a))
    ++count;

  EXPECT_EQ(count, 6u);
  EXPECT_EQ(seq_to_string(a), "1,2,3");
}

TEST(AhComb, NextCombinationIndicesArrayEnumeratesAll)
{
  Array<size_t> idx = {0, 1, 2};
  const size_t n = 5;

  std::vector<std::array<size_t, 3>> seen;
  while (true)
    {
      seen.push_back({idx(0), idx(1), idx(2)});
      if (not next_combination_indices(idx, n))
        break;
    }

  const std::vector<std::array<size_t, 3>> expected = {
    {0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 2, 3}, {0, 2, 4},
    {0, 3, 4}, {1, 2, 3}, {1, 2, 4}, {1, 3, 4}, {2, 3, 4}
  };
  EXPECT_EQ(seen, expected);

  // Default behavior resets to first combination at the end.
  EXPECT_EQ(idx(0), 0u);
  EXPECT_EQ(idx(1), 1u);
  EXPECT_EQ(idx(2), 2u);
}

TEST(AhComb, NextCombinationIndicesResetOnLastCanBeDisabled)
{
  Array<size_t> idx = {2, 3, 4};
  EXPECT_FALSE(next_combination_indices(idx, 5, false));
  EXPECT_EQ(idx(0), 2u);
  EXPECT_EQ(idx(1), 3u);
  EXPECT_EQ(idx(2), 4u);
}

/**
 * @brief Verifies input validation for next_combination_indices.
 *
 * Checks that next_combination_indices throws a std::domain_error when the
 * indices are not strictly increasing, throws std::out_of_range when an index
 * is >= n, and throws std::domain_error when the number of indices is greater
 * than n.
 */
TEST(AhComb, NextCombinationIndicesValidation)
{
  Array<size_t> not_increasing = {0, 2, 2};
  EXPECT_THROW(next_combination_indices(not_increasing, 5), std::domain_error);

  Array<size_t> out_of_range = {0, 1, 5};
  EXPECT_THROW(next_combination_indices(out_of_range, 5), std::out_of_range);

  Array<size_t> k_gt_n = {0, 1, 2};
  EXPECT_THROW(next_combination_indices(k_gt_n, 2), std::domain_error);
}

TEST(AhComb, NextCombinationIndicesDynArray)
{
  DynArray<size_t> idx;
  idx.reserve(2);
  idx(0) = 0;
  idx(1) = 1;

  size_t count = 1;
  while (next_combination_indices(idx, 4))
    ++count;

  EXPECT_EQ(count, 6u); // C(4,2)
  EXPECT_EQ(idx(0), 0u);
  EXPECT_EQ(idx(1), 1u);
}

TEST(AhComb, CombinationCountBasicCases)
{
  EXPECT_EQ(combination_count(0, 0), 1u);
  EXPECT_EQ(combination_count(5, 0), 1u);
  EXPECT_EQ(combination_count(5, 5), 1u);
  EXPECT_EQ(combination_count(5, 2), 10u);
  EXPECT_EQ(combination_count(5, 3), 10u);
  EXPECT_EQ(combination_count(20, 10), 184756u);
  EXPECT_EQ(combination_count(6, 8), 0u);
}

TEST(AhComb, CombinationCountOverflowThrows)
{
  if (sizeof(size_t) >= 8)
    EXPECT_THROW(combination_count(68, 34), std::runtime_error);
  else
    EXPECT_THROW(combination_count(35, 17), std::runtime_error);
}

TEST(AhComb, CombinationMaskEnumerationAndReset)
{
  uint64_t mask = first_combination_mask(3); // 0b00111
  const size_t n = 5;

  std::vector<uint64_t> seen;
  while (true)
    {
      seen.push_back(mask);
      if (not next_combination_mask(mask, n))
        break;
    }

  const std::vector<uint64_t> expected = {
    0x07, 0x0b, 0x0d, 0x0e, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c
  };
  EXPECT_EQ(seen, expected);
  EXPECT_EQ(mask, uint64_t(0x07)); // reset to first
}

TEST(AhComb, CombinationMaskValidation)
{
  uint64_t mask = 0;
  EXPECT_THROW((void)next_combination_mask(mask, 65), std::out_of_range);

  mask = (uint64_t(1) << 7); // outside n=5
  EXPECT_THROW((void)next_combination_mask(mask, 5), std::domain_error);
}

TEST(AhComb, ForEachCombinationArrayAndEarlyStop)
{
  Array<int> values = {10, 20, 30, 40};

  std::vector<std::string> seen;
  bool full = for_each_combination(values, 2, [&seen](const Array<int> & c)
  {
    seen.push_back(seq_to_string(c));
    return true;
  });
  EXPECT_TRUE(full);

  const std::vector<std::string> expected = {
    "10,20", "10,30", "10,40", "20,30", "20,40", "30,40"
  };
  EXPECT_EQ(seen, expected);

  size_t calls = 0;
  bool done = for_each_combination(values, 3, [&calls](const Array<int> &)
  {
    ++calls;
    return calls < 3;
  });
  EXPECT_FALSE(done);
  EXPECT_EQ(calls, 3u);
}

TEST(AhComb, ForEachCombinationEdgeCases)
{
  Array<int> values = {1, 2, 3};

  size_t calls = 0;
  bool ok = for_each_combination(values, 0, [&calls](const Array<int> & c)
  {
    ++calls;
    EXPECT_EQ(c.size(), 0u);
    return true;
  });
  EXPECT_TRUE(ok);
  EXPECT_EQ(calls, 1u);

  EXPECT_THROW((void)for_each_combination(values, 4,
              [](const Array<int> &) { return true; }), std::domain_error);
}

TEST(AhComb, BuildCombinationsArrayAndDynArray)
{
  Array<int> values = {1, 2, 3, 4};
  auto combs = build_combinations(values, 3);
  ASSERT_EQ(combs.size(), 4u);
  EXPECT_EQ(seq_to_string(combs(0)), "1,2,3");
  EXPECT_EQ(seq_to_string(combs(1)), "1,2,4");
  EXPECT_EQ(seq_to_string(combs(2)), "1,3,4");
  EXPECT_EQ(seq_to_string(combs(3)), "2,3,4");

  DynArray<int> dyn;
  dyn.reserve(4);
  dyn(0) = 1;
  dyn(1) = 2;
  dyn(2) = 3;
  dyn(3) = 4;

  auto combs_dyn = build_combinations(dyn, 2);
  ASSERT_EQ(combs_dyn.size(), 6u);
  EXPECT_EQ(seq_to_string(combs_dyn(0)), "1,2");
  EXPECT_EQ(seq_to_string(combs_dyn(5)), "3,4");
}

TEST(AhComb, NodiscardAllPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto all = all_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_TRUE(all);
}

TEST(AhComb, NodiscardNonePerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto none = none_perm(l, [](const DynList<int> &) { return false; });
  EXPECT_TRUE(none);
}

TEST(AhComb, NodiscardFilterPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto filtered = filter_perm(l, [](const DynList<int> &) { return true; });
  EXPECT_EQ(filtered.size(), 2u);
}

TEST(AhComb, NodiscardMapPerm)
{
  DynList<int> l1 = {1, 2};
  DynList<DynList<int>> l = {l1};

  // Should not trigger nodiscard warning - use result
  auto mapped = map_perm<int>(l, [](const DynList<int> & p) { return p.get_first(); });
  EXPECT_EQ(mapped.size(), 2u);
}

TEST(AhComb, GrayCodeConversion)
{
  // n = 0..7
  const std::vector<uint32_t> expected_gray = {
    0, 1, 3, 2, 6, 7, 5, 4
  };

  for (uint32_t i = 0; i < expected_gray.size(); ++i)
    {
      uint32_t g = bin_to_gray(i);
      EXPECT_EQ(g, expected_gray[i]) << "bin_to_gray mismatch at i=" << i;
      EXPECT_EQ(gray_to_bin(g), i) << "gray_to_bin mismatch at g=" << g;
    }
}

TEST(AhComb, BuildGrayCode)
{
  const size_t n = 4;
  auto gray = build_gray_code(n);
  
  ASSERT_EQ(gray.size(), 16u);
  
  // Verify adjacency property (exactly one bit difference)
  for (size_t i = 1; i < gray.size(); ++i)
    {
      uint32_t diff = gray[i] ^ gray[i-1];
      // diff should be a power of 2
      EXPECT_TRUE(diff > 0 && (diff & (diff - 1)) == 0)
        << "Not a Gray code sequence at i=" << i 
        << " (values: " << gray[i-1] << ", " << gray[i] << ")";
    }
    
  // Verify cyclic property
  uint32_t diff_last = gray[gray.size()-1] ^ gray[0];
  EXPECT_TRUE(diff_last > 0 && (diff_last & (diff_last - 1)) == 0);
}

TEST(AhComb, GrayCodeLargeValue)
{
  uint64_t val = 0x123456789ABCDEF0ULL;
  uint64_t g = bin_to_gray(val);
  EXPECT_EQ(gray_to_bin(g), val);
}

TEST(AhComb, BuildGrayCodeThrowsOnTooLarge)
{
  EXPECT_THROW(build_gray_code(32), std::domain_error);
}
