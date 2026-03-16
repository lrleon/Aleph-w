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
 * @file bplus_tree_example.cc
 * @brief Illustrative example for Aleph::BPlus_Tree
 */

#include <exception>
#include <iostream>

#include <ah-errors.H>
#include <tpl_bplus_tree.H>

using namespace Aleph;

namespace
{
  template <typename T>
  void print_array(const char * label, const Array<T> & values)
  {
    std::cout << label << " [";
    for (size_t i = 0; i < values.size(); ++i)
      {
        if (i != 0)
          std::cout << ", ";
        std::cout << values[i];
      }
    std::cout << "]\n";
  }
}

int main()
{
  try
    {
      using Ledger = BPlus_Tree<int, Aleph::less<int>, 3>;

      Ledger ledger = {
          105, 110, 115, 120, 125, 130, 135,
          140, 145, 150, 155, 160, 165, 170
      };

      std::cout << "B+ Tree example\n";
      std::cout << "==============\n";
      print_array("all keys      :", ledger.keys());
      print_array("range 123..158:", ledger.range(123, 158));

      if (const auto lb = ledger.lower_bound(128); lb.has_value())
        std::cout << "lower_bound(128) -> " << *lb << '\n';

      if (const auto ub = ledger.upper_bound(145); ub.has_value())
        std::cout << "upper_bound(145) -> " << *ub << '\n';

      ledger.insert(172);
      ledger.insert(175);
      ledger.remove(125);
      ledger.remove(130);

      ah_runtime_error_unless(ledger.verify())
        << "BPlus_Tree example produced an invalid tree";

      std::cout << "\nafter updates\n";
      std::cout << "-------------\n";
      print_array("all keys      :", ledger.keys());
      print_array("range 140..176:", ledger.range(140, 176));
      std::cout << "size          : " << ledger.size() << '\n';
      std::cout << "height        : " << ledger.height() << '\n';

      std::cout << "\nB+ Tree keeps user keys in the leaves, so range scans walk leaf links.\n";
    }
  catch (const std::exception & ex)
    {
      std::cerr << "bplus_tree_example failed: " << ex.what() << '\n';
      return 1;
    }

  return 0;
}
