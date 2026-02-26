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
 * @file lcs_longest_common_substring_example.cc
 * @brief Illustrative example for LCS and Longest Common Substring.
 */

# include <iostream>
# include <string>

# include <String_DP.H>

using namespace Aleph;

/**
 * @brief Example program demonstrating longest_common_subsequence and longest_common_substring.
 *
 * Prints two sample input strings and the results from computing their longest common subsequence
 * and a longest common substring (including lengths, substring value, and begin indices) to standard output.
 *
 * @return int Exit code 0 on successful execution.
 */
int main()
{
  const std::string a = "AGGTAB";
  const std::string b = "GXTXAYB";

  const auto lcs = longest_common_subsequence(a, b);
  const auto lcss = longest_common_substring("xabxac", "abcabxabcd");

  std::cout << "LCS / Longest Common Substring Example\n";
  std::cout << "a = " << a << "\n";
  std::cout << "b = " << b << "\n\n";

  std::cout << "LCS length: " << lcs.length << "\n";
  std::cout << "LCS value : " << lcs.subsequence << "\n\n";

  std::cout << "Longest Common Substring length: " << lcss.length << "\n";
  std::cout << "Substring: " << lcss.substring << "\n";
  std::cout << "Begin in first string : " << lcss.begin_a << "\n";
  std::cout << "Begin in second string: " << lcss.begin_b << "\n";

  return 0;
}
