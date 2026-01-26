
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_dynLhash.H
 *  @brief Dynamic hash table mapping keys to records with separate chaining.
 *
 *  This header provides DynLhashTable, a hash table implementation that maps
 *  keys to records using separate chaining for collision resolution. It supports
 *  the subscript operator `[]` for convenient access and modification.
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNLHASH_H
# define TPL_DYNLHASH_H

# include <tpl_lhash.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph
{
  /** @class DynLhashTable
   *  @brief Dynamic hash table mapping keys to records with separate chaining.
   *
   *  DynLhashTable implements a key-value mapping using a hash table with
   *  separate chaining for collision resolution. It provides O(1) average-case
   *  complexity for insertions, lookups, and deletions.
   *
   *  The table supports both traditional methods (`insert`, `search`, `remove`)
   *  and the subscript operator `[]` for convenient access:
   *  - `table[key] = record` inserts or updates the entry
   *  - `record = table[key]` retrieves the value (throws if key not found)
   *
   *  Pointers returned by `insert()` and `search()` remain valid until the
   *  corresponding entry is removed, allowing direct modification of stored records.
   *
   *  @tparam Key Type of the indexing key (must be hashable).
   *  @tparam Record Type of the value associated with each key.
   *  @tparam Cmp Equality comparison functor for keys (default: Aleph::equal_to<Key>).
   *
   *  @par Example
   *  @code
   *  DynLhashTable<std::string, int> ages;
   *  ages.insert("Alice", 30);
   *  ages["Bob"] = 25;
   *  
   *  int* ptr = ages.search("Alice");
   *  if (ptr) std::cout << *ptr;  // prints 30
   *  
   *  int bob_age = ages["Bob"];   // bob_age = 25
   *  @endcode
   *
   *  @see LhashTableVtl for the underlying hash table implementation
   *  @ingroup Hash
   */
  template <typename Key, typename Record, class Cmp = Aleph::equal_to<Key>>
  class DynLhashTable : public LhashTableVtl<Key>
  {
  private:
    struct DLBucket : public LhashTableVtl<Key>::Bucket
    {
      Record record;

      DLBucket(const Key & key, const Record & _record)
        : LhashTableVtl<Key>::Bucket(key), record(_record)
      { /* Empty */
      }

      DLBucket(const Key & key, Record && _record)
        : LhashTableVtl<Key>::Bucket(key), record(std::forward<Record>(_record))
      { /* Empty */
      }

      DLBucket(Key && key, const Record & _record)
        : LhashTableVtl<Key>::Bucket(std::forward<Key>(key)), record(_record)
      { /* Empty */
      }

      DLBucket(Key && key, Record && _record)
        : LhashTableVtl<Key>::Bucket(std::forward<Key>(key)),
          record(std::forward<Record>(_record))
      { /* Empty */
      }
    };

    static DLBucket * record_to_bucket(Record *rec)
    {
      DLBucket *ret_val = 0;
      size_t offset = (size_t) &ret_val->record;
      return (DLBucket *) (((size_t) rec) - offset);
    }

    class DLProxy
    {
      DynLhashTable & table;
      const Key & key;
      DLBucket *bucket;

    public:
      DLProxy(DynLhashTable & _table, const Key & _key)
        : table(_table), key(_key)
      {
        Record *record = table.search(key);
        bucket = record not_eq nullptr ? record_to_bucket(record) : nullptr;
      }

      operator const Record &() const
      {
        ah_invalid_argument_if(bucket == nullptr)
        << "access to unexisting entry";

        return bucket->record;
      }

      DLProxy &operator =(const Record & record)
      {
        if (bucket != nullptr)
          {
            bucket->record = record;
            return *this;
          }

        bucket = new DLBucket(key, record);
        table.LhashTableVtl<Key>::insert(bucket);
        return *this;
      }

      DLProxy &operator =(const DLProxy & proxy)
      {
        ah_invalid_argument_if(proxy.bucket == nullptr)
        << "access to unexisting entry";

        if (bucket != nullptr)
          {
            bucket->record = proxy.bucket->record;
            return *this;
          }

        bucket = new DLBucket(key, proxy.bucket->record);
        table.LhashTableVtl<Key>::insert(bucket);
        return *this;
      }
    };

  public:
    /// The type of hash function pointer.
    using Hash_Fct_Ptr = typename DynLhashTable<Key, Record>::Hash_Fct_Ptr;

    /** @brief Swap contents with another table.
     *  @param[in,out] table The table to swap with.
     */
    void swap(DynLhashTable & table) noexcept
    {
      this->LhashTableVtl<Key>::swap(table);
    }

    /** @brief Construct a dynamic hash table.
     *
     *  @param[in] len Initial size of the hash table (default: DefaultPrime).
     *  @param[in] hash_fct Hash function to use (default: dft_hash_fct<Key>).
     *
     *  @throw std::bad_alloc If memory allocation fails.
     */
    DynLhashTable(size_t len = DefaultPrime,
                  Hash_Fct_Ptr hash_fct = dft_hash_fct<Key>)
      : LhashTableVtl<Key>(len, hash_fct)
    {
      // Empty
    }

  private:
    void copy(const DynLhashTable & table)
    {
      for (typename LhashTableVtl<Key>::Iterator it(const_cast<DynLhashTable &>(table));
           it.has_curr(); it.next_ne())
        {
          auto *Bucket = static_cast<DLBucket *>(it.get_curr());
          insert(Bucket->get_key(), Bucket->record);
        }
    }

  public:
    /** @brief Copy constructor.
     *  @param[in] table The table to copy from.
     *  @throw std::bad_alloc If memory allocation fails.
     */
    DynLhashTable(const DynLhashTable & table)
      : LhashTableVtl<Key>()
    {
      copy(table);
    }

    /** @brief Move constructor.
     *  @param[in,out] table The table to move from.
     *  @warning After move, the source table is in a moved-from state and
     *           must not be used except for destruction or assignment.
     */
    DynLhashTable(DynLhashTable && table) noexcept
      : LhashTableVtl<Key>(std::move(table))
    {
      // Base class move constructor properly transfers ownership
    }

    /** @brief Copy assignment operator.
     *  @param[in] table The table to copy from.
     *  @return Reference to this table.
     *  @throw std::bad_alloc If memory allocation fails.
     */
    DynLhashTable &operator =(const DynLhashTable & table)
    {
      if (this == &table)
        return *this;

      this->empty();
      copy(table);

      return *this;
    }

    /** @brief Move assignment operator.
     *  @param[in,out] table The table to move from.
     *  @return Reference to this table.
     *  @warning After move, the source table is in a moved-from state and
     *           must not be used except for destruction or assignment.
     */
    DynLhashTable &operator =(DynLhashTable && table) noexcept
    {
      if (this != &table)
        LhashTableVtl<Key>::operator=(std::move(table));
      return *this;
    }

  private:
    Record * __insert(DLBucket *bucket)
    {
      LhashTableVtl<Key>::insert(bucket);
      return &bucket->record;
    }

  public:
    /** @brief Insert a key-record pair into the table.
     *
     *  If the key already exists, a new entry is added (duplicates allowed).
     *  The returned pointer remains valid until the entry is removed.
     *
     *  @param[in] key The key to insert.
     *  @param[in] record The record to associate with the key.
     *  @return Pointer to the stored record within the table.
     *  @throw std::bad_alloc If memory allocation fails.
     */
    Record * insert(const Key & key, const Record & record)
    {
      return __insert(new DLBucket(key, record));
    }

    /// @overload
    Record * insert(const Key & key, Record && record = Record())
    {
      return __insert(new DLBucket(key, std::forward<Record>(record)));
    }

    /// @overload
    Record * insert(Key && key, const Record & record)
    {
      return __insert(new DLBucket(std::forward<Key>(key), record));
    }

    /// @overload
    Record * insert(Key && key, Record && record)
    {
      return __insert(new DLBucket(std::forward<Key>(key),
                                   std::forward<Record>(record)));
    }

    /** @brief Search for a key in the table.
     *
     *  @param[in] key The key to search for.
     *  @return Pointer to the associated record if found, nullptr otherwise.
     *
     *  @note The returned pointer can be used to modify the record in-place.
     */
    Record * search(const Key & key)
    {
      auto *bucket = static_cast<DLBucket *>(LhashTableVtl<Key>::search(key));
      return bucket != nullptr ? &bucket->record : nullptr;
    }

    /** @brief Remove an entry from the table.
     *
     *  The record pointer must have been obtained from insert() or search().
     *  After removal, the pointer becomes invalid.
     *
     *  @param[in] record Pointer to the record to remove (must be valid).
     *
     *  @warning Passing an invalid pointer results in undefined behavior.
     */
    void remove(Record *record)
    {
      DLBucket *bucket = record_to_bucket(record);
      LhashTableVtl<Key>::remove(bucket);
      delete bucket;
    }

    /** @brief Access or insert an entry using subscript notation.
     *
     *  - For assignment (`table[key] = value`): inserts or updates the entry.
     *  - For reading (`value = table[key]`): returns the value or throws.
     *
     *  @param[in] key The key to access.
     *  @return A proxy object that handles both read and write operations.
     *  @throw std::invalid_argument If reading a non-existent key.
     */
    DLProxy operator [](const Key & key) const
    {
      return DLProxy(const_cast<DynLhashTable &>(*this), key);
    }

    /// @overload
    DLProxy operator [](const Key & key)
    {
      return DLProxy(*this, key);
    }
  };
} // end namespace Aleph

# endif // TPL_DYNLHASH_H
