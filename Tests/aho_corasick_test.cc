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
 * @file aho_corasick_test.cc
 * @brief Tests for Aho_Corasick.H.
 */

# include <gtest/gtest.h>

# include <stdexcept>

# include <Aho_Corasick.H>

using namespace Aleph;

TEST(AhoCorasick, ClassicDictionary)
{
  Aho_Corasick ac;
  const size_t id_he = ac.add_pattern("he");
  const size_t id_she = ac.add_pattern("she");
  const size_t id_his = ac.add_pattern("his");
  const size_t id_hers = ac.add_pattern("hers");

  ac.build();

  const auto matches = ac.search("ahishers");

  ASSERT_EQ(matches.size(), 4u);
  EXPECT_EQ(matches[0], (Aho_Corasick::Match{1, id_his}));
  EXPECT_EQ(matches[1], (Aho_Corasick::Match{3, id_she}));
  EXPECT_EQ(matches[2], (Aho_Corasick::Match{4, id_he}));
  EXPECT_EQ(matches[3], (Aho_Corasick::Match{4, id_hers}));
}

TEST(AhoCorasick, OverlappingPatterns)
{
  Aho_Corasick ac;
  const size_t id_a = ac.add_pattern("a");
  const size_t id_aa = ac.add_pattern("aa");
  const size_t id_aaa = ac.add_pattern("aaa");
  ac.build();

  const auto matches = ac.search("aaaa");

  size_t cnt_a = 0;
  size_t cnt_aa = 0;
  size_t cnt_aaa = 0;

  for (const auto & m : matches)
    {
      if (m.pattern_id == id_a)
        ++cnt_a;
      else if (m.pattern_id == id_aa)
        ++cnt_aa;
      else if (m.pattern_id == id_aaa)
        ++cnt_aaa;
    }

  EXPECT_EQ(cnt_a, 4u);
  EXPECT_EQ(cnt_aa, 3u);
  EXPECT_EQ(cnt_aaa, 2u);
  EXPECT_TRUE(ac.contains_any("xyz aaaa xyz"));
}

TEST(AhoCorasick, BuildRequiredBeforeSearch)
{
  Aho_Corasick ac;
  ac.add_pattern("abc");
  EXPECT_THROW((void) ac.search("abc"), std::runtime_error);
}

TEST(AhoCorasick, IncrementalBuildAfterPreviousBuild)
{
  Aho_Corasick ac;
  const size_t id_he = ac.add_pattern("he");
  ac.build();

  const auto he_only = ac.search("he");
  ASSERT_EQ(he_only.size(), 1u);
  EXPECT_EQ(he_only[0], (Aho_Corasick::Match{0, id_he}));

  const size_t id_she = ac.add_pattern("she");
  ac.build();

  const auto she_matches = ac.search("she");
  ASSERT_EQ(she_matches.size(), 2u);
  EXPECT_EQ(she_matches[0], (Aho_Corasick::Match{0, id_she}));
  EXPECT_EQ(she_matches[1], (Aho_Corasick::Match{1, id_he}));

  // Rebuilding incrementally must not create spurious matches.
  const auto he_after_rebuild = ac.search("he");
  ASSERT_EQ(he_after_rebuild.size(), 1u);
  EXPECT_EQ(he_after_rebuild[0], (Aho_Corasick::Match{0, id_he}));
}

TEST(AhoCorasick, EmptyPatternRejected)
{
  Aho_Corasick ac;
  EXPECT_THROW((void) ac.add_pattern(""), std::invalid_argument);
}

TEST(AhoCorasick, ClearAndRebuild)
{
  Aho_Corasick ac;
  ac.add_pattern("test");
  ac.build();
  EXPECT_TRUE(ac.contains_any("unit test"));

  ac.clear();
  EXPECT_EQ(ac.pattern_count(), 0u);
  EXPECT_FALSE(ac.is_built());

  const size_t id = ac.add_pattern("abc");
  ac.build();

  const auto matches = ac.search("xxabcxx");
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0], (Aho_Corasick::Match{2, id}));
  EXPECT_EQ(ac.pattern(id), "abc");
}

