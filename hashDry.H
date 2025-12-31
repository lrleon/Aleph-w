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

# ifndef HASHDRY_H
# define HASHDRY_H

/** @file hashDry.H
    @brief Common operations for open addressing hash tables (CRTP mixin).

    This file provides the OhashCommon class template, which implements
    shared functionality for open addressing hash tables using the
    Curiously Recurring Template Pattern (CRTP). Both ODhashTable and
    OLhashTable inherit from this class.

    The mixin provides:
    - Insert operations (insert, search_or_insert, contains_or_insert)
    - Search and removal operations
    - Automatic resizing based on load factor
    - Iterator support
    - Functional programming methods (through inheritance)

    @see ODhashTable
    @see OLhashTable
    @ingroup Hash
*/

# include <aleph.H>
# include <tpl_dynArray.H>
# include <ah-errors.H>

/** @brief CRTP mixin providing common operations for open addressing hash tables.

    This template class provides shared implementation for hash table operations
    that are common between ODhashTable (double hashing) and OLhashTable
    (linear probing). It uses CRTP to achieve static polymorphism, avoiding
    virtual function overhead while enabling code reuse.

    @tparam HashTbl The derived hash table class (ODhashTable or OLhashTable).
                    Must provide: allocate_bucket(), hard_allocate_bucket(),
                    deallocate_bucket(), test_resize(), search(), table, len, N,
                    lower_alpha, upper_alpha, with_resize, current_alpha(), Bucket.
    @tparam Key The type of keys stored in the table.

    @par Provided Operations:
    - insert() - Insert a new key (fails if key already exists)
    - search_or_insert() - Search for key, insert if not found
    - contains_or_insert() - Like search_or_insert but returns pair with bool
    - has() / contains() - Check if key exists
    - find() - Get reference to key (throws if not found)
    - remove_ptr() - Remove key by pointer
    - resize() - Change table capacity
    - rehash() - Rebuild table with same capacity
    - keys() / items() - Get list of all keys
    - Iterator - Bidirectional iteration over entries

    @par Automatic Resizing:
    When `with_resize` is enabled in the derived class, the table automatically:
    - Grows when it becomes nearly full (before insert would fail)
    - Shrinks when load factor drops below `lower_alpha` threshold

    @par Thread Safety:
    This class is NOT thread-safe. External synchronization is required
    for concurrent access.

    @see ODhashTable
    @see OLhashTable
    @ingroup Hash
*/
template <class HashTbl, typename Key>
class OhashCommon
{
  /** @brief Returns a pointer to the derived class (CRTP pattern).
      @return Pointer to this object cast to HashTbl*.
  */
  HashTbl * me() { return static_cast<HashTbl*>(this); }

  /** @brief Returns a const pointer to the derived class (CRTP pattern).
      @return Const pointer to this object cast to const HashTbl*.
  */
  const HashTbl * const_me() const { return static_cast<const HashTbl*>(this); }

public:

  /** @brief Returns the lower load factor threshold for automatic shrinking.
      
      When automatic resizing is enabled and the load factor drops below
      this value after a removal, the table shrinks.

      @return The lower alpha threshold (typically 0.25).
  */
  [[nodiscard]] constexpr float get_lower_alpha() const noexcept { return const_me()->lower_alpha; }

  /** @brief Returns the upper load factor threshold for automatic growing.
      
      When automatic resizing is enabled and the load factor would exceed
      this value, the table grows before insertion.

      @return The upper alpha threshold (typically 0.75).
  */
  [[nodiscard]] constexpr float get_upper_alpha() const noexcept { return const_me()->upper_alpha; }

  /** @brief Copies all entries from another hash table.
      
      Copies all BUSY entries from the source table to this table.
      This table must be empty and have sufficient capacity.

      @param other Source hash table to copy from.

      @pre This table must be empty (N == 0).
      @pre This table must have capacity >= other.N.
      @post This table contains all entries from other.
  */
  void copy_from_table(const HashTbl & other)
  {
    assert(me()->N == 0 and me()->len >= other.N);
    for (long i = 0, c = 0; c < other.N; ++i)
      {
        auto & bucket = other.table[i];
        if (bucket.status != HashTbl::BUSY)
          continue;

        me()->insert(bucket.key);
        ++c;
      }
    assert(me()->N == other.N);
  }

