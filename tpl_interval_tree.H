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


/** @file tpl_interval_tree.H
 *  @brief Interval tree: augmented BST for overlap/stabbing queries.
 *
 *  An interval tree stores closed intervals [low, high] and efficiently
 *  answers overlap and stabbing queries in O(log n) expected time.
 *
 *  The implementation is an augmented treap where each node stores the
 *  maximum endpoint in its subtree. This allows pruning entire subtrees
 *  during overlap searches.
 *
 *  ## Features
 *  - Insert/remove intervals: O(log n) expected
 *  - Overlap search (find one): O(log n) expected
 *  - Find all overlaps: O(k log n) where k = number of results
 *  - Stabbing query: O(k log n)
 *
 *  ## Applications
 *  - Sweep-line segment intersection
 *  - Temporal scheduling conflict detection
 *  - Windowing queries in computational geometry
 *
 *  @see tpl_treap.H Base treap
 *  @ingroup Trees
 *  @author Leandro Rabindranath Leon
 */

# ifndef TPL_INTERVAL_TREE_H
# define TPL_INTERVAL_TREE_H

# include <gsl/gsl_rng.h>

# include <cassert>
# include <ctime>

# include <ahFunction.H>
# include <tpl_binNodeUtils.H>
# include <treapNode.H>
# include <tpl_binTreeOps.H>
# include <ah-errors.H>
# include <ah-concepts.H>
# include <ah-dry.H>
# include <ahDry.H>
# include <htlist.H>
# include <utility>

namespace Aleph
{
  /** @brief Closed interval [low, high].
   *
   *  Represents a closed interval with endpoints of type `T`.
   *  All comparisons use a functor so custom types work.
   *
   *  @tparam T Endpoint type (must be totally ordered).
   *  @ingroup Trees
   */
  template <typename T>
  struct Interval
  {
    T low; ///< Lower endpoint
    T high; ///< Upper endpoint

    /** @brief Default constructor.
     *  @throws none
     */
    Interval() = default;

    /** @brief Construct interval [lo, hi].
     *  @param[in] lo Lower endpoint.
     *  @param[in] hi Upper endpoint.
     *  @throws none
     */
    Interval(const T & lo, const T & hi)
      noexcept(std::is_nothrow_copy_constructible_v<T>)
      : low(lo), high(hi) {}

    /** @brief Construct interval [lo, hi] (move version).
     *  @param[in] lo Lower endpoint.
     *  @param[in] hi Upper endpoint.
     *  @throws none
     */
    Interval(T && lo, T && hi)
      noexcept(std::is_nothrow_move_constructible_v<T>)
      : low(std::move(lo)), high(std::move(hi)) {}

    /** @brief Construct a point interval [p, p].
     *  @param[in] p The point value.
     *  @return An interval where low == high == p.
     *  @throws none
     */
    static Interval point(const T & p)
      noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
      return {p, p};
    }

    /** @brief Return true if this interval overlaps with `other`.
     *  @tparam Compare Comparison functor type.
     *  @param[in] other The other interval.
     *  @param[in] cmp Comparison functor for endpoints.
     *  @return true if intervals overlap, false otherwise.
     *  @throws none
     */
    template <class Compare = Aleph::less<T>>
    [[nodiscard]] bool overlaps(const Interval & other,
                                const Compare & cmp = Compare()) const
      noexcept(std::is_nothrow_invocable_v<const Compare &, const T &, const T &>)
    {
      // [a,b] overlaps [c,d] iff a <= d and c <= b
      return not cmp(other.high, low) and not cmp(high, other.low);
    }

    /** @brief Return true if this interval contains the point `p`.
     *  @tparam Compare Comparison functor type.
     *  @param[in] p The query point.
     *  @param[in] cmp Comparison functor for endpoints.
     *  @return true if p is in [low, high], false otherwise.
     *  @throws none
     */
    template <class Compare = Aleph::less<T>>
    [[nodiscard]] bool contains(const T & p,
                                const Compare & cmp = Compare()) const
      noexcept(std::is_nothrow_invocable_v<const Compare &, const T &, const T &>)
    {
      return not cmp(p, low) and not cmp(high, p);
    }

    /** @brief Return true if low <= high.
     *  @tparam Compare Comparison functor type.
     *  @param[in] cmp Comparison functor for endpoints.
     *  @return true if low <= high according to cmp.
     *  @throws none
     */
    template <class Compare = Aleph::less<T>>
    [[nodiscard]] bool is_valid(const Compare & cmp = Compare()) const
      noexcept(std::is_nothrow_invocable_v<const Compare &, const T &, const T &>)
    {
      return not cmp(high, low);
    }

    /** @brief Equality operator.
     *  @param[in] other Interval to compare with.
     *  @return true if endpoints are equal.
     *  @throws none
     */
    bool operator ==(const Interval & other) const
      noexcept(noexcept(std::declval<const T &>() == std::declval<const T &>()))
    {
      return low == other.low and high == other.high;
    }

