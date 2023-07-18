
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

# ifndef TPL_OLHASH_H
# define TPL_OLHASH_H

# include <iostream>
# include <primes.H>
# include <dlink.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hashDry.H>
# include <hash-fct.H>


using namespace Primes;

using namespace Aleph;

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif

namespace Aleph {

  /** Tabla hash con resolución de colisiones por direccionamiento
      abierto y sondeo lineal de cubetas disponibles.

      Este tipo instrumenta una tabla hash cerrada (el arreglo es
      contiguo en memoria), que guarda las colisiones dentro de la misma
      tabla. Cuando hay una colisión, se sondea linealmente la primera
      cubeta a partir del índice dado por la función hash.

      OLhashTable maneja tres parámetros tipo:
      -# Key: el tipo de dato que guarda la tabla.
      -# Record: el tipo de dato a mapear por la clave.
      -# Cmp: la clase de comparación entre claves. retorna true cuando
      las claves son iguales.

      @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  class OLhashTable
    : public OhashCommon<OLhashTable<Key, Cmp>, Key>,
      public GenericTraverse<OLhashTable<Key, Cmp>>,
      public LocateFunctions<OLhashTable<Key, Cmp>, Key>,
      public FunctionalMethods<OLhashTable<Key, Cmp>, Key>,
      public EqualToMethod<OLhashTable<Key, Cmp>>,
      public StlAlephIterator<OLhashTable<Key, Cmp>>
  {
    friend class OhashCommon<OLhashTable<Key, Cmp>, Key>;

  public:

    using Key_Type = Key;

    using Item_Type = Key;

    using Hash_Fct = std::function<size_t(const Key &)>;

    using Hash_Fct_Ptr = size_t (*) (const Key&);

    enum Status { EMPTY, BUSY, DELETED };

    struct Bucket
    {
      Key key;
      char status = EMPTY;

      Bucket() noexcept : status(EMPTY) {}
      void reset() noexcept { status = EMPTY; }
    };

    static Bucket * key_to_bucket(Key * rec) noexcept
    {
      Bucket * ret_val = 0;
      long offset = (long) &ret_val->key;
      return (Bucket*) ((long) rec - offset);
    }

    Bucket *    table = nullptr;
    size_t      N = 0;

  protected:

    size_t len;
    float  lower_alpha;
    float  upper_alpha;
    Cmp    cmp;

  private:

    Hash_Fct hash_fct;
    bool     with_resize;

    bool is_valid_bucket(Bucket * bucket) const noexcept
    {
      if (bucket < &table[0] or bucket >= &table[len])
        return false;

      int offset_with_base = (char*) bucket - (char*) &table[0];

      return offset_with_base % sizeof(*bucket) == 0;
    }

  public:

    const Cmp & get_compare() const { return cmp; }

    Cmp & get_compare() { return cmp; }

  protected:

    /// Instancia una tabla hash con función hash __hash_fct y
    /// dimensión len.
    OLhashTable(size_t __len, Hash_Fct __hash_fct, Cmp __cmp,
                float __lower_alpha, float __upper_alpha, bool __with_resize)
      : table(nullptr), N(0), len(Primes::next_prime(__len)),
        lower_alpha(__lower_alpha), upper_alpha(__upper_alpha), cmp(__cmp),
        hash_fct(__hash_fct), with_resize(__with_resize)
    {
      table = new Bucket [len];
    }

    OLhashTable(size_t len, Hash_Fct hash_fct, Hash_Fct, Cmp cmp,
                float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, hash_fct, cmp, lower_alpha, upper_alpha, with_resize) {}

  public:

    OLhashTable(size_t len            = Primes::DefaultPrime,
                Hash_Fct_Ptr hash_fct = Aleph::dft_hash_fct<Key>,
                Cmp cmp               = Cmp(),
                float lower_alpha     = hash_default_lower_alpha,
                float upper_alpha     = hash_default_upper_alpha,
                bool with_resize      = true)
      : OLhashTable(len, Hash_Fct(hash_fct), cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    OLhashTable(size_t len, Hash_Fct_Ptr hash_fct, Hash_Fct_Ptr,
                Cmp cmp, float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, Hash_fct(hash_fct), cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    /** Constructor with two hash function for metaprogramming
        compatibility with ODhashTable
    */
    OLhashTable(size_t len, Hash_Fct hash_fct, Hash_Fct_Ptr, Cmp cmp,
                float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, hash_fct, cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    Special_Ctors(OLhashTable, Key);

    /// Libera toda la memoria ocupada.
    ~OLhashTable()
    {
      if (table != nullptr)
        delete [] table;
    }

    void swap(OLhashTable & other) noexcept
    {
      std::swap(table, other.table);
      std::swap(N, other.N);
      std::swap(len, other.len);
      std::swap(cmp, other.cmp);
      std::swap(hash_fct, other.hash_fct);
      std::swap(lower_alpha, other.lower_alpha);
      std::swap(upper_alpha, other.upper_alpha);
      std::swap(with_resize, other.with_resize);
    }

    OLhashTable(const OLhashTable & other)
      : OLhashTable(other.len, other.hash_fct, other.cmp,
                    other.lower_alpha, other.upper_alpha, other.with_resize)
    {
      this->copy_from_table(other);
    }

    OLhashTable(OLhashTable && other) noexcept : OLhashTable(other)
    {
      swap(other);
    }

    OLhashTable & operator = (const OLhashTable & other)
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
          hash_fct = other.hash_fct;
          cmp = other.cmp;
          lower_alpha = other.lower_alpha;
          upper_alpha = other.upper_alpha;
        }

