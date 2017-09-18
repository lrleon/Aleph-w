# ifndef TPL_PROTECTED_CACHE_H
# define TPL_PROTECTED_CACHE_H

# include <tpl_cache.H>
# include <useMutex.H>

# include <pthread.h>

    template <class Key, class Data>
class Protected_Cache : protected Cache<Key, Data>
{
private:

  pthread_mutex_t mutex;

public:

  typedef typename Cache<Key, Data>::Cache_Entry Cache_Entry;
  
  Protected_Cache(size_t (*hash_fct)(const Key&), const size_t & size) 
    : Cache<Key,Data>(hash_fct, size)
  {
    init_mutex(mutex);
  }

  virtual ~Protected_Cache() 
  { 
    destroy_mutex(mutex); 
  }

  Cache_Entry * insert(const Key& key, const Data& data)
  {
    CRITICAL_SECTION(mutex);

    return Cache<Key,Data>::insert(key, data);
  }

  Cache_Entry * search(const Key & key) 
  {
    CRITICAL_SECTION(mutex);

    return Cache<Key,Data>::search(key);
  }

  Cache_Entry * insert_and_lock(const Key& key, const Data& data)
  {
    CRITICAL_SECTION(mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::insert(key, data);
    this->lock_entry(cache_entry);

    return cache_entry;
  }

  Cache_Entry * search_and_lock(const Key & key) 
  {
    CRITICAL_SECTION(mutex);
    Cache_Entry * cache_entry = Cache<Key,Data>::search(key); 
    this->lock_entry(cache_entry);

    return cache_entry;
 }

  Cache_Entry * search_next_and_lock(Cache_Entry * cache_entry) 
  {
    CRITICAL_SECTION(mutex);

    return Cache<Key,Data>::search_next(cache_entry);
  }

  void unlock_entry(Cache_Entry * cache_entry) 
    Exception_Prototypes(std::runtime_error)
  { 
    CRITICAL_SECTION(mutex);
    Cache<Key,Data>::unlock_entry(cache_entry);
  }

  void remove(Cache_Entry * cache_entry) 
    Exception_Prototypes(std::runtime_error)
  {
    CRITICAL_SECTION(mutex);
    Cache<Key,Data>::remove(cache_entry);
  }

  void expand(const size_t & plus_size) 
    Exception_Prototypes(std::range_error, std::bad_alloc)
  {
    CRITICAL_SECTION(mutex);

    Cache<Key,Data>::expand(plus_size);
  }
  
  const size_t & get_size() const 
  { 
    CRITICAL_SECTION(mutex);

    return Cache<Key,Data>::get_size(); 
  }

  const size_t & get_num_items() const 
  { 
    CRITICAL_SECTION(mutex);

    return Cache<Key,Data>::get_num_items(); 
  }
};


# endif  // TPL_PROTECTED_CACHE_H


