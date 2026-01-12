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

/** @file slink.H
 *  @brief Singly linked list base class.
 *
 *  Low-level singly linked node without data. Base for Snode.
 *
 *  @see tpl_snode.H Typed singly linked node
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
 */

# ifndef SLINK_H
# define SLINK_H

# include <aleph.H>
# include <cstddef>
# include <cstdint>

using namespace Aleph;

namespace Aleph {

  /** Singly linked handle to list nodes.

      `Slink` models a pointer to a node inside a singly linked list.

      @ingroup Sequences
      @see Snode Slist DynSlist Dlink 
  */
  class Slink
  {
  protected:

    Slink * next;

  public:

    /// Default constructor.
    Slink() : next(this) { /* Empty */ }

    /// Reset the link so it points to itself.
    void reset() noexcept
    {
      next = this; 
    }
    /// Return true if this link is empty (points to itself).
    [[nodiscard]] constexpr bool is_empty() const noexcept
    {
      return next == this; 
    }

    /// Return the next link.
    [[nodiscard]] constexpr Slink *& get_next() noexcept
    {
      return next; 
    }

    /// Return the next link (const overload).
    [[nodiscard]] constexpr const Slink * get_next() const noexcept
    {
      return next;
    }

    /** Insert link `p` right after `this`.

        `insert_next(p)` inserts the single link `p` after the node
        whose link is `this`.

        @param[in] p single link to insert
    */
    void insert_next(Slink * p)
    {
      assert(p not_eq nullptr);
      assert(p->is_empty());

      p->next = next;
      next = p;
    }

    /** Remove the link right after `this`.

        `remove_next()` removes the next single link and returns its
        address.

        @return pointer to the removed link
    */
    Slink * remove_next()
    {
      assert(not is_empty());
      Slink * ret_val = next;
      next = ret_val->next;
      ret_val->reset();
      return ret_val;
    }    
  };
  /** Generate a conversion function from the link name to the enclosing
      structure. The function name is literally the parameter provided
      as `link_name`.

      This macro is useful whenever a structure stores two or more
      `Slink` members and we need to retrieve the owning structure from
      a given link pointer.

      For example:
      struct Registro
      {
      ...
      Slink l1;
      Slink l2;
      ...
      };

      Then `SLINK_TO_TYPE(Registro, l1)` and
      `SLINK_TO_TYPE(Registro, l2)` produce the following functions:

      -# `Registro * l1_to_type(Slink * link)`, which receives a pointer
      to the field `l1` and returns the pointer to the owning record.

      -# `Registro * l2_to_type(Slink * link)`, which receives a pointer
      to the field `l2` and returns the pointer to the owning record.

      The goal is to have naming schemes that allow distinguishing the
      different link fields.

      @param type_name type of the enclosing structure (struct or class)
      @param link_name name of the link field inside the structure
      @ingroup Sequences
  */
# define SLINK_TO_TYPE(type_name, link_name)                            \
  static type_name * slink_to_type(Slink * link)                        \
  {                                                                     \
    type_name * ptr_zero = 0;                                           \
    size_t offset_link = (size_t) &(ptr_zero->link_name);               \
    unsigned long address_type = ((unsigned long) link) - offset_link;	\
    return (type_name *) address_type;                                  \
  } 

} // end namespace Aleph
# endif /* SLINK_H */

