
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

# ifndef TPL_BINNODEXT_H
# define TPL_BINNODEXT_H

# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace Aleph {

class BinNodeXt_Data
{
  size_t count; // cardinalidad del árbol

public:

  BinNodeXt_Data() noexcept : count(1) {}
  BinNodeXt_Data(SentinelCtor) noexcept : count(0) {}

  size_t & getCount() noexcept { return count; }
  size_t size() const noexcept { return count; }

  void reset() noexcept { count = 1; }
};

/** \brief Node for extended binary search tree

    \class BinNodeXt

    @ingroup Arboles
*/
DECLARE_BINNODE_SENTINEL(BinNodeXt, 255, BinNodeXt_Data);

  /** Return the number of nodes of the tree fron `p` is root.

      @param p pointer to root
      @ingroup Arboles
   */
template <class Node> inline auto & COUNT(Node * p) noexcept
{
  return p->getCount();
}


    template <class Node> static inline
Node * __select_rec(Node * r, const size_t i) noexcept
{
  assert(r != Node::NullPtr);
  assert(COUNT(Node::NullPtr) == 0);

  if (i == COUNT(LLINK(r)))
    return r;

  if (i < COUNT(LLINK(r)))
    return __select_rec(LLINK(r), i);

  return __select_rec(RLINK(r), i - COUNT(LLINK(r)) - 1);
}

  /** Recursively select the i-th node inorder sense.

      @param[in] r root of tree
      @param[in] i position inorder sense
      @return pointer to the i-th node
      @throw out_of_range if `i` is greater or equal than the number of nodes
      of overall  tree
      @see select()

      @ingroup Arboles
  */
    template <class Node> inline
Node * select_rec(Node * r, const size_t i)
{
  if (i >= COUNT(r))
    throw std::out_of_range("infix position out of range");

  return __select_rec(r, i);
}

    /** Iterative selection of a node according to inorder position
        without exception.

        @param[in] r root of tree
        @param[in] pos position inorder whose node wants to be located
        @return a pointer to the node at the inorder position pos

        @see select_rec()
        @ingroup Arboles
     */
    template <class Node> inline
Node * select_ne(Node * r, const size_t pos) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);
  for (size_t i = pos; i != COUNT(LLINK(r)); /* nada */)
    {
      assert(i < COUNT(r) and
	     COUNT(LLINK(r)) + COUNT(RLINK(r)) + 1 == COUNT(r));

      if (i < COUNT(LLINK(r)))
        r = LLINK(r);
      else
        {
          i -= COUNT(LLINK(r)) + 1;
          r = RLINK(r);
        }
    }

  return r;
}

      /** Iterative selection of a node according to inorder psoition

        @param[in] r root of tree
        @param[in] pos position inorder whose node wants to be located
        @return a pointer to the node at the inorder position pos
        @throw out_of_range if pos is greater or equal than the number
        of nodes.

        @see select_rec()
        @ingroup Arboles
     */
    template <class Node> inline
Node * select(Node * r, const size_t pos)
{
  assert(COUNT(Node::NullPtr) == 0);
  if (pos >= COUNT(r))
    throw std::out_of_range("infix position out of range");

  return select_ne(r, pos);
}

    /** Iterative selection of a node according to inorder psoition and
	determination of parent of selected node

        @param[in] r root of tree
        @param[in] pos position inorder whose node wants to be located
        @param[out] parent of selected node
        @return a pointer to the node at the inorder position pos
        @throw out_of_range if pos is greater or equal than the number
        of nodes.

        @see select_rec()
        @ingroup Arboles
     */
    template <class Node> inline
Node * select(Node * root, const size_t pos, Node *& parent)
{
  assert(COUNT(Node::NullPtr) == 0);

  if (pos >= COUNT(root))
    throw std::out_of_range("infix position out of range");

  parent = Node::NullPtr;
  for (size_t i = pos; i != COUNT(LLINK(root)); /* nada */)
    {
      assert(i < COUNT(root) and
	     COUNT(LLINK(root)) + COUNT(RLINK(root)) + 1 == COUNT(root));

      parent = root;
      if (i < COUNT(LLINK(root)))
        root = LLINK(root);
      else
        {
          i -= COUNT(LLINK(root)) + 1;
          root = RLINK(root);
        }
    }

  return root;
}

