
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# ifndef TPL_TREAP_H
# define TPL_TREAP_H

# include <gsl/gsl_rng.h>
# include <ahFunction.H>
# include <tpl_binNodeUtils.H>
# include <treapNode.H>
# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph
{

  /** Treap (a special type of ramdomized binary search tree).

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its operations.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). Since the priority is chosen
      just one time and the adjustements are done in a botton-top way
      (by contrast with the randomized which is top-bottom), the treap
      takes less time.

      The class internally uses the gsl random number generator of
      [GSL - GNU Scientific Library](\https://www.gnu.org/software/gsl/).
      By default, the Mersene twister is used and the seed is taken from
      system time.

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Treap
{
public:

  using Node = NodeType<Key>;  /// The type of node

private:

  Node      head;
  Node *    head_ptr;
  Node *&   tree_root;
  gsl_rng * r;
  Compare cmp;

  void init(unsigned int seed)
  {
    PRIO(head_ptr) = Min_Priority;
    r = gsl_rng_alloc (gsl_rng_mt19937);

    if (r == nullptr)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

  public:

  /// Set the random number generator seed
  void set_seed(unsigned long seed) noexcept { gsl_rng_set(r, seed); }

   /// Swap in constant time all the nodes of `this` with `tree`
  void swap (Gen_Treap & tree) noexcept
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(cmp, tree.cmp);
    std::swap(r, tree.r);
  }

  /// return the comparison criteria
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  /// Initialize a treap with random `seed` and comparison criteria
  /// `__cmp`
  Gen_Treap(unsigned long seed, Compare & __cmp = Compare())
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(nullptr), cmp(__cmp)
  {
    init(seed);
  }

  /// Initialize a treap with random seed from system time and
  /// comparison criteria `cmp`
  Gen_Treap(Compare cmp = Compare())
    : Gen_Treap(time(nullptr), cmp) { /* empty */ }

  ~Gen_Treap()
  {
    if (r != nullptr)
      gsl_rng_free(r);
  }

  /// Get a pointer to gsl random number generator
  gsl_rng * gsl_rng_object() noexcept { return r;}

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

  Node * insert(Node * root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return p;

    Node * insertion_result = nullptr;
    if (cmp(KEY(p), KEY(root)))
      {
        insertion_result = insert(LLINK(root), p);
        if (insertion_result == Node::NullPtr)
          return Node::NullPtr;

        LLINK(root) = insertion_result;
        if (PRIO(insertion_result) < PRIO(root))
          return rotate_to_right(root);
        else
          return root;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
        insertion_result = insert(RLINK(root), p);
        if (insertion_result == Node::NullPtr)
          return Node::NullPtr;

        RLINK(root) = insertion_result;
        if (PRIO(insertion_result) < PRIO(root))
          return rotate_to_left(root);
        else
          return root;
      }

    return Node::NullPtr;
  }

  Node * search_or_insert (Node *& root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
        Node * ret = search_or_insert(LLINK(root), p);
        if (ret == p) // p was inserted?
          if (PRIO(LLINK(root)) < PRIO(root))
            root = rotate_to_right (root);

        return ret;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
        Node * ret = search_or_insert(RLINK(root), p);
        if (ret == p) // p was inserted?
          if (PRIO(RLINK(root)) < PRIO(root))
            root = rotate_to_left (root);

        return ret;
      }

    return root;
  }

  Node * insert_dup(Node * root, Node * p) noexcept
  {
    if (root == Node::NullPtr)
      return p;

    if (cmp(KEY(p), KEY(root)))
      {
        Node * result = LLINK(root) = insert_dup(LLINK(root), p);
        if (PRIO(result) < PRIO(root))
          return rotate_to_right(root);
        else
          return root;
      }
    else
      {
	Node * result = RLINK(root) = insert_dup(RLINK(root), p);
	if (PRIO(result) < PRIO(root))
	  return rotate_to_left(root);
	else
	  return root;
      }
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

    PRIO(p)       = gsl_rng_get(r); // selección aleatoria de prioridad
    Node * result = insert(tree_root, p);
    if (result == Node::NullPtr)
      return nullptr;

    tree_root = result;

    return p;
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

    PRIO(p) = gsl_rng_get(r); // selección aleatoria de prioridad

    return search_or_insert(tree_root, p);
  }

  /** Insert a node in the tree.

      This method does not fail. It always inserts.

      @param[in] p pointer to the node to insert
      @return the `p` pointer
  */
  Node * insert_dup(Node * p) noexcept
  {
    assert(p != Node::NullPtr);

    PRIO(p)   = gsl_rng_get(r); // selección aleatoria de prioridad
    tree_root = insert_dup(tree_root, p);

    return p;
  }

  /// Return `true` if `this` is a consistent treap
  bool verify() const noexcept { return is_treap(tree_root); }

  /** Remove a key from the tree

      @param[in] key to remove
      @return a valid pointer to the removed node if `key` was found
      in the tree, `nullptr` otherwise
  */
  Node * remove(const Key & key) noexcept
  {
    Node ** pp = &RLINK(head_ptr);
    Node * p   = tree_root;

    while (p != Node::NullPtr) // descender buscando la clave
      if (cmp(key, KEY(p)))
        {
          pp = &LLINK(p);
          p  = LLINK(p);
        }
      else if (cmp(KEY(p), key))
        {
          pp = &RLINK(p);
          p  = RLINK(p);
        }
      else
        break; // encontrada!

    if (p == Node::NullPtr)
      return nullptr; // clave no fue encontrada

        // rotar p hasta que devenga hoja
    while (not (LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr))
      if (PRIO(LLINK(p)) < PRIO(RLINK(p)))
        {
          *pp = rotate_to_right(p);
          pp  = &RLINK(*pp);
        }
      else
        {
          *pp = rotate_to_left(p);
          pp  = &LLINK(*pp);
        }

    *pp = Node::NullPtr;

    p->reset();

    return p;
  }

