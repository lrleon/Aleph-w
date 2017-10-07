
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
  @file ah_stdc++_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


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
