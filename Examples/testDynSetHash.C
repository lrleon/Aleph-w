
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
# include <time.h>
# include <gsl/gsl_rng.h> 
# include <cassert>
# include <iostream>
# include <ahSearch.H>
# include <ahSort.H>
# include <tpl_sort_utils.H>
# include <tpl_dynSetHash.H>
# include <tpl_dynMapOhash.H>

# define NumItems 10000

using namespace std;

gsl_rng * r = NULL;

template <template <typename, class> class SetType> unsigned long 
insert_n_random_items_in_set
(SetType<unsigned long, Aleph::equal_to<unsigned long>> & table, 
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
      
      if (table.current_alpha() > 1.1)
        {
          cout << "Resizing table to " << 1.5*table.size() << endl;
          table.resize(1.5*table.size());
          cout << "done!" << endl;
        }
    }
  cout << "done" << endl;

  return dup_counter;
}

template <template <typename, class> class HashTable>
HashTable<unsigned long, Aleph::equal_to<unsigned long>>
create_table
(const HashTable<unsigned long, Aleph::equal_to<unsigned long>> & other)
{
  typedef HashTable<unsigned long, Aleph::equal_to<unsigned long>> SetType;
  SetType table;
  for (typename SetType::Iterator it(other); it.has_curr(); it.next())
    table.insert(it.get_curr());

  return table;
}

