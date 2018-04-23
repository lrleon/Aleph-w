
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# ifndef TPL_DNODE_H
# define TPL_DNODE_H

# include <type_traits>
# include <ahFunction.H>
# include <dlink.H>

using namespace Aleph;

namespace Aleph {

    /** Node belonging to a double circular linked list with header node.

        @ingroup Secuencias
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
  Dnode<T> *& get_first() const noexcept { return get_next(); } 

  /// Get the last node
  Dnode<T> *& get_last() noexcept { return get_prev(); }

  /// Remove the last node and return its address
  Dnode<T>* remove_last() noexcept 
  { 
    return static_cast<Dnode<T>*>(Dlink::remove_prev()); 
  }

  /// Remove the first node and return its address
  Dnode<T>* remove_first() noexcept
  {
    return static_cast<Dnode<T>*>(Dlink::remove_next()); 
  }
  
  Dnode() noexcept 
  {
    static_assert(std::is_default_constructible<T>::value, 
		  "No default constructor for T");
  }

      /// Construct a node with a copy of `item`
  Dnode(const T & item) noexcept(noexcept(T(item))) : data(item) 
  {
    static_assert(std::is_copy_constructible<T>::value, 
		  "No copy constructor for T");
  }

  /// Construct a new node with the item moved
  Dnode(T && item) noexcept(noexcept(std::swap(data, item)))
  {
    static_assert(std::is_move_constructible<T>::value,
		  "No move constructor for T");
    std::swap(data, item);
  }

  /// Swap `this` with `p`
  Dnode & swap(Dnode & p) noexcept(noexcept(std::swap(data, p.data)))
  {
    
    Dlink::swap(p);
    std::swap(data, p.data);
    return *this;
  }

  /// Copy constructor
  Dnode(const Dnode & node) noexcept(std::is_nothrow_copy_assignable<T>::value)
  : Dlink(node), data(node.data) 
  {
    static_assert(std::is_copy_constructible<T>::value, 
		  "No copy constructor for T");
  }

  /// Move constructor
  Dnode(Dnode && node) noexcept(noexcept(std::forward<T>(node.data)))
  : Dlink(node), data(std::forward<T>(node.data))
  {
    /* empty */ 
  }

  /// Copy assigment
  Dnode & operator = (const Dnode & p) 
  noexcept(std::is_nothrow_copy_assignable<T>::value)
  {
     static_assert(std::is_copy_assignable<T>::value,
		  "No copy assign for T");
    data = p.data;
    return *this;
  }

  /// Move asignment
  Dnode & operator = (Dnode && p) 
  noexcept(std::is_nothrow_move_assignable<T>::value)
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

          @ingroup Secuencias
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

    /** Return a pointer to the current node.

	\throw overflow_error if the iterator is not positioned on a
	current node
    */
    Dnode<T> * get_current() const 
    {
      return static_cast<Dnode<T>*>(this->Dlink::Iterator::get_current()); 
    }

    Dnode<T> * get_curr() const  /// \overload get_current()
    {
      return Dnode<T>::Iterator::get_current(); 
    }

    /** Remove the current node of the list a return a pointer to the
	removed node. 

	The iterator is moved on position forward.

	\throw overflow_error if the iterator is not positioned on a
	current node
    */
    Dnode * del() 
    { return static_cast<Dnode*>(Dlink::Iterator::del()); }
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

