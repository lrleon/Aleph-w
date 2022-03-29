
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

# include <tpl_dynLhash.H>
# include <tpl_dynArray.H>
# include <iostream>
# include <time.h>
# include <stdlib.h>

# define NumItems 10000

using namespace std;

typedef DynLhashTable<unsigned, unsigned> HTable;

static size_t hashFct(const unsigned & key)
{
  return key;
}

static void testResize(HTable& table)
{
  if (table.get_num_busy_slots() > (99*table.capacity())/100 
      && table.size()/table.capacity() > 3)
    {
      unsigned long currSize = table.capacity();
      cout << "Resizing hash table from " << currSize << " ... ";
      cout << table.resize(1.5 * table.capacity()) << endl;
    }
}

static void printPars(const HTable& table)
{
  cout << "Table length = " << table.capacity() << endl
       << "Busy slots   = " << table.get_num_busy_slots() << endl
       << "Num items    = " << table.size() << endl;
}

int main(int argn, char *argc[])
{
  Primes::check_primes_database();

  unsigned long n = argn > 1 ? atoi(argc[1]) : NumItems;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);
  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  HTable table(1.15*n, hashFct);
  DynArray<unsigned int> keys(n);
  unsigned int i;
  unsigned int foundCounter = 0;

  for (i = 0; i < n/2; i++)
    {
      keys[i] = rand();
      testResize(table);
      if (table.search(keys(i)) == NULL)
	  assert(table.insert(keys[i], i) != NULL);
      else
	foundCounter++;
    }

  cout << foundCounter << " duplicated numbers" << endl;

  assert( table.size() + foundCounter == n/2);
  printPars(table);

  for (i = n/2; i < n; i++)
    {
      keys[i] = rand();
      testResize(table);
      table[keys[i]] = i;
      table[keys[i]] = table[keys[i]];
    }
    
  printPars(table);
    
  unsigned * ptr;
  foundCounter = 0;
  for (i = 0; i < n; i++)
    {
      ptr = table.search(keys[i]);
      if (ptr != NULL)
	table.remove(ptr);
      else
	foundCounter++;
    }
}
