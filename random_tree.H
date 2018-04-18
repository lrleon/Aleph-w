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
#ifndef RANDOM_TREE_H
#define RANDOM_TREE_H

# include <gsl/gsl_rng.h>
# include <memory>
# include <tpl_tree_node.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

template <typename T>
class RandTree
{
  gsl_rng * r = nullptr;

  BinNode<T> * random(unsigned long n)
  {
    if (n == 0)
      return nullptr;

    // inorder position of the root
    unsigned long i = gsl_rng_uniform_int(r, n);
    BinNode<T> * root = new BinNode<T>();
    LLINK(root) = random(i);
    RLINK(root) = random(n - i - 1);
    return root;
  }

public:

  RandTree(unsigned long seed)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);
    if (r == nullptr)
      throw std::bad_alloc();
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  RandTree() : RandTree(time(nullptr)) {}

  ~RandTree()
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

  Tree_Node<T> * operator () (size_t n)
  {
    assert(n != 0);
    BinNode<T> * l = random(n - 1); // only left side
    BinNode<T> broot;
    broot.getL() = l;
    Tree_Node<T> * root = new Tree_Node<T>;
    bin_to_tree(&broot, root);
    destroyRec(l);
    return root;
  }
};




#endif
