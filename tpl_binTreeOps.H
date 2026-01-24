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


/** @file tpl_binTreeOps.H
 *  @brief Binary tree operations (split, join, rotate).
 *
 *  Common operations for binary search trees including
 *  rotations, splits, and joins.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BINTREEOPS_H
# define TPL_BINTREEOPS_H

# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>

namespace Aleph
{
  /** Functor encompassing basic operation for binary search trees.

      \see BinNode
      \ingroup Trees
  */
  template <class Node, class Cmp = Aleph::less<typename Node::key_type>>
  class BinTree_Operation
  {
  protected:
    Cmp cmp;

  public:
    ///  Return the comparison criteria
    Cmp &key_comp() noexcept { return cmp; }

    /// \overload key_comp()
    Cmp &get_compare() noexcept { return cmp; }

    using Key = typename Node::key_type; /// The type of key

    using key_type = typename Node::key_type; /// The type of key

    /// Initialize the functor with comparison criteria `__cmp`
    BinTree_Operation(Cmp __cmp = Cmp()) noexcept : cmp(__cmp)
    { /* empty */
    }

    /** Search a node with a specific key.

        @param[in] root of the tree
        @param[in] key to be searched
        @return a pointer to a node containing the key if this was found;
        `Node::NullPtr` otherwise
    */
    Node * search(Node *root, const Key & key) noexcept
    {
      return searchInBinTree(root, key, cmp);
    }

    /** Search a key and find its node and parent.

        @param[in] root of tree
        @param[in] key to search
        @param[out] parent pointer to parent node if key was
        found. Otherwise, value is undetermined
        @return a valid pointer to a node containing `key` if this is
        found; otherwise, it returns a pointer to the last visited node
    */
    Node * search_parent(Node *root, const Key & key, Node *& parent) noexcept
    {
      return Aleph::search_parent<Node, Cmp>(root, key, parent, cmp);
    }

    /** Rank search of a key in a binary search tree.

        In a binary search tree the rank search of a key consists in
        determining the node that would be parent of key.

        `search_rank_parent(root, key)` searches a node containing
        key. If `key` is found, then its node is returned. Otherwise,
        the last visited node, that would be the parent of `key` if this
        was inserted in the tree, is returned.

        @param[in] root of general tree
        @param[in] key to search
        @return pointer to a node containing `key` if this node exists
        or the last visited node otherwise
        @note It is not verified if the tree is empty
        @ingroup Trees
    */
    Node * search_rank_parent(Node *root, const Key & key) noexcept
    {
      return Aleph::search_rank_parent<Node, Cmp>(root, key, cmp);
    }

    /** Insert a node `p` in a binary search tree

        `insert(root, p)` inserts the node `p` in the binary search tree
        with `root`

        @param[in,out] root of tree
        @param[in] p pointer to the node to be inserted
        @return `p` if this was inserted; that is if `p->get_key()` is
        not in the tree; otherwise, `Node::NullPtr` is returned
    */
    Node * insert(Node *& root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return root = p;

      if (cmp(KEY(p), KEY(root))) // p < root
        {
          if (LLINK(root) != Node::NullPtr) [[likely]]
            __builtin_prefetch(LLINK(root));
          return insert(LLINK(root), p);
        }
      if (cmp(KEY(root), KEY(p))) // p > root
        {
          if (RLINK(root) != Node::NullPtr) [[likely]]
            __builtin_prefetch(RLINK(root));
          return insert(RLINK(root), p);
        }

      return Node::NullPtr; // Repeated key ==> no insertion
    }

    /** Insert a node `p` in a binary search tree

        `insert_dup(root, p)` inserts the node `p` in the binary
        search tree with `root`. The key contained in `p` can be already
        present in the tree.

        @param[in,out] root of tree
        @param[in] p pointer to the node to be inserted
        @return the pointer `p`
    */
    Node * insert_dup(Node *& root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return root = p;

      if (cmp(KEY(p), KEY(root))) // p < root
        {
          if (LLINK(root) != Node::NullPtr) [[likely]]
            __builtin_prefetch(LLINK(root));
          return insert_dup(LLINK(root), p);
        }

      if (RLINK(root) != Node::NullPtr) [[likely]]
        __builtin_prefetch(RLINK(root));
      return insert_dup(RLINK(root), p);
    }

    /** Search or insert a node in a binary search tree.

        `search_or_insert(root, p)` searches in `root` a node
        containing `p->get_key()`. If found, then this node is
        returned. Otherwise, `p` is inserted and returned.

        @param[in,out] r roor of tree
        @param[in] p node to search or insert
        @return `p` if its key was not in the tree; otherwise, a pointer
        containing the tree is returned.
    */
    Node * search_or_insert(Node *& r, Node *p) noexcept
    {
      if (r == Node::NullPtr)
        return r = p;

      if (cmp(KEY(p), KEY(r))) // p < root
        {
          if (LLINK(r) != Node::NullPtr) [[likely]]
            __builtin_prefetch(LLINK(r));
          return search_or_insert(LLINK(r), p);
        }
      if (cmp(KEY(r), KEY(p))) // p > root
        {
          if (RLINK(r) != Node::NullPtr) [[likely]]
            __builtin_prefetch(RLINK(r));
          return search_or_insert(RLINK(r), p);
        }

      return r; // key found
    }

  private:
    bool __split_key_rec(Node *root, const Key & key, Node *& ts, Node *& tg)
      noexcept
    {
      if (root == Node::NullPtr)
        {
          ts = tg = Node::NullPtr;
          return true;
        }

      if (cmp(key, KEY(root))) // ¿key < KEY(root)?
        {
          if (__split_key_rec(LLINK(root), key, ts, LLINK(root)))
            {
              tg = root;
              return true;
            }
          return false;
        }

      if (cmp(KEY(root), key)) // ¿key > KEY(root)?
        if (__split_key_rec(RLINK(root), key, RLINK(root), tg))
          {
            ts = root;
            return true;
          }

      return false;
    }

  public:
    /** Split recursively according to a key.

        `split_key_rec(root, key, ts, tg)` splits the tree with `root` in
        two trees `t1` which contain the keys lesser than `key` and `t2`
        which contains the keys greater than `key`.

        The split only is performed if `key` is not in the tree.

        @param[in,out] root of tree
        @param[in] key for slitting
        @param[out] ts tree with keys lesser than `key`
        @param[out] tg tree with keys greater than `key`
        @return `true` if the tree wa split; that if `key` was not in the
        tree. Otherwise, the split is not performed and it return `false`
    */
    bool split_key_rec(Node *& root, const Key & key, Node *& ts, Node *& tg)
      noexcept
    {
      const bool ret = __split_key_rec(root, key, ts, tg);
      if (ret)
        root = Node::NullPtr;
      return ret;
    }

  private:
    void __split_key_dup_rec(Node *root, const typename Node::key_type & key,
                             Node *& ts, Node *& tg) noexcept
    {
      if (root == Node::NullPtr)
        { // Key is not in tree ==> split will succeed
          ts = tg = Node::NullPtr;
          return;
        }

      if (cmp(key, KEY(root))) // ¿key < KEY(root)?
        __split_key_dup_rec(LLINK(root), key, ts, LLINK(root));
      else if (cmp(KEY(root), key)) // ¿key > KEY(root)?
        __split_key_dup_rec(RLINK(root), key, RLINK(root), tg);
      else // key == KEY(root)
        __split_key_dup_rec(LLINK(root), key, ts, LLINK(root));
    }

  public:
    /** Split a tree according to a key value

        `split_key_dup_rec(root, key, ts, tg)` splits according to `key
        the tree with `root` and build two trees. `t1`contains the keys
        lesser than `key` and `t2` the keys greater or equal than `key`.

        @param[in,out] root of tre to be split
        @param[in] key for splitting
        @param[out] ts tree with the keys lesser than `key`
        @param[out] tg tree with the keys greater or equal than `key`
    */
    void split_key_dup_rec(Node *& root, const typename Node::key_type & key,
                           Node *& ts, Node *& tg) noexcept
    {
      __split_key_dup_rec(root, key, ts, tg);
      root = Node::NullPtr;
    }

    /** Remove a key from a binary search tree

        @param[in,out] root of tree
        @param[in] key to remove
        @return a valid pointer to the removed node if `key` was found
        in the tree, `Node::NullPtr` otherwise
    */
    Node * remove(Node *& root, const Key & key) noexcept
    {
      if (root == Node::NullPtr)
        return Node::NullPtr;

      if (cmp(key, KEY(root))) // ¿key < KEY(root)?
        return remove(LLINK(root), key);
      if (cmp(KEY(root), key)) // ¿key > KEY(root)?
        return remove(RLINK(root), key);

      Node *ret_val = root;
      root = join_exclusive(LLINK(root), RLINK(root));

      ret_val->reset();

      return ret_val;
    }

    /** Insert the node `p` as root of a binary search tree

        `insert_root(root, p)` inserts in the tree `root` the node
        `p`. After insertion, `p` becomes the new root of tree.

        @param[in,out] root of binary search tree
        @param[in] p pointer to node to insert
        @return a pointer to `p` if this was inserted; that is, if
        `p->get_key()` was not present in the tree. Otherwise, no insertion
        is done and `Node::NullPtr` is returned
    */
    Node * insert_root(Node *& root, Node *p) noexcept
    {
      Node *l = Node::NullPtr, *r = Node::NullPtr;

      if (not split_key_rec(root, KEY(p), l, r))
        return Node::NullPtr;

      LLINK(p) = l;
      RLINK(p) = r;
      root = p;

      return root;
    }

    /** Insert node `p` as root of a binary search tree. The key of `p`
        can be duplicated.

        @param[in,out] root of tree
        @param[in] p node to insert as root
        @return the pointer `p` which has became the root of tree
    */
    Node * insert_dup_root(Node *& root, Node *p) noexcept
    {
      split_key_dup_rec(root, KEY(p), LLINK(p), RLINK(p));

      return root = p;
    }

    /** Union of two binary search trees

        \warning This union is \f$O(n \lg m)\f$ where n and m are the sizes
        of `t1` and `t2`respectively. **Use `join()` which is much more faster**

        @param[in] t1 root of first tree
        @param[in] t2 root of second tree
        @param[out] dup root of tree where the duplicated keys will be put
        @return a pointer to the resulting root of tree
    */
    Node * join_preorder(Node *t1, Node *t2, Node *& dup) noexcept
    {
      if (t2 == Node::NullPtr)
        return t1;

      Node *l = LLINK(t2);
      Node *r = RLINK(t2);

      if (insert(t1, t2) == Node::NullPtr)
        insert(dup, t2);

      join_preorder(t1, l, dup);
      join_preorder(t1, r, dup);

      return t1;
    }

    /** Fast union of two binary search trees

        `join(t1, t2, dup)` joins the nodes of `t1` with the nodes
        of `t2`. The duplicated keys of `t2` are copied in the binary
        search tree `dup`.

        @param[in] t1 root of first tree
        @param[in] t2 root of second tree
        @param[out] dup tree where the duplicated keys of t2 are put
        @return pointer to the root of resulting join
    */
    Node * join(Node *t1, Node *t2, Node *& dup) noexcept
    {
      if (t1 == Node::NullPtr)
        return t2;

      if (t2 == Node::NullPtr)
        return t1;

      Node *l = LLINK(t1);
      Node *r = RLINK(t1);

      t1->reset();

      while (insert_root(t2, t1) == Node::NullPtr)
        {
          Node *p = remove(t1, KEY(t1));

          assert(p != Node::NullPtr);

          insert(dup, t1);
        }

      LLINK(t2) = join(l, LLINK(t2), dup);
      RLINK(t2) = join(r, RLINK(t2), dup);

      return t2;
    }

    /** Split a binary search tree according to a key.

        `split_key(root, key, l, r)` splits the tree `root`
        according to `key`. At the end, `l` contains all the keays lesser
        than `key` and `r ` all the keys greater or equal than `key`.

        @param[in,out] root of tree to split
        @param[in] key for splitting
        @param[out] l tree with keys lesser than `key`
        @param[out] r tree with keys greater or equal than `key`
    */
    void split_key(Node *root, const Key & key, Node *& l, Node *& r) noexcept
    {
      if (root == Node::NullPtr)
        {
          l = r = Node::NullPtr;
          return;
        }

      Node **current_parent = nullptr;
      Node **pending_child = nullptr;
      char current_is_right;
      if (cmp(key, KEY(root)))
        {
          r = root;
          pending_child = &l;
          current_is_right = true;
        }
      else
        {
          l = root;
          pending_child = &r;
          current_is_right = false;
        }

      Node *current = root;
      while (current != Node::NullPtr)
        {
          if (cmp(key, KEY(current)))
            { /* current must be in right side */
              if (not current_is_right)
                {
                  current_is_right = not current_is_right;
                  *pending_child = *current_parent; /* change of side */
                  pending_child = current_parent;
                }

              current_parent = &LLINK(current);
            }
          else
            { /* current must be in left side */
              if (current_is_right)
                {
                  current_is_right = not current_is_right;
                  *pending_child = *current_parent; /* change of side */
                  pending_child = current_parent;
                }
              current_parent = &RLINK(current);
            }

          current = *current_parent;
        }

      *pending_child = Node::NullPtr;
      root = Node::NullPtr;
    }

    /** Insert a node as root in a binary search tree.

        This version first inserts `p` as a leaf of a tree. Then `p` is rotated
        until the root.

        @param[in] root of tree
        @param[in] p pointer to the node to insert
        @return pointer to `p` if `p->get_key()` is not in the
        tree. Otherwise the function returns `Node::NullPtr`
    */
    Node * insert_root_rec(Node *root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p; /* insertion in an empty tree */

      if (cmp(KEY(p), KEY(root)))
        { /* insert in the left subtree */
          Node *left_branch = insert_root_rec(LLINK(root), p);
          if (left_branch == Node::NullPtr)
            return Node::NullPtr;

          LLINK(root) = left_branch;
          root = rotate_to_right(root);
        }
      else if (cmp(KEY(root), KEY(p)))
        { /* insert in the right subtree */
          Node *right_branch = insert_root_rec(RLINK(root), p);
          if (right_branch == Node::NullPtr)
            return Node::NullPtr;

          RLINK(root) = right_branch;
          root = rotate_to_left(root);
        }
      else
        return Node::NullPtr; /* duplicated key */

      return root;
    }

    /** Search and eventually insert `p` as root in a binary search tree

        @param[in] root of tree
        @param[in] p pointer to the node to eventually insert
        @return if `p` is inserted, then it returns `p`; otherwise, it
        returns a pointer to the tree node containing to `p->get_key()`
    */
    Node * search_or_insert_root_rec(Node *root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p; // insertion in an empty tree

      if (cmp(KEY(p), KEY(root)))
        { // insert in the left subtree
          Node *left_branch = search_or_insert_root_rec(LLINK(root), p);
          if (left_branch == p) // Was there insertion?
            {
              LLINK(root) = left_branch;
              root = rotate_to_right(root);

              return p;
            }

          return left_branch; // no la hubo
        }
      if (cmp(KEY(root), KEY(p)))
        { // insert in the right subtree
          Node *right_branch = search_or_insert_root_rec(RLINK(root), p);
          if (right_branch == p) //Was there insertion?
            {
              RLINK(root) = right_branch;
              root = rotate_to_left(root);

              return p;
            }

          return right_branch; // no la hubo
        }

      return root;
    }
  };


  /** Functor encompassing basic operation for extended binary search trees.

      \see BinNode
      \ingroup Trees
  */
  template <class Node,
            class Cmp = Aleph::less<typename Node::key_type>>
  class BinTreeXt_Operation : public BinTree_Operation<Node, Cmp>
  {
  public:
    using Key = typename Node::key_type;

    /// Initialize the functor with comparison criteria `cmp`
    BinTreeXt_Operation(Cmp cmp = Cmp()) noexcept
      : BinTree_Operation<Node, Cmp>(cmp)
    {
      // empty
    }

    /** Compute the inorder position of a key

        @param[in] r root of tree
        @param[in] key to be searched
        @param[out] p pointer to the node containing `key`
        @return inorder position of `key` if this is in the tree or -1 if
        `key` is not found

        @ingroup Trees
    */
    long inorder_position(Node *r, const Key & key, Node *& p) noexcept
    {
      assert(COUNT(Node::NullPtr) == 0);

      if (r == Node::NullPtr)
        return -1;

      if (this->cmp(key, KEY(r)))
        {
          if (LLINK(r) != Node::NullPtr) [[likely]]
            __builtin_prefetch(LLINK(r));
          return inorder_position(LLINK(r), key, p);
        }
      if (this->cmp(KEY(r), key))
        {
          if (RLINK(r) != Node::NullPtr) [[likely]]
            __builtin_prefetch(RLINK(r));
          long ret = inorder_position(RLINK(r), key, p);
          if (ret != -1)
            return ret + COUNT(LLINK(r)) + 1;
          return ret;
        }
      p = r;
      return COUNT(LLINK(r));
    }

    /** Find the inorder position of a key in an extended binary search
        tree.

        `find_position(r, key, p)` searches `key` in the tree with root
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
        whose key is immediately greater than `key`.

        @param[in] r root of the tree
        @param[in] key to be searched
        @param[out] p reference to pointer to node
        @return the positions of `key` in the tree
    */
    long find_position(Node *r, const Key & key, Node *& p) noexcept
    {
      assert(COUNT(Node::NullPtr) == 0);

      Node *parent = nullptr;
      long pos = COUNT(LLINK(r));

      while (r != Node::NullPtr)
        if (const auto & kr = KEY(r); this->cmp(key, kr)) // key < KEY(r): go left
          {
            Node *next = LLINK(r);
            __builtin_prefetch(next, 0, 3);

            parent = r;
            r = next;

            pos -= (COUNT(RLINK(r)) + 1);
          }
        else if (this->cmp(kr, key)) // KEY(r) < key: go right
          {
            Node *next = RLINK(r);
            __builtin_prefetch(next, 0, 3);

            parent = r;
            r = next;

            pos += (COUNT(LLINK(r)) + 1);
          }
        else
          {
            p = r;
            return pos;
          }

      p = parent;
      return pos;
    }


    /** Split an extended binary search tree according to a key

        `split_key_rec(root, key, l, r)` splits a tree according a
        non-existing key

        @param[in,out] root pointer to tree root
        @param[in] key for splitting
        @param[out] l tree with keys lesser than `key`
        @param[out] r tree with keys greater than `key`
        @return `true` if tree was split; that is if `key` is not in the
        tree. Otherwise, if `key` is in the tree, `false` is returned
    */
    bool split_key_rec(Node *root, const Key & key, Node *& l, Node *& r)
      noexcept
    {
      if (root == Node::NullPtr)
        {
          l = r = Node::NullPtr;
          return true;
        }

      if (this->cmp(key, KEY(root)))
        {
          if (not split_key_rec(LLINK(root), key, l, LLINK(root)))
            return false;

          r = root;
          COUNT(r) -= COUNT(l);
        }
      else if (this->cmp(KEY(root), key))
        {
          if (not split_key_rec(RLINK(root), key, RLINK(root), r))
            return false;

          l = root;
          COUNT(l) -= COUNT(r);
        }
      else
        return false; // Duplicate Key

      return true;
    }

    /** Split an extended binary search tree according to a key which can be
        in the tree.

        `split_key__dup_rec(root, key, l, r)` splits a tree according a
        key. The key can be in the tree.

        @param[in,out] root pointer to tree root
        @param[in] key for splitting
        @param[out] l tree with keys lesser than `key`
        @param[out] r tree with keys greater than `key`
    */
    void split_key_dup_rec(Node *root, const Key & key, Node *& l, Node *& r)
      noexcept
    {
      if (root == Node::NullPtr)
        {
          l = r = Node::NullPtr;
          return;
        }

      if (this->cmp(key, KEY(root)))
        {
          split_key_dup_rec(LLINK(root), key, l, LLINK(root));
          r = root;
          COUNT(r) -= COUNT(l);
        }
      else
        {
          split_key_dup_rec(RLINK(root), key, RLINK(root), r);
          l = root;
          COUNT(l) -= COUNT(r);
        }
    }

    /** Insert a node `p` as root of an extended binary search tree.

        `insert_root_xt(root, p)` inserts as root in the extended binary search
        tree `root` the node `p`. After insertion, if there is no
        duplicated key, `p` becomes the root of the tree.

        @param[in,out] root of tree
        @param[in] p pointer to the node to insert
        @return if the key contained in `p` is not in tree, then returns
        `p`, since this was inserted and has became the root. Otherwise,
        it returns `Node::NullPtr`

        @ingroup Trees
    */
    Node * insert_root(Node *& root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p;

      if (not split_key_rec(root, KEY(p), LLINK(p), RLINK(p)))
        return Node::NullPtr;

      COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
      root = p;

      return p;
    }

    /** Insert a node as root of an extended binary search tree.

        `insert_dup_root(root, p)` inserts the node `p` as the
        new root of the tree `root`.

        This insertion allows duplicates.

        @param[in,out] root of tree
        @param[in] p pointer to the to insert
        @return pointer to the inserted node `p` that has become root
    */
    Node * insert_dup_root(Node *& root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p;

      split_key_dup_rec(root, KEY(p), LLINK(p), RLINK(p));

      COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;

      return root = p;
    }
  };
} // end namespace Aleph

# endif // TPL_BINTREEOPS_H
