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

/** @file tpl_rbRk.H
 *  @brief Red-Black tree with rank (order statistics).
 *
 *  Bottom-up RB tree augmented with subtree sizes for
 *  order statistics queries.
 *
 *  ## Features
 *  - select(k): O(log n) k-th element
 *  - position(x): O(log n) rank of element
 *  - split_pos(): Split at position
 *
 *  @see tpl_rb_tree.H Base RB tree
 *  @see tpl_avlRk.H AVL with rank
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RBRK_H
# define TPL_RBRK_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTreeOps.H>
# include <rbNodeRk.H>
# include <ah-errors.H>

namespace Aleph {

/** @brief Red-black tree with rank support (select/position operations).

    This class extends the standard red-black tree with subtree size counts,
    enabling efficient O(log n) rank-based operations:
    
    - **select(i)**: Get the i-th smallest element (0-indexed)
    - **position(key)**: Get the rank/position of a key
    - **find_position(key)**: Get insertion position even if key doesn't exist
    - **split_pos(pos, t1, t2)**: Split tree at a position
    - **remove_pos(i)**: Remove element at position i
    
    @par How it works:
    Each node stores a `count` field representing the number of nodes in its
    subtree (including itself). Rotations update these counts to maintain the
    invariant. This adds O(1) overhead per rotation but enables O(log n) ranked
    operations.
    
    @par Complexity:
    - All basic operations (search, insert, remove): O(log n)
    - Rank operations (select, position): O(log n)
    - Space: O(n) + O(log n) for stack during operations
    
    @par Example:
    @code
    Rb_Tree_Rk<int> tree;
    for (int i = 0; i < 100; ++i)
        tree.insert(new Rb_Tree_Rk<int>::Node(i));
    
    // Get 50th smallest element
    auto node = tree.select(49);  // 0-indexed, returns node with key 49
    
    // Get position of key 42
    auto [pos, found] = tree.position(42);  // pos = 42, found points to node
    @endcode
    
    @tparam NodeType Node template (RbNodeRk or RbNodeRkVtl).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys.
    
    @see Gen_Rb_Tree Basic red-black tree without rank support.
    @see GenTdRbTreeRk Top-down red-black with rank.
    @ingroup Trees
*/
template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rb_Tree_Rk
{
public:

  typedef NodeType<Key> Node;

private:

  Node head_node;
  Node * head;
  Node *& root;
  FixedStack<Node *> rb_stack;
  Compare cmp;
  static constexpr signed char CmpLess = -1;
  static constexpr signed char CmpEqual = 0;
  static constexpr signed char CmpGreater = 1;

  Node * search_and_stack_rb(const Key & key,
                             signed char & cmp_result)
  {
    Node * p = root;
    rb_stack.push(head);
    cmp_result = CmpEqual;
    do
      {
        rb_stack.push(p);
        const Key & pk = KEY(p);
        if (cmp(key, pk))
          {
            cmp_result = CmpLess;
            p = LLINK(p);
          }
        else if (cmp(pk, key))
          {
            cmp_result = CmpGreater;
            p = RLINK(p);
          }
        else
          {
            cmp_result = CmpEqual;
            return p;
          }
      }
    while (p != Node::NullPtr);

    return rb_stack.top();
  }

  Node * search_dup_and_stack_rb(const Key & key,
                                 signed char & cmp_result)
  {
    Node * p = root;
    rb_stack.push(head);
    cmp_result = CmpEqual;
    do
      {
        rb_stack.push(p);
        const Key & pk = KEY(p);
        if (cmp(key, pk))
          {
            cmp_result = CmpLess;
            p = LLINK(p);
          }
        else
          {
            cmp_result = CmpGreater;
            p = RLINK(p);
          }
      }
    while (p != Node::NullPtr);

    return rb_stack.top();
  }

  // Rotate to right with counter update
  Node * rotate_to_right_rk(Node * p, Node * pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    assert(LLINK(pp) == p or RLINK(pp) == p);

    Node * q = LLINK(p);
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

  // Rotate to left with counter update
  Node * rotate_to_left_rk(Node * p, Node * pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    assert(LLINK(pp) == p or RLINK(pp) == p);

    Node * q = RLINK(p);
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

  void fix_red_condition(Node * p)
  {
    assert(COLOR(p) == RED);

    while (p != root)
      {
        Node * pp = rb_stack.pop();
        if (COLOR(pp) == BLACK)
          break;

        if (root == pp)
          {
            COLOR(root) = BLACK;
            break;
          }

        Node * ppp = rb_stack.pop();
        Node * spp = LLINK(ppp) == pp ? RLINK(ppp) : LLINK(ppp);

        if (COLOR(spp) == RED)
          {
            COLOR(ppp) = RED;
            COLOR(pp) = BLACK;
            COLOR(spp) = BLACK;
            p = ppp;
            continue;
          }

        Node * pppp = rb_stack.pop();
        if (LLINK(pp) == p and LLINK(ppp) == pp)
          {
            rotate_to_right_rk(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else if (RLINK(pp) == p and RLINK(ppp) == pp)
          {
            rotate_to_left_rk(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else
          {
            if (RLINK(pp) == p)
              {
                rotate_to_left_rk(pp, ppp);
                rotate_to_right_rk(ppp, pppp);
              }
            else
              {
                rotate_to_right_rk(pp, ppp);
                rotate_to_left_rk(ppp, pppp);
              }
            COLOR(p) = BLACK;
          }

        COLOR(ppp) = RED;
        break;
      }

    rb_stack.empty();
  }

  void find_succ_and_swap(Node * p, Node *& pp)
  {
    Node *& ref_rb_stack = rb_stack.top();

    Node * fSucc = p;
    Node * succ = RLINK(p);
    rb_stack.push(succ);

    while (LLINK(succ) != Node::NullPtr)
      {
        fSucc = succ;
        succ = LLINK(succ);
        rb_stack.push(succ);
      }

    ref_rb_stack = succ;
    rb_stack.top() = p;

    if (LLINK(pp) == p)
      LLINK(pp) = succ;
    else
      RLINK(pp) = succ;

    LLINK(succ) = LLINK(p);
    LLINK(p) = Node::NullPtr;

    if (RLINK(p) == succ)
      {
        RLINK(p) = RLINK(succ);
        RLINK(succ) = p;
        pp = succ;
      }
    else
      {
        Node * succr = RLINK(succ);
        RLINK(succ) = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p) = succr;
        pp = fSucc;
      }

    std::swap(COLOR(succ), COLOR(p));
    COUNT(succ) = COUNT(p);  // Copy counter to successor position
  }

  void fix_black_condition(Node * p)
  {
    if (COLOR(p) == RED)
      {
        COLOR(p) = BLACK;
        return;
      }

    Node * pp = rb_stack.popn(2);
    while (p != root)
      {
        assert(LLINK(pp) == p or RLINK(pp) == p);
        assert(LLINK(rb_stack.top()) == pp or RLINK(rb_stack.top()) == pp);

        Node * sp = LLINK(pp) == p ? RLINK(pp) : LLINK(pp);
        if (COLOR(sp) == RED)
          {
            Node *& ppp = rb_stack.top();

            if (LLINK(pp) == p)
              {
                sp = LLINK(sp);
                ppp = rotate_to_left_rk(pp, ppp);
              }
            else
              {
                sp = RLINK(sp);
                ppp = rotate_to_right_rk(pp, ppp);
              }

            COLOR(ppp) = BLACK;
            COLOR(pp) = RED;
          }

        Node * np, * snp;
        if (LLINK(pp) == p)
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
            Node * ppp = rb_stack.top();

            if (RLINK(sp) == np)
              rotate_to_left_rk(pp, ppp);
            else
              rotate_to_right_rk(pp, ppp);

            COLOR(sp) = COLOR(pp);
            COLOR(pp) = BLACK;
            COLOR(np) = BLACK;

            return;
          }

        if (COLOR(snp) == RED)
          {
            Node * ppp = rb_stack.top();

            if (LLINK(sp) == snp)
              {
                rotate_to_right_rk(sp, pp);
                rotate_to_left_rk(pp, ppp);
              }
            else
              {
                rotate_to_left_rk(sp, pp);
                rotate_to_right_rk(pp, ppp);
              }

            COLOR(snp) = COLOR(pp);
            COLOR(pp) = BLACK;

            return;
          }

        if (COLOR(pp) == RED)
          {
            COLOR(pp) = BLACK;
            COLOR(sp) = RED;
            return;
          }

        COLOR(sp) = RED;
        p = pp;
        pp = rb_stack.pop();
      }
  }

  // Update counters along the stack after insertion
  void update_counters_after_insertion() noexcept
  {
    // Stack contains: [head, ..., nodes in path, ..., closest to insertion]
    // top(0) = closest node, top(size-1) = head (don't update)
    const size_t sz = rb_stack.size();
    for (size_t i = 0; i < sz - 1; ++i)
      ++COUNT(rb_stack.top(i));
  }

  // Update counters along the stack after deletion
  void update_counters_after_deletion() noexcept
  {
    const size_t sz = rb_stack.size();
    for (size_t i = 0; i < sz - 1; ++i)
      --COUNT(rb_stack.top(i));
  }

public:

  typedef Key key_type;

  Compare & key_comp() noexcept { return cmp; }

  Compare & get_compare() noexcept { return key_comp(); }

  Gen_Rb_Tree_Rk(Compare cmp_arg = Compare())
    : head(&head_node), root(head_node.getR()),
      rb_stack(Node::MaxHeight), cmp(cmp_arg)
  { /* empty */ }

  void swap(Gen_Rb_Tree_Rk & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  Gen_Rb_Tree_Rk(Gen_Rb_Tree_Rk && tree) noexcept
    : head(&head_node), root(head_node.getR()),
      rb_stack(Node::MaxHeight), cmp(std::move(tree.cmp))
  {
    root = tree.root;
    tree.root = Node::NullPtr;
  }

  Gen_Rb_Tree_Rk & operator = (Gen_Rb_Tree_Rk && tree) noexcept
  {
    if (this != &tree)
      {
        root = tree.root;
        tree.root = Node::NullPtr;
        cmp = std::move(tree.cmp);
      }
    return *this;
  }

  virtual ~Gen_Rb_Tree_Rk() = default;

  Node * search(const Key & key)
  {
    Node * retVal = Aleph::searchInBinTree<Node, Compare>(root, key, cmp);
    return retVal == Node::NullPtr ? nullptr : retVal;
  }

  Node *& getRoot() noexcept { return root; }

  [[nodiscard]] bool is_empty() const noexcept { return root == Node::NullPtr; }

  [[nodiscard]] size_t size() const noexcept { return COUNT(root); }

  Node * insert(Node * p)
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      return root = p;

    signed char cmp_result = CmpEqual;
    Node * q = search_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else if (cmp_result == CmpGreater)
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return nullptr;
      }

    update_counters_after_insertion();
    fix_red_condition(p);

    return p;
  }

  Node * search_or_insert(Node * p)
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      return root = p;

    signed char cmp_result = CmpEqual;
    Node * q = search_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else if (cmp_result == CmpGreater)
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return q;
      }

    update_counters_after_insertion();
    fix_red_condition(p);

    return p;
  }

  Node * insert_dup(Node * p)
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      return root = p;

    signed char cmp_result = CmpEqual;
    Node * q = search_dup_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else
      RLINK(q) = p;

    update_counters_after_insertion();
    fix_red_condition(p);

    return p;
  }

  [[nodiscard]] bool verify() const { return is_red_black_bst_rk(root, cmp) and check_rank_tree(root); }

  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    signed char cmp_result = CmpEqual;
    Node * q = search_and_stack_rb(key, cmp_result);
    if (cmp_result != CmpEqual)
      {
        rb_stack.empty();
        return nullptr;
      }

    Node * pq = rb_stack.top(1);
    Node * p;

    while (true)
      {
        if (LLINK(q) == Node::NullPtr)
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = RLINK(q);
            else
              p = RLINK(pq) = RLINK(q);
            break;
          }

        if (RLINK(q) == Node::NullPtr)
          {
            if (LLINK(pq) == q)
              p = LLINK(pq) = LLINK(q);
            else
              p = RLINK(pq) = LLINK(q);
            break;
          }

        find_succ_and_swap(q, pq);
      }

    update_counters_after_deletion();

    if (COLOR(q) == BLACK)
      fix_black_condition(p);

    q->reset();
    rb_stack.empty();

    return q;
  }

  /** Return the i-th node in order sense.

      @param[in] i inorder position of node to be selected
      @return a pointer to the i-th node inorder sense
      @throw out_of_range if i is greater or equal than the number
      of nodes of tree
  */
  Node * select(const size_t i) const
  {
    return Aleph::select(root, i);
  }

  /** Compute the inorder position of a key.

      @param[in] key to be searched
      @return a pair with the inorder position and the node pointer.
      If key is not found, position is -1.
  */
  std::pair<long, Node*> position(const Key & key) const noexcept
  {
    std::pair<long, Node*> ret_val;

    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
      inorder_position(root, key, ret_val.second);

    return ret_val;
  }

  /** Find the inorder position of a key in the tree.

      @param[in] key to be searched
      @return a pair with the position and the node pointer
  */
  std::pair<long, Node*> find_position(const Key & key) const noexcept
  {
    std::pair<long, Node*> r(-2, nullptr);

    r.first = BinTreeXt_Operation<Node, Compare>(cmp).
      find_position(root, key, r.second);

    return r;
  }

