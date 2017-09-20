/*
 *			    Top Dowm Splay Trees
 */

/* This code is a c++ adaptation of Danny Sleator code. It can be loaded from 

   http://www.link.cs.cmu.edu/link/ftp-site/splaying/SplayTree.java

 */

# ifndef TPL_SPLAY_TREE_H
# define TPL_SPLAY_TREE_H

# include <tpl_binNode.H>
# include <tpl_binNodeUtils.H>

using namespace Aleph;


    template <template <class> class NodeType, typename Key, class Compare>
class GenTdSplayTree 
{
public:

  using Node = NodeType<Key>;

private:

  Node headnode; 
  Node * head, *& root;
  Compare cmp;

public:
  
      /// El tipo de clave que contiene el nodo
  using key_type = Key;

      /// Retorna una referencia al criterio de comparación
  Compare & key_comp() noexcept { return cmp; }

      /// \overload key_comp()
  Compare & get_compare() noexcept { return key_comp(); }

  /** search key within tree and splay that node, if not found it return
      Node::NullPtr */ 
  void splay(const Key & key) noexcept
  {
    Node headNode;
    Node * l = &headNode;
    Node * r = &headNode;
    Node * t = root;

    while (true)
      if (cmp(key, KEY(t)))
	{     
	  if (LLINK(t) == Node::NullPtr)
	    break;

	  if (cmp(key, KEY(LLINK(t))))
	    {
	      t = rotate_to_right(t);
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
	      t = rotate_to_left(t);
	      if (RLINK(t) == Node::NullPtr)
		break;
	    }

	  RLINK(l) = t;
	  l        = t;
	  t        = RLINK(t);
	}
      else
	break;

    /* reassembling of sub trees in one with current as root */
    RLINK(l) = LLINK(t);
    LLINK(r) = RLINK(t);
    LLINK(t) = headNode.getR();
    RLINK(t) = headNode.getL();
		 
    root = t;
  }

  /// Constructor
  GenTdSplayTree(Compare cmp = Compare()) noexcept
    : head(&headnode), root(headnode.getR()), cmp(cmp)
  {
    // Empty
  }

  void swap(GenTdSplayTree & tree)
  {
    std::swap(root, tree.root);
    std::swap(cmp, tree.cmp);
  }

  /// Destructor
  virtual ~GenTdSplayTree() { /* empty */ }

private:

  Node * __insert(Node * p) noexcept
  {
    if (cmp(KEY(p), KEY(root)))
      { /* root is predecessor of p */
	LLINK(p)    = LLINK(root);
	RLINK(p)    = root;
	LLINK(root) = Node::NullPtr;
      }
    else 
      { /* root is successor of p */
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
  Node * insert(Node * p) noexcept
  {
    assert(p != Node::NullPtr);
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

  Node * insert_dup(Node * p) noexcept
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
  Node * search(const Key & key) noexcept
  {
    if (root == Node::NullPtr)
      return nullptr;
      
    splay(key);

    return are_equals<Key, Compare>(KEY(root), key) ? root : nullptr;
  }

  Node * search_or_insert(Node * p) noexcept
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
  Node * remove(const Key & key) noexcept
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
  Node *& getRoot() noexcept
  {
    return root;
  } 

  bool verify() const { return true; }

    /** Iterador sobre los nodos.

      Esta clase permite recorrer los nodos del árbol en forma
      ordenada según el criterio de comparación especificado en el
      momento de instanciación del árbol.

     @ingroup Arboles
  */
  struct Iterator : public BinNodeInfixIterator<Node>
  {
    Iterator(GenTdSplayTree & t) : BinNodeInfixIterator<Node>(t.getRoot()) {}
  };
};


    template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree : public GenTdSplayTree<BinNode, Key, Compare>
{
  using Base = GenTdSplayTree<BinNode, Key, Compare>;
  using Base::Base;
};


    template <typename Key, class Compare = Aleph::less<Key>>
struct Splay_Tree_Vtl : public GenTdSplayTree<BinNodeVtl, Key, Compare>
{
  using Base = GenTdSplayTree<BinNodeVtl, Key, Compare>;
  using Base::Base;
};


#endif /* TPL_SPLAY_TREE_H */
