
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

# include <cstdlib>
# include <ctime>
# include <iostream>
# include <fstream>
# include <random>
# include <ah-errors.H>
# include <aleph.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

static void print_key(BinTree<int>::Node *p, int, int) 
{
  static ofstream output("bin-tree-aux.Tree", ios::out); 
  if (not output.is_open())
    ah_runtime_error() << "Could not open bin-tree-aux.Tree";
  output << p->get_key() << " ";
}


static void print_ex(BinTree<int>::Node *p, int, int) 
{
  static ofstream example("example-34-ar-aux.Tree", ios::out); 
  if (not example.is_open())
    ah_runtime_error() << "Could not open example-34-ar-aux.Tree";
  example << p->get_key() << " ";
}

static void print_tex(BinTree<int>::Node *p, int, int) 
{
  static ofstream tex("example-34-aux.tex", ios::out);
  if (not tex.is_open())
    ah_runtime_error() << "Could not open example-34-aux.tex";
  tex << " $" << p->get_key() << "\\ $";
}


int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); }
      catch (...) { n = 1000; }
    }

  unsigned int t = std::time(0);
  if (argc > 2)
    {
      try { t = static_cast<unsigned int>(stoul(argv[2])); }
      catch (...) { t = std::time(0); }
    }

  std::mt19937 rng(t);

  cout << "writeBinTree " << n << " " << t << endl;

  BinTree<int> tree;
  BinTree<int>::Node *node;

  for (int i = 0; i < 30; i++)
    {
      int value;
      do
	{
	  value = std::uniform_int_distribution<int>(0, 499)(rng);
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_ex);
  inOrderRec(tree.getRoot(), print_tex);
  destroyRec(tree.getRoot());
  tree.getRoot() = nullptr; // Reset tree state after destruction

  for (int i = 0; i < n; i++)
    {
      int value;
      do
	{
	  value = std::uniform_int_distribution<int>(0, n * 10 - 1)(rng);
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new BinTree<int>::Node (value);
      tree.insert(node);
    }

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
  tree.getRoot() = nullptr;
}

