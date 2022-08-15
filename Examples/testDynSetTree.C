
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

# include <time.h>
# include <gsl/gsl_rng.h>
# include <cassert>
# include <iostream>
# include <ahSearch.H>
# include <ahSort.H>
# include <tpl_sort_utils.H>
# include <tpl_dynMapTree.H>

# define NumItems 10000

using namespace std;

gsl_rng * r = NULL;

unsigned long insert_n_random_items_in_set(DynSetTree<unsigned long> & table,
					   DynArray<unsigned long> & keys,
					   unsigned long n)
{
  unsigned long dup_counter = 0;

  cout << "Testing simple insertions and searches ...." << endl;

  for (int i = 0; i < n; i++)
    {
      keys[i] = gsl_rng_get(r);
      if (not table.has(keys(i)))
	table.insert(keys(i));
      else
	++dup_counter;
    }

  return dup_counter;
}

DynSetTree<unsigned long> create_table(const DynSetTree<unsigned long> & other)
{
  DynSetTree<unsigned long> table;
  other.for_each([&table] (unsigned long item)
		 {
		   table.insert(item);
		 });

  return table;
}

void test_DynSet(size_t n)
{
  DynSetTreap<unsigned long> t = { 1, 2, 3, 4, 5};
  DynSetTree<unsigned long> table;
  DynArray<unsigned long> keys;
  unsigned int dup_counter = insert_n_random_items_in_set(table, keys, n);

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (int i = 0; i < n; i++)
    if (table.search(keys(i)) != NULL)
      {
        table.remove(keys(i));
        ++removed_counter;
      }

  assert(removed_counter == num_inserted);
  assert(table.size() == 0);

  cout << removed_counter << " items removed" << endl;

  cout << "testing empty() method ...." << endl;
  dup_counter = insert_n_random_items_in_set(table, keys, n);

  table.empty();

  assert(table.size() == 0);

  unsigned long repeated_counter = 0;
  cout << "Reinserting keys ...." << endl;
  for (int i = 0; i < n; ++i)
    if (table.insert(keys(i)) == NULL)
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previus value" << endl;

  assert(dup_counter == repeated_counter);

  cout << "Done!" << endl;

  {
    cout << "Testing iterator and map ...." << endl;

    DynList<unsigned long> l = table.template maps<unsigned long>
      (/* Lambda */ [] (const unsigned long & k) -> unsigned long
      {
        return k;
      });

    for (DynList<unsigned long>::Iterator it(l); it.has_curr(); it.next())
      assert(table.search(it.get_curr()) != NULL);

    cout << "done!" << endl;
  }

  {
    cout << "testing lvalue copy constructor ...." << endl;
    DynSetTree<unsigned long> tmp = table;

    assert(table.equal_to(tmp));
  }

  {
    cout << "testing lvalue assigment...." << endl;
    DynSetTree<unsigned long> aux;
    for (int i = 0; i < n/2; ++i)
      {
        unsigned long key = gsl_rng_get(r);
        while (aux.has(key))
          key = gsl_rng_get(r);
        aux.insert(key);
      }

    aux = table;

    assert(aux == table);
  }

  {
    cout << "Testing rvalue constructor ...." << endl;
    DynSetTree<unsigned long> tmp = create_table(table);
    assert(tmp == table);
    cout << "done!" << endl
         << endl
         << "Testing rvalue assign = .... " << endl
         << endl;
    tmp = create_table(table);
    cout << "done!" << endl
         << endl;
  }

  {
    DynArray<size_t> dups; // stores indexes of duplicated keys of keys array

    cout << "done" << endl
         << "Reinserting ...." << endl;
    for (int i = 0; i < n; ++i)
      if (table.insert(keys(i)) == NULL)
        dups.append(i);

    cout << "Searching inserted keys ...." << endl;
    for (size_t i = 0; i < n; ++i)
      {
        unsigned long * ptr = table.search(keys(i));
        assert(ptr != NULL);
        if (dups.size() > 0 and dups(binary_search(dups, i)) == i)
          continue; // keys[] contains a dup entry
      }
  }

  {
    cout << "Testing keys() in set ...." << endl
         << endl;
    DynList<unsigned long> the_keys = table.keys();
    assert(the_keys.size() == table.size());
    assert(all(the_keys, /* Lambda */ [&table] (const size_t & key)
               {
                 return table.has(key);
               }));
  }

  {
    cout << endl
	 << "Testing filter of keys multiples of 13" << endl;

    DynList<unsigned long> v13 =
      filter(table, [](const unsigned long & key)
             {
               return key % 13 == 0;
             });

    table.filter(/* Lambda */ [] (const unsigned long & key)
                 {
                   return key % 13 == 0;
                 }).for_each(/* Lambda */ [&v13] (const unsigned long & key)
                             {
                               cout << key << " ";
                               assert(contains(v13, key));
                             });
    cout << endl;
  }
}


unsigned long
insert_n_random_items_in_map(DynMapTree<unsigned long, long> & table,
                             DynArray<unsigned long> & keys,
                             unsigned long n)
{
  unsigned long dup_counter = 0;
  cout << "Testing simple insertions and searches ...." << endl;
  for (long i = 0; i < n; i++)
    {
      keys[i] = gsl_rng_get(r);
      if (not table.has(keys(i)))
        assert(table.insert(keys(i), i));
      else
        ++dup_counter;
    }
  return dup_counter;
}

void test_DynMap(size_t n)
{
  typedef DynMapTree<unsigned long, long> MapType;
  MapType table;
  DynArray<unsigned long> keys;
  unsigned int dup_counter = insert_n_random_items_in_map(table, keys, n);

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (int i = 0; i < n; i++)
    if (table.search(keys(i)) != NULL)
      {
        table.remove(keys(i));
        ++removed_counter;
      }

  assert(removed_counter == num_inserted);
  assert(table.size() == 0);

  cout << removed_counter << " items removed" << endl;

  cout << "testing empty() method ...." << endl;
  dup_counter = insert_n_random_items_in_map(table, keys, n);

  table.empty();

  assert(table.size() == 0);

  unsigned long repeated_counter = 0;
  cout << "Reinserting keys ...." << endl;
  for (int i = 0; i < n; ++i)
    if (table.insert(keys(i), i) == NULL)
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previus value" << endl;

  assert(dup_counter == repeated_counter);

  cout << "Done!" << endl
       << endl
       << "Testing for_each and a battery of other tests ...." << endl;

  assert(table.all([&table] (const std::pair<const unsigned long, long> & p)
                   {
                     auto * ptr = table.search(p.first);
                     assert(ptr != nullptr);
                     assert(table.get_data(p.first) == ptr->second);
                     return table.has(p.first);
                   })
         );

  cout << "done!" << endl
       << endl
       << "testing keys() method and other tests ...." << endl;
  DynList<unsigned long> the_keys = table.keys();
  assert(all(the_keys, /* Lambda */ [&table] (unsigned long k)
             { return table.has(k); }));

  cout << "done!" << endl
       << endl
       << "Testing items() method and othet stuff ...." << endl;
  DynList<std::pair<unsigned long, long>> items = table.items();
  assert(all(items, /* Lambda */ [&table]
             (std::pair<unsigned long, long> p)
             { return table.find(p.first) == p.second; } ));
  cout << "done!" << endl
       << endl;
}

int main(int argn, char *argc[])
{
  unsigned long n = argn > 1 ? atoi(argc[1]) : NumItems;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);

  cout << argc[0] << " " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  test_DynSet(n);

  test_DynMap(n);

  gsl_rng_free(r);
}