  /** @brief Removes all entries from the table without deallocating storage.
      
      Resets all buckets to their initial empty state and sets the
      entry count to zero. The table capacity remains unchanged.

      @post size() == 0
      @post capacity() unchanged
  */
  void clean_table()
  {
    for (long i = 0; i < me()->len; ++i)
      me()->table[i].reset();
    me()->N = 0;
  }

  /** @brief Checks if resize is needed before inserting and performs it.
      
      Called before each insert operation to ensure there is at least
      one empty bucket available (required as a sentinel for probing).
      If automatic resizing is enabled and the table is nearly full,
      doubles the table capacity.
  */
  void check_resize_before_insert()
  {
    // Keep at least one EMPTY bucket as a sentinel.
    if (me()->with_resize and me()->N >= me()->len - 1)
      me()->resize(Primes::next_prime(2 * me()->len));
  }

  /** @brief Inserts a key into the hash table (copy version).
      
      Attempts to insert a copy of the key. If the key already exists,
      the insertion fails and nullptr is returned.

      @param key The key to insert.
      @return Pointer to the inserted key, or nullptr if key already exists
              or allocation failed.

      @note This function is marked [[nodiscard]] - check the return value
            to detect insertion failures.
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] Key * insert(const Key & key)
  {
    check_resize_before_insert();
    auto bucket = me()->allocate_bucket(key);
    if (bucket == nullptr)
      return nullptr;

    bucket->key = key;

    return me()->test_resize(bucket, key);
  }

  /** @brief Inserts a key into the hash table (move version).
      
      Attempts to insert a key using move semantics. If the key already
      exists, the insertion fails and nullptr is returned.

      @param key The key to insert (will be moved from on success).
      @return Pointer to the inserted key, or nullptr if key already exists
              or allocation failed.

      @note This function is marked [[nodiscard]] - check the return value
            to detect insertion failures.
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] Key * insert(Key && key)
  {
    check_resize_before_insert();
    auto bucket = me()->allocate_bucket(key);
    if (bucket == nullptr)
      return nullptr;

    std::swap(bucket->key, key);

    return me()->test_resize(bucket, bucket->key);
  }

  /** @brief Searches for a key, inserting it if not found (copy version).
      
      If the key exists, returns a pointer to it. Otherwise, inserts
      a copy of the key and returns a pointer to the newly inserted key.

      @param key The key to search for or insert.
      @return Pointer to the existing or newly inserted key, or nullptr
              if allocation failed.

      @note This function is marked [[nodiscard]] - the return value
            indicates success/failure and provides the key pointer.
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] Key * search_or_insert(const Key & key)
  {
    check_resize_before_insert();
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (bucket == nullptr)
      return nullptr;
    if (get<1>(r))
      return &bucket->key;

    bucket->key = key;

    return me()->test_resize(bucket, bucket->key);
  }

  /** @brief Searches for a key, inserting it if not found (move version).
      
      If the key exists, returns a pointer to it. Otherwise, inserts
      the key using move semantics and returns a pointer to the newly
      inserted key.

      @param key The key to search for or insert (moved from on insert).
      @return Pointer to the existing or newly inserted key, or nullptr
              if allocation failed.

      @note This function is marked [[nodiscard]] - the return value
            indicates success/failure and provides the key pointer.
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] Key * search_or_insert(Key && key)
  {
    check_resize_before_insert();
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (bucket == nullptr)
      return nullptr;
    if (get<1>(r))
      return &bucket->key;

    std::swap(bucket->key, key);

    return me()->test_resize(bucket, bucket->key);
  }

  /** @brief Searches for a key, inserting if not found, with existence flag (copy version).
      
      Similar to search_or_insert(), but also indicates whether the key
      was already present in the table.

      @param key The key to search for or insert.
      @return A pair containing:
              - first: Pointer to the key (existing or newly inserted),
                       or nullptr on failure.
              - second: true if the key was already present, false if inserted.

      @note This function is marked [[nodiscard]].
      
      @par Example:
      @code
      auto [ptr, existed] = table.contains_or_insert(key);
      if (ptr) {
          if (existed) std::cout << "Key already existed\n";
          else std::cout << "Key was inserted\n";
      }
      @endcode
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] std::pair<Key*, bool> contains_or_insert(const Key & key)
  {
    check_resize_before_insert();
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (bucket == nullptr)
      return { nullptr, false };
    if (get<1>(r))
      return { &bucket->key, true };

    bucket->key = key;
    auto key_ptr = me()->test_resize(bucket, bucket->key);

    return { key_ptr, false };
  }

  /** @brief Searches for a key, inserting if not found, with existence flag (move version).
      
      Similar to search_or_insert(), but also indicates whether the key
      was already present in the table.

      @param key The key to search for or insert (moved from on insert).
      @return A pair containing:
              - first: Pointer to the key (existing or newly inserted),
                       or nullptr on failure.
              - second: true if the key was already present, false if inserted.

      @note This function is marked [[nodiscard]].
      
      @par Complexity:
      - Average case: O(1) amortized
      - Worst case: O(n) when rehashing is triggered
  */
  [[nodiscard]] std::pair<Key*, bool> contains_or_insert(Key && key)
  {
    check_resize_before_insert();
    auto r = me()->hard_allocate_bucket(key);
    auto bucket = get<0>(r);
    if (bucket == nullptr)
      return { nullptr, false };
    if (get<1>(r))
      return { &bucket->key, true };

    std::swap(bucket->key, key);
    auto key_ptr = me()->test_resize(bucket, bucket->key);

    return { key_ptr, false };
  }

