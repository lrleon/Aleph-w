
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

