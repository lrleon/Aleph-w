
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include <string>
# include <tpl_arrayQueue.H>

using namespace std;

int count = -1;

struct Foo
{
  int * ptr;

  void swap(Foo & f)
  {
    std::swap(ptr, f.ptr);
  }

  Foo() : ptr(NULL)
  {
    ptr = new int;
    *ptr = ::count--;
  }

  Foo(int i) : ptr(NULL)
  {
    ptr = new int;
    *ptr = i;
  }

  Foo(const Foo & f) : ptr(NULL)
  {
    ptr = new int;
    *ptr = *f.ptr;
  }

  Foo(Foo && f) : ptr(NULL)
  {
    std::swap(ptr, f.ptr);
  }

  Foo & operator = (const Foo & f)
  {
    if (this == &f)
      return *this;

    *ptr = *f.ptr;

    return *this;
  }

  Foo & operator = (Foo && f)
  {
    swap(f);
    return *this;
  }

  ~Foo()
  {
    if (ptr != NULL)
      {
	delete ptr;
	ptr = NULL;
      }
  }

  // operator int () { return *ptr; }

  operator int () const { return *ptr; }
};

template <typename T>
void print(const ArrayQueue<T> & q)
{
  cout << "capacity = " << q.capacity() << endl
       << "size     = " << q.size() << endl;

  for (int i = 0; i < q.size(); ++i)
    cout << (T) q.front(i) << " ";
  cout << endl ;

  for (int i = 0; i < q.size(); ++i)
    cout << (T) q.rear(i) << " ";
  cout << endl
       << endl;
}

template <typename T>
ArrayQueue<T> create_queue(int n)
{
  cout << "Creating rval queue ";
  ArrayQueue<T> q;
  for (int i = 0; i < n; ++i)
    cout << q.put(T(i)) << " ";
  cout << endl;

  return q;
}

int main(int, char * argc[])
{
  size_t n = atoi(argc[1]);
  ArrayQueue<int> q(n);

  print(q);

  cout << "Inserting " << n << " values ";
  for (int i = 0; i < n; i++)
    cout << q.put(i) << " ";
  cout << " done!" << endl << endl;

  print(q);

  cout << "Consulting all values until underflow ";
  for (int i = 0; 1; i++)
    {
      try
	{
	  int val = q.rear(i);
	  cout << val  << " " ;
	}
      catch (std::range_error & exc)
	{
	  cout << endl << exc.what() << endl;
	  break;
	}
    }
  cout << " done! " << endl << endl;

  cout << "Deleting all values in steps of 3 until underflow ";
  while (1)
    {
      try
	{
	  printf("%d ", q.getn(3));
	}
      catch (std::underflow_error & exc)
	{
	  cout << endl << exc.what() << endl;
	  break;
	}
    }
  cout << " done! " << endl << endl;

  print(q);

  cout << "Inserting " << n << " values " << endl;
  for (int i = 0; i < n; i++)
    cout << q.put(i) << " ";
  cout << " done!" << endl << endl;

  print(q);

  int m = atoi(argc[2]);

  cout << "Deleting " << m << " items" << endl;
  for (int i = 0; i < m; i++)
    cout << q.get() << " ";

  cout << "done!" << endl << endl;

  cout << "q = " << endl;
  print(q);

  cout << "Testing constructors ... " << endl;

  ArrayQueue<int> q1 = q;

  print(q1);

  ArrayQueue<int> q2 = create_queue<int>(n);
  print(q2);


  ArrayQueue<Foo> q3 = create_queue<Foo>(n);
  print(q3);

  printf("Ended\n");
}
