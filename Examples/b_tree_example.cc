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
 * @file b_tree_example.cc
 * @brief Illustrative example for Aleph::B_Tree
 */

#include <exception>
#include <iostream>

#include <ah-errors.H>
#include <tpl_b_tree.H>

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
      using Catalog = B_Tree<int, Aleph::less<int>, 3>;

      Catalog catalog = {40, 10, 90, 20, 70, 60, 30, 50, 80};

      std::cout << "B-Tree example\n";
      std::cout << "==============\n";
      print_array("initial keys :", catalog.keys());
      std::cout << "height       : " << catalog.height() << '\n';

      if (const auto lb = catalog.lower_bound(55); lb.has_value())
        std::cout << "lower_bound(55) -> " << *lb << '\n';

      if (const auto ub = catalog.upper_bound(60); ub.has_value())
        std::cout << "upper_bound(60) -> " << *ub << '\n';

      catalog.insert(65);
      catalog.insert(75);
      catalog.insert(5);
      catalog.remove(40);
      catalog.remove(20);

      ah_runtime_error_unless(catalog.verify())
        << "B_Tree example produced an invalid tree";

      std::cout << "\nafter updates\n";
      std::cout << "-------------\n";
      print_array("sorted keys  :", catalog.keys());
      std::cout << "size         : " << catalog.size() << '\n';
      std::cout << "height       : " << catalog.height() << '\n';

      if (const auto mn = catalog.min_key(); mn.has_value())
        std::cout << "min          : " << *mn << '\n';

      if (const auto mx = catalog.max_key(); mx.has_value())
        std::cout << "max          : " << *mx << '\n';

      std::cout << "\nB-Tree keeps data sorted while internal nodes also hold keys.\n";
    }
  catch (const std::exception & ex)
    {
      std::cerr << "b_tree_example failed: " << ex.what() << '\n';
      return 1;
    }

  return 0;
}
