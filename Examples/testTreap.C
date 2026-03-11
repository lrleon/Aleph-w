
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
# include <iostream>
# include <time.h>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_treap.H>
# include <tpl_binNodeUtils.H>

using namespace std;
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

int main(int argn, char *argc[])
{
  int n = 10;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

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







