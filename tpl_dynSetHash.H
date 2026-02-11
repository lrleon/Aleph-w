
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


/** @file tpl_dynSetHash.H
 *  @brief Dynamic set implementations based on hash tables.
 *
 *  This file provides `DynSetHash` and related hash-based set implementations
 *  offering O(1) average-case insertion, deletion, and search. Multiple hash
 *  table variants are supported including linear probing and separate chaining.
 *
 *  @ingroup Sets
 *  @author Leandro Rabindranath León
 *  @ingroup Hash
 */

# ifndef TPL_DYNSETHASH_H
# define TPL_DYNSETHASH_H

# include <algorithm>
# include <typeinfo>
# include <ahDry.H>
# include <ahIterator.H>
# include <primes.H>
# include <htlist.H>
# include <tpl_dynArray.H>
# include <tpl_dynMapOhash.H>
# include <tpl_dynLhash.H>
# include <tpl_linHash.H>
# include <ah-concepts.H>


namespace Aleph
{
  /** Self-adjusting dynamic hash table

      DynHashTable<Key> implements a linear, dynamic hash table where the user
      does not need to worry about the table size; it is automatically adjusted
      according to load-factor thresholds defined at construction time.

      @param Key The domain type used to index the table.

      @see LinearHashTable
      @ingroup Hash
  */
  template <typename Key,
            template <typename, class> class HashTable = LhashTable,
            class Cmp = Aleph::equal_to<Key>>
    requires EqualityComparator<Cmp, Key>
  class DynHashTable
      : public HashTable<Key, Cmp>,
        public GenericTraverse<DynHashTable<Key, HashTable, Cmp>>,
        public LocateFunctions<DynHashTable<Key, HashTable, Cmp>, Key>,
        public FunctionalMethods<DynHashTable<Key, HashTable, Cmp>, Key>,
        public GenericKeys<DynHashTable<Key, HashTable, Cmp>, Key>,
        public EqualToMethod<DynHashTable<Key, HashTable, Cmp>>,
        public StlAlephIterator<DynHashTable<Key, HashTable, Cmp>>
  {
  protected:
    using Base = HashTable<Key, Cmp>;

    using Bucket = typename HashTable<Key, Cmp>::Bucket;

  public:
    /// Hash function type.
    using Hash_Fct = typename Base::Hash_Fct;

    using Hash_Fct_Ptr = typename Base::Hash_Fct_Ptr;

    using Key_Type = Key;

    using Item_Type = Key;

    /** Creates a dynamic linear hash table.

        @param[in] hash_fct Hash function.
        @param cmp Key comparison functor.
        @param[in] len Initial and minimum table size. The table will never
        shrink below this value.
        @param[in] lower_alpha Lower load threshold; the table shrinks when the
        load factor goes below this value.
        @param[in] upper_alpha Upper load threshold; the table grows when the
        load factor goes above this value.
    */
    DynHashTable(size_t len = Primes::DefaultPrime,
                 Hash_Fct_Ptr hash_fct = Aleph::dft_hash_fct<Key>,
                 Cmp cmp = Cmp(),
                 float lower_alpha = hash_default_lower_alpha,
                 float upper_alpha = hash_default_upper_alpha)
      : Base(len, hash_fct, cmp, lower_alpha, upper_alpha, true, true)
    {
      // empty
    }

    DynHashTable(size_t len, Hash_Fct hash_fct, Cmp cmp,
                 float lower_alpha, float upper_alpha)
      : Base(len, hash_fct, cmp, lower_alpha, upper_alpha, true, true)
    {
      // empty
    }

  private:
    void copy(const DynHashTable & other)
    {
      for (typename Base::Iterator it(other); it.has_curr(); it.next_ne())
        {
          auto *bucket = static_cast<Bucket *>(it.get_curr());
          insert(bucket->get_key());
        }
    }

  public:
    /** Copy constructor.

        Creates a copy with the same table size as the original. This preserves
        the performance characteristics and avoids the overhead of recalculating
        an optimal size.

        @note The copy will have the same table size (len) as the original, even
        if the original has been expanded and then shrunk through deletions. This
        may temporarily use more memory than strictly necessary, but the dynamic
        resizing will adjust the size automatically as elements are added or removed.

        @note The const_cast on get_compare() is necessary because the base
        class's get_compare() method is not marked const. This is a limitation
        of the base HashTable class.
    */
    DynHashTable(const DynHashTable & other)
      : Base(other.len, other.hash_fct,
             const_cast<DynHashTable &>(other).get_compare(),
             other.lower_alpha, other.upper_alpha, true, true)
    {
      copy(other);
    }

    DynHashTable(DynHashTable && other) noexcept
      : Base(other.len, other.hash_fct, other.get_compare(),
             other.lower_alpha, other.upper_alpha, true, true)
    {
      this->swap(other);
    }

    Special_Ctors(DynHashTable, Key);

    ~DynHashTable()
    {
      this->empty();
    }

    DynHashTable &operator =(const DynHashTable & other)
    {
      if (this == &other)
        return *this;

      this->empty();
      copy(other);

      return *this;
    }

    DynHashTable &operator =(DynHashTable && other) noexcept
    {
      if (this == &other)
        return *this;

      this->empty();
      this->swap(other);
      return *this;
    }

  protected:
    Key * insert_bucket(Bucket *bucket)
    {
      auto *ret_val = static_cast<Bucket *>(this->Base::insert(bucket));
      if (ret_val == nullptr) // is the key in the table?
        { // yes! ==> free bucket
          delete bucket;
          return nullptr;
        }

      return &ret_val->get_key();
    }

    std::pair<Key *, bool> search_or_insert_bucket(Bucket *bucket)
    {
      auto *ret_val = static_cast<Bucket *>(this->Base::search_or_insert(bucket));
      if (ret_val != bucket) // is the key in the table?
        { // yes! ==> free bucket
          delete bucket;
          return {&ret_val->get_key(), true};
        }

      return {&ret_val->get_key(), false};
    }

  public:
    /// Inserts key into the hash set. Returns a pointer to the stored key if
    /// it was inserted, or nullptr if the key was not inserted.
    Key * insert(const Key & key)
    {
      return insert_bucket(new Bucket(key));
    }

    Key * insert(Key && key)
    {
      return insert_bucket(new Bucket(std::forward<Key>(key)));
    }

    Key * search_or_insert(const Key & key)
    {
      return get<0>(search_or_insert_bucket(new Bucket(key)));
    }

    Key * search_or_insert(Key && key)
    {
      return get<0>(search_or_insert_bucket(new Bucket(std::forward<Key>(key))));
    }

    // Returns true if key is already in the table. Otherwise, inserts key and
    // returns false.
    std::pair<Key *, bool> contains_or_insert(const Key & key)
    {
      return search_or_insert_bucket(new Bucket(key));
    }

    std::pair<Key *, bool> contains_or_insert(Key && key)
    {
      return search_or_insert_bucket(new Bucket(std::forward<Key>(key)));
    }

    Key * add(const Key & key)
    {
      return insert_bucket(new Bucket(key));
    }

    Key * add(Key && key)
    {
      return insert_bucket(new Bucket(std::forward<Key>(key)));
    }

    Key * append(const Key & key)
    {
      return insert_bucket(new Bucket(key));
    }

    Key * append(Key && key)
    {
      return insert_bucket(new Bucket(std::forward<Key>(key)));
    }

    /** Searches for a key in the hash table.

        @param[in] key the key to search for
        @return pointer to the key if found, nullptr otherwise
        @note This method is noexcept if the hash function and comparison
        function do not throw exceptions.
    */
    Key * search(const Key & key) const noexcept
    {
      auto *bucket = static_cast<Bucket *>(this->Base::search(key));
      return bucket != nullptr ? &bucket->get_key() : nullptr;
    }

    /** Checks if a key exists in the hash table.

        @param[in] key the key to check
        @return true if the key exists, false otherwise
    */
    bool has(const Key & key) const noexcept
    {
      return this->Base::search(key) != nullptr;
    }

    /// Alias for has()
    bool contains(const Key & key) const noexcept { return has(key); }

    const Key &find(const Key & key) const
    {
      auto *bucket = static_cast<Bucket *>(this->Base::search(key));
      ah_domain_error_if(bucket == nullptr) << "Key not found in hash";

      return bucket->get_key();
    }

    Key &find(const Key & key)
    {
      auto *bucket = static_cast<Bucket *>(this->Base::search(key));

      ah_domain_error_if(bucket == nullptr) << "Key not found in hash";

      return bucket->get_key();
    }

  protected:
    static Bucket * key_to_bucket(Key *key)
    {
      Bucket *ret_val = 0;
      const auto offset = reinterpret_cast<size_t>(&ret_val->get_key());

      return reinterpret_cast<Bucket *>(reinterpret_cast<size_t>(key) - offset);
    }

  public:
    /** Removes a key from the hash table using its pointer.

        @warning The key pointer MUST be obtained from a previous call to
        insert() or search(). Passing any other pointer will result in
        undefined behavior and likely cause a segmentation fault.

        This method uses pointer arithmetic to recover the bucket containing
        the key, which only works if the pointer truly points to a key
        stored inside the hash table.

        @param[in] key pointer to the key to remove (must come from insert/search)

        @see remove(const Key &) for a safer alternative that searches by value
    */
    void remove(Key *key)
    {
      Bucket *bucket = key_to_bucket(key);
      this->Base::remove(bucket);
      delete bucket;
    }

    Key remove(const Key & key)
    {
      auto *bucket = static_cast<Bucket *>(this->Base::search(key));
      ah_domain_error_if(bucket == nullptr) << "Key not found in hash table";

      this->Base::remove(bucket);
      auto ret_val = bucket->get_key();
      delete bucket;
      return ret_val;
    }

    class Iterator : public Base::Iterator
    {
    public:
      using Item_Type = Key;

      using Set_Type = DynHashTable;

      /// Default constructor creates an "end" iterator
      Iterator() noexcept = default;

      Iterator(const DynHashTable & table) : Base::Iterator(table) {}

      Key &get_curr_ne() noexcept
      {
        return this->Base::Iterator::get_curr_ne()->get_key();
      }

      const Key &get_curr_ne() const noexcept
      {
        return const_cast<Iterator *>(this)->get_curr_ne();
      }

      const Key &get_curr() const
      {
        return const_cast<Iterator *>(this)->get_curr();
      }

      Key &get_curr()
      {
        return this->Base::Iterator::get_curr()->get_key();
      }

      void del() { delete this->Base::Iterator::del(); }
    };

    const Key &get_first() const
    {
      return this->get_it().get_curr();
    }

    Key &get_first()
    {
      return this->get_it().get_curr();
    }

    const Key &get_last() const
    {
      auto it = this->get_it();
      it.reset_last();
      return it.get_curr();
    }

    Key &get_last()
    {
      auto it = this->get_it();
      it.reset_last();
      return it.get_curr();
    }
  };

