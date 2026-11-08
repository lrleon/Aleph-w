
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
 * @file al-domain.cc
 * @brief Tests for Al Domain
 */
# include <gtest/gtest.h>

// al-domain.H first, on its own: keys() sorts with ahSort.H, which the
// header must include
# include <al-domain.H>

# include <climits>
# include <stdexcept>
# include <vector>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(Domain, basic)
{
  AlDomain<int> domain;

  EXPECT_TRUE(domain.is_empty());

  for (size_t i = 0; i < 10; ++i)
    domain.insert(i);

  EXPECT_FALSE(domain.contains(-1));
  EXPECT_FALSE(domain.contains(10));

  EXPECT_TRUE(domain.all([] (auto i) { return i >= 0 and i < 10; }));
}

namespace
{
  // The keys of d, as keys() gives them
  std::vector<int> keys_of(const AlDomain<int> & d)
  {
    std::vector<int> ret;
    d.keys().for_each([&ret] (int k) { ret.push_back(k); });
    return ret;
  }
}

TEST(IntRange, AStepThatIsNotPositiveThrows)
{
  // A step of 0 looped forever
  EXPECT_THROW(IntRange(0, 3, 0), std::domain_error);
  EXPECT_THROW(IntRange(0, 3, -1), std::domain_error);
  EXPECT_THROW(IntRange(5, 3, 0), std::domain_error);

  EXPECT_EQ(IntRange(0, 10, 2).size(), 6u);
  EXPECT_EQ(IntRange(5, 3).size(), 0u);
}

TEST(IntRange, ARangeUpToTheMaximumOfIntEnds)
{
  // Where long has 32 bits, as on Windows, the index of the loop
  // overflowed after INT_MAX
  const IntRange r(INT_MAX - 2, INT_MAX);
  EXPECT_EQ(keys_of(r), (std::vector<int>{INT_MAX - 2, INT_MAX - 1, INT_MAX}));

  const IntRange s(INT_MAX - 3, INT_MAX, 2);
  EXPECT_EQ(keys_of(s), (std::vector<int>{INT_MAX - 3, INT_MAX - 1}));

  const IntRange t(INT_MIN, INT_MIN + 1);
  EXPECT_EQ(keys_of(t), (std::vector<int>{INT_MIN, INT_MIN + 1}));
}

TEST(IntRange, ASizeAboveTheRangeOfIntThrows)
{
  // The elements are ints: a negative int converted to size_t, as
  // IntRange(-1), looped until memory ran out
  EXPECT_THROW(IntRange(static_cast<size_t>(INT_MAX) + 2), std::domain_error);
  EXPECT_THROW(IntRange(static_cast<size_t>(-1)), std::domain_error);

  EXPECT_EQ(keys_of(IntRange(size_t(3))), (std::vector<int>{0, 1, 2}));
}

TEST(AlDomain, KeysFollowTheInsertionsAndTheRemovals)
{
  // keys() kept the sorted keys of its first call
  AlDomain<int> d;
  (void) d.insert(3);
  (void) d.insert(1);
  EXPECT_EQ(keys_of(d), (std::vector<int>{1, 3}));

  (void) d.insert(2);
  EXPECT_EQ(keys_of(d), (std::vector<int>{1, 2, 3}));

  d.remove(1);
  EXPECT_EQ(keys_of(d), (std::vector<int>{2, 3}));

  (void) d.search_or_insert(0);
  EXPECT_EQ(keys_of(d), (std::vector<int>{0, 2, 3}));

  (void) d.contains_or_insert(5);
  EXPECT_EQ(keys_of(d), (std::vector<int>{0, 2, 3, 5}));

  (void) d.append(4);
  EXPECT_EQ(keys_of(d), (std::vector<int>{0, 2, 3, 4, 5}));

  d.remove_ptr(d.search(3));
  EXPECT_EQ(keys_of(d), (std::vector<int>{0, 2, 4, 5}));

  d.add(DynList<int>({7, 6}));
  EXPECT_EQ(keys_of(d), (std::vector<int>{0, 2, 4, 5, 6, 7}));

  AlDomain<int> e;
  (void) e.insert(9);
  EXPECT_EQ(keys_of(e), (std::vector<int>{9}));
  d.swap(e);
  EXPECT_EQ(keys_of(d), (std::vector<int>{9}));
  EXPECT_EQ(keys_of(e), (std::vector<int>{0, 2, 4, 5, 6, 7}));

  e.empty();
  EXPECT_TRUE(keys_of(e).empty());
  (void) e.insert(8);
  e.clear();
  EXPECT_TRUE(keys_of(e).empty());
}

TEST(AlDomain, KeysFollowARemovalThroughAnIterator)
{
  // A removal that AlDomain does not see changes the number of keys, which
  // keys() compares
  AlDomain<int> d;
  for (int i = 0; i < 5; ++i)
    (void) d.insert(i);
  EXPECT_EQ(keys_of(d).size(), 5u);

  auto it = d.get_it();
  const int removed = it.get_curr();
  it.del();

  std::vector<int> expected;
  for (int i = 0; i < 5; ++i)
    if (i != removed)
      expected.push_back(i);
  EXPECT_EQ(keys_of(d), expected);
}

TEST(AlDomain, ACopyHasTheSameKeys)
{
  AlDomain<int> d;
  (void) d.insert(2);
  (void) d.insert(1);
  EXPECT_EQ(keys_of(d), (std::vector<int>{1, 2}));

  AlDomain<int> c = d;
  (void) c.insert(0);
  EXPECT_EQ(keys_of(c), (std::vector<int>{0, 1, 2}));
  EXPECT_EQ(keys_of(d), (std::vector<int>{1, 2}));
}