/** Compute the inorder position of a key

    @param[in] r root of tree
    @param[in] key to be searched
    @param[out] p pointer to the node containing `key`
    @param[in] cmp comparison criteria
    @return inorder position of `key`

    @ingroup Arboles
*/
    template <class Node, class Compare> inline
long inorder_position(Node * r,
                      const typename Node::key_type & key,
                      Node *& p, Compare & cmp) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr)
    return -1;

  if (cmp(key, KEY(r)))
    return inorder_position(LLINK(r), key, p, cmp);
  else if (cmp(KEY(r), key))
    {
      long ret = inorder_position(RLINK(r), key, p, cmp);
      if (ret != -1)
        return ret + COUNT(LLINK(r)) + 1;
      return ret;
    }
  else
    {
      p = r;
      return COUNT(LLINK(r));
    }
}

/// \overload inorder_position()
template <class Node, class Compare = Aleph::less<typename Node::key_type>>
      inline long inorder_position(Node * r,
                                   const typename Node::key_type & key,
                                   Node *& p, Compare && cmp = Compare())
  noexcept
{
  return inorder_position(r, key, p, cmp);
}

/// \overload inorder_position()
    template <class Node, class Compare> inline
long inorder_position(Node * r, const typename Node::key_type & key,
                      Compare & cmp) noexcept
{
  Node * p = nullptr;
  return inorder_position(r, key, p, cmp);
}

/// \overload inorder_position()
    template <class Node, class Compare = Aleph::less<typename Node::key_type>>
inline long inorder_position(Node * r, const typename Node::key_type & key,
			     Compare && cmp = Compare()) noexcept
{
  return inorder_position(r, key, cmp);
}

     /** Find the inorder position of a key in an extended binary search
	 tree.

        `find_position(r, key, p, cmp)` searches `key` in the tree with root
        `r`. If `key` is found then its inorder position is returned and
        a pointer to the node containing the key is written in output
        parameter `p`. Otherwise, the function returns the position
        that would have `key` if this was in the tree. At this regard,
        there are three cases:
        -# if `key` is lesser than the minimum key of tree, then -1 is
        returned and `p` is the node with the smallest key.
        -# If `key` is greater than the maximum key in the tree, then
        the number of keys is returned and `p` is the node with the
        maximum key in the tree.
        -# For any other case, the returned value is the position that
        would have `key` if this was in the tree and `p` is the node
        whose key is inmediataly greater than `key`.

        @param[in] r root of tree
        @param[in] key to be searched
        @param[out] p reference to pointer to node
        @param[in] cmp comparison criteria
        @return the positions of `key` in the tree

        @ingroup Arboles
     */
    template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
long find_position(Node * r, const typename Node::key_type & key,
                   Node *& p, Compare & cmp) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);

  Node * parent = nullptr;
  long pos = COUNT(LLINK(r));

  while (r != Node::NullPtr)
    if (cmp(key, KEY(r)))
      {
        parent = r;
        r = LLINK(r);
        pos -= (COUNT(RLINK(r)) + 1);
      }
    else if (cmp(KEY(r), key))
      {
        parent = r;
        r = RLINK(r);
        pos += COUNT(LLINK(r)) + 1;
      }
    else
      {
        p = r;
        return pos;
      }

  p = parent;

  return pos;
}

template <class Node,
	  class Compare = Aleph::less<typename Node::key_type>> inline
long find_position(Node * r, const typename Node::key_type & key,
                   Node *& p, Compare && cmp = Compare()) noexcept
{
  return find_position(r, key, p, cmp);
}

    /** Insert a node in an extended binary search tree.

        `insert_by_key_xt(root, p, cmp) inserts the node `p` in the
        extended binary search tree with root `r`.

        @param[in,out] r the tree root
        @param[in] p the node to insert
        @param[in] cmp comparison criteria
        @return if `p->get_key()` is not in the tree, then `p` is
        returned (is was inserted). Otherwise it returns `Node::NullPtr`

        @ingroup Arboles
     */
    template <class Node, class Compare> inline
