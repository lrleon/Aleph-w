
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
  @file tpl_test_treapRk.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_TREAPRK_H
# define TPL_TREAPRK_H

# include <gsl/gsl_rng.h>
# include <ahFunction.H>
# include <tpl_binTreeOps.H>
# include <ran_array.h>
# include <treapNode.H>

using namespace Aleph;

namespace Aleph {

class TreapRkNode_Data
{
  unsigned long priority; 

  unsigned long count;

public:

  TreapRkNode_Data() : priority(Max_Priority), count(1) 
  { 
    /* Empty */ 
  }

  TreapRkNode_Data(SentinelCtor) 
    : priority(Max_Priority), count(0) 
  { 
    /* Empty */ 
  }

  unsigned long & getPriority() { return priority; }

  unsigned long & getCount() { return count; }

  void reset() 
  {
    priority = Max_Priority; 
    count = 1;
  }
};

DECLARE_BINNODE_SENTINEL(Treap_Rk_Node, 80, TreapRkNode_Data);


/** 
  @brief Extended binary tree of generic randomized search of type
  Treap. 

  @details An extended treap is a special class of binary tree
  search in which change operations are randomized. 
  Consequently, all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The character "extended" provides the following benefits on the
  set of elements:
  -# Insertion, search and deletion of a key.
  -# Access to the i-th element of infix tour.
  -# Knowledge of the infix position given a key.
  -# Union and partition as a keyword or infix position.
  .
  All these operations are performed in expected time of \f$O(\lg
  n)\f$. 

  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster. 

  In performance studies, the standard class set<T>
  and map<T> implanted by TreapRk type show more
  faster than implementations gnu and Boost.

  @param NodeType Type of node that will manage the tree. It may be
  TreapNode or TreapNodeVtl type as desired or may not have
  virtual destructors. 
  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys.

  @ingroup Arboles

  @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl set multiset map multimap
*/
    template <template <class> class NodeType, class Key, class Compare>
class Gen_Treap_Rk
{
public:

/** @brief The type of internal node. */
  typedef NodeType<Key> Node;

private:

  Node      head;
  Node *    head_ptr;
  Node *&   tree_root;
  gsl_rng * r;
  Compare & cmp;

  void init(unsigned int seed)
  {
    PRIO(head_ptr) = Min_Priority;
    r = gsl_rng_alloc(gsl_rng_mt19937);

    if (r == NULL)
      throw std::bad_alloc();

    gsl_rng_set(r, seed % gsl_rng_max(r));
  }

public:

/** @brief Returns a reference to the comparison criterion */
  Compare & key_comp() { return cmp; }

/** @overload key_comp() */
  Compare & get_compare() { return key_comp(); }

/** @brief Constructor; initialize random number generator */
  Gen_Treap_Rk(unsigned int seed, Compare & __cmp) 
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

  Gen_Treap_Rk(unsigned int seed, Compare && __cmp) 
    : head_ptr(&head), tree_root(RLINK(head_ptr)), r(NULL), cmp(__cmp)
  {
    init(seed);
  }

  ~Gen_Treap_Rk()
  {
    if (r != NULL)
      gsl_rng_free(r);
  }

/** 
  @brief Builder copy "root".

  @details This constructor is provided for purposes of "temporary use". No
  performes copy of tree state.
*/
  Gen_Treap_Rk(const Gen_Treap_Rk &) 
  {
    /* Empty */
  }

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this
*/
  void swap(Gen_Treap_Rk & tree)
  {
    std::swap(tree_root, tree.tree_root);
    std::swap(cmp, tree.cmp);
    std::swap(r, tree.r);
  }

/** @brief Returns the root of the "treap" extended. */
  Node *& getRoot() { return tree_root; }

/** @brief Search the "key" key in the "treap" extended "this" */
  Node * search(const Key & key)
  {
    Node* ret_val = searchInBinTree<Node, Compare>(tree_root, key, cmp);

    return ret_val == Node::NullPtr ? NULL : ret_val;
  }

private:

