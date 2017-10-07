
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
  @file tpl_dynSetTree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_DYNSETTREE_H
# define TPL_DYNSETTREE_H

# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_avl.H>
# include <tpl_rand_tree.H>
# include <tpl_rb_tree.H>
# include <tpl_splay_tree.H>
# include <tpl_nodePool.H>

using namespace Aleph;

namespace Aleph { 

/** 
  @brief dynamic elements set generic type T implemented
  by a class of binary trees.

  @details DynSetTree element defines a table type is Key
  instrumented with the class of binary search tree Tree <Key> and
  Compare ordered by the criterion () ().

  @ingroup Arboles 
*/
    template <
      typename Key, 
      template <typename, class> class Tree = Avl_Tree, 
      class Compare = Aleph::less<Key>
      >
class DynSetTree
{
public:

/** 
  @typedef Binary node type used by the internal binary tree search. 
*/
  typedef typename Tree<Key, Compare>::Node Node;

private:

  static const size_t dim = 13;

  typedef Tree<Key, Compare> Tree_Type;

  mutable Tree<Key, Compare> tree; 
  size_t num_nodes; 
  Node_Pool<Node> node_pool;

public:

  typedef DynSetTree Set_Type;

  typedef Key Item_Type;

  typedef Key Key_Type;

/** 
  @brief Exchanges all elements of treap this with tree treap
  in constant time (and very fast).

  @param[in] tree Treap to exchange with this
*/
  void swap(DynSetTree & dset)
  {
    tree.swap(dset.tree);
    std::swap(num_nodes, dset.num_nodes);
  }

/** 
  @brief Instance a dynamic set empty 
*/
  DynSetTree(Compare && cmp = Compare()) 
    : tree(std::move(cmp)), num_nodes(0), node_pool(dim)
  {
    /* Empty */ 
  }

  DynSetTree(Compare & cmp) 
    : tree(cmp), num_nodes(0), node_pool(dim)
  { 
    /* Empty */ 
  }

/** 
  @brief Instance a dynamic set copy of srcTree. 
*/
  DynSetTree(const DynSetTree & srcTree) 
    : tree(srcTree.tree.get_compare()), num_nodes(srcTree.num_nodes),
      node_pool(srcTree.dim)
  { 
    Node * srcRoot = srcTree.tree.getRoot(); 
    try 
      {
	tree.getRoot() = copyRec(srcRoot);
      }
    catch (...)
      {
	num_nodes = 0;
	throw;
      }
  }

  DynSetTree(const DynList<Key> & list) : DynSetTree()
  {
    list.for_each([this] (const Key & k) { insert(k); });
  }

  DynSetTree(DynSetTree && srcTree) : num_nodes(0), node_pool(dim)
  { 
    swap(srcTree);
  }

/** 
  @brief Removes all elements of the set 
*/
  void empty()
  {
    destroyRec(tree.getRoot());
    num_nodes = 0;
  }

  DynSetTree & operator = (const DynList<Key> & list)
  {
    return *this = DynSetTree(list);
  }

/** 
  @brief Assign to this the srcTree dynamic set. 
*/
  DynSetTree & operator = (const DynSetTree & srcTree)
  {
    if (this == &srcTree)
      return *this;

    Node *src_root = (Node*) const_cast<DynSetTree&>(srcTree).tree.getRoot();

    empty();

    tree.getRoot() = copyRec(src_root);
    num_nodes       = srcTree.num_nodes;

    return *this;
  }

/** 
  @brief Assign to this the srcTree dynamic set. 
*/
  DynSetTree & operator = (DynSetTree && srcTree)
   {
     swap(srcTree);
     return *this;
   }

/** 
  @brief Destroyer; all elements are released. 
*/
  virtual ~DynSetTree() 
  {
    destroyRec(tree.getRoot());
  }

private:

