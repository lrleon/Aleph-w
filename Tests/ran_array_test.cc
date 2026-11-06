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
 * @file ran_array_test.cc
 * @brief `ran_array.h`, deprecated, gives the numbers of Knuth's generator
 * until it goes in 8.0.0.
 */

#include <gtest/gtest.h>

#include <vector>

// ran_array.h is deprecated, but supported until 8.0.0; this test is its
// coverage.
#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <ran_array.h>

// The check of rng.c: after ran_start(310952), the first number of the 2010th
// batch of 1009 is 995235265. ran_arr_next() gives the first 100 numbers of
// each batch, so that number is its draw 200900, counted from 0
TEST(RanArray, GivesTheNumberOfTheCheckOfKnuth)
{
  ran_start(310952L);
  for (long i = 0; i < 200900; ++i)
    static_cast<void>(ran_arr_next());
  EXPECT_EQ(ran_arr_next(), 995235265L);
}

// The same seed gives the same numbers, in [0, 2^30)
TEST(RanArray, TheSameSeedGivesTheSameNumbers)
{
  ran_start(4357L);
  std::vector<long> first;
  for (int i = 0; i < 1000; ++i)
    first.push_back(ran_arr_next());

  ran_start(4357L);
  for (const long x : first)
    {
      EXPECT_GE(x, 0L);
      EXPECT_LT(x, 1L << 30);
      EXPECT_EQ(ran_arr_next(), x);
    }
}

#if defined(_MSC_VER) && !defined(__clang__)
#  pragma warning(pop)
#else
#  pragma GCC diagnostic pop
#endif
