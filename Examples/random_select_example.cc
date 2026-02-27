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
 * @file random_select_example.cc
 * @brief Demonstrates the use of Aleph's random_select algorithm.
 *
 * This example shows how to use the iterative, 3-way partitioned Quickselect
 * implementation provided by `tpl_sort_utils.H` to find the k-th smallest
 * elements in different arrays, including calculating the median efficiently.
 */

#include <iostream>
#include <iomanip>

#include <tpl_sort_utils.H>
#include <tpl_dynArray.H>

using namespace Aleph;

namespace
{
  void print_rule()
  {
    std::cout << "--------------------------------------------------------\n";
  }

  template <typename Container>
  void print_array(const char * label, const Container & a)
  {
    std::cout << std::left << std::setw(15) << label << ": [";
    for (size_t i = 0; i < a.size(); ++i)
      {
        std::cout << a[i];
        if (i < a.size() - 1)
          std::cout << ", ";
      }
    std::cout << "]\n";
  }
}

int main()
{
  std::cout << "\n=== Random Select (Quickselect) Demonstration ===\n\n";

  {
    std::cout << "Scenario A: Finding the median of an array\n";
    print_rule();
    DynArray<int> arr = {9, 2, 4, 7, 3, 7, 10, 2, 7, 1, 8};
    print_array("Original Array", arr);

    const size_t median_idx = arr.size() / 2;
    
    // random_select takes (array, k-th index, comparator)
    const int median = random_select(arr, static_cast<long>(median_idx));
    
    std::cout << "Position k      : " << median_idx << " (Median)\n";
    std::cout << "Selected Value  : " << median << "\n";
    
    // Note that the array has been partially sorted so that all elements
    // before `median_idx` are <= median, and all elements after are >= median.
    print_array("Partially Sorted", arr);
    print_rule();
    std::cout << "\n";
  }

  {
    std::cout << "Scenario B: Finding the top 3 largest elements\n";
    print_rule();
    DynArray<int> arr = {15, 3, 9, 8, 22, 17, 4, 12, 19};
    print_array("Original Array", arr);

    // To find the top 3, we select the (n-3)-th element
    const size_t k = arr.size() - 3;
    const int k_val = random_select(arr, static_cast<long>(k));

    std::cout << "Position k      : " << k << " (n - 3)\n";
    std::cout << "Selected Value  : " << k_val << "\n";
    print_array("Partially Sorted", arr);
    
    std::cout << "Top 3 elements  : [";
    for (size_t i = k; i < arr.size(); ++i)
      {
        std::cout << arr[i];
        if (i < arr.size() - 1)
          std::cout << ", ";
      }
    std::cout << "]\n";
    
    print_rule();
    std::cout << "\n";
  }

  {
    std::cout << "Scenario C: Handling arrays with many duplicates\n";
    print_rule();
    // The 3-way partition (Dutch National Flag) efficiently handles duplicates
    DynArray<int> arr = {5, 5, 5, 2, 5, 5, 8, 5, 5, 1, 5, 9};
    print_array("Original Array", arr);

    const size_t k = 8;
    const int val = random_select(arr, static_cast<long>(k));

    std::cout << "Position k      : " << k << "\n";
    std::cout << "Selected Value  : " << val << "\n";
    print_array("Partially Sorted", arr);
    
    print_rule();
    std::cout << "\n";
  }

  std::cout << "Done.\n";
  return 0;
}
