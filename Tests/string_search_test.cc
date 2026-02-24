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
 * @file string_search_test.cc
 * @brief Tests for String_Search.H.
 */

# include <gtest/gtest.h>

# include <String_Search.H>

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

TEST(StringSearch, KmpPrefixFunction)
{
  const auto pi = kmp_prefix_function("aabaaab");
  expect_array_eq(pi, {0, 1, 0, 1, 2, 2, 3});
}

TEST(StringSearch, KmpSearchOverlapping)
{
  const auto matches = kmp_search("ababa", "aba");
  expect_array_eq(matches, {0, 2});
}

TEST(StringSearch, KmpEmptyPattern)
{
  const auto matches = kmp_search("abc", "");
  expect_array_eq(matches, {0, 1, 2, 3});
}

TEST(StringSearch, ZAlgorithmBasic)
{
  const auto z = z_algorithm("aaaaa");
  expect_array_eq(z, {5, 4, 3, 2, 1});
}

TEST(StringSearch, ZSearchMatchesKmp)
{
  const std::string text = "abcababcababcab";
  const std::string pattern = "abcab";

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);

  ASSERT_EQ(z.size(), kmp.size());
  for (size_t i = 0; i < z.size(); ++i)
    EXPECT_EQ(z[i], kmp[i]);
}

TEST(StringSearch, HorspoolMatchesKmp)
{
  const std::string text = "abracadabra abracadabra";
  const std::string pattern = "abra";

  const auto expected = kmp_search(text, pattern);
  const auto got = boyer_moore_horspool_search(text, pattern);

  ASSERT_EQ(got.size(), expected.size());
  for (size_t i = 0; i < got.size(); ++i)
    EXPECT_EQ(got[i], expected[i]);
}

TEST(StringSearch, RabinKarpMatchesKmp)
{
  const std::string text = "the quick brown fox jumps over the quick brown fox";
  const std::string pattern = "quick";

  const auto expected = kmp_search(text, pattern);
  const auto got = rabin_karp_search(text, pattern);

  ASSERT_EQ(got.size(), expected.size());
  for (size_t i = 0; i < got.size(); ++i)
    EXPECT_EQ(got[i], expected[i]);
}

TEST(StringSearch, NoMatchCase)
{
  const auto a = kmp_search("abcdefgh", "xyz");
  const auto b = z_search("abcdefgh", "xyz");
  const auto c = boyer_moore_horspool_search("abcdefgh", "xyz");
  const auto d = rabin_karp_search("abcdefgh", "xyz");

  EXPECT_TRUE(a.is_empty());
  EXPECT_TRUE(b.is_empty());
  EXPECT_TRUE(c.is_empty());
  EXPECT_TRUE(d.is_empty());
}

TEST(StringSearch, SingleCharPatternAndText)
{
  const auto a = kmp_search("a", "a");
  const auto b = z_search("a", "a");
  const auto c = boyer_moore_horspool_search("a", "a");
  const auto d = rabin_karp_search("a", "a");

  expect_array_eq(a, {0});
  expect_array_eq(b, {0});
  expect_array_eq(c, {0});
  expect_array_eq(d, {0});
}

TEST(StringSearch, SingleCharPatternNoMatch)
{
  EXPECT_TRUE(kmp_search("b", "a").is_empty());
  EXPECT_TRUE(z_search("b", "a").is_empty());
  EXPECT_TRUE(boyer_moore_horspool_search("b", "a").is_empty());
  EXPECT_TRUE(rabin_karp_search("b", "a").is_empty());
}

TEST(StringSearch, EmptyTextNonEmptyPattern)
{
  EXPECT_TRUE(kmp_search("", "abc").is_empty());
  EXPECT_TRUE(z_search("", "abc").is_empty());
  EXPECT_TRUE(boyer_moore_horspool_search("", "abc").is_empty());
  EXPECT_TRUE(rabin_karp_search("", "abc").is_empty());
}

