
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

# include <gsl/gsl_rng.h>

# include <cassert>
# include <iostream>
# include <string>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_olhash.H>
# include <tpl_odhash.H>

# include <ctime>
using namespace std;

static unsigned long tbl_size = 0;

struct gsl_rng_holder 
{
  gsl_rng * r;
  gsl_rng_holder(unsigned int seed) 
  {
    r = gsl_rng_alloc(gsl_rng_mt19937);
    gsl_rng_set(r, seed % gsl_rng_max(r));
  }
  ~gsl_rng_holder() { gsl_rng_free(r); }
  operator gsl_rng*() { return r; }
};

constexpr int DEFAULT_N = 100;

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
void test_hash_table(size_t n, gsl_rng * r)
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
              if (table.search(keys(i)) == nullptr)
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

          assert(ptr != nullptr);
          assert(*ptr == keys(i));
        }
      cout << "done!" << endl
           << endl
           << "testing deletion ...." << endl;
      for (int i = 0; i < n; i += 2)
        {
          ptr = table.search(keys(i));
	  
          assert(ptr != nullptr);

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
              if (table.search(keys(i)) == nullptr)
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
          assert(ptr != nullptr);
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
      if (ptr == nullptr)
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
        assert(key_ptr != nullptr);
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

int main(int argc, char *argv[])
{ 
  int n = DEFAULT_N;
  unsigned int t = std::time(0);

  try 
    {
      if (argc > 1)
	n = std::stoi(argv[1]);

      if (argc > 2)
	t = std::stoi(argv[2]);
    }
  catch (...)
    {
      // ignore
    }

  if (n <= 0)
    {
      cout << "n must be positive" << endl;
      return 1;
    }

  cout << "testOhash " << n << " " << t << endl;

  gsl_rng_holder r(t);

  test_hash_table<ODhashTable, unsigned int>(n, r);

  //test_hash_table<OLhashTable, unsigned int>(n, r);
}
