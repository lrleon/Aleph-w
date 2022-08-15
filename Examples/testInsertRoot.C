
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

# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void printNode(BinNode<int>* node)
{
  cout << node->get_key() << " ";
}


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  DynArray<int> array;

  cout << argc[0] << " " << n << " " << t << endl;

  BinNode<int> *root = new BinNode<int> ((int) (10.0*n*rand()/(RAND_MAX+1.0)));
  cout << root->get_key() << " ";

  array[0] = root->get_key();

  BinNode<int> *p;
  int value;
  int ins_count = 1;

  for (i = 1; i < n; i++)
    {
      do 
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  p = searchInBinTree(root, value);
	}
      while (p != NULL);

      p = new BinNode<int> (value);
      cout << value << " ";
      root = insert_root(root, p);

      array[i] = value;

      ins_count++;
    }
  
  cout << endl << ins_count << " items inserted" << endl;
  assert(check_bst(root));

  cout << "prefix: ";
  preOrderThreaded(root, printNode);
  cout << endl;

  for (i = 0; i < 10; i++)
    {
      int idx;
      do
	{
	  idx = (int) (1.0*n*rand()/(RAND_MAX+1.0));

	  value = array[idx];

	  p = searchInBinTree(root, value);

	}
      while (p == NULL);

      cout << value << " ";

      p = remove_from_bst(root, value);

      assert(p != NULL);
    }

  cout << endl;
  preOrderThreaded(root, printNode);
  cout << endl;
  destroyRec(root);
}
