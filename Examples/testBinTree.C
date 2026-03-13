
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
# include <cstdlib>
# include <cassert>
# include <iostream>
# include <vector>
# include <algorithm>
# include <random>
# include <memory>

# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>

# include <gsl/gsl_rng.h>

using namespace std;
using namespace Aleph;

struct GslRngDeleter
{
  void operator()(gsl_rng * r) const { gsl_rng_free(r); }
};

using GslRngHandle = std::unique_ptr<gsl_rng, GslRngDeleter>;

static void printNode(BinTree<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}


struct Write
{
  string operator () (Tree_Node<int> * p)
  {
    return to_string(p->get_key());
  }
};

void print_forest_par(Tree_Node<int> * p)
{
  cout << "(" << p->get_key();
  p->for_each_child([](auto c) { print_forest_par(c); });
  cout << ")";
}

void print_forest_deway(Tree_Node<int> * p, const string & idx)
{
  cout << "(" << idx << ":" << p->get_key() << ")";
  size_t counter = 1;
  p->for_each_child([&idx, &counter](auto c) {
      stringstream s;
      s << idx << '.' << counter++;
      print_forest_deway(c, s.str());
    });
}

int main(int argc, char *argv[])
{
  int n = argc > 1 ? stoi(argv[1]) : 1000;

  unsigned int t = std::time(0);

  if (argc > 2)
    t = stoi(argv[2]);

  GslRngHandle rng(gsl_rng_alloc(gsl_rng_mt19937));
  gsl_rng_set(rng.get(), t);

  cout << argv[0] << " " << n << " " << t << endl;

  BinTree<int>  tree;

  int ins_count = 0;

  // Validate input to prevent infinite loops
  if (n > 1000)
    {
      cerr << "Error: n must be <= 1000 to avoid infinite loops with current RNG range." << endl;
      return 1;
    }

  cout << "Inserting " << n << " random values in tree ...\n";

  for (int i = 0; i < n; i++)
    {
      int value;
      BinTree<int>::Node *node;
      do 
	{
	  value = gsl_rng_uniform_int(rng.get(), 1000);
	  node = tree.search(value);
	}
      while (node not_eq nullptr);
      
      node = new BinTree<int>::Node (value);
      tree.insert(node);
      ins_count++;

      cout << value << " ";
    }
  cout << endl << endl;

  Tree_Node<int> * ttree = 
    bin_to_forest<Tree_Node<int>, BinTree<int>::Node>(tree.getRoot());

  // Check if forest conversion succeeded before proceeding
  if (ttree == nullptr)
    {
      cout << "Warning: Empty tree, no forest to process." << endl;
      return 0;
    }

  generate_forest<Tree_Node<int>, Write> (ttree, std::cout);

  cout << endl << "Secuencia paréntesis: ";  
  print_forest_par(ttree);
  auto rc = ttree->get_right_sibling();
  while (rc != nullptr)
    {
      print_forest_par(rc);
      rc = rc->get_right_sibling();
    }
  cout << endl << endl;

  cout << "Secuencia en notación Deway: ";
  print_forest_deway(ttree, "1");
  rc = ttree->get_right_sibling();
  size_t counter = 2;
  while (rc != nullptr)
    {
      stringstream s;
      s << counter++;
      print_forest_deway(rc, s.str());
      rc = rc->get_right_sibling();
    }
  cout << endl;

  destroy_forest(ttree);

  assert(tree.verifyBin());
  cout << endl << ins_count << " insertions" << endl
       << "prefijo: " << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  cout << "sufijo: " << endl;
  postOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  cout << "infijo: " << endl;
  inOrderRec(tree.getRoot(), printNode);
  cout << endl << endl;

  cout << "Code = " << code(tree.getRoot()) << endl;

  int ipl = internal_path_length(tree.getRoot());
  
  cout << "IPL = " << ipl << endl
       << "EPL = " << ipl + 2*n << endl;

  BinTree<int>::Node * t1 = nullptr, * t2 = nullptr;

  BinTree<int>::Node * aux;
  aux = copyRec(tree.getRoot());

  split_key(aux, 487, t1, t2);
  cout << "t1: ";
  preOrderRec(t1, printNode); cout << endl << endl;
  cout << "t2: ";
  preOrderRec(t2, printNode); cout << endl << endl;

  int del_count = 0;

  cout << "Removing " << n/4 << " keys" << endl;

  std::vector<int> keys;
  Aleph::for_each_in_order<BinTree<int>::Node>(tree.getRoot(), [&keys](auto node) {
      keys.push_back(node->get_key());
  });
  std::shuffle(keys.begin(), keys.end(), std::mt19937(t));

  for (size_t i = 0; i < (size_t)(n/4) && i < keys.size(); i++)
    {
      int value = keys[i];
      BinTree<int>::Node * node = tree.remove(value);
      if (node != nullptr)
	{
	  del_count++;
	  cout << node->get_key() << " ";
	  delete node;
	}
    }
  
  cout << endl << del_count << " deletions" << endl
       << "prefijo: ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;
  cout << endl;

  assert(tree.verifyBin());

  destroyRec(tree.getRoot());
  destroyRec(t1);
  destroyRec(t2);

  cout << argv[0] << " " << n << " " << t << endl;
  }

