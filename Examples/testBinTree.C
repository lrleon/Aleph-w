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
# include <iostream>
# include <iostream>

# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>

# include <gsl/gsl_rng.h>

using namespace std;
using namespace Aleph;

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

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? stoi(argc[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = stoi(argc[2]);

  gsl_rng * rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, t);

  cout << argc[0] << " " << n << " " << t << endl;

  BinTree<int>  tree;
  BinTree<int>::Node *node;
  int value;

  cout << "Inserting " << n << " random values in treee ...\n";

  int ins_count = 0;

  for (i = 0; i < n; i++)
    {
      do 
	{
	  value = gsl_rng_uniform_int(rng, 1000);
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

  std::ofstream * o = (std::ofstream *)&cout;
  generate_forest<Tree_Node<int>, Write> (ttree, *o);

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

  for (i = 0; i < n/4; i++)
    do 
      {
	value = gsl_rng_uniform_int(rng, 1000);
	node = tree.search(value);
	if (node != nullptr)
	  {
	    node = tree.remove(value);
	    assert(node != nullptr);
	    del_count++;
	    cout << node->get_key() << " ";
	    delete node;
	  }
      }
    while (node == nullptr);
  
  cout << endl << del_count << " deletions" << endl
       << "prefijo: ";
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;
  cout << endl;

  assert(tree.verifyBin());

  destroyRec(tree.getRoot());
  destroyRec(t1);
  destroyRec(t2); 

  cout << argc[0] << " " << n << " " << t << endl;
}

// 1018058241
