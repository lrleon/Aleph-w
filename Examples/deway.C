
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
 * @file deway.C
 * @brief Dewey ("deway") numbering for tree nodes (path-based hierarchical addresses).
 *
 * ## Overview
 *
 * This example demonstrates **Dewey / Deway numbering**: assign each node in a
 * rooted tree a hierarchical address that encodes its full path from the root.
 *
 * Conceptually, each node address is a sequence of child indices. For instance:
 *
 * - root: empty address
 * - first child: `0`
 * - second child of the first child: `0.1`
 *
 * ## Data model used by this example
 *
 * This file builds and prints multiple representations:
 *
 * - a random **binary search tree** using `BinNode<int>`
 * - a converted **forest / general tree** using `Tree_Node<int>`
 *
 * Then it prints the Dewey numbering on the forest.
 *
 * ## Usage / CLI
 *
 * This example uses TCLAP:
 *
 * - `--nodes` / `-n <int>`: number of nodes (default: 10).
 * - `--seed` / `-s <unsigned>`: RNG seed (`0` means use `time()`; default: 0).
 * - `--help`: show help.
 *
 * ```bash
 * ./deway
 * ./deway --nodes 20
 * ./deway --nodes 30 --seed 42
 * ./deway --help
 * ```
 *
 * ## Algorithm
 *
 * 1. Build a random BST by recursive insertion in a numeric range.
 * 2. Convert the binary tree to a forest (`bin_to_forest`).
 * 3. Fix the `is_root` flag for sibling roots created by the conversion.
 * 4. Print traversals and finally compute and print Dewey numbers.
 *
 * ## Complexity
 *
 * - Tree generation and traversals are linear in the number of nodes: `O(n)`.
 * - Dewey numbering visits each node once: `O(n)`.
 *
 * ## Pitfalls and edge cases
 *
 * - Dewey addresses are **not stable** under insertions/removals: changing sibling order
 *   changes addresses.
 * - The initial tree is a BST built from random values; its height can vary.
 *
 * ## References / see also
 *
 * - `generate_forest.C` (related forest utilities)
 * - `tpl_tree_node.H` (n-ary tree nodes)
 *
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <iostream>
# include <tclap/CmdLine.h>
# include <tpl_binNodeUtils.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>
# include <ah-errors.H>

using namespace std;
using namespace Aleph;

/**
 * @brief Recursively compute and print Deway numbering for a tree node
 * 
 * @param p Current node
 * @param prefix Array storing the current Deway address
 * @param len Current depth in the tree
 * @param dim Maximum dimension of prefix array
 */
void deway(Tree_Node<int> * p, int prefix[], const int & len, 
           const size_t & dim)
{
  int i = 1;

  if (len == 0)
    cout << "Root ";
  else
    cout << "Node " << prefix[i++];
  
  while (i <= len)
    cout << "." << prefix[i++];

  cout << " \"" << p->get_key() << "\"" << endl;
  
  if (static_cast<size_t>(len) >= dim)
    ah_overflow_error_if(true) << "Array dimension is smaller than Deway chain";
  
  Tree_Node<int> * child = p->get_left_child(); 
  
  for (int j = 0; child != nullptr; ++j, child = child->get_right_sibling())
    {
      prefix[len + 1] = j;
      deway(child, prefix, len + 1, dim);
    }
}

/**
 * @brief Print Deway numbering for a forest
 * 
 * @param p Root of the first tree in the forest
 * @param h Height of the original binary tree (used to size prefix array)
 */
void deway(Tree_Node<int> * p, const int & h)
{
  const size_t dim = 10 * h;

  int * prefix = new int [dim];

  for (int i = 0; p != nullptr; ++i, p = p->get_right_sibling())
    {
      prefix[0] = i;
      deway(p, prefix, 0, dim);
    }

  delete [] prefix;
}

template <class Node>
static void printNode(Node * node, int, int)
{ 
  cout << " " << node->get_key();
}

/**
 * @brief Generate a random integer in range [l, r]
 */
int random_int(int l, int r)
{
  assert(l <= r);
  const int n = r - l;
  const int rd = 1 + static_cast<int>(1.0 * n * rand() / (RAND_MAX + 1.0));
  return l + rd - 1;
}

/**
 * @brief Recursively build a random binary search tree
 */
BinNode<int> * random_tree(int l, int r)
{
  if (l > r)
    return nullptr;

  auto * root = new BinNode<int>(random_int(l, r));

  LLINK(root) = random_tree(l, KEY(root) - 1);
  RLINK(root) = random_tree(KEY(root) + 1, r);

  return root;
}

int main(int argc, char * argv[])
{
  try
    {
      TCLAP::CmdLine cmd("Deway numbering example for trees", ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "nodes", 
                                 "Number of nodes in the tree",
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

      cout << "Deway Numbering Example" << endl;
      cout << "=======================" << endl;
      cout << "Parameters: n=" << n << ", seed=" << t << endl << endl;

      // Generate random binary tree
      BinNode<int> * bp = random_tree(1, n);

      cout << "Binary tree (preorder):";
      preOrderRec(bp, printNode);
      cout << endl << endl;
      
      cout << "Binary tree (inorder):";
      inOrderRec(bp, printNode);
      cout << endl << endl;
      
      // Convert to forest
      Tree_Node<int> * tree = bin_to_forest<Tree_Node<int>, BinNode<int>>(bp);

      // bin_to_forest creates a forest where sibling trees are linked,
      // but the is_root flag is not set correctly for siblings.
      // We fix this by setting is_root=true for all sibling trees.
      for (auto* t = tree; t != nullptr; t = t->get_right_sibling())
        t->set_is_root(true);

      cout << "Forest (preorder):";
      forest_preorder_traversal(tree, printNode);
      cout << endl << endl;

      cout << "Forest (postorder):";
      forest_postorder_traversal(tree, printNode);
      cout << endl << endl;

      // Verify conversion is reversible
      BinNode<int> * prb = forest_to_bin<Tree_Node<int>, BinNode<int>>(tree);
      assert(areEquivalents(prb, bp));
      cout << "Conversion verification: PASSED" << endl << endl;

      // Print Deway numbering
      cout << "Deway Numbering:" << endl;
      cout << "----------------" << endl;
      deway(tree, computeHeightRec(bp));

      // Cleanup
      destroyRec(bp);
      destroyRec(prb);
      destroy_forest(tree);

      cout << endl << "Done." << endl;
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
