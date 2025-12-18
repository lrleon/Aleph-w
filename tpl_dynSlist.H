
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
# include <ah-errors.H>

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

    void clear() noexcept
    {
      while (not this->is_empty())
        delete this->remove_first_ne();
      num_items    = 0;
      current_pos  = 0;
      current_node = this;
    }
    typename Slist<T>::Node *
    get_previous_to_pos(const size_t pos, const bool allow_equal)
    {
      ah_out_of_range_error_if(pos > num_items || (not allow_equal and pos == num_items))
        << "position out of range";

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

    DynSlist(const DynSlist & other) : DynSlist()
    {
      using Node = typename Slist<T>::Node;

      Node * head = nullptr;
      Node * tail = nullptr;
      size_t n = 0;

      try
        {
          const Node * end = static_cast<const Node*>(&other);
          for (const Node * cur = other.get_first_ne(); cur != end;
               cur = cur->get_next())
            {
              auto * node = new Node(cur->get_data());
              node->get_next() = nullptr;

              if (tail == nullptr)
                head = tail = node;
              else
                {
                  tail->get_next() = node;
                  tail = node;
                }
              ++n;
            }
        }
      catch (...)
        {
          while (head != nullptr)
            {
              Node * next = head->get_next();
              delete head;
              head = next;
            }
          throw;
        }

      if (head != nullptr)
        {
          this->get_next() = head;
          tail->get_next() = this;
        }

      num_items    = n;
      current_pos  = 0;
      current_node = this;
    }

    DynSlist & operator=(const DynSlist & other)
    {
      if (this == &other)
        return *this;

      using Node = typename Slist<T>::Node;

      Node * head = nullptr;
      Node * tail = nullptr;
      size_t n = 0;

      try
        {
          const Node * end = static_cast<const Node*>(&other);
          for (const Node * cur = other.get_first_ne(); cur != end;
               cur = cur->get_next())
            {
              auto * node = new Node(cur->get_data());
              node->get_next() = nullptr;

              if (tail == nullptr)
                head = tail = node;
              else
                {
                  tail->get_next() = node;
                  tail = node;
                }
              ++n;
            }
        }
      catch (...)
        {
          while (head != nullptr)
            {
              Node * next = head->get_next();
              delete head;
              head = next;
            }
          throw;
        }

      clear();

      if (head != nullptr)
        {
          this->get_next() = head;
          tail->get_next() = this;
        }

      num_items    = n;
      current_pos  = 0;
      current_node = this;
      return *this;
    }

    DynSlist(DynSlist && other) noexcept : DynSlist()
    {
      using Node = typename Slist<T>::Node;

      if (other.is_empty())
        return;

      Node * other_sentinel = static_cast<Node*>(&other);
      Node * head = other_sentinel->get_next();
      Node * tail = head;
      while (tail->get_next() != other_sentinel)
        tail = tail->get_next();

      this->get_next() = head;
      tail->get_next() = this;

      num_items = other.num_items;

      other_sentinel->reset();
      other.num_items    = 0;
      other.current_pos  = 0;
      other.current_node = &other;

      current_pos  = 0;
      current_node = this;
    }

    DynSlist & operator=(DynSlist && other) noexcept
    {
      if (this == &other)
        return *this;

      using Node = typename Slist<T>::Node;

      clear();

      if (other.is_empty())
        return *this;

      Node * other_sentinel = static_cast<Node*>(&other);
      Node * head = other_sentinel->get_next();
      Node * tail = head;
      while (tail->get_next() != other_sentinel)
        tail = tail->get_next();

      this->get_next() = head;
      tail->get_next() = this;

      num_items = other.num_items;

      other_sentinel->reset();
      other.num_items    = 0;
      other.current_pos  = 0;
      other.current_node = &other;

      current_pos  = 0;
      current_node = this;

      return *this;
    }

    /** Access the element at position `i`.

        Access is `O(i)` because it requires traversing `i` nodes.

        @param[in] i zero-based index to access
        @throw std::out_of_range if `i` is outside `[0, size()]`
        @return reference to the element at position `i`
    */
    T & operator [] (const size_t i)
    {
      return get_previous_to_pos(i, false)->get_next()->get_data();
    }

    /// Const overload of `operator[]`.
    const T & operator[](const size_t i) const
    {
      ah_out_of_range_error_if(i >= num_items) << "position out of range";

      using Node = typename Slist<T>::Node;
      const Node * cur = this->get_first_ne();

      for (size_t idx = 0; idx < i; ++idx)
        cur = cur->get_next();

      return cur->get_data();
    }

    /// Return the number of stored elements.
    size_t size() const noexcept { return num_items; }

    /** Insert an element at position `pos`.

        @param[in] pos insertion position in `[0, size()]`
        @param[in] data value to store
        @throw std::out_of_range if `pos` is invalid
        @throw std::bad_alloc on memory exhaustion
    */
    void insert(const int pos, const T & data)
    {
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), true);
      auto * node = new typename Slist<T>::Node (data);
      prev->insert_next(node);
      ++num_items;
      current_pos  = 0;
      current_node = this;
    }

    /// Insert an element by moving the provided value.
    void insert(const int pos, T && data)
    {
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), true);
      auto * node = new typename Slist<T>::Node (std::move(data));
      prev->insert_next(node);
      ++num_items;
      current_pos  = 0;
      current_node = this;
    }

    /** Remove the node at position `pos`.

        @param[in] pos zero-based position to erase
        @throw std::out_of_range if `pos` is invalid
    */
    void remove(const int pos)
    {
      auto * prev = get_previous_to_pos(static_cast<size_t>(pos), false);
      auto * node_to_delete = prev->remove_next();
      delete node_to_delete;
      --num_items;
      current_pos  = 0;
      current_node = this;
    }

    virtual ~DynSlist() { clear(); }

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

