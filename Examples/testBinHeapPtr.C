
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
