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
 * @file writeBalance.C
 * @brief Demonstrates BST balancing operation (DSW algorithm)
 * 
 * This program creates an unbalanced BST with rank information and then
 * applies the balance operation to produce a perfectly balanced tree.
 * It generates visualization files showing the tree before and after balancing.
 * 
 * The balancing algorithm (Day-Stout-Warren) works in O(n) time by first
 * converting the tree to a vine (right-skewed chain) and then performing
 * rotations to achieve perfect balance.
 * 
 * Output files:
 * - balance-before-aux.Tree: Original unbalanced tree
 * - balance-after-aux.Tree: Perfectly balanced tree
 * 
 * Usage: writeBalance [-n <count>] [-s <seed>]
 */

# include <iostream>
# include <fstream>
# include <tclap/CmdLine.h>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>

using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;

// Output streams
ofstream outputb;
ofstream outputa;

void print_keyb(Node* p, int, int)
{
  outputb << p->get_key() << " ";
}

void print_keya(Node* p, int, int)
{
  outputa << p->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate BST balancing (DSW algorithm).\n"
        "Creates an unbalanced tree and shows the result of balancing.",
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

      cout << "=== BST Balancing Demo (DSW Algorithm) ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      outputb.open("balance-before-aux.Tree", ios::out);
      outputa.open("balance-after-aux.Tree", ios::out);

      if (!outputb.is_open() || !outputa.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build ranked tree (will be unbalanced due to random insertion)
      Node* root = Node::NullPtr;
      int value;

      cout << "Building unbalanced BST with " << n << " elements..." << endl;
      for (int i = 0; i < n; i++)
        {
          while (true)
            {
              value = static_cast<int>(100.0 * n * rand() / (RAND_MAX + 1.0));
              if (searchInBinTree(root, value) == Node::NullPtr)
                break;
            }
          Node* p = new Node(value);
          insert_by_key_xt(root, p);
        }

      assert(check_rank_tree(root));

      int height_before = computeHeightRec(root);
      int node_count = root->getCount();
      cout << "Before balancing:" << endl;
      cout << "  Nodes: " << node_count << ", Height: " << height_before << endl;
      cout << "  Optimal height would be: " << static_cast<int>(log2(n)) << endl;

      // Write tree before balancing
      preOrderRec(root, &print_keyb);
      outputb << endl;

      // Balance the tree
      cout << endl << "Applying DSW balancing algorithm..." << endl;
      root = balance_tree(root);

      assert(check_rank_tree(root));

      int height_after = computeHeightRec(root);
      cout << "After balancing:" << endl;
      cout << "  Nodes: " << root->getCount() << ", Height: " << height_after << endl;
      cout << "  Height reduction: " << (height_before - height_after) << " levels" << endl;

      // Write tree after balancing
      preOrderRec(root, &print_keya);
      outputa << endl;

      // Clean up
      outputb.close();
      outputa.close();
      destroyRec(root);

      cout << endl << "Generated files:" << endl;
      cout << "  - balance-before-aux.Tree (original unbalanced)" << endl;
      cout << "  - balance-after-aux.Tree (after balancing)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
