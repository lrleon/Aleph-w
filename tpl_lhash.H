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

# ifndef TPL_LHASH_H
# define TPL_LHASH_H

# include <iostream>
# include <primes.H>
# include <dlink.H>
# include <htlist.H>
# include <tpl_dynArray.H>
# include <hashDry.H>
# include <hash-fct.H>
# include <tpl_dynArray.H>
# include <hash-dry.H>
# include <ah-errors.H>

# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif

using namespace Primes;

using namespace Aleph;

namespace Aleph
{
  /** Generic hash table with collision resolution by separate chaining.

      The GenLhashTable type implements a generic hash table with
      collision resolution by separate chaining, where the bucket type is a
      type parameter.

      Normally, this is intended as the back-end of the LhashTable and
      LhashTableVtl types, which are fundamentally the same, except that they
      already define their bucket classes without or with a virtual destructor.

      @param Key indexing key type of the table.
      @param BucketType bucket type.
      @param Cmp key comparison class.

      @see LhashTable LhashTableVtl
      @ingroup Hash
  */
  template <typename Key, class BucketType, class Cmp>
  class GenLhashTable : public HashStats<GenLhashTable<Key, BucketType, Cmp>>
  {
    friend class HashStats<GenLhashTable<Key, BucketType, Cmp>>;

  public:
    using Bucket = BucketType;

    using Hash_Fct = std::function<size_t(const Key &)>;

    using Hash_Fct_Ptr = size_t (*)(const Key &);

    using Key_Type = Key;

    using Item_Type = Key;

  protected:
    Hash_Fct hash_fct;

  private:
    using BucketList = Dnode<Key>; // Bucket list type
    using BucketItor = typename Dnode<Key>::Iterator; // Bucket iterator
    using Node = Dnode<Key>; // Node alias

    BucketList *table;

  protected:
    size_t len; // Table size
    Cmp cmp;
    float lower_alpha;
    float upper_alpha;

  private:
    size_t N; // Number of elements in the table
    size_t busy_slots_counter; // Number of occupied entries
    bool remove_all_buckets; // free buckets in destructor
    bool with_resize;

  public:
    Cmp &get_compare() { return cmp; }

    const Cmp &get_compare() const { return cmp; }

  protected:
    GenLhashTable(const size_t table_size, Hash_Fct hash_f, Cmp cpm_fct,
                  const float lower, const float upper,
                  const bool remove_all, const bool resize)
      : hash_fct(hash_f), table(nullptr),
        len(Primes::next_prime(table_size)), cmp(cpm_fct),
        lower_alpha(lower), upper_alpha(upper),
        N(0), busy_slots_counter(0), remove_all_buckets(remove_all),
        with_resize(resize)
    {
      table = new BucketList [len];
    }

  public:
    /** Instantiate a generic hash table.

        @param[in] hash_fct hash function.
        @param cmp key comparison functor.
        @param[in] lower_alpha lower load factor.
        @param[in] upper_alpha upper load factor.
        @param[in] table_size table size.
        @param[in] remove_all_buckets if true, then all buckets are freed when
        the destructor is invoked.
        @param with_resize if true, the table is resized when the load factor
        exceeds upper_alpha.
        @throw bad_alloc if there is not enough memory to allocate the table.
    */
    GenLhashTable(size_t table_size = Primes::DefaultPrime,
                  Hash_Fct_Ptr hash_fct = Aleph::dft_hash_fct<Key>,
                  Cmp cmp = Cmp(),
                  float lower_alpha = hash_default_lower_alpha,
                  float upper_alpha = hash_default_upper_alpha,
                  bool remove_all_buckets = true,
                  bool with_resize = true)
      : GenLhashTable(table_size, Hash_Fct(hash_fct), cmp,
                      lower_alpha, upper_alpha, remove_all_buckets, with_resize)
    {}

