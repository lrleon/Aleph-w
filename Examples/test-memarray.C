
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
