
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







