

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


# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_avl.H>
# include <tpl_binTree.H>
# include <tpl_rb_tree.H>
# include <tpl_rand_tree.H>
# include <tpl_treap.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;
using namespace std;


ofstream output_splay("splay.Tree", ios::out);

void print_splay(Splay_Tree<int>::Node * p, int, int)
{
  output_splay << p->get_key() << " ";
}


ofstream output_bin("bin.Tree", ios::out);

void print_bin(BinTree<int>::Node * p, int, int)
{
  output_bin << p->get_key() << " ";
}


ofstream output_rb("rb.Tree", ios::out);

void print_rb(Rb_Tree<int>::Node * p, int, int)
{
  output_rb << p->get_key() << " ";
}


ofstream output_avl("avl.Tree", ios::out);

void print_avl(Avl_Tree<int>::Node * p, int, int)
{
  output_avl << p->get_key() << " ";
}


ofstream output_rand("rand.Tree", ios::out);

void print_rand(Rand_Tree<int>::Node * p, int, int)
{
  output_rand << p->get_key() << " ";
}


ofstream output_treap("treap.Tree", ios::out);

void print_treap(Treap<int>::Node * p, int, int)
{
  output_treap << p->get_key() << " ";
}



int main(int argn, char *argc[])
{
  int n = 512;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "writeAllTrees " << n << " " << t << endl;

  {
    srand(t);
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

    preOrderRec(tree.getRoot(), print_splay);

    destroyRec(tree.getRoot());
  }

  {
    srand(t);
    Rand_Tree<int> tree;
    Rand_Tree<int>::Node *node;
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
	node = new Rand_Tree<int>::Node (value);
	tree.insert(node);
      }

    preOrderRec(tree.getRoot(), print_rand);

    destroyRec(tree.getRoot());
  }

  {
    srand(t);
    Rb_Tree<int> tree;
    Rb_Tree<int>::Node *node;
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
	node = new Rb_Tree<int>::Node (value);
	tree.insert(node);
      }

    preOrderRec(tree.getRoot(), print_rb);

    destroyRec(tree.getRoot());
  }

  {
    srand(t);
    BinTree<int> tree;
    BinTree<int>::Node *node;
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
	node = new BinTree<int>::Node (value);
	tree.insert(node);
      }

    preOrderRec(tree.getRoot(), print_bin);

    destroyRec(tree.getRoot());
  }

  {
    srand(t);
    Avl_Tree<int> tree;
    Avl_Tree<int>::Node *node;
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
	node = new Avl_Tree<int>::Node (value);
	tree.insert(node);
      }

    preOrderRec(tree.getRoot(), print_avl);

    destroyRec(tree.getRoot());
  }

  {
    srand(t);
    Treap<int> tree;
    Treap<int>::Node *node;
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
	node = new Treap<int>::Node (value);
	tree.insert(node);
      }

    preOrderRec(tree.getRoot(), print_treap);

    destroyRec(tree.getRoot());
  }
}
