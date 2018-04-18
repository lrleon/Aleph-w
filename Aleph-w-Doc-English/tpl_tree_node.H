 
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
     notice, "this" list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, "this" list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of "this" software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from "this" software
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

  I request users of "this" software to return to 

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
  @file tpl_tree_node.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_TREE_H
# define TPL_TREE_H

# include <stdexcept>
# include <dlink.H>
# include <ahFunction.H>
# include <htlist.H>
# include <tpl_binNode.H>

# define ISROOT(p)      ((p)->is_root())
# define ISLEAF(p)      ((p)->is_leaf()) 
# define ISLEFTMOST(p)  ((p)->is_leftmost())
# define ISRIGHTMOST(p) ((p)->is_rightmost())

# define SIBLING_LIST(p) ((p)->get_sibling_list())
# define CHILD_LIST(p)   ((p)->get_child_list())
# define LCHILD(p)       ((p)->get_left_child())
# define RSIBLING(p)     ((p)->get_right_sibling())
# define IS_UNIQUE_SIBLING(p) (RSIBLING(p) == (p))

namespace Aleph
{

/** 
  @brief Generic m-rios trees.

  @details The class Tree_Node<Key> defines general trees of any
  order represented by linked lists.

  @param Key Type of data contained in each node of the tree.

  @ingroup Arboles 
*/
    template <class T> 
class Tree_Node 
{
  T data;
  Dlink child;
  Dlink sibling;
  struct Flags
  {
    unsigned int is_root          : 1;
    unsigned int is_leaf          : 1;
    unsigned int is_leftmost      : 1;
    unsigned int is_rightmost     : 1;
    Flags() : is_root(1), is_leaf(1), is_leftmost(1), is_rightmost(1) {}
  };

  Flags flags;
  LINKNAME_TO_TYPE(Tree_Node, child);
  LINKNAME_TO_TYPE(Tree_Node, sibling);

  Tree_Node * upper_link()
  {
    return child_to_Tree_Node(child.get_prev()); 
  }

  Tree_Node * lower_link() 
  {
    return child_to_Tree_Node(child.get_next()); 
  }

  Tree_Node * left_link() 
  {
    return sibling_to_Tree_Node(sibling.get_prev()); 
  }

  Tree_Node * right_link() 
  {
    return sibling_to_Tree_Node(sibling.get_next()); 
  }

public:

/** @brief Returns modifiable reference to the content of the node. */
  T & get_key() { return get_data(); }

/** @brief Returns modifiable reference to the content of the node. */
  T & get_data() { return data; }

/** @brief Generic data type that contains the node. */
  typedef T key_type;

  Dlink * get_child_list() { return &child; }

  Dlink * get_sibling_list() { return &sibling; }

/** @brief Returns true if "this" is the root of the general tree. */
  bool is_root() const { return flags.is_root; }

/** @brief Returns true if "this" is a leaf node. */
  bool is_leaf() const { return flags.is_leaf; }

/** @brief Returns true if "this" is the node's left of his brothers. */
  bool is_leftmost() const { return flags.is_leftmost; }

/** @brief Returns true if "this" is the node's rightmost of his brothers. */
  bool is_rightmost() const { return flags.is_rightmost; }

  void set_is_root(bool value) { flags.is_root = value; }

  void set_is_leaf(bool value) { flags.is_leaf = value; }

  void set_is_leftmost(bool value) { flags.is_leftmost = value; }

