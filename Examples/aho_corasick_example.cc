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
 * @file aho_corasick_example.cc
 * @brief Illustrative example for Aho-Corasick multi-pattern matching.
 */

# include <iostream>
# include <string>

# include <Aho_Corasick.H>

using namespace Aleph;

int main()
{
  Aho_Corasick ac;
  ac.add_pattern("he");
  ac.add_pattern("she");
  ac.add_pattern("his");
  ac.add_pattern("hers");
  ac.build();

  const std::string text = "ahishers";
  const auto matches = ac.search(text);

  std::cout << "Aho-Corasick Example\n";
  std::cout << "Text: " << text << "\n\n";

  std::cout << "Matches (position -> pattern):\n";
  for (auto [position, pattern_id] : matches)
      std::cout << "  " << position << " -> "
                << ac.pattern(pattern_id)
                << " (id=" << pattern_id << ")\n";

  return 0;
}
