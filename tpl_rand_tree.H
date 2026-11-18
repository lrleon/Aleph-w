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


/** @file tpl_rand_tree.H
 *  @brief Randomized binary search tree.
 *
 *  BST with randomized insertion that maintains expected
 *  O(log n) height without explicit balancing.
 *
 *  ## Features
 *  - Root insertion with probability 1/(n+1)
 *  - Expected O(log n) operations
 *  - Simpler than deterministic balancing
 *
 *  @see tpl_treap.H Alternative randomized BST
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RAND_TREE_H
# define TPL_RAND_TREE_H

# include <ctime>
# include <climits>
# include <type_traits>
# include <utility>
# include <ah-random.H>
# include <ahUtils.H>
# include <ahFunction.H>
# include <tpl_randNode.H>
# include <tpl_binTreeOps.H>
# include <ah-errors.H>
# include <ah-concepts.H>

using namespace Aleph;

namespace Aleph
{
  /** @brief Randomized binary search tree with rank support.

      This class implements a randomized binary search tree where random
      decisions during insertion and deletion ensure that the tree has the
      same expected properties as if it were built from a random permutation
      of keys, regardless of actual insertion order.

      @par Key Insight:
      At each insertion, a random "raffle" determines whether the new node
      becomes the root (and existing nodes are split around it) or is inserted
      recursively into a subtree. This ensures that every node has an equal
      probability of being at any position, producing expected O(log n) height.

      @par Ranked Operations:
      Unlike basic BSTs, randomized trees maintain subtree sizes (COUNT),
      enabling O(log n) access by rank (inorder position):
      - select(i): Get the i-th smallest element
      - position(key): Get the rank of a key
      - remove_pos(i): Remove the i-th element

      This allows treating the tree like a sorted dynamic array with O(log n)
      indexed access, insertion, and deletion.

      @par Split and Join:
      Randomized trees excel at split/join operations, which take O(log(max(n,m)))
      time. This is dramatically better than other balanced trees that require
      O(n log m) for joining two trees of sizes n and m.

      @tparam NodeType Node template (RandNode or RandNodeVtl).
      @tparam Key The type of keys stored in the tree.
      @tparam Compare Comparison functor for ordering keys.

      @par Complexity:
      - Search: O(log n) expected
      - Insert: O(log n) expected
      - Delete: O(log n) expected
      - Select (by rank): O(log n) expected
      - Position (get rank): O(log n) expected
      - Split: O(log n) expected
      - Join: O(log(max(n,m))) expected - **much better than other trees**
      - Space: O(n) + O(1) per node for COUNT

      @par Random Number Generator:
      Uses a Gsl_Compatible_Rng, the Mersenne Twister (mt19937) with the
      numbers of the one of GSL, which it used before: for a seed, the
      trees are the same. Seed can be set via set_seed() for reproducible
      behavior.

      @par Example:
      @code
      Rand_Tree<int> tree;

      // Insert nodes
      tree.insert(new Rand_Tree<int>::Node(42));
      tree.insert(new Rand_Tree<int>::Node(17));
      tree.insert(new Rand_Tree<int>::Node(99));
      tree.insert(new Rand_Tree<int>::Node(8));

      // Ranked access (treat as sorted array)
      auto smallest = tree.select(0);     // Get 0th element (8)
      auto median = tree.select(tree.size() / 2);  // Get median

      // Find position/rank of a key
      size_t pos = tree.position(42);  // Returns 2 (0-indexed)

      // Remove by position
      auto removed = tree.remove_pos(1);  // Remove 2nd smallest
      delete removed;

      // Efficient split at key
      Rand_Tree<int> left, right;
      tree.split_key(50, left, right);  // All <= 50 go to left, > 50 to right

      // Efficient join (O(log(max(n,m))))
      tree.join(left, right);
      @endcode

      @note This is a low-level implementation managing raw nodes.
            For automatic memory management, use DynSetRandTree.
      @note All operations maintain COUNT values for ranked operations.
      @note Best choice when frequent split/join operations are needed.

      @see Rand_Tree Convenient typedef for Rand_Tree<Key, Compare>.
      @see Treap Alternative randomized tree (often faster by constant factor).
      @see TreapRk Treap with rank support.
      @see DynSetRandTree High-level wrapper with automatic memory management.
      @ingroup Trees
  */
  template <template <typename> class NodeType, typename Key, class Compare>
    requires StrictWeakOrder<Compare, Key>
  class Gen_Rand_Tree
  {
  public:
    using Node = NodeType<Key>; /// The type of node

  private:
    Node *tree_root;
    Gsl_Compatible_Rng_Owner r;
    Compare cmp;

    // The insertions and the removal compare on the way down, the split of
    // an insertion at the root included, and link or count on the way
    // back, so a comparison that throws leaves the tree as it was
    Node * random_insert(Node *root, Node *p) noexcept(nothrow_compare_v<Compare, Key>)
    {
      if (root == Node::NullPtr)
        return p;

      // Prefetch children to reduce memory latency
      __builtin_prefetch(LLINK(root));
      __builtin_prefetch(RLINK(root));

      const long n = COUNT(root);
      if (const size_t rn = r->uniform_int(n + 1); rn == n) [[unlikely]]
          return BinTreeXt_Operation<Node, Compare>(cmp).insert_root(root, p);

      Node *result;
      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      if (cmp(pk, rk)) // KEY(p) < KEY(root)
        {
          result = random_insert(LLINK(root), p);
          if (result != Node::NullPtr) // p was inserted
            {
              LLINK(root) = result;
              ++COUNT(root);
              return root;
            }
        }
      else if (cmp(rk, pk)) // KEY(p) > KEY(root)
        {
          result = random_insert(RLINK(root), p);
          if (result != Node::NullPtr) // p was inserted
            {
              RLINK(root) = result;
              ++COUNT(root);
              return root;
            }
        }

      return Node::NullPtr; // duplicated key ==> no insertion
    }

    Node * random_insert_dup(Node *root, Node *p) noexcept(nothrow_compare_v<Compare, Key>)
    {
      if (root == Node::NullPtr)
        return p;

      // Prefetch children to reduce memory latency
      __builtin_prefetch(LLINK(root));
      __builtin_prefetch(RLINK(root));

      const long n = COUNT(root);
      if (const size_t rn = r->uniform_int(n + 1); rn == n) [[unlikely]]
          return BinTreeXt_Operation<Node, Compare>(cmp).insert_dup_root(root, p);

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      if (cmp(pk, rk)) // KEY(p) < KEY(root)
        LLINK(root) = random_insert_dup(LLINK(root), p);
      else
        RLINK(root) = random_insert_dup(RLINK(root), p);

      ++COUNT(root);

      return root;
    }

    Node * random_search_or_insert(Node *& root, Node *p)
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      if (root == Node::NullPtr)
        return root = p;

      // Prefetch children to reduce memory latency
      if (LLINK(root) != Node::NullPtr)
        __builtin_prefetch(LLINK(root));
      if (RLINK(root) != Node::NullPtr)
        __builtin_prefetch(RLINK(root));

      const long n = COUNT(root);
      if (const size_t rn = r->uniform_int(n + 1); rn == n) [[unlikely]]
        {
          if (BinTreeXt_Operation<Node, Compare>(cmp).insert_root(root, p) != Node::NullPtr)
            return p;

          return searchInBinTree<Node, Compare>(root, KEY(p), cmp);
        }

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      if (cmp(pk, rk))
        {
          Node *ret = random_search_or_insert(LLINK(root), p);
          if (ret == p)
            ++COUNT(root);

          return ret;
        }
      if (cmp(rk, pk))
        {
          Node *ret = random_search_or_insert(RLINK(root), p);
          if (ret == p)
            ++COUNT(root);

          return ret;
        }

      return root; // key == root: found existing
    }

  public:
    Gen_Rand_Tree(const Gen_Rand_Tree &) = delete;

    Gen_Rand_Tree &operator =(const Gen_Rand_Tree &) = delete;

  private:
    // The seed of the generator, modulo 2^32 - 1, as it was taken for GSL
    static unsigned long generator_seed(const unsigned long seed) noexcept
    {
      return seed % static_cast<unsigned long>(Gsl_Compatible_Rng::max());
    }

  public:
    /// return the comparison criteria
    Compare &key_comp() noexcept { return cmp; }

    /// \overload key_comp() const version
    const Compare &key_comp() const noexcept { return cmp; }

    /// \overload key_comp()
    Compare &get_compare() noexcept { return key_comp(); }

    /// \overload get_compare() const version
    const Compare &get_compare() const noexcept { return cmp; }

    /// The random number generator, which draws the positions of the nodes
    [[nodiscard]] Gsl_Compatible_Rng & rng() noexcept { return *r; }

