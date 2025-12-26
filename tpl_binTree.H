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

# ifndef TPL_BINTREE_H
# define TPL_BINTREE_H

# include <utility>

# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph {

    /** Simple binary search tree.

	This class implements a simple binary search tree. By simple we
	want to say that no balance operation are done. Consequently the
	performance depends on whether keys insertion order.

	In general, if the insertion order is random and there is no
	many removals, this tree is very good and its operations
	trend to be \f$O(\lg n)\f$. If there are many removals, the
	performance is lightly degraded trending to be \f$O(\sqrt(n)\f$.

	If you cannot assure that the insertion order is random, **then
	do not use this tree**.

        @see BinTree BinTreeVtl DynBinTree

        @ingroup Arboles
     */
  template <template <typename> class NodeType, typename Key, class Compare>
class GenBinTree 
{
public:

  using Node = NodeType<Key>; /// The node

private:

  Node      headNode;
  Node  *   head;
  Node *&   root;
  Compare   cmp;

  template <class Inserter>
  static void move_all(Node *& src, Inserter inserter) noexcept
  {
    auto rec = [&] (auto && self, Node * n) noexcept -> void
    {
      if (n == Node::NullPtr)
        return;

      Node * l = LLINK(n);
      Node * r = RLINK(n);
      n->reset();
      inserter(n);
      self(self, l);
      self(self, r);
    };

    Node * n = src;
    src = Node::NullPtr;
    rec(rec, n);
  }

public:

  GenBinTree(const GenBinTree & ) = delete;

  GenBinTree & operator = (const GenBinTree &) = delete;

  /// Swap `this` with `tree` in constant time
  void swap(GenBinTree & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// return the comparison criteria
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  /// Initialize an empty tree with comparison criteria `__cmp`
  GenBinTree(Compare _cmp = Compare()) noexcept
    : head(&headNode), root(headNode.getR()), cmp(_cmp)
  {
    // empty
  }

    /** Search a key.

        @param[in] key to be searched
        @return a pointer to the containing key if this was found;
        otherwise `nullptr`
    */
  Node * search(const Key & key) const noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).search(root, key);
  }

  virtual ~GenBinTree() = default;

  /// Return `true` if the tree is a consistent (correct) binary search tree
  bool verify() const { return check_bst<Node, Compare>(root, cmp); }

    /// Return the root of tree
  Node *& getRoot() noexcept { return root; }

    /// Return the root of tree
  Node * getRoot() const noexcept { return root; }

  /// \overload verify()
  bool verifyBin() const { return check_bst<Node, Compare>(root, cmp); } 

      /** Insert a node in the tree

          @param[in] p pointer to the node to insert
          @return if `p->get_key()` is not in the tree, then the pointer
          `p` is returned (it was inserted); otherwise, `nullptr`
          is returned 
       */
  Node * insert(Node *p) noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).insert(root, p);
  }

      /** Insert a node in the tree.

	  This method does not fail. It always inserts.

          @param[in] p pointer to the node to insert
          @return the `p` pointer
       */
  Node * insert_dup(Node *p) noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).insert_dup(root, p);
  }

      /** Search or insert a key.

          `search_or_insert(p)` searches in the tree the key
          `KEY(p)`. If this key is found, then a pointer to the node
          containing it is returned. Otherwise, `p` is inserted.

          @param[in] p node containing a key to be searched and
          eventually inserted
          @return if the key contained in `p` is found, then a pointer
          to the containing key in the tree is returned. Otherwise, `p`
          is inserted and returned
       */
  Node * search_or_insert(Node *p) noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).search_or_insert(root, p);
  }

  /** Split the tree according to a key

      `split(key, l, r)` splits the tree according to `key`. That is, if
      key is not present in the tree, then the tree is split in two
      trees `l` which contains the key lesser than `key` and `r` which
      contains the keys greater than `key`. If `key` is found in the
      tree, then the split is not done

      @param[in] key for splitting
      @param[out] l resulting tree with the keys lesser than `key`
      @param[out] r resulting tree with the keys greater than `key`
      @return true if the tree was split. `false` otherwise
  */
  bool split(const Key & key, GenBinTree & l, GenBinTree & r) noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).
      split_key_rec(root, key, l.root, r.root);
  }

  /** Split the tree according to a key that could be in the tree

      `split_dup(key, l, r)` splits the tree according to `key` in two
      trees `l` which contains the key lesser than `key` and `r` which
      contains the keys greater or equal than `key`. 

      @param[in] key for splitting
      @param[out] l resulting tree with the keys lesser than `key`
      @param[out] r resulting tree with the keys greater or equal than `key`
  */
  void split_dup(const Key & key, GenBinTree & l, GenBinTree & r) noexcept
  {
    Aleph::split_key_dup_rec<Node, Compare>(root, key, l.root, r.root, cmp);
  }

  /** Remove a key from the tree

      @param[in] key to remove
      @return a valid pointer to the removed node if `key` was found
      in the tree, `nullptr` otherwise
  */
  Node * remove(const Key & key) noexcept
  {
    return BinTree_Operation<Node, Compare>(cmp).remove(root, key);
  }

      /** Join `tree` with `this`. Duplicated keys of `tree` are put in
	  `dup` parameter.

          @param[in,out] tree to join with `this`
          @param[out] dup tree where the duplicated key belonging to
          `tree` are put.
       */
  void join(GenBinTree & tree, GenBinTree & dup) noexcept
  {
    auto op = BinTree_Operation<Node, Compare>(cmp);
    move_all(tree.root, [&] (Node * n) noexcept
             {
               if (op.insert(root, n) == Node::NullPtr)
                 op.insert(dup.root, n);
             });
  }

  /** Join `this` with `t` independently of the presence of duplicated keys

      `join(t)` produces a random tree result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      @param[in] t tree to join with `this`
      keys are inserted
  */
  void join_dup(GenBinTree & t) noexcept
  {
    auto op = BinTree_Operation<Node, Compare>(cmp);
    move_all(t.root, [&] (Node * n) noexcept { op.insert_dup(root, n); });
  }

  /** Join exclusive of `this` with `t`

      Exclusive means that all the keys of `this` are lesser than all
      the keys of `t`. This knowledge allows a more effcient way for
      joining that when the keys ranks are overlapped. However, use very
      carefully because the algorithm does not perform any check and the
      result would be incorrect.

      @param[in] t tree to exclusively join with `this`
      keys are inserted
  */
  void join_exclusive(GenBinTree & t) noexcept
  {
    root = Aleph::join_exclusive<Node>(root, t.root);
    t.root = Node::NullPtr;
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(GenBinTree & tree) : BinNodeInfixIterator<Node>(tree.getRoot()) {}
    Iterator(const GenBinTree & tree)
      : BinNodeInfixIterator<Node>(tree.getRoot()) {}
  };
};

    /** Binary search tree with nodes without virtual destructors,

        @see GenBinTree BinTreeVtl DynBinTree

        @ingroup Arboles
     */
    template <typename Key, class Compare = Aleph::less<Key>>
struct BinTree : public GenBinTree<BinNode, Key, Compare>
{
  using Base = GenBinTree<BinNode, Key, Compare>;
  using Base::Base;
};


    /** Binary search tree with nodes with virtual destructors,

        @see GenBinTree BinTreeVtl DynBinTree

        @ingroup Arboles
     */
    template <typename Key, class Compare = Aleph::less<Key>>
struct BinTreeVtl : public GenBinTree<BinNodeVtl, Key, Compare>
{
  using Base = GenBinTree<BinNodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph
# endif /* TPL_BINTREE_H */

