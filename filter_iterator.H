
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


/** @file filter_iterator.H
 *  @brief Generic filter iterator wrapper for Aleph containers.
 *
 *  This file provides a generic filter iterator that wraps an existing
 *  iterator and filters elements based on a user-defined predicate.
 *  Only elements that satisfy the filter predicate are visible during iteration.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef FILTER_ITERATOR_H
# define FILTER_ITERATOR_H

# include <stdexcept>
# include <ah-errors.H>
# include <ah-errors.H>

namespace Aleph
{

/** @brief Generic filter iterator wrapper.
 *
 *  Filter_Iterator wraps an existing iterator and filters elements
 *  based on a user-defined predicate. Only elements that satisfy the
 *  filter predicate are visible during iteration.
 *
 *  This class is parameterized by the following types:
 *  - **Container**: A container type from the Aleph library (e.g.,
 *    DynList, DynArray, DynSetTree).
 *  - **It**: An iterator class that must belong to the Container type.
 *    Since there may be several iterator classes associated with a
 *    container (Container::Iterator, Container::Rev_Iterator, etc.),
 *    this parameter allows selecting which one to use.
 *  - **Show_Item**: A functor class that determines whether an element
 *    should be visible through the iterator. The filter criterion is
 *    implemented via the boolean call `Show_Item::operator()(Item_Type)`
 *    which must return true if the element should be shown, or false
 *    otherwise.
 *
 *  The purpose of Filter_Iterator is to provide a generic iterator
 *  that filters elements according to a criterion defined by the
 *  Show_Item class. This allows generic algorithms using Filter_Iterator
 *  to have different behaviors depending on the Show_Item filter.
 *
 *  Aleph containers export two important types in the context of
 *  Filter_Iterator: Set_Type and Item_Type, which correspond to the
 *  type of container being iterated over and the type of element
 *  returned by the iterator It, respectively.
 *
 *  **Usage example**:
 *  @code
 *  // Define a filter that shows only even numbers
 *  struct ShowEven {
 *    bool operator()(int x) const { return x % 2 == 0; }
 *  };
 *
 *  DynList<int> list;
 *  list.append(1); list.append(2); list.append(3);
 *  list.append(4); list.append(5); list.append(6);
 *
 *  Filter_Iterator<DynList<int>, DynList<int>::Iterator, ShowEven> it(list);
 *
 *  for (; it.has_curr(); it.next())
 *    std::cout << it.get_curr() << " "; // Prints: 2 4 6
 *  @endcode
 *
 *  @tparam Container The container type being iterated over. Must be
 *          an Aleph container type that supports iteration.
 *  @tparam It The base iterator type for the container. Should be one
 *          of the iterator classes provided by Container.
 *  @tparam Show_Item A functor class that determines element visibility.
 *          Must provide: `bool operator()(const Item_Type&)` that returns
 *          true if the item should be shown, false otherwise.
 *
 *  @warning This class is designed for Aleph containers and does not work
 *           with the standard library (std).
 *
 *  @warning This class is **not thread-safe**. Concurrent access from
 *           multiple threads requires external synchronization.
 *
 *  @note The iterator catches `std::overflow_error` and `std::underflow_error`
 *        internally to handle boundary conditions gracefully. Other exceptions
 *        from the filter functor will propagate to the caller.
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */
template <class Container, class It, class Show_Item>
class Filter_Iterator : public It
{
  Show_Item show_item;

  const Container *container_ptr = nullptr;

  /// User-defined pointer for extensibility (e.g., passing context data to filter)
  void *cookie = nullptr;

  /** @brief Advance to the first element that passes the filter.
   *
   *  Starting from the current position, advances the iterator until
   *  finding an element that satisfies the filter predicate, or until
   *  reaching the end of the sequence.
   *
   *  Catches `std::overflow_error` internally to handle end-of-sequence
   *  gracefully without propagating the exception.
   */
  void goto_first_valid_item()
  {
    try
      { // position at the first element that passes show_item filter
        while (true)
          {
            if (not this->It::has_curr() or show_item(this->It::get_curr()))
              return;
            this->It::next();
          }
      }
    catch (const std::overflow_error &) { /* continue in overflow; do not propagate */ }
  }

  /** @brief Move forward to the next element that passes the filter.
   *
   *  First advances past the current element, then continues advancing
   *  until finding an element that satisfies the filter predicate, or
   *  until reaching the end of the sequence.
   *
   *  Catches `std::overflow_error` internally to handle end-of-sequence
   *  gracefully without propagating the exception.
   */
  void forward()
  {
    try
      { // advance to the next item that passes show_item filter
        this->It::next();
        for (; true; this->It::next())
            if (not this->It::has_curr() or show_item(this->It::get_curr()))
              return;
      }
    catch (const std::overflow_error &) { /* continue in overflow; do not propagate */ }
  }

