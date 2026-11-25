
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
 * @file quotient_filter_limits_test.cc
 * @brief A quotient filter holds an item once, full or not, and takes
 *        the items of another filter all or none
 */

// Compact_Quotient_Filter::merge() dropped without a word the items of the
// other filter that did not fit, and the filter then denied items that it
// had been given, where it promises no false negatives.
// Quotient_Filter::merge() threw for an item that the full filter held
// already, and left a filter that could not take the other with some of
// its items. Compact_Quotient_Filter::insert() threw for an item that the
// full filter held. These tests fill filters, and merge them against the
// insertion of the same items one by one.

#include <gtest/gtest.h>

#include <random>
#include <stdexcept>

#include <compact-quotient-filter.H>
#include <quotient-filter.H>
#include <tpl_array.H>

using namespace Aleph;

namespace
{
  template <class Filter>
  class QuotientFilterLimits : public ::testing::Test
  {};

  using Filters = ::testing::Types<Quotient_Filter<int>, Compact_Quotient_Filter<int>>;
  TYPED_TEST_SUITE(QuotientFilterLimits, Filters);

  // The k-th item of these tests
  constexpr int item(const int k) noexcept { return k * 7919 + 13; }

  // Inserts items until the filter holds n, and returns those whose
  // insertion made it grow: the ones whose fingerprint no other item has
  template <class Filter>
  Array<int> fill(Filter & f, const size_t n)
  {
    Array<int> in;
    for (int k = 0; f.size() < n; ++k)
      {
        const size_t before = f.size();
        f.insert(item(k));
        if (f.size() > before)
          in.append(item(k));
      }
    return in;
  }

  // How many of the items of `in` the filter denies
  template <class Filter>
  size_t denied(const Filter & f, const Array<int> & in)
  {
    size_t n = 0;
    for (size_t i = 0; i < in.size(); ++i)
      if (not f.contains(in(i)))
        ++n;
    return n;
  }

  // How many of the items below `universe` the two filters answer
  // differently for
  template <class Filter>
  size_t differences(const Filter & a, const Filter & b, const int universe)
  {
    size_t n = 0;
    for (int x = 0; x < universe; ++x)
      if (a.contains(x) != b.contains(x))
        ++n;
    return n;
  }
}

// A full filter takes again each item that it holds, and refuses one that
// it does not hold, which leaves it as it was
TYPED_TEST(QuotientFilterLimits, AFullFilterHoldsAnItemOnce)
{
  for (const int q : {1, 2, 4, 6})
    for (const int r : {3, 8, 20})
      {
        SCOPED_TRACE(testing::Message() << "q = " << q << ", r = " << r);
        TypeParam f(q, r);
        const Array<int> in = fill(f, f.capacity());
        ASSERT_EQ(f.size(), f.capacity());

        for (size_t i = 0; i < in.size(); ++i)
          {
            EXPECT_NO_THROW(f.insert(in(i))) << "item " << i;
            EXPECT_EQ(f.size(), f.capacity()) << "item " << i;
          }

        int other = 0;
        while (f.contains(item(other)))
          ++other;
        EXPECT_THROW(f.insert(item(other)), std::overflow_error);
        EXPECT_EQ(f.size(), f.capacity());
        EXPECT_FALSE(f.contains(item(other)));
        EXPECT_EQ(denied(f, in), 0u);
      }
}

// Two filters with random items, merged, against a third one that takes
// the same items one by one. If they fit, the merged filter has the size
// and the answers of the third; if not, merge() throws and the filter is
// as it was
TYPED_TEST(QuotientFilterLimits, AMergeAgreesWithTheInsertionsOneByOne)
{
  size_t fit = 0, refused = 0;
  for (const int q : {1, 2, 3, 4, 6})
    for (unsigned seed = 1; seed <= 80; ++seed)
      {
        SCOPED_TRACE(testing::Message() << "q = " << q << ", seed " << seed);
        const size_t slots = size_t{1} << q;
        const int universe = static_cast<int>(slots * 4);
        std::mt19937 rng(seed);
        const size_t na = rng() % (slots + 1), nb = rng() % (slots + 1);

        TypeParam a(q, 10), b(q, 10), one_by_one(q, 10);
        Array<int> items;
        for (size_t i = 0; i < na + nb; ++i)
          {
            const int x = static_cast<int>(rng() % universe);
            (i < na ? a : b).insert(x);
            items.append(x);
          }

        bool they_fit = true;
        try
          {
            for (size_t i = 0; i < items.size(); ++i)
              one_by_one.insert(items(i));
          }
        catch (const std::overflow_error &)
          {
            they_fit = false;
          }

        const TypeParam before(a), b_before(b);
        if (they_fit)
          {
            ++fit;
            ASSERT_NO_THROW(a.merge(b));
            EXPECT_EQ(a.size(), one_by_one.size());
            EXPECT_EQ(differences(a, one_by_one, universe), 0u);
          }
        else
          {
            ++refused;
            ASSERT_THROW(a.merge(b), std::overflow_error);
            EXPECT_EQ(a.size(), before.size());
            EXPECT_EQ(differences(a, before, universe), 0u);
          }
        EXPECT_EQ(denied(a, items), they_fit ? 0u : denied(before, items));
        EXPECT_EQ(b.size(), b_before.size());
        EXPECT_EQ(differences(b, b_before, universe), 0u);
      }
  // the two cases are tested
  EXPECT_GT(fit, 100u);
  EXPECT_GT(refused, 100u);
}

// A full filter takes a filter with items that it holds: itself, a copy of
// it and one with some of its items
TYPED_TEST(QuotientFilterLimits, AFullFilterMergesWhatItHolds)
{
  for (const int q : {1, 3, 5})
    {
      SCOPED_TRACE(testing::Message() << "q = " << q);
      TypeParam f(q, 12);
      const Array<int> in = fill(f, f.capacity());
      const TypeParam copy(f);

      TypeParam some(q, 12);
      for (size_t i = 0; i < in.size(); i += 2)
        some.insert(in(i));

      EXPECT_NO_THROW(f.merge(copy));
      EXPECT_NO_THROW(f.merge(some));
      EXPECT_NO_THROW(f.merge(f));
      EXPECT_EQ(f.size(), f.capacity());
      EXPECT_EQ(denied(f, in), 0u);

      // and one item that it does not hold is one too many
      int other = 0;
      while (f.contains(item(other)))
        ++other;
      TypeParam one(q, 12);
      one.insert(item(other));
      EXPECT_THROW(f.merge(one), std::overflow_error);
      EXPECT_EQ(f.size(), f.capacity());
      EXPECT_EQ(denied(f, in), 0u);
      EXPECT_FALSE(f.contains(item(other)));
    }
}

// A filter merged with itself is as it was, with half of its slots taken
// and full
TYPED_TEST(QuotientFilterLimits, AFilterMergedWithItselfIsAsItWas)
{
  for (const int q : {1, 3, 5})
    for (const size_t part : {size_t{2}, size_t{1}})
      {
        SCOPED_TRACE(testing::Message() << "q = " << q << ", 1/" << part << " of the slots");
        TypeParam f(q, 12);
        const Array<int> in = fill(f, f.capacity() / part);
        const TypeParam before(f);

        EXPECT_NO_THROW(f.merge(f));
        EXPECT_EQ(f.size(), before.size());
        EXPECT_EQ(differences(f, before, item(static_cast<int>(2 * f.capacity()))), 0u);
        EXPECT_EQ(denied(f, in), 0u);
      }
}
