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

#ifndef TPL_DYNMAPOHASH_H
#define TPL_DYNMAPOHASH_H

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <tpl_olhash.H>
#include <tpl_odhash.H>

namespace Aleph
{

/** @brief Open addressing hash map for key-value pairs.
 *
 *  MapOpenHash<Key, Data, Cmp, HashTable> implements an associative container
 *  that maps keys to values using open addressing hash tables. It provides
 *  O(1) average-case complexity for insert, search, and delete operations.
 *
 *  ## Features
 *
 *  - **Open addressing**: Uses either linear probing (OLhashTable) or
 *    double hashing (ODhashTable) for collision resolution
 *  - **Automatic resizing**: Table grows/shrinks based on load factor thresholds
 *  - **STL-compatible interface**: Supports operator[], find(), contains(), etc.
 *  - **Functional interface**: Inherits traverse(), maps(), filter(), foldl(), etc.
 *
 *  ## Example Usage
 *
 *  @code
 *  // Create a map with string keys and int values
 *  MapODhash<std::string, int> ages;
 *
 *  // Insert elements
 *  ages.insert("Alice", 30);
 *  ages["Bob"] = 25;
 *
 *  // Search and access
 *  if (ages.has("Alice"))
 *    std::cout << "Alice's age: " << ages["Alice"] << std::endl;
 *
 *  // Iterate
 *  ages.traverse([](auto& pair) {
 *    std::cout << pair.first << ": " << pair.second << std::endl;
 *    return true;
 *  });
 *
 *  // Remove
 *  ages.remove("Bob");
 *  @endcode
 *
 *  ## Hash Table Variants
 *
 *  - **MapODhash**: Uses double hashing (better distribution, more complex)
 *  - **MapOLhash**: Uses linear probing (simpler, better cache locality)
 *
 *  ## Thread Safety
 *
 *  This class is not thread-safe. External synchronization is required
 *  for concurrent access from multiple threads.
 *
 *  @tparam Key Type of keys. Must be hashable and equality-comparable.
 *  @tparam Data Type of mapped values.
 *  @tparam Cmp Equality comparator for keys (default: Aleph::equal_to<Key>).
 *  @tparam HashTable Underlying hash table type (default: ODhashTable).
 *
 *  @see MapODhash Double hashing variant
 *  @see MapOLhash Linear probing variant
 *  @see ODhashTable Underlying double hashing table
 *  @see OLhashTable Underlying linear probing table
 *
 *  @ingroup Hash
 */
template <typename Key, typename Data,
          class Cmp = Aleph::equal_to<Key>,
          template <typename, class> class HashTable = ODhashTable>
struct MapOpenHash :
    public HashTable<std::pair<Key, Data>, Dft_Pair_Cmp<Key, Data, Cmp>>
{
  /// @brief The key-value pair type stored in the map.
  using Pair = std::pair<Key, Data>;

  /// @brief The base hash table type.
  using Base = HashTable<std::pair<Key, Data>, Dft_Pair_Cmp<Key, Data, Cmp>>;

  using Base::Base;
  using Base::insert;

  /// @brief Function type for hash functions.
  using Hash_Fct = std::function<size_t(const Key &)>;

  /// @brief Function pointer type for hash functions.
  using Hash_Fct_Ptr = size_t (*) (const Key &);

  /// @brief The type of keys in the map.
  using Key_Type = Key;

  /// @brief The type of mapped values.
  using Data_Type = Data;

  /// @brief Alias for Data_Type (compatibility with other containers).
  using Value_Type = Data;

  /// @brief The item type stored in the map (key-value pair).
  using Item_Type = Pair;

  /// @brief Self-reference type for generic programming.
  using Set_Type = MapOpenHash;

  /** @brief Construct a map with specified parameters.
   *
   *  @param[in] len Initial table size (default: DefaultPrime).
   *  @param[in] first_hash_fct Primary hash function.
   *  @param[in] second_hash_fct Secondary hash function (for double hashing).
   *  @param[in] cmp Equality comparator for keys.
   *  @param[in] lower_alpha Lower load factor threshold for shrinking.
   *  @param[in] upper_alpha Upper load factor threshold for growing.
   *  @param[in] with_resize Enable automatic resizing.
   *
   *  @throw std::bad_alloc If memory allocation fails.
   */
  MapOpenHash(size_t len                   = Primes::DefaultPrime,
              Hash_Fct_Ptr first_hash_fct  = dft_hash_fct<Key>,
              Hash_Fct_Ptr second_hash_fct = snd_hash_fct<Key>,
              Cmp  cmp                     = Cmp(),
              float lower_alpha            = hash_default_lower_alpha,
              float upper_alpha            = hash_default_upper_alpha,
              bool with_resize             = true)
    : Base(len, std::bind(map_hash_fct<Key, Data, Hash_Fct>,
                          first_hash_fct, std::placeholders::_1),
           std::bind(map_hash_fct<Key, Data, Hash_Fct>,
                     second_hash_fct, std::placeholders::_1),
           Dft_Pair_Cmp<Key, Data, Cmp>(cmp),
           lower_alpha, upper_alpha, with_resize) {}

  /** @brief Convert a key pointer to its containing pair pointer.
   *
   *  Given a pointer to the key member of a pair, returns a pointer
   *  to the enclosing pair. This relies on the fact that the key is
   *  the first member of std::pair.
   *
   *  @param[in] ptr Pointer to a key within a pair.
   *  @return Pointer to the containing pair.
   *
   *  @warning Only valid for pointers obtained from pairs in this map.
   */
  [[nodiscard]] static Pair * key_to_pair(Key * ptr) noexcept
  {
    return reinterpret_cast<Pair*>(ptr);
  }

  [[nodiscard]] static const Pair * key_to_pair(const Key * ptr) noexcept
  {
    return reinterpret_cast<const Pair*>(ptr);
  }

  /** @brief Convert a data pointer to its containing pair pointer.
   *
   *  Given a pointer to the data (second) member of a pair, returns
   *  a pointer to the enclosing pair.
   *
   *  @param[in] ptr Pointer to data within a pair.
   *  @return Pointer to the containing pair.
   *
   *  @warning Only valid for pointers obtained from pairs in this map.
   */
  [[nodiscard]] static Pair * data_to_pair(Data * ptr) noexcept
  {
    return reinterpret_cast<Pair*>(
        reinterpret_cast<char*>(ptr) - offsetof(Pair, second));
  }

  [[nodiscard]] static const Pair * data_to_pair(const Data * ptr) noexcept
  {
    return reinterpret_cast<const Pair*>(
        reinterpret_cast<const char*>(ptr) - offsetof(Pair, second));
  }

  /** @brief Get the data associated with a key by reference.
   *
   *  Given a reference to a key that is stored in the map, returns
   *  a reference to its associated data.
   *
   *  @param[in] key Reference to a key within a stored pair.
   *  @return Reference to the associated data.
   *
   *  @warning Only valid for keys obtained from pairs in this map.
   */
  [[nodiscard]] static Data & get_data(Key & key) noexcept
  {
    return key_to_pair(&key)->second;
  }

  [[nodiscard]] static const Data & get_data(const Key & key) noexcept
  {
    return key_to_pair(&key)->second;
  }

  /** @brief Get the key associated with a data pointer.
   *
   *  Given a pointer to data stored in the map, returns a reference
   *  to its associated key.
   *
   *  @param[in] data_ptr Pointer to data within a stored pair.
   *  @return Reference to the associated key.
   *
   *  @warning Only valid for data pointers obtained from this map.
   */
  [[nodiscard]] static const Key & get_key(Data * data_ptr) noexcept
  {
    return data_to_pair(data_ptr)->first;
  }

  [[nodiscard]] static const Key & get_key(const Data * data_ptr) noexcept
  {
    return data_to_pair(data_ptr)->first;
  }

  /** @brief Insert a key-value pair (copy semantics).
   *
   *  Inserts a new key-value pair into the map. If the key already
   *  exists, no insertion occurs.
   *
   *  @param[in] key The key to insert.
   *  @param[in] data The value to associate with the key.
   *  @return Pointer to the inserted pair, or nullptr if key exists.
   *
   *  @throw std::bad_alloc If memory allocation fails during resize.
   */
  Pair * insert(const Key & key, const Data & data)
  {
    return this->Base::insert(Pair(key, data));
  }

  /** @brief Insert a key-value pair (move data).
   *
   *  @param[in] key The key to insert.
   *  @param[in] data The value to move into the map.
   *  @return Pointer to the inserted pair, or nullptr if key exists.
   */
  Pair * insert(const Key & key, Data && data)
  {
    return this->Base::insert(Pair(key, std::forward<Data>(data)));
  }

  /** @brief Insert a key-value pair (move both).
   *
   *  @param[in] key The key to move into the map.
   *  @param[in] data The value to move into the map.
   *  @return Pointer to the inserted pair, or nullptr if key exists.
   */
  Pair * insert(Key && key, Data && data)
  {
    return this->Base::insert(Pair(std::forward<Key>(key),
                                   std::forward<Data>(data)));
  }

  /** @brief Insert a key-value pair (move key, copy data).
   *
   *  @param[in] key The key to move into the map.
   *  @param[in] data The value to copy into the map.
   *  @return Pointer to the inserted pair, or nullptr if key exists.
   */
  Pair * insert(Key && key, const Data & data)
  {
    return this->Base::insert(Pair(std::forward<Key>(key), data));
  }

  /** @brief Search for a key in the map.
   *
   *  @param[in] key The key to search for.
   *  @return Pointer to the pair if found, nullptr otherwise.
   */
  [[nodiscard]] Pair * search(const Key & key) const noexcept
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::search() requires Data to be default-constructible");
    return this->Base::search(Pair(key, Data()));
  }