  /** @brief Alias for insert() (copy version).
      
      Provides STL-like naming for insert operations.

      @param key The key to append/insert.
      @return Pointer to the inserted key, or nullptr on failure.
      @see insert(const Key &)
  */
  [[nodiscard]] Key * append(const Key & key)
  {
    return this->insert(key);
  }

  /** @brief Alias for insert() (move version).
      
      Provides STL-like naming for insert operations.

      @param key The key to append/insert (will be moved from).
      @return Pointer to the inserted key, or nullptr on failure.
      @see insert(Key &&)
  */
  [[nodiscard]] Key * append(Key && key)
  {
    return this->insert(std::move(key));
  }

  /** @brief Checks if a key exists in the table.
      
      @param key The key to search for.
      @return true if the key exists, false otherwise.
      
      @par Complexity: O(1) average, O(n) worst case.
  */
  [[nodiscard]] constexpr bool has(const Key & key) const noexcept
  {
    return const_me()->search(key) != nullptr;
  }

  /** @brief Alias for has().
      
      @param key The key to search for.
      @return true if the key exists, false otherwise.
      @see has()
  */
  [[nodiscard]] constexpr bool contains(const Key & key) const noexcept { return has(key); }

  /** @brief Finds a key and returns a reference to it.
      
      Searches for the key and returns a reference to it. If the key
      is not found, throws an exception.

      @param key The key to find.
      @return Reference to the stored key.
      @throw std::domain_error If the key is not found.
      
      @par Complexity: O(1) average, O(n) worst case.
  */
  [[nodiscard]] Key & find(const Key & key)
  {
    auto key_ptr = me()->search(key);
    ah_domain_error_if(key_ptr == nullptr)
      << "Key not found in hash";

    return *key_ptr;
  }

  /** @brief Finds a key and returns a const reference to it.
      
      Const version of find().

      @param key The key to find.
      @return Const reference to the stored key.
      @throw std::domain_error If the key is not found.
      @see find(const Key &)
  */
  [[nodiscard]] const Key & find(const Key & key) const
  {
    return const_cast<OhashCommon*>(this)->find(key);
  }

