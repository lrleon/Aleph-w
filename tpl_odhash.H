

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

    ODhashTable(size_t   len                 = Primes::DefaultPrime,
                Hash_Fct_Ptr first_hash_fct  = Aleph::dft_hash_fct<Key>,
                Hash_Fct_Ptr second_hash_fct = Aleph::snd_hash_fct<Key>,
                Cmp      cmp                 = Cmp(),
                float    lower_alpha         = hash_default_lower_alpha,
                float    upper_alpha         = hash_default_upper_alpha,
                bool     with_resize         = true)
      : ODhashTable(len, Hash_Fct(first_hash_fct), Hash_Fct(second_hash_fct),
                    cmp, lower_alpha, upper_alpha, with_resize) {}

    /// Libera toda la tabla hash.
    ~ODhashTable()
    {
      if (table != nullptr)
        delete [] table;
    }

    ODhashTable(const ODhashTable & other)
      : ODhashTable(other.len, other.hash_fct, other.second_hash_fct, other.cmp,
                    other.lower_alpha, other.upper_alpha, other.with_resize)
    {
      assert(table != nullptr);
      this->copy_from_table(other);
    }

    ODhashTable(ODhashTable && other) : ODhashTable(other)
    {
      assert(table != nullptr);
      swap(other);
    }

    Special_Ctors(ODhashTable, Key);

    ODhashTable & operator = (const ODhashTable & other)
    {
      if (this == &other)
        return *this;

      if (len > other.N)
        this->clean_table();
      else
        {
          Bucket * new_table = new Bucket [other.len];
          delete [] table;
          table = new_table;
          N = 0;
          len = other.len;
          hash_fct        = other.hash_fct;
          second_hash_fct = other.second_hash_fct;
          cmp             = other.cmp;
          lower_alpha     = other.lower_alpha;
          upper_alpha     = other.upper_alpha;
          with_resize     = other.with_resize;
        }

      this->copy_from_table(other);

      return *this;
    }

    ODhashTable & operator = (ODhashTable && other) noexcept
    {
      swap(other);
      return *this;
    }

    OHASH_COMMON(ODhashTable);


    /// busca en la tabla la clave key. Retorna un puntero al registro
    /// asociado a key dentro de la tabla; nullptr de lo contrario.
    Key * search(const Key & key) const noexcept
    {
      const size_t i_fst = hash_fct(key) % len; // 1er sondeo (1ra fun hash)
      if (table[i_fst].status == EMPTY) 
        return nullptr;

      if (table[i_fst].status == BUSY and cmp(table[i_fst].key, key))
        {
          assert(table[i_fst].probe_type == FIRST_PROBE);
          assert(table[i_fst].probe_counter > 0);
          return &table[i_fst].key;
        }

      const size_t i_snd = second_hash_fct(key) % len; // 2do sondeo

      if (table[i_snd].status == EMPTY)
        return nullptr;

      if (table[i_snd].status == BUSY and cmp(table[i_snd].key, key)) 
        {
          assert(table[i_snd].probe_type == SECOND_PROBE);
          assert(table[i_snd].probe_counter > 0);
          return &table[i_snd].key;
        }

      size_t i = i_snd; 
      // sondeo lineal a partir de índice de 2da función hash
      for (size_t count = 0; count < len; ++count)
        {
          index_forward(i);
          switch (table[i].status)
            {
            case EMPTY:
              assert(table[i].probe_counter == 0);
              return nullptr;
            case BUSY:
              assert(table[i].probe_counter > 0);
              if (cmp(table[i].key, key))
                {
                  assert(table[i].probe_type == LINEAR_PROBE);
                  return &table[i].key;
                }
              break;
            case DELETED:
              assert(table[i].probe_counter > 0);
              break;
            default: AH_ERROR("ODhashTable search: inconsistent bucket status");
            }
        }

      return nullptr;
    }

    Hash_Fct get_second_hash_fct() const noexcept { return second_hash_fct; }

    void set_second_hash_fct(Hash_Fct fct) noexcept
    {
      second_hash_fct = fct;
    }

    void set_second_hash_fct(Hash_Fct_Ptr fct) noexcept
    {
      second_hash_fct = Hash_Fct(fct);
    }

  private:

    Bucket * allocate_bucket(const Key & key) noexcept
    {
      assert(N < len);

      const size_t i_fst = hash_fct(key) % len; // sondeo con 1ra función hash
      if (table[i_fst].status != BUSY) // ¿cubeta disponible?
        return allocate_bucket(table[i_fst], FIRST_PROBE);

      if (cmp(table[i_fst].key, key)) // test if key is alredy present
        return nullptr;

      const size_t i_snd = second_hash_fct(key) % len; // 2do hash
      if (table[i_snd].status != BUSY) // ¿cubeta disponible?
        {
          table[i_fst].probe_counter++;
          return allocate_bucket(table[i_snd], SECOND_PROBE);
        }

      if (cmp(table[i_snd].key, key)) // test if key is alredy present
        return nullptr;

      size_t i = i_snd;
      for (size_t c = 0; c < len; ++c)
        {
          index_forward(i);
          switch (table[i].status)
            {
            case BUSY:
              if (cmp(table[i].key, key))
                { // duplicated key ==> rollback previous probe_counter increases
                  for (size_t k = 0; k < c; ++k)
                    {
                      index_backward(i);
                      table[i].probe_counter--;
                    }
                  return nullptr;
                }
              break;
            case DELETED:
            case EMPTY:
              table[i_fst].probe_counter++;
              table[i_snd].probe_counter++;
              return allocate_bucket(table[i], LINEAR_PROBE);
            default: AH_ERROR("ODhashTable: Invalid bucket status"); break;
            }
          table[i].probe_counter++;
        }

      return nullptr;
    }

    // search key. If found return true, otherwise false and allocates the bucket
    tuple<Bucket*, bool> hard_allocate_bucket(const Key & key) noexcept
    {
      assert(N < len);

      const size_t i_fst = hash_fct(key) % len; // sondeo con 1ra función hash
      if (table[i_fst].status != BUSY) // ¿cubeta disponible?
        return tuple<Bucket*, bool>(allocate_bucket(table[i_fst], FIRST_PROBE),
                                    false);

      if (cmp(table[i_fst].key, key)) // test if key is alredy present
        return tuple<Bucket*, bool>(&table[i_fst], true);

      const size_t i_snd = second_hash_fct(key) % len; // 2do hash
      if (table[i_snd].status != BUSY) // ¿cubeta disponible?
        {
          table[i_fst].probe_counter++;
          return tuple<Bucket*, bool>(allocate_bucket(table[i_snd], SECOND_PROBE),
                                      false);
        }

      if (cmp(table[i_snd].key, key)) // test if key is alredy present
        return tuple<Bucket*, bool>(&table[i_snd], true);

      size_t i = i_snd;
      for (size_t c = 0; c < len; ++c)
        {
          index_forward(i);
          switch (table[i].status)
            {
            case BUSY:
              if (cmp(table[i].key, key))
                { // duplicated key ==> rollback previous probe_counter increases
                  const size_t idx = i;
                  for (size_t k = 0; k < c; ++k)
                    {
                      index_backward(i);
                      table[i].probe_counter--;
                    }
                  return tuple<Bucket*, bool>(&table[idx], true);
                }
              break;
            case DELETED:
            case EMPTY:
              table[i_fst].probe_counter++;
              table[i_snd].probe_counter++;
              return make_tuple(allocate_bucket(table[i], LINEAR_PROBE), false);
            default: AH_ERROR("ODhashTable: Invalid bucket status"); break;
            }
          table[i].probe_counter++;
        }
      assert(false); // never must reach this point
      return tuple<Bucket*, bool>((Bucket*)nullptr, false);
    }

    /// Elimina de la tabla el registro record. record debe pertenecer a
    /// la tabla y debe haberse determinado previamente mediante una
    /// inserción o búsqueda. Se dispara la excepción invalid_argument si
    /// el registro no corresponde a una cubeta de la tabla. Se dispara
    /// domain_error si el registro en la tabla no contiene un elemento.
    void remove_bucket(Bucket * bucket)
    {
      if (not is_valid_bucket(bucket))
        throw std::invalid_argument("key pty does not belong to hash table");

      if (bucket->status != BUSY)
        throw std::domain_error("Bucket containing key is not BUSY");

      deallocate_bucket(bucket);
    }

    // returns true if the bucket contains the key
    bool process_entry_for_remove(Bucket * bucket, const Key & key) 
    {
      switch (bucket->status)
        {
        case EMPTY:
          throw std::domain_error("Key not in hash table");
        case BUSY:
          if (cmp(bucket->key, key))
            {
              bucket->status = DELETED;
              decrease_probe_counter(bucket);
              return true;
            }
          break;
        case DELETED:
          decrease_probe_counter(bucket);
          break;
        default: AH_ERROR("Inconsistent bucket status");
        }
      return false;
    }

  public:

    void remove(const Key & key)
    {
      --N;
      try
        {
          const size_t i_fst = hash_fct(key) % len;
          if (process_entry_for_remove(&table[i_fst], key))
            return;
    
          const size_t i_snd = second_hash_fct(key) % len;
          if (process_entry_for_remove(&table[i_snd], key))
            return;

          size_t i = i_snd;
          for (size_t c = 0; i < len; ++c)
            {
              index_forward(i);
              if (process_entry_for_remove(&table[i], key))
                return;
            }
        }
      catch (...)
        {
          ++N;
          this->rehash(); // in order to leave the table in a consistent state
          throw;
        }
    }
  
    using Stats = typename OhashCommon<ODhashTable<Key, Cmp>, Key>::Stats;

    Stats stats()	const							
    {									
      DynArray<size_t> lens;						
      size_t num_busy = 0, num_deleted = 0, num_empty = 0;			
      size_t max_len = std::numeric_limits<size_t>::min();			
      for (size_t i = 0; i < len; ++i)					
        switch (table[i].status)						
          {								
          case BUSY:							
            {								
              ++num_busy;							
              const Key & key = table[i].key;				
              size_t count = 1;
              const size_t i_fst = hash_fct(key) % len;
              if (cmp(table[i_fst].key, key))
                {
                  assert(table[i_fst].probe_type == FIRST_PROBE);
                  assert(table[i_fst].probe_counter > 0);
                  ;
                }
              else 
                {
                  ++count;
                  size_t i_snd = second_hash_fct(key) % len;
                  if (cmp(table[i_snd].key, key))
                    {
                      assert(table[i_snd].probe_type == SECOND_PROBE);
                      assert(table[i_snd].probe_counter > 0);
                      ;
                    }
                  else
                    {
                      for (size_t i = index_forward(i_snd); true; 
                           index_forward(i))
                        {
                          if (table[i].status == BUSY and cmp(table[i].key, key))
                            break;
                          ++count;
                        }
                    }
                }
              max_len = std::max(max_len, count);				
              update_stat_len(lens, count);				
              break;							
            }								
          case EMPTY:							
            ++num_empty;							
            update_stat_len(lens, 0);					
            break;								
          case DELETED:							
            ++num_deleted;							
            break;								
          }								
 									
      float avg = 0, sum = 0;						
      for (size_t i = 0; i < lens.size(); ++i)				
        {									
          avg += lens(i)*i;						
          sum += lens(i);							
        }									
									
      avg /= sum;								
      float var = 0;							
      for (size_t i = 0; i < lens.size(); ++i)				
        {									
          float s = i - avg;						
          var += lens(i)*s*s;						
        }									
      var /= sum;								
   									
      Stats stats;								
      stats.num_busy = num_busy;						
      stats.num_deleted = num_deleted;					
      stats.num_empty = num_empty;						
      std::swap(lens, stats.lens);						
      stats.avg = avg;							
      stats.var = var;							
      stats.max_len = max_len;

      return stats;
    }
  };


  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  using SetODhash = ODhashTable<Key, Cmp>;


# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# undef EMPTY
# undef BUSY
# undef DELETED
# undef NO_PROBED    
# undef FIRST_PROBE  
# undef SECOND_PROBE 
# undef LINEAR_PROBE 

} // end namespace Aleph

# endif // TPL_ODHASH_H

