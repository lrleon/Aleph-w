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
/*
 *			    Top Dowm Splay Trees
 */

/* This code is a c++ adaptation of Danny Sleator code. It can be loaded from

   http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java

 */

# ifndef TPL_SPLAY_TREE_RK_H
# define TPL_SPLAY_TREE_RK_H

# include <tpl_binNodeXt.H>
# include <ah-errors.H>

using namespace Aleph;


template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTreeRk
{
public:
  typedef NodeType<Key> Node;

private:
  Node *root;
  Compare cmp;

public:
  /// The key type stored in the node
  typedef Key key_type;

  /// Returns a reference to the comparison functor
  Compare &key_comp() { return cmp; }

  /// \overload key_comp()
  Compare &get_compare() { return key_comp(); }

  /** search key within tree and splay that node.

      @warning The tree must not be empty. All public methods check for
      empty tree before calling splay().
   */
  void splay(const Key & key) noexcept
  {
    assert(root != Node::NullPtr); // splay requires non-empty tree
    Node header(sentinelCtor);
    Node * head_ptr = &header;
    Node * l = head_ptr;
    Node * r = head_ptr;
    Node * t = root;

    size_t l_size = 0;
    size_t r_size = 0;

    while (true)
      if (cmp(key, KEY(t)))
        {
          if (LLINK(t) == Node::NullPtr)
            break;

          if (cmp(key, KEY(LLINK(t))))
            {
              t = rotate_to_right_xt(t);
              if (LLINK(t) == Node::NullPtr)
                break;
            }

          LLINK(r) = t;
          r = t;
          t = LLINK(t);
          r_size += 1 + COUNT(RLINK(r));
        }
      else if (cmp(KEY(t), key))
        {
          if (RLINK(t) == Node::NullPtr)
            break;

          if (cmp(KEY(RLINK(t)), key))
            {
              t = rotate_to_left_xt(t);
              if (RLINK(t) == Node::NullPtr)
                break;
            }

          RLINK(l) = t;
          l = t;
          t = RLINK(t);
          l_size += 1 + COUNT(LLINK(l));
        }
      else
        break;

    l_size += COUNT(LLINK(t));
    r_size += COUNT(RLINK(t));
    COUNT(t) = l_size + r_size + 1;

    /* reassembling of sub trees in one with t as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);

    LLINK(t) = RLINK(head_ptr);
    RLINK(t) = LLINK(head_ptr);

    for (Node * y = LLINK(t); y != Node::NullPtr; y = RLINK(y))
      {
        COUNT(y) = l_size;
        l_size -= 1 + COUNT(LLINK(y));
      }

    for (Node * y = RLINK(t); y != Node::NullPtr; y = LLINK(y))
      {
        COUNT(y) = r_size;
        r_size -= 1 + COUNT(RLINK(y));
      }

    root = t;
  }

  /// Constructor
  GenTdSplayTreeRk(Compare __cmp = Compare())
    : root(Node::NullPtr), cmp(__cmp)
  {
    // Empty
  }

  void swap(GenTdSplayTreeRk & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// Destructor
  virtual ~GenTdSplayTreeRk()
  { /* empty */
  }

private:
  Node * __insert(Node *p)
  {
    COUNT(p) = COUNT(root) + 1;
    if (cmp(KEY(p), KEY(root)))
      {
        COUNT(root) -= COUNT(LLINK(root));
        LLINK(p) = LLINK(root);
        RLINK(p) = root;
        LLINK(root) = Node::NullPtr;
      }
    else
      {
        COUNT(root) -= COUNT(RLINK(root));
        RLINK(p) = RLINK(root);
        LLINK(p) = root;
        RLINK(root) = Node::NullPtr;
      }

    return root = p; // inserted node become root
  }

public:
  /** Inserts a node in a top down splay tree.

      @return a pointer to the inserted node if node is not found in tree;
      nullptr otherwise.
      @param p a pointer to the node to be inserted
  */
  Node * insert(Node *p)
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);

    splay(key);

    if (are_equals<Key, Compare>(KEY(root), key, cmp))
      return nullptr; // item is already in tree

    return __insert(p);
  }

  Node * insert_dup(Node *p)
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    splay(KEY(p));

    return __insert(p);
  }

  /**
     Searches a key in a top down splay tree.

     @return a pointer to the node containing the key if the key is found;
     nullptr otherwise.
     @param key key to search
  */
  Node * search(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    splay(key);

    return are_equals<Key, Compare>(KEY(root), key, cmp) ? root : nullptr;
  }

  Node * search_or_insert(Node *p)
  {
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);
    splay(key);
    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      return root;

    return __insert(p);
  }

  /**
     Remove a key from a top-down splay tree.

     Searches a key in a top-down splay tree and remove the containing
     the key if this is found.

     @return a pointer to node containing the removed key.
     @param key to search
  */
  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    splay(key);

    if (no_equals<Key, Compare>(KEY(root), key, cmp))
      return nullptr; /* key not found */

    Node *ret_val = root; /* store node to delete */

    if (LLINK(root) == Node::NullPtr)
      root = RLINK(root);
    else
      {
        Node *p = RLINK(root);
        root = LLINK(root);
        splay(key);
        RLINK(root) = p;
        COUNT(root) = COUNT(LLINK(root)) + 1 + COUNT(RLINK(root));
      }

    ret_val->reset();

    return ret_val;
  }

  /// Get the top-down splay tree's root
  Node *&getRoot()
  {
    return root;
  }

  bool verify() const { return check_rank_tree(root) and check_bst(root, cmp); }

  /// Returns the number of nodes stored in the tree.
  size_t size() const
  {
    return COUNT(root);
  }

  /// Returns true if the tree is empty.
  bool is_empty() const
  {
    return root == Node::NullPtr;
  }

  /** Returns the inorder (sorted) position of key.

      `position(key)` searches in the extended splay tree for key (which
      takes \f$O(\lg n)\f$ time) and returns the inorder position of the
      node that contains the key.

      @param[in] key key to search and whose inorder position is requested.
      @return a pair containing the inorder position of key within the
      ordered set together with the node if the key is found in the tree;
      otherwise it returns -1 and the node is unspecified.
   */
  std::pair<long, Node *> position(const Key & key)
  {
    if (root == Node::NullPtr)
      return {-1, nullptr};

    splay(key);

    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      return {static_cast<long>(COUNT(LLINK(root))), root};

    return {-1, nullptr};
  }

  /** Returns the inorder (sorted) position of key.

      `find_position(key)` searches in the extended splay tree for key
      (which takes \f$O(\lg n)\f$ time) and returns the inorder position
      of the node that contains the key together with the node. If the
      key is not in the tree, `find_position(key)` returns the position
      where it would be if key belonged to the tree.

      The return value is a `pair<long, Node*>`. The first component is
      the inorder position and the second is the node.

      If key is not found in the tree, three cases can be distinguished:
      -# If key is smaller than the minimum key in the tree, then `first`
         is -1 and `second` is the node containing the minimum key.
      -# If key is greater than the maximum key in the tree, then `first`
         is COUNT(root) (number of nodes in the tree) and `second` is the
         node containing the maximum key.
      -# In any other case, `first` is the position that key would have in
         the tree and `second` (node `p`) is a key immediately adjacent to
         key. Note that `second` can hold a smaller or greater key, but it
         is guaranteed to be immediately adjacent to key.

      @param[in] key key to search and whose inorder position is requested.
      @return a pair containing the inorder position of key within the
      ordered set together with the node if the key is found in the tree;
      otherwise it returns -1 and the node is unspecified.
   */
  std::pair<long, Node *> find_position(const Key & key)
  {
    if (root == Node::NullPtr)
      return {-1, nullptr};

    splay(key);

    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      return {static_cast<long>(COUNT(LLINK(root))), root};

    // key not found - determine position it would have
    if (cmp(key, KEY(root)))
      return {static_cast<long>(COUNT(LLINK(root))) - 1, root}; // less than root
    return {static_cast<long>(COUNT(LLINK(root))) + 1, root};
    // greater than root
  }

  /** Returns the node whose inorder position in the extended tree is i.

      `select(i)` returns the node in the extended splay tree whose
      inorder position is i.

      @param[in] i inorder position to select.
      @return pointer to the node corresponding to inorder position i.
      @throw out_of_range if i is greater than or equal to the number of
      nodes in the tree.
  */
  Node * select(const size_t & i)
  {
    return Aleph::select(root, i);
  }

  /** Inorder iterator over the extended splay tree.

      Allows traversing the tree elements in order.
   */
  class Iterator
  {
    GenTdSplayTreeRk *tree_ptr = nullptr;
    Node *curr = Node::NullPtr;
    size_t pos = 0;

  public:
    Iterator() = default;

    Iterator(GenTdSplayTreeRk & tree) noexcept
      : tree_ptr(&tree), curr(Node::NullPtr), pos(0)
    {
      if (tree_ptr->root != Node::NullPtr)
        curr = find_min(tree_ptr->root);
    }

    bool has_curr() const noexcept { return curr != Node::NullPtr; }

    Node * get_curr() const
    {
      ah_overflow_error_if(curr == Node::NullPtr)
          << "Iterator is at end";
      return curr;
    }

    Node * get_curr_ne() const noexcept { return curr; }

    void next()
    {
      ah_overflow_error_if(curr == Node::NullPtr)
          << "Iterator is at end";
      curr = inorder_successor(tree_ptr->root, KEY(curr));
      ++pos;
    }

    void next_ne() noexcept
    {
      curr = inorder_successor(tree_ptr->root, KEY(curr));
      ++pos;
    }

    size_t get_pos() const noexcept { return pos; }

    void reset_first() noexcept
    {
      pos = 0;
      if (tree_ptr != nullptr && tree_ptr->root != Node::NullPtr)
        curr = find_min(tree_ptr->root);
      else
        curr = Node::NullPtr;
    }

  private:
    static Node * find_min(Node *r) noexcept
    {
      while (LLINK(r) != Node::NullPtr)
        r = LLINK(r);
      return r;
    }

    static Node * inorder_successor(Node *root, const Key & key) noexcept
    {
      Node *successor = Node::NullPtr;
      Node *curr = root;

      while (curr != Node::NullPtr)
        {
          if (key < KEY(curr))
            {
              successor = curr;
              curr = LLINK(curr);
            }
          else
            curr = RLINK(curr);
        }

      return successor;
    }
  };
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk : public GenTdSplayTreeRk<BinNodeXt, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXt, Key, Compare>;
  using Base::Base;
};


template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk_Vtl : public GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>;
  using Base::Base;
};


#endif /* TPL_SPLAY_TREE_RK_H */