  Key * __insert(Node * p)
  {
    if (tree.search_or_insert(p) != p)
      {
	node_pool.deallocate(p);
	return NULL;
      }

    ++num_nodes;

    return &p->get_key();
  }

public:

/** 
  @brief Enter a key in the dynamic set.

  @details Insert the "key" key in the dynamic set.

  @param[in] key The key to insert.

  @return A pointer to the insert key if it is not within
  from the tree; NULL otherwise.
*/
  Key * insert(const Key & key)
  {
    return __insert(node_pool.allocate(key));
  }

  Key * insert(Key && key)
  {
    return __insert(node_pool.allocate(std::move(key)));
  }
  
  Key * append(const Key & key)
  {
    return __insert(node_pool.allocate(key));
  }

  Key * append(Key && key)
  {
    return __insert(node_pool.allocate(std::move(key)));
  }

private:

  Key * __search_or_insert(Node * p)
  {
    Node * q = tree.search_or_insert(p);
    if (q != p)
      node_pool.deallocate(p);
    else
      ++num_nodes;

    return &q->get_key();
  }


public:

/** 
  @brief Look for the <code>"key"</code> key in the binary tree search or
  insert if not found.

  @details <code>search_or_insert(key)</code> search the binary tree a
  node whose key is <code>KEY(p)</code>.
  If the key is found, then it returns a pointer to it; otherwise
  inserting <code>key</code> in the binary search tree this.

	@param[in] key The key to search or insert.
	
  @return A pointer to the key in the tree
*/
  Key * search_or_insert(const Key & key)
  {
    return __search_or_insert(node_pool.allocate(key));
  }

  Key * search_or_insert(Key && key)
  {
    return __search_or_insert(node_pool.allocate(std::move(key)));
  }

private:

  Key * __insert_dup(Node * q)
  {
    Node * p = tree.insert_dup(q);
    ++num_nodes;
    return &p->get_key();
  }

public:

  Key * insert_dup(const Key & key)
  {
    return __insert_dup(node_pool.allocate(key));
  }

  Key * insert_dup(Key && key)
  {
    return __insert_dup(node_pool.allocate(std::move(key)));
  }

/** 
  @brief Pseudo synonymous of insert; does not return any value. 
*/
  Key * put(const Key & key)
  {
    return insert(key);
  }

  Key * put(Key && key)
  {
    return insert(std::move(key));
  }

/** 
  @brief Deletes a key of dynamic set.

  @details remove(key) Search the "key" key of the set and removed it.

  @param[in] key the key to delete

  @return Number of items that have the set.
*/
  size_t remove(const Key & key)
  {
    Node * p = static_cast<Node*>(tree.remove(key));

    if (p == NULL)
      return num_nodes;

    node_pool.deallocate(p);

    return --num_nodes;
  }

/** 
  @brief Deletes a key of dynamic set.

  @details remove(key) Search the "key" key of the set and removed it.

  @param[in] key the key to delete

  @return Number of items that have the set.
*/
  Key remove_pos(const size_t i)
  {
    Node * p = static_cast<Node*>(tree.remove_pos(i));
    const Key ret_val = KEY(p);

    node_pool.deallocate(p);

    --num_nodes;

    return ret_val;
  }

/** 
  @brief Returns true if key belongs to the dynamic set. 
*/
  bool exist(const Key & key) const
  {
    return const_cast<DynSetTree&>(*this).search(key) != NULL;
  }

  bool has(const Key & key) const
  {
    return exist(key);
  }

