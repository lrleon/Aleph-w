
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
# include <aleph.H>

# include "protected_cache.H"

using namespace std;

static size_t hashFct(const unsigned & i)
{
  return i;
}

int main(int argn, char *argc[])
{
  int n = 16;
  unsigned int t = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testCache " << n << " " << t << endl;

  Protected_Cache<unsigned, int> cache(&hashFct, n);

  int i;
  for (i = 0; i < n; i++)
    cache.insert(i, i);

  try
    {
      for (i = 0; i < n; i++)
	cache.insert(i, i);
    }
  catch (exception & e)
    {
      cout << e.what() << endl;
    }

  Protected_Cache<unsigned, int>::Cache_Entry * cache_entry;

  try
    {
      n--;
      cache_entry = cache.search(n);
      cache.remove(cache_entry);
      n++;

      for (i = 0; i < n - 1; i++)
	{
	  cache.insert(i, n);
	  cache_entry = cache.search_and_lock(i);
	  while (cache_entry not_eq NULL)
	    {
	      cout << "(" << cache_entry->get_key() << ","
		   << cache_entry->get_data() << ")" << endl;
	      cache_entry = cache.search_next_and_lock(cache_entry);
	    }
	}
    }
  catch (exception & e)
    {
      cout << e.what() << endl << endl;
      exit(1);
    }

  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);
      if (cache_entry not_eq NULL)
	cache.remove(cache_entry);
    }


  cache.expand(n/2);


}
