
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

# include <time.h>
# include <iostream>
# include <fstream>
# include <aleph.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

ofstream output("insert_root-aux.Tree", ios::out);

static void printNode(BinNode<int>* node, int, int)
{
  output << node->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinNode<int> * p, * root = NULL;
  int value;
  int ins_count = 1;

  for (i = 0; i < n; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));

      p = new BinNode<int> (value);
      
      while (Aleph::insert_root(root, p) == NULL)
	KEY(p) = (int) (10.0*n*rand()/(RAND_MAX+1.0));

      cout << value << " ";

      ins_count++;
    }
  
  cout << endl << ins_count << " items inserted" << endl;
  assert(check_bst(root));

  cout << "prefix: ";
  preOrderRec(root, printNode);
  cout << endl;

  destroyRec(root);
}
