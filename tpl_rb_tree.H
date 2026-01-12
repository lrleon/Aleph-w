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

/** @file tpl_rb_tree.H
 *  @brief Red-Black tree implementation (bottom-up balancing).
 *
 *  Self-balancing BST with color constraints ensuring O(log n) height.
 *  Uses bottom-up insertion with post-insertion rebalancing.
 *
 *  ## Properties
 *  - Every node is red or black
 *  - Root is black
 *  - No two adjacent red nodes
 *  - All paths have same black-height
 *
 *  ## Complexity: O(log n) for all operations
 *
 *  @see tpl_tdRbTree.H Top-down variant
 *  @see tpl_rbRk.H With rank support
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_RB_TREE_H
# define TPL_RB_TREE_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_binNodeUtils.H>
# include <rbNode.H>

namespace Aleph
{

/** @brief Red-black binary search tree implementation (bottom-up).

    A red-black tree is a self-balancing binary search tree where each node
    has a color (red or black). The tree maintains balance through five
    invariants that guarantee the height is at most 2*log₂(n+1), ensuring
    O(log n) worst-case time for all operations.

    @par Red-Black Tree Properties:
    1. Every node is either red or black
    2. The root is black
    3. All leaves (null nodes) are black
    4. Red nodes have only black children
    5. Every path from root to leaf has the same number of black nodes

    These properties ensure the tree remains approximately balanced, with
    the longest path at most twice the length of the shortest path.

    This is the bottom-up implementation that uses a stack to track ancestors
    during descent and repairs violations by ascending back to the root.

    @tparam NodeType Node template (typically RbNode or RbNodeVtl).
    @tparam Key The type of keys stored in the tree.
    @tparam Compare Comparison functor for ordering keys.

    @par Complexity:
    - Search: O(log n) worst case
    - Insert: O(log n) worst case, at most 2 rotations
    - Delete: O(log n) worst case, at most 3 rotations
    - Size: O(1)
    - Space: O(n) + O(log n) for ancestor stack during operations

    @par Example:
    @code
    Rb_Tree<int> tree;
    tree.insert(new Rb_Tree<int>::Node(42));
    tree.insert(new Rb_Tree<int>::Node(17));
    tree.insert(new Rb_Tree<int>::Node(99));

    auto node = tree.search(42);
    if (node != nullptr)
      std::cout << "Found: " << node->get_key() << '\n';

    tree.remove(42);
    @endcode

    @note This is a low-level implementation managing raw nodes.
          For automatic memory management, use DynSetRbTree.

    @see GenTdRbTree Top-down (single-pass) alternative implementation.
    @see DynSetRbTree High-level wrapper with automatic memory management.
    @see Avl_Tree Alternative balanced tree (stricter balance, more rotations).
    @ingroup Trees
 */
