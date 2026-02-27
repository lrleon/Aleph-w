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
 * @file gray_code_example.cc
 * @brief Demonstrates usage of Gray code utilities in Aleph-w.
 */

#include <iostream>
#include <iomanip>
#include <bitset>

#include <ah-comb.H>
#include <print_rule.H>

using namespace Aleph;

namespace
{
  /** @brief Format a number as binary string with fixed width. */
  template <size_t N>
  std::string to_bin(uint32_t val)
  {
    std::bitset<N> bits(val);
    return bits.to_string();
  }
}

int main()
{
  std::cout << "\n=== Gray Code Utilities Demonstration ===\n\n";

  {
    std::cout << "Scenario A: Binary to Gray and back\n";
    print_rule();
    std::cout << std::left << std::setw(10) << "Decimal" 
              << std::setw(12) << "Binary" 
              << std::setw(12) << "Gray" 
              << "Recovered Binary\n";
    print_rule();

    for (uint32_t i = 0; i < 16; ++i)
      {
        uint32_t g = bin_to_gray(i);
        uint32_t b = gray_to_bin(g);
        
        std::cout << std::left << std::setw(10) << i 
                  << std::setw(12) << to_bin<4>(i) 
                  << std::setw(12) << to_bin<4>(g) 
                  << to_bin<4>(b) << (i == b ? " [OK]" : " [ERR]") << "\n";
      }
    print_rule();
    std::cout << "\n";
  }

  {
    std::cout << "Scenario B: n-bit Gray code sequence generation\n";
    print_rule();
    const size_t n = 3;
    auto sequence = build_gray_code(n);
    
    std::cout << "Generated " << n << "-bit Gray code sequence:\n";
    for (size_t i = 0; i < sequence.size(); ++i)
      {
        std::cout << "  Step " << std::setw(2) << i << ": " 
                  << to_bin<3>(sequence[i]);
        if (i > 0)
          {
            uint32_t diff = sequence[i] ^ sequence[i-1];
            std::cout << " (diff bit: " << std::popcount(diff) << ")";
          }
        std::cout << "\n";
      }
    
    // Check cyclic property
    uint32_t cyclic_diff = sequence[sequence.size()-1] ^ sequence[0];
    std::cout << "  Cyclic difference bit count: " << std::popcount(cyclic_diff) << "\n";
    
    print_rule();
    std::cout << "\n";
  }

  {
    std::cout << "Scenario C: Large value conversion\n";
    print_rule();
    uint64_t val = 0xDEADBEEFCAFEBABELL;
    uint64_t g = bin_to_gray(val);
    uint64_t b = gray_to_bin(g);
    
    std::cout << "Original : 0x" << std::hex << val << "\n";
    std::cout << "Gray     : 0x" << g << "\n";
    std::cout << "Recovered: 0x" << b << "\n";
    std::cout << std::dec;
    if (val == b)
      std::cout << "Successfully converted and recovered 64-bit value.\n";
    else
      std::cout << "Error in 64-bit conversion.\n";
    print_rule();
  }

  std::cout << "\nDone.\n";
  return 0;
}
