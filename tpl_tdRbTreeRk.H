
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


/** @file tpl_tdRbTreeRk.H
 *  @brief Top-down Red-Black tree with rank support.
 *
 *  Extends top-down RB tree with subtree size counts for
 *  O(log n) select() and position() operations.
 *
 *  ## Features
 *  - select(k): Find k-th smallest element
 *  - position(key): Find rank of key
 *  - O(log n) insert with rank updates
 *
 *  @see tpl_tdRbTree.H Base implementation
 *  @see tpl_rbRk.H Bottom-up variant with rank
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TDRBTREERK_H
# define TPL_TDRBTREERK_H

# include <functional>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTreeOps.H>
# include <rbNodeRk.H>

namespace Aleph
{

/** @brief Top-down red-black tree with rank support (select/position).
    
    This class extends the top-down red-black tree algorithm with subtree
    counters in each node, enabling O(log n) operations for:
    - select(i): Get the i-th smallest element
    - position(key): Get the rank/position of a key
    - find_position(key): Find position even if key doesn't exist
    - split_pos(pos, t1, t2): Split tree by position
    
    @par Complexity:
    - search, select, position: O(log n)
    - insert: O(log n) - uses stack for count updates
    - remove: O(n) - swaps make incremental updates unreliable
    
    For O(log n) remove with rank, use Rb_Tree_Rk (bottom-up) which
    tracks the full path during descent.
    
    @par Advantages over bottom-up:
    - Single-pass descent for insert (rotations done on the way down)
    - O(1) size() operation
    - O(log n) indexed access (select)
    - O(log n) position queries
    
    @tparam NodeType Node template (typically RbNodeRk or RbNodeRkVtl).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys.
    
    @see Gen_Rb_Tree_Rk Bottom-up red-black tree with rank support.
    @see GenTdRbTree Top-down red-black tree without rank support.
    @ingroup Trees
 */
template <template <class> class NodeType, typename Key, 
          class Compare = std::less<Key>>
