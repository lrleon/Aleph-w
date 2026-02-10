
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


/** @file tpl_tdRbTree.H
 *  @brief Top-down Red-Black tree implementation.
 *
 *  Red-Black tree with top-down single-pass insertion.
 *  Simpler code path than bottom-up variant.
 *
 *  ## Features
 *  - Single pass from root to leaf
 *  - No parent pointers needed
 *  - Color flips during descent
 *
 *  ## Complexity: O(log n) for all operations
 *
 *  @see tpl_rb_tree.H Bottom-up variant
 *  @see tpl_tdRbTreeRk.H With rank support
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TDRBTREE_H
# define TPL_TDRBTREE_H

# include <functional>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_arrayStack.H>
# include <rbNode.H>
# include <ah-concepts.H>

namespace Aleph
{

/** @brief Top-down red-black binary search tree implementation.
    
    This class implements the top-down (single-pass) red-black tree algorithm
    as described by Guibas and Sedgewick. Unlike the bottom-up approach that
    requires backtracking after insertion/deletion, this algorithm performs
    all necessary rotations and color changes while descending the tree.
    
    @par Advantages over bottom-up:
    - No stack needed for ancestor path (saves O(log n) space)
    - Better cache locality (single pass through tree)
    - Potentially faster due to reduced memory traffic
    
    @par Disadvantages:
    - More complex code
    - May perform more color flips than strictly necessary
    
    @par Complexity:
    - Search: O(log n) worst case
    - Insert: O(log n) worst case, at most 2 rotations
    - Delete: O(log n) worst case, at most 3 rotations
    - Size: O(1)
    
    @tparam NodeType Node template (typically RbNode or RbNodeVtl).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys (default: std::less<Key>).
    
    @see Gen_Rb_Tree Bottom-up red-black tree implementation.
    @see Avl_Tree Alternative balanced tree with stricter balance.
    @ingroup Trees
 */
template <template <class> class NodeType, typename Key,
          class Compare = std::less<Key>>
    requires StrictWeakOrder<Compare, Key>
class GenTdRbTree
{
public:
  using Node = NodeType<Key>;
  using key_type = Key;
  using compare_type = Compare;

private:
  Node headNode;      ///< Sentinel header node (parent of root)
  Node headParent;    ///< Sentinel grandparent node
  Node *head;         ///< Pointer to header
  Node *fHead;        ///< Pointer to header's parent
  Node *&root;        ///< Reference to root (right child of head)
  Compare cmp;        ///< Comparison functor
  size_t num_nodes;   ///< Number of nodes in tree

  /// Returns true if a < b according to comparator
  bool less(const Key& a, const Key& b) const noexcept
  {
    return cmp(a, b);
  }

  /// Returns true if a == b (neither a < b nor b < a)
  bool equals(const Key& a, const Key& b) const noexcept
  {
    return not cmp(a, b) and not cmp(b, a);
  }

  /// Get sibling of node p whose parent is fp
  static Node* getSibling(Node *p, Node *fp) noexcept
  {
    assert(LLINK(fp) == p or RLINK(fp) == p);
    return LLINK(fp) == p ? RLINK(fp) : LLINK(fp);
  }

  /// Restore red-black property after color flip creates two consecutive reds
  void restoreRedCondition(Node *p, Node *&fp, Node *ffp, Node *fffp) noexcept
  {
    assert(LLINK(fp) == p or RLINK(fp) == p);
    assert(COLOR(fp) == RED);
    assert(COLOR(p) == RED);

    if (fp == root)
      {
        COLOR(fp) = BLACK;
        return;
      }

    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(COLOR(ffp) == BLACK);
    assert(LLINK(fffp) == ffp or RLINK(fffp) == ffp);

    COLOR(ffp) = RED;

    if (LLINK(fp) == p and LLINK(ffp) == fp)
      {
        COLOR(fp) = BLACK;
        rotate_to_right(ffp, fffp);
      }
    else if (RLINK(fp) == p and RLINK(ffp) == fp)
      {
        COLOR(fp) = BLACK;
        rotate_to_left(ffp, fffp);
      }
    else 
      {
        COLOR(p) = BLACK;
        if (RLINK(fp) == p)
          {
            rotate_to_left(fp, ffp);
            rotate_to_right(ffp, fffp);
          }
        else
          {
            rotate_to_right(fp, ffp);
            rotate_to_left(ffp, fffp);
          }
        fp = fffp;
      }
  }
  
