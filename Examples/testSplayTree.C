
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
# include <string>
# include <aleph.H>
# include <tpl_splay_tree.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;


static void print(Splay_Tree<int>::Node *p, int, int) 
{
  cout << p->get_key() << " ";
}

static int r[] = { 776, 846, 736, 515, 528, 677, 404, 629, 879,  762 };

int main(int argc, char *argv[])
{
  int n = 1000;
  unsigned int t = std::time(0);
  int value;

  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = 1000;
        }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  if (argc > 2)
    {
      try
        {
          t = stoi(argv[2]);
        }
      catch (...)
        {
          t = std::time(0);
        }
    }

  srand(t);

  cout << "testSplayTree " << n << " " << t << endl;

  Splay_Tree<int> tree;
  Splay_Tree<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  unsigned int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*10.0*rand()/(RAND_MAX+1.0));
      //      value = r[i];
      node = tree.search(value);
      if (node == NULL)
	{
	  insCount++;
	  node = new Splay_Tree<int>::Node (value);
	  tree.insert(node);
	  cout << value << " ";
	}
    }
  cout << endl;

  cout << endl << "Preorder " << endl << endl;
  preOrderRec(tree.getRoot(), print);

  cout << endl << insCount << " Items inserted" << endl;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node != NULL)
	{
	  delCount++;
	  delete node;
	}
    }
    
  cout << delCount << " Items removed" << endl;

  destroyRec(tree.getRoot());
  cout << "testSplayTree " << n << " " << t << endl;
}