class GenTdRbTreeRk 
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

  /// Rotate to right with counter update
  static Node* rotate_to_right_rk(Node *p, Node *pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    
    Node *q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;
    
    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    
    if (LLINK(pp) == p)
      LLINK(pp) = q;
    else
      RLINK(pp) = q;
    
    return q;
  }

  /// Rotate to left with counter update
  static Node* rotate_to_left_rk(Node *p, Node *pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    
    Node *q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;
    
    // Update counters
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    
    if (LLINK(pp) == p)
      LLINK(pp) = q;
    else
      RLINK(pp) = q;
    
    return q;
  }

  /// Restore red-black property after color flip
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
        rotate_to_right_rk(ffp, fffp);
      }
    else if (RLINK(fp) == p and RLINK(ffp) == fp)
      {
        COLOR(fp) = BLACK;
        rotate_to_left_rk(ffp, fffp);
      }
    else 
      {
        COLOR(p) = BLACK;
        if (RLINK(fp) == p)
          {
            rotate_to_left_rk(fp, ffp);
            rotate_to_right_rk(ffp, fffp);
          }
        else
          {
            rotate_to_right_rk(fp, ffp);
            rotate_to_left_rk(ffp, fffp);
          }
        fp = fffp;
      }
  }
  
  /// Flip colors with counter preservation
  static void flipColors(Node* p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == BLACK);
    assert(COLOR(LLINK(p)) == RED and COLOR(RLINK(p)) == RED);

    COLOR(p) = RED;
    COLOR(LLINK(p)) = COLOR(RLINK(p)) = BLACK;
  }

  /// Search for insertion point, flipping colors proactively.
  /// Uses stack to track path for O(log n) count updates.
  /// @return Inserted node or nullptr if duplicate
  Node* searchFlipColorsAndInsert(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);

    // Stack to track visited nodes for count updates
    Node* stack[128];
    size_t stack_len = 0;

    Node *p    = root;
    Node *fp   = head;
    Node *ffp  = fHead;
    Node *fffp = Node::NullPtr;
    Node *nextNode;

    stack[stack_len++] = p;

    while (true)
      {
        if (equals(KEY(q), KEY(p)))
          return nullptr;  // Duplicate key

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

        fffp = ffp;
        ffp  = fp;
        fp   = p;
        p    = nextNode;
        stack[stack_len++] = p;
      }

    // Perform insertion
    if (less(KEY(q), KEY(p)))
      LLINK(p) = q;
    else 
      RLINK(p) = q;

    if (COLOR(p) == RED)
      restoreRedCondition(q, p, fp, ffp);

    // Update counts bottom-up - O(log n)
    // Each node's count = left_count + right_count + 1
    // Children are already correct (leaf has count=1, rotated nodes recalculated)
    updateCountsFromStack(stack, stack_len);

    return q;
  }

  /// Search for insertion point allowing duplicates.
  /// Uses stack for O(log n) count updates.
  Node* searchFlipColorsAndInsertDup(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);

    Node* stack[128];
    size_t stack_len = 0;

    Node *p    = root;
    Node *fp   = head;
    Node *ffp  = fHead;
    Node *fffp = Node::NullPtr;
    Node *nextNode;

    stack[stack_len++] = p;

    while (true)
      {
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

        fffp = ffp;
        ffp  = fp;
        fp   = p;
        p    = nextNode;
        stack[stack_len++] = p;
      }

    if (less(KEY(q), KEY(p)))
      LLINK(p) = q;
    else 
      RLINK(p) = q;

    if (COLOR(p) == RED)
      restoreRedCondition(q, p, fp, ffp);

    // Update counts bottom-up - O(log n)
    updateCountsFromStack(stack, stack_len);

    return q;
  }

  /// Recursively update counts in subtree - O(n)
  static size_t updateCountRec(Node *p) noexcept
  {
    if (p == Node::NullPtr)
      return 0;
    
    size_t left_count = updateCountRec(LLINK(p));
    size_t right_count = updateCountRec(RLINK(p));
    COUNT(p) = left_count + right_count + 1;
    return COUNT(p);
  }
  
  /// Update counts for nodes in stack (bottom-up) - O(log n)
  /// After rotations, nodes may have moved but their counts can be
  /// recalculated from their current children.
  static void updateCountsFromStack(Node** stack, size_t len) noexcept
  {
    // Process bottom-up: children are already correct
    while (len > 0)
      {
        Node* p = stack[--len];
        COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
      }
  }
  

  /// Descend to left child for deletion, ensuring red
  static Node* gotoLeftAndColorRed(Node *fp, Node *&ffp) noexcept
  {
    assert(fp != Node::NullPtr);
    assert(ffp != Node::NullPtr);
    assert(LLINK(fp) != Node::NullPtr);

    Node *p = LLINK(fp);

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = RLINK(fp);

    if (COLOR(fp) == BLACK)
      {
        assert(COLOR(sp) == RED);
        rotate_to_left_rk(fp, ffp);
        COLOR(fp) = RED;
        COLOR(sp) = BLACK;
        ffp = sp;
        sp = RLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      {
        assert(COLOR(fp) == RED);

        COLOR(p) = RED;
        COLOR(fp) = BLACK;

        Node *np  = RLINK(sp);
        Node *snp = LLINK(sp);

        if (COLOR(snp) == BLACK and COLOR(np) == BLACK) 
          {
            COLOR(sp) = RED;
            return p;
          }

        if (COLOR(np) == RED)
          {
            ffp = rotate_to_left_rk(fp, ffp);
            COLOR(sp) = RED;
            COLOR(np) = BLACK;
            return p;
          }
          
        assert(COLOR(snp) == RED);

        rotate_to_right_rk(sp, fp);
        ffp = rotate_to_left_rk(fp, ffp);
      }
    return p;
  }

  /// Descend to right child for deletion, ensuring red
  static Node* gotoRightAndColorRed(Node *fp, Node *&ffp) noexcept
  {
    assert(fp != Node::NullPtr);
    assert(ffp != Node::NullPtr);
    assert(RLINK(fp) != Node::NullPtr);

    Node *p = RLINK(fp);

    if (COLOR(p) == RED)  
      return p; 

    Node *sp = LLINK(fp);

    if (COLOR(fp) == BLACK)
      {
        assert(COLOR(sp) == RED);
        rotate_to_right_rk(fp, ffp);
        COLOR(fp) = RED;
        COLOR(sp) = BLACK;
        ffp = sp;
        sp = LLINK(fp);
      }

    if (COLOR(LLINK(p)) == BLACK and COLOR(RLINK(p)) == BLACK)
      {
        assert(COLOR(fp) == RED);

        COLOR(p) = RED;
        COLOR(fp) = BLACK;

        Node *np  = LLINK(sp);
        Node *snp = RLINK(sp);

        if (COLOR(snp) == BLACK and COLOR(np) == BLACK)
          {
            COLOR(sp) = RED;
            return p;
          }

        if (COLOR(np) == RED)
          {
            ffp = rotate_to_right_rk(fp, ffp);
            COLOR(sp) = RED;
            COLOR(np) = BLACK;
            return p;
          }
          
        assert(COLOR(snp) == RED);

        rotate_to_left_rk(sp, fp);
        ffp = rotate_to_right_rk(fp, ffp);
      }
    return p;
  }

  /// Find successor and swap for deletion
  static void findSuccAndSwap(Node *p, Node *&fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(RLINK(p) != Node::NullPtr);
    
    size_t original_count = COUNT(p);

    Node *fSucc  = p;
    Node *succ   = gotoRightAndColorRed(p, fp); 
    Node *ffSucc = fp;
    
    while (LLINK(succ) != Node::NullPtr)
      {
        ffSucc = fSucc;
        fSucc  = succ;
        succ   = gotoLeftAndColorRed(fSucc, ffSucc);
      }

    if (LLINK(fp) == p)
      LLINK(fp) = succ;
    else
      RLINK(fp) = succ;
    
    LLINK(succ) = LLINK(p);
    LLINK(p)    = Node::NullPtr; 

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

    Color tmp   = COLOR(succ);
    COLOR(succ) = COLOR(p); 
    COLOR(p)    = tmp;
    
    // Preserve count at successor's new position
    COUNT(succ) = original_count;
  }

  /// Find predecessor and swap for deletion
  static void findPredAndSwap(Node *p, Node *&fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) != Node::NullPtr);
    
    size_t original_count = COUNT(p);

    Node *fPred  = p;
    Node *pred   = gotoLeftAndColorRed(p, fp);
    Node *ffPred = fp;
    
    while (RLINK(pred) != Node::NullPtr)
      {
        ffPred = fPred;
        fPred  = pred;
        pred   = gotoRightAndColorRed(fPred, ffPred);
      }

    if (LLINK(fp) == p)
      LLINK(fp) = pred;
    else
      RLINK(fp) = pred;
    
    RLINK(pred) = RLINK(p);
    RLINK(p)    = Node::NullPtr; 

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

    Color tmp   = COLOR(pred);
    COLOR(pred) = COLOR(p); 
    COLOR(p)    = tmp;
    
    COUNT(pred) = original_count;
  }

  /// Color root red if safe
  void colorRootAsRed() noexcept
  {
    if (COLOR(root) == RED)
      return;

    if (COLOR(LLINK(root)) == BLACK and COLOR(RLINK(root)) == BLACK)
      COLOR(root) = RED;      
  }

  /// Search for key while coloring nodes red for deletion.
  /// Also collects visited nodes in stack for count updates.
  Node* searchAndColorRed(const Key& key, Node *&fp,
                          Node** stack, size_t& stack_len) noexcept
  {
    Node *p   = root;
    fp        = head;
    Node *ffp = fHead;
    
    stack_len = 0;

    colorRootAsRed();
    stack[stack_len++] = p;

    while (true)
      {
        if (equals(key, KEY(p)))
          return p;

        ffp = fp;
        fp  = p;

        if (less(key, KEY(p)))
          {
            if (LLINK(p) == Node::NullPtr)
              return p;  // Key not found, return current
            p = gotoLeftAndColorRed(fp, ffp);
          }
        else
          {
            if (RLINK(p) == Node::NullPtr)
              return p;  // Key not found, return current
            p = gotoRightAndColorRed(fp, ffp);
          }
        
        stack[stack_len++] = p;
      }
  }

  /// Remove node, making it a red leaf
  static void removeAndRendLeafRed(Node *p, Node *fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(fp != Node::NullPtr);

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
    COUNT(Node::NullPtr) = 0;
    COLOR(head) = BLACK;
    COUNT(head) = 0;
  }

