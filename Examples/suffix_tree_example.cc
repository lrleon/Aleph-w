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
 * @file suffix_tree_example.cc
 * @brief Illustrative example for the naive compressed suffix tree.
 */

# include <iostream>
# include <string>

# include <Suffix_Structures.H>

using namespace Aleph;

/**
 * @brief Example program demonstrating construction and queries on a Naive_Suffix_Tree for the string "banana".
 *
 * Constructs a Naive_Suffix_Tree from "banana", prints the input text and node count, checks whether the pattern "ana"
 * is contained in the text, and prints all match starting positions to standard output.
 *
 * @return 0 on successful execution.
 */
int main()
{
  const std::string text = "banana";
  Naive_Suffix_Tree st(text);

  std::cout << "Naive Suffix Tree Example\n";
  std::cout << "Text: " << text << "\n";
  std::cout << "Node count: " << st.node_count() << "\n\n";

  const std::string pattern = "ana";
  const auto matches = st.find_all(pattern);

  std::cout << "Pattern: " << pattern << "\n";
  std::cout << "Contains: " << (not matches.is_empty() ? "yes" : "no") << "\n";
  std::cout << "Matches at positions: ";
  for (size_t i = 0; i < matches.size(); ++i)
    std::cout << matches[i] << (i + 1 == matches.size() ? "" : " ");
  std::cout << "\n";

  return 0;
}