  /** @brief Subscript operator for const access.
      
      Returns a const reference to the key. Equivalent to find().

      @param key The key to access.
      @return Const reference to the stored key.
      @throw std::domain_error If the key is not found.
  */
  const Key & operator [] (const Key & key) const
  {
    return find(key);
  }

  /** @brief Subscript operator with insertion semantics.
      
      Returns a reference to the key. If the key doesn't exist,
      it is inserted first.

      @param key The key to access or insert.
      @return Reference to the stored key.
  */
  const Key & operator [] (const Key & key)
  {
    return *search_or_insert(key);
  }

  /** @brief Removes an entry by pointer.
      
      Removes the entry pointed to by the given pointer. The pointer
      must have been obtained from this table (via insert, search, etc.).
      
      If automatic resizing is enabled and the load factor drops below
      the lower threshold, the table is shrunk.

      @param key Pointer to the key to remove (must be in this table).
      
      @pre key must point to a valid entry in this table.
      @post The entry is removed and size() decreases by 1.
      
      @par Complexity: O(1) amortized.
  */
  void remove_ptr(Key * key)
  {
    auto bucket = HashTbl::key_to_bucket(key);
    me()->deallocate_bucket(bucket);

    if (me()->with_resize and me()->current_alpha() < me()->lower_alpha)
      resize(Primes::next_prime(me()->len/2 + 1));
  }

  /** @brief Resizes the hash table to a new capacity.
      
      Changes the table capacity to the specified size and rehashes
      all entries. The new size must be large enough to hold all
      current entries plus at least one empty sentinel bucket.

      @param new_size The new capacity (should be a prime number).
      @return The actual new capacity (may differ from requested if
              new_size was 0 or equal to current capacity).
      @throw std::overflow_error If new_size is too small for current entries.
      
      @par Complexity: O(n) where n is the number of entries.
  */
  size_t resize(size_t new_size)
  {
    assert(me()->len > 0);

    if (new_size == 0 or new_size == me()->len)
      return me()->len;

    // new_size must leave at least one EMPTY bucket as sentinel
    ah_overflow_error_if(me()->N >= new_size)
      << "New size is not enough for current number of entries";

    auto * new_table = new typename HashTbl::Bucket[new_size];
    typename HashTbl::Bucket * old_table = me()->table;
    const size_t old_len = me()->len;

    int old_N = me()->N;

    me()->table = new_table;
    me()->len   = new_size;
    me()->N     = 0;

    for (int i = 0, c = 0; i < old_len and c < old_N; ++i)
      if (old_table[i].status == HashTbl::BUSY)
        {
          Key & key = old_table[i].key;
          auto bucket = me()->allocate_bucket(key);
          std::swap(bucket->key, key);
          ++c;
        }

    assert(old_N == me()->N);

    delete [] old_table;

    return me()->len;
  }

  /** @brief Rebuilds the hash table with the same capacity.
      
      Creates a new internal table with the same capacity and
      re-inserts all entries. This can help improve performance
      after many deletions by eliminating deleted bucket markers.
      
      @par Complexity: O(n) where n is the number of entries.
  */
  void rehash()
  {
    auto new_table = new typename HashTbl::Bucket [me()->len];
    auto old_table = me()->table;

    int old_N = me()->N;

    me()->table = new_table;
    me()->N     = 0;

    for (int i = 0, c = 0; i < me()->len and c < old_N; ++i)
      if (old_table[i].status == HashTbl::BUSY)
        {
          Key & key = old_table[i].key;
          auto bucket = me()->allocate_bucket(key);
          std::swap(bucket->key, key);
          ++c;
        }

    assert(old_N == me()->N);

    delete [] old_table;
  }

  /** @brief Clears all entries and resets to default capacity.
      
      Removes all entries and resets the table to its default
      initial capacity (Primes::DefaultPrime).
      
      @post size() == 0
      @post capacity() == Primes::DefaultPrime
  */
  void empty()
  {
    delete [] me()->table;
    me()->N = 0;
    me()->len = Primes::DefaultPrime;
    me()->table = new typename HashTbl::Bucket [me()->len];
  }

