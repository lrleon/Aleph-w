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

# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNodeUtils.H>
# include <tpl_rand_tree.H>
# include <stdlib.h>
# include <iostream>
# include <time.h>


using namespace Aleph;


DynArray<unsigned long> rand_sequence;


long aleatorio()
{
  // entre 1 y 1000
  long r = 1+ (int) (1000.0*rand()/(RAND_MAX+1.0));

  rand_sequence[rand_sequence.size()] = r;

  return r;
}

void print_aleatorio_and_reset_dynarray()
{
  cout << endl
       << "Secuencia aleatorios: ";
  for (int i = 0; i < rand_sequence.size(); i++)
    cout << " " << (unsigned long) rand_sequence[i];
  
  cout << endl;

  rand_sequence.cut(0);
}




/* 
 * ATENCION: DEBUG debe estar definido para que esta rutina funcione
 */
static void printNode(Rand_Tree_Vtl<int>::Node* node, int, int)
{ 
  cout << node->get_key() << " ";
}

int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 2;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  Rand_Tree_Vtl<int> tree;
  Rand_Tree_Vtl<int>::Node * node;
  int value;

  //  tree.init_random(153);

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	}
      while (node != NULL);

      cout << value << " ";
      node = new Rand_Tree_Vtl<int>::Node (value);
      tree.insert(node);
    }

  cout << endl << endl
       << "start-prefix ";

  preOrderRec(tree.getRoot(), printNode);
  assert(check_rank_tree(tree.getRoot()));
  assert(check_bst(tree.getRoot()));

  print_aleatorio_and_reset_dynarray();

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

  assert(check_rank_tree(tree.getRoot()));
  assert(check_bst(tree.getRoot()));
  
  cout << endl << endl
       << "start-prefix ";

  preOrderRec(tree.getRoot(), printNode);
  assert(check_rank_tree(tree.getRoot()));
  assert(check_bst(tree.getRoot()));

  print_aleatorio_and_reset_dynarray();

  destroyRec(tree.getRoot());

  cout << endl << endl << "testRandTree " << n << " " << t << endl;
}
