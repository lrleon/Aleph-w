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
 * @file lis_example.cc
 * @brief Visual walkthrough for LIS and LNDS variants.
 */

# include <iostream>
# include <iomanip>

# include <LIS.H>
# include <print_rule.H>

using namespace Aleph;

namespace
{
  void print_array(const char * label, const Array<int> & arr)
  {
    std::cout << std::setw(14) << std::left << label << ": [";
    for (size_t i = 0; i < arr.size(); ++i)
      {
        if (i > 0)
          std::cout << ", ";
        std::cout << arr[i];
      }
    std::cout << "]\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Longest Increasing Subsequence Toolkit ===\n\n";

  // Example 1: classic LIS
  {
    std::cout << "Scenario A: Classic LIS\n";
    print_rule();
    Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
    print_array("Input", seq);
    const auto r = longest_increasing_subsequence(seq);
    print_array("One LIS", r.subsequence);
    std::cout << std::setw(14) << std::left << "LIS length" << ": " << r.length
              << "\n";
    std::cout << std::setw(14) << std::left << "Length-only" << ": "
              << lis_length(seq) << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 2: already sorted
  {
    std::cout << "Scenario B: Already sorted\n";
    print_rule();
    Array<int> seq = {1, 3, 5, 7, 9};
    print_array("Input", seq);
    const auto r = longest_increasing_subsequence(seq);
    print_array("One LIS", r.subsequence);
    std::cout << std::setw(14) << std::left << "LIS length" << ": " << r.length
              << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 3: reverse sorted
  {
    std::cout << "Scenario C: Reverse sorted\n";
    print_rule();
    Array<int> seq = {9, 7, 5, 3, 1};
    print_array("Input", seq);
    const auto r = longest_increasing_subsequence(seq);
    print_array("One LIS", r.subsequence);
    std::cout << std::setw(14) << std::left << "LIS length" << ": " << r.length
              << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 4: non-decreasing subsequence
  {
    std::cout << "Scenario D: Longest non-decreasing subsequence\n";
    print_rule();
    Array<int> seq = {3, 1, 2, 2, 3, 1};
    print_array("Input", seq);
    const auto r = longest_nondecreasing_subsequence(seq);
    print_array("One LNDS", r.subsequence);
    std::cout << std::setw(14) << std::left << "LNDS length" << ": "
              << r.length << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 5: decreasing sequence via custom comparator
  {
    std::cout << "Scenario E: Decreasing subsequence via comparator\n";
    print_rule();
    Array<int> seq = {5, 1, 4, 2, 8, 3};
    print_array("Input", seq);
    const auto lds = longest_increasing_subsequence(seq, Aleph::greater<int>());
    const auto lnds_desc = longest_nondecreasing_subsequence(seq,
                                                             Aleph::greater<int>());
    print_array("Strict dec", lds.subsequence);
    print_array("Non-inc", lnds_desc.subsequence);
    std::cout << std::setw(14) << std::left << "LDS length" << ": "
              << lds.length << "\n";
    std::cout << std::setw(14) << std::left << "Non-inc len" << ": "
              << lnds_desc.length << "\n";
    print_rule();
  }

  std::cout << "\nDone.\n";
  return 0;
}
