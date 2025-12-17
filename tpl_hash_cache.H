
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

# ifndef TPL_HASH_CACHE_H
# define TPL_HASH_CACHE_H

# include <memory>
# include <aleph.H>
# include <tpl_dnode.H>
# include <tpl_lhash.H>
# include <ah-errors.H>

namespace Aleph {


  /** Cache de datos mapeados a una clave.

      Hash_Cache<Key,Data> implanta un cache instrumentado mediante
      una tabla hash que asocia claves de un conjunto dominio Key a
      datos de un conjunto rango Data.

      Los métodos principales de Hash_Cache operan según un tipo
      denominado Cache_Entry, el cual es la unidad que contiene el par
      (clave,dato).

      @param Key tipo de clave de indización.
      @param Data tipo de dato asociado a la clave.
      @param Cmp clase de comparación entre las claves.

      @see Cache_Entry
      @ingroup Hash
  */
  template <typename  Key, typename Data, class Cmp = Aleph::equal_to<Key>>
  class Hash_Cache
  {

  public:

    /** Entrada de un cache.

        Cache_Entry define una entrada del cache.f Esta entrada es el
        medio a través del cual se observar y manipula el dato contenido
        en el cache.

        @see Hash_Cache

        @ingroup Hash
    */
    class Cache_Entry : public LhashTable<Key, Cmp>::Bucket
    {
      friend class Hash_Cache<Key, Data>;

      Data    data;
      Dlink   dlink_lru; // enlace a la cola lru
      Dlink* link_lru() { return &dlink_lru; }
      bool locked; // indica si la entrada está trancada
      bool is_in_hash_table; // indica si entrada está contenida
                             // dentro de tabla hash  
      void lock()
      {
        ah_runtime_error_if(locked)
          << "Cache_Entry is already locked";

        locked = true;
      }

      void unlock()
      {
        ah_runtime_error_if(not locked)
          << "Cache_Entry is not locked";

        locked = false;
      }

      Dlink   dlink_inside;

      Dlink * link_inside() { return &dlink_inside; }

      LINKNAME_TO_TYPE(Cache_Entry, dlink_inside);

    public:

      Cache_Entry()
        : data(), locked(false), is_in_hash_table(false) { /* empty */ }

      /// Retorna una referencia al dato asociado a la clave
      Data & get_data() { return data; }
      
      /// Retorna true si la entrada está trancada.
      const bool & is_locked() const { return locked; }

      /// Retorna true si la entrada está dentro de la tabla.
      const bool & is_in_table() const { return is_in_hash_table; }
    }; // fin class Cache_Entry

  private:

    LhashTable<Key, Cmp> hash_table;
    LINKNAME_TO_TYPE(Cache_Entry, dlink_lru);
    Dlink    lru_list; // cabecera de la lista lru
    size_t   num_lru;  // número de elementos en lista lru
    Dlink  inside_list; // lista de cubetas apartadas y metidas en tabla
    size_t cache_size;  // máx número de entradas que puede tener cache
    Dlink  locked_list; // lista de entradas trancadas
    size_t num_locked;  // número de elementos trancados
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
      cache_entry->link_lru()->del(); // elimine de posición actual
      lru_list.insert(cache_entry->link_lru()); // llévela a trasero
    }

    void do_lru(Cache_Entry * cache_entry)
    {
      cache_entry->link_lru()->del(); // elimine de posición actual
      lru_list.append(cache_entry->link_lru()); // llévela al frente
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
      ah_underflow_error_if(lru_list.is_empty()) // ¿existe una entrada disponible?
        << "All entries are locked"; // no ==> ¡excepción! 

      // obtenga  entrada más antigua; menos recientemente accedida
      Dlink * lru_entry_link = lru_list.get_prev();
      Cache_Entry * cache_entry = dlink_lru_to_Cache_Entry(lru_entry_link);

      // si cache_entry contenida en tabla ==> eliminarlo
      if (cache_entry->is_in_hash_table)
        remove_entry_from_hash_table(cache_entry);

      do_mru(cache_entry); // entrada deviene más recientemente accedida

      return cache_entry;
    }

  public:

