
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
      return t;
    };

  Tree tree = (*create)(n);
} 

int main(int argc, char * argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = std::stoi(argv[1]); }
      catch (...) { n = 1000; }
    }

  test_map_tree<DynMapTree<int, int, Avl_Tree>>(n);

  test_container <DynList, int> (n);

  test_container <DynDlist, int> (n);

  test_tree <Avl_Tree, int> (n);

  return 0;
}
