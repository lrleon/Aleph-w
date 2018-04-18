/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# include <gsl/gsl_rng.h>
# include <iostream>
# include <tpl_dynBinHeap.H>
# include <tpl_dynSetTree.H>

using namespace std;


template <class Tree>
void test(size_t n, unsigned long seed)
{
  cout << "Testing for " << typeid(Tree).name() << endl
       << endl;
  auto r= gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  Tree tree;
  for (auto i = 0; i < n; ++i)
    {
      auto val = gsl_rng_get(r);
      auto p = tree.insert(new typename Tree::Node(val));
      if (p == nullptr)
	delete p;
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
  gsl_rng_free(r);
}

void usage()
{
  cout << "test-tree-itor [n] [seed]" << endl
       << endl;
  exit(0);
}

int main(int argc, char *argv[])
{
  size_t n = argc > 1 ? atoi(argv[1]) : 10;
  unsigned long seed = argc > 2 ? atoi(argv[2]) : 0;

  test<BinTree<long>>(n, seed); 
  test<Avl_Tree<long>>(n, seed); 
  test<Splay_Tree<long>>(n, seed); 
  test<Rb_Tree<long>>(n, seed);
  test<Rand_Tree<long>>(n, seed);
  test<Treap<long>>(n, seed);
  test<Treap_Rk<long>>(n, seed);
}

