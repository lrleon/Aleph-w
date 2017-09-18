
# ifndef TPL_BINTREE_H
# define TPL_BINTREE_H

# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph {

    /** Simple binary search tree.

	This class implements a simple binary search tree. By simple we
	want to say that no balance operation are done. Consequently the
	performance depends on whether keys insertion order.

	In general, if the insertion order is random and there is no
	many removals, the this tree is very good and its operations
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
  Compare & cmp;

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
  GenBinTree(Compare & __cmp) noexcept
    : head(&headNode), root(headNode.getR()), cmp(__cmp)
  {
    // empty
  }

  GenBinTree(Compare && cmp = Compare()) noexcept : GenBinTree(cmp) 
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

  virtual ~GenBinTree() noexcept { /* Empty */ }

  /// Return `true` if the tree is a consistent (correct) binary search tree
  bool verify() const { return check_bst<Node, Compare>(root); }

    /// Return the root of tree
  Node *& getRoot() { return root; }

  /// \overload verify()
  bool verifyBin() const { return check_bst<Node, Compare>(root); } 

      /** Insert a node in the tree

          @param[in] p pointer to the node to insert
          @return if `p->get_key()` is not in the tree, then the pointer
          `p` is returned (it was inserted); othewise, `nullptr`
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
    BinTree_Operation<Node, Compare>(cmp).
      split_key_dup_rec(root, key, l.root, r.root);
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
    root = BinTree_Operation<Node, Compare>(cmp).join(root, tree.root, dup);
    tree.root = Node::NullPtr;
  }

  /** Join `this` with `t` independently of the presence of duplicated keys

      `join(t)` produces a random tree result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      @param[in] t tree to join with `this`
      keys are inserted
  */
  void join_dup(GenBinTree & t) noexcept
  {
    root = BinTree_Operation<Node, Compare>(cmp).join_dup(root, t.root);
    t.tree_root = Node::NullPtr;
  }

  /** Join exclusive of `this` with `t`

      Exclusive means that all the keys of `this` are lesser than all
      the keys of `t`. This knowlege allows a more effcient way for
      joining that when the keys ranks are overlapped. However, use very
      carefully because the algorithm does not perform any check and the
      result would be incorrect.

      @param[in] t tree to exclusively join with `this`
      keys are inserted
  */
  void join_exclusive(GenBinTree & t) noexcept
  {
    root = join_exclusive(root, t.root);    
    t.root = Node::NullPtr;
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(GenBinTree & tree) : BinNodeInfixIterator<Node>(tree.getRoot()) {}
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

