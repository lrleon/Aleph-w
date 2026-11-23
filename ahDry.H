
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
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AHDRY_H
# define AHDRY_H

# include <cstddef>
# include <tuple>
# include <functional>
# include <sstream>
# include <initializer_list>
# include <type_traits>

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
 *  The iterator-pair constructor only accepts a type that can be
 *  dereferenced, incremented and compared with `!=`, which is exactly what
 *  its body uses. Arithmetic arguments never select it, so a call such as
 *  `Set_Type(3, 7)` resolves to another constructor of the container (for
 *  instance a count/value one) or fails with a "no matching constructor"
 *  error, instead of failing inside this macro on `*it`. The constraint is
 *  deliberately weaker than `std::input_iterator`: Aleph iterators without
 *  complete `std::iterator_traits` remain accepted.
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
    requires (not std::is_arithmetic_v<It>                                  \
              and requires (It it) { *it; ++it; it != it; })                \
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
 *  Its comparisons are `noexcept` only if those of `Cmp` are, with the
 *  conversion of their result to `bool`, so that a map whose comparison
 *  throws lets the exception reach its caller.
 *
 *  @tparam Key Key type
 *  @tparam Data Data type
 *  @tparam Cmp Key comparator (default: std::equal_to<Key>)
 */
template<typename Key, typename Data, class Cmp = std::equal_to<Key>>
struct Dft_Pair_Cmp
{
  Cmp cmp;

  /** @brief Builds the comparator of the keys by default.
   *
   *  It is a constructor apart from the one that takes the comparator,
   *  and not a default argument of that one: GCC and Clang do not count
   *  a default argument in the `noexcept` of the implicit default
   *  constructor of a class that holds or inherits this one, and what
   *  that argument threw would end the program there.
   *
   *  @throws What the default construction of `Cmp` throws; nothing when
   *          it is `noexcept`, so that a map whose comparator has no
   *          state is built without throwing, and its tree too.
   */
  Dft_Pair_Cmp() noexcept(std::is_nothrow_default_constructible_v<Cmp>)
    : cmp() {}

  /** @brief Wraps the comparator `__cmp` of the keys.
   *
   *  @param[in] __cmp The comparator, which is copied.
   *  @throws What the copy of `__cmp` throws.
   */
  Dft_Pair_Cmp(Cmp __cmp) noexcept(std::is_nothrow_copy_constructible_v<Cmp>)
    : cmp(__cmp) {}

  /// Pair vs Pair comparison; it throws what `cmp` throws
  bool operator()(const std::pair<Key, Data> &p1,
                  const std::pair<Key, Data> &p2) const
    noexcept(std::is_nothrow_invocable_r_v<bool, const Cmp &,
                                           const Key &, const Key &>)
  {
    return cmp(p1.first, p2.first);
  }

  /// Heterogeneous: Pair vs Key; it throws what `cmp` throws
  bool operator()(const std::pair<Key, Data> &p,
                  const Key &k) const
    noexcept(std::is_nothrow_invocable_r_v<bool, const Cmp &,
                                           const Key &, const Key &>)
  {
    return cmp(p.first, k);
  }

  /// Heterogeneous: Key vs Pair; it throws what `cmp` throws
  bool operator()(const Key &k,
                  const std::pair<Key, Data> &p) const
    noexcept(std::is_nothrow_invocable_r_v<bool, const Cmp &,
                                           const Key &, const Key &>)
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