  /// Flip colors: black parent with two red children becomes red with two black children
  static void flipColors(Node* p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == BLACK);
    assert(COLOR(LLINK(p)) == RED and COLOR(RLINK(p)) == RED);

    COLOR(p) = RED;
    COLOR(LLINK(p)) = COLOR(RLINK(p)) = BLACK;
  }

  /// Search for insertion point, flipping colors proactively to maintain balance
  Node* searchFlipColorsAndInsert(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);
    assert(LLINK(q) == Node::NullPtr and RLINK(q) == Node::NullPtr);

    Node *p    = root;              // Current node
    Node *fp   = head;              // p's parent
    Node *ffp  = fHead;             // p's grandparent
    Node *fffp = Node::NullPtr;     // p's great-grandparent
    Node *nextNode;

    while (true)
      {
        if (equals(KEY(q), KEY(p)))
          return nullptr;  // Duplicated key, insertion not possible

        // Proactive color flip: if p is black with two red children, flip
        if (COLOR(p) == BLACK and COLOR(LLINK(p)) == RED 
            and COLOR(RLINK(p)) == RED)
          {
            flipColors(p);
            if (COLOR(fp) == RED)  // May violate red condition
              {
                assert(fffp != Node::NullPtr);
                restoreRedCondition(p, fp, ffp, fffp);
              }
          }

        // Descend to appropriate child
        if (less(KEY(q), KEY(p)))
          { 
            if (LLINK(p) == Node::NullPtr)
              break;
            nextNode = LLINK(p);
          }
        else 
          {
            if (RLINK(p) == Node::NullPtr)
              break;
            nextNode = RLINK(p);
          }

        // Update ancestor chain
        fffp = ffp;
        ffp  = fp;
        fp   = p;
        p    = nextNode;
      }

    ++num_nodes;

    // Perform insertion
    if (less(KEY(q), KEY(p)))
      LLINK(p) = q;
    else 
      RLINK(p) = q;

    if (COLOR(p) == RED)  // May violate red condition
      restoreRedCondition(q, p, fp, ffp);

    return q;
  }

  /// Search for insertion point allowing duplicates
  Node* searchFlipColorsAndInsertDup(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);
    assert(LLINK(q) == Node::NullPtr and RLINK(q) == Node::NullPtr);

    Node *p    = root;
    Node *fp   = head;
    Node *ffp  = fHead;
    Node *fffp = Node::NullPtr;
    Node *nextNode;

    while (true)
      {
        // Proactive color flip
        if (COLOR(p) == BLACK and COLOR(LLINK(p)) == RED 
            and COLOR(RLINK(p)) == RED)
          {
            flipColors(p);
            if (COLOR(fp) == RED)
              {
                assert(fffp != Node::NullPtr);
                restoreRedCondition(p, fp, ffp, fffp);
              }
          }

        // Descend (duplicates go right)
        if (less(KEY(q), KEY(p)))
          { 
            if (LLINK(p) == Node::NullPtr)
              break;
            nextNode = LLINK(p);
          }
        else  // >= goes right
          {
            if (RLINK(p) == Node::NullPtr)
              break;
            nextNode = RLINK(p);
          }

        fffp = ffp;
        ffp  = fp;
        fp   = p;
        p    = nextNode;
      }

    ++num_nodes;

    if (less(KEY(q), KEY(p)))
      LLINK(p) = q;
    else 
      RLINK(p) = q;

    if (COLOR(p) == RED)
      restoreRedCondition(q, p, fp, ffp);

    return q;
  }

  /// Descend to left child, ensuring current node is red for deletion
  static Node* gotoLeftAndColorRed(Node *fp, Node *&ffp) noexcept
  {
    assert(fp != Node::NullPtr);
    assert(ffp != Node::NullPtr);
    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(LLINK(fp) != Node::NullPtr);

    Node *p = LLINK(fp);

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = RLINK(fp);

    // fp black means we came from another black node
    if (COLOR(fp) == BLACK)
      {
        assert(COLOR(sp) == RED);
        rotate_to_left(fp, ffp);
        COLOR(fp) = RED;
        COLOR(sp) = BLACK;
        ffp = sp;
        sp = RLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      {
        assert(COLOR(LLINK(fp)) == BLACK);
        assert(COLOR(RLINK(fp)) == BLACK);
        assert(COLOR(fp) == RED);

        COLOR(p) = RED;
        COLOR(fp) = BLACK;

        Node *np  = RLINK(sp);   // Nephew of p
        Node *snp = LLINK(sp);   // Sibling-nephew of p

        if (COLOR(snp) == BLACK and COLOR(np) == BLACK) 
          {
            COLOR(sp) = RED;
            return p;
          }

        if (COLOR(np) == RED)
          {
            ffp = rotate_to_left(fp, ffp);
            COLOR(sp) = RED;
            COLOR(np) = BLACK;
            assert(ffp == sp);
            return p;
          }
          
        assert(COLOR(snp) == RED);

        rotate_to_right(sp, fp);
        ffp = rotate_to_left(fp, ffp);
        assert(ffp == snp);
      }
    return p;
  }

  /// Descend to right child, ensuring current node is red for deletion
  static Node* gotoRightAndColorRed(Node *fp, Node *&ffp) noexcept
  {
    assert(fp != Node::NullPtr);
    assert(ffp != Node::NullPtr);
    assert(LLINK(ffp) == fp or RLINK(ffp) == fp);
    assert(RLINK(fp) != Node::NullPtr);

    Node *p = RLINK(fp);

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = LLINK(fp);

    if (COLOR(fp) == BLACK)
      {
        assert(COLOR(sp) == RED);
        rotate_to_right(fp, ffp);
        COLOR(fp) = RED;
        COLOR(sp) = BLACK;
        ffp = sp;
        sp = LLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      {
        assert(COLOR(RLINK(fp)) == BLACK);
        assert(COLOR(LLINK(fp)) == BLACK);
        assert(COLOR(fp) == RED);

        COLOR(p) = RED;
        COLOR(fp) = BLACK;

        Node *np  = LLINK(sp);   // Nephew of p
        Node *snp = RLINK(sp);   // Sibling-nephew of p

        if (COLOR(snp) == BLACK and COLOR(np) == BLACK)
          {
            COLOR(sp) = RED;
            return p;
          }

        if (COLOR(np) == RED)
          {
            ffp = rotate_to_right(fp, ffp);
            COLOR(sp) = RED;
            COLOR(np) = BLACK;
            assert(ffp == sp);
            return p;
          }
          
        assert(COLOR(snp) == RED);

        rotate_to_left(sp, fp);
        ffp = rotate_to_right(fp, ffp);
        assert(ffp == snp);
      }
    return p;
  }

  /// Find successor and swap with node p for deletion
  static void findSuccAndSwap(Node *p, Node *&fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(RLINK(p) != Node::NullPtr);
    assert(fp != Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);

    Node *fSucc  = p;
    Node *succ   = gotoRightAndColorRed(p, fp); 
    Node *ffSucc = fp;
    
    while (LLINK(succ) != Node::NullPtr)
      {
        ffSucc = fSucc;
        fSucc  = succ;
        succ   = gotoLeftAndColorRed(fSucc, ffSucc);
      }

    // Update parent of p to point to succ
    if (LLINK(fp) == p)
      LLINK(fp) = succ;
    else
      RLINK(fp) = succ;
    
    // Swap left branches
    LLINK(succ) = LLINK(p);
    LLINK(p)    = Node::NullPtr; 

    // Swap right branches (two cases)
    if (RLINK(p) == succ) 
      {
        RLINK(p)    = RLINK(succ); 
        RLINK(succ) = p;
        fSucc       = fp;
        fp          = succ;
      }
    else
      {
        Node *tmp    = fp;
        Node *succr  = RLINK(succ); 
          
        RLINK(succ)  = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p)     = succr;
        fp           = fSucc;
        fSucc        = tmp; 
      }

    // Swap colors
    Color tmp   = COLOR(succ);
    COLOR(succ) = COLOR(p); 
    COLOR(p)    = tmp;
  }

  /// Find predecessor and swap with node p for deletion
  static void findPredAndSwap(Node *p, Node *&fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) != Node::NullPtr);
    assert(fp != Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);

    Node *fPred  = p;
    Node *pred   = gotoLeftAndColorRed(p, fp);
    Node *ffPred = fp;
    
    while (RLINK(pred) != Node::NullPtr)
      {
        ffPred = fPred;
        fPred  = pred;
        pred   = gotoRightAndColorRed(fPred, ffPred);
      }

    // Update parent of p to point to pred
    if (LLINK(fp) == p)
      LLINK(fp) = pred;
    else
      RLINK(fp) = pred;
    
    // Swap right branches
    RLINK(pred) = RLINK(p);
    RLINK(p)    = Node::NullPtr; 

    // Swap left branches (two cases)
    if (LLINK(p) == pred) 
      {
        LLINK(p)    = LLINK(pred); 
        LLINK(pred) = p;
        fPred       = fp;
        fp          = pred;
      }
    else
      {
        Node *tmp    = fp;
        Node *predl  = LLINK(pred); 
          
        LLINK(pred)  = LLINK(p);
        RLINK(fPred) = p;
        LLINK(p)     = predl;
        fp           = fPred;
        fPred        = tmp; 
      }

    // Swap colors
    Color tmp   = COLOR(pred);
    COLOR(pred) = COLOR(p); 
    COLOR(p)    = tmp;
  }

  /// Color root red if safe (both children black)
  void colorRootAsRed() noexcept
  {
    if (COLOR(root) == RED)
      return;

    if (COLOR(LLINK(root)) == BLACK and COLOR(RLINK(root)) == BLACK)
      COLOR(root) = RED;      
  }

  /// Search for key while coloring nodes red for deletion
  Node* searchAndColorRed(const Key& key, Node *&fp) noexcept
  {
    Node *p   = root;
    fp        = head;
    Node *ffp = fHead;

    colorRootAsRed();

    while (true)
      {
        if (equals(key, KEY(p)))
          return p;

        ffp = fp;
        fp  = p;

        if (less(key, KEY(p)))
          {
            if (LLINK(p) == Node::NullPtr)
              return p;
            p = gotoLeftAndColorRed(fp, ffp);
          }
        else
          {
            if (RLINK(p) == Node::NullPtr)
              return p;
            p = gotoRightAndColorRed(fp, ffp);
          }
      }
  }

  /// Remove node p and ensure it becomes a red leaf
  static void removeAndRendLeafRed(Node *p, Node *fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(fp != Node::NullPtr);
    assert(LLINK(fp) == p or RLINK(fp) == p);

    while (LLINK(p) != Node::NullPtr or RLINK(p) != Node::NullPtr)
      {
        if (RLINK(p) != Node::NullPtr)
          findSuccAndSwap(p, fp);
        else if (LLINK(p) != Node::NullPtr)
          findPredAndSwap(p, fp);
      }

    if (LLINK(fp) == p)
      LLINK(fp) = Node::NullPtr;
    else
      RLINK(fp) = Node::NullPtr;
  }

  /// Initialize header nodes
  void init() noexcept
  {
    RLINK(fHead) = head;
    COLOR(Node::NullPtr) = BLACK;
    COLOR(head) = BLACK;
    num_nodes = 0;
  }

