

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
# include <tpl_binNodeUtils.H>
# include <tpl_tree_node.H>


using namespace std;
using namespace Aleph;

int random(int l, int r)
{
  assert(l <= r);

  int n = r - l;

  int rd = 1 + (int) (1.0*n*rand()/(RAND_MAX+1.0));

  return l + rd - 1;
}


BinNode<int> * random_tree(int l, int r)
{
  if (l > r)
    return NULL;

  BinNode<int> * root = new BinNode<int> (random(l, r));

  LLINK(root) = random_tree(l, KEY(root) - 1);
  RLINK(root) = random_tree(KEY(root) + 1, r);

  return root;
}


    template <class Node>
static void printNode(Node * node, int, int)
{ 
  cout << " " << node->get_key();
}


int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 2;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinNode<int> * bp = random_tree(1, n);

  cout << "Prefijo:";
  preOrderRec(bp,  printNode);
  cout << endl << endl;
  
  cout << "Infijo:";
  inOrderRec(bp,  printNode);
  cout << endl << endl;
  
  Tree_Node<int> * tree = bin_to_forest< Tree_Node<int>, BinNode<int> > (bp);

  forest_preorder_traversal(tree, printNode);

  BinNode<int> * prb = forest_to_bin< Tree_Node<int>, BinNode<int> > (tree);

  assert(areEquivalents(prb, bp));
}
