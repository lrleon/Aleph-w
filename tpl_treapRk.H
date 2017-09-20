# ifndef TPL_TREAPRK_H
# define TPL_TREAPRK_H

# include <gsl/gsl_rng.h>
# include <ahFunction.H>
# include <tpl_binTreeOps.H>
# include <ran_array.h>
# include <treapNode.H>

using namespace Aleph;

namespace Aleph {

class TreapRkNode_Data
{
  unsigned long priority; 

  unsigned long count;

public:

  TreapRkNode_Data() noexcept : priority(Max_Priority), count(1) 
  { 
    /* empty */ 
  }

  TreapRkNode_Data(SentinelCtor) noexcept : priority(Max_Priority), count(0) 
  { 
    /* empty */ 
  }

  unsigned long & getPriority() noexcept { return priority; }

  unsigned long & getCount() noexcept { return count; }

  void reset() noexcept { count = 1; }
};

DECLARE_BINNODE_SENTINEL(Treap_Rk_Node, 80, TreapRkNode_Data);
  
  /** Extended treap (a special type of ramdomized binary search tree)
      which manages selection and splitting for inorder position.

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its
      operations. In addition, this extended tree uses and second
      integer for storing subtrees sizes.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). Since the priority is chosen
      just one time and the adjustements are done in a botton-top way
      (by contrast with the randomized which is top-bottom), the treap
      takes less time. 

      Although tgus approach trends to be faster than the randomized
      trees, takes in account that this treap is more space consuming
      because each node requires 2 additional integers to the data
      (priority and counter) in constrast with the radomized which only
      requieres one interger (the counter).

      For splitting and join of independent and large data sets the
      ramdomized option trends to be faster. The split is equivalent,
      but the join is definitively faster. The join of two trees of n
      and m keys respectively takes \f$O(n \lg m)\f$ with treaps, while
      it takes \f$O(\max(\lg n, \lg m))\f$ with radomized trees. In
      addition,
      
      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/).
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */
    template <template <class> class NodeType, class Key, class Compare>
class Gen_Treap_Rk
{
public:

  using Node = NodeType<Key>; /// The type of node

private:

  Node      head;
  Node *    head_ptr;
  Node *&   tree_root;
  gsl_rng * r;
  Compare   cmp;

  void init(unsigned int seed)
  {
    PRIO(head_ptr) = Min_Priority;
    r = gsl_rng_alloc(gsl_rng_mt19937);

    if (r == nullptr)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

public:

   /// Set the random number generator seed
  void set_seed(unsigned long seed) noexcept { gsl_rng_set(r, seed); }

  /// return the comparison criteria
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  /// Initialize a treap with random `seed` and comparison criteria
  /// `__cmp` 
  Gen_Treap_Rk(unsigned long seed, Compare cmp) 
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(nullptr), cmp(cmp)
  {
    init(seed);
  }

  Gen_Treap_Rk(Compare cmp = Compare()) : Gen_Treap_Rk(time(nullptr), cmp)
  {
    // empty
  }

  ~Gen_Treap_Rk()
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

  /// Swap in constant time all the nodes of `this` with `tree`
  void swap(Gen_Treap_Rk & tree) noexcept
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(cmp, tree.cmp);
    std::swap(r, tree.r);
  }

  /// Return the tree's root
  Node *& getRoot() noexcept { return tree_root; }

  /** Search a key in a treap

      @param[in] key to be searched
      @return a pointer to the node containing `key` if this is found;
      otherwise, `nullptr` is returned
  */
  Node * search(const Key & key) const noexcept
  {
    Node* ret_val = searchInBinTree(tree_root, key, cmp);
    return ret_val == Node::NullPtr ? nullptr : ret_val;
  }

private:

  bool insert(Node *& root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      {
	root = p;
	return true;
      }

    if (cmp(KEY(p), KEY(root)))
      {
	if (insert(LLINK(root), p))
	  {
	    ++COUNT(root);
	    if (PRIO(LLINK(root)) < PRIO(root) )
	      root = rotate_to_right_xt(root);

	    return true;
	  }
      }
    else if (cmp(KEY(root), KEY(p)))
      {
	if (insert(RLINK(root), p))
	  {
	    ++COUNT(root);
	    if (PRIO(RLINK(root)) < PRIO(root) )
	      root = rotate_to_left_xt(root);

	    return true;
	  }
      }

    return false;
  }