Node * insert_by_key_xt(Node *& r, Node * p, Compare & cmp) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr)
    return r = p;

  Node * q = Node::NullPtr;
  if (cmp(KEY(p), KEY(r)))
    {
      q = insert_by_key_xt(LLINK(r), p, cmp);
      if (q != Node::NullPtr)
        ++COUNT(r);
    }
  else if (cmp(KEY(r), KEY(p)))
    {
      q = insert_by_key_xt(RLINK(r), p, cmp);
      if (q != Node::NullPtr)
        ++COUNT(r);
    }
  // else return Node::NullPtr; is not needed

  return q;
}

/// \overload insert_by_key_xt(Node *& r, Node * p, Compare & cmp)
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>> inline
Node * insert_by_key_xt(Node *& r, Node * p, Compare && cmp = Compare())
    noexcept
{
  return insert_by_key_xt(r, p, cmp);
}

/** Insert a node in an extended binary search tree without testing
    for duplicity

    @param[in,out] r the tree root
    @param[in] p pointer to the node to be inserted
    @param[in] cmp comparison criteria
    @return the pointer p

    @ingroup Arboles
*/
    template <class Node, class Compare> inline
Node * insert_dup_by_key_xt(Node *& r, Node * p, Compare & cmp) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr)
    return r = p;

  Node * q;
  if (cmp(KEY(p), KEY(r)))
    q = insert_dup_by_key_xt(LLINK(r), p, cmp);
  else
    q = insert_dup_by_key_xt(RLINK(r), p, cmp);

  ++COUNT(r);

  return q;
}

/// \overload insert_dup_by_key_xt(Node *& r, Node * p, Compare & cmp)
    template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
Node * insert_dup_by_key_xt(Node *& r, Node * p, Compare && cmp = Compare())
      noexcept
{
  return insert_by_key_xt(r, p, cmp);
}

/** Search or insert a node in an extended binary search tree.

    `search_or_insert_by_key_xt(r, p, cmp)` searches in the extended
    tree with root `r` the key contained in node `p`. If the key is
    found, the function returns the containing the key; otherwise `p` is
    inserted in the tree and this pointer is returned.

    @param[in] r root of tre
    @param[in] p pointer to node to search or insert
    @return if `p` is inserted, then this is returned; otherwise the
    pointer to the node containing the key of is returned
*/
    template <class Node, class Compare> inline
Node * search_or_insert_by_key_xt(Node *& r, Node * p, Compare & cmp) noexcept
{
  assert(COUNT(Node::NullPtr) == 0);

  if (r == Node::NullPtr)
    return r = p;

  Node * q;
  if (cmp(KEY(p), KEY(r)))
    {
      q = insert_by_key_xt(LLINK(r), p, cmp);
      if (q != Node::NullPtr)
        ++COUNT(r);
    }
  else if (cmp(KEY(r), KEY(p)))
    {
      q = insert_by_key_xt(RLINK(r), p, cmp);
      if (q != Node::NullPtr)
        ++COUNT(r);
    }
  else
    return r;

  return q;
}

/// \overload search_or_insert_by_key_xt(Node *& r, Node * p, Compare & cmp)
template <class Node,
	  class Compare = Aleph::less<typename Node::key_type>> inline
Node * search_or_insert_by_key_xt(Node *& r, Node * p,
                                  Compare && cmp = Compare()) noexcept
{
  return search_or_insert_by_key_xt(r, p, cmp);
}


    template <class Node, class Compare> static inline
