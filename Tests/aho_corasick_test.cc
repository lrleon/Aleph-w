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

  EXPECT_EQ(matches.size(), 4u);
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

TEST(AhoCorasick, EmptyPatternRejected)
{
  Aho_Corasick ac;
  EXPECT_THROW((void) ac.add_pattern(""), std::domain_error);
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

  // Build a 10K text with "hay" repeated and a few "needle" insertions
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
  ac.add_pattern("a");
  ac.add_pattern("b");
  ac.build();

  const auto matches = ac.search("abba");
  EXPECT_EQ(matches.size(), 4u);
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
