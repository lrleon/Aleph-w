

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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


# include <cstdlib>
# include <ctime> 

# include <iostream>

# include <tpl_tree_node.H>  
# include <generate_tree.H>
# include <tpl_binTree.H>

using namespace Aleph;

using namespace std;

static void printNode(BinTreeVtl<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

struct WN
{
  string operator () (Tree_Node<int> * p)
  {
    return to_string(p->get_key());
  }
};


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinTreeVtl<int>  tree;
  BinTreeVtl<int>::Node *node;
  int value;

  cout << "Inserting " << n << " random values in treee ...\n";

  int ins_count = 0;

  for (i = 0; i < n; i++)
    {
      do 
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  }
      while (node not_eq NULL);

      node = new BinTreeVtl<int>::Node (value);
      tree.insert(node);
      ins_count++;

      cout << value << " ";
    }
  cout << endl << endl;

  assert(tree.verifyBin());
  cout << endl << ins_count << " insertions" << endl
       << "prefijo: " << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  Tree_Node<int> * root = 
    bin_to_forest < Tree_Node<int>, BinNodeVtl<int> > (tree.getRoot()); 

  ofstream o("arborescencia.Tree", ios::trunc);

  generate_forest<Tree_Node<int>, WN> (root, o); 

  destroy_tree(root);

  destroyRec(tree.getRoot());

  cout << "testBinTreeVtl " << n << " " << t << endl;
}

// 1018058241