TEST(AhoCorasick, LargePatternSet)
{
  Aho_Corasick ac;

  // Insert 100 patterns: "p000" through "p099"
  Array<size_t> ids;
  for (int i = 0; i < 100; ++i)
    {
      std::string pat = "p";
      if (i < 10)
        pat += "00";
      else
        pat += "0";
      pat += std::to_string(i);
      ids.append(ac.add_pattern(pat));
    }

  ac.build();

  // Build text containing some patterns
  std::string text = "xxp000xxp050xxp099xx";
  const auto matches = ac.search(text);

  EXPECT_GE(matches.size(), 3u);

  // Verify each match points to a valid pattern
  for (size_t i = 0; i < matches.size(); ++i)
    {
      const auto & m = matches[i];
      EXPECT_LT(m.pattern_id, 100u);
      const auto & pat = ac.pattern(m.pattern_id);
      EXPECT_EQ(text.substr(m.position, pat.size()), pat);
    }
}

TEST(AhoCorasick, LongTextSearch)
{
  Aho_Corasick ac;
  ac.add_pattern("needle");
  ac.add_pattern("hay");
  ac.build();

  // Build a 10K text with "stack" repeated and a few "needle" insertions
  std::string text;
  text.reserve(10000);
  for (int i = 0; i < 2000; ++i)
    text += "stack";
  // Insert needles at known positions
  text.replace(100, 6, "needle");
  text.replace(5000, 6, "needle");

  const auto matches = ac.search(text);
  EXPECT_GE(matches.size(), 2u);

  size_t needle_count = 0;
  for (size_t i = 0; i < matches.size(); ++i)
    if (ac.pattern(matches[i].pattern_id) == "needle")
      ++needle_count;
  EXPECT_EQ(needle_count, 2u);
}

TEST(AhoCorasick, SingleCharPatterns)
{
  Aho_Corasick ac;
  const size_t id_a = ac.add_pattern("a");
  const size_t id_b = ac.add_pattern("b");
  ac.build();

  const auto matches = ac.search("abba");
  ASSERT_EQ(matches.size(), 4u);
  EXPECT_EQ(matches[0], (Aho_Corasick::Match{0, id_a}));
  EXPECT_EQ(matches[1], (Aho_Corasick::Match{1, id_b}));
  EXPECT_EQ(matches[2], (Aho_Corasick::Match{2, id_b}));
  EXPECT_EQ(matches[3], (Aho_Corasick::Match{3, id_a}));
}

TEST(AhoCorasick, BinaryPatterns)
{
  Aho_Corasick ac;
  std::string pat;
  pat.push_back(static_cast<char>(0x00));
  pat.push_back(static_cast<char>(0xFF));
  ac.add_pattern(pat);
  ac.build();

  std::string text = "abc";
  text.push_back(static_cast<char>(0x00));
  text.push_back(static_cast<char>(0xFF));
  text += "xyz";

  const auto matches = ac.search(text);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0].position, 3u);
}

TEST(AhoCorasick, EmptyText)
{
  Aho_Corasick ac;
  ac.add_pattern("he");
  ac.add_pattern("she");
  ac.build();

  const auto matches = ac.search("");
  EXPECT_TRUE(matches.is_empty());
}

TEST(AhoCorasick, NoMatchesFound)
{
  Aho_Corasick ac;
  ac.add_pattern("he");
  ac.add_pattern("she");
  ac.build();

  const auto matches = ac.search("xyzxyz");
  EXPECT_TRUE(matches.is_empty());
}

// --- 3.3 Aho-Corasick with patterns that are prefixes of each other ---

TEST(AhoCorasick, PrefixChainPatterns)
{
  // Patterns {"a", "ab", "abc"} where each is a prefix of the next.
  // Exercises output link propagation through the trie.
  Aho_Corasick ac;
  const size_t id_a = ac.add_pattern("a");
  const size_t id_ab = ac.add_pattern("ab");
  const size_t id_abc = ac.add_pattern("abc");
  ac.build();

  const auto matches = ac.search("abcabc");

  // Count matches per pattern
  size_t cnt_a = 0;
  size_t cnt_ab = 0;
  size_t cnt_abc = 0;

  for (size_t i = 0; i < matches.size(); ++i)
    {
      if (matches[i].pattern_id == id_a)
        ++cnt_a;
      else if (matches[i].pattern_id == id_ab)
        ++cnt_ab;
      else if (matches[i].pattern_id == id_abc)
        ++cnt_abc;
    }

  // "a" at positions 0, 3
  EXPECT_EQ(cnt_a, 2u);
  // "ab" at positions 0, 3
  EXPECT_EQ(cnt_ab, 2u);
  // "abc" at positions 0, 3
  EXPECT_EQ(cnt_abc, 2u);

  EXPECT_EQ(matches.size(), 6u);
}

