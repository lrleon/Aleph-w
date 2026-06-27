
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

# include <ctime>

# include <gsl/gsl_rng.h>

# include <tclap/CmdLine.h>

# include <iostream>
# include <aleph.H>
# include <tpl_dynSetHash.H>

# include <cassert>
using namespace std;
# include <cassert>
using namespace TCLAP;

CmdLine cmd = { "testHash", ' ', "0" };

ValueArg<size_t> num_keys =
  { "n", "num", "num of keys", false, 10, "num of keys", cmd };

ValueArg<size_t> seed = { "s", "seed", "seed", false, (size_t) time(nullptr),
			  "seed for random", cmd };

void test(size_t n, unsigned long seed)
{
  gsl_rng * r = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));

  DynMapHash<size_t, string> tbl;
  DynArray<pair<size_t, string>> a;

  for (size_t i = 0; i < n; ++i)
    {
      unsigned long val = gsl_rng_get(r);
      const string sval = to_string(val);
      a.append(make_pair(i, sval));
      tbl.insert(i, sval);
    }

  tbl.for_each([] (auto p) { cout << p.first << ", " << p.second << endl; });

  for (auto it = tbl.get_it(); it.has_curr(); it.next())
    {
      auto p = it.get_curr();
      auto key = p.first;
      auto ptr = tbl.search(key);
      cout << key << " == " << ptr->first << " " << p.second << " == "
	   << ptr->second << endl;
      assert(key == ptr->first);
      assert(p.second == ptr->second);
    }

  for (size_t i = 0; i < n; ++i)
    {
      auto p1 = a(i);
      auto ptr = tbl.search(p1.first);
      assert(p1.first == ptr->first);
      assert(p1.second == ptr->second);
    }

  gsl_rng_free(r);
}

int main(int argc, char *argv[])
{
  cmd.parse(argc, argv);
  test(num_keys.getValue(), seed.getValue());
}