# if ALEPH_WITH_GSL
    /** @brief A `gsl_rng` whose numbers come from rng().

        @deprecated The generator is no longer of GSL; use rng(). It goes
        in 8.0.0.
        @return The `gsl_rng`, which the tree frees.
        @throw std::bad_alloc If there is not enough memory, the first time.
    */
    [[deprecated("the generator is no longer of GSL: use rng(); it goes in 8.0.0")]]
    gsl_rng * gsl_rng_object() { return r.gsl(); }
# endif

    /// Set the random number generator seed, modulo 2^32 - 1
    void set_seed(const unsigned long seed) noexcept
    {
      r->seed(generator_seed(seed));
    }

    /// Initialize a random tree with random `seed` and comparison criteria
    /// `__cmp`
    Gen_Rand_Tree(const unsigned int seed, const Compare & __cmp = Compare())
      : tree_root(Node::NullPtr), r(generator_seed(seed)), cmp(__cmp)
    {
      // empty
    }

    /// Initialize a random tree with random seed from system time and
    /// comparison criteria `cmp`
    Gen_Rand_Tree(const Compare & cmp = Compare()) noexcept
      : Gen_Rand_Tree(time(nullptr), cmp)
    { /* empty */
    }

    /** @brief Swap in constant time all the nodes of `this` with `tree`.
        @param[in,out] tree the tree to exchange with.
        @throw what swapping the comparison criteria throws. They are
               exchanged first, so that, if that swap throws, nothing else is
               exchanged, and both trees are left as they were if it left its
               arguments as they were.
    */
    void swap(Gen_Rand_Tree & tree) noexcept(std::is_nothrow_swappable_v<Compare>)
    {
      // the swap that the specification evaluates: that of Compare found by
      // its arguments, else std::swap(); first, since it may throw
      using std::swap;
      swap(cmp, tree.cmp);
      std::swap(tree_root, tree.tree_root);
      r.swap(tree.r);
    }

    /** @brief Take the nodes, the generator and the comparison criterion of
        another tree, which is left empty, with a generator of its own
        seeded with the time.

        The previous move left the source without a generator: its next
        insertion in a tree with nodes drew from a null generator.

        @param[in,out] other the tree to move from.
        @throw bad_alloc if there is not enough memory for the generator
               left to `other`, or what the copy of the comparison criterion
               throws; then `other` is unchanged.
        @note It allocates the generator left to `other`.
    */
    Gen_Rand_Tree(Gen_Rand_Tree && other)
      : Gen_Rand_Tree(time(nullptr), other.cmp)
    {
      // the comparison criterion is already a copy of that of other, which
      // keeps its own: exchanging it again would need its move assignment
      std::swap(tree_root, other.tree_root);
      r.swap(other.r);
    }

    /** @brief Exchange this tree with another, which is left with the
        previous nodes, generator and comparison criterion of this one.

        The previous assignment took the generator of `other`, which then
        drew from a null one, and freed the nodes of this tree, which the
        tree does not own: they go now to `other`, as in swap().

        @param[in,out] other the tree to move from.
        @return a reference to this tree.
        @throw what swapping the comparison criteria throws; then nothing is
               exchanged.
    */
    Gen_Rand_Tree &operator =(Gen_Rand_Tree && other) noexcept(noexcept(swap(other)))
    {
      swap(other);
      return *this;
    }

    virtual ~Gen_Rand_Tree() noexcept
    {
      // Note: nodes are NOT freed here. DynSetTree::empty() handles node deletion.
      // The underlying tree only manages the tree structure, not node memory.
    }

    /** @brief Insert a node in the tree

        @param[in] p pointer to the node to insert
        @return if `p->get_key()` is not in the tree, then the pointer
        `p` is returned (it was inserted); otherwise, `nullptr` is returned
        @throws anything that the comparison throws; the tree changes only
        after the last comparison, so it is left as it was, without `p`.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    Node * insert(Node *p) noexcept(nothrow_compare_v<Compare, Key>)
    {
      assert(p != Node::NullPtr);
      assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
      assert(COUNT(p) == 1);

      Node *result = random_insert(tree_root, p);
      if (result == Node::NullPtr)
        return nullptr;

      tree_root = result; // the new root, which is p only now and then

      return p;
    }

    /** @brief Insert a node in the tree.

        This method does not fail. It always inserts.

        @param[in] p pointer to the node to insert
        @return the `p` pointer
        @throws anything that the comparison throws; the tree changes only
        after the last comparison, so it is left as it was, without `p`.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    Node * insert_dup(Node *p) noexcept(nothrow_compare_v<Compare, Key>)
    {
      assert(p != Node::NullPtr);
      assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
      assert(COUNT(p) == 1);

      tree_root = random_insert_dup(tree_root, p);

      return p;
    }

  private:
    Node * random_join_exclusive(Node *tl, Node *tr) noexcept
    {
      if (tl == Node::NullPtr)
        return tr;

      if (tr == Node::NullPtr)
        return tl;

      const size_t & m = COUNT(tl);
      const size_t & n = COUNT(tr);
      if (const size_t rn = 1 + r->uniform_int(m + n); rn <= m)
        { // left subtree wins
          COUNT(tl) += COUNT(tr);
          RLINK(tl) = random_join_exclusive(RLINK(tl), tr);
          return tl;
        }
      COUNT(tr) += COUNT(tl);
      LLINK(tr) = random_join_exclusive(tl, LLINK(tr));
      return tr;
    }

    Node * random_remove(Node *& root, const Key & key)
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      if (root == Node::NullPtr)
        return Node::NullPtr;

      Node *ret_val;
      const Key & rk = KEY(root);
      if (cmp(key, rk))
        {
          ret_val = random_remove(LLINK(root), key);
          if (ret_val != Node::NullPtr)
            --COUNT(root);

          return ret_val;
        }
      if (cmp(rk, key))
        {
          ret_val = random_remove(RLINK(root), key);
          if (ret_val != Node::NullPtr)
            --COUNT(root);

          return ret_val;
        }

      // key was found
      ret_val = root;
      root = random_join_exclusive(LLINK(root), RLINK(root));
      ret_val->reset();

      return ret_val;
    }

  public:
    /** @brief Remove a key from the tree

        @param[in] key to remove
        @return a valid pointer to the removed node if `key` was found
        in the tree, `nullptr` otherwise
        @throws anything that the comparison throws; the tree changes only
        after the last comparison, so it is left as it was.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    Node * remove(const Key & key) noexcept(nothrow_compare_v<Compare, Key>)
    {
      Node *ret_val = random_remove(tree_root, key);

      return ret_val != Node::NullPtr ? ret_val : nullptr;
    }

    /** @brief Search a key.

        @param[in] key to be searched
        @return a pointer to the containing key if this was found;
        otherwise `nullptr`
        @throws anything that the comparison throws; the search does not
        modify the tree, so it is left as it was.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    Node * search(const Key & key) const noexcept(nothrow_compare_v<Compare, Key>)
    {
      Node *retVal = searchInBinTree<Node, Compare>(tree_root, key, cmp);
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
        @throws anything that the comparison throws; the tree changes only
        after the last comparison, so it is left as it was, without `p`.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    Node * search_or_insert(Node *p) noexcept(nothrow_compare_v<Compare, Key>)
    {
      assert(p != Node::NullPtr);
      assert(COUNT(p) == 1);

      return random_search_or_insert(tree_root, p);
    }

    /// Return `true` if this is a consistent randomized tree
    [[nodiscard]] bool verify() const
    {
      return check_rank_tree(tree_root) and check_bst(tree_root, cmp);
    }

    /// Return the tree's root
    Node *&getRoot() noexcept { return tree_root; }

    /** Return the i-th node in inorder sense


        @param[in] i inorder position to be located
        @return a valid pointer to the i-th node inorder sense
        @throw out_of_range if i is greater or equal that the number of nodes
    */
    Node * select(const size_t i) const
    {
      return Aleph::select(tree_root, i);
    }

    /// Return the number of nodes that have the tree
    [[nodiscard]] size_t size() const noexcept { return COUNT(tree_root); }

    /// Return true if the tree is empty
    [[nodiscard]] bool is_empty() const noexcept { return tree_root == Node::NullPtr; }

    /** @brief Compute the inorder position of a key

        @param[in] key to be searched
        @return a tuple with the position of key and a pointer
        to the node containing it. If `key` is not in the tree, then
        first the first value is -1.
        @throws anything that the comparison throws; the search does not
        modify the tree, so it is left as it was.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.

        @ingroup Trees
    */
    std::pair<long, Node *> position(const Key & key) const
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      std::pair<long, Node *> ret_val;
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

        Otherwise, the tuple returns the position that would have `key`
        if this was in the tree and the parent node that the key should be.
        At this regard, there are three cases:
        1. if `key` is lesser than the minimum key of tree, then first is
        -1 and the node is one with the smallest key.

        2. If `key` is greater than the maximum key in the tree, then
        first is the number of keys and the node is one with the
        maximum key in the tree.

        3. For any other case, first is the inorder position that
        would have `key` if this was in the tree and second is the
        node whose key is immediately greater than `key`.

        @param[in] key to be searched
        @return a pair with the inorder position and related node
        @throws anything that the comparison throws; the search does not
        modify the tree, so it is left as it was.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    std::pair<long, Node *> find_position(const Key & key) const
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      std::pair<long, Node *> r(-2, nullptr);

      r.first = BinTreeXt_Operation<Node, Compare>(cmp).
          find_position(tree_root, key, r.second);

      return r;
    }

  private:
    Node * remove_pos(Node *& root, const size_t pos) noexcept
    {
      if (pos == COUNT(LLINK(root)))
        {
          Node *ret_val = root;
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
    Node * remove_pos(const size_t i)
    {
      ah_out_of_range_error_if(i >= COUNT(tree_root)) << "infix position out of range";

      return remove_pos(tree_root, i);
    }

    /** @brief Split the tree according to a key

        It splits with the comparison criteria of the tree.

        @param[in] key for splitting
        @param[out] t1 tree with keys lesser than `key`
        @param[out] t2 tree with keys greater than `key`
        @return `true` if tree was split; that is if `key` is not in the
        tree. Otherwise, if `key` is in the tree, `false` is returned
        @throws anything that the comparison throws; every comparison comes
        before the first change, so the three trees are left as they were.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    bool split_key(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      return split_key_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot(), cmp);
    }

    /** @brief Split the tree according to a key that could be in the tree

        `split_dup(key, t1, t2)` splits the tree according to `key` in
        two trees `t1` which contains the key lesser than `key` and
        `t2` which contains the keys greater or equal than `key`. It
        splits with the comparison criteria of the tree.

        @param[in] key for splitting
        @param[out] t1 resulting tree with the keys lesser than `key`
        @param[out] t2 resulting tree with the keys greater or equal than `key`
        @throws anything that the comparison throws; every comparison comes
        before the first change, so the three trees are left as they were.
        @note It is `noexcept` only if the comparison cannot throw, so that
        what it throws reaches the caller.
    */
    void split_key_dup(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
      noexcept(nothrow_compare_v<Compare, Key>)
    {
      split_key_dup_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot(), cmp);
    }

    /**  Split a extended binary tree according to a position

         `split_pos(pos, t1, t2)` splits the tree in two trees `t1` and
         `t2` according to a position `pos`. `t1` contains the keys from 0
         to `pos - 1` inorder sense and `t2` the keys from `pos` to n -
         1. After completion `this` becames empty.

         It is not `noexcept`: a position past the end throws, and inside
         `noexcept` that would end the program.

         @param[in] pos position for splitting, at most size()
         @param[out] t1 tree where the rank of keys \f$[0, pos)\f$ will be put
         @param[out] t2 tree where the rank of keys \f$[pos, n)\f$ will be put
         @throw std::out_of_range if `pos > size()`; then nothing changes
    */
    void split_pos(size_t pos, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
    {
      split_pos_rec(tree_root, pos, t1.getRoot(), t2.getRoot());
    }

  private:
    Node * random_join(Node *t1, Node *t2, Node *& dup)
    {
      if (t1 == Node::NullPtr)
        return t2;

      if (t2 == Node::NullPtr)
        return t1;

      Node *ret = Node::NullPtr;
      const size_t m = COUNT(t1);
      const size_t n = COUNT(t2);
      if (const size_t rn = 1 + r->uniform_int(m + n); rn <= m)
        { // root of t1 wins
          Node *l = LLINK(t1), *r = RLINK(t1);
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
        { // root of t2 wins
          Node *l = LLINK(t2), *r = RLINK(t2);
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

    Node * random_join(Node *t1, Node *t2)
    {
      if (t1 == Node::NullPtr)
        return t2;

      if (t2 == Node::NullPtr)
        return t1;

      Node *ret = Node::NullPtr;
      const size_t & m = COUNT(t1);
      const size_t & n = COUNT(t2);
      const size_t total = m + n;
      if (const size_t rn = 1 + r->uniform_int(total); rn <= m)
        { // root of t1 wins
          Node *l = LLINK(t1), *r = RLINK(t1);
          t1->reset();
          ret = insert_dup_root_xt(t2, t1, cmp);
          LLINK(ret) = random_join(LLINK(ret), l);
          RLINK(ret) = random_join(RLINK(ret), r);
        }
      else
        { // root of t2 wins
          Node *l = LLINK(t2), *r = RLINK(t2);
          t2->reset();
          ret = insert_dup_root_xt(t1, t2, cmp);
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

        @param[in] t randomized tree to join with `this`
        @param[out] dup randomized tree where nodes containing duplicated
        keys are inserted
        @pre The comparison does not throw; if it does, the program ends
        (`std::terminate()`).
        @note It stays `noexcept` because it moves the nodes one by one: a
        comparison that threw in the middle would leave nodes out of every
        tree.
    */
    void join(Gen_Rand_Tree & t, Gen_Rand_Tree & dup) noexcept
    {
      tree_root = random_join(tree_root, t.tree_root, dup.tree_root);
      t.tree_root = Node::NullPtr;
    }

    /** @brief Join `this` with `t` independently of the presence of
        duplicated keys

        `join(t)` produces a random tree result of join of `this` and
        `t`. The resulting tree is stored in `this`.

        @param[in] t randomized tree to join with `this`
        keys are inserted
        @pre The comparison does not throw; if it does, the program ends
        (`std::terminate()`).
        @note It stays `noexcept` because it moves the nodes one by one: a
        comparison that threw in the middle would leave nodes out of every
        tree.
    */
    void join_dup(Gen_Rand_Tree & t) noexcept
    {
      tree_root = random_join(tree_root, t.tree_root);
      t.tree_root = Node::NullPtr;
    }

    /** Join exclusive of `this` with `t`

        Exclusive means that all the keys of `this` are lesser than all
        the keys of `t`. This knowledge allows a more efficient way for
        joining that when the keys ranks are overlapped. However, use very
        carefully because the algorithm does not perform any check and the
        result would be incorrect.

        @param[in] t randomized tree to exclusively join with `this`
        keys are inserted
    */
    void join_exclusive(Gen_Rand_Tree & t) noexcept
    {
      tree_root = random_join_exclusive(tree_root, t.tree_root);
      t.tree_root = Node::NullPtr;
    }

    /** Iterator on nodes of the tree.

        The visit order is inorder.

        @ingroup Trees
    */
    struct Iterator : public BinNodeInfixIterator<Node>
    {
      Iterator(Gen_Rand_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    };
  };

  /** Randomized binary search tree.

      This class implements a randomized binary search tree. It is shown
      that this type of tree always is equivalent to a classic binary
      search tree built from a random insertion sequence. Consequently, all
      the operations of this tree are \f$O(\lg n)\f$ expected case,
      independently of insertion order and of removals are interleaved
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

      The class uses a Gsl_Compatible_Rng, the Mersenne twister with the
      numbers of the one of [GSL - GNU Scientific
      Library](\https://www.gnu.org/software/gsl/), which it used before.
      By default, the seed is taken from system time.

      @see Treap Treap_Vtl Treap_Rk

      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  struct Rand_Tree : public Gen_Rand_Tree<RandNode, Key, Compare>
  {
    using Base = Gen_Rand_Tree<RandNode, Key, Compare>;
    using Base::Base;
  };


  /** Randomized binary search tree.

      This class implements a randomized binary search tree. It is
      shown that this type of tree always is equivalent to a classic
      binary search tree built from a random insertion
      sequence. Consequently, all the operations of this tree are
      \f$O(\lg n)\f$ expected case, independently of insertion order
      and of removals are interleaved with insertions.

      In addition, these trees support `select()` and `position()`
      operations. That is, their keys can be accessed according to their
      inorder position and logarithmic time. This allows to treat the
      tree as if it was an array.

      This tree type is unbeatable when there are splits and
      especially joins operations on very large data sets. Other tree
      types perform the join of independent data sets in \f$O(n \lg
      m)\f$, where \f$n\f$ and \f$m\f$ are the size of two data sets,
      while the randomized approach takes \f$O(\max(\lg n, \lg m))\f$.

      The class uses a Gsl_Compatible_Rng, the Mersenne twister with the
      numbers of the one of [GSL - GNU Scientific
      Library](\https://www.gnu.org/software/gsl/), which it used before.
      By default, the seed is taken from system time.

      @see Treap Treap_Vtl Treap_Rk

      @ingroup Trees
  */
  template <typename Key, class Compare = Aleph::less<Key>>
    requires StrictWeakOrder<Compare, Key>
  struct Rand_Tree_Vtl : public Gen_Rand_Tree<RandNodeVtl, Key, Compare>
  {
    using Base = Gen_Rand_Tree<RandNodeVtl, Key, Compare>;
    using Base::Base;
  };
} // end namespace Aleph

# endif // TPL_RAND_TREE_H
