
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
# include <tpl_sort_utils.H>
# include <tpl_random_queue.H>

struct Foo
{
  unsigned long p;
  unsigned long s;

  friend ostream & operator << (ostream & out, const Foo & f)
  {
    return out << "(" << f.p << "," << f.s << ")";
  }
};

DynArray<Foo> create(size_t n)
{
  assert(n % 10 == 0);
  DynArray<Foo> ret; ret.reserve(n);

  for (long i = 0; i < n/10; ++i)
    {
      Foo f; 
      f.s = i;
      for (long j = 0; j < 10; ++j)
	{
	  f.p = 10*i + j;
	  ret(f.p) = f;
	}
    }

  assert(is_sorted(ret, [] (auto r1, auto r2) { return r1.p < r2.p; })); 
  assert(is_sorted(ret, [] (auto r1, auto r2) { return r1.s < r2.s; }));

  {
    auto u = shuffle(ret);
    auto index = build_index(u, [] (auto p1, auto p2) { return p1.s < p2.s; });
    assert(is_sorted(index, [&u] (auto i1, auto i2) 
		     { return u(i1).s < u(i2).s; }));
  }

  DynArray<size_t> index; index.reserve(n);
  for (long i = 0; i < index.size(); ++i)
    index(i) = i;
      
  for (long i = 0; i < n/10; ++i)
    {
      Foo f; f.s = i;
      {
	DynList<Foo*> dups = 
	  bsearch_dup(ret, f, [] (auto r1, auto r2) { return r1.s < r2.s; });
	assert(dups.all([i] (auto ptr) { return ptr->s == i; }));
      }
    }

  return ret;
}


void usage()
{
  cout << "test-bsearch n" << endl;
  exit(0);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
    usage();

  const size_t n = atoi(argv[1]);
  
  auto a = create(n);

  cout << "Every thing is ok" << endl;
}

