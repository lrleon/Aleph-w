
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


/** @file tpl_snode.H
 *  @brief Typed singly linked node.
 *
 *  Singly linked node storing data of type T.
 *
 *  @see slink.H Base singly linked structure
 *  @see tpl_slist.H Singly linked list
 *  @ingroup Sequences
 *  @author Leandro Rabindranath León
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
      @ingroup Sequences
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