      this->copy_from_table(other);

      return *this;
    }

    OLhashTable & operator = (OLhashTable && other) noexcept
    {
      swap(other);
      return *this;
    }

    /// Busca la clave key y retorna el registro asociado si key se
    /// encuentra dentro de la tabla; de lo contrario, retorna nullptr.
    Key * search(const Key & key) const noexcept
    {
      long i = hash_fct(key) % len, c = 0;
      while (c < len and table[i].status != EMPTY)
        {
          if (table[i].status == BUSY and cmp(table[i].key, key))
            return &table[i].key;

          ++c;
          if (++i == len)
            i = 0;
        }

      return nullptr; // No se encuentra la clave
    }

  protected:

    Bucket * allocate_bucket(const Key & key) noexcept
    {
      assert(N < len);

      size_t i = hash_fct(key) % len;

      while (table[i].status == BUSY) // find a slot that is not busy
        {
          if (cmp(key, table[i].key))
            return nullptr;

          if (++i == len)
            i = 0;
        }

      Bucket * bucket = &table[i];

      // i contiene celda con DELETED o EMPTY ==> ocuparla
      bucket->status = BUSY;
      N++;

      return bucket;
    }

    // Allocate a new bucket for storing key. It always returns a
    // pointer to the bucket entry into the table. If the key is
    // already inserted in the table, then second value is true,
    // otherwise, not allocation is done and false is returned as second
    tuple<Bucket*, bool> hard_allocate_bucket(const Key & key) noexcept
    {
      assert(N < len);

      size_t i = hash_fct(key) % len;
      while (table[i].status == BUSY) // find a slot that is not busy
        {
          if (cmp(key, table[i].key))
            return make_tuple(&table[i], true);

          if (++i == len)
            i = 0;
        }

      Bucket * bucket = &table[i];

      // i contiene celda con DELETED o EMPTY ==> ocuparla
      bucket->status = BUSY;
      N++;

      return make_tuple(bucket, false);
    }

    /// Elimina de la tabla el registro apuntado por record. Obviamente,
    /// record debe apuntar a una entrada arrojada por insert() o
    /// search(). Dispara excepciones invalid_argument si record no es una
    /// dirección dentro del rango de la tabla o domain_error si la cubeta
    /// del registro no está ocupada.
    void deallocate_bucket(Bucket * bucket)
    {
      if (not is_valid_bucket(bucket))
        throw std::invalid_argument("record address is not inside table's range");

      if (bucket->status != BUSY)
        throw std::domain_error("Bucket containing record is not busy");

      --N;
      int i = bucket - &table[0]; // índice de brecha
      table[i].status = DELETED;
    }

  public:

    /** Remove the key referenced by key. key must be a valid refernce
        to the key previously inserted or retrieved in the table. If
        the key is not in the table, then a domain_error exception is
        thrown. */
    void remove(const Key & key)
    {
      Key * key_ptr = search(key);
      if (key_ptr == nullptr)
        throw std::domain_error("Key not in hash table");

      this->remove_ptr(key_ptr);
    }

    OHASH_COMMON(OLhashTable);

    using Stats = typename OhashCommon<OLhashTable<Key, Cmp>, Key>::Stats;

    Stats stats() const
    {
      DynArray<size_t> lens;
      size_t num_busy = 0, num_deleted = 0, num_empty = 0;
      size_t max_len = std::numeric_limits<size_t>::min();
      for (int i = 0; i < len; ++i)
        switch (table[i].status)
          {
          case BUSY:
            {
              ++num_busy;
              const Key & key = table[i].key;
              long i = hash_fct(key) % len;
              size_t count = 1;

              while (true)
                {
                  if (table[i].status == BUSY and cmp(table[i].key, key))
                    break;
                  ++count;
                  if (++i == len)
                    i = 0;
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
      for (int i = 0; i < lens.size(); ++i)				
        {									
          avg += lens(i)*i;						
          sum += lens(i);							
        }									
									
      avg /= sum;								
      float var = 0;							
      for (int i = 0; i < lens.size(); ++i)				
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
  using SetOLhash = OLhashTable<Key, Cmp>;

								
}
# endif // TPL_OLHASH_H