    void swap(GenLhashTable & other) noexcept
    {
      std::swap(hash_fct, other.hash_fct);
      std::swap(table, other.table);
      std::swap(len, other.len);
      std::swap(cmp, other.cmp);
      std::swap(N, other.N);
      std::swap(busy_slots_counter, other.busy_slots_counter);
      std::swap(remove_all_buckets, other.remove_all_buckets);
      std::swap(with_resize, other.with_resize);
    }

    GenLhashTable(const GenLhashTable &) = delete;

    GenLhashTable &operator=(const GenLhashTable &) = delete;

    GenLhashTable(GenLhashTable && other) noexcept
      : hash_fct(std::move(other.hash_fct)),
        table(other.table),
        len(other.len),
        cmp(std::move(other.cmp)),
        lower_alpha(other.lower_alpha),
        upper_alpha(other.upper_alpha),
        N(other.N),
        busy_slots_counter(other.busy_slots_counter),
        remove_all_buckets(other.remove_all_buckets),
        with_resize(other.with_resize)
    {
      other.table = nullptr;
      other.len = 0;
      other.N = 0;
      other.busy_slots_counter = 0;
    }

    GenLhashTable &operator=(GenLhashTable && other) noexcept
    {
      if (this != &other)
        {
          if (remove_all_buckets && table != nullptr)
            empty();
          delete [] table;

          hash_fct = std::move(other.hash_fct);
          table = other.table;
          len = other.len;
          cmp = std::move(other.cmp);
          lower_alpha = other.lower_alpha;
          upper_alpha = other.upper_alpha;
          N = other.N;
          busy_slots_counter = other.busy_slots_counter;
          remove_all_buckets = other.remove_all_buckets;
          with_resize = other.with_resize;

          other.table = nullptr;
          other.len = 0;
          other.N = 0;
          other.busy_slots_counter = 0;
        }
      return *this;
    }

    /// Empties the hash table; frees memory of all buckets.
    void empty() noexcept
    {
      for (size_t i = 0; i < len; ++i)
        for (BucketItor itor(table[i]); itor.has_curr(); /* nothing */)
          delete static_cast<Bucket *>(itor.del_ne());
      busy_slots_counter = N = 0;
    }

  private:
    Bucket *
    search_in_bucket_list(BucketList & list, const Key & key) const
    {
      for (BucketItor it(list); it.has_curr(); it.next_ne())
          if (auto *bucket = static_cast<Bucket *>(it.get_curr()); cmp(key, bucket->get_key()))
            return bucket;

      return nullptr;
    }

  protected:
    /** Helper method for derived classes to perform custom heterogeneous searches.

        This method allows derived classes (such as DynMapHashTable) to search
        for elements using a different type than the stored key type, without
        needing to construct the full key object. This is particularly useful
        for map-like wrappers that store pairs but want to search by key only.

        @param hash_func Function to compute the hash of the search key
        @param search_key The key to search for (can be different type than Key)
        @return Pointer to the bucket if found, nullptr otherwise

        @note The comparator (cmp) must support comparison between the stored
        Key type and the SearchKey type for this to work. For example,
        Dft_Pair_Cmp supports comparing Pair with Key.

        Example usage in DynMapHashTable:
        @code
        Pair * search(const Key & key) const {
          return this->search_with_custom_hash(original_hash_fct, key);
        }
        @endcode
    */
    template <typename HashFunc, typename SearchKey>
    Bucket * search_with_custom_hash(HashFunc hash_func,
                                       const SearchKey & search_key) const noexcept
    {
      const auto bucket_idx = hash_func(search_key) % len;

      for (BucketItor it(table[bucket_idx]); it.has_curr(); it.next_ne())
        if (auto *bucket = static_cast<Bucket *>(it.get_curr());
            cmp(bucket->get_key(), search_key))
          return bucket;

      return nullptr;
    }

  public:
    Hash_Fct get_hash_fct() const noexcept { return hash_fct; }

    /// Set the internal hash function
    void set_hash_fct(Hash_Fct fct) noexcept
    {
      hash_fct = fct;
    }

    void set_hash_fct(Hash_Fct_Ptr fct) noexcept
    {
      hash_fct = Hash_Fct(fct);
    }

    /// return the current table load
    float current_alpha() const noexcept { return 1.0 * N / len; }

