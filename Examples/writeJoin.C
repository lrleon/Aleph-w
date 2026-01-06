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
 * @ingroup Ejemplos
*/

/**
 * @file writeJoin.C
 * @brief Demonstrates BST join operation and generates visualization files
 * 
 * This program creates two separate BSTs with random unique values and then
 * joins them into a single BST. It generates .Tree files in preorder format
 * that can be used for visualization with tools like btreepic.
 * 
 * Output files:
 * - join-1-aux.Tree: First tree before join
 * - join-2-aux.Tree: Second tree before join
 * - join-aux.Tree: Resulting joined tree
 * 
 * The join operation combines two BSTs into one while maintaining the BST
 * property. This is useful for illustrating tree algorithms in educational
 * materials.
 * 
 * Usage: writeJoin [-n <count>] [-s <seed>]
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