public:
  /// Default constructor
  GenTdRbTreeRk() noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()), cmp()
  {
    init();
  }

  /// Constructor with comparator
  explicit GenTdRbTreeRk(const Compare & __cmp) noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()), cmp(__cmp)
  {
    init();
  }

  /// Move constructor
  GenTdRbTreeRk(GenTdRbTreeRk && other) noexcept
    : head(&headNode), fHead(&headParent), root(headNode.getR()),
      cmp(std::move(other.cmp))
  {
    init();
    root = other.root;
    other.root = Node::NullPtr;
  }

  /// Move assignment
  GenTdRbTreeRk & operator=(GenTdRbTreeRk && other) noexcept
  {
    if (this != &other)
      {
        root = other.root;
        cmp = std::move(other.cmp);
        other.root = Node::NullPtr;
      }
    return *this;
  }

  GenTdRbTreeRk(const GenTdRbTreeRk &) = delete;
  GenTdRbTreeRk & operator=(const GenTdRbTreeRk &) = delete;

  /// Reset tree (does not free nodes)
  void reset() noexcept
  {
    root = Node::NullPtr;
  }

  /// Swap with another tree
  void swap(GenTdRbTreeRk & other) noexcept
  {
    std::swap(root, other.root);
    std::swap(cmp, other.cmp);
  }

  virtual ~GenTdRbTreeRk() = default;

  /// Get number of nodes O(1)
  size_t size() const noexcept { return COUNT(root); }

  /// Check if empty
  bool is_empty() const noexcept { return root == Node::NullPtr; }

  /// Get comparator
  Compare & get_compare() noexcept { return cmp; }
  const Compare & get_compare() const noexcept { return cmp; }

  /** @brief Insert a node.
      @return Pointer to inserted node, or nullptr if duplicate.
   */
  Node* insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr)
      {
        root = p;
        return p;
      }

    return searchFlipColorsAndInsert(p);
  }

  /** @brief Insert a node allowing duplicates.
      @return Pointer to inserted node.
   */
  Node* insert_dup(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr)
      {
        root = p;
        return p;
      }

    return searchFlipColorsAndInsertDup(p);
  }

  /** @brief Search or insert.
      @return Inserted node or existing node with same key.
   */
  Node* search_or_insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr)
      {
        root = p;
        return p;
      }

    Node *found = search(KEY(p));
    if (found != nullptr)
      return found;

    return searchFlipColorsAndInsert(p);
  }

  /** @brief Search for a key.
      @return Pointer to node, or nullptr if not found.
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

  /** @brief Remove node with given key.
      @return Pointer to removed node, or nullptr if not found.
      @note Count update uses stack collected during search - O(log n) amortized,
            but may need full recalc after complex swaps.
   */
  Node* remove(const Key& key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;
    
    Node* stack[128];
    size_t stack_len = 0;
      
    Node *fp;
    Node *p = searchAndColorRed(key, fp, stack, stack_len);

    if (not equals(KEY(p), key))
      return nullptr;

    removeAndRendLeafRed(p, fp);
    
    // Recalculate counts for visited nodes - usually O(log n)
    // For complex cases with many swaps, fall back to full recalc
    if (root != Node::NullPtr)
      {
        // The stack contains nodes visited during search
        // After swaps, some may have moved, so do full recalc for safety
        updateCountRec(root);
      }

    return p;
  }

  /// Get root pointer
  Node*& getRoot() noexcept { return root; }
  Node* getRoot() const noexcept { return root; }

  // ===================== RANK OPERATIONS =====================

  /** @brief Select i-th node in order.
      @param i Zero-based position.
      @return Pointer to i-th node.
      @throw out_of_range if i >= size().
   */
  Node* select(size_t i) const
  {
    return Aleph::select(root, i);
  }

  /** @brief Find position of a key.
      @return Pair of (position, node pointer). Position is -1 if not found.
   */
  std::pair<long, Node*> position(const Key & key) const noexcept
  {
    std::pair<long, Node*> ret_val;
    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
      inorder_position(root, key, ret_val.second);
    return ret_val;
  }

  /** @brief Find position of a key (even if not in tree).
      @return Pair of (position, node pointer).
   */
  std::pair<long, Node*> find_position(const Key & key) const noexcept
  {
    std::pair<long, Node*> r(-2, nullptr);
    r.first = BinTreeXt_Operation<Node, Compare>(cmp).
      find_position(root, key, r.second);
    return r;
  }

  /** @brief Remove node at position i.
      @param i Zero-based position.
      @return Pointer to removed node.
   */
  Node* remove_pos(size_t i)
  {
    Node* p = select(i);
    if (p == nullptr)
      return nullptr;
    return remove(KEY(p));
  }

  /** @brief Split tree by position.
      @param pos Split position.
      @param t1 Receives nodes [0, pos).
      @param t2 Receives nodes [pos, size).
   */
  void split_pos(size_t pos, GenTdRbTreeRk & t1, GenTdRbTreeRk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return;

    if (pos == 0)
      {
        t2.root = root;
        root = Node::NullPtr;
        return;
      }

    if (pos >= size())
      {
        t1.root = root;
        root = Node::NullPtr;
        return;
      }

    // Simple O(n) implementation
    size_t count = 0;
    split_pos_inorder(root, pos, t1, t2, count);
    root = Node::NullPtr;
  }

