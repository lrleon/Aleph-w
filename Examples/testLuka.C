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
# include <string.h>
# include <iostream>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace std;

gsl_rng * r = NULL;


void init_random(int seed)
{
  r = gsl_rng_alloc (gsl_rng_mt19937);

  if (r == NULL)
    throw std::bad_alloc();

  gsl_rng_set(r, seed % gsl_rng_max(r));
}


size_t get_rand(const size_t & n)
{
  return gsl_rng_uniform_int(r, n) + 1;
}


    template <class Node>
Node * random_tree(const size_t & n)
{
  if (n == 0) 
    return Node::NullPtr;

  Node * root = new Node; // apartar memoria nodo 

      /* sorteo que define aleatoriamente la posición infija de la raíz */
  const size_t i = get_rand(n);

  LLINK(root) = random_tree<Node>(i - 1); // aleatorio de r - 1 nodos
  RLINK(root) = random_tree<Node>(n - i); // aleatorio de n - r nodos

  return root;
}


    template <class Node>
string luka(Node * p)
{
  if (p == Node::NullPtr)
    return string("b");

  return string("a") + luka(LLINK(p)) + luka(RLINK(p));
}

    template <class Node>
Node * luka_to_tree(char *& cod)
{
  if (*cod == '\0')
    return Node::NullPtr;

  if (*cod++ == 'b')
    return Node::NullPtr;

  Node * p = new Node;

  LLINK(p) = luka_to_tree<Node>(cod);
  RLINK(p) = luka_to_tree<Node>(cod);

  return p;
}

int main(int argn, char *argc[])
{ 
  const size_t n = argc[1] ? atol(argc[1]) : 10;

  int t = time(NULL);

  if (argn > 2)
    t = atol(argc[2]);

  cout << "testLuka " << n << " " << t << endl;

  init_random(t);

  BinNode<int> * r = random_tree< BinNode<int> >(n);

  cout << "luka(r) = " << luka(r) << endl;

  char * cod = strdup(luka(r).c_str());

  BinNode<int> * aux =  luka_to_tree< BinNode<int> >(cod);

  assert(areSimilar(aux, r));

  destroyRec(aux);
  destroyRec(r);
}
