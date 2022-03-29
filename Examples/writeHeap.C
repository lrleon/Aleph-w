

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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


# include <stdlib.h>
# include <iostream>
# include <fstream>
# include <time.h>
# include <tpl_arrayHeap.H>


using namespace std;
using namespace Aleph;


# define NIL (-1)
# define LLINK(i, n) (2*i)
# define RLINK(i, n) (2*i + 1)


ofstream output("heap-ejm-aux.Tree", ios::out);
ofstream output1("heap-ejm-aux.tex", ios::out);

void preorder(int v[], int n, int i)
{
  if (i > n)
    return;

  output << " " << v[i];

  cout << " " << v[i];

  assert(v[i] > 0);

  preorder(v, n, LLINK(i, n));
  preorder(v, n, RLINK(i, n));
}


void inorder(int v[], int n, int i)
{
  if (i > n)
    return;

  assert(v[i] > 0);

  inorder(v, n, LLINK(i, n));
  output << " " << v[i];
  cout << " " << v[i];
  inorder(v, n, RLINK(i, n));
}

void level(int v[], int n)
{
  for (int i = 1; i <= n; ++i)
    {
      assert(v[i] > 0);
      output1 << v[i] << " ";
    }
} 

bool existe(ArrayHeap<int> & heap, const int & x)
{
  for (int i = 1; i < heap.size(); ++i)
    if (heap[i] == x)
      return true;

  return false;
}

int main(int argn, char* argc[])
{
  srand(time(0));
  unsigned int n = 10;

  if (argn > 1)
    n = atoi(argc[1]);

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  {
    ArrayHeap<int> heap;
    int i, value;

    for (i = 0; i < n; i++)
      {
	do
	  {
	    value = 1 + (int) (10.0*n*rand()/(RAND_MAX + 10.0*n)); 
	    assert(value > 0);
	  }
	while (existe(heap, value));
	
	heap.insert(value);
	cout << value << " ";
      }

    cout << endl;

    output << "Prefix ";
    preorder(&heap[0], heap.size(), 1);
    cout << endl;

    output << endl 
	   << "Infix ";

    inorder(&heap[0], heap.size(), 1);

    output << endl;

    level(&heap[0], heap.size());
  }
}
