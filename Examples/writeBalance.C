
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
 
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>


using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;


ofstream outputb("balance-before-aux.Tree", ios::out); 
ofstream outputa("balance-after-aux.Tree", ios::out); 

void print_keyb(Node * p, int, int)
{
  outputb << p->get_key() << " ";
}

void print_keya(Node * p, int, int)
{
  outputa << p->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
  Node * root = Node::NullPtr;

  for (i = 0; i < n - 1; i++)
    {
      while (true)
	{
	  value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) == Node::NullPtr)
	    break;
	}
      Node * p = new Node (value); 
      insert_by_key_xt(root, p);
    }

  assert(check_rank_tree(root));

  preOrderRec(root, &print_keyb); 

  root = balance_tree(root);

  assert(check_rank_tree(root));

  preOrderRec(root, &print_keya);

  destroyRec(root);
}
