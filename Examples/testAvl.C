
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
# include <aleph.H>
# include <tpl_avl.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

static void print_pair(Avl_Tree<int>::Node *p, int, int) 
{
  cout << "(" << p->get_key() << "," << (int) (p->getDiff()) << ") ";
}

static void print(Avl_Tree<int>::Node *p, int, int) 
{
  cout << p->get_key() << " ";
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

  cout << "testAvl " << n << " " << t << endl;

  Avl_Tree<int> tree;
  Avl_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      do
	{
	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	}
      while (node not_eq NULL);

      cout << value << " ";
      node = new Avl_Tree<int>::Node (value);
      tree.insert(node);
    }
  cout << endl << "verifying avl tree after insertions ... " 
       << endl;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  cout << "start-prefix ";
  preOrderRec(tree.getRoot(), print);
  cout << endl << endl;

  cout << endl
       << "start-infix ";
  inOrderRec(tree.getRoot(), print_pair);
  cout << endl << endl;
    
  
  for (i = 0; i < n/2; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	} 
      while (node == NULL);

      cout << value << " ";
      delete node;
    }

  cout << endl
       << "start-prefix ";
  preOrderRec(tree.getRoot(), print);
  cout << endl << endl;
    
  cout << endl << "verifying avl tree after deletions ... " ;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  destroyRec(tree.getRoot());
}

