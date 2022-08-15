
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

# include <stdlib.h>
# include <iostream>
# include <time.h>
# include <tpl_arrayHeap.H>

int keys [] = { 36, 32, 4, 12, 52, 59, 2, 2, 26, 1 };

using namespace std;
using namespace Aleph;


# define NIL (-1)
# define LLINK(i, n) (2*i)
# define RLINK(i, n) (2*i + 1)


void preorder(int v[], int n, int i)
{
  if (i > n)
    return;

  cout << " " << v[i];

  preorder(v, n, LLINK(i, n));
  preorder(v, n, RLINK(i, n));
}


void inorder(int v[], int n, int i)
{
  if (i > n)
    return;

  inorder(v, n, LLINK(i, n));
  cout << " " << v[i];
  inorder(v, n, RLINK(i, n));
}


int main(int argn, char* argc[])
{
  srand(time(0));
  unsigned int n = 10;

  if (argn > 1)
    n = atoi(argc[1]);

  {
    ArrayHeap<int> heap(n);
    unsigned int i, value;

    for (i = 0; i < n; i++)
      {
	value = 1 + (int) (100.0*rand()/(RAND_MAX+1.0));
	// value = keys[i];
	cout << value << " ";
	heap.insert(value);
      }

    cout << endl 
	 << "Preorder ";

    preorder(&heap[0], heap.size(), 1);
 
    cout << "\n\n";

    cout << endl 
	 << "inorder ";

    inorder(&heap[0], heap.size(), 1);

    cout << "\n\n";

    for (i = 1; i <= heap.size(); i++)
      cout << heap[i] << " ";

    cout << "\n\n";

    for (i = 0; i < n; i++)
      {
	value = heap.getMin();
	cout << value << " ";
      }

    cout << "\n\n";
  }

  {
    int * array1 = new int [n];

    int * array2 = new int [n];

    for (int i = 0; i < n; i++)
      {
	int value = 1 + (int) (100.0*rand()/(RAND_MAX+1.0));
	array1[i] = array2[i] = value;
      }
    heapsort(array1, n);

    for (int i = 0; i < n; i++)
      cout << array1[i] << " ";

    cout << endl;

    delete [] array1;

    faster_heapsort(array2, n);

    for (int i = 0; i < n; i++)
      cout << array2[i] << " ";

    cout << endl;

    delete [] array2;
  }
}
