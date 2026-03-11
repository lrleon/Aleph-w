
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
# include <tpl_binHeap.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

struct Foo
{
  int number;

  Foo(int i) : number(i) { /* empty */ }

  bool operator < (const Foo& r)
  {
    return number < r.number;
  }

  bool operator <= (const Foo& r)
  {
    return number <= r.number;
  }
};


int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testBinHeap " << n << " " << t << endl;

  BinHeap<Foo*> heap;
  BinHeap<Foo*>::Node *node;
  Foo *ptr;
  int i;

  for (i = n - 1; i >= 0; i--)
    {
      ptr = new Foo (i);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }

  //  assert(heap.verify_heap());  

  int value;

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node ;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }

  //  assert(heap.verify_heap());  

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }

  //  assert(heap.verify_heap());

  for (i = 0; i < n/2; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }
  //  assert(heap.verify_heap());  

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      ptr = new Foo (value);
      node = new BinHeap<Foo*>::Node;
      node->get_key() = ptr;
      heap.insert(node);
    }
  //  assert(heap.verify_heap());  

  for (i = 0; i < n + n/2; i++)
    {
      node = heap.getMin();
      delete node->get_key();
      delete node;
    }
  //  assert(heap.verify_heap());  

}