TEST(StringSearch, PatternLongerThanText)
{
  EXPECT_TRUE(kmp_search("ab", "abcdef").is_empty());
  EXPECT_TRUE(z_search("ab", "abcdef").is_empty());
  EXPECT_TRUE(boyer_moore_horspool_search("ab", "abcdef").is_empty());
  EXPECT_TRUE(rabin_karp_search("ab", "abcdef").is_empty());
}

TEST(StringSearch, StressLongRandomStringAllAlgorithmsAgree)
{
  // Build a long text with a known pattern planted at specific positions
  const std::string pattern = "XYZW";
  std::string text(10000, 'a');

  // Plant pattern at known positions
  const size_t positions[] = {0, 42, 500, 1234, 5000, 9996};
  for (const size_t pos : positions)
    text.replace(pos, pattern.size(), pattern);

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);
  const auto bmh = boyer_moore_horspool_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  // All algorithms must agree
  ASSERT_EQ(kmp.size(), z.size());
  ASSERT_EQ(kmp.size(), bmh.size());
  ASSERT_EQ(kmp.size(), rk.size());

  for (size_t i = 0; i < kmp.size(); ++i)
    {
      EXPECT_EQ(kmp[i], z[i]);
      EXPECT_EQ(kmp[i], bmh[i]);
      EXPECT_EQ(kmp[i], rk[i]);
    }

  // Must find at least the planted positions
  EXPECT_EQ(kmp.size(), 6u);
}

TEST(StringSearch, BinaryContentMatching)
{
  // Build text with non-ASCII / binary content
  std::string text;
  for (int i = 0; i < 256; ++i)
    text.push_back(static_cast<char>(i));
  text += text; // 512 bytes, all byte values

  std::string pattern;
  pattern.push_back(static_cast<char>(0xFE));
  pattern.push_back(static_cast<char>(0xFF));
  pattern.push_back(static_cast<char>(0x00));

  const auto kmp = kmp_search(text, pattern);
  const auto bmh = boyer_moore_horspool_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  ASSERT_EQ(kmp.size(), bmh.size());
  ASSERT_EQ(kmp.size(), rk.size());

  for (size_t i = 0; i < kmp.size(); ++i)
    {
      EXPECT_EQ(kmp[i], bmh[i]);
      EXPECT_EQ(kmp[i], rk[i]);
    }

  // Pattern {0xFE, 0xFF, 0x00} spans the boundary once (pos 254)
  EXPECT_EQ(kmp.size(), 1u);
}

TEST(StringSearch, PatternEqualsText)
{
  const std::string s = "hello world";
  expect_array_eq(kmp_search(s, s), {0});
  expect_array_eq(z_search(s, s), {0});
  expect_array_eq(boyer_moore_horspool_search(s, s), {0});
  expect_array_eq(rabin_karp_search(s, s), {0});
}

TEST(StringSearch, StressRepeatedCharacterPattern)
{
  // "aaa...a" (1000 a's), pattern "aaaa" — many overlapping matches
  const std::string text(1000, 'a');
  const std::string pattern(4, 'a');

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);
  const auto bmh = boyer_moore_horspool_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  EXPECT_EQ(kmp.size(), 997u); // 1000 - 4 + 1
  EXPECT_EQ(z.size(), 997u);
  EXPECT_EQ(bmh.size(), 997u);
  EXPECT_EQ(rk.size(), 997u);
}

// --- 3.1 Z-algorithm with full 256-byte alphabet ---

TEST(StringSearch, ZSearchFullByteAlphabet)
{
  // Build a text that uses all 256 byte values so that
  // find_unused_delimiter fails and z_search must handle it.
  std::string text;
  for (int i = 0; i < 256; ++i)
    text.push_back(static_cast<char>(i));
  text += text; // 512 bytes, every byte present

  // Pattern: bytes {0x80, 0x81, 0x82}
  std::string pattern;
  pattern.push_back(static_cast<char>(0x80));
  pattern.push_back(static_cast<char>(0x81));
  pattern.push_back(static_cast<char>(0x82));

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);

  // Z-search must agree with KMP regardless of delimiter availability
  ASSERT_EQ(z.size(), kmp.size());
  for (size_t i = 0; i < z.size(); ++i)
    EXPECT_EQ(z[i], kmp[i]);

  // The pattern appears once per 256-byte block
  EXPECT_EQ(kmp.size(), 2u);
}

