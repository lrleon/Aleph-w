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
 * @file tree_dp_example.cc
 * @brief Illustrated tree DP + rerooting walkthrough.
 */

# include <iostream>
# include <iomanip>

# include <Tree_DP.H>

using namespace Aleph;

using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

namespace
{
  void print_rule()
  {
    std::cout << "------------------------------------------------------------\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== Tree DP / Rerooting DP ===\n\n";

  // Build a tree:
  //       0
  //      / \_
  //     1   2
  //    / \   \_
  //   3   4   5
  G g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);
  auto n4 = g.insert_node(4);
  auto n5 = g.insert_node(5);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n1, n4, 1);
  g.insert_arc(n2, n5, 1);

  G::Node * nodes[] = {n0, n1, n2, n3, n4, n5};

  std::cout << "Tree (root=0):\n";
  std::cout << "      0\n";
  std::cout << "    /   \\\n";
  std::cout << "   1     2\n";
  std::cout << "  / \\     \\\n";
  std::cout << " 3   4     5\n\n";

  const auto subtree_sizes = tree_subtree_sizes(g, n0);
  const auto max_dist = tree_max_distance(g, n0);
  const auto sum_dist = tree_sum_of_distances(g, n0);

  Gen_Tree_DP<G, size_t> id_map(g, n0,
    [](auto *) -> size_t { return 1; },
    [](auto *, const size_t & a, auto *, const size_t & c) -> size_t {
      return a + c;
    });

  Gen_Tree_DP<G, int> value_sum_dp(g, n0,
    [](auto * p) -> int { return p->get_info(); },
    [](auto *, const int & acc, auto *, const int & child) -> int {
      return acc + child;
    });

  std::cout << "Per-node metrics:\n";
  print_rule();
  std::cout << std::left
            << std::setw(8) << "Node"
            << std::setw(16) << "Subtree size"
            << std::setw(16) << "Max distance"
            << std::setw(18) << "Sum distances"
            << std::setw(18) << "Subtree value sum"
            << "\n";
  print_rule();

  for (int i = 0; i < 6; ++i)
    {
      const size_t id = id_map.id_of(nodes[i]);
      std::cout << std::left
                << std::setw(8) << i
                << std::setw(16) << subtree_sizes[id]
                << std::setw(16) << max_dist[id]
                << std::setw(18) << sum_dist[id]
                << std::setw(18) << value_sum_dp.value(nodes[i])
                << "\n";
    }
  print_rule();

  std::cout << "\nDone.\n";
  return 0;
}
