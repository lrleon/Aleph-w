
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file ahIterator.H
 *  @brief Iterator traits and STL-compatible iterator wrappers.
 *
 *  This file provides iterator infrastructure for Aleph-w containers,
 *  enabling compatibility with STL algorithms and C++20 ranges.
 *
 *  ## Features
 *
 *  - `iterator_traits<Itor>`: Type traits for iterators
 *  - `__iterator<Set>`: Mutable STL-compatible iterator wrapper
 *  - `__const_iterator<Set>`: Const STL-compatible iterator wrapper
 *  - `STL_ALEPH_ITERATOR`: Macro to add begin/end to containers
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Container uses macro to get STL interface
 *  class MyContainer {
 *    using Iterator = ...;
 *    STL_ALEPH_ITERATOR(MyContainer);
 *  };
 *
 *  // Now works with range-for and STL algorithms
 *  MyContainer c;
 *  for (auto& item : c) { ... }
 *  std::find(c.begin(), c.end(), value);
 *  ```
 *
 *  ## C++20 Ranges Compatibility
 *
 *  The iterators satisfy `std::input_iterator` requirements:
 *  - `operator*` is const (required for `std::indirectly_readable`)
 *  - Proper `==` and `!=` operators
 *  - Prefix and postfix `++`
 *
 *  @see ah-ranges.H C++20 ranges support
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

# ifndef AHITERATOR_H
# define AHITERATOR_H

# include <type_traits>

namespace Aleph
{

/**
 * @brief Iterator traits for Aleph-w iterators.
 *
 * Provides standard type aliases for iterator properties,
 * similar to std::iterator_traits.
 *
 * @tparam Itor Iterator type with nested type definitions
 */
  template <class Itor>
  struct iterator_traits
  {
    typedef typename Itor::value_type value_type;         ///< Element type
    typedef typename Itor::difference_type difference_type; ///< Distance type
    typedef typename Itor::pointer pointer;               ///< Pointer to element
    typedef typename Itor::reference reference;           ///< Reference to element
  };

# define STL_ITOR_SPEC(It_Name)						\
  public:								\
									\
  using T = typename Set_Type::Item_Type;				\
  using Itor = typename Set_Type::Iterator;				\
									\
  using Itor::Itor;							\
									\
  Itor operator ++ ()							\
  {									\
    this->next();							\
    return *this;							\
  }									\
									\
  Itor operator ++ (int)						\
  {									\
    Itor ret_val = *this;						\
    this->next();							\
    return ret_val;							\
  }									\
									\
  bool operator == (const Itor & it) const noexcept			\
  {									\
    if (this->has_curr() and it.has_curr())				\
      return this->get_pos() == it.get_pos();				\
									\
    if (not this->has_curr() and not it.has_curr())			\
      return true;							\
									\
    return false;							\
  }									\
									\
  bool operator != (const Itor & it) const noexcept			\
  {									\
    return not (*this == it);						\
  }


/**
 * @brief STL-compatible mutable iterator wrapper.
 *
 * Wraps an Aleph-w Iterator to provide STL-compatible interface
 * including `operator*`, `operator->`, `operator++`, and comparison.
 *
 * @tparam Set_Type Container type with nested Iterator class
 *
 * @note `operator*` is const to satisfy `std::indirectly_readable` concept.
 */
  template <class Set_Type>
  struct __iterator : public Set_Type::Iterator
  {
    STL_ITOR_SPEC(__iterator);

    // Note: operator* must be const to satisfy std::indirectly_readable concept
    // (required by std::ranges::input_iterator). The constness of the iterator
    // does not affect the constness of the pointed-to value.
    T &operator *() const { return const_cast<T &>(this->get_curr()); }

    T * operator ->() const
    {
      return &const_cast<T &>(this->get_curr());
    }

    static __iterator begin(const Set_Type & s) noexcept
    {
      return __iterator(s);
    }

    static __iterator end(const Set_Type & s) noexcept
    {
      __iterator it(s);
      it.Itor::end();
      return it;
    }
  };

/**
 * @brief STL-compatible const iterator wrapper.
 *
 * Like `__iterator` but provides only const access to elements.
 *
 * @tparam Set_Type Container type with nested Iterator class
 */
  template <class Set_Type>
  struct __const_iterator : public Set_Type::Iterator
  {
    STL_ITOR_SPEC(__const_iterator);

    const T &operator *() const noexcept { return this->get_curr(); }

    T const * operator ->() const noexcept
    {
      return const_cast<T *>(&this->get_curr());
    }

    static __const_iterator cbegin(const Set_Type & s) noexcept
    {
      return __const_iterator(s);
    }

    static __const_iterator cend(const Set_Type & s) noexcept
    {
      __const_iterator it(s);
      it.Itor::end();
      return it;
    }
  };


# define STL_ALEPH_ITERATOR(Set_Name)					\
  using iterator = __iterator<Set_Name>;				\
									\
  using const_iterator = __const_iterator<Set_Name>;			\
									\
  iterator begin() noexcept { return iterator::begin(*this); }		\
									\
  iterator end() noexcept { return iterator::end(*this); }		\
									\
  const_iterator begin() const noexcept					\
  {									\
    return const_iterator::cbegin(*this);				\
  }									\
									\
  const_iterator end() const noexcept					\
  {									\
    return const_iterator::cend(*this);					\
  }									\
  									\
  const_iterator cbegin() const	noexcept				\
  {									\
    return const_iterator::cbegin(*this);				\
  }									\
									\
  const_iterator cend()	const noexcept					\
  {									\
    return const_iterator::cend(*this);					\
  }									\
									\
  const_iterator cbegin() noexcept					\
  {									\
    return const_iterator::cbegin(*this);				\
  }									\
  									\
  const_iterator cend() noexcept					\
  {									\
    return const_iterator::cend(*this);					\
  }									\
									\
  friend const_iterator cbegin(const Set_Name & s) noexcept		\
  { return s.begin(); }							\
									\
  friend const_iterator cend(const Set_Name & s) noexcept		\
  { return s.end(); }							\
  									\
  friend const_iterator begin(const Set_Name & s) noexcept		\
  { return s.begin(); }							\
									\
  friend const_iterator end(const Set_Name & s) noexcept		\
  { return s.end(); }							\
									\
  friend iterator begin(Set_Name & s) noexcept { return s.begin(); }	\
									\
  friend iterator end(Set_Name & s) noexcept { return s.end(); }

# ifdef nada
  inline template <class Itor>
  typename iterator_traits<Itor>::difference_type distance(Itor it1, Itor it2)
  {
    typename iterator_traits<Itor>::difference_type d = 0;

    while (it1 not_eq it2)
      {
        d++;
        it1++;
      }

    return d;
  }

# endif
}; // end namespace Aleph

# endif // AHITERATOR_H