  void set_is_rightmost(bool value) { flags.is_rightmost = value; }

/** @brief Empty constructor (undefined key) */
  Tree_Node() { /* Empty */ }

/** @brief Constructor with __data data value */
  Tree_Node(const T & __data) : data(__data) { /* Empty */ }

/** @brief Returns left brother of "this" */
  Tree_Node * get_left_sibling() 
  {
    if (is_leftmost()) 
      return NULL;

    return left_link();
  }

/** @brief Returns right brother of "this" */
  Tree_Node * get_right_sibling()
  {
    if (is_rightmost()) 
      return NULL;  

    return right_link();
  }

/** @brief Returns the leftmost child of "this" */
  Tree_Node * get_left_child() 
  {
    if (is_leaf()) 
      return NULL;

    return lower_link();
  }

/** @brief Returns the rightmost child to "this" */
  Tree_Node * get_right_child()
  {
    if (is_leaf())
      return NULL;

    Tree_Node * left_child = lower_link();

    assert(ISLEFTMOST(left_child));

    return left_child->left_link();
  }

/**
  @brief Returns the i-th child of "this".

  @details Returns the i-th child of "this".
     
  @param[in] i Son ordinal that you wish to access.

  @return A pointer to the i-th child of "this"; NULL if not present.
*/
  Tree_Node * get_child(const int & i) 
  {
    Tree_Node * c = get_left_child();

    for (int j = 1; c != NULL and j < i; ++j)
      c = c->get_right_sibling();

    return c;
  }

/** @brief Returns the parent of "this" */
  Tree_Node * get_parent()  
  {
    if (is_root()) 
      return NULL; 

    Tree_Node * p = this;
    /** Scroll down to the node on the left */
    while (not ISLEFTMOST(p)) 
      p = p->left_link();

    assert(not ISROOT(p));
    assert(not CHILD_LIST(p)->is_empty()); 

    return p->upper_link();
  }

/**
  @brief Insert "p" at right brother of "this".

  @details Insert "p" at right brother of "this".

  @param[in] p Node to insert at right brother.
*/
  void insert_right_sibling(Tree_Node * p)
  {
    if (p == NULL) 
      return;

    assert(CHILD_LIST(p)->is_empty());
    assert(SIBLING_LIST(p)->is_empty());
    assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and p->is_leaf());

    p->set_is_root(false);
    p->set_is_leftmost(false);

    Tree_Node * old_next_node = get_right_sibling(); 
    if (old_next_node != NULL)
      {
        assert(not this->is_rightmost());
	p->set_is_rightmost(false);
      }
    else
      {
	assert(this->is_rightmost());
	p->set_is_rightmost(true);
      }

    this->set_is_rightmost(false);
    this->sibling.insert(SIBLING_LIST(p));
  }
/**
  @brief Insert "p" at left brother of "this".

  @details Insert "p" at left brother of "this".

  @param[in] p Node to insert at left brother.
*/
  void insert_left_sibling(Tree_Node * p)
  {
    if (p == NULL) 
      return;

    if (this->is_root())
      throw std::domain_error("Cannot insert sibling of a root");

    assert(CHILD_LIST(p)->is_empty());
    assert(SIBLING_LIST(p)->is_empty());
    assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and p->is_leaf());

    p->set_is_root(false);
    p->set_is_rightmost(false);

    Tree_Node * old_prev_node = this->get_left_sibling();
    if (old_prev_node != NULL) 
      {
        assert(not this->is_leftmost());
	p->set_is_leftmost(false);
      }
    else /* "this" es más a la izq ==> p debe a ser primogénito */ 
      { 
	assert(this->is_leftmost());

        Tree_Node * parent = this->get_parent();

    /* Busca la raíz del árbol. Para ello buscamnos la hoja de "this" */
	Tree_Node * leaf = this;
	while (not leaf->is_leaf())
	  {
	    leaf = leaf->get_left_child();
	    assert(leaf != NULL);
	  }
	
	Tree_Node * root = leaf->lower_link();
	assert(root != NULL);

	Dlink tree = CHILD_LIST(root)->cut_list(CHILD_LIST(this));
	tree.del();
	CHILD_LIST(parent)->insert(CHILD_LIST(p));
	p->set_is_leftmost(true);

	assert(p->get_parent() == parent);
      }

    this->set_is_leftmost(false);  
    this->sibling.append(SIBLING_LIST(p));
  }

/**
  @brief Insert "p" as the leftmost child of "this".

  @details Insert "p" as the leftmost child of "this".

  @param[in] "p" Node to insert.
*/
  void insert_leftmost_child(Tree_Node * p)
  {
    if (p == NULL) 
      return;

    assert(CHILD_LIST(p)->is_empty());
    assert(SIBLING_LIST(p)->is_empty());
    assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and p->is_leaf());

    p->set_is_root(false);
    if (this->is_leaf())
      {
        this->set_is_leaf(false);
        CHILD_LIST(this)->insert(CHILD_LIST(p));
      } 
    else
      {
	Tree_Node * old_left_child = this->lower_link();
	Tree_Node * leaf = old_left_child;
	while (not leaf->is_leaf())
	  leaf = leaf->get_left_child();

	Tree_Node * root = leaf->lower_link();
	Dlink subtree = CHILD_LIST(root)->cut_list(CHILD_LIST(old_left_child));
	subtree.del();
	CHILD_LIST(this)->insert(CHILD_LIST(p));
	SIBLING_LIST(old_left_child)->append(SIBLING_LIST(p));
	old_left_child->set_is_leftmost(false);
	p->set_is_rightmost(false);
	assert(p->get_right_sibling() == old_left_child);
	assert(old_left_child->get_left_sibling() == p);
      }
    assert(p->is_leftmost());
  }