    /* Retorna true si la clave fue insertada 
    (si no se encuentra en el árbol) */
  bool insert(Node *& root, Node * p) 
  {
    if (root == Node::NullPtr)
      {
	root = p;
	return true;
      }

    if (cmp(KEY(p), KEY(root)))
      {
	if (insert(LLINK(root), p))
	  {
	    ++COUNT(root);
	    if (PRIO(LLINK(root)) < PRIO(root) )
	      root = rotate_to_right_xt(root);

	    return true;
	  }
      }
    else if (cmp(KEY(root), KEY(p)))
      {
	if (insert(RLINK(root), p))
	  {
	    ++COUNT(root);
	    if (PRIO(RLINK(root)) < PRIO(root) )
	      root = rotate_to_left_xt(root);

	    return true;
	  }
      }

    return false;
  }

public:

/** @brief Search or insert "p". Returns "p" if KEY(p) is not within the tree;
otherwise, the address of the node contentivo KEY(p) is returned */
  Node * search_or_insert(Node *& root, Node * p) 
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
	Node * ret = search_or_insert(LLINK(root), p);

    /** Did insertion?
    If the answer is YES, then increase counters and eventually
    then rebalance */
        if (ret == p) 
	  {         
	    ++COUNT(root);
	    if (PRIO(LLINK(root)) < PRIO(root))
	      root = rotate_to_right_xt(root);
	    
	    assert(PRIO(root) <= PRIO(LLINK(root)) and
	      PRIO(root) <= PRIO(LLINK(root)));
	  }

	return ret;
      }
    else if (cmp(KEY(root), KEY(p)))
      {
	Node * ret = search_or_insert(RLINK(root), p);

    /** Did insertion?
    If the answer is YES, then increase counters and eventually
    then rebalance */
        if (ret == p) 
	  {     
	        
	    ++COUNT(root);
	    if (PRIO(RLINK(root)) < PRIO(root))
	      root = rotate_to_left_xt(root);

	    assert(PRIO(root) <= PRIO(LLINK(root)) and
	      PRIO(root) <= PRIO(LLINK(root)));
	  }

	return ret;
      }

    assert(PRIO(root) <= PRIO(LLINK(root)) and PRIO(root) <= PRIO(LLINK(root)));

    return root; /* root contiene KEY(p) */
  }

  /* Retorna p */
  Node * insert_dup(Node *& root, Node * p) 
  {
    if (root == Node::NullPtr)
      return root = p;

    if (cmp(KEY(p), KEY(root)))
      {
        insert_dup(LLINK(root), p);
	++COUNT(root);
        if (PRIO(LLINK(root)) < PRIO(root))
          root = rotate_to_right_xt(root);
      }
    else
      {
	insert_dup(RLINK(root), p);
	++COUNT(root);
	if (PRIO(RLINK(root)) < PRIO(root))
	  root = rotate_to_left_xt(root);
      }

    return p;
  }

/** 
  @brief Insert the "p" node in the "treap" extended "this".

	insert(p) insert the "p" node in the "treap" extended "this".

	@param[in] p Node to insert.
	
  @return A pointer to the newly inserted node if your password was not
  contained in treap; NULL otherwise.
*/
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return insert(tree_root, p) ? p : NULL;
  }


/**
  @brief Insert the "p" node in the "treap" extended "this" with the possibility of
  duplication.

	@details insert_dup(p) insert the "p" node in the "treap" extended "this".

	@param[in] p Node to insert.
	
  @return A pointer to the newly node inserted.
*/
  Node * insert_dup(Node * p)
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return insert_dup(tree_root, p);
  }

/**
  @brief Search or insert the key KEY(p).
	
	@details search_or_insert() searches the "treap" node containing the
  key KEY(p). If such a node is found, then it is
  returns; otherwise, "p" is inserted and is returned as
  result.

	@param[in] p node to eventually insert.
	
  @return If KEY(p) is in the tree, then it returns
  a pointer to the node contentivo KEY(p). Otherwise, the
  insert "p" in "treap" and "p" is returned.
*/
  Node * search_or_insert(Node * p)
  {
    assert(p != Node::NullPtr);

    PRIO(p) = gsl_rng_get(r);

    return search_or_insert(tree_root, p);
  }

  bool verify() { return is_treap(tree_root); }

