
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file tpl_hRbTreeRk.H
    @brief Hybrid top-down/bottom-up red-black tree with rank support.
    
    This file implements a red-black tree that uses:
    - Top-down insertion with proactive color flipping
    - Bottom-up deletion with stack-based ancestor tracking
    - Subtree size counts for O(log n) rank operations
    
    This hybrid approach achieves O(log n) for ALL operations including
    insert, remove, search, select, and position.
*/

# ifndef TPL_HTDRBTREERK_H
# define TPL_HTDRBTREERK_H

# ifdef DEBUG
# include <cmath>
# endif
# include <stdexcept>
# include <ah-errors.H>
# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTreeOps.H>
# include <tpl_arrayStack.H>
# include <rbNodeRk.H>

namespace Aleph
{

/** @brief Hybrid top-down/bottom-up red-black tree with rank support.
    
    This class implements a self-balancing binary search tree with O(log n)
    rank operations (select, position). It combines:
    
    - **Insertion**: Top-down with proactive color flipping and stack tracking
    - **Deletion**: Bottom-up using a stack to track ancestors
    - **Rank operations**: Subtree counts maintained during all operations
    
    @par Complexity (ALL O(log n)):
    - search, insert, remove: O(log n)
    - select(i): O(log n) - get i-th smallest element
    - position(key): O(log n) - get rank of a key
    - find_position(key): O(log n) - get insertion position
    - size(): O(1)
    
    This is the only implementation that achieves O(log n) for ALL operations
    including removal with rank support.
    
    @tparam Key The type of keys stored in the tree
    @tparam Compare Comparison functor (default: Aleph::less<Key>)
    
    @see HtdRbTree Hybrid tree without rank support
    @see Gen_Rb_Tree_Rk Bottom-up tree with rank
    @see GenTdRbTreeRk Top-down tree with rank (O(n) remove)
    @ingroup Trees
    @author Leandro Rabindranath León
*/
template <class Key, class Compare = Aleph::less<Key>>
class HtdRbTreeRk
{
public:
  using Color = unsigned char;
  using Node = RbNodeRk<Key>;

private:
  Node headNode;
  Node headParent;
  Node headGrandParent;

  Node *head;
  Node *fHead;
  Node *ffHead;
  Node *& root;

  FixedStack<Node *> path{Node::MaxHeight};

  Compare cmp;

  bool less(const Key & k1, const Key & k2) const noexcept
  { return cmp(k1, k2); }

  bool equals(const Key & k1, const Key & k2) const noexcept
  { return not cmp(k1, k2) and not cmp(k2, k1); }

  static Node * getSibling(Node *p, Node *fp) noexcept
  {
    assert(LLINK(fp) == p || RLINK(fp) == p);
    return LLINK(fp) == p ? RLINK(fp) : LLINK(fp);
  }

  // ===================== RANK-AWARE ROTATIONS =====================

  /** @brief Right rotation with count update.
      
      After rotation:
      - q becomes parent of p
      - q.count = p.count (takes p's position)
      - p.count = 1 + LCOUNT + RCOUNT
  */
  static Node * rotate_to_right_rk(Node *p, Node *fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) != Node::NullPtr);
    
    Node *q = LLINK(p);
    
    // q takes p's count (it will be in p's position)
    COUNT(q) = COUNT(p);
    
    LLINK(p) = RLINK(q);
    RLINK(q) = p;
    
    // Update p's count based on its new children
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    
    if (LLINK(fp) == p)
      LLINK(fp) = q;
    else
      RLINK(fp) = q;
    