  /** @brief Returns the number of entries in the table.
      @return The number of stored keys.
  */
  [[nodiscard]] constexpr size_t size() const noexcept { return const_me()->N; }

  /** @brief Checks if the table is empty.
      @return true if size() == 0, false otherwise.
  */
  [[nodiscard]] constexpr bool is_empty() const noexcept { return const_me()->N == 0; }

  /** @brief Returns the current capacity of the table.
      @return The number of buckets in the internal array.
  */
  [[nodiscard]] constexpr size_t capacity() const noexcept { return const_me()->len; }

  /** @brief Bidirectional iterator for traversing hash table entries.
      
      This iterator provides sequential access to all BUSY (occupied)
      buckets in the hash table. Iteration order is based on internal
      bucket positions and is not related to key ordering.
      
      @par Supported Operations:
      - reset_first() / reset_last() - Position at first/last entry
      - next() / prev() - Move forward/backward
      - get_curr() - Access current entry
      - has_curr() - Check if positioned at valid entry
      - del() - Delete current entry (invalidates iterator position)
      
      @par Example:
      @code
      for (auto it = table.get_it(); it.has_curr(); it.next())
          std::cout << it.get_curr() << "\n";
      @endcode
      
      @warning Modifying the table (except via del()) during iteration
               invalidates the iterator.
  */
  class Iterator
  {
    HashTbl * table_ptr = nullptr;  ///< Pointer to the hash table being iterated.
    long curr_idx          = 0;     ///< Current bucket index.
    long ordinal           = 0;     ///< Logical position (0-based count of visited entries).

    /** @brief Validates iterator state (debug only).
        @return true if iterator state is consistent.
    */
    bool check() const noexcept
    {
      assert(table_ptr != nullptr);
      return curr_idx >= 0 and curr_idx < static_cast<long>(table_ptr->len) and
        ordinal >= -1 and ordinal <= static_cast<long>(table_ptr->size());
    }

    /** @brief Advances to next entry without bounds checking.
        @pre Iterator must be at a valid position.
    */
    void locate_next_available_entry_ne() noexcept
    {
      assert(check());
      if (++ordinal == table_ptr->size())
        return;

      while (table_ptr->table[++curr_idx].status != HashTbl::BUSY);
    }

    /** @brief Moves to previous entry without bounds checking.
        @pre Iterator must be at a valid position.
    */
    void locate_prev_available_entry_ne()
    {
      assert(check());
      if (--ordinal == -1)
        return;

      while (table_ptr->table[--curr_idx].status != HashTbl::BUSY);
    }

    /** @brief Advances to next entry with bounds checking.
        @throw std::overflow_error If already at end.
    */
    void locate_next_available_entry()
    {
      ah_overflow_error_if(ordinal >= table_ptr->size())
        << "Iterator next() overflow";
      locate_next_available_entry_ne();
    }

    /** @brief Moves to previous entry with bounds checking.
        @throw std::underflow_error If already at beginning.
    */
    void locate_prev_available_entry()
    {
      ah_underflow_error_if(ordinal <= -1)
        << "Iterator next() underflow";
      locate_prev_available_entry_ne();
    }

  public:

    /** @brief Positions the iterator at the first entry.
        
        Resets the iterator to point to the first BUSY bucket in the table.
        If the table is empty, the iterator is positioned at end().
    */
    void reset_first() noexcept
    {
      assert(table_ptr != nullptr);
      curr_idx = 0;
      ordinal = -1;
      if (table_ptr->is_empty())
        {
          curr_idx = table_ptr->len;
          ordinal = 0;
          return;
        }

      while (table_ptr->table[curr_idx].status != HashTbl::BUSY)
        ++curr_idx;

      ordinal = 0;
      assert(check());
    }

