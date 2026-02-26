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
 * @file suffix_array_lcp_example.cc
 * @brief Illustrative example for suffix array + LCP (Kasai).
 */

# include <iostream>
# include <string>

# include <Suffix_Structures.H>

using namespace Aleph;

/**
 * @brief Example program that builds and prints the suffix array and Kasai LCP array for a sample string.
 *
 * Prints a header, the input text ("banana"), and a table with each index, the suffix array value,
 * the corresponding suffix string, and the LCP value computed by the Kasai algorithm.
 *
 * @return int Exit status code (0 on success).
 */
int main()
{
  const std::string text = "banana";
  const auto sa = suffix_array(text);
  const auto lcp = lcp_array_kasai(text, sa);

  std::cout << "Suffix Array + LCP Example\n";
  std::cout << "Text: " << text << "\n\n";

  std::cout << "i  sa[i]  suffix          lcp[i]\n";
  for (size_t i = 0; i < sa.size(); ++i)
    {
      std::cout << i << "    " << sa[i] << "      "
                << text.substr(sa[i]) << "      " << lcp[i] << '\n';
    }

  return 0;
}
