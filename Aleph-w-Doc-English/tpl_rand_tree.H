
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file tpl_rand_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_RAND_TREE_H
# define TPL_RAND_TREE_H

# include <limits.h>
# include <gsl/gsl_rng.h>
# include <ahUtils.H>
# include <ahFunction.H>
# include <tpl_randNode.H>
# include <tpl_binTreeOps.H>

using namespace Aleph;

namespace Aleph { 

/** 
  @brief Binary tree of generic randomized search.

  @details A randomized binary tree search is a binary tree
  search on which modification operations (insert and
  deletion) are made at random. Consequently,
  all operations on this class are binary tree
  \f$O(\lg n)\f$, regardless of
  any bias that exists about the order of insertion or
  removal of keys.

  @param NodeType Type of node that will manage the tree.
  it can be kind RandNode or RandNodeVtl
  as desired or may not have virtual destructors.
  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys. 

  @ingroup Arboles
*/
  template <template <typename> class NodeType, typename Key, class Compare>
class Gen_Rand_Tree
{
public:

/** @brief Binary node type. */
  typedef NodeType<Key> Node;

private:

  Node *    tree_root; 
  gsl_rng * r;
  Compare & cmp;

/** 
  @brief Insert randomly in a binary tree search with ranges.

  @details random_insert(root,node) Randomly inserted the "node" node
  binary tree search with ranges "root" independently
  the key value that contains "node". The binary tree of 
  resulting search is always random in the sense that
  It is equivalent to that produced by an insertion sequence
  random.

  The routine belongs to Gen_Rand_Tree class, but can be used
  in general for any kind of binary tree search
  rank derivative BinNodeXt. It is confined within
  Gen_Rand_Tree by the random number generator.

  @param[in] root The root of the binary tree search with ranges.
  @param[in] p Node to insert.

  @return if the "node" key is not in the tree
  binary, then the root of the binary tree is returned
  search ranges containing the new node; than
  otherwise, the routine returns Node::NullPtr.

  @see BinNodeXt
*/
  Node * random_insert(Node * root, Node * p)
  {
    const long & n = COUNT(root); 
    const size_t rn = gsl_rng_uniform_int(r, n + 1);

    /** Do you earn "p" the draw of being root? */
    if (rn == n) 
      return BinTreeXt_Operation<Node, Compare>(cmp).insert_root(root, p);

    Node * result;

    /** KEY(p) < KEY(root) ?
    If the answer is YES, then insert into the left tree */
    if (cmp(KEY(p), KEY(root))) 
      {
        result = random_insert(LLINK(root), p); 
   
    /** Did insertion?
    If the answer is YES then update branch and accountants */
        if (result != Node::NullPtr) 
          { 
            LLINK(root) = result;
            ++COUNT(root); 

            return root;
          }
      }

    /** KEY(p) > KEY(root) ?
    If the answer is YES, then insert into the right tree */
    else if (cmp(KEY(root), KEY(p))) 
      { 
        result = random_insert(RLINK(root), p); 

    /** Did insertion?
    If the answer is YES then update branch and accountants */
        if (result != Node::NullPtr) 
          {    
            RLINK(root) = result;
            ++COUNT(root); 

            return root;
          }
      }

    return Node::NullPtr; /* Clave duplicada ==> no hay inserción */
  }

/** 
  @brief Randomized insert and replicated into a binary tree
  search with ranges.

  @details random_insert_dup(root,node) randomly inserted "node" node in
  binary tree search with ranges "root" independently
  the key value that contains "node". The binary tree
  resulting search is always random in the sense that
  it is equivalent to that produced by an insertion sequence
  random.

  The routine belongs to Gen_Rand_Tree class, but can be used
  in general for any kind of binary tree search
  rank derivative BinNodeXt. It is confined within
  Gen_Rand_Tree by the random number generator.

  @param[in] root The root of the binary tree search with ranges. 
  @param[in] p Node to insert.

  @return The pointer p which is the new root of the tree.

  @see BinNodeXt
*/
  Node * random_insert_dup(Node * root, Node * p)
  {
    const long & n = COUNT(root); 
    const size_t rn = gsl_rng_uniform_int(r, n + 1);
    if (rn == n) /* ¿Gana p el sorteo de ser raíz? */
      return BinTreeXt_Operation<Node, Compare>(cmp).insert_dup_root(root, p); 

    if (cmp(KEY(p), KEY(root))) /* KEY(p) < KEY(root) ? */
      LLINK(root) = random_insert_dup(LLINK(root), p); 
    else
      RLINK(root) = random_insert_dup(RLINK(root), p); 
      
    ++COUNT(root); 

    return root;
  }