private:

  // Compute black height of RB tree (count black nodes to any leaf)
  static size_t black_height(Node * p) noexcept
  {
    size_t h = 0;
    while (p != Node::NullPtr)
      {
        if (COLOR(p) == BLACK)
          ++h;
        p = LLINK(p);  // Any path works since all paths have same black height
      }
    return h;
  }

  // Simple rotation without parent link update (for internal use)
  static Node * rotate_right_simple(Node * p) noexcept
  {
    Node * q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    return q;
  }

  static Node * rotate_left_simple(Node * p) noexcept
  {
    Node * q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;
    COUNT(p) = COUNT(LLINK(p)) + COUNT(RLINK(p)) + 1;
    COUNT(q) = COUNT(LLINK(q)) + COUNT(RLINK(q)) + 1;
    return q;
  }

  // Fix red-red violation going up from node p
  // pp_ptr is a pointer to the pointer that points to the subtree being fixed
  static void fix_red_violation(Node *& subtree_root) noexcept
  {
    // Simplified iterative fix for red-red violations
    // This version uses recursion-based fixing during join
    if (subtree_root == Node::NullPtr)
      return;

    // Check left child for red-red
    if (LLINK(subtree_root) != Node::NullPtr and
        COLOR(LLINK(subtree_root)) == RED)
      {
        Node * l = LLINK(subtree_root);
        if (LLINK(l) != Node::NullPtr and COLOR(LLINK(l)) == RED)
          {
            // Left-left case
            subtree_root = rotate_right_simple(subtree_root);
            COLOR(subtree_root) = BLACK;
            COLOR(RLINK(subtree_root)) = RED;
            return;
          }
        if (RLINK(l) != Node::NullPtr and COLOR(RLINK(l)) == RED)
          {
            // Left-right case
            LLINK(subtree_root) = rotate_left_simple(l);
            subtree_root = rotate_right_simple(subtree_root);
            COLOR(subtree_root) = BLACK;
            COLOR(RLINK(subtree_root)) = RED;
            return;
          }
      }

    // Check right child for red-red
    if (RLINK(subtree_root) != Node::NullPtr and
        COLOR(RLINK(subtree_root)) == RED)
      {
        Node * r = RLINK(subtree_root);
        if (RLINK(r) != Node::NullPtr and COLOR(RLINK(r)) == RED)
          {
            // Right-right case
            subtree_root = rotate_left_simple(subtree_root);
            COLOR(subtree_root) = BLACK;
            COLOR(LLINK(subtree_root)) = RED;
            return;
          }
        if (LLINK(r) != Node::NullPtr and COLOR(LLINK(r)) == RED)
          {
            // Right-left case
            RLINK(subtree_root) = rotate_right_simple(r);
            subtree_root = rotate_left_simple(subtree_root);
            COLOR(subtree_root) = BLACK;
            COLOR(LLINK(subtree_root)) = RED;
            return;
          }
      }
  }

  // Extract minimum node with rebalancing
  // Returns the extracted min and updates p to the new root
  static Node * extract_min_rb(Node *& p) noexcept
  {
    if (p == Node::NullPtr)
      return Node::NullPtr;

    if (LLINK(p) == Node::NullPtr)
      {
        Node * ret = p;
        p = RLINK(p);
        ret->reset();
        return ret;
      }

    // Use recursive approach with fix-up
    struct ExtractHelper
    {
      static Node * extract(Node *& p, bool & height_decreased) noexcept
      {
        if (LLINK(p) == Node::NullPtr)
          {
            Node * ret = p;
            height_decreased = (COLOR(p) == BLACK);
            p = RLINK(p);
            ret->reset();
            return ret;
          }

        bool child_decreased = false;
        Node * ret = extract(LLINK(p), child_decreased);
        --COUNT(p);

        if (child_decreased)
          {
            // Left subtree's black height decreased
            // Need to rebalance
            Node * s = RLINK(p);  // sibling
            if (s != Node::NullPtr)
              {
                if (COLOR(s) == RED)
                  {
                    // Case 1: sibling is red
                    p = rotate_left_simple(p);
                    COLOR(p) = BLACK;
                    COLOR(LLINK(p)) = RED;
                    // Continue fixing in LLINK(p)
                    s = RLINK(LLINK(p));
                  }

                if (s != Node::NullPtr)
                  {
                    if ((LLINK(s) == Node::NullPtr or COLOR(LLINK(s)) == BLACK) and
                        (RLINK(s) == Node::NullPtr or COLOR(RLINK(s)) == BLACK))
                      {
                        // Case 2: sibling and its children are black
                        COLOR(s) = RED;
                        if (COLOR(p) == RED)
                          {
                            COLOR(p) = BLACK;
                            height_decreased = false;
                          }
                        else
                          height_decreased = true;
                      }
                    else
                      {
                        // Case 3 or 4: sibling has a red child
                        if (RLINK(s) == Node::NullPtr or COLOR(RLINK(s)) == BLACK)
                          {
                            // Case 3: left child of sibling is red
                            COLOR(LLINK(s)) = BLACK;
                            COLOR(s) = RED;
                            RLINK(p) = rotate_right_simple(s);
                          }
                        // Case 4: right child of sibling is red
                        Color old_color = COLOR(p);
                        p = rotate_left_simple(p);
                        COLOR(p) = old_color;
                        COLOR(LLINK(p)) = BLACK;
                        if (RLINK(p) != Node::NullPtr)
                          COLOR(RLINK(p)) = BLACK;
                        height_decreased = false;
                      }
                  }
                else
                  height_decreased = (COLOR(p) == BLACK);
              }
            else
              height_decreased = (COLOR(p) == BLACK);
          }
        else
          height_decreased = false;

        return ret;
      }
    };

    bool dummy = false;
    return ExtractHelper::extract(p, dummy);
  }

  // Extract maximum node with rebalancing
  static Node * extract_max_rb(Node *& p) noexcept
  {
    if (p == Node::NullPtr)
      return Node::NullPtr;

    if (RLINK(p) == Node::NullPtr)
      {
        Node * ret = p;
        p = LLINK(p);
        ret->reset();
        return ret;
      }

    struct ExtractHelper
    {
      static Node * extract(Node *& p, bool & height_decreased) noexcept
      {
        if (RLINK(p) == Node::NullPtr)
          {
            Node * ret = p;
            height_decreased = (COLOR(p) == BLACK);
            p = LLINK(p);
            ret->reset();
            return ret;
          }

        bool child_decreased = false;
        Node * ret = extract(RLINK(p), child_decreased);
        --COUNT(p);

        if (child_decreased)
          {
            Node * s = LLINK(p);  // sibling
            if (s != Node::NullPtr)
              {
                if (COLOR(s) == RED)
                  {
                    p = rotate_right_simple(p);
                    COLOR(p) = BLACK;
                    COLOR(RLINK(p)) = RED;
                    s = LLINK(RLINK(p));
                  }

                if (s != Node::NullPtr)
                  {
                    if ((LLINK(s) == Node::NullPtr or COLOR(LLINK(s)) == BLACK) and
                        (RLINK(s) == Node::NullPtr or COLOR(RLINK(s)) == BLACK))
                      {
                        COLOR(s) = RED;
                        if (COLOR(p) == RED)
                          {
                            COLOR(p) = BLACK;
                            height_decreased = false;
                          }
                        else
                          height_decreased = true;
                      }
                    else
                      {
                        if (LLINK(s) == Node::NullPtr or COLOR(LLINK(s)) == BLACK)
                          {
                            COLOR(RLINK(s)) = BLACK;
                            COLOR(s) = RED;
                            LLINK(p) = rotate_left_simple(s);
                          }
                        Color old_color = COLOR(p);
                        p = rotate_right_simple(p);
                        COLOR(p) = old_color;
                        COLOR(RLINK(p)) = BLACK;
                        if (LLINK(p) != Node::NullPtr)
                          COLOR(LLINK(p)) = BLACK;
                        height_decreased = false;
                      }
                  }
                else
                  height_decreased = (COLOR(p) == BLACK);
              }
            else
              height_decreased = (COLOR(p) == BLACK);
          }
        else
          height_decreased = false;

        return ret;
      }
    };

    bool dummy = false;
    return ExtractHelper::extract(p, dummy);
  }

  // Join two RB trees with a pivot node
  // All keys in t1 < pivot < all keys in t2
  // bh1 and bh2 are black heights
  static Node * join_with_pivot_rb(Node * t1, size_t bh1, Node * pivot,
                                   Node * t2, size_t bh2) noexcept
  {
    if (bh1 == bh2)
      {
        // Simple case: equal black heights
        LLINK(pivot) = t1;
        RLINK(pivot) = t2;
        COLOR(pivot) = RED;
        COUNT(pivot) = COUNT(t1) + COUNT(t2) + 1;
        return pivot;
      }

    if (bh1 > bh2)
      {
        // t1 is taller (more black nodes), descend right spine
        return join_right_rb(t1, bh1, pivot, t2, bh2);
      }
    // t2 is taller, descend left spine
    return join_left_rb(t1, bh1, pivot, t2, bh2);
  }

  // Join when bh1 > bh2
  static Node * join_right_rb(Node * t1, size_t bh1, Node * pivot,
                              Node * t2, size_t bh2) noexcept
  {
    if (t1 == Node::NullPtr)
      {
        LLINK(pivot) = Node::NullPtr;
        RLINK(pivot) = t2;
        COLOR(pivot) = BLACK;
        COUNT(pivot) = COUNT(t2) + 1;
        return pivot;
      }

    // Descend right spine until black height matches bh2
    size_t curr_bh = bh1;
    if (COLOR(t1) == BLACK)
      --curr_bh;

    if (curr_bh == bh2)
      {
        // Found the join point
        LLINK(pivot) = RLINK(t1);
        RLINK(pivot) = t2;
        COLOR(pivot) = RED;
        COUNT(pivot) = COUNT(RLINK(t1)) + COUNT(t2) + 1;
        RLINK(t1) = pivot;
        COUNT(t1) = COUNT(LLINK(t1)) + COUNT(pivot) + 1;

        // Fix red-red if needed
        if (COLOR(t1) == RED)
          {
            // t1 is red and pivot is red - need rotation
            // Since we came from the right, do left rotation
            Node * result = rotate_left_simple(t1);
            COLOR(result) = BLACK;
            COLOR(LLINK(result)) = RED;
            return result;
          }
        return t1;
      }

    // Continue descending
    Node * right_result = join_right_rb(RLINK(t1), curr_bh, pivot, t2, bh2);
    RLINK(t1) = right_result;
    COUNT(t1) = COUNT(LLINK(t1)) + COUNT(RLINK(t1)) + 1;

    // Check for red-red violation
    if (COLOR(t1) == BLACK and RLINK(t1) != Node::NullPtr and COLOR(RLINK(t1)) == RED)
      {
        if (RLINK(RLINK(t1)) != Node::NullPtr and COLOR(RLINK(RLINK(t1))) == RED)
          {
            Node * result = rotate_left_simple(t1);
            COLOR(result) = BLACK;
            COLOR(LLINK(result)) = RED;
            COLOR(RLINK(result)) = BLACK;
            return result;
          }
        if (LLINK(RLINK(t1)) != Node::NullPtr and COLOR(LLINK(RLINK(t1))) == RED)
          {
            RLINK(t1) = rotate_right_simple(RLINK(t1));
            Node * result = rotate_left_simple(t1);
            COLOR(result) = BLACK;
            COLOR(LLINK(result)) = RED;
            COLOR(RLINK(result)) = BLACK;
            return result;
          }
      }

    return t1;
  }

  // Join when bh2 > bh1
  static Node * join_left_rb(Node * t1, size_t bh1, Node * pivot,
                             Node * t2, size_t bh2) noexcept
  {
    if (t2 == Node::NullPtr)
      {
        LLINK(pivot) = t1;
        RLINK(pivot) = Node::NullPtr;
        COLOR(pivot) = BLACK;
        COUNT(pivot) = COUNT(t1) + 1;
        return pivot;
      }

    size_t curr_bh = bh2;
    if (COLOR(t2) == BLACK)
      --curr_bh;

    if (curr_bh == bh1)
      {
        RLINK(pivot) = LLINK(t2);
        LLINK(pivot) = t1;
        COLOR(pivot) = RED;
        COUNT(pivot) = COUNT(t1) + COUNT(LLINK(t2)) + 1;
        LLINK(t2) = pivot;
        COUNT(t2) = COUNT(pivot) + COUNT(RLINK(t2)) + 1;

        if (COLOR(t2) == RED)
          {
            Node * result = rotate_right_simple(t2);
            COLOR(result) = BLACK;
            COLOR(RLINK(result)) = RED;
            return result;
          }
        return t2;
      }

    Node * left_result = join_left_rb(t1, bh1, pivot, LLINK(t2), curr_bh);
    LLINK(t2) = left_result;
    COUNT(t2) = COUNT(LLINK(t2)) + COUNT(RLINK(t2)) + 1;

    // Check for red-red violation
    if (COLOR(t2) == BLACK and LLINK(t2) != Node::NullPtr and COLOR(LLINK(t2)) == RED)
      {
        if (LLINK(LLINK(t2)) != Node::NullPtr and COLOR(LLINK(LLINK(t2))) == RED)
          {
            Node * result = rotate_right_simple(t2);
            COLOR(result) = BLACK;
            COLOR(LLINK(result)) = BLACK;
            COLOR(RLINK(result)) = RED;
            return result;
          }
        if (RLINK(LLINK(t2)) != Node::NullPtr and COLOR(RLINK(LLINK(t2))) == RED)
          {
            LLINK(t2) = rotate_left_simple(LLINK(t2));
            Node * result = rotate_right_simple(t2);
            COLOR(result) = BLACK;
            COLOR(LLINK(result)) = BLACK;
            COLOR(RLINK(result)) = RED;
            return result;
          }
      }

    return t2;
  }

  // Recursive join of two RB trees
  static Node * join_exclusive_rec_rb(Node * t1, size_t bh1,
                                      Node * t2, size_t bh2) noexcept
  {
    if (t1 == Node::NullPtr)
      return t2;
    if (t2 == Node::NullPtr)
      return t1;

    // Extract a pivot from the larger tree
    Node * pivot;
    if (bh1 >= bh2)
      {
        pivot = extract_max_rb(t1);
        if (t1 != Node::NullPtr)
          bh1 = black_height(t1);
        else
          bh1 = 0;
      }
    else
      {
        pivot = extract_min_rb(t2);
        if (t2 != Node::NullPtr)
          bh2 = black_height(t2);
        else
          bh2 = 0;
      }

    return join_with_pivot_rb(t1, bh1, pivot, t2, bh2);
  }

  // Split by key recursively
  Node * split_key_rec_rb(Node * p, const Key & key,
                          Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return Node::NullPtr;
      }

    Node * found;
    if (cmp(key, KEY(p)))
      {
        Node * left_t2;
        found = split_key_rec_rb(LLINK(p), key, t1, left_t2);

        LLINK(p) = Node::NullPtr;
        const size_t left_t2_bh = (left_t2 != Node::NullPtr) ? black_height(left_t2) : 0;
        const size_t right_bh = (RLINK(p) != Node::NullPtr) ? black_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t2 = join_with_pivot_rb(left_t2, left_t2_bh, p, right_tree, right_bh);
        if (t2 != Node::NullPtr)
          COLOR(t2) = BLACK;
      }
    else if (cmp(KEY(p), key))
      {
        Node * right_t1;
        found = split_key_rec_rb(RLINK(p), key, right_t1, t2);

        RLINK(p) = Node::NullPtr;
        const size_t right_t1_bh = (right_t1 != Node::NullPtr) ? black_height(right_t1) : 0;
        const size_t left_bh = (LLINK(p) != Node::NullPtr) ? black_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t1 = join_with_pivot_rb(left_tree, left_bh, p, right_t1, right_t1_bh);
        if (t1 != Node::NullPtr)
          COLOR(t1) = BLACK;
      }
    else
      {
        t1 = LLINK(p);
        t2 = RLINK(p);
        LLINK(p) = RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;
        found = p;
        if (t1 != Node::NullPtr)
          COLOR(t1) = BLACK;
        if (t2 != Node::NullPtr)
          COLOR(t2) = BLACK;
      }

    return found;
  }

  // Split by key including duplicates
  void split_key_dup_rec_rb(Node * p, const Key & key,
                            Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return;
      }

    if (cmp(key, KEY(p)))
      {
        Node * left_t2;
        split_key_dup_rec_rb(LLINK(p), key, t1, left_t2);

        LLINK(p) = Node::NullPtr;
        const size_t left_t2_bh = (left_t2 != Node::NullPtr) ? black_height(left_t2) : 0;
        const size_t right_bh = (RLINK(p) != Node::NullPtr) ? black_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t2 = join_with_pivot_rb(left_t2, left_t2_bh, p, right_tree, right_bh);
        if (t2 != Node::NullPtr)
          COLOR(t2) = BLACK;
      }
    else
      {
        Node * right_t1;
        split_key_dup_rec_rb(RLINK(p), key, right_t1, t2);

        RLINK(p) = Node::NullPtr;
        const size_t right_t1_bh = (right_t1 != Node::NullPtr) ? black_height(right_t1) : 0;
        const size_t left_bh = (LLINK(p) != Node::NullPtr) ? black_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t1 = join_with_pivot_rb(left_tree, left_bh, p, right_t1, right_t1_bh);
        if (t1 != Node::NullPtr)
          COLOR(t1) = BLACK;
      }
  }

  // Split by position recursively
  void split_pos_rec_rb(Node * p, size_t pos, Node *& t1, Node *& t2) noexcept
  {
    if (p == Node::NullPtr)
      {
        t1 = t2 = Node::NullPtr;
        return;
      }

    size_t left_count = COUNT(LLINK(p));

    if (pos <= left_count)
      {
        Node * left_t2;
        split_pos_rec_rb(LLINK(p), pos, t1, left_t2);

        LLINK(p) = Node::NullPtr;
        const size_t left_t2_bh = (left_t2 != Node::NullPtr) ? black_height(left_t2) : 0;
        const size_t right_bh = (RLINK(p) != Node::NullPtr) ? black_height(RLINK(p)) : 0;

        Node * right_tree = RLINK(p);
        RLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t2 = join_with_pivot_rb(left_t2, left_t2_bh, p, right_tree, right_bh);
        if (t2 != Node::NullPtr)
          COLOR(t2) = BLACK;
      }
    else
      {
        Node * right_t1;
        split_pos_rec_rb(RLINK(p), pos - left_count - 1, right_t1, t2);

        RLINK(p) = Node::NullPtr;
        const size_t right_t1_bh = (right_t1 != Node::NullPtr) ? black_height(right_t1) : 0;
        const size_t left_bh = (LLINK(p) != Node::NullPtr) ? black_height(LLINK(p)) : 0;

        Node * left_tree = LLINK(p);
        LLINK(p) = Node::NullPtr;
        COUNT(p) = 1;
        COLOR(p) = RED;

        t1 = join_with_pivot_rb(left_tree, left_bh, p, right_t1, right_t1_bh);
        if (t1 != Node::NullPtr)
          COLOR(t1) = BLACK;
      }
  }

