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
 * This example generates a random binary search tree, converts it to
 * a forest representation, and outputs the forest structure to a file
 * in a format suitable for visualization.
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
