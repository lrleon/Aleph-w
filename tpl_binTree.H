
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


/** @file tpl_binTree.H
 *  @brief Generic unbalanced binary search tree.
 *
 *  Basic BST without balancing. Use for educational purposes or when
 *  data is known to be randomly ordered. For general use, prefer
 *  balanced trees like AVL or Red-Black.
 *
 *  ## Complexity
 *  - Average: O(log n) for balanced input
 *  - Worst: O(n) for sorted input
 *
 *  @see tpl_avl.H AVL tree (balanced)
 *  @see tpl_rb_tree.H Red-Black tree (balanced)
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BINTREE_H
# define TPL_BINTREE_H

# include <utility>

# include <tpl_binTreeOps.H>
# include <ah-concepts.H>

using namespace Aleph;

namespace Aleph {

    /** @brief Simple (unbalanced) binary search tree.

        GenBinTree implements a basic binary search tree without any balancing
        operations. The tree's performance depends entirely on the insertion order
        of keys.

        @par Performance Characteristics:
        - **Random insertion order** + **few deletions**: Operations trend toward
          O(log n) due to probabilistic balance. This is often the best choice
          for simplicity when insertion order is naturally random.
        - **Many deletions**: Performance degrades to approximately O(√n) as
          the tree becomes less balanced over time.
        - **Sequential insertion order**: Worst case O(n) - tree degenerates
          into a linked list. **Do not use** if insertion order is not random.

        @par When to Use:
        ✓ Insertion order is random or randomized
        ✓ Few deletions relative to insertions
        ✓ Simplicity and minimal overhead are priorities
        ✓ Testing/benchmarking against balanced trees

        @par When NOT to Use:
        ✗ Cannot guarantee random insertion order
        ✗ Frequent deletions
        ✗ Need guaranteed O(log n) worst-case performance
        ✗ Production code with adversarial inputs

        **WARNING**: If you cannot ensure random insertion order, use a
        balanced tree (Avl_Tree, Rb_Tree, Treap, etc.) instead.

        @tparam NodeType Node template (BinNode or BinNodeVtl).
        @tparam Key The type of keys stored in the tree.
        @tparam Compare Comparison functor for ordering keys.

        @par Complexity (assuming random insertion order):
        - Search: O(log n) expected, O(n) worst case
        - Insert: O(log n) expected, O(n) worst case
        - Delete: O(log n) expected, O(n) worst case
        - Space: O(n) - minimal overhead

        @par Example:
        @code
        BinTree<int> tree;

        // Insert in random order (GOOD)
        std::vector<int> keys = {50, 25, 75, 10, 30, 60, 90};
        std::shuffle(keys.begin(), keys.end(), rng);
        for (int k : keys)
          tree.insert(new BinTree<int>::Node(k));

        // Search
        auto node = tree.search(30);

        // Remove
        auto removed = tree.remove(25);
        delete removed;

        // BAD: Sequential insertion (creates linked list)
        // for (int i = 0; i < 1000; ++i)
        //   tree.insert(new BinTree<int>::Node(i));  // O(n²) total time!
        @endcode

        @note This is a low-level implementation managing raw nodes.
              For automatic memory management, use DynSetBinTree.
        @note No balancing overhead - fastest when insertion order is favorable.
        @note Use as a baseline for benchmarking balanced tree implementations.

        @see BinTree Convenient typedef for BinTree<Key, Compare>.
        @see Avl_Tree Balanced alternative with O(log n) worst-case guarantee.
        @see Treap Randomized alternative with O(log n) expected (simpler than AVL).
        @see DynSetBinTree High-level wrapper with automatic memory management.
        @ingroup Trees
     */
  template <template <typename> class NodeType, typename Key, class Compare>
    requires StrictWeakOrder<Compare, Key>
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

     @ingroup Trees
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    /// Default constructor creates an "end" iterator
    Iterator() noexcept = default;
    Iterator(GenBinTree & tree) : BinNodeInfixIterator<Node>(tree.getRoot()) {}
    Iterator(const GenBinTree & tree)
      : BinNodeInfixIterator<Node>(tree.getRoot()) {}
  };
};

    /** Binary search tree with nodes without virtual destructors,

        @see GenBinTree BinTreeVtl DynBinTree

        @ingroup Trees
     */
    template <typename Key, class Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
struct BinTree : public GenBinTree<BinNode, Key, Compare>
{
  using Base = GenBinTree<BinNode, Key, Compare>;
  using Base::Base;
};


    /** Binary search tree with nodes with virtual destructors,

        @see GenBinTree BinTreeVtl DynBinTree

        @ingroup Trees
     */
    template <typename Key, class Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
struct BinTreeVtl : public GenBinTree<BinNodeVtl, Key, Compare>
{
  using Base = GenBinTree<BinNodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph
# endif /* TPL_BINTREE_H */