bool __split_key_rec_xt(Node * root, const typename Node::key_type & key,
                        Node *& l, Node *& r, Compare & cmp) noexcept
{
  if (root == Node::NullPtr)
    {
      l = r = Node::NullPtr;
      return true;
    }

  if (cmp(key, KEY(root)))
    {
      if (not __split_key_rec_xt(LLINK(root), key, l, LLINK(root), cmp))
        return false;

      r = root;
      COUNT(r) -= COUNT(l);
    }
  else if (cmp(KEY(root), key))
    {
      if (not __split_key_rec_xt(RLINK(root), key, RLINK(root), r, cmp))
        return false;

      l = root;
      COUNT(l) -= COUNT(r);
    }
  else
    return false;

  return true;
}

/** Split an extended binary search tree accoding to a key

    `split_key_rec_xt(root, key, l, r, cmp)` splits a tree according a
    non existing key

    @param[in,out] root pointer to tree root
    @param[in] key for splitting
    @param[out] l tree with keys lesser than `key`
    @param[out] r tree with keys greater than `key`
    @return `true` if tree was split; that is if `key` is not in the
    tree. Otherwise, if `key` is in the tree, `false` is returned

    @ingroup Arboles
*/
template <class Node, class Compare> inline
bool split_key_rec_xt(Node *& root, const typename Node::key_type & key,
                      Node *& l, Node *& r, Compare & cmp) noexcept
{
  bool ret = __split_key_rec_xt(root, key, l, r, cmp);
  if (ret)
    root = Node::NullPtr;
  return ret;
}

/// \overload split_key_rec_xt()
template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
bool split_key_rec_xt(Node *& root, const typename Node::key_type & key,
                      Node *& l, Node *& r, Compare && cmp = Compare())
  noexcept
{
  return split_key_rec_xt(root, key, l, r, cmp);
}


    template <class Node, class Compare> static inline
void __split_key_dup_rec_xt(Node * root, const typename Node::key_type & key,
                            Node *& l, Node *& r, Compare & cmp) noexcept
{
  if (root == Node::NullPtr)
    {
      l = r = Node::NullPtr;
      return;
    }

  if (cmp(key, KEY(root)))
    {
      __split_key_dup_rec_xt(LLINK(root), key, l, LLINK(root), cmp);
      r = root;
      COUNT(r) -= COUNT(l);
    }
  else
    {
      __split_key_dup_rec_xt(RLINK(root), key, RLINK(root), r, cmp);
      l = root;
      COUNT(l) -= COUNT(r);
    }
}


/** Split an extended binary search tree accoding to a key which can be
    in the tree.

    `split_key__dup_rec_xt(root, key, l, r, cmp)` splits a tree according a
    key. The key can be in the tree.

    @param[in,out] root pointer to tree root
    @param[in] key for splitting
    @param[out] l tree with keys lesser than `key`
    @param[out] r tree with keys greater than `key`
    @param[in] cmp comparison criteria

    @ingroup Arboles
*/
 template <class Node, class Compare> inline
void split_key_dup_rec_xt(Node *& root, const typename Node::key_type & key,
                          Node *& l, Node *& r, Compare & cmp) noexcept
{
  __split_key_dup_rec_xt<Node, Compare>(root, key, l, r, cmp);
  root = Node::NullPtr;
}

/// \overload split_key_dup_rec_xt()
 template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
void split_key_dup_rec_xt(Node *& root, const typename Node::key_type & key,
                          Node *& l, Node *& r, Compare && cmp = Compare())
   noexcept
{
  return split_key_dup_rec_xt(root, key, l, r, cmp);
}

    /** Insert a node `p` as root of an extended binary search tree.

        `insert_root_xt(root, p)` inserts as root in the extended binary search
        tree `root` the node `p`. After insertion, if there is no
        duplicated key, `p` becomes the root of the tree.

        @param[in,out] root of tree
        @param[in] p pointer to the node to insert
        @param[in] cmp comparison criteria
        @return if the key contained in `p` is not in tree, then returns
        `p`, since this was inserted and has became the root. Otherwise,
        it returns `Node::NullPtr`

        @ingroup Arboles
     */
    template <class Node, class Compare> inline