  bool contains(const Key & key) const
  {
    return exist(key);
  }

/** 
  @brief Returns a modifiable reference to an element in the set.

  @details find (key) searches the specified key in the set and returns a
  coded reference to the value contained in the set.

  @param [in] key The key to look for.

  @return A reference to the key modifiable key contained within
  assembly.

  @throw domain_error If key is not within the set.

  Notenote carefully to alter the search order,
  because the reference is modified.
*/
  Key & find(const Key & key) throw(std::exception, std::domain_error)
  {
    Node * node = static_cast<Node*>(tree.search(key));

    if (node == NULL)
      throw std::domain_error("key not found");

    return node->get_key();
  }

/** 
  @brief Returns the (ordered) infix position of the "key" key or whatever
  his position would belong to the tree.

	@details find_position(key) search in the extended tree the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key.

	@param[in] key Clave a buscar y a determinar posición infija.
	
  @return The infix position of the key within the set key
  whether it is ordered or otherwise, the position
  where he would meet belong to the tree.  
	
	@warning Only works if tree handlers ranges.
*/
  std::pair<int, Key*> find_position(const Key & key) const
  { 
    if (num_nodes == 0)
      return std::pair<int, Key*> (0, NULL);

    std::pair<int, Node*> p = tree.find_position(key); 

    return std::pair<int, Key*> (p.first, &p.second->get_key()); 
  }

/** 
  @brief Searches for an item in the set.

  @details search (key) searches the specified key in the set. 
  If the element is in the set, then it returns a
  pointer to the value contained in the set; otherwise
  retrona NULL.

  @param [in] key The key to look for.

  @return A pointer to the element in the set if it is
  located therein; L NULL otherwise.

  @note Note carefully to alter the search order,
  because through the key modifiable pointer.
*/
  Key * search(const Key & key) const
    throw(std::exception, std::domain_error)
  {
    Node * node = static_cast<Node*>(tree.search(key));

    if (node == NULL)
      return NULL;

    return &(node->get_key());
  }

/** 
  @brief Returns the key contained in the lower assembly according
  given the comparison test.
*/
  const Key & min() const
  {
    if (num_nodes == 0)
      throw std::domain_error("set is empty");

    return find_min(tree.getRoot())->get_key();
  }

/** 
  @overload min() 
*/
  const Key & get_first() { return min(); }

/** 
  @brief Returns the largest key contained in the assembly according to the criterion
  since comparison. 
*/
  const Key & max() const
  {
    if (num_nodes == 0)
      throw std::domain_error("set is empty");

    return find_max(tree.getRoot())->get_key();
  }

/** 
  @overload max 
*/
  const Key & get_last() { return max(); }

/** 
  @brief max synonymous
*/
  const Key & get()
  {
    return max();
  }

/** 
  @brief Returns the cardinality of the set. 
*/
  const size_t & size() const { return num_nodes; }

/** 
  @brief Returns true if the set is empty. 
*/
  bool is_empty() const { return num_nodes == 0; }

/** 
  @brief Calculates and returns the internal path length of the tree
  binary search. 
*/
  size_t internal_path_length() const 
  {
    return Aleph::internal_path_length(tree.getRoot()); 
  }

  Node * get_root_node() const { return tree.getRoot(); }

  const Key & get_root() const
  {
    if (num_nodes == 0)
      throw std::domain_error("Tree is empty");
    return KEY(tree.getRoot());
  }

/** 
  @brief Returns any element of the set 
*/
  const Key & get_item() const { return get_root(); }

/** 
  @brief Calculates and returns the height of the binary search tree. 
*/
  size_t height() const { return computeHeightRec(tree.getRoot()); }

/** 
  @brief Makes a prefix on all nodes in the tree and travel
  operation invokes <code> visitFct </ code> at each visit
*/
      template <class Op>
  void for_each_in_preorder(void (*visitFct)(Node*, int, int))
  {
    Node * root = static_cast<Node*>(tree.getRoot());
    preOrderRec(root, visitFct); 
  }

/** 
  @brief Returns the (ordered) infix position of the "key" key.

  @details position(key) search in the extended treap the "key" key (which
  takes time \f$O(\lg n)\f$) and returns the infix position of the node
  containing the key.

  @param[in] key The key to seek and to determine infix position.

  @return The infix position of the specified key in the set
  ordered if the key is found; Otherwise -1

  @warning Only works if tree handlers ranges.
*/
  long position(const Key & key) const
  {
    std::pair<long, Node*> p = tree.position(key);
    return p.first; 
  }

/** 
  @brief Returns the ith node in infix position.

  @param [in] i desired position.
  
  @return The reference to the i-th key inserted in the set.
*/
  Key & select(const size_t & i) 
  {
    return tree.select(i)->get_key();
  }