    /** @brief Inequality operator.
     *  @param[in] other Interval to compare with.
     *  @return true if endpoints differ.
     *  @throws none
     */
    bool operator !=(const Interval & other) const
      noexcept(noexcept(*this == other))
    {
      return not (*this == other);
    }
  };

  /** @brief BST comparator for intervals: order by (low, then high).
   *
   *  @tparam T Endpoint type.
   *  @tparam Compare Comparison functor for endpoints.
   *  @ingroup Trees
   */
  template <typename T, class Compare = Aleph::less<T>>
  struct Interval_Less
  {
    Compare cmp;

    /** @brief Constructor.
     *  @param[in] c Endpoint comparison functor.
     *  @throws none
     */
    Interval_Less(const Compare & c = Compare())
      noexcept(std::is_nothrow_copy_constructible_v<Compare>)
      : cmp(c) {}

    /** @brief Comparison operator.
     *  @param[in] a First interval.
     *  @param[in] b Second interval.
     *  @return true if a < b (lexicographical order on low, then high).
     *  @throws none
     */
    bool operator ()(const Interval<T> & a, const Interval<T> & b) const
      noexcept(std::is_nothrow_invocable_v<const Compare &, const T &, const T &>)
    {
      if (cmp(a.low, b.low))
        return true;
      if (cmp(b.low, a.low))
        return false;
      return cmp(a.high, b.high);
    }
  };

  /** @brief Trait to extract endpoint type from Interval<T>.
   *  @ingroup Trees
   */
  template <typename Key>
  struct interval_endpoint;

  template <typename T>
  struct interval_endpoint<Interval<T>>
  {
    using type = T;
  };

  template <typename Key>
  using interval_endpoint_t = typename interval_endpoint<Key>::type;

  /** @brief Data portion of an interval tree node.
   *
   *  Stores the random priority (for treap heap property) and the
   *  maximum endpoint in the subtree (for interval query pruning).
   *
   *  @tparam T Endpoint type.
   *  @ingroup Trees
   */
  template <typename T>
  class Interval_Tree_Node_Data
  {
    unsigned long priority;
    T max_endpoint;

  public:
    Interval_Tree_Node_Data() noexcept
      : priority(Max_Priority), max_endpoint() {}

    Interval_Tree_Node_Data(SentinelCtor) noexcept
      : priority(Max_Priority), max_endpoint() {}

    unsigned long &getPriority() noexcept { return priority; }

    T &getMaxEndpoint() noexcept { return max_endpoint; }
    const T &getMaxEndpoint() const noexcept { return max_endpoint; }

    static void reset() noexcept
    { /* empty */
    }
  };

  /** @brief Interval tree node with sentinel support.
   *
   *  Cannot use DECLARE_BINNODE_SENTINEL because our Control_Data is
   *  templated on T (extracted from Key = Interval<T>).  We replicate
   *  the macro's structure manually.
   *
   *  @tparam Key must be Interval<T> for some type T.
   *  @ingroup Trees
   */
  template <typename Key>
  class Interval_Tree_Node
      : public Interval_Tree_Node_Data<interval_endpoint_t<Key>>
  {
  public:
    using Endpoint = interval_endpoint_t<Key>;
    using Data = Interval_Tree_Node_Data<Endpoint>;

    static constexpr size_t MaxHeight = 80;
    static Interval_Tree_Node *const NullPtr;

    using key_type = Key;
    using Key_Type = Key;

  private:
    Key key = Key();
    Interval_Tree_Node *lLink;
    Interval_Tree_Node *rLink;

    // sentinel node
    static Interval_Tree_Node sentinel_node;

  public:
    Key &get_key() noexcept { return key; }
    const Key &get_key() const noexcept { return key; }

    Interval_Tree_Node *&getL() noexcept { return lLink; }
    Interval_Tree_Node *&getR() noexcept { return rLink; }
    const Interval_Tree_Node * getL() const noexcept { return lLink; }
    const Interval_Tree_Node * getR() const noexcept { return rLink; }

    Interval_Tree_Node(const Key & k)
      : key(k), lLink(NullPtr), rLink(NullPtr)
    {
      this->getMaxEndpoint() = k.high;
    }

    Interval_Tree_Node(Key && k) noexcept
      : key(std::move(k)), lLink(NullPtr), rLink(NullPtr)
    {
      this->getMaxEndpoint() = key.high;
    }

    Interval_Tree_Node(const Interval_Tree_Node & node)
      : Data(node),
        key(node.key), lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    Interval_Tree_Node(Interval_Tree_Node && node) noexcept
      : Data(std::move(static_cast<Data &>(node))),
        key(std::move(node.key)), lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    Interval_Tree_Node()
      : lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    void reset() noexcept
    {
      Data::reset();
      this->getMaxEndpoint() = key.high;
      rLink = lLink = NullPtr;
    }

  private:
    // sentinel constructor
    Interval_Tree_Node(SentinelCtor)
      : Data(sentinelCtor), lLink(nullptr), rLink(nullptr)
    {
      /* empty - links set to nullptr will be fixed when NullPtr is initialized */
    }
  };

  template <typename Key>
  Interval_Tree_Node<Key>
  Interval_Tree_Node<Key>::sentinel_node(sentinelCtor);

  template <typename Key>
  Interval_Tree_Node<Key> *const
      Interval_Tree_Node<Key>::NullPtr = &Interval_Tree_Node<Key>::sentinel_node;

  /** @brief Interval tree node with virtual destructor.
   *  @tparam Key must be Interval<T>.
   *  @ingroup Trees
   */
  template <typename Key>
  class Interval_Tree_NodeVtl
      : public Interval_Tree_Node_Data<interval_endpoint_t<Key>>
  {
  public:
    using Endpoint = interval_endpoint_t<Key>;
    using Data = Interval_Tree_Node_Data<Endpoint>;

    static constexpr size_t MaxHeight = 80;
    static Interval_Tree_NodeVtl *const NullPtr;

    using key_type = Key;
    using Key_Type = Key;

  private:
    Key key = Key();
    Interval_Tree_NodeVtl *lLink;
    Interval_Tree_NodeVtl *rLink;

    static Interval_Tree_NodeVtl sentinel_node;

  public:
    Key &get_key() noexcept { return key; }
    const Key &get_key() const noexcept { return key; }

    Interval_Tree_NodeVtl *&getL() noexcept { return lLink; }
    Interval_Tree_NodeVtl *&getR() noexcept { return rLink; }
    const Interval_Tree_NodeVtl * getL() const noexcept { return lLink; }
    const Interval_Tree_NodeVtl * getR() const noexcept { return rLink; }

    Interval_Tree_NodeVtl(const Key & k)
      : key(k), lLink(NullPtr), rLink(NullPtr)
    {
      this->getMaxEndpoint() = k.high;
    }

    Interval_Tree_NodeVtl(Key && k) noexcept
      : key(std::move(k)), lLink(NullPtr), rLink(NullPtr)
    {
      this->getMaxEndpoint() = key.high;
    }

    Interval_Tree_NodeVtl(const Interval_Tree_NodeVtl & node)
      : Data(node),
        key(node.key), lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    Interval_Tree_NodeVtl(Interval_Tree_NodeVtl && node) noexcept
      : Data(std::move(static_cast<Data &>(node))),
        key(std::move(node.key)), lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    Interval_Tree_NodeVtl()
      : lLink(NullPtr), rLink(NullPtr)
    {
      /* empty */
    }

    virtual ~Interval_Tree_NodeVtl() = default;

    void reset() noexcept
    {
      Data::reset();
      this->getMaxEndpoint() = key.high;
      rLink = lLink = NullPtr;
    }

  private:
    Interval_Tree_NodeVtl(SentinelCtor)
      : Data(sentinelCtor), lLink(nullptr), rLink(nullptr)
    {
      /* empty */
    }
  };

  template <typename Key>
  Interval_Tree_NodeVtl<Key>
  Interval_Tree_NodeVtl<Key>::sentinel_node(sentinelCtor);

  template <typename Key>
  Interval_Tree_NodeVtl<Key> *const
      Interval_Tree_NodeVtl<Key>::NullPtr = &Interval_Tree_NodeVtl<Key>::sentinel_node;

  /// Access max_endpoint of an interval tree node
  template <class Node>
  inline auto &MAX_EP(Node *p) noexcept { return p->getMaxEndpoint(); }

  template <class Node>
  inline const auto &MAX_EP(const Node *p) noexcept
  {
    return p->getMaxEndpoint();
  }

  /** @brief Augmented treap storing intervals with overlap/stabbing queries.
   *
   *  Gen_Interval_Tree is a standalone augmented treap that stores
   *  `Interval<T>` values and maintains a `max_endpoint` field in each
   *  node for efficient interval query pruning.
   *
   *  This is a low-level class managing raw node pointers.  For automatic
   *  memory management, use `DynIntervalTree`.
   *
   *  @tparam NodeType Node template (Interval_Tree_Node or Interval_Tree_NodeVtl).
   *  @tparam T Endpoint type.
   *  @tparam Compare Comparison functor for endpoints.
   *
   *  @par Complexity:
   *  - Insert: O(log n) expected
   *  - Remove: O(log n) expected
   *  - overlap_search: O(log n) expected
   *  - all_overlaps: O(k log n) where k = number of results
   *  - stab: O(k log n)
   *
   *  @ingroup Trees
   */
  template <template <typename> class NodeType,
            typename T,
            class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  class Gen_Interval_Tree
  {
  public:
    using Key = Interval<T>;
    using IntervalCmp = Interval_Less<T, Compare>;
    using Node = NodeType<Key>;

  private:
    Node head;
    Node *head_ptr;
    Node *& tree_root;
    gsl_rng *r;
    Compare cmp;
    size_t num_nodes;

    void init(const unsigned int seed)
    {
      PRIO(head_ptr) = Min_Priority;
      r = gsl_rng_alloc(gsl_rng_mt19937);
      ah_bad_alloc_if(r == nullptr);
      gsl_rng_set(r, seed % gsl_rng_max(r));
    }

    /// Recalculate max_endpoint from node's own high and children
    static void update_max(Node *p, const Compare & cmp) noexcept
    {
      auto & mx = MAX_EP(p);
      mx = KEY(p).high;
      if (LLINK(p) != Node::NullPtr)
        if (cmp(mx, MAX_EP(LLINK(p))))
          mx = MAX_EP(LLINK(p));
      if (RLINK(p) != Node::NullPtr)
        if (cmp(mx, MAX_EP(RLINK(p))))
          mx = MAX_EP(RLINK(p));
    }

    /// Rotate right with augmented field update
    static Node * rotate_to_right_it(Node *p, const Compare & cmp) noexcept
    {
      assert(p != Node::NullPtr);
      Node *q = LLINK(p);
      LLINK(p) = RLINK(q);
      RLINK(q) = p;
      update_max(p, cmp); // old parent first (now child)
      update_max(q, cmp); // new parent (depends on updated p)
      return q;
    }

    /// Rotate left with augmented field update
    static Node * rotate_to_left_it(Node *p, const Compare & cmp) noexcept
    {
      assert(p != Node::NullPtr);
      Node *q = RLINK(p);
      RLINK(p) = LLINK(q);
      LLINK(q) = p;
      update_max(p, cmp);
      update_max(q, cmp);
      return q;
    }

    Node * insert(Node *root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p;

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      const IntervalCmp interval_cmp(cmp);
      Node *result = nullptr;
      if (interval_cmp(pk, rk))
        {
          result = insert(LLINK(root), p);
          if (result == Node::NullPtr)
            return Node::NullPtr;
          LLINK(root) = result;
          update_max(root, cmp);
          if (PRIO(LLINK(root)) < PRIO(root))
            return rotate_to_right_it(root, cmp);
          return root;
        }
      if (interval_cmp(rk, pk))
        {
          result = insert(RLINK(root), p);
          if (result == Node::NullPtr)
            return Node::NullPtr;
          RLINK(root) = result;
          update_max(root, cmp);
          if (PRIO(RLINK(root)) < PRIO(root))
            return rotate_to_left_it(root, cmp);
          return root;
        }

      return Node::NullPtr; // duplicate
    }

    Node * insert_dup(Node *root, Node *p) noexcept
    {
      if (root == Node::NullPtr)
        return p;

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      const IntervalCmp interval_cmp(cmp);
      if (interval_cmp(pk, rk))
        {
          LLINK(root) = insert_dup(LLINK(root), p);
          update_max(root, cmp);
          if (PRIO(LLINK(root)) < PRIO(root))
            return rotate_to_right_it(root, cmp);
          return root;
        }
      RLINK(root) = insert_dup(RLINK(root), p);
      update_max(root, cmp);
      if (PRIO(RLINK(root)) < PRIO(root))
        return rotate_to_left_it(root, cmp);
      return root;
    }

    static Node * join_exclusive(Node *t1, Node *t2,
                                 const Compare & cmp) noexcept
    {
      if (t1 == Node::NullPtr)
        return t2;
      if (t2 == Node::NullPtr)
        return t1;

      if (PRIO(t1) < PRIO(t2))
        {
          RLINK(t1) = join_exclusive(RLINK(t1), t2, cmp);
          update_max(t1, cmp);
          return t1;
        }
      LLINK(t2) = join_exclusive(t1, LLINK(t2), cmp);
      update_max(t2, cmp);
      return t2;
    }

    Node * remove(Node *& root, const Key & key) noexcept
    {
      if (root == Node::NullPtr)
        return Node::NullPtr;

      const Key & rk = KEY(root);
      const IntervalCmp interval_cmp(cmp);
      Node *ret_val;
      if (interval_cmp(key, rk))
        ret_val = remove(LLINK(root), key);
      else if (interval_cmp(rk, key))
        ret_val = remove(RLINK(root), key);
      else
        {
          ret_val = root;
          root = join_exclusive(LLINK(root), RLINK(root), cmp);
          return ret_val;
        }

      if (ret_val == Node::NullPtr)
        return Node::NullPtr;

      update_max(root, cmp);
      return ret_val;
    }

    /// Recursive: find all intervals overlapping query
    template <class Op>
    static void all_overlaps(Node *root, const Key & query,
                             Op & op, const Compare & cmp)
    {
      if (root == Node::NullPtr)
        return;

      // Prune: if max_endpoint < query.low, no overlap in this subtree
      if (cmp(MAX_EP(root), query.low))
        return;

      // Recurse left
      all_overlaps(LLINK(root), query, op, cmp);

      // Check current node
      if (KEY(root).overlaps(query, cmp))
        op(KEY(root));

      // Prune right: if root.low > query.high, no overlap in right subtree
      if (cmp(query.high, KEY(root).low))
        return;

      // Recurse right
      all_overlaps(RLINK(root), query, op, cmp);
    }

    /// Verify max_endpoint invariant
    static bool check_max(Node *root, const Compare & cmp) noexcept
    {
      if (root == Node::NullPtr)
        return true;

      T expected = KEY(root).high;
      if (LLINK(root) != Node::NullPtr)
        if (cmp(expected, MAX_EP(LLINK(root))))
          expected = MAX_EP(LLINK(root));
      if (RLINK(root) != Node::NullPtr)
        if (cmp(expected, MAX_EP(RLINK(root))))
          expected = MAX_EP(RLINK(root));

      if (cmp(MAX_EP(root), expected) or cmp(expected, MAX_EP(root)))
        return false; // MAX_EP(root) != expected

      return check_max(LLINK(root), cmp) and check_max(RLINK(root), cmp);
    }

    static size_t count_nodes(Node *root) noexcept
    {
      if (root == Node::NullPtr)
        return 0;
      return 1 + count_nodes(LLINK(root)) + count_nodes(RLINK(root));
    }

  public:
    /// Set the random number generator seed
    void set_seed(const unsigned long seed) noexcept { gsl_rng_set(r, seed); }

    /// Return the interval comparison functor (over Interval<T> keys)
    IntervalCmp key_comp() const noexcept { return IntervalCmp(cmp); }

    /// Return the endpoint comparison functor (const)
    const Compare & endpoint_comp() const noexcept { return cmp; }

    /// Backwards-compatible accessor for the endpoint comparator
    const Compare & get_compare() const noexcept { return endpoint_comp(); }

    /// Swap all state with `tree` in O(1)
    void swap(Gen_Interval_Tree & tree)
      noexcept(noexcept(std::swap(tree_root, tree.tree_root)) and
               noexcept(std::swap(cmp, tree.cmp)) and
               noexcept(std::swap(r, tree.r)) and
               noexcept(std::swap(num_nodes, tree.num_nodes)))
    {
      std::swap(tree_root, tree.tree_root);
      std::swap(cmp, tree.cmp);
      std::swap(r, tree.r);
      std::swap(num_nodes, tree.num_nodes);
    }

    /// Construct with explicit seed and comparator
    Gen_Interval_Tree(const unsigned long seed,
                      const Compare & _cmp = Compare())
      : head_ptr(&head), tree_root(RLINK(head_ptr)), r(nullptr),
        cmp(_cmp), num_nodes(0)
    {
      init(seed);
    }

    /// Construct with default seed (from system time)
    Gen_Interval_Tree(const Compare & _cmp = Compare())
      : Gen_Interval_Tree(static_cast<unsigned long>(time(nullptr)), _cmp)
    {
      /* empty */
    }

    Gen_Interval_Tree(const Gen_Interval_Tree &) = delete;
    Gen_Interval_Tree &operator =(const Gen_Interval_Tree &) = delete;
    Gen_Interval_Tree(Gen_Interval_Tree &&) = delete;
    Gen_Interval_Tree &operator =(Gen_Interval_Tree &&) = delete;

    ~Gen_Interval_Tree()
    {
      if (r != nullptr)
        gsl_rng_free(r);
    }

    /// Get a pointer to the GSL random number generator
    [[nodiscard]] gsl_rng * gsl_rng_object() const noexcept { return r; }

    /// Return the tree's root
    Node *&getRoot() noexcept { return tree_root; }

    /// Return the tree's root (const)
    Node * getRoot() const noexcept { return tree_root; }

    /// Return the number of nodes
    [[nodiscard]] size_t size() const noexcept { return num_nodes; }

    /// Return true if the tree is empty
    [[nodiscard]] bool is_empty() const noexcept { return num_nodes == 0; }

    /// Reset the node count (used after manual tree destruction)
    void reset_num_nodes() noexcept { num_nodes = 0; }

    /** @brief Search for an exact interval in the tree.
     *
     *  @param key the interval to search for
     *  @return pointer to the node if found, nullptr otherwise
     */
    Node * search(const Key & key) const noexcept
    {
      Node *ret_val = searchInBinTree(tree_root, key, key_comp());
      return ret_val == Node::NullPtr ? nullptr : ret_val;
    }

    /** @brief Insert a node (no duplicates).
     *
     *  @param p pointer to the node to insert
     *  @return p if inserted, nullptr if duplicate
     */
    Node * insert(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      PRIO(p) = gsl_rng_get(r);
      p->getMaxEndpoint() = KEY(p).high;
      Node *result = insert(tree_root, p);
      if (result == Node::NullPtr)
        return nullptr;
      tree_root = result;
      ++num_nodes;
      return p;
    }

    /** @brief Insert a node (duplicates allowed).
     *
     *  @param p pointer to the node to insert
     *  @return the p pointer
     */
    Node * insert_dup(Node *p) noexcept
    {
      assert(p != Node::NullPtr);
      PRIO(p) = gsl_rng_get(r);
      p->getMaxEndpoint() = KEY(p).high;
      tree_root = insert_dup(tree_root, p);
      ++num_nodes;
      return p;
    }

    /** @brief Remove an interval from the tree.
     *
     *  @param key the interval to remove
     *  @return pointer to removed node if found, nullptr otherwise
     */
    Node * remove(const Key & key) noexcept
    {
      Node *ret_val = remove(tree_root, key);
      if (ret_val == Node::NullPtr)
        return nullptr;
      ret_val->reset();
      --num_nodes;
      return ret_val;
    }

    /** @brief Find any interval that overlaps with `query` (CLRS algorithm).
     *
     *  @param query the query interval
     *  @return pointer to a node whose interval overlaps query, or nullptr
     *
     *  @par Complexity: O(log n) expected
     */
    Node * overlap_search(const Key & query) const noexcept
    {
      Node *x = tree_root;
      while (x != Node::NullPtr)
        {
          if (KEY(x).overlaps(query, cmp))
            return x;

          if (LLINK(x) != Node::NullPtr and
              not cmp(MAX_EP(LLINK(x)), query.low))
            x = LLINK(x);
          else
            x = RLINK(x);
        }
      return nullptr;
    }

    /** @brief Apply `op` to every interval overlapping `query`.
     *
     *  @param query the query interval
     *  @param op callable with signature `void(const Interval<T>&)`
     *
     *  @par Complexity: O(k log n) where k = number of overlapping intervals
     */
    template <class Op>
    void all_overlaps(const Key & query, Op && op) const
    {
      all_overlaps(tree_root, query, op, cmp);
    }

    /** @brief Find all intervals overlapping `query`.
     *
     *  @param query the query interval
     *  @return DynList of overlapping intervals
     */
    [[nodiscard]] DynList<Key> find_all_overlaps(const Key & query) const
    {
      DynList<Key> result;
      all_overlaps(query, [&result](const Key & iv) { result.append(iv); });
      return result;
    }

    /** @brief Apply `op` to every interval containing point `p`.
     *
     *  @param p the query point
     *  @param op callable with signature `void(const Interval<T>&)`
     *
     *  @par Complexity: O(k log n)
     */
    template <class Op>
    void stab(const T & p, Op && op) const
    {
      Key point_iv(p, p);
      all_overlaps(point_iv, std::forward<Op>(op));
    }

    /** @brief Find all intervals containing point `p`.
     *
     *  @param p the query point
     *  @return DynList of intervals containing p
     */
    [[nodiscard]] DynList<Key> find_stab(const T & p) const
    {
      return find_all_overlaps(Key(p, p));
    }

    /** @brief Verify BST, heap, and max_endpoint invariants.
     *
     *  @return true if all invariants hold
     */
    [[nodiscard]] bool verify() const noexcept
    {
      if (not check_bst(tree_root, key_comp()))
        return false;
      if (not is_treap(tree_root))
        return false;
      if (not check_max(tree_root, cmp))
        return false;
      if (count_nodes(tree_root) != num_nodes)
        return false;
      return true;
    }

    /** @brief Inorder iterator over nodes. */
    struct Iterator : public BinNodeInfixIterator<Node>
    {
      Iterator() noexcept = default;

      Iterator(const Gen_Interval_Tree & t)
        : BinNodeInfixIterator<Node>(t.tree_root) {}
    };
  };

  /** @brief Interval tree using nodes without virtual destructor.
   *
   *  @tparam T Endpoint type.
   *  @tparam Compare Comparison functor for endpoints.
   *  @ingroup Trees
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  struct Interval_Tree
      : public Gen_Interval_Tree<Interval_Tree_Node, T, Compare>
  {
    using Base = Gen_Interval_Tree<Interval_Tree_Node, T, Compare>;
    using Base::Base;
  };

  /** @brief Interval tree using nodes with virtual destructor.
   *
   *  @tparam T Endpoint type.
   *  @tparam Compare Comparison functor for endpoints.
   *  @ingroup Trees
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  struct Interval_Tree_Vtl
      : public Gen_Interval_Tree<Interval_Tree_NodeVtl, T, Compare>
  {
    using Base = Gen_Interval_Tree<Interval_Tree_NodeVtl, T, Compare>;
    using Base::Base;
  };

  /** @brief High-level interval tree with automatic memory management.
   *
   *  `DynIntervalTree<T>` wraps `Interval_Tree<T>` and provides:
   *  - Automatic new/delete for nodes
   *  - CRTP mixins for functional methods
   *  - Validation on insert (low <= high)
   *  - Copy/move semantics
   *
   *  @tparam T Endpoint type.
   *  @tparam Compare Comparison functor for endpoints.
   *
   *  @par Example:
   *  @code
   *  DynIntervalTree<int> tree;
   *  tree.insert(Interval<int>(1, 5));
   *  tree.insert(3, 10);
   *
   *  // Find any overlap with [4, 7]
   *  auto result = tree.overlap_search({4, 7});
   *
   *  // Find all intervals containing point 6
   *  auto hits = tree.find_stab(6);
   *  @endcode
   *
   *  @ingroup Trees
   */
  template <typename T, class Compare = Aleph::less<T>>
    requires StrictWeakOrder<Compare, T>
  class DynIntervalTree
      : public EqualToMethod<DynIntervalTree<T, Compare>>,
        public FunctionalMethods<DynIntervalTree<T, Compare>, Interval<T>>,
        public GenericKeys<DynIntervalTree<T, Compare>, Interval<T>>,
        public StlAlephIterator<DynIntervalTree<T, Compare>>
  {
  public:
    using Key = Interval<T>;
    using Item_Type = Key;
    using Key_Type = Key;
    using Node = typename Interval_Tree<T, Compare>::Node;
    using Tree = Interval_Tree<T, Compare>;

  private:
    Tree tree;

    void copy_from(const DynIntervalTree & src)
    {
      src.for_each([this](const Key & iv) { append(iv); });
    }

    void destroy_tree()
    {
      destroyRec(tree.getRoot());
      tree.reset_num_nodes();
    }

  public:
    /** @brief Default constructor.
     *  @param[in] cmp Comparison functor for endpoints.
     */
    DynIntervalTree(const Compare & cmp = Compare()) : tree(cmp) {}

    /** @brief Construct with explicit seed for the internal RNG.
     *  @param[in] seed Random seed.
     *  @param[in] cmp Comparison functor for endpoints.
     */
    DynIntervalTree(const unsigned long seed,
                    const Compare & cmp = Compare())
      : tree(seed, cmp) {}

    /** @brief Copy constructor. 
     *  @param[in] src Source tree to copy from.
     */
    DynIntervalTree(const DynIntervalTree & src)
      : tree(src.tree.endpoint_comp())
    {
      copy_from(src);
    }

    /** @brief Move constructor.
     *
     *  Not declared noexcept because the initializer
     *  `tree(src.tree.endpoint_comp())` constructs a new Tree, which
     *  allocates a GSL RNG via gsl_rng_alloc and copies the comparator —
     *  both can throw.
     *
     *  @param[in,out] src Source tree to move from.
     */
    DynIntervalTree(DynIntervalTree && src)
      : tree(src.tree.endpoint_comp())
    {
      tree.swap(src.tree);
    }

    /** @brief Copy assignment.
     *  @param[in] src Source tree to copy from.
     *  @return Reference to this tree.
     */
    DynIntervalTree &operator =(const DynIntervalTree & src)
    {
      if (this == &src)
        return *this;
      DynIntervalTree tmp(src);
      swap(tmp);
      return *this;
    }

    /** @brief Move assignment.
     *  @param[in,out] src Source tree to move from.
     *  @return Reference to this tree.
     */
    DynIntervalTree &operator =(DynIntervalTree && src)
      noexcept(noexcept(std::declval<Tree &>().swap(std::declval<Tree &>())))
    {
      if (this == &src)
        return *this;
      destroy_tree();
      tree.swap(src.tree);
      return *this;
    }

    /** @brief Destructor. */
    ~DynIntervalTree() { destroy_tree(); }

    /** @brief Swap state with another tree in O(1).
     *  @param[in,out] other The other tree.
     */
    void swap(DynIntervalTree & other)
      noexcept(noexcept(std::declval<Tree &>().swap(std::declval<Tree &>())))
    {
      tree.swap(other.tree);
    }

    Special_Ctors(DynIntervalTree, Key);

    /** @brief Insert an interval (unique).
     *
     *  @param[in] iv the interval to insert.
     *  @return reference to the key stored in the tree.
     *  @throws std::domain_error if iv.low > iv.high (invalid interval) or 
     *          if the interval already exists in the tree.
     */
    Key &insert(const Key & iv)
    {
      ah_domain_error_if(not iv.is_valid(tree.endpoint_comp()))
        << "Invalid interval: low > high";
      Node *p = new Node(iv);
      Node *result = tree.insert(p);
      if (result == nullptr)
        {
          delete p;
          ah_domain_error() << "Duplicate interval";
        }
      return KEY(result);
    }

    /** @brief Insert an interval (move version, unique).
     *
     *  @param[in,out] iv the interval to insert.
     *  @return reference to the key stored in the tree.
     *  @throws std::domain_error if iv.low > iv.high (invalid interval) or 
     *          if the interval already exists in the tree.
     */
    Key &insert(Key && iv)
    {
      ah_domain_error_if(not iv.is_valid(tree.endpoint_comp()))
        << "Invalid interval: low > high";
      Node *p = new Node(std::move(iv));
      Node *result = tree.insert(p);
      if (result == nullptr)
        {
          delete p;
          ah_domain_error() << "Duplicate interval";
        }
      return KEY(result);
    }

    /** @brief Insert an interval given endpoints (unique).
     *
     *  @param[in] lo lower endpoint.
     *  @param[in] hi upper endpoint.
     *  @return reference to the key stored in the tree.
     *  @throws std::domain_error if lo > hi or if the interval already exists.
     */
    Key &insert(const T & lo, const T & hi)
    {
      return insert(Key(lo, hi));
    }

    /** @brief Insert interval, allowing duplicates.
     *
     *  @param[in] iv the interval to insert.
     *  @return reference to the stored key.
     *  @throws std::domain_error if iv.low > iv.high.
     */
    Key &insert_dup(const Key & iv)
    {
      ah_domain_error_if(not iv.is_valid(tree.endpoint_comp()))
        << "Invalid interval: low > high";
      Node *p = new Node(iv);
      tree.insert_dup(p);
      return KEY(p);
    }

    /** @brief Append an interval (alias for insert_dup).
     *  @param[in] iv the interval to append.
     *  @return reference to the stored key.
     *  @throws std::domain_error if iv.low > iv.high.
     */
    Key &append(const Key & iv) { return insert_dup(iv); }

    /** @brief Remove an interval from the tree.
     *
     *  @param[in] iv the interval to remove.
     *  @return true if removed, false if not found.
     */
    bool remove(const Key & iv)
    {
      Node *p = tree.remove(iv);
      if (p == nullptr)
        return false;
      delete p;
      return true;
    }

    /** @brief Remove an interval given endpoints.
     *
     *  @param[in] lo lower endpoint.
     *  @param[in] hi upper endpoint.
     *  @return true if removed, false if not found.
     */
    bool remove(const T & lo, const T & hi)
    {
      return remove(Key(lo, hi));
    }

    /** @brief Search for an exact interval.
     *
     *  @param[in] iv interval to search.
     *  @return pointer to the stored interval if found, nullptr otherwise.
     *  @throws none
     */
    const Key * search(const Key & iv) const noexcept
    {
      auto *p = tree.search(iv);
      return p != nullptr ? &KEY(p) : nullptr;
    }

    /** @brief Find any interval overlapping `query`.
     *
     *  @param[in] query the query interval.
     *  @return pointer to an overlapping interval, or nullptr if none.
     *  @throws none
     *  @par Complexity: O(log n) expected.
     */
    const Key * overlap_search(const Key & query) const noexcept
    {
      auto *p = tree.overlap_search(query);
      return p != nullptr ? &KEY(p) : nullptr;
    }

    /** @brief Overload of overlap_search() using endpoints.
     *  @param[in] lo lower endpoint.
     *  @param[in] hi upper endpoint.
     *  @return pointer to an overlapping interval, or nullptr.
     *  @throws none
     */
    const Key * overlap_search(const T & lo, const T & hi) const noexcept
    {
      return overlap_search(Key(lo, hi));
    }

    /** @brief Apply op to every overlapping interval.
     *
     *  @tparam Op Callable with signature `void(const Key&)`.
     *  @param[in] query the query interval.
     *  @param[in] op callable to invoke for each overlap.
     *  @throws Any exception thrown by @p op.
     */
    template <class Op>
    void all_overlaps(const Key & query, Op && op) const
    {
      tree.all_overlaps(query, std::forward<Op>(op));
    }

    /** @brief Find all overlapping intervals.
     *
     *  @param[in] query the query interval.
     *  @return DynList containing all overlapping intervals.
     */
    [[nodiscard]] DynList<Key> find_all_overlaps(const Key & query) const
    {
      return tree.find_all_overlaps(query);
    }

    /** @brief Overload of find_all_overlaps() using endpoints.
     *  @param[in] lo lower endpoint.
     *  @param[in] hi upper endpoint.
     *  @return DynList of overlapping intervals.
     */
    [[nodiscard]] DynList<Key> find_all_overlaps(const T & lo,
                                                 const T & hi) const
    {
      return find_all_overlaps(Key(lo, hi));
    }

    /** @brief Apply op to every interval containing point p.
     *  @tparam Op Callable with signature `void(const Key&)`.
     *  @param[in] p query point.
     *  @param[in] op callable to invoke for each containing interval.
     *  @throws none
     */
    template <class Op>
    void stab(const T & p, Op && op) const
    {
      tree.stab(p, std::forward<Op>(op));
    }

    /** @brief Find all intervals containing point p.
     *  @param[in] p query point.
     *  @return DynList of intervals containing p.
     */
    [[nodiscard]] DynList<Key> find_stab(const T & p) const
    {
      return tree.find_stab(p);
    }

    /** @brief Return the number of intervals in the tree.
     *  @return number of nodes.
     *  @throws none
     */
    [[nodiscard]] size_t size() const noexcept { return tree.size(); }

    /** @brief Return true if the tree contains no intervals.
     *  @return true if empty.
     *  @throws none
     */
    [[nodiscard]] bool is_empty() const noexcept { return tree.is_empty(); }

    /** @brief Return true if the tree contains no intervals (STL compatible).
     *  @return true if empty.
     *  @throws none
     */
    [[nodiscard]] bool empty() const noexcept { return is_empty(); }

    /** @brief Verify all tree invariants (BST, Treap, MaxEndpoint).
     *  @return true if valid.
     *  @throws none
     */
    [[nodiscard]] bool verify() const noexcept { return tree.verify(); }

    /** @brief Traverse all intervals in inorder (sorted by low endpoint).
     *
     *  If op returns false, traversal stops early.
     *
     *  @tparam Operation callable with signature `bool(const Key&)`.
     *  @param[in] op callable to perform on each interval.
     *  @return true if all items were traversed.
     *  @throws anything that `op` throws.
     */
    template <class Operation>
    bool traverse(Operation & op)
    {
      return Aleph::traverse(tree.getRoot(), [&op](Node *p)
                               {
                                 return op(p->get_key());
                               });
    }

    /** @brief Overload of traverse() for rvalue callbacks.
     *  @tparam Operation callable with signature `bool(const Key&)`.
     *  @param[in] op callable to perform on each interval.
     *  @return true if all items were traversed.
     *  @throws anything that `op` throws.
     */
    template <class Operation>
    bool traverse(Operation && op = Operation())
    {
      return traverse<Operation>(op);
    }

    /** @brief Const overload of traverse().
     *  @tparam Operation callable with signature `bool(const Key&)`.
     *  @param[in] op callable to perform on each interval.
     *  @return true if all items were traversed.
     *  @throws anything that `op` throws.
     */
    template <class Operation>
    bool traverse(Operation & op) const
    {
      return Aleph::traverse(tree.getRoot(), [&op](Node *p)
                               {
                                 return op(p->get_key());
                               });
    }

    /** @brief Const rvalue overload of traverse().
     *  @tparam Operation callable with signature `bool(const Key&)`.
     *  @param[in] op callable to perform on each interval.
     *  @return true if all items were traversed.
     *  @throws anything that `op` throws.
     */
    template <class Operation>
    bool traverse(Operation && op = Operation()) const
    {
      return traverse<Operation>(op);
    }

    /** @brief Inorder iterator yielding Interval<T> keys.
     *
     *  Satisfies the Aleph iterator protocol (has_curr/get_curr/next).
     */
    class Iterator
    {
      typename Tree::Iterator it;

    public:
      /** @brief Default constructor. */
      Iterator() noexcept = default;

      /** @brief Construct for a given tree.
       *  @param[in] t tree to iterate over.
       */
      Iterator(const DynIntervalTree & t) : it(t.tree) {}

      /** @brief Check if there is a current element.
       *  @return true if valid current element exists.
       */
      [[nodiscard]] bool has_curr() const noexcept { return it.has_curr(); }

      /** @brief Return the current interval.
       *  @return constant reference to interval.
       *  @throws ah_invalid_argument if no current element exists.
       */
      const Key &get_curr() const
      {
        return KEY(static_cast<const Node*>(it.get_curr()));
      }

      /** @brief Return the current interval (no check).
       *  @return constant reference to interval.
       *  @throws none
       */
      const Key &get_curr_ne() const noexcept
      {
        return KEY(static_cast<const Node*>(it.get_curr_ne()));
      }

      /** @brief Advance to the next element.
       *  @throws ah_invalid_argument if already at end.
       */
      void next() { it.next(); }

      /** @brief Advance to the next element (no check).
       *  @throws none
       */
      void next_ne() noexcept { it.next_ne(); }

      /** @brief Go back to the previous element.
       *  @throws ah_invalid_argument if already at beginning.
       */
      void prev() { it.prev(); }

      /** @brief Reset to the first element in traversal order.
       *  @throws none
       */
      void reset_first() noexcept { it.reset_first(); }

      /** @brief Move to the position after the last element.
       *  @throws none
       */
      void end() noexcept { it.end(); }

      /** @brief Return the 0-indexed current position.
       *  @return index of current element.
       *  @throws ah_invalid_argument if no current element.
       */
      [[nodiscard]] size_t get_pos() const { return it.get_pos(); }
    };
  };
} // end namespace Aleph

# endif // TPL_INTERVAL_TREE_H
