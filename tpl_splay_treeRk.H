
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
/*
 *			    Top Dowm Splay Trees
 */

/* This code is a c++ adaptation of Danny Sleator code. It can be loaded from 

   http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java

 */

# ifndef TPL_SPLAY_TREE_H
# define TPL_SPLAY_TREE_H

# include <tpl_binNodeXt.H>


using namespace Aleph;


    template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTreeRk
{
public:

  typedef NodeType<Key> Node;

private:

  Node * root;
  Compare cmp;

public:
  
      /// El tipo de clave que contiene el nodo
  typedef Key key_type;

      /// Retorna una referencia al criterio de comparación
  Compare & key_comp() { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() { return key_comp(); }

  /** search key within tree and splay that node, if not found it return
      Node::NullPtr */ 
  void splay(const Key & key) noexcept
  {
    Node header(sentinelCtor);
    Node * head_ptr = &header;
    Node * l = head_ptr;
    Node * r = head_ptr;
    Node * t = root;

    while (true)
      if (cmp(key, KEY(t)))
	{     
	  if (LLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(key, KEY(LLINK(t))))
	    {
	      t = rotate_to_right_xt(t);
	      if (LLINK(t) == Node::NullPtr)
		break;
	    }

	  LLINK(r) = t;
	  r        = t;
	  t        = LLINK(t);
	}
      else if (cmp(KEY(t), key))
	{   
	  if (RLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(KEY(RLINK(t)), key))
	    {
	      t = rotate_to_left_xt(t);
	      if (RLINK(t) == Node::NullPtr)
		break;
	    }

	  RLINK(l) = t;
	  l        = t;
	  t        = RLINK(t);
	}
      else
	break;

    /* 
       t points to the splayed node
       l points to the predecessor of splayed node
       r points to the predecessor of splayed node
    */

    if (l != head_ptr)
      COUNT(l) += -COUNT(RLINK(l)) + COUNT(LLINK(t));
    if (r != head_ptr)
      COUNT(r) += -COUNT(LLINK(r)) + COUNT(RLINK(t));

    /* reassembling of sub trees in one with t as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);

    LLINK(t) = RLINK(head_ptr);
    RLINK(t) = LLINK(head_ptr);

    COUNT(t) = COUNT(LLINK(t)) + 1 + COUNT(RLINK(t));

    root = t;
  }

  /// Constructor
  GenTdSplayTreeRk(Compare __cmp = Compare())
      : root(Node::NullPtr), cmp(__cmp)
  {
    // Empty
  }

  void swap(GenTdSplayTreeRk & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// Destructor
  virtual ~GenTdSplayTreeRk() { /* empty */ }

private:

  Node * __insert(Node * p)
  {
    COUNT(p) = COUNT(root) + 1;
    if (cmp(KEY(p), KEY(root)))
      { 
	COUNT(root) -= COUNT(LLINK(root));
	LLINK(p)     = LLINK(root);
	RLINK(p)     = root;
	LLINK(root)  = Node::NullPtr;
      }
    else 
      { 
	COUNT(root) -= COUNT(RLINK(root));
	RLINK(p)    = RLINK(root);
	LLINK(p)    = root;
	RLINK(root) = Node::NullPtr;
      }

    return root = p; // inserted node become root
  }

public:

  /** Inserts a node in a top down splay tree.

      @return a pointer to the inserted node if node is not found in tree;
      nullptr otherwise.
      @param p a pointer to the node to be inserted
  */
  Node * insert(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(COUNT(p) == 1);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);  

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);

    splay(key);

    if (are_equals<Key, Compare>(KEY(root), key, cmp))
      return nullptr; // item is already in tree

    return __insert(p);
  }

  Node * insert_dup(Node * p)
  {
    assert(p != Node::NullPtr);
    assert(LLINK(p) == Node::NullPtr and RLINK(p) == Node::NullPtr);  

    /* test insertion in empty tree */
    if (root == Node::NullPtr)
      return root = p;

    splay(KEY(p));

    return __insert(p);
  }

  /**
     Searches a key in a top down splay tree.

     @return a pointer to the node containing the key if the key is found;
     nullptr otherwise.
     @param key key to search
  */
  Node * search(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;
      
    splay(key);

    return are_equals<Key, Compare>(KEY(root), key) ? root : nullptr;
  }

  Node * search_or_insert(Node * p)
  {
    if (root == Node::NullPtr)
      return root = p;

    const Key & key = KEY(p);
    splay(key);
    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      return root;

    return __insert(p);
  }

  /**
     Remove a key from a top down splay tree.

     Searches a key in a top down splay tree and remove the containing
     the key if this is found. 

     @return a pointer to node containing the removed key.
     @param key to search
  */
  Node * remove(const Key & key)
  {
    if (root == Node::NullPtr)
      return nullptr;

    splay(key);
      
    if (no_equals<Key, Compare>(KEY(root), key, cmp))
      return nullptr; /* key not found */

    Node * ret_val = root; /* store node to delete */

    if (LLINK(root) == Node::NullPtr)
      root = RLINK(root);
    else
      {
	Node * p = RLINK(root);
	root = LLINK(root);
	splay(key);
	RLINK(root) = p;
      }

    ret_val->reset();

    return ret_val;
  }

  /// Get the top down splay tree's root
  Node *& getRoot()
  {
    return root;
  } 

  bool verify() const { return check_rank_tree(root); }

      /// Retorna la cantidad de nodos que contiene el treap.
  size_t size() const 
  {
    return COUNT(root);
  }

        /// Retorna true si el treap está vacío.
  bool is_empty() const 
  {
    return root == Node::NullPtr; 
  }

  /** Retorna la posición infija (ordenada) de la clave key.

      position(key) busca en el árbol splay extendido la clave key (lo cual
      toma tiempo \f$O(\lg n)\f$) y retorna la posición infija del nodo
      que contiene la clave.

      @param[in] key clave a buscar y a determinar posición infija.
      @return un par conteniendo la posición infija de la clave key
      dentro del conjunto ordenado junto con el nodo si la clave se
      encuentra en el mapeo; de lo contrarion, se retorna -1 y el nodo
      es indeterminado.
   */
  std::pair<int, Node*> position (const Key & key)
  {
    std::pair<int, Node*> ret_val;

    splay(key);

    if (are_equals<Key, Compare>(key, KEY(root), cmp))
      {
	ret_val.first = COUNT(LLINK(root));
	ret_val.second = root;
      }
    else
      ret_val.first = -1;

    return ret_val;
  }

  /** Retorna la posición infija (ordenada) de la clave key.

      find_position(key) busca en el árbo splay extendido la clave key (lo cual
      toma tiempo \f$O(\lg n)\f$) y retorna la posición infija del nodo
      que contiene la clave junto con el nodo. En caso de que la clave
      no esté en el árbol, position(key) retorna la posición donde
      debería estar si key perteneciese al árbol.

      El valor de retorno es un pair<int,Node*>. El campo first es la
      posición infija y second el nodo.

      Si key no se encuentra en el árbol, entonces se pueden
      distinguir tres casos:
	-# Si key es menor que la mínima clave del árbol, entonces first
	 -1 y second es el nodo contentivo de la clave mínima. 
	-# Si key es mayor que la máxima clave del árbol, entonces first
	es COUNT(r) (número de nodos del árbol) y second es
	el nodo contentivo de la clave máxima.
	-# En cualquier otro caso, first es la posición
	que tendría la clave key en árbol y second nodo p es una clave
	inmediatamente adyancente a key. Note que second puede tener una
	clave menor o mayor, pero se garantiza que es inmediatamente
	continua a key.

      @param[in] key clave a buscar y a determinar posición infija.
      @return un par conteniendo la posición infija de la clave key
      dentro del conjunto ordenado junto con el nodo si la clave se
      encuentra en el mapeo; de lo contrarion, se retorna -1 y el nodo
      es indeterminado.
   */
  std::pair<int, Node*> find_position (const Key & key)
  {
    std::pair<int, Node*> r(-2, nullptr);

    r.first = COUNT(Splay(key));
    r.second = root;

    return r;
  }

      /** Retorna el nodo cuya posición infija en el treap extendido es i.

	  select(i) retorna el nodo del árbol splay extendido cuya
	  posición infija es i. 

	  @param[in] i posición infija a seleccionar.
	  @return puntero al nodo correspondiente a la posición infija i.
	  @throw out_of_range si i es mayor o igual que la cantidad de nodos
	  que contiene el árbol.
      */
  Node * select (const size_t & i)
  {
    return Aleph::select(root, i); 
  }
};


    template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk : public GenTdSplayTreeRk<BinNodeXt, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXt, Key, Compare>;
  using Base::Base;
};


    template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Rk_Vtl : public GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>
{
  using Base = GenTdSplayTreeRk<BinNodeXtVtl, Key, Compare>;
  using Base::Base;
};


#endif /* TPL_SPLAY_TREE_H */
