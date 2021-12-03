
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

# ifndef TPL_TREE_H
# define TPL_TREE_H

# include <stdexcept>
# include <dlink.H>
# include <ahDry.H>
# include <ahIterator.H>
# include <ahFunction.H>
# include <ahFunctional.H>
# include <htlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>
# include <tpl_binNode.H>

# define ISROOT(p)      ((p)->is_root())
# define ISLEAF(p)      ((p)->is_leaf())
# define ISLEFTMOST(p)  ((p)->is_leftmost())
# define ISRIGHTMOST(p) ((p)->is_rightmost())

# define SIBLING_LIST(p) ((p)->get_sibling_list())
# define CHILD_LIST(p)   ((p)->get_child_list())
# define SIBLING_LINK(p) ((p)->get_sibling_list())
# define LCHILD(p)       ((p)->get_left_child())
# define RSIBLING(p)     ((p)->get_right_sibling())
# define IS_UNIQUE_SIBLING(p) (RSIBLING(p) == (p))

namespace Aleph
{

  /** Árboles m-rios genéricos.

      La clase Tree_Node<Key> define árboles generales de cualquier
      orden representados mediante listas enlazadas.

      @param Key el tipo de dato que contiene cada nodo del árbol.

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
      Flags() noexcept
      : is_root(1), is_leaf(1), is_leftmost(1), is_rightmost(1) {}
    };

    Flags flags;

    LINKNAME_TO_TYPE(Tree_Node, child);
    LINKNAME_TO_TYPE(Tree_Node, sibling);

    Tree_Node * upper_link() const noexcept
    {
      return child_to_Tree_Node(child.get_prev());
    }

    Tree_Node * lower_link() const noexcept
    {
      return child_to_Tree_Node(child.get_next());
    }

    Tree_Node * left_link() const noexcept
    {
      return sibling_to_Tree_Node(sibling.get_prev());
    }

    Tree_Node * right_link() const noexcept
    {
      return sibling_to_Tree_Node(sibling.get_next());
    }

  public:

    using Item_Type = Tree_Node*;

    /// retorna referencia modificable al contenido del nodo.
    T & get_key() noexcept { return get_data(); }

    const T & get_key() const noexcept { return get_data(); }

    /// retorna referencia modificable al contenido del nodo.
    T & get_data() noexcept { return data; }

    const T & get_data() const noexcept { return data; }

    /// Tipo de dato genérico que contiene el nodo.
    using key_type = T;

    Dlink * get_child_list() noexcept { return &child; }

    Dlink * get_sibling_list() noexcept { return &sibling; }

    /// Retorna true si this es la raíz del árbol general.
    bool is_root() const noexcept { return flags.is_root; }

    /// Retorna true si this es un nodo hoja.
    bool is_leaf() const noexcept { return flags.is_leaf; }

    /// Retorna true si this es el nodo más a la izquierda de sus hermanos.
    bool is_leftmost() const noexcept { return flags.is_leftmost; }

    /// Retorna true si this es el nodo más a la derecha de sus hermanos.
    bool is_rightmost() const noexcept { return flags.is_rightmost; }

    void set_is_root(bool value) noexcept { flags.is_root = value; }

    void set_is_leaf(bool value) noexcept { flags.is_leaf = value; }

    void set_is_leftmost(bool value) noexcept { flags.is_leftmost = value; }

    void set_is_rightmost(bool value) noexcept { flags.is_rightmost = value; }

    /// Constructor vacío (clave indefinida).
    Tree_Node() noexcept { /* empty */ }

    /// Constructor con valor de dato __data
    Tree_Node(const T & __data)
      noexcept(std::is_nothrow_copy_constructible<T>::value)
      : data(__data) { /* empty */ }

    Tree_Node(T && __data)
      noexcept(std::is_nothrow_move_constructible<T>::value)
      : data(std::move(__data)) { /* empty */ }

    /// Retorna hermano izquierdo de this.
    Tree_Node * get_left_sibling() const noexcept
    {
      if (is_leftmost())
        return nullptr;

      return left_link();
    }

    /// Retorna hermano derecho de this.
    Tree_Node * get_right_sibling() const noexcept
    {
      if (is_rightmost())
        return nullptr;

      return right_link();
    }

    /// retorna el hijo más a la izquierda de this.
    Tree_Node * get_left_child() const noexcept
    {
      if (is_leaf())
        return nullptr;

      return lower_link();
    }

    /// retorna el hijo más a la derecha de this.
    Tree_Node * get_right_child() const noexcept
    {
      if (is_leaf())
        return nullptr;

      Tree_Node * left_child = lower_link();

      assert(ISLEFTMOST(left_child));

      return left_child->left_link();
    }

    /** Retorna el i-ésimo hijo de this.

        Retorna el i-ésimo hijo de this.

        @param[in] i ordinal del hijo al que se desea acceder.
        @return puntero al i-ésimo hijo de this; nullptr si no existe.
    */
    Tree_Node * get_child(const size_t i) const noexcept
    {
      Tree_Node * c = get_left_child();
      for (int j = 0; c != nullptr and j < i; ++j)
        c = c->get_right_sibling();

      return c;
    }

    /// Retorna el padre de this.
    Tree_Node * get_parent() const noexcept
    {
      if (is_root())
        return nullptr;

      Tree_Node *  p = const_cast<Tree_Node*>(this);
      while (not ISLEFTMOST(p)) // baje hasta el nodo más a la izquierda
        p = p->left_link();

      assert(not ISROOT(p));
      assert(not CHILD_LIST(p)->is_empty());

      return p->upper_link();
    }

    /** Inserta p como hermano derecho de this.

        Inserta p como hermano derecho de this.

        @param[in] p nodo a insertar como hermano derecho.
    */
    void insert_right_sibling(Tree_Node * p) noexcept
    {
      if (p == nullptr)
        return;

      assert(CHILD_LIST(p)->is_empty());
      assert(SIBLING_LIST(p)->is_empty());
      assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and
             p->is_leaf());

      p->set_is_root(false);
      p->set_is_leftmost(false);

      Tree_Node * old_next_node = get_right_sibling();
      if (old_next_node != nullptr)
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

    /** Inserta p como hermano izquierdo de this.

        Inserta p como hermano izquierdo de this.

        @param[in] p nodo a insertar como hermano izquierdo.
    */
    void insert_left_sibling(Tree_Node * p)
    {
      if (p == nullptr)
        return;

      if (this->is_root())
        throw std::domain_error("Cannot insert sibling of a root");

      assert(CHILD_LIST(p)->is_empty());
      assert(SIBLING_LIST(p)->is_empty());
      assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and
             p->is_leaf());

      p->set_is_root(false);
      p->set_is_rightmost(false);

      Tree_Node * old_prev_node = this->get_left_sibling();
      if (old_prev_node != nullptr)
        {
          assert(not this->is_leftmost());
          p->set_is_leftmost(false);
        }
      else
        { // this es más a la izq ==> p debe a ser primogénito
          assert(this->is_leftmost());

          Tree_Node * parent = this->get_parent();

          // Busca la raíz del árbol. Para ello buscamnos la hoja de this
          Tree_Node * leaf = this;
          while (not leaf->is_leaf())
            {
              leaf = leaf->get_left_child();
              assert(leaf != nullptr);
            }

          Tree_Node * root = leaf->lower_link();
          assert(root != nullptr);

          Dlink tree = CHILD_LIST(root)->cut_list(CHILD_LIST(this));
          tree.del();
          CHILD_LIST(parent)->insert(CHILD_LIST(p));
          p->set_is_leftmost(true);

          assert(p->get_parent() == parent);
        }

      this->set_is_leftmost(false);
      this->sibling.append(SIBLING_LIST(p));
    }

    /** Inserta p como el hijo más a la izquierda de this.

        Inserta p como el hijo más a la izquierda de this.

        @param[in] p nodo a insertar.
    */
    void insert_leftmost_child(Tree_Node * p) noexcept
    {
      if (p == nullptr)
        return;

      assert(CHILD_LIST(p)->is_empty());
      assert(SIBLING_LIST(p)->is_empty());
      assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and
             p->is_leaf());

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

    /** Inserta p como el hijo más a la derecho de this.

        Inserta p como el hijo más a la derecho de this.

        @param[in] p nodo a insertar.
    */
    void insert_rightmost_child(Tree_Node * p) noexcept
    {
      if (p == nullptr)
        return;

      assert(CHILD_LIST(p)->is_empty());
      assert(SIBLING_LIST(p)->is_empty());
      assert(p->is_rightmost() and p->is_leftmost() and p->is_root() and
             p->is_leaf());

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

    /// join `tree` as subtree of root `this`
    Tree_Node * join(Tree_Node * tree)
    {
      assert(this->is_root());
      assert(tree != nullptr);
      assert(tree->is_root() and tree->is_leftmost() and tree->is_rightmost());

      tree->set_is_root(false);

      if (this->is_leaf())
        {
          assert(CHILD_LIST(this)->is_empty() and
                 SIBLING_LIST(this)->is_empty());
          this->set_is_leaf(false);
          CHILD_LIST(this)->splice(CHILD_LIST(tree));
        }
      else
        {
          Tree_Node * right_child = this->lower_link()->left_link();
          right_child->set_is_rightmost(false);
          tree->set_is_leftmost(false);
          SIBLING_LINK(right_child)->splice(SIBLING_LINK(tree));
        }

      return this;
    }

    /** Insert `tree` to the right of `this`

        Assuming that `this` is part of a forrest, this method
        insert `tree` to the right of this.

        Be careful with the fact that `tree` will not always be
        inserted as the rightmost tree, but as the tree to right of
        `this`.

        @param[in] tree the tree to insert
        @throw domain_error if tree is not root
    */
    void insert_tree_to_right(Tree_Node * tree)
    {
      if (tree == nullptr)
        return;

      if (not this->is_root())
        throw std::domain_error("\"this\" is not root");

      tree->set_is_leftmost(false);
      Tree_Node * old_next_tree = this->get_right_tree();
      if (old_next_tree != nullptr)
        {
          assert(not this->is_rightmost());
          tree->set_is_rightmost(false);
        }

      this->set_is_rightmost(false);
      SIBLING_LIST(this)->insert(SIBLING_LIST(tree));
    }

    /// Retorna el árbol a la izquierda de this.
    Tree_Node * get_left_tree() const noexcept
    {
      if (is_leftmost())
        return nullptr;
      assert(not is_leftmost());
      return left_link();
    }

    /// Retorna el árbol a la derecha de this.
    Tree_Node * get_right_tree() const noexcept
    {
      if (is_rightmost())
        return nullptr;

      assert(not is_rightmost());
      return right_link();
    }

    /// Retorna el árbol más a la derecha de la arborescencia
    /// this. Dispara excepción range_error si this no es el árbol más a
    /// la izquierda de toda la arborescencia.
    Tree_Node * get_last_tree() const
    {
      if (not is_leftmost())
        throw std::range_error("\"this\" is not the leftmost tree in the forest");

      return left_link();
    }

    /// Return a list with all trees belonging to the forrest
    template <template <typename> class Container = DynList>
    Container<Tree_Node*> trees() const
    {
      Container<Tree_Node*> ret;
      for (auto t = const_cast<Tree_Node*>(this); t != nullptr;
           t = t->get_right_tree())
        ret.append(t);
      return ret;
    }

    /// Visita cada hijo de this y ejecuta la operación operation sobre el
    /// nodo hijo
    template <typename Operation>
    void for_each_child(Operation & op) const
    {
      for (Tree_Node * child = get_left_child(); child != nullptr;
           child = child->get_right_sibling())
        op(child);
    }

    template <typename Operation>
    void for_each_child(Operation && op = Operation()) const
    {
      for_each_child<Operation>(op);
    }

    /// Retorna una lista de los nodos hijos de this
    template <template <typename> class Container = DynList>
    Container<Tree_Node*> children_nodes() const
    {
      Container<Tree_Node*> ret_val;
      this->for_each_child([&ret_val] (Tree_Node * p) { ret_val.append(p); });
      return ret_val;
    }

    /// Retorna una lista de los contenidos de los hijos de this.
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

  private:

    template <class Operation> static
    bool preorder(const Tree_Node * root, Operation & op)
    {
      if (root == nullptr)
        return true;

      if (not op(root))
        return false;

      for (Tree_Node * child = root->get_left_child(); child != nullptr;
           child = child->get_right_sibling())
        if (not preorder(child, op))
          return false;

      return true;
    }

  public:

    /// Recorre en prefijo todos los nodos y ejecuta op
    template <class Operation>
    bool traverse(Operation op)
    {
      return preorder(this, op);
    }

    template <class Operation>
    bool traverse(Operation op) const
    {
      return const_cast<Tree_Node*>(this)->traverse(op);
    }

    template <class Op> bool level_traverse(Op op)
    {
      DynListQueue<Tree_Node*> q;
      q.put(this);
      while (not q.is_empty())
        {
          Tree_Node * p = q.get();
          if (not op(p))
            return false;
          p->for_each_child([&q] (auto cptr) { q.put(cptr); });
        }
      return false;
    }

    template <class Op> bool level_traverse(Op op) const
    {
      return const_cast<Tree_Node*>(this)->level_traverse(op);
    }

    Functional_Methods(Tree_Node*);

    /** Iterator sobre los hijos de this

     */
    class Children_Iterator
    {
      Tree_Node * curr = nullptr;

    public:

      Children_Iterator(const Tree_Node & p) noexcept
        : curr(p.get_left_child()) {}

      Children_Iterator(Tree_Node & p) noexcept
        : curr(p.get_left_child()) {}

      Children_Iterator(Tree_Node * p) noexcept
        : curr(p->get_left_child()) {}

      Children_Iterator(const Children_Iterator & it) noexcept
        : curr(const_cast<Children_Iterator&>(it).curr) {}

      bool has_curr() const noexcept { return curr != nullptr; }

      Tree_Node * get_curr_ne() const noexcept { return curr; }

      Tree_Node * get_curr() const
      {
        if (curr == nullptr)
          throw std::overflow_error("Children_Iterator::get_curr()");
        return get_curr_ne();
      }

      void next_ne() noexcept { curr = curr->get_right_sibling(); }

      void next()
      {
        if (curr == nullptr)
          throw std::overflow_error("Children_Iterator::next()");
        next_ne();
      }
    };

    Children_Iterator children_it() const
    {
      return Children_Iterator(*this);
    }

    struct Children_Set // truco para usar Pair_Iterator
    {
      Children_Set(const Tree_Node &&) {}
      Children_Set(const Tree_Node &) {}
      struct Iterator : public Children_Iterator
      {
        using Children_Iterator::Children_Iterator;
      };
    };

    class Iterator
    {
      Tree_Node * root = nullptr;
      Tree_Node * curr = nullptr;
      long pos = 0;
      DynListStack<Tree_Node*> s;

    public:

      using Item_Type = Tree_Node*;

      void swap(Iterator & it)
      {
        std::swap(root, it.root);
        std::swap(curr, it.curr);
        std::swap(pos, it.pos);
        s.swap(it.s);
      }

      Iterator(Tree_Node * __root = nullptr) noexcept
        : root(__root), curr(root)
      {
        // empty
      }

      Iterator(Tree_Node & root) : Iterator(&root) {}

      Iterator(const Iterator & it)
        : root(it.root), curr(it.curr), pos(it.pos), s(it.s)
      {
        // empty
      }

      Iterator(Iterator && it) { swap(it); }

      Iterator & operator = (const Iterator & it)
      {
        if (this == &it)
          return *this;

        root = it.root;
        curr = it.curr;
        pos = it.pos;
        s = it.s;
        return *this;
      }

      Iterator & operator = (Iterator && it)
      {
        swap(it);
        return *this;
      }

      void reset_first() noexcept
      {
        s.empty();
        curr = root;
      }

      bool has_curr() const noexcept { return curr != nullptr; }

      Tree_Node * get_curr_ne() const noexcept { return curr; }

      Tree_Node * get_curr() const
      {
        if (not has_curr())
          throw std::overflow_error("Iterator overflow");
        return get_curr_ne();
      }

      void next_ne() noexcept
      {
        ++pos;
        Tree_Node * lchild = curr->get_left_child();
        if (lchild == nullptr)
          {
            if (s.is_empty())
              curr = nullptr;
            else
              curr = s.pop();

            return;
          }

        for (auto p = curr->get_right_child(); p != lchild;
             p = p->get_left_sibling())
          s.push(p);

        curr = lchild;
      }

      void next()
      {
        if (not has_curr())
          throw std::overflow_error("Iterator overflow");
        next_ne();
      }

      void end()
      {
        curr = nullptr;
        s.empty();
        pos = -1;
      }

      /// Return the current position of iterator. Only valid if
      // has_curr() == true
      size_t get_pos() const { return pos; }
    };

    Iterator get_it() const
    {
      return Iterator(const_cast<Tree_Node*>(this));
    }

    STL_ALEPH_ITERATOR(Tree_Node);
  };

  template <typename T>
  struct Tree_Node_Vtl : public Tree_Node<T>
  {
    virtual ~Tree_Node_Vtl() {}
  };

  template <class Node> static inline
  void clone_tree(Node * src, Node * tgt)
  {
    using It = typename Node::Children_Iterator;
    for (It it(src); it.has_curr(); it.next_ne())
      tgt->insert_rightmost_child(new Node(it.get_curr_ne()->get_key()));

    using PItor = Pair_Iterator<It>;
    for (PItor itor{It(*src), It(*tgt)}; itor.has_curr(); itor.next_ne())
      {
        auto p = itor.get_curr_ne();
        clone_tree(p.first, p.second);
      }
  }

  template <class Node>
  Node * clone_tree(Node * root)
  {
    if (root == nullptr)
      return nullptr;
    Node * ret = new Node(root->get_key());
    clone_tree(root, ret);
    return ret;
  }

  template <class Node> static inline
  void __tree_preorder_traversal(Node * root, const int & level,
                                 const int & child_index,
                                 void (*visitFct)(Node *, int, int))
  {
    (*visitFct)(root, level, child_index);
    Node * child = root->get_left_child();
    for (int i = 0; child != nullptr; ++i, child = child->get_right_sibling())
      __tree_preorder_traversal(child, level + 1, i, visitFct);
  }

  /** Recorre en prefijo un árbol.

      tree_preorder_traversal((root,visit) realiza un recorrido prefijo
      sobre el árbol con raíz root. Si visitFct es especificado,
      entonces, por cada nodo visitado, se invoca la función.

      La función de vista tiene la siguiente especificación:

      void (*visitFct)(Node* p, int level, int pos)

      Donde:
      -# p: puntero al nodo actualmente visitado.
      -# level: el nivel de p en el árbol.
      -# child_index: índice de p dentro de sus hermanos.

      @param[in] root raíz del árbol a visitar.
      @param[in] visitFct puntero a la función de visita.
      @see forest_preorder_traversal() tree_postorder_traversal()
      @see forest_postorder_traversal()
      @throw domain_error si root no es un nodo raíz de un árbol.
      @ingroup Arboles
  */
  template <class Node> inline
  void tree_preorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {

    if (not root->is_root())
      throw std::domain_error("root is not root");

    __tree_preorder_traversal(root, 0, 0, visitFct);
  }

  /** Recorre en prefijo una arborescencia.

      forest_preorder_traversal((root,visit) realiza un recorrido prefijo
      sobre la arborescencia root. Si visitFct es especificado,
      entonces, por cada nodo visitado, se invoca la función.

      La función de vista tiene la siguiente especificación:

      void (*visitFct)(Node* p, int level, int pos)

      Donde:
      -# p: puntero al nodo actualmente visitado.
      -# level: el nivel de p en el árbol.
      -# child_index: índice de p dentro de sus hermanos.

      @param[in] root raíz del árbol primer árbol en la arborescencia.
      @param[in] visitFct puntero a la función de visita.
      @throw domain_error si root no es un nodo raíz de un árbol.
      @see tree_preorder_traversal() tree_postorder_traversal()
      @see forest_postorder_traversal()
      @ingroup Arboles
  */
  template <class Node> inline
  void forest_preorder_traversal(Node * root,
                                 void (*visitFct)(Node *, int, int))
  {
    if (not root->is_root())
      throw std::domain_error("root is not root");

    for (/* nada */; root != nullptr; root = root->get_right_tree())
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

    for (int i = 0; child not_eq nullptr;
         i++, child = child->get_right_sibling())
      __tree_postorder_traversal(child, level + 1, i, visitFct);

    (*visitFct)(node, level, child_index);
  }

  /** Recorre en sufijo un árbol.

      tree_postorder_traversal((root,visit) realiza un recorrido prefijo
      sobre el árbol con raíz root. Si visitFct es especificado,
      entonces, por cada nodo visitado, se invoca la función.

      La función de vista tiene la siguiente especificación:

      void (*visitFct)(Node* p, int level, int pos)

      Donde:
      -# p: puntero al nodo actualmente visitado.
      -# level: el nivel de p en el árbol.
      -# child_index: índice de p dentro de sus hermanos.

      @param[in] root raíz del árbol a visitar.
      @param[in] visitFct puntero a la función de visita.
      @see forest_preorder_traversal() tree_preorder_traversal()
      @see forest_postorder_traversal()
      @ingroup Arboles
  */
  template <class Node> inline
  void tree_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {
    __tree_postorder_traversal(root, 0, 0, visitFct);
  }

  /** Recorre en sufijo una arborescencia.

      forest_postorder_traversal((root,visit) realiza un recorrido sufijo
      sobre el árbol con raíz root. Si visitFct es especificado,
      entonces, por cada nodo visitado, se invoca la función.

      La función de vista tiene la siguiente especificación:

      void (*visitFct)(Node* p, int level, int pos)

      Donde:
      -# p: puntero al nodo actualmente visitado.
      -# level: el nivel de p en el árbol.
      -# child_index: índice de p dentro de sus hermanos.

      @param[in] root raíz del árbol a visitar.
      @param[in] visitFct puntero a la función de visita.
      @see forest_preorder_traversal() tree_preorder_traversal()
      @see tree_postorder_traversal()
      @throw domain_error si root no es nodo raíz del árbol más a la
      izquierda de la arborescencia.
      @ingroup Arboles
  */
  template <class Node> inline
  void forest_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {
    if (not root->is_leftmost())
      throw std::domain_error("root is not the leftmost node of forest");

    if (not root->is_root())
      throw std::domain_error("root is not root");

    for (/* nada */; root not_eq nullptr; root = root->get_right_sibling())
      {
        assert(root->is_root());
        __tree_postorder_traversal(root, 0, 0, visitFct);
      }
  }

  /** Retorna true si t1 es igual a t2

      @ingroup Arboles
  */
  template <class Node, class Eq>
  inline bool are_tree_equal(Node * t1, Node * t2, Eq & eq)
    noexcept(noexcept(eq(t1->get_key(), t2->get_key())))
  {
    if (t1 == nullptr)
      return t2 == nullptr;

    if (t2 == nullptr)
      return false;

    if (not eq(t1->get_key(), t2->get_key()))
      return false;

    try
      {
        return zipEq(t1->children_nodes(), t2->children_nodes()).all([] (auto p)
          {
            return are_tree_equal(p.first, p.second);
          });
      }
    catch (std::length_error)
      {
        return false;
      }
  }

  template <class Node,
            class Eq = std::equal_to<typename Node::key_type>>
  inline bool are_tree_equal(Node * t1, Node * t2, Eq && eq = Eq())
    noexcept(noexcept(are_tree_equal<Node, Eq>(t1, t2, eq)))
  {
    return are_tree_equal<Node, Eq>(t1, t2, eq);
  }

  /** Destruye (libera memoria) el árbol cuya raíz es root.

      destroy_tree(root) libera toda la memoria ocupada por el
      árbol cuya raíz es root.

      @param[in] root raíz del árbol que se desea liberar.
      @ingroup Arboles
  */
  template <class Node> inline
  void destroy_tree(Node * root)
  {
    if (root == nullptr)
      return;

    if (not IS_UNIQUE_SIBLING(root))
      SIBLING_LIST(root)->del(); // no ==> sacarlo de lista hermanos

    // recorrer los subárboles de derecha a izquierda
    for (Node * p = (Node*) root->get_right_child(); p != nullptr; /* nada */)
      {
        Node * to_delete = p;      // respaldar subárbol a borrar p
        p = (Node*) p->get_left_sibling(); // Avanzar p a hermano izquierdo
        destroy_tree(to_delete);   // eliminar recursivamente árbol
      }

    if (root->is_leftmost()) // ¿sacar lista hijos?
      CHILD_LIST(root)->del();

    delete root;
  }

  /** Destruye (libera memoria) la arborescencia cuya primer
      árbol es root.

      destroy_forest(root) libera toda la memoria ocupada por el
      la arborescencia cuyo primer árbol tiene raíz root.

      @param[in] root raíz del primer árbol de la arborescencia
      que se desea destruir.
      @throw domain_error si root no es nodo raíz del árbol más a la
      izquierda de la arborescencia.
      @ingroup Arboles
  */
  template <class Node> inline
  void destroy_forest(Node * root)
  {
    if (root == nullptr)
      return;

    if (not root->is_leftmost())
      throw std::domain_error("root is not the leftmost tree of forest");

    if (not root->is_root())
      throw std::domain_error("root is not root");

    while (root != nullptr) // recorre los árboles de izquierda a derecha
      {
        Node * to_delete = root;          // respalda raíz
        root = root->get_right_sibling(); // avanza a siguiente árbol
        SIBLING_LIST(to_delete)->del();   // elimine de lista árboles
        destroy_tree(to_delete);          // Borre el árbol
      }
  }

  /** Calcula la altura del árbol root.

      @param[in] root raíz del árbol.
      @return altura del árbol en raíz root.
      @ingroup Arboles
  */
  template <class Node>
  size_t compute_height(Node * root)
  {
    if (root == nullptr)
      return 0;

    size_t temp_h, max_h = 0;
    for (Node * aux = root->get_left_child(); aux != nullptr;
         aux = aux->get_right_sibling())
      if ((temp_h = compute_height(aux)) > max_h)
        max_h = temp_h;

    return max_h + 1;
  }

  template <class Node> static inline
  Node * __deway_search(Node * node, int path [],
                        const int & idx, const size_t & size)
  {
    if (node == nullptr)
      return nullptr;

    if (idx > size)
      throw std::out_of_range("index out of maximum range");

    if (path[idx] < 0) // verifique si se ha alcanzado el nodo
      return node;
    // avance hasta el próximo hijo path[0]
    Node * child = node->get_left_child();
    for (int i = 0; i < path[idx] and child != nullptr; ++i)
      child = child->get_right_sibling();

    return __deway_search(child, path, idx + 1, size); // próximo nivel
  }

  /** Retorna un nodo de una arborescencia dado su número de Deway.

      deway_search(root,path,size) toma el número de Deway guardado
      en path, de longitud size y busca en la arborescencia cuyo primer
      árbol es root un nodo que se corresponda con el número de Deway dado.

      @param[in] root raíz del primer árbol de la arborescencia.
      @param[in] path arreglo que contiene el número de Deway.
      @param[in] size tamaño del número de deway.
      @return puntero al nodo correspondiente al número de Deway
      dado; nullptr si no existe,
      @ingroup Arboles
  */
  template <class Node> inline
  Node * deway_search(Node * root, int path [], const size_t & size)
  {
    for (int i = 0; root != nullptr; i++, root = root->get_right_sibling())
      if (path[0] == i)
        return __deway_search(root, path, 1, size);

    return nullptr;
  }

  template <class Node, class Equal> inline static
  Node * __search_deway(Node * root, const typename Node::key_type & key,
                        const size_t & current_level, int deway [],
                        const size_t & size, size_t & n);

  /** Busca key en arborescencia y calcula el número de Deway del
      nodo contentivo de la clave key.

      search_deway(root,key,deway,n) busca en la arborescencia
      cuyo primer árbol es root un nodo que contenga la clave key.
      Si el nodo es encontrado, entonces la rutina guarda en deway[]
      el número de Deway del nodo encontrado.

      La búsqueda se realiza con criterio de igualdad Equal()().

      @param[in] root raíz del primer árbol de la arborescencia.
      @param[in] key clave a buscar
      @param[out] deway arreglo que contiene el número de Deway.
      @param[in] size tamaño máximo del número de deway.
      @param[out] n tamaño del número de Deway calculado (si se
      encuentra el nodo).
      @return puntero al nodo contentivo de la clave key;
      nullptr si no existe ningún nodo con clave key,
      @throw overflow_error si size no es suficiente para almacenar la
      secuencia de Deway.
      @ingroup Arboles
  */
  template <class Node,
            class Equal = Aleph::equal_to<typename Node::key_type> > inline
  Node * search_deway(Node * root, const typename Node::key_type & key,
                      int deway [], const size_t & size, size_t & n)
  {
    n = 1; // valor inicial de longitud de número de Deway

    if (size < n)
      throw std::overflow_error("there is no enough space for deway array");

    for (int i = 0; root != nullptr; i++, root = root->get_right_sibling())
      {
        deway[0] = i;
        Node * result =
          __search_deway <Node, Equal> (root, key, 0, deway, size, n);
        if (result != nullptr)
          return result;
      }

    return nullptr;
  }

  template <class Node, class Equal> inline static
  Node * __search_deway(Node * root,
                        const typename Node::key_type & key,
                        const size_t & current_level, int deway [],
                        const size_t & size, size_t & n)
  {

    if (current_level >= size)
      throw std::overflow_error("there is no enough space for deway array");

    if (root == nullptr)
      return nullptr;

    if (Equal()(root->get_key(), key))
      {
        n = current_level + 1; // longitud del arreglo deway
        return root;
      }

    Node * child = root->get_left_child();
    for (int i = 0; child != nullptr;
         i++, child = child->get_right_sibling())
      {
        deway[current_level + 1] = i;
        Node * result = __search_deway <Node, Equal>
          (child, key, current_level + 1, deway, size, n);

        if (result!= nullptr)
          return result;
      }

    return nullptr;
  }

  /** Convierte una arborescencia a su árbol binario equivalente.

      forest_to_bin(root) toma una arborescencia derivada de Tree_Node
      y lo convierte a su equivalente árbol binario.

      La rutina maneja dos parámetros tipo:
      -# TNode: tipo de árbol basado en Tree_Node.
      -# BNode: tipo de árbol binario basado en BinNode.

      El procedimiento asume que ambos tipos comparten el mismo
      tipo de clave.

      @param[in] root raíz del primer árbol perteneciente a
      la arborescencia a convertir.
      @return raíz del árbol binario equivalente a la arborescencia
      dada.
      @throw bad_alloc si no hay suficiente memoria.
      @see bin_to_forest()
      @ingroup Arboles
  */
  template <class TNode, class BNode>
  BNode * forest_to_bin(TNode * root)
  {
    if (root == nullptr)
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

  /** Convierte un árbol binario a su arborescencia equivalente.

      bin_to_forest(root) toma un árbol binario derivado de BinNode
      y lo convierte a su arborescencia equivalente.

      La rutina maneja dos parámetros tipo:
      -# TNode: tipo de árbol basado en Tree_Node.
      -# BNode: tipo de árbol binario basado en BinNode.

      El procedimiento asume que ambos tipos comparten el mismo
      tipo de clave.

      @param[in] broot raíz del árbol binario a convertir.
      @return raíz del primer árbol equivalente a árbol binario
      dado.
      @throw bad_alloc si no hay suficiente memoria.
      @see forest_to_bin()
      @ingroup Arboles
  */
  template <class TNode, class BNode> inline
  TNode * bin_to_forest(BNode * broot)
  {
    if (broot == BNode::NullPtr)
      return nullptr;

    TNode * troot = new TNode (KEY(broot));
    bin_to_tree(broot, troot);
    return troot;
  }

} // fin namespace Aleph

# endif // TPL_TREE_H

