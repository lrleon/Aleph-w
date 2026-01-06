/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2025 Leandro Rabindranath Leon

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

/** @file ah-mapping.H
 *  @brief Bidirectional mapping between two types.
 *
 *  Provides AHMapping for maintaining bijective mappings where both
 *  forward (A→B) and reverse (B→A) lookups are efficient. Uses
 *  tree-based storage for O(log n) operations.
 *
 *  @ingroup Utilidades
 *  @author Leandro Rabindranath León
 */

#ifndef AH_MAPPING_H
#define AH_MAPPING_H

#include <ah-errors.H>
#include <tpl_dynMapTree.H>

namespace Aleph {

/**
 * @class AHMapping
 * @brief A generic key-value mapping container with inverse operation support.
 *
 * This class provides a dictionary-like interface with the ability to create
 * an inverse mapping where values become keys and vice versa. It's implemented
 * using a balanced binary search tree (DynMapTree) internally.
 *
 * @tparam Key The type of the keys in the mapping
 * @tparam ValueType The type of the values in the mapping
 *
 * @note The ValueType must be copy-constructible and have operator< defined
 *       for the inverse() operation to work correctly.
 *
 * @ingroup Containers
 */
template<typename Key, typename ValueType>
class AHMapping
{
  DynMapTree<Key, ValueType> tbl;

public:
  using key_type = Key;                   ///< Type of the keys
  using mapped_type = ValueType;          ///< Type of the mapped values
  using value_type = std::pair<const Key, ValueType>; ///< Type of the key-value pairs
  using size_type = size_t;               ///< Type for size-related operations

  /**
   * @brief Default constructor. Creates an empty mapping.
   */
  AHMapping() = default;

  /**
   * @brief Copy constructor.
   * @param other The mapping to copy from
   */
  AHMapping(const AHMapping &other) = default;

  /**
   * @brief Move constructor.
   * @param other The mapping to move from
   */
  AHMapping(AHMapping &&other) noexcept = default;

  /**
   * @brief Constructs a mapping from a list of key-value pairs.
   *
   * @param key The first key
   * @param value The first value
   * @param pairs Additional key-value pairs (must be in pairs)
   *
   * @note The number of arguments must be even (key-value pairs)
   */
  template<typename ... Pairs>
  AHMapping(const Key &key, const ValueType &value, Pairs ... pairs)
    : AHMapping(pairs...)
  {
    tbl.insert(key, value);
  }

  /**
   * @brief Copy assignment operator.
   * @param other The mapping to copy from
   * @return Reference to this mapping
   */
  AHMapping& operator=(const AHMapping &other) = default;

  /**
   * @brief Move assignment operator.
   * @param other The mapping to move from
   * @return Reference to this mapping
   */
  AHMapping& operator=(AHMapping &&other) noexcept = default;

  /**
   * @brief Inserts or updates a key-value pair in the mapping.
   * @param key The key to insert/update
   * @param value The value to associate with the key
   */
  void insert(const Key &key, const ValueType &value)
  {
    tbl.insert(key, value);
  }

  /**
   * @brief Accesses the value associated with the given key.
   * @param key The key to look up
   * @return Const reference to the associated value
   * @throw std::domain_error if the key is not found
   */
  const ValueType& operator[](const Key &key) const
  {
    auto ptr = tbl.search(key);
    ah_domain_error_if(ptr == nullptr) << "Key not found: " << key;
    return ptr->second;
  }

  /**
   * @brief Accesses the value associated with the given key.
   * @param key The key to look up
   * @return Reference to the associated value (creates if doesn't exist)
   * @note If key doesn't exist, inserts a default-constructed value
   */
  ValueType& operator[](const Key &key)
  {
    return tbl[key];
  }

  /**
   * @brief Creates a new mapping with keys and values swapped.
   * @return A new mapping where original values are keys and vice versa
   * @note If the original mapping has duplicate values, the last inserted
   *       key-value pair will overwrite previous ones in the inverse.
   */
  AHMapping<ValueType, Key> inverse() const
  {
    AHMapping<ValueType, Key> ret;
    for (auto it = tbl.get_it(); it.has_curr(); it.next())
    {
      auto curr = it.get_curr();
      ret.insert(curr.second, curr.first);
    }
    return ret;
  }

  /**
   * @brief Removes the key-value pair with the given key.
   * @param key The key to remove
   * @return true if the key was found and removed, false otherwise
   */
  bool remove(const Key &key) noexcept
  {
    try {
      return tbl.remove(key);
    } catch (...) {
      return false;
    }
  }

  /**
   * @brief Checks if a key exists in the mapping.
   * @param key The key to check
   * @return true if the key exists, false otherwise
   */
  bool valid_key(const Key &key) const noexcept
  {
    return tbl.has(key);
  }

  /**
   * @brief Gets all keys in the mapping.
   * @return A list of all keys
   */
  DynList<Key> keys() const
  {
    return tbl.keys();
  }

  /**
   * @brief Gets all values in the mapping.
   * @return A list of all values
   */
  DynList<ValueType> values() const
  {
    DynList<ValueType> result;
    for (auto it = tbl.get_it(); it.has_curr(); it.next())
      result.append(it.get_curr().second);
    return result;
  }

  /**
   * @brief Gets the number of key-value pairs in the mapping.
   * @return The number of elements
   */
  size_t size() const noexcept
  {
    return tbl.size();
  }

  /**
   * @brief Checks if the mapping is empty.
   * @return true if the mapping is empty, false otherwise
   */
  bool empty() const noexcept
  {
    return size() == 0;
  }

  /**
   * @brief Removes all key-value pairs from the mapping.
   */
  void clear() noexcept
  {
    tbl.empty();
  }

  /**
   * @brief Checks if the mapping contains a specific value.
   * @param value The value to search for
   * @return true if the value is found, false otherwise
   */
  bool contains_value(const ValueType &value) const
  {
    for (auto it = tbl.get_it(); it.has_curr(); it.next())
      if (it.get_curr().second == value)
        return true;
    return false;
  }

  /**
   * @brief Applies a function to each key-value pair.
   * @tparam F Function type with signature void(const Key&, const ValueType&)
   * @param f The function to apply
   */
  template <typename F>
  void for_each(F f) const
  {
    static_assert(std::is_invocable_v<F, const Key&, const ValueType&>,
                 "Function must take (const Key&, const ValueType&) parameters");
    
    for (auto it = tbl.get_it(); it.has_curr(); it.next())
    {
      const auto &pair = it.get_curr();
      f(pair.first, pair.second);
    }
  }
};

} // end namespace Aleph

#endif // AH_MAPPING_H
