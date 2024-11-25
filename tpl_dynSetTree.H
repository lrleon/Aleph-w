
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

# ifndef TPL_DYNSETTREE_H
# define TPL_DYNSETTREE_H

# include <typeinfo>
# include <ah-args-ctor.H>
# include <ahIterator.H>
# include <ah-zip.H>
# include <ah-arena.H>
# include <tpl_binNodeUtils.H>
# include <tpl_binNodeXt.H>
# include <tpl_binTree.H>
# include <tpl_treap.H>
# include <tpl_treapRk.H>
# include <tpl_avl.H>
# include <tpl_rand_tree.H>
# include <tpl_rb_tree.H>
# include <tpl_tdRbTree.H>
# include <tpl_splay_tree.H>

using namespace Aleph;

namespace Aleph {

  template <typename Node>
  class AbstractTreeNodeAllocator
  {
  public:

    AbstractTreeNodeAllocator() {}
    virtual Node * alloc_lval(const typename Node::key_type & key) = 0;
    virtual Node * alloc_rval(typename Node::key_type && key) = 0;
    virtual void unalloc(Node *) = 0;
    virtual ~AbstractTreeNodeAllocator() {};
  };

  template <typename Node>
  class DftTreeNodeAllocator : public AbstractTreeNodeAllocator<Node>
  {
  public:

    virtual Node * alloc_lval(const typename Node::key_type & key)
    {
      return new Node(key);
    }
    
    virtual Node * alloc_rval(typename Node::key_type && key)
    {
      return new Node(std::forward<typename Node::key_type>(key));
    }

    virtual void unalloc(Node * p)
    {
      delete p;
    }

    virtual ~DftTreeNodeAllocator() { /* empty */ }
  };

  template <typename Node>
  struct ArenaTreeAllocator : public AbstractTreeNodeAllocator<Node>
  {
    AhArenaAllocator arena;

    ArenaTreeAllocator(const size_t & sz = 1024*1024)
      : arena(sz)
    {
      // empty
    }

    ArenaTreeAllocator(const char * base_addr, const size_t & sz)
      : arena(base_addr, sz)
    {
      // empty
    }

    virtual Node * alloc_lval(const typename Node::key_type & key)
    {
      Node * ptr = allocate<Node>(arena, key);
      if (ptr == nullptr)
        throw bad_alloc();
      return ptr;
    }
    
    virtual Node * alloc_rval(typename Node::key_type && key)
    {
      Node * ptr =
        allocate<Node>(arena, std::forward<typename Node::key_type>(key));
      if (ptr == nullptr)
        throw bad_alloc();
      return ptr;
    }

    virtual void unalloc(Node * p)
    {
      dealloc<Node>(arena, p);
    }

    ~ArenaTreeAllocator() { /* empty */ }
  };

  template <typename Container, typename T>
  struct CmpContainer
  {
    bool operator () (const Container & c1, const Container & c2) const
    {
      return lexicographical_compare(c1.begin(), c1.end(), c2.begin(), c2.end(),
                                     [] (const T & item1, const T & item2)
                                     {
                                       return item1 < item2;
                                     });
    }
  };

