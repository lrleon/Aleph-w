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

/** @file tpl_binNode.H
 *  @brief Basic binary tree node definitions.
 *
 *  This file provides the fundamental binary tree node templates including
 *  `BinNode` (basic node) and `BinNodeVtl` (node with virtual destructor).
 *  These serve as building blocks for all binary tree implementations.
 *
 *  @ingroup Trees
 */

# ifndef TPL_BINNODE_H
# define TPL_BINNODE_H

# include <iostream>
# include <stdexcept>
# include <type_traits>
# include <utility>
# include <ahDefs.H>
# include <ahAssert.H>
 # include <ah-errors.H>

/** \addtogroup Arboles
    @{

    General convention for binary trees
    -----------------------------------

    In `Aleph-w` (\f$\aleph_\omega\f$) the binary trees are managed by
    nodes, not by the keys that these contain. Many tree operations,
    concretely those modifying them, take as parameters nodes. For
    example, ig you have a binary search `tree` of integers, and you
    want to insert 10, then you must first allocate the node, put it the
    key and then insert into the tree. Some such as:

    Bintree<int>::Node * p = new Bintree<int>::Node(10);
    tree.insert(p);

    This usage is some complicated and tedious most of the
    time. However, it simplifies enormously the tree algorithms, since
    these do not need to worry by memory management. Eventually, it
    could also simplificate the user's life and definitively improve the
    performance. Suppose for example that you have two trees, and you
    need to remove a key from one and insert it into the another. In
    this case you could do as follows:

    auto ptr = tree.remove(10); // remove node with 10 and return ptr
    tree2.insert(ptr);

    If the tree managed the memory, then the removal from `tree1` would
    perform a `delete`. Afterward, in order to insert 10 into `tree2` it
    would be need to allocate the node, copy it the 10 and insert it
    into the tree. As you see

    If this sound some complicated, do not worry. `Aleph-w`
    (\f$\aleph_\omega\f$) exports other interfaces that wrappers and
    automatize the memory management.

    Extension by inheritance
    ------------------------

    Another advantage of `Aleph-w` (\f$\aleph_\omega\f$) approach for
    binary trees is that it allows to extend the data contained in the
    nodes by inheritance. Suppose that you search by a integer value,
    but that you have several types of data. Consider for example
    `Student` and `Professor` classes, then you could do:

    Professor_Node : public BinTree<int>::Node { ... };
    Student_Node : public BinTree<int>::Node { ... };

    Since that tree operations are by `BinTree<int>::Node`, you could then
    insert student and professors nodes, and eventually other derived
    types, without need of change your insertion code. Of course,
    specific code related to a specific class will need to cast from
    `BinTree<int>::Node` to the correspondent derived class for some
    operations.

    Virtual Destructors
    -------------------

    If you use the technique explained above and the derived class is
    enough complex, then perhaps you need to call to the destructor of
    derived class when you perform `delete` on a node pointer. In this
    case, you need that the destructor of `BinTree<int>::Node` is
    virtual. In this situation you must use `BinTreeVtl<int>` in order
    to indicate that the node destructor is virtual.

    The same approach is used for the remainder of binary trees: `Avl`,
    `Splay`, ...

    This separation is desirable because if you know that do not need to
    call to the derived destructor, then you can save memory by using
    nodes with simple destructors.

    @}
*/


namespace Aleph
{
  struct Empty_Node
  {
    Empty_Node() = default;

    explicit Empty_Node(SentinelCtor) noexcept
    { /* empty */
    }

    static void reset() noexcept
    { /* empty */
    }

