

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

    /// Tranca la entrada cache_entry, la cual debe pertenecer al cache y
    /// no debe estar trancada. Dispara excepción runtime_error si la
    /// entrada ya está trancada o domain_error si no está en el cache.
    void lock_entry(Cache_Entry * cache_entry)
    {

      if (cache_entry->is_locked())
        throw std::runtime_error("Cache_Entry is already locked");

      if (not cache_entry->is_in_table())
        throw std::domain_error("Cache_Entry is not in the cache");

      remove_entry_from_lru_list(cache_entry);
      insert_entry_to_locked_list(cache_entry);
      cache_entry->lock();
    }

    /// Destranca la entrada cache_entry. Dispara excepción runtime_error si la
    /// entrada no está trancada.
    void unlock_entry(Cache_Entry * cache_entry)
    {
      if (not cache_entry->is_locked())
        throw std::runtime_error("Cache_Entry is not locked");

      remove_entry_from_locked_list(cache_entry);
      insert_entry_to_lru_list(cache_entry);
      cache_entry->unlock();
    }

    /// Elimina la entrada cache_entry del cache. Dispara excepción
    /// runtime_error si la entrada está trancada o domain_error si no
    /// está contenida en el cache.
    void remove(Cache_Entry * cache_entry)
    {
      if (cache_entry->is_locked())
        throw std::runtime_error("Cache_Entry is already locked");
      if (not cache_entry->is_in_table())
        throw std::domain_error("Cache_Entry is not in the cache");

      remove_entry_from_hash_table(cache_entry);
    }

    /// Aumenta el tamaño de cache en plus_size entradas. Dispara
    /// bad_alloc si no hay suficiente memoria. El nuevo tamaño de la
    /// tabla es calculado a ser el mismo radio especificado durante
    /// la construcción.
    void expand(const size_t & plus_size)
    {
      if (plus_size == 0)
        throw std::range_error ("bad plus_size");

      const size_t new_cache_size = cache_size + plus_size;

      // apartar plus_size nuevas entradas
      Cache_Entry * entries_array = new Cache_Entry [plus_size];

      try
        {
          std::unique_ptr<Chunk_Descriptor>     // apartar el descriptor
            chunk_descriptor (new Chunk_Descriptor (entries_array));

          // Calcular nuevo tamaño de tabla y relocalizar sus entradas
          const float curr_hash_ratio = 1.0*cache_size/hash_table.capacity();
          const size_t new_hash_capacity = new_cache_size/curr_hash_ratio;

          hash_table.resize(new_hash_capacity);

          // meter nuevas entradas en lru_list
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

    /// Retorna el tamaño de cache.
    const size_t & capacity() const { return cache_size; }

    /// Retorna en número de datos que están contenidos en el cache.
    const size_t & size() const { return hash_table.size(); }

    /// Retorna la cantidad de entradas del cache que están
    /// trancadas. La diferencia con el tamaño proporciona su capacidad
    /// en entradas que pueden insertarse.
    const size_t & get_num_locked() const { return num_locked; }

    /// Retorna la cantidad de entradas de la tabla hash que tienen
    /// elementos. Su relación con el tamaño de la tabla proporciona una
    /// idea de la eficiencia de la función hash.
    const size_t & get_num_busy_slots() const
    {
      return hash_table.get_num_busy_slots();
    }

    /// Retorna el tamaño de la tabla hash.
    const size_t & get_hash_capacity() const
    {
      return hash_table.capacity();
    }

    /// Iterador sobre los elementos contenidos en un cache. El orden
    /// de visita es desde el más recientemente accedido hasta el menos
    /// recientemente accedido.
    class Iterator : public Dlink::Iterator
    {
    public:
      /// El tipo de conjunto sobre el cual se itera.
      typedef Hash_Cache Set_Type;
      /// El tipo de elemento que retorna get_curr().
      typedef Cache_Entry * Item_Type;

      /// Instancia un iterador sobre cache.
      Iterator(Hash_Cache & cache) : Dlink::Iterator(&cache.inside_list) {}

      /// Retorna el Cache_Entry actual.
      Cache_Entry * get_curr()
      {
        Dlink * dl = Dlink::Iterator::get_curr();
        return Cache_Entry::dlink_inside_to_Cache_Entry(dl);
      }
    };
  };


} // end namespace Aleph

# endif // TPL_HASH_CACHE_H

