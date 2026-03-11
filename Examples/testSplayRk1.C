
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

# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <tpl_splay_treeRk.H>
# include <tpl_binNodeXt.H>


using namespace std;

int main(int argn, char *argc[]) {
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree_Rk<int> tree;
  Splay_Tree_Rk<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++) {
    value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
    node = tree.search(value);
    if (node == NULL) {
      insCount++;
      node = new Splay_Tree_Rk<int>::Node (value);
      tree.insert(node);
      }
    }
  
  cout << insCount << " Items inserted" << endl;

  if (Aleph::check_rank_tree(tree.getRoot()))
		cout << "arbol equilibrado" << endl;
	else
		cout << "error de equilibrio en el arbol" << endl;

/*
  for (i = 0; i < n; i++) {
    value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
    node = tree.remove(value);
    if (node != NULL) {
      delCount++;
      delete node;
      }
    }

  cout << delCount << " Items removed" << endl;
*/
  destroyRec(tree.getRoot());
  cout << "testSplayTree " << n << " " << t << endl;
  }
