
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
# ifndef TPL_CACHE_H
# define TPL_CACHE_H

# include <memory>
# include <aleph.H>
# include <tpl_dnode.H>
# include <tpl_lhash.H>

namespace Aleph {

/* 
   Esta es una implantacion de un cache asociativo basado en una tabla
   hash. El cache maneja pares <Key, Data> donde Key es la clave
   asociativa y Data es el dato relacionado a Key. No se permiten
   pares <Key, Data> duplicados pero si es posible tener key
   duplicados. 

   El cache tiene un tamaño definido por cache_size especificado en el
   constructor. Cuando el número de pares insertados en el cache
   alcanza cache_size, entonces se dice que el cache está lleno. Si se
   intenta insertar un nuevo par en un cache lleno, entonces se debe
   eliminar un par. En esta implatación, se elimina el par menos
   recientemente utilizado (lru de sus siglas en inglés).

   La implantación se basa en una tabla hash con resolución de
   colisiones por encadenamiento separado. Cada bucket de la tabla
   almacena el par. Adicionalmente, el bucket contiene un enlace que
   funge de conector a una lista doblemente enlazada que simula el
   orden lru. Además, el bucket contiene un enlace directo al cache
   usado para actualizar sus estadísticas.

   Los pares pueden ser "bloqueados"; esto es: cuando un par es
   bloqueado, este no puede eliminarse del cache hasta que éste no sea
   liberado. Un bucket bloqueado nunca será seleccionado para
   reemplazo por la política lru.

*/


  template <class Key, class Data, class Cmp = std::equal_to<Key>> 
class Cache
{
public:

  class Cache_Entry : public LhashTable<Key>::Bucket
  { 
    friend class Cache<Key, Data>;

    Data    data; 
    Dlink   dlink_lru; // enlace cola lru 
    Dlink   dlink_inside;

    bool locked; // Indicates that entry cannot be deleted

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
      // empty
    }
    
    Cache_Entry() : data(), locked(false), is_in_hash_table(false)
    {
      // empty
    }

    Data & get_data() { return data; }

    const bool & is_locked() const { return locked; }

    const bool & is_in_table() const { return is_in_hash_table; }

    static Cache_Entry * convert_to_cache_entry(Data * data_ptr)
    {
      return reinterpret_cast<Cache_Entry*>(data_ptr);
    }
  }; // fin class Cache_Entry

  
private:

  Dlink           lru_list; 
  Dlink           locked_list;
  Dlink           inside_list;

  LhashTable<Key> hash_table;

  size_t          cache_size;  /* longitud del cache */

public:

  const size_t & get_num_entries() const 
  {
    assert(hash_table.size() <= cache_size);

    return hash_table.size();
  }

private:

  size_t   num_lru;     /* numero de elementos en lista lru */
  size_t   num_locked;  /* numero de elementos bloqueados */

  /* This is a list of chunks of memory. Each one is added to the list
     while requiered during the cache lifetime.

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

  /*
    elimina de tabla hash y hace la entrada la menos recientemente usada
  */
  void remove_entry_from_hash_table(Cache_Entry * cache_entry)
  {
    assert(not cache_entry->is_locked());

    cache_entry->link_inside()->del();

    hash_table.remove(cache_entry);
    cache_entry->is_in_hash_table = false;
    do_lru(cache_entry);
  }

  /* 
     busca y retorna proxima entrada segun prioridad lru que no tenga
     cerrojo (lock), elimina de la tabla hash y hace la entrada la mas
     recientemente usada.
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
    hash_table(size, hash_fct),
    cache_size(hash_table.capacity()), num_lru(0), num_locked(0)
  {
    assert(size > 1);
    assert(hash_fct != nullptr);

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

  virtual ~Cache() throw(domain_error)
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
    
    if (cache_entry != nullptr)
      {
	do_mru(cache_entry);
	move_to_inside_front(cache_entry);
      }	
    
    return cache_entry;
  }
  
  /* retorna proxima entrada del cache con la misma clave de
     cache_entry si esta existe o nulo de lo contrario */  
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
      // empty
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

} // end namespace Aleph

# endif // TPL_CACHE_H 
