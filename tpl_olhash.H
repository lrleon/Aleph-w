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

/** @file tpl_olhash.H
 *  @brief Open addressing hash table with linear probing.
 *
 *  This file implements an open-addressed hash table using linear probing
 *  for collision resolution. Linear probing offers excellent cache
 *  performance due to sequential memory access patterns.
 *
 *  ## Linear Probing Strategy
 *
 *  When a collision occurs at position h(k), the probe sequence is:
 *  ```
 *  h(k, i) = (h(k) + i) mod m
 *  ```
 *  Simply checking the next slot until an empty one is found.
 *
 *  ## Key Features
 *
 *  - Excellent cache locality (sequential access)
 *  - Simple implementation
 *  - Automatic resizing based on load factor
 *  - Supports deletion with tombstones
 *
 *  ## Complexity
 *
 *  | Operation | Average | Worst Case |
 *  |-----------|---------|------------|
 *  | insert | O(1) | O(n) |
 *  | search | O(1) | O(n) |
 *  | remove | O(1) | O(n) |
 *
 *  ## Primary Clustering
 *
 *  Linear probing suffers from primary clustering: consecutive occupied
 *  slots form clusters that grow over time. Keep load factor < 0.5 to
 *  minimize this effect.
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  OLhashTable<int> table;
 *
 *  table.insert(42);
 *  table.insert(17);
 *
 *  if (table.search(42) != nullptr)
 *    std::cout << "Found 42\n";
 *
 *  table.remove(42);
 *  ```
 *
 *  ## When to Use
 *
 *  - Small to medium tables
 *  - Cache performance is critical
 *  - Keys are uniformly distributed
 *
 *  @see tpl_odhash.H Double hashing (less clustering)
 *  @see tpl_lhash.H Chaining (no clustering)
 *  @see hashDry.H Common open addressing utilities
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_OLHASH_H
# define TPL_OLHASH_H

# include <iostream>
# include <cstddef>
# include <cstdint>
# include <primes.H>
# include <dlink.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hashDry.H>
# include <hash-fct.H>
# include <ah-errors.H>

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

namespace Aleph
{

  /** @brief Open addressing hash table with linear probing collision resolution.

      This class implements a closed hash table (contiguous memory array)
      that resolves collisions using linear probing. When a collision occurs,
      consecutive buckets are checked sequentially until an empty slot is found.

      Linear probing has better cache locality than double hashing but may
      suffer from primary clustering. It is simpler and often faster for
      small to medium-sized tables.

      @tparam Key The type of keys stored in the table.
      @tparam Cmp Comparison functor for keys (default: Aleph::equal_to<Key>).
                  Must return true if two keys are equal.

      @par Collision Resolution Strategy:
      Linear probing from hash_fct(key) % len, incrementing by 1 until
      an empty bucket is found.

      @par Example:
      @code
      OLhashTable<std::string> table;
      table.insert("hello");
      table.insert("world");
      auto ptr = table.search("hello");
      if (ptr != nullptr)
        std::cout << "Found: " << *ptr << '\n';
      @endcode

      @see ODhashTable for double hashing variant.
      @see OhashCommon for shared hash table operations.
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

    using Hash_Fct_Ptr = size_t (*)(const Key &);

    enum Status
    {
      EMPTY, BUSY, DELETED
    };

    struct Bucket
    {
      Key key;
      char status = EMPTY;

      Bucket() noexcept: status(EMPTY) {}
      void reset() noexcept { status = EMPTY; }
    };

    [[nodiscard]] static Bucket *key_to_bucket(Key *rec) noexcept
    {
      // Note: cannot be constexpr due to reinterpret_cast
      const auto base = reinterpret_cast<std::uintptr_t>(rec);
      const auto offset = offsetof(Bucket, key);
      return reinterpret_cast<Bucket *>(base - offset);
    }

    Bucket *table = nullptr;
    size_t N = 0;

   protected:

    size_t len;
    float lower_alpha;
    float upper_alpha;
    Cmp cmp;

   private:

    Hash_Fct hash_fct;
    bool with_resize;

    [[nodiscard]] bool is_valid_bucket(Bucket *bucket) const noexcept
    {
      if (table == nullptr)
        return false;

      const auto begin = reinterpret_cast<std::uintptr_t>(&table[0]);
      const auto end   = reinterpret_cast<std::uintptr_t>(&table[len]);
      const auto addr  = reinterpret_cast<std::uintptr_t>(bucket);

      if (addr < begin or addr >= end)
        return false;

      const auto offset_with_base =
        static_cast<std::ptrdiff_t>(addr - begin);

      return offset_with_base % sizeof(*bucket) == 0;
    }

   public:

    [[nodiscard]] constexpr const Cmp &get_compare() const noexcept { return cmp; }

    [[nodiscard]] constexpr Cmp &get_compare() noexcept { return cmp; }

   public:

    /// Instantiate a hash table with hash function __hash_fct and
    /// dimension len.
    OLhashTable(const size_t l, Hash_Fct hash_f, Cmp cmp_f,
                const float l_alpha, const float u_alpha, const bool resize)
      : table(nullptr), N(0), len(Primes::next_prime(l)),
        lower_alpha(l_alpha), upper_alpha(u_alpha), cmp(cmp_f),
        hash_fct(hash_f), with_resize(resize)
    {
      table = new Bucket[len];
    }

    OLhashTable(size_t len, Hash_Fct hash_fct, Hash_Fct, Cmp cmp,
                float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, hash_fct, cmp, lower_alpha, upper_alpha, with_resize) {}

    OLhashTable(size_t len = Primes::DefaultPrime,
                Hash_Fct_Ptr hash_fct = Aleph::dft_hash_fct<Key>,
                Cmp cmp = Cmp(),
                float lower_alpha = hash_default_lower_alpha,
                float upper_alpha = 0.70f,
                bool with_resize = true)
      : OLhashTable(len, Hash_Fct(hash_fct), cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    OLhashTable(size_t len, Hash_Fct_Ptr hash_fct, Hash_Fct_Ptr,
                Cmp cmp, float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, Hash_Fct(hash_fct), cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    /** Constructor with two hash functions for metaprogramming
        compatibility with ODhashTable type
    */
    OLhashTable(size_t len, Hash_Fct hash_fct, Hash_Fct_Ptr, Cmp cmp,
                float lower_alpha, float upper_alpha, bool with_resize)
      : OLhashTable(len, hash_fct, cmp,
                    lower_alpha, upper_alpha, with_resize) {}

    Special_Ctors(OLhashTable, Key);

    /// Release all occupied memory.
    ~OLhashTable()
    {
      if (table != nullptr)
        delete[] table;
    }

    void swap(OLhashTable &other) noexcept
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

    OLhashTable(const OLhashTable &other)
      : OLhashTable(other.len, other.hash_fct, other.cmp,
                    other.lower_alpha, other.upper_alpha, other.with_resize)
    {
      this->copy_from_table(other);
    }

    OLhashTable(OLhashTable &&other) noexcept: OLhashTable(other)
    {
      swap(other);
    }

    OLhashTable &operator=(const OLhashTable &other)
    {
      if (this == &other)
        return *this;

      if (len > other.N)
        this->clean_table();
      else
        {
          auto *new_table = new Bucket[other.len];
          delete[] table;
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

    OLhashTable &operator=(OLhashTable &&other) noexcept
    {
      swap(other);
      return *this;
    }

    /// Finds the key and returns the associated record if key is
    /// find inside the table; otherwise, nullptr is returned.
    [[nodiscard]] Key *search(const Key &key) const noexcept
    {
      size_t i = hash_fct(key) % len;
      for (size_t c = 0; c < len; ++c)
        {
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          if (table[i].status == EMPTY) [[unlikely]]
            return nullptr;

          if (table[i].status == BUSY and cmp(table[i].key, key)) [[likely]]
            return &table[i].key;

          if (++i == len)
            i = 0;
        }

      return nullptr; // Key not found
    }

   protected:

    Bucket *allocate_bucket(const Key &key) noexcept
    {
      Bucket * first_deleted = nullptr;
      size_t i = hash_fct(key) % len;

      for (size_t c = 0; c < len; ++c)
        {
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          auto & b = table[i];
          if (b.status == EMPTY) [[likely]]
            {
              Bucket *bucket = first_deleted != nullptr ? first_deleted : &b;
              bucket->status = BUSY;
              ++N;
              return bucket;
            }
          if (b.status == BUSY)
            {
              if (cmp(key, b.key)) [[unlikely]]
                  return nullptr;  // Duplicate
            }
          else // DELETED
            {
              if (first_deleted == nullptr)
                first_deleted = &b;
            }

          if (++i == len)
            i = 0;
        }

      if (first_deleted != nullptr) [[unlikely]]
        {
          first_deleted->status = BUSY;
          ++N;
          return first_deleted;
        }

      return nullptr;
    }

    // Allocate a new bucket for storing key. It always returns a
    // pointer to the bucket entry into the table. If the key is
    // already inserted in the table, then second value is true,
    // otherwise, not allocation is done, and false is returned as second
    std::tuple<Bucket *, bool> hard_allocate_bucket(const Key &key) noexcept
    {
      Bucket * first_deleted = nullptr;
      size_t i = hash_fct(key) % len;

      for (size_t c = 0; c < len; ++c)
        {
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          auto & b = table[i];
          if (b.status == EMPTY) [[likely]]
            {
              Bucket *bucket = first_deleted != nullptr ? first_deleted : &b;
              bucket->status = BUSY;
              ++N;
              return {bucket, false};
            }
          else if (b.status == BUSY)
            {
              if (cmp(key, b.key))
                return {&b, true};  // Found existing
            }
          else // DELETED
            {
              if (first_deleted == nullptr)
                first_deleted = &b;
            }

          if (++i == len)
            i = 0;
        }

      if (first_deleted != nullptr) [[unlikely]]
        {
          first_deleted->status = BUSY;
          ++N;
          return {first_deleted, false};
        }

      return {nullptr, false};
    }

    /// Index of previous bucket (handles wrap-around)
    size_t prev_index(const size_t i) const noexcept
    {
      return (i == 0) ? len - 1 : i - 1;
    }

    /// Index of next bucket (handles wrap-around)
    size_t next_index(const size_t i) const noexcept
    {
      return (i + 1 == len) ? 0 : i + 1;
    }

    /// Cleanup DELETED entries that are at the end of collision chains.
    /// If the bucket at idx has a following EMPTY bucket, it can be marked
    /// EMPTY instead of DELETED. Then propagate backwards to clean any
    /// DELETED buckets that now also end in EMPTY.
    /// This implements Knuth's optimization to avoid tombstone accumulation.
    void cleanup_deleted_chain(size_t idx) noexcept
    {
      // If next bucket is not EMPTY, we must keep this as DELETED
      if (table[next_index(idx)].status != EMPTY)
        return;

      // Next is EMPTY, so we can mark this as EMPTY
      table[idx].status = EMPTY;

      // Propagate backwards: any DELETED bucket whose next is now EMPTY
      // can also become EMPTY
      size_t prev = prev_index(idx);
      size_t count = 0;
      while (table[prev].status == DELETED and count < len)
        {
          table[prev].status = EMPTY;
          prev = prev_index(prev);
          ++count;
        }
    }

    /// Removes the record pointed to by record from the table. Obviously,
    /// record must point to an entry returned by insert() or
    /// search(). Fires invalid_argument exceptions if record is not an
    /// address within table range or domain_error if bucket
    /// of the registry is not busy.
    void deallocate_bucket(Bucket *bucket)
    {
      ah_invalid_argument_if(not is_valid_bucket(bucket))
        << "record address is not inside table's range";

      ah_domain_error_if(bucket->status != BUSY)
        << "Bucket containing record is not busy";

      --N;
      const auto idx = static_cast<size_t>(bucket - &table[0]);
      table[idx].status = DELETED;
      cleanup_deleted_chain(idx);
    }

   public:

    /** Remove the key referenced by key. key must be a valid reference
        to the key previously inserted or retrieved in the table. If
        the key is not in the table, then a domain_error exception is
        thrown. */
    void remove(const Key &key)
    {
      size_t i = hash_fct(key) % len;
      for (size_t c = 0; c < len; ++c)
        {
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          if (table[i].status == EMPTY) [[unlikely]]
            ah_domain_error() << "Key not in hash table";

          if (table[i].status == BUSY and cmp(table[i].key, key)) [[likely]]
            {
              table[i].status = DELETED;
              --N;
              cleanup_deleted_chain(i);
              return;
            }

          if (++i == len)
            i = 0;
        }

      ah_domain_error() << "Key not in hash table";
    }

   OHASH_COMMON(OLhashTable);

    using Stats = typename OhashCommon<OLhashTable<Key, Cmp>, Key>::Stats;

    [[nodiscard]] Stats stats() const
    {
      DynArray<size_t> lens;
      size_t num_busy = 0;
      size_t num_deleted = 0;
      size_t num_empty = 0;
      size_t max_len = std::numeric_limits<size_t>::min();
      for (size_t i = 0; i < len; ++i)
        switch (table[i].status)
          {
            case BUSY:
              {
                ++num_busy;
                const Key &key = table[i].key;
                size_t i = hash_fct(key) % len;
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

      float avg = 0;
      float sum = 0;
      for (size_t i = 0; i < lens.size(); ++i)
        {
          avg += lens(i) * i;
          sum += lens(i);
        }

      avg /= sum;
      float var = 0;
      for (size_t i = 0; i < lens.size(); ++i)
        {
          const float s = i - avg;
          var += lens(i) * s * s;
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