  const Key & select(const size_t & i) const
  {
    return tree.select(i)->get_key();
  }

  Key & operator () (const size_t & i)
  {
    return select(i);
  }

  Key & access (const size_t & i)
  {
    return select(i);
  }

  bool verify() 
  {
    return tree.verify() and check_binary_search_tree(tree.getRoot()); 
  }

private:

    template <class Key_Op>
struct Node_Op
{
  Key_Op & key_op;

  Node_Op(Key_Op & __key_op) : key_op(__key_op) { /* Empty */ }

  Node_Op(Key_Op && __key_op) : key_op(__key_op)
  {
    /* Empty */ 
  }

  void operator () (Node * root) 
  { 
    assert(root != NULL);
    key_op(KEY(root));
  }
};

public:

/** 
  @brief Performs a prefix on all the keys and set path
  invokes the operation Op.

  @details <code> Op (p) </ code> has the following structure:

	@code
	     struct Key_Op
	     {
	       // State attributes that you wish to keep

	       Key_Op(...) // Optional builder if necessary to initialize
	       {
	         // inicialization
	       }

	       void operator () (const Key & key)
	       {
	         // Operation on key "key"
	       }
	     };
	@endcode

	@param[in] key_op Operation to be performed on each key 
	
  @see For_Each_Preorder
*/
      template <class Key_Op> 
  void for_each_preorder(Key_Op & key_op) 
  {
    Node * root = (Node *) tree.getRoot();

    Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

    For_Each_Preorder<Node, Node_Op<Key_Op>> () (root, node_op);
  }

/** 
  @overload for_each_preorder 
*/
      template <class Key_Op> 
  void for_each_preorder(Key_Op && key_op = Key_Op()) 
  {
    for_each_preorder<Key_Op>(key_op);
  }

/** 
  @brief Performs an infix travel on all keys set and
  invokes the operation Op.

  @details <code> Op (p) </ code> has the following structure:

  @code
       struct Key_Op
       {
         // State attributes that you wish to keep

         Key_Op(...) // Optional builder if necessary to initialize
         {
           // inicialization
         }

         void operator () (const Key & key)
         {
           // Operation on key "key"
         }
       };
  @endcode

  @param[in] key_op operation to be performed on each key

  @see For_Each_In_Order
*/
      template <class Key_Op> 
  void for_each_inorder(Key_Op & key_op) 
  {
    Node * root = (Node *) tree.getRoot();

    Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

    For_Each_In_Order<Node, Node_Op<Key_Op>> () (root, node_op);
  }

/** 
  @overload for_each_inorder 
*/
      template <class Key_Op> 
  void for_each_inorder(Key_Op && key_op = Key_Op()) 
  {
    for_each_inorder<Key_Op>(key_op);
  }

/** 
  @brief Efectúa un recorrido sufijo sobre todas las claves del conjunto e
  invoca la operacion Op.

  @details <code>Op(p)</code> tiene la siguiente estructura: 

  @code
       struct Key_Op
       {
         // State attributes that you wish to keep

         Key_Op(...) // Optional builder if necessary to initialize
         {
           // inicialization
         }

         void operator () (const Key & key)
         {
           // Operation on key "key"
         }
       };
  @endcode

  @param[in] key_op Operation to be performed on each key

  @see For_Each_Postorder
*/
      template <class Key_Op> 
  void for_each_postorder(Key_Op & key_op) 
  {
    Node * root = (Node *) tree.getRoot();

    Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

    For_Each_Postorder<Node, Node_Op <Key_Op>> () (root, node_op);
  }

/** 
  @overload for_each_postorder 
*/
      template <class Key_Op> 
  void for_each_postorder(Key_Op && key_op = Key_Op()) 
  {
    for_each_postorder<Key_Op>(key_op);
  }

/** 
  @brief Union of two binary trees search.

  @details join(t,dup) build a binary tree search corresponding 
  to the union of this with t. Duplicate keys is inserted in dup.

  @param[in] t Binary tree search that wants to join this.
  @param[out] dup Binary tree search with the key duplicate of t.

  @note After the call tree becomes empty and esta t becomes
  the union of the two trees; however the values of t1 and t2 are unchanged.
  note After the call the tree t empty and esta Becomes Becomes
  Both the union of the trees; however the values of t1 and t2 are unchanged.
  
  @return this
  
  @ingroup Arboles
*/
  DynSetTree & join(DynSetTree & t, DynSetTree & dup)
  {
    BinTree_Operation <Node, Compare> (tree.get_compare()).
      join(tree.getRoot(), t.tree.getRoot(), dup.tree.getRoot());

    t.num_nodes = 0;
    t.tree.getRoot() = Node::NullPtr;

    dup.num_nodes = COUNT(dup.tree.getRoot());
    num_nodes = COUNT(tree.getRoot());

    return *this;
  }

/** 
  @overload joint 
*/
  DynSetTree & join(DynSetTree & t, DynSetTree && dup = DynSetTree())
  {
    return join(t, dup);
  }

/** 
  @brief Union of two binary trees search.

  @details join_dup(t) builds a binary tree search corresponding 
  to the union of this with t in which they may have duplicate keys.

  @param[in] t Binary tree search that wants to join this.
  
  @note After the call the t tree becomes empty and this becomes
  the union of the both trees;
  
  @return this
  
  @ingroup Arboles
*/
  DynSetTree & join_dup(DynSetTree & t)
  {
    tree.join_dup(t.tree);
    t.num_nodes = 0;
    t.tree.getRoot() = Node::NullPtr;
    num_nodes = COUNT(tree.getRoot());
    return *this;
  }

/** 
  @brief Partition the binary search tree as a key.

  @details split_key (key, l, r) partitions, according to the key code, the
  binary search tree into two trees esta r l. After
  esta operation tree becomes empty, l contains
  all keys and key r less than the majors.

  @param[in] key Partition key.
  @param[out] l Tree with smaller keys that key.
  @param[out] r Tree with the biggest keys that key.

  @return true if key is not within the binary tree; in which
  If the partition was made successfully. Otherwise, if
  key is inside the tree, the tree is not partitioned and the
  result is false.
*/
  bool split_key(const Key & key, DynSetTree & l, DynSetTree & r)
  {
    if (not split_key_rec_xt<Node, Compare>(tree.getRoot(), key, 
					    l.tree.getRoot(), r.tree.getRoot()))
      return false;

    tree.getRoot() = Node::NullPtr;
    num_nodes = 0;
    l.num_nodes = COUNT(l.tree.getRoot());
    r.num_nodes = COUNT(r.tree.getRoot());

    return true;
  }

/** 
  @brief Partition the binary search tree as an infix position.

  @details split_pos (pos, l, r) partitioned binary search tree esta
  Two trees l and r. After
  esta operation tree becomes empty, containing l pos
  first key and the remaining r.

  @param [in] position after partition
  @param [out] l tree with keys between [0, pos]
  @param [out] r tree with the keys in the range (pos, N)
  where N is the number of keys
*/
  void split_pos(const size_t pos, DynSetTree & l, DynSetTree & r)
  {
    split_pos_rec(tree.getRoot(), pos, l.tree.getRoot(), r.tree.getRoot());

    tree.getRoot() = Node::NullPtr;
    num_nodes = 0;
    l.num_nodes = COUNT(l.tree.getRoot());
    r.num_nodes = COUNT(r.tree.getRoot());
  }

/** 
  @brief Partition the binary search tree as a key
  may be present on the tree.

  @details split_dup (key, l, r) partitions, according to the key code, the
  binary search tree into two trees esta r l. After
  esta operation tree becomes empty, l contains
  all keys under key yr greater or equal.

  @param [in] key Partition key.
  @param [out] l Tree with smaller keys That key.
  @param [out] r Tree with key greater than or equal to key.
*/
  void split_key_dup(const Key & key, DynSetTree & l, DynSetTree & r)
  {
    split_key_dup_rec_xt<Node,Compare>(tree.getRoot(), key, 
				       l.tree.getRoot(), r.tree.getRoot());

    tree.getRoot() = Node::NullPtr;
    num_nodes = 0;
    l.num_nodes = COUNT(l.tree.getRoot());
    r.num_nodes = COUNT(r.tree.getRoot());
  }

