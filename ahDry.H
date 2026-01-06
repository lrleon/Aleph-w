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

/** @file ahDry.H
 *  @brief DRY (Don't Repeat Yourself) utilities and macros.
 *
 *  This file provides utility macros that implement common patterns
 *  to avoid code repetition.
 *
 *  ## Active Macros
 *
 *  - `Special_Ctors(Set_Type, Type)`: Generates constructors from lists,
 *    iterators, and initializer_list. Cannot be replaced by CRTP because
 *    constructors don't inherit in C++.
 *
 *  - `Generic_Items(Type)`: Generates an items() method that returns all
 *    elements in a container.
 *
 *  - `Generate_Proxy_Operator(Class_Name)`: Generates operator[] for
 *    map-like containers.
 *
 *  ## Deprecated Macros (REMOVED)
 *
 *  The following macros have been migrated to CRTP classes in `ah-dry.H`
 *  and `ah-dry-mixin.H`:
 *
 *  - `Generic_Traverse` → `GenericTraverse` class in ah-dry.H
 *  - `Locate_Functions` → `LocateFunctions` class in ah-dry.H
 *  - `Functional_Methods` → `FunctionalMethods` class in ah-dry.H
 *  - `Generic_Keys` → `GenericItems` class in ah-dry.H
 *  - `Equal_To_Method` → `EqualToMethod` class in ah-dry.H
 *  - `Map_Sequences_Methods` → `MapSequencesMethods` class in ah-dry.H
 *
 *  For new code, prefer using the CRTP classes from ah-dry.H or the
 *  documented mixins from ah-dry-mixin.H.
 *
 *  @see ah-dry.H for CRTP class implementations
 *  @see ah-dry-mixin.H for documented mixin implementations
 *  @ingroup Utils
 *  @author Leandro Rabindranath León
 */

# ifndef AHDRY_H
# define AHDRY_H

# include <cstddef>
# include <tuple>
# include <functional>
# include <sstream>
# include <initializer_list>

# include <ahFunctional.H>
# include <ah-errors.H>

namespace Aleph
{

template<typename T> class DynList;

/** @brief Generates special constructors for containers.
 *
 *  This macro generates three constructors:
 *  1. Constructor from any list-like container with for_each method
 *  2. Constructor from iterator pair [begin, end)
 *  3. Constructor from std::initializer_list
 *
 *  @note This cannot be migrated to CRTP because constructors don't
 *        inherit in C++.
 *
 *  @param Set_Type The container class name
 *  @param Type The element type
 *
 *  Example:
 *  @code
 *  template <typename T>
 *  class MyContainer {
 *    // ... members ...
 *  public:
 *    MyContainer() = default;
 *    Special_Ctors(MyContainer, T);
 *  };
 *  @endcode
 */
# define Special_Ctors(Set_Type, Type)                                      \
  template <template <typename> class List>                                 \
  Set_Type(const List<Type> & l) : Set_Type()                               \
  {                                                                         \
    l.for_each([this] (const Type & item) { (void)this->append(item); });   \
  }                                                                         \
                                                                            \
  template <class It>                                                       \
  Set_Type(It b, It e) : Set_Type()                                         \
  {                                                                         \
    for (It it = b; it != e; ++it)                                          \
      (void)this->append(*it);                                              \
  }                                                                         \
                                                                            \
  Set_Type(std::initializer_list<Type> l) : Set_Type()                      \
  {                                                                         \
    for (const auto & item : l)                                             \
      (void)this->append(item);                                             \
  }

/** @brief Generates an items() method returning all container elements.
 *
 *  @param Type The element type
 *
 *  @note For new code, consider using KeysMixin from ah-dry-mixin.H instead.
 */
# define Generic_Items(Type)                                            \
  template <template <typename> class Container = DynList>              \
  Container<Type> items() const                                         \
  {                                                                     \
    return this->template maps<Type, Container> ([] (const Type & key)  \
                                                 { return key; });      \
  }

/** @brief Generates operator[] for map-like containers.
 *
 *  @param Class_Name The container class name (unused, kept for compatibility)
 */
# define Generate_Proxy_Operator(Class_Name)        \
  const Data & operator [] (const Key & key) const  \
  {                                                 \
    return find(key);                               \
  }                                                 \
                                                    \
  Data & operator [] (const Key & key)              \
  {                                                 \
    return find(key);                               \
  }


// ============================================================================
// Utility functions and classes
// ============================================================================

/** @brief Converts any streamable value to string.
 *  @param d Value to convert
 *  @return String representation
 */
template<typename Type> inline
std::string to_str(const Type &d)
{
  std::ostringstream os;
  os << d;
  return os.str();
}

/** @brief Default comparator for pair types in hash maps.
 *
 *  Wraps a key comparator to work with std::pair<Key, Data>.
 *  Supports heterogeneous comparison (Pair vs Key).
 *
 *  @tparam Key Key type
 *  @tparam Data Data type
 *  @tparam Cmp Key comparator (default: std::equal_to<Key>)
 */
template<typename Key, typename Data, class Cmp = std::equal_to<Key>>
struct Dft_Pair_Cmp
{
  Cmp cmp;
  Dft_Pair_Cmp(Cmp __cmp = Cmp()) : cmp(__cmp) {}

  /// Pair vs Pair comparison
  bool operator()(const std::pair<Key, Data> &p1,
                  const std::pair<Key, Data> &p2) const noexcept
  {
    return cmp(p1.first, p2.first);
  }

  /// Heterogeneous: Pair vs Key
  bool operator()(const std::pair<Key, Data> &p,
                  const Key &k) const noexcept
  {
    return cmp(p.first, k);
  }

  /// Heterogeneous: Key vs Pair
  bool operator()(const Key &k,
                  const std::pair<Key, Data> &p) const noexcept
  {
    return cmp(k, p.first);
  }
};

/** @brief Converts a pointer to Key to a pointer to the containing pair.
 *  @param ptr Pointer to the key within a pair
 *  @return Pointer to the containing pair
 */
template<typename Key, typename Data>
std::pair<Key, Data> *key_to_pair(Key *ptr)
{
  using PairType = std::pair<Key, Data>;
  return reinterpret_cast<PairType *>(
      reinterpret_cast<char *>(ptr) - offsetof(PairType, first));
}

/// @overload
template<typename Key, typename Data>
const std::pair<Key, Data> *key_to_pair(const Key *ptr)
{
  using PairType = std::pair<Key, Data>;
  return reinterpret_cast<const PairType *>(
      reinterpret_cast<const char *>(ptr) - offsetof(PairType, first));
}

/** @brief Converts a pointer to Data to a pointer to the containing pair.
 *  @param ptr Pointer to the data within a pair
 *  @return Pointer to the containing pair
 */
template<typename Key, typename Data>
std::pair<Key, Data> *data_to_pair(Data *ptr)
{
  using PairType = std::pair<Key, Data>;
  return reinterpret_cast<PairType *>(
      reinterpret_cast<char *>(ptr) - offsetof(PairType, second));
}

/// @overload
template<typename Key, typename Data>
const std::pair<Key, Data> *data_to_pair(const Data *ptr)
{
  using PairType = std::pair<Key, Data>;
  return reinterpret_cast<const PairType *>(
      reinterpret_cast<const char *>(ptr) - offsetof(PairType, second));
}

} // end namespace Aleph

# endif // AHDRY_H
