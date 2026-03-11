
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
# include <string>
# include <cstdlib>
# include <cerrno>
# include <climits>
# include <stdexcept>
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

  Foo() : ptr(nullptr)
  {
    ptr = new int;
    *ptr = ::count--;
  }

  Foo(int i) : ptr(nullptr)
  {
    ptr = new int;
    *ptr = i;
  }

  Foo(const Foo & f) : ptr(nullptr)
  {
    ptr = new int;
    *ptr = *f.ptr;
  }

  Foo(Foo && f) : ptr(nullptr)
  {
    std::swap(ptr, f.ptr);
  }

  Foo & operator = (const Foo & f)
  {
    if (this == &f)
      return *this;

    // Handle null pointers safely
    if (f.ptr == nullptr)
      {
        delete ptr;
        ptr = nullptr;
      }
    else if (ptr == nullptr)
      {
        ptr = new int;
        *ptr = *f.ptr;
      }
    else
      {
        *ptr = *f.ptr;
      }

    return *this;
  }

  Foo & operator = (Foo && f)
  {
    swap(f);
    return *this;
  }

  ~Foo()
  {
    if (ptr != nullptr)
      {
	delete ptr;
	ptr = nullptr;
      }
  }

  // operator int () { return *ptr; }

  operator int () const
  {
    if (ptr == nullptr)
      throw std::logic_error("attempt to access value of moved-from Foo");
    return *ptr;
  }
};

template <typename T>
void print(const ArrayQueue<T> & q)
{
  cout << "capacity = " << q.capacity() << endl
       << "size     = " << q.size() << endl;

  for (size_t i = 0; i < q.size(); ++i)
    cout << (T) q.front(i) << " ";
  cout << endl ;

  for (size_t i = 0; i < q.size(); ++i)
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

int main(int argc, char * argv[])
{
  // Check argument count
  if (argc < 3)
    {
      cerr << "Usage: " << argv[0] << " <queue-size> <items-to-delete>" << endl;
      cerr << "Both arguments must be non-negative integers." << endl;
      return 1;
    }

  // Parse first argument (queue size)
  char *endptr1;
  errno = 0;
  long n_long = strtol(argv[1], &endptr1, 10);
  
  // Validate first argument
  if (errno != 0 or *endptr1 != '\0' or n_long < 0 or n_long > SIZE_MAX)
    {
      cerr << "Error: Invalid queue size argument. Must be a non-negative integer fitting in size_t." << endl;
      return 1;
    }
  
  size_t n = static_cast<size_t>(n_long);
  ArrayQueue<int> q(n);

  print(q);

  cout << "Inserting " << n << " values ";
  for (size_t i = 0; i < n; ++i)
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
  for (size_t i = 0; i < n; ++i)
    cout << q.put(i) << " ";
  cout << " done!" << endl << endl;

  print(q);

  // Parse second argument (items to delete)
  char *endptr2;
  errno = 0;
  long m_long = strtol(argv[2], &endptr2, 10);
  
  // Validate second argument
  if (errno != 0 or *endptr2 != '\0' or m_long < 0 or m_long > INT_MAX)
    {
      cerr << "Error: Invalid items-to-delete argument. Must be a non-negative integer fitting in int." << endl;
      return 1;
    }
  
  int m = static_cast<int>(m_long);

  cout << "Deleting " << m << " items" << endl;
  for (int i = 0; i < m; i++)
    {
      try
        {
          cout << q.get() << " ";
        }
      catch (std::underflow_error & exc)
        {
          cout << endl << exc.what() << endl;
          break;
        }
    }

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
