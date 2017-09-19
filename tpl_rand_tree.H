
# ifndef TPL_RAND_TREE_H
# define TPL_RAND_TREE_H

# include <limits.h>
# include <gsl/gsl_rng.h>
# include <ahUtils.H>
# include <ahFunction.H>
# include <tpl_randNode.H>
# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph { 

  /** Randomized binary sarch tree.

      This class implements a randomized binary search tree. It is shown
      that this type of tree always is equivalent to a classic binary
      search tree built from a random insertion sequence. Consequently, all
      the operations of this tree are \f$O(\lg n)\f$ expected case,
      independentely of insertion order and of removals are interleaved
      with insertions.

      In addition, these trees support `select()` and `position()`
      operations. That is, their keys can be accessed according to their
      inorder position and logarithmic time. This allows to treat the
      tree as if it was an array.

      This tree type is unbeatable when there are splits and and
      especially joins operations on very large data sets. Other tree
      types perform the join of independent data sets in \f$O(n \lg
      m)\f$, where \f$n\f$ and \f$m\f$ are the size of two data
      sets, while the randomized approach takes \f$O(\max(\lg n, \lg
      m))\f$. 

      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/). 
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
   */
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rand_Tree
{
public:

  using Node = NodeType<Key>;  /// The type of node

private:

  Node *    tree_root; 
  gsl_rng * r;
  Compare   cmp;

  Node * random_insert(Node * root, Node * p) noexcept
  {
    const long & n = COUNT(root); 
    const size_t rn = gsl_rng_uniform_int(r, n + 1);
    if (rn == n) // p wins the raffle?
      return BinTreeXt_Operation<Node, Compare>(cmp).insert_root(root, p);

    Node * result;
    if (cmp(KEY(p), KEY(root))) // KEY(p) < KEY(root) ?
      {
        result = random_insert(LLINK(root), p); 
        if (result != Node::NullPtr) // p was inserted?
          {
            LLINK(root) = result;
            ++COUNT(root); 
            return root;
          }
      }
    else if (cmp(KEY(root), KEY(p))) // KEY(p) > KEY(root) ?
      {
        result = random_insert(RLINK(root), p); 
        if (result != Node::NullPtr) // p was inserted?
          { 
            RLINK(root) = result;
            ++COUNT(root); 
            return root;
          }
      }

    return Node::NullPtr; // duplicated key ==> no insertion
  }

  Node * random_insert_dup(Node * root, Node * p) noexcept
  {
    const long & n = COUNT(root); 
    const size_t rn = gsl_rng_uniform_int(r, n + 1);
    if (rn == n) // p wins the raffle?
      return BinTreeXt_Operation<Node, Compare>(cmp).insert_dup_root(root, p); 

    if (cmp(KEY(p), KEY(root))) // KEY(p) < KEY(root) ?
      LLINK(root) = random_insert_dup(LLINK(root), p); 
    else
      RLINK(root) = random_insert_dup(RLINK(root), p); 
      
    ++COUNT(root); 

    return root;
  }

  Gen_Rand_Tree& operator = (const Gen_Rand_Tree&);

  void init(unsigned long seed)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);

    if (r == nullptr)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