  Gen_Rand_Tree& operator = (const Gen_Rand_Tree&);

  void init(unsigned int seed)
  {
    r = gsl_rng_alloc (gsl_rng_mt19937);

    if (r == NULL)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

public:

/** @brief Returns a reference to the criterion of comparison. */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }


/** @brief You get a pointer to the object gsl_rng random number generator. */
  gsl_rng * gsl_rng_object() { return r;}

/** @brief Constructor; the random number generator is instantiated
for the mersen twister. */
  Gen_Rand_Tree(unsigned int seed, Compare && __cmp) 
    : tree_root(Node::NullPtr), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

  Gen_Rand_Tree(unsigned int seed, Compare & __cmp) 
    : tree_root(Node::NullPtr), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this
*/
void swap(Gen_Rand_Tree & tree)
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(r, tree.r);
    std::swap(cmp, tree.cmp);
  }

/** @brief destroyer; set free generator of random number */
  virtual ~Gen_Rand_Tree() 
  {
    if (r != NULL)
      gsl_rng_free(r);
  }

/** 
  @brief Insertion into a binary tree search randomized.

  @details insert(p) inserts the p node in the binary tree search 
  randomized "this".

  @param[in] p Node to insert.
  
  @return The pointer to the "p" node if the key "p" is not contained in
  randomized tree; NULL otherwise.
*/
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);
    assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
    assert(COUNT(p) == 1);

    Node * result = random_insert(tree_root, p);
    if (result == Node::NullPtr) 
      return NULL;

    return tree_root = result;
  }

/** 
  @brief Insertion into a binary tree search randomized replicability.

  @details insert_dup(p) inserts the p node in the binary tree search 
  randomized "this".

  @param[in] p Node to insert.

  @return The pointer to the p node.
*/
  Node * insert_dup(Node * p)
  {
    assert(p != Node::NullPtr);
    assert((LLINK(p) == Node::NullPtr) and (RLINK(p) == Node::NullPtr));
    assert(COUNT(p) == 1);

    return tree_root = random_insert_dup(tree_root, p);
  }

/** 
  @brief Random exclusive union of two binary trees search,
  exclusive with ranges.

  @details random_join_exclusive(tl,tr) joins two binary trees search
  ranges into one. By unique is meant that all the 
  keys of "tl" are smaller than all keys of "tr". To
  unlike join_exclusive(), random_join_exclusive() chooses
  randomly which of the roots of "tl" and "tr" will become root
  resulting binary tree. Consequently, the tree always
  it will be random.

  The routine can be used for any class of tree
  binary search with ranges derived of BinNodeXt. It
  presence in the Gen_Rand_Tree class due to the
  availability random number generator.

  @param[in] tl Binary tree search with ranges of minor keys.
  @param[in] tr Binary tree search with ranges of biggest keys.
  
  @return The resulting tree of the exclusive union.
  
  @note The routine does not validate that "tl" and "tr" are binary trees
  search and their ranges are not exclusive.

  @ingroup Arboles
  
  @see BinNodeXt join_exclusive() Gen_Rand_Tree::random_insert()
*/
  Node * random_join_exclusive(Node * tl, Node * tr)
  {
    if (tl == Node::NullPtr) 
      return tr;

    if (tr == Node::NullPtr) 
      return tl;

    const size_t & m = COUNT(tl);
    const size_t & n = COUNT(tr);
    const size_t rn = 1 + gsl_rng_uniform_int(r, m + n);  
    if (rn <= m) 
      {    
    /* Rama izquierda gana sorteo */
        COUNT(tl) += COUNT(tr);
        RLINK(tl) = random_join_exclusive(RLINK(tl), tr);

        return tl;
      }
    else 
      {
        COUNT(tr) += COUNT(tl);
        LLINK(tr) = random_join_exclusive(tl, LLINK(tr));

        return tr;
      }
  }

