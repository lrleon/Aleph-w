
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
*/


/**
 * @file writeBalance.C
 * @brief Demonstrates BST balancing operation (recursive median selection with rotations)
 * 
 * This program demonstrates `balance_tree()` from `tpl_balanceXt.H`, which
 * rebalances a BST by repeatedly selecting the median (by inorder position)
 * and rotating it up to become the root, then recursing on the left and right
 * subtrees.
 *
 * It generates visualization files showing the transformation from an
 * unbalanced BST to a size-balanced tree.
 *
 * ## Why Balance Trees?
 *
 * ### The Problem with Unbalanced Trees
 *
 * Unbalanced trees degrade to linked lists in worst case:
 * - **Worst case**: O(n) search time instead of O(log n)
 * - **Performance**: Degrades significantly
 * - **Cache**: Poor cache locality
 *
 * ### Benefits of Balancing
 *
 * Balancing ensures:
 * - **Optimal performance**: O(log n) search, insert, delete operations
 * - **Predictable**: Consistent performance characteristics
 * - **Cache friendly**: Better memory access patterns
 * - **Height**: Minimum possible height
 *
 * ## Algorithm Overview
 *
 * The algorithm used in this example is:
 *
 * 1. Select the node at inorder position `n/2`.
 * 2. Rotate it up until it becomes the root.
 * 3. Recurse on left and right subtrees.
 *
 * In Aleph-w this is implemented by `select_gotoup_root()` + `balance_tree()`.
 *
 * ### Total Complexity
 *
 * - **Time**: O(n log n)
 * - **Space**: O(1) - constant extra space (in-place)
 * - **Rotations**: O(n log n) in the worst case
 *
 * ## Perfect Balance
 *
 * ### Definition
 *
 * The routine used here produces a **size-balanced** tree:
 * - For each node, the difference between the cardinalities of its left and
 *   right subtrees is at most 1.
 * - This yields a height that is O(log n) (so searches become logarithmic),
 *   but it does not require building a complete/perfect tree level-by-level.
 *
 * ### Example
 *
 * ```
 * Unbalanced (height 4):    Balanced (height 3):
 *       1                         4
 *        \                       / \
 *         2                     2   6
 *          \                   / \ / \
 *           3                 1 3 5 7
 *            \
 *             4
 * ```
 *
 * ## Comparison with Other Balancing Methods
 *
 * | Method | Time | Space | Result | Notes |
 * |--------|------|-------|--------|-------|
 * | Median-rotations (`balance_tree`) | O(n log n) | O(1) | Size-balanced | Implemented by `tpl_balanceXt.H` |
 * | AVL | O(n log n) | O(log n) | Height-balanced | Maintains during ops |
 * | Red-Black | O(n log n) | O(log n) | Relaxed balance | Maintains during ops |
 * | Rebuild | O(n) | O(n) | Perfect | Requires extra memory |
 *
 * ## Applications
 *
 * ### Tree Optimization
 * - **Improve performance**: Optimize existing BSTs
 * - **One-time operation**: Balance tree before heavy queries
 * - **Legacy code**: Improve performance without rewriting
 *
 * ### Educational
 * - **Visualize balancing**: See tree transformation
 * - **Learn algorithms**: Understand balancing techniques
 * - **Algorithm study**: Compare balancing methods
 *
 * ### Data Structure Conversion
 * - **Prepare trees**: Optimize trees for operations
 * - **Format conversion**: Convert between tree formats
 * - **Preprocessing**: Prepare data for algorithms
 *
 * ### Performance Tuning
 * - **Query optimization**: Optimize trees before queries
 * - **Batch processing**: Balance after bulk insertions
 * - **Maintenance**: Periodic tree optimization
 *
 * ## Output Files
 *
 * - **`balance-before-aux.Tree`**: Original unbalanced tree (preorder)
 *   - Shows tree before balancing
 *   - May have high height
 *
 * - **`balance-after-aux.Tree`**: Perfectly balanced tree (preorder)
 *   - Shows tree after DSW algorithm
 *   - Minimum height achieved
 *
 * Both files can be visualized with `btreepic` tool to see the transformation.
 *
 * ## Usage
 *
 * ```bash
 * # Generate balanced tree with 50 nodes
 * writeBalance -n 50
 *
 * # Use specific seed for reproducibility
 * writeBalance -n 100 -s 12345
 *
 * # Generate larger tree
 * writeBalance -n 200
 * ```
 *
 * ## Algorithm Properties
 *
 * ### Advantages
 * - **Efficient**: O(n) time complexity
 * - **In-place**: O(1) extra space
 * - **Simple**: Easy to understand and implement
 * - **Perfect balance**: Achieves optimal height
 *
 * ### Limitations
 * - **One-time**: Doesn't maintain balance during operations
 * - **Rotations**: Many rotations may be performed
 * - **Not incremental**: Must rebuild entire tree
 *
 * @see tpl_balanceXt.H DSW balancing implementation
 * @see btreepic.C Visualization tool
 * @see timeAllTree.C Tree performance comparison (includes balanced trees)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>

using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;

// Output streams
ofstream outputb;
ofstream outputa;

void print_keyb(Node* p, int, int)
{
  outputb << p->get_key() << " ";
}

void print_keya(Node* p, int, int)
{
  outputa << p->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate BST balancing (DSW algorithm).\n"
        "Creates an unbalanced tree and shows the result of balancing.",
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

      if (n <= 0)
        {
          cerr << "Error: number of elements must be positive\n";
          return 1;
        }

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== BST Balancing Demo (median rotations) ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      outputb.open("balance-before-aux.Tree", ios::out);
      outputa.open("balance-after-aux.Tree", ios::out);

      if (!outputb.is_open() || !outputa.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build ranked tree (will be unbalanced due to random insertion)
      Node* root = Node::NullPtr;
      int value;

      cout << "Building unbalanced BST with " << n << " elements..." << endl;
      for (int i = 0; i < n; i++)
        {
          while (true)
            {
              value = static_cast<int>(100.0 * n * rand() / (RAND_MAX + 1.0));
              if (searchInBinTree(root, value) == Node::NullPtr)
                break;
            }
          Node* p = new Node(value);
          insert_by_key_xt(root, p);
        }

      assert(check_rank_tree(root));

      int height_before = computeHeightRec(root);
      int node_count = root->getCount();
      cout << "Before balancing:" << endl;
      cout << "  Nodes: " << node_count << ", Height: " << height_before << endl;
      cout << "  Approx. optimal height would be: " << (static_cast<int>(log2(node_count)) + 1) << endl;

      // Write tree before balancing
      preOrderRec(root, &print_keyb);
      outputb << endl;

      // Balance the tree
      cout << endl << "Balancing tree by median selection + rotations..." << endl;
      root = balance_tree(root);

      assert(check_rank_tree(root));

      int height_after = computeHeightRec(root);
      cout << "After balancing:" << endl;
      cout << "  Nodes: " << root->getCount() << ", Height: " << height_after << endl;
      cout << "  Height reduction: " << (height_before - height_after) << " levels" << endl;

      // Write tree after balancing
      preOrderRec(root, &print_keya);
      outputa << endl;

      // Clean up
      outputb.close();
      outputa.close();
      destroyRec(root);

      cout << endl << "Generated files:" << endl;
      cout << "  - balance-before-aux.Tree (original unbalanced)" << endl;
      cout << "  - balance-after-aux.Tree (after balancing)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
