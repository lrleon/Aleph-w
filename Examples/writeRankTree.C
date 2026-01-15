/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
 * @author Leandro Rabindranath León
 * @ingroup Examples
*/

/**
 * @file writeRankTree.C
 * @brief Demonstrates ranked BST with subtree counts
 * 
 * This program demonstrates **order statistics** in binary search trees
 * using extended nodes (`BinNodeXt`) that maintain subtree size information.
 * Order statistics enable efficient answers to "what is the k-th element?"
 * and "what is the position of element x?" queries.
 *
 * ## What are Order Statistics?
 *
 * ### Problem Statement
 *
 * Order statistics operations answer questions like:
 * - **select(k)**: "What is the k-th smallest element?" (0-indexed)
 * - **rank(x)**: "What is the position of element x?" (how many elements < x)
 *
 * **Example**:
 * ```
 * Tree: [1, 3, 5, 7, 9]
 * select(2) = 5  (3rd smallest element)
 * rank(5) = 2    (5 is at position 2)
 * ```
 *
 * ### Standard BST Limitation
 *
 * Standard BSTs require O(n) time for these operations:
 * - Must traverse entire tree to count elements
 * - No efficient way to find k-th element
 * - Rank queries require full traversal
 *
 * ### Ranked BST Solution
 *
 * Ranked BSTs achieve O(log n) for order statistics:
 * - Maintain subtree size at each node
 * - Use size information to navigate efficiently
 * - Achieve logarithmic time complexity
 *
 * ## Extended Binary Node (BinNodeXt)
 *
 * ### What is BinNodeXt?
 *
 * `BinNodeXt` extends standard binary nodes with:
 * - **Subtree count**: Number of nodes in subtree rooted at this node
 * - **Maintained during**: Insert, delete, rotations
 * - **Updated in**: O(1) per operation (amortized)
 * - **Extra memory**: One integer per node
 *
 * ### Maintaining Subtree Size
 *
 * Subtree size is updated during:
 * - **Insertion**: Increment sizes along path to insertion point
 * - **Deletion**: Decrement sizes along path
 * - **Rotations**: Recalculate sizes after rotation
 *
 * **Cost**: O(1) amortized per operation
 *
 * ## How It Works
 *
 * ### Select Operation (find k-th smallest)
 *
 * ```
 * select(node, k):
 *   left_size = size(left_child)  // O(1) lookup
 *   if k < left_size:
 *     return select(left_child, k)  // k-th is in left subtree
 *   else if k == left_size:
 *     return node.key  // Found! This is the k-th element
 *   else:
 *     return select(right_child, k - left_size - 1)  // k-th is in right
 * ```
 *
 * **Time**: O(log n) - height of tree
 * **Key insight**: Use subtree size to decide which subtree contains k-th element
 *
 * ### Rank Operation (find position of x)
 *
 * ```
 * rank(node, x):
 *   if x < node.key:
 *     return rank(left_child, x)  // x is in left subtree
 *   else if x == node.key:
 *     return size(left_child)  // Found! Position = left subtree size
 *   else:
 *     return size(left_child) + 1 + rank(right_child, x)  // x in right
 * ```
 *
 * **Time**: O(log n) - height of tree
 * **Key insight**: Count elements smaller than x
 *
 * ## Complexity Comparison
 *
 * | Operation | Standard BST | Ranked BST | Improvement |
 * |-----------|--------------|------------|-------------|
 * | select(k) | O(n) | O(log n) | **Exponential** |
 * | rank(x) | O(n) | O(log n) | **Exponential** |
 * | insert | O(log n) | O(log n) | Same |
 * | delete | O(log n) | O(log n) | Same |
 * | search | O(log n) | O(log n) | Same |
 *
 * **Trade-off**: Slight overhead (one integer per node) for huge speedup
 *
 * ## Applications
 *
 * ### Statistics and Percentiles
 * - **Median finding**: `select(n/2)` in O(log n)
 * - **Percentiles**: `select(k)` for various k values
 * - **Quartiles**: Find 25th, 50th, 75th percentiles
 *
 * ### Range Queries
 * - **Count in range**: `rank(b) - rank(a)` gives count in [a, b]
 * - **Range extraction**: Extract elements in range efficiently
 * - **Database queries**: Range queries on indexed data
 *
 * ### Database Indexing
 * - **Order statistics**: Efficient k-th element queries
 * - **Rank queries**: Find position of values
 * - **Index operations**: Support complex queries
 *
 * ### Competitive Programming
 * - **K-th element**: Common problem type
 * - **Order statistics**: Frequently needed
 * - **Efficient solution**: O(log n) instead of O(n)
 *
 * ### Data Analysis
 * - **Top-k queries**: Find k largest/smallest
 * - **Ranking**: Rank elements by value
 * - **Statistical analysis**: Percentiles, medians
 *
 * ## Example Use Cases
 *
 * ### Finding Median
 * ```cpp
 * int median = tree.select(tree.size() / 2);
 * ```
 *
 * ### Counting Elements in Range
 * ```cpp
 * int count = tree.rank(b) - tree.rank(a);  // Elements in [a, b)
 * ```
 *
 * ### Finding Percentile
 * ```cpp
 * int p90 = tree.select(tree.size() * 9 / 10);  // 90th percentile
 * ```
 *
 * ## Output Files
 *
 * - **`rank-tree-aux.Tree`**: Tree visualization with:
 *   - **Preorder traversal**: Keys in preorder
 *   - **START-AUX section**: Subtree counts (inorder)
 *   - **Position tags**: For `btreepic` visualization
 *
 * The visualization shows both:
 * - **Tree structure**: Binary search tree layout
 * - **Rank information**: Subtree counts at each node
 *
 * ## Usage
 *
 * ```bash
 * # Generate ranked tree with 25 nodes
 * writeRankTree -n 25
 *
 * # Use specific seed
 * writeRankTree -n 50 -s 42
 * ```
 *
 * @see tpl_binNodeXt.H Extended binary node with rank support
 * @see tpl_binNodeUtils.H BST utility functions (uses rank)
 * @see writeSplit.C Split operation (uses rank)
 * @see btreepic.C Visualization tool
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNodeXt.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binTree.H>

using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;

ofstream output;

void print_key(Node* p, int, int)
{
  output << p->get_key() << " ";
}

void print_count(Node* p, int, int)
{
  output << p->getCount() << " ";
}

void print_tag(Node*, int, int pos)
{
  output << "tag " << pos << " " << pos << " N -15 35" << endl;
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate ranked BST with subtree counts.\n"
        "Creates a tree where each node stores the size of its subtree.",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "count",
                                 "Number of elements",
                                 false, 10, "int");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      cmd.parse(argc, argv);

      int n = nArg.getValue();
      unsigned int t = seedArg.getValue();

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== Ranked BST Demo ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output file
      output.open("rank-tree-aux.Tree", ios::out);
      if (!output.is_open())
        {
          cerr << "Error: cannot open output file" << endl;
          return 1;
        }

      // Build ranked tree
      Node* root = Node::NullPtr;

      cout << "Building ranked BST..." << endl;
      cout << "Values: ";
      for (int i = 0; i < n; i++)
        {
          int value;
          while (true)
            {
              value = static_cast<int>(10.0 * n * rand() / (RAND_MAX + 1.0));
              if (searchInBinTree(root, value) == Node::NullPtr)
                break;
            }

          Node* p = new Node(value);
          insert_by_key_xt(root, p);
          cout << value << " ";
        }
      cout << endl << endl;

      assert(check_rank_tree(root));
      assert(check_bst(root));

      // Display tree statistics
      cout << "Tree statistics:" << endl;
      cout << "  Total nodes: " << root->getCount() << endl;
      cout << "  Height: " << computeHeightRec(root) << endl;
      cout << "  Root key: " << KEY(root) << endl;
      cout << "  Root count: " << root->getCount() << endl;

      // Demonstrate select operations
      cout << endl << "Order statistics (select):" << endl;
      for (int i = 0; i < min(n, 5); i++)
        {
          Node* sel = select(root, i);
          cout << "  Position " << i << ": " << KEY(sel) << endl;
        }

      // Write preorder traversal of keys
      preOrderRec(root, &print_key);

      // Write auxiliary section with subtree counts
      output << endl << endl << "START-AUX ";
      inOrderRec(root, &print_count);

      // Write position tags for visualization
      output << endl << endl << "% Position tags (infix order)" << endl << endl;
      inOrderRec(root, &print_tag);
      output << endl;

      output.close();

      cout << endl << "Generated file:" << endl;
      cout << "  - rank-tree-aux.Tree (with subtree counts and position tags)" << endl;

      destroyRec(root);
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