private:

  static Node * join_exclusive(Node * t1, Node * t2) noexcept
  {
    if (t1 == Node::NullPtr)
      return t2;

    if (t2 == Node::NullPtr)
      return t1;

    if (PRIO(t1) < PRIO(t2))
      {
	RLINK(t1) = join_exclusive(RLINK(t1), t2);
	return t1;
      }
    else
      {
	LLINK(t2) = join_exclusive(t1, LLINK(t2));
	return t2;
      }
  }

  Node * remove(Node *& root, const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return Node::NullPtr;

    if (cmp(key, KEY(root)))
      return remove(LLINK(root), key);
    else if (cmp(KEY(root), key))
      return remove(RLINK(root), key);

    Node * ret = root;     // key found!
    root = join_exclusive(LLINK(root), RLINK(root));
    ret->reset();

    return ret;
  }

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

      `join(t, dup)` produces a treap result of join of `this` and
      `t`. The resulting tree is stored in `this`.

      Nodes containing duplicated keys are inserted in the randomized
      tree `dup`. The nodes could belong to any of two trees

      @param[in] t ramdomized tree to join with `this`
      @param[out] dup ramdomized tree where nodes containing duplicated
      keys are inserted
   */
  void join(Gen_Treap & t, Gen_Treap & dup) noexcept
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
  void join_dup(Gen_Treap & t) noexcept
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
  void join_exclusive(Gen_Treap & t) noexcept
  {
    tree_root = join_exclusive(tree_root, t.tree_root);    
    t.tree_root = Node::NullPtr;
  }

   /** Split the tree according to a key

      @param[in] key for splitting
      @param[out] t1 tree with keys lesser than `key`
      @param[out] t2 tree with keys greater than `key`
      @return `true` if tree was split; that is if `key` is not in the
      tree. Otherwise, if `key` is in the tree, `false` is returned
  */
  bool split_key(const Key & key, Gen_Treap & t1, Gen_Treap & t2)
  {
    return split_key_rec(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  /** Split the tree according to a key that could be in the tree

      `split_dup(key, t1, t2)` splits the tree according to `key` in two
      trees `t1` which contains the key lesser than `key` and `t2` which
      contains the keys greater or equal than `key`. 

      @param[in] key for splitting
      @param[out] t1 resulting tree with the keys lesser than `key`
      @param[out] t2 resulting tree with the keys greater or equal than `key`
  */
  void split_key_dup(const Key & key, Gen_Treap & t1, Gen_Treap & t2)
  {
    split_key_dup_rec(tree_root, key, t1.getRoot(), t2.getRoot());
  }

   /** Iterator on nodes of the tree.

      The visit order is inorder.

      @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(Gen_Treap & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};

/** Treap (a special type of ramdomized binary search tree) uing nodes
    without virtual destructor.

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its operations.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). However, since the priority is
      chosen just one time and the adjustements are done in a botton-top
      way (by contrast with the randomized which is top-bottom), the
      treap takes less time.
      
      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific
      Library](\https://www.gnu.org/software/gsl/).
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */ 
    template <typename Key, class Compare = Aleph::less<Key>>
struct Treap : public Gen_Treap<TreapNode, Key, Compare> 
{
  using Base = Gen_Treap<TreapNode, Key, Compare>;
  using Base::Base;
};

  
/** Treap (a special type of ramdomized binary search tree) using nodes
    with virtual destructors.

      The treap is a binary search tree whose very high performance is
      achieved by randomization. The basic idea is to store a priority
      value in each node which is randomly chosen. By the side of keys,
      the tree a binary search, but by the side of priorities, the tree
      is a heap. It is shown that this class of tree has an expected
      perfomance of \f$O(\lg n)\f$ for the majority of its operations.

      The treap is faster than the randomized tree by a constant time
      (both approaches are logarithmic). However, since the priority is
      chosen just one time and the adjustements are done in a botton-top
      way (by contrast with the randomized which is top-bottom), the
      treap takes less time.
      
      The class internally uses the gsl random number generator of 
      [GSL - GNU Scientific
      Library](\https://www.gnu.org/software/gsl/). 
      By default, the Mersene twister is used and the seed is taken from
      system time. 

      @ingroup Arboles
      @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl Rand_Tree
   */
    template <typename Key, class Compare = Aleph::less<Key>>
struct Treap_Vtl : public Gen_Treap<TreapNodeVtl, Key, Compare>
{
  using Base = Gen_Treap<TreapNodeVtl, Key, Compare>;
  using Base::Base;
};


} // end namespace Aleph
# endif // TPL_TREAP_H

