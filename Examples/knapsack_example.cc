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
 * @file knapsack_example.cc
 * @brief Rich, side-by-side demo for Knapsack variants.
 */

# include <iostream>
# include <iomanip>

# include <Knapsack.H>
# include <print_rule.H>

using namespace Aleph;

using Item = Knapsack_Item<int, int>;

namespace
{
  void print_item_table(const Array<Item> & items, const Array<const char *> & names)
  {
    std::cout << std::left
              << std::setw(4) << "#"
              << std::setw(20) << "Item"
              << std::setw(10) << "Weight"
              << std::setw(10) << "Value" << "\n";
    for (size_t i = 0; i < items.size(); ++i)
      std::cout << std::left
                << std::setw(4) << i
                << std::setw(20) << names[i]
                << std::setw(10) << items[i].weight
                << std::setw(10) << items[i].value << "\n";
  }

  void print_selection_summary(const Array<Item> & items,
                               const Array<const char *> & names,
                               const Array<size_t> & selected)
  {
    Array<size_t> freq = Array<size_t>::create(items.size());
    for (size_t i = 0; i < items.size(); ++i)
      freq(i) = 0;

    int total_w = 0;
    int total_v = 0;
    for (size_t i = 0; i < selected.size(); ++i)
      {
        ++freq(selected[i]);
        total_w += items[selected[i]].weight;
        total_v += items[selected[i]].value;
      }

    std::cout << "Chosen items:\n";
    for (size_t i = 0; i < items.size(); ++i)
      if (freq[i] > 0)
        std::cout << "  " << freq[i] << " x " << names[i]
                  << "  (w=" << items[i].weight
                  << ", v=" << items[i].value << ")\n";

    if (selected.is_empty())
      std::cout << "  [empty]\n";

    std::cout << "Total weight: " << total_w << "\n";
    std::cout << "Total value : " << total_v << "\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Knapsack Toolkit ===\n\n";

  // Example 1: 0/1 knapsack
  {
    std::cout << "Scenario A: Expedition backpack (0/1)\n";
    print_rule();

    Array<Item> items = {{4, 6}, {3, 5}, {2, 3}, {5, 7}, {1, 2}};
    Array<const char *> names = {"Tent", "Sleeping bag", "Food",
                                 "Water flask", "First aid kit"};
    const int capacity = 10;

    print_item_table(items, names);
    std::cout << "Capacity: " << capacity << "\n";

    const auto r = knapsack_01(items, capacity);
    std::cout << "Optimal value: " << r.optimal_value << "\n";
    print_selection_summary(items, names, r.selected_items);
    print_rule();
    std::cout << "\n";
  }

  // Example 2: value-only shortcut
  {
    std::cout << "Scenario B: Value-only query (space optimized)\n";
    print_rule();
    Array<Item> items = {{4, 6}, {3, 5}, {2, 3}, {5, 7}, {1, 2}};
    std::cout << "Capacity: 10\n";
    std::cout << "Optimal value only: " << knapsack_01_value(items, 10) << "\n";
    print_rule();
    std::cout << "\n";
  }

  // Example 3: unbounded knapsack
  {
    std::cout << "Scenario C: Infinite stock (unbounded)\n";
    print_rule();
    Array<Item> items = {{1, 1}, {3, 4}, {4, 5}};
    Array<const char *> names = {"Bronze coin", "Silver coin", "Gold coin"};
    const int capacity = 7;

    print_item_table(items, names);
    std::cout << "Capacity: " << capacity << "\n";

    const auto r = knapsack_unbounded(items, capacity);
    std::cout << "Optimal value: " << r.optimal_value << "\n";
    print_selection_summary(items, names, r.selected_items);
    print_rule();
    std::cout << "\n";
  }

  // Example 4: bounded knapsack
  {
    std::cout << "Scenario D: Warehouse with limited stock (bounded)\n";
    print_rule();
    Array<Item> items = {{2, 3}, {3, 5}, {4, 7}};
    Array<size_t> counts = {3, 2, 1};
    Array<const char *> names = {"Widget A", "Widget B", "Widget C"};
    const int capacity = 10;

    print_item_table(items, names);
    std::cout << "Stock limits:\n";
    for (size_t i = 0; i < items.size(); ++i)
      std::cout << "  " << names[i] << " -> " << counts[i] << "\n";
    std::cout << "Capacity: " << capacity << "\n";

    const auto r = knapsack_bounded(items, counts, capacity);
    std::cout << "Optimal value: " << r.optimal_value << "\n";
    print_selection_summary(items, names, r.selected_items);
    print_rule();
  }

  std::cout << "\nDone.\n";
  return 0;
}
