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
 * @file writeInsertRoot.C
 * @brief Demonstrates root insertion in BST
 * 
 * This program demonstrates **root insertion**, an alternative insertion
 * strategy for binary search trees. Unlike standard insertion (which places
 * new elements at leaves), root insertion always places new elements at the
 * root using rotations. This creates a self-adjusting tree structure that
 * adapts to insertion patterns.
 *
 * ## Standard vs Root Insertion
 *
 * ### Standard Insertion
 *
 * - **Traversal**: From root to leaf
 * - **Insertion**: New node becomes leaf
 * - **Time**: O(log n) average, O(n) worst case
 * - **Result**: New element at bottom of tree
 * - **Structure**: Tree grows downward
 *
 * ### Root Insertion
 *
 * - **Traversal**: From root to insertion point
 * - **Insertion**: New node inserted at correct position
 * - **Rotation**: Rotate path to bring new element to root
 * - **Time**: O(log n) average, O(n) worst case
 * - **Result**: New element always at root
 * - **Structure**: Tree adjusts upward
 *
 * ## Algorithm
 *
 * ### Root Insertion Process
 *
 * Root insertion works by:
 * ```
 * root_insert(tree, value):
 *   1. Search: Find insertion point (as in standard insertion)
 *   2. Insert: Create new node at that position
 *   3. Rotate up: Perform rotations along path to move new node to root
 *      - While new node is not root:
 *        - If new node is left child: rotate right
 *        - If new node is right child: rotate left
 * ```
 *
 * ### Rotations
 *
 * The rotations maintain the BST property while moving the node upward:
 * - **Right rotation**: Moves left child up
 * - **Left rotation**: Moves right child up
 * - **BST property**: Maintained throughout
 *
 * ## Properties
 *
 * ### Advantages
 *
 * ✅ **Temporal locality**: Recently inserted elements near root
 * ✅ **Cache friendly**: Frequently accessed elements at top
 * ✅ **Self-adjusting**: Tree adapts to insertion patterns
 * ✅ **Fast access**: Recent elements accessed quickly
 * ✅ **Splay-like**: Similar to splay tree behavior
 *
 * ### Disadvantages
 *
 * ❌ **No balance guarantee**: Tree may become unbalanced
 * ❌ **More rotations**: Extra overhead compared to standard insertion
 * ❌ **Worst case**: Still O(n) height possible
 * ❌ **Overhead**: Rotations add computational cost
 *
 * ## Applications
 *
 * ### Caching
 * - **LRU cache**: Keep recently used items accessible
 * - **Temporal locality**: Exploit access patterns
 * - **Performance**: Fast access to recent items
 *
 * ### Self-Adjusting Structures
 * - **Adapt to patterns**: Tree adapts to insertion order
 * - **Dynamic optimization**: Optimize based on usage
 * - **Learning**: Tree "learns" from insertions
 *
 * ### Educational
 * - **Tree rotations**: Demonstrate rotation operations
 * - **BST variants**: Show alternative insertion strategies
 * - **Algorithm design**: Understand self-adjusting structures
 *
 * ### Foundation for Splay Trees
 * - **Similar concept**: Splay trees extend root insertion
 * - **Access patterns**: Both optimize for temporal locality
 * - **Implementation**: Root insertion is simpler version
 *
 * ## Comparison with Splay Trees
 *
 * | Aspect | Root Insertion | Splay Trees |
 * |--------|----------------|-------------|
 * | When rotates | On insertion | On access |
 * | Rotates | New nodes only | Any accessed node |
 * | Complexity | O(log n) avg | O(log n) amortized |
 * | Balance | No guarantee | No guarantee |
 * | Use case | Insert-heavy | Access-heavy |
 *
 * ### Key Difference
 *
 * - **Splay trees**: Rotate accessed nodes to root (not just inserted)
 * - **Root insertion**: Only rotates newly inserted nodes
 * - **Both**: Provide temporal locality benefits
 *
 * ## Tree Structure Comparison
 *
 * ### Standard Insertion
 * ```
 * Insert sequence: 5, 3, 7, 1, 9
 * Result:
 *       5
 *      / \
 *     3   7
 *    /     \
 *   1       9
 * ```
 *
 * ### Root Insertion
 * ```
 * Insert sequence: 5, 3, 7, 1, 9
 * Result (after each insertion):
 *   5 → 3 → 3 → 1 → 1
 *       5   7   3   3
 *           5   7   7
 *               5   5
 *                   9
 * ```
 * Recent insertions cluster near root!
 *
 * ## Output Files
 *
 * - **`insert_root-aux.Tree`**: Tree structure in preorder format
 *   - Shows tree after root insertion
 *   - Can be visualized with btreepic
 *
 * The visualization shows how recently inserted elements cluster near the root,
 * creating a different structure than standard BST insertion.
 *
 * ## Usage
 *
 * ```bash
 * # Generate tree with root insertion (20 nodes)
 * writeInsertRoot -n 20
 *
 * # Use specific seed for reproducibility
 * writeInsertRoot -n 30 -s 42
 *
 * # Compare with standard insertion
 * write_tree -n 20  # Standard insertion
 * writeInsertRoot -n 20  # Root insertion
 * ```
 *
 * ## Performance Characteristics
 *
 * | Operation | Standard Insertion | Root Insertion |
 * |-----------|-------------------|----------------|
 * | Insert | O(log n) | O(log n) + rotations |
 * | Search (recent) | O(log n) | O(1) to O(log n) |
 * | Search (old) | O(log n) | O(log n) to O(n) |
 * | Balance | No | No |
 *
 * @see tpl_binNodeUtils.H BST utility functions including root insertion
 * @see tpl_splay_tree.H Splay tree (related self-adjusting structure)
 * @see write_tree.C Standard insertion (for comparison)
 * @see btreepic.C Visualization tool
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <ctime>
# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

