
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
  @file tpl_hash_cache.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_HASH_CACHE_H
# define TPL_HASH_CACHE_H

# include <memory>
# include <aleph.H>
# include <tpl_dnode.H>
# include <tpl_lhash.H>

namespace Aleph {


/** 
  @brief Cache data mapped to a key.

  @details Hash_Cache <Key, Data> implements a cache instrumented by
  a hash table that associates keys Key to joint dominance
  data set Data range.

  The main methods operate on a type Hash_Cache
  Cache_Entry called, which is the drive that contains the pair
  (key, data).

  @param Key Key of indexing.
  @param Data Data associated with the key.
  @param Cmp Class of comparison between the keys.

  @see Cache_Entry

  @ingroup Hash
  */
  template <typename  Key, typename Data, class Cmp = Aleph::equal_to<Key>>
class Hash_Cache
{

public:

/** 
  @brief Entering a cache.

  @details Cache_Entry defines an input This input is cache.f
  means by which to observe and manipulate the data contained
  in the cache.

  @see Hash_Cache

  @ingroup Hash
*/
  class Cache_Entry : public LhashTable<Key, Cmp>::Bucket
  { 

    friend class Hash_Cache<Key, Data>;

    Data    data; 
    Dlink   dlink_lru; /* Loop to the queue lru */
    Dlink* link_lru() { return &dlink_lru; }
    bool locked; /* It indicates if the entry is locked */
    bool is_in_hash_table; /* It indicates whether entry is contained within hash table */
                    
    void lock() 
    { 
      if (locked)
        throw std::runtime_error("Cache_Entry is already locked");

      locked = true; 
    }

    void unlock()
    { 
      if (not locked)
        throw std::runtime_error("Cache_Entry is not locked");

      locked = false; 
    }
    Dlink   dlink_inside;

    Dlink * link_inside() { return &dlink_inside; }

    LINKNAME_TO_TYPE(Cache_Entry, dlink_inside);

  public:
    Cache_Entry() : locked(false), is_in_hash_table(false) { /* Empty */ }


/** 
  @brief Returns a reference to the data associated with the key. 
*/
    Data & get_data() { return data; }

/** 
  @brief Returns true if the entry is locked.
*/
    const bool & is_locked() const { return locked; }

/** 
  @brief Returns true if the input is within the table
*/
    const bool & is_in_table() const { return is_in_hash_table; }
  }; /* End class Cache_Entry */

private:

  LhashTable<Key, Cmp> hash_table;
  LINKNAME_TO_TYPE(Cache_Entry, dlink_lru);
  Dlink    lru_list; /** Top of the list lru */
  size_t   num_lru;  /** Number of elements in list lru */
  Dlink  inside_list; /** List of remote buckets and stuck in table */
  size_t cache_size;  /** Maximum number of entries that can be cached */
  Dlink  locked_list; /** Entry List trancadas */
  size_t num_locked;  /** Number of locked items */
  typedef Dnode<Cache_Entry*> Chunk_Descriptor;
  Chunk_Descriptor chunk_list;  
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

  void do_mru(Cache_Entry * cache_entry) 
  { 
    cache_entry->link_lru()->del(); /* Delete de current position */
    lru_list.insert(cache_entry->link_lru()); /* Take it to the back */
  }
  void do_lru(Cache_Entry * cache_entry) 
  { 
    cache_entry->link_lru()->del(); /* Delete de current position */
    lru_list.append(cache_entry->link_lru()); /* Take it to the front */
  }
  void remove_entry_from_hash_table(Cache_Entry * cache_entry)
  {
    cache_entry->link_inside()->del();
    hash_table.remove(cache_entry);
    cache_entry->is_in_hash_table = false;
    do_lru(cache_entry);
  }
  Cache_Entry * get_lru_entry()
  {

    if (lru_list.is_empty())   /* Is there an entrance available?*/
      throw std::underflow_error("All entries are locked"); /* No ==> ¡Exception! */

    /* Get oldest; least recently accessed */
    Dlink * lru_entry_link = lru_list.get_prev();
    Cache_Entry * cache_entry = dlink_lru_to_Cache_Entry(lru_entry_link);

    /* If contained in table cache_entry ==> Delete it */
    if (cache_entry->is_in_hash_table) 
      remove_entry_from_hash_table(cache_entry);

    do_mru(cache_entry); /* Becomes most recently accessed entry */

    return cache_entry;
  }

public:

/** 
  @brief Instance cache.

  @details This constructor creates a new cache capacity __cache_size
  and instrumented by a hash table size __hash_size.

  @param [in] hash_fct pointer to the hash function.
  @param [in] __hash_size size of the internal hash table
  @param [in] __cache_size cache size.

  @throw bad_alloc If there is Insufficient Memory.

  @note Since the hash table resolves background
  separate chain collisions, the size of the table
  It can be smaller than the cache size. This is not valid in
  all. If good performance is desired, then make sure
  have a good hash function and the table size is
  at least 20% greater than the cache.
*/
  Hash_Cache(size_t (*hash_fct)(const Key&), 
             const size_t & __hash_size, const size_t & __cache_size) 

