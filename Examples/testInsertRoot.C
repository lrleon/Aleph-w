
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

# include <ctime>
# include <iostream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <string>

# include <cstdlib>
# include <cassert>
using namespace std;
# include <cstdlib>
# include <cassert>
using namespace Aleph;


static void printNode(BinNode<int>* node)
{
  cout << node->get_key() << " ";
}


int main(int argc, char * argv[])
{
  int i;
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

  DynArray<int> array;

  cout << argv[0] << " " << n << " " << t << endl;

  BinNode<int> *root = new BinNode<int> ((int) (10.0*n*rand()/(RAND_MAX+1.0)));
  cout << root->get_key() << " ";

  array[0] = root->get_key();

  BinNode<int> *p;
  int value;
  int ins_count = 1;

  for (i = 1; i < n; i++)
    {
      do 
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  p = searchInBinTree(root, value);
	}
      while (p != NULL);

      p = new BinNode<int> (value);
      cout << value << " ";
      root = insert_root(root, p);

      array[i] = value;

      ins_count++;
    }
  
  cout << endl << ins_count << " items inserted" << endl;
  assert(check_bst(root));

  cout << "prefix: ";
  preOrderThreaded(root, printNode);
  cout << endl;

  int removals = (n < 10) ? n : 10;
  for (i = 0; i < removals; i++)
    {
      int idx;
      do
	{
	  idx = (int) (1.0*n*rand()/(RAND_MAX+1.0));

	  value = array[idx];

	  p = searchInBinTree(root, value);

	}
      while (p == NULL);

      cout << value << " ";

      p = remove_from_bst(root, value);

      assert(p != NULL);
      delete p; // Free the detached node to prevent memory leak
    }

  cout << endl;
  preOrderThreaded(root, printNode);
  cout << endl;
  destroyRec(root);
}