  /** Dynamic set of elements of generic type T implemented
      through a class of binary trees.

      DynSetTree defines a table of elements of type Key that is
      instrumented with the Tree<Key> binary search tree class and
      sorted using the Compare()() criterion.

      @ingroup Arboles
  */
  template <typename Key,
            template <typename, class> class Tree = Avl_Tree,
            class Compare = Aleph::less<Key>>
  class DynSetTree
    : public LocateFunctions<DynSetTree<Key, Tree, Compare>, Key>,
      public FunctionalMethods<DynSetTree<Key, Tree, Compare>, Key>,
      public GenericKeys<DynSetTree<Key, Tree, Compare>, Key>,
      public EqualToMethod<DynSetTree<Key, Tree, Compare>>,
      public StlAlephIterator<DynSetTree<Key, Tree, Compare>>
  {
  public:

    /// Type of binary node used by the binary search tree
    /// internal.
    using Node = typename Tree<Key, Compare>::Node;

    using Tree_Type = Tree<Key, Compare>;

  private:

    static const size_t dim = 13;

    mutable Tree<Key, Compare> tree;
    size_t num_nodes;


  public:

    typedef DynSetTree Set_Type;

    typedef Key Item_Type;

    typedef Key Key_Type;

    /** Exchange all elements of the treap this with the treap tree
        in constant time (and extremely fast).

        @param[in] tree the treap to exchange with this
    */
    void swap(DynSetTree & dset) noexcept
    {
      tree.swap(dset.tree);
      std::swap(num_nodes, dset.num_nodes);
    }

    /// Instantiate a dynamic set
    DynSetTree(Compare cmp = Compare())
      : tree(cmp), num_nodes(0)
    {
      // empty
    }

    DynSetTree(const char * base_addr, const size_t & sz,
               Compare cmp = Compare())
      : tree(cmp), num_nodes(0)
    {
      // empty
    }

    /// instantiates a dynamic copy of srctree
    DynSetTree(const DynSetTree & srcTree)
      : tree(srcTree.tree.get_compare()), num_nodes(srcTree.num_nodes)
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

    Special_Ctors(DynSetTree, Key);

    DynSetTree(DynSetTree && srcTree)
      : num_nodes(0)
    {
      swap(srcTree);
    }

    /// remove all elements from the set
    void empty()
    {
      destroyRec(tree.getRoot());
      num_nodes = 0;
    }

    DynSetTree & operator = (const DynList<Key> & list)
    {
      return *this = DynSetTree(list);
    }

    /// assigns this to the dynamic array srctree
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

    /// Asigna a this el conjunto dinámico srcTree.
    DynSetTree & operator = (DynSetTree && srcTree)
    {
      swap(srcTree);
      return *this;
    }

    /// Destroyer; all elements are released.
    virtual ~DynSetTree()
    {
      empty();
    }

  private:

    Key * __insert(Node * p)
    {
      if (tree.search_or_insert(p) != p)
        return nullptr;

      ++num_nodes;

      return &p->get_key();
    }

  public:

    /** Inserts a key into the dynamic set.

        Inserts the key key into the dynamic set.

        @param[in] key key to insert.
        @return pointer to the inserted key if it is not inside
        of the tree; nullptr otherwise.
    */
    Key * insert(const Key & key)
    {
      Node * p = new Node(key);
      Key * key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        { // No (KEY(p) is already in the tree) ==> free p and return nullptr
          delete p;
          return nullptr;
        }

      return key_p;
    }

    Key * insert(Key && key)
    {
      Node * p = new Node(std::forward<Key>(key));
      Key * key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        {
          delete p;
          return nullptr;
        }

      return key_p;
    }

    Key * append(const Key & key)
    {
      return insert(key);
    }

    Key * append(Key && key)
    {
      return insert(std::forward<Key>(key));
    }

  private:

    Key * __search_or_insert(Node * p)
    {
      Node * q = tree.search_or_insert(p);
      if (q != p) // waw there an insertion?
        delete p; // No (KEY(p) is already in the tree) ==> free
                  // allocated node
      else
        ++num_nodes;

      return &q->get_key();
    }

    pair<Node*, bool> __contains_or_insert(Node * p)
    {
      Node * q = tree.search_or_insert(p);
      if (q != p)
        { // KEY(p) is already inside the tree
          delete p; 
          return pair<Node*, bool>(q, true);
        }
      ++num_nodes;
      return pair<Node*, bool>(p, false);
    }

  public:

    /** Look for the key <code>key</code> in the binary search tree or
        inserts it if it is not found.

        <code>search_or_insert(key)</code> searches the binary tree for a
        node whose key is <code>KEY(p)</code>. If the key is
        finds, then a pointer to it is returned; of what
        Otherwise <code>key</code> is inserted into the binary tree of
        search this.

        @param[in] key the key to search or insert.
        @return pointer to the key inside the tree
    */
    Key * search_or_insert(const Key & key)
    {
      return __search_or_insert(new Node(key));
    }

    Key * search_or_insert(Key && key)
    {
      return
        __search_or_insert(new Node(std::forward<Key>(key)));
    }

    /* Look for the key <code>key</code> in the binary search tree and
       eventually inserts it if it is not found.

       <code>contains_or_insert(key)</code> searches the tree for the key
       <code>key</code>. If the key is already found, then it is returned
       true. Otherwise, the key is inserted and false is returned.

       @param[in] key key to find or insert
       @return a pair whose first field is a pointer to the found or
       inserted key, and the second field is a boolean whose value is
       `false` if the key was inserted; `true` otherwise, that is if the
       key is already present in the tree.
    */
    pair<Key*, bool> contains_or_insert(const Key & key)
    {
      auto p = __contains_or_insert(new Node(key));
      return pair<Key*, bool>(&p.first->get_key(), p.second);
    }

    pair<Key*, bool> contains_or_insert(Key && key)
    {
      auto p = __contains_or_insert(new Node(std::forward<Key>(key)));
      return pair<Key*, bool>(&p.first->get_key(), p.second);
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
      return __insert_dup(new Node(key));
    }

    Key * insert_dup(Key && key)
    {
      return __insert_dup(new Node(std::forward<Key>(key)));
    }

    Key * put(const Key & key)
    {
      return insert(key);
    }

    Key * put(Key && key)
    {
      return insert(std::forward<Key>(key));
    }

    /** Removes a key from the dynamic set.

        remove(key) finds the key key in the set and removes it.

        @param[in] key key to delete
        @return number of elements that the set has.
    */
    size_t remove(const Key & key)
    {
      Node * p = static_cast<Node*>(tree.remove(key));

      if (p == nullptr)
        return num_nodes;

      delete p;

      return --num_nodes;
    }

    /** Deletes `key` and returns a full copy of stored key

        This method is intended to be used with compound keys, by example
        pairs, whose searching is done by a particular member of compound
        data.

        @param[in] key key to be removed
        @return a full copy of stored key
        @throw domain_error if the `key` is not found in the set
    */
    Key del(const Key & key)
    {
      Node * p = static_cast<Node*>(tree.remove(key));

      if (p == nullptr)
        throw domain_error("DynSetTree::del key is not found in the tree");

      auto ret_val = p->get_key();

      delete p;

      --num_nodes;

      return ret_val;
    }

    /** Removes a key from the dynamic set.

        remove(key) finds the key key in the set and removes it.

        @param[in] key key to delete
        @return number of elements that the set has.
    */
    Key remove_pos(const size_t i)
    {
      Node * p = static_cast<Node*>(tree.remove_pos(i));
      const Key ret_val = KEY(p);

      delete p;

      --num_nodes;

      return ret_val;
    }

    /// Returns true if key belongs to the dynamic set.
    bool exist(const Key & key) const
    {
      return const_cast<DynSetTree&>(*this).search(key) != nullptr;
    }

    bool has(const Key & key) const
    {
      return exist(key);
    }

    bool contains(const Key & key) const
    {
      return exist(key);
    }

    /** Returns a modifiable reference to an element within the
        set.

        find(key) looks for the key key in the set and returns a
        modifiable reference to the value contained in the set.

        @param[in] key key to search for.
        @return modifiable reference to the key contained within
        of the set.
        @throw domain_error if key is not within the set.

        @note Be very careful with altering the search order,
        since the reference is modifiable.
    */
    Key & find(const Key & key) const
    {
      Node * node = static_cast<Node*>(tree.search(key));

      if (node == nullptr)
        throw std::domain_error("key not found");

      return node->get_key();
    }

    /** Returns the infix (ordinate) position of the key key or whatever
        It would be your position of belonging to the tree.

        find_position(key) searches the extended tree for the key key
        (which takes time \f$O(\lg n)\f$) and returns the position
        infix of the node containing the key.

        @param[in] key key to search for and determine infix position.
        @return infix position of the key within the set
        ordered if it is found or, otherwise, the position
        where it would be if it belonged to the tree.

        @warning It only works if the tree handles ranges.
    */
    std::pair<int, Key*> find_position(const Key & key) const
    {
      if (num_nodes == 0)
        return std::pair<int, Key*> (0, nullptr);

      std::pair<int, Node*> p = tree.find_position(key);

      return std::pair<int, Key*> (p.first, &p.second->get_key());
    }

    /** Find an element in the set.

        search(key) searches for the key key in the set. If he
        element is found in the set, then a
        pointer to the value contained in the set; otherwise
        nullptr is returned.

        @param[in] key key to search for.
        @return pointer to the element in the set if it is
        found in it; nullptr of the opposite.

        @note Be very careful with altering the search order,
        because through the pointer the key is modifiable.
    */
    Key * search(const Key & key) const
    {
      Node * node = static_cast<Node*>(tree.search(key));

      if (node == nullptr)
        return nullptr;

      return &(node->get_key());
    }

    /// Returns the smallest key contained in the set according to the criterion
    /// comparison given.
    const Key & min() const
    {
      if (num_nodes == 0)
        throw std::domain_error("set is empty");

      return find_min(tree.getRoot())->get_key();
    }

    /// \overload min
    const Key & get_first() const { return min(); }

    /// Returns the largest key contained in the set according to the criteria
    /// comparison given.
    const Key & max() const
    {
      if (num_nodes == 0)
        throw std::domain_error("set is empty");

      return find_max(tree.getRoot())->get_key();
    }

    /// \overload max
    const Key & get_last() const { return max(); }

    /// Sinónimo de max
    const Key & get() const { return max(); }

    /// Returns the cardinality of the set.
    const size_t & size() const { return num_nodes; }

    /// returns true if the set is empty
    bool is_empty() const { return num_nodes == 0; }

    /// Calculates and returns the length of the internal path of the tree
    /// search binary.
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

    /// Returns any element of the set
    const Key & get_item() const { return get_root(); }

    /// Calculates and returns the height of the binary search tree.
    size_t height() const { return computeHeightRec(tree.getRoot()); }

    /// Performs a prefix traversal over all nodes in the tree and
    /// invokes the <code>visitFct</code> operation on each visit
    template <class Op>
    void for_each_in_preorder(void (*visitFct)(Node*, int, int))
    {
      Node * root = static_cast<Node*>(tree.getRoot());
      preOrderRec(root, visitFct);
    }

    /** Returns the infix (ordered) position of the key key.

        position(key) searches the extended treap for the key key (what
        which takes time \f$O(\lg n)\f$) and returns the position
        infix of the node containing the key.

        @param[in] key key to search for and determine infix position.
        @return infix position of the key within the set
        sorted if key found; -1 otherwise

        @warning It only works if the tree handles ranges.
    */
    long position(const Key & key) const
    {
      std::pair<long, Node*> p = tree.position(key);
      return p.first;
    }

    /** Returns the ith node in infix position

        @param[in] i desired position
        @return reference to the ith key inserted into the set
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

    const Key & operator [] (const Key & key) const
    {
      return find(key);
    }

    const Key & operator [] (const Key & key)
    {
      return *search_or_insert(key);
    }

    Key & access (const size_t & i)
    {
      return select(i);
    }

    bool verify()
    {
      return tree.verify() and check_bst(tree.getRoot());
    }

  private:

    template <class Key_Op>
    struct Node_Op
    {
      Key_Op & key_op;

      Node_Op(Key_Op & __key_op) : key_op(__key_op) { /* empty */ }