    static Empty_Node &get_data()
    {
      ah_domain_error() << "Empty_Node has not data";
      __builtin_unreachable();
    }
  };

# define INIT_CLASS_BINNODE(Name, height, Control_Data)               \
  template <typename Key>                                             \
  class Name : public Control_Data                                    \
  {                                                                   \
public:                                                               \
 static const size_t MaxHeight = height;                              \
 static Name * const NullPtr;                                         \
 typedef Key key_type;                                                \
 typedef Key Key_Type;                                                \
                                                                      \
private:                                                              \
                                                                      \
 Key    key = Key();                                                  \
 Name * lLink;                                                        \
 Name * rLink;                                                        \
                                                                      \
public:                                                               \
                                                                      \
 Key & get_key() noexcept { return key; }                             \
 const Key & get_key() const noexcept { return key; }                 \
 Name *& getL() noexcept { return lLink; }                            \
 Name *& getR() noexcept { return rLink; }                            \
 const Name * getL() const noexcept { return lLink; }                 \
 const Name * getR() const noexcept { return rLink; }                 \
 Name(const Key& k)                              \
   : key(k), lLink(NullPtr), rLink(NullPtr)                           \
 {                                                                    \
   static_assert(std::is_copy_constructible<Key>::value,              \
                 "No copy constructor for Key");                      \
 }                                                                    \
 Name(Key && k) noexcept                                              \
   : key(std::move(k)), lLink(NullPtr), rLink(NullPtr)                \
 {                                                                    \
   static_assert(std::is_move_constructible<Key>::value,              \
                 "No move constructor for Key");                      \
 }                                                                    \
 Name(const Control_Data & control_data, const Key & k)               \
   : Control_Data(control_data),                                      \
     key(k), lLink(NullPtr), rLink(NullPtr)                           \
 {                                                                    \
   /* Empty */                                                        \
 }                                                                    \
 Name(const Name & node)                          \
   : Control_Data(node),                                              \
     key(node.key), lLink(NullPtr), rLink(NullPtr)                    \
 {                                                                    \
   /* Empty */                                                        \
 }                                                                    \
 Name(Name && node)                                                   \
   : Control_Data(std::move(static_cast<Control_Data &>(node))),      \
     key(std::move(node.key)), lLink(NullPtr), rLink(NullPtr)         \
 {                                                                    \
   /* Empty */                                                        \
 }                                                                    \
 Name(const Control_Data & control_data) noexcept :                   \
   Control_Data(control_data),                                        \
   lLink(NullPtr), rLink(NullPtr)                                     \
 {                                                                    \
   /* Empty */                                                        \
 }                                                                    \
 Name()                                      \
 : lLink(NullPtr), rLink(NullPtr)                                     \
 {                                                                    \
   static_assert(std::is_default_constructible<Key>::value,           \
                 "No default constructor for Key");                   \
 }                                                                    \
 void reset() noexcept                                                \
 {                                                                    \
   Control_Data::reset();                                             \
   rLink = lLink = NullPtr;                                           \
 }                                                                    \
 static Name * key_to_node(Key & __key) noexcept                      \
 {                                                                    \
   Name * node_zero = 0;                                              \
   size_t offset = (size_t) &(node_zero->key);                        \
   unsigned long addr = (unsigned long)(&__key);                      \
   return (Name*) (addr - offset);                                    \
 }


  /** Specify tree node for a binary tree.

      `DECLARE_BINNODE(Name, height, Control_Data)` generates two
      classes of binary nodes called `Name` and `NameVtl`,
      respectevely. The only difference is expressed by the fact that
      for `NameVtl` its destructor is virtual.

      Each node has an attribute called `key`, accessible through
      `KEY(p)` or `p->get_key()`, where `p` is a pointer to the node.

      A binary node has two static attributes:

      -# `NullPtr` which represents to the empty tree
      -# `MaxHeight`: an estimated value of maximum height of tree. This
      value is used as helper for recursive and stack based
      algorithms for allocate enough stack space.

      @param Name the name of class defining the node.
      @param height maximum height that could have the tree
      @param Control_Data control data according to tree type
      @see DECLARE_BINNODE_SENTINEL
      @ingroup Arboles
  */
# define DECLARE_BINNODE(Name, height, Control_Data)                    \
  INIT_CLASS_BINNODE(Name, height, Control_Data)                        \
};                                                                      \
  template <typename Key> Name<Key> * const Name<Key>::NullPtr = nullptr; \
  INIT_CLASS_BINNODE(Name##Vtl, height, Control_Data)                   \
  virtual ~Name##Vtl() { /* empty */ }                                  \
};                                                                      \
  template <typename Key> Name##Vtl<Key> *                              \
  const Name##Vtl<Key>::NullPtr = nullptr


