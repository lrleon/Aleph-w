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

# ifndef FILTER_ITERATOR_H
# define FILTER_ITERATOR_H

# include <stdexcept>
# include <ah-errors.H>

namespace Aleph
{
  /** Generic filter iterator wrapper.

      Filter_Iterator wraps an existing iterator and filters elements
      based on a user-defined predicate. Only elements that satisfy the
      filter predicate are visible during iteration.

      This class is parameterized by the following types:
      -# Container: A container type from the Aleph library (e.g.,
         DynList, DynArray, DynSetTree).
      -# It: An iterator class that must belong to the Container type.
         Since there may be several iterator classes associated with a
         container (Container::Iterator, Container::Rev_Iterator, etc.),
         this parameter allows selecting which one to use.
      -# Show_Item: A functor class that determines whether an element
         should be visible through the iterator. The filter criterion is
         implemented via the boolean call Show_Item::operator()(Item_Type)
         which must return true if the element should be shown, or false
         otherwise.
      .

      The purpose of Filter_Iterator is to provide a generic iterator
      that filters elements according to a criterion defined by the
      Show_Item class. This allows generic algorithms using Filter_Iterator
      to have different behaviors depending on the Show_Item filter.

      Aleph containers export two important types in the context of
      Filter_Iterator: Set_Type and Item_Type, which correspond to the
      type of container being iterated over and the type of element
      returned by the iterator It, respectively.

      Usage example:
      @code
      // Define a filter that shows only even numbers
      struct ShowEven {
        bool operator()(int x) const { return x % 2 == 0; }
      };

      DynList<int> list = {1, 2, 3, 4, 5, 6};
      Filter_Iterator<DynList<int>, DynList<int>::Iterator, ShowEven> it(list);

      for (; it.has_curr(); it.next())
        cout << it.get_curr() << " "; // Prints: 2 4 6
      @endcode

      @tparam Container The container type being iterated over. Must be
              an Aleph container type that supports iteration.
      @tparam It The base iterator type for the container. Should be one
              of the iterator classes provided by Container.
      @tparam Show_Item A functor class that determines element visibility.
              Must provide: `bool operator()(const Item_Type&)` that returns
              true if the item should be shown, false otherwise.

      @note This class is designed for Aleph containers and does not work
            with the standard library (std).
      @ingroup Secuencias
      @author Leandro Rabindranath León
   */
  template <class Container, class It, class Show_Item>
  class Filter_Iterator : public It
  {
    Show_Item show_item;

    const Container *container_ptr = nullptr;

    /// User-defined pointer for extensibility (e.g., passing context data to filter)
    void *cookie = nullptr;

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

    void forward()
    {
      this->It::next();
      try
        { // advance to the next item that passes show_item filter
          for (; true; this->It::next())
              if (not this->It::has_curr() or show_item(this->It::get_curr()))
                return;
        }
      catch (const std::overflow_error &) { /* continue in overflow; do not propagate */ }
    }

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

    void backward()
    {
      this->It::prev();
      try
        {
          for (; true; this->It::prev())
            if (not this->It::has_curr() or show_item(this->It::get_curr()))
              return;
        }
      catch (const std::underflow_error &) { /* continue in underflow; do not propagate */ }
    }

  public:
    /// The type of element returned by get_curr()
    typedef typename It::Item_Type Item_Type;

    /// The type of the base iterator
    typedef It Iterator_Type;

    /** Returns a const reference to the container being iterated over.
        @return Const reference to the container
     */
    const Container &get_container() const
    {
      return *container_ptr;
    }

    /** Returns a reference to the underlying base iterator.
        @return Reference to the base iterator
     */
    It &get_iterator() { return *this; }

    /** Returns a reference to the filter functor.

        Use this function if you need to access or modify the filter state.
        Note that the filter object will be destroyed when the
        ~Filter_Iterator() destructor is called.

        @return Reference to the filter functor
     */
    Show_Item &get_filter() { return show_item; }

    /** Sets a new filter functor.

        This allows changing the filtering criterion after construction.
        After setting a new filter, you may need to call reset_first() or
        reset_last() to reposition the iterator according to the new filter.

        @param si The new filter functor to use
     */
    void set_filter(Show_Item si)
    {
      show_item = si;
    }

    /** Default constructor.

        Creates a filter iterator with only a filter functor.
        The container must be set later or this iterator used with
        copy/assignment from another iterator.

        @param si The filter functor to use (default constructed if not provided)
     */
    Filter_Iterator(Show_Item si = Show_Item()) : show_item(si)
    {
      // empty
    }

    /** Constructs a filter iterator over a container.

        The iterator is automatically positioned at the first element
        that passes the filter predicate.

        @param c The container to iterate over
        @param si The filter functor to use (default constructed if not provided)
        @throw std::bad_alloc if there is not enough memory
     */
    Filter_Iterator(const Container & c, Show_Item si = Show_Item())
      : It(c), show_item(si), container_ptr(&c)
    {
      goto_first_valid_item();
    }

    /** Constructs a filter iterator with a cookie.

        The cookie is a user-defined pointer that can be used to attach
        arbitrary data to the iterator for extensibility purposes.

        @param c The container to iterate over
        @param __cookie User-defined pointer for extensibility
        @param si The filter functor to use (default constructed if not provided)
        @throw std::bad_alloc if there is not enough memory
     */
    Filter_Iterator(const Container & c, void *__cookie,
                    Show_Item si = Show_Item())
      : Filter_Iterator(c, si)
    {
      cookie = __cookie;
    }

    /** Sets the cookie pointer.

        The cookie is a user-defined pointer for extensibility purposes.

        @param __cookie The new cookie value
     */
    void set_cookie(void *__cookie) { cookie = __cookie; }

    /** Advances the iterator to the next filtered element.

        Moves forward in the sequence, skipping elements that don't pass
        the filter, until reaching the next visible element or the end.
     */
    void next() { forward(); }

    /** Advances the iterator to the next filtered element (noexcept version).

        This is the exception-safe version of next(). If an exception occurs
        during iteration (except overflow_error which is caught internally),
        it is silently caught and the iterator remains in a valid state.

        @note This method is noexcept and will never throw
     */
    void next_ne() noexcept
    {
      try
        {
          forward();
        }
      catch (...) {}
    }

    /** Moves the iterator backward to the previous filtered element.

        Moves backward in the sequence, skipping elements that don't pass
        the filter, until reaching the previous visible element or the beginning.
     */
    void prev() { backward(); }

    /** Resets the iterator to the first filtered element.

        Positions the iterator at the first element in the sequence that
        passes the filter predicate.
     */
    void reset_first() { goto_first_valid_item(); }

    /** Resets the iterator to the last filtered element.

        Positions the iterator at the last element in the sequence that
        passes the filter predicate.
     */
    void reset_last() { goto_last_valid_item(); }
  };
} // end namespace Aleph
# endif // FILTER_ITERATOR_H
