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
 * @file matrix_chain_example.cc
 * @brief Illustrated examples for matrix-chain optimization.
 */

# include <iostream>
# include <iomanip>

# include <Matrix_Chain.H>
# include <print_rule.H>

using namespace Aleph;

namespace
{
  void print_dims(const Array<size_t> & dims)
  {
    std::cout << "Matrices: ";
    for (size_t i = 0; i + 1 < dims.size(); ++i)
      {
        if (i > 0)
          std::cout << "  ";
        std::cout << "A" << (i + 1)
                  << "(" << dims[i] << "x" << dims[i + 1] << ")";
      }
    std::cout << "\n";
  }

  void run_case(const Array<size_t> & dims, const char * title)
  {
    std::cout << title << "\n";
    print_rule();
    print_dims(dims);
    const auto r = matrix_chain_order(dims);
    std::cout << std::setw(26) << std::left << "Minimum multiplications"
              << ": " << r.min_multiplications << "\n";
    std::cout << std::setw(26) << std::left << "Optimal parenthesization"
              << ": " << r.parenthesization << "\n";
    print_rule();
    std::cout << "\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Matrix-Chain Dynamic Programming ===\n\n";

  run_case({30, 35, 15, 5, 10, 20, 25}, "Scenario A: CLRS reference chain");
  run_case({10, 20, 30}, "Scenario B: Two matrices");
  run_case({10, 30, 5, 60}, "Scenario C: Three matrices (high asymmetry)");
  run_case({5, 5, 5, 5, 5}, "Scenario D: Uniform square matrices");

  std::cout << "Scenario E: Cost-only helper\n";
  print_rule();
  Array<size_t> dims = {40, 20, 30, 10, 30};
  print_dims(dims);
  std::cout << std::setw(26) << std::left << "matrix_chain_min_cost"
            << ": " << matrix_chain_min_cost(dims) << "\n";
  print_rule();

  std::cout << "\nDone.\n";
  return 0;
}
