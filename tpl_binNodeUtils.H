
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


/** @file tpl_binNodeUtils.H
 *  @brief Utility functions for binary tree operations.
 *
 *  This file provides a comprehensive set of utility functions for binary trees
 *  including traversals (inorder, preorder, postorder, level-order), tree
 *  construction, copying, destruction, rotation, and various query operations.
 *
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BINNODEUTILS_H
# define TPL_BINNODEUTILS_H

# include <ahFunction.H>
# include <tpl_arrayStack.H>
# include <tpl_arrayQueue.H>
# include <tpl_dynListQueue.H>
# include <bitArray.H>
# include <tpl_dynDlist.H>
# include <tpl_binNode.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph
{
  template <class Node>
  inline static
  void __inorder_rec(Node *node, const int & level, int & position,
                     void (*visitFct)(Node *, int, int))
  {
    if (node == Node::NullPtr)
      return;

    __inorder_rec(LLINK(node), level + 1, position, visitFct);

    (*visitFct)(node, level, position);
    ++position;

    __inorder_rec(RLINK(node), level + 1, position, visitFct);
  }

  /** Traverse recursively inorder a binary tree.

      `inOrderRec(root,visit)` performs an inorder traversal of tree
      rooted by `root` and on each node executes a visit function with
      the following signature:

          void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# `p`: pointer to visited node.
      -# `level`: level of node `p`.
      -# `pos`: ordinal indicating the visited order

      \deprecated Probably this function will be removed in future
      versions. Use the functor `For_Each_In_Order` or `traverse()` instead

      @param[in] root pointer to tree's root
      @param[in] visitFct pointer to visit function
      @return the number of nodes of tree
      @see preOrderRec() postOrderRec() For_Each_In_Order traverse()
      @ingroup Trees
  */
  template <class Node>
  inline
  int inOrderRec(Node *root, void (*visitFct)(Node *, int, int))
  {
    int position = 0;
    __inorder_rec(root, 0, position, visitFct);
    return position;
  }

  template <class Node>
  inline static
  void __preorder_rec(Node *p, const int & level, int & position,
                      void (*visitFct)(Node *, int, int))
  {
    if (p == Node::NullPtr)
      return;

    (*visitFct)(p, level, position);
    ++position;

    __preorder_rec(LLINK(p), level + 1, position, visitFct);
    __preorder_rec(RLINK(p), level + 1, position, visitFct);
  }

  /** Traverse recursively in preorder a binary tree.

      `preOrderRec(root,visit)` performs an inorder traversal of tree
      rooted by `root` and on each node executes a visit function with
      the following signature:

          void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# `p`: pointer to visited node.
      -# `level`: level of node `p`.
      -# `pos`: ordinal indicating the visit order

      \deprecated Probably this function will be removed in future
      versions. Use the functor For_Each_Preorder instead

      @param[in] root pointer to tree's root
      @param[in] visitFct pointer to visit function
      @return the number of nodes of tree
      @see preOrderRec() postOrderRec()
      @ingroup Trees
  */
  template <class Node>
  inline
  int preOrderRec(Node *root, void (*visitFct)(Node *, int, int))
  {
    int position = 0;
    __preorder_rec(root, 0, position, visitFct);
    return position;
  }

  template <class Node>
  inline static
  void __postorder_rec(Node *node, const int & level, int & position,
                       void (*visitFct)(Node *, int, int))
  {
    if (node == Node::NullPtr)
      return;

    __postorder_rec(LLINK(node), level + 1, position, visitFct);
    __postorder_rec(RLINK(node), level + 1, position, visitFct);

    (*visitFct)(node, level, position);
    ++position;
  }

  /** Traverse recursively in postorder a binary tree.

      `postOrderRec(root,visit)` performs an inorder traversal of tree
      rooted by `root` and on each node executes a visit function with
      the following signature:

          void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# `p`: pointer to visited node.
      -# `level`: level of node `p`.
      -# `pos`: ordinal indicating the visit order

      \deprecated Probably this function will be removed in future
      versions. Use the functor For_Each_Postorder instead

      @param[in] root pointer to tree's root
      @param[in] visitFct pointer to visit function
      @return the number of nodes of tree
      @see preOrderRec() postOrderRec()
      @ingroup Trees
  */
  template <class Node>
  inline
  int postOrderRec(Node *root, void (*visitFct)(Node *, int, int))
  {
    int position = 0;
    __postorder_rec(root, 0, position, visitFct);
    return position;
  }

  /** Generic inorder traversal of a binary tree

      For_Each_In_Order traverses a binary tree in order. For each
      is node is called an operation with the following signature:

         struct Op
         {
           void operator () (Node * p)
           {
             // operation
           }
         };

      @ingroup Trees
   */
  template <class Node>
  class For_Each_In_Order
  {
    template <class Op>
    static void for_each_inorder(Node *root, Op & op)
    {
      if (root == Node::NullPtr)
        return;

      for_each_inorder(LLINK(root), op);
      op(root);
      for_each_inorder(RLINK(root), op);
    }

  public:
    /// Invoke to traversal from root node
    template <class Op>
    void traverse(Node *root, Op & op) const
    {
      for_each_inorder<Op>(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op & op) const
    {
      for_each_inorder<Op>(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op && op) const
    {
      for_each_inorder<Op>(root, op);
    }
  };

  /** Execute an operation in order sense for each node of tree.

      @param[in] root of tree
      @param[in] op operation to be executed on each node
      @ingroup Trees
  */
  template <class Node, class Op>
  inline
  void for_each_in_order(Node *root, Op && op)
  {
    return For_Each_In_Order<Node>().template traverse<Op>(root, op);
  }

  /** Generic preorder traversal of a binary tree

     For_Each_Preorder traverses a binary tree in preorder. For each
     is node is called an operation with the following signature:

         struct Op
         {
           void operator () (Node * p)
           {
             // operation
           }
         };

     @ingroup Trees
 *  @author Leandro Rabindranath León
 */
  template <class Node>
  class For_Each_Preorder
  {
    template <class Op>
    static void preorder(Node *root, Op & op)
    {
      if (root == Node::NullPtr)
        return;

      op(root);
      preorder(LLINK(root), op);
      preorder(RLINK(root), op);
    }

  public:
    /// Invoke the traversal
    template <class Op>
    void traverse(Node *root, Op & op) const
    {
      return preorder(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op & op) const
    {
      preorder<Op>(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op && op = Op()) const
    {
      preorder<Op>(root, op);
    }
  };

  /** Execute an operation in preorder sense for each node of tree.

      @param[in] root of tree
      @param[in] op operation to be executed on each node
      @ingroup Trees
  */
  template <class Node, class Op>
  void for_each_preorder(Node *root, Op && op)
  {
    For_Each_Preorder<Node>().template traverse<Op>(root, op);
  }


  /** Generic postorder traversal of a binary tree

      For_Each_Postorder traverses a binary tree in preorder. For each
      is node is called an operation with the following signature:

            struct Op
      {
        void operator () (Node * p)
        {
          // operation
        }
      };

      @ingroup Trees
  */
  template <class Node>
  class For_Each_Postorder
  {
    template <class Op>
    static void postorder(Node *root, Op & op)
    {
      if (root == Node::NullPtr)
        return;

      postorder(LLINK(root), op);
      postorder(RLINK(root), op);
      op(root);
    }

  public:
    /// Invoke the traversal
    template <class Op>
    void traverse(Node *root, Op & op) const
    {
      return postorder(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op & op) const
    {
      postorder<Op>(root, op);
    }

    /// \overload traverse(Node * root, Op & op)
    template <class Op>
    void operator ()(Node *root, Op && op = Op()) const
    {
      postorder<Op>(root, op);
    }
  };

  /** Execute an operation in postorder sense for each node of tree.

      @param[in] root of tree
      @param[in] op operation to be executed on each node
      @ingroup Trees
  */
  template <class Node, class Op>
  void for_each_postorder(Node *root, Op && op)
  {
    For_Each_Postorder<Node>().template traverse<Op>(root, op);
  }


  template <class Node>
  static void prefix(Node *root, DynList<Node *> & acc)
  {
    if (root == Node::NullPtr)
      return;

    acc.append(root);
    prefix(LLINK(root), acc);
    prefix(RLINK(root), acc);
  }

  template <class Node>
  static void infix(Node *root, DynList<Node *> & acc)
  {
    if (root == Node::NullPtr)
      return;

    infix(LLINK(root), acc);
    acc.append(root);
    infix(RLINK(root), acc);
  }

  template <class Node>
  static void suffix(Node *root, DynList<Node *> & acc)
  {
    if (root == Node::NullPtr)
      return;

    suffix(LLINK(root), acc);
    suffix(RLINK(root), acc);
    acc.append(root);
  }

  /** Return a list with preorder traversal of a tree

      @param[in] root of tree
      @return a list of nodes sorted by preorder traversal
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  DynList<Node *> prefix(Node *root)
  {
    DynList<Node *> ret_val;
    prefix(root, ret_val);
    return ret_val;
  }

  /** Return a list with inorder traversal of a tree

      @param[in] root of tree
      @return a list of nodes sorted by inorder traversal
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  DynList<Node *> infix(Node *root)
  {
    DynList<Node *> ret_val;
    infix(root, ret_val);
    return ret_val;
  }

  /** Return a list with postorder traversal of a tree

      @param[in] root of tree
      @return a list of nodes sorted by postorder traversal
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  DynList<Node *> suffix(Node *root)
  {
    DynList<Node *> ret_val;
    suffix(root, ret_val);
    return ret_val;
  }


  /** Count the number of nodes of a binary tree

      @param[in] root of tree
      @return the number of nodes
      @ingroup Trees
   */
  template <class Node>
  inline
  size_t compute_cardinality_rec(Node *root) noexcept
  {
    if (root == Node::NullPtr)
      return 0;

    return compute_cardinality_rec(LLINK(root)) + 1 +
           compute_cardinality_rec(RLINK(root));
  }

  /// \overload compute_cardinality_rec(Node * root)
  template <class Node>
  inline
  size_t size(Node *root) noexcept
  {
    return compute_cardinality_rec(root);
  }

  /** Compute recursively the height of `root`

      @param[in] root of tree
      @return the height
      @ingroup Trees
   */
  template <class Node>
  inline size_t computeHeightRec(Node *root) noexcept
  {
    if (root == Node::NullPtr)
      return 0;

    const size_t left_height = computeHeightRec(LLINK(root));
    const size_t right_height = computeHeightRec(RLINK(root));

    return 1 + std::max(left_height, right_height);
  }

  /** Free recursively all the memory occupied by the tree `root`

      \note It is assumed that the nodes were allocated with `new`
      operator.

      @param[in] root of tree to free
      @ingroup Trees
   */
  template <class Node>
  inline
  void destroyRec(Node *& root) noexcept
  {
    if (root == Node::NullPtr)
      return;

    destroyRec(LLINK(root));
    destroyRec(RLINK(root));
    delete root;
    root = Node::NullPtr;
  }

  /** Traverses recursively the tree and calls key's destructors

          @param[in] root of tree to free
          @ingroup Trees
       */
  template <class Node>
  inline
  void callKeyDestructorsRec(Node *& root) noexcept
  {
    if (root == Node::NullPtr)
      return;

    using Key = typename Node::key_type;

    callKeyDestructorsRec(LLINK(root));
    callKeyDestructorsRec(RLINK(root));
    root->get_key().~Key();
    root = Node::NullPtr;
  }

  /** Copy recursively a tree

      @param[in] root of tre to be copied
      @return a copy of tree with `root`
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  inline
  Node * copyRec(Node *root)
  {
    if (root == Node::NullPtr)
      return Node::NullPtr;

    Node *tgt_root = new Node(*root);

    try
      {
        LLINK(tgt_root) = copyRec<Node>(LLINK(root));
        RLINK(tgt_root) = copyRec<Node>(RLINK(root));
      }
    catch (...)
      {
        assert(RLINK(tgt_root) == Node::NullPtr);

        if (LLINK(tgt_root) != Node::NullPtr)
          destroyRec(LLINK(tgt_root)); // TODO: diff de Node*&

        delete tgt_root;

        throw;
      }

    return tgt_root;
  }

  /** Return `true` if both trees are similar

      Two binary tree are similar if they have the same topological
      form.

      @param[in] t1 root of first tree
      @param[in] t2 root of second tree
      @return `true` if `t1` and `t2` are similar; `false` otherwise
      @see areEquivalents()
   */
  template <class Node>
  inline
  bool areSimilar(Node *t1, Node *t2) noexcept
  {
    if (t1 == t2) // that include the case when t1 and t2 are the same
      // or both are Node::NullPtr
      return true;

    if (t1 == Node::NullPtr or t2 == Node::NullPtr)
      return false;

    return areSimilar(LLINK(t1), LLINK(t2)) and
           areSimilar(RLINK(t1), RLINK(t2));
  }

  /** Return `true` if trees are equivalents

      Two binary tree are similar if they are similar and each pair of
      nodes in preorder traversal have the same keys

      @param[in] t1 root of first tree
      @param[in] t2 root of second tree
      @param[in] op comparison operation
      @return `true` t1 and t2 are equivalents; `false` otherwise
      @see areEquivalents()
    */
  template <class Node, class Equal>
  inline
  bool areEquivalents(Node *t1, Node *t2, Equal & op) noexcept
  {
    if (t1 == t2)
      return true;

    if (t1 == Node::NullPtr or t2 == Node::NullPtr)
      return false;

    if (not op(KEY(t1), KEY(t2)))
      return false;

    return areEquivalents(LLINK(t1), LLINK(t2), op) and
           areEquivalents(RLINK(t1), RLINK(t2), op);
  }

  /// \overload areEquivalents(Node * t1, Node * t2, Equal & op)
  template <class Node, class Equal = std::equal_to<typename Node::key_type>>
  inline bool areEquivalents(Node *t1, Node *t2, Equal && op = Equal()) noexcept
  {
    return areEquivalents(t1, t2, op);
  }

  /** Traverse a binary tree by levels.

      The visit function must have the following signature:

          void (*visitFct)(Node* p, int level, bool is_left)

      Where:
      -# `p`: pointer to currently visited node
      -# `pos`: ordinal indicating the visit order
      -# `is_left`: `true` if `p` is a left child; `false` otherwise

      @param[in] root of tree
      @param[in] visitFct visit function
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
     */
  template <class Node>
  inline
  void levelOrder(Node *root, void (*visitFct)(Node *, int, bool))
  {
    if (root == Node::NullPtr)
      return;

    DynListQueue<std::pair<Node *, bool>> queue;
    queue.put(std::pair<Node *, bool>(root, bool()));

    for (int pos = 0; not queue.is_empty(); pos++)
      {
        std::pair<Node *, bool> pr = queue.get();
        Node *& p = pr.first;

        (*visitFct)(p, pos, pr.second);

        if (LLINK(p) != Node::NullPtr)
          queue.put(std::pair<Node *, bool>(LLINK(p), true));

        if (RLINK(p) != Node::NullPtr)
          queue.put(std::pair<Node *, bool>(RLINK(p), false));
      }
  }


  /** Level traverse a tree and execute an operation

      `operation()` must have the following signature:

          bool operation(Node* p)

      if the result is `true` then the traversal continues; otherwise it
      stops.

      @param[in] root of tree
      @param[in] operation to execute on each visited node
      @return `true` if all the nodes were visited; `false` otherwise

      @ingroup Trees
  */
  template <class Node, class Operation>
  inline
  bool level_traverse(Node *root, Operation & operation)
  {
    if (root == Node::NullPtr)
      return true;

    DynListQueue<Node *> queue;
    queue.put(root);
    while (not queue.is_empty())
      {
        Node *p = queue.get();
        if (not operation(p))
          return false;

        if (LLINK(p) != Node::NullPtr)
          queue.put(LLINK(p));

        if (RLINK(p) != Node::NullPtr)
          queue.put(RLINK(p));
      }
    return true;
  }

  template <class Node, class Operation>
  inline
  bool level_traverse(Node *root, Operation && operation)
  {
    return level_traverse<Node, Operation>(root, operation);
  }

  /** Build a binary tree form its preorder and inorder traversals

      `build_tree()` takes two dynamic arrays with the preorder and
      inorder traversal of keys and builds the correspondent tree.

      @param[in] preorder array with the preorder traversal
      @param[in] l_p first index in `preorder`
      @param[in] r_p last index in `preorder`
      @param[in] inorder array with the preorder traversal
      @param[in] l_i first index in `inorder`
      @param[in] r_i last index in `inorder`
      @return the root of built tree
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <template <class> class Node, typename Key>
  inline
  Node<Key> * build_tree(const DynArray<Key> & preorder, long l_p, long r_p,
                         const DynArray<Key> & inorder, long l_i, long r_i)
  {
    if (l_p > r_p)
      {
        assert(l_i > r_i);
        return Node<Key>::NullPtr;
      }

    assert(r_p - l_p == r_i - l_i);

    auto *root = new Node<Key>(preorder[l_p]);
    if (r_p == l_p)
      return root;

    assert(l_i <= r_i);

    int i = 0;
    for (int j = l_i; j <= r_i; ++j)
      if (inorder[j] == preorder[l_p])
        {
          i = j - l_i;
          break;
        }

    assert(i <= r_i);

    LLINK(root) = build_tree<Node, Key>(preorder, l_p + 1, l_p + i,
                                        inorder, l_i, l_i + (i - 1));
    RLINK(root) = build_tree<Node, Key>(preorder, l_p + i + 1, r_p,
                                        inorder, l_i + i + 1, r_i);
    return root;
  }

  template <template <class> class Node, typename Key>
  inline
  Node<Key> * build_postorder(const DynArray<Key> & post, long lp, long rp,
                              const DynArray<Key> & in, long li, long ri)
  {
    assert(rp - lp == ri - li);
    if (lp > rp)
      return Node<Key>::NullPtr;

    auto *root = new Node<Key>(post[rp]);

    int i = li;
    for (; i <= ri; ++i) // search in inorder array the index of root
      if (in[i] == post[rp])
        break;

    assert(i <= ri);

    LLINK(root) = build_postorder<Node, Key>(post, lp, lp + (i - li) - 1,
                                             in, li, i - 1);
    RLINK(root) = build_postorder<Node, Key>(post, rp - (ri - i), rp - 1,
                                             in, i + 1, ri);
    return root;
  }

  template <class Node>
  inline static void
  __compute_nodes_in_level(Node *root, long level, const long current_level,
                           DynDlist<Node *> & level_list)
  {
    if (root == Node::NullPtr)
      return;

    if (current_level == level)
      {
        level_list.append(root);
        return; // it is not worth descending further
      }

    __compute_nodes_in_level(LLINK(root), level, current_level + 1, level_list);
    __compute_nodes_in_level(RLINK(root), level, current_level + 1, level_list);
  }

  /** Count the number of nodes in a specific tree level

      @param[in] root of tre
      @param[in] level desired to be counted
      @return a list with all the nodes of `level`
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  inline
  DynDlist<Node *> compute_nodes_in_level(Node *root, const int & level)
  {
    DynDlist<Node *> list;
    __compute_nodes_in_level(root, level, 0, list);
    return list;
  }

  /** Traverse inorder a binary tree without recursion and without stack.

      `inOrderThreaded(root,visit)` traverses inorder the binary tree
      by building partial threads to succesor nodes. This implicates
      that during the traversal the links coulld be invalid.

      The visit function has the following signature:

          void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# `p`: pointer to the currently visited node
      -# `level`: the level of visited node
      -# pos: ordinal position in the inorder traversal

      @param[in] root of tree
      @param[in] visitFct pointer to visit function
      @see preOrderThreaded()
      @ingroup Trees
   */
  template <class Node>
  inline
  void inOrderThreaded(Node *root, void (*visitFct)(Node *))
  {
    if (root == Node::NullPtr)
      return;

    Node *p = root, *r = Node::NullPtr;
    while (p != Node::NullPtr)
      {
        Node *q = LLINK(p);
        if (q == Node::NullPtr)
          { // No left branch ==> visit p
            (*visitFct)(p);
            r = p;
            p = RLINK(p);
            continue;
          }

        // Move towards the rightmost node of the left branch
        while (q != r and RLINK(q) != Node::NullPtr)
          q = RLINK(q);

        if (q != r) // Does p have a predecessor?
          { // yes ==> leave a thread in order to later come back and visit p
            RLINK(q) = p; // place the thread here
            p = LLINK(p); // keep descending on the left
            continue;
          }

        (*visitFct)(p);

        RLINK(q) = Node::NullPtr; // delete thread
        r = p;
        p = RLINK(p); // advance to the right branch
      }
  }

  /** Traverse preorder a binary tree without recursion and without stack.

      `preOrderThreaded(root,visit)` traverses preorder the binary tree
      by building partial threads to successor nodes. This implicates
      that during the traversal the links could be invalid.

      The visit function has the following signature:

          void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# `p`: pointer to the currently visited node
      -# `level`: the level of visited node
      -# pos: ordinal position in the inorder traversal

      @param[in] node of tree
      @param[in] visitFct pointer to visit function
      @see inOrderThreaded()
      @ingroup Trees
  */
  template <class Node>
  inline
  void preOrderThreaded(Node *node, void (*visitFct)(Node *))
  {
    if (node == Node::NullPtr)
      return;

    Node *p = node, *r = Node::NullPtr;
    while (p != Node::NullPtr)
      {
        Node *q = LLINK(p);

        if (q == Node::NullPtr)
          {
            (*visitFct)(p);
            r = p;
            p = RLINK(p);
            continue;
          }

        // advance towards the rightmost node of the left branch
        while (q != r and RLINK(q) != Node::NullPtr)
          q = RLINK(q);

        if (q != r)
          {
            RLINK(q) = p;
            (*visitFct)(p);
            p = LLINK(p);
            continue;
          }

        RLINK(q) = Node::NullPtr; /* delete thread */
        r = p;
        p = RLINK(p); /* advance to right branch */
      }
  }

  template <class Node>
  inline static
  size_t __internal_path_length(Node *p, const size_t & level) noexcept
  {
    if (p == Node::NullPtr)
      return 0;

    return level + __internal_path_length(LLINK(p), level + 1) +
           __internal_path_length(RLINK(p), level + 1);
  }

  /** Compute the internal path length

      @param[in] p root of tree
      @return the internal path length

      @ingroup Trees
   */
  template <class Node>
  inline
  size_t internal_path_length(Node *p) noexcept
  {
    return __internal_path_length(p, 0);
  }

  /** Compute a bit code for the binary tree

      `tree_to_bits(root, array)` takes the binary tree with `root`
      and computes its prefix code (Lukasiewicz`s word) in a bit `array`.

      @param[in] root the root
      @param[out] array bit array where the code will be stored
      \throw bad_alloc if there is no enough memory
      @see bits_to_tree() BitArray save_tree_keys_in_prefix()
      load_tree_keys_in_prefix()

      @ingroup Trees
  */
  template <class Node>
  inline
  void tree_to_bits(Node *root, BitArray & array)
  {
    if (root == Node::NullPtr)
      {
        array.push(1);
        return;
      }

    array.push(0);
    tree_to_bits(LLINK(root), array);
    tree_to_bits(RLINK(root), array);
  }

  /** Compute a bit code for the binary tree

      `tree_to_bits(root)` takes the binary tree with `root`
      and computes its prefix code (Lukasiewicz`s word) in a bit array

      @param[in] root the root
      @return  array bit array with the tree code
      \throw bad_alloc if there is no enough memory
      @see bits_to_tree() BitArray save_tree_keys_in_prefix()
      load_tree_keys_in_prefix()

      @ingroup Trees
  */
  template <class Node>
  inline
  BitArray tree_to_bits(Node *root)
  {
    BitArray ret_val;
    tree_to_bits(root, ret_val);
    return ret_val;
  }

  /** Compute a string with the Lukasiewicz`s word of a tree

      @param[in] root of tree
      @return a string corresponding to the Lukasiewicz`s word of the
      tree
      \throw bad_alloc if there is no enough memory
  */
  template <class Node>
  inline std::string code(Node *root)
  {
    const BitArray bits = tree_to_bits(root);
    const size_t n = bits.size();
    std::string str;
    str.reserve(n);
    for (size_t i = 0; i < n; ++i)
      str.push_back(bits(i) ? 'b' : 'a');

    return str;
  }

  template <class Node>
  static inline
  Node * __bits_to_tree(const BitArray & array, int & i)
  {
    if (const int bit = array.read_bit(i++); bit == 1)
      return Node::NullPtr;

    Node *p = new Node;
    LLINK(p) = __bits_to_tree<Node>(array, i);
    RLINK(p) = __bits_to_tree<Node>(array, i);

    return p;
  }

  /** Build a binary tree given its bits code.

      `bits_to_tree(array, idx)` takes a bit array and from the
      starting index `idx` builds the corresponding tree.

      @param[in] array bits array
      @param[in] idx starting index
      @return the root of equivalent binary tree
      \throw bad_alloc if there is no enough memory

      @see tree_to_bits() BitArray save_tree_keys_in_prefix() load_tree_keys_in_prefix()
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * bits_to_tree(const BitArray & array, int idx = 0)
  {
    return __bits_to_tree<Node>(array, idx);
  }

  /** Store in output stream the tree keys in preorder

      `save_tree_keys_in_prefix(root, output)` traverses recursively
      the tree in preorder. For each node, it saves in the stream its
      key.

      Each visit call to functor `Get_Key` whose function is to
      extract and return a stringficated version of the key. Its
      signature must be as follows:

          std::string gk(Node * p)

      @param[in] root of tree
      @param[out] output stream
      @ingroup Trees
      @see load_tree_keys_in_prefix()
  */
  template <class Node>
  inline
  void save_tree_keys_in_prefix(Node *root, std::ostream & output)
  {
    if (root == Node::NullPtr)
      return;

    output << root->get_key() << " ";

    save_tree_keys_in_prefix(LLINK(root), output);
    save_tree_keys_in_prefix(RLINK(root), output);
  }

  /** Load the keys stored in preorder from an input stream.

      `load_tree_keys_in_prefix(root, input)` traverses recursively
      the tree `root`. For each visited node a key is loaded from the stream

      @param[in] root of tree
      @param[in] input stream where are the keys in preorder
      @ingroup Trees
   */
  template <class Node>
  inline
  void load_tree_keys_in_prefix(Node *root, std::istream & input)
  {
    if (root == Node::NullPtr)
      return;

    input >> root->get_key();

    load_tree_keys_in_prefix(LLINK(root), input);
    load_tree_keys_in_prefix(RLINK(root), input);
  }


  /** Store a binary tree in a stream

      `save_tree(root, output)` saves the binary tree with `root` in
      the stream `output`. The tree could be restored through
      `load_tree()`.

      The operator `<<` must overload for the key of node.

      @param[in] root of tree
      @param[out] output stream
      \throw bad_alloc if there is no enough memory
      @see load_tree()
      @ingroup Trees
   */
  template <class Node>
  inline
  void save_tree(Node *root, std::ostream & output)
  {
    BitArray prefix;
    tree_to_bits(root, prefix);
    prefix.save(output);
    save_tree_keys_in_prefix(root, output);
  }


  /** Load and build a binary tree from a stream

      `load_tree(input)` reads the stream `input` and load a binary
      tree previously saved with `save_tree()`.

      @param[in] input stream
      @return the root of loaded and built binary tree
      \throw bad_alloc if there is no enough memory
      @see save_tree()
      @ingroup Trees
   */
  template <class Node>
  inline
  Node * load_tree(std::istream & input)
  {
    BitArray prefix;
    prefix.load(input);
    Node *root = bits_to_tree<Node>(prefix);
    load_tree_keys_in_prefix(root, input);
    return root;
  }


  template <class Node, class Get_Key>
  inline
  void put_tree_keys_in_array(Node *root, std::ostream & out)
  {
    if (root == Node::NullPtr)
      return;

    if (const std::string str = Get_Key()(root); str == "\t")
      out << "\"\t\"";
    else if (str == "\n")
      out << "\"\\n\"";
    else
      out << "\"" << str << "\"";
    out << ", ";

    put_tree_keys_in_array<Node, Get_Key>(LLINK(root), out);
    put_tree_keys_in_array<Node, Get_Key>(RLINK(root), out);
  }


  template <class Node, class Load_Key>
  inline
  void load_tree_keys_from_array(Node *root, const char *keys[], int & idx)
  {
    if (root == Node::NullPtr)
      return;

    if (Load_Key()(root, keys[idx]))
      ++idx;

    load_tree_keys_from_array<Node, Load_Key>(LLINK(root), keys, idx);
    load_tree_keys_from_array<Node, Load_Key>(RLINK(root), keys, idx);
  }

  /** Generate C++ array declarations for a binary tree.

      `save_tree_in_array_of_chars(root, array_name, output)`
      generates two array declarations that would allow to restore the
      original binary tree. The generated declarations would have the
      following form:

          const unsigned char array_name_cdp[n] = { unsigned char list };

          const char * array_name_k[] = { key in prefix order };

      The first array is a bit array containing the tree code (its
      Lukasiewicz word). The second array contains a strinficted version
      of the key values that were generated through the functor `Get_Key`,
      whose structure must be as follows:

          std::string get_key(Node * p);

      The goodness of this function is to embed binary trees in C++ source
      code.

      @param[in] root of tree
      @param[in] array_name prefix name to be added to array variables.
      @param[out] output stream where the arrays should be written

      @see load_tree_from_array() BitArray
      @ingroup Trees
  */
  template <class Node, class Get_Key>
  inline
  void save_tree_in_array_of_chars(Node *root,
                                   const std::string & array_name,
                                   std::ostream & output)
  {
    BitArray prefix;
    tree_to_bits(root, prefix);
    prefix.save_in_array_of_chars(array_name + "_cdp", output);
    output << "const char * " << array_name << "_k[] = { " << '\n';
    put_tree_keys_in_array<Node, Get_Key>(root, output);
    output << "nullptr };" << '\n';
  }


  /** Build a binary tree from two arrays

      `load_tree_from_array(bits, num_bits, keys)` takes a bit array
      `bits` of `num_bits`, whose values of `unsigned char` type contain
      the tree code. The code is therefore read and the tree is
      built. Afterward, the array `keys` is read and the values set to the
      nodes keys in preorder.

      The functor `Load_Key` is used in order to set the node key from a
      array entry. Its structure must be as follows:

          bool load_key(Node * p, const char * str)

      The functor must take the string `str`, perform any needed
      transformation and set the key of node `p`. If `load_key()` returns
      `true` then it is assumed that the key was already set and the process
      advances to the next key. Otherwise, `str` continues to be the
      current key and the process advances to the next node.
      for the next node in the prefix path.

      @param[in] bits array where the tree code is stored
      @param[in] num_bits number of bits to be read
      @param[in] keys array where the keys in preorder are stored
      @return the tree root
      \throw bad_alloc if there is no enough memory

      @see save_tree_in_array_of_chars() BitArray
      @ingroup Trees
  */
  template <class Node, class Load_Key>
  inline
  Node * load_tree_from_array(const unsigned char bits[],
                              const size_t & num_bits,
                              const char *keys[])
  {
    BitArray prefix;
    prefix.load_from_array_of_chars(bits, num_bits);
    Node *root = bits_to_tree<Node>(prefix);
    int i = 0;
    load_tree_keys_from_array<Node, Load_Key>(root, keys, i);
    return root;
  }


  /** Return `true` if `p` is a binary search tree

      @param[in] p root of the tree
      @param[in] cmp comparison criteria
      @return `true` if `p` is a binary search tree according to
      `Compare` criteria.

      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  bool check_bst(Node *p, const Compare & cmp = Compare())
  {
    if (p == Node::NullPtr)
      return true;

    if (LLINK(p) != Node::NullPtr and
        (not less_or_equal_than(KEY(LLINK(p)), KEY(p), cmp) or
         not check_bst(LLINK(p), cmp)))
      return false;

    if (RLINK(p) != Node::NullPtr and
        (not less_or_equal_than(KEY(p), KEY(RLINK(p)), cmp) or
         not check_bst(RLINK(p), cmp)))
      return false;

    return true;
  }


  /** Build a binary search tree from its preorder traversal

      @param[in] preorder dynamic array where the preorder traversal
      is found
      @param[in] l lower index
      @param[in] r upper index
      @return root of resulting tree
      \throw bad_alloc if there is no enough memory
      @ingroup Trees
   */
  template <class Node>
  inline Node *
  preorder_to_bst(DynArray<typename Node::key_type> & preorder, int l, int r)
  {
    if (l > r)
      return Node::NullPtr;

    Node *root = new Node(preorder[l]);

    if (l == r)
      return root;

    int first_greater = l + 1;
    while ((first_greater <= r) and (preorder[first_greater] < preorder[l]))
      ++first_greater;

    LLINK(root) = preorder_to_bst<Node>(preorder, l + 1, first_greater - 1);
    RLINK(root) = preorder_to_bst<Node>(preorder, first_greater, r);

    return root;
  }

  /// Constants for three-way comparison results
  enum ThreeWayCmp : int
  {
    CmpLess = -1,    ///< First argument is less than second
    CmpEqual = 0,    ///< Arguments are equal
    CmpGreater = 1   ///< First argument is greater than second
  };

  /** Three-way comparison using a binary comparator.

      Returns CmpLess if a < b, CmpEqual if a == b, CmpGreater if a > b.
      This reduces two comparisons to one when the result is cached.

      @param[in] a first value to compare
      @param[in] b second value to compare
      @param[in] cmp binary less-than comparator
      @return CmpLess, CmpEqual, or CmpGreater indicating the ordering
      @ingroup Trees
   */
  template <typename T, class Compare = Aleph::less<T>>
  inline ThreeWayCmp three_way_compare(const T & a, const T & b,
                                       const Compare & cmp = Compare()) noexcept
  {
    if (cmp(a, b))
      return CmpLess;
    if (cmp(b, a))
      return CmpGreater;
    return CmpEqual;
  }

  /** Search a key in a binary search tree

      @param[in] root of tree
      @param[in] key to search
      @param[in] cmp key comparison criteria
      @return a valid pointer to the node containing the searched key
      if this was found; `Node::NullPtr` otherwise
      @ingroup Trees
   */
  /** Search a key in a binary search tree using optimistic search.
   *
   *  Uses single comparison per level with deferred duplicate detection,
   *  reducing comparisons from ~1.5h to h+1 for tree height h.
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * searchInBinTree(Node *root, const typename Node::key_type & key,
                         const Compare & cmp = Compare()) noexcept
  {
    Node *candidate = nullptr;  // Tracks potential match when going right

    while (root != Node::NullPtr)
      {
        __builtin_prefetch(LLINK(root));
        __builtin_prefetch(RLINK(root));

        if (cmp(key, KEY(root)))  // key < root: go left
          root = LLINK(root);
        else  // key >= root: go right, mark as candidate
          {
            candidate = root;
            root = RLINK(root);
          }
      }

    // Optimistic check: if candidate exists and key == candidate, return it
    if (candidate != nullptr and not cmp(KEY(candidate), key)) [[unlikely]]
      return candidate;

    return Node::NullPtr;
  }

  /** Return the minimum key contained in a binary search tree

      @param[in] root of tree
      @return pointer to node containing minimum key
      @note It is not verified if tree is empty
      @ingroup Trees
      @see find_max()
   */
  template <class Node>
  inline
  Node * find_min(Node *root) noexcept
  {
    while (LLINK(root) != Node::NullPtr)
      root = LLINK(root);

    return root;
  }

  /** Return the maximum key contained in a binary search tree

      @param[in] root of tree
      @return pointer to node containing maximum key
      @note It is not verified if tree is empty
      @ingroup Trees
      @see find_max()
   */
  template <class Node>
  inline
  Node * find_max(Node *root) noexcept
  {
    while (RLINK(root) != Node::NullPtr)
      root = RLINK(root);

    return root;
  }

  /** Find the inorder successor of `p`

      @param[in] p a node pointer
      @param[out] pp p's parent
      @return successor of `p`
      @see find_predecessor()
      @ingroup Trees
   */
  template <class Node>
  inline
  Node * find_successor(Node *p, Node *& pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(RLINK(p) != Node::NullPtr);

    pp = p;
    p = RLINK(p);
    while (LLINK(p) != Node::NullPtr)
      {
        pp = p;
        p = LLINK(p);
      }

    return p;
  }

  /** Find the inorder predecessor of `p`

      @param[in] p a node pointer
      @param[out] pp p's parent
      @return predecessor of `p`
      @see find_successor()
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * find_predecessor(Node *p, Node *& pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) != Node::NullPtr);

    pp = p;
    p = LLINK(p);

    while (RLINK(p) != Node::NullPtr)
      {
        pp = p;
        p = RLINK(p);
      }

    return p;
  }

  /** Search a key and find its node and parent.

      @param[in] root of tree
      @param[in] key to search
      @param[out] parent pointer to parent node if key was
      found. Otherwise value is undetermined
      @param[in] cmp comparison criteria
      @return a valid pointer to a node containing `key` if this is
      found; otherwise, it returns a pointer to the last visited node
      @see searchInBinTree()
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * search_parent(Node *root, const typename Node::key_type & key,
                       Node *& parent, const Compare & cmp = Compare()) noexcept
  {
    assert((LLINK(parent) == root) or (RLINK(parent) == root));
    assert(root != Node::NullPtr);

    while (true)
      {
        const auto c = three_way_compare(key, KEY(root), cmp);
        if (c == CmpLess) [[likely]]
          {
            if (LLINK(root) == Node::NullPtr)
              return root;

            parent = root;
            root = LLINK(root);
          }
        else if (c == CmpGreater) [[likely]]
          {
            if (RLINK(root) == Node::NullPtr)
              return root;

            parent = root;
            root = RLINK(root);
          }
        else [[unlikely]]
          return root;
      }
  }

  /** Rank search of a key in a binary search tree.

      In a binary search tree the rank search of a key consists in
      determining the node that would be parent of key.

      `search_rank_parent(root, key, cmp)` searches a node containing
      key. If `key` is found, then its node is returned. Otherwise,
      the last visited node, that would be the parent of `key` if this
      was inserted in the tree, is returned.

      @param[in] root of general tree
      @param[in] key to search
      @param[in] cmp comparison criteria
      @return pointer to a node containing `key` if this node exists
      or the last visited node otherwise
      @note It is not verified if the tree is empty
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline Node *
  search_rank_parent(Node *root, const typename Node::key_type & key,
                     const Compare & cmp = Compare()) noexcept
  {
    assert(root != Node::NullPtr);

    while (true)
      if (const auto & root_key = KEY(root); cmp(key, root_key))
        {
          if (LLINK(root) == Node::NullPtr)
            return root;

          root = LLINK(root);
        }
      else if (cmp(root_key, key))
        {
          if (RLINK(root) == Node::NullPtr)
            return root;

          root = RLINK(root);
        }
      else
        return root;
  }

  /** Insert a node `p` in a binary search tree

      `insert_in_bst(root, p)` inserts the node `p` in
      the binary search tree with `root`

      @param[in,out] r of tree.
      @param[in] p pointer to the node to be inserted.
      @param[in] cmp comparison criteria.
      @return `p` if this was inserted; that is if `p->get_key()` is
      not in the tree; otherwise, `Node::NullPtr` is returned
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * insert_in_bst(Node *& r, Node *p, const Compare & cmp = Compare()) noexcept
  {
    if (r == Node::NullPtr) [[unlikely]]
        return r = p;

    const auto & pk = KEY(p);
    const auto & rk = KEY(r);

    if (cmp(pk, rk))
      return insert_in_bst<Node, Compare>(LLINK(r), p, cmp);
    if (cmp(rk, pk))
      return insert_in_bst<Node, Compare>(RLINK(r), p, cmp);

    [[unlikely]] return Node::NullPtr;
  }

  /** Insert a node `p` in a binary search tree

      `insert_dup_in_bst(root, p)` inserts the node `p` in
      the binary search tree with `root`. The key contained in `p` can be
      already present in the tree.

      @param[in,out] root of tree
      @param[in] p pointer to the node to be inserted
      @param[in] cmp comparison criteria
      @return the pointer `p`
      @ingroup Trees
  */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * insert_dup_in_bst(Node *& root, Node *p, const Compare & cmp = Compare())
    noexcept
  {
    if (root == Node::NullPtr) [[unlikely]]
        return root = p;

    const auto & pk = KEY(p);
    const auto & rk = KEY(root);

    if (cmp(pk, rk))
      return insert_dup_in_bst(LLINK(root), p, cmp);

    return insert_dup_in_bst(RLINK(root), p, cmp);
  }

  /** Search or insert a node in a binary search tree.

      `search_or_insert_in_bst(root, p, cmp)` searches in `root` a node
      containing `p->get_key()`. If found, then this node is
      returned. Otherwise, `p` is inserted and returned.

      @param[in,out] r roor of tree
      @param[in] p node to search or insert
      @param[in] cmp comparison criteria
      @return `p` if its key was not in the tree; otherwise, a pointer
      containing the tree is returned.

      @ingroup Trees
  */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * search_or_insert_in_bst(Node *& r, Node *p, const Compare & cmp = Compare())
    noexcept
  {
    if (r == Node::NullPtr) [[unlikely]]
        return r = p;

    const auto & pk = KEY(p);
    const auto & rk = KEY(r);

    if (cmp(pk, rk))
      return search_or_insert_in_bst<Node, Compare>(LLINK(r), p, cmp);
    if (cmp(rk, pk))
      return search_or_insert_in_bst<Node, Compare>(RLINK(r), p, cmp);

    [[unlikely]] return r;
  }


  template <class Node, class Compare>
  static inline
  bool __split_key_rec(Node *root, const typename Node::key_type & key,
                       Node *& ts, Node *& tg, Compare & cmp) noexcept
  {
    if (root == Node::NullPtr)
      { // key is not in the tree ==> split will succeed
        ts = tg = Node::NullPtr;
        return true;
      }

    if (cmp(key, KEY(root)))
      {
        if (__split_key_rec(LLINK(root), key, ts, LLINK(root), cmp))
          {
            tg = root;
            return true;
          }
        return false;
      }

    if (cmp(KEY(root), key))
      if (__split_key_rec(RLINK(root), key, RLINK(root), tg, cmp))
        {
          ts = root;
          return true;
        }

    return false; // key exists in the tree
  }


  /** Split recursively according to a key.

      `split_key_rec(root, key, ts, tg, cmp)` splits the tree with `root`
      in two trees `t1` which contain the keys lesser than `key` and `t2`
      which contains the keys greater than `key`.

      The split only is performed if `key` is not in the tree.

      @param[in,out] root of tree
      @param[in] key for slitting
      @param[out] ts tree with keys lesser than `key`
      @param[out] tg tree with keys greater than `key`
      @param[in] cmp comparison criteria
      @return `true` if the tree wa split; that if `key` was not in the
      tree. Otherwise, the split is not performed and it return `false`

      @see split_key()
      @ingroup Trees
      */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  bool split_key_rec(Node *& root, const typename Node::key_type & key,
                     Node *& ts, Node *& tg, const Compare & cmp = Compare()) noexcept
  {
    const bool ret = __split_key_rec(root, key, ts, tg, cmp);
    if (ret)
      root = Node::NullPtr;
    return ret;
  }


  template <class Node, class Compare>
  static inline
  void __split_key_dup_rec(Node *root, const typename Node::key_type & key,
                           Node *& ts, Node *& tg, Compare & cmp) noexcept
  {
    if (root == Node::NullPtr)
      {
        ts = tg = Node::NullPtr;
        return;
      }

    if (cmp(KEY(root), key))
      {
        __split_key_dup_rec(RLINK(root), key, RLINK(root), tg, cmp);
        ts = root;
      }
    else
      {
        __split_key_dup_rec(LLINK(root), key, ts, LLINK(root), cmp);
        tg = root;
      }
  }


  /** Split a tree according to a key value

      `split_key_dup_rec(root, key, ts, tg, cmp)` splits according to `key
      the tree with `root` and build two trees. `t1`contains the keys
      lesser than `key` and `t2` the keys greater or equal than `key`.

      @param[in,out] root of tre to be split
      @param[in] key for splitting
      @param[out] ts tree with the keys lesser than `key`
      @param[out] tg tree with the keys greater or equal than `key`
      @param[in] cmp comparison criteria
      @see split_key()
      @ingroup Trees
      */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  void split_key_dup_rec(Node *& root, const typename Node::key_type & key,
                         Node *& ts, Node *& tg, const Compare & cmp = Compare()) noexcept
  {
    __split_key_dup_rec(root, key, ts, tg, cmp);
    root = Node::NullPtr;
  }


  /** Exclusive join of two binary trees

      `join_exclusive(ts, tg)` joins `ts` and `ts`. The exclusive sense
      means that all the keys of `ts` are lesser that all the keys of `tg`

      @param[in] ts tree with keys lesser than `tg`
      @param[in] tg tree with keys greater than `ts`
      @return the root of resulting joined tree
      @warning No validation about the exclusive ranges is done

      @ingroup Trees
      @see remove_from_bst()
       */
  template <class Node>
  inline
  Node * join_exclusive(Node *& ts, Node *& tg) noexcept
  {
    if (ts == Node::NullPtr)
      return tg;

    if (tg == Node::NullPtr)
      return ts;

    LLINK(tg) = join_exclusive(RLINK(ts), LLINK(tg));

    RLINK(ts) = tg;
    Node *ret_val = ts;
    ts = tg = Node::NullPtr; // empty the trees

    return ret_val;
  }

  /** Remove a key from a binary search tree

      @param[in,out] root of tree
      @param[in] key to remove
      @param[in] cmp comparison criteria
      @return a valid pointer to the removed node if `key` was found
      in the tree, `Node::NullPtr` otherwise

      @see join_exclusive()
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * remove_from_bst(Node *& root, const typename Node::key_type & key,
                         const Compare & cmp = Compare()) noexcept
  {
    if (root == Node::NullPtr)
      return Node::NullPtr;

    if (cmp(key, KEY(root)))
      return remove_from_bst(LLINK(root), key, cmp);
    if (cmp(KEY(root), key))
      return remove_from_bst(RLINK(root), key, cmp);

    Node *ret_val = root; // save root that is going to be removed
    root = join_exclusive(LLINK(root), RLINK(root));

    ret_val->reset();

    return ret_val;
  }

  /** Insert the node `p` as root of a binary search tree

      `insert_root(root, p, cmp)` inserts in the tree `root` the node
      `p`. After insertion, `p` becomes the new root of tree.

      @param[in,out] root of binary search tree
      @param[in] p pointer to node to insert
      @param[in] cmp comparison criteria
      @return a pointer to `p` if this was inserted; that is, if
      `p->get_key()` was not present in the tree. Otherwise, no insertion
      is done and `Node::NullPtr` is returned
      @see insert_root_rec()
      @ingroup Trees
  */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * insert_root(Node *& root, Node *p, const Compare & cmp = Compare()) noexcept
  {
    Node *l = Node::NullPtr, *r = Node::NullPtr;

    if (not split_key_rec(root, KEY(p), l, r, cmp))
      return Node::NullPtr;

    LLINK(p) = l;
    RLINK(p) = r;
    root = p;

    return root;
  }

  /** Insert node `p` as root of a binary search tree. The key of `p`
      can be duplicated.

      @param[in,out] root of tree
      @param[in] p node to insert as root
      @param[in] cmp comparison criteria
      @return the pointer `p` which has became the root of tree
      @see insert_root_rec()
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * insert_dup_root(Node *& root, Node *p, const Compare & cmp = Compare()) noexcept
  {
    split_key_dup_rec(root, KEY(p), LLINK(p), RLINK(p), cmp);
    return root = p;
  }


  /** Union of two binary search trees

      \warning This union is \f$O(n \lg m)\f$ where n and m are the sizes
      of `t1` and `t2`respectively. **Use `join()` which is much more faster**

      @param[in] t1 root of first tree
      @param[in] t2 root of second tree
      @param[out] dup root of tree where the duplicated keys will be put
      @param[in] cmp comparison criteria
      @return a pointer to the resulting root of tree
      @see join()
      @ingroup Trees
  */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * join_preorder(Node *t1, Node *t2, Node *& dup,
                       const Compare & cmp = Compare()) noexcept
  {
    if (t2 == Node::NullPtr)
      return t1;

    Node *l = LLINK(t2);
    Node *r = RLINK(t2);

    if (insert_in_bst(t1, t2, cmp) == Node::NullPtr)
      insert_in_bst(dup, t2, cmp); // insertion has failed

    join_preorder(t1, l, dup, cmp);
    join_preorder(t1, r, dup, cmp);

    return t1;
  }

  /** Fast union of two binary search trees

      `join(t1, t2, dup, cmp)` joins the nodes of `t1` with the nodes
      of `t2`. The duplicated keys of `t2` are copied in the binary
      search tree `dup`.

      @param[in] t1 root of first tree
      @param[in] t2 root of second tree
      @param[out] dup tree where the duplicated keys of t2 are put
      @param[in] cmp comparison criteria
      @return pointer to the root of resulting join
      @see join_preorder()
      @ingroup Trees
   */
  template <class Node,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * join(Node *t1, Node *t2, Node *& dup, const Compare & cmp = Compare())
    noexcept
  {
    if (t1 == Node::NullPtr)
      return t2;

    if (t2 == Node::NullPtr)
      return t1;

    Node *l = LLINK(t1);
    Node *r = RLINK(t1);

    t1->reset();

    while (t1 != Node::NullPtr and insert_root(t2, t1, cmp) == Node::NullPtr)
      {
        Node *p = remove_from_bst(t1, KEY(t1), cmp);
        l = LLINK(t1);
        r = RLINK(t1);

        assert(p != Node::NullPtr);

        insert_in_bst(dup, p, cmp);
      }

    LLINK(t2) = join(l, LLINK(t2), dup, cmp);
    RLINK(t2) = join(r, RLINK(t2), dup, cmp);

    return t2;
  }

  /** Rotate to the right the tree with root `p`

      @param[in] p root to rotate
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * rotate_to_right(Node *p) noexcept
  {
    assert(p != Node::NullPtr);

    Node *q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    return q;
  }

  /** Rotate to the right the tree with root `p` and update its parent

      @param[in] p root to rotate
      @param[in] pp parent of p
      @return pointer to the new root after rotation
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * rotate_to_right(Node *p, Node *pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    assert(LLINK(pp) == p or RLINK(pp) == p);

    Node *q = LLINK(p);
    LLINK(p) = RLINK(q);
    RLINK(q) = p;

    if (LLINK(pp) == p) // update the parent
      LLINK(pp) = q;
    else
      RLINK(pp) = q;

    return q;
  }

  /** Rotate to the left the tree with root `p`

      @param[in] p root to rotate
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * rotate_to_left(Node *p) noexcept
  {
    assert(p != Node::NullPtr);

    Node *q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

    return q;
  }

  /** Rotate to the left the tree with root `p` and update its parent

      @param[in] p root to rotate
      @param pp parent of `p`
      @ingroup Trees
  */
  template <class Node>
  inline
  Node * rotate_to_left(Node *p, Node *pp) noexcept
  {
    assert(p != Node::NullPtr);
    assert(pp != Node::NullPtr);
    assert(LLINK(pp) == p or RLINK(pp) == p);

    Node *q = RLINK(p);
    RLINK(p) = LLINK(q);
    LLINK(q) = p;

    // update the parent
    if (LLINK(pp) == p)
      LLINK(pp) = q;
    else
      RLINK(pp) = q;

    return q;
  }

  /** Split a binary search tree according to a key.

      `split_key(root, key, l, r, cmp)` splits the tree `root`
      according to `key`. At the end, `l` contains all the keys lesser
      than `key` and `r ` all the keys greater or equal than `key`.

      @param[in,out] root of tree to split
      @param[in] key for splitting
      @param[out] l tree with keys lesser than `key`
      @param[out] r tree with keys greater or equal than `key`
      @param[in] cmp comparison criteria
      @see split_key_rec()
      @ingroup Trees
   */
  template <class Node, class Key,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  void split_key(Node *& root, const Key & key, Node *& l, Node *& r,
                 const Compare & cmp = Compare()) noexcept
  {
    assert(l == Node::NullPtr and r == Node::NullPtr);
    if (root == Node::NullPtr)
      {
        l = r = Node::NullPtr;
        return;
      }

    Node **current_parent = nullptr;
    Node **pending_child = nullptr;
    char current_is_right = true;
    if (cmp(key, KEY(root)))
      {
        r = root;
        pending_child = &l;
      }
    else
      {
        l = root;
        pending_child = &r;
        current_is_right = false;
      }

    Node *current = root;
    while (current != Node::NullPtr)
      {
        if (cmp(key, KEY(current)))
          { /* current must be in right side */
            if (not current_is_right)
              {
                current_is_right = not current_is_right;
                *pending_child = *current_parent; /* change of side */
                pending_child = current_parent;
              }
            current_parent = &LLINK(current);
          }
        else
          { /* current must be in left side */
            if (current_is_right)
              {
                current_is_right = not current_is_right;
                *pending_child = *current_parent; /* change of side */
                pending_child = current_parent;
              }
            current_parent = &RLINK(current);
          }
        current = *current_parent;
      }
    *pending_child = Node::NullPtr;
    root = Node::NullPtr;
  }

  /** Swap a node with its successor inorder

      @param[in] p pointer to node to swap with successor
      @param[in,out] pp parent of `p`
      @param[in] q successor of `p`
      @param[in,out] pq parent of `q`
      @see swap_node_with_predecessor()
      @ingroup Trees
  */
  template <class Node>
  inline
  void swap_node_with_successor(Node *p, // Node for swapping
                                Node *& pp, // parent of p
                                Node *q, // Successor inorder of p
                                Node *& pq) // parent of q
    noexcept
  {
    assert(p != Node::NullPtr and q != Node::NullPtr and
           pp != Node::NullPtr and pq != Node::NullPtr);
    assert(LLINK(pp) == p or RLINK(pp) == p);
    assert(LLINK(pq) == q or RLINK(pq) == q);
    assert(LLINK(q) == Node::NullPtr);

    /* Set of pp to its new son q */
    if (LLINK(pp) == p)
      LLINK(pp) = q;
    else
      RLINK(pp) = q;

    LLINK(q) = LLINK(p);
    LLINK(p) = Node::NullPtr;

    /* Checks if successor is right child of p. In this case, p will
       become q's son. This situation happens when p's son does not have
       a left branch */
    if (RLINK(p) == q)
      {
        RLINK(p) = RLINK(q);
        RLINK(q) = p;
        pq = pp;
        pp = q;
        return;
      }

    /* In this case, successor is the leftmost node descending from
       right son of p */
    Node *qr = RLINK(q);
    RLINK(q) = RLINK(p);
    LLINK(pq) = p;
    RLINK(p) = qr;

    std::swap(pp, pq);
  }

  /** Swap a node with its predecessor inorder

      @param[in] p pointer to node to swap with predecessor
      @param[in,out] pp parent of `p`
      @param[in] q predecessor of `p`
      @param[in,out] pq parent of `q`
      @see swap_node_with_successor()
      @ingroup Trees
  */
  template <class Node>
  inline
  void swap_node_with_predecessor(Node *p, // Node for swapping
                                  Node *& pp, // p's parent
                                  Node *q, // Predecessor inorder of p
                                  Node *& pq) // q's parent
    noexcept
  {
    assert((p != Node::NullPtr) and (q != Node::NullPtr) and
           (pp != Node::NullPtr) and (pq != Node::NullPtr));
    assert((RLINK(pp) == p) or (LLINK(pp) == p));
    assert((RLINK(pq) == q) or (LLINK(pq) == q));
    assert(RLINK(q) == Node::NullPtr);

    /* Set of pp to its new son q */
    if (RLINK(pp) == p)
      RLINK(pp) = q;
    else
      LLINK(pp) = q;

    RLINK(q) = RLINK(p);
    RLINK(p) = Node::NullPtr;

    /* Checks if predecessor is left child of p. In this case, p will
       become q's son. This situation happens when p's son does not have
       a right branch */
    if (LLINK(p) == q)
      {
        LLINK(p) = LLINK(q);
        LLINK(q) = p;
        pq = pp;
        pp = q;
        return;
      }

    /* In this case, predecessor is the rightmost node descending from
       right son of p */
    Node *ql = LLINK(q);
    LLINK(q) = LLINK(p);
    RLINK(pq) = p;
    LLINK(p) = ql;
    std::swap(pp, pq);
  }

  /** Insert a node as root in a binary search tree.

      This version first insert `p` as a leaf of tree. Then `p` is rotated
      until the root.

      @param[in] root of tree
      @param[in] p pointer to the node to insert
      @param[in] cmp comparison criteria
      @return pointer to `p` if `p->get_key()` is not in the
      tree. Otherwise the function returns `Node::NullPtr`
      @see insert_root()
      @ingroup Trees
  */
  template <class Node, class Key,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * insert_root_rec(Node *root, Node *p,
                         const Compare & cmp = Compare()) noexcept
  {
    if (root == Node::NullPtr)
      return p; /* insertion in empty tree */

    if (cmp(KEY(p), KEY(root)))
      { /* insert in left subtree */
        Node *left_branch = insert_root_rec(LLINK(root), p, cmp);
        if (left_branch == Node::NullPtr)
          return Node::NullPtr;

        LLINK(root) = left_branch;
        root = rotate_to_right(root);
      }
    else if (cmp(KEY(root), KEY(p)))
      { /* insert in right subtree */
        Node *right_branch = insert_root_rec(RLINK(root), p, cmp);
        if (right_branch == Node::NullPtr)
          return Node::NullPtr;

        RLINK(root) = right_branch;
        root = rotate_to_left(root);
      }

    return Node::NullPtr; /* duplicated key */
  }

  /** Search and eventually insert `p` as root in a binary search tree

      @param[in] root of tree
      @param[in] p pointer to the node to eventually insert
      @param[in] cmp comparison criteria
      @return if `p` is inserted, then it returns `p`; otherwise, it
      returns a pointer to the tree node containing to `p->get_key()`

      @ingroup Trees
  */
  template <class Node, class Key,
            class Compare = Aleph::less<typename Node::key_type>>
  inline
  Node * search_or_insert_root_rec(Node *root, Node *p,
                                   const Compare & cmp = Compare()) noexcept
  {
    if (root == Node::NullPtr)
      return p; // insertion in empty tree

    if (cmp(KEY(p), KEY(root)))
      { // insert in left subtree
        Node *left_branch = search_or_insert_root_rec(LLINK(root), p, cmp);
        if (left_branch == p)
          {
            LLINK(root) = left_branch;
            root = rotate_to_right(root);
            return p;
          }

        return left_branch;
      }
    if (cmp(KEY(root), KEY(p)))
      { // insert in right subtree
        Node *right_branch = search_or_insert_root_rec(RLINK(root), p, cmp);
        if (right_branch == p)
          {
            RLINK(root) = right_branch;
            root = rotate_to_left(root);
            return p;
          }

        return right_branch;
      }

    return root;
  }


  /** Preorder iterator on the nodes of a binary tree.

    This class export a full iterator for the nodes of a binary tree
    where the visit order corresponds to a preorder sense.

    @ingroup Trees
*/
  template <class Node>
  class BinNodePrefixIterator
  {
    Node *root = nullptr;
    Node *curr = Node::NullPtr;
    ArrayStack<Node *> s;

  public:
    /// Swap `this`with `it`
    void swap(BinNodePrefixIterator & it) noexcept
    {
      std::swap(root, it.root);
      std::swap(curr, it.curr);
      s.swap(it.s);
    }

    BinNodePrefixIterator() = default;

    /// Initialize an iterator on the first node in preorder for the tree
    /// with root `__root`
    BinNodePrefixIterator(Node *__root) noexcept
      : root(__root), curr(root), s(Node::MaxHeight)
    {
      // empty
    }

    BinNodePrefixIterator(const BinNodePrefixIterator & it)
      : root(it.root), curr(it.curr), s(it.s)
    {
      // empty
    }

    BinNodePrefixIterator(BinNodePrefixIterator && it) noexcept
    {
      swap(it);
    }

    /// Reset the iterator to the first node in preorder sense
    void reset_first() noexcept
    {
      curr = root;
      s.empty();
    }

  private:
    // Helper function for finding last node
    static Node * last(Node *p) noexcept
    {
      if (RLINK(p) != Node::NullPtr)
        return last(RLINK(p));

      if (LLINK(p) != Node::NullPtr)
        return last(LLINK(p));

      return p;
    }

  public:
    /// Reset the iterator to the last node in preorder
    void reset_last() noexcept
    {
      curr = last(root);
      s.empty();
    }

    /// Put the iterator in end state
    void end() noexcept
    {
      curr = Node::NullPtr;
      s.empty();
    }

    BinNodePrefixIterator &operator =(const BinNodePrefixIterator & it)
    {
      if (this == &it)
        return *this;

      root = it.root;
      curr = it.curr;
      s = it.s;
      return *this;
    }

    BinNodePrefixIterator &operator =(BinNodePrefixIterator && it) noexcept
    {
      swap(it);
      return *this;
    }

    /// Return `true` if iterator has current node
    [[nodiscard]] bool has_curr() const noexcept { return curr != Node::NullPtr; }

    /// Return the current link guaranteeing no exception. Be careful.
    Node * get_curr_ne() const noexcept { return curr; }

    /// Return a pointer to current node
    Node * get_curr() const
    {
      ah_overflow_error_if(not has_curr()) << "Iterator overflow";
      return get_curr_ne();
    }

    /// Move the iterator one position forward guaranteeing no
    /// exception. Be careful.
    void next_ne() noexcept
    {
      auto l = LLINK(curr), r = RLINK(curr);
      if (l != Node::NullPtr)
        {
          curr = l;
          if (r != Node::NullPtr)
            s.push(r);
          return;
        }

      if (r != Node::NullPtr)
        {
          curr = r;
          return;
        }

      if (s.is_empty())
        curr = Node::NullPtr;
      else
        curr = s.pop();
    }

    /// Move the iterator one position forward
    void next()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator overflow";
      next_ne();
    }
  };

  /** Traverse a tree in preorder via its iterator and performs a conditioned
      operation on each item.

       `prefix_traverse(root, operation)` instantiates the internal
       iterator of the class and traverses each item performing
       `operation(p)`, where `p` is a node pointer.

       `operation` must have the following signature:

           bool operation(Node * p)

       If `operation(p)` returns `true` then the iterator is advanced
       and the next item processed. Otherwise. the traversal stops.

       \param root
       \param[in] op to be performed on each item
       \return `true` if all the nodes were visited (`operation` on each
       one always returned `true`) or `false` if the traversal was
       stopped because there was a `false` result on an item.
       \throw anything that could throw `operation`

       @ingroup Trees
   */
  template <class Node, class Op>
  bool prefix_traverse(Node *root, Op op)
  {
    for (BinNodePrefixIterator<Node> it(root); it.has_curr(); it.next_ne())
      if (not op(it.get_curr()))
        return false;
    return true;
  }


  /** Traverse in preorder all the container and performs an operation on each
      element.

      `prefix_for_each(operation)` traverses the container and on each element
      `item` is performed `operation(item)`.

      `operation` must have the following signature:

          void operation(Node * p)

      Overloading of this method allow that the signature can be
      lightly different; for example, remove the reference or the
      `const`.

      \param root
      \param op operation to be performed on each item
      \throw anything that can throw `operation`
   */
  template <class Node, class Op>
  void prefix_for_each(Node *root, Op op)
  {
    for (BinNodePrefixIterator<Node> it(root); it.has_curr(); it.next_ne())
      op(it.get_curr());
  }


  /** Inorder iterator on the nodes of a binary tree.

      This class export a full iterator for the nodes of a binary tree
      where the visit order corresponds to a inorder sense.

      @ingroup Trees
  */
  template <class Node>
  class BinNodeInfixIterator
  {
    mutable Node *root = Node::NullPtr;
    Node *curr = Node::NullPtr;
    long pos = 0;
    ArrayStack<Node *> s;

    Node * advance_to_min(Node *r) noexcept
    {
      while (LLINK(r) != Node::NullPtr)
        {
          s.push(r);
          r = LLINK(r);
        }
      return r;
    }

    static Node * advance_to_max(Node *r) noexcept
    {
      while (RLINK(r) != Node::NullPtr)
        r = RLINK(r);

      return r;
    }

    void init() noexcept
    {
      if (root != Node::NullPtr)
        curr = advance_to_min(root);
      pos = 0;
    }

  public:
    /// Return `true` if the iterator is on the first node
    bool is_in_first() const noexcept
    {
      return curr != Node::NullPtr and LLINK(curr) == Node::NullPtr and s.is_empty();
    }

    bool is_last() const noexcept
    {
      return curr != Node::NullPtr and RLINK(curr) == Node::NullPtr and s.is_empty();
    }

    void swap(BinNodeInfixIterator & it) noexcept
    {
      std::swap(root, it.root);
      std::swap(curr, it.curr);
      std::swap(pos, it.pos);
      s.swap(it.s);
    }

    BinNodeInfixIterator() = default;

    /// Initialize an iterator on the first node inorder
    BinNodeInfixIterator(Node *__root) noexcept
      : root(__root), s(Node::MaxHeight)
    {
      init();
    }

    BinNodeInfixIterator(const BinNodeInfixIterator & it)
      : root(it.root), curr(it.curr), pos(it.pos), s(it.s)
    {
      // empty
    }

    BinNodeInfixIterator(BinNodeInfixIterator && it) noexcept { swap(it); }

    /// Reset the iterator to the first node inorder
    void reset_first() noexcept
    {
      s.empty();
      init();
    }

    /// Reset the iterator to the first node inorder
    void reset_last() noexcept
    {
      s.empty();
      curr = advance_to_max(root);
      pos = -2;
    }

    void end() noexcept
    {
      s.empty();
      curr = Node::NullPtr;
      pos = -1;
    }

    BinNodeInfixIterator &operator =(const BinNodeInfixIterator & it)
    {
      if (this == &it)
        return *this;

      root = it.root;
      curr = it.curr;
      pos = it.pos;
      s = it.s;
      return *this;
    }

    BinNodeInfixIterator &operator =(BinNodeInfixIterator && it)
      noexcept
    {
      swap(it);
      return *this;
    }

    /// Return `true` the iterator has current node
    bool has_curr() const noexcept { return curr != Node::NullPtr; }

    /// Return the current link guaranteeing no exception. Be careful.
    Node * get_curr_ne() const noexcept { return curr; }

    /// Return the current node. Throw `overflow_error` if there is no current
    Node * get_curr() const
    {
      ah_overflow_error_if(not has_curr()) << "Iterator overflow";
      return curr;
    }

    /// Return the current position of iterator. Only valid if has_curr() == true
    size_t get_pos() const { return pos; }

    void next_ne() noexcept
    {
      ++pos;
      curr = RLINK(curr);
      if (curr != Node::NullPtr)
        {
          curr = advance_to_min(curr);
          return;
        }

      if (s.is_empty())
        curr = Node::NullPtr;
      else
        curr = s.pop();
    }

    /// Move the iterator one position forward. Throw `overflow_error` if
    /// there is no current
    void next()
    {
      ah_overflow_error_if(not has_curr()) << "Iterator overflow";
      next_ne();
    }
  };

  /** Traverse a tree in preorder via its iterator and performs a conditioned
      operation on each item.

       `prefix_traverse(root, operation)` instantiates the internal
       iterator of the class and traverses each item performing
       `operation(p)`, where `p` is a node pointer.

       `operation` must have the following signature:

           bool operation(Node * p)

       If `operation(p)` returns `true` then the iterator is advanced
       and the next item processed. Otherwise. the traversal stops.

       \param root
       \param[in] op operation to be performed on each item
       \return `true` if all the nodes were visited (`operation` on each
       one always returned `true`) or `false` if the traversal was
       stopped because there was a `false` result on an item.
       \throw anything that could throw `operation`

       @ingroup Trees
   */
  template <class Node, class Op>
  bool infix_traverse(Node *root, Op op)
  {
    for (BinNodeInfixIterator<Node> it(root); it.has_curr(); it.next_ne())
      if (not op(it.get_curr()))
        return false;
    return true;
  }

  /** \overload infix_traverse()

      @ingroup Trees
   */
  template <class Node, class Op>
  bool traverse(Node *root, Op op)
  {
    return infix_traverse(root, op);
  }

  /** Traverse all the container and performs an operation on each
      element.

      `for_each(operation)` traverses the container and on each element
      `item` is performed `operation(item)`.

      `operation` must have the following signature:

          void operation(Node * p)

      Overloading of this method allow that the signature can be
      lightly different; for example, remove the reference or the
      `const`.

      \param root
      \param[in] op `operation` to be done on each element.
      \throw anything that can throw `operation`
   */
  template <class Node, class Op>
  void infix_for_each(Node *root, Op op)
  {
    for (BinNodeInfixIterator<Node> it(root); it.has_curr(); it.next_ne())
      op(it.get_curr());
  }
} // end Aleph

# endif // TPL_BINNODEUTILS_H