  class Iterator
  {
  protected:
    
    mutable DynSetTree * tree_ptr; 
    mutable Node * curr;
    mutable int    curr_pos;

    static const int Pos_Not_Current     = -1;
    static const int Pos_Empty_Container = -2;
    static const int Pos_Not_Updated     = -3;

  private:

    bool is_container_empty() const
    {
      return tree_ptr->size() == 0;
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

      curr_pos = tree_ptr->position(curr->get_key());
    }

    void update_curr() const
    {
      assert(curr_pos != Pos_Not_Updated);

      if (curr_pos == Pos_Empty_Container or curr_pos == Pos_Not_Current or
	  curr_pos == tree_ptr->size())
	return;
      
      curr = Node::key_to_node(tree_ptr->select(curr_pos));
    }

  public:

/** 
  @brief Empty constructor; It makes no sense if you do not assign a treap. 
*/
    Iterator() : tree_ptr(NULL), curr(NULL), curr_pos(Pos_Not_Current)
    {
      /* Empty */ 
    }

/** 
  @brief Instancia un iterador a partir del menor nodo del treap __tree. 
*/
    Iterator(const DynSetTree & tree) 
      : tree_ptr(&const_cast<DynSetTree&>(tree)), curr(NULL)
    {
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

/** 
  @brief Place the iterator position pos
*/
    void set_pos(size_t pos)
    {
      if (tree_ptr == NULL)
	std::domain_error("There is not tree associated to iterator");

      curr_pos = pos;
      curr = Node::key_to_node(tree_ptr->select(curr_pos));
    }

/** 
  @brief Put the key in the key iterator
*/
    void set_key(const Key & key)
    {
      if (tree_ptr == NULL)
	std::domain_error("There is not tree associated to iterator");

      std::pair<int, Node*> p = tree_ptr->find_position(key);
      curr = Node::key_to_node(KEY(p));
      curr_pos = p.first;
    }

/** 
  @brief An iterator instance from the state of the iterator "itor". 
*/
    Iterator (const Iterator & itor)
      : tree_ptr(itor.tree_ptr), curr(itor.curr), curr_pos(itor.curr_pos)
    {
      /* Empty */
    }

/**   
  @brief Assigns the iterator "this" to iterator "itor".
*/
    Iterator & operator = (const Iterator & itor)
    {
      if (this == &itor)
	return *this;

      tree_ptr = itor.tree_ptr;
      curr = itor.curr;
      curr_pos = itor.curr_pos;

      return *this;
    }

/** 
  @brief Resets the iterator to the last node (lower) of treap. 
*/
    void reset_first()
    {
      curr = NULL;
      curr_pos = is_container_empty() ? Pos_Empty_Container : 0;
    }

/** 
  @brief Resets the iterator to the last node (greater) of treap. 
*/
    void reset_last()
    {
      curr = NULL;
      curr_pos = 
	is_container_empty() ? Pos_Empty_Container : tree_ptr->size() - 1;
    }

/** 
  @brief Restart the current position of the iterator to the item with value
  "key" key.

  @param[in] key The key to place as the current position.

  @throw domain_error If the key is not contained in the tree.
*/
    void reset_to_key (const Key & key)
    {
      std::pair<long, Key*> p = tree_ptr->position(key);
      curr_pos = p.first;
    }

/** 
  @brief Restart the current position of the iterator to "node" node.

  @param[in] node Node to place as the current position.

  @note No check is performed about the node
  inside or outside the "treap".
*/
    void reset_to_node (Node * node)
    {
      curr = node;
      curr_pos = -2;
    }

/** 
  @brief Place the current position of the iterator in the "post" position. 
*/
    void reset_to_pos (size_t pos)
    {
      curr = NULL;
      curr_pos = pos;
    }

/** 
  @brief Returns the current node. 
*/
    Key & get_current() const
    {
      if (not curr_updated())
	update_curr();

      return KEY(curr); 
    }

/** 
  @brief Returns the current node. 
*/
    Key & get_curr() const
    {
      return get_current();
    }

/** 
  @brief Returns the infix position of the current node on which
  the iterator is positioned.

  @return The infix position of the current node on which
  the iterator is positioned.

  @throw underflow_error If the iterator is overwhelmed by below. 
  @throw overflow_error  If the iterator is overflowing above. 
*/
    size_t get_current_position() const
      throw (std::exception, std::underflow_error, std::overflow_error)
    {
      if (not pos_updated())
	update_pos();

      if (curr_pos < -1 )
	throw std::range_error ("DynSetTree::Iterator has not current");
	
      if (curr_pos > COUNT (tree_ptr->getRoot() ) )
	throw std::range_error ("DynSetTree::iterator has not current");

      return curr_pos;
    }

/** 
  @brief Returns true if the iterator is positioned on a current element. 
*/
    bool has_current() const
    {
      if (not pos_updated())
	update_pos();

      return curr_pos >= 0 and curr_pos < tree_ptr->size();
    }

/** 
  @brief Returns true if the iterator is positioned on a current element. 
*/
    bool has_curr() const
    {
      return has_current();
    }

/** 
  @brief Advances the iterator one position backward. 
*/
    void prev() throw (std::exception, std::underflow_error)
    {
      if (not has_current() )
	throw std::underflow_error ("DynSetTree::Iterator has not current");

      --curr_pos;
      curr = NULL;
    }

/** 
  @brief Advances the iterator one position forward. 
*/
    void next() throw (std::exception, std::overflow_error)
    {
      if (not has_current() )
	throw std::underflow_error ("DynSetTree::Iterator has not current");

      ++curr_pos;
      curr = NULL;
    }

/** 
  @brief Deletes the current node on which the iterator is positioned;
  it is positioned in the following deleted. 
*/
    Key del()
    {
      if (not has_current() )
	throw std::underflow_error ("DynSetTree::Iterator has not current");

      if (not curr_updated())
	update_curr();

      Key ret_val = curr->get_key();
      tree_ptr->remove(ret_val);

      curr = NULL;

      return ret_val;
    }

/** 
  @brief Returns true if "*this" is on the same node as "itor". 
*/
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
  
/** 
  @brief Returns true if "*this" is not the same node as "itor". 
*/
    bool operator != (const Iterator & itor) const
    {
      return not (*this == itor);
    }
  }; /* End class Iterator */

/** 
  @brief Traverse all the set of pairs and for eah pair executes operation.

  @details Operation must have the signature

  @code
  bool operation(T & item)
  @endcode

  If @code operation() @endcode returns false then the traversal is
  aborted; otherwise the the routine advance and so on

  @param[in] operation Operation

  @return true if all items are traversed; false otherwise
*/
  template <class Operation>
  bool traverse(Operation & op)
  {
    return Aleph::traverse(tree.getRoot(), [&op] (Node * p)
			   {
			     return op(p->get_key());
			   });
  }