public:
  /// Default constructor
  GenTdRbTree() noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()), cmp()
  {
    init();
  }

  /// Constructor with comparator
  explicit GenTdRbTree(const Compare & __cmp) noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()), cmp(__cmp)
  {
    init();
  }

  /// Move constructor
  GenTdRbTree(GenTdRbTree && other) noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()),
      cmp(std::move(other.cmp)), num_nodes(other.num_nodes)
  {
    RLINK(fHead) = head;
    COLOR(Node::NullPtr) = BLACK;
    COLOR(head) = BLACK;
    root = other.root;
    other.root = Node::NullPtr;
    other.num_nodes = 0;
  }

  /// Move assignment operator
  GenTdRbTree & operator=(GenTdRbTree && other) noexcept
  {
    if (this != &other)
      {
        root = other.root;
        num_nodes = other.num_nodes;
        cmp = std::move(other.cmp);
        other.root = Node::NullPtr;
        other.num_nodes = 0;
      }
    return *this;
  }

  /// Deleted copy constructor (use explicit copy if needed)
  GenTdRbTree(const GenTdRbTree &) = delete;
  
  /// Deleted copy assignment
  GenTdRbTree & operator=(const GenTdRbTree &) = delete;

  /// Reset tree to empty state (does not free nodes)
  void reset() noexcept
  {
    root = Node::NullPtr;
    num_nodes = 0;
  }

  /// Swap contents with another tree
  void swap(GenTdRbTree & other) noexcept
  {
    std::swap(root, other.root);
    std::swap(num_nodes, other.num_nodes);
    std::swap(cmp, other.cmp);
  }

  virtual ~GenTdRbTree() = default;

  /// Get number of nodes in tree (O(1))
  size_t size() const noexcept { return num_nodes; }

  /// Check if tree is empty
  bool is_empty() const noexcept { return root == Node::NullPtr; }

  /// Get reference to comparator
  Compare & get_compare() noexcept { return cmp; }
  const Compare & get_compare() const noexcept { return cmp; }

  /** @brief Insert a node into the tree.
      
      @param p Node to insert (must be red with null children).
      @return Pointer to inserted node, or nullptr if key already exists.
   */
  Node* insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    if (root == Node::NullPtr)
      {
        root = p;
        ++num_nodes;
        return p;
      }

    return searchFlipColorsAndInsert(p);
  }

  /** @brief Insert a node, allowing duplicates.
      
      @param p Node to insert.
      @return Pointer to inserted node (always succeeds).
   */
  Node* insert_dup(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    if (root == Node::NullPtr)
      {
        root = p;
        ++num_nodes;
        return p;
      }

    return searchFlipColorsAndInsertDup(p);
  }

  /** @brief Search for key or insert if not found.
      
      @param p Node to search or insert.
      @return Pointer to p if inserted, or pointer to existing node with same key.
   */
  Node* search_or_insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);

    if (root == Node::NullPtr)
      {
        root = p;
        ++num_nodes;
        return p;
      }

    // Search first
    Node *found = search(KEY(p));
    if (found != nullptr)
      return found;  // Return existing node

    // Not found, insert
    return searchFlipColorsAndInsert(p);
  }

  /** @brief Search for a key in the tree.
      
      @param key Key to search for.
      @return Pointer to node containing key, or nullptr if not found.
   */
  Node* search(const Key& key) const noexcept
  {
    Node* p = root;
    while (p != Node::NullPtr)
      {
        if (equals(key, KEY(p)))
          return p;
        p = less(key, KEY(p)) ? LLINK(p) : RLINK(p);
      }
    return nullptr;
  }

  /** @brief Remove and return node with given key.
      
      @param key Key to remove.
      @return Pointer to removed node, or nullptr if not found.
   */
  Node* remove(const Key& key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;
      
    Node *fp;
    Node *p = searchAndColorRed(key, fp);

    if (not equals(KEY(p), key))
      return nullptr;

    removeAndRendLeafRed(p, fp);
    --num_nodes;

    return p;
  }

  /// Get reference to root pointer
  Node*& getRoot() noexcept { return root; }
  
  /// Get const root pointer  
  Node* getRoot() const noexcept { return root; }

