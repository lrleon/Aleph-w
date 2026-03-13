
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
# include <ctime>
# include <cstdlib>
# include <cassert>

# include <iostream>
# include <string>
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


int main(int argc, char *argv[])
{
  int n = 1000;
  unsigned int t = std::time(0);

  try 
    {
      if (argc > 1)
	n = std::stoi(argv[1]);

      if (argc > 2)
	t = std::stoi(argv[2]);
    }
  catch (...)
    {
      // ignore
    }

  if (n <= 0)
    {
      cout << "n must be positive" << endl;
      return 1;
    }

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

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
