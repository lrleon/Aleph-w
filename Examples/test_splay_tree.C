
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

# include <iostream>
# include <gsl/gsl_rng.h>
# include <tpl_dynarray_set.H>
# include <tpl_splay_tree.H>
# include <string>

# include <cassert>
using namespace std;


void print_node(Splay_Tree<int>::Node * p, int, int)
{
  cout << KEY(p) << " " ;
}


void write_node(Splay_Tree<int>::Node * p, int, int)
{
  cout << KEY(p) << " " ;
}


int main(int argc, char * argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); }
      catch (...) { n = 1000; }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  unsigned int seed = 0;
  if (argc > 2)
    {
      try { seed = static_cast<unsigned int>(stoul(argv[2])); }
      catch (...) { seed = 0; }
    }

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