ofstream output;

static void printNode(BinNode<int>* node, int, int)
{
  output << node->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate root insertion in BST.\n"
        "Shows how root insertion maintains recently inserted elements near the root.",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "count",
                                 "Number of elements",
                                 false, 20, "int");
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

      cout << "=== Root Insertion Demo ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output file
      output.open("insert_root-aux.Tree", ios::out);
      if (!output.is_open())
        {
          cerr << "Error: cannot open output file" << endl;
          return 1;
        }

      BinNode<int>* root = nullptr;
      int inserted_count = 0;
      int last_inserted = -1;

      cout << "Inserting values: ";
      for (int i = 0; i < n; i++)
        {
          int value = static_cast<int>(10.0 * n * rand() / (RAND_MAX + 1.0));

          BinNode<int>* p = new BinNode<int>(value);
          
          // insert_root returns nullptr if duplicate
          while (Aleph::insert_root(root, p) == nullptr)
            {
              KEY(p) = static_cast<int>(10.0 * n * rand() / (RAND_MAX + 1.0));
            }

          cout << KEY(p) << " ";
          last_inserted = KEY(p);
          inserted_count++;
        }
      cout << endl << endl;

      assert(check_bst(root));

      cout << "Statistics:" << endl;
      cout << "  Items inserted: " << inserted_count << endl;
      cout << "  Tree height: " << computeHeightRec(root) << endl;
      cout << "  Last inserted (should be root): " << last_inserted << endl;
      cout << "  Actual root: " << KEY(root) << endl;
      
      if (KEY(root) == last_inserted)
        cout << "  [OK] Root is the last inserted element" << endl;
      else
        cout << "  [Note] Root may have changed due to rotations" << endl;

      // Write preorder traversal
      preOrderRec(root, printNode);
      output << endl;
      output.close();

      cout << endl << "Generated file:" << endl;
      cout << "  - insert_root-aux.Tree" << endl;

      destroyRec(root);
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
