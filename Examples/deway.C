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
 * @file deway.C
 * @brief Example demonstrating Deway numbering for tree nodes
 * 
 * This example demonstrates the Deway numbering system (also known as
 * Dewey Decimal notation), a hierarchical addressing scheme for tree nodes
 * where each node's address encodes its complete path from the root. This
 * addressing scheme is fundamental to many hierarchical systems.
 *
 * ## What is Deway Numbering?
 *
### Definition
 *
 * Deway numbering assigns each node a unique address based on its position
 * in the tree. The address is a sequence of integers representing the path
 * from root to the node.
 *
### Visual Example
 *
 * ```
 *        Root (no address or "")
 *       /    \
 *    Node 0    Node 1
 *    /   \      /   \
 * 0.0   0.1  1.0   1.1
 * ```
 *
### Addressing Rules
 *
 * - **Root**: Has no address (empty string "") or special marker
 * - **First child**: Address is parent address + ".0"
 * - **i-th child**: Address is parent address + "." + i
 * - **Uniqueness**: Each node has exactly one unique address
 *
## Key Properties
 *
### Hierarchical Structure
 *
 * - **Path encoding**: Address directly encodes the path from root
 * - **Parent-child relationship**: Can derive parent from child address
 * - **Sibling ordering**: Siblings have sequential addresses
 *
### Uniqueness
 *
 * - **One-to-one**: Each node maps to exactly one address
 * - **Complete**: Every node has an address
 * - **Deterministic**: Same tree structure → same addresses
 *
### Navigation
 *
 * - **Find node**: Given address, can navigate directly to node
 * - **Find parent**: Remove last component of address
 * - **Find children**: Append child indices to address
 *
## Applications
 *
### File Systems
 * - **Directory paths**: `/usr/bin/gcc` is Deway-like addressing
 * - **Hierarchical storage**: Organize files in tree structure
 * - **Path resolution**: Resolve paths to files/directories
 *
### XML/HTML Processing
 * - **XPath expressions**: `/html/body/div[0]` uses Deway-like addressing
 * - **DOM navigation**: Navigate document tree structure
 * - **Tree queries**: Query nodes by path
 *
### Database Indexing
 * - **Hierarchical keys**: Index hierarchical data
 * - **B-tree keys**: Some B-tree implementations use similar addressing
 * - **Tree-structured data**: Index tree data efficiently
 *
### Tree Serialization
 * - **Compact representation**: Encode tree structure compactly
 * - **Network transmission**: Send tree structure efficiently
 * - **Storage**: Store tree structure in database
 *
### Tree Navigation
 * - **Direct addressing**: Access nodes by address
 * - **Path queries**: Find nodes matching path pattern
 * - **Tree algorithms**: Many algorithms use path-based addressing
 *
## Algorithm
 *
### Process
 *
 * This example:
 * ```
 * 1. Generate random binary tree with n nodes
 * 2. Convert binary tree to general tree (forest) structure
 * 3. Recursively compute Deway addresses:
 *    deway(node, prefix, depth):
 *      - Print current address (prefix)
 *      - For each child i:
 *        - prefix[depth] = i
 *        - deway(child, prefix, depth+1)
 * ```
 *
### Recursive Computation
 *
 * ```
 * compute_deway(node, parent_address):
 *   if node is root:
 *     address = ""
 *   else:
 *     address = parent_address + "." + child_index
 *   
 *   print(node, address)
 *   for each child i:
 *     compute_deway(child[i], address)
 * ```
 *
## Example Output
 *
### Tree Structure
 *
 * ```
 *        A
 *       / \
 *      B   C
 *     / \   \
 *    D   E   F
 * ```
 *
### Deway Addresses
 *
 * ```
 * Root: ""
 * Node 0: "0"      (B)
 * Node 0.0: "0.0"  (D)
 * Node 0.1: "0.1"  (E)
 * Node 1: "1"      (C)
 * Node 1.0: "1.0"  (F)
 * ```
 *
## Comparison with Other Addressing
 *
 * | Scheme | Format | Example | Use Case |
 * |--------|--------|---------|----------|
 * | Deway | Sequence | "0.1.2" | Tree nodes |
 * | File path | String | "/usr/bin" | File systems |
 * | XPath | String | "/html/body" | XML/HTML |
 * | Array index | Integer | [0][1][2] | Arrays |
 *
## Properties of Deway Addresses
 *
### Advantages
 *
 * ✅ **Hierarchical**: Reflects tree structure
 * ✅ **Unique**: Each node has unique address
 * ✅ **Navigable**: Can navigate using address
 * ✅ **Compact**: Efficient representation
 *
### Limitations
 *
 * ❌ **Dynamic trees**: Addresses change when tree structure changes
 * ❌ **Not stable**: Insertion/deletion changes addresses
 * ❌ **Order-dependent**: Depends on insertion order
 *
## Usage
 *
 * ```bash
 * # Generate tree and show Deway addresses
 * deway -n 20
 *
 * # Use specific seed
 * deway -n 30 -s 42
 * ```
 *
## Related Concepts
 *
### Tree Traversal
 *
 * Deway numbering is related to tree traversal:
 * - **Preorder**: Visit nodes in Deway order
 * - **Depth-first**: Follows depth-first pattern
 *
### Path Representation
 *
 * - **String paths**: Convert to string representation
 * - **Array paths**: Store as integer array
 * - **Compact encoding**: Encode efficiently
 *
 * @see generate_tree.H Tree generation utilities
 * @see tpl_tree_node.H N-ary tree node structure
 * @see generate_forest.C Forest generation (related)
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
