
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
# include <memory>
# include <concepts>
# include <stdexcept>
# include <ah-errors.H>
# include <tpl_memArray.H>

using namespace std;
using namespace Aleph;

int g_count = -1;

struct Foo
{
  std::unique_ptr<int> ptr;

  Foo() : ptr(std::make_unique<int>(::g_count--)) {}

  Foo(int i) : ptr(std::make_unique<int>(i)) {}

  Foo(const Foo & f) : ptr(f.ptr ? std::make_unique<int>(*f.ptr) : nullptr) {}

  Foo(Foo && f) noexcept = default;

  Foo & operator = (const Foo & f)
  {
    if (this != &f)
      ptr = f.ptr ? std::make_unique<int>(*f.ptr) : nullptr;
    return *this;
  }

  Foo & operator = (Foo && f) noexcept = default;

  ~Foo() = default;

  operator int () const 
  { 
    ah_domain_error_if(not ptr) << "Attempt to access null Foo";
    return *ptr; 
  }
};

template <typename T>
  requires std::convertible_to<T, int>
void print(const MemArray<T> & s)
{
  cout << "capacity = " << s.capacity() << endl
       << "size     = " << s.size() << endl;

  for (size_t i = 0; i < s.size(); ++i)
    {
      int val = s[i];
      cout << val << " ";
    }
  cout << endl
       << endl;
}

template <typename T>
  requires std::constructible_from<T, int>
MemArray<T> create_memarray(int n)
{
  MemArray<T> ret(n);

  for (int i = 0; i < n; ++i)
    ret.put(T(i));

  return ret;
}

int main(int argc, char * argv[])
{
  MemArray<int> s;

  print(s);

  int n = 1000;
  if (argc > 1)
    {
      try { n = stoi(argv[1]); }
      catch (const std::invalid_argument &) { n = 1000; }
      catch (const std::out_of_range &) { n = 1000; }
      ah_invalid_argument_if(n <= 0) << "n must be > 0";
    }

  for (int i = 0; i < n; ++i)
    s.put(i);

  print(s);

  int m = n / 4;
  if (argc > 2)
    {
      try { m = stoi(argv[2]); }
      catch (const std::invalid_argument &) { m = n / 4; }
      catch (const std::out_of_range &) { m = n / 4; }
      ah_invalid_argument_if(m < 0 or m > n) << "m must be between 0 and n";
    }
  
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
