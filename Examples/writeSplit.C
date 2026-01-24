
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
 * @file writeSplit.C
 * @brief Demonstrates BST split by position operation
 * 
 * This program demonstrates the **split** operation for binary search trees
 * with rank information. It divides a BST at a given position, producing
 * two separate BSTs and generating visualization files. Split is a fundamental
 * operation that enables efficient tree manipulation.
 *
 * ## What is Tree Split?
 *
 * ### Definition
 *
 * The split operation divides a BST at position k, creating:
 * - **Left tree**: Contains elements at positions 0 to k-1 (k smallest elements)
 * - **Right tree**: Contains elements at positions k to n-1 (remaining elements)
 *
 * **Key properties**:
 * - Both resulting trees maintain the BST property
 * - All keys in left tree < all keys in right tree
 * - Original tree is divided at position k
 *
 * ### Visual Example
 *
 * ```
 * Original tree (n=7):      Split at position 3:
 *       4                    Left (0-2):    Right (3-6):
 *      / \                     2               4
 *     2   6                   / \             / \
 *    / \ / \                 1   3           5   6
 *   1  3 5  7                              /     \
 *                                          7
 * ```
 *
 * ## Rank Information
 *
 * ### Why Rank Information is Needed
 *
 * Standard BSTs don't support efficient position-based operations:
 * - **Without rank**: Finding k-th element requires O(n) time
 * - **With rank**: Finding k-th element requires O(log n) time
 *
 * ### BinNodeXt (Extended Binary Node)
 *
 * This example uses `BinNodeXt` which maintains:
 * - **Subtree size**: Number of nodes in subtree rooted at this node
 * - **Order-statistics helpers**: `select(root, k)` and `inorder_position(...)`/`find_position(...)`
 * - **Maintained**: Updated during insert, delete, rotations
 *
 * ### Rank Operations
 *
 * - **`select(root, k)`**: Find k-th smallest element in O(h)
 * - **`inorder_position(root, key, ...)`**: Find inorder position of a key in O(h)
 * - **`find_position(root, key, ...)`**: Find exact/insertion position of a key in O(h)
 * - **Enables**: Efficient position-based splitting
 *
 * ## Algorithm
 *
 * ### Split by Position
 *
 * Split by position works by:
 * ```
 * split_by_position(tree, k):
 *   1. Find node at position k using select(root, k)  // O(h)
 *   2. Disconnect tree at that node
 *   3. Rebuild left subtree (positions 0 to k-1)
 *   4. Rebuild right subtree (positions k+1 to n-1)
 *   5. Return (left_tree, node_at_k, right_tree)
 * ```
 *
 * ### Detailed Steps
 *
 * 1. **Find split node**: Use rank information to find node at position k
 *    - Time: O(h) where h is the tree height
 *
 * 2. **Disconnect**: Remove node from tree structure
 *    - Time: O(h) for this ranked BST representation
 *
 * 3. **Rebuild subtrees**: Construct left and right trees
 *    - Time: O(h)
 *
 * ### Complexity
 *
 * | Tree Type | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Balanced with rank | O(log n) | Height is O(log n) |
 * | Balanced without rank | O(n) | Must traverse to find position |
 * | Unbalanced | O(n) | Worst case (height can be linear) |
 *
 * ## Applications
 *
 * ### Order Statistics
 * - **K-th element**: Find k-th smallest/largest element
 * - **Median**: Find median element efficiently
 * - **Percentiles**: Find elements at specific percentiles
 *
 * ### Range Queries
 * - **Extract range**: Split tree to get elements in range [a, b]
 * - **Range operations**: Process elements in specific range
 * - **Partitioning**: Divide data into ranges
 *
 * ### Tree Operations
 * - **Building block**: Used in many tree algorithms
 * - **Tree manipulation**: Enable complex tree operations
 * - **Functional programming**: Immutable tree operations
 *
 * ### Database Indexing
 * - **Partitioning**: Partition index structures
 * - **Range queries**: Efficient range query processing
 * - **Bulk operations**: Process subsets of data
 *
 * ## Complementary Operations
 *
 * ### Split and Join
 *
 * - **Split**: Divides tree into parts (this operation)
 * - **Join**: Combines trees (see `writeJoin.C`)
 * - **Together**: Enable powerful tree manipulation
 *
 * ### Example: Range Extraction
 *
 * ```
 * // Extract range [a, b] from tree
 * (left, middle, right) = split_3_ways(tree, a, b)
 * // left: elements < a
 * // middle: elements in [a, b]
 * // right: elements > b
 * ```
 *
 * ## Split Variants
 *
 * ### Split by Position
 * - **This example**: Split at position k
 * - **Uses rank**: Requires rank information
 * - **Efficient**: O(log n) with rank
 *
 * ### Split by Value
 * - **Alternative**: Split at value x
 * - **No rank needed**: Can work without rank
 * - **Efficient**: O(log n) for balanced trees
 *
 * ### Three-Way Split
 * - **Advanced**: Split into three parts
 * - **Range extraction**: Extract range [a, b]
 * - **Useful**: For range queries
 *
 * ## Output Files
 *
 * - **`split-before-aux.Tree`**: Original tree with SPLIT marker (preorder)
 *   - Shows tree before split
 *   - Marks split position
 *
 * - **`split-1-aux.Tree`**: Left subtree - positions 0 to pos-1 (preorder)
 *   - Contains k smallest elements
 *   - Maintains BST property
 *
 * - **`split-2-aux.Tree`**: Right subtree - positions pos to n-1 (preorder)
 *   - Contains remaining elements
 *   - Maintains BST property
 *
 * All files can be visualized with `btreepic` to see the split operation.
 *
 * ## Usage
 *
 * ```bash
 * # Split tree with 30 nodes at position 10
 * writeSplit -n 30 -p 10
 *
 * # Use specific seed for reproducibility
 * writeSplit -n 50 -s 42 -p 25
 *
 * # Split at different positions
 * writeSplit -n 100 -p 50  # Split at middle
 * ```
 *
 * @see tpl_binNodeXt.H Extended binary node with rank support
 * @see tpl_binNodeUtils.H BST utility functions including split
 * @see writeJoin.C Complementary join operation
 * @see writeRankTree.C Rank operations demonstration
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

