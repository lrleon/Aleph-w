/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# include <iostream>
# include <aleph.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

using namespace Aleph;
using namespace std;

typedef BinNode<int> Node;

void print_node(Node * p, int, int)
{
  //  cout << "(" << p->get_key() << "," << p->getCount() << ")" ;
  cout << p->get_key() << " " ;
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value;
  Node * root1 = Node::NullPtr;

  for (i = 0; i < n; i++)
    {
      while (1)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root1, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      root1 = insert_root(root1, p);
    }

  assert(check_bst(root1));

  cout << endl << "T1" << endl;
  preOrderRec(root1, &print_node); cout << endl << endl;

  Node * root2 = Node::NullPtr;
  for (i = 0; i < n; i++)
    {
      while (1)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root1, value) == NULL and
	      searchInBinTree(root2, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      root2 = insert_root(root2, p);
    }

  assert(check_bst(root2));

  cout << endl << "T2" << endl;
  preOrderRec(root2, &print_node); cout << endl;

  Node * r1 = copyRec(root1);
  Node * r2 = copyRec(root2);

  Node * dup;

  Node * root = join(root1, root2, dup);
  assert(check_bst(root));
  cout << endl << "Join(T1, T2)" << endl;
  preOrderRec(root, &print_node); cout << endl << endl;
  destroyRec(root);

  Node * r = join(r2, r1, dup);
  assert(check_bst(r));
  cout << endl << "Join(T2, T1)" << endl;
  preOrderRec(r, &print_node); cout << endl << endl;
  destroyRec(r);

  {
    Node * t1 = NULL, * t2 = NULL;

    for (i = 0; i < n; ++i)
      {
	insert_in_bst(t1, new Node (rand()));
	insert_in_bst(t2, new Node (rand()));
      }

    cout << "t1: ";
    preOrderRec(t1, &print_node); cout << endl << endl;

    cout << "t2: ";
    preOrderRec(t2, &print_node); cout << endl << endl;

    Node * dup = NULL;
    Node * j = join(t1, t2, dup);

    cout << "join(t1, t2, dup): ";
    preOrderRec(j, &print_node); cout << endl << endl;

    cout << "dup: ";
    preOrderRec(dup, &print_node); cout << endl << endl;

    destroyRec(j);
    destroyRec(dup);
  }
}
