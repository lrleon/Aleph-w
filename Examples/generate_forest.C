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
 * @file generate_forest.C
 * @brief Example demonstrating forest generation from binary trees
 * 
 * This example demonstrates converting a binary search tree into a
 * forest (collection of trees) representation. A forest is useful for
 * representing hierarchical structures where nodes can have multiple
 * children (n-ary trees), which is more flexible than binary trees.
 *
 * ## What is a Forest?
 *
### Graph Theory Definition
 *
 * A **forest** is a collection of trees. In graph theory:
 * - A **tree** is a connected acyclic graph
 * - A **forest** is a disjoint union of trees (may be disconnected)
 * - Forest = collection of trees with no cycles
 *
### In This Context
 *
 * We convert a **binary tree** (each node has at most 2 children) to a
 * **general tree/forest** (each node can have any number of children):
 *
 * - **Binary tree**: Limited to 2 children per node
 * - **General tree**: Unlimited children per node
 * - **Forest**: Collection of general trees
 *
## Conversion Process
 *
### Algorithm Steps
 *
 * The example performs the following:
 *
 * 1. **Generate binary tree**: Creates a random binary search tree with n nodes
 * 2. **Traverse binary tree**: Visits all nodes in the binary tree
 * 3. **Convert structure**: Transforms binary tree to general tree:
 *    - Binary tree nodes → General tree nodes
 *    - Binary tree edges → General tree parent-child relationships
 * 4. **Output forest**: Writes forest structure in suitable format
 *
### Conversion Details
 *
 * The conversion preserves the hierarchical structure:
 * - **Root nodes**: Binary tree root becomes forest root
 * - **Child relationships**: Binary tree children become forest children
 * - **Node data**: Key values preserved
 * - **Structure**: Hierarchical relationships maintained
 *
## Applications
 *
### Tree Visualization
 * - **Format conversion**: Convert between tree representations
 * - **Visualization tools**: Some tools work better with n-ary trees
 * - **Display**: More flexible display options
 *
### Data Structure Conversion
 * - **Adapting algorithms**: Algorithms designed for n-ary trees
 * - **Flexibility**: General trees more flexible than binary
 * - **Representation**: Better representation for some problems
 *
### Algorithm Testing
 * - **Test cases**: Generate test cases for tree algorithms
 * - **Format compatibility**: Test algorithms on different formats
 * - **Validation**: Verify algorithm correctness on different structures
 *
### Format Conversion
 * - **Interoperability**: Convert between different tree formats
 * - **Data exchange**: Exchange tree data between systems
 * - **Standardization**: Convert to standard formats
 *
## Forest Properties
 *
### Structure
 * - **Multiple roots**: Forest can have multiple root nodes
 * - **Disconnected**: Trees in forest are disconnected
 * - **Hierarchical**: Each tree maintains hierarchy
 *
### Advantages Over Binary Trees
 * - **Flexibility**: Unlimited children per node
 * - **Natural representation**: Better for some problems
 * - **Simpler algorithms**: Some algorithms simpler for n-ary trees
 *
## Usage
 *
 * ```bash
 * # Generate forest with 50 nodes
 * generate_forest -n 50 -o forest.txt
 *
 * # Use specific seed for reproducibility
 * generate_forest -n 100 -s 12345 -o output.txt
 *
 * # Generate large forest
 * generate_forest -n 1000 -o large_forest.txt
 * ```
 *
## Output Format
 *
 * The forest is output in a format suitable for:
 * - **Visualization**: Can be visualized with tree tools
 * - **Processing**: Can be processed by tree algorithms
 * - **Storage**: Can be saved and loaded later
 *
## Example Use Cases
 *
### File System Representation
 * - **Directories**: Can have any number of subdirectories
 * - **Natural fit**: General trees match file system structure
 * - **Navigation**: Easier navigation than binary representation
 *
### Organizational Charts
 * - **Hierarchy**: Represent organizational structures
 * - **Flexibility**: Managers can have any number of subordinates
 * - **Visualization**: Better visualization than binary trees
 *
### XML/HTML Parsing
 * - **DOM trees**: Represent document structure
 * - **Nested elements**: Elements can have any number of children
 * - **Natural representation**: Matches document structure
 *
 * @see generate_tree.H Tree generation utilities
 * @see tpl_tree_node.H N-ary tree node structure
 * @see tpl_binTree.H Binary tree structures
 * @see ntreepic.C N-ary tree visualization (related)
 * @author Leandro Rabindranath León
 * @ingroup Examples
 */

# include <cstdlib>
# include <ctime> 
# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <tpl_tree_node.H>  
# include <generate_tree.H>
# include <tpl_binTree.H>

using namespace Aleph;
using namespace std;

static void printNode(BinTreeVtl<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

/**
 * @brief Functor to convert tree node to string for output
 */
struct WriteNode
{
  string operator () (Tree_Node<int> * p)
  {
    return to_string(p->get_key());
  }
};

int main(int argc, char *argv[])
{
  try
    {
      TCLAP::CmdLine cmd("Generate forest from random binary tree", ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "nodes", 
                                 "Number of nodes in the tree",
                                 false, 100, "int");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      TCLAP::ValueArg<string> outputArg("o", "output",
                                         "Output file name",
                                         false, "arborescencia.Tree", "string");
      cmd.add(outputArg);

      cmd.parse(argc, argv);

      int n = nArg.getValue();
      unsigned int t = seedArg.getValue();
      string outputFile = outputArg.getValue();

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "Forest Generation Example" << endl;
      cout << "=========================" << endl;
      cout << "Parameters: n=" << n << ", seed=" << t << endl;
      cout << "Output file: " << outputFile << endl << endl;

      BinTreeVtl<int> tree;
      BinTreeVtl<int>::Node *node;
      int value;

      cout << "Inserting " << n << " random values into BST..." << endl;

      int ins_count = 0;

      for (int i = 0; i < n; i++)
        {
          do 
            {
              value = static_cast<int>(10.0 * n * rand() / (RAND_MAX + 1.0));
              node = tree.search(value);
            }
          while (node != nullptr);

          node = new BinTreeVtl<int>::Node(value);
          tree.insert(node);
          ins_count++;
        }
      
      cout << ins_count << " insertions completed" << endl;

      assert(tree.verifyBin());
      cout << "BST verification: PASSED" << endl << endl;

      cout << "Preorder traversal: ";
      preOrderRec(tree.getRoot(), printNode);
      cout << endl << endl;

      // Convert to forest
      Tree_Node<int> * root = 
        bin_to_forest<Tree_Node<int>, BinNodeVtl<int>>(tree.getRoot()); 

      // Write to file
      ofstream output(outputFile, ios::trunc);
      generate_forest<Tree_Node<int>, WriteNode>(root, output); 
      output.close();

      cout << "Forest written to " << outputFile << endl;

      // Cleanup
      destroy_tree(root);
      destroyRec(tree.getRoot());

      cout << endl << "Done." << endl;
    }
  catch (TCLAP::ArgException &e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