private:
  void split_pos_inorder(Node *p, size_t pos, 
                         GenTdRbTreeRk & t1, GenTdRbTreeRk & t2,
                         size_t & count) noexcept
  {
    if (p == Node::NullPtr)
      return;

    Node *left = LLINK(p);
    Node *right = RLINK(p);

    split_pos_inorder(left, pos, t1, t2, count);

    p->reset();
    if (count < pos)
      t1.insert(p);
    else
      t2.insert(p);
    ++count;

    split_pos_inorder(right, pos, t1, t2, count);
  }

public:
  /// Verify tree properties
  bool verify() const noexcept
  {
    return is_red_black_bst_rk(root, cmp) and check_rank_tree(root);
  }

  /// Iterator
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(GenTdRbTreeRk & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    Iterator(const GenTdRbTreeRk & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};

/** @brief Top-down red-black tree with rank (no virtual destructor).
    @tparam Key Key type.
    @tparam Compare Comparison functor.
 */
template <typename Key, class Compare = std::less<Key>>
class TdRbTreeRk : public GenTdRbTreeRk<RbNodeRk, Key, Compare>
{
  using Base = GenTdRbTreeRk<RbNodeRk, Key, Compare>;
public:
  using Base::Base;
};

/** @brief Top-down red-black tree with rank (virtual destructor).
    @tparam Key Key type.
    @tparam Compare Comparison functor.
 */
template <typename Key, class Compare = std::less<Key>>
class TdRbTreeRkVtl : public GenTdRbTreeRk<RbNodeRkVtl, Key, Compare>
{
  using Base = GenTdRbTreeRk<RbNodeRkVtl, Key, Compare>;
public:
  using Base::Base;
};

} // namespace Aleph

# endif /* TPL_TDRBTREERK_H */

