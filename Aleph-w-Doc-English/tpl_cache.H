 
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_cache.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_CACHE_H
# define TPL_CACHE_H

# include <memory>
# include <aleph.H>
# include <tpl_dnode.H>
# include <tpl_lhash.H>

namespace Aleph {

/** 
  @brief This is an implementation of an associative cache based on a table
  hash.

  @details The cache handlesdetails pair <Key, Data> where Key is the key
  associative and Data is the data related to Key. Not allowed
  pair <Key, Data> duplicates but if it is possible to have key
  duplicates.

  The cache has a size defined by the specified cache_size
  builder. When the number of pairs inserted in the cache
  cache_size reached, then we say that the cache is full. Whether
  try to insert a new pair on a full cache, then it should
  delete a pair. In this implanting, the pair is eliminated less
  Recently Used (LRU for its acronym in English).

  The implementation is based on a hash table resolution
  separately chain collisions. Each bucket of the table
  stores the pair. Additionally, the bucket contains a link
  Connector serves a doubly linked list that simulates
  lru order. In addition, the bucket contains a direct link to the cache
  used to update their statistics.

  Peers can be "blocked"; that is, when a pair is
  locked, it can not be removed from the cache until it is not
  released. A bucket never locked will be selected to
  LRU replacement policy.
*/


    template <class Key, class Data> 
class Cache
{
public:

  class Cache_Entry : public LhashTable<Key>::Bucket
  { 
    friend class Cache<Key, Data>;

    Data    data; 
    Dlink   dlink_lru; /* Enlace cola lru */
    Dlink   dlink_inside;

    bool locked; /* Indicates that entry cannot be deleted */

    bool is_in_hash_table;

    void lock() Exception_Prototypes(std::runtime_error)
    { 
      if (locked)
	throw std::runtime_error("Cache_Entry is already locked");

      locked = true; 
    }

    void unlock() Exception_Prototypes(std::runtime_error)
    { 
      if (not locked)
	throw std::runtime_error("Cache_Entry is not locked");

      locked = false; 
    }

    Dlink* link_lru() { return &dlink_lru; }

    Dlink* link_inside() { return &dlink_inside; }

  public:

    Cache_Entry(const Key & k, const Data & d) : 
      LhashTable<Key>::Bucket(k), 
      data(d), locked(false), is_in_hash_table(false)
    {
      /* Empty */
    }
    
    Cache_Entry() : locked(false), is_in_hash_table(false)
    {
      /* Empty */
    }

    Data & get_data() { return data; }

    const bool & is_locked() const { return locked; }

    const bool & is_in_table() const { return is_in_hash_table; }

    static Cache_Entry * convert_to_cache_entry(Data * data_ptr)
    {
      return reinterpret_cast<Cache_Entry*>(data_ptr);
    }
  }; /* End class Cache_Entry */

  
private:

  Dlink           lru_list; 
  Dlink           locked_list;
  Dlink           inside_list;

  LhashTable<Key> hash_table;

  size_t          cache_size;  /* Length cache */

public:

  const size_t & get_num_entries() const 
  {
    assert(hash_table.size() <= cache_size);

    return hash_table.size();
  }

private:

  size_t   num_lru;     /* Lru number of elements in list */
  size_t   num_locked;  /* Number of blocked items */

/** 
  @typedef Dnode<Cache_Entry*> Chunk_Descriptor;

  @brief This is a list of chunks of memory. 

  @details Each one is added to the list while requiered during the cache lifetime.

  A chunk of memory has the interesting property of being continuous
  in memory, therefore can be cached by the computer. 
*/
  typedef Dnode<Cache_Entry*> Chunk_Descriptor;

  Chunk_Descriptor chunk_list;

protected:  

  LINKNAME_TO_TYPE(Cache_Entry, dlink_lru);

  LINKNAME_TO_TYPE(Cache_Entry, dlink_inside);

  void insert_entry_to_lru_list(Cache_Entry * cache_entry)
  {
    num_lru++;
    lru_list.insert(cache_entry->link_lru());
  }

  void remove_entry_from_lru_list(Cache_Entry * cache_entry)
  {
    num_lru--;
    cache_entry->link_lru()->del();
  }

  void insert_entry_to_locked_list(Cache_Entry * cache_entry)
  {
    num_locked++;
    locked_list.insert(cache_entry->link_lru());
  }

  void remove_entry_from_locked_list(Cache_Entry * cache_entry)
  {
    num_locked--;
    cache_entry->link_lru()->del();
  }

  void move_to_inside_front(Cache_Entry * cache_entry)
  {
    cache_entry->link_inside()->del();
    inside_list.insert(cache_entry->link_inside());
  }

  void move_to_lru_front(Cache_Entry * cache_entry)
  {
    cache_entry->link_lru()->del();
    lru_list.insert(cache_entry->link_lru());
  }

  void move_to_lru_rear(Cache_Entry * cache_entry)
  {
    cache_entry->link_lru()->del();
    lru_list.append(cache_entry->link_lru()); 
  }

