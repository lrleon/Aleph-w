
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

# include <gsl/gsl_rng.h>
# include <iostream>
# include <memory>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>

using namespace std;

struct GslRngDeleter
{
  void operator()(gsl_rng * r) const { gsl_rng_free(r); }
};

using GslRngHandle = std::unique_ptr<gsl_rng, GslRngDeleter>;

template <class Tree>
void test(size_t n, unsigned long seed)
{
  cout << "Testing for " << typeid(Tree).name() << endl
       << endl;
  GslRngHandle r(gsl_rng_alloc (gsl_rng_mt19937));
  gsl_rng_set(r.get(), seed % gsl_rng_max(r.get()));

  Tree tree;
  for (size_t i = 0; i < n; ++i)
    {
      auto val = gsl_rng_get(r.get());
      auto node = new typename Tree::Node(val);
      auto p = tree.insert(node);
      if (p == nullptr)
	delete node;
    }

  {
    typename Tree::Iterator it = tree;
    auto it_c = it;
    auto it_m = move(it_c);
  }

  for (typename Tree::Iterator it(tree); it.has_curr(); it.next())
    cout << it.get_curr()->get_key() << " " << endl;
  cout << endl;

  destroyRec(tree.getRoot());
}

void usage()
{
  cout << "test-tree-itor [n] [seed]" << endl
       << endl;
  exit(0);
}

int main(int argc, char *argv[])
{
  size_t n = 10;
  unsigned long seed = 0;

  try
    {
      if (argc > 1)
        n = static_cast<size_t>(stoul(argv[1]));
      if (argc > 2)
        seed = stoul(argv[2]);
    }
  catch (...)
    {
      // Fallback to defaults
    }

  test<BinTree<long>>(n, seed); 
  test<Avl_Tree<long>>(n, seed); 
  test<Splay_Tree<long>>(n, seed); 
  test<Rb_Tree<long>>(n, seed);
  test<Rand_Tree<long>>(n, seed);
  test<Treap<long>>(n, seed);
  test<Treap_Rk<long>>(n, seed);
}