    /// Inserts bucket into the table and returns its address if the key is not
    /// already in the table; otherwise returns nullptr.
    Bucket * insert(Bucket *bucket)
    {
      const auto i = hash_fct(bucket->get_key()) % len;
      auto & list = table[i];

      if (list.is_empty()) // is table[i] list empty?
        ++busy_slots_counter; // yes ==> update occupied counter

      if (search_in_bucket_list(list, bucket->get_key()) != nullptr)
        return nullptr;

      list.append(bucket); // insert bucket at the end
      ++N;

      if (with_resize and current_alpha() >= upper_alpha)
        resize(next_prime(2 * len));

      return bucket;
    }

    // Search bucket->key. If found, returns the bucket address inside the table.
    // Otherwise, inserts and returns bucket.
    Bucket * search_or_insert(Bucket *bucket)
    {
      const auto i = hash_fct(bucket->get_key()) % len;
      auto & list = table[i];

      if (list.is_empty()) // is table[i] list empty?
        ++busy_slots_counter; // yes ==> update occupied counter

      auto *p = search_in_bucket_list(list, bucket->get_key());
      if (p != nullptr)
        return p;

      list.append(bucket); // insert bucket at the end
      ++N;

      if (with_resize and current_alpha() >= upper_alpha)
        resize(next_prime(2 * len));

      return bucket;
    }

    /// Search in the table for a bucket with key. Returns a pointer to the
    /// bucket if found; otherwise nullptr.
    Bucket * search(const Key & key) const noexcept
    {
      const auto i = hash_fct(key) % len;
      return search_in_bucket_list(table[i], key);
    }

  private:
    // Remove without perform resizing. This is strictly required inside
    // the del() method of Iterator. In addition, dries the code
    Bucket * remove_bucket(Bucket *bucket) noexcept
    {
      const auto idx = hash_fct(bucket->get_key()) % len;
      bucket->del(); // remove from its collision list

      if (table[idx].is_empty()) // did the list become empty?
        --busy_slots_counter; // yes ==> update occupied lists counter

      --N;

      return bucket;
    }

  public:
    /// Removes bucket from the table. Returns the bucket address.
    /// WARNING: it is not verified that the bucket belongs to the table.
    Bucket * remove(Bucket *bucket) noexcept
    { // save next collision
      remove_bucket(bucket);

      if (with_resize and current_alpha() < lower_alpha)
        resize(next_prime(len / 2));

      return bucket;
    }

    /// Resizes the hash table to new_size and re-locates keys.
    /// Throws bad_alloc if there is not enough memory to relocate the array.
    size_t resize(const size_t new_size)
    {
      assert(len > 0);

      if (new_size == 0)
        return len;

      auto *new_table = new BucketList [new_size];
      BucketList *old_table = table; // save current table state
      const size_t old_size = len;
      table = new_table; // empty table with new array
      len = new_size;
      busy_slots_counter = N = 0;

      for (size_t i = 0; i < old_size; ++i) // reinsert buckets
        // traverse collision list in old_table[i]
        for (BucketItor it(old_table[i]); it.has_curr(); /* Nothing */)
          insert(static_cast<Bucket *>(it.del_ne())); // remove and insert into table[]

      delete [] old_table; // free old table memory

      return len;
    }

    /// Frees the table memory and, if remove_all_buckets is true
    /// (specified in the constructor), also frees memory of all buckets.
    virtual ~GenLhashTable()
    {
      if (remove_all_buckets)
        empty();

      delete [] table;
    }

    /// Returns the next bucket that collides with bucket. If it does not exist,
    /// then nullptr is returned.
    Bucket * search_next(Bucket *bucket) const
    {
      assert(bucket != nullptr);

      const auto i = hash_fct(bucket->get_key()) % len;

      BucketItor itor(table[i]);
      itor.set(bucket);

      while (true)
        {
          itor.next_ne();

          if (not itor.has_curr())
            return nullptr;

          if (auto *node = static_cast<Bucket *>(itor.get_curr()); cmp(bucket->get_key(), node->get_key()))
            return node;
        }
    }

