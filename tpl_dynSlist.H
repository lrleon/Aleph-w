
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

# ifndef TPL_DYNSLIST_H
# define TPL_DYNSLIST_H

# include <tpl_slist.H>
# include <cstddef>
# include <stdexcept>
# include <utility>

using namespace Aleph;

namespace Aleph {

  /** Dynamic list of elements of type `T` implemented with a singly
      linked list of nodes.

      @tparam T data type stored in each node

      @see Slist Snode Slink
      @ingroup Secuencias
  */
  template <typename T>
  class DynSlist : public Slist<T>
  {
    size_t num_items;
    size_t      current_pos;
    Snode<T> * current_node;
    typename Slist<T>::Node *
    get_previous_to_pos(size_t pos, bool allow_equal)
    {
      if (pos > num_items || (not allow_equal and pos == num_items))
        throw std::out_of_range ("position out of range");

      if (pos < current_pos)
        {
          current_pos  = 0;
          current_node = this;
        }
      while (current_pos < pos)
        {
          current_node = current_node->get_next();
          ++current_pos;
        }
      return current_node;
    }

  public:

    /// Constructor.
    DynSlist() : num_items(0), current_pos(0), current_node(this)
    {
      // Empty
    }

    /** Access the element at position `i`.

        Access is `O(i)` because it requires traversing `i` nodes.

        @param[in] i zero-based index to access
        @throw std::out_of_range if `i` is outside `[0, size()]`
        @return reference to the element at position `i`
    */
    T & operator [] (size_t i)
    {
      return get_previous_to_pos(i, false)->get_next()->get_data();
    }

    /// Const overload of `operator[]`.
    const T & operator[](size_t i) const
    {
      return const_cast<DynSlist&>(*this)[i];
    }

    /// Return the number of stored elements.
    size_t size() const noexcept { return num_items; }

    /** Insert an element at position `pos`.

        @param[in] pos insertion position in `[0, size()]`
        @param[in] data value to store
        @throw std::out_of_range if `pos` is invalid
        @throw std::bad_alloc on memory exhaustion
    */
    void insert(int pos, const T & data)
    {
      auto * node = new typename Slist<T>::Node (data);
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), true);
      prev->insert_next(node);
      ++num_items;
    }

    /// Insert an element by moving the provided value.
    void insert(int pos, T && data)
    {
      auto * node = new typename Slist<T>::Node (std::move(data));
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), true);
      prev->insert_next(node);
      ++num_items;
    }

    /** Remove the node at position `pos`.

        @param[in] pos zero-based position to erase
        @throw std::out_of_range if `pos` is invalid
    */
    void remove(int pos)
    {
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), false);
      auto * node_to_delete = prev->remove_next();
      delete node_to_delete;
      --num_items;
      current_node = prev;
    }

    /// Destructor.
    virtual ~DynSlist()
    {      // eliminar nodo por nodo hasta que la lista devenga vacía
      while (not this->is_empty())
        delete this->remove_first_ne(); // remove_first de clase Slink
    }

    /** Iterator specialized for `DynSlist` returning payload references.

        @ingroup Sequences
    */
    class Iterator : public Slist<T>::Iterator
    {
    public:
      /// Type of the data structure being iterated.
      typedef Slist<T> Set_Type;
      /// Type returned by `get_curr()`.
      typedef T Item_Type;

      /// Constructor.
      explicit Iterator(DynSlist & list) : Slist<T>::Iterator(list) { }

      /// Return a reference to the current payload.
      T & get_curr() { return Slist<T>::Iterator::get_curr()->get_data(); }

      /// Return a const reference to the current payload.
      const T & get_curr() const
      {
        return Slist<T>::Iterator::get_curr()->get_data();
      }
    };
  };

} // end namespace Aleph

# endif // TPL_DYNSLIST_H