public:

      /// return the comparison criteria
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  /// Get a pointer to gsl random number generator
  gsl_rng * gsl_rng_object() noexcept { return r;}

  /// Set the random number generator seed
  void set_seed(unsigned long seed) noexcept { gsl_rng_set(r, seed); }

  /// Initialize a random tree with random `seed` and comparison criteria
  /// `__cmp` 
  Gen_Rand_Tree(unsigned int seed, Compare & __cmp) noexcept
    : tree_root(Node::NullPtr), r(nullptr), cmp(__cmp)
  {
    init(seed);
  }

  Gen_Rand_Tree(unsigned int seed, Compare && cmp = Compare()) noexcept
    : Gen_Rand_Tree(seed, cmp)
  {
    // empty 
  }

  /// Initialize a random tree with random seed from system time and
  /// comparison criteria `cmp`
  Gen_Rand_Tree(Compare & cmp) noexcept 
  : Gen_Rand_Tree(time(nullptr), cmp) { /* empty */ }

  Gen_Rand_Tree(Compare && cmp = Compare()) noexcept
    : Gen_Rand_Tree(time(nullptr), std::forward<Compare>(cmp)) { /* empty */ }

  /// Swap in constant time all the nodes of `this` with `tree`
  void swap(Gen_Rand_Tree & tree) noexcept
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(r, tree.r);
    std::swap(cmp, tree.cmp);
  }

  virtual ~Gen_Rand_Tree() noexcept
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

   /** Insert a node in the tree

       @param[in] p pointer to the node to insert
       @return if `p->get_key()` is not in the tree, then the pointer
       `p` is returned (it was inserted); othewise, `nullptr` is returned
   */
  Node * insert(Node * p) noexcept
  {
    assert(p != Node::NullPtr);
    assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
    assert(COUNT(p) == 1);

    Node * result = random_insert(tree_root, p);
    if (result == Node::NullPtr) 
      return nullptr;

    return tree_root = result;
  }

  /** Insert a node in the tree.

      This method does not fail. It always inserts.

      @param[in] p pointer to the node to insert
      @return the `p` pointer
  */
  Node * insert_dup(Node * p) noexcept
  {
    assert(p != Node::NullPtr);
    assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
    assert(COUNT(p) == 1);

    return tree_root = random_insert_dup(tree_root, p);
  }

private:

  Node * random_join_exclusive(Node * tl, Node * tr) noexcept
  {
    if (tl == Node::NullPtr) 
      return tr;

    if (tr == Node::NullPtr) 
      return tl;

    const size_t & m = COUNT(tl);
    const size_t & n = COUNT(tr);
    const size_t rn = 1 + gsl_rng_uniform_int(r, m + n);  
    if (rn <= m) 
      {    // left subtree wins
        COUNT(tl) += COUNT(tr);
        RLINK(tl) = random_join_exclusive(RLINK(tl), tr);
        return tl;
      }
    else 
      {
        COUNT(tr) += COUNT(tl);
        LLINK(tr) = random_join_exclusive(tl, LLINK(tr));
        return tr;
      }
  }

  Node * random_remove(Node *& root, const Key & key) noexcept
  {
    if (root == Node::NullPtr) 
      return Node::NullPtr;

    Node * ret_val;
    if (cmp(key, KEY(root)))
      {
        ret_val = random_remove(LLINK(root), key);
        if (ret_val != Node::NullPtr)
          COUNT(root)--;

        return ret_val;
      }
    else if (cmp(KEY(root), key))
      {
        ret_val = random_remove(RLINK(root), key);
        if (ret_val != Node::NullPtr)
          COUNT(root)--;

        return ret_val;
      }

        // key was found
    ret_val = root;
    root = random_join_exclusive(LLINK(root), RLINK(root));
    ret_val->reset();

    return ret_val;
  }