  /**

     @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
    requires EqualityComparator<Cmp, Key>
  struct DynSetLhash : public DynHashTable<Key, LhashTable, Cmp>
  {
    typedef DynHashTable<Key, LhashTable, Cmp> Base;
    using Base::Base;
  };

  /**

     @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
    requires EqualityComparator<Cmp, Key>
  struct DynSetLinHash : public DynHashTable<Key, LinearHashTable, Cmp>
  {
    typedef DynHashTable<Key, LinearHashTable, Cmp> Base;
    using Base::Base;
  };

  /**

     @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  using DynSetHash = DynHashTable<Key, LhashTable, Cmp>;

  /**

     @ingroup Hash
  */
  template <typename Key, typename Data,
            template <class, class> class HashTable = LhashTable,
            class Cmp = Aleph::equal_to<Key>>
    requires EqualityComparator<Cmp, Key>
  class DynMapHashTable
      : public DynHashTable<std::pair<Key, Data>,
                            HashTable, Dft_Pair_Cmp<Key, Data, Cmp>>
  {
    using Pair = std::pair<Key, Data>;

    using Base =
    DynHashTable<std::pair<Key, Data>, HashTable, Dft_Pair_Cmp<Key, Data, Cmp>>;

    using Bucket = typename Base::Bucket;

  public:
    using Hash_Fct = std::function<size_t(const Key &)>;
    using Hash_Fct_Ptr = size_t (*)(const Key &);

  private:
    // Store the original hash function that works on Key (not Pair)
    // This allows heterogeneous search without constructing Data()
    Hash_Fct_Ptr original_hash_fct;

    static constexpr bool has_search_with_custom_hash =
        requires(const DynMapHashTable *self, Hash_Fct_Ptr hf, const Key & k)
          {
            self->search_with_custom_hash(hf, k);
          };

  public:
    static Data &get_data(const Key & key)
    {
      return key_to_pair<Key, Data>(&const_cast<Key &>(key))->second;
    }

    static const Key &get_key(Data *data_ptr)
    {
      return data_to_pair<Key, Data>(data_ptr)->first;
    }

    using Value_Type = Data;

    // using Base::Base;  // no more need. But I don't remember why I put it
    using Base::insert; // in this way, insert with a pair is exported
    using Iterator = typename Base::Iterator;

    DynMapHashTable(size_t len = Primes::DefaultPrime,
                    Hash_Fct_Ptr hash_fct = dft_hash_fct,
                    Cmp cmp = Cmp(),
                    float lower_alpha = hash_default_lower_alpha,
                    float upper_alpha = hash_default_upper_alpha)
      : Base(len, std::bind(map_hash_fct<Key, Data, Hash_Fct>,
                            hash_fct, std::placeholders::_1),
             Dft_Pair_Cmp<Key, Data, Cmp>(cmp), lower_alpha, upper_alpha),
        original_hash_fct(hash_fct) {}

    /// Inserts into the hash map the pair (key, record) indexed by key.
    /// Returns a pointer to the pair stored inside the table.
    /// Throws bad_alloc if there is not enough memory.
    Pair * insert(const Key & key, const Data & data)
    {
      return this->insert_bucket(new typename Base::Bucket(Pair(key, data)));
    }

    Pair * insert(const Key & key, Data && data)
    {
      return this->insert_bucket
          (new typename Base::Bucket(Pair(key, std::forward<Data>(data))));
    }

    Pair * insert(Key && key, Data && data)
    {
      return this->insert_bucket
          (new typename Base::Bucket(Pair(std::forward<Key>(key), std::forward<Data>(data))));
    }

    Pair * insert(Key && key, const Data & data)
    {
      return this->insert_bucket
          (new typename Base::Bucket(Pair(std::forward<Key>(key), data)));
    }

    /** Searches for key and, if found, returns a pointer to the associated
        pair stored in the table. Otherwise (key not found), returns nullptr.

        @note This method uses heterogeneous search and does NOT require Data
        to have a default constructor. It searches by Key only.
    */
    Pair * search(const Key & key) const noexcept
    {
      if constexpr (has_search_with_custom_hash)
        {
          auto *bucket = static_cast<Bucket *>(
            this->search_with_custom_hash(original_hash_fct, key));
          return bucket != nullptr ? &bucket->get_key() : nullptr;
        }
      else
        return Base::search(Pair(key, Data()));
    }

    Pair * search(Key && key) const noexcept
    {
      if constexpr (has_search_with_custom_hash)
        {
          auto *bucket = static_cast<Bucket *>(
            this->search_with_custom_hash(original_hash_fct, key));
          return bucket != nullptr ? &bucket->get_key() : nullptr;
        }
      else
        return Base::search(Pair(std::forward<Key>(key), Data()));
    }

    /// Checks if a key exists in the map
    bool has(const Key & key) const noexcept { return search(key) != nullptr; }

    bool has(Key && key) const noexcept { return search(std::forward<Key>(key)) != nullptr; }

    /// Alias for has()
    bool contains(const Key & key) const noexcept { return has(key); }

    bool contains(Key && key) const noexcept { return has(std::forward<Key>(key)); }

    /** Finds and returns a reference to the value associated with key.

        @param[in] key the key to search for
        @return reference to the value associated with the key
        @throw std::domain_error if the key is not found

        @note This method does NOT require Data to have a default constructor.
    */
    Data &find(const Key & key)
    {
      auto *pair = search(key);
      ah_domain_error_if(pair == nullptr) << "Key not found in hash table";
      return pair->second;
    }

    const Data &find(const Key & key) const
    {
      auto *pair = search(key);
      ah_domain_error_if(pair == nullptr) << "Key not found in hash table";
      return pair->second;
    }

    /** Subscript operator for map access/insertion.

        @note This method REQUIRES Data to have a default constructor because
        it needs to insert a new element if the key doesn't exist.
        If Data is not default-constructible, use find() or search() instead.
    */
    Data &operator [](const Key & key)
    {
      return this->search_or_insert(Pair(key, Data()))->second;
    }

    const Data &operator [](const Key & key) const
    {
      return this->find(key);
    }

    Data &operator [](Key && key)
    {
      return this->search_or_insert(Pair(std::forward<Key>(key), Data()))->second;
    }

    const Data &operator [](Key && key) const
    {
      return this->find(std::forward<Key>(key));
    }

    /// Removes from the table the key whose pointer must be the result of a
    /// prior insertion or search.
    void remove_by_data(Data & data)
    {
      Base::remove(data_to_pair<Key, Data>(&data));
    }

    /** Removes a key-value pair from the map and returns the value.

        @param[in] key the key to remove
        @return the value associated with the removed key
        @throw std::domain_error if the key is not found

        @note Uses move semantics to return the value efficiently. If Data's
        move constructor is noexcept, this method provides strong exception safety.

        @note This method does NOT require Data to have a default constructor.
        It uses heterogeneous search to find the key.
    */
    Data remove(const Key & key)
    {
      auto *pair = search(key);
      ah_domain_error_if(pair == nullptr) << "Key not found in hash table";

      auto ret_val = std::move(pair->second);
      auto *bucket = Base::key_to_bucket(pair);
      this->Base::Base::remove(bucket);
      delete bucket;
      return ret_val;
    }

    Data remove(Key && key)
    {
      return remove(key); // Forward to const& version
    }

    DynList<Key> keys() const
    {
      return this->template maps<Key>([](auto p) { return p.first; });
    }

    DynList<Key> values() const
    {
      return this->template maps<Data>([](auto p) { return p.second; });
    }

    DynList<Data *> values_ptr()
    {
      DynList<Data *> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr().second);
      return ret;
    }