    /** Instancia un cache.

        Este constructor crea un nuevo cache de capacidad __cache_size
        e instrumentado mediante una tabla hash de tamaño __hash_size.

        \param[in] hash_fct puntero a la función hash.
        \param[in] __hash_size tamaño de la tabla hash interna
        \param[in] __cache_size tamaño del cache.

        @throw bad_alloc si no hay suficiente memoria.

        @note Puesto que la tabla hash de trasfondo resuelve las
        colisiones por encadenamiento separado, el tamaño de la tabla
        puede ser menor que el tamaño del cache. Esto no se valida en
        absoluto. Si se desea un buen rendimiento, entonces asegúrese de
        tener una buena función hash y de que el tamaño de la tabla sea
        al menos un 20% mayor que el de cache.
    */
    Hash_Cache(size_t (*hash_fct)(const Key&),
               size_t __hash_size, const size_t & __cache_size)
      : hash_table(__hash_size, hash_fct),
        num_lru(0), cache_size(__cache_size), num_locked(0)
    {
      // apartar entradas del cache
      Cache_Entry * entries_array = new Cache_Entry [cache_size];

      try
        { // apartar el descriptor del arreglo
          std::unique_ptr<Chunk_Descriptor>
            chunk_descriptor (new Chunk_Descriptor (entries_array));
          chunk_list.insert(chunk_descriptor.get());

          // insertar cada Cache_Entry en lista lru
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
    {      // recorrer lista de bloques y liberarlos
      while (not chunk_list.is_empty())
        {
          Chunk_Descriptor * current_chunk = chunk_list.remove_next();

          delete [] current_chunk->get_data();
          delete current_chunk;
        }
    }

    /** Inserta un nuevo par (key,data) en el cache.

        insert(key,data) inserta en el cache el par (key,data). Si el
        cache está lleno, entonces el elementos más antiguamente
        accedido es eliminado y substituido por el par de inserción.

        Después de la inserción, el par (key,data) deviene el elemento
        más recientemente accedido.

        @param[in] key clave de indización.
        @param[in] data dato asociado a la clave.
        @return un puntero a un Objeto de tipo [[Cache_Entry]]
        contentivo del par (key,data) recién insertado.

        @throw underflow_error si el cache está lleno y todas sus
        entradas trancadas.
    */
    Cache_Entry * insert(const Key & key, const Data & data)
    {
      Cache_Entry * cache_entry = get_lru_entry(); // entrada más antigua
      cache_entry->get_key()    = key;             // escribirle el par
      cache_entry->get_data()   = data;
      inside_list.insert(cache_entry->link_inside());
      hash_table.insert(cache_entry);
      cache_entry->is_in_hash_table = true;
      return cache_entry;
    }
    /** Busca una clave en el cache.

        search(key) busca en el cache una entrada con valor key.

        @param[in] key clave a buscar.
        @return un puntero a un Cache_Entry contentivo de la clave con
        todos los datos asociados si la clave es encontrada. nullptr de lo
        contrario.
    */
    Cache_Entry * search(const Key & key)
    {     // buscar en la tabla hash
      Cache_Entry * cache_entry = (Cache_Entry*) hash_table.search(key);
      if (cache_entry != nullptr) // ¿fue encontrada la clave?
        {     // sí ==> hacerla la más recientemente usada
          do_mru(cache_entry);
          move_to_inside_front(cache_entry);
        }
      return cache_entry;
    }

    /** Busca la próxima entrada con el mismo valor de clave de cache_entry.

        Esta función se provee para buscar datos distintos que estén
        insertados en el cache bajo el mismo valor de clave.

        @param[in] cache_entry una entrada resultante de una búsqueda
        previa u otra llamada a esta primitiva.
        @return puntero al próximo Cache_Entry contentivo de la misma
        clave -si éste existe-, o nullptr de lo contrario.
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
      ah_runtime_error_if(cache_entry->is_locked())
        << "Cache_Entry is already locked";

      ah_domain_error_if(not cache_entry->is_in_table())
        << "Cache_Entry is not in the cache";

      remove_entry_from_lru_list(cache_entry);
      insert_entry_to_locked_list(cache_entry);
      cache_entry->lock();
    }

    /// Destranca la entrada cache_entry. Dispara excepción runtime_error si la
    /// entrada no está trancada.
    void unlock_entry(Cache_Entry * cache_entry)
    {
      ah_runtime_error_if(not cache_entry->is_locked())
        << "Cache_Entry is not locked";

      remove_entry_from_locked_list(cache_entry);
      insert_entry_to_lru_list(cache_entry);
      cache_entry->unlock();
    }

    /// Elimina la entrada cache_entry del cache. Dispara excepción
    /// runtime_error si la entrada está trancada o domain_error si no
    /// está contenida en el cache.
    void remove(Cache_Entry * cache_entry)
    {
      ah_runtime_error_if(cache_entry->is_locked())
        << "Cache_Entry is already locked";
      ah_domain_error_if(not cache_entry->is_in_table())
        << "Cache_Entry is not in the cache";

      remove_entry_from_hash_table(cache_entry);
    }

    /// Aumenta el tamaño de cache en plus_size entradas. Dispara
    /// bad_alloc si no hay suficiente memoria. El nuevo tamaño de la
    /// tabla es calculado a ser el mismo radio especificado durante
    /// la construcción.
    void expand(const size_t & plus_size)
    {
      ah_range_error_if(plus_size == 0)
        << "bad plus_size";

      const size_t new_cache_size = cache_size + plus_size;

      // apartar plus_size nuevas entradas
      auto * entries_array = new Cache_Entry [plus_size];

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

