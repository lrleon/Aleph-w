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
 * @file writeSplit.C
 * @brief Demonstrates BST split by position operation
 * 
 * This program creates a BST with rank information (BinNodeXt) and then
 * splits it by position, generating visualization files for the original
 * tree and both resulting subtrees.
 * 
 * The split operation divides a tree at a given position (rank), producing
 * two trees: one with all elements at positions less than the split point,
 * and one with elements at positions greater than or equal to the split point.
 * 
 * Output files:
 * - split-before-aux.Tree: Original tree with SPLIT directive
 * - split-1-aux.Tree: Left subtree (positions 0 to pos-1)
 * - split-2-aux.Tree: Right subtree (positions pos to n-1)
 * 
 * Usage: writeSplit [-n <count>] [-s <seed>] [-p <position>]
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

// Output streams
ofstream output;
ofstream output_1;
ofstream output_2;

void print_key(Node* p, int, int)
{
  output << p->get_key() << " ";
}

void print_key1(Node* p, int, int)
{
  output_1 << p->get_key() << " ";
}

void print_key2(Node* p, int, int)
{
  output_2 << p->get_key() << " ";
}

int main(int argc, char* argv[])
{
  try
    {
      TCLAP::CmdLine cmd(
        "Demonstrate BST split by position operation.\n"
        "Creates a tree and splits it at a given position.",
        ' ', "1.0");

      TCLAP::ValueArg<int> nArg("n", "count",
                                 "Number of elements",
                                 false, 10, "int");
      cmd.add(nArg);

      TCLAP::ValueArg<unsigned int> seedArg("s", "seed",
                                             "Random seed (0 = use time)",
                                             false, 0, "unsigned int");
      cmd.add(seedArg);

      TCLAP::ValueArg<int> posArg("p", "position",
                                   "Split position (-1 = middle)",
                                   false, -1, "int");
      cmd.add(posArg);

      cmd.parse(argc, argv);

      int n = nArg.getValue();
      unsigned int t = seedArg.getValue();
      int split_pos = posArg.getValue();

      if (t == 0)
        t = time(nullptr);

      srand(t);

      cout << "=== BST Split by Position Demo ===" << endl;
      cout << "Elements: " << n << ", Seed: " << t << endl << endl;

      // Open output files
      output.open("split-before-aux.Tree", ios::out);
      output_1.open("split-1-aux.Tree", ios::out);
      output_2.open("split-2-aux.Tree", ios::out);

      if (!output.is_open() || !output_1.is_open() || !output_2.is_open())
        {
          cerr << "Error: cannot open output files" << endl;
          return 1;
        }

      // Build ranked tree
      Node* root = Node::NullPtr;
      int value;

      cout << "Building ranked BST with " << n << " elements..." << endl;
      for (int i = 0; i < n; i++)
        {
          while (true)
            {
              value = static_cast<int>(1.0 * n * rand() / (RAND_MAX + 1.0));
              if (searchInBinTree(root, value) == Node::NullPtr)
                break;
            }
          Node* p = new Node(value);
          insert_by_key_xt(root, p);
        }

      int num_nodes = root->getCount();
      cout << "  Nodes: " << num_nodes 
           << ", Height: " << computeHeightRec(root) << endl;

      assert(check_rank_tree(root));
      assert(check_bst(root));

      // Write original tree
      preOrderRec(root, &print_key);

      // Determine split position
      if (split_pos < 0 || split_pos >= num_nodes)
        split_pos = num_nodes / 2;

      cout << endl << "Splitting at position " << split_pos 
           << " (of " << num_nodes << ")..." << endl;

      // Add SPLIT directive for visualization
      output << "SPLIT " << split_pos << " \"\" \"\"" << endl;

      // Perform split
      Node* l;
      Node* r;
      split_pos_rec(root, split_pos, l, r);
      cout << "  ...done" << endl;

      // Verify and write left subtree
      if (l != Node::NullPtr)
        {
          assert(check_rank_tree(l));
          assert(check_bst(l));
          preOrderRec(l, print_key1);
          output_1 << endl;
          cout << endl << "Left subtree:" << endl;
          cout << "  Nodes: " << (l ? l->getCount() : 0) 
               << ", Height: " << computeHeightRec(l) << endl;
        }
      else
        {
          cout << endl << "Left subtree: empty" << endl;
        }

      // Verify and write right subtree
      if (r != Node::NullPtr)
        {
          assert(check_rank_tree(r));
          assert(check_bst(r));
          preOrderRec(r, print_key2);
          output_2 << endl;
          cout << "Right subtree:" << endl;
          cout << "  Nodes: " << (r ? r->getCount() : 0) 
               << ", Height: " << computeHeightRec(r) << endl;
        }
      else
        {
          cout << "Right subtree: empty" << endl;
        }

      // Clean up
      output.close();
      output_1.close();
      output_2.close();
      destroyRec(l);
      destroyRec(r);

      cout << endl << "Generated files:" << endl;
      cout << "  - split-before-aux.Tree (original with split directive)" << endl;
      cout << "  - split-1-aux.Tree (left subtree)" << endl;
      cout << "  - split-2-aux.Tree (right subtree)" << endl;
    }
  catch (TCLAP::ArgException& e)
    {
      cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
      return 1;
    }

  return 0;
}
