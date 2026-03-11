
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
# include <fstream>
# include <aleph.H>
# include <tpl_rb_tree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


ofstream output("rb-example-aux.Tree", ios::out); 

int position = 0;


ofstream file("rb-tree-aux.Tree", ios::out); 

static void print_key(Rb_Tree<int>::Node *p, int, int) 
{
  file << p->get_key() << " ";
}


static void print_color(Rb_Tree<int>::Node *p, int, int pos) 
{
  if (COLOR(p) == RED)
    file << pos << " ";
}


static void print_key_ex(Rb_Tree<int>::Node *p, int, int) 
{
  output << p->get_key() << " ";
}


static void print_color_ex(Rb_Tree<int>::Node *p, int, int pos) 
{
  if (COLOR(p) == RED)
    output << pos << " ";
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

  cout << "writeRb " << n << " " << t << endl;

  {
    Rb_Tree<int> tree;
    Rb_Tree<int>::Node *node;
    int i;

    for (i = 0; i < 30; i++)
      {
	do
	  {
	    value = (int) (100.0*rand()/(RAND_MAX+1.0));
	    node = tree.search(value);
	  } while (node not_eq NULL);
	node = new Rb_Tree<int>::Node (value);
	tree.insert(node);
      }

    assert(is_red_black(tree.getRoot())); 

    preOrderRec(tree.getRoot(), print_key_ex);
    output << endl << "START-SHADOW "; 
    position = 0; inOrderRec(tree.getRoot(), print_color_ex);
    output << endl;
    
    destroyRec(tree.getRoot());
  }

  {
     Rb_Tree<int> tree;
    Rb_Tree<int>::Node *node;
    int i;
    for (i = 0; i < n; i++)
      {
	do
	  {
	    value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	    node = tree.search(value);
	  } while (node not_eq NULL);
	node = new Rb_Tree<int>::Node (value);
	tree.insert(node);
      }

    assert(is_red_black(tree.getRoot())); 

    preOrderRec(tree.getRoot(), print_key);
    file << "START-SHADOW ";
    position = 0; inOrderRec(tree.getRoot(), print_color);

    destroyRec(tree.getRoot());
  }
}

