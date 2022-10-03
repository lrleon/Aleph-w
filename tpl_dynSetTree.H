
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
      return new Node(forward<typename Node::key_type>(key));
    }

    virtual void unalloc(Node * p)
    {
      delete p;
    }

    virtual ~DftTreeNodeAllocator() { /* empty */ }
  };
    

  /** Conjunto dinámico de elementos de tipo genérico T implementado
      mediante una clase de árboles binarios.

      DynSetTree define una tabla de elementos de tipo Key que está
      instrumentada con la clase de árbol binario de búsqueda Tree<Key> y
      ordenado mediante el criterio Compare()().

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

    /// Tipo de nodo binario empleado por el árbol binario de búsqueda
    /// interno.
    using Node = typename Tree<Key, Compare>::Node;

    using Tree_Type = Tree<Key, Compare>;

  private:

    static const size_t dim = 13;

    mutable Tree<Key, Compare> tree;
    size_t num_nodes;

    AbstractTreeNodeAllocator<Node> * allocator_ptr = nullptr;

  public:

    typedef DynSetTree Set_Type;

    typedef Key Item_Type;

    typedef Key Key_Type;

    void set_allocator(AbstractTreeNodeAllocator<Node> * alloc_ptr)
    {
      allocator_ptr = alloc_ptr;
    }

    /** Intercambia todos los elementos del treap this con el treap tree
        en tiempo contante (y extremadamente rápido).

        @param[in] tree el treap a intercambiar con this
    */
    void swap(DynSetTree & dset) noexcept
    {
      tree.swap(dset.tree);
      std::swap(num_nodes, dset.num_nodes);
    }

    /// Instancia un conjunto dinámico
    DynSetTree(Compare cmp = Compare())
      : tree(cmp), num_nodes(0),
        allocator_ptr(new DftTreeNodeAllocator<Node>)
    {
      // empty
    }

    /// Instancia un conjunto dinámico copia de srcTree.
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

    DynSetTree(DynSetTree && srcTree) : num_nodes(0)
    {
      swap(srcTree);
    }

    /// Elimina todos los elementos del conjunto
    void empty()
    {
      destroyRec(tree.getRoot());
      num_nodes = 0;
    }

    DynSetTree & operator = (const DynList<Key> & list)
    {
      return *this = DynSetTree(list);
    }

    /// Asigna a this el conjunto dinámico srcTree.
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

    /// Destructor; todos los elementos son liberados.
    virtual ~DynSetTree()
    {
      empty();
      if (allocator_ptr != nullptr)
        delete allocator_ptr;
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

    /** Inserta una clave en el conjunto dinámico.

        Inserta la clave key en el conjunto dinámico.

        @param[in] key clave a insertar.
        @return puntero a la clave insertada si ésta no está dentro
        del árbol; nullptr de lo contrario.
    */
    Key * insert(const Key & key)
    {
      Node * p = allocator_ptr->alloc_lval(key);
      Key * key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        {
          allocator_ptr->unalloc(p);
          return nullptr;
        }

      return key_p;
    }

    Key * insert(Key && key)
    {
      Node * p = allocator_ptr->alloc_rval(forward<Key>(key));
      Key * key_p = __insert(p);
      if (key_p == nullptr) // was there insertion?
        {
          allocator_ptr->unalloc(p);
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
        allocator_ptr->unalloc(p); // No (KEY(p) is already in the tree) ==>
                                   // free allocated node
      else
        ++num_nodes;

      return &q->get_key();
    }

    pair<Node*, bool> __contains_or_insert(Node * p)
    {
      Node * q = tree.search_or_insert(p);
      if (q != p)
        { // KEY(p) is already inside the tree
          allocator_ptr->unalloc(p);
          return pair<Node*, bool>(q, true);
        }
      ++num_nodes;
      return pair<Node*, bool>(p, false);
    }

  public:

    /** Busca la clave <code>key</code> en el árbol binario de búsqueda o
        lo inserta en caso de no encontrarse.

        <code>search_or_insert(key)</code> busca en el árbol binario un
        nodo cuya clave sea <code>KEY(p)</code>. Si la clave se
        encuentra, entonces se retorna un puntero a ella; de lo
        contrario se inserta <code>key</code> en el árbol binario de
        búsqueda this.

        @param[in] key la clave a buscar o insertar.
        @return puntero a la clave dentro del árbol
    */
    Key * search_or_insert(const Key & key)
    {
      return __search_or_insert(allocator_ptr->alloc_lval(key));
    }

    Key * search_or_insert(Key && key)
    {
      return
        __search_or_insert(allocator_ptr->alloc_rval(forward<Key>(key)));
    }

    /* Busca la clave <code>key</code> en el árbol binario de búsqueda y
       eventualmente lo inserta en caso de no encontrarse.

       <code>contains_or_insert(key)</code> busca en el árbol la clave
       <code>key</code>. Si la clave ya se encuentra, entonces se retorna
       true. De lo contrario la clave es insertada y se retorna false.

       @param[in] key clave a encontrar o a insertar
       @return a pair whose first field is a pointer to the found or
       inserted key and the second field is a boolean whose value is
       `false` if the key was inserted; `true` otherwise, that is if the
       key is already present in the tree.
    */
    pair<Key*, bool> contains_or_insert(const Key & key)
    {
      auto p = __contains_or_insert(allocator_ptr->alloc_lval(key));
      return pair<Key*, bool>(&p.first->get_key(), p.second);
    }

    pair<Key*, bool> contains_or_insert(Key && key)
    {
      auto p =
        __contains_or_insert(allocator_ptr->alloc_rval(forward<Key>(key)));
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
      return __insert_dup(allocator_ptr->alloc_lval(key));
    }

    Key * insert_dup(Key && key)
    {
      return __insert_dup(allocator_ptr->alloc_rval(forward<Key>(key)));
    }

    Key * put(const Key & key)
    {
      return insert(key);
    }

    Key * put(Key && key)
    {
      return insert(std::forward<Key>(key));
    }

    /** Elimina una clave del conjunto dinámico.

        remove(key) busca la clave key del conjunto y la elimina.

        @param[in] key clave a eliminar
        @return número de elementos que tiene el conjunto.
    */
    size_t remove(const Key & key)
    {
      Node * p = static_cast<Node*>(tree.remove(key));

      if (p == nullptr)
        return num_nodes;

      allocator_ptr->unalloc(p);

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

      allocator_ptr->unalloc(p);

      --num_nodes;

      return ret_val;
    }

    /** Elimina una clave del conjunto dinámico.

        remove(key) busca la clave key del conjunto y la elimina.

        @param[in] key clave a eliminar
        @return número de elementos que tiene el conjunto.
    */
    Key remove_pos(const size_t i)
    {
      Node * p = static_cast<Node*>(tree.remove_pos(i));
      const Key ret_val = KEY(p);

      allocator_ptr->unalloc(p);

      --num_nodes;

      return ret_val;
    }

    /// Retorna true si key pertenece al conjunto dinámico.
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

    /** Retorna una referencia modificable a un elemento dentro del
        conjunto.

        find(key) busca la clave key en el conjunto y retorna una
        referencia modificable hacia el valor contenido en el conjunto.

        @param[in] key clave a buscar.
        @return referencia modificable a la clave key contenida dentro
        del conjunto.
        @throw domain_error si key no está dentro del conjunto.

        @note Téngase sumo cuidado con  alterar el orden de búsqueda,
        pues la referencia es modificable.
    */
    Key & find(const Key & key) const
    {
      Node * node = static_cast<Node*>(tree.search(key));

      if (node == nullptr)
        throw std::domain_error("key not found");

      return node->get_key();
    }

    /** Retorna la posición infija (ordenada) de la clave key o lo que
        seria su posición de pertenecer al árbol.

        find_position(key) busca en el árbol extendido la clave key
        (lo cual toma tiempo \f$O(\lg n)\f$) y retorna la posición
        infija del nodo que contiene la clave.

        @param[in] key clave a buscar y a determinar posición infija.
        @return posición infija de la clave key dentro del conjunto
        ordenado si ésta se encuentra o, de lo contrario, la posición
        donde se encontraría de pertenecer al árbol.

        @warning Sólo funciona si el árbol maneja rangos.
    */
    std::pair<int, Key*> find_position(const Key & key) const
    {
      if (num_nodes == 0)
        return std::pair<int, Key*> (0, nullptr);

      std::pair<int, Node*> p = tree.find_position(key);

      return std::pair<int, Key*> (p.first, &p.second->get_key());
    }

    /** Busca un elemento en el conjunto.

        search(key) busca la clave key en el conjunto. Si el
        elemento se encuentra en el conjunto, entonces se retorna un
        puntero al valor contenido en el conjunto; de lo contrario
        se retrona nullptr.

        @param[in] key clave a buscar.
        @return puntero al elemento en en el conjunto si éste se
        encuentra en el mismo; nullptr de l contrario.

        @note Téngase sumo cuidado con  alterar el orden de búsqueda,
        pues  a través del puntero la clave es modificable.
    */
    Key * search(const Key & key) const
    {
      Node * node = static_cast<Node*>(tree.search(key));

      if (node == nullptr)
        return nullptr;

      return &(node->get_key());
    }

    /// Retorna la menor clave contenida en el conjunto según el criterio
    /// de comparación dado.
    const Key & min() const
    {
      if (num_nodes == 0)
        throw std::domain_error("set is empty");

      return find_min(tree.getRoot())->get_key();
    }

    /// \overload min
    const Key & get_first() const { return min(); }

    /// Retorna la mayor clave contenida en el conjunto según el criterio
    /// de comparación dado.
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

    /// Retorna la cardinalidad del conjunto.
    const size_t & size() const { return num_nodes; }

    /// Retorna true si el conjunto está vacío.
    bool is_empty() const { return num_nodes == 0; }

    /// Calcula y retorna la longitud del camino interno del árbol
    /// binario de búsqueda.
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

    /// Retorna un elemento cualquiera del conjunto
    const Key & get_item() const { return get_root(); }

    /// Calcula y retorna la altura del árbol binario de búsqueda.
    size_t height() const { return computeHeightRec(tree.getRoot()); }

    /// Efectúa un recorrido prefijo sobre todas los nodos del árbol e
    /// invoca la operación <code>visitFct</code> en cada visita
    template <class Op>
    void for_each_in_preorder(void (*visitFct)(Node*, int, int))
    {
      Node * root = static_cast<Node*>(tree.getRoot());
      preOrderRec(root, visitFct);
    }

    /** Retorna la posición infija (ordenada) de la clave key.

        position(key) busca en el treap extendido la clave key (lo
        cual toma tiempo \f$O(\lg n)\f$) y retorna la posición
        infija del nodo que contiene la clave.

        @param[in] key clave a buscar y a determinar posición infija.
        @return posición infija de la clave key dentro del conjunto
        ordenado si la clave se encuentra; -1 de lo contrario

        @warning Sólo funciona si el árbol maneja rangos.
    */
    long position(const Key & key) const
    {
      std::pair<long, Node*> p = tree.position(key);
      return p.first;
    }

    /** Retorna el i-ésimo nodo en posición infija

        @param[in] i posición deseada
        @return referencia a la i-esima clave insertada en el conjunto
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

    /** Unión de dos árboles binarios de búsqueda.

        join_dup(t) construye un árbol binario de búsqueda
        correspondiente a la unión de this con t en la cual pueden haber
        claves duplicadas.

        @param[in] t árbol binario de búsqueda que se quiere unir a this.
        @note Luego de las llamadas el árbol t deviene vacíos y this deviene
        la unión de ambos árboles;
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

    /** Particiona el árbol binario de búsqueda según una clave.

        split_key(key,l,r) particiona, según la clave key, el
        árbol binario de búsqueda this en dos árboles l y r. Luego de la
        operación el árbol this deviene vacío, l contiene
        todas las claves menores que key y r las mayores.

        @param[in] key clave de partición.
        @param[out] l árbol con las claves menores que key.
        @param[out] r árbol con las claves mayores que key.
        @return true si key no está dentro del árbol binario; en cuyo
        caso la partición fue hecha exitosamente. De lo contrario, si
        key se encuentra dentro del árbol, el árbol no se particiona y el
        resultado es false.
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
