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
 * @file z_algorithm_example.cc
 * @brief Illustrative example for Z-algorithm pattern matching.
 */

# include <iostream>
# include <string>
# include <format>
# include <ranges>

# include <String_Search.H>

using namespace Aleph;

template <typename Range>
void print_range(std::string_view label, const Range & r)
{
  std::cout << label << ": ";
  for (const auto & item : r)
    std::cout << std::format("{} ", item);
  std::cout << "\n";
}

int main()
{
  const std::string text = "aabcaabxaaaz";
  const std::string pattern = "aab";

  const auto z = z_algorithm(text);
  const auto matches = z_search(text, pattern);

  std::cout << std::format("Z-Algorithm Example\nText   : {}\nPattern: {}\n\n",
                           text, pattern);

  print_range("Z-array", z);
  print_range("Matches at positions", matches);

  return 0;
}
