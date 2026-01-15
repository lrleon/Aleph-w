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
 * @file writeBalance.C
 * @brief Demonstrates BST balancing operation (DSW algorithm)
 * 
 * This program demonstrates the Day-Stout-Warren (DSW) algorithm for
 * converting an unbalanced binary search tree into a perfectly balanced
 * tree. It generates visualization files showing the transformation from
 * an unbalanced tree to a perfectly balanced one.
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
 * ## DSW Algorithm Overview
 *
 * ### Day-Stout-Warren Algorithm
 *
 * The DSW algorithm is an elegant O(n) time, O(1) space algorithm for
 * balancing binary search trees. It works in two phases:
 *
 * ### Phase 1: Create a Vine (Right-Skewed Tree)
 *
 * **Goal**: Convert tree to a linear chain (all nodes have only right children)
 *
 * **Algorithm**:
 * ```
 * create_vine(root):
 *   current = root
 *   While current has left child:
 *     Rotate right at current
 *     current = current.parent
 *   current = current.right
 * ```
 *
 * **Result**: All nodes form a right-skewed chain
 * **Time**: O(n) - visit each node once
 *
 * ### Phase 2: Balance the Vine
 *
 * **Goal**: Transform vine into perfectly balanced tree
 *
 * **Algorithm**:
 * ```
 * balance_vine(vine_root, n):
 *   m = 2^floor(log2(n+1)) - 1  // Number of nodes in perfect tree
 *   compress(vine_root, n - m)  // Compress excess nodes
 *   While m > 1:
 *     compress(vine_root, m/2)
 *     m = m/2
 * ```
 *
 * **Compress operation**: Performs left rotations in a pattern
 * **Time**: O(n) - perform O(n) rotations
 *
 * ### Total Complexity
 *
 * - **Time**: O(n) - linear time algorithm
 * - **Space**: O(1) - constant extra space (in-place)
 * - **Rotations**: O(n) rotations performed
 *
 * ## Perfect Balance
 *
 * ### Definition
 *
 * A **perfectly balanced** tree has:
 * - **Height**: ⌊log₂(n)⌋ or ⌈log₂(n)⌉ (minimum possible)
 * - **Structure**: All levels full except possibly the last
 * - **Performance**: Optimal search performance
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
 * | DSW | O(n) | O(1) | Perfect | In-place, simple |
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

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== BST Balancing Demo (DSW Algorithm) ===" << endl;
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
      cout << "  Optimal height would be: " << static_cast<int>(log2(n)) << endl;

      // Write tree before balancing
      preOrderRec(root, &print_keyb);
      outputb << endl;

      // Balance the tree
      cout << endl << "Applying DSW balancing algorithm..." << endl;
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
