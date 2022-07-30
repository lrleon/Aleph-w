
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_dynSetTree.H>
# include <tpl_dynMapTree.H>
# include <tpl_arrayStack.H>

using namespace std;
using namespace Aleph;

template <template <class> class Container, typename T>
void test_container(int n)
{
  cout << "Probando con contenedor tipo lista" << endl;
  Container<T> l;

  cout << "append de R values\n";
  for (int i = 0; i < n; ++i)
    l.append(T(i));
  cout << endl;

  cout << "append de L values\n";
  for (int i = 0; i < n; ++i)
    {
      T t = i;
      l.append(t);
    }
  cout << endl;

  cout << "insert de R values\n";
  for (int i = 0; i < n; ++i)
    l.insert(T(i));
  cout << endl;

  cout << "insert de L values\n";
  for (int i = 0; i < n; ++i)
    {
      T t = i;
      l.insert(t);
    }
  cout << endl;

  for (typename Container<T>::Iterator it(l); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;
}

template <template <class, class> class Tree, typename T>
void test_tree(int n)
{
  cout << "Probando con contenedor tipo arbol" << endl;
  DynSetTree<T, Tree> l;

  cout << "insert de R values\n";
  for (int i = 0; i < n; ++i)
    l.insert(T(i));
  cout << endl;

  cout << "insert de L values (no debe insertar en arbol)\n";
  for (int i = 0; i < n; ++i)
    {
      T t = i;
      l.insert(t);
    }
  cout << endl;

  cout << "insert_dup de R values\n";
  for (int i = 0; i < n; ++i)
    l.insert_dup(T(i));
  cout << endl;

  cout << "insert_dup de L values\n";
  for (int i = 0; i < n; ++i)
    l.insert_dup(i);
  cout << endl;

  l.for_each([/* Lambda */] (const T & key) { cout << key << " "; });
  cout << endl;
}

template <class Tree>
void test_map_tree(int n)
{
  Tree (*create)(int) = [/* Lambda */] (int n) -> Tree
    {
      Tree t;
      for (int i = 0; i < n; ++i)
	t.insert(i, i + 2);
      return std::move(t);
    };

  Tree tree = (*create)(n);
} 

int main(int, char * argc[])
{
  int n = atoi(argc[1]);

  test_map_tree<DynMapTree<int, int, Avl_Tree>>(n);

  return 0;

  test_container <DynList, int> (n);

  test_container <DynDlist, int> (n);

  test_tree <Avl_Tree, int> (n);
}