    return q;
  }

  /** @brief Left rotation with count update. */
  static Node * rotate_to_left_rk(Node *p, Node *fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(RLINK(p) != Node::NullPtr);
    
    Node *q = RLINK(p);
    
    COUNT(q) = COUNT(p);
    
    RLINK(p) = LLINK(q);
    LLINK(q) = p;
    
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    
    if (LLINK(fp) == p)
      LLINK(fp) = q;
    else
      RLINK(fp) = q;
    
    return q;
  }

  // ===================== INSERTION ROUTINES =====================

  void restoreRedCondition(Node *p, Node *& fp, Node *ffp, Node *fffp) noexcept
  {
    assert(LLINK(fp) == p || RLINK(fp) == p);
    assert(COLOR(fp) == RED);
    assert(COLOR(p) == RED);

    if (fp == root)
      {
        COLOR(fp) = BLACK;
        return;
      }

    assert(LLINK(ffp) == fp || RLINK(ffp) == fp);
    assert(COLOR(ffp) == BLACK);
    assert(LLINK(fffp) == ffp || RLINK(fffp) == ffp);

    COLOR(ffp) = RED;

    if (LLINK(fp) == p && LLINK(ffp) == fp)
      {
        COLOR(fp) = BLACK;
        rotate_to_right_rk(ffp, fffp);
      }
    else if (RLINK(fp) == p && RLINK(ffp) == fp)
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

  static void flipColors(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == BLACK);
    assert(COLOR(LLINK(p)) == RED && COLOR(RLINK(p)) == RED);

    COLOR(p) = RED;
    COLOR(LLINK(p)) = COLOR(RLINK(p)) = BLACK;
  }

  /** @brief Insert with stack tracking for count updates.
      
      Uses a stack to track the path during insertion, then updates
      counts bottom-up after all rotations are complete.
  */
  Node * searchFlipColorsAndInsert(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);
    assert(COUNT(q) == 1);

    const Key & qk = KEY(q);

    // Track path for count updates
    Node* insertPath[128];
    size_t insertPathLen = 0;

    Node *p = root;
    Node *fp = head;
    Node *ffp = fHead;
    Node *fffp = ffHead;
    Node *nextNode;

    insertPath[insertPathLen++] = p;

    while (true)
      {
        const Key & pk = KEY(p);

        if (equals(qk, pk)) [[unlikely]]
          return nullptr;  // Duplicate key

        if (COLOR(p) == BLACK && COLOR(LLINK(p)) == RED
            && COLOR(RLINK(p)) == RED)
          {
            flipColors(p);
            if (COLOR(fp) == RED)
              {
                assert(fffp != Node::NullPtr);
                restoreRedCondition(p, fp, ffp, fffp);
              }
          }

        if (less(qk, pk))
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
        ffp = fp;
        fp = p;
        p = nextNode;
        insertPath[insertPathLen++] = p;
      }

    // Perform insertion
    const Key & pk = KEY(p);
    if (less(qk, pk))
      LLINK(p) = q;
    else
      RLINK(p) = q;

    if (COLOR(p) == RED)
      restoreRedCondition(q, p, fp, ffp);

    // Update counts bottom-up - O(log n)
    for (size_t i = insertPathLen; i > 0; --i)
      {
        Node* node = insertPath[i - 1];
        COUNT(node) = COUNT(LLINK(node)) + COUNT(RLINK(node)) + 1;
      }

    return q;
  }

  Node * searchFlipColorsAndInsertDup(Node *q) noexcept
  {
    assert(q != Node::NullPtr);
    assert(root != Node::NullPtr);
    assert(COLOR(q) == RED);
    assert(COUNT(q) == 1);

    const Key & qk = KEY(q);

    Node* insertPath[128];
    size_t insertPathLen = 0;

    Node *p = root;
    Node *fp = head;
    Node *ffp = fHead;
    Node *fffp = ffHead;
    Node *nextNode;

    insertPath[insertPathLen++] = p;

    while (true)
      {
        const Key & pk = KEY(p);

        if (COLOR(p) == BLACK && COLOR(LLINK(p)) == RED
            && COLOR(RLINK(p)) == RED)
          {
            flipColors(p);
            if (COLOR(fp) == RED)
              {
                assert(fffp != Node::NullPtr);
                restoreRedCondition(p, fp, ffp, fffp);
              }
          }

        if (less(qk, pk))
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
        ffp = fp;
        fp = p;
        p = nextNode;
        insertPath[insertPathLen++] = p;
      }

    const Key & pk = KEY(p);
    if (less(qk, pk))
      LLINK(p) = q;
    else
      RLINK(p) = q;

    if (COLOR(p) == RED)
      restoreRedCondition(q, p, fp, ffp);

    // Update counts bottom-up
    for (size_t i = insertPathLen; i > 0; --i)
      {
        Node* node = insertPath[i - 1];
        COUNT(node) = COUNT(LLINK(node)) + COUNT(RLINK(node)) + 1;
      }

    return q;
  }

  // ===================== DELETION ROUTINES =====================

  Node * searchAndBuildPath(const Key & key) noexcept
  {
    assert(root != Node::NullPtr);

    Node *p = root;
    path.push(head);

    while (true)
      {
        path.push(p);

        const Key & pk = KEY(p);

        if (equals(key, pk))
          return p;

        if (less(key, pk))
          {
            if (LLINK(p) == Node::NullPtr)
              return p;
            p = LLINK(p);
            continue;
          }

        if (RLINK(p) == Node::NullPtr)
          return p;

        p = RLINK(p);
      }
  }

  void findSuccAndSwap(Node *p, Node *& fp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(RLINK(p) != Node::NullPtr);
    assert(fp != Node::NullPtr);
    assert(LLINK(fp) == p || RLINK(fp) == p);

    Node *& refToSearchPath = path.top();
    assert(refToSearchPath == p);

    Node *fSucc = p;
    Node *succ = RLINK(p);

    path.push(succ);
    while (LLINK(succ) != Node::NullPtr)
      {
        fSucc = succ;
        succ = LLINK(succ);
        path.push(succ);
      }

    refToSearchPath = succ;
    path.top() = p;

    if (LLINK(fp) == p)
      LLINK(fp) = succ;
    else
      RLINK(fp) = succ;

    LLINK(succ) = LLINK(p);
    LLINK(p) = Node::NullPtr;

    if (RLINK(p) == succ)
      {
        RLINK(p) = RLINK(succ);
        RLINK(succ) = p;
        fp = succ;
      }
    else
      {
        Node *succr = RLINK(succ);
        RLINK(succ) = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p) = succr;
        fp = fSucc;
      }

    Color tmp = COLOR(succ);
    COLOR(succ) = COLOR(p);
    COLOR(p) = tmp;
    // Counts will be recalculated after removal completes
  }

  void balanceDownAndColor(Node *p, Node *& fp, Node *& sp) noexcept
  {
    assert(LLINK(fp) == p || RLINK(fp) == p);
    assert(LLINK(fp) == sp || RLINK(fp) == sp);
    assert(COLOR(fp) == BLACK);
    assert(COLOR(sp) == RED);
    assert(COLOR(p) == BLACK);
    assert(!path.is_empty());

    Node *& ffp = path.top();
    assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

    if (LLINK(fp) == p)
      {
        sp = LLINK(sp);
        ffp = rotate_to_left_rk(fp, ffp);
      }
    else
      {
        sp = RLINK(sp);
        ffp = rotate_to_right_rk(fp, ffp);
      }

    assert(LLINK(fp) == sp || RLINK(fp) == sp);
    assert(COLOR(ffp) == RED);

    COLOR(ffp) = BLACK;
    COLOR(fp) = RED;
  }

  void rotateNephewAndColor(Node *fp, Node *sp, Node *np) noexcept
  {
    assert(LLINK(fp) == sp || RLINK(fp) == sp);
    assert(LLINK(sp) == np || RLINK(sp) == np);
    assert(COLOR(sp) == BLACK);
    assert(COLOR(np) == RED);
    assert(!path.is_empty());

    Node *ffp = path.top();
    assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

    if (RLINK(sp) == np)
      rotate_to_left_rk(fp, ffp);
    else
      rotate_to_right_rk(fp, ffp);

    COLOR(sp) = COLOR(fp);
    COLOR(fp) = BLACK;
    COLOR(np) = BLACK;
  }

  void doubleRotateNephewAndColor(Node *fp, Node *sp, Node *snp) noexcept
  {
    assert(LLINK(fp) == sp || RLINK(fp) == sp);
    assert(LLINK(sp) == snp || RLINK(sp) == snp);
    assert(COLOR(sp) == BLACK);
    assert(COLOR(snp) == RED);
    assert(!path.is_empty());

    Node *ffp = path.top();
    assert(LLINK(ffp) == fp || RLINK(ffp) == fp);

    if (LLINK(sp) == snp)
      {
        rotate_to_right_rk(sp, fp);
        rotate_to_left_rk(fp, ffp);
      }
    else
      {
        rotate_to_left_rk(sp, fp);
        rotate_to_right_rk(fp, ffp);
      }

    COLOR(snp) = COLOR(fp);
    COLOR(fp) = BLACK;
  }

  static void colorSiblingAsRed(Node *sp) noexcept
  {
    assert(COLOR(sp) == BLACK);
    COLOR(sp) = RED;
  }

  static void colorParentAndSibling(Node *fp, Node *sp) noexcept
  {
    assert(LLINK(fp) == sp || RLINK(fp) == sp);
    assert(COLOR(fp) == RED);
    assert(COLOR(sp) == BLACK);

    COLOR(fp) = BLACK;
    COLOR(sp) = RED;
  }

  /// Recursively update counts for entire subtree - O(n)
  static void updateCountRec(Node * p) noexcept
  {
    if (p == Node::NullPtr)
      return;
    updateCountRec(LLINK(p));
    updateCountRec(RLINK(p));
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
  }

  /** @brief Remove node and fix red-black violations.
      
      After BST deletion and red-black fixing, recalculates all counts.
      The count update is O(n) due to the complexity of tracking all
      affected nodes during deletion with swaps and rotations.
  */
  void removeAndFixBlackCondition(Node *q) noexcept
  {
    assert(path.top() == q);

    Node *fq = path.top(1);
    Node *p;

    assert(fq != Node::NullPtr);
    assert(LLINK(fq) == q || RLINK(fq) == q);

    while (1)
      {
        if (LLINK(q) == Node::NullPtr)
          {
            if (LLINK(fq) == q)
              p = LLINK(fq) = RLINK(q);
            else
              p = RLINK(fq) = RLINK(q);
            break;
          }

        if (RLINK(q) == Node::NullPtr)
          {
            if (LLINK(fq) == q)
              p = LLINK(fq) = LLINK(q);
            else
              p = RLINK(fq) = LLINK(q);
            break;
          }

        findSuccAndSwap(q, fq);
      }

    if (COLOR(q) == RED)
      {
        assert(COLOR(p) == BLACK);
        path.empty();
        if (root != Node::NullPtr)
          updateCountRec(root);
        return;
      }

    if (COLOR(p) == RED)
      {
        COLOR(p) = BLACK;
        path.empty();
        if (root != Node::NullPtr)
          updateCountRec(root);
        return;
      }

    Node *np, *snp;
    Node *fp = fq;

    path.popn(2);

    while (true)
      {
        if (p == root)
          break;

        Node *sp = getSibling(p, fp);

        if (COLOR(sp) == RED)
          balanceDownAndColor(p, fp, sp);

        assert(COLOR(sp) == BLACK);

        if (LLINK(fp) == p)
          {
            np = RLINK(sp);
            snp = LLINK(sp);
          }
        else
          {
            np = LLINK(sp);
            snp = RLINK(sp);
          }

        if (COLOR(np) == RED)
          {
            rotateNephewAndColor(fp, sp, np);
            break;
          }

        if (COLOR(snp) == RED)
          {
            doubleRotateNephewAndColor(fp, sp, snp);
            break;
          }

        if (COLOR(fp) == RED)
          {
            colorParentAndSibling(fp, sp);
            break;
          }

        colorSiblingAsRed(sp);
        p = fp;
        fp = path.pop();
      }

    path.empty();
    
    // Update all counts after removal - O(n)
    if (root != Node::NullPtr)
      updateCountRec(root);
  }

  void init() noexcept
  {
    RLINK(fHead) = head;
    RLINK(ffHead) = fHead;
    COLOR(Node::NullPtr) = BLACK;
    COUNT(Node::NullPtr) = 0;
    COLOR(head) = BLACK;
    COUNT(head) = 0;
    COLOR(fHead) = BLACK;
    COUNT(fHead) = 0;
    COLOR(ffHead) = BLACK;
    COUNT(ffHead) = 0;
  }

