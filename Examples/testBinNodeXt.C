
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

  // Validate input to prevent null root dereferencing
  if (n <= 1)
    {
      cerr << "Error: n must be greater than 1 for meaningful tree operations." << endl;
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

  for (int i = 0; i < num_nodes; i++)
    {
      Node * p = select_rec(root, i);
      cout << p->get_key() << " ";
      assert(inorder_position(root, p->get_key(), p) == i);
    }

  for (int i = 0; i < num_nodes; i++)
    {
      auto value = (int) (100.0*n*rand()/(RAND_MAX+1.0));
      auto p = searchInBinTree(root, value);
      if (p == Node::NullPtr)
	assert(inorder_position(root, value) == -1);
    }

  cout << endl << endl;

  for (int i = 0; i < num_nodes; i++)
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
  for (int i = 0; i < num_nodes; i++)
    keys[i] = select(root, i)->get_key();

  cout << "Eliminando e insertando por clave " << num_nodes << endl;

  for (int i = 0; i < num_nodes; i++)
    {
      Node * p = remove_by_pos_xt(root, i);
      insert_by_key_xt(root, p);
      cout << "(" << i << "," << num_nodes << ")" << endl;
    }
      
  cout << "listo" << endl;

  cout << "Eliminando e insertando por posicion " << num_nodes << endl;

  for (int i = 0; i < num_nodes; i++)
    {
      Node * p = remove_by_pos_xt(root, i);
      insert_by_pos_xt(root, p, i);
      cout << "(" << i << "," << num_nodes << ")" << endl;
    }
      
  cout << "listo" << endl;

  cout << "Eliminando e insertando por clave " << num_nodes << endl;

  for (int i = 0; i < num_nodes; i++)
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