Node * insert_root_xt(Node *& root, Node * p, Compare & cmp) noexcept
{
  if (root == Node::NullPtr)
    {
      root = p;
      return p;
    }

  if (not split_key_rec_xt(root, KEY(p), LLINK(p), RLINK(p), cmp))
    return Node::NullPtr;

  COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
  root = p;

  return p;
}

/// \overload insert_root_xt(Node *& root, Node * p, Compare & cmp)
    template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
Node * insert_root_xt(Node *& root, Node * p, Compare && cmp = Compare()) 
      noexcept
{
  return insert_root_xt(root, p, cmp);
}

    /** Insert a node as root of an extended binary search tree.

        `insert_dup_root_xt(root, p, cmp)` inserts the node `p` as the
        new root of the tree `root`.

        This insertion allows duplicates.

        @param[in,out] root of tree
        @param[in] p pointer to the to insert
        @param[in] cmp comparison criteria
        @return pointer to the inserted node `p` that has became root

        @ingroup Arboles
     */
    template <class Node, class Compare> inline
Node * insert_dup_root_xt(Node *& root, Node * p, Compare & cmp) noexcept
{
  if (root == Node::NullPtr)
    return p;

  split_key_dup_rec_xt(root, KEY(p), LLINK(p), RLINK(p), cmp);
  COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;

  return root = p;
}

/// \overload insert_dup_root_xt(Node *& root, Node * p, Compare & cmp)
template <class Node,
	  class Compare = Aleph::less<typename Node::key_type>> inline
Node * insert_dup_root_xt(Node *& root, Node * p, Compare && cmp = Compare())
   noexcept
{
  return insert_dup_root_xt(root, p, cmp);
}


    template <class Node> static inline
void __split_pos_rec(Node * r, size_t i, Node *& ts, Node *& tg) noexcept
{
  if (i == COUNT(LLINK(r)))
    {
      ts = LLINK(r);
      tg = r;
      LLINK(tg) = Node::NullPtr;
      COUNT(tg) -= COUNT(ts);
      return;
    }

  if (i < COUNT(LLINK(r)))
    {
      __split_pos_rec(LLINK(r), i, ts, LLINK(r));
      tg = r;
      COUNT(r) -= COUNT(ts);
    }
  else
    {
      __split_pos_rec(RLINK(r), i - (COUNT(LLINK(r)) + 1), RLINK(r), tg);
      ts = r;
      COUNT(r) -= COUNT(tg);
    }
}


 /**  Split a extended binary tree according to a position

      `split_pos_rec(r, i, ts, tg)` splits the tree with root `r` en two
        trees `ts` and `tg` according to a position `i`. `ts` contains
        the keys from 0 to `i - 1` inorder sense and `tg` the keys from
        `i` to n - 1. After completion the original tree `r` becames
        empty.

        @param[in,out] r pointer to the root of tree
        @param[in] i position for splitting
        @param[out] ts tree where the rank of keys \f$[0, i)\f$ will be put
        @param[out] tg tree where the rank of keys \f$[i, n]\f$ will be put

        @ingroup Arboles
    */
template <class Node> inline
void split_pos_rec(Node *& r, const size_t i, Node *& ts, Node *& tg)
{
  if (i > COUNT(r))
    throw std::out_of_range("infix position out of range");

  if (i == COUNT(r)) // ¿Es la última posición?
    {
      ts = r;
      r = tg = Node::NullPtr;
      return;
    }

  __split_pos_rec<Node>(r, i, ts, tg);

  r = Node::NullPtr;
}

    /** Insert a node in a specific inorder position in a binary tree.

        `insert_by_pos_xt(r, p, pos)` inserts in the position `pos` the
        node `p`.

        \warning According the key contained in `p`, the isnertion could
        violate the required order for a binary search tree

        @param[in,out] r root of tree
        @param[in] p node to insert
        @param[in] pos position to insert the node

        @ingroup Arboles
     */
    template <class Node>  inline
