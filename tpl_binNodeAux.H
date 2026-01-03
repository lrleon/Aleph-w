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

/** @file tpl_binNodeAux.H
 *  @brief Binary tree node base definitions and declaration macros.
 *
 *  This file provides macros for declaring binary tree node classes with
 *  customizable control data (balance factors, colors, priorities, etc.).
 *
 *  ## Macro System
 *
 *  The macros generate consistent binary node structures:
 *  - `DECLARE_BINNODE(name, height, Control_Data)`: Creates node class
 *  - `SET_BINNODE_NULL_POINTER(ptr, name)`: Initializes sentinel pointer
 *
 *  ## Generated Class Members
 *
 *  Each declared node class includes:
 *  - `key`: The stored key value
 *  - `lLink`, `rLink`: Left and right child pointers
 *  - `NullPtr`: Static sentinel pointer
 *  - `MaxHeight`: Maximum tree height constant
 *  - Constructors for various initialization scenarios
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Define custom control data (e.g., for AVL balance factor)
 *  class MyNodeData {
 *    int balance = 0;
 *  public:
 *    int& getBalance() { return balance; }
 *    void reset() { balance = 0; }
 *  };
 *
 *  // Declare node type with control data
 *  DECLARE_BINNODE(MyNode, 64, MyNodeData);
 *  SET_BINNODE_NULL_POINTER(nullptr, MyNode);
 *
 *  // Use the node
 *  MyNode<int>* node = new MyNode<int>(42);
 *  ```
 *
 *  @see tpl_binNode.H Extended node with sentinel support
 *  @see tpl_binNodeUtils.H Node utility functions
 *
 *  @ingroup Arboles
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BINNODE_H
# define TPL_BINNODE_H
 
# include <ahDefs.H>
# include <ahAssert.H>
# include <ahNew.H>


/**
 * @brief Macro to generate binary node class body.
 *
 * Creates a template class with:
 * - Key storage and left/right pointers
 * - Static NullPtr sentinel
 * - Multiple constructors
 * - Control data inheritance
 *
 * @param name Class name to generate
 * @param height Maximum tree height (for stack sizing)
 * @param Control_Data Base class with balance/color data
 */
# define CORPUS_BINNODE(name, height, Control_Data)                   \
    template <class Key>                                              \
class name : public Control_Data                                      \
{                                                                     \
public:                                                               \
                                                                      \
  static const size_t MaxHeight = height;                             \
                                                                      \
  static name * NullPtr;                                              \
                                                                      \
private:                                                              \
                                                                      \
  Key    key;                                                         \
  name * lLink;                                                       \
  name * rLink;                                                       \
                                                                      \
public:                                                               \
                                                                      \
  Key& get_key() { return key; }                                       \
                                                                      \
  name*& getL() { return lLink; }                                     \
                                                                      \
  name*& getR() { return rLink; }                                     \
                                                                      \
  name(const Key& k) : key(k), lLink(NullPtr), rLink(NullPtr)         \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data, const Key& k) :             \
    Control_Data(control_data),                                       \
    key(k), lLink(NullPtr), rLink(NullPtr)                            \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(const Control_Data & control_data) :                           \
    Control_Data(control_data),                                       \
    lLink(NullPtr), rLink(NullPtr)                                    \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name() : lLink(NullPtr), rLink(NullPtr)                             \
  {                                                                   \
    /* Empty */                                                       \
  }                                                                   \
                                                                      \
  name(SentinelCtor) : Control_Data(sentinelCtor) { /* Empty */ }     \
                                                                      \
  void reset() { rLink = lLink = NullPtr; }


/**
 * @brief Declare a binary node class and its virtual destructor variant.
 *
 * Creates two classes:
 * - `name<Key>`: Basic node class
 * - `nameVtl<Key>`: Same with virtual destructor for polymorphism
 *
 * @param name Base class name
 * @param height Maximum supported tree height
 * @param Control_Data Base class with node-specific data
 */
# define DECLARE_BINNODE(name, height, Control_Data) \
CORPUS_BINNODE(name, height, Control_Data)           \
};                                                   \
CORPUS_BINNODE(name##Vtl, height, Control_Data)      \
virtual ~name##Vtl() { /* empty */ }                 \
}

/**
 * @brief Initialize the static NullPtr sentinel for a node type.
 *
 * @param ptr Pointer value for sentinel (usually nullptr)
 * @param name Node class name
 */
# define SET_BINNODE_NULL_POINTER(ptr, name)                        \
template <class Key> name<Key> * name<Key>::NullPtr = ptr;          \
template <class Key> name##Vtl<Key> * name##Vtl<Key>::NullPtr = ptr


/// Basic binary node with no extra control data
DECLARE_BINNODE(BinNode, 255, Aleph::Empty_Class);

/// Initialize BinNode sentinel to nullptr
SET_BINNODE_NULL_POINTER(nullptr, BinNode);


# endif
