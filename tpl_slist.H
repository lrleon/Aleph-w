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

/** @file tpl_slist.H
 *  @brief Singly linked list with typed nodes.
 *
 *  This file provides Slist, a simple singly linked list where each node
 *  stores a value of type T. Designed for minimal memory overhead and
 *  simple forward traversal.
 *
 *  ## Key Features
 *
 *  - One pointer per node (minimal overhead)
 *  - O(1) insertion at head
 *  - Forward traversal only
 *  - Move semantics support
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | insert_first | O(1) |
 *  | remove_first | O(1) |
 *  | search | O(n) |
 *  | reverse | O(n) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Slist<int> list;
 *
 *  list.insert_first(new Snode<int>(42));
 *  list.insert_first(new Snode<int>(17));
 *
 *  for (auto it = list.get_first(); it != nullptr; it = it->get_next())
 *    std::cout << it->get_data() << " ";
 *  ```
 *
 *  ## When to Use
 *
 *  - Memory is extremely constrained
 *  - Only forward traversal needed
 *  - LIFO access pattern (stack-like)
 *
 *  @see tpl_dnode.H Doubly linked nodes
 *  @see tpl_dynDlist.H Full-featured doubly linked list
 *  @see tpl_snode.H Node definition
 *
 *  @ingroup Secuencias
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SLIST_H
# define TPL_SLIST_H

# include <ahDefs.H>
# include <tpl_snode.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

  /** Singly linked list of nodes that store values of type `T`.

      `Slist<T>` is a node-based list where each element is a `Snode<T>`
      storing the payload of type `T`.

      @tparam T data type held by each node
      @ingroup Secuencias
  */
  template <typename T> class Slist : public Snode<T>
  {

  public:

    typedef Snode<T> Node;

    /// Default constructor.
    Slist() = default;

    /** Insert `node` right after this sentinel (at the beginning).

        @param[in] node pointer to the node to insert
    */
    void insert_first(Node * node)
    {
      assert(node not_eq nullptr);
      assert(node->is_empty());

      this->insert_next(node);
    }

    /// Remove and return the first node without checking emptiness.
    Node * remove_first_ne() noexcept
    {
      return this->remove_next();
    }

    /** Remove the first node of the list.

        @throw underflow_error if the list is empty
        @return pointer to the removed node
    */
    Node * remove_first()
    {
      ah_underflow_error_if(this->is_empty()) << "list is empty";

      return this->remove_next();
    }

    /// Return a pointer to the first node; throw if the list is empty.
    Node * get_first()
    {
      ah_underflow_error_if(this->is_empty()) << "list is empty";

      return this->get_next();
    }

    /// Return a pointer to the first node (const overload).
    const Node * get_first() const
    {
      ah_underflow_error_if(this->is_empty()) << "list is empty";

      return reinterpret_cast<const Node*>(this->Slink::get_next());
    }

    /// Return the first node without checking emptiness.
    Node * get_first_ne() noexcept { return this->get_next(); }

    /// Return the first node without checking emptiness (const overload).
    const Node * get_first_ne() const noexcept
    {
      return reinterpret_cast<const Node*>(this->Slink::get_next());
    }

    /** Iterator over singly linked nodes.

        This iterator walks the list from the head sentinel forward.

        @ingroup Sequences
        @see Slist
    */
    class Iterator
    {

    private:

      Slist * list;
      Node  * current;

    public:
      /// Type of the set being iterated.
      typedef Snode<T> Set_Type;
      /// Type of the element returned by get_curr().
      typedef Snode<T> * Item_Type;

      /** Construct an iterator over `list`.

          @param[in] _list list to traverse
      */
      explicit Iterator(Slist & _list) noexcept
        : list(&_list), current(_list.is_empty() ? &_list
                                                 : _list.get_first_ne())
      {
      }

      /// Return true if the iterator currently points to a node.
      bool has_curr() const noexcept { return current != list; }

      /** Return the current node.

          @return pointer to the current node
          @throw overflow_error if the iterator is not positioned on a node
      */
      Node * get_curr()
      {
        ah_overflow_error_if(not this->has_curr()) << "Slist::Iterator overflow";

        return current;
      }
      /** Advance the iterator to the next node.

          @throw overflow_error if the iterator is not positioned on a node
      */
      void next()

      {
        ah_overflow_error_if(not this->has_curr()) << "Slist::Iterator overflow";

        current = current->get_next();
      }
      /// Reset the iterator to the first node of the list.
      void reset_first() noexcept
      {
        current = list->is_empty() ? list : list->get_first_ne();
      }

      /** Assign the iterator to point to `node`.

          @param[in] node node to point to
          @return reference to this iterator
      */
      Iterator & operator = (Node * node)
      {
        if (current == node)
          return *this;

        current = node;
        return *this;
      }
    };
  };

} // end namespace Aleph

# endif /* TPL_SLIST_H */