void insert_by_pos_xt(Node *& r, Node * p, size_t pos)
{
  assert(COUNT(Node::NullPtr) == 0);

  split_pos_rec(r, pos, LLINK(p), RLINK(p));
  COUNT(p) = COUNT(LLINK(p)) + 1 + COUNT(RLINK(p));
  r = p;
}

    /** Exclusive union of two extended binary search trees

        `join_exclusive_xt(ts, tg)` joins `ts` with `tg` in a
        tree. The trees must be exclusive in the sense that the all
        the keys of `ts` must be lesser than all the keys of `tg`.

        @param[in] ts extended binary search tree with keys lesser than `tg`
        @param[in] tg extended binary search tree with keys greater than `tg`

        @ingroup Arboles
        @see remove_by_key_xt()
     */
    template <class Node> inline
Node * join_exclusive_xt(Node *& ts, Node *& tg) noexcept
{
  if (ts == Node::NullPtr)
    return tg;

  if (tg == Node::NullPtr)
    return ts;

  LLINK(tg) = join_exclusive_xt(RLINK(ts), LLINK(tg));
  RLINK(ts) = tg;

      // actualizar contadores
  COUNT(tg) = COUNT(LLINK(tg)) + 1 + COUNT(RLINK(tg));
  COUNT(ts) = COUNT(LLINK(ts)) + 1 + COUNT(RLINK(ts));

  Node * ret_val = ts;
  ts = tg = Node::NullPtr; // deben quedar vacíos después del join

  return ret_val;
}


/** Remove a key of extended binary tree

    `remove_by_key_xt(root, key, cmp)` searches in the extended binary
    tree `root` the `key`. If `key` is found, then the node containing
    it is removed from the tree.

    @param[in,out] root of tree
    @param[in] key to search and eventually to remove
    @param[in] cmp comparison criteria
    @return if key is found, then a pointer to the removed node is
    returned. Otherwise the function returns `Node::NullPtr`

    @see join_exclusive_xt()
    @ingroup Arboles
*/
    template <class Node,
              class Compare = Aleph::less<typename Node::key_type>> inline
Node * remove_by_key_xt(Node *& root, const typename Node::key_type & key,
			Compare & cmp) noexcept
{
  assert(root != Node::NullPtr);

  if (root == Node::NullPtr)
    return Node::NullPtr;

  Node * ret_val = Node::NullPtr;
  if (cmp(key, KEY(root)))
    {
      ret_val = remove_by_key_xt(LLINK(root), key, cmp);
      if (ret_val != Node::NullPtr)
        --COUNT(root);

      return ret_val;
    }
  else if (cmp(KEY(root), key))
    {
      ret_val = remove_by_key_xt(RLINK(root), key, cmp);
      if (ret_val != Node::NullPtr)
        --COUNT(root);

      return ret_val;
    }

  ret_val = root;
  root = join_exclusive_xt(LLINK(root), RLINK(root));

  ret_val->reset();

  return ret_val;
}

/// \overload Node * remove_by_key_xt()
template <class Node, class Compare = Aleph::less<typename Node::key_type>>
inline Node * remove_by_key_xt(Node *& root,
                               const typename Node::key_type & key,
                               Compare && cmp = Compare()) noexcept
{
  return remove_by_key_xt(root, key, cmp);
}


    template <class Node> static inline
Node * __remove_by_pos_xt(Node *& root, size_t pos) noexcept
{
  if (COUNT(LLINK(root)) == pos) // found position?
    {
      Node * ret_val = root;
      root = join_exclusive_xt(LLINK(root), RLINK(root));

      ret_val->reset();

      return ret_val;
    }

  Node * ret_val;
  if (pos < COUNT(LLINK(root)))
    ret_val = __remove_by_pos_xt(LLINK(root), pos);
  else
    ret_val = __remove_by_pos_xt(RLINK(root), pos - (COUNT(LLINK(root)) + 1));

  if (ret_val != Node::NullPtr) // removal was done?
    --COUNT(root);

  return ret_val;
}

/** Remove from a extended binary tree the node whose inorder position
    is `pos`.

    @param[in,out] root of tree
    @param[in] pos iorder position of node to be removed
    @return pointer to the removed node
    \throw out_of_range if `pos` is greater than the number of nodes of tree

    @ingroup Arboles
*/
    template <class Node> inline