TEST(AhoCorasick, PrefixChainWithSuffixOverlap)
{
  // Patterns {"a", "ab", "abc", "bc", "c"} — mixture of prefixes and
  // suffix-overlapping patterns to exercise output links.
  Aho_Corasick ac;
  const size_t id_a = ac.add_pattern("a");
  const size_t id_ab = ac.add_pattern("ab");
  const size_t id_abc = ac.add_pattern("abc");
  const size_t id_bc = ac.add_pattern("bc");
  const size_t id_c = ac.add_pattern("c");
  ac.build();

  const auto matches = ac.search("abc");

  size_t cnt_a = 0;
  size_t cnt_ab = 0;
  size_t cnt_abc = 0;
  size_t cnt_bc = 0;
  size_t cnt_c = 0;

  for (size_t i = 0; i < matches.size(); ++i)
    {
      if (matches[i].pattern_id == id_a) ++cnt_a;
      else if (matches[i].pattern_id == id_ab) ++cnt_ab;
      else if (matches[i].pattern_id == id_abc) ++cnt_abc;
      else if (matches[i].pattern_id == id_bc) ++cnt_bc;
      else if (matches[i].pattern_id == id_c) ++cnt_c;
    }

  EXPECT_EQ(cnt_a, 1u);
  EXPECT_EQ(cnt_ab, 1u);
  EXPECT_EQ(cnt_abc, 1u);
  EXPECT_EQ(cnt_bc, 1u);
  EXPECT_EQ(cnt_c, 1u);
  EXPECT_EQ(matches.size(), 5u);
}

TEST(AhoCorasick, LongPrefixChain)
{
  // Patterns "x", "xx", "xxx", ..., "xxxxxxxxxx" (1..10 x's)
  Aho_Corasick ac;
  Array<size_t> ids;
  for (int len = 1; len <= 10; ++len)
    ids.append(ac.add_pattern(std::string(static_cast<size_t>(len), 'x')));
  ac.build();

  // Text is 15 x's
  const std::string text(15, 'x');
  const auto matches = ac.search(text);

  // Count occurrences of each pattern length
  Array<size_t> counts = Array<size_t>::create(10);
  for (size_t i = 0; i < 10; ++i)
    counts[i] = 0;

  for (size_t i = 0; i < matches.size(); ++i)
    ++counts[matches[i].pattern_id];

  // Pattern of length k appears (15 - k + 1) times
  for (int k = 1; k <= 10; ++k)
    EXPECT_EQ(counts[static_cast<size_t>(k - 1)],
              static_cast<size_t>(15 - k + 1))
      << "pattern length " << k;
}

TEST(AhoCorasick, PrefixPatternsMatchPositions)
{
  // Verify exact match positions for prefix patterns
  Aho_Corasick ac;
  const size_t id_a = ac.add_pattern("a");
  const size_t id_ab = ac.add_pattern("ab");
  const size_t id_abc = ac.add_pattern("abc");
  ac.build();

  const auto matches = ac.search("xabcx");

  Array<size_t> pos_a;
  Array<size_t> pos_ab;
  Array<size_t> pos_abc;

  for (size_t i = 0; i < matches.size(); ++i)
    {
      if (matches[i].pattern_id == id_a)
        pos_a.append(matches[i].position);
      else if (matches[i].pattern_id == id_ab)
        pos_ab.append(matches[i].position);
      else if (matches[i].pattern_id == id_abc)
        pos_abc.append(matches[i].position);
    }

  ASSERT_EQ(pos_a.size(), 1u);
  EXPECT_EQ(pos_a[0], 1u);

  ASSERT_EQ(pos_ab.size(), 1u);
  EXPECT_EQ(pos_ab[0], 1u);

  ASSERT_EQ(pos_abc.size(), 1u);
  EXPECT_EQ(pos_abc[0], 1u);
}

TEST(AhoCorasick, DeterministicStress)
{
  Aho_Corasick ac;
  const int num_patterns = 1000;
  Array<std::string> patterns;
  
  // Deterministic pattern generation
  for (int i = 0; i < num_patterns; ++i)
    {
      std::string p = "pat" + std::to_string(i);
      patterns.append(p);
      ac.add_pattern(p);
    }
  
  ac.build();
  
  // Deterministic text generation
  std::string text;
  text.reserve(200000);
  for (int i = 0; i < 5000; ++i)
    {
      text += "some_random_filler_";
      if (i % 10 == 0)
        text += patterns[i % num_patterns];
    }
  
  const auto matches = ac.search(text);
  EXPECT_GT(matches.size(), 0u);
  
  for (const auto & m : matches)
    {
      const auto & pat = ac.pattern(m.pattern_id);
      EXPECT_EQ(text.substr(m.position, pat.size()), pat);
    }
}