  /** @brief Search for a key in the map (move semantics).
   *
   *  @param[in] key The key to search for.
   *  @return Pointer to the pair if found, nullptr otherwise.
   */
  [[nodiscard]] Pair * search(Key && key) const noexcept
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::search() requires Data to be default-constructible");
    return this->Base::search(Pair(std::move(key), Data()));
  }

  /** @brief Check if a key exists in the map.
   *
   *  @param[in] key The key to check.
   *  @return true if the key exists, false otherwise.
   */
  [[nodiscard]] bool has(const Key & key) const noexcept
  {
    return search(key) != nullptr;
  }

  /** @brief Check if a key exists in the map (move semantics).
   *
   *  @param[in] key The key to check.
   *  @return true if the key exists, false otherwise.
   */
  [[nodiscard]] bool has(Key && key) const noexcept
  {
    return search(std::move(key)) != nullptr;
  }

  /** @brief Check if a key exists in the map.
   *
   *  Alias for has().
   *
   *  @param[in] key The key to check.
   *  @return true if the key exists, false otherwise.
   */
  [[nodiscard]] bool contains(const Key & key) const noexcept
  {
    return has(key);
  }

  /** @brief Check if a key exists in the map (move semantics).
   *
   *  @param[in] key The key to check.
   *  @return true if the key exists, false otherwise.
   */
  [[nodiscard]] bool contains(Key && key) const noexcept
  {
    return has(std::move(key));
  }

  /** @brief Find and return the value for a key.
   *
   *  @param[in] key The key to find.
   *  @return Reference to the associated value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] Data & find(const Key & key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::find() requires Data to be default-constructible");
    return Base::find(Pair(key, Data())).second;
  }

  /** @brief Find and return the value for a key (move semantics).
   *
   *  @param[in] key The key to find.
   *  @return Reference to the associated value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] Data & find(Key && key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::find() requires Data to be default-constructible");
    return Base::find(Pair(std::move(key), Data())).second;
  }

  /** @brief Find and return the value for a key (const).
   *
   *  @param[in] key The key to find.
   *  @return Const reference to the associated value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] const Data & find(const Key & key) const
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::find() requires Data to be default-constructible");
    return Base::find(Pair(key, Data())).second;
  }

  /** @brief Find and return the value for a key (const, move).
   *
   *  @param[in] key The key to find.
   *  @return Const reference to the associated value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] const Data & find(Key && key) const
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::find() requires Data to be default-constructible");
    return Base::find(Pair(std::move(key), Data())).second;
  }

  /** @brief Access or insert a value by key.
   *
   *  If the key exists, returns a reference to its value.
   *  If not, inserts a new pair with default-constructed value
   *  and returns a reference to it.
   *
   *  @param[in] key The key to access or insert.
   *  @return Reference to the value associated with the key.
   *
   *  @throw std::bad_alloc If insertion requires memory allocation.
   */
  [[nodiscard]] Data & operator [] (const Key & key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::operator[] requires Data to be default-constructible");
    return this->search_or_insert(Pair(key, Data()))->second;
  }

  /** @brief Access value by key (const version).
   *
   *  @param[in] key The key to access.
   *  @return Const reference to the value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] const Data & operator [] (const Key & key) const
  {
    return this->find(key);
  }

  /** @brief Access or insert a value by key (move semantics).
   *
   *  @param[in] key The key to access or insert.
   *  @return Reference to the value associated with the key.
   */
  [[nodiscard]] Data & operator [] (Key && key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::operator[] requires Data to be default-constructible");
    return this->search_or_insert(Pair(std::move(key), Data()))->second;
  }

  /** @brief Access value by key (const, move).
   *
   *  @param[in] key The key to access.
   *  @return Const reference to the value.
   *
   *  @throw std::domain_error If the key is not found.
   */
  [[nodiscard]] const Data & operator [] (Key && key) const
  {
    return this->find(std::move(key));
  }

  /** @brief Remove an entry by its data pointer.
   *
   *  Removes the key-value pair that contains the given data pointer.
   *
   *  @param[in] data Reference to data within a stored pair.
   *
   *  @warning Only valid for data references obtained from this map.
   */
  void remove_by_data(Data & data)
  {
    Base::remove_ptr(data_to_pair(&data));
  }

  /** @brief Remove an entry by key.
   *
   *  @param[in] key The key to remove.
   *
   *  @throw std::domain_error If the key is not found.
   */
  void remove(const Key & key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::remove() requires Data to be default-constructible");
    Base::remove(Pair(key, Data()));
  }

  /** @brief Remove an entry by key (move semantics).
   *
   *  @param[in] key The key to remove.
   *
   *  @throw std::domain_error If the key is not found.
   */
  void remove(Key && key)
  {
    static_assert(std::is_default_constructible<Data>::value,
                  "MapOpenHash::remove() requires Data to be default-constructible");
    Base::remove(Pair(std::move(key), Data()));
  }

  /// @brief Iterator type for traversing the map.
  using Iterator = typename Base::Iterator;

  /** @brief Get a list of all keys in the map.
   *
   *  @return DynList containing copies of all keys.
   */
  [[nodiscard]] DynList<Key> keys() const
  {
    return this->template maps<Key>([] (auto p) { return p.first; });
  }

  /** @brief Get a list of all values in the map.
   *
   *  @return DynList containing copies of all values.
   */
  [[nodiscard]] DynList<Data> values() const
  {
    return this->template maps<Data>([] (auto p) { return p.second; });
  }

  /** @brief Get a list of pointers to all values.
   *
   *  @return DynList containing pointers to all values in the map.
   *
   *  @note The pointers remain valid until the map is modified.
   */
  [[nodiscard]] DynList<Data*> values_ptr()
  {
    DynList<Data*> ret;
    for (Iterator it(*this); it.has_curr(); it.next_ne())
      ret.append(&it.get_curr().second);
    return ret;
  }

  /** @brief Get a list of pointers to all pairs.
   *
   *  @return DynList containing pointers to all pairs in the map.
   *
   *  @note The pointers remain valid until the map is modified.
   */
  [[nodiscard]] DynList<Pair*> items_ptr()
  {
    DynList<Pair*> ret;
    for (Iterator it(*this); it.has_curr(); it.next_ne())
      ret.append(&it.get_curr());
    return ret;
  }
};


