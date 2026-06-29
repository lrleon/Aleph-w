
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

# include <cstdlib>
# include <iostream>
# include <ctime>
# include <string>
# include <tpl_arrayHeap.H>

int keys [] = { 36, 32, 4, 12, 52, 59, 2, 2, 26, 1 };

int * keys_ptr = keys;

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




int main(int argc, char* argv[])
{
  srand(std::time(0));
  int n = 10;

  if (argc > 1)
    {
      try
        {
          n = stoi(argv[1]);
        }
      catch (...)
        {
          n = 10;
        }
    }

  if (n <= 0)
    {
      cerr << "Error: n must be a positive integer." << endl;
      return 1;
    }

  {
    // ArrayHeap<int*> heap(n, [] (int * ptr1, int * ptr2)
    // {
    //   return *ptr1 < *ptr2;
    // });
  }

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
