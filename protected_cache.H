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

/** @file protected_cache.H
 *  @brief Thread-safe cache wrapper using mutex protection.
 *
 *  This file provides Protected_Cache, a thread-safe wrapper around
 *  the Cache class that uses mutex locks for concurrent access.
 *
 *  ## Features
 *
 *  - Thread-safe insert, search, and remove operations
 *  - RAII lock management via std::lock_guard
 *  - Atomic "lock entry" operations for exclusive access
 *  - Same interface as underlying Cache
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Protected_Cache<string, Data> cache(hash_fct, 1000);
 *
 *  // Thread 1
 *  auto entry = cache.insert("key1", data1);
 *
 *  // Thread 2 (concurrent)
 *  auto found = cache.search("key1");  // Safe
 *  ```
 *
 *  @see tpl_cache.H Underlying cache implementation
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_PROTECTED_CACHE_H
# define TPL_PROTECTED_CACHE_H

# include <mutex>

# include <tpl_cache.H>

/**
 * @brief Thread-safe cache with mutex protection.
 *
 * Wraps Cache<Key, Data> with a mutex to enable safe concurrent
 * access from multiple threads. All public operations acquire the
 * mutex before accessing the underlying cache.
 *
 * @tparam Key Type of cache keys
 * @tparam Data Type of cached data
 *
 * ## Thread Safety
 *
 * All methods are thread-safe. The mutex is held for the duration
 * of each operation.
 *
 * @note For fine-grained locking, consider using individual entry
 *       locks via insert_and_lock() / unlock_entry().
 *
 * @ingroup Hash
 */
    template <class Key, class Data>
class Protected_Cache : protected Cache<Key, Data>
{
private:

  std::mutex _mutex;  ///< Mutex for thread synchronization

public:

  /// Cache entry type from base class
  typedef typename Cache<Key, Data>::Cache_Entry Cache_Entry;

  /**
   * @brief Construct a thread-safe cache.
   *
   * @param hash_fct Hash function for keys
   * @param size Maximum number of cache entries
   */
  Protected_Cache(size_t (*hash_fct)(const Key&), const size_t & size)
    : Cache<Key,Data>(hash_fct, size)
  {
    // empty
  }

  /**
   * @brief Insert a key-data pair into the cache.
   *
   * Thread-safe insertion. If key exists, updates the data.
   *
   * @param key Key to insert
   * @param data Data associated with key
   * @return Pointer to inserted cache entry
   */
  Cache_Entry * insert(const Key& key, const Data& data)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    return Cache<Key,Data>::insert(key, data);
  }

  /**
   * @brief Search for a key in the cache.
   *
   * Thread-safe lookup.
   *
   * @param key Key to search for
   * @return Pointer to cache entry if found, nullptr otherwise
   */
  Cache_Entry * search(const Key & key)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    return Cache<Key,Data>::search(key);
  }

  /**
   * @brief Insert and lock an entry for exclusive access.
   *
   * The entry remains locked until unlock_entry() is called.
   * Useful for atomic read-modify-write operations.
   *
   * @param key Key to insert
   * @param data Data associated with key
   * @return Pointer to locked cache entry
   */
  Cache_Entry * insert_and_lock(const Key& key, const Data& data)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::insert(key, data);
    this->lock_entry(cache_entry);

    return cache_entry;
  }

  /**
   * @brief Search and lock an entry for exclusive access.
   *
   * @param key Key to search for
   * @return Pointer to locked cache entry if found
   */
  Cache_Entry * search_and_lock(const Key & key)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::search(key);
    this->lock_entry(cache_entry);

    return cache_entry;
  }

  /**
   * @brief Search for next entry with same key and lock it.
   *
   * @param cache_entry Current entry
   * @return Pointer to next matching entry if found
   */
  Cache_Entry * search_next_and_lock(Cache_Entry * cache_entry)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    return Cache<Key,Data>::search_next(cache_entry);
  }

  /**
   * @brief Unlock a previously locked cache entry.
   *
   * @param cache_entry Entry to unlock
   */
  void unlock_entry(Cache_Entry * cache_entry)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    Cache<Key,Data>::unlock_entry(cache_entry);
  }

  /**
   * @brief Remove an entry from the cache.
   *
   * @param cache_entry Entry to remove
   */
  void remove(Cache_Entry * cache_entry)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    Cache<Key,Data>::remove(cache_entry);
  }

  /**
   * @brief Expand the cache capacity.
   *
   * @param plus_size Number of additional entries
   */
  void expand(const size_t & plus_size)
  {
    std::lock_guard<std::mutex> lock(_mutex);

    Cache<Key,Data>::expand(plus_size);
  }

  /**
   * @brief Get the cache capacity.
   *
   * @return Maximum number of entries
   */
  const size_t & get_size()
  {
    std::lock_guard<std::mutex> lock(_mutex);

    return Cache<Key,Data>::get_size();
  }

  /**
   * @brief Get the current number of entries.
   *
   * @return Number of entries in cache
   */
  const size_t & get_num_items()
  {
    std::lock_guard<std::mutex> lock(_mutex);

    return Cache<Key,Data>::get_num_items();
  }
};


# endif  // TPL_PROTECTED_CACHE_H