public:

 /** Remove a key from the tree

      @param[in] key to remove
      @return a valid pointer to the removed node if `key` was found
      in the tree, `nullptr` otherwise
  */
  Node * remove(const Key & key) noexcept
  {
    Node * ret_val = random_remove(tree_root, key);

    return ret_val != Node::NullPtr ? ret_val : nullptr;
  }

   /** Search a key.

       @param[in] key to be searched
       @return a pointer to the containing key if this was found;
       otherwise `nullptr` 
    */
  Node * search(const Key & key) const noexcept
  {
    Node * retVal = searchInBinTree<Node, Compare>(tree_root, key, cmp);
    return retVal == Node::NullPtr ? nullptr : retVal;
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
  Node * search_or_insert(Node * p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);

    Node * result = search(KEY(p));
    if (result != nullptr)
      return result;

    tree_root = random_insert(tree_root, p);

    return p;
  }

  /// Return `true` if this is a consistent randomized tree
  bool verify() const
  {
    return check_rank_tree(tree_root);
  }

  /// Return the tree's root
  Node *& getRoot() noexcept { return tree_root; }

  /** Return the i-th node in inorder sense


      @param[in] i inorder position to be located
      @return a valid pointer to the i-th node inorder sense
      @throw out_of_range if i is greater or equal that the number of nodes
  */
  Node * select(const size_t i) const noexcept
  {
    return Aleph::select(tree_root, i); 
  }

  /// Return the number of nodes that have the tree
  size_t size() const noexcept { return COUNT(tree_root); }

    /** Compute the inorder position of a key

	@param[in] key to be searched
	@return a tuple with the position of key and a pointer
	to the node containing it. If `key` is not in the tree, then
	first the first value is -1.

	@ingroup Arboles
    */
  std::pair<long, Node*> position(const Key & key) noexcept
  {
    std::pair<long, Node*> ret_val;
    ret_val.first = BinTreeXt_Operation<Node, Compare> (cmp).
      inorder_position(tree_root, key, ret_val.second);

    return ret_val;
  }

  /** Find the inorder position of a key in the tree.

      `find_position(key)` determines the inorder position that has or
      had `key` in the tree. Themethod return a tuple with a position
      and a node pointer.

      If `key` is found then its inorder position is returned along with
      a pointer to the node containing it.

      Otherwise, the the tuple returns the position that would have `key`
      if this was in the tree and the parent node that the key would
      had. At this regard, there are three cases: 
      1. if `key` is lesser than the minimum key of tree, then first is
         -1 and the node is one with the smallest key.
	 
      2. If `key` is greater than the maximum key in the tree, then
         first is the number of keys and the node is one with the
         maximum key in the tree. 

      3. For any other case, first is the inorder position that would
         have `key` if this was in the tree and second is the node 
	 whose key is inmediataly greater than `key`.

      @param[in] key to be searched
      @return a pair with the inorder position and and related node
     */
  std::pair<long, Node*> find_position(const Key & key) noexcept
  {
    std::pair<long, Node*> r(-2, nullptr);
    
    r.first = BinTreeXt_Operation<Node, Compare> (cmp) .
      find_position(tree_root, key, r.second);

    return r;
  }

private:

  Node * remove_pos(Node *& root, const size_t pos) noexcept
  {
    if (pos == COUNT(LLINK(root)))
      {
	Node * ret_val = root;
	root = random_join_exclusive(LLINK(ret_val), RLINK(ret_val));
	return ret_val;
      }

    --COUNT(root);
    if (pos < COUNT(LLINK(root)))
      return remove_pos(LLINK(root), pos);
    else
      return remove_pos(RLINK(root), pos - COUNT(LLINK(root)) - 1);
  }