/**
  @brief Insert "p" as the rightmost child of "this".

  @details Insert "p" as the rightmost child of "this".

  @param[in] "p" Node to insert.
*/
  void insert_rightmost_child(Tree_Node * p)
  {
    if (p == NULL) 
      return;

    assert(CHILD_LIST(p)->is_empty());
    assert(SIBLING_LIST(p)->is_empty());
    assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and p->is_leaf());

    p->set_is_root(false);

    if (this->is_leaf())
      {
        this->set_is_leaf(false);
        CHILD_LIST(this)->insert(CHILD_LIST(p));
      } 
    else
      {
        Tree_Node * old_right_child_node = this->lower_link()->left_link();
        old_right_child_node->set_is_rightmost(false);
        p->set_is_leftmost(false);
        SIBLING_LIST(old_right_child_node)->insert(SIBLING_LIST(p));
      }
  }

/**
  @brief Deputy tree "tree" to the tree structure "this".

  @details Deputy tree "tree" to the tree structure "this".

  @param[in] tree Tree to add to the tree structure.

  @throw domain_error If "tree" is not root.

  @note "tree" must be root of a tree.
*/
  void insert_tree_to_right(Tree_Node * tree)
  {
    if (tree == NULL) 
      return;

    if (not this->is_root()) 
      throw std::domain_error("\"this\" is not root");

    tree->set_is_leftmost(false);
    Tree_Node * old_next_tree = this->get_right_tree();
    if (old_next_tree != NULL) 
      {
        assert(not this->is_rightmost());
        tree->set_is_rightmost(false);
      }

    this->set_is_rightmost(false);  
    SIBLING_LIST(this)->insert(SIBLING_LIST(tree));
  }

/** @brief Return the tree to the left of "this"
 */
  Tree_Node * get_left_tree() 
  {
    if (is_leftmost()) 
      return NULL;
    assert(not is_leftmost()); 
    return left_link();
  }

/** @brief Return the tree to the right of "this" */
  Tree_Node * get_right_tree() 
  {
    if (is_rightmost()) 
      return NULL;

    assert(not is_rightmost());
    return right_link();
  }

/** @brief Returns the rightmost tree down tree "this".
Shoot range_error exception if "this" is not the tree more
left the entire tree structure.
 */
  Tree_Node * get_last_tree() 
  {
    if (not is_leftmost())
      throw std::range_error("\"this\" is not the leftmost tree in the forest");

    return left_link();
  }

/** @brief Visit every child of "this" and executes the "operation" operation
  on the child node. */
  template <typename Operation>
  void for_each_child(Operation & op)
  {
    for (Tree_Node * child = get_left_child(); child != NULL; 
	 child = child->get_right_sibling())
      op(child);
  }

  template <typename Operation>
  void for_each_child(Operation && op = Operation())
  {
    for_each_child<Operation>(op);
  }

  template <typename Operation>
  void for_each_child(Operation & op) const
  {
    return const_cast<Tree_Node*>(this)->for_each_child(op);
  }

  template <typename Operation>
  void for_each_child(Operation && op = Operation()) const
  {
    return const_cast<Tree_Node*>(this)->for_each_child(op);
  }

/** @brief Returns a list of child nodes of "this" */
  template <template <typename> class Container = DynList>
  Container<Tree_Node*> children_nodes() const
  {
    Container<Tree_Node*> ret_val;
    this->for_each_child([&ret_val] (Tree_Node * p) { ret_val.append(p); });
    return ret_val;
  }

/** @brief Returns a list of the contents of the children of "this" */
  template <template <typename> class Container = DynList>
  Container<T> children() const
  {
    Container<T> ret_val;
    this->for_each_child([&ret_val] (Tree_Node * p) 
			 {
			   ret_val.append(p->get_key());
			 });
    return ret_val;
  }
};

    template <class Node> static inline