TEST(StringSearch, ZSearchFullByteAlphabetOverlapping)
{
  // Construct text with all 256 bytes, then plant an overlapping pattern
  std::string text;
  for (int i = 0; i < 256; ++i)
    text.push_back(static_cast<char>(i));

  // Append a known overlapping section at the boundary
  const std::string pattern = text.substr(250, 6); // last 6 bytes of first block
  text += text; // 512 bytes

  const auto kmp = kmp_search(text, pattern);
  const auto z = z_search(text, pattern);

  ASSERT_EQ(z.size(), kmp.size());
  for (size_t i = 0; i < z.size(); ++i)
    EXPECT_EQ(z[i], kmp[i]);

  EXPECT_GE(kmp.size(), 2u);
}

// --- 3.2 Rabin-Karp with adversarial / collision-prone inputs ---

TEST(StringSearch, RabinKarpRepeatedSingleChar)
{
  // All same character: maximizes hash collisions for naive hashes,
  // since every window has the same character distribution.
  const std::string text(5000, 'A');
  const std::string pattern(7, 'A');

  const auto kmp = kmp_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  ASSERT_EQ(rk.size(), kmp.size());
  for (size_t i = 0; i < rk.size(); ++i)
    EXPECT_EQ(rk[i], kmp[i]);

  EXPECT_EQ(rk.size(), 5000u - 7u + 1u);
}

TEST(StringSearch, RabinKarpBinaryPermutations)
{
  // Two-character alphabet with patterns that are permutations of each
  // other. "aab" and "aba" have identical character frequencies, which
  // can cause collisions in sum-based hashes.
  std::string text;
  for (int i = 0; i < 500; ++i)
    text += "aab";
  // Plant "aba" at a few positions
  text.replace(99, 3, "aba");
  text.replace(600, 3, "aba");

  const auto kmp_aab = kmp_search(text, "aab");
  const auto rk_aab = rabin_karp_search(text, "aab");

  ASSERT_EQ(rk_aab.size(), kmp_aab.size());
  for (size_t i = 0; i < rk_aab.size(); ++i)
    EXPECT_EQ(rk_aab[i], kmp_aab[i]);

  const auto kmp_aba = kmp_search(text, "aba");
  const auto rk_aba = rabin_karp_search(text, "aba");

  ASSERT_EQ(rk_aba.size(), kmp_aba.size());
  for (size_t i = 0; i < rk_aba.size(); ++i)
    EXPECT_EQ(rk_aba[i], kmp_aba[i]);
}

TEST(StringSearch, RabinKarpLongPatternSparseMatches)
{
  // Long pattern with very few matches — stresses the hash verification path.
  std::string text(10000, 'x');
  const std::string pattern(100, 'y');

  // Plant the pattern at exactly two positions
  text.replace(1234, 100, pattern);
  text.replace(7777, 100, pattern);

  const auto kmp = kmp_search(text, pattern);
  const auto rk = rabin_karp_search(text, pattern);

  ASSERT_EQ(rk.size(), kmp.size());
  for (size_t i = 0; i < rk.size(); ++i)
    EXPECT_EQ(rk[i], kmp[i]);

  EXPECT_EQ(rk.size(), 2u);
}

TEST(StringSearch, RabinKarpDifferentBases)
{
  // Verify correctness with different user-supplied bases
  const std::string text = "the cat sat on the mat the cat";
  const std::string pattern = "the cat";

  const auto kmp = kmp_search(text, pattern);
  const uint64_t bases[] = {2, 31, 256, 1000000007ull, 911382323ull};

  for (const uint64_t base : bases)
    {
      const auto rk = rabin_karp_search(text, pattern, base);
      ASSERT_EQ(rk.size(), kmp.size()) << "Failed with base=" << base;
      for (size_t i = 0; i < rk.size(); ++i)
        EXPECT_EQ(rk[i], kmp[i]) << "Failed with base=" << base;
    }
}