/** @brief Open addressing hash map using linear probing.
 *
 *  MapOLhash is a type alias for MapOpenHash using OLhashTable as the
 *  underlying storage. Linear probing offers excellent cache locality
 *  but may suffer from clustering with certain hash functions.
 *
 *  @tparam Key Type of keys.
 *  @tparam Data Type of mapped values.
 *  @tparam Cmp Equality comparator for keys.
 *
 *  @see MapOpenHash Base template
 *  @see MapODhash Double hashing variant
 *
 *  @ingroup Hash
 */
template <typename Key, typename Data, class Cmp = Aleph::equal_to<Key>>
struct MapOLhash : public MapOpenHash<Key, Data, Cmp, OLhashTable>
{
  using MapOpenHash<Key, Data, Cmp, OLhashTable>::MapOpenHash;
};


/** @brief Open addressing hash map using double hashing.
 *
 *  MapODhash is a type alias for MapOpenHash using ODhashTable as the
 *  underlying storage. Double hashing provides better distribution and
 *  avoids clustering at the cost of slightly more computation.
 *
 *  @tparam Key Type of keys.
 *  @tparam Data Type of mapped values.
 *  @tparam Cmp Equality comparator for keys.
 *
 *  @see MapOpenHash Base template
 *  @see MapOLhash Linear probing variant
 *
 *  @ingroup Hash
 */
template <typename Key, typename Data, class Cmp = Aleph::equal_to<Key>>
struct MapODhash : public MapOpenHash<Key, Data, Cmp, ODhashTable>
{
  using MapOpenHash<Key, Data, Cmp, ODhashTable>::MapOpenHash;
};

} // end namespace Aleph

#endif // TPL_DYNMAPOHASH_H