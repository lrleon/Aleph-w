
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

# include <iostream>
# include <gsl/gsl_rng.h>
# include <tpl_dynarray_set.H>
# include <tpl_splay_tree.H>

using namespace std;


void print_node(Splay_Tree<int>::Node * p, int, int)
{
  cout << KEY(p) << " " ;
}


void write_node(Splay_Tree<int>::Node * p, int, int)
{
  cout << KEY(p) << " " ;
}


int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int seed = 0;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    seed = atoi(argc[2]);

  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);;
  gsl_rng_set(r, seed % gsl_rng_max(r));

  cout << "test_Splay_Tree " << n << " " << seed << endl;

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node * p;

  DynArray_Set<int> values;

  cout << "Testing for insertions" << endl;

  for (int i = 0; i < n; i++)
    {
      int value;
      do
	{
	  value = gsl_rng_uniform_int(r, 100*n);
	  p = tree.search(value);
	}
      while (p != NULL);

      cout << value << " ";
      p = new Splay_Tree<int>::Node (value);
      tree.insert(p);
      values.append(value);
    }

  cout << endl << "Inorder " << endl;
  inOrderRec(tree.getRoot(), print_node);

  cout << endl << "Preorder " << endl;
  preOrderRec(tree.getRoot(), print_node);

  cout << endl << endl;

  cout << "Testing for random searches" << endl;
  for (int i = 0; i < n; ++i)
    {
      int idx = gsl_rng_uniform_int(r, values.size());
      int value = values(idx);
      cout << value << " ";
      if (tree.search(value) == NULL)
	AH_ERROR("BUG detected ins searching");
    }
  cout << "Done" << endl;
  
  cout << "Removing test" << endl;

  for (int i = 0; i < n/7; i++)
    {       
      int & value = values(gsl_rng_uniform_int(r, values.size()));
      p = tree.remove(value);
      cout << value << " ";
      assert(p != NULL);
      assert(value == p->get_key());
      delete p;
      values.remove(value);
    }

  cout << endl << "Inorder " << endl << endl;
  inOrderRec(tree.getRoot(), print_node);

  cout << endl;

  destroyRec(tree.getRoot());

  DynList<int>({122, 363, 1247, 510, 701, 1565, 1157, 728, 1564, 492, 861, 422}).
    for_each([&tree] (int k)
	     {
	       tree.insert(new Splay_Tree<int>::Node(k));
	     });
  cout << endl << "Preorder " << endl;
  preOrderRec(tree.getRoot(), print_node);

  destroyRec(tree.getRoot());
  gsl_rng_free(r);

  cout << endl;
}