  void do_mru(Cache_Entry * cache_entry) 
  { 
    move_to_lru_front(cache_entry); 
  }

  void do_lru(Cache_Entry * cache_entry) 
  { 
    move_to_lru_rear(cache_entry); 
  }

/** 
  @brief Removes hash table and makes the least recently used entry
*/
  void remove_entry_from_hash_table(Cache_Entry * cache_entry)
  {
    assert(not cache_entry->is_locked());

    cache_entry->link_inside()->del();

    hash_table.remove(cache_entry);
    cache_entry->is_in_hash_table = false;
    do_lru(cache_entry);
  }

/** 
  @brief Search and returns next entry according lru priority that has no lock (lock)
  removed from the hash table and makes the most recently used entry. 
*/ 
  Cache_Entry * get_lru_entry()
  {
    assert(hash_table.size() <= cache_size);

    if (lru_list.is_empty())
      throw std::bad_alloc ();

    Dlink * lru_entry_link = lru_list.get_prev();
    Cache_Entry * cache_entry = dlink_lru_to_Cache_Entry(lru_entry_link);

    assert(not cache_entry->is_locked());

    if (cache_entry->is_in_hash_table)
      {
	assert(hash_table.search(cache_entry->get_key()) == cache_entry);
	remove_entry_from_hash_table(cache_entry);
      }

    do_mru(cache_entry);
	  
    return cache_entry;
  }

  Cache_Entry * insert_pair(const Key & key, const Data & data)
  {
    Cache_Entry *cache_entry = get_lru_entry();
	
    cache_entry->get_key()  = key;
    cache_entry->get_data() = data;
	
    inside_list.insert(cache_entry->link_inside());

    hash_table.insert(cache_entry);
    cache_entry->is_in_hash_table = true;

    return cache_entry;
  }

public:

  Cache(size_t (*hash_fct)(const Key&), const size_t & size) : 
    hash_table(hash_fct, size, false),
    cache_size(hash_table.capacity()), num_lru(0), num_locked(0)
  {
    assert(size > 1);
    assert(hash_fct != NULL);

    Cache_Entry * entries_array = new Cache_Entry [cache_size];

    try 
      {
 	std::unique_ptr<Chunk_Descriptor> 
	  chunk_descriptor (new Chunk_Descriptor (entries_array));

	chunk_list.insert(chunk_descriptor.get());

	for (int i = 0; i < cache_size; i++)
	  insert_entry_to_lru_list(&entries_array[i]);
	
	chunk_descriptor.release();
      }
    catch (...)
      {
	delete [] entries_array;
	throw;
      }
  }

  virtual ~Cache() 
  {
    assert(hash_table.size() <= cache_size);
    	
    while (not chunk_list.is_empty())
      {
	Chunk_Descriptor * current_chunk = chunk_list.remove_next();

	delete [] current_chunk->get_data();
	delete current_chunk;
      }
  }      

  Cache_Entry * search(const Key & key)
  {
    assert(hash_table.size() <= cache_size);
    
    Cache_Entry * cache_entry = 
      static_cast<Cache_Entry*>(hash_table.search(key));
    
    if (cache_entry != NULL)
      {
	do_mru(cache_entry);
	move_to_inside_front(cache_entry);
      }	
    
    return cache_entry;
  }
  
/** 
  @brief Returns next cache entry with the same key
  cache_entry if it exists, or null otherwise 
*/  
  Cache_Entry * search_next(Cache_Entry * cache_entry)
  {
    Cache_Entry *next_entry = 
      static_cast<Cache_Entry*>(hash_table.search_next(cache_entry));
    
    if (next_entry != NULL)
      {
	do_mru(next_entry);
	move_to_inside_front(cache_entry);
      }	
    
    return next_entry;
  }

  Cache_Entry * insert(const Key& key, const Data& data) 
    Exception_Prototypes (std::bad_alloc)
  {
    assert(hash_table.size() <= cache_size);

    return insert_pair(key, data);
  }

  void lock_entry(Cache_Entry * cache_entry) 
    Exception_Prototypes(std::runtime_error)
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
    Exception_Prototypes(std::runtime_error)
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
    Exception_Prototypes(std::runtime_error)
  {
    assert(hash_table.search(cache_entry->get_key()) == cache_entry);

    if (cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is already locked");

    remove_entry_from_hash_table(cache_entry);
  }

  void expand(const size_t & plus_size) 
    Exception_Prototypes(std::range_error, std::bad_alloc)
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
      /* Empty */
    }

    Cache_Entry * get_current()  
    { 
      Cache_Entry * ret_val =  
	Cache_Entry::dlink_inside_to_Cache_Entry(Dlink::Iterator::get_current()); 

      assert(ret_val->is_in_table()); 

      return ret_val; 
    } 
  };
};

} /* End namespace Aleph */

# endif // TPL_CACHE_H 