    /// Returns the table capacity.
    const size_t &capacity() const noexcept { return len; }

    /// Returns the number of elements contained in the table.
    const size_t &size() const noexcept { return N; }

    /// Returns the number of occupied entries in the array.
    const size_t &
    get_num_busy_slots() const noexcept { return busy_slots_counter; }

    [[nodiscard]] constexpr bool is_empty() const noexcept { return N == 0; }

    /** Iterator over a GenLhashTable hash table.

        @note The observation sequence produced by the iterator is not ordered.
        @ingroup Hash
    */
    class Iterator
    {
      long curr_index = -1; // index in table
      long curr_pos = 0;
      BucketItor curr_itor; // Iterator over table[curr_index]
      GenLhashTable *hash_table = nullptr;

      // Advance to the next available entry in table
      void locate_next_available_entry_ne() noexcept
      {
        while (true)
          {
            if (curr_index == hash_table->len - 1)
              { // remain in overflow state
                curr_index = hash_table->len;
                return;
              }

            ++curr_index;

            if (not hash_table->table[curr_index].is_empty())
              {
                BucketItor itor(hash_table->table[curr_index]);
                curr_itor = itor;
                return;
              }
          }
      }

      void locate_next_available_entry()
      {
        while (true)
          {
            ah_overflow_error_if(curr_index == hash_table->len)
              << "hash table iterator overflow";

            if (curr_index == hash_table->len - 1)
              { // remain in overflow state
                curr_index = hash_table->len;
                return;
              }

            ++curr_index;

            if (not hash_table->table[curr_index].is_empty())
              {
                BucketItor itor(hash_table->table[curr_index]);
                curr_itor = itor;
                return;
              }
          }
      }

      void locate_next_available_bucket_ne() noexcept
      {
        curr_itor.next_ne();
        if (not curr_itor.has_curr())
          locate_next_available_entry_ne();
        curr_pos++;
      }

      void locate_next_available_bucket()
      {
        curr_itor.next();
        if (not curr_itor.has_curr())
          locate_next_available_entry();
        curr_pos++;
      }

      void locate_prev_available_entry_ne() noexcept
      {
        while (true)
          {
            if (curr_index == 0)
              { // remain in underflow state
                curr_index = -1;
                return;
              }

            --curr_index;

            if (not hash_table->table[curr_index].is_empty())
              {
                BucketItor itor(hash_table->table[curr_index]);
                curr_itor = itor;
                curr_itor.reset_last();
                return;
              }
          }
      }

      void locate_prev_available_entry()
      {
        while (true)
          {
            ah_underflow_error_if(curr_index == -1)
              << "hash table iterator underflow";

            if (curr_index == 0)
              { // remain in underflow state
                curr_index = -1;
                return;
              }

            --curr_index;

            if (not hash_table->table[curr_index].is_empty())
              {
                BucketItor itor(hash_table->table[curr_index]);
                curr_itor = itor;
                curr_itor.reset_last();
                return;
              }
          }
      }

      void locate_prev_available_bucket_ne() noexcept
      {
        curr_itor.prev_ne();
        if (not curr_itor.has_curr())
          locate_prev_available_entry_ne();
        curr_pos--;
      }

      void locate_prev_available_bucket()
      {
        curr_itor.prev();
        if (not curr_itor.has_curr())
          locate_prev_available_entry();
        curr_pos--;
      }

    public:
      /// Set type over which iteration occurs.
      using Set_Type = GenLhashTable;

      /// Item type returned by get_curr().
      using Item_Type = Bucket *;

      /// Instantiate an iterator over the hash table.
      Iterator(const GenLhashTable & table) noexcept
        : hash_table(&const_cast<GenLhashTable &>(table))
      {
        try
          {
            locate_next_available_entry();
          }
        catch (const std::overflow_error &)
          { /* nothing to do */
          }
      }

      /// Instantiate an empty iterator.
      Iterator() = default;