template <template <typename, class> class HashTable>
void test_DynSetLinHash(size_t n)
{
  typedef HashTable<unsigned long, Aleph::equal_to<unsigned long>> SetType;
  SetType table; 
  DynArray<unsigned long> keys;
  unsigned int dup_counter = insert_n_random_items_in_set(table, keys, n);

  typename SetType::Stats stats = table.stats();
  table.print_stats(stats); 
    
  cout << table.size() << " items inserted" << endl
       << dup_counter << " duplicated numbers" << endl
       << endl
       << "testing deletions ...." << endl;

  {
    const auto ctable = table;
    assert(table.all([&ctable] (auto k) { return ctable.find(k) == k; }));
  }

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (size_t i = 0; i < n; i++)
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
  for (size_t i = 0; i < n; ++i)
    if (table.insert(keys(i)) == NULL) 
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previus value" << endl;

  assert(dup_counter == repeated_counter);
  
  cout << "Done!" << endl;

  {
    cout << "Testing iterator and map ...." << endl;  

    DynList<unsigned long> l = table.template maps<unsigned long>
      ([] (unsigned long k) -> unsigned long
       {
         return k;
       });

    for (DynList<unsigned long>::Iterator it(l); it.has_curr(); it.next())
      assert(table.search(it.get_curr()) != NULL);

    cout << "done!" << endl;
  }

  {
    cout << "testing lvalue copy constructor ...." << endl;
    SetType tmp = table;

    assert(table.equal_to(tmp));
  }

  {
    cout << "testing lvalue assigment...." << endl;
    SetType aux;
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
    SetType tmp = create_table(table);
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
    cout << "testing del() of Iterator ...." << endl
         << "Deleting all entries through del() ...." << endl;

    for (typename SetType::Iterator it(table); it.has_curr(); /* empty */)
      it.del();

    assert(table.is_empty());

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
      filter(table, [] (const unsigned long & key)
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


template <class HashTable>  
unsigned long insert_n_random_items_in_map(HashTable & table, 
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
  
  cout << n << " tries " << endl
       << dup_counter << " duplicated" << endl
       << "size = " << table.size() << endl
       << endl
       << "Performing map search test" << endl
       << endl;
  
  cout << "keys =";
  sort(keys).for_each([] (auto k) { cout << " " << k; });
  cout << endl
       << "table = ";
  sort(table.keys()).for_each([] (auto k) { cout << " " << k; });
  cout << endl;

  for (auto i = 0; i < n; i++)
    assert(table.search(keys(i)));

  assert(table.search(keys(1)));

  assert(keys.all([&table] (auto k) { return table.search(k) != nullptr; }));
  cout << "Passed" << endl
       << endl;

  return dup_counter;
}

template <template <typename, typename, class> class HashTable>
void test_DynMapLinHash(size_t n)
{
  using MapType = DynMapHash<unsigned long, long>;
  MapType table; 
  DynArray<unsigned long> keys;
  unsigned int dup_counter = insert_n_random_items_in_map(table, keys, n);

  typename MapType::Stats stats = table.stats();
  table.print_stats(stats); 

  cout << table.size() << " items inserted" << endl
       << dup_counter << " duplicated numbers" << endl
       << endl
       << "testing deletions ...." << endl;

  unsigned long removed_counter = 0;
  size_t num_inserted = table.size();
  for (int i = 0; i < n; i++)
    if (table.search(keys(i)) != nullptr)
      { 
        table.remove(keys(i));
        ++removed_counter;
      }
  cout << removed_counter << " items removed" << endl;

  assert(removed_counter == num_inserted);
  assert(table.size() == 0);
  //keys.cut();

  cout << "testing empty() method ...." << endl;
  dup_counter = insert_n_random_items_in_map(table, keys, n);

  table.empty();

  assert(table.size() == 0);

  unsigned long repeated_counter = 0;
  cout << "Reinserting keys ...." << endl;
  for (size_t i = 0; i < n; ++i)
    if (table.insert(keys(i), i) == NULL) 
      ++repeated_counter;

  cout << repeated_counter << " duplicated numbers" << endl
       << dup_counter << " was the previus value" << endl;

  assert(dup_counter == repeated_counter);
  
  cout << "Done!" << endl
       << endl
       << "Testing for_each and a battery of other tests ...." << endl;

  assert(table.all(/* Lambda */ [&table] 
                   (const std::pair<const unsigned long, long> & p)
                                {
                                  auto ptr = table.search(p.first);
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
    /*       << "Testing values() method ...." << endl;
             DynList<long> values = table.values();
             assert(all(values, [&table] (long & val)
             { return table.search(table.get_key(ptr)) == ptr; }));
             cout << "done!" << endl
             << endl */
       << "Testing items() method and othet stuff ...." << endl;
  DynList<std::pair<unsigned long, long>> items = table.items();
  assert(all(items, /* Lambda */ [&table] 
             (std::pair<unsigned long, long> p)
                                 { return table.find(p.first) == p.second; } ));
  cout << "done!" << endl
       << endl
       << "Testing remove by data pointer ...." << endl;
  removed_counter = 0;
  for_each(keys, [&table, &removed_counter] (const unsigned long & k)
                 {
                   auto ptr = table.search(k);
                   if (ptr == nullptr)
                     return;

                   table.remove_by_data(ptr->second);
                   ++removed_counter;
                 });
  assert(table.is_empty());

  cout << endl
       << "Reinserting keys for doing other tests ...." << endl;
  for (int i = 0; i < n; ++i)
    table.insert(keys(i), i);
  assert(table.size() == removed_counter);
  cout << "done!" << endl
       << endl;

}

int main(int argn, char *argc[])
{
  assert(Primes::check_primes_database());
  assert(next_prime(5) == 5);

  unsigned long n = argn > 1 ? atoi(argc[1]) : NumItems;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);

  cout << argc[0] << " " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  test_DynSetLinHash<DynSetLhash>(n);
  test_DynMapLinHash<DynMapHash>(n);

  test_DynSetLinHash<DynSetLinHash>(n);
  test_DynMapLinHash<DynMapLinHash>(n);

  cout << "testing of ODhash based set ...." << endl
       << endl;
  test_DynSetLinHash<SetODhash>(n);
  cout << endl
       << "Done all test of ODhash based set!" << endl
       << endl
       << endl
       << "testing of OLhash based set ...." << endl
       << endl;
  test_DynSetLinHash<SetOLhash>(n);
  cout << endl
       << "Done all test of OLhash based set!" << endl
       << endl
       << endl
       << "Testing all tests of OLhash based map" << endl
       << endl;
  // test_DynMapLinHash<DynMapODHash>(n);
  // cout << "Done all tests of OD hash based map" << endl
  //      << endl
  //      << endl
  //      << "Testing of OLhash based map" << endl
  //      << endl;
  // test_DynMapLinHash<DynMapOLHash>(n);
  // cout << "Done all tests of OL hash based map" << endl
  //      << endl
  //      << endl;
  gsl_rng_free(r);
}
