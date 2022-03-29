

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
 }
  };

  static DLBucket * record_to_bucket(Record * rec)
  {
    DLBucket * ret_val = 0;
    size_t offset = (size_t) &ret_val->record;
    return (DLBucket*) (((size_t) rec) - offset);
  }

  class DLProxy
  {
    DynLhashTable & table;
    const Key &     key;
    DLBucket *      bucket;

  public:

    DLProxy(DynLhashTable & _table, const Key& _key)
      : table(_table), key(_key)
    {
      Record * record = table.search(key);
      bucket = record not_eq nullptr ? record_to_bucket(record) : nullptr;
    }

    operator const Record & () const
    {
      if (bucket == nullptr)
        throw std::invalid_argument("access to unexisting entry");

      return bucket->record;
    }

    DLProxy& operator = (const Record& record)
    {
      if (bucket != nullptr)
        {
          bucket->record = record;
          return *this;
        }

      bucket = new DLBucket (key, record);
      table.LhashTable<Key>::insert(bucket);
      return *this;
    }

    DLProxy& operator = (const DLProxy& proxy)
    {
      if (proxy.bucket == nullptr)
        throw std::invalid_argument("access to unexisting entry");

      if (bucket != nullptr)
        {
          bucket->record = proxy.bucket->record;
          return *this;
        }

      bucket = new DLBucket (key, proxy.bucket->record);
      table.LhashTable<Key>::insert(bucket);
      return *this;
    }
  };

public:

    /// El tipo de función hash.
  using Hash_Fct_Ptr = typename DynLhashTable<Key, Record>::Hash_Fct_Ptr;

  void swap(DynLhashTable & table)
  {
    this->LhashTable<Key>::swap(table);
  }

    /// Instancia un mapeo hash dinámico con función hash hash_fct y
    /// tabla de longitud len. Dispara excepción bad_alloc si no hay
    /// memoria para apartar la tabla interna.
  DynLhashTable(size_t len           = DefaultPrime,
                Hash_Fct_Ptr hash_fct = dft_hash_fct<Key>)
    : LhashTable<Key>(len, hash_fct)
  {
    // Empty
  }

private:

  void copy(DynLhashTable & table)
  {
    for (typename LhashTable<Key>::Iterator it(table); it.has_curr();
         it.next_ne())
      {
        DLBucket * Bucket = (DLBucket*) it.get_curr_ne();
        insert(Bucket->get_key(), Bucket->record);
      }
  }

public:

  DynLhashTable(const DynLhashTable & table)
    : LhashTable<Key>(table.hash_fct, table.len)
  {
    copy(table);
  }

  DynLhashTable(DynLhashTable && table)
  {
    swap(table);
  }

  DynLhashTable & operator = (const DynLhashTable & table)
  {
    if (this == &table)
      return *this;

    this->empty();
    copy(table);

    return *this;
  }

  DynLhashTable & operator = (DynLhashTable && table)
  {
    swap(table);
    return *this;
  }

private:

  Record * __insert(DLBucket * bucket)
  {
    LhashTable<Key>::insert(bucket);
    return &bucket->record;
  }

public:

    /// Inserta en el mapeo hash el par (key,record) indizado por la clave
    /// key. Retorna un puntero al registro dentro de la tabla. Dispara la
    /// excepción bad_alloc si no hay suficiente memoria.
  Record * insert(const Key & key, const Record & record)
  {
    return __insert(new DLBucket (key, record));
  }

  Record * insert(const Key & key, Record && record = Record())
  {
    return __insert(new DLBucket(key, std::forward<Record>(record)));
  }

  Record * insert(Key && key, const Record & record)
  {
    return __insert(new DLBucket (std::forward<Key>(key), record));
  }

  Record * insert(Key && key, Record && record)
  {
    return __insert(new DLBucket(std::forward<Key>(key),
                                 std::forward<Record>(record)));
  }

    /// Busca la clave key y, si se encuentra, entonces retorna un puntero
    /// dentro de la tabla al registro asociado. De lo contrario -no se
    /// encuentra la clave-, se retorna nullptr:
  Record * search(const Key & key)
  {
    DLBucket * bucket = (DLBucket*) LhashTable<Key>::search(key);
    return bucket != nullptr ? &bucket->record : nullptr;
  }

    /// Elimina de la tabla el registro record (que debe haber sido
    /// obtenido mediante la inserción o búsqueda.
  void remove(Record * record)
  {
    DLBucket* bucket = record_to_bucket(record);
    LhashTable<Key>::remove(bucket);
    delete bucket;
  }

  DLProxy operator [] (const Key& key) const
  {
    return DLProxy ( const_cast<DynLhashTable&>(*this), key);
  }

  DLProxy operator [] (const Key& key)
  {
    return DLProxy (*this, key);
  }
};

} // end namespace Aleph

# endif // TPL_DYNLHASH_H

