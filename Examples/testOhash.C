
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

# include <gsl/gsl_rng.h>

# include <cassert>
# include <iostream>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_olhash.H>
# include <tpl_odhash.H>

using namespace std;

static unsigned long tbl_size = 0;

gsl_rng * r = NULL;

# define DEFAULT_N 100

template <template <typename,class> class HashTable, typename Key,
          class Cmp = Aleph::equal_to<Key>> 
HashTable<Key, Cmp> create_table(const HashTable<Key, Cmp> & s)
{
  typedef HashTable<Key, Cmp> Hset;
  Hset ret_val;
  for (typename Hset::Iterator it(s); it.has_curr(); it.next())
    ret_val.insert(it.get_curr());
  
  return ret_val;
}

template <template <typename, class> class HashTable, typename Key,
          class Cmp = Aleph::equal_to<Key>>
void test_hash_table(size_t n)
{
  typedef HashTable<Key, Cmp> Hset;
  DynArray<Key> keys(n);
  
  Hset table;

  tbl_size = table.size();

  for (int k = 0; k < 4; k++)
    {  
      cout << "k = " << k << endl
           << "testing insertions and initial searches" << endl;

      for (int i = 0; i < n; i++)
        {
          while (true)
            {
              keys[i] = gsl_rng_get(r);
              if (table.search(keys(i)) == NULL)
                break;
            }
	  
          table.insert(keys(i));
        }

      cout << "done" << endl
           << endl;

      table.print_stats(table.stats());

      cout << endl
           << "testing searches or previous inserted keys" << endl;
      Key * ptr;
      for (int i = 0; i < n; i++)
        {
          const Key k = keys(i);

          ptr = table.search(k);

          assert(ptr != NULL);
          assert(*ptr == keys(i));
        }
      cout << "done!" << endl
           << endl
           << "testing deletion ...." << endl;
      for (int i = 0; i < n; i += 2)
        {
          ptr = table.search(keys(i));
	  
          assert(ptr != NULL);

          table.remove_ptr(ptr);
        }
      cout << "done!" << endl
           << endl
           << "Reinserting others keys ...." << endl;
      for (int i = 0; i < n; i += 2)
        {
          while (true)
            {
              keys[i] = gsl_rng_get(r);
              if (table.search(keys(i)) == NULL)
                break;
            }
          table.insert(keys(i));
        }
      cout << "done!" << endl
           << endl
           << "Removing all the keys ...." << endl;
      for (int i = 0; i < n; i++)
        {
          const Key & key = keys(i);
          ptr = table.search(key);
          assert(ptr != NULL);
          table.remove_ptr(ptr);
        }

      assert(table.size() == 0);
      cout << "done! k = " << k << endl
           << endl;
    }

  cout << "Sorting keys backup ...." << endl;
  quicksort_op(keys);

  cout << "done!" << endl
       << endl 
       << "Testing iterator ...." << endl
       << endl
       << "Reinserting the keys ...."
       << endl;
  for (int i = 0; i < n; ++i)
    table.insert(keys(i));

  int count = 0;
  for (typename Hset::Iterator it(table);
       it.has_curr(); it.next(), ++count)
    {
      const Key & curr = it.get_curr();
      int idx = binary_search(keys, curr);
      assert(idx >= 0);
      assert(curr == keys(idx));
    }
  assert(count == table.size());
  cout << "done!" << endl
       << endl
       << "Testing backward iterator ...." << endl;
  {
    typename Hset::Iterator it(table);
    count = 0;
    it.reset_last();
    for (int i = 0; it.has_curr(); it.prev(), ++i, ++count)
      {
        const Key & curr = it.get_curr();
        int idx = binary_search(keys, curr);
        assert(idx >= 0);
        assert(curr == keys(idx));
      }
    assert(count == table.size());
    cout << "done!" << endl
         << endl; 
  }
  cout << "done!" << endl
       << endl
       << "Testing del() of iterator ...." << endl
       << "Deleting all the keys via del() of iterator"
       << endl;
  count = 0;
  for (typename Hset::Iterator it(table); it.has_curr(); ++count)
    it.del();
  
  cout << "done!. Deleted " << count << " entries " << endl
       << endl;

  assert(count == n);
  assert(table.is_empty());

  cout << "Inserting again all keys ...." << endl
       << endl;
  for (int i = 0; i < n; ++i)
    table.insert(keys(i));

  cout << "done!" << endl
       << endl
       << "Deleting a 10% of keys for causing deleted entries ...." << endl
       << endl;
  for (int i = 0; i < n/10; ++i)
    {
      unsigned long idx = gsl_rng_uniform_int(r, keys.size());
      const Key & key = keys(idx);
      
      Key * ptr = table.search(key);
      if (ptr == NULL)
        continue;

      table.remove_ptr(ptr);
    }

  table.print_stats(table.stats());

  {
    cout << "Testing copy constructor" << endl;
    Hset aux = table;
    assert(aux.size() == table.size());
    for (typename Hset::Iterator it(table); it.has_curr(); it.next())
      {
        Key * key_ptr = aux.search(it.get_curr());
        assert(key_ptr != NULL);
        assert(*key_ptr == it.get_curr());
      }
    cout << "done!" << endl;
  }

  {
    cout << "Testing rvalue copy constructor ...." << endl;
    Hset aux = create_table(table);

    assert(aux == table);

    aux = create_table(table);

    assert(aux == table);

    cout << "done!" << endl
         << endl;
  }
}

int main(int argn, char *argc[])
{ 
  int n = argc[1] ? atoi(argc[1]) : DEFAULT_N;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argc[2]);

  cout << "testOhash " << n << " " << t << endl;

  r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, t % gsl_rng_max(r));

  test_hash_table<ODhashTable, unsigned int>(n);

  //test_hash_table<OLhashTable, unsigned int>(n);
  
  gsl_rng_free(r);
}