/** 
  @brief Elimination randomized in a binary tree with ranges.

  @details random_remove(root, key) search the binary tree with ranges the 
  "key" key and where to find a node containing that key,
  performed a randomized elimination through
  Gen_Rand_Tree::random_join_exclusive() of the hives of node 
  removed. The resulting binary search tree is always
  random in the sense that is equivalent to that produced by an
  random insertion sequence.   

  The routine can be used for any class of tree
  binary search with ranges derived of BinNodeXt. It
  presence in the Gen_Rand_Tree class due to the
  availability random number generator.

  @param[in,out] root Root of tree binary of ranges search.
  @param[in] key The key to delete.

  @return A pointer to the deleted node whether key is
  in the binary tree search with ranges; Node::NullPtr than
  otherwise.

  @see Gen_Rand_Tree::random_join_exclusive() BinNodeXt
*/
  Node * random_remove(Node *& root, const Key & key)
  {
    if (root == Node::NullPtr) 
      return Node::NullPtr;

    Node * ret_val;
    if (cmp(key, KEY(root)))
      {
        ret_val = random_remove(LLINK(root), key);
        if (ret_val != Node::NullPtr)
          COUNT(root)--;

        return ret_val;
      }
    else if (cmp(KEY(root), key))
      {
        ret_val = random_remove(RLINK(root), key);
        if (ret_val != Node::NullPtr)
          COUNT(root)--;

        return ret_val;
      }

    /* Clave encontrada */
    ret_val = root;
    root = random_join_exclusive(LLINK(root), RLINK(root));
    ret_val->reset();

    return ret_val;
  }

/** 
  @brief Deletes a key randomized binary search tree.

  @details remove(key) Search in the randomized binary tree search the
  "key" key and where to find a node containing that key,
  randomly makes its removal.

  @param[in] key The key to delete.

  @return A pointer to the deleted node whether key is
  in the binary tree search with ranges; NULL than
  otherwise.
*/
  Node * remove(const Key & key)
  {
    Node * ret_val = random_remove(tree_root, key);

    return ret_val != Node::NullPtr ? ret_val : NULL;
  }

/** @brief Search the "key" key in the randomized binary tree search. */
  Node * search(const Key & key)
  {
    Node * retVal = searchInBinTree<Node, Compare>(tree_root, key, cmp);

    return retVal == Node::NullPtr ? NULL : retVal;
  }

/** 
  @brief Search the p key in the binary tree search or inserts 
  in case if not found.

  @details search_or_insert(p) searches AVL tree a node whose key
  is <code>KEY(p)</code>. If the key is found, then returns a pointer 
  to the node that hosts it. Otherwise p inserted into the search 
  binary tree this.

  @param[in] p Node to find or insert.

  @return A pointer to "p" if the key "p" is not
  contained within the tree. Otherwise, it returns a
  pointer to the node in the tree containing
  <code>KEY(p)</code>.
*/
  Node * search_or_insert(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);

    Node * result = search(KEY(p));
    if (result != NULL)
      return result;

    tree_root = random_insert(tree_root, p);

    return p;
  }

  bool verify() const
  {
    return check_rank_tree(tree_root);
  }

/** @brief Gets a pointer to the root of the randomized binary tree search. */
  Node *& getRoot() { return tree_root; }

/** 
  @brief Returns the node whose infix position in the tree is "i".

  @details select(i) returns the node tree whose infix position is "i".

  @param[in] i Infix position to select.

  @return A pointer to node corresponding to the infix position "i".

  @throw out_of_range If "i" is greater or equal to the number of nodes
  containing the "treap".
*/
  Node * select(const size_t & i) const
  {
    return Aleph::select(tree_root, i); 
  }

/** @brief Returns the number of nodes containing the tree. */
  size_t size() const { return COUNT(tree_root); }