  /**  Specify tree node for a binary tree.

       `DECLARE_BINNODE_SENTINEL(Name, height, Control_Data)` generates
       two classes of binary nodes called `Name` and `NameVtl`,
       respectively. The only difference is expressed by the fact that
       for `NameVtl` its destructor is virtual.

       In this version, a special static member called `sentinel_node` is
       declared. In this context, a sentinel node is a node representing
       the empty tree whose state is initialized by the call to
       `Control_Data(sentinelCtor)`.

       Each node has an attribute called `key`, accessible through
       `KEY(p)` or `p->get_key()`, where `p` is a pointer to the node.

       A binary node has two static attributes:

       -# `NullPtr` which represents to the empty tree
       -# `MaxHeight`: an estimated value of maximun height of tree. This
       value is used as helper for recursive and stack based
       algorithms for allocate enough stack space.

       @param Name the name of class defining the node.
       @param height maximun height that could have the tree
       @param Control_Data control data according to tree type

       @see DECLARE_BINNODE
       @ingroup Arboles
  */
# define DECLARE_BINNODE_SENTINEL(Name, height, Control_Data)       \
  INIT_CLASS_BINNODE(Name, height, Control_Data)                    \
  Name(SentinelCtor) :                                              \
  Control_Data(sentinelCtor), lLink(NullPtr), rLink(NullPtr) {}     \
  static Name sentinel_node;                                        \
};                                                                  \
  template <typename Key>                                           \
  Name<Key> Name<Key>::sentinel_node(sentinelCtor);                 \
  template <typename Key>                                           \
  Name<Key> * const Name<Key>::NullPtr = &Name<Key>::sentinel_node;    \
  INIT_CLASS_BINNODE(Name##Vtl, height, Control_Data)               \
  virtual ~Name##Vtl() { /* empty */ }                              \
private:                                                            \
 Name##Vtl(SentinelCtor) :                                          \
 Control_Data(sentinelCtor), lLink(NullPtr), rLink(NullPtr) {}      \
 static Name##Vtl sentinel_node;                                    \
};                                                                  \
  template <typename Key>                                           \
  Name##Vtl<Key> Name##Vtl<Key>::sentinel_node(sentinelCtor);       \
  template <typename Key>                                           \
  Name##Vtl<Key> * const Name##Vtl<Key>::NullPtr =                  \
    &Name##Vtl<Key>::sentinel_node

  /** Return a pointer to left subtree
      @ingroup Arboles
  */
  template <class Node>
  constexpr inline Node *&LLINK(Node *p) noexcept
  {
    return p->getL();
  }

  template <class Node>
  constexpr inline const Node *LLINK(const Node *p) noexcept
  {
    return p->getL();
  }

  /** Return the right tree of p

      @ingroup Arboles
  */
  template <class Node>
  constexpr inline Node *&RLINK(Node *p) noexcept
  {
    return p->getR();
  }

  template <class Node>
  constexpr inline const Node *RLINK(const Node *p) noexcept
  {
    return p->getR();
  }

  /** Return a modifiable reference to the key stored in the node.

      @ingroup Arboles
  */
  template <class Node>
  constexpr inline
  typename Node::Key_Type &KEY(Node *p) noexcept
  {
    return p->get_key();
  }

  template <class Node>
  constexpr inline
  const typename Node::Key_Type &KEY(const Node *p) noexcept
  {
    return p->get_key();
  }

  template <class Node>
  struct node_traits
  {
    using node_type = Node;
    using key_type = typename Node::Key_Type;

    static constexpr node_type * null() noexcept { return node_type::NullPtr; }

    static constexpr node_type *& left(node_type * p) noexcept { return p->getL(); }
    static constexpr node_type *& right(node_type * p) noexcept { return p->getR(); }

    static const node_type * left(const node_type * p) noexcept { return p->getL(); }
    static const node_type * right(const node_type * p) noexcept { return p->getR(); }

    static key_type & key(node_type * p) noexcept { return p->get_key(); }
    static const key_type & key(const node_type * p) noexcept { return p->get_key(); }
  };

  /** \brief Node for binary search tree

      \class BinNode

      @ingroup Arbores
      @ingroup Arboles
  */
  DECLARE_BINNODE(BinNode, 2048, Empty_Node);
} // end namespace Aleph
# endif