// Output streams
ofstream output;
ofstream output_1;
ofstream output_2;

void print_key(Node* p, int, int)
{
  output << p->get_key() << " ";
}

void print_key1(Node* p, int, int)
{
  output_1 << p->get_key() << " ";
}

void print_key2(Node* p, int, int)
{
  output_2 << p->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate BST split by position operation.\n"
        "Creates a tree and splits it at a given position.",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "count",
                                 "Number of elements",
                                 false, 10, "int");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      TCLAP::ValueArg<int> posArg("p", "position",
                                   "Split position (-1 = middle)",
                                   false, -1, "int");
      cmd.add(posArg);

      cmd.parse(argc, argv);

      int n = nArg.getValue();
      unsigned int t = seedArg.getValue();
      int split_pos = posArg.getValue();

      if (n <= 0)
        {
          cerr << "Error: number of elements must be positive\n";
          return 1;
        }

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== BST Split by Position Demo ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      output.open("split-before-aux.Tree", ios::out);
      output_1.open("split-1-aux.Tree", ios::out);
      output_2.open("split-2-aux.Tree", ios::out);

      if (!output.is_open() || !output_1.is_open() || !output_2.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build ranked tree
      Node* root = Node::NullPtr;
      int value;

      cout << "Building ranked BST with " << n << " elements..." << endl;
      for (int i = 0; i < n; i++)
        {
          while (true)
            {
              value = static_cast<int>(1.0 * n * rand() / (RAND_MAX + 1.0));
              if (searchInBinTree(root, value) == Node::NullPtr)
                break;
            }
          Node* p = new Node(value);
          insert_by_key_xt(root, p);
        }

      int num_nodes = root->getCount();
      cout << "  Nodes: " << num_nodes 
           << ", Height: " << computeHeightRec(root) << endl;

      assert(check_rank_tree(root));
      assert(check_bst(root));

      // Write original tree
      preOrderRec(root, &print_key);

      // Determine split position
      if (split_pos < 0 || split_pos >= num_nodes)
        split_pos = num_nodes / 2;

      cout << endl << "Splitting at position " << split_pos 
           << " (of " << num_nodes << ")..." << endl;

      // Add SPLIT directive for visualization
      output << "SPLIT " << split_pos << " \"\" \"\"" << endl;

      // Perform split
      Node* l;
      Node* r;
      split_pos_rec(root, split_pos, l, r);
      cout << "  ...done" << endl;

      // Verify and write left subtree
      if (l != Node::NullPtr)
        {
          assert(check_rank_tree(l));
          assert(check_bst(l));
          preOrderRec(l, print_key1);
          output_1 << endl;
          cout << endl << "Left subtree:" << endl;
          cout << "  Nodes: " << (l ? l->getCount() : 0) 
               << ", Height: " << computeHeightRec(l) << endl;
        }
      else
        {
          cout << endl << "Left subtree: empty" << endl;
        }

      // Verify and write right subtree
      if (r != Node::NullPtr)
        {
          assert(check_rank_tree(r));
          assert(check_bst(r));
          preOrderRec(r, print_key2);
          output_2 << endl;
          cout << "Right subtree:" << endl;
          cout << "  Nodes: " << (r ? r->getCount() : 0) 
               << ", Height: " << computeHeightRec(r) << endl;
        }
      else
        {
          cout << "Right subtree: empty" << endl;
        }

      // Clean up
      output.close();
      output_1.close();
      output_2.close();
      destroyRec(l);
      destroyRec(r);

      cout << endl << "Generated files:" << endl;
      cout << "  - split-before-aux.Tree (original with split directive)" << endl;
      cout << "  - split-1-aux.Tree (left subtree)" << endl;
      cout << "  - split-2-aux.Tree (right subtree)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