  // Search or insert p. Return p if KEY(p) is not in the tree;
  // otherwise, it returns a pointer to a node containing KEY(p) 
  Node * search_or_insert(Node *& root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
	Node * ret = search_or_insert(LLINK(root), p);
        if (ret == p) // insertion done?
	  { // yes ==> increase counter and perhaps rotate
	    ++COUNT(root);
	    if (PRIO(LLINK(root)) < PRIO(root))
	      root = rotate_to_right_xt(root);
	    
	    assert(PRIO(root) <= PRIO(LLINK(root)) and
		   PRIO(root) <= PRIO(LLINK(root)));
	  }

	return ret;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
	Node * ret = search_or_insert(RLINK(root), p);
        if (ret == p) // insertion done?
	  { // yes ==> increase counter and perhaps rotate
	    ++COUNT(root);
	    if (PRIO(RLINK(root)) < PRIO(root))
	      root = rotate_to_left_xt(root);

	    assert(PRIO(root) <= PRIO(LLINK(root)) and
		   PRIO(root) <= PRIO(LLINK(root)));
	  }

	return ret;
      }

    assert(PRIO(root) <= PRIO(LLINK(root)) and PRIO(root) <= PRIO(LLINK(root)));

    return root;
  }

  // Return p; root could be modified
  Node * insert_dup(Node *& root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
        insert_dup(LLINK(root), p);
	++COUNT(root);
        if (PRIO(LLINK(root)) < PRIO(root))
          root = rotate_to_right_xt(root);
      }
    else
      {
	insert_dup(RLINK(root), p);
	++COUNT(root);
	if (PRIO(RLINK(root)) < PRIO(root))
	  root = rotate_to_left_xt(root);
      }

    return p;
  }

public:

    /** Insert a node in a treap.

	insert(p) inserta el nodo p en el treap this.

	@param[in] p pointer to the node to be inserted
	@return if `p->get_key()` is not in the tree, then `p` is
	inserted and this node pointer is returned. Otherwise, it is
	returned `nullptr`.
    */
  Node * insert(Node * p) noexcept
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return insert(tree_root, p) ? p : nullptr;
  }

  /** Insert a node in the tree.

      This method does not fail. It always inserts.

      @param[in] p pointer to the node to insert
      @return the `p` pointer
  */
  Node * insert_dup(Node * p) noexcept
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return insert_dup(tree_root, p);
  }

 
  /** Search or insert a key.

      `search_or_insert(p)` searches in the tree the key `KEY(p)`. If
      this key is found, then a pointer to the node containing it is
      returned. Otherwise, `p` is inserted.

      @param[in] p node containing a key to be searched and
      eventually inserted
      @return if the key contained in `p` is found, then a pointer
      to the containing key in the tree is returned. Otherwise, `p`
      is inserted and returned
  */
  Node * search_or_insert(Node * p) noexcept
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return search_or_insert(tree_root, p);
  }

  /// Return `true` if the treap is consistent
  bool verify() const { return is_treap(tree_root); }

private:

  static Node * join_exclusive(Node * t1, Node * t2) noexcept
  {
    if (t1 == Node::NullPtr)
      return t2;

    if (t2 == Node::NullPtr)
      return t1;

    if (PRIO(t1) < PRIO(t2))
      {
	COUNT(t1) += COUNT(t2);
	RLINK(t1) = join_exclusive(RLINK(t1), t2);
	return t1;
      }
    else
      {
	COUNT(t2) += COUNT(t1);
	LLINK(t2) = join_exclusive(t1, LLINK(t2) );
	return t2;
      }
  }

  Node * remove(Node *& root, const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return Node::NullPtr;

    Node * ret_val;
    if (cmp(key, KEY(root) ))
      ret_val = remove(LLINK(root), key);
    else if (cmp(KEY(root), key))
      ret_val = remove(RLINK(root), key);
    else
      {
	ret_val = root;
	root = join_exclusive(LLINK(root), RLINK(root) );
	
	return ret_val;
      }

    if (ret_val == Node::NullPtr)
      return Node::NullPtr;
      
    --COUNT(root);

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
    Node * ret_val = remove(tree_root, key);
    if (ret_val == Node::NullPtr)
      return nullptr;

    ret_val->reset();

    return ret_val;
  } 

  /** Remove from the treap all the keys between  inorder position `beg`
      and `end`.

      @param[in] beg beggining inorder position
      @param[in] end ending inorder position
      @return a pointer to a tree root containing all the removed nodes
      @throw range_error if the range is invalid
  */
  Node * remove(const size_t beg, const size_t end) noexcept
  {
    if (beg > end or end > COUNT(tree_root))
      throw std::range_error("remove of TreapRk out of range");

    Node * before_beg, * after_end, * aux;

    Node * ret_val = tree_root;

    split_pos_rec(ret_val, end + 1, aux, after_end);
    split_pos_rec(aux, beg, before_beg, ret_val);

    tree_root = join_exclusive(before_beg, after_end);

    return ret_val;
  }