public:

  /** Remove the key at inorder position `i`

      @param[in] i inorder position to be removed
      @return a valid pointer to the removed node
      \throw out_of_range if `i` is greater or equal than the number of
      nodes of tree
   */
  Node * remove_pos(const size_t i) noexcept
  {
    if (i >= COUNT(tree_root))
      throw std::out_of_range("infix position out of range");

    return remove_pos(tree_root, i);
  }

  /** Split the tree according to a key

      @param[in] key for splitting
      @param[out] t1 tree with keys lesser than `key`
      @param[out] t2 tree with keys greater than `key`
      @return `true` if tree was split; that is if `key` is not in the
      tree. Otherwise, if `key` is in the tree, `false` is returned
  */
  bool split_key(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
    noexcept
  {
    return split_key_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

   /** Split the tree according to a key that could be in the tree

      `split_dup(key, t1, t2)` splits the tree according to `key` in two
      trees `t1` which contains the key lesser than `key` and `t2` which
      contains the keys greater or equal than `key`. 

      @param[in] key for splitting
      @param[out] t1 resulting tree with the keys lesser than `key`
      @param[out] t2 resulting tree with the keys greater or equal than `key`
  */ 
  void split_key_dup(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2) 
    noexcept
  {
    split_key_dup_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  /**  Split a extended binary tree according to a position

       `split_pos(pos, ts, tg)` splits the tree two trees `t1` and `t2`
       according to a position `pos`. `t1` contains the keys from 0 to
       `pos - 1` inorder sense and `tg` the keys from `pos` to n -
       1. After completion `this` becames empty.

       @param[in] pos position for splitting
       @param[out] t1 tree where the rank of keys \f$[0, i)\f$ will be put
       @param[out] t2 tree where the rank of keys \f$[i, n]\f$ will be put
  */
  void split_pos(size_t pos, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2) noexcept
  {
    split_pos_rec(tree_root, pos, t1.getRoot(), t2.getRoot());
  }

private:

  Node * random_join(Node * t1, Node * t2, Node *& dup)
  {
    if (t1 == Node::NullPtr) 
      return t2;

    if (t2 == Node::NullPtr) 
      return t1;

    Node * ret = Node::NullPtr;
    const size_t & m = COUNT(t1);
    const size_t & n = COUNT(t2);
    const size_t rn = 1 + gsl_rng_uniform_int(r, m + n);
    if (rn <= m)
      {      // root of t1 wins
	Node * l = LLINK(t1), * r = RLINK(t1);
	t1->reset();
      t1_wins:
	ret = insert_root_xt(t2, t1, cmp);
	if (ret != Node::NullPtr)
	  {
	    LLINK(ret) = random_join(LLINK(ret), l, dup);
	    RLINK(ret) = random_join(RLINK(ret), r, dup);
	    COUNT(ret) = COUNT(LLINK(ret)) + 1 + COUNT(RLINK(ret));
	  }
	else
	  {
	    dup = random_insert_dup(dup, random_remove(t2, KEY(t1)));
	    goto t1_wins;
	  }
      }
    else 
      {     // root of t2 wins
	Node * l = LLINK(t2), * r = RLINK(t2);
	t2->reset();
      t2_wins:
	ret = insert_root_xt(t1, t2, cmp);
	if (ret != Node::NullPtr)
	  {
	    LLINK(ret) = random_join(l, LLINK(ret), dup);
	    RLINK(ret) = random_join(r, RLINK(ret), dup);
	    COUNT(ret) = COUNT(LLINK(ret)) + 1 + COUNT(RLINK(ret));
	  }
	else
	  {
	    dup = random_insert_dup(dup, random_remove(t1, KEY(t2)));
	    goto t2_wins;
	  }
      }
    
    return ret;
  }

  Node * random_join(Node * t1, Node * t2)
  {
    if (t1 == Node::NullPtr) 
      return t2;

    if (t2 == Node::NullPtr) 
      return t1;

    Node * ret = Node::NullPtr;
    const size_t & m = COUNT(t1);
    const size_t & n = COUNT(t2);
    const size_t total = m + n;
    const size_t rn = 1 + gsl_rng_uniform_int(r, total);
    if (rn <= m) 
      {     // root of t1 wins
	Node * l = LLINK(t1), * r = RLINK(t1);
	t1->reset();
	ret  = insert_dup_root_xt(t2, t1, cmp);
	LLINK(ret) = random_join(LLINK(ret), l);
	RLINK(ret) = random_join(RLINK(ret), r);
      }
    else 
      {     // root of t2 wins
	Node * l = LLINK(t2), * r = RLINK(t2);
	t2->reset();
	ret  = insert_dup_root_xt(t1, t2, cmp);
	LLINK(ret) = random_join(l, LLINK(ret));
	RLINK(ret) = random_join(r, RLINK(ret));
      }

    COUNT(ret) = total;
    return ret;
  }

public:

  /** Join `this` with `t` filtering the duplicated keys

      `join(t, dup)` produces a random tree result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      Nodes containing duplicated keys are inserted in the randomized
      tree `dup`. The nodes could belong to any of two trees

      @param[in] t ramdomized tree to join with `this`
      @param[out] dup ramdomized tree where nodes containing duplicated
      keys are inserted
   */
  void join(Gen_Rand_Tree & t, Gen_Rand_Tree & dup) noexcept
  {
    tree_root = random_join(tree_root, t.tree_root, dup.tree_root);
    t.tree_root = Node::NullPtr;
  }

  /** Join `this` with `t` independently of the presence of duplicated keys

      `join(t)` produces a random tree result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      @param[in] t ramdomized tree to join with `this`
      keys are inserted
   */
  void join_dup(Gen_Rand_Tree & t) noexcept
  {
    tree_root = random_join(tree_root, t.tree_root);
    t.tree_root = Node::NullPtr;
  }

  /** Join exclusive of `this` with `t`

      Exclusive means that all the keys of `this` are lesser than all
      the keys of `t`. This knowlege allows a more effcient way for
      joining that when the keys ranks are overlapped. However, use very
      carefully because the algorithm does not perform any check and the
      result would be incorrect.

      @param[in] t ramdomized tree to exclusively join with `this`
      keys are inserted
  */
  void join_exclusive(Gen_Rand_Tree & t) noexcept
  {
    tree_root = random_join_exclusive(tree_root, t.tree_root);    
    t.tree_root = Node::NullPtr;
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(Gen_Rand_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};

  /** Randomized binary sarch tree.

      This class implements a randomized binary search tree. It is shown
      that this type of tree always is equivalent to a classic binary
      search tree built from a random insertion sequence. Consequently, all
      the operations of this tree are \f$O(\lg n)\f$ expected case,
      independentely of insertion order and of removals are interleaved
      with insertions.
      
      The principle of this tree is balancing by taking random decisions
      whose probabilities guarantee that each node will have a chance of
      \f$1/n\f$ for becoming the root of the tree.

      In addition, these trees support `select()` and `position()`
      operations. That is, their keys can be accessed according to their
      inorder position and logarithmic time. This allows to treat the
      tree as if it was an array.

      This tree type is unbeatable when there are splits and and
      especially joins operations on very large and independent data
      sets. Other tree types perform the join of independent data sets
      in \f$O(n \lg m)\f$, where \f$n\f$ and \f$m\f$ are the size of two
      data sets, while the randomized approach takes \f$O(\max(\lg n,
      \lg m))\f$.

      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/). 
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @see Treap Treap_Vtl Treap_Rk 

      @ingroup Arboles
   */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rand_Tree : public Gen_Rand_Tree<RandNode, Key, Compare> 
{
  using Base = Gen_Rand_Tree<RandNode, Key, Compare>;
  using Base::Base;
};


  /** Randomized binary sarch tree.

      This class implements a randomized binary search tree. It is shown
      that this type of tree always is equivalent to a classic binary
      search tree built from a random insertion sequence. Consequently, all
      the operations of this tree are \f$O(\lg n)\f$ expected case,
      independentely of insertion order and of removals are interleaved
      with insertions.

      In addition, these trees support `select()` and `position()`
      operations. That is, their keys can be accessed according to their
      inorder position and logarithmic time. This allows to treat the
      tree as if it was an array.

      This tree type is unbeatable when there are splits and and
      especially joins operations on very large data sets. Other tree
      types perform the join of independent data sets in \f$O(n \lg
      m)\f$, where \f$n\f$ and \f$m\f$ are the size of two data
      sets, while the randomized approach takes \f$O(\max(\lg n, \lg
      m))\f$. 

      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/). 
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @see Treap Treap_Vtl Treap_Rk 

      @ingroup Arboles
   */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Rand_Tree_Vtl : public Gen_Rand_Tree<RandNodeVtl,Key,Compare>
{
  using Base = Gen_Rand_Tree<RandNodeVtl,Key,Compare>;
  using Base::Base;
};


} // end namespace Aleph

# endif // TPL_RAND_TREE_H 

