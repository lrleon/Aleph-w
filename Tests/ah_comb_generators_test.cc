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
 * @file ah_comb_generators_test.cc
 * @brief Tests for lazy permutation/combination enumeration
 *        (ah-comb-generators.H).
 *
 * Property tests comparing lazy_permutations against std::next_permutation
 * (the trusted eager reference) and lazy_combinations against
 * combination_count / a brute-force bitmask enumeration: same count, same
 * set of results, no duplicates. Also covers empty/single-element inputs,
 * duplicate elements (ties), k == 0, k == n, and early break.
 */

#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <ah-comb-generators.H>

using namespace Aleph;

namespace
{
template <typename T>
std::vector<T> vec_of(const Array<T> &a)
{
  return to_stdvector(a);
}
}  // namespace

TEST(CombGenerators, PermutationsOfEmptyAndSingleton)
{
  int count = 0;
  for (const Array<int> &p : lazy_permutations(Array<int>{}))
    { (void) p; ++count; }
  EXPECT_EQ(count, 1);  // the empty sequence has exactly one permutation

  std::vector<std::vector<int>> seen;
  for (const Array<int> &p : lazy_permutations(Array<int>{7}))
    seen.push_back(vec_of(p));
  EXPECT_EQ(seen, (std::vector<std::vector<int>>{{7}}));
}

TEST(CombGenerators, PermutationsMatchStdNextPermutationCount)
{
  std::vector<int> ref = {1, 2, 3, 4};
  size_t eager_count = 0;
  do
    ++eager_count;
  while (std::next_permutation(ref.begin(), ref.end()));

  size_t lazy_count = 0;
  for (const Array<int> &p : lazy_permutations(Array<int>{1, 2, 3, 4}))
    { (void) p; ++lazy_count; }

  EXPECT_EQ(eager_count, 24u);  // 4!
  EXPECT_EQ(lazy_count, eager_count);
}

TEST(CombGenerators, PermutationsMatchStdNextPermutationContent)
{
  std::vector<std::vector<int>> eager;
  std::vector<int> ref = {1, 2, 3};
  std::sort(ref.begin(), ref.end());
  do
    eager.push_back(ref);
  while (std::next_permutation(ref.begin(), ref.end()));

  std::vector<std::vector<int>> lazy;
  for (const Array<int> &p : lazy_permutations(Array<int>{3, 1, 2}))
    lazy.push_back(vec_of(p));  // must copy: the yielded array is reused

  EXPECT_EQ(lazy, eager);  // both lexicographically ordered from the sorted start
}

TEST(CombGenerators, PermutationsWithDuplicatesCollapseLikeStd)
{
  std::vector<int> ref = {1, 1, 2};
  std::sort(ref.begin(), ref.end());
  size_t eager_count = 0;
  do
    ++eager_count;
  while (std::next_permutation(ref.begin(), ref.end()));

  size_t lazy_count = 0;
  for (const Array<int> &p : lazy_permutations(Array<int>{2, 1, 1}))
    { (void) p; ++lazy_count; }

  EXPECT_EQ(eager_count, 3u);  // 3!/2! = 3, not 6
  EXPECT_EQ(lazy_count, eager_count);
}

TEST(CombGenerators, PermutationsAreDistinctAsMultiset)
{
  std::set<std::vector<int>> seen;
  for (const Array<int> &p : lazy_permutations(Array<int>{1, 2, 3, 4}))
    seen.insert(vec_of(p));  // copy before the buffer advances
  EXPECT_EQ(seen.size(), 24u);
}

TEST(CombGenerators, PermutationsEarlyBreakStopsWork)
{
  int count = 0;
  for (const Array<int> &p : lazy_permutations(Array<int>{1, 2, 3, 4, 5}))
    {
      (void) p;
      if (++count == 3)
        break;
    }
  EXPECT_EQ(count, 3);  // stopped well before all 120 permutations
}