      Node_Op(Key_Op && __key_op) : key_op(__key_op)
      {
        /* empty */
      }

      void operator () (Node * root)
      {
        assert(root != nullptr);
        key_op(KEY(root));
      }
    };

  public:

    /** Efectúa un recorrido prefijo sobre todas las claves del conjunto e
        invoca la operacion Op.

        <code>Op(p)</code> tiene la siguiente estructura:
        \code
        struct Key_Op
        {
        // atributos de estado que se deseen mantener

        Key_Op(...) // constructor opcional si es necesario inicializar
        {
        // inicialización
        }

        void operator () (const Key & key)
        {
        // operación sobre clave key
        }
        };
        \endcode

        @param[in] key_op operación a ejecutar sobre cada clave
        \see For_Each_Preorder
    */
    template <class Key_Op>
    void for_each_preorder(Key_Op & key_op) const
    {
      Node * root = (Node *) tree.getRoot();

      Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

      For_Each_Preorder<Node> () (root, node_op);
    }

    /// \overload for_each_preorder
    template <class Key_Op>
    void for_each_preorder(Key_Op && key_op = Key_Op()) const
    {
      for_each_preorder<Key_Op>(key_op);
    }

    /** Efectúa un recorrido infijo sobre todas las claves del conjunto e
        invoca la operacion Op.

        <code>Op(p)</code> tiene la siguiente estructura:
        \code
        struct Key_Op
        {
        // atributos de estado que se deseen mantener

        Key_Op(...) // constructor opcional si es necesario inicializar
        {
        // inicialización
        }

        void operator () (const Key & key)
        {
        // operación sobre clave key
        }
        };
        \endcode

        @param[in] key_op operación a ejecutar sobre cada clave
        \see For_Each_In_Order
    */
    template <class Key_Op>
    void for_each_inorder(Key_Op & key_op) const
    {
      Node * root = (Node *) tree.getRoot();

      Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

      For_Each_In_Order<Node> () (root, node_op);
    }

    /// \overload for_each_inorder
    template <class Key_Op>
    void for_each_inorder(Key_Op && key_op = Key_Op()) const
    {
      for_each_inorder<Key_Op>(key_op);
    }

    /** Efectúa un recorrido sufijo sobre todas las claves del conjunto e
        invoca la operacion Op.

        <code>Op(p)</code> tiene la siguiente estructura:
        \code
        struct Key_Op
        {
        // atributos de estado que se deseen mantener

        Key_Op(...) // constructor opcional si es necesario inicializar
        {
        // inicialización
        }

        void operator () (const Key & key)
        {
        // operación sobre clave key
        }
        };
        \endcode

        @param[in] key_op operación a ejecutar sobre cada clave
        \see For_Each_Postorder
    */
    template <class Key_Op>
    void for_each_postorder(Key_Op & key_op)
    {
      Node * root = (Node *) tree.getRoot();

      Node_Op <Key_Op> node_op(const_cast<Key_Op&>(key_op));

      For_Each_Postorder<Node> () (root, node_op);
    }

    /// \overload for_each_postorder
    template <class Key_Op>
    void for_each_postorder(Key_Op && key_op = Key_Op())
    {
      for_each_postorder<Key_Op>(key_op);
    }

    /** Unión de dos árboles binarios de búsqueda.

        join(t,dup) construye un árbol binario de búsqueda
        correspondiente a la unión de this con t. Las claves duplicadas
        se inserta, en dup.

        @param[in] t árbol binario de búsqueda que se quiere unir a this.
        @param[out] dup árbol binario de búsqueda con las claves
        duplicadas de t.
        @note Luego de las llamadas el árbol t deviene vacíos y this deviene
        la unión de ambos árboles;
        sin embargo los valores de t1 y t2 no se modifican.
        @return this
        @ingroup Arboles
    */
    DynSetTree & join(DynSetTree & t, DynSetTree & dup)
    {
      tree.join(t.tree, dup.tree);
      return *this;
    }

    /// \overload join
    DynSetTree & join(DynSetTree & t, DynSetTree && dup = DynSetTree())
    {
      return join(t, dup);
    }

    /** Union of two binary search trees.

        join_dup(t) builds a binary search tree
        corresponding to the union of this with t in which there may be
        duplicate keys.

        @param[in] t binary search tree that you want to join to this.
        @note After the calls the t-tree becomes empty and this becomes
        the union of both trees;
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

    /** Partitions the binary search tree based on a key.

        split_key(key,l,r) partitions, according to the key key, the
        binary search tree this in two trees l and r. After the
        operation the tree this becomes empty, l contains
        all keys less than key and r the major ones.

        @param[in] key partition key.
        @param[out] l tree with keys less than key.
        @param[out] r tree with keys greater than key.
        @return true if key is not inside the binary tree; in which
        case the partition was done successfully. Otherwise, yes
        key is located inside the tree, the tree is not partitioned and the
        result is false.
    */
    bool split_key(const Key & key, DynSetTree & l, DynSetTree & r)
    {
      if (not tree.split_key(key, l.tree, r.tree))
        return false;

      tree.getRoot() = Node::NullPtr;
      num_nodes = 0;
      l.num_nodes = COUNT(l.tree.getRoot());
      r.num_nodes = COUNT(r.tree.getRoot());

      return true;
    }

    /** Particiona el árbol binario de búsqueda según una posición
        infija.

        split_pos(pos,l,r) particiona árbol binario de búsqueda this
        en dos árboles l y r. Luego de la operación el árbol this
        deviene vacío, l contiene las pos primeras claves y r las
        restantes.

        @param[in] pos posición de partición
        @param[out] l árbol con las claves entre intervalo [0,pos]
        @param[out] r árbol con las claves en el intervalo (pos,N),
        donde N es el número de claves
    */
    void split_pos(const size_t pos, DynSetTree & l, DynSetTree & r)
    {
      tree.split_pos(pos, l.tree, r.tree);
      num_nodes = 0;
      l.num_nodes = COUNT(l.tree.getRoot());
      r.num_nodes = COUNT(r.tree.getRoot());
    }

    /** Particiona el árbol binario de búsqueda según una clave que
        puede estar presente en el árbol.

        split_dup(key,l,r) particiona, según la clave key, el
        árbol binario de búsqueda this en dos árboles l y r. Luego de la
        operación el árbol this deviene vacío, l contiene
        todas las claves menores que key y r las mayores o iguales.

        @param[in] key clave de partición.
        @param[out] l árbol con las claves menores que key.
        @param[out] r árbol con las claves mayores o iguales que key.
    */
    void split_key_dup(const Key & key, DynSetTree & l, DynSetTree & r)
    {
      tree.split_key_dup(key, l.tree, r.tree);
      tree.getRoot() = Node::NullPtr;
      num_nodes = 0;
      l.num_nodes = COUNT(l.tree.getRoot());
      r.num_nodes = COUNT(r.tree.getRoot());
    }

    struct Iterator : public Tree_Type::Iterator
    {
      using Base = typename Tree_Type::Iterator;

      using Set_Type = DynSetTree;

      Iterator(const DynSetTree & tree) : Base(tree.tree) { /* empty */ }

      const Key & get_curr_ne() const noexcept
      {
        return Base::get_curr_ne()->get_key();
      }

      Key & get_curr_ne() noexcept { return Base::get_curr_ne()->get_key(); }

      const Key & get_curr() const { return Base::get_curr()->get_key(); }

      Key & get_curr() { return Base::get_curr()->get_key(); }
    };

    /** Traverse all the set of pairs and for each key executes the operation op.

        Operation must have the signature

        \code
        bool operation(T & item)
        \endcode

        If \code operation() \endcode returns false then the traversal is
        aborted; otherwise the the routine advance and so on

        @param[in] operation
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
  };


# define SETTREE_ITOR(Name, Key, Cmp)                           \
  class Iterator : public DynSetTree<Key, Name, Cmp>::Iterator  \
  {                                                             \
  public:                                                       \
    Iterator() : DynSetTree<Key, Name, Cmp>::Iterator()         \
      { /* empty */ }                                           \
                                                                \
    Iterator(DynSetTree<Key, Name, Cmp> & tree)                 \
      : DynSetTree<Key, Name, Cmp>::Iterator(tree)              \
      { /* empty */ }                                           \
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      de tipo BinTree<Key>.

      @see DynSetTree DynMapTree BinTree<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetBinTree : public DynSetTree<Key, BinTree, Compare>
  {
  public:
    using Base = DynSetTree<Key, BinTree, Compare>;
    using Base::Base;
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      AVL de tipo Avl_Tree<Key>.

      @see DynSetTree DynMapTree Avl_Tree<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetAvlTree : public DynSetTree<Key, Avl_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Avl_Tree, Compare>;
    using Base::Base;
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      splay de tipo Splay_Tree<Key>.

      @see DynSetTree DynMapTree Splay_Tree<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetSplayTree : public DynSetTree<Key, Splay_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Splay_Tree, Compare>;
    using Base::Base;
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      aleatorios de tipo Rand_Tree<Key>.

      @see DynSetTree DynMapTree Rand_Tree<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetRandTree : public DynSetTree<Key, Rand_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Rand_Tree, Compare>;
    using Base::Base;

    class Iterator : public DynSetTree<Key, Rand_Tree, Compare>::Iterator
    {
    public:
      Iterator() : DynSetTree<Key, Rand_Tree, Compare>::Iterator()
      { /* empty */ }

      Iterator(DynSetTree<Key, Rand_Tree, Compare> & tree)
        : DynSetTree<Key, Rand_Tree, Compare>::Iterator(tree)
      { /* empty */ }
    };
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      aleatorios treaps de tipo Treap<Key>.

      @see DynSetTree DynMapTree Treap<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTreap : public DynSetTree<Key, Treap, Compare>
  {
  public:
    using Base = DynSetTree<Key, Treap, Compare>;
    using Base::Base;
  };

  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      aleatorios treaps de tipo Treap<Key>.

      @see DynSetTree DynMapTree Treap<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetTreapRk : public DynSetTree<Key, Treap_Rk, Compare>
  {
  public:
    using Base = DynSetTree<Key, Treap_Rk, Compare>;
    using Base::Base;
    SETTREE_ITOR(Treap_Rk, Key, Compare);
  };


  /** Conjunto dinámico implementado mediante árboles binarios de búsqueda
      aleatorios treaps de tipo Rb_Tree<Key>.

      @see DynSetTree DynMapTree Rb_Tree<Key>
      @ingroup Arboles
  */
  template  <typename Key, class Compare = Aleph::less<Key>>
  class DynSetRbTree : public DynSetTree<Key, Rb_Tree, Compare>
  {
  public:
    using Base = DynSetTree<Key, Rb_Tree, Compare>;
    using Base::Base;
  };


  template <typename T, class Op, class C>
  DynSetTree<T> set_unify(const C & c, Op op)
  {
    DynSetTree<T> ret;
    for (auto it = c.get_it(); it.has_curr(); it.next_ne())
      ret.insert(op(it.get_curr()));
    return ret;
  }

} // end namespace Aleph

# endif /* TPL_DYNSETTREE_H */
