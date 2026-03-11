
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
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_binNodeXt.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binTree.H>

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


int main(int argn, char *argc[])
{
  int i, n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  int value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
  Node * root = Node::NullPtr;

  for (i = 0; i < n - 1; i++)
    {
      value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
      if (searchInBinTree(root, value) == Node::NullPtr)
	{
	  cout << value << " ";
	  Node * p = new Node (value);
	  insert_by_key_xt(root, p);
	}
      else
	cout << "." << endl;
    }

  cout << endl << endl;

  preOrderRec(root, &print_node); cout << endl;
  inOrderRec(root, &print_node); cout << endl;

  assert(check_rank_tree(root));
  assert(check_bst(root));

  cout << endl;

  int num_nodes = root->getCount();

  for (i = 0; i < num_nodes; i++)
    {
      Node * p = select_rec(root, i);
      cout << p->get_key() << " ";
      assert(inorder_position(root, p->get_key(), p) == i);
    }

  for (i = 0; i < num_nodes; i++)
    {
      auto value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
      auto p = searchInBinTree(root, value);
      if (p == Node::NullPtr)
	assert(inorder_position(root, value) == -1);
    }

  cout << endl << endl;

  for (i = 0; i < num_nodes; i++)
    cout << select(root, i)->get_key() << " ";

  {
    Node * q = NULL;

    Node * p = select(root, 0);
    int pos = find_position(root, p->get_key() - 1, q);
    cout << endl << endl 
	 <<"Pos of " << q->get_key() - 1 << " is " << pos << endl
	 << "Next key is " << q->get_key() << endl;

    p =  select(root, num_nodes/2);
    pos = find_position(root, p->get_key(), q);
    cout << endl 
	 << "Pos of " << p->get_key() << " is " << pos << endl
	 << "key in node is " << q->get_key() << endl;
    
    pos = find_position(root, p->get_key() - 1, q);
    cout << endl 
	 << "Pos of " << p->get_key() - 1 << " is " << pos << endl
	 << "key in node is " << q->get_key() << endl;

    pos = find_position(root, p->get_key() + 1, q);
    cout << endl 
	 << "Pos of " << p->get_key() + 1 << " is " << pos << endl
	 << "key in node is " << q->get_key() << endl;

    p =  select(root, num_nodes - 1);
    pos = find_position(root, p->get_key() + 1, q);
    cout << endl 
	 << "Pos of " << p->get_key() + 1 << " is " << pos << endl
	 << "Next key is " << p->get_key() << endl << endl;
  }

  DynArray<int> keys(num_nodes);
  for (i = 0; i < num_nodes; i++)
    keys[i] = select(root, i)->get_key();

  cout << "Eliminando e insertando por clave " << num_nodes << endl;

  for (i = 0; i < num_nodes; i++)
    {
      Node * p = remove_by_pos_xt(root, i);
      insert_by_key_xt(root, p);
      cout << "(" << i << "," << num_nodes << ")" << endl;
    }
      
  cout << "listo" << endl;

  cout << "Eliminando e insertando por posicion " << num_nodes << endl;

  for (i = 0; i < num_nodes; i++)
    {
      Node * p = remove_by_pos_xt(root, i);
      insert_by_pos_xt(root, p, i);
      cout << "(" << i << "," << num_nodes << ")" << endl;
    }
      
  cout << "listo" << endl;

  cout << "Eliminando e insertando por clave " << num_nodes << endl;

  for (i = 0; i < num_nodes; i++)
    {
      Node * p = select(root, i);
      remove_by_key_xt(root, p->get_key());
      insert_by_pos_xt(root, p, i);

      assert(check_rank_tree(root));
      assert(check_bst(root));

      cout << "(" << i << "," << num_nodes << ")" << endl;
    }
      
  cout << "listo" << endl;


  Node * l, * r;

  cout << endl << endl << "Particionando recursivamente ... " << endl << endl;
  /* particion recursivamente */
  split_pos_rec(root, num_nodes/2, l, r);
  cout << " ...  listo" << endl;

  //  split_tree_pos(root, num_nodes/2, l, r);

  assert(check_rank_tree(l));
  assert(check_bst(l));

  assert(check_rank_tree(r));
  assert(check_bst(r));

  inOrderRec(l, print_node); 
  cout << " | ";
  inOrderRec(r, print_node); 
  cout << endl << endl;

  destroyRec(l);
  destroyRec(r);
}