void __tree_preorder_traversal(Node * root, const int & level, 
                               const int & child_index,
                               void (*visitFct)(Node *, int, int))
{
  (*visitFct)(root, level, child_index);
  Node * child = root->get_left_child(); 
  for (int i = 0; child != NULL; ++i, child = child->get_right_sibling())
    __tree_preorder_traversal(child, level + 1, i, visitFct);
}

/** 
  @brief Walking on prefix a tree.

  @details tree_preorder_traversal((root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level of p in the tree.
  -# child_index: Index of p within their brothers.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.

  @see forest_preorder_traversal() tree_postorder_traversal()
  @see forest_postorder_traversal()

  @throw domain_error If root is not a "root" node of a tree.

  @ingroup Arboles
*/
    template <class Node> inline
void tree_preorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
{

  if (not root->is_root())
    throw std::domain_error("root is not root");

  __tree_preorder_traversal(root, 0, 0, visitFct);
}

/** 
  @brief Walking on prefix a tree structure.

  @details forest_preorder_traversal((root,visit) takes a tour prefix on
  the root tree structure. If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level of p in the tree.
  -# child_index: Index of p within their brothers.

  @param[in] root The root of tree first tree in the tree structure.
  @param[in] visitFct Pointer to the function to visit.

  @throw domain_error If root is not a "root" node of a tree.

  @see tree_preorder_traversal() tree_postorder_traversal()
  @see forest_postorder_traversal()

  @ingroup Arboles
*/
    template <class Node> inline
void forest_preorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
{

  if (not root->is_root())
    throw std::domain_error("root is not root");

  for (/* Nothing */; root != NULL; root = root->get_right_tree())
    {
      assert(root->is_root());
    __tree_preorder_traversal(root, 0, 0, visitFct);
    }
}

    template <class Node> static inline
void __tree_postorder_traversal(Node * node, const int & level, 
                                const int & child_index,
                                void (*visitFct)(Node *, int, int))
{
  Node * child = node->get_left_child();

  for (int i = 0; child not_eq NULL; i++, child = child->get_right_sibling())
    __tree_postorder_traversal(child, level + 1, i, visitFct);

  (*visitFct)(node, level, child_index);
}

/** 
  @brief Walking on suffix a tree.

  @details tree_postorder_traversal((root,visit) takes a tour prefix on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level of p in the tree.
  -# child_index: Index of p within their brothers.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.

  @see forest_preorder_traversal() tree_preorder_traversal()
  @see forest_postorder_traversal()

  @ingroup Arboles
*/
    template <class Node> inline
void tree_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
{
  __tree_postorder_traversal(root, 0, 0, visitFct);
}

/** 
  @brief Walking on suffix a tree structure.

  @details forest_postorder_traversal((root,visit) takes a suffix tour on
  binary tree with root "root". If visitFct is specified,
  then for each node visited, the function is invoked.

  The view function has the following specification:

  void (*visitFct)(Node* p, int level, int pos)

  Where:
  -# p: Pointer to the node currently visited.
  -# level: Level of p in the tree.
  -# child_index: Index of p within their brothers.

  @param[in] root The root of the tree to visit.
  @param[in] visitFct Pointer to the function to visit.

  @see forest_preorder_traversal() tree_preorder_traversal()
  @see tree_postorder_traversal()

  @throw domain_error If root is no root node of the tree more to the
  left of the tree structure.

  @ingroup Arboles
*/
    template <class Node> inline
void forest_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
{
  if (not root->is_leftmost())
    throw std::domain_error("root is not the leftmost node of forest");

  if (not root->is_root())
    throw std::domain_error("root is not root");

  for (/* Nothing */; root not_eq NULL; root = root->get_right_sibling())
    {
      assert(root->is_root());
      __tree_postorder_traversal(root, 0, 0, visitFct);
    }
}

/** 
  @brief Destroy (free memory) tree whose root is "root".

  @details destroy_tree(root) set free all the memory occupied by the
  tree whose root is "root".

  @param[in] root the root of the tree you want to set free.

  @ingroup Arboles
*/
    template <class Node> inline 
void destroy_tree(Node * root)
{
  if (not IS_UNIQUE_SIBLING(root))
    SIBLING_LIST(root)->del(); /* No ==> sacarlo de lista hermanos */

      /* Recorrer los subárboles de derecha a izquierda */
  for (Node * p = (Node*) root->get_right_child(); p != NULL; /* Nada */)
    {
      Node * to_delete = p;      /* Respaldar subárbol a borrar p */
      p = (Node*) p->get_left_sibling(); /* Avanzar p a hermano izquierdo */
      destroy_tree(to_delete);   /* Eliminar recursivamente árbol */
    }

  if (root->is_leftmost()) /* ¿Sacar lista hijos? */
    CHILD_LIST(root)->del(); 

  delete root;
}

