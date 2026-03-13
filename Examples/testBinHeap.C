
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
# include <tpl_dynArray.H>
# include <tpl_binHeap.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

static void printNode(BinHeap<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}


int main(int argc, char *argv[])
{
  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); } catch (...) { n = 1000; }
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

  cout << "testBinHeap " << n << " " << t << endl;

  BinHeap<int> heap;
  BinHeap<int>::Node *node;
  int i;

  for (i = n - 1; i >= 0; i--)
    {
      node = new BinHeap<int>::Node (i);
      heap.insert(node);
    }

  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node;
    }

  assert(heap.verify_heap());  

  int value;

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = new BinHeap<int>::Node (value);
      heap.insert(node);
      cout << value << " ";
    }
  cout << endl;

  assert(heap.verify_heap());

  for (i = 0; i < n; i++)
    {
      node = heap.getMin();
      delete node;
    }

  assert(heap.verify_heap());  
  assert(heap.size() == 0);

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = new BinHeap<int>::Node (value);
      heap.insert(node);
    }

  assert(heap.verify_heap());

  for (i = 0; i < n/2; i++)
    {
      node = heap.getMin();
      delete node;
    }
  assert(heap.verify_heap());  

  for (i = n - 1; i >= 0; i--)
    {
      value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = new BinHeap<int>::Node (value);
      heap.insert(node);
    }
  assert(heap.verify_heap());  

  for (i = 0; i <= n + n/2; i++)
    {
      try 
	{
	  node = heap.getMin();
	  delete node;
	}
      catch (exception & e)
	{
	  cout << e.what() << endl;
	}
    }
    assert(heap.verify_heap());  
    assert(heap.size() == 0);


    n = 4;
    DynArray<BinHeap<int>::Node*> nodes(2*n);
    //    BinHeap<int>::Node* nodes[2*n];
    for (i = 2*n - 1; i >= 0; i--)
      {
	node = new BinHeap<int>::Node (i);
	heap.insert(node);
	nodes[i] = node;
      }

    assert(heap.verify_heap());  

    for (i = 0; i < n/2; i++)
      {
	value = (int) (1.0*n*rand()/(RAND_MAX+1.0));
	if (nodes[value] != NULL)
	  {
	    node = heap.remove(nodes[value]);
	    delete node;
	    nodes[value] = NULL;
	  }
      }

    assert(heap.verify_heap());  

    heap.remove_all_and_delete();

    for (i = 0; i < n/2; i++)
      {
	value = (int) (n*100.0*rand()/(RAND_MAX+1.0));
	node = new BinHeap<int>::Node (value);
	heap.insert(node);
      }
    assert(heap.verify_heap());  

    

    while (heap.size() > 0)
      {
	node = heap.getMin();
	delete node;
      }

    assert(heap.verify_heap());  
    assert(heap.size() == 0);

    for (i = n - 1; i >= 0; i--)
      {
	node = new BinHeap<int>::Node (i);
	heap.insert(node);
      }
    assert(heap.verify_heap());  

    for (i = 0; i < n; i++)
      {
	node = heap.getMin();
	delete node;
      }
    assert(heap.verify_heap());  
    assert(heap.size() == 0);

    cout << "End" << endl;
}
