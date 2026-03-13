
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
# include <iostream>
# include <string>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>

# include <cstdlib>
# include <cassert>
using namespace std;
# include <cstdlib>
# include <cassert>
using namespace Aleph;


static void printNode(BinNode<int>* node)
{
  cout << node->get_key() << " ";
}


int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); } catch (...) { n = 1000; }
    }

  unsigned int t = std::time(0);
  if (argc > 2)
    {
      try { t = stoul(argv[2]); } catch (...) { t = std::time(0); }
    }

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  BinTree<int> tree;
  BinTree<int>::Node * p;
  int value;
  int ins_count = 0;

  // Guard against tiny inputs that would make splitting impossible
  if (n <= 2)
    {
      cerr << "Error: n must be greater than 2 for meaningful tree splitting." << endl;
      return 1;
    }

  for (int i = 0; i < n; i++)
    {
      value = (int) (10.0*n*rand()/(RAND_MAX+1.0));    
      p = new BinNode<int> (value);
      while (tree.insert(p) == NULL)
	KEY(p) = (int) (10.0*n*rand()/(RAND_MAX+1.0));
      ins_count++;
    }
  
  assert(check_bst(tree.getRoot()));

  cout << ins_count << " keys inserted" << endl;
  preOrderThreaded(tree.getRoot(), printNode);
  cout << endl;

  BinTree<int>::Node * t1, *t2;

  do
    value = (int) (10.0*n*rand()/(RAND_MAX+1.0));    
  while (not split_key_rec(tree.getRoot(), value, t1, t2));

  cout << "Partition key: " << value << endl << endl;

  cout << "Left Tree" << endl;
  preOrderThreaded(t1, printNode);
  cout << endl;

  cout << "Right Tree" << endl;
  preOrderThreaded(t2, printNode);
  cout << endl;

  destroyRec(t1);
  destroyRec(t2);
}