private:
  /// Test black height property recursively
  static bool testBlackHeight(Node *p, int &max, int bh = 0) noexcept
  {
    if (p == Node::NullPtr)
      return true;

    if (COLOR(p) == BLACK)
      ++bh;

    if (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr)
      {
        if (max == -1)
          max = bh;
        return bh == max;
      }

    return testBlackHeight(LLINK(p), max, bh)
        and testBlackHeight(RLINK(p), max, bh);
  }

  /// Test red-black properties for a single node
  static bool testNode(Node* p) noexcept
  {
    if (p == Node::NullPtr)
      return true;
      
    // Red nodes must have black children
    if (COLOR(p) == RED and
        not (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK))
      return false;

    int max = -1;
    int bh  = 0;

    return testBlackHeight(p, max, bh);
  }

  /// Recursively verify red-black properties
  static bool verify(Node* p) noexcept
  {
    if (p == Node::NullPtr)
      return true;
    if (not testNode(p))
      return false;
    return verify(LLINK(p)) and verify(RLINK(p));
  }

public:
  /** @brief Verify that tree satisfies all red-black properties.
      
      @return true if tree is valid, false otherwise.
   */
  bool verifyRedBlack() const noexcept
  {
    if (root == Node::NullPtr)
      return true;
    return verify(root);
  }

  /// Alias for verify
  bool verify() const noexcept { return verifyRedBlack(); }

  /** @brief Iterator over tree nodes in sorted order.
      
      Allows traversing nodes according to the comparison criteria.
      
      @ingroup Trees
   */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(GenTdRbTree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    Iterator(const GenTdRbTree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};

/** @brief Top-down red-black tree without virtual destructor.
    
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor (default: std::less<Key>).
 */
template <typename Key, class Compare = std::less<Key>>
    requires StrictWeakOrder<Compare, Key>
class TdRbTree : public GenTdRbTree<RbNode, Key, Compare>
{
  using Base = GenTdRbTree<RbNode, Key, Compare>;
public:
  using Base::Base;
};

/** @brief Top-down red-black tree with virtual destructor.
    
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor (default: std::less<Key>).
 */
template <typename Key, class Compare = std::less<Key>>
    requires StrictWeakOrder<Compare, Key>
class TdRbTreeVtl : public GenTdRbTree<RbNodeVtl, Key, Compare>
{
  using Base = GenTdRbTree<RbNodeVtl, Key, Compare>;
public:
  using Base::Base;
};

} // namespace Aleph

# endif /* TPL_TDRBTREE_H */
