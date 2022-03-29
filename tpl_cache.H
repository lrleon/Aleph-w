

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

  Cache_Entry * search_next(Cache_Entry * cache_entry)
  {
    Cache_Entry *next_entry =
      static_cast<Cache_Entry*>(hash_table.search_next(cache_entry));

    if (next_entry != nullptr)
      {
        do_mru(next_entry);
        move_to_inside_front(cache_entry);
      }

    return next_entry;
  }

  Cache_Entry * insert(const Key& key, const Data& data)
  {
    assert(hash_table.size() <= cache_size);

    return insert_pair(key, data);
  }

  void lock_entry(Cache_Entry * cache_entry)
  {
    assert(num_locked < get_num_entries());
    assert(num_lru > 0);
    assert(hash_table.search(cache_entry->get_key()) == cache_entry);
    assert(cache_entry->is_in_hash_table);

    if (cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is already locked");

    remove_entry_from_lru_list(cache_entry);
    insert_entry_to_locked_list(cache_entry);

    cache_entry->lock();
  }

  void unlock_entry(Cache_Entry * cache_entry)
  {
    assert(hash_table.search(cache_entry->get_key()) == cache_entry);
    assert(cache_entry->is_in_hash_table);
    assert(num_locked <= get_num_entries());

    if (not cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is not locked");

    remove_entry_from_locked_list(cache_entry);
    insert_entry_to_lru_list(cache_entry);

    cache_entry->unlock();
  }

  void remove(Cache_Entry * cache_entry)
  {
    assert(hash_table.search(cache_entry->get_key()) == cache_entry);

    if (cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is already locked");

    remove_entry_from_hash_table(cache_entry);
  }

  void expand(const size_t & plus_size)
  {
    assert(hash_table.size() <= cache_size);

    if (plus_size == 0)
      throw std::range_error ("bad plus_size");

    const size_t new_cache_size = cache_size + plus_size;

    Cache_Entry * entries_array = new Cache_Entry [plus_size];

    try
      {
        std::unique_ptr<Chunk_Descriptor>
          chunk_descriptor (new Chunk_Descriptor (entries_array));

        hash_table.resize(13*(new_cache_size)/10);

        for (int i = 0; i < plus_size; i++)
          insert_entry_to_lru_list(&entries_array[i]);

        chunk_list.insert(chunk_descriptor.release());

        cache_size = new_cache_size;
      }
    catch (...)
      {
        delete []  entries_array;
        throw;
      }
  }

  const size_t & capacity() const { return cache_size; }

  const size_t & size() const { return hash_table.size(); }

  const size_t & get_num_locked() const { return num_locked; }

  const size_t & get_num_busy_slots() const
  {
    return hash_table.get_num_busy_slots();
  }

  struct Iterator : public Dlink::Iterator
  {
    Iterator(Cache& _cache) : Dlink::Iterator(&_cache.inside_list)
    {
      // empty
    }

    Cache_Entry * get_curr()
    {
      Cache_Entry * ret_val =
        Cache_Entry::dlink_inside_to_Cache_Entry(Dlink::Iterator::get_curr());

      assert(ret_val->is_in_table());

      return ret_val;
    }
  };
  };

} // end namespace Aleph

# endif // TPL_CACHE_H
