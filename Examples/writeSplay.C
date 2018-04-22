
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

# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;
using namespace std;


ofstream output("splay-tree-aux.Tree", ios::out);



void print_key(Splay_Tree<int>::Node * p, int, int)
{
  output << p->get_key() << " ";
}



int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "writeSplay " << n << " " << t << endl;

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      while (true)
	{
	  value = 1+(int) (n*1.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  if (node == NULL)
	    break;
	}
      node = new Splay_Tree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
}
