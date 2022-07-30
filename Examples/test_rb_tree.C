
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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

# include <aleph.H>
# include <tpl_rb_tree.H>
# include <stdlib.h>
# include <iostream>
# include <time.h>


using namespace Aleph;

static void printNode(Rb_Tree<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

static void print_color(Rb_Tree<int>::Node* node, int, int pos)
{
  if (COLOR(node) == RED)
    cout << "shadow " << pos << endl;
}

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 2;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  Rb_Tree<int>  tree;
  Rb_Tree<int>::Node *node;
  int value;

  for (i = 0; i < n; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
	  cout << value << " ";
	  node = new Rb_Tree<int>::Node (value);
	  tree.insert(node);
	}
    }

  assert(is_red_black_tree(tree.getRoot()));

  cout << endl 
       << "start-prefix ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;
  inOrderRec(tree.getRoot(), print_color);
  cout << endl << endl;

  for (i = 0; i < n/2; i++)
    {
      do
	{
	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	}
      while (node == NULL);
      cout << value << " ";
      delete node;
    }

  cout << endl 
       << "start-prefix ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;
  inOrderRec(tree.getRoot(), print_color);

  assert(is_red_black_tree(tree.getRoot()));

  cout << "destroying tree" << endl;

  destroyRec(tree.getRoot());

  cout << "testRb_Tree " << n << " " << t << endl;
}