  template <class Operation>
  bool traverse(Operation && op = Operation())
  {
    return traverse<Operation>(op);
  }

  template <class Operation>
  bool traverse(Operation & op) const
  {
    return Aleph::traverse(tree.getRoot(), [&op] (Node * p)
			   {
			     return op(p->get_key());
			   });
  }

  template <class Operation>
  bool traverse(Operation && op = Operation()) const
  {
    return traverse<Operation>(op);
  }

  Functional_Methods(Key);
  Generic_Keys(Key);
  Equal_To_Method(DynSetTree);
};


# define SETTREE_ITOR(Name, Key, Cmp)					\
  class Iterator : public DynSetTree<Key, Name, Cmp>::Iterator		\
  {									\
  public:								\
    Iterator() : DynSetTree<Key, Name, Cmp>::Iterator()			\
    { /* Empty */ }							\
    									\
    Iterator(DynSetTree<Key, Name, Cmp> & tree)				\
      : DynSetTree<Key, Name, Cmp>::Iterator(tree)			\
    { /* Empty */ }							\
  };


/** 
  @brief Dinamic set implemented by binary tree search
  of BinTree<Key> type.

  @see DynSetTree DynMapTree BinTree<Key>
      	
  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetBinTree : public DynSetTree<Key, BinTree, Compare>
    { /* Empty */ };


/** 
  @brief Dinamic set implemented by AVL binary tree search
  of Avl_Tree<Key> type.

  @see DynSetTree DynMapTree Avl_Tree<Key>
      
  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetAvlTree : public DynSetTree<Key, Avl_Tree, Compare>
    { /* Empty */ };


