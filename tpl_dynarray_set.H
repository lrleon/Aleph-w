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

/** @file tpl_dynarray_set.H
 *  @brief Array-based dynamic set.
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DYNARRAY_SET_H
# define TPL_DYNARRAY_SET_H

# include <stdexcept>
# include <utility>

# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
 # include <ah-errors.H>

namespace Aleph
{

  /** Set-like container backed by a dynamic array.

      `DynArray_Set<T, Equal>` is a lightweight set-like container that stores
      its elements in a `DynArray<T>`.

      - **Insertion** is very fast (amortized append).
      - **Search** is linear.
      - **Removal** of an already found element is fast (swap-with-last).

      This container **allows duplicates** for performance reasons.

      @tparam T Element type.
      @tparam Equal Equality predicate used for linear search. It must model a
      boolean binary predicate.

      @see DynArray
      @ingroup Sequences
  */
  template <typename T, class Equal = Aleph::equal_to<T>>
  class DynArray_Set : public DynArray<T>
  {
    using Base = DynArray<T>;

    Equal eq;

  public:
    using Item_Type = T;
    using Key_Type = T;

    using Base::Base;

    /** Construct an empty set using a custom equality predicate.

        @param __eq Equality predicate used by linear search methods.
    */
    explicit DynArray_Set(Equal __eq) : Base(), eq(std::move(__eq))
    {
      // empty
    }

    /** Construct a set with a given initial dimension using a custom equality
        predicate.

        @param __eq Equality predicate used by linear search methods.
        @param dim Initial dimension.
    */
    DynArray_Set(Equal __eq, const size_t dim) : Base(dim), eq(std::move(__eq))
    {
      // empty
    }

    /** Construct a set using custom DynArray layout parameters and a custom
        equality predicate.

        @param __eq Equality predicate used by linear search methods.
        @param _pow_dir Directory power-of-two.
        @param _pow_seg Segment power-of-two.
        @param _pow_block Block power-of-two.
    */
    DynArray_Set(Equal __eq, size_t _pow_dir, size_t _pow_seg, size_t _pow_block)
      : Base(_pow_dir, _pow_seg, _pow_block), eq(std::move(__eq))
    {
      // empty
    }

    /** Return the equality predicate used by this container. */
    const Equal & get_equal() const noexcept { return eq; }

    /** Replace the equality predicate used by this container.

        @warning Changing the predicate after inserting elements changes the
        semantics of subsequent `search()` / `contains()` operations.
    */
    void set_equal(Equal __eq) { eq = std::move(__eq); }

    /** Insert `item` (duplicates are allowed).

        @return Pointer to the inserted element inside the container.
        @throw std::bad_alloc if memory allocation fails.
    */
    T * insert(const T & item)
    {
      return &this->append(item);
    }

    /// \overload insert(const T &)
    T * insert(T && item)
    {
      return &this->append(std::forward<T>(item));
    }

    /** Search for the first element equal to `key`.

        @return Pointer to the stored element if found, nullptr otherwise.
    */
    T * search(const T & key) noexcept
    {
      for (size_t i = 0; i < this->size(); ++i)
        if (eq(this->access(i), key))
          return &this->access(i);
      return nullptr;
    }

    /// \overload search(const T &)
    const T * search(const T & key) const noexcept
    {
      return const_cast<DynArray_Set *>(this)->search(key);
    }

    /** Check whether `key` is present.

        @return true if at least one matching element exists.
    */
    bool contains(const T & key) const noexcept
    {
      return search(key) != nullptr;
    }

    /** Find and return a reference to the first matching element.

        @throw std::domain_error if `key` is not found.
    */
    T & find(const T & key)
    {
      auto * p = search(key);
      ah_domain_error_if(p == nullptr) << "key not found";
      return *p;
    }

    /// \overload find(const T &)
    const T & find(const T & key) const
    {
      return const_cast<DynArray_Set *>(this)->find(key);
    }

    /** Count how many occurrences of `key` exist.

        @return Number of occurrences.
    */
    size_t count(const T & key) const noexcept
    {
      size_t n = 0;
      for (size_t i = 0; i < this->size(); ++i)
        if (eq(this->access(i), key))
          ++n;
      return n;
    }

    /** Remove a single occurrence of `key`.

        If there are multiple occurrences, only one is removed.

        @return true if an element was removed; false if not found.
    */
    bool remove_one(const T & key)
    {
      auto * p = search(key);
      if (p == nullptr)
        return false;
      this->remove(*p);
      return true;
    }

    /** Remove all occurrences of `key`.

        @return The number of removed elements.
    */
    size_t remove_all(const T & key)
    {
      size_t removed = 0;
      for (size_t i = 0; i < this->size();)
        {
          if (eq(this->access(i), key))
            {
              this->remove(this->access(i));
              ++removed;
            }
          else
            ++i;
        }
      return removed;
    }
  };

}

# endif // TPL_DYNARRAY_SET_H

