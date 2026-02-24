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
 * @file suffix_structures_test.cc
 * @brief Tests for Suffix_Structures.H.
 */

# include <gtest/gtest.h>

# include <stdexcept>

# include <Suffix_Structures.H>

using namespace Aleph;

namespace
{
  void expect_array_eq(const Array<size_t> & a,
                       std::initializer_list<size_t> expected)
  {
    ASSERT_EQ(a.size(), expected.size());
    size_t i = 0;
    for (const size_t value : expected)
      EXPECT_EQ(a[i++], value);
  }
}

TEST(SuffixStructures, SuffixArrayBanana)
{
  const auto sa = suffix_array("banana");
  expect_array_eq(sa, {5, 3, 1, 0, 4, 2});
}

TEST(SuffixStructures, LcpKasaiBanana)
{
  const auto sa = suffix_array("banana");
  const auto lcp = lcp_array_kasai("banana", sa);
  expect_array_eq(lcp, {1, 3, 0, 0, 2, 0});
}

TEST(SuffixStructures, LcpKasaiRejectsInvalidInput)
{
  Array<size_t> bad;
  bad.append(0);
  EXPECT_THROW((void) lcp_array_kasai("banana", bad), std::domain_error);
}

TEST(SuffixStructures, NaiveSuffixTreeContainsAndFindAll)
{
  Naive_Suffix_Tree st("banana");

  EXPECT_TRUE(st.contains("ana"));
  EXPECT_TRUE(st.contains("nana"));
  EXPECT_FALSE(st.contains("apple"));

  expect_array_eq(st.find_all("ana"), {1, 3});
  expect_array_eq(st.find_all("nana"), {2});
  EXPECT_TRUE(st.find_all("apple").is_empty());
  expect_array_eq(st.find_all(""), {0, 1, 2, 3, 4, 5, 6});
}

TEST(SuffixStructures, SuffixAutomatonContainsAndDistinctCount)
{
  Suffix_Automaton sam;
  sam.build("ababa");

  EXPECT_TRUE(sam.contains("aba"));
  EXPECT_TRUE(sam.contains("bab"));
  EXPECT_FALSE(sam.contains("abc"));

  EXPECT_EQ(sam.distinct_substring_count(), 9u);
  EXPECT_LE(sam.state_count(), 2u * 5u);
}

TEST(SuffixStructures, SuffixAutomatonLongestCommonSubstring)
{
  Suffix_Automaton sam;
  sam.build("xabxac");

  EXPECT_EQ(sam.longest_common_substring("abcabxabcd"), "abxa");
  EXPECT_EQ(longest_common_substring_sam("xabxac", "abcabxabcd"), "abxa");
}

TEST(SuffixStructures, SuffixArrayLongerText)
{
  const std::string text = "mississippi";
  const auto sa = suffix_array(text);

  ASSERT_EQ(sa.size(), text.size());

  // Verify lexicographic ordering of suffixes
  for (size_t i = 1; i < sa.size(); ++i)
    {
      const std::string_view prev_suffix(text.data() + sa[i - 1],
                                          text.size() - sa[i - 1]);
      const std::string_view curr_suffix(text.data() + sa[i],
                                          text.size() - sa[i]);
      EXPECT_LT(prev_suffix, curr_suffix);
    }
}

TEST(SuffixStructures, SuffixArraySingleChar)
{
  const auto sa = suffix_array("a");
  expect_array_eq(sa, {0});
}

TEST(SuffixStructures, SuffixArrayEmpty)
{
  const auto sa = suffix_array("");
  EXPECT_TRUE(sa.is_empty());
}

TEST(SuffixStructures, SuffixTreeRepeatedPattern)
{
  Naive_Suffix_Tree st("abcabcabc");

  EXPECT_TRUE(st.contains("abc"));
  EXPECT_TRUE(st.contains("cab"));
  EXPECT_TRUE(st.contains("bcab"));
  EXPECT_FALSE(st.contains("abcd"));

  const auto matches = st.find_all("abc");
  ASSERT_EQ(matches.size(), 3u);
  expect_array_eq(matches, {0, 3, 6});
}

TEST(SuffixStructures, SuffixTreeSingleChar)
{
  Naive_Suffix_Tree st("x");
  EXPECT_TRUE(st.contains("x"));
  EXPECT_FALSE(st.contains("y"));
  expect_array_eq(st.find_all("x"), {0});
}

TEST(SuffixStructures, SuffixAutomatonEmptyAndSingleChar)
{
  Suffix_Automaton sam;
  sam.build("");
  EXPECT_EQ(sam.distinct_substring_count(), 0u);
  EXPECT_TRUE(sam.contains(""));
  EXPECT_FALSE(sam.contains("a"));

  sam.build("z");
  EXPECT_EQ(sam.distinct_substring_count(), 1u);
  EXPECT_TRUE(sam.contains("z"));
  EXPECT_FALSE(sam.contains("a"));
}

TEST(SuffixStructures, SuffixAutomatonLcsNoCommon)
{
  EXPECT_EQ(longest_common_substring_sam("abc", "xyz"), "");
}

TEST(SuffixStructures, StressLongerSuffixArray)
{
  // Build a 5000-char string and verify SA ordering
  std::string text;
  text.reserve(5000);
  for (int i = 0; i < 5000; ++i)
    text.push_back('a' + (i * 7 + 3) % 26);

  const auto sa = suffix_array(text);
  ASSERT_EQ(sa.size(), text.size());

  for (size_t i = 1; i < sa.size(); ++i)
    {
      const std::string_view prev_suffix(text.data() + sa[i - 1],
                                          text.size() - sa[i - 1]);
      const std::string_view curr_suffix(text.data() + sa[i],
                                          text.size() - sa[i]);
      EXPECT_LE(prev_suffix, curr_suffix);
    }
}

TEST(SuffixStructures, BinaryContentSuffixAutomaton)
{
  std::string text;
  for (int i = 0; i < 256; ++i)
    text.push_back(static_cast<char>(i));

  Suffix_Automaton sam;
  sam.build(text);

  // Every single byte should be a substring
  for (int i = 0; i < 256; ++i)
    {
      std::string pat(1, static_cast<char>(i));
      EXPECT_TRUE(sam.contains(pat));
    }

  EXPECT_FALSE(sam.contains(text + "x"));
}
