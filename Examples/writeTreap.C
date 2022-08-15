
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
# include <iostream>
# include <fstream>
# include <time.h>
# include <aleph.H>
# include <tpl_treap.H>
# include <gmpfrxx.h> 
# include <tpl_binNodeUtils.H>



using namespace std;
using namespace Aleph;

long randomLong()
{
  return (long) (100.0*rand()/(RAND_MAX+1.0));
}

ofstream output("treap-aux.Tree", ios::out); 
fstream fig_file;
fstream tex_file;

Treap<int>::Node * last_node = NULL;


gsl_rng * rand_gen = NULL;

void print_pair(Treap<int>::Node *node, int, int) 
{
  mpz_class pri = 
    mpz_class(gsl_rng_max(rand_gen)) + mpz_class(node->getPriority()) ;

  pri = pri / 10000000;

  tex_file << "$(" << node->get_key() << "," << pri.get_si();

  if (node != last_node)
    tex_file << ")$, ";
  else
    tex_file << ")$ ";
}

void print_key(Treap<int>::Node *node, int, int)
{
  fig_file << node->get_key() << " ";
}

void print_treap(Treap<int>::Node *node, int, int)
{
  output << node->get_key() << " ";
}



void print_prio(Treap<int>::Node *node, int, int)
{
  mpz_class pri = 
    mpz_class(gsl_rng_max(rand_gen)) + mpz_class(node->getPriority()) ;

  pri = pri / 10000000;

  fig_file << pri.get_si() << " "; 
} 


int main(int argn, char *argc[])
{
  int n = 10;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "writeTreap " << n << " " << t << endl;

  Treap<int> tree;
  Treap<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < 30; i++)
    {
      while (true)
	{
	  value = 1 + (int) (100.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  if (node == NULL)
	    break;
	}
      node = new Treap<int>::Node (value);
      tree.insert(node);
    }

  assert(is_treap(tree.getRoot()));

  rand_gen = tree.gsl_rng_object();

  cout << "Min = " << gsl_rng_min(rand_gen) << endl
       << "Max = " << gsl_rng_max(rand_gen) << endl;

  fig_file.open("bal-04-aux.Tree", ios::out);  
  preOrderRec(tree.getRoot(), print_key); 
  fig_file << endl << "START-AUX " << endl;
  inOrderRec(tree.getRoot(), print_prio); 

  last_node = find_max(tree.getRoot());

  tex_file.open("treap-aux.tex", ios::out); 
  tex_file << "~\\ ";
  inOrderRec(tree.getRoot(), print_pair); 
  tex_file << "~\\ ";

  destroyRec(tree.getRoot()); 

  for (i = 0; i < n; i++)
    {
      value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
          node = new Treap<int>::Node (value); 
          tree.insert(node);
        }
    }

  assert(is_treap(tree.getRoot()));
  fstream output("treap-aux.Tree");
  preOrderRec(tree.getRoot(), print_treap);
}