Node * remove_by_pos_xt(Node *& root, size_t pos)
{
  if (pos >= COUNT(root))
    throw std::out_of_range("infix position out of range");

  return __remove_by_pos_xt(root, pos);
}

/** Return `true` if `root` is a valid extended binary tree.

    @ingroup Arboles
*/
    template <class Node> inline
bool check_rank_tree(Node * root) noexcept
{
  if (root == Node::NullPtr)
    return true;

  if (COUNT(LLINK(root)) + COUNT(RLINK(root)) + 1 != COUNT(root))
    return false;

  return check_rank_tree(LLINK(root)) and check_rank_tree(RLINK(root));
}

    /**  Rotate to right the extended bianry tree with root `p`

	 @param[in] p root to rotate
	 @return pointer to the new root
         @ingroup Arboles
     */
    template <class Node> inline
Node * rotate_to_right_xt(Node* p) noexcept
{
  assert(p != Node::NullPtr);
  assert(COUNT(LLINK(p)) + 1 + COUNT(RLINK(p)) == COUNT(p));

  Node * q = LLINK(p);
  LLINK(p) = RLINK(q);
  RLINK(q) = p;
  COUNT(p) -= 1 + COUNT(LLINK(q));
  COUNT(q) += 1 + COUNT(RLINK(p));
  assert(COUNT(LLINK(q)) + 1 + COUNT(RLINK(q)) == COUNT(q));
  return q;
}

    /**  Rotate to left the extended bianry tree with root `p`.

         @param[in] p root to rotate.
         @return pointer to the new root.
         @ingroup Arboles
     */
    template <class Node> inline
Node * rotate_to_left_xt(Node* p) noexcept
{
  assert(p != Node::NullPtr);
  assert(COUNT(LLINK(p)) + 1 + COUNT(RLINK(p)) == COUNT(p));

  Node * q = RLINK(p);
  RLINK(p) = LLINK(q);
  LLINK(q) = p;
  COUNT(p) -= 1 + COUNT(RLINK(q));
  COUNT(q) += 1 + COUNT(LLINK(p));
  assert(COUNT(LLINK(q)) + 1 + COUNT(RLINK(q)) == COUNT(q));
  return q;
}


    /** Search or insert a key in an extended binary search tree

        @param[in] root of the tree
        @param[in] p pointer to the node whose key must be searched and
        eventually, if not found, be inserted
        @param[in] cmp comparison criteria
        @return if the node was inserted (its key was not in the tree),
        then the inserted node is returned. Otherwise, a pointer to a
        node containg the key in returned
     */
    template <class Node, class Key, class Compare> inline
Node * search_or_insert_root_rec_xt(Node * root, Node * p, Compare & cmp)
      noexcept
{
  if (root == Node::NullPtr)
    return p; // insertion in empty tree

  if (cmp(KEY(p), KEY(root)))
    { // insert in left subtree
      Node * left_branch = search_or_insert_root_rec_xt(LLINK(root), p, cmp);
      if (left_branch == p) // p was inserted?
        {
          ++COUNT(root);
          LLINK(root) = left_branch;
          root        = rotate_to_right_xt(root);
          return p;
        }

        return left_branch;
    }
  else if (cmp(KEY(root), KEY(p)))
    { // insert in right subtree
      Node * right_branch = search_or_insert_root_rec_xt(RLINK(root), p, cmp);
      if (right_branch == p) // p was inserted?
        {
          ++COUNT(root);
          RLINK(root) = right_branch;
          root        = rotate_to_left_xt(root);
          return p;
        }

      return right_branch;
    }

  return root;
}

/// \overload search_or_insert_root_rec_xt()
template <class Node, class Key,
              class Compare = Aleph::less<typename Node::key_type>> inline
Node * search_or_insert_root_rec_xt(Node * root, Node * p,
                                    Compare && cmp = Compare()) noexcept
{
  return search_or_insert_root_rec_xt(root, p, cmp);
}


} // end namespace Aleph

# endif // TPL_BINNODEXT_H
