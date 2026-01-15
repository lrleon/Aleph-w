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
 * @file writeJoin.C
 * @brief Demonstrates BST join operation and generates visualization files
 * 
 * This program demonstrates the **join** operation for binary search trees,
 * which combines two BSTs into a single BST while maintaining the BST property.
 * Join is a fundamental operation that enables efficient tree manipulation and
 * is the inverse of the split operation.
 *
 * ## What is Tree Join?
 *
### Definition
 *
 * The join operation combines two BSTs T₁ and T₂ where:
 * - **Precondition**: All keys in T₁ < all keys in T₂ (disjoint key ranges)
 * - **Result**: Single BST containing all keys from both trees
 * - **Property**: BST property maintained in result
 *
### Visual Example
 *
 * ```
 * T₁:        T₂:           Join(T₁, T₂):
 *   3          8               3
 *  / \        / \             / \
 * 1   5      7   9           1   5
 *                              \   \
 *                               2   8
 *                                  / \
 *                                 7   9
 * ```
 *
## Algorithm
 *
### Basic Join Algorithm
 *
 * The join operation works by:
 * ```
 * join(T₁, T₂):
 *   1. Find rightmost node in T₁ (largest key)
 *   2. Make T₂ the right subtree of that node
 *   3. Return T₁ (now containing all nodes)
 * ```
 *
### Detailed Steps
 *
 * 1. **Find rightmost node**: Traverse right child pointers in T₁
 *    - This is the largest key in T₁
 *    - Time: O(h₁) where h₁ is height of T₁
 *
 * 2. **Attach T₂**: Set right child of rightmost node to T₂ root
 *    - Time: O(1)
 *
 * 3. **Maintain balance** (if using balanced trees):
 *    - May need rotations to maintain balance
 *    - Time: O(h₁ + h₂)
 *
### Complexity
 *
 * | Tree Type | Complexity | Notes |
 * |-----------|-----------|-------|
 * | Balanced | O(log n) | Height is O(log n) |
 * | Unbalanced | O(n) | Worst case height is O(n) |
 * | With rank info | O(log n) | Can use rank for efficiency |
 *
## Applications
 *
### Tree Merging
 * - **Combine datasets**: Merge two sorted datasets efficiently
 * - **Set operations**: Implement set union
 * - **Database**: Merge index structures
 *
### Functional Programming
 * - **Immutable operations**: Create new tree without modifying originals
 * - **Persistent data structures**: Maintain history
 * - **Tree manipulation**: Building block for other operations
 *
### Algorithm Building Blocks
 * - **Split-join paradigm**: Used with split for tree manipulation
 * - **Range operations**: Extract and rejoin ranges
 * - **Tree operations**: Implement complex tree algorithms
 *
### Database Operations
 * - **Index merging**: Merge database indexes
 * - **Query optimization**: Combine query results
 * - **Bulk operations**: Efficient bulk updates
 *
## Key Properties
 *
### BST Property Preservation
 *
 * - **Result is valid BST**: All left children < parent < right children
 * - **Inorder traversal**: Gives sorted sequence of all keys
 * - **Search property**: Binary search still works
 *
### Efficiency
 *
 * - **Fast**: O(log n) for balanced trees
 * - **In-place**: Can modify trees in-place (or create new)
 * - **No copying**: Doesn't copy node data
 *
### Building Block
 *
 * Join is used in:
 * - **Split-join**: Inverse of split operation
 * - **Merge operations**: Combining trees
 * - **Range queries**: Extracting and rejoining ranges
 *
## Complementary Operations
 *
### Split and Join
 *
 * - **Split**: Divides tree into two parts (see `writeSplit.C`)
 * - **Join**: Combines two trees into one
 * - **Together**: Enable powerful tree manipulation
 *
### Example: Range Extraction
 *
 * ```
 * // Extract range [a, b]
 * (left, middle, right) = split_3_ways(tree, a, b)
 * result = join(left, middle)
 * final = join(result, right)
 * ```
 *
## Output Files
 *
 * - **`join-1-aux.Tree`**: First BST before join (preorder)
 *   - Contains smaller keys
 *   - Left tree in join operation
 *
 * - **`join-2-aux.Tree`**: Second BST before join (preorder)
 *   - Contains larger keys
 *   - Right tree in join operation
 *
 * - **`join-aux.Tree`**: Resulting joined BST (preorder)
 *   - Contains all keys from both trees
 *   - Maintains BST property
 *
 * All files can be visualized with `btreepic` to see the transformation.
 *