    throw (std::exception, std::bad_alloc)

    : hash_table(hash_fct, __hash_size, false),
      num_lru(0), cache_size(__cache_size), num_locked(0)
  {
    /* Apartar entradas del cache */
    Cache_Entry * entries_array = new Cache_Entry [cache_size];

    try 
    { 

    /* Divide the descriptor array */
    std::unique_ptr<Chunk_Descriptor> 
        chunk_descriptor (new Chunk_Descriptor (entries_array));
    chunk_list.insert(chunk_descriptor.get());

    /** Insert each Cache_Entry List lru */
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

  virtual ~Hash_Cache() 
  {      
    /** Browse blocklist and release */
    while (not chunk_list.is_empty())
      {
        Chunk_Descriptor * current_chunk = chunk_list.remove_next();

        delete [] current_chunk->get_data();
        delete current_chunk;
      }
  }      

/** 
  @brief Insert a new pair (key, data) in the cache.

  @details insert (key, data) inserted into the cache the pair (key, data). if he
  cache is full, then the oldest continuously elements
  accessed is removed and replaced by the insertion torque.

  After insertion, the pair (key, data) becomes the element
  most recently accessed.

  @param[in] key Key of indexing.
  @param[in] data Data associated with the key.

  @return a pointer to an object of type [[Cache_Entry]]
  contentive pair (key, data) newly inserted.

  @throw underflow_error If the cache is full and all locked entries.
*/
  Cache_Entry * insert(const Key & key, const Data & data) 
  {
    Cache_Entry * cache_entry = get_lru_entry(); /* Oldest */
    cache_entry->get_key()    = key;             /* Write the pair */
    cache_entry->get_data()   = data;
    inside_list.insert(cache_entry->link_inside());
    hash_table.insert(cache_entry);            
    cache_entry->is_in_hash_table = true;
    return cache_entry;
  }

/** 
  @brief Search a key in the cache.

  @details search (key) searches the cache entry with key value.

  @param [in] key The key to look for.

  @return a pointer to a document containing key with Cache_Entry
  all data associated if the key is found. NULL than
  otherwise
*/
  Cache_Entry * search(const Key & key)
  {     

    /* Search the hash table */
    Cache_Entry * cache_entry = (Cache_Entry*) hash_table.search(key);
    if (cache_entry != NULL) /* Key Found?*/
      {     /* Yes ==> make the most recently used*/
        do_mru(cache_entry);
        move_to_inside_front(cache_entry);
      } 
    return cache_entry;
  }

/** 
  @brief Search next entry with the same key value cache_entry.

  @details details This function is provided to search various data that are
  inserted in the cache under the same key value.

  @param [in] cache_entry input resulting from a search
  Prior or another call to this primitive.

  @return A pointer to the next contentivo Cache_Entry thereof
  if it exists-key, or NULL otherwise.
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

/** 
  @brief cache_entry Trancabrief the entry, which must be in the cache and
  It should not be locked. Shoot runtime_error exception if the
  entry is already locked or domain_error if it is not in the cache.
*/
  void lock_entry(Cache_Entry * cache_entry) 

    throw(std::exception, std::runtime_error, std::domain_error)

  {

    if (cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is already locked");

    if (not cache_entry->is_in_table())
      throw std::domain_error("Cache_Entry is not in the cache");

    remove_entry_from_lru_list(cache_entry);
    insert_entry_to_locked_list(cache_entry);
    cache_entry->lock();
  }

/** 
  @brief cache_entry Destranca the entrance.

  @throw runtime_error if the input is not locked.
*/
  void unlock_entry(Cache_Entry * cache_entry) 

    throw(std::exception, std::runtime_error)

  { 

    if (not cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is not locked");

    remove_entry_from_locked_list(cache_entry);
    insert_entry_to_lru_list(cache_entry);
    cache_entry->unlock();
  }
/** 
  @brief Eliminates cache_entry entry cache.

  @throw runtime_error if the entry is locked or
  domain_error if it is not contained in the cache.
*/
  void remove(Cache_Entry * cache_entry) 

    throw(std::exception, std::runtime_error, std::domain_error)

  {

    if (cache_entry->is_locked())
      throw std::runtime_error("Cache_Entry is already locked");
    if (not cache_entry->is_in_table())
      throw std::domain_error("Cache_Entry is not in the cache");

    remove_entry_from_hash_table(cache_entry);
  }
/** 
  @brief Enlarges plus_size cache entries.

  @details The new table size is calculated to be the same radius
  specified during construction.

  @throw bad_alloc if there is insufficient memory.
*/ 
  void expand(const size_t & plus_size) 

    throw(std::exception, std::range_error, std::bad_alloc)

  {

    if (plus_size == 0)
      throw std::range_error ("bad plus_size");

    const size_t new_cache_size = cache_size + plus_size;

    /* Away plus_size new entries */
    Cache_Entry * entries_array = new Cache_Entry [plus_size];

    try
      {

    std::unique_ptr<Chunk_Descriptor>     /* Away descriptor */
      chunk_descriptor (new Chunk_Descriptor (entries_array));

    /* Calculate new table size and relocate your tickets */
    const float curr_hash_ratio = 1.0*cache_size/hash_table.capacity();
    const size_t new_hash_capacity = new_cache_size/curr_hash_ratio;

    hash_table.resize(new_hash_capacity);

    /** Put new entries lru_list */
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


/** 
  @brief Returns the cache size. 
*/
  const size_t & capacity() const { return cache_size; }

/** 
  @brief Returns the number of data that are contained in the cache. 
*/
  const size_t & size() const { return hash_table.size(); }

/** 
  @brief Returns the number of cache entries that are
  locked, The difference in size provides capacity
  of tickets that can be inserted.
*/
  const size_t & get_num_locked() const { return num_locked; }

/** 
  @brief Returns the number of entries in the hash table that have
  elements.

  @details Their relation to the size of the table provides a
  idea of the efficiency of the hash function.
*/
  const size_t & get_num_busy_slots() const 
  { 
    return hash_table.get_num_busy_slots(); 
  }

/** 
  @brief Returns the size of the hash table. 
*/
  const size_t & get_hash_capacity() const 
  {
    return hash_table.capacity();
  }

/** 
  @brief iterator over the elements contained in a cache. the order
  Business is from the most recently accessed to less
  recently accessed.
*/
  class Iterator : public Dlink::Iterator
  {

  public:
/** 
  @brief The set type on which iterates. 
*/
    typedef Hash_Cache Set_Type;
/** 
  @brief The type of item returning get_current(). 
*/
    typedef Cache_Entry * Item_Type;

/** 
  @brief An iterator over cache instance.
*/
    Iterator(Hash_Cache & cache) : Dlink::Iterator(&cache.inside_list) {}

/** 
  @brief Return the current Cache_Entry. 
*/
    Cache_Entry * get_current()  
    { 
      Dlink * dl = Dlink::Iterator::get_current();
      return Cache_Entry::dlink_inside_to_Cache_Entry(dl); 
    } 
  };
};


} /* End namespace Aleph */

# endif // TPL_HASH_CACHE_H 