private:

  static Node * remove_pos(Node *& root, const size_t pos) noexcept
  {
    if (pos == COUNT(LLINK(root)))
      {
	Node * ret_val = root;
	root = join_exclusive(LLINK(ret_val), RLINK(ret_val));
	return ret_val;
      }

    --COUNT(root);
    if (pos < COUNT(LLINK(root)))
      return remove_pos(LLINK(root), pos);
    else
      return remove_pos(RLINK(root), pos - COUNT(LLINK(root)) - 1);
  }
  
public:

  /** Remove the node at the inorder position pos

      @param[in] pos inorder position of node to be removed
      @return a vaid pointer to the removed node
      @throw out_of_range if `pos` is greater or equal than the number
      of nodes of tree
   */
  Node * remove_pos(const size_t pos)
  {
    if (pos >= COUNT(tree_root))
      throw std::out_of_range("infix position out of range");

    return remove_pos(tree_root, pos);
  }

      /** Return the i-th node in order sense

	  @param[in] i inorder position of node to be selected
	  @return a pointer to the i-th node inorder sense
	  @throw out_of_range if `pos` is greater or equal than the number
	  of nodes of tree
      */
  Node * select(const size_t i) const
  {
    return Aleph::select(tree_root, i); 
  }

      /// Return the number of nodes contained in the tree
  size_t size() const noexcept { return COUNT(tree_root); }

      /// Return `true` if tree is empty
  bool is_empty() const noexcept { return tree_root == Node::NullPtr; }

  /**Compute the inorder position of a key

      @param[in] r root of tree
      @param[in] key to be searched
      @return inorder position of `key` if this is in the tree or -1 if
      `key` is not found

      @ingroup Arboles
   */
  std::pair<int, Node*> position(const Key & key) const noexcept
  {
    std::pair<int, Node*> ret_val;

    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
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
  std::pair<int, Node*> find_position(const Key & key) const noexcept
  {
    std::pair<int, Node*> r(-2, nullptr);

    BinTreeXt_Operation<Node, Compare> op(cmp);
    r.first = op.find_position(tree_root, key, r.second);

    return r;
  }

   /** Split the tree according to a key

      @param[in] key for splitting
      @param[out] t1 tree with keys lesser than `key`
      @param[out] t2 tree with keys greater than `key`
      @return `true` if tree was split; that is if `key` is not in the
      tree. Otherwise, if `key` is in the tree, `false` is returned
  */
  bool split_key(const Key & key, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2) noexcept
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
  void
  split_key_dup(const Key & key, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2) noexcept
  {
    split_key_dup_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  /** Split the tree at the inorder position pos

      @param[in] pos inorder position where to split
      @param[out] t1 tree where the rank of keys \f$[0, pos)\f$ will be put
      @param[out] t2 tree where the rank of keys \f$[pos, n]\f$ will be put
  */
  void split_pos(size_t pos, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2)
  {
    split_pos_rec(tree_root, pos, t1.getRoot(), t2.getRoot());
  }

private:

  void join_dup(Node *& t1, Node * t2) noexcept
  {
    if (t2 == Node::NullPtr)
      return;

    Node * l = LLINK(t2), * r = RLINK(t2);
    t2->reset();
    t1 = insert_dup(t1, t2);
    join_dup(t1, l);
    join_dup(t1, r);
  }

  void join(Node *& t1, Node * t2, Node *& dup) noexcept
  {
    if (t2 == Node::NullPtr)
      return;

    Node * l = LLINK(t2), * r = RLINK(t2);
    t2->reset();
  ins:
    Node * ret = insert(t1, t2);
    if (ret == Node::NullPtr)
      {
	dup = insert_dup(dup, remove(t1, KEY(t2)));
	goto ins;
      }

    t1 = ret;
    join(t1, l, dup);
    join(t1, r, dup);
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
  void join(Gen_Treap_Rk & t, Gen_Treap_Rk & dup) noexcept
  {
    join(tree_root, t.tree_root, dup.tree_root);
    t.tree_root = Node::NullPtr;
  }

  /** Join `this` with `t` independently of the presence of duplicated keys

      `join(t)` produces a treap result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      @param[in] t ramdomized tree to join with `this`
      keys are inserted
   */
  void join_dup(Gen_Treap_Rk & t) noexcept
  {
    join_dup(tree_root, t.tree_root);
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
  void join_exclusive(Gen_Treap_Rk & t) noexcept
  {
    tree_root = join_exclusive(tree_root, t.tree_root);    
    t.tree_root = Node::NullPtr;
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Arboles
   */
  class Iterator
  {
  protected:
    
    mutable Gen_Treap_Rk * tree_ptr; 
    mutable Node *         curr;
    mutable int            curr_pos;

    static const int Pos_Not_Current     = -1;
    static const int Pos_Empty_Container = -2;
    static const int Pos_Not_Updated     = -3;

  private:

    bool is_container_empty() const noexcept
    {
      return COUNT(tree_ptr->getRoot()) == 0;
    }

    bool pos_updated() const noexcept
    {
      return curr_pos != Pos_Not_Updated;
    }

    bool curr_updated() const noexcept
    {
      return curr != nullptr;
    }

    bool is_updated() noexcept
    {
      return pos_updated() and curr_updated();
    }

    void update_pos() const noexcept
    {
      assert(curr != nullptr);

      curr_pos = BinTreeXt_Operation<Node, Compare>(tree_ptr->cmp).
	inorder_position(tree_ptr->getRoot(), KEY(curr), curr);
    }

    void update_curr() const noexcept
    {
      assert(curr_pos != Pos_Not_Updated);

      if (curr_pos == Pos_Empty_Container or curr_pos == Pos_Not_Current or
	  curr_pos == COUNT(tree_ptr->getRoot()))
	return;

      curr = Aleph::select(tree_ptr->getRoot(), curr_pos);
    }

  public:

    Iterator() noexcept
    : tree_ptr(nullptr), curr(nullptr), curr_pos(Pos_Not_Current)
    {
      /* empty */ 
    }

        /// Initialize an iterator on `__tree`
    Iterator(Gen_Treap_Rk & __tree) noexcept
      : tree_ptr(&__tree), curr(nullptr)
    {
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

    /// Initialize an iterator startin from node `__curr`
    Iterator(Gen_Treap_Rk & __tree, Node * __curr) noexcept
      : tree_ptr(&__tree), curr(__curr), curr_pos(Pos_Not_Updated)
    {
      // empty
    }

    /// Initialize an iterator starting from the iorder position `pos`
    Iterator(Gen_Treap_Rk & __tree, const size_t pos) noexcept
      : tree_ptr(&__tree), curr(nullptr), curr_pos(pos)
    {
      // empty
    }

    Iterator(const Iterator & itor) noexcept
      : tree_ptr(itor.tree_ptr), curr(itor.curr), curr_pos(itor.curr_pos)
    {
      // Empty
    }

    Iterator & operator = (const Iterator & itor) noexcept
    {
      if (this == &itor)
	return *this;

      tree_ptr = itor.tree_ptr;
      curr = itor.curr;
      curr_pos = itor.curr_pos;

      return *this;
    }

    /// Reset the iterator to the first position
    void reset_first() noexcept
    {
      curr = nullptr;
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

    /// Reset the iterator to the last position
    void reset_last() noexcept
    {
      curr = nullptr;
      curr_pos = (is_container_empty() ? Pos_Empty_Container : 
		  COUNT(tree_ptr->getRoot()) - 1);
    }

    /// Put the iterator in the end state
    void end() noexcept
    {
      reset_last();
      next();
    }

        /** Put the iterator to a node according to specific key

	    If `key` is not present, the the iterator is put on the node

	    @param[in] key to to guide te positiong
	    @throw domain_error si la clave no está contenida en el árbol.
	*/
    void reset_to_key(const Key & key) noexcept
    {
      std::pair<int, Node*> p = tree_ptr->find_position(key);
      curr_pos = p.first;
    }

        /** Put the current node to a specific node

	    @param[in] node toput as current
	    @note Obviously the node must belong to the tree
	*/
    void reset_to_node(Node * node) noexcept
    {
      curr = node;
      curr_pos = -2;
    }

    /// Put the current to the position pos
    void reset_to_pos(size_t pos) noexcept
    {
      curr = nullptr;
      curr_pos = pos;
    }

    /// Return the current node
    Node * get_current() const noexcept
    {
      if (not curr_updated())
	update_curr();

      return curr; 
    }

    /// \overload get_current()
    Node * get_curr() const noexcept
    {
      return get_current();
    }

        /** return the position of current node

	    @return current position 
	    está posicionado el iterador.
	    @throw underflow_error if the iterator has not current node
	    after a `prev()` operation
	    @throw overflow_error if the iterator has not current node
	    after a `next()` operation
	 */
    size_t get_current_position() const
      throw(std::exception, std::underflow_error, std::overflow_error)
    {
      if (not pos_updated())
	update_pos();

      if (curr_pos < -1 )
	throw std::range_error("TreapRk iterator has not current");
	
      if (curr_pos > COUNT(tree_ptr->getRoot() ) )
	throw std::range_error("TreapRk iterator has not current");

      return curr_pos;
    }

    /// \overload get_current_position()
    size_t get_pos() const { return get_current_position(); }

    /// Return `true` if iterator has current node
    bool has_current() const noexcept
    {
      if (not pos_updated())
	update_pos();

      return curr_pos >= 0 and curr_pos < COUNT(tree_ptr->getRoot());
    }

    /// \overload has_current()
    bool has_curr() const noexcept
    {
      return has_current();
    }

    /// Move the iterator one position backward. Throws `underflow_error`
    /// if there is no current
    void prev() throw(std::exception, std::underflow_error)
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      --curr_pos;
      curr = nullptr;
    }

    /// Move the iterator one position forward. Throws `overflow_error`
    /// if there is no current
    void next() throw(std::exception, std::overflow_error)
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      ++curr_pos;
      curr = nullptr;
    }

    /// Remove the current node and move the iterator one position forward
    Node * del()
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      if (not curr_updated())
	update_curr();

      Node * ret_val = tree_ptr->remove(KEY(curr) );
      
      curr = nullptr;

      return ret_val;
    }

    /// Return `true` if `this` is equal to `itor`
    bool operator == (const Iterator & itor) const noexcept
    {
      if (is_container_empty() and itor.is_container_empty())
	return true;

      if (pos_updated() and itor.pos_updated())
	return curr_pos == itor.curr_pos;

      if (curr_updated() and itor.curr_updated())
	return curr == itor.curr;

      if (not pos_updated())
	{
	  update_pos();
	  return curr_pos == itor.curr_pos;
	}

      itor.update_pos();

      return curr_pos == itor.curr_pos;
    }
  
        /// Return `true` if `this` is not equal to `itor`
    bool operator != (const Iterator & itor) const
    {
      return not (*this == itor);
    }

    bool verify(Gen_Treap_Rk * r) const noexcept
    {
      return tree_ptr->getRoot() == r->getRoot();
    }

    bool verify(const Iterator & it) const noexcept
    {
      return tree_ptr->getRoot() == it.tree_ptr->getRoot();
    }
  }; // end class Iterator
};

/** Extended treap (a special type of ramdomized binary search tree)
      which manages selection and splitting for inorder position.

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its
      operations. In addition, this extended tree uses and second
      integer for storing subtrees sizes.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). Since the priority is chosen
      just one time and the adjustements are done in a botton-top way
      (by contrast with the randomized which is top-bottom), the treap
      takes less time. 

      Although tgus approach trends to be faster than the randomized
      trees, takes in account that this treap is more space consuming
      because each node requires 2 additional integers to the data
      (priority and counter) in constrast with the radomized which only
      requieres one interger (the counter).

      For splitting and join of independent and large data sets the
      ramdomized option trends to be faster. The split is equivalent,
      but the join is definitively faster. The join of two trees of n
      and m keys respectively takes \f$O(n \lg m)\f$ with treaps, while
      it takes \f$O(\max(\lg n, \lg m))\f$ with radomized trees. 

      
      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/).
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */  
    template <typename Key, class Compare = Aleph::less<Key>>
struct Treap_Rk : public Gen_Treap_Rk<Treap_Rk_Node, Key, Compare> 
{
  using Base = Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>;
  using Base::Base;
};

/** Extended treap (a special type of ramdomized binary search tree)
      which manages selection and splitting for inorder position.

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its
      operations. In addition, this extended tree uses and second
      integer for storing subtrees sizes.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). Since the priority is chosen
      just one time and the adjustements are done in a botton-top way
      (by contrast with the randomized which is top-bottom), the treap
      takes less time. 

      Although tgus approach trends to be faster than the randomized
      trees, takes in account that this treap is more space consuming
      because each node requires 2 additional integers to the data
      (priority and counter) in constrast with the radomized which only
      requieres one interger (the counter).

      For splitting and join of independent and large data sets the
      ramdomized option trends to be faster. The split is equivalent,
      but the join is definitively faster. The join of two trees of n
      and m keys respectively takes \f$O(n \lg m)\f$ with treaps, while
      it takes \f$O(\max(\lg n, \lg m))\f$ with radomized trees. In
      addition,
      
      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/).
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */ 
    template <typename Key, class Compare = Aleph::less<Key>>
struct Treap_Rk_Vtl : public Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare> 
{
  using Base = Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph

# endif // TPL_TREAPRK_H
