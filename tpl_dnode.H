
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


/** @file tpl_dnode.H
 *  @brief Doubly linked list node with typed data.
 *
 *  This file provides Dnode<T>, a node for circular doubly linked lists
 *  that inherits from Dlink. Each node stores a value of type T.
 *
 *  ## Key Features
 *
 *  - Circular doubly linked structure
 *  - Inherits navigation from Dlink
 *  - O(1) insertion and removal
 *  - Bidirectional traversal
 *
 *  ## Memory Layout
 *
 *  ```
 *  +------+------+------+
 *  | prev | next | data |
 *  +------+------+------+
 *  ```
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | insert_next/prev | O(1) |
 *  | remove_next/prev | O(1) |
 *  | get_data | O(1) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Dnode<int> head;  // Header node (sentinel)
 *
 *  head.insert_next(new Dnode<int>(42));
 *  head.insert_next(new Dnode<int>(17));
 *
 *  // Traverse forward
 *  for (auto* p = head.get_next(); p != &head; p = p->get_next())
 *    std::cout << p->get_data() << " ";
 *  ```
 *
 *  ## Design Pattern
 *
 *  Uses a sentinel (header) node pattern: the list is never empty as
 *  the header always exists. This simplifies boundary conditions.
 *
 *  @see dlink.H Base doubly linked structure
 *  @see tpl_dynDlist.H High-level doubly linked list
 *  @see tpl_snode.H Singly linked node alternative
 *
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_DNODE_H
# define TPL_DNODE_H

# include <type_traits>
# include <ahFunction.H>
# include <dlink.H>
 # include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

    /** Node belonging to a double circular linked list with header node.

        @ingroup Sequences
        @see Dnode::Iterator
    */ 
template <typename T> class Dnode : public Dlink 
{
private:

  T data;

public:

  /// Return the next node to `this`
  Dnode<T> *& get_next() const noexcept { return (Dnode<T>*&) next; }

  /// Return the previous node to `this`
  Dnode<T> *& get_prev() const noexcept { return (Dnode<T>*&) prev; }

  /// Remove the previous node to `this`; return its address
  Dnode<T>* remove_prev() noexcept { return (Dnode<T>*) Dlink::remove_prev(); }

  /// Remove the next node to `this`; return its address
  Dnode<T>* remove_next() noexcept { return (Dnode<T>*) Dlink::remove_next(); }

  /// Get the first node
  Dnode<T> *& get_first_ne() const noexcept { return get_next(); } 

  /// Get the last node
  Dnode<T> *& get_last_ne() const noexcept { return get_prev(); }

  /// Get the first node
  Dnode<T> *& get_first() const
  {
    ah_underflow_error_if(this->is_empty()) << "Dnode is empty";
    return get_first_ne();
  } 

  /// Get the last node
  Dnode<T> *& get_last() const
  {
    ah_underflow_error_if(this->is_empty()) << "Dnode is empty";
    return get_last_ne();
  }

  /// Remove the last node and return its address
  Dnode<T>* remove_last_ne() noexcept 
  { 
    return static_cast<Dnode<T>*>(Dlink::remove_prev()); 
  }

  /// Remove the first node and return its address
  Dnode<T>* remove_first_ne() noexcept
  {
    return static_cast<Dnode<T>*>(Dlink::remove_next()); 
  }

  /// Remove the last node and return its address
  Dnode<T>* remove_last()
  {
    ah_underflow_error_if(this->is_empty()) << "Dnode is empty";
    return remove_last_ne();
  }

  /// Remove the first node and return its address
  Dnode<T>* remove_first()
  {
    ah_underflow_error_if(this->is_empty()) << "Dnode is empty";
    return remove_first_ne();
  }
  
  Dnode() noexcept 
  {
    static_assert(std::is_default_constructible<T>::value, 
		  "No default constructor for T");
  }

      /// Construct a node with a copy of `item`
  Dnode(const T & item) : data(item) 
  {
    static_assert(std::is_copy_constructible<T>::value, 
		  "No copy constructor for T");
  }

  /// Construct a new node with the item moved
  Dnode(T && item) : data(std::move(item))
  {
    static_assert(std::is_move_constructible<T>::value,
		  "No move constructor for T");
  }

  /// Swap `this` with `p`
  Dnode & swap(Dnode & p)
  {
    
    Dlink::swap(p);
    std::swap(data, p.data);
    return *this;
  }

  /// Copy constructor
  Dnode(const Dnode & node)
  : Dlink(node), data(node.data) 
  {
    static_assert(std::is_copy_constructible<T>::value, 
		  "No copy constructor for T");
  }

  /// Move constructor
  Dnode(Dnode && node) noexcept
  : Dlink(std::move(node)), data(std::move(node.data))
  {
    /* empty */ 
  }

  /// Copy assigment
  Dnode & operator = (const Dnode & p) 
  {
     static_assert(std::is_copy_assignable<T>::value,
		  "No copy assign for T");
    data = p.data;
    return *this;
  }

  /// Move asignment
  Dnode & operator = (Dnode && p) 
  {
    std::swap(data, p.data);
    return *this;
  }

    /// Return a modifiable reference to the data contained in the node
  T & get_data() noexcept { return data; }

    /// Return a modifiable reference to the data contained in the node
  const T & get_data() const noexcept { return data; }

     /// The data type 
  using key_type = T;

  /// \overload get_data()
  T & get_key() noexcept { return data; }

  /// \overload get_data()
  const T & get_key() const noexcept { return data; }

 /// Given an reference to the data in the node, returns a pointer to
 /// the `Dnode` object that contains it.
  static Dnode * data_to_node(T & data) noexcept
  {
    Dnode * zero = 0;
    long offset = (long) &(zero->data);
    unsigned long  addr = (unsigned long) (&data);
    return (Dnode*) (addr - offset);
  }

      /** Iterator on a list of `Dnode` objects

          @ingroup Sequences
       */
  class Iterator : public Dlink::Iterator
  {
  public:
    
        /// The type of set
    using Set_Type = Dnode<T>;
    
        /// The type of element of set
    using Item_Type = Dnode<T>*;

    using Base = Dlink::Iterator;

    using Base::Base;

        /// Return the current link guaranteeing no exception. Be careful.
    Dnode<T> * get_curr_ne() const noexcept
    {
      return static_cast<Dnode<T>*>(this->Dlink::Iterator::get_curr_ne()); 
    }

    /** Return a pointer to the current node.

	\throw overflow_error if the iterator is not positioned on a
	current node
    */
    Dnode<T> * get_curr() const 
    {
      return static_cast<Dnode<T>*>(this->Dlink::Iterator::get_curr()); 
    }

    /** Remove the current node of the list a return a pointer to the
	removed node. 

	The iterator is moved on position forward.

	\throw overflow_error if the iterator is not positioned on a
	current node
    */
    Dnode * del() 
    { return static_cast<Dnode*>(Dlink::Iterator::del()); }

    /// Same as del() but without exception check
    Dnode * del_ne() noexcept
    { return static_cast<Dnode*>(Dlink::Iterator::del_ne()); }
  };
};

template <typename T> Dnode<T> * Dlink::to_dnode() noexcept
{
  return static_cast<Dnode<T>*>(this);
}

template <typename T> const Dnode<T> * Dlink::to_dnode() const noexcept
{
  return static_cast<const Dnode<T>*>(this);
}

template <typename T> T & Dlink::to_data() noexcept
{
  return to_dnode<T>()->get_data();
}

template <typename T> const T & Dlink::to_data() const noexcept
{
  return to_dnode<T>()->get_data();
}


}

# endif /* TPL_DNODE_H */

