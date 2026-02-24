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
 * @file string_palindromes_test.cc
 * @brief Tests for String_Palindromes.H.
 */

# include <gtest/gtest.h>

# include <String_Palindromes.H>

using namespace Aleph;

TEST(StringPalindromes, ManacherOddPalindrome)
{
  const auto r = manacher("abacaba");
  EXPECT_EQ(r.longest_palindrome, "abacaba");
  EXPECT_EQ(r.longest_begin, 0u);
  EXPECT_EQ(r.longest_length, 7u);
  ASSERT_EQ(r.odd_radius.size(), 7u);
  EXPECT_EQ(r.odd_radius[3], 4u);
}

TEST(StringPalindromes, ManacherEvenPalindrome)
{
  const auto r = manacher("abba");
  EXPECT_EQ(r.longest_palindrome, "abba");
  EXPECT_EQ(r.longest_begin, 0u);
  EXPECT_EQ(r.longest_length, 4u);
  ASSERT_EQ(r.even_radius.size(), 4u);
  EXPECT_GE(r.even_radius[2], 2u);
}

TEST(StringPalindromes, ManacherRepeatedCharacter)
{
  const auto r = manacher("aaaa");
  EXPECT_EQ(r.longest_palindrome, "aaaa");
  EXPECT_EQ(longest_palindromic_substring("aaaa"), "aaaa");
}

TEST(StringPalindromes, ManacherEmpty)
{
  const auto r = manacher("");
  EXPECT_TRUE(r.odd_radius.is_empty());
  EXPECT_TRUE(r.even_radius.is_empty());
  EXPECT_EQ(r.longest_length, 0u);
  EXPECT_EQ(r.longest_palindrome, "");
}

TEST(StringPalindromes, ManacherSingleChar)
{
  const auto r = manacher("x");
  EXPECT_EQ(r.longest_palindrome, "x");
  EXPECT_EQ(r.longest_length, 1u);
  EXPECT_EQ(r.longest_begin, 0u);
}

TEST(StringPalindromes, ManacherNoPalindromeAbove1)
{
  // "abcdef" — no palindrome longer than 1 character
  const auto r = manacher("abcdef");
  EXPECT_EQ(r.longest_length, 1u);
}

TEST(StringPalindromes, ManacherLongPalindrome)
{
  // Build a long palindrome: "abcba" repeated structure
  std::string half = "abcdefghijklmnopqrstuvwxyz";
  std::string palindrome = half;
  std::string rev_half(half.rbegin(), half.rend());
  palindrome += rev_half;

  const auto r = manacher(palindrome);
  EXPECT_EQ(r.longest_length, palindrome.size());
  EXPECT_EQ(r.longest_palindrome, palindrome);
}

TEST(StringPalindromes, ManacherPalindromeInMiddle)
{
  const auto r = manacher("xxxracecarxxx");
  EXPECT_EQ(r.longest_palindrome, "xxxracecarxxx");
  EXPECT_EQ(r.longest_length, 13u);
}

TEST(StringPalindromes, ManacherStressRepeatedPattern)
{
  // 1000 chars all same letter — entire string is a palindrome
  const std::string text(1000, 'z');
  const auto r = manacher(text);
  EXPECT_EQ(r.longest_length, 1000u);
  EXPECT_EQ(r.longest_palindrome, text);
}
