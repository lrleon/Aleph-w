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
  /**
   * @brief Helper to print an array of integers formatted in a single line
   * brackets and separated by commas.
   *
   * @param label Text label printed before the array.
   * @param arr   Array of integers to print.
   */
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

  /**
   * @brief Encapsulates setup, computation, and result printing for a single LIS/LNDS scenario.
   *
   * @param label      The title for the scenario (e.g. "Scenario A: ...").
   * @param seq        The input sequence.
   * @param compute_fn Function that computes the LIS/LNDS result from the sequence.
   * @param res_label  Label for the computed subsequence (e.g. "One LIS").
   * @param len_label  Label for the computed length (e.g. "LIS length").
   */
  template <typename Fn>
  void run_scenario(const char * label,
                    const Array<int> & seq,
                    Fn compute_fn,
                    const char * res_label,
                    const char * len_label)
  {
    std::cout << label << "\n";
    print_rule();
    print_array("Input", seq);
    const auto r = compute_fn(seq);
    print_array(res_label, r.subsequence);
    std::cout << std::setw(14) << std::left << len_label << ": " << r.length << "\n";
  }
}

/**
 * @brief Demonstrates usage of Aleph-w LIS/LNDS utilities by running several example scenarios and printing their inputs and results.
 *
 * Runs five scenarios: classic LIS, already sorted, reverse sorted, longest non-decreasing subsequence,
 * and decreasing subsequences via a custom comparator. Each scenario prints the input sequence, one
 * computed subsequence, and its length using Aleph-w helpers.
 *
 * @return int Exit code (0 on success).
 */

int main()
{
  std::cout << "\n=== Longest Increasing Subsequence Toolkit ===\n\n";

  // Example 1: classic LIS
  {
    Array<int> seq = {10, 9, 2, 5, 3, 7, 101, 18};
    run_scenario("Scenario A: Classic LIS", seq,
                 [](const auto & s) { return longest_increasing_subsequence(s); },
                 "One LIS", "LIS length");
    std::cout << std::setw(14) << std::left << "Length-only" << ": "
              << lis_length(seq) << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 2: already sorted
  {
    Array<int> seq = {1, 3, 5, 7, 9};
    run_scenario("Scenario B: Already sorted", seq,
                 [](const auto & s) { return longest_increasing_subsequence(s); },
                 "One LIS", "LIS length");
    print_rule();
    std::cout << "\n";
  }

  // Example 3: reverse sorted
  {
    Array<int> seq = {9, 7, 5, 3, 1};
    run_scenario("Scenario C: Reverse sorted", seq,
                 [](const auto & s) { return longest_increasing_subsequence(s); },
                 "One LIS", "LIS length");
    print_rule();
    std::cout << "\n";
  }

  // Example 4: non-decreasing subsequence
  {
    Array<int> seq = {3, 1, 2, 2, 3, 1};
    run_scenario("Scenario D: Longest non-decreasing subsequence", seq,
                 [](const auto & s) { return longest_nondecreasing_subsequence(s); },
                 "One LNDS", "LNDS length");
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