/** 
  @brief Dinamic set implemented by splay binary tree search 
	of Splay_Tree<Key> type.

	@see DynSetTree DynMapTree Splay_Tree<Key>
	
  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetSplayTree : public DynSetTree<Key, Splay_Tree, Compare>
    { /* Empty */ };


/** 
  @brief Dinamic set implemented by random binary tree 
  search of Rand_Tree<Key> type.

  @see DynSetTree DynMapTree Rand_Tree<Key>

  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetRandTree : public DynSetTree<Key, Rand_Tree, Compare>
{ 
public:

  class Iterator : public DynSetTree<Key, Rand_Tree, Compare>::Iterator 
  { 
  public: 
    Iterator() : DynSetTree<Key, Rand_Tree, Compare>::Iterator() 
    { /* Empty */ } 

    Iterator(DynSetTree<Key, Rand_Tree, Compare> & tree) 
      : DynSetTree<Key, Rand_Tree, Compare>::Iterator(tree) 
    { /* Empty */ } 
  }; 
};


/** 
  @brief Dinamic set implemented by treaps random binary 
  tree search of Treap<Key> type.


	@see DynSetTree DynMapTree Treap<Key>

	@ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetTreap : public DynSetTree<Key, Treap, Compare>
    { /* Empty */ };

/** 
  @brief Dinamic set implemented by treaps random binary 
  tree search of Treap<Key> type.

  @see DynSetTree DynMapTree Treap<Key>

  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetTreapRk : public DynSetTree<Key, Treap_Rk, Compare>
{
public:
  SETTREE_ITOR(Treap_Rk, Key, Compare);
};


/** 
  @brief Dinamic set implemented by treaps random binary 
  tree search of Rb_Tree<Key> type.

  @see DynSetTree DynMapTree Rb_Tree<Key>

  @ingroup Arboles
*/
    template  <typename Key, class Compare = Aleph::less<Key>>
class DynSetRbTree : public DynSetTree<Key, Rb_Tree, Compare>
    { /* Empty */ }; 


} /* End namespace Aleph */

# endif /* TPL_DYNSETTREE_H */