private:

  static Node * join_treaps(Node * t1, Node * t2)
  {
    if (t1 == Node::NullPtr)
      return t2;

    if (t2 == Node::NullPtr)
      return t1;

    if (PRIO(t1) < PRIO(t2) )
      {
	COUNT(t1) += COUNT(t2);
	RLINK(t1) = join_treaps(RLINK(t1), t2);
	
	return t1;
      }
    else
      {
	COUNT(t2) += COUNT(t1);
	LLINK(t2) = join_treaps(t1, LLINK(t2) );
	
	return t2;
      }
  }

  Node * remove(Node *& root, const Key & key)
  {
    if (root == Node::NullPtr)
      return Node::NullPtr;

    Node * ret_val;

    if (cmp(key, KEY(root) ))
      ret_val = remove(LLINK(root), key);
    else if (cmp(KEY(root), key))
      ret_val = remove(RLINK(root), key);
    else
      {
	ret_val = root;
	root = join_treaps(LLINK(root), RLINK(root) );
	
	return ret_val;
      }

    if (ret_val == Node::NullPtr)
      return Node::NullPtr;
      
    --COUNT(root);

    return ret_val;
  }

public:

/**
  @brief Eliminates the "key" key of "treap" extended "this"..

  @details remove(key) searches for the "key" key in the "treap" extended "this" and,
  found the key, removes "treap" node.

  @param[in] key The key to delete.
  
  @return A pointer to the newly deleted node if the key is
  in the treap; NULL otherwise.
*/
  Node * remove(const Key & key)
  {
    Node * ret_val = remove(tree_root, key);

    if (ret_val == Node::NullPtr)
      return NULL;

    ret_val->reset();

    return ret_val;
  } 

/**
  @brief Eliminates the "treap" extended all keys between the
  infix position "beg" and "end".

  @details rmeove(beg,end) remove the "treap" extended "this" all keys
  between infix positions "beg" and "end". Returns a tree which is 
  contains the keys deleted.

  @param[in] beg Infix position where begins the range to be removed.
  @param[in] end Infix position where ends the range to be removed.

  @return A pointer to the root of "treap" extended containing
  all keys removed.

  @throw range_error If the range infix positions is invalid.
*/
  Node * remove(const size_t beg, const size_t end)
  {
    if (beg > end or end > COUNT(tree_root))
      throw std::range_error("remove of TreapRk out of range");

    Node * before_beg, * after_end;

    Node * ret_val = tree_root;

    split_pos_rec(ret_val, end + 1, ret_val, after_end);
    split_pos_rec(ret_val, beg, before_beg, ret_val);

    tree_root = join_treaps(before_beg, after_end);

    return ret_val;
  }

