
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

# ifndef TPL_DYNLHASH_H
# define TPL_DYNLHASH_H

# include <tpl_lhash.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

   /** Key dynamic mapping implemented through a hash table with
       Collisions resolution for separate chain.

       The Dynlhashable <key type, record> implements a mapping through
       A hash table with collision resolution by chain
       separate. The export type observers to know the radius of
       Use and, eventually, readjust the size of the table, so
       that it exhibits better performance.

       They can be used safely to records within the
       board.

       Apart from the traditional methods of insertion and search, the
       Operator [] FUNGES AT THE TIME AS A CONSULTATION AND INSERTION. The
       Table assignment [key] = record corresponds to an insertion;
       While any expression of the Table Type [Key] that
       Involve reading corresponds to a search.

       @Param Key the type of indexing key
       @Param Record the type of range associated with the keys.
       @param CMP Comparison class between the keys.

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
      ah_invalid_argument_if(bucket == nullptr)
        << "access to unexisting entry";

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
      ah_invalid_argument_if(proxy.bucket == nullptr)
        << "access to unexisting entry";

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

    /// The type of hash function.
  using Hash_Fct_Ptr = typename DynLhashTable<Key, Record>::Hash_Fct_Ptr;

  void swap(DynLhashTable & table)
  {
    this->LhashTable<Key>::swap(table);
  }

    /// Instance a dynamic hash mapping with hash hash_fct and
    /// Len length table. Exception Bad_alloc shoots if there is no
    /// Memory to separate the internal table.
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
        DLBucket * Bucket = (DLBucket*) it.get_curr();
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

    /// Insert in the hash the pair (Key, Record) indisted by the key
    /// Key. Returns a pointer to the registration within the table. Try the
    /// Exception Bad_alloc if there is not enough memory.
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

    /// Look for the key key and, if you are, then a pointer returns
    /// Within the table to the associated record. Otherwise -I don't know
    /// Find the key-, Nullptr is returned:
  Record * search(const Key & key)
  {
    DLBucket * bucket = (DLBucket*) LhashTable<Key>::search(key);
    return bucket != nullptr ? &bucket->record : nullptr;
  }

    /// Eliminate the record record from the table (which must have been
    /// obtained by insertion or search.Dyb
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

