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
 * @file string_algorithms_test.cc
 * @brief Integration tests for String_Algorithms.H umbrella header.
 */

# include <gtest/gtest.h>

# include <String_Algorithms.H>

using namespace Aleph;

TEST(StringAlgorithmsUmbrella, BasicInteroperability)
{
  EXPECT_EQ(kmp_search("abracadabra", "abra").size(), 2u);

  Aho_Corasick ac;
  ac.add_pattern("abra");
  ac.build();
  EXPECT_TRUE(ac.contains_any("abracadabra"));

  EXPECT_EQ(suffix_array("banana").size(), 6u);
  EXPECT_EQ(longest_palindromic_substring("abacaba"), "abacaba");
  EXPECT_EQ(edit_distance("kitten", "sitting"), 3u);
}

TEST(StringAlgorithmsUmbrella, SearchAlgorithmsAgreeOnSameInput)
{
  const std::string text = "the quick brown fox jumps over the lazy dog fox";
  const std::string pattern = "fox";

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);
  const auto bmh = boyer_moore_horspool_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  ASSERT_EQ(kmp.size(), z.size());
  ASSERT_EQ(kmp.size(), bmh.size());
  ASSERT_EQ(kmp.size(), rk.size());

  for (size_t i = 0; i < kmp.size(); ++i)
    {
      EXPECT_EQ(kmp[i], z[i]);
      EXPECT_EQ(kmp[i], bmh[i]);
      EXPECT_EQ(kmp[i], rk[i]);
    }
}

TEST(StringAlgorithmsUmbrella, AhoCorasickMatchesKmpForSinglePattern)
{
  const std::string text = "ababababab";
  const std::string pattern = "abab";

  const auto kmp_matches = kmp_search(text, pattern);

  Aho_Corasick ac;
  ac.add_pattern(pattern);
  ac.build();
  const auto ac_matches = ac.search(text);

  // Same number of matches
  ASSERT_EQ(ac_matches.size(), kmp_matches.size());

  // Same positions
  for (size_t i = 0; i < ac_matches.size(); ++i)
    EXPECT_EQ(ac_matches[i].position, kmp_matches[i]);
}

TEST(StringAlgorithmsUmbrella, SuffixTreeMatchesKmpForSearch)
{
  const std::string text = "abracadabra";
  const std::string pattern = "abra";

  const auto kmp_matches = kmp_search(text, pattern);

  Naive_Suffix_Tree st(text);
  auto tree_matches = st.find_all(pattern);

  ASSERT_EQ(tree_matches.size(), kmp_matches.size());

  for (size_t i = 0; i < tree_matches.size(); ++i)
    EXPECT_EQ(tree_matches[i], kmp_matches[i]);
}

TEST(StringAlgorithmsUmbrella, SamContainsAgreesWithSuffixTree)
{
  const std::string text = "mississippi";

  Naive_Suffix_Tree st(text);
  Suffix_Automaton sam;
  sam.build(text);

  const char * patterns[] = {"issi", "miss", "pi", "ppi", "xyz", "sip", ""};
  for (const char * p : patterns)
    EXPECT_EQ(st.contains(p), sam.contains(p))
      << "Disagreement on pattern: \"" << p << "\"";
}

TEST(StringAlgorithmsUmbrella, LcsVsSamLcs)
{
  const std::string a = "xabxac";
  const std::string b = "abcabxabcd";

  const auto dp_result = longest_common_substring(a, b);
  const auto sam_result = longest_common_substring_sam(a, b);

  // Both should find the same length
  EXPECT_EQ(dp_result.length, sam_result.size());
  EXPECT_EQ(dp_result.substring, sam_result);
}