    /** @brief Positions the iterator at the last entry.
        
        Resets the iterator to point to the last BUSY bucket in the table.
        If the table is empty, the iterator is positioned before begin().
    */
    void reset_last() noexcept
    {
      assert(table_ptr != nullptr);
      if (table_ptr->is_empty())
        {
          curr_idx = -1;
          ordinal = -1;
          return;
        }

      curr_idx = table_ptr->len - 1;
      while (table_ptr->table[curr_idx].status != HashTbl::BUSY)
        --curr_idx;

      ordinal = table_ptr->size() - 1;
      assert(check());
    }

    /** @brief Positions the iterator at the end (past-the-last element).
        
        After calling this, has_curr() will return false.
    */
    void end() noexcept
    {
      put_itor_at_the_end(*this);
    }

    /** @brief Returns the current logical position.
        @return The 0-based ordinal position of the current entry.
    */
    [[nodiscard]] constexpr long get_pos() const noexcept { return ordinal; }

    /** @brief Returns a reference to the current entry without checking.
        @return Reference to the current key.
        @pre has_curr() must be true.
    */
    Key & get_curr_ne() noexcept
    {
      assert(table_ptr->table[curr_idx].status == HashTbl::BUSY);
      return table_ptr->table[curr_idx].key;
    }

    /** @brief Returns a const reference to the current entry without checking.
        @return Const reference to the current key.
        @pre has_curr() must be true.
    */
    const Key & get_curr_ne() const noexcept
    {
      return const_cast<Iterator*>(this)->get_curr_ne();
    }

    /** @brief Returns a reference to the current entry with bounds checking.
        @return Reference to the current key.
        @throw std::overflow_error If iterator is past the end.
        @throw std::underflow_error If iterator is before the beginning.
    */
    Key & get_curr()
    {
      ah_overflow_error_if(ordinal == table_ptr->size())
        << "O hash::Iterator next() overflow";

      ah_underflow_error_if(ordinal == -1)
        << "O hash::Iterator next() underflow";

      assert(check());

      return get_curr_ne();
    }

    /** @brief Returns a const reference to the current entry with bounds checking.
        @return Const reference to the current key.
        @throw std::overflow_error If iterator is past the end.
        @throw std::underflow_error If iterator is before the beginning.
    */
    const Key & get_curr() const
    {
      return const_cast<Iterator*>(this)->get_curr();
    }

    /** @brief Checks if the iterator is at a valid position.
        @return true if pointing to a valid entry, false otherwise.
    */
    bool has_curr() const noexcept
    {
      assert(table_ptr != nullptr);
      if (table_ptr->is_empty())
        return false;
      return ordinal >= 0 and ordinal < table_ptr->size();
    }

    /** @brief Checks if the iterator is at the last entry.
        @return true if at the last entry, false otherwise.
    */
    [[nodiscard]] constexpr bool is_last() const noexcept { return ordinal == table_ptr->size() - 1; }

    /** @brief Advances to the next entry with bounds checking.
        @throw std::overflow_error If already at end.
    */
    void next() { locate_next_available_entry(); }

    /** @brief Advances to the next entry without bounds checking.
        @pre has_curr() must be true.
    */
    void next_ne() noexcept { locate_next_available_entry_ne(); }

    /** @brief Moves to the previous entry without bounds checking.
        @pre ordinal > 0.
    */
    void prev_ne() { locate_prev_available_entry_ne(); }

    /** @brief Moves to the previous entry with bounds checking.
        @throw std::underflow_error If already at beginning.
    */
    void prev() { locate_prev_available_entry(); }

    /** @brief Constructs an iterator positioned at the first entry.
        @param table The hash table to iterate over.
    */
    Iterator(const HashTbl & table) noexcept
      : table_ptr(&const_cast<HashTbl&>(table)), curr_idx(0), ordinal(-1)
    {
      reset_first();
    }

