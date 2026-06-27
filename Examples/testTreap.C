
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
# include <iostream>
# include <string>
# include <ctime>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_treap.H>
# include <tpl_binNodeUtils.H>

# include <cstdlib>
# include <cassert>
using namespace std;
# include <cstdlib>
# include <cassert>
using namespace Aleph;

DynArray<unsigned long> rand_sequence;

long aleatorio()
{
  while (true)
    {
      // entre 1 y 1000
      unsigned long r = 1+ (int) (1000.0*rand()/(RAND_MAX+1.0));

      if (sequential_search(rand_sequence, r, 0, rand_sequence.size() - 1) ==
	  -1)
	{
	  rand_sequence[rand_sequence.size()] = r;
	  
	  return r;
	}
    }
}

void print_aleatorio_and_reset_dynarray()
{
  cout << endl
       << "Secuencia aleatorios: ";
  for (int i = 0; i < rand_sequence.size(); i++)
    cout << " " << (long) rand_sequence[i];
  
  cout << endl;

  rand_sequence.cut(0);
}

void printNode(Treap<int>::Node *node, int, int)
{
  cout << node->get_key() << " ";
}

void printNode(Treap<int>::Node *node, int, bool)
{
  cout << node->get_key() << " ";
}

void printPrio(Treap<int>::Node *node, int, int)
{
  cout << node->getPriority() << " ";
}

void printPair(Treap<int>::Node *node, int, int)
{
  cout << "(" << node->get_key() << "," << node->getPriority() << ") ";
}

int main(int argc, char *argv[])
{
  int n = 10;
  unsigned int t = std::time(0);
  int value;

  try 
    {
      if (argc > 1)
	n = std::stoi(argv[1]);

      if (argc > 2)
	t = std::stoi(argv[2]);
    }
  catch (...)
    {
      // ignore
    }

  if (n <= 0)
    {
      cout << "n must be positive" << endl;
      return 1;
    }

  srand(t);

  cout << "testTreapRec " << n << " " << t << endl;

  Treap<int> tree;
  Treap<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	}
      while (node not_eq NULL);

      node = new Treap<int>::Node (value);
      tree.insert(node);
      cout << "(" << value << "," << PRIO(node) << ") ";
    }

  cout << endl << endl
       << "level order" << endl;
  
  level_traverse(tree.getRoot(), [] (Treap<int>::Node * p)
		 {
		   cout << p->get_key() << " ";
		   return true;
		 });

  assert(is_treap(tree.getRoot()));

  cout << endl << endl
       << "Preorden" << endl;

  preOrderRec(tree.getRoot(), printNode);

  cout << endl << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl << endl;

  print_aleatorio_and_reset_dynarray(); 

  cout << endl << endl;


  cout << endl << endl
       << "Preorden prio" << endl;

  preOrderRec(tree.getRoot(), printPrio);

  cout << endl << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl << endl;

  //  print_aleatorio_and_reset_dynarray(); 

  cout << endl << endl;

  for (i = 0; i < n/2; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	} 
      while (node == NULL);

      cout << value << " ";
      delete node;
    }

  cout << endl << "verifying Treap after deletions ... " 
       << endl;
  assert(is_treap(tree.getRoot()));
  cout << " done" << endl;

  cout << "Preorden" << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl;

  cout << "The path length is " << internal_path_length(tree.getRoot())
       << endl;

  destroyRec(tree.getRoot()); 

  cout << endl << "testTreapRec " << n << " " << t << endl;
}







