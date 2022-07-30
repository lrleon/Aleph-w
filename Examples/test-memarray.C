
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# include <tpl_memArray.H>

using namespace std;
using namespace Aleph;

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
void print(const MemArray<T> & s)
{
  cout << "capacity = " << s.capacity() << endl
       << "size     = " << s.size() << endl;

  for (int i = 0; i < s.size(); ++i)
    {
      int val = s[i];
      cout << val << " ";
    }
  cout << endl
       << endl;
}

template <typename T>
MemArray<T> create_memarray(int n)
{
  MemArray<T> ret(n);

  for (int i = 0; i < n; ++i)
    ret.put(T(i));

  return ret;
}

int main(int, char * argc[])
{
  MemArray<int> s;

  print(s);

  int n = atoi(argc[1]);

  for (int i = 0; i < n; ++i)
    s.put(i);

  print(s);

  int m = n/4;
  if (argc[2] != NULL)
    m = atoi(argc[2]);
  
  cout << "Extracting " << m << " items" << endl;
  for (int i = 0; i < m; ++i)
    cout << s.get() << " ";
  cout << endl;

  print(s);

  MemArray<int> c(s);

  print(c);

  print(create_memarray<int>(n));

  print(create_memarray<Foo>(n));
}