TEST(CombGenerators, CombinationsKZeroYieldsOneEmptyCombination)
{
  std::vector<std::vector<int>> seen;
  for (const Array<int> &c : lazy_combinations(Array<int>{1, 2, 3}, 0))
    seen.push_back(vec_of(c));
  ASSERT_EQ(seen.size(), 1u);
  EXPECT_TRUE(seen[0].empty());
}

TEST(CombGenerators, CombinationsKEqualsNYieldsWholeSetOnce)
{
  std::vector<std::vector<int>> seen;
  for (const Array<int> &c : lazy_combinations(Array<int>{1, 2, 3}, 3))
    seen.push_back(vec_of(c));
  ASSERT_EQ(seen.size(), 1u);
  EXPECT_EQ(seen[0], (std::vector<int>{1, 2, 3}));
}

TEST(CombGenerators, CombinationsKGreaterThanNThrows)
{
  EXPECT_THROW(
    {
      for (const Array<int> &c : lazy_combinations(Array<int>{1, 2}, 3))
        (void) c;
    },
    std::domain_error);
}

TEST(CombGenerators, CombinationsCountMatchesCombinationCount)
{
  const Array<int> a{1, 2, 3, 4, 5, 6};
  for (size_t k = 0; k <= a.size(); ++k)
    {
      size_t count = 0;
      for (const Array<int> &c : lazy_combinations(a, k))
        { (void) c; ++count; }
      EXPECT_EQ(count, combination_count(a.size(), k)) << "k=" << k;
    }
}

TEST(CombGenerators, CombinationsMatchBruteForceBitmaskEnumeration)
{
  const Array<int> a{10, 20, 30, 40, 50};
  const size_t n = a.size();
  const size_t k = 3;

  // Brute-force reference: every n-bit mask with exactly k bits set,
  // in ascending numeric order, collects the values at the set positions
  // in index order — the same convention lazy_combinations documents.
  std::vector<std::vector<int>> expected;
  for (uint64_t mask = 0; mask < (1u << n); ++mask)
    {
      if (static_cast<size_t>(__builtin_popcountll(mask)) != k)
        continue;
      std::vector<int> combo;
      for (size_t i = 0; i < n; ++i)
        if (mask & (1u << i))
          combo.push_back(a[i]);
      expected.push_back(combo);
    }
  std::sort(expected.begin(), expected.end());

  std::vector<std::vector<int>> got;
  for (const Array<int> &c : lazy_combinations(a, k))
    got.push_back(vec_of(c));
  std::sort(got.begin(), got.end());

  EXPECT_EQ(got, expected);
}

TEST(CombGenerators, CombinationsAreDistinct)
{
  const Array<int> a{1, 2, 3, 4, 5};
  std::set<std::vector<int>> seen;
  for (const Array<int> &c : lazy_combinations(a, 2))
    seen.insert(vec_of(c));
  EXPECT_EQ(seen.size(), combination_count(5, 2));
}

TEST(CombGenerators, CombinationsPreserveRelativeOrderOfSource)
{
  const Array<char> a{'a', 'b', 'c'};
  std::vector<std::vector<char>> seen;
  for (const Array<char> &c : lazy_combinations(a, 2))
    seen.push_back(vec_of(c));
  EXPECT_EQ(seen, (std::vector<std::vector<char>>{{'a', 'b'}, {'a', 'c'}, {'b', 'c'}}));
}

TEST(CombGenerators, CombinationSnapshotsSurviveGeneratorAdvancement)
{
  Generator<Array<int>> gen = lazy_combinations(Array<int>{1, 2, 3}, 2);

  auto it = gen.begin();
  ASSERT_NE(it, gen.end());
  Array<int> first = *it;

  ++it;
  ASSERT_NE(it, gen.end());
  Array<int> second = *it;

  EXPECT_EQ(vec_of(first), (std::vector<int>{1, 2}));
  EXPECT_EQ(vec_of(second), (std::vector<int>{1, 3}));
}