      /// Reset the iterator to the first bucket.
      void reset_first() noexcept
      {
        curr_index = -1;
        curr_pos = 0;
        locate_next_available_entry();
      }

      /// Reset the iterator to the last bucket.
      void reset_last() noexcept
      {
        if (hash_table->is_empty())
          {
            curr_index = -1;
            curr_pos = -1;
            return;
          }
        curr_index = hash_table->len;
        curr_pos = static_cast<long>(hash_table->N) - 1;
        locate_prev_available_entry();
      }

      void end() noexcept
      {
        put_itor_at_the_end(*this);
      }

      /// Returns true if the iterator has a current bucket.
      bool has_curr() const noexcept
      {
        return curr_index >= 0 and curr_index < hash_table->len;
      }

      Bucket * get_curr_ne() noexcept
      {
        return static_cast<Bucket *>(curr_itor.get_curr_ne());
      }

      /// Returns the current bucket.
      Bucket * get_curr()
      {
        ah_underflow_error_if(curr_index == -1)
          << "hash table iterator underflow";

        ah_overflow_error_if(curr_index == hash_table->len)
          << "hash table iterator overflow";

        return static_cast<Bucket *>(curr_itor.get_curr());
      }

      long get_pos() const noexcept { return curr_pos; }

      /// Advances the iterator by one bucket.
      void next_ne() noexcept
      {
        locate_next_available_bucket_ne();
      }

      void next()
      {
        ah_overflow_error_if(curr_index == hash_table->len)
          << "hash table iterator overflow";
        next_ne();
      }

      /// Moves the iterator back by one bucket.
      void prev_ne() noexcept
      {
        locate_prev_available_bucket_ne();
      }

      void prev()
      {
        ah_underflow_error_if(curr_index == -1)
          << "hash table iterator underflow";
        locate_prev_available_bucket();
      }

      Bucket * del()
      {
        Bucket *ret_val = get_curr();
        next();
        hash_table->remove_bucket(ret_val);
        return ret_val;
      }
    };
  };

  /** Bucket without virtual destructor for a hash table with collision
      resolution by separate chaining.

      @param Key hash lookup key type
      @see LhashTable
      @ingroup Hash
  */
  template <typename Key>
  using LhashBucket = Dnode<Key>;


  /** Bucket with virtual destructor for a hash table with collision
      resolution by separate chaining.

      @param Key hash lookup key type
      @see LhashTable
      @ingroup Hash
  */
  template <typename Key>
  struct LhashBucketVtl : public Dnode<Key>
  {
    using Base = Dnode<Key>;
    using Base::Base;

    /// Virtual destructor.
    virtual ~LhashBucketVtl() = default;
  };

  /** Generic hash table with collision resolution by separate chaining and
      buckets without virtual destructor.

      The LhashTable type implements a generic hash table with collision
      resolution by separate chaining where buckets do not have a virtual
      destructor.

      @param Key indexing key type of the table.
      @param Cmp key comparison class; must return true if keys are equal.

      @see GenLhashTable LhashTableVtl
      @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  struct LhashTable : public GenLhashTable<Key, LhashBucket<Key>, Cmp>
  {
    using Base = GenLhashTable<Key, LhashBucket<Key>, Cmp>;
    using Base::Base;
  };

  /** Generic hash table with collision resolution by separate chaining and
      buckets with virtual destructor.

      The LhashTableVtl type implements a generic hash table with collision
      resolution by separate chaining where buckets have a virtual destructor.

      @param Key indexing key type of the table.
      @param Cmp key comparison class; must return true if keys are equal.

      @see GenLhashTable LhashTable
      @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  struct LhashTableVtl : public GenLhashTable<Key, LhashBucketVtl<Key>, Cmp>
  {
    using Base = GenLhashTable<Key, LhashBucketVtl<Key>, Cmp>;
    using Base::Base;
  };
} // end namespace Aleph

# ifdef NBACKUP
# define N NBACKUP
# undef NBACKUP
# endif

# ifdef MBACKUP
# define M MBACKUP
# undef MBACKUP
# endif

# endif/* TPL_LHASH_H */
