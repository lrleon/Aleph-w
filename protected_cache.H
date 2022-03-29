

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

# ifndef TPL_PROTECTED_CACHE_H
# define TPL_PROTECTED_CACHE_H

# include <mutex>

# include <tpl_cache.H>

using namespace std;

    template <class Key, class Data>
class Protected_Cache : protected Cache<Key, Data>
{
private:

  mutex _mutex;

public:

  typedef typename Cache<Key, Data>::Cache_Entry Cache_Entry;

  Protected_Cache(size_t (*hash_fct)(const Key&), const size_t & size)
    : Cache<Key,Data>(hash_fct, size)
  {
    // empty
  }

  Cache_Entry * insert(const Key& key, const Data& data)
  {
    lock_guard<mutex> lock(_mutex);

    return Cache<Key,Data>::insert(key, data);
  }

  Cache_Entry * search(const Key & key)
  {
    lock_guard<mutex> lock(_mutex);

    return Cache<Key,Data>::search(key);
  }

  Cache_Entry * insert_and_lock(const Key& key, const Data& data)
  {
    lock_guard<mutex> lock(_mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::insert(key, data);
    this->lock_entry(cache_entry);

    return cache_entry;
  }

  Cache_Entry * search_and_lock(const Key & key)
  {
    lock_guard<mutex> lock(_mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::search(key);
    this->lock_entry(cache_entry);

    return cache_entry;
  }

  Cache_Entry * search_next_and_lock(Cache_Entry * cache_entry)
  {
    lock_guard<mutex> lock(_mutex);

    return Cache<Key,Data>::search_next(cache_entry);
  }

  void unlock_entry(Cache_Entry * cache_entry)
  {
    lock_guard<mutex> lock(_mutex);
    Cache<Key,Data>::unlock_entry(cache_entry);
  }

  void remove(Cache_Entry * cache_entry)
  {
    lock_guard<mutex> lock(_mutex);
    Cache<Key,Data>::remove(cache_entry);
  }

  void expand(const size_t & plus_size)
  {
    lock_guard<mutex> lock(_mutex);

    Cache<Key,Data>::expand(plus_size);
  }

  const size_t & get_size()
  {
    lock_guard<mutex> lock(_mutex);

    return Cache<Key,Data>::get_size();
  }

  const size_t & get_num_items()
  {
    lock_guard<mutex> lock(_mutex);

    return Cache<Key,Data>::get_num_items();
  }
};


# endif  // TPL_PROTECTED_CACHE_H