public:

  /** Join this tree exclusively with another tree.

      All keys in this tree must be less than all keys in t.
      After the operation, t becomes empty and this tree contains all nodes.

      @param[in,out] t tree to join with (will be empty after)
      @note O(m log(n+m)) time complexity where m is size of t
  */
  void join_exclusive(Gen_Rb_Tree_Rk & t) noexcept
  {
    if (t.root == Node::NullPtr)
      return;

    if (root == Node::NullPtr)
      {
        root = t.root;
        t.root = Node::NullPtr;
        return;
      }

    // Collect nodes from t in preorder and insert them one by one
    // This is O(m log(n+m)) but guarantees correctness
    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(t.root);
    t.root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        // Reset node and insert
        p->reset();
        insert(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);
      }
  }

  /** Split tree by key.

      After the operation:
      - t1 contains all keys < key
      - t2 contains all keys > key
      - This tree becomes empty

      @param[in] key the splitting key
      @param[out] t1 tree to receive keys < key
      @param[out] t2 tree to receive keys > key
      @return pointer to node containing key, or nullptr if not found
      @note O(n) time complexity (simple iterative implementation)
  */
  Node * split_key(const Key & key, Gen_Rb_Tree_Rk & t1,
                   Gen_Rb_Tree_Rk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    // Simple O(n) implementation: iterate through tree and partition
    Node * found = nullptr;
    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(root);
    root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);

        p->reset();

        if (cmp(KEY(p), key))
          t1.insert(p);
        else if (cmp(key, KEY(p)))
          t2.insert(p);
        else
          found = p;  // Found the key
      }

    return found;
  }

  /** Split tree by key including duplicates.

      After the operation:
      - t1 contains all keys <= key
      - t2 contains all keys > key
      - This tree becomes empty

      @param[in] key the splitting key
      @param[out] t1 tree to receive keys <= key
      @param[out] t2 tree to receive keys > key
      @note O(n) time complexity (simple iterative implementation)
  */
  void split_key_dup(const Key & key, Gen_Rb_Tree_Rk & t1,
                     Gen_Rb_Tree_Rk & t2) noexcept
  {
    if (root == Node::NullPtr)
      return;

    FixedStack<Node*> stack(Node::MaxHeight);
    stack.push(root);
    root = Node::NullPtr;

    while (not stack.is_empty())
      {
        Node * p = stack.pop();
        Node * left = LLINK(p);
        Node * right = RLINK(p);

        if (right != Node::NullPtr)
          stack.push(right);
        if (left != Node::NullPtr)
          stack.push(left);

        p->reset();

        if (cmp(key, KEY(p)))
          t2.insert(p);
        else
          t1.insert_dup(p);
      }
  }

  /** Split tree by inorder position.

      After the operation:
      - t1 contains nodes at positions [0, pos)
      - t2 contains nodes at positions [pos, size)
      - This tree becomes empty

      @param[in] pos the splitting position (0-based)
      @param[out] t1 tree to receive positions [0, pos)
      @param[out] t2 tree to receive positions [pos, size)
      @note O(n) time complexity (simple iterative implementation)
  */
  void split_pos(size_t pos, Gen_Rb_Tree_Rk & t1,
                 Gen_Rb_Tree_Rk & t2) noexcept
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

    // Simple O(n) implementation using inorder traversal
    size_t count = 0;
    FixedStack<Node*> stack(Node::MaxHeight);
    Node * curr = root;
    root = Node::NullPtr;

    // Inorder traversal
    while (curr != Node::NullPtr or not stack.is_empty())
      {
        while (curr != Node::NullPtr)
          {
            stack.push(curr);
            Node * left = LLINK(curr);
            LLINK(curr) = Node::NullPtr;  // Disconnect
            curr = left;
          }

        curr = stack.pop();
        Node * right = RLINK(curr);
        curr->reset();

        if (count < pos)
          t1.insert(curr);
        else
          t2.insert(curr);

        ++count;
        curr = right;
      }
  }

  /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Trees
  */
  class Iterator : public BinTreeXt_Iterator<Gen_Rb_Tree_Rk, Node, Key, Compare>
  {
    using Base = BinTreeXt_Iterator<Gen_Rb_Tree_Rk, Node, Key, Compare>;

  public:

    using Base::Base;
    using Base::operator=;
  }; // end class Iterator
};


/** Red-Black binary search tree with nodes without virtual destructor and
    with subtree counters for select/position operations.

    @param Key key type stored in the tree nodes.
    @param Compare comparison functor between keys.
    @see Rb_Tree_Rk_Vtl
    @ingroup Trees
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Rk : public Gen_Rb_Tree_Rk<RbNodeRk, Key, Compare>
{
  using Base = Gen_Rb_Tree_Rk<RbNodeRk, Key, Compare>;
  using Base::Base;
};


/** Red-Black binary search tree with virtual destructor in its nodes and
    with subtree counters for select/position operations.

    @param Key key type stored in the tree nodes.
    @param Compare comparison functor between keys.
    @see Rb_Tree_Rk
    @ingroup Trees
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Rk_Vtl : public Gen_Rb_Tree_Rk<RbNodeRkVtl, Key, Compare>
{
  using Base = Gen_Rb_Tree_Rk<RbNodeRkVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph

# endif // TPL_RBRK_H
