
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
 *  concepts, enabling use with std::ranges and STL algorithms. Iterators
 *  that opt in via the `aleph_random_access_iterator` marker (see
 *  AlephRandomAccessItor) are promoted to std::random_access_iterator.
 *
 *  @ingroup Iterators
 *  @author Leandro Rabindranath León
 */

#ifndef AH_ITERATOR_H
#define AH_ITERATOR_H

#include <compare>
#include <iterator>
#include <concepts>
#include <type_traits>
#include <utility>

namespace Aleph
{
  // Forward declaration only: extract_from_stl_container() below names
  // Aleph::DynList in a non-dependent qualified-id, which needs the
  // namespace and template to be visible even though the function body
  // is not instantiated until htlist.H's full definition is in scope.
  template <typename T> class DynList;
}

/** @brief Detects Aleph iterators that opted in to random-access promotion.

    An underlying Aleph iterator `I` qualifies only when it:
    - declares the self-referential marker `aleph_random_access_iterator == I`
      (so derived iterators that reuse a base iterator without redefining
      positioning consistently do NOT inherit the promotion), and
    - exposes the logical-position interface `get_pos()` / `set_pos()` together
      with backward movement `prev()`.
*/
template <class I>
concept AlephRandomAccessItor = requires { typename I::aleph_random_access_iterator; } and
  std::same_as<typename I::aleph_random_access_iterator, I> and
  requires(I &it, const I &cit, long n) {
    { cit.get_pos() } -> std::convertible_to<long>;
    it.set_pos(n);
    it.prev();
  };

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

  /// Category is promoted to random-access when the underlying Aleph iterator
  /// opts in (see AlephRandomAccessItor); otherwise it stays forward, which is
  /// the historical default and never a regression.
  using iterator_category =
    std::conditional_t<AlephRandomAccessItor<Itor>, std::random_access_iterator_tag, std::forward_iterator_tag>;
  using iterator_concept = iterator_category;

  using difference_type = std::ptrdiff_t;

  using reference = decltype(std::declval<const Itor &>().get_curr());

  using pointer = std::add_pointer_t<std::remove_reference_t<reference>>;

  using value_type = T;

  /// Default constructor creates an "end" iterator (no current element).
  StlIterator() noexcept = default;

  using Itor::Itor;

  /// Pre-increment: advance to the next element.
  StlIterator &operator ++ ()
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
  bool operator == (const StlIterator &it) const
  {
    if constexpr (requires(const Itor &a, const Itor &b) {
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
  bool operator != (const StlIterator &it) const
  {
    return not (*this == it);
  }

  // Note: operator* must be const to satisfy std::indirectly_readable.
  // Constness of the iterator object does not imply constness of the element.
  /// Dereference: return the current element.
  reference operator * () const
  {
    return this->get_curr();
  }

  /// Member access to the current element.
  pointer operator ->() const
  {
    return &this->get_curr();
  }

  // Random-access operations. These overloads exist only when the underlying
  // Aleph iterator opts in via the `aleph_random_access_iterator` marker
  // (e.g. DynArray, Array). They operate on the logical position exposed by
  // get_pos()/set_pos(), so any physical/circular index mapping stays internal.

  /// Pre-decrement: move to the previous element.
  StlIterator &operator -- ()
    requires AlephRandomAccessItor<Itor>
  {
    this->prev();
    return *this;
  }

  /// Post-decrement: move to the previous element and return the old iterator.
  StlIterator operator -- (int)
    requires AlephRandomAccessItor<Itor>
  {
    StlIterator ret_val = *this;
    this->prev();
    return ret_val;
  }

  /// Advance the iterator `n` positions (`n` may be negative).
  StlIterator &operator += (difference_type n)
    requires AlephRandomAccessItor<Itor>
  {
    this->set_pos(this->get_pos() + n);
    return *this;
  }

  /// Move the iterator back `n` positions (`n` may be negative).
  StlIterator &operator -= (difference_type n)
    requires AlephRandomAccessItor<Itor>
  {
    this->set_pos(this->get_pos() - n);
    return *this;
  }

  /// Return an iterator advanced `n` positions.
  StlIterator operator + (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlIterator ret_val = *this;
    ret_val += n;
    return ret_val;
  }

  /// Return an iterator advanced `n` positions (symmetric form).
  friend StlIterator operator + (difference_type n, const StlIterator &it)
    requires AlephRandomAccessItor<Itor>
  {
    return it + n;
  }

  /// Return an iterator moved back `n` positions.
  StlIterator operator - (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlIterator ret_val = *this;
    ret_val -= n;
    return ret_val;
  }

  /// Return the signed distance between two iterators.
  difference_type operator - (const StlIterator &it) const
    requires AlephRandomAccessItor<Itor>
  {
    return static_cast<difference_type>(this->get_pos() - it.get_pos());
  }

  /// Access the element `n` positions away from the current one.
  reference operator [] (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlIterator tmp = *this;
    tmp += n;
    return *tmp;
  }

  /// Order two iterators by their logical position.
  std::strong_ordering operator <=> (const StlIterator &it) const
    requires AlephRandomAccessItor<Itor>
  {
    return this->get_pos() <=> it.get_pos();
  }

  /// Create an iterator positioned at the first element of the container.
  static StlIterator begin(SetType &s)
  {
    return StlIterator(s);
  }

  /// Create an end iterator for the container.
  static StlIterator end(SetType &s)
  {
    StlIterator it(s);
    it.Itor::end();
    return it;
  }
};

namespace Aleph
{

namespace const_iterator_detail
{
  /// What a read-only view hands out for an item handed out as `R`: a
  /// reference becomes a reference to const, a pointer a pointer to const
  /// and a value stays a value.
  template <class R> struct const_item { using type = R; };
  template <class T> struct const_item<T &> { using type = const T &; };
  template <class T> struct const_item<T *> { using type = const T *; };
  template <class R> using const_item_t = typename const_item<std::remove_cv_t<R>>::type;

  /// Carries the `Item_Type` of the iterator `It`, when it has one.
  template <class It, class = void> struct item_type_of { };
  template <class It> struct item_type_of<It, std::void_t<typename It::Item_Type>>
  {
    /// The type of the items, as in the underlying iterator.
    using Item_Type = typename It::Item_Type;
  };

  /// Carries the `Set_Type` of the iterator `It`, when it has one.
  template <class It, class = void> struct set_type_of { };
  template <class It> struct set_type_of<It, std::void_t<typename It::Set_Type>>
  {
    /// The type of the container, as in the underlying iterator.
    using Set_Type = typename It::Set_Type;
  };
}

/** @brief Read-only view of an Aleph iterator: the iterator of a constant
    container.

    `Const_Iterator<It>` wraps the iterator `It` of a container and exposes
    its reading protocol (`has_curr()`, `get_curr()`, `next()`, `prev()`,
    `reset_first()`, `get_pos()`...) and nothing else: `get_curr()` hands out
    the current item as a reference to const (a pointer to const for the
    iterators that hand out pointers), and the members that modify the
    container through the iterator (`del()`, `set()`, `insert()`...) do not
    exist. It is what `get_it()` returns on a constant container, and the
    `const_iterator` of the STL interface of every container. Each member
    exists only when the underlying iterator has it, and so do the typedefs
    `Item_Type` and `Set_Type`.

    The underlying iterator is built from the constant container through a
    constructor that accepts it when the iterator has one (`DynArray` keeps
    one, so that the view never allocates); otherwise through a `const_cast`
    that stays harmless, because the view exposes nothing modifiable.

    @tparam It the iterator of the container.
    @see const_iterator_t iterator_for StlConstIterator
    @ingroup Sequences
*/
template <class It>
class Const_Iterator : public const_iterator_detail::item_type_of<It>,
                       public const_iterator_detail::set_type_of<It>
{
  // The cursor moves and is read through a constant view: some iterators
  // read through non-const members, so the view keeps it mutable.
  mutable It it;

  template <class C>
  static It make(const C & c)
  {
    if constexpr (requires { It(c); })
      return It(c);
    else
      return It(const_cast<C &>(c));
  }

public:
  /// The underlying iterator.
  using Itor = It;

  /// Random access when the underlying iterator opts in; see AlephRandomAccessItor.
  using aleph_random_access_iterator =
    std::conditional_t<AlephRandomAccessItor<It>, Const_Iterator, void>;

  /// A view with no current item.
  Const_Iterator() = default;

  /** @brief View positioned on the first item of the container `c`.
      @param[in] c the container; it is never modified through the view.
  */
  template <class C>
    requires (not std::same_as<std::remove_cvref_t<C>, Const_Iterator>
              and not std::same_as<std::remove_cvref_t<C>, It>)
  explicit Const_Iterator(const C & c) : it(make(c)) {}

  /** @brief Read-only view over an existing iterator.
      @param[in] underlying the iterator; the view starts at its position.
  */
  explicit Const_Iterator(It underlying) noexcept(std::is_nothrow_move_constructible_v<It>)
    : it(std::move(underlying)) {}

  /// `true` if there is a current item.
  template <class I = It>
  [[nodiscard]] auto has_curr() const noexcept(noexcept(std::declval<I &>().has_curr()))
    -> decltype(std::declval<I &>().has_curr())
  {
    return it.has_curr();
  }

  /** @brief The current item, read-only.
      @return a reference to const (a pointer to const for the iterators
              that hand out pointers).
      @throw what `get_curr()` of the underlying iterator throws.
  */
  template <class I = It>
  [[nodiscard]] auto get_curr() const noexcept(noexcept(std::declval<I &>().get_curr()))
    -> const_iterator_detail::const_item_t<decltype(std::declval<I &>().get_curr())>
  {
    return it.get_curr();
  }

  /// get_curr() without the checks of the underlying iterator.
  template <class I = It>
  [[nodiscard]] auto get_curr_ne() const noexcept(noexcept(std::declval<I &>().get_curr_ne()))
    -> const_iterator_detail::const_item_t<decltype(std::declval<I &>().get_curr_ne())>
  {
    return it.get_curr_ne();
  }

  /// Move to the next item; what the underlying iterator throws.
  template <class I = It>
  auto next() noexcept(noexcept(std::declval<I &>().next())) -> decltype(std::declval<I &>().next())
  {
    return it.next();
  }

  /// next() without checks.
  template <class I = It>
  auto next_ne() noexcept(noexcept(std::declval<I &>().next_ne())) -> decltype(std::declval<I &>().next_ne())
  {
    return it.next_ne();
  }

  /// Move to the previous item; what the underlying iterator throws.
  template <class I = It>
  auto prev() noexcept(noexcept(std::declval<I &>().prev())) -> decltype(std::declval<I &>().prev())
  {
    return it.prev();
  }

  /// prev() without checks.
  template <class I = It>
  auto prev_ne() noexcept(noexcept(std::declval<I &>().prev_ne())) -> decltype(std::declval<I &>().prev_ne())
  {
    return it.prev_ne();
  }

  /// Position the view on the first item.
  template <class I = It>
  auto reset_first() noexcept(noexcept(std::declval<I &>().reset_first()))
    -> decltype(std::declval<I &>().reset_first())
  {
    return it.reset_first();
  }

  /// Position the view on the last item.
  template <class I = It>
  auto reset_last() noexcept(noexcept(std::declval<I &>().reset_last()))
    -> decltype(std::declval<I &>().reset_last())
  {
    return it.reset_last();
  }

  /// Position the view on the first item.
  template <class I = It>
  auto reset() noexcept(noexcept(std::declval<I &>().reset())) -> decltype(std::declval<I &>().reset())
  {
    return it.reset();
  }

  /// Position the view past the last item.
  template <class I = It>
  auto end() noexcept(noexcept(std::declval<I &>().end())) -> decltype(std::declval<I &>().end())
  {
    return it.end();
  }

  /// Ordinal position of the current item.
  template <class I = It>
  [[nodiscard]] auto get_pos() const noexcept(noexcept(std::declval<I &>().get_pos()))
    -> decltype(std::declval<I &>().get_pos())
  {
    return it.get_pos();
  }

  /// Position the view on the item at ordinal position `pos`.
  template <class I = It>
  auto set_pos(const long pos) noexcept(noexcept(std::declval<I &>().set_pos(pos)))
    -> decltype(std::declval<I &>().set_pos(pos))
  {
    return it.set_pos(pos);
  }

  /// `true` if the current item is the last one.
  template <class I = It>
  [[nodiscard]] auto is_last() const noexcept(noexcept(std::declval<I &>().is_last()))
    -> decltype(std::declval<I &>().is_last())
  {
    return it.is_last();
  }

  /// `true` if the current item is the first one.
  template <class I = It>
  [[nodiscard]] auto is_in_first() const noexcept(noexcept(std::declval<I &>().is_in_first()))
    -> decltype(std::declval<I &>().is_in_first())
  {
    return it.is_in_first();
  }

  /// `true` if the current item is the last one.
  template <class I = It>
  [[nodiscard]] auto is_in_last() const noexcept(noexcept(std::declval<I &>().is_in_last()))
    -> decltype(std::declval<I &>().is_in_last())
  {
    return it.is_in_last();
  }

  /// Exchange the views.
  void swap(Const_Iterator & other) noexcept(std::is_nothrow_swappable_v<It>)
  {
    using std::swap;
    swap(it, other.it);
  }

  /// Equality of the underlying iterators, when they have it.
  template <class I = It>
  [[nodiscard]] auto operator == (const Const_Iterator & other) const
    -> decltype(std::declval<I &>() == std::declval<I &>())
  {
    return it == other.it;
  }

  /// Negation of `==`.
  template <class I = It>
  [[nodiscard]] auto operator != (const Const_Iterator & other) const
    -> decltype(not (std::declval<I &>() == std::declval<I &>()))
  {
    return not (it == other.it);
  }
};

/** @brief The read-only iterator of the container `C`.
    @tparam C a container with a nested `Iterator`.
*/
template <class C>
using const_iterator_t = Const_Iterator<typename C::Iterator>;

/** @brief Iterator on `c` positioned on its first item: the container's own
    iterator on a modifiable container.
    @param[in] c the container.
    @return `typename C::Iterator`.
*/
template <class C>
  requires (not std::is_const_v<C>)
[[nodiscard]] auto iterator_for(C & c)
{
  return typename C::Iterator(c);
}

/** @brief Iterator on `c` positioned on its first item: the read-only view
    on a constant container.
    @param[in] c the container.
    @return `const_iterator_t<C>`.
*/
template <class C>
[[nodiscard]] auto iterator_for(const C & c)
{
  return const_iterator_t<C>(c);
}

} // end namespace Aleph

/** @brief STL-compatible const iterator adapter for Aleph containers.

    This adapter provides a read-only view over the underlying Aleph iterator.

    @tparam SetType Aleph container type.
*/
template <class SetType>
struct StlConstIterator : public Aleph::Const_Iterator<typename SetType::Iterator>
{
  using T = typename SetType::Item_Type;
  using Itor = typename SetType::Iterator;
  using Base = Aleph::Const_Iterator<Itor>;

  /// Category is promoted to random-access when the underlying Aleph iterator
  /// opts in (see AlephRandomAccessItor); otherwise it stays forward, which is
  /// the historical default and never a regression.
  using iterator_category =
    std::conditional_t<AlephRandomAccessItor<Itor>, std::random_access_iterator_tag, std::forward_iterator_tag>;
  using iterator_concept = iterator_category;

  using difference_type = std::ptrdiff_t;

  using pointer = const T *;

  using reference = const T &;

  using value_type = T;

  /// Default constructor creates an "end" iterator (no current element).
  StlConstIterator() noexcept = default;

  /// Positioned on the first item of `s`, which is never modified.
  StlConstIterator(const SetType &s) : Base(s) {}

  /// Pre-increment: advance to the next element.
  StlConstIterator &operator ++ ()
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
  bool operator == (const StlConstIterator &it) const
  {
    if constexpr (requires(const Base &a, const Base &b) {
                    { a == b } -> std::convertible_to<bool>;
                  })
      return static_cast<const Base &>(*this) == static_cast<const Base &>(it);

    if (this->has_curr() and it.has_curr())
      return this->get_pos() == it.get_pos();

    if (not this->has_curr() and not it.has_curr())
      return true;

    return false;
  }

  /// Inequality.
  bool operator != (const StlConstIterator &it) const
  {
    return not (*this == it);
  }

  /// Dereference: return the current element.
  const T &operator * () const
  {
    return this->get_curr();
  }

  /// Member access to the current element.
  const T *operator ->() const
  {
    return &this->get_curr();
  }

  // Random-access operations, mirroring StlIterator. These overloads exist
  // only when the underlying Aleph iterator opts in via the
  // `aleph_random_access_iterator` marker (e.g. DynArray, Array). They operate
  // on the logical position exposed by get_pos()/set_pos(), so any
  // physical/circular index mapping stays internal.

  /// Pre-decrement: move to the previous element.
  StlConstIterator &operator -- ()
    requires AlephRandomAccessItor<Itor>
  {
    this->prev();
    return *this;
  }

  /// Post-decrement: move to the previous element and return the old iterator.
  StlConstIterator operator -- (int)
    requires AlephRandomAccessItor<Itor>
  {
    StlConstIterator ret_val = *this;
    this->prev();
    return ret_val;
  }

  /// Advance the iterator `n` positions (`n` may be negative).
  StlConstIterator &operator += (difference_type n)
    requires AlephRandomAccessItor<Itor>
  {
    this->set_pos(this->get_pos() + n);
    return *this;
  }

  /// Move the iterator back `n` positions (`n` may be negative).
  StlConstIterator &operator -= (difference_type n)
    requires AlephRandomAccessItor<Itor>
  {
    this->set_pos(this->get_pos() - n);
    return *this;
  }

  /// Return an iterator advanced `n` positions.
  StlConstIterator operator + (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlConstIterator ret_val = *this;
    ret_val += n;
    return ret_val;
  }

  /// Return an iterator advanced `n` positions (symmetric form).
  friend StlConstIterator operator + (difference_type n, const StlConstIterator &it)
    requires AlephRandomAccessItor<Itor>
  {
    return it + n;
  }

  /// Return an iterator moved back `n` positions.
  StlConstIterator operator - (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlConstIterator ret_val = *this;
    ret_val -= n;
    return ret_val;
  }

  /// Return the signed distance between two iterators.
  difference_type operator - (const StlConstIterator &it) const
    requires AlephRandomAccessItor<Itor>
  {
    return static_cast<difference_type>(this->get_pos() - it.get_pos());
  }

  /// Access the element `n` positions away from the current one.
  reference operator [] (difference_type n) const
    requires AlephRandomAccessItor<Itor>
  {
    StlConstIterator tmp = *this;
    tmp += n;
    return *tmp;
  }

  /// Order two iterators by their logical position.
  std::strong_ordering operator <=> (const StlConstIterator &it) const
    requires AlephRandomAccessItor<Itor>
  {
    return this->get_pos() <=> it.get_pos();
  }

  /// Create a const iterator positioned at the first element of the container.
  static StlConstIterator cbegin(const SetType &s)
  {
    return StlConstIterator(s);
  }

  /// Create a const end iterator for the container.
  static StlConstIterator cend(const SetType &s)
  {
    StlConstIterator it(s);
    it.end();
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
  SetName *me()
  {
    return static_cast<SetName *>(this);
  }

  const SetName *const_me() const
  {
    return static_cast<const SetName *>(this);
  }

public:
  using iterator = StlIterator<SetName>;

  using const_iterator = StlConstIterator<SetName>;

  /// Return an STL-compatible iterator to the first element.
  iterator begin() noexcept
  {
    return iterator::begin(*me());
  }

  /// Return an STL-compatible end iterator.
  iterator end() noexcept
  {
    return iterator::end(*me());
  }

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

  friend const_iterator cbegin(const SetName &s) noexcept
  {
    return s.begin();
  }

  friend const_iterator cend(const SetName &s) noexcept
  {
    return s.end();
  }

  friend const_iterator begin(const SetName &s) noexcept
  {
    return s.begin();
  }

  friend const_iterator end(const SetName &s) noexcept
  {
    return s.end();
  }

  friend iterator begin(SetName &s) noexcept
  {
    return s.begin();
  }

  friend iterator end(SetName &s) noexcept
  {
    return s.end();
  }
};

/** @brief Extract all items from an STL container into an Aleph `DynList`.

    @tparam Container Any STL-like container supporting range-for iteration.
    @param c Source container.
    @return A `DynList` containing copies of all elements from `c`.

    @ingroup Functional
*/
template <class Container>
inline Aleph::DynList<typename Container::value_type> extract_from_stl_container(const Container &c)
{
  Aleph::DynList<typename Container::value_type> ret;
  for (const auto &i : c)
    ret.append(i);
  return ret;
}

#endif  // AH_ITERATOR_H
