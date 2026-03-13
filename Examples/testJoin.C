
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
# include <ctime>
# include <cstdlib>
# include <cassert>

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


int main(int argc, char *argv[])
{
  int n = argc > 1 ? atoi(argv[1]) : 10;

  unsigned int t = std::time(0);

  if (argc > 2)
    t = atoi(argv[2]);

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  int i, value;
  Node * root1 = Node::NullPtr;

  for (i = 0; i < n; i++)
    {
      while (1)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root1, value) == Node::NullPtr)
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
	  if (searchInBinTree(root1, value) == Node::NullPtr and
	      searchInBinTree(root2, value) == Node::NullPtr)
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

  Node * dup = Node::NullPtr;

  Node * root = join(root1, root2, dup);
  assert(check_bst(root));
  cout << endl << "Join(T1, T2)" << endl;
  preOrderRec(root, &print_node); cout << endl << endl;
  destroyRec(root);
  destroyRec(dup); 
  dup = Node::NullPtr; // Reinitialize dup to avoid dangling pointer

  Node * res = join(r2, r1, dup); // renamed r to res to avoid confusion with r1/r2
  assert(check_bst(res));
  cout << endl << "Join(T2, T1)" << endl;
  preOrderRec(res, &print_node); cout << endl << endl;
  destroyRec(res);
  destroyRec(dup);

  {
    Node * t1 = Node::NullPtr, * t2 = Node::NullPtr;

    for (i = 0; i < n; ++i)
      {
	insert_in_bst(t1, new Node (rand()));
	insert_in_bst(t2, new Node (rand()));
      }

    cout << "t1: ";
    preOrderRec(t1, &print_node); cout << endl << endl;

    cout << "t2: ";
    preOrderRec(t2, &print_node); cout << endl << endl;

    Node * dup_inner = Node::NullPtr;
    Node * j = join(t1, t2, dup_inner);

    cout << "join(t1, t2, dup): ";
    preOrderRec(j, &print_node); cout << endl << endl;

    cout << "dup: ";
    preOrderRec(dup_inner, &print_node); cout << endl << endl;

    destroyRec(j);
    destroyRec(dup_inner);
  }
}