    DynList<Pair *> items_ptr()
    {
      DynList<Pair *> ret;
      for (Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(&it.get_curr());
      return ret;
    }
  };

  /**

     @ingroup Hash
  */
  template <typename Key, typename Data,
            class Cmp = Aleph::equal_to<Key>>
  using DynMapLinHash = DynMapHashTable<Key, Data, LinearHashTable, Cmp>;

  /**

     @ingroup Hash
  */
  template <typename Key, typename Data,
            class Cmp = Aleph::equal_to<Key>>
  using DynMapHash = DynMapHashTable<Key, Data, LhashTable, Cmp>;


  // Implementation coming from ahFunctional.H

  template <typename T, template <typename> class Container>
  inline
  DynList<T> join(const Container<T> & c1, const Container<T> & c2)
  {
    DynSetLhash<T> table(c1);
    c2.for_each([&table](const T & item)
                  {
                    table.insert(item);
                  });
    return table.keys();
  }

  template <typename T, template <typename> class Container = DynList>
  inline
  DynList<T> intercept(const Container<T> & c1, const Container<T> & c2)
  {
    DynSetLhash<T> set1 = c1;
    DynSetLhash<T> set2 = c2;
    return set1.filter([&set2](const T & i) { return set2.contains(i); });
  }

  template <typename T, template <typename> class Container>
  inline
  DynList<T> unique(const Container<T> & c)
  {
    return DynSetLhash<T>(c).keys();
  }

  template <typename T, template <typename> class Container>
  inline
  DynList<T> repeated(const Container<T> & c)
  {
    DynList<T> ret;
    DynSetLhash<T> table;

    c.for_each([&table, &ret](const T & i)
                 {
                   auto *ptr = table.insert(i);
                   if (ptr == nullptr)
                     ret.append(i);
                 });

    return ret;
  }

  template <typename T, template <typename> class Container>
  inline
  DynList<std::pair<T, size_t>> repeated_with_index(const Container<T> & c)
  {
    DynList<std::pair<T, size_t>> ret;
    DynSetLhash<T> table;

    size_t i = 0;
    c.for_each([&table, &ret, &i](const T & item)
                 {
                   auto *ptr = table.insert(item);
                   if (ptr == nullptr)
                     ret.append(std::pair<T, size_t>(item, i));
                   ++i;
                 });

    return ret;
  }
} // end namespace Aleph

# endif // TPL_DYNSETHASH_H
