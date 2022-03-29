

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
