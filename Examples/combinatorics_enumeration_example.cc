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
 * @file combinatorics_enumeration_example.cc
 * @brief Extended lexicographic permutation and k-combination enumeration.
 */

# include <iomanip>
# include <iostream>

# include <ah-comb.H>

using namespace Aleph;

namespace
{
  template <class Seq>
  void print_seq(const Seq & s)
  {
    std::cout << "[";
    for (size_t i = 0; i < s.size(); ++i)
      {
        if (i > 0)
          std::cout << ", ";
        std::cout << s(i);
      }
    std::cout << "]";
  }

  void rule()
  {
    std::cout << "------------------------------------------------------------\n";
  }

  void demo_next_permutation_multiset()
  {
    std::cout << "[1] next_permutation with duplicates (multiset)\n";
    rule();

    Array<int> a = {1, 1, 2, 3};

    size_t count = 1;
    std::cout << std::setw(3) << count << " : ";
    print_seq(a);
    std::cout << "\n";

    while (next_permutation(a))
      {
        ++count;
        std::cout << std::setw(3) << count << " : ";
        print_seq(a);
        std::cout << "\n";
      }

    std::cout << "Total distinct lexicographic permutations: " << count << "\n";
    std::cout << "After last step, reset to first: ";
    print_seq(a);
    std::cout << "\n\n";
  }

  void demo_next_permutation_custom_order()
  {
    std::cout << "[2] next_permutation with custom comparator (descending order)\n";
    rule();

    Array<int> a = {4, 3, 2, 1}; // first in Aleph::greater order

    for (size_t step = 1; step <= 5; ++step)
      {
        std::cout << "Step " << step << " -> ";
        print_seq(a);
        std::cout << "\n";
        (void) next_permutation(a, Aleph::greater<int>());
      }

    std::cout << "\n";
  }

  void demo_next_combination_indices()
  {
    std::cout << "[3] next_combination_indices (k-combinations over [0..n))\n";
    rule();

    const size_t n = 6;
    Array<size_t> idx = {0, 1, 2};

    size_t count = 0;
    while (true)
      {
        ++count;
        std::cout << std::setw(3) << count << " : ";
        print_seq(idx);
        std::cout << "\n";

        if (not next_combination_indices(idx, n))
          break;
      }

    std::cout << "Total combinations C(" << n << ", 3): " << count
              << " (verified by combination_count = "
              << combination_count(n, 3) << ")\n\n";
  }

  void demo_bitmask_combinations()
  {
    std::cout << "[4] next_combination_mask (fixed-popcount bitmask)\n";
    rule();

    const size_t n = 6;
    uint64_t mask = first_combination_mask(3); // 000111

    size_t count = 0;
    while (true)
      {
        ++count;
        std::cout << std::setw(3) << count << " : mask = ";
        for (size_t b = n; b > 0; --b)
          std::cout << (((mask >> (b - 1)) & 1ULL) ? '1' : '0');
        std::cout << "\n";

        if (not next_combination_mask(mask, n))
          break;
      }

    std::cout << "Total bitmask combinations: " << count << "\n\n";
  }

  void demo_materialized_combinations()
  {
    std::cout << "[5] for_each_combination / build_combinations\n";
    rule();

    Array<std::string> features = {
      "geom", "strings", "graphs", "dp", "net"
    };

    std::cout << "Feature triplets:\n";
    (void) for_each_combination(features, 3,
      [] (const Array<std::string> & c)
      {
        std::cout << "  - ";
        print_seq(c);
        std::cout << "\n";
        return true;
      });

    auto pairs = build_combinations(features, 2);
    std::cout << "\nMaterialized pairs: " << pairs.size() << "\n";
    std::cout << "First: ";
    print_seq(pairs(0));
    std::cout << " | Last: ";
    print_seq(pairs(pairs.size() - 1));
    std::cout << "\n\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Combinatorics / Enumeration ===\n\n";

  demo_next_permutation_multiset();
  demo_next_permutation_custom_order();
  demo_next_combination_indices();
  demo_bitmask_combinations();
  demo_materialized_combinations();

  std::cout << "Done.\n";
  return 0;
}