/** 
  @brief Returns the (ordered) infix position of the "key" key.

  @details position(key) search in the random tree the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key.

  @param[in] key The key to seek and to determine infix position.
  
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<long,Node*> position(const Key & key)
  {
    std::pair<long,Node*> ret_val;
    ret_val.first = BinTreeXt_Operation<Node, Compare> (cmp).
      inorder_position(tree_root, key, ret_val.second);

    return ret_val;
  }

/** 
  @brief Returns the (ordered) infix position of the "key" key.

  @details find_position(key) searches in the random tree the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key with the node. If the key
  not in the tree, position(key) returns the position where
  it should be if "key" belonged to the tree.

  The return value is a pair<int,Node*>. The "first" field is the
  infix position and the "second" the node.

  If key is not in the tree, then it can be
  distinguish three cases:
    -# If "key" is less than the minimum key tree, then "first"
    -1 and "second" is the node containing the minimum key.
    -# If the "key" is greater than the maximum tree key, then "first"
    is COUNT(r) (number of nodes in the tree) and "second" is
    the node containing the maximum key node.
    -# In any case, "first" is the position
    that would have the "key" key to tree and "second" node "p" is a key
    adjacent immediately to "key". Note that "second" can have a
    key minor or major, but is guaranteed to be immediately
    continuing to "key".
  .
  @param[in] key The key to seek and to determine infix position.
  
  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<int,Node*> find_position(const Key & key)
  {
    std::pair<int,Node*> r(-2, NULL);
    
    r.first = BinTreeXt_Operation<Node, Compare> (cmp) .
      find_position(tree_root, key, r.second);

    return r;
  }

private:

  Node * remove_pos(Node *& root, const size_t pos)
  {
    if (pos == COUNT(LLINK(root)))
      {
	Node * ret_val = root;
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

/** 
  @brief Removes the key in infix position "i".

  @param[in] i Position to be deleted

  @return A node removed
*/
  Node * remove_pos(const size_t i)
  {
    if (i >= COUNT(tree_root))
      throw std::out_of_range("infix position out of range");

    return remove_pos(tree_root, i);
  }

  bool split_key(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
  {
    return split_key_rec_xt(tree_root, key, t1,getRoot(), t2,getRoot());
  }

  void split_key_dup(const Key & key, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
  {
    split_key_dup_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  void split_pos(size_t pos, Gen_Rand_Tree & t1, Gen_Rand_Tree & t2)
  {
    split_pos_rec(tree_root, pos, t1.getRoot(), t2.getRoot());
  }

  void join(Gen_Rand_Tree & t, Gen_Rand_Tree & dup)
  {
    Node * l = (Node*) LLINK(t.tree_root);
    Node * r = (Node*) RLINK(t.tree_root);
    
    t.tree_root->reset();
    Node * ret = insert(t.tree_root);
    if (ret == NULL)
      dup.insert(t.tree_root);

    join(l, dup);
    join(r, dup);
  }

  void join_dup(Gen_Rand_Tree & t)
  {
    tree_root = random_join_exclusive(tree_root, t.tree_root);    
  }
};

/** 
  @brief Binary tree of randomized search of node without virtual destructor.

  @details A randomized binary tree search is a binary tree
  search on which modification operations (insert and
  deletion) are made at random. Consequently,
  all operations on this class are binary tree
  \f$O(\lg n)\f$, regardless of
  any bias that exists about the order of insertion or
  removal of keys.

  The Rand_Tree class manages the nodes without virtual destructor.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys.

  @see Rand_Tree_Vtl DynSetRandTree DynMapBinTree

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Rand_Tree : public Gen_Rand_Tree<RandNode, Key, Compare> 
{
public:

/** @brief Starts the random number generator with the value "seed" and
sets the comparison operation "cmp" */
  Rand_Tree(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Rand_Tree<RandNode, Key, Compare>(seed, std::move(cmp))
  {
    /* Empty */
  }

  Rand_Tree(Compare && cmp = Compare()) 
    : Gen_Rand_Tree<RandNode, Key, Compare>(time(NULL), std::move(cmp))
  {
    /* Empty */
  }

  Rand_Tree(Compare & cmp) 
    : Gen_Rand_Tree<RandNode, Key, Compare>(time(NULL), cmp)
  {
    /* Empty */
  }

  Rand_Tree(unsigned int seed, Compare & cmp) 
    : Gen_Rand_Tree<RandNode, Key, Compare>(seed, cmp)
  {
    /* Empty */
  }
};

/** 
  @brief Binary tree of randomized search of node with virtual destructor.

  @details A randomized binary tree search is a binary tree
  search on which modification operations (insert and
  deletion) are made at random. Consequently,
  all operations on this class are binary tree
  \f$O(\lg n)\f$, regardless of
  any bias that exists about the order of insertion or
  removal of keys.

  The Rand_Tree_Vtl class manages the nodes with virtual destructor.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys. 

  @see Rand_Tree DynSetRandTree DynMapBinTree

  @ingroup Arboles
*/
    template <typename Key, class Compare = Aleph::less<Key>>
class Rand_Tree_Vtl : public Gen_Rand_Tree<RandNodeVtl,Key,Compare>
{
public:

/** @brief Starts the random number generator with the value "seed" and
sets the comparison operation "cmp" */
  Rand_Tree_Vtl(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Rand_Tree<RandNodeVtl, Key, Compare>(seed, std::move(cmp))
  {
    /* Empty */
  }

  Rand_Tree_Vtl(Compare && cmp = Compare()) 
    : Gen_Rand_Tree<RandNodeVtl, Key, Compare>(time(NULL), std::move(cmp))
  {
    /* Empty */
  }

  Rand_Tree_Vtl(unsigned int seed, Compare & cmp) 
    : Gen_Rand_Tree<RandNodeVtl, Key, Compare>(seed, cmp)
  {
    /* Empty */
  }

  Rand_Tree_Vtl(Compare & cmp) 
    : Gen_Rand_Tree<RandNodeVtl, Key, Compare>(time(NULL), std::move(cmp))
  {
    /* Empty */
  }
};


} /* End namespace Aleph */

# endif // TPL_RAND_TREE_H 
