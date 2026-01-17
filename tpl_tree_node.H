
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file tpl_tree_node.H
 *  @brief General tree (n-ary tree) node.
 *
 *  Node for general trees with arbitrary number of children.
 *  Uses first-child/next-sibling representation.
 *
 *  @see tpl_binNodeUtils.H Binary tree utilities
 *  @ingroup Trees
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TREE_H
# define TPL_TREE_H

# include <stdexcept>
# include <dlink.H>
# include <ahDry.H>
# include <ah-dry-mixin.H>
# include <ahIterator.H>
# include <ahFunction.H>
# include <ahFunctional.H>
# include <htlist.H>
# include <tpl_dynListStack.H>
# include <tpl_dynListQueue.H>
# include <tpl_binNode.H>
# include <ah-errors.H>

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

  /** Generic m-ary trees.

      The Tree_Node<Key> class defines general trees of any order,
      represented by linked lists.

      @param Key the data type stored in each tree node.

      @ingroup Trees
  */
  template <class T>
  class Tree_Node;  // Forward declaration for CRTP

  template <class T>
  class Tree_Node : public FunctionalMixin<Tree_Node<T>, Tree_Node<T>*>
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

    /// Returns a modifiable reference to the node contents.
    T & get_key() noexcept { return get_data(); }

    [[nodiscard]] constexpr const T & get_key() const noexcept { return get_data(); }

    /// Returns a modifiable reference to the node contents.
    T & get_data() noexcept { return data; }

    [[nodiscard]] constexpr const T & get_data() const noexcept { return data; }

    /// Generic data type stored in the node.
    using key_type = T;

    Dlink * get_child_list() noexcept { return &child; }

    Dlink * get_sibling_list() noexcept { return &sibling; }

    /// Returns true if this is the root of the general tree.
    [[nodiscard]] constexpr bool is_root() const noexcept { return flags.is_root; }

    /// Returns true if this is a leaf node.
    [[nodiscard]] constexpr bool is_leaf() const noexcept { return flags.is_leaf; }

    /// Returns true if this is the leftmost node among its siblings.
    [[nodiscard]] constexpr bool is_leftmost() const noexcept { return flags.is_leftmost; }

    /// Returns true if this is the rightmost node among its siblings.
    [[nodiscard]] constexpr bool is_rightmost() const noexcept { return flags.is_rightmost; }

    void set_is_root(bool value) noexcept { flags.is_root = value; }

    void set_is_leaf(bool value) noexcept { flags.is_leaf = value; }

    void set_is_leftmost(bool value) noexcept { flags.is_leftmost = value; }

    void set_is_rightmost(bool value) noexcept { flags.is_rightmost = value; }

    /// Empty constructor (undefined key).
    Tree_Node() = default;

    /// Constructor with data value __data
    Tree_Node(const T & d)
      : data(d) { /* empty */ }

    Tree_Node(T && d)
      : data(std::move(d)) { /* empty */ }

    /// Returns the left sibling of this.
    Tree_Node * get_left_sibling() const noexcept
    {
      if (is_leftmost())
        return nullptr;

      return left_link();
    }

    /// Returns the right sibling of this.
    Tree_Node * get_right_sibling() const noexcept
    {
      if (is_rightmost())
        return nullptr;

      return right_link();
    }

    /// Returns the leftmost child of this.
    Tree_Node * get_left_child() const noexcept
    {
      if (is_leaf())
        return nullptr;

      return lower_link();
    }

    /// Returns the rightmost child of this.
    Tree_Node * get_right_child() const noexcept
    {
      if (is_leaf())
        return nullptr;

      const Tree_Node * left_child = lower_link();

      assert(ISLEFTMOST(left_child));

      return left_child->left_link();
    }

    /** Returns the i-th child of this.

        Returns the i-th child of this.

        @param[in] i ordinal of the child to access.
        @return pointer to the i-th child of this; nullptr if it does not exist.
    */
    Tree_Node * get_child(const size_t i) const noexcept
    {
      Tree_Node * c = get_left_child();
      for (size_t j = 0; c != nullptr and j < i; ++j)
        c = c->get_right_sibling();

      return c;
    }

    /// Returns the parent of this.
    Tree_Node * get_parent() const noexcept
    {
      if (is_root())
        return nullptr;

      auto *  p = const_cast<Tree_Node*>(this);
      while (not ISLEFTMOST(p)) // go down to the leftmost node
        p = p->left_link();

      assert(not ISROOT(p));
      assert(not CHILD_LIST(p)->is_empty());

      return p->upper_link();
    }

    /** Inserts p as the right sibling of this.

        Inserts p as the right sibling of this.

        @param[in] p node to insert as right sibling.
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

    /** Inserts p as the left sibling of this.

        Inserts p as the left sibling of this.

        @param[in] p node to insert as left sibling.
    */
    void insert_left_sibling(Tree_Node * p)
    {
      if (p == nullptr)
        return;

      ah_domain_error_if(this->is_root())
        << "Cannot insert sibling of a root";

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
        { // this is the leftmost ==> p must become the first child
          assert(this->is_leftmost());

          Tree_Node * parent = this->get_parent();

          // Find the tree root. To do so, we look for the leaf of this
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

    /** Inserts p as the leftmost child of this.

        @param[in] p node to insert.
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

    /** Inserts p as the rightmost child of this.

        @param[in] p node to insert.
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

      ah_domain_error_if(not this->is_root())
        << "\"this\" is not root";

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

    /// Returns the tree to the left of this.
    Tree_Node * get_left_tree() const noexcept
    {
      if (is_leftmost())
        return nullptr;
      assert(not is_leftmost());
      return left_link();
    }

    /// Returns the tree to the right of this.
    Tree_Node * get_right_tree() const noexcept
    {
      if (is_rightmost())
        return nullptr;

      assert(not is_rightmost());
      return right_link();
    }

    /// Returns the rightmost tree of the forest containing this.
    /// Throws range_error if this is not the leftmost tree of the forest.
    Tree_Node * get_last_tree() const
    {
      ah_range_error_if(not is_leftmost())
        << "\"this\" is not the leftmost tree in the forest";

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

    /// Visits each child of this and executes the operation on the child node.
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

    /// Returns a list with the child nodes of this.
    template <template <typename> class Container = DynList>
    Container<Tree_Node*> children_nodes() const
    {
      Container<Tree_Node*> ret_val;
      this->for_each_child([&ret_val] (Tree_Node * p) { ret_val.append(p); });
      return ret_val;
    }

    /// Returns a list with the contents of the children of this.
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

    /// Preorder traversal over all nodes executing op.
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
      return true;
    }

    template <class Op> bool level_traverse(Op op) const
    {
      return const_cast<Tree_Node*>(this)->level_traverse(op);
    }

    // Note: for_each(), all(), exists(), maps(), filter(), foldl(), 
    // fold(), partition(), take(), drop(), rev(), length() are now 
    // provided by FunctionalMixin<Tree_Node<T>, Tree_Node<T>*>

    /** Iterator over the children of this

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
        ah_overflow_error_if(curr == nullptr) << "Children_Iterator::get_curr()";
        return get_curr_ne();
      }

      void next_ne() noexcept { curr = curr->get_right_sibling(); }

      void next()
      {
        ah_overflow_error_if(curr == nullptr) << "Children_Iterator::next()";
        next_ne();
      }
    };

    Children_Iterator children_it() const
    {
      return Children_Iterator(*this);
    }

    struct Children_Set // trick to use Pair_Iterator
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

      void swap(Iterator & it) noexcept
      {
        std::swap(root, it.root);
        std::swap(curr, it.curr);
        std::swap(pos, it.pos);
        s.swap(it.s);
      }

      Iterator(Tree_Node * r = nullptr) noexcept
        : root(r), curr(root)
      {
        // empty
      }

      Iterator(Tree_Node & root) : Iterator(&root) {}

      Iterator(const Iterator & it)
        : root(it.root), curr(it.curr), pos(it.pos), s(it.s)
      {
        // empty
      }

      Iterator(Iterator && it) noexcept
        : root(nullptr), curr(nullptr), pos(0), s()
      {
        swap(it);
      }

      Iterator & operator = (Iterator it)
      {
        it.swap(*this);
        return *this;
      }

      void reset_first() noexcept
      {
        s.empty();
        pos = 0;
        curr = root;
      }

      bool has_curr() const noexcept { return curr != nullptr; }

      Tree_Node * get_curr_ne() const noexcept { return curr; }

      Tree_Node * get_curr() const
      {
        ah_overflow_error_if(not has_curr()) << "Iterator overflow";
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
        ah_overflow_error_if(not has_curr()) << "Iterator overflow";
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
    virtual ~Tree_Node_Vtl() = default;
  };

  template <class Node> static inline
  void clone_tree(Node * src, Node * tgt)
  {
    using It = typename Node::Children_Iterator;
    for (It it(src); it.has_curr(); it.next_ne())
      tgt->insert_rightmost_child(new Node(it.get_curr()->get_key()));

    using PItor = Pair_Iterator<It>;
    for (PItor itor{It(*src), It(*tgt)}; itor.has_curr(); itor.next_ne())
      {
        auto p = itor.get_curr();
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

  /** Preorder traversal of a tree.

      tree_preorder_traversal((root,visit) performs a preorder traversal
      over the tree rooted at root. If visitFct is specified,
      then for each visited node the function is invoked.

      The visit function has the following specification:

      void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# p: pointer to the currently visited node.
      -# level: the level of p in the tree.
      -# child_index: index of p among its siblings.

      @param[in] root root of the tree to traverse.
      @param[in] visitFct pointer to the visit function.
      @see forest_preorder_traversal() tree_postorder_traversal()
      @see forest_postorder_traversal()
      @throw domain_error if root is not a root node of a tree.
      @ingroup Trees
  */
  template <class Node> inline
  void tree_preorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {

    ah_domain_error_if(not root->is_root())
      << "root is not root";

    __tree_preorder_traversal(root, 0, 0, visitFct);
  }

  /** Preorder traversal of a forest.

      forest_preorder_traversal((root,visit) performs a preorder traversal
      over the forest whose first tree is root. If visitFct is specified,
      then for each visited node the function is invoked.

      The visit function has the following specification:

      void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# p: pointer to the currently visited node.
      -# level: the level of p in the tree.
      -# child_index: index of p among its siblings.

      @param[in] root root of the first tree in the forest.
      @param[in] visitFct pointer to the visit function.
      @throw domain_error if root is not a root node of a tree.
      @see tree_preorder_traversal() tree_postorder_traversal()
      @see forest_postorder_traversal()
      @ingroup Trees
  */
  template <class Node> inline
  void forest_preorder_traversal(Node * root,
                                 void (*visitFct)(Node *, int, int))
  {
    ah_domain_error_if(not root->is_root()) << "root is not root";

    for (/* nothing */; root != nullptr; root = root->get_right_tree())
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

  /** Postorder traversal of a tree.

      tree_postorder_traversal((root,visit) performs a postorder traversal
      over the tree rooted at root. If visitFct is specified,
      then for each visited node the function is invoked.

      The visit function has the following specification:

      void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# p: pointer to the currently visited node.
      -# level: the level of p in the tree.
      -# child_index: index of p among its siblings.

      @param[in] root root of the tree to traverse.
      @param[in] visitFct pointer to the visit function.
      @see forest_preorder_traversal() tree_preorder_traversal()
      @see forest_postorder_traversal()
      @ingroup Trees
  */
  template <class Node> inline
  void tree_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {
    __tree_postorder_traversal(root, 0, 0, visitFct);
  }

  /** Postorder traversal of a forest.

      forest_postorder_traversal((root,visit) performs a postorder traversal
      over the forest whose first tree is root. If visitFct is specified,
      then for each visited node the function is invoked.

      The visit function has the following specification:

      void (*visitFct)(Node* p, int level, int pos)

      Where:
      -# p: pointer to the currently visited node.
      -# level: the level of p in the tree.
      -# child_index: index of p among its siblings.

      @param[in] root root of the tree to traverse.
      @param[in] visitFct pointer to the visit function.
      @see forest_preorder_traversal() tree_preorder_traversal()
      @see tree_postorder_traversal()
      @throw domain_error if root is not the root node of the leftmost tree
      of the forest.
      @ingroup Trees
  */
  template <class Node> inline
  void forest_postorder_traversal(Node * root, void (*visitFct)(Node *, int, int))
  {
    ah_domain_error_if(not root->is_leftmost()) << "root is not the leftmost node of forest";

    ah_domain_error_if(not root->is_root()) << "root is not root";

    for (/* nothing */; root not_eq nullptr; root = root->get_right_sibling())
      {
        assert(root->is_root());
        __tree_postorder_traversal(root, 0, 0, visitFct);
      }
  }

  /** Returns true if t1 is equal to t2

      @ingroup Trees
  */
  template <class Node, class Eq>
  inline bool are_tree_equal(Node * t1, Node * t2, Eq & eq)
  {
    if (t1 == nullptr)
      return t2 == nullptr;

    if (t2 == nullptr)
      return false;

    if (not eq(t1->get_key(), t2->get_key()))
      return false;

    try
      {
        return zipEq(t1->children_nodes(), t2->children_nodes()).
          all([&eq] (auto p)
              {
                return are_tree_equal(p.first, p.second, eq);
              });
      }
    catch (const std::length_error &)
      {
        return false;
      }
  }

  template <class Node,
            class Eq = std::equal_to<typename Node::key_type>>
  inline bool are_tree_equal(Node * t1, Node * t2, Eq && eq = Eq())
  {
    return are_tree_equal<Node, Eq>(t1, t2, eq);
  }

  /** Destroys (frees memory) the tree whose root is root.

      destroy_tree(root) frees all the memory occupied by the
      tree whose root is root.

      @param[in] root root of the tree to be freed.
      @ingroup Trees
  */
  template <class Node> inline
  void destroy_tree(Node * root)
  {
    if (root == nullptr)
      return;

    if (not IS_UNIQUE_SIBLING(root))
      SIBLING_LIST(root)->del(); // no ==> remove from sibling list

    // traverse subtrees from right to left
    for (Node * p = static_cast<Node *>(root->get_right_child()); p != nullptr; /* nada */)
      {
        Node * to_delete = p;      // backup subtree to delete
        p = static_cast<Node *>(p->get_left_sibling()); // advance to left sibling
        destroy_tree(to_delete);   // recursively delete tree
      }

    if (root->is_leftmost()) // remove children list?
      CHILD_LIST(root)->del();

    delete root;
  }

  /** Destroys (frees memory) the forest whose first tree is root.

      destroy_forest(root) frees all the memory occupied by the
      forest whose first tree has root as its root.

      @param[in] root root of the first tree of the forest
      to be destroyed.
      @throw domain_error if root is not the root node of the leftmost
      tree of the forest.
      @ingroup Trees
  */
  template <class Node> inline
  void destroy_forest(Node * root)
  {
    if (root == nullptr)
      return;

    ah_domain_error_if(not root->is_leftmost()) << "root is not the leftmost tree of forest";

    ah_domain_error_if(not root->is_root()) << "root is not root";

    while (root != nullptr) // traverse trees from left to right
      {
        Node * to_delete = root;          // backup root
        root = (Node*) root->get_right_sibling(); // advance to next tree
        SIBLING_LIST(to_delete)->del();   // remove from tree list
        destroy_tree(to_delete);          // delete the tree
      }
  }

  /** Computes the height of the tree root.

      @param[in] root tree root.
      @return height of the tree rooted at root.
      @ingroup Trees
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

    ah_out_of_range_error_if(static_cast<size_t>(idx) >= size)
      << "index out of maximum range";

    if (path[idx] < 0) // check whether the node has been reached
      return node;
    // advance to the next child path[0]
    Node * child = node->get_left_child();
    for (int i = 0; i < path[idx] and child != nullptr; ++i)
      child = child->get_right_sibling();

    return __deway_search(child, path, idx + 1, size); // next level
  }

  /** Returns a node of a forest given its Dewey number.

      deway_search(root,path,size) takes the Dewey number stored
      in path, of length size, and searches in the forest whose first
      tree is root for the node that corresponds to the given Dewey number.

      @param[in] root root of the first tree of the forest.
      @param[in] path array containing the Dewey number.
      @param[in] size length of the Dewey number.
      @return pointer to the node corresponding to the given Dewey number;
      nullptr if it does not exist,
      @ingroup Trees
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

  /** Searches key in a forest and computes the Dewey number of the
      node containing the key.

      search_deway(root,key,deway,n) searches in the forest
      whose first tree is root a node containing the key.
      If the node is found, then the routine stores in deway[]
      the Dewey number of the found node.

      The search is performed using the equality criterion Equal()().

      @param[in] root root of the first tree of the forest.
      @param[in] key key to search.
      @param[out] deway array that stores the Dewey number.
      @param[in] size maximum length of the Dewey number.
      @param[out] n length of the computed Dewey number (if the node is found).
      @return pointer to the node containing key;
      nullptr if there is no node with key,
      @throw overflow_error if size is not sufficient to store the Dewey sequence.
      @ingroup Trees
  */
  template <class Node,
            class Equal = Aleph::equal_to<typename Node::key_type> > inline
  Node * search_deway(Node * root, const typename Node::key_type & key,
                      int deway [], const size_t & size, size_t & n)
  {
    n = 1; // initial length value of the Dewey number

    ah_overflow_error_if(size < n) << "there is no enough space for deway array";

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

    ah_overflow_error_if(current_level >= size) << "there is no enough space for deway array";

    if (root == nullptr)
      return nullptr;

    if (Equal()(root->get_key(), key))
      {
        n = current_level + 1; // length of deway array
        return root;
      }

    Node * child = root->get_left_child();
    for (int i = 0; child != nullptr;
         i++, child = child->get_right_sibling())
      {
        ah_overflow_error_if(current_level + 1 >= size)
          << "there is no enough space for deway array";
        deway[current_level + 1] = i;
        Node * result = __search_deway <Node, Equal>
          (child, key, current_level + 1, deway, size, n);

        if (result!= nullptr)
          return result;
      }

    return nullptr;
  }

  /** Converts a forest to its equivalent binary tree.

      forest_to_bin(root) takes a forest derived from Tree_Node
      and converts it to its equivalent binary tree.

      The routine takes two type parameters:
      -# TNode: tree type based on Tree_Node.
      -# BNode: binary tree type based on BinNode.

      The procedure assumes that both types share the same key type.

      @param[in] root root of the first tree belonging to
      the forest to convert.
      @return root of the binary tree equivalent to the given forest.
      @throw bad_alloc if there is not enough memory.
      @see bin_to_forest()
      @ingroup Trees
  */
  template <class TNode, class BNode>
  BNode * forest_to_bin(TNode * root)
  {
    if (root == nullptr)
      return BNode::NullPtr;

    auto * result = new BNode (root->get_key());
    LLINK(result) = static_cast<BNode *>(forest_to_bin<TNode, BNode>(root->get_left_child()));
    RLINK(result) = forest_to_bin<TNode, BNode>(root->get_right_sibling());

    return result;
  }

  template <class TNode, class BNode> inline static
  void insert_child(BNode * lnode, TNode * tree_node)
  {
    if (lnode == BNode::NullPtr)
      return;

    auto * child = new TNode(KEY(lnode));
    tree_node->insert_leftmost_child(child);
  }

  template <class TNode, class BNode> inline static
  void insert_sibling(BNode * rnode, TNode * tree_node)
  {
    if (rnode == BNode::NullPtr)
      return;

    auto * sibling = new TNode(KEY(rnode));
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

  /** Converts a binary tree to its equivalent forest.

      bin_to_forest(root) takes a binary tree derived from BinNode
      and converts it to its equivalent forest.

      The routine takes two type parameters:
      -# TNode: tree type based on Tree_Node.
      -# BNode: binary tree type based on BinNode.

      The procedure assumes that both types share the same key type.

      @param[in] broot root of the binary tree to convert.
      @return root of the first tree equivalent to the given binary tree.
      @throw bad_alloc if there is not enough memory.
      @see forest_to_bin()
      @ingroup Trees
  */
  template <class TNode, class BNode> inline
  TNode * bin_to_forest(BNode * broot)
  {
    if (broot == BNode::NullPtr)
      return nullptr;

    auto * troot = new TNode (KEY(broot));
    bin_to_tree(broot, troot);
    return troot;
  }

} // end namespace Aleph

# endif // TPL_TREE_H

