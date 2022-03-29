

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

# include <iostream>
# include <time.h>
# include <gsl/gsl_rng.h>
# include <aleph.H>
# include <tpl_dynarray_set.H>
# include <tpl_splay_treeRk.H>
# include <tpl_binNodeUtils.H>
# include <tpl_sort_utils.H>

using namespace std;
using namespace Aleph;

DynArray<unsigned long> rand_sequence;


void printNode(Splay_Tree_Rk<int>::Node *node, int, int)
{
  cout << node->get_key() << " ";
}

int main(int argn, char *argc[])
{
  int n = 10;
  unsigned int seed = 0;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    seed = atoi(argc[2]);

  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);;
  gsl_rng_set(r, seed % gsl_rng_max(r));

  cout << argc[0] << " " << n << " " << seed << endl;

  DynArray_Set<int> keys; keys.reserve(n);
  Splay_Tree_Rk<int> tree;
  Splay_Tree_Rk<int>::Node *node;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (int i = 0; i < n; i++)
    { 
      int value;
      do
	{
	  value = gsl_rng_uniform_int(r, 100*n);
	  node = tree.search(value);
	  assert(tree.verify());
	}
      while (node not_eq NULL);

      cout << value << " ";
      node = new Splay_Tree_Rk<int>::Node (value);
      tree.insert(node);
      assert(tree.verify());
      keys(i) = value;
    }
  cout << endl << endl;

  assert(check_rank_tree(tree.getRoot()));

  cout << "Sorting keys array" << endl;
  quicksort(keys);
  for (int i = 0; i < keys.size(); ++i)
    cout << keys(i) << " ";
  cout << endl
       << "done" << endl << endl;

  cout << "inorden traversal prio" << endl;
  inOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  cout << "Testing select" << endl;

  for (int i = 0; i < n; i++)
    {
      node = tree.select(i);
      cout << node->get_key() << " ";
      assert(node->get_key() == keys(i));
    }

  cout << "done!" << endl << endl;

  assert(check_rank_tree(tree.getRoot()));

  cout << "testing random positions" << endl;
  for (int i = 0; i < n; ++i)
    {
      int idx = gsl_rng_uniform_int(r, keys.size());
      std::pair<int, Splay_Tree_Rk<int>::Node*> pos = 
	tree.position(keys(idx));
      assert(check_rank_tree(tree.getRoot()));
      assert(pos.second != NULL);
      assert(pos.second->get_key() == keys(idx));
      assert(idx == pos.first);
      cout << idx << "<-->" << pos.first << endl
	   << keys(i) << "<-->" << pos.second->get_key() << " " << endl;
    }

  for (int i = 0; i < n/2; i++)
    { 
      int idx = gsl_rng_uniform_int(r, keys.size());
      int & value = keys(idx);
      cout << value << " ";
      node = tree.remove(value);
      assert(node != NULL);
      assert(node->get_key() == value);
      delete node;
      keys.remove(value);
    }

  cout << endl << "verifying Splay_Rk after deletions ... " 
       << endl;
  assert(check_rank_tree(tree.getRoot()));
  cout << " done" << endl;

  cout << "Preorden" << endl;
  inOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "The path length is " << internal_path_length(tree.getRoot())
       << endl;

  destroyRec(tree.getRoot()); 

  cout << endl << argc[0] << " " << n << " " << seed << endl;
 
  gsl_rng_free(r);
}







