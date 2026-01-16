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
 * @brief Ranked BST demo using `BinNodeXt` subtree sizes (select/order-statistics + visualization output).
 *
 * ## Overview
 *
 * This example demonstrates a *ranked* binary search tree: each node stores the
 * size of its subtree, enabling **order statistics** operations such as:
 *
 * - `select(root, k)` — return the node at inorder position `k`.
 * - rank/position queries via helpers in `tpl_binNodeUtils.H`.
 *
 * This file builds a plain BST (not self-balancing) using `insert_by_key_xt()`.
 * Therefore, asymptotic costs are expressed in terms of the tree height **h**.
 *
 * ## Data model used by this example
 *
 * - **Node type**: `Node = BinNodeXt<int>`
 * - **Key type**: `int`
 * - **Rank field**: `Node::getCount()` stores subtree size.
 *
 * ## Usage / CLI
 *
 * This example uses TCLAP:
 *
 * - `--count` / `-n <int>`: number of inserted elements (default: 10).
 * - `--seed` / `-s <unsigned>`: RNG seed (`0` means use `time()`; default: 0).
 * - `--help`: show help.
 *
 * ```bash
 * # Generate ranked BST with 25 nodes
 * ./writeRankTree --count 25
 *
 * # Use a specific seed
 * ./writeRankTree --count 50 --seed 42
 *
 * # Show help
 * ./writeRankTree --help
 * ```
 *
 * ## Algorithm / operations
 *
 * - **Insertion**: `insert_by_key_xt(root, new Node(key))` updates subtree counts.
 * - **Select**: `select(root, k)` uses left-subtree sizes to descend.
 *
 * ## Output
 *
 * This example generates one file:
 *
 * - `rank-tree-aux.Tree`
 *
 * The file contains:
 *
 * - A preorder traversal of keys.
 * - A `START-AUX` section with inorder traversal of subtree counts.
 * - Position tags (inorder) intended for `btreepic`-style visualization.
 *
 * ## Complexity
 *
 * Let **h** be the tree height.
 *
 * - insert/search/remove: `O(h)`
 * - `select(k)` / rank queries: `O(h)`
 *
 * If the tree were balanced, then `h = O(log n)`. In this example (plain BST)
 * `h` can be `O(n)` in the worst case.
 *
 * ## Pitfalls and edge cases
 *
 * - For sorted/near-sorted insert sequences, the BST can become degenerate.
 * - Output is overwritten: `rank-tree-aux.Tree` is opened for writing each run.
 *
 * ## References / see also
 *
 * - `tpl_binNodeXt.H` (ranked node)
 * - `tpl_binNodeUtils.H` (order-statistics utilities)
 * - `writeSplit.C` (split operation uses ranks)
 * - `btreepic.C` (visualization)
 *
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