/** 
  @brief Destroy (free memory) down tree whose first tree is "root".

  @details destroy_forest(root) set free all the memory occupied by the
  the tree structure whose first tree has "root" root.

  @param[in] root The root of the first tree of the tree structure 
  to be destroyed.

  @throw domain_error If "root" is no root node of the tree more to the
  left of the tree structure.

  @ingroup Arboles
*/
    template <class Node> inline 
void destroy_forest(Node * root)
{

  if (not root->is_leftmost())
    throw std::domain_error("root is not the leftmost tree of forest");

  if (not root->is_root())
    throw std::domain_error("root is not root");

  while (root != NULL) /* Recorre los árboles de izquierda a derecha */
    {
      Node * to_delete = root;          /* Respalda raíz */
      root = root->get_right_sibling(); /* Avanza a siguiente árbol */
      SIBLING_LIST(to_delete)->del();   /* Elimine de lista árboles */ 
      destroy_tree(to_delete);          /* Borre el árbol */
    }
}

/** 
  @brief Calculate the height of the tree root.

  @param[in] root The root of tree.
  
  @return The height of the tree "root" root.
  
  @ingroup Arboles
*/
    template <class Node>
size_t compute_height(Node * root) 
{
  size_t temp_h, max_h = 0;
  for (Node * aux = root->get_left_child(); aux != NULL;
       aux = aux->get_right_sibling()) 
    if ((temp_h = compute_height(aux)) > max_h)
      max_h = temp_h;

  return max_h + 1;
}

    template <class Node> static inline
Node * __deway_search(Node * node, int path [], 
                      const int & idx, const size_t & size)
{
  if (node == NULL) 
    return NULL;

  if (idx > size)
    throw std::out_of_range("index out of maximum range");

  if (path[idx] < 0) /* Verifique si se ha alcanzado el nodo */
    return node; 
      /* Avance hasta el próximo hijo path[0] */
  Node * child = node->get_left_child();
  for (int i = 0; i < path[idx] and child != NULL; ++i)
    child = child->get_right_sibling();

  return __deway_search(child, path, idx + 1, size); /* Próximo nivel */
}

/** 
  @brief Returns a node of a tree structure Deway given their number.

  @details deway_search(root,path,size) takes the number of saved Deway
  in path, of length size and searches the tree structure whose first
  tree's "root" node that corresponds to the number of Deway given.

  @param[in] root The root of first tree of the tree structure.
  @param[in] path Array containing the number of Deway. 
  @param[in] size Size of deway number.
  
  @return pointer to the node corresponding to the number of Deway
  given; NULL if not,
  
  @ingroup Arboles
*/
    template <class Node> inline
Node * deway_search(Node * root, int path [], const size_t & size)
{
  for (int i = 0; root != NULL; i++, root = root->get_right_sibling())
    if (path[0] == i) 
      return __deway_search(root, path, 1, size);

  return NULL;
}

    template <class Node, class Equal> inline static
Node * __search_deway(Node * root, const typename Node::key_type & key,
                      const size_t & current_level, int deway [], 
                      const size_t & size, size_t & n);

/** 
  @brief Search "key" in the tree structure and calculates the number of Deway
  of contentivo node of the "key" key.

  @details search_deway(root,key,deway,n) searches the tree structure
  whose first tree is "root" a node containing the "key" key.
  If the node is found, then the routine saves deway[]
  the number of node Deway found.

  The search is performed with criterion of equality Equal()(). 

  @param[in] root The root of first tree of the tree structure.
  @param[in] key Key to search
  @param[out] deway Array containing the number of Deway. 
  @param[in] size Maximum size of the number of deway.
  @param[out] n Tamaño del número de Deway calculado (si se 
  encuentra el nodo).
  
  @return A pointer to the contentivo node of the "key" key;
  NULL if no node with "key" key.
  
  @throw overflow_error If "size" is not sufficient to store
  Deway sequence.
  
  @ingroup Arboles
*/
    template <class Node, 
              class Equal = Aleph::equal_to<typename Node::key_type> > inline
