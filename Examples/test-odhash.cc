
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

# include <memory>
# include <iostream>
# include "tpl_odhash.H"

using namespace std;

struct Foo
{
  string a = "hello world";
  Foo() {}
  Foo(const string & str) : a(str) {}
};

struct FooPairEqual
{
  bool operator () (const pair<pair<const Foo*, const Foo*>, string> & p1,
		    const pair<pair<const Foo*, const Foo*>, string> & p2) const
  {
    return p1.first == p2.first;
  }
};

using FooMap = ODhashTable<pair<pair<const Foo*, const Foo*>, string>,
			   FooPairEqual>;

static size_t fst_unit_pair_hash
(const pair<pair<const Foo*, const Foo*>, string> & p)
{
  return pair_dft_hash_fct(p.first);
}

static size_t snd_unit_pair_hash
(const pair<pair<const Foo*, const Foo*>, string> & p)
{
  return pair_snd_hash_fct(p.first);
}

FooMap m(5, fst_unit_pair_hash, snd_unit_pair_hash);

int main()
{
  DynArray<shared_ptr<Foo>> foos;
  for (size_t i = 0; i < 1000; ++i)
    foos.append(shared_ptr<Foo>(new Foo(to_string(i))));

  shared_ptr<Foo> f1(new Foo("Hola"));
  shared_ptr<Foo> f2(new Foo("Mundo"));

  static_cast<void>(m.insert(make_pair(make_pair(f1.get(), f2.get()),
                                       f1->a + " " + f2->a)));

  for (auto it = foos.get_it(); it.has_curr(); it.next())
    {
      auto f1 = it.get_curr().get();
      for (auto it = foos.get_it(); it.has_curr(); it.next())
        static_cast<void>(m.insert(make_pair(make_pair(f1, it.get_curr().get()),
                                             f1->a + " " + it.get_curr()->a)));
    }

  for (auto it = foos.get_it(); it.has_curr(); it.next())
    {
      auto f1 = it.get_curr().get();
      for (auto it = foos.get_it(); it.has_curr(); it.next())
	m.remove(make_pair(make_pair(f1, it.get_curr().get()), ""));
    }

  for (auto it = m.get_it(); it.has_curr(); it.next())
    {
      auto p = it.get_curr();
      cout << p.second << endl;
    }
}
