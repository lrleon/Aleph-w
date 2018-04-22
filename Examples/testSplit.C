
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void printNode(BinNode<int>* node)
{
  cout << node->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinTree<int> tree;
  BinTree<int>::Node * p;
  int value;
  int ins_count = 0;

  for (i = 1; i < n - 1; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));    
      p = new BinNode<int> (value);
      while (tree.insert(p) == NULL)
	KEY(p) = (int) (10.0*n*rand()/(RAND_MAX+1.0));
      ins_count++;
    }
  
  assert(check_bst(tree.getRoot()));

  cout << ins_count << " keys inserted" << endl;
  preOrderThreaded(tree.getRoot(), printNode);
  cout << endl;

  BinTree<int>::Node * t1, *t2;

  do
    value = (int) (10.0*n*rand()/(RAND_MAX+1.0));    
  while (not split_key_rec(tree.getRoot(), value, t1, t2));

  cout << "Clave de particion: " << value << endl << endl;

  cout << "Left Tree" << endl;
  preOrderThreaded(t1, printNode);
  cout << endl;

  cout << "Right Tree" << endl;
  preOrderThreaded(t2, printNode);
  cout << endl;

  destroyRec(t1);
  destroyRec(t2);
}

// 1018058241
