
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
# include <aleph.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

using namespace Aleph;
using namespace std;

typedef BinNode<int> Node;

void print_node(Node * p, int, int)
{
  //  cout << "(" << p->get_key() << "," << p->getCount() << ")" ;
  cout << p->get_key() << " " ;
}


int main(int argc, char *argv[])
{
  int n = argc > 1 ? atoi(argv[1]) : 10;

  unsigned int t = std::time(0);

  if (argc > 2)
    t = atoi(argv[2]);

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  int i, value;
  Node * root1 = Node::NullPtr;

  for (i = 0; i < n; i++)
    {
      while (1)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root1, value) == Node::NullPtr)
	    break;
	}
      Node * p = new Node (value);
      root1 = insert_root(root1, p);
    }

  assert(check_bst(root1));

  cout << endl << "T1" << endl;
  preOrderRec(root1, &print_node); cout << endl << endl;

  Node * root2 = Node::NullPtr;
  for (i = 0; i < n; i++)
    {
      while (1)
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  if (searchInBinTree(root1, value) == Node::NullPtr and
	      searchInBinTree(root2, value) == Node::NullPtr)
	    break;
	}
      Node * p = new Node (value);
      root2 = insert_root(root2, p);
    }

  assert(check_bst(root2));

  cout << endl << "T2" << endl;
  preOrderRec(root2, &print_node); cout << endl;

  Node * r1 = copyRec(root1);
  Node * r2 = copyRec(root2);

  Node * dup = Node::NullPtr;

  Node * root = join(root1, root2, dup);
  assert(check_bst(root));
  cout << endl << "Join(T1, T2)" << endl;
  preOrderRec(root, &print_node); cout << endl << endl;
  destroyRec(root);
  destroyRec(dup); 
  dup = Node::NullPtr; // Reinitialize dup to avoid dangling pointer

  Node * res = join(r2, r1, dup); // renamed r to res to avoid confusion with r1/r2
  assert(check_bst(res));
  cout << endl << "Join(T2, T1)" << endl;
  preOrderRec(res, &print_node); cout << endl << endl;
  destroyRec(res);
  destroyRec(dup);

  {
    Node * t1 = Node::NullPtr, * t2 = Node::NullPtr;

    for (i = 0; i < n; ++i)
      {
	insert_in_bst(t1, new Node (rand()));
	insert_in_bst(t2, new Node (rand()));
      }

    cout << "t1: ";
    preOrderRec(t1, &print_node); cout << endl << endl;

    cout << "t2: ";
    preOrderRec(t2, &print_node); cout << endl << endl;

    Node * dup_inner = Node::NullPtr;
    Node * j = join(t1, t2, dup_inner);

    cout << "join(t1, t2, dup): ";
    preOrderRec(j, &print_node); cout << endl << endl;

    cout << "dup: ";
    preOrderRec(dup_inner, &print_node); cout << endl << endl;

    destroyRec(j);
    destroyRec(dup_inner);
  }
}