Node * search_deway(Node * root, const typename Node::key_type & key,
                    int deway [], const size_t & size, size_t & n)
{
  n = 1; /* Valor inicial de longitud de número de Deway */

  if (size < n)
    throw std::overflow_error("there is no enough space for deway array");

  for (int i = 0; root != NULL; i++, root = root->get_right_sibling())
    {
      deway[0] = i;
      Node * result =
        __search_deway <Node, Equal> (root, key, 0, deway, size, n);
      if (result != NULL) 
        return result;
    }

  return NULL;
}

    template <class Node, class Equal> inline static
Node * __search_deway(Node * root,  
                      const typename Node::key_type & key,
                      const size_t & current_level, int deway [], 
                      const size_t & size, size_t & n)
{

  if (current_level >= size) 
    throw std::overflow_error("there is no enough space for deway array");

  if (root == NULL) 
    return NULL;

  if (Equal () (KEY(root), key))
    {
      n = current_level + 1; /* Longitud del arreglo deway */
      return root;
    }

  Node * child = root->get_left_child();
  for (int i = 0; child != NULL; 
       i++, child = child->get_right_sibling())
    {
      deway[current_level + 1] = i;
      Node * result = __search_deway <Node, Equal> 
        (child, key, current_level + 1, deway, size, n);

      if (result!= NULL) 
        return result;
    }

  return NULL;
}

/** 
  @brief Becomes a tree structure to its equivalent binary tree.
        
  @details forest_to_bin(root) takes a tree structure derived from Tree_Node
  and converted to its equivalent binary tree.

  The routine operates two type parameters:
  -# TNode: Tree type based on Tree_Node.
  -# BNode: Type of binary tree based on BinNode.

  The procedure assumes that both share the same
  key type.
  
  @param[in] root The root pf the first tree belonging to
  turn the tree structure.
  
  @return The root of the tree structure equivalent to that given binary tree.

  @throw bad_alloc If there is insufficient memory.
  
  @see bin_to_forest()
  
  @ingroup Arboles
*/
    template <class TNode, class BNode>
BNode * forest_to_bin(TNode * root)
{
  if (root == NULL) 
    return BNode::NullPtr;  

  BNode * result = new BNode (root->get_key());
  LLINK(result) = forest_to_bin<TNode,BNode>(root->get_left_child());
  RLINK(result) = forest_to_bin<TNode,BNode>(root->get_right_sibling());

  return result;
}

    template <class TNode, class BNode> inline static
void insert_child(BNode * lnode, TNode * tree_node)
{
  if (lnode == BNode::NullPtr) 
    return;  

  TNode * child = new TNode(KEY(lnode));
  tree_node->insert_leftmost_child(child);
}

    template <class TNode, class BNode> inline static
void insert_sibling(BNode * rnode, TNode * tree_node)
{
  if (rnode == BNode::NullPtr) 
    return;  

  TNode * sibling = new TNode(KEY(rnode));
  tree_node->insert_right_sibling(sibling);
}

    template <class TNode, class BNode> inline static
void bin_to_tree(BNode * broot, TNode * troot)
{
  if (broot == BNode::NullPtr) 
    return;  

  insert_child(LLINK(broot), troot);
  TNode * left_child =  troot->get_left_child();

  bin_to_tree(LLINK(broot), left_child);

  insert_sibling(RLINK(broot), troot);
  TNode * right_sibling = troot->get_right_sibling();

  bin_to_tree(RLINK(broot), right_sibling);
}

/** 
  @brief Converts a binary tree equivalent tree structure.
        
  @details bin_to_forest(root) takes a binary tree derived BinNode
  and converts it to the equivalent tree structure.

  The routine operates two type parameters:
  -# TNode: Tree type based on Tree_Node.
  -# BNode: Binary tree type based on BinNode.

  The procedure assumes that both share the same
  key type.

  @param[in] broot Binary tree root to convert.

  @return The root of the first tree to binary tree equivalent
  given.

  @throw bad_alloc If there is insufficient memory.

  @see forest_to_bin()

  @ingroup Arboles
*/
    template <class TNode, class BNode> inline 
TNode * bin_to_forest(BNode * broot) 
{
  if (broot == BNode::NullPtr) 
    return NULL;  

  TNode * troot = new TNode (KEY(broot));
  bin_to_tree(broot, troot);
  return troot;
}

} /* fin namespace Aleph */

# endif // TPL_TREE_H
