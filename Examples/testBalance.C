
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
# include <cstdlib>
# include <cassert>

# include <iostream>
# include <string>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_balanceXt.H>


using namespace Aleph;
using namespace std;

typedef BinNodeXt<int> Node;

void print_node(Node * p, int, int)
{
  cout << "(" << p->get_key() << "," << p->getCount() << ")" ;
}


void print_key(Node * p, int, int)
{
  cout << p->get_key() << " ";
}


int main(int argc, char *argv[])
{
  int n = 10;
  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = 10;
        }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  unsigned int t = std::time(0);

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

  cout << argv[0] << " " << n << " " << t << endl;

  int value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
  Node * root = Node::NullPtr;

  for (int i = 0; i < n - 1; i++)
    {
      while (1)
	{
	  value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root, value) not_eq Node::NullPtr)
	    continue;

	  Node * p = new Node (value);
	  insert_by_key_xt(root, p);
	  break;
	}
    }

  assert(check_rank_tree(root));

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  root = balance_tree(root);

  assert(check_rank_tree(root));

  cout << endl << endl;

  preOrderRec(root, &print_key); cout << endl;

  destroyRec(root);
}
