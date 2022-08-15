
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

# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

using namespace Aleph;
using namespace std;

typedef BinNode<int> Node;

ofstream output("join-aux.Tree", ios::out);
ofstream output1("join-1-aux.Tree", ios::out);
ofstream output2("join-2-aux.Tree", ios::out);

void print_key(Node * p, int, int)
{
  output << p->get_key() << " " ;
}


void print_key1(Node * p, int, int)
{
  output1 << p->get_key() << " " ;
}


void print_key2(Node * p, int, int)
{
  output2 << p->get_key() << " " ;
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

  for (i = 0; i < n/2; i++)
    {
      while (true)
	{
	  value = rand();
	  if (searchInBinTree(root1, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      insert_root(root1, p);
    }

  assert(check_bst(root1));

  preOrderRec(root1, &print_key1); 

  Node * root2 = Node::NullPtr;
  for (i = 0; i < n/2; i++)
    {
      while (true)
	{
	  value = rand();
	  if (searchInBinTree(root1, value) == NULL and
	      searchInBinTree(root2, value) == NULL)
	    break;
	}
      Node * p = new Node (value);
      insert_in_bst(root2, p);
    }

  assert(check_bst(root2));

  preOrderRec(root2, &print_key2);

  Node * dup = NULL;

  Node * root = join(root1, root2, dup);

  assert(dup ==NULL); // no deben haber repetidos

  assert(check_bst(root));

  preOrderRec(root, &print_key);

  destroyRec(root);
}
