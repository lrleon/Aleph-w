
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
 * @file quotient_filter_removal_test.cc
 * @brief A quotient filter keeps every item that it holds through the
 *        removals of the others, in each of the states that it may be in
 */

// remove() of the two quotient filters shifted the rest of the cluster one
// slot to the left and marked as not shifted a run start that landed on a
// slot whose occupied bit was set, which may be the bit of a later run: the
// filter then denied items that it held, where it promises no false
// negatives, or looped for ever. These tests remove items and ask for every
// other one, against the exact set of the items in.

#include <gtest/gtest.h>

#include <random>
#include <utility>

#include <compact-quotient-filter.H>
#include <quotient-filter.H>
#include <tpl_array.H>

using namespace Aleph;

namespace
{
  template <class Filter>
  class QuotientFilterRemoval : public ::testing::Test
  {};

  using Filters = ::testing::Types<Quotient_Filter<int>, Compact_Quotient_Filter<int>>;
  TYPED_TEST_SUITE(QuotientFilterRemoval, Filters);

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

  // How many of the items of `in`, from the one at `from` on, the filter
  // denies
  template <class Filter>
  size_t denied(const Filter & f, const Array<int> & in, const size_t from = 0)
  {
    size_t n = 0;
    for (size_t i = from; i < in.size(); ++i)
      if (not f.contains(in(i)))
        ++n;
    return n;
  }

  // Fills a filter, whatever it holds, with as many items as it has slots,
  // asks for them all, removes them one by one and clears it: every slot is
  // read and written
  template <class Filter>
  void expect_that_every_slot_serves(Filter & f, const char * state)
  {
    f.clear();
    const Array<int> in = fill(f, f.capacity());
    EXPECT_EQ(f.size(), f.capacity()) << state;
    EXPECT_EQ(denied(f, in), 0u) << state;
    for (size_t i = 0; i < in.size(); ++i)
      {
        EXPECT_TRUE(f.remove(in(i))) << state << ", removal " << i;
        EXPECT_EQ(denied(f, in, i + 1), 0u) << state << ", removal " << i;
      }
    EXPECT_TRUE(f.is_empty()) << state;
    f.insert(item(0));
    f.clear();
    EXPECT_TRUE(f.is_empty()) << state;
    EXPECT_FALSE(f.contains(item(0))) << state;
  }
}

// Every count of items in filters of 2 to 128 slots, removed in the order
// in which they went in. The smallest case that failed: 16 slots and 13
// items, whose second removal lost an item
TYPED_TEST(QuotientFilterRemoval, ARemovalKeepsEveryOtherItem)
{
  for (int q = 1; q <= 7; ++q)
    for (size_t n = 1; n <= (size_t{1} << q); ++n)
      {
        TypeParam f(q, 5);
        const Array<int> in = fill(f, n);
        ASSERT_EQ(denied(f, in), 0u) << "q = " << q << ", " << n << " items";

        for (size_t i = 0; i < in.size(); ++i)
          {
            ASSERT_TRUE(f.remove(in(i)))
              << "q = " << q << ", " << n << " items, removal " << i;
            ASSERT_EQ(f.size(), in.size() - i - 1)
              << "q = " << q << ", " << n << " items, removal " << i;
            ASSERT_EQ(denied(f, in, i + 1), 0u)
              << "q = " << q << ", " << n << " items, removal " << i;
          }
        EXPECT_TRUE(f.is_empty());
      }
}

// Random insertions and removals against the exact set of the items in:
// after each one, the filter holds as many items as the set and denies
// none of them
TYPED_TEST(QuotientFilterRemoval, RandomRemovalsAgreeWithTheExactSet)
{
  for (const int q : {2, 3, 4, 6, 8})
    for (const int r : {1, 3, 8, 20})
      for (unsigned seed = 1; seed <= 8; ++seed)
        {
          TypeParam f(q, r);
          Array<int> in;
          std::mt19937 rng(seed);
          int next = 0;
          for (int op = 0; op < 300; ++op)
            {
              const bool room = f.size() < f.capacity();
              if (in.is_empty() or (room and rng() % 3 != 0))
                {
                  if (not room)
                    continue;
                  const int k = item(next++);
                  const size_t before = f.size();
                  f.insert(k);
                  if (f.size() > before)
                    in.append(k);
                }
              else
                {
                  const size_t i = rng() % in.size();
                  ASSERT_TRUE(f.remove(in(i)))
                    << "q = " << q << ", r = " << r << ", seed " << seed << ", operation " << op;
                  in(i) = in(in.size() - 1);
                  (void) in.remove_last();
                }
              ASSERT_EQ(f.size(), in.size())
                << "q = " << q << ", r = " << r << ", seed " << seed << ", operation " << op;
              ASSERT_EQ(denied(f, in), 0u)
                << "q = " << q << ", r = " << r << ", seed " << seed << ", operation " << op;
            }
        }
}

// The filters read and write their slots without a check of the range,
// which the constructors guarantee by reserving them all: every slot of a
// filter serves in each of the states that it may come to be in, with
// remainders from one bit to as many as the filter takes
TYPED_TEST(QuotientFilterRemoval, EverySlotServesInEachStateOfAFilter)
{
  for (const int q : {1, 2, 4, 7})
    for (const int r : {1, 5, 31, 57})
      {
        SCOPED_TRACE(testing::Message() << "q = " << q << ", r = " << r);

        TypeParam built(q, r);
        expect_that_every_slot_serves(built, "built");

        built.insert(item(1));
        TypeParam copied(built);
        expect_that_every_slot_serves(copied, "copied");

        TypeParam assigned(1, r);
        assigned = built;
        expect_that_every_slot_serves(assigned, "copy assigned");

        TypeParam moved(std::move(built));
        expect_that_every_slot_serves(moved, "moved to");
        expect_that_every_slot_serves(built, "moved from");

        TypeParam move_assigned(1, r);
        move_assigned = std::move(moved);
        expect_that_every_slot_serves(move_assigned, "move assigned");
        expect_that_every_slot_serves(moved, "moved from by an assignment");

        TypeParam a(q, r), b(1, r);
        a.swap(b);
        expect_that_every_slot_serves(a, "swapped, the small one");
        expect_that_every_slot_serves(b, "swapped, the large one");
      }
}