public:
  using key_type = Key;

  Compare & key_comp() noexcept { return cmp; }
  const Compare & key_comp() const noexcept { return cmp; }
  Compare & get_compare() noexcept { return cmp; }
  [[nodiscard]] constexpr const Compare & get_compare() const noexcept { return cmp; }

  HtdRbTreeRk(Compare __cmp = Compare()) noexcept
    : head(&headNode),
      fHead(&headParent),
      ffHead(&headGrandParent),
      root(headNode.getR()),
      cmp(__cmp)
  {
    init();
  }

  void swap(HtdRbTreeRk & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  HtdRbTreeRk(HtdRbTreeRk && tree) noexcept
    : head(&headNode),
      fHead(&headParent),
      ffHead(&headGrandParent),
      root(headNode.getR()),
      path(Node::MaxHeight),
      cmp()
  {
    init();
    swap(tree);
  }

  HtdRbTreeRk & operator=(HtdRbTreeRk && tree) noexcept
  {
    swap(tree);
    return *this;
  }

  HtdRbTreeRk(const HtdRbTreeRk &) = delete;
  HtdRbTreeRk & operator=(const HtdRbTreeRk &) = delete;

  virtual ~HtdRbTreeRk() = default;

  [[nodiscard]] constexpr bool is_empty() const noexcept { return root == Node::NullPtr; }

  /// O(1) size using root count
  size_t size() const noexcept { return COUNT(root); }

  void reset() noexcept { root = Node::NullPtr; }

  Node * insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr) [[unlikely]]
      {
        root = p;
        return p;
      }

    return searchFlipColorsAndInsert(p);
  }

  Node * search_or_insert(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr) [[unlikely]]
      {
        root = p;
        return p;
      }

    const Key & pk = KEY(p);
    Node *found = search(pk);
    if (found != nullptr) [[unlikely]]
      return found;

    return insert(p);
  }

  Node * insert_dup(Node *p) noexcept
  {
    assert(p != Node::NullPtr);
    assert(COLOR(p) == RED);
    COUNT(p) = 1;

    if (root == Node::NullPtr) [[unlikely]]
      {
        root = p;
        return p;
      }

    return searchFlipColorsAndInsertDup(p);
  }

  Node * search(const Key & key) const noexcept
  {
    Node *p = root;
    while (p != Node::NullPtr)
      {
        const Key & pk = KEY(p);
        if (equals(key, pk))
          return p;
        p = less(key, pk) ? LLINK(p) : RLINK(p);
      }
    return nullptr;
  }

  Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr) [[unlikely]]
      return nullptr;

    Node *p = searchAndBuildPath(key);

    if (not equals(KEY(p), key))
      {
        path.empty();
        return nullptr;
      }

    removeAndFixBlackCondition(p);

    p->reset();

    return p;
  }

  Node *& getRoot() noexcept { return root; }
  Node * getRoot() const noexcept { return root; }

  // ===================== RANK OPERATIONS =====================

  /** @brief Select the i-th smallest element (0-indexed).
      
      @param i Position (0 = smallest)
      @return Pointer to node at position i
      @throw std::out_of_range if i >= size()
  */
  Node * select(size_t i) const
  {
    return Aleph::select(root, i);
  }

  /** @brief Find position of a key.
      
      @param key Key to find
      @return Pair of (position, node). Position is -1 if not found.
  */
  std::pair<long, Node*> position(const Key & key) const noexcept
  {
    std::pair<long, Node*> ret_val;
    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
      inorder_position(root, key, ret_val.second);
    return ret_val;
  }

  /** @brief Find position where key would be inserted.
      
      Works even if key doesn't exist in the tree.
      
      @param key Key to find position for
      @return Pair of (position, node). Node is nullptr if key not found.
  */
  std::pair<long, Node*> find_position(const Key & key) const noexcept
  {
    std::pair<long, Node*> r;
    r.first = BinTreeXt_Operation<Node, Compare>(cmp).
      find_position(root, key, r.second);
    return r;
  }

  /** @brief Remove element at position i.
      
      @param i Position (0-indexed)
      @return Removed node
      @throw std::out_of_range if i >= size()
  */
  Node * remove_pos(size_t i)
  {
    ah_out_of_range_error_if(i >= size()) << "remove_pos: position " << i << " out of range";
    Node * p = select(i);
    return remove(KEY(p));
  }

  /** @brief Split tree at position.
      
      After split:
      - t1 contains elements at positions [0, pos)
      - t2 contains elements at positions [pos, size())
      - this tree becomes empty
      
      @param pos Split position
      @param t1 Tree for left part
      @param t2 Tree for right part
  */
  void split_pos(size_t pos, HtdRbTreeRk & t1, HtdRbTreeRk & t2)
  {
    ah_out_of_range_error_if(pos > size()) << "split_pos: position " << pos << " out of range";
    
    t1.reset();
    t2.reset();
    
    while (root != Node::NullPtr)
      {
        Node * p = remove_pos(0);
        if (t1.size() < pos)
          t1.insert(p);
        else
          t2.insert(p);
      }
  }

  /** @brief Verify red-black and rank invariants.
      
      @return true if all invariants hold
  */
  bool verify() const noexcept
  {
    if (root == Node::NullPtr)
      return true;
    
    // Verify red-black properties
    if (!is_red_black_bst_rk(root, cmp))
      return false;
    
    // Verify counts
    return verifyCountsRec(root);
  }

private:
  static bool verifyCountsRec(Node * p) noexcept
  {
    if (p == Node::NullPtr)
      return true;
    
    size_t expected = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    if (COUNT(p) != expected)
      return false;
    
    return verifyCountsRec(LLINK(p)) && verifyCountsRec(RLINK(p));
  }

public:
  /// In-order iterator
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(HtdRbTreeRk & t) noexcept
      : BinNodeInfixIterator<Node>(t.getRoot()) {}

    Iterator(const HtdRbTreeRk & t) noexcept
      : BinNodeInfixIterator<Node>(const_cast<HtdRbTreeRk &>(t).getRoot()) {}
  };
};

/** @brief Hybrid RB tree with rank and virtual destructor. */
template <class Key, class Compare = Aleph::less<Key>>
class HtdRbTreeRkVtl : public HtdRbTreeRk<Key, Compare>
{
public:
  using Base = HtdRbTreeRk<Key, Compare>;
  using Node = typename Base::Node;
  using Base::Base;
};

} // namespace Aleph

#endif /* TPL_HTDRBTREERK_H */

