
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

# ifndef TPL_DYNLHASH_H
# define TPL_DYNLHASH_H

# include <tpl_lhash.H>

using namespace Aleph;

namespace Aleph {

   /** Mapeo dinámico clave-rango instrumentado mediante una tabla hash con
       resolución de colisiones por encadenamiento separado.

       El tipo DynLhashTable<Key,Record> instrumenta un mapeo mediante
       una tabla hash con resolución de colisiones por encadenamiento
       separado. El tipo exporta observadores para conocer el radio de
       uso y, eventualmente, reajustar el tamaño de la tabla, de modo
       que ésta exhiba un mejor desempeño.

       Pueden usarse de manera segura punteros a registros dentro de la
       tabla.

       Aparte los métodos tradicionales de inserción y búsqueda, el
       operador [] funge a la vez como consulta e inserción. La
       asignación tabla[key]=record se corresponde con una inserción;
       mientras que cualquier expresión del tipo table[key] que
       involucre lectura se corresponde con una búsqueda.

       @param Key el tipo de clave de indización
       @param Record el tipo de rango asociado a las claves.
       @param Cmp clase de comparación entre las claves.

       @ingroup Hash
    */
  template <typename Key, typename Record, class Cmp = Aleph::equal_to<Key>>
class DynLhashTable : public LhashTable<Key>
{
private:

  struct DLBucket : public LhashTable<Key>::Bucket
  {
    Record record;

    DLBucket(const Key & key, const Record & _record)
      : LhashTable<Key>::Bucket(key), record(_record) { /* Empty */ }

    DLBucket(const Key & key, Record && _record)
      : LhashTable<Key>::Bucket(key), record(std::forward<Record>(_record))
    { /* Empty */ }

    DLBucket(Key && key, const Record & _record)
      : LhashTable<Key>::Bucket(std::forward<Key>(key)), record(_record)
    { /* Empty */ }

    DLBucket(Key && key, Record && _record)
      : LhashTable<Key>::Bucket(std::forward<Key>(key)),
        record(std::forward<Record>(_record)) { /* Empty */ }
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

