
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
# include <ctime>
# include <string>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_treapRk.H>
# include <tpl_binNodeUtils.H>

# include <cstdlib>
# include <cassert>
using namespace std;
# include <cstdlib>
# include <cassert>
using namespace Aleph;


void printNode(Treap_Rk<int>::Node *node, int, int)
{
  cout << node->get_key() << " ";
}

void printPrio(Treap_Rk<int>::Node *node, int, int)
{
  cout << node->getPriority() << " ";
}


int main(int argc, char *argv[])
{
  int n = 10;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); } catch (...) { n = 10; }
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

  cout << "testTreap_Rk " << n << " " << t << endl;

  DynArray<int> keys;
  Treap_Rk<int> tree;
  Treap_Rk<int>::Node *node;
  int i, value;

  cout << "Inserting " << n << " random values in tree ...\n";

  for (i = 0; i < n; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	}
      while (node not_eq NULL);

      cout << value << " ";
      node = new Treap_Rk<int>::Node (value);
      tree.insert(node);
      keys.append(value);
    }

  assert(is_treap(tree.getRoot()));

  assert(check_rank_tree(tree.getRoot()));

  cout << endl << endl
       << "Preorden" << endl;

  preOrderRec(tree.getRoot(), printNode);

  cout << endl << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl << endl;

  for (i = 0; i < n; i++)
    {
      node = tree.select(i);
      cout << node->get_key() << " ";
    }

  cout << endl << endl;

  cout << "Lista de posiciones infijas" << endl;

  for (i = 0; i < n; ++i)
    {
      std::pair<int,Treap_Rk<int>::Node*> pos = tree.position(keys[i]);
      cout << keys[i] << "<-->" << pos.first << endl;
    }

  cout << endl << endl;

  for (i = 0; i < n/2; i++)
    { 
      value = keys[i];
      node = tree.remove(value);
      assert(node != NULL);

      cout << value << " ";
      delete node;
    }

  cout << endl << "verifying Treap_Rk after deletions ... " 
       << endl;
  assert(is_treap(tree.getRoot()));
  assert(check_rank_tree(tree.getRoot()));
  cout << " done" << endl;

  cout << "Preorden" << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl;

  cout << "The path length is " << internal_path_length(tree.getRoot())
       << endl;

  cout << "Recorrido por iterador" << endl;
  for (Treap_Rk<int>::Iterator it(tree); it.has_curr(); it.next())
    cout << KEY(it.get_curr()) << " ";
  cout << endl;
    
  n = tree.size();
  size_t beg = n/4;
  size_t end = 3*n/4;
  cout << "Eliminacion de rango [" << beg << " .. " << end << "]" << endl;
  Treap_Rk<int>::Node * removed_tree = tree.remove(beg, end);

  assert(is_treap(tree.getRoot()));
  assert(is_treap(removed_tree));

  // Verify rank metadata is correctly updated after removal
  assert(check_rank_tree(tree.getRoot()));
  assert(check_rank_tree(removed_tree));

  cout << "Arbol restante" << endl;
  inOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "Arbol eliminado" << endl;
  inOrderRec(removed_tree, printNode);
  cout << endl;

  destroyRec(removed_tree); 
  destroyRec(tree.getRoot()); 
  tree.getRoot() = nullptr;

  cout << endl << "testTreap_Rk " << n << " " << t << endl;
}







