
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


/** @file ah-iterator.H
 *  @brief STL-compatible iterator adapters for Aleph containers.
 *
 *  Provides StlIterator and StlConstIterator that wrap Aleph container
 *  iterators to satisfy std::input_iterator and std::forward_iterator
 *  concepts, enabling use with std::ranges and STL algorithms.
 *
 *  @ingroup Iterators
 *  @author Leandro Rabindranath León
 */

# ifndef AH_ITERATOR_H
# define AH_ITERATOR_H

# include <iterator>
# include <concepts>
# include <type_traits>
# include <utility>

 /** @brief STL-compatible mutable iterator adapter for Aleph containers.
 
     This adapter wraps an Aleph-style iterator (`SetType::Iterator`) and exposes
     the operations expected by the STL and C++20 ranges.

     @tparam SetType Aleph container type.

     @note Requirements on `SetType::Iterator`:
     - Constructible from `SetType&`.
     - `bool has_curr() const`.
     - `void next()`.
     - `auto & get_curr() const` returning the current element.
     - `auto get_pos() const` returning a position that is stable and comparable
       between iterators over the same container.
     - `void end()` to move the iterator to the end sentinel.
 */
 template <class SetType>
 struct StlIterator : public SetType::Iterator
 {
  using T = typename SetType::Item_Type;
  using Itor = typename SetType::Iterator;
  using iterator_category = std::forward_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;

  using difference_type = std::ptrdiff_t;

  using reference = decltype(std::declval<const Itor &>().get_curr());

  using pointer = std::add_pointer_t<std::remove_reference_t<reference>>;

  using value_type = T;

  /// Default constructor creates an "end" iterator (no current element).
  StlIterator() noexcept = default;

  using Itor::Itor;

  /// Pre-increment: advance to the next element.
  StlIterator& operator ++ ()
  {
    this->next();
    return *this;
  }

  /// Post-increment: advance to the next element and return the old iterator.
  StlIterator operator ++ (int)
  {
    StlIterator ret_val = *this;
    this->next();
    return ret_val;
  }

  /// Equality compares positions when both iterators are valid; otherwise
  /// both must be in the end state.
  bool operator == (const StlIterator & it) const
  {
    if constexpr (requires (const Itor & a, const Itor & b)
                  {
                    { a == b } -> std::convertible_to<bool>;
                  })
      return static_cast<const Itor &>(*this) == static_cast<const Itor &>(it);

    if (this->has_curr() and it.has_curr())
      return this->get_pos() == it.get_pos();
      // return this->get_curr() == it.get_curr();

    if (not this->has_curr() and not it.has_curr())
      return true;

    return false;
  }

  /// Inequality.
  bool operator != (const StlIterator & it) const
  {
    return not (*this == it);
  }

  // Note: operator* must be const to satisfy std::indirectly_readable.
  // Constness of the iterator object does not imply constness of the element.
  /// Dereference: return the current element.
  reference operator * () const { return this->get_curr(); }

  /// Member access to the current element.
  pointer operator -> () const { return &this->get_curr(); }

  /// Create an iterator positioned at the first element of the container.
  static StlIterator begin(SetType & s)
  {
    return StlIterator(s);
  }

  /// Create an end iterator for the container.
  static StlIterator end(SetType & s)
  {
    StlIterator it(s);
    it.Itor::end();
    return it;
  }
};


 /** @brief STL-compatible const iterator adapter for Aleph containers.
 
     This adapter provides a read-only view over the underlying Aleph iterator.

     @tparam SetType Aleph container type.
 */
 template <class SetType>
 struct StlConstIterator : public SetType::Iterator
 {
  using T = typename SetType::Item_Type;
  using Itor = typename SetType::Iterator;
  using iterator_category = std::forward_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;

  using difference_type = std::ptrdiff_t;

  using pointer = const T*;

  using reference = const T&;

  using value_type = T;

  /// Default constructor creates an "end" iterator (no current element).
  StlConstIterator() noexcept = default;

  using Itor::Itor;

  /// Pre-increment: advance to the next element.
  StlConstIterator& operator ++ ()
  {
    this->next();
    return *this;
  }

  /// Post-increment: advance to the next element and return the old iterator.
  StlConstIterator operator ++ (int)
  {
    StlConstIterator ret_val = *this;
    this->next();
    return ret_val;
  }

  /// Equality compares positions when both iterators are valid; otherwise
  /// both must be in the end state.
  bool operator == (const StlConstIterator & it) const
  {
    if constexpr (requires (const Itor & a, const Itor & b)
                  {
                    { a == b } -> std::convertible_to<bool>;
                  })
      return static_cast<const Itor &>(*this) == static_cast<const Itor &>(it);

    if (this->has_curr() and it.has_curr())
      return this->get_pos() == it.get_pos();

    if (not this->has_curr() and not it.has_curr())
      return true;

    return false;
  }

  /// Inequality.
  bool operator != (const StlConstIterator & it) const
  {
    return not (*this == it);
  }

  /// Dereference: return the current element.
  const T & operator * () const { return this->get_curr(); }

  /// Member access to the current element.
  const T * operator -> () const { return &this->get_curr(); }

  /// Create a const iterator positioned at the first element of the container.
  static StlConstIterator cbegin(const SetType & s)
  {
    return StlConstIterator(s);
  }

  /// Create a const end iterator for the container.
  static StlConstIterator cend(const SetType & s)
  {
    StlConstIterator it(s);
    it.Itor::end();
    return it;
  }
};


 /** @brief Mixin that adds STL `begin()/end()` and `cbegin()/cend()` to Aleph containers.

     Containers inheriting from this class must expose `Item_Type` and `Iterator`,
     where `Iterator` meets the requirements described in `StlIterator`.

     @tparam SetName Aleph container type.
 */
 template <class SetName>
 class StlAlephIterator
 {
  SetName * me() { return static_cast<SetName*>(this); }

  const SetName * const_me() const { return static_cast<const SetName*>(this); }

public:

  using iterator = StlIterator<SetName>;

  using const_iterator = StlConstIterator<SetName>;

  /// Return an STL-compatible iterator to the first element.
  iterator begin() noexcept { return iterator::begin(*me()); }

  /// Return an STL-compatible end iterator.
  iterator end() noexcept { return iterator::end(*me()); }

  /// Return a const iterator to the first element.
  const_iterator begin() const noexcept
  {
    return const_iterator::cbegin(*const_me());
  }

  /// Return a const end iterator.
  const_iterator end() const noexcept
  {
    return const_iterator::cend(*const_me());
  }

  /// Return a const iterator to the first element.
  const_iterator cbegin() const noexcept
  {
    return const_iterator::cbegin(*const_me());
  }

  /// Return a const end iterator.
  const_iterator cend() const noexcept
  {
    return const_iterator::cend(*const_me());
  }

  friend const_iterator cbegin(const SetName & s) noexcept { return s.begin(); }

  friend const_iterator cend(const SetName & s) noexcept { return s.end(); }

  friend const_iterator begin(const SetName & s) noexcept { return s.begin(); }

  friend const_iterator end(const SetName & s) noexcept { return s.end(); }

  friend iterator begin(SetName & s) noexcept { return s.begin(); }

  friend iterator end(SetName & s) noexcept { return s.end(); }
};


 /** @brief Extract all items from an STL container into an Aleph `DynList`.

     @tparam Container Any STL-like container supporting range-for iteration.
     @param c Source container.
     @return A `DynList` containing copies of all elements from `c`.

     @ingroup Functional
 */
template <class Container> inline
Aleph::DynList<typename Container::value_type>
extract_from_stl_container(const Container & c)
{
  Aleph::DynList<typename Container::value_type> ret;
  for (const auto & i : c)
    ret.append(i);
  return ret;
}


# endif // AH_ITERATOR_H
