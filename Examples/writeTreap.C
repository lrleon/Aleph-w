
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

# include <cstdlib>
# include <cassert>
# include <charconv>
# include <ctime>
# include <gsl/gsl_rng.h>
# include <fstream>
# include <iostream>
# include <string_view>
# include <ah-errors.H>
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

ofstream output;
fstream fig_file;
fstream tex_file;

Treap<int>::Node * last_node = nullptr;


gsl_rng * rand_gen = nullptr;

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


int main(int argc, char *argv[])
{
  int n = 10;
  unsigned int t = std::time(0);
  int value;

  if (argc > 1)
    {
      const std::string_view arg = argv[1];
      auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), n);
      ah_invalid_argument_if(ec != std::errc{} or ptr != arg.data() + arg.size() or n < 0)
        << "Invalid or out-of-range value for n";
    }

  if (n <= 0)
    {
      cerr << "n must be positive" << endl;
      return 1;
    }

  if (argc > 2)
    {
      const std::string_view arg = argv[2];
      auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), t);
      ah_invalid_argument_if(ec != std::errc{} or ptr != arg.data() + arg.size())
        << "Invalid or out-of-range value for t";
    }

  srand(t);

  output.open("treap-aux.Tree", ios::out | ios::trunc);
  ah_runtime_error_if(not output.is_open())
    << "Could not open treap-aux.Tree";
  fig_file.open("bal-04-aux.Tree", ios::out | ios::trunc);
  ah_runtime_error_if(not fig_file.is_open())
    << "Could not open bal-04-aux.Tree";
  tex_file.open("treap-aux.tex", ios::out | ios::trunc);
  ah_runtime_error_if(not tex_file.is_open())
    << "Could not open treap-aux.tex";

  cout << "writeTreap " << n << " " << t << endl;

  Treap<int> tree;
  tree.set_seed(t);
  Treap<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in tree ...\n";

  for (i = 0; i < n; i++)
    {
      while (true)
	{
	  value = 1 + (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	  if (node == nullptr)
	    break;
	}
      node = new Treap<int>::Node (value);
      tree.insert(node);
    }

  assert(is_treap(tree.getRoot()));

  rand_gen = tree.gsl_rng_object();

  cout << "Min = " << gsl_rng_min(rand_gen) << endl
       << "Max = " << gsl_rng_max(rand_gen) << endl;

  preOrderRec(tree.getRoot(), print_key); 
  fig_file << endl << "START-AUX " << endl;
  inOrderRec(tree.getRoot(), print_prio); 

  last_node = find_max(tree.getRoot());

  tex_file << "~\\ ";
  inOrderRec(tree.getRoot(), print_pair); 
  tex_file << "~\\ ";

  destroyRec(tree.getRoot()); 
  tree.getRoot() = nullptr; // Reset tree state after destruction 
  tree.set_seed(t);

  for (i = 0; i < n; i++)
    {
      value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == nullptr)
	{
          node = new Treap<int>::Node (value); 
          tree.insert(node);
        }
    }

  assert(is_treap(tree.getRoot()));

  preOrderRec(tree.getRoot(), print_treap);
}






