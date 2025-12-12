
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

# ifndef TPL_SNODE_H
# define TPL_SNODE_H

# include <slink.H>
# include <utility>

using namespace Aleph;

namespace Aleph {

  /** Singly linked node that stores data of type `T`.

      `Snode` inherits the `Slink` interface and adds the payload held
      by the node.

      @tparam T type of the stored value

      @see DynSlist Snode Slink
      @ingroup Secuencias
   */
template <typename T> 
class Snode : public Slink
{

  T data;

public:

  /// Return a modifiable reference to the stored data.
  T & get_data() { return data; }

  /// Return a constant reference to the stored data.
  const T & get_data() const { return data; }

  /// Default constructor.
  Snode() = default;

  /// Constructor that copies the value.
  Snode(const T & _data) : data(_data) { /* empty */ }

  /// Constructor that moves the value.
  Snode(T && _data) : data(std::move(_data)) { /* empty */ }

  /** Remove the node right after `this` and return it. */
  Snode * remove_next() { return static_cast<Snode*>(Slink::remove_next()); }

  /// Return the next node after `this`.
  Snode *& get_next() { return reinterpret_cast<Snode*&>(Slink::get_next()); }

  /// Return the next node after `this` (const overload).
  const Snode * get_next() const
  {
    return reinterpret_cast<const Snode*>(Slink::get_next());
  }

  /// Remove and return the first node (for symmetry with list front).
  Snode * remove_first() { return remove_next(); }

  /// Return the first node (const alias of `get_next`).
  const Snode * get_first() const { return get_next(); }
};

} // end namespace Aleph

# endif /* TPL_SNODE_H */

