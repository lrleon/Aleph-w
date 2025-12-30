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

# ifndef TPL_RBRK_H
# define TPL_RBRK_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTreeOps.H>
# include <rbNodeRk.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

/** Extended Red-Black binary search tree with select and position.

    A red-black binary search tree is a binary search tree whose height is
    bounded by O(lg n) and whose update operations are bounded in time
    by inspecting O(lg n) nodes.

    This extended version maintains a count in each node representing
    the number of nodes in its subtree, enabling O(log n) operations
    for selecting by position and finding the position of a key.

    @param NodeType node type, either RbNodeRk or RbNodeRkVtl.
    @param Key key type stored in the tree nodes.
    @param Compare comparison functor between keys.
    @see Rb_Tree_Rk Rb_Tree_Rk_Vtl
    @ingroup Arboles
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

  Node * search_and_stack_rb(const Key & key)
  {
    Node * p = root;
    rb_stack.push(head);
    do
      {
        rb_stack.push(p);
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else if (cmp(KEY(p), key))
          p = RLINK(p);
        else
          return p;
      }
    while (p != Node::NullPtr);

    return rb_stack.top();
  }

  Node * search_dup_and_stack_rb(const Key & key)
  {
    Node * p = root;
    rb_stack.push(head);
    do
      {
        rb_stack.push(p);
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else
          p = RLINK(p);
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

  Gen_Rb_Tree_Rk(Compare __cmp = Compare())
    : head(&head_node), root(head_node.getR()),
      rb_stack(Node::MaxHeight), cmp(__cmp)
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

  bool is_empty() const noexcept { return root == Node::NullPtr; }

  size_t size() const noexcept { return COUNT(root); }

  Node * insert(Node * p)
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      return root = p;

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q)))
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p)))
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

    Node * q = search_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q)))
      LLINK(q) = p;
    else if (cmp(KEY(q), KEY(p)))
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

    Node * q = search_dup_and_stack_rb(KEY(p));
    if (cmp(KEY(p), KEY(q)))
      LLINK(q) = p;
    else
      RLINK(q) = p;

    update_counters_after_insertion();
    fix_red_condition(p);

    return p;
  }

  bool verify() const { return is_red_black_bst_rk(root, cmp) and check_rank_tree(root); }

  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    Node * q = search_and_stack_rb(key);
    if (no_equals<Key, Compare>(KEY(q), key, cmp))
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

  /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Arboles
  */
  class Iterator
  {
  protected:

    mutable Gen_Rb_Tree_Rk * tree_ptr;
    mutable Node * curr;
    mutable int curr_pos;

    static constexpr int Pos_Not_Current = -1;
    static constexpr int Pos_Empty_Container = -2;
    static constexpr int Pos_Not_Updated = -3;

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
          curr_pos == static_cast<int>(COUNT(tree_ptr->getRoot())))
        return;

      curr = Aleph::select(tree_ptr->getRoot(), curr_pos);
    }

  public:

    Iterator() noexcept
      : tree_ptr(nullptr), curr(nullptr), curr_pos(Pos_Not_Current)
    { /* empty */ }

    Iterator(const Gen_Rb_Tree_Rk & __tree) noexcept
      : tree_ptr(&const_cast<Gen_Rb_Tree_Rk&>(__tree)), curr(nullptr)
    {
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

    Iterator(const Gen_Rb_Tree_Rk & __tree, Node * __curr) noexcept
      : tree_ptr(&const_cast<Gen_Rb_Tree_Rk&>(__tree)),
        curr(__curr), curr_pos(Pos_Not_Updated)
    { /* empty */ }

    Iterator(const Gen_Rb_Tree_Rk & __tree, const size_t pos) noexcept
      : tree_ptr(&const_cast<Gen_Rb_Tree_Rk&>(__tree)),
        curr(nullptr), curr_pos(pos)
    { /* empty */ }

    Iterator(const Iterator & itor) noexcept
      : tree_ptr(itor.tree_ptr), curr(itor.curr), curr_pos(itor.curr_pos)
    { /* empty */ }

    Iterator & operator = (const Iterator & itor) noexcept
    {
      if (this == &itor)
        return *this;

      tree_ptr = itor.tree_ptr;
      curr = itor.curr;
      curr_pos = itor.curr_pos;

      return *this;
    }

    void reset_first() noexcept
    {
      curr = nullptr;
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

    void reset_last() noexcept
    {
      curr = nullptr;
      curr_pos = is_container_empty() ? Pos_Empty_Container :
        static_cast<int>(COUNT(tree_ptr->getRoot())) - 1;
    }

    void end() noexcept
    {
      put_itor_at_the_end(*this);
    }

    void reset_to_key(const Key & key) noexcept
    {
      std::pair<long, Node*> p = tree_ptr->find_position(key);
      curr_pos = p.first;
    }

    void reset_to_node(Node * node) noexcept
    {
      curr = node;
      curr_pos = Pos_Not_Updated;
    }

    void reset_to_pos(const size_t pos) noexcept
    {
      curr = nullptr;
      curr_pos = pos;
    }

    Node * get_curr_ne() const noexcept
    {
      if (not curr_updated())
        update_curr();
      return curr;
    }

    Node * get_curr() const noexcept
    {
      return get_curr_ne();
    }

    size_t get_current_position() const
    {
      if (not pos_updated())
        update_pos();

      ah_range_error_if(curr_pos < -1) << "Iterator has no current";
      ah_range_error_if(curr_pos > static_cast<int>(COUNT(tree_ptr->getRoot())))
        << "Iterator has no current";

      return curr_pos;
    }

    size_t get_pos() const { return get_current_position(); }

    bool has_curr() const noexcept
    {
      if (not pos_updated())
        update_pos();

      return curr_pos >= 0 and
        curr_pos < static_cast<int>(COUNT(tree_ptr->getRoot()));
    }

    void prev()
    {
      ah_underflow_error_if(not has_curr()) << "Iterator has no current";
      --curr_pos;
      curr = nullptr;
    }

    void next_ne() noexcept
    {
      ++curr_pos;
      curr = nullptr;
    }

    void next()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator has no current";
      next_ne();
    }

    Node * del()
    {
      ah_underflow_error_if(not has_curr()) << "Iterator has no current";

      if (not curr_updated())
        update_curr();

      Node * ret_val = tree_ptr->remove(KEY(curr));
      curr = nullptr;

      return ret_val;
    }

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

    bool operator != (const Iterator & itor) const
    {
      return not (*this == itor);
    }
  }; // end class Iterator
};


/** Red-Black binary search tree with nodes without virtual destructor and
    with subtree counters for select/position operations.

    @param Key key type stored in the tree nodes.
    @param Compare comparison functor between keys.
    @see Rb_Tree_Rk_Vtl
    @ingroup Arboles
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
    @ingroup Arboles
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Rk_Vtl : public Gen_Rb_Tree_Rk<RbNodeRkVtl, Key, Compare>
{
  using Base = Gen_Rb_Tree_Rk<RbNodeRkVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph

# endif // TPL_RBRK_H