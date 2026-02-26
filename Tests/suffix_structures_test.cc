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
# include <chrono>

# include <Suffix_Structures.H>
# include "test_helpers.H"

using namespace Aleph;
using namespace Aleph::test_helpers;

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

TEST(SuffixStructures, LcpKasaiRejectsDuplicates)
{
  // Suffix array for "abc" is {0, 1, 2}.
  // We provide a malformed SA {0, 1, 1} which has a duplicate.
  Array<size_t> sa = build_array<size_t>(0, 1, 1);
  EXPECT_THROW((void) lcp_array_kasai("abc", sa), std::out_of_range);
}

TEST(SuffixStructures, LcpKasaiRejectsOutOfRange)
{
  // Suffix array for "abc" is {0, 1, 2}.
  // We provide a malformed SA {0, 1, 3} where 3 is out of range.
  Array<size_t> sa = build_array<size_t>(0, 1, 3);
  EXPECT_THROW((void) lcp_array_kasai("abc", sa), std::out_of_range);
}

TEST(SuffixStructures, NaiveSuffixTreeContainsAndFindAll)
{
  Naive_Suffix_Tree st("banana");

  EXPECT_EQ(st.text_size(), 6u);
  EXPECT_EQ(st.node_count(), 11u);
  EXPECT_EQ(st.nodes().size(), st.node_count());

  EXPECT_TRUE(st.contains("ana"));
  EXPECT_TRUE(st.contains("nana"));
  EXPECT_FALSE(st.contains("apple"));

  expect_array_eq(st.find_all("ana"), {1, 3});
  expect_array_eq(st.find_all("nana"), {2});
  EXPECT_TRUE(st.find_all("apple").is_empty());
  expect_array_eq(st.find_all(""), {0, 1, 2, 3, 4, 5, 6});
}

TEST(SuffixStructures, NaiveSuffixTreeSentinelExhaustion)
{
  // Construct a string containing all 256 byte values
  std::string full_alphabet;
  for (int i = 0; i < 256; ++i)
    full_alphabet.push_back(static_cast<char>(i));

  // Should throw domain_error because no unique sentinel is available
  EXPECT_THROW(Naive_Suffix_Tree st(full_alphabet), std::domain_error);
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

  const auto start = std::chrono::steady_clock::now();
  const auto sa = suffix_array(text);
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // Performance regression check: O(n log n) construction for n=5000 is fast.
  // We only fail the test if ENABLE_PERF_TESTS is set, otherwise we just log.
  if (std::getenv("ENABLE_PERF_TESTS"))
    EXPECT_LT(elapsed, 500) << "Performance regression: suffix_array(5000) took " << elapsed << "ms";
  else if (elapsed >= 500)
    std::cout << "[ PERF    ] Warning: suffix_array(5000) took " << elapsed << "ms\n";

  ASSERT_EQ(sa.size(), text.size());

  for (size_t i = 1; i < sa.size(); ++i)
    {
      const std::string_view prev_suffix(text.data() + sa[i - 1],
                                          text.size() - sa[i - 1]);
      const std::string_view curr_suffix(text.data() + sa[i],
                                          text.size() - sa[i]);
      EXPECT_LT(prev_suffix, curr_suffix);
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

TEST(SuffixStructures, SuffixAutomatonCloneLinkInvariant)
{
  // Regression: cloning a state could produce a non-root state with
  // link == -1, causing out-of-range access in distinct_substring_count()
  // and longest_common_substring().
  Suffix_Automaton sam;
  sam.build("bcaaaccaaacccaacca");

  // Verify invariant: only state 0 may have link == -1
  const auto & states = sam.states();
  for (size_t i = 1; i < states.size(); ++i)
    EXPECT_GE(states[i].link, 0) << "state " << i << " has link == -1";

  // These must not crash
  EXPECT_GT(sam.distinct_substring_count(), 0u);
  EXPECT_EQ(sam.longest_common_substring("aacca"), "aacca");
  EXPECT_TRUE(sam.contains("caaaccaaaccc"));
}
