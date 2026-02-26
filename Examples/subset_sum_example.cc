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
 * @file subset_sum_example.cc
 * @brief Illustrated subset-sum scenarios (DP + MITM).
 */

# include <iostream>
# include <iomanip>

# include <Subset_Sum.H>

using namespace Aleph;

namespace
{
  void print_rule()
  {
    std::cout << "------------------------------------------------------------\n";
  }

  void print_values(const Array<int> & vals)
  {
    std::cout << "Values: [";
    for (size_t i = 0; i < vals.size(); ++i)
      {
        if (i > 0)
          std::cout << ", ";
        std::cout << vals[i];
      }
    std::cout << "]\n";
  }

  void print_solution(const Array<int> & vals, const Array<size_t> & idx)
  {
    if (idx.is_empty())
      {
        std::cout << "Chosen subset: [empty]\n";
        return;
      }

    int sum = 0;
    std::cout << "Chosen subset: ";
    for (size_t i = 0; i < idx.size(); ++i)
      {
        if (i > 0)
          std::cout << " + ";
        std::cout << vals[idx[i]];
        sum += vals[idx[i]];
      }
    std::cout << " = " << sum << "\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Subset Sum Toolkit ===\n\n";

  // Example 1: DP with reconstruction
  {
    std::cout << "Scenario A: Classical DP reconstruction\n";
    print_rule();
    Array<int> vals = {3, 34, 4, 12, 5, 2};
    const int target = 9;

    print_values(vals);
    std::cout << "Target: " << target << "\n";

    const auto r = subset_sum(vals, target);
    if (r.exists)
      print_solution(vals, r.selected_indices);
    else
      std::cout << "No subset found.\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 2: existence-only queries
  {
    std::cout << "Scenario B: Existence-only API\n";
    print_rule();
    Array<int> vals = {1, 5, 11, 5};
    print_values(vals);
    std::cout << "Sum to 11? " << (subset_sum_exists(vals, 11) ? "Yes" : "No")
              << "\n";
    std::cout << "Sum to 100? " << (subset_sum_exists(vals, 100) ? "Yes" : "No")
              << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 3: counting subsets
  {
    std::cout << "Scenario C: Counting all subsets by target\n";
    print_rule();
    Array<int> vals = {1, 2, 3, 4, 5};
    print_values(vals);
    for (int t = 3; t <= 10; ++t)
      std::cout << "  target=" << std::setw(2) << t << " -> "
                << subset_sum_count(vals, t) << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 4: MITM for larger n / signed values
  {
    std::cout << "Scenario D: Meet-in-the-middle with signed values\n";
    print_rule();
    Array<int> vals = {-7, -3, -2, 5, 8, 11, 13, 21};
    const int target = 10;
    print_values(vals);
    std::cout << "Target: " << target << "\n";

    const auto r = subset_sum_mitm(vals, target);
    if (r.exists)
      print_solution(vals, r.selected_indices);
    else
      std::cout << "No subset found.\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 5: DP vs MITM consistency
  {
    std::cout << "Scenario E: DP vs MITM consistency table\n";
    print_rule();
    Array<int> vals = {2, 3, 7, 8, 10};
    print_values(vals);
    std::cout << std::setw(8) << std::left << "Target"
              << std::setw(8) << "DP"
              << std::setw(8) << "MITM" << "\n";
    for (int t = 0; t <= 30; t += 5)
      {
        const bool dp_ans = subset_sum_exists(vals, t);
        const auto mitm_r = subset_sum_mitm(vals, t);
        std::cout << std::setw(8) << std::left << t
                  << std::setw(8) << (dp_ans ? "true" : "false")
                  << std::setw(8) << (mitm_r.exists ? "true" : "false") << "\n";
      }
    print_rule();
  }

  std::cout << "\nDone.\n";
  return 0;
}
