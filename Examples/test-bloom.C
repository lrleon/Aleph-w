
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

# include <tclap/CmdLine.h>

# include <bloom-filter.H>

using namespace TCLAP;

CmdLine cmd = { "test-bloom", ' ', "0.0" };

ValueArg<size_t> n = { "n", "n", "number of items", true, 1000,
		       "number of items", cmd };
ValueArg<double> p = { "p", "prob", "probability of failures", true, .01,
		       "probability of failures", cmd };
ValueArg<unsigned long> seed = { "s", "seed", "seed", true, 0, "seed", cmd };

template <typename T>
ostream & operator << (ostream & s, const DynList<T> & l)
{
  if (l.is_empty())
    return s << "EMPTY";

  l.for_each([&s] (const T & i) { s << i << " "; });
  return s;
}

int main(int argc, char *argv[])
{
  cmd.parse(argc, argv);

  if (p.getValue() <= 0 or p.getValue() >= 1)
    {
      cout << "probability must be inside (0, 1)" << endl;
      abort();
    }

  Bloom_Filter<long> f(n.getValue(), p.getValue(), seed.getValue());
  Bloom_Filter<long> ff(f);

  ff.swap(f);

  auto t = f.estimate(n.getValue(), p.getValue());
  auto m = get<0>(t);
  cout << "seeds = " << f.hash_seeds() << endl
       << "hashes(10) = " << f.hashes(10) << endl
       << "hashes(9)  = " << f.hashes(9) << endl
       << "common(10, 9) = " << f.common_hashes(10, 9) << endl
       << "m = " << m << " (" << m/8 + m% 8 << " bytes)" << endl
       << "k = " << f.get_k() << endl
       << "sizeof(size_t) = " << sizeof(size_t) << endl
       << endl
       << endl
       << "Inserting " << n.getValue() << " items sequentially" << endl
       << endl;

  for (auto i = 0; i < n.getValue(); ++i)
    f.append(i);

  auto x = f.get_x();
  cout << "Done!" << endl
       << "Bits set to 1 = " << x << " of " << m << " ("
       << 100.0*x/m << " %)" << endl    
       << endl
       << "Generating random searches" << endl;
  
  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed.getValue() % gsl_rng_max(r));
  size_t false_positive_count = 0;
  size_t failed_search_count = 0;
  for (auto i = 0; i < 10*n.getValue(); ++i)
    {
      long val = gsl_rng_get(r);
      if (val >= 0 and val < n.getValue())
	{
	  if (not f.contains(val))
	    cout << "ERROR: " << val << " was not found" << endl;
	}
      else
	{
	  ++failed_search_count;
	  if (f.contains(val))
	    ++false_positive_count;
	}
    }

  cout << "done!" << endl
       << "Total searches  = " << 10*n.getValue() << endl
       << "Failed searches = " << failed_search_count << endl
       << "False positives = " << false_positive_count << endl
       << "Error = " << 100.0*false_positive_count/failed_search_count 
       << " % " << endl
       << endl;

  gsl_rng_free(r);
}