    /** @brief Deletes the current entry and advances to the next.
        
        Removes the current entry from the table. After deletion,
        the iterator is positioned at the next entry (if any).
        
        @throw std::overflow_error If not at a valid position.
        
        @warning After del(), the logical positions of subsequent
                 entries change.
    */
    void del()
    {
      ah_overflow_error_if(not has_curr())
        << "Overflow in del() of iterator";

      table_ptr->deallocate_bucket(&table_ptr->table[curr_idx]);
      if (table_ptr->size() == 0)
        return;

      while (table_ptr->table[++curr_idx].status != HashTbl::BUSY);
    }
  };

  /** @brief Returns a list containing all keys in the table.
      
      Creates and returns a DynList containing copies of all keys
      stored in the hash table.
      
      @return DynList<Key> containing all keys.
      
      @par Complexity: O(n) where n is the number of entries.
  */
  [[nodiscard]] DynList<Key> keys() const
  {
    return const_me()->template maps<Key> ([] (const Key & key) { return key; });
  }

  /** @brief Alias for keys().
      @return DynList<Key> containing all keys.
      @see keys()
  */
  [[nodiscard]] auto items() const { return keys(); }

  /** @brief Statistics about hash table performance.
      
      Contains metrics about bucket utilization and collision
      distribution, useful for analyzing hash function quality
      and table sizing decisions.
  */
  struct Stats
  {
    size_t num_busy = 0;     ///< Number of occupied buckets.
    size_t num_deleted = 0;  ///< Number of buckets marked as deleted.
    size_t num_empty = 0;    ///< Number of empty buckets.
    DynArray<size_t> lens;   ///< Distribution of probe sequence lengths.
    float avg = 0;           ///< Average probe sequence length.
    float var = 0;           ///< Variance of probe sequence lengths.
    size_t max_len = 0;      ///< Maximum probe sequence length.

    /** @brief Default constructor initializing max_len to minimum. */
    Stats() : max_len(std::numeric_limits<size_t>::min())
    {
      assert(lens.size() == 0);
    }
  };

  /** @brief Prints statistics to standard output.
      
      Displays formatted statistics including capacity, size,
      bucket states, load factor, and probe length distribution.
      
      @param stats Statistics object to print.
  */
  void print_stats(const Stats & stats) const
  {
    std::cout << "M             = " << this->capacity() << std::endl
              << "N             = " << this->size() << std::endl
              << "busy slots    = " << stats.num_busy << std::endl
              << "deleted slots = " << stats.num_deleted << std::endl
              << "empty slots   = " << stats.num_empty << std::endl
              << "alpha         = " << const_me()->current_alpha() << std::endl
              << "max length    = " << stats.max_len << std::endl;
    for (int i = 0; i < stats.lens.size(); ++i)
      std::cout << "    " << i << " = " << stats.lens(i) << std::endl;
  }

  /** @brief Returns the current load factor (alpha).
      
      The load factor is the ratio of stored entries to table capacity.
      A higher load factor means more efficient memory usage but
      potentially longer probe sequences.
      
      @return The load factor as N/capacity.
  */
  [[nodiscard]] constexpr float current_alpha() const noexcept 
  {
    return 1.0f*const_me()->N/const_me()->len; 
  }
};


/** @brief CRTP mixin providing statistics and configuration for chained hash tables.
    
    This class provides statistics gathering and load factor configuration
    for hash tables using separate chaining (linked lists per bucket).
    It uses CRTP for static polymorphism.
    
    @tparam HashTbl The derived hash table class.
    
    @par Provided Operations:
    - stats() - Compute chain length statistics
    - print_stats() - Display statistics
    - set_lower_alpha() / set_upper_alpha() - Configure load factor thresholds
    - get_lower_alpha() / get_upper_alpha() - Query thresholds
    
    @see LhashTable
    @ingroup Hash
*/
template <class HashTbl>
class HashStats
{
  /** @brief Returns a pointer to the derived class (CRTP pattern).
      @return Pointer to this object cast to HashTbl*.
  */
  HashTbl * me() { return static_cast<HashTbl*>(this); }

  /** @brief Returns a const pointer to the derived class (CRTP pattern).
      @return Const pointer to this object cast to const HashTbl*.
  */
  const HashTbl * const_me() const { return static_cast<const HashTbl*>(this); }

