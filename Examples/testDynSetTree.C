
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

# include <ctime>
# include <cerrno>
# include <climits>
# include <gsl/gsl_rng.h>
# include <cassert>
# include <iostream>
# include <ahFunctional.H>
# include <ahSort.H>
# include <tpl_sort_utils.H>
# include <tpl_dynMapTree.H>

# define NumItems 10000

using namespace std;

gsl_rng * r = nullptr;

unsigned long insert_n_random_items_in_set(DynSetTree<unsigned long> & table,
					   DynArray<unsigned long> & keys,
					   unsigned long n)
{
  unsigned long dup_counter = 0;

  cout << "Testing simple insertions and searches ...." << endl;

  for (unsigned long i = 0; i < n; i++)
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
  unsigned long dup_counter = insert_n_random_items_in_set(table, keys, n);

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (size_t i = 0; i < n; i++)
    if (table.search(keys(i)) != nullptr)
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
  for (size_t i = 0; i < n; ++i)
    if (table.insert(keys(i)) == nullptr)
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previous value" << endl;

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
      assert(table.search(it.get_curr()) != nullptr);

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
    for (size_t i = 0; i < n/2; ++i)
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
    
    // Clear table to ensure reinsertion tests against an empty set
    table.empty();
    
    for (size_t i = 0; i < n; ++i)
      if (table.insert(keys(i)) == nullptr)
        dups.append(i);

    cout << "Searching inserted keys ...." << endl;
    for (size_t i = 0; i < n; ++i)
      {
        unsigned long * ptr = table.search(keys(i));
        assert(ptr != nullptr);
        if (dups.size() > 0)
          {
            const auto dup_idx = binary_search(dups, i);
            if (dup_idx < dups.size() and dups(dup_idx) == i)
              continue; // keys[] contains a dup entry
          }
      }
  }

  {
    cout << "Testing keys() in set ...." << endl
         << endl;
    DynList<unsigned long> the_keys = table.keys();
    assert(the_keys.size() == table.size());
    assert(all(the_keys, /* Lambda */ [&table] (const unsigned long & key)
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
  for (unsigned long i = 0; i < n; i++)
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
  unsigned long dup_counter = insert_n_random_items_in_map(table, keys, n);

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (size_t i = 0; i < n; i++)
    if (table.search(keys(i)) != nullptr)
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
  for (size_t i = 0; i < n; ++i)
    if (table.insert(keys(i), i) == nullptr)
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previous value" << endl;

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
       << "Testing items() method and other stuff ...." << endl;
  DynList<std::pair<unsigned long, long>> items = table.items();
  assert(all(items, /* Lambda */ [&table]
             (std::pair<unsigned long, long> p)
             { return table.find(p.first) == p.second; } ));
  cout << "done!" << endl
       << endl;
}

int main(int argc, char *argv[])
{
  unsigned long n = NumItems;
  if (argc > 1)
    {
      if (argv[1][0] == '-')
        {
          cerr << "Invalid n: " << argv[1] << endl;
          return 1;
        }
      char * endptr = nullptr;
      errno = 0;
      n = strtoul(argv[1], &endptr, 10);
      if (errno != 0 or endptr == argv[1] or *endptr != '\0')
        {
          cerr << "Invalid n: " << argv[1] << endl;
          return 1;
        }
    }

  unsigned int t = (unsigned int) time(nullptr);
  if (argc > 2)
    {
      if (argv[2][0] == '-')
        {
          cerr << "Invalid t: " << argv[2] << endl;
          return 1;
        }
      char * endptr = nullptr;
      errno = 0;
      const unsigned long parsed_t = strtoul(argv[2], &endptr, 10);
      if (errno != 0 or endptr == argv[2] or *endptr != '\0' or parsed_t > UINT_MAX)
        {
          cerr << "Invalid t: " << argv[2] << endl;
          return 1;
        }
      t = static_cast<unsigned int>(parsed_t);
    }

  cout << argv[0] << " " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  test_DynSet(n);

  test_DynMap(n);

  gsl_rng_free(r);
}