template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rb_Tree
{
public:
  using Node = NodeType<Key>;
  using key_type = Key;
  using compare_type = Compare;

private:
  Node head_node;                    ///< Sentinel header node
  Node *head;                        ///< Pointer to sentinel
  Node *& root;                      ///< Reference to root (right child of head)
  FixedStack<Node *> rb_stack;       ///< Stack for ancestor path
  Compare cmp;                       ///< Comparison functor
  size_t num_nodes;                  ///< Number of nodes in tree
  static constexpr signed char CmpLess = -1;
  static constexpr signed char CmpEqual = 0;
  static constexpr signed char CmpGreater = 1;

  /// Search for key and stack ancestors
  Node * search_and_stack_rb(const Key & key,
                             signed char & cmp_result) noexcept
  {
    Node *p = root;
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

  /// Search for insertion point (allows duplicates) and stack ancestors
  Node * search_dup_and_stack_rb(const Key & key,
                                 signed char & cmp_result) noexcept
  {
    Node *p = root;
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

  /// Fix red-red violation after insertion
  void fix_red_condition(Node *p) noexcept
  {
    assert(COLOR(p) == RED);

    while (p != root)
      {
        Node *pp = rb_stack.pop();  // parent of p
        if (COLOR(pp) == BLACK)     // is p's parent black?
          break;                    // yes ==> no consecutive reds ==> done

        if (root == pp)             // is p a direct child of the root?
          {
            COLOR(root) = BLACK;
            break;
          }

        Node *ppp = rb_stack.pop();  // grandparent of p
        Node *spp = LLINK(ppp) == pp ? RLINK(ppp) : LLINK(ppp);  // uncle
        
        if (COLOR(spp) == RED)       // is p's uncle red?
          {
            COLOR(ppp) = RED;
            COLOR(pp) = BLACK;
            COLOR(spp) = BLACK;
            p = ppp;
            continue;
          }

        Node *pppp = rb_stack.pop();  // great-grandparent of p
        if (LLINK(pp) == p and LLINK(ppp) == pp)
          {
            rotate_to_right(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else if (RLINK(pp) == p and RLINK(ppp) == pp)
          {
            rotate_to_left(ppp, pppp);
            COLOR(pp) = BLACK;
          }
        else
          {
            if (RLINK(pp) == p)
              {
                rotate_to_left(pp, ppp);
                rotate_to_right(ppp, pppp);
              }
            else
              {
                rotate_to_right(pp, ppp);
                rotate_to_left(ppp, pppp);
              }
            COLOR(p) = BLACK;
          }

        COLOR(ppp) = RED;
        break;
      }

    rb_stack.empty();
  }

  /// Find successor and swap with node being deleted
  void find_succ_and_swap(Node *p, Node *& pp) noexcept
  {
    Node *& ref_rb_stack = rb_stack.top();

    Node *fSucc = p;
    Node *succ = RLINK(p);
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
        Node *succr = RLINK(succ);
        RLINK(succ) = RLINK(p);
        LLINK(fSucc) = p;
        RLINK(p) = succr;
        pp = fSucc;
      }

    std::swap(COLOR(succ), COLOR(p));
  }

  /// Fix black-height violation after deletion
  void fix_black_condition(Node *p) noexcept
  {
    if (COLOR(p) == RED)
      {
        COLOR(p) = BLACK;
        return;
      }

    Node *pp = rb_stack.popn(2);
    while (p != root)
      {
        assert(LLINK(pp) == p or RLINK(pp) == p);
        assert(LLINK(rb_stack.top()) == pp or RLINK(rb_stack.top()) == pp);

        Node *sp = LLINK(pp) == p ? RLINK(pp) : LLINK(pp);
        if (COLOR(sp) == RED)
          {
            Node *& ppp = rb_stack.top();

            if (LLINK(pp) == p)
              {
                sp = LLINK(sp);
                ppp = rotate_to_left(pp, ppp);
              }
            else
              {
                sp = RLINK(sp);
                ppp = rotate_to_right(pp, ppp);
              }

            COLOR(ppp) = BLACK;
            COLOR(pp) = RED;
          }

        Node *np, *snp;
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
            Node *ppp = rb_stack.top();

            if (RLINK(sp) == np)
              rotate_to_left(pp, ppp);
            else
              rotate_to_right(pp, ppp);

            COLOR(sp) = COLOR(pp);
            COLOR(pp) = BLACK;
            COLOR(np) = BLACK;

            return;
          }

        if (COLOR(snp) == RED)
          {
            Node *ppp = rb_stack.top();

            if (LLINK(sp) == snp)
              {
                rotate_to_right(sp, pp);
                rotate_to_left(pp, ppp);
              }
            else
              {
                rotate_to_left(sp, pp);
                rotate_to_right(pp, ppp);
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

  /// Initialize tree state
  void init() noexcept
  {
    num_nodes = 0;
  }

public:
  /// Returns a reference to the comparison criteria
  Compare & key_comp() noexcept { return cmp; }
  const Compare & key_comp() const noexcept { return cmp; }

  /// \overload key_comp()
  Compare & get_compare() noexcept { return cmp; }
  const Compare & get_compare() const noexcept { return cmp; }

  /// Default constructor with optional comparator
  Gen_Rb_Tree(Compare __cmp = Compare()) noexcept
    : head(&head_node), root(head_node.getR()),
      rb_stack(Node::MaxHeight), cmp(__cmp), num_nodes(0)
  {
    /* empty */
  }

  /** Swaps all elements with another tree in constant time.
      @param[in] tree the red-black tree to swap with
   */
  void swap(Gen_Rb_Tree & tree) noexcept
  {
    std::swap(root, tree.root);
    std::swap(num_nodes, tree.num_nodes);
    std::swap(cmp, tree.cmp);
  }

  /// Move constructor
  Gen_Rb_Tree(Gen_Rb_Tree && tree) noexcept
    : head(&head_node), root(head_node.getR()),
      rb_stack(Node::MaxHeight), cmp(std::move(tree.cmp)), 
      num_nodes(tree.num_nodes)
  {
    root = tree.root;
    tree.root = Node::NullPtr;
    tree.num_nodes = 0;
  }

  /// Move assignment operator
  Gen_Rb_Tree & operator=(Gen_Rb_Tree && tree) noexcept
  {
    if (this != &tree)
      {
        root = tree.root;
        num_nodes = tree.num_nodes;
        cmp = std::move(tree.cmp);
        tree.root = Node::NullPtr;
        tree.num_nodes = 0;
      }
    return *this;
  }

  /// Destructor
  virtual ~Gen_Rb_Tree() = default;

  /** @brief Search for a key in the tree.
      @param key Key to search for.
      @return Pointer to node containing key, or nullptr if not found.
   */
  Node * search(const Key & key) const noexcept
  {
    Node *p = root;
    while (p != Node::NullPtr)
      {
        if (cmp(key, KEY(p)))
          p = LLINK(p);
        else if (cmp(KEY(p), key))
          p = RLINK(p);
        else
          return p;
      }
    return nullptr;
  }

  /// Get reference to root pointer
  Node *& getRoot() noexcept { return root; }
  
  /// Get const root pointer
  Node * getRoot() const noexcept { return root; }

  /// Returns true if the tree is empty
  bool is_empty() const noexcept { return root == Node::NullPtr; }

  /// Returns the number of nodes in the tree (O(1))
  size_t size() const noexcept { return num_nodes; }

  /// Reset tree to empty state (does not free nodes)
  void reset() noexcept
  {
    root = Node::NullPtr;
    num_nodes = 0;
  }

  /** @brief Insert a node into the tree.
      
      @param p Node to insert (must be red with null children).
      @return Pointer to inserted node, or nullptr if key already exists.
   */
  Node * insert(Node *p) noexcept
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      {
        ++num_nodes;
        return root = p;
      }

    signed char cmp_result = CmpEqual;
    Node *q = search_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else if (cmp_result == CmpGreater)
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return nullptr;  // duplicate key
      }
    
    ++num_nodes;
    fix_red_condition(p);

    return p;
  }

  /** @brief Search for key or insert if not found.
      
      @param p Node to search or insert.
      @return Pointer to p if inserted, or pointer to existing node with same key.
   */
  Node * search_or_insert(Node *p) noexcept
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      {
        ++num_nodes;
        return root = p;
      }

    signed char cmp_result = CmpEqual;
    Node *q = search_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else if (cmp_result == CmpGreater)
      RLINK(q) = p;
    else
      {
        rb_stack.empty();
        return q;  // return existing node
      }
    
    ++num_nodes;
    fix_red_condition(p);

    return p;
  }

  /** @brief Insert a node allowing duplicates.
      
      @param p Node to insert.
      @return Pointer to inserted node (always succeeds).
   */
  Node * insert_dup(Node *p) noexcept
  {
    assert(p != nullptr and p != Node::NullPtr);
    assert(COLOR(p) == RED);

    if (root == Node::NullPtr)
      {
        ++num_nodes;
        return root = p;
      }

    signed char cmp_result = CmpEqual;
    Node *q = search_dup_and_stack_rb(KEY(p), cmp_result);
    if (cmp_result == CmpLess)
      LLINK(q) = p;
    else
      RLINK(q) = p;

    ++num_nodes;
    fix_red_condition(p);

    return p;
  }

  /// Verify that tree satisfies red-black properties
  bool verify() const noexcept { return is_red_black_bst(root, cmp); }

  /** @brief Remove the node containing key.
      
      @param key Key to remove.
      @return Pointer to removed node, or nullptr if not found.
   */
  Node * remove(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;

    signed char cmp_result = CmpEqual;
    Node *q = search_and_stack_rb(key, cmp_result);
    if (cmp_result != CmpEqual)
      {
        rb_stack.empty();
        return nullptr;
      }

    Node *pq = rb_stack.top(1);
    Node *p;
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

    if (COLOR(q) == BLACK)
      fix_black_condition(p);

    q->reset();
    rb_stack.empty();
    --num_nodes;

    return q;
  }

  /** @brief Iterator over tree nodes in sorted order.
      
      Allows traversing nodes according to the comparison criteria.
      
      @ingroup Trees
   */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    /// Default constructor creates an "end" iterator
    Iterator() noexcept = default;
    Iterator(Gen_Rb_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
    Iterator(const Gen_Rb_Tree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};

/** @brief Red-black tree with nodes without virtual destructor.

   @tparam Key Key type stored in tree nodes.
   @tparam Compare Comparison functor between keys.
   @see Rb_Tree_Vtl
   @ingroup Trees
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree : public Gen_Rb_Tree<RbNode, Key, Compare>
{
  using Base = Gen_Rb_Tree<RbNode, Key, Compare>;
  using Base::Base;
};

/** @brief Red-black tree with virtual destructor in nodes.

   @tparam Key Key type stored in tree nodes.
   @tparam Compare Comparison functor between keys.
   @see Rb_Tree
   @ingroup Trees
*/
template <typename Key, class Compare = Aleph::less<Key>>
struct Rb_Tree_Vtl : public Gen_Rb_Tree<RbNodeVtl, Key, Compare>
{
  using Base = Gen_Rb_Tree<RbNodeVtl, Key, Compare>;
  using Base::Base;
};

} // end namespace Aleph

# endif // TPL_RB_TREE_H