  float  lower_alpha;          ///< Lower load factor threshold for shrinking.
  float  upper_alpha;          ///< Upper load factor threshold for growing.
  size_t busy_slots_counter = 0;  ///< Number of buckets with at least one entry.
  size_t N = 0;                ///< Total number of entries.

public:

  /** @brief Statistics about chain length distribution.
      
      Contains metrics about how entries are distributed across
      chains (buckets), useful for analyzing hash function quality.
  */
  struct Stats
  {
    float avg;              ///< Average chain length.
    float var;              ///< Variance of chain lengths.
    DynArray<size_t> lens;  ///< Distribution: lens[i] = number of chains with length i.
  };

private:

  /** @brief Updates chain length histogram.
      @param lens The histogram array to update.
      @param i The chain length to record.
  */
  static void update_stat_len(DynArray<size_t> & lens, size_t i)
  {
    if (lens.exist(i))
      lens(i) += 1;
    else
      lens.touch(i) = 1;
  }

public:

  /** @brief Computes statistics about chain length distribution.
      
      Iterates through all buckets to compute the distribution
      of chain lengths, along with mean and variance.
      
      @return Stats object containing chain length statistics.
      
      @par Complexity: O(n + m) where n is entries and m is capacity.
  */
  Stats stats() const
  {
    DynArray<size_t> lens;
    for (int i = 0; i < const_me()->capacity(); ++i)
      {
        Dlink & list = const_me()->table[i];
        size_t count = 0;
        for (typename HashTbl::BucketItor it(list); it.has_curr(); 
             it.next(), ++count)
          ;

        update_stat_len(lens, count);
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
    stats.avg = avg;
    stats.var = var;
    std::swap(lens, stats.lens);

    return stats;
  }

  /** @brief Prints statistics to standard output.
      
      Displays formatted statistics including capacity, size,
      busy slots, average chain length, standard deviation,
      load factor, and chain length distribution.
      
      @param stats Statistics object to print.
  */
  void print_stats(const Stats & stats) const
  {
    std::cout << "M          = " << const_me()->capacity() << std::endl
         << "N          = " << const_me()->size() << std::endl
         << "busy slots = " << busy_slots_counter << std::endl
         << "Average    = " << stats.avg << std::endl
         << "Desv       = " << sqrt(stats.var) << std::endl
         << "alpha      = " << (1.0*const_me()->size())/const_me()->capacity() 
         << std::endl;
    for (int i = 0; i < stats.lens.size(); ++i)
      std::cout << "    " << i << " = " << stats.lens(i) << std::endl;
  }

  /** @brief Sets the upper load factor threshold.
      
      When the load factor exceeds this value, the table
      automatically grows (if auto-resize is enabled).
      
      @param __upper_alpha New upper threshold.
      @throw std::domain_error If __upper_alpha <= lower_alpha.
  */
  void set_upper_alpha(float __upper_alpha)
  {
    ah_domain_error_if(__upper_alpha <= lower_alpha)
      << "upper_alpha lower than lower_alpha";

    upper_alpha = __upper_alpha;
  }

  /** @brief Sets the lower load factor threshold.
      
      When the load factor drops below this value after a removal,
      the table automatically shrinks (if auto-resize is enabled).
      
      @param __lower_alpha New lower threshold.
      @throw std::domain_error If __lower_alpha >= upper_alpha.
  */
  void set_lower_alpha(float __lower_alpha)
  {
    ah_domain_error_if(__lower_alpha >= upper_alpha)
      << "lower_alpha greater than upper_alpha";

    lower_alpha = __lower_alpha;
  }

  /** @brief Returns the lower load factor threshold.
      @return The lower alpha threshold.
  */
  [[nodiscard]] constexpr float get_lower_alpha() const noexcept { return lower_alpha; }

  /** @brief Returns the upper load factor threshold.
      @return The upper alpha threshold.
  */
  [[nodiscard]] constexpr float get_upper_alpha() const noexcept { return upper_alpha; }
};


# endif // HASHDRY_H