  /** @brief Move to the last element that passes the filter.
   *
   *  Positions the iterator at the end, then moves backward until
   *  finding an element that satisfies the filter predicate, or until
   *  reaching the beginning of the sequence.
   *
   *  Catches `std::underflow_error` internally to handle beginning-of-sequence
   *  gracefully without propagating the exception.
   */
  void goto_last_valid_item()
  {
    try
      {
        for (this->It::reset_last(); true; this->It::prev())
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (const std::underflow_error &) { /* continue in underflow; do not propagate */ }
  }

  /** @brief Move backward to the previous element that passes the filter.
   *
   *  First moves back past the current element, then continues moving
   *  backward until finding an element that satisfies the filter predicate,
   *  or until reaching the beginning of the sequence.
   *
   *  Catches `std::underflow_error` internally to handle beginning-of-sequence
   *  gracefully without propagating the exception.
   */
  void backward()
  {
    try
      {
        this->It::prev();
        for (; true; this->It::prev())
          if (not this->It::has_curr() or show_item(this->It::get_curr()))
            return;
      }
    catch (const std::underflow_error &) { /* continue in underflow; do not propagate */ }
  }

public:
  /// The type of element returned by get_curr()
  using Item_Type = typename It::Item_Type;

  /// The type of the base iterator
  using Iterator_Type = It;

  /// The type of container being iterated
  using Container_Type = Container;

  /// The type of the filter functor
  using Filter_Type = Show_Item;

  /** @brief Returns a const reference to the container being iterated over.
   *
   *  @return Const reference to the container.
   *  @throw std::domain_error if the iterator was default-constructed
   *         without a container.
   */
  const Container & get_container() const
  {
    ah_domain_error_if(container_ptr == nullptr) << "Filter_Iterator: no container set";
    return *container_ptr;
  }

  /** @brief Check if a container has been set.
   *
   *  @return true if the iterator has a container, false otherwise.
   */
  bool has_container() const noexcept { return container_ptr != nullptr; }

  /** @brief Returns a reference to the underlying base iterator.
   *
   *  This provides access to the base iterator functionality if needed.
   *
   *  @return Reference to the base iterator.
   */
  It & get_iterator() noexcept { return *this; }

  /** @brief Returns a const reference to the underlying base iterator.
   *
   *  @return Const reference to the base iterator.
   */
  const It & get_iterator() const noexcept { return *this; }

  /** @brief Returns a reference to the filter functor.
   *
   *  Use this function if you need to access or modify the filter state.
   *  Note that the filter object will be destroyed when the
   *  ~Filter_Iterator() destructor is called.
   *
   *  @return Reference to the filter functor.
   */
  Show_Item & get_filter() noexcept { return show_item; }

  /** @brief Returns a const reference to the filter functor.
   *
   *  @return Const reference to the filter functor.
   */
  const Show_Item & get_filter() const noexcept { return show_item; }

  /** @brief Sets a new filter functor.
   *
   *  This allows changing the filtering criterion after construction.
   *  After setting a new filter, you should call reset_first() or
   *  reset_last() to reposition the iterator according to the new filter.
   *
   *  @param si The new filter functor to use.
   */
  void set_filter(Show_Item si) { show_item = si; }

  /** @brief Sets the cookie pointer.
   *
   *  The cookie is a user-defined pointer for extensibility purposes,
   *  such as passing context data to the filter functor.
   *
   *  @param __cookie The new cookie value.
   */
  void set_cookie(void *__cookie) noexcept { cookie = __cookie; }

  /** @brief Gets the cookie pointer.
   *
   *  @return The current cookie value.
   */
  void * get_cookie() const noexcept { return cookie; }

  /** @brief Default constructor.
   *
   *  Creates a filter iterator with only a filter functor.
   *  The container must be set later by copy/assignment from another
   *  iterator that has a container.
   *
   *  @warning Calling get_container() on a default-constructed iterator
   *           will throw std::domain_error.
   *
   *  @param si The filter functor to use (default constructed if not provided).
   */
  Filter_Iterator(Show_Item si = Show_Item()) noexcept
    : show_item(si)
  {
    // empty
  }

  /** @brief Constructs a filter iterator over a container.
   *
   *  The iterator is automatically positioned at the first element
   *  that passes the filter predicate.
   *
   *  @param c The container to iterate over.
   *  @param si The filter functor to use (default constructed if not provided).
   *  @throw std::bad_alloc if there is not enough memory.
   */
  Filter_Iterator(const Container & c, Show_Item si = Show_Item())
    : It(c), show_item(si), container_ptr(&c)
  {
    goto_first_valid_item();
  }

  /** @brief Constructs a filter iterator with a cookie.
   *
   *  The cookie is a user-defined pointer that can be used to attach
   *  arbitrary data to the iterator for extensibility purposes.
   *
   *  @param c The container to iterate over.
   *  @param __cookie User-defined pointer for extensibility.
   *  @param si The filter functor to use (default constructed if not provided).
   *  @throw std::bad_alloc if there is not enough memory.
   */
  Filter_Iterator(const Container & c, void *__cookie,
                  Show_Item si = Show_Item())
    : Filter_Iterator(c, si)
  {
    cookie = __cookie;
  }

  /** @brief Advances the iterator to the next filtered element.
   *
   *  Moves forward in the sequence, skipping elements that don't pass
   *  the filter, until reaching the next visible element or the end.
   */
  void next() { forward(); }

  /** @brief Advances the iterator to the next filtered element (noexcept version).
   *
   *  This is the exception-safe version of next(). All exceptions are
   *  silently caught and the iterator remains in a valid state.
   *
   *  @note This method is noexcept and will never throw.
   */
  void next_ne() noexcept
  {
    try
      {
        forward();
      }
    catch (...) {}
  }

  /** @brief Moves the iterator backward to the previous filtered element.
   *
   *  Moves backward in the sequence, skipping elements that don't pass
   *  the filter, until reaching the previous visible element or the beginning.
   */
  void prev() { backward(); }

  /** @brief Moves the iterator backward (noexcept version).
   *
   *  This is the exception-safe version of prev(). All exceptions are
   *  silently caught and the iterator remains in a valid state.
   *
   *  @note This method is noexcept and will never throw.
   */
  void prev_ne() noexcept
  {
    try
      {
        backward();
      }
    catch (...) {}
  }

  /** @brief Resets the iterator to the first filtered element.
   *
   *  Positions the iterator at the first element in the sequence that
   *  passes the filter predicate.
   */
  void reset_first()
  {
    this->It::reset_first();
    goto_first_valid_item();
  }

  /** @brief Resets the iterator to the last filtered element.
   *
   *  Positions the iterator at the last element in the sequence that
   *  passes the filter predicate.
   */
  void reset_last() { goto_last_valid_item(); }

  /** @brief Count the number of elements that pass the filter.
   *
   *  Iterates through the entire sequence and counts elements that
   *  satisfy the filter predicate. This is a destructive operation
   *  that modifies the iterator position.
   *
   *  @return The number of filtered elements.
   *  @note After calling this method, the iterator position is undefined.
   *        Call reset_first() to reposition.
   */
  size_t count()
  {
    size_t n = 0;
    for (reset_first(); this->has_curr(); next())
      ++n;
    return n;
  }

  /** @brief Check if there are no elements that pass the filter.
   *
   *  Resets the iterator to the first position and checks if any
   *  element passes the filter.
   *
   *  @return true if no elements pass the filter, false otherwise.
   *  @note This modifies the iterator position. Call reset_first()
   *        after if you need to iterate.
   */
  bool empty()
  {
    reset_first();
    return not this->has_curr();
  }

  /** @brief Apply a function to all filtered elements.
   *
   *  Iterates through all elements that pass the filter and applies
   *  the given operation to each one.
   *
   *  @tparam Op A callable type that accepts Item_Type.
   *  @param op The operation to apply to each filtered element.
   *  @note This modifies the iterator position.
   *
   *  @code
   *  Filter_Iterator<DynList<int>, DynList<int>::Iterator, ShowEven> it(list);
   *  it.for_each([](int x) { std::cout << x << " "; });
   *  @endcode
   */
  template <typename Op>
  void for_each(Op op)
  {
    for (reset_first(); this->has_curr(); next())
      op(this->get_curr());
  }

  /** @brief Find the first element that satisfies an additional predicate.
   *
   *  Searches through filtered elements until finding one that also
   *  satisfies the given predicate.
   *
   *  @tparam Pred A callable type that accepts Item_Type and returns bool.
   *  @param pred The additional predicate to check.
   *  @return true if an element was found, false otherwise.
   *  @note If found, the iterator is positioned at that element.
   *        If not found, has_curr() returns false.
   */
  template <typename Pred>
  bool find_if(Pred pred)
  {
    for (reset_first(); this->has_curr(); next())
      if (pred(this->get_curr()))
        return true;
    return false;
  }
};

/** @brief Default filter functor that shows all elements.
 *
 *  This filter always returns true, making all elements visible.
 *  Useful as a default filter or when no filtering is needed.
 *
 *  @tparam T The type of elements being filtered.
 */
template <typename T>
struct Show_All
{
  /// Always returns true (shows all elements).
  bool operator()(const T &) const noexcept { return true; }
};

/** @brief Filter functor that hides all elements.
 *
 *  This filter always returns false, making no elements visible.
 *  Useful for testing or special cases.
 *
 *  @tparam T The type of elements being filtered.
 */
template <typename T>
struct Show_None
{
  /// Always returns false (hides all elements).
  bool operator()(const T &) const noexcept { return false; }
};

} // end namespace Aleph

# endif // FILTER_ITERATOR_H
