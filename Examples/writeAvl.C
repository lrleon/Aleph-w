
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
# include <string>
# include <aleph.H>
# include <tpl_avl.H>
# include <tpl_binNodeUtils.H>

# include <cassert>
using namespace std;
# include <cassert>
using namespace Aleph;

ofstream file("avl-tree-aux.Tree", ios::out); 


static void print_key(Avl_Tree<int>::Node *p, int, int) 
{
  file << p->get_key() << " ";
}


int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); } catch (...) { n = 1000; }
    }

  if (n <= 0)
    {
      cerr << "n must be positive" << endl;
      return 1;
    }

  unsigned int t = std::time(0);
  if (argc > 2)
    {
      try { t = stoul(argv[2]); } catch (...) { t = std::time(0); }
    }

  srand(t);

  cout << "writeAvl " << n << " " << t << endl;

  Avl_Tree<int> tree;
  Avl_Tree<int>::Node *node;
  int i, value;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    {
      do
	{

	  value = (int) (n*10.0*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	} while (node not_eq NULL);
      node = new Avl_Tree<int>::Node (value);
      tree.insert(node);
    }
  cout << endl << "verifying avl tree after insertions ... " 
       << endl;
  assert(is_avl(tree.getRoot()));
  cout << " done" << endl;

  preOrderRec(tree.getRoot(), print_key);

  destroyRec(tree.getRoot());
}