private:

  static Node * remove_pos(Node *& root, const size_t pos)
  {
    if (pos == COUNT(LLINK(root)))
      {
	Node * ret_val = root;
	root = join_treaps(LLINK(ret_val), RLINK(ret_val));
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

  @param[in] Position to be deleted

  @return Node removed
*/
  Node * remove_pos(const size_t pos)
  {
    if (pos >= COUNT(tree_root))
      throw std::out_of_range("infix position out of range");

    return remove_pos(tree_root, pos);
  }

/**
  @brief Returns the node whose infix position in the "treap" extended is "i".

  @details select(i) returns the "treap" node extended whose infix position 
  is "i".

  @param[in] i Infix position to select.

  @return A pointer to node corresponding to the infix position "i".

  @throw out_of_range If "i" is greater or equal to the number of nodes
  containing the "treap".
*/
  Node * select(const size_t & i)
  {
    return Aleph::select(tree_root, i); 
  }

/** @brief Returns the amount of nodes containing the "treap". */
  size_t size() const 
  {
    return COUNT(tree_root);
  }

/** @brief Returns true if the "treap" is empty. */
  bool is_empty() const 
  {
    return tree_root == Node::NullPtr; 
  }

/**
  @brief Returns the (ordered) infix position of the "key" key.

  @details position(key) search in the extended treap the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key.

  @param[in] key The key to seek and to determine infix position.

  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<int, Node*> position(const Key & key)
  {
    std::pair<int, Node*> ret_val;

    ret_val.first = BinTreeXt_Operation<Node, Compare>(cmp).
      inorder_position(tree_root, key, ret_val.second);

    return ret_val;
  }

/**
  @brief Returns the (ordered) infix position of the "key" key.

  @details find_position(key) searches in the extended tree the "key" key (which
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
  immediately adjacent to "key". Note that "second" can have a
  key minor or major, but is guaranteed to be immediately
  continuing to "key".
  .

  @param[in] key The key to seek and to determine infix position.

  @return A pair containing the infix position of the "key" key
  within the ordered set with the node if the key is in the mapping; 
  otherwise, it returns -1 and node is undetermined.
*/
  std::pair<int, Node*> find_position(const Key & key)
  {
    std::pair<int, Node*> r(-2, NULL);

    r.first = BinTreeXt_Operation <Node, Compare>(cmp) .
      find_position(tree_root, key, r.second);

    return r;
  }

  bool split_key(const Key & key, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2)
  {
    return split_key_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  void split_key_dup(const Key & key, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2)
  {
    split_key_dup_rec_xt(tree_root, key, t1.getRoot(), t2.getRoot());
  }

  void split_pos(size_t pos, Gen_Treap_Rk & t1, Gen_Treap_Rk & t2)
  {
    split_pos_rec(tree_root, pos, t1.getRoot(), t2.getRoot());
  }

  void join(Gen_Treap_Rk & t, Gen_Treap_Rk & dup)
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

  void join_dup(Gen_Treap_Rk & t)
  {
    tree_root = join_treaps(tree_root, t.tree_root);    
  }

/**
  @brief Iterator over the nodes of an extended "treap".

  @details This class lets you scroll through the nodes of a "treap" shaped
  ordered according to the criteria specified in comparison
  instantiation time "tTreap".

  @ingroup Arboles
*/
  class Iterator
  {
  protected:
    
    mutable Gen_Treap_Rk * tree_ptr; 
    mutable Node *         curr;
    mutable int            curr_pos;

    static const int Pos_Not_Current     = -1;
    static const int Pos_Empty_Container = -2;
    static const int Pos_Not_Updated     = -3;

  private:

    bool is_container_empty() const
    {
      return COUNT(tree_ptr->getRoot()) == 0;
    }

    bool pos_updated() const
    {
      return curr_pos != Pos_Not_Updated;
    }

    bool curr_updated() const
    {
      return curr != NULL;
    }

    bool is_updated() 
    {
      return pos_updated() and curr_updated();
    }

    void update_pos() const
    {
      assert(curr != NULL);

      curr_pos = BinTreeXt_Operation<Node, Compare>(tree_ptr->cmp).
	inorder_position(tree_ptr->getRoot(), KEY(curr), curr);
    }

    void update_curr() const
    {
      assert(curr_pos != Pos_Not_Updated);

      if (curr_pos == Pos_Empty_Container or curr_pos == Pos_Not_Current or
	  curr_pos == COUNT(tree_ptr->getRoot()))
	return;

      curr = Aleph::select(tree_ptr->getRoot(), curr_pos);
    }

  public:

/** @brief Empty constructor; It makes no sense if you do not assign a treap. */
    Iterator() : tree_ptr(NULL), curr(NULL), curr_pos(Pos_Not_Current)
    {
      /* Empty */ 
    }

/** @brief Instance an iterator from the child node of treap "__tree" */
    Iterator(Gen_Treap_Rk & __tree) 
      : tree_ptr(&__tree), curr(NULL)
    {
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

/** @brief Instance an iterator from the "__curr" node of treap "__tree" */
    Iterator(Gen_Treap_Rk & __tree, Node * __curr)
      : tree_ptr(&__tree), curr(__curr), curr_pos(Pos_Not_Updated)
    {
      /* Empty */
    }

/** @brief An iterator instance from the "post" position */
    Iterator(Gen_Treap_Rk & __tree, const size_t & pos)
      : tree_ptr(&__tree), curr(NULL), curr_pos(pos)
    {
      /* Empty */
    }

/** @brief An iterator instance from the state of the iterator "itor" */
    Iterator(const Iterator & itor)
      : tree_ptr(itor.tree_ptr), curr(itor.curr), curr_pos(itor.curr_pos)
    {
      /* Empty */
    }

/** @brief Assigns the iterator "this" to iterator "itor" */
    Iterator & operator = (const Iterator & itor)
    {
      if (this == &itor)
	return *this;

      tree_ptr = itor.tree_ptr;
      curr = itor.curr;
      curr_pos = itor.curr_pos;

      return *this;
    }

/** @brief Resets the iterator to the last node (lower) of treap.  */
    void reset_first()
    {
      curr = NULL;
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

/** @brief Resets the iterator to the last node (greater) of treap. */
    void reset_last()
    {
      curr = NULL;
      curr_pos = (is_container_empty() ? Pos_Empty_Container : 
		  COUNT(tree_ptr->getRoot()) - 1);
    }

/**
  @brief Restart the current position of the iterator to the item with value
  "key" key.

  @param[in] key The key to place as the current position.

  @throw domain_error If the key is not contained in the tree.
*/
    void reset_to_key(const Key & key)
    {
      curr_pos = BinTreeXt_Operation<Node, Compare>(tree_ptr->cmp).
	inorder_position(tree_ptr->getRoot(), key, curr);
    }

/** 
  @brief Restart the current position of the iterator to "node" node.

  @param[in] node Node to place as the current position.

  @note No check is performed about the node
  inside or outside the "treap".
*/
    void reset_to_node(Node * node)
    {
      curr = node;
      curr_pos = -2;
    }

/** @brief Place the current position of the iterator in the "post" position  */
    void reset_to_pos(size_t pos)
    {
      curr = NULL;
      curr_pos = pos;
    }

/** @brief Returns the current node. */
    Node * get_current() const
    {
      if (not curr_updated())
	update_curr();

      return curr; 
    }

/** @brief Returns the current node. */
    Node * get_curr() const
    {
      return get_current();
    }

/** 
      @brief Returns the infix position of the current node on which
      this iterator is positioned.
	    
      @throw underflow_error If the iterator is overwhelmed by below.
      @throw overflow_error If the iterator is overflowing above. 
*/
    size_t get_current_position() const
      throw(std::exception, std::underflow_error, std::overflow_error)
    {
      if (not pos_updated())
	update_pos();

      if (curr_pos < -1 )
	throw std::range_error("TreapRk iterator has not current");
	
      if (curr_pos > COUNT(tree_ptr->getRoot() ) )
	throw std::range_error("TreapRk iterator has not current");

      return curr_pos;
    }

/** @brief Returns true if the iterator is positioned on a current element. */
    bool has_current() const
    {
      if (not pos_updated())
	update_pos();

      return curr_pos >= 0 and curr_pos < COUNT(tree_ptr->getRoot());
    }

/** @brief Returns true if the iterator is positioned on a current element. */
    bool has_curr() const
    {
      return has_current();
    }

/** @brief Advances the iterator one position backward.  */
    void prev() throw(std::exception, std::underflow_error)
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      --curr_pos;
      curr = NULL;
    }

/** @brief Advances the iterator one position forward.  */
    void next() throw(std::exception, std::overflow_error)
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      ++curr_pos;
      curr = NULL;
    }

/** @brief Deletes the current node on which the iterator is positioned;
it is positioned in the following deleted. */
    Node * del()
    {
      if (not has_current() )
	throw std::underflow_error("TreapRk iterator has not current");

      if (not curr_updated())
	update_curr();

      Node * ret_val = tree_ptr->remove(KEY(curr) );
      
      curr = NULL;

      return ret_val;
    }

/** @brief Returns true if "*this" is on the same node as "itor".  */
    bool operator == (const Iterator & itor) const
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
  
/** @brief Returns true if "*this" is not equal to "itor" */
    bool operator != (const Iterator & itor) const
    {
      return not (*this == itor);
    }

    bool verify(Gen_Treap_Rk * r) const
    {
      return tree_ptr->getRoot() == r->getRoot();
    }

    bool verify(const Iterator & it) const
    {
      return tree_ptr->getRoot() == it.tree_ptr->getRoot();
    }
  }; /* End class Iterator */
};

/**
  @brief Extended binary tree of generic randomized search of type
  Treap with nodes no virtual destructor.

  @details An extended treap is a special class of binary tree
  search in which change operations are randomized. 
  Consequently, all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The character "extended" provides the following benefits on the
  set of elements:
  -# Insertion, search and deletion of a key.
  -# Access to the i-th element of infix tour.
  -# Knowledge of the infix position given a key.
  -# Union and partition as a keyword or infix position.
  All these operations are performed in expected time of \f$O(\lg
  n)\f$.
  .
  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster. 

  In performance studies, the standard class set<T>
  and map<T> implanted by TreapRk type show more
  faster than implementations gnu and Boost.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys.

  @ingroup Arboles

  @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl set multiset map multimap
*/
    template <class Key, class Compare = Aleph::less<Key> >
class Treap_Rk : public Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>
{
public:

/** @brief Starts the random number generator with the value "seed" and
sets the comparison operation "cmp" */
  Treap_Rk(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>(seed, std::move(cmp))
   {
     /* Empty */
   }

  Treap_Rk(Compare && cmp = Compare()) 
    : Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>(time(NULL), std::move(cmp))
   {
     /* Empty */
   }

  Treap_Rk(unsigned int seed, Compare & cmp) 
     : Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>(seed, cmp)
   {
     /* Empty */
   }

  Treap_Rk(Compare & cmp) 
     : Gen_Treap_Rk<Treap_Rk_Node, Key, Compare>(time(NULL), cmp)
   {
     /* Empty */
   }
};

/** 
  @brief Extended binary tree of generic randomized search of type
  Treap with virtual destructor in their nodes.

  @details An extended treap is a special class of binary tree
  search in which change operations are randomized. 
  Consequently, all operations on this binary tree class are \f$O(\lg n)\f$, 
  regardless of any bias that exists about the order of insertion or
  removal of keys.

  The character "extended" provides the following benefits on the
  set of elements:
  -# Insertion, search and deletion of a key.
  -# Access to the i-th element of infix tour.
  -# Knowledge of the infix position given a key.
  -# Union and partition as a keyword or infix position.
  All these operations are performed in expected time of \f$O(\lg
  n)\f$. 

  The treap is probably the class of binary search tree than 
  preserves a better balance probabilistic performance. Empirical 
  studies suggest that this is the type of binary tree search faster. 

  In performance studies, the standard class set<T>
  and map<T> implanted by TreapRk type show more
  faster than implementations gnu and Boost.

  @param Key Type of key to save the nodes.
  @param Compare Standard of comparison between the nodes keys.

  @ingroup Arboles

  @see Treap Treap_Vtl Treap_Rk Treap_Rk_Vtl set multiset map multimap
*/
    template <class Key, class Compare = Aleph::less<Key> >
class Treap_Rk_Vtl : public Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>
{
public:

/** @brief Starts the random number generator with the value "seed" and
sets the comparison operation "cmp" */
  Treap_Rk_Vtl(unsigned int seed, Compare && cmp = Compare()) 
    : Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>(seed, std::move(cmp))
   {
     /* Empty */
   }

  Treap_Rk_Vtl(Compare && cmp = Compare()) 
    : Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>(time(NULL), std::move(cmp))
   {
     /* Empty */
   }

   Treap_Rk_Vtl(unsigned int seed, Compare & cmp) 
     : Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>(seed, cmp)
   {
     /* Empty */
   }

  Treap_Rk_Vtl(Compare & cmp) 
    : Gen_Treap_Rk<Treap_Rk_NodeVtl, Key, Compare>(time(NULL), cmp)
  {
    /* Empty */
  }
};

} /* End namespace Aleph */

# endif // TPL_TREAPRK_H
