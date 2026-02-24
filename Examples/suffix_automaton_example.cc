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
 * @file suffix_automaton_example.cc
 * @brief Illustrative example for suffix automaton.
 */

# include <iostream>
# include <string>

# include <Suffix_Structures.H>

using namespace Aleph;

/**
 * @brief Demonstrates basic usage of the Suffix_Automaton API with a sample string.
 *
 * Builds a suffix automaton for the string "ababa", prints the original text,
 * the number of automaton states, the number of distinct substrings, whether
 * the substring "bab" is contained, and the longest common substring with
 * "zzabxababa".
 *
 * @return int Exit status (0 indicates success).
 */
int main()
{
  const std::string text = "ababa";
  Suffix_Automaton sam;
  sam.build(text);

  std::cout << "Suffix Automaton Example\n";
  std::cout << "Text: " << text << "\n";
  std::cout << "States: " << sam.state_count() << "\n";
  std::cout << "Distinct substrings: " << sam.distinct_substring_count() << "\n";
  std::cout << "Contains 'bab'? " << (sam.contains("bab") ? "yes" : "no") << "\n";

  const std::string other = "zzabxababa";
  std::cout << "LCS(text, '" << other << "') = "
            << sam.longest_common_substring(other) << "\n";

  return 0;
}
