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
*/

/**
 * @file writeInsertRoot.C
 * @brief Demonstrates root insertion in BST
 * 
 * This program demonstrates the root insertion technique for binary search
 * trees. Unlike standard BST insertion (which inserts at a leaf), root
 * insertion always places the new element at the root of the tree using
 * rotations.
 * 
 * Root insertion has several interesting properties:
 * - Recently inserted elements are near the root (faster access)
 * - Can be used to implement efficient splay-like behavior
 * - Useful for temporal locality patterns
 * 
 * Output files:
 * - insert_root-aux.Tree: Tree in preorder format for visualization
 * 
 * Usage: writeInsertRoot [-n <count>] [-s <seed>]
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