## Usage
 *
 * ```bash
 * # Join two trees with 10 nodes each
 * writeJoin -n 10
 *
 * # Use specific seed for reproducibility
 * writeJoin -n 20 -s 12345
 *
 * # Join larger trees
 * writeJoin -n 50
 * ```
 *
## Algorithm Variants
 *
### Simple Join
 * - **Basic version**: Just attach T₂ to rightmost of T₁
 * - **Fast**: O(h₁) time
 * - **May unbalance**: Doesn't maintain balance
 *
### Balanced Join
 * - **Maintains balance**: Performs rotations if needed
 * - **Slower**: O(h₁ + h₂) time
 * - **Better structure**: Maintains tree balance
 *
### Rank-Based Join
 * - **Uses rank info**: If nodes have subtree counts
 * - **Efficient**: Can optimize with rank information
 * - **Complex**: More complex implementation
 *
 * @see tpl_binNodeUtils.H BST utility functions including join
 * @see writeSplit.C Complementary split operation
 * @see writeRankTree.C Rank operations (useful for join optimization)
 * @see btreepic.C Visualization tool
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

using namespace Aleph;
using namespace std;

typedef BinNode<int> Node;

// Output streams for the three trees
ofstream output;
ofstream output1;
ofstream output2;

void print_key(Node * p, int, int)
{
  output << p->get_key() << " ";
}

void print_key1(Node * p, int, int)
{
  output1 << p->get_key() << " ";
}

void print_key2(Node * p, int, int)
{
  output2 << p->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate BST join operation.\n"
        "Creates two BSTs and joins them into one, generating visualization files.",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "count",
                                 "Number of elements per tree (total = 2n)",
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

      cout << "=== BST Join Operation Demo ===" << endl;
      cout << "Elements per tree: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      output.open("join-aux.Tree", ios::out);
      output1.open("join-1-aux.Tree", ios::out);
      output2.open("join-2-aux.Tree", ios::out);

      if (!output.is_open() || !output1.is_open() || !output2.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build first tree with n/2 elements
      int value;
      Node* root1 = Node::NullPtr;

      cout << "Building first tree with " << n/2 << " elements..." << endl;
      for (int i = 0; i < n/2; i++)
        {
          while (true)
            {
              value = rand() % (n * 100) + 1;
              if (searchInBinTree(root1, value) == nullptr)
                break;
            }
          Node* p = new Node(value);
          insert_root(root1, p);
        }

      assert(check_bst(root1));
      int n1 = preOrderRec(root1, &print_key1);
      output1 << endl;
      
      cout << "  Nodes: " << n1 
           << ", Height: " << computeHeightRec(root1) << endl;

      // Build second tree with n/2 elements (non-overlapping keys)
      Node* root2 = Node::NullPtr;

      cout << "Building second tree with " << n/2 << " elements..." << endl;
      for (int i = 0; i < n/2; i++)
        {
          while (true)
            {
              value = rand() % (n * 100) + 1;
              if (searchInBinTree(root1, value) == nullptr &&
                  searchInBinTree(root2, value) == nullptr)
                break;
            }
          Node* p = new Node(value);
          insert_in_bst(root2, p);
        }

      assert(check_bst(root2));
      int n2 = preOrderRec(root2, &print_key2);
      output2 << endl;
      
      cout << "  Nodes: " << n2 
           << ", Height: " << computeHeightRec(root2) << endl;

      // Join the two trees
      cout << endl << "Joining trees..." << endl;
      Node* dup = nullptr;
      Node* root = join(root1, root2, dup);

      if (dup != nullptr)
        {
          cout << "Warning: duplicates found (unexpected)" << endl;
        }

      assert(check_bst(root));
      int n_total = preOrderRec(root, &print_key);
      output << endl;
      
      cout << "Resulting tree:" << endl;
      cout << "  Nodes: " << n_total 
           << ", Height: " << computeHeightRec(root) << endl;

      // Clean up
      output.close();
      output1.close();
      output2.close();
      destroyRec(root);

      cout << endl << "Generated files:" << endl;
      cout << "  - join-1-aux.Tree (first tree)" << endl;
      cout << "  - join-2-aux.Tree (second tree)" << endl;
      cout << "  - join-aux.Tree (joined result)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
