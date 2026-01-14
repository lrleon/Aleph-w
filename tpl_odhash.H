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

/** @file tpl_odhash.H
 *  @brief Open addressing hash table with double hashing.
 *
 *  This file implements an open-addressed hash table that uses double
 *  hashing for collision resolution. Double hashing provides excellent
 *  distribution and avoids primary and secondary clustering.
 *
 *  ## Double Hashing Strategy
 *
 *  When a collision occurs at position h1(k), the probe sequence is:
 *  ```
 *  h(k, i) = (h1(k) + i * h2(k)) mod m
 *  ```
 *  where h2(k) is a secondary hash function.
 *
 *  ## Key Features
 *
 *  - No clustering (unlike linear probing)
 *  - Better cache locality than chaining
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
 *  Expected probes: 1/(1-α) for unsuccessful, (1/α)ln(1/(1-α)) for successful
 *  where α is the load factor.
 *
 *  ## Load Factor
 *
 *  The table resizes when load factor exceeds a threshold (default ~0.5).
 *  Lower load factors mean faster operations but more memory.
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  ODhashTable<int> table;
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
 *  ## Comparison with OLhashTable
 *
 *  | Feature | ODhashTable | OLhashTable |
 *  |---------|-------------|-------------|
 *  | Probe | Double hash | Linear |
 *  | Clustering | None | Primary |
 *  | Cache | Good | Excellent |
 *
 *  @see tpl_olhash.H Linear probing variant
 *  @see tpl_lhash.H Chaining with linear expansion
 *  @see hashDry.H Common open addressing utilities
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_ODHASH_H
# define TPL_ODHASH_H

# include <iostream>
# include <cstddef>
# include <cstdint>

# include <primes.H>
# include <dlink.H>
# include <tpl_dynArray.H>
# include <array_it.H>
# include <ahDry.H>
# include <hash-dry.H>
# include <hashDry.H>
# include <hash-fct.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph
{
# ifdef N
# define NBACKUP N
# undef N
# endif

# ifdef M
# define MBACKUP M
# undef M
# endif


  /** @brief Open addressing hash table with double hashing collision resolution.

      This class implements a closed hash table (contiguous memory array)
      that resolves collisions using double hashing. When a collision occurs,
      a second hash function is called to probe for an available bucket.
      If the second probe also collides, linear probing continues from
      that index.

      The table uses a probe counter mechanism that enables O(1) deletion
      without relocation. Each bucket tracks how many keys depend on it
      being in the collision chain, allowing efficient garbage collection
      of DELETED entries.

      Based on the algorithms described in Knuth's "The Art of Computer
      Programming", Volume 3.

      @tparam Key The type of keys stored in the table.
      @tparam Cmp Comparison functor for keys (default: Aleph::equal_to<Key>).
                  Must return true if two keys are equal.

      @par Collision Resolution Strategy:
      1. First probe: hash_fct(key) % len
      2. Second probe: second_hash_fct(key) % len
      3. Linear probing from second probe position

      @par Example:
      @code
      ODhashTable<int> table;
      table.insert(42);
      table.insert(17);
      if (table.has(42))
        std::cout << "Found 42" << '\n';
      table.remove(42);
      @endcode

      @see OLhashTable for linear probing variant.
      @see OhashCommon for shared hash table operations.
      @ingroup Hash
  */
  template <typename Key, class Cmp = Aleph::equal_to<Key>>
  class ODhashTable
      : public OhashCommon<ODhashTable<Key, Cmp>, Key>,
        public GenericTraverse<ODhashTable<Key, Cmp>>,
        public LocateFunctions<ODhashTable<Key, Cmp>, Key>,
        public FunctionalMethods<ODhashTable<Key, Cmp>, Key>,
        public EqualToMethod<ODhashTable<Key, Cmp>>,
        public StlAlephIterator<ODhashTable<Key, Cmp>>
  {
    friend class OhashCommon<ODhashTable<Key, Cmp>, Key>;

  public:
    using Key_Type = Key;

    using Item_Type = Key;

    using Hash_Fct = std::function<size_t(const Key &)>;

    using Hash_Fct_Ptr = size_t (*)(const Key &);

    enum Status { EMPTY, BUSY, DELETED };

    enum Probe { NO_PROBED, FIRST_PROBE, SECOND_PROBE, LINEAR_PROBE };

    struct Bucket
    {
      Key key;
      unsigned char status = EMPTY; // status EMPTY, DELETED o BUSY
      unsigned char probe_type = NO_PROBED; // FIRST_PROBE SECOND_PROBE LINEAR_PROBE
      unsigned int probe_counter = 0;

      Bucket() noexcept
        : key(), status(EMPTY), probe_type(NO_PROBED), probe_counter(0)
      { /* empty */
      }

      void reset() noexcept // put all as when constructed
      {
        status = EMPTY;
        probe_type = NO_PROBED;
        probe_counter = 0;
      }

      [[nodiscard]] constexpr bool valid() const noexcept
      {
        return (status == EMPTY or status == DELETED or status == BUSY) and
               (probe_type == NO_PROBED or probe_type == FIRST_PROBE or
                probe_type == SECOND_PROBE or probe_type == LINEAR_PROBE);
      }

      friend std::ostream &operator <<(std::ostream & out, const Bucket & bucket)
      {
        std::string status_str;
        switch (bucket.status)
          {
          case EMPTY: status_str = "EMPTY";
            break;
          case BUSY: status_str = "BUSY";
            break;
          case DELETED: status_str = "DELETED";
            break;
          }
        std::string probe_type_str;
        switch (bucket.probe_type)
          {
          case NO_PROBED: probe_type_str = "NO_PROBED";
            break;
          case FIRST_PROBE: probe_type_str = "FIRST_PROBE";
            break;
          case SECOND_PROBE: probe_type_str = "SECOND_PROBE";
            break;
          case LINEAR_PROBE: probe_type_str = "LINEAR_PROBE";
            break;
          }
        return out << "Bucket at " << &bucket << '\n'
               << "status = " << status_str << '\n'
               << "probe_type = " << probe_type_str << '\n'
               << "probe_counter = " << bucket.probe_counter;
      }
    };

    Bucket *table = nullptr; //bucket arrangement
    Hash_Fct hash_fct = nullptr; // first hash function
    Hash_Fct second_hash_fct = nullptr; // second hash function

    Cmp cmp;

  protected:
    size_t len; // table size
    float lower_alpha;
    float upper_alpha;

  private:
    size_t N; // number of buckets occupied
    bool with_resize;

    Bucket * allocate_bucket(Bucket & bucket, unsigned char probe_type) noexcept
    {
      assert(bucket.status != BUSY);

      ++N;
      bucket.status = BUSY;
      bucket.probe_type = probe_type;
      ++bucket.probe_counter;

      return &bucket;
    }

    void decrease_probe_counter(Bucket *bucket) noexcept
    {
      assert(bucket->status == BUSY or bucket->status == DELETED);

      --bucket->probe_counter;
      if (bucket->probe_counter == 0) // <mark EMPTY on the bucket?
        bucket->status = EMPTY;
    }

    void deallocate_bucket(Bucket *bucket) noexcept
    {
      deallocate_bucket_with_record(bucket, [](Bucket *) {});
    }

    size_t index_forward(size_t & i) const noexcept
    {
      assert(i < len);
      if (++i == len)
        i = 0;
      return i;
    }

    size_t index_backward(size_t & i) const noexcept
    {
      assert(i < len);
      if (i-- == 0)
        i = len - 1;
      return i;
    }

    [[nodiscard]] bool is_valid_bucket(Bucket *bucket) const noexcept
    {
      if (table == nullptr)
        return false;

      const auto begin = reinterpret_cast<std::uintptr_t>(&table[0]);
      const auto end = reinterpret_cast<std::uintptr_t>(&table[len]);
      const auto addr = reinterpret_cast<std::uintptr_t>(bucket);

      if (addr < begin or addr >= end)
        return false;

      const auto offset_with_base =
          static_cast<std::ptrdiff_t>(addr - begin);
      return offset_with_base % sizeof(*bucket) == 0;
    }

    [[nodiscard]] size_t bucket_to_index(Bucket *bucket) const noexcept
    {
      assert(is_valid_bucket(bucket));
      return bucket - &table[0];
    }

    /// Returns true if the key reference points to a key inside the table
    [[nodiscard]] bool is_key_in_table(const Key & key) const noexcept
    {
      if (table == nullptr)
        return false;

      const auto addr = reinterpret_cast<std::uintptr_t>(&key);
      const auto table_begin = reinterpret_cast<std::uintptr_t>(&table[0]);
      const auto table_end = reinterpret_cast<std::uintptr_t>(&table[len]);

      if (addr < table_begin or addr >= table_end)
        return false;

      constexpr auto key_offset = offsetof(Bucket, key);
      return ((addr - table_begin - key_offset) % sizeof(Bucket)) == 0;
    }

    /// Converts a key reference inside the table to its containing bucket
    [[nodiscard]] Bucket * key_in_table_to_bucket(const Key & key) noexcept
    {
      assert(is_key_in_table(key));
      return key_to_bucket(const_cast<Key *>(&key));
    }

    /// Template version of deallocate_bucket that accepts a callback
    /// to record modified buckets before decrementing their probe_counter
    template <typename RecordBucket>
    void deallocate_bucket_with_record(Bucket *bucket,
                                       RecordBucket && record_bucket) noexcept
    {
      assert(bucket->status == BUSY);

      bucket->status = DELETED;
      const Key & key = bucket->key;

      const size_t i_fst = hash_fct(key) % len;
      if (&table[i_fst] == bucket)
        {
          assert(Cmp () (table[i_fst].key, key));
          assert(table[i_fst].probe_type == FIRST_PROBE);
        }
      else
        {
          const size_t i_snd = second_hash_fct(key) % len;
          if (&table[i_snd] == bucket)
            {
              assert(Cmp () (table[i_snd].key, key));
              assert(table[i_snd].probe_type == SECOND_PROBE);
              record_bucket(&table[i_fst]);
              decrease_probe_counter(&table[i_fst]);
            }
          else
            {
              record_bucket(&table[i_fst]);
              decrease_probe_counter(&table[i_fst]);
              record_bucket(&table[i_snd]);
              decrease_probe_counter(&table[i_snd]);
              size_t i = i_snd;
              for (index_forward(i); &table[i] != bucket; index_forward(i))
                {
                  assert(table[i].status != EMPTY);
                  record_bucket(&table[i]);
                  decrease_probe_counter(&table[i]);
                }
              assert(Cmp () (table[i].key, key));
              assert(table[i].probe_type == LINEAR_PROBE);
            }
        }

      decrease_probe_counter(bucket);
      --N;
    }

  public:
    [[nodiscard]] static Bucket * key_to_bucket(Key *rec) noexcept
    {
      const auto base = reinterpret_cast<std::uintptr_t>(rec);
      const auto offset = offsetof(Bucket, key);
      return reinterpret_cast<Bucket *>(base - offset);
    }

    [[nodiscard]] constexpr const Cmp &get_compare() const noexcept { return cmp; }

    [[nodiscard]] constexpr Cmp &get_compare() noexcept { return cmp; }

    void swap(ODhashTable & other) noexcept
    {
      std::swap(table, other.table);
      std::swap(hash_fct, other.hash_fct);
      std::swap(second_hash_fct, other.second_hash_fct);
      std::swap(cmp, other.cmp);
      std::swap(N, other.N);
      std::swap(len, other.len);
    }

  protected:
    ODhashTable(const size_t l, Hash_Fct fst_hash_fct,
                Hash_Fct snd_hash_fct, Cmp cpm_fct,
                const float lower, const float upper, const bool resize)
      : table(nullptr), hash_fct(fst_hash_fct),
        second_hash_fct(snd_hash_fct), cmp(cpm_fct),
        len(Primes::next_prime(l)),
        lower_alpha(lower), upper_alpha(upper),
        N(0), with_resize(resize)
    {
      table = new Bucket[len];
    }

  public:
    /** Instantiate a closed hash table with collision resolution by
        double hashing.

        @param[in] first_hash_fct hash function for the first probe.
        @param[in] second_hash_fct hash function for the second probe.
        @param cmp comparison functor for keys.
        Must return true if two keys are equal.
        By default, Aleph::equal_to<Key> is used.
        @param lower_alpha lower load factor threshold for resizing.
         When the load factor goes below this value, the table shrinks.
        0 < lower_alpha < upper_alpha < 1.
        0.25 is a reasonable default.
        0.1 gives faster operations but uses more memory.
        @param upper_alpha upper load factor threshold for resizing.
         When the load factor goes above this value, the table expands.
        0 < lower_alpha < upper_alpha < 1.
        0.5 is a reasonable default.
        0.7 gives faster operations but uses more memory.
        @param with_resize if true, the table resizes automatically when
         load factor thresholds are crossed.
        @param[in] len table size.

        @throw bad_alloc if there is no memory for the bucket table.
    */
    ODhashTable(size_t len = Primes::DefaultPrime,
                Hash_Fct_Ptr first_hash_fct = Aleph::dft_hash_fct<Key>,
                Hash_Fct_Ptr second_hash_fct = Aleph::snd_hash_fct<Key>,
                Cmp cmp = Cmp(),
                float lower_alpha = hash_default_lower_alpha,
                float upper_alpha = hash_default_upper_alpha,
                bool with_resize = true)
      : ODhashTable(len, Hash_Fct(first_hash_fct), Hash_Fct(second_hash_fct),
                    cmp, lower_alpha, upper_alpha, with_resize) {}

    /// Free the whole hash table.
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

    ODhashTable(ODhashTable && other) noexcept
      : ODhashTable(other)
    {
      assert(table != nullptr);
      swap(other);
    }

    Special_Ctors(ODhashTable, Key);

    ODhashTable &operator =(const ODhashTable & other)
    {
      if (this == &other)
        return *this;

      if (len > other.N)
        this->clean_table();
      else
        {
          auto *new_table = new Bucket [other.len];
          delete [] table;
          table = new_table;
          N = 0;
          len = other.len;
          hash_fct = other.hash_fct;
          second_hash_fct = other.second_hash_fct;
          cmp = other.cmp;
          lower_alpha = other.lower_alpha;
          upper_alpha = other.upper_alpha;
          with_resize = other.with_resize;
        }

      this->copy_from_table(other);

      return *this;
    }

    ODhashTable &operator =(ODhashTable && other) noexcept
    {
      swap(other);
      return *this;
    }

    OHASH_COMMON(ODhashTable);

    /// searches the table for the key. Return a pointer to the record
    /// associated with key within the table; nullptr otherwise.
    [[nodiscard]] Key * search(const Key & key) const noexcept
    {
      const size_t i_fst = hash_fct(key) % len; // 1st probe (1st hash function)
      if (table[i_fst].status == EMPTY) [[unlikely]]
          return nullptr;

      if (table[i_fst].status == BUSY and cmp(table[i_fst].key, key)) [[likely]]
        {
          assert(table[i_fst].probe_type == FIRST_PROBE);
          assert(table[i_fst].probe_counter > 0);
          return &table[i_fst].key;
        }

      // Early exit: if BUSY with different key and probe_counter == 1,
      // no other key with this first hash can exist in the table
      if (table[i_fst].status == BUSY and table[i_fst].probe_counter == 1)
        return nullptr;

      const size_t i_snd = second_hash_fct(key) % len; // 2nd probe

      // If both hashes collide to same index, skip directly to linear probing
      if (i_fst == i_snd) [[unlikely]]
          goto linear_probe;

      if (table[i_snd].status == EMPTY)
        return nullptr;

      if (table[i_snd].status == BUSY and cmp(table[i_snd].key, key))
        {
          assert(table[i_snd].probe_type == SECOND_PROBE);
          assert(table[i_snd].probe_counter > 0);
          return &table[i_snd].key;
        }

      // Early exit: if BUSY with different key and probe_counter == 1,
      // no other key passed through this bucket
      if (table[i_snd].status == BUSY and table[i_snd].probe_counter == 1)
        return nullptr;

    linear_probe:
      size_t i = i_snd;
      // Linear polling from index of 2nd hash function
      for (size_t count = 0; count < len; ++count)
        {
          index_forward(i);
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);
          switch (table[i].status)
            {
            case EMPTY:
              assert(table[i].probe_counter == 0);
              return nullptr;
            case BUSY:
              assert(table[i].probe_counter > 0);
              if (cmp(table[i].key, key)) [[unlikely]]
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

    [[nodiscard]] constexpr Hash_Fct get_second_hash_fct() const noexcept { return second_hash_fct; }

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

      const size_t i_fst = hash_fct(key) % len;

      // EMPTY at first probe: safe to insert, no collision chain
      if (table[i_fst].status == EMPTY)
        return allocate_bucket(table[i_fst], FIRST_PROBE);

      // BUSY at first probe: check for duplicate
      if (table[i_fst].status == BUSY && cmp(table[i_fst].key, key))
        return nullptr;

      const size_t i_snd = second_hash_fct(key) % len;

      // EMPTY at second probe: safe to insert if first wasn't the key
      if (table[i_snd].status == EMPTY)
        {
          // If first was DELETED, use it; otherwise use second
          if (table[i_fst].status == DELETED)
            return allocate_bucket(table[i_fst], FIRST_PROBE);
          ++table[i_fst].probe_counter;
          return allocate_bucket(table[i_snd], SECOND_PROBE);
        }

      // BUSY at second probe: check for duplicate
      if (table[i_snd].status == BUSY && cmp(table[i_snd].key, key))
        return nullptr;

      // Need to do linear probing - search for duplicate and find insertion point
      Bucket *candidate = nullptr;
      size_t candidate_idx = 0;

      // Check if first or second are candidates (DELETED)
      if (table[i_fst].status == DELETED)
        {
          candidate = &table[i_fst];
          candidate_idx = 0; // Marker: use first probe slot
        }
      else if (table[i_snd].status == DELETED)
        {
          candidate = &table[i_snd];
          candidate_idx = 1; // Marker: use second probe slot
        }

      size_t i = i_snd;
      for (size_t c = 0; c < len; ++c)
        {
          index_forward(i);
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          if (table[i].status == EMPTY)
            {
              // End of collision chain - no duplicate, insert
              if (candidate)
                {
                  // Insert at the first DELETED slot found
                  if (candidate_idx == 0) // First probe slot
                    return allocate_bucket(*candidate, FIRST_PROBE);
                  if (candidate_idx == 1) // Second probe slot
                    {
                      ++table[i_fst].probe_counter;
                      return allocate_bucket(*candidate, SECOND_PROBE);
                    }
                  // Linear probe slot - need to update all counters
                  ++table[i_fst].probe_counter;
                  ++table[i_snd].probe_counter;
                  size_t j = i_snd;
                  for (size_t k = 0; k < candidate_idx - 2; ++k)
                    {
                      index_forward(j);
                      ++table[j].probe_counter;
                    }
                  return allocate_bucket(*candidate, LINEAR_PROBE);
                }
              // No DELETED found, insert at this EMPTY slot
              ++table[i_fst].probe_counter;
              ++table[i_snd].probe_counter;
              // Increment probe_counter for all buckets between i_snd and i (exclusive)
              size_t j = i_snd;
              for (size_t k = 0; k < c; ++k)
                {
                  index_forward(j);
                  ++table[j].probe_counter;
                }
              return allocate_bucket(table[i], LINEAR_PROBE);
            }

          if (table[i].status == BUSY && cmp(table[i].key, key))
            return nullptr; // Duplicate found

          if (table[i].status == DELETED && candidate == nullptr)
            {
              candidate = &table[i];
              candidate_idx = c + 2; // Store position (2 = after first and second)
            }
        }

      // Table full, but might have a DELETED slot
      if (candidate)
        {
          if (candidate_idx == 0)
            return allocate_bucket(*candidate, FIRST_PROBE);
          if (candidate_idx == 1)
            {
              ++table[i_fst].probe_counter;
              return allocate_bucket(*candidate, SECOND_PROBE);
            }
          ++table[i_fst].probe_counter;
          ++table[i_snd].probe_counter;
          size_t j = i_snd;
          for (size_t k = 0; k < candidate_idx - 2; ++k)
            {
              index_forward(j);
              ++table[j].probe_counter;
            }
          return allocate_bucket(*candidate, LINEAR_PROBE);
        }

      return nullptr;
    }

    // search key. If found return (bucket_ptr, true), otherwise allocates and
    // returns (new_bucket_ptr, false)
    std::tuple<Bucket *, bool> hard_allocate_bucket(const Key & key) noexcept
    {
      assert(N < len);

      Bucket *candidate = nullptr;
      size_t candidate_linear_steps = 0; // How many linear steps to reach candidate

      const size_t i_fst = hash_fct(key) % len;
      switch (table[i_fst].status)
        {
        case EMPTY:
          return {allocate_bucket(table[i_fst], FIRST_PROBE), false};
        case DELETED:
          candidate = &table[i_fst];
          // candidate at FIRST_PROBE, no linear steps needed
          break;
        case BUSY:
          if (cmp(table[i_fst].key, key))
            return {&table[i_fst], true}; // found existing
          break;
        }

      const size_t i_snd = second_hash_fct(key) % len;
      switch (table[i_snd].status)
        {
        case EMPTY:
          if (candidate) // candidate is at i_fst (FIRST_PROBE)
            return {allocate_bucket(*candidate, FIRST_PROBE), false};
          ++table[i_fst].probe_counter;
          return {allocate_bucket(table[i_snd], SECOND_PROBE), false};
        case DELETED:
          if (candidate == nullptr)
            {
              candidate = &table[i_snd];
              // candidate at SECOND_PROBE, no linear steps needed
            }
          break;
        case BUSY:
          if (cmp(table[i_snd].key, key))
            return {&table[i_snd], true}; // found existing
          break;
        }

      // Determine if candidate is at i_fst or i_snd (for probe_type later)
      const bool candidate_at_first = (candidate == &table[i_fst]);
      const bool candidate_at_second = (candidate == &table[i_snd]);

      size_t i = i_snd;
      for (size_t c = 0; c < len; ++c)
        {
          index_forward(i);
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);
          switch (table[i].status)
            {
            case BUSY:
              if (cmp(table[i].key, key))
                return {&table[i], true}; // found existing
              break;
            case DELETED:
              if (candidate == nullptr)
                {
                  candidate = &table[i];
                  candidate_linear_steps = c + 1; // Steps from i_snd
                }
              break;
            case EMPTY:
              {
                // End of collision chain - insert at candidate or here
                if (candidate)
                  {
                    if (candidate_at_first)
                      return {allocate_bucket(*candidate, FIRST_PROBE), false};

                    ++table[i_fst].probe_counter;

                    if (candidate_at_second)
                      return {allocate_bucket(*candidate, SECOND_PROBE), false};

                    // Candidate is in LINEAR_PROBE region
                    ++table[i_snd].probe_counter;
                    // Increment all buckets between i_snd and candidate
                    size_t j = i_snd;
                    for (size_t k = 0; k < candidate_linear_steps - 1; ++k)
                      {
                        index_forward(j);
                        ++table[j].probe_counter;
                      }
                    return {allocate_bucket(*candidate, LINEAR_PROBE), false};
                  }
                // No candidate found, insert at this EMPTY slot
                ++table[i_fst].probe_counter;
                ++table[i_snd].probe_counter;
                // Increment all buckets between i_snd and here
                size_t j = i_snd;
                for (size_t k = 0; k < c; ++k)
                  {
                    index_forward(j);
                    ++table[j].probe_counter;
                  }
                return {allocate_bucket(table[i], LINEAR_PROBE), false};
              }
            default: AH_ERROR("ODhashTable: Invalid bucket status");
              break;
            }
        }

      // All slots checked (full table), use candidate if found
      if (candidate)
        {
          if (candidate_at_first)
            return {allocate_bucket(*candidate, FIRST_PROBE), false};

          ++table[i_fst].probe_counter;

          if (candidate_at_second)
            return {allocate_bucket(*candidate, SECOND_PROBE), false};

          // Candidate is in LINEAR_PROBE region
          ++table[i_snd].probe_counter;
          size_t j = i_snd;
          for (size_t k = 0; k < candidate_linear_steps - 1; ++k)
            {
              index_forward(j);
              ++table[j].probe_counter;
            }
          return {allocate_bucket(*candidate, LINEAR_PROBE), false};
        }

      return {nullptr, false};
    }

    /// Delete the record from the table. record must belong to
    /// the table and must have been previously determined by a
    /// insert or search. The invalid_argument exception is raised if
    /// the record does not correspond to a bucket in the table. It shoots
    /// domain_error if the record in the table does not contain an element.
    void remove_bucket(Bucket *bucket)
    {
      ah_invalid_argument_if(not is_valid_bucket(bucket)) << "key pty does not belong to hash table";
      ah_domain_error_if(bucket->status != BUSY) << "Bucket containing key is not BUSY";

      deallocate_bucket(bucket);
    }

  public:
    /// Remove a key from the hash table.
    /// If key is a reference to a key inside the table (from search/insert),
    /// it uses the fast path via deallocate_bucket.
    /// If key is external, it searches for the key and removes it.
    /// Throws domain_error if the key is not found.
    void remove(const Key & key)
    {
      // Fast path: if key is a reference to a key inside the table
      if (is_key_in_table(key)) [[likely]]
        {
          Bucket *bucket = key_in_table_to_bucket(key);
          ah_domain_error_if(bucket->status != BUSY)
            << "Bucket containing key is not BUSY";
          deallocate_bucket(bucket);
          return;
        }

      // External key: search for it (without modifying probe_counters)
      const size_t i_fst = hash_fct(key) % len;

      // Check first probe position
      ah_domain_error_if(table[i_fst].status == EMPTY) << "Key not in hash table";

      if (table[i_fst].status == BUSY and cmp(table[i_fst].key, key)) [[likely]]
        {
          deallocate_bucket(&table[i_fst]);
          return;
        }

      // Early exit: if BUSY with different key and probe_counter == 1,
      // no other key with this first hash can exist in the table
      ah_domain_error_if(table[i_fst].status == BUSY and table[i_fst].probe_counter == 1)
         << "Key not in hash table";

      const size_t i_snd = second_hash_fct(key) % len;

      // If both hashes collide to same index, skip directly to linear probing
      if (i_fst == i_snd) [[unlikely]]
          goto linear_probe_remove;

      // Check second probe position
      ah_domain_error_if(table[i_snd].status == EMPTY) << "Key not in hash table";

      if (table[i_snd].status == BUSY and cmp(table[i_snd].key, key))
        {
          deallocate_bucket(&table[i_snd]);
          return;
        }

      // Early exit: if BUSY with different key and probe_counter == 1,
      // no other key passed through this bucket
      ah_domain_error_if(table[i_snd].status == BUSY and table[i_snd].probe_counter == 1)
        << "Key not in hash table";

    linear_probe_remove:
      // Linear probing (search only, no modifications until key found)
      size_t i = i_snd;
      for (size_t c = 0; c < len; ++c)
        {
          index_forward(i);
          // Prefetch next bucket for better cache performance
          __builtin_prefetch(&table[(i + 1 < len) ? i + 1 : 0], 0, 1);

          switch (table[i].status)
            {
            case EMPTY:
              ah_domain_error() << "Key not in hash table";
              break; // unreachable, but silences -Wimplicit-fallthrough

            case BUSY:
              if (cmp(table[i].key, key)) [[unlikely]]
                {
                  deallocate_bucket(&table[i]);
                  return;
                }
              break;

            case DELETED:
              break;
            }
        }

      ah_domain_error() << "Key not in hash table";
    }

    using Stats = typename OhashCommon<ODhashTable<Key, Cmp>, Key>::Stats;

    [[nodiscard]] Stats stats() const
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
                  assert(table[i_fst].probe_counter > 0);;
                }
              else
                {
                  ++count;
                  size_t i_snd = second_hash_fct(key) % len;
                  if (cmp(table[i_snd].key, key))
                    {
                      assert(table[i_snd].probe_type == SECOND_PROBE);
                      assert(table[i_snd].probe_counter > 0);;
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
