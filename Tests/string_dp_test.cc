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
 * @file string_dp_test.cc
 * @brief Tests for String_DP.H.
 */

# include <gtest/gtest.h>

# include <String_DP.H>

using namespace Aleph;

TEST(StringDP, LevenshteinDistanceClassic)
{
  EXPECT_EQ(levenshtein_distance("kitten", "sitting"), 3u);
  EXPECT_EQ(edit_distance("kitten", "sitting"), 3u);
  EXPECT_EQ(levenshtein_distance("", "abc"), 3u);
  EXPECT_EQ(levenshtein_distance("abc", ""), 3u);
  EXPECT_EQ(levenshtein_distance("abc", "abc"), 0u);
}

TEST(StringDP, DamerauLevenshteinDistance)
{
  EXPECT_EQ(damerau_levenshtein_distance("ca", "ac"), 1u);
  EXPECT_EQ(damerau_levenshtein_distance("abc", "acb"), 1u);
  EXPECT_EQ(damerau_levenshtein_distance("abcd", "acbd"), 1u);
}

TEST(StringDP, LongestCommonSubsequence)
{
  const auto r = longest_common_subsequence("AGGTAB", "GXTXAYB");
  EXPECT_EQ(r.length, 4u);
  EXPECT_EQ(r.subsequence, "GTAB");
}

TEST(StringDP, LongestCommonSubstring)
{
  const auto r = longest_common_substring("xabxac", "abcabxabcd");
  EXPECT_EQ(r.length, 4u);
  EXPECT_EQ(r.substring, "abxa");
  EXPECT_EQ(r.begin_a, 1u);
  EXPECT_EQ(r.begin_b, 3u);
}

TEST(StringDP, LongestCommonSubstringNoMatch)
{
  const auto r = longest_common_substring("abc", "XYZ");
  EXPECT_EQ(r.length, 0u);
  EXPECT_EQ(r.substring, "");
}

TEST(StringDP, LevenshteinSingleChar)
{
  EXPECT_EQ(levenshtein_distance("a", "a"), 0u);
  EXPECT_EQ(levenshtein_distance("a", "b"), 1u);
  EXPECT_EQ(levenshtein_distance("a", ""), 1u);
  EXPECT_EQ(levenshtein_distance("", "a"), 1u);
}

TEST(StringDP, LevenshteinDistanceOne)
{
  // Insertion
  EXPECT_EQ(levenshtein_distance("abc", "abxc"), 1u);
  // Deletion
  EXPECT_EQ(levenshtein_distance("abcd", "abd"), 1u);
  // Substitution
  EXPECT_EQ(levenshtein_distance("abc", "axc"), 1u);
}

TEST(StringDP, LevenshteinIdentical)
{
  EXPECT_EQ(levenshtein_distance("hello world", "hello world"), 0u);
}

TEST(StringDP, DamerauLevenshteinSingleChar)
{
  EXPECT_EQ(damerau_levenshtein_distance("a", "a"), 0u);
  EXPECT_EQ(damerau_levenshtein_distance("a", "b"), 1u);
  EXPECT_EQ(damerau_levenshtein_distance("a", ""), 1u);
  EXPECT_EQ(damerau_levenshtein_distance("", "a"), 1u);
}

TEST(StringDP, DamerauLevenshteinEmpty)
{
  EXPECT_EQ(damerau_levenshtein_distance("", ""), 0u);
}

TEST(StringDP, DamerauLevenshteinVsLevenshtein)
{
  // Transposition: DL = 1, Lev = 2
  EXPECT_EQ(damerau_levenshtein_distance("ab", "ba"), 1u);
  EXPECT_EQ(levenshtein_distance("ab", "ba"), 2u);
}

TEST(StringDP, LcsEmptyStrings)
{
  const auto r1 = longest_common_subsequence("", "abc");
  EXPECT_EQ(r1.length, 0u);
  EXPECT_EQ(r1.subsequence, "");

  const auto r2 = longest_common_subsequence("abc", "");
  EXPECT_EQ(r2.length, 0u);
  EXPECT_EQ(r2.subsequence, "");
}

TEST(StringDP, LcsSingleChar)
{
  const auto r = longest_common_subsequence("a", "a");
  EXPECT_EQ(r.length, 1u);
  EXPECT_EQ(r.subsequence, "a");
}

TEST(StringDP, LongestCommonSubstringSingleChar)
{
  const auto r = longest_common_substring("a", "a");
  EXPECT_EQ(r.length, 1u);
  EXPECT_EQ(r.substring, "a");
}

TEST(StringDP, StressLongerEditDistance)
{
  // Two strings of length 100 that differ in exactly 10 positions
  std::string a(100, 'a');
  std::string b = a;
  for (int i = 0; i < 10; ++i)
    b[i * 10] = 'b';

  EXPECT_EQ(levenshtein_distance(a, b), 10u);
}
