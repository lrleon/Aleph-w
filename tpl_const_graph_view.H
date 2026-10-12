/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_const_graph_view.H
 *  @brief Strict read-only access to a graph: `Const_Graph_View` and the
 *  read-only iterators on the nodes, on the arcs and on the arcs of a node.
 *
 *  The `const` of a graph of Aleph-w protects its *topology*: the members
 *  that insert or remove nodes and arcs are not available through it. Its
 *  iterators and its traversals still hand out `Node *` and `Arc *`, because
 *  the algorithms that receive a `const GT &` keep their bookkeeping (control
 *  bits, counters and cookies) in the nodes and in the arcs. As a
 *  consequence, the information stored in a node or in an arc can be written
 *  through a `const GT &`:
 *
 *  @code
 *  void reader(const Graph & g)
 *  {
 *    g.for_each_arc([] (Graph::Arc * a) { a->get_info() = 0; });  // compiles
 *  }
 *  @endcode
 *
 *  That meaning is kept. What this header adds is the strict counterpart, for
 *  the code that must only read:
 *
 *  - `Const_Node_Iterator<GT>`, `Const_Arc_Iterator<GT>` and
 *    `Const_Node_Arc_Iterator<GT>` wrap the iterators of the graph `GT` and
 *    hand out `const Node *` and `const Arc *`.
 *  - `Const_Graph_View<GT>` is a non-owning view of a graph whose whole
 *    interface is expressed with those pointers to const.
 *
 *  @code
 *  void reader(Const_Graph_View<Graph> g)
 *  {
 *    g.for_each_arc([] (const Graph::Arc * a) { use(a->get_info()); });
 *    // g.for_each_arc([] (Graph::Arc * a) { ... });  does not compile
 *  }
 *
 *  reader(const_view(graph));
 *  @endcode
 *
 *  Through a pointer to const neither the information (`get_info()` is a
 *  reference to const) nor the bookkeeping (`NODE_BITS`, `NODE_COUNTER`,
 *  `NODE_COOKIE` and their arc counterparts are not assignable) can be
 *  written. An algorithm that needs the bookkeeping does not compile on a
 *  view; it runs on the graph itself, which its owner keeps.
 *
 *  The view satisfies `AlephGraph`, with `Node` and `Arc` being the constant
 *  node and arc types. So the generic code written in terms of
 *  `typename GT::Node *` and `typename GT::Arc *` reads a view unchanged:
 *  the filtered iterators `Node_Iterator<GT, SN>`, `Arc_Iterator<GT, SA>`
 *  and `Node_Arc_Iterator<GT, SA>`, the free functions `for_each_node()`,
 *  `for_each_arc()`, `forall_node()`..., and the concepts `NodeFilter`,
 *  `ArcFilter` and `ArcDistance`, which then require operations on pointers
 *  to const. The algorithms that keep no bookkeeping in the graph, such as
 *  `most_negative_cycle_bounded()` and `find_disjoint_negative_cycles()`, run
 *  on a view too. They need a distance functor that takes a `const Arc *`:
 *  the default one, `Dft_Dist`, hands out a modifiable reference to the
 *  information of the arc and does not compile for a view.
 *
 *  @code
 *  struct Cost
 *  {
 *    using Distance_Type = long;
 *    long operator () (const Graph::Arc * a) const { return a->get_info(); }
 *  };
 *
 *  auto cycle = most_negative_cycle_bounded(const_view(graph), 4, Cost());
 *  // cycle.cycle_arcs is a DynList<const Graph::Arc *>
 *  @endcode
 *
 *  ## Cost
 *
 *  A view is one pointer and its iterators are the iterators of the graph:
 *  nothing is copied and nothing is allocated per node or per arc. Only the
 *  members that build a list (`nodes()`, `arcs()`, `filter_nodes()`,
 *  `nodes_map()`...) allocate, as their counterparts of the graph do.
 *
 *  ## Lifetime and invalidation
 *
 *  A view does not own the graph: it is valid while the graph is alive and
 *  stays at the same address. It is not a snapshot: it shows the changes that
 *  the owner makes to the graph. An iterator obtained from a view is
 *  invalidated exactly when the iterator of the graph that it wraps is.
 *
 *  ## What a view does not protect
 *
 *  A view constrains what can be written through its interface; it is not a
 *  security boundary. The arcs keep their end points in the public data
 *  members `src_node` and `tgt_node`, which are `void *` and which the arc
 *  classes ask not to use: read through a `const Arc *` they still convert to
 *  a `Node *` with a cast. The same holds for what a cookie points to. Use
 *  `get_src_node()`, `get_tgt_node()` and `get_connected_node()`.
 *
 *  @ingroup Graphs
 */

# ifndef TPL_CONST_GRAPH_VIEW_H
# define TPL_CONST_GRAPH_VIEW_H

# include <cstddef>
# include <type_traits>
# include <utility>

# include <ah-concepts.H>
# include <ah-graph-concepts.H>
# include <ah-iterator.H>
# include <htlist.H>

namespace Aleph
{
  template <class GT>
  class Const_Graph_View;

  namespace const_graph_view_detail
  {
    /** @brief Hands a node or an arc read through a view back to the graph.
     *
     *  The members of the graph classes that only read still take `Node *`
     *  and `Arc *`. This is the single place where the view and its
     *  iterators remove the `const` that they add; nothing reached through
     *  the result is written.
     *
     *  @param[in] p a node or an arc of the graph.
     *  @return `p`, as the graph expects it.
     */
    template <class T>
    [[nodiscard]] constexpr T * as_graph_item(const T *p) noexcept
    {
      return const_cast<T *>(p);
    }
  } // namespace const_graph_view_detail

  /** @brief Read-only iterator on the nodes of a graph.
   *
   *  Wraps `GT::Node_Iterator` and hands out `const Node *`. The members
   *  that modify the graph through the iterator (`del()` on the graphs
   *  built on lists, for instance) do not exist.
   *
   *  @tparam GT the graph type.
   *  @see Const_Graph_View Const_Arc_Iterator Const_Node_Arc_Iterator
   *  @ingroup Graphs
   */
  template <class GT>
  class Const_Node_Iterator : public Const_Iterator<typename GT::Node_Iterator>
  {
    using Base = Const_Iterator<typename GT::Node_Iterator>;

    // A node iterator handing out a reference to the stored pointer would
    // turn into a reference to a constant pointer to a modifiable node.
    static_assert(std::is_same_v<decltype(std::declval<const Base &>().get_curr()),
                                 const typename GT::Node *>,
                  "GT::Node_Iterator::get_curr() must hand out a Node * by value");

  public:
    /// The type of the items: a pointer to a constant node.
    using Item_Type = const typename GT::Node *;

    /// The type of the graph whose nodes are visited.
    using Set_Type = GT;

    /// An iterator with no current node.
    Const_Node_Iterator() = default;

    /** @brief Iterator positioned on the first node of `g`.
     *  @param[in] g the graph; it is not modified through the iterator.
     */
    explicit Const_Node_Iterator(const GT & g) : Base(g) {}

    /** @brief Iterator positioned on the first node seen through `view`.
     *  @param[in] view a view of the graph.
     */
    explicit Const_Node_Iterator(const Const_Graph_View<GT> & view)
      : Base(*view.graph_ptr) {}

    /** @brief The current node.
     *  @return a pointer to the current node.
     *  @throw std::overflow_error if there is no current node.
     */
    [[nodiscard]] Item_Type get_current_node() const { return this->get_curr(); }

    /// get_current_node() without checking that there is a current node.
    [[nodiscard]] Item_Type get_current_node_ne() const noexcept
    {
      return this->get_curr_ne();
    }
  };

  /** @brief Read-only iterator on the arcs of a graph.
   *
   *  Wraps `GT::Arc_Iterator` and hands out `const Arc *` and, for the end
   *  points of the current arc, `const Node *`.
   *
   *  @tparam GT the graph type.
   *  @see Const_Graph_View Const_Node_Iterator Const_Node_Arc_Iterator
   *  @ingroup Graphs
   */
  template <class GT>
  class Const_Arc_Iterator : public Const_Iterator<typename GT::Arc_Iterator>
  {
    using Base = Const_Iterator<typename GT::Arc_Iterator>;

    static_assert(std::is_same_v<decltype(std::declval<const Base &>().get_curr()),
                                 const typename GT::Arc *>,
                  "GT::Arc_Iterator::get_curr() must hand out an Arc * by value");

  public:
    /// The type of the items: a pointer to a constant arc.
    using Item_Type = const typename GT::Arc *;

    /// The type of the graph whose arcs are visited.
    using Set_Type = GT;

    /// An iterator with no current arc.
    Const_Arc_Iterator() = default;

    /** @brief Iterator positioned on the first arc of `g`.
     *  @param[in] g the graph; it is not modified through the iterator.
     */
    explicit Const_Arc_Iterator(const GT & g) : Base(g) {}

    /** @brief Iterator positioned on the first arc seen through `view`.
     *  @param[in] view a view of the graph.
     */
    explicit Const_Arc_Iterator(const Const_Graph_View<GT> & view)
      : Base(*view.graph_ptr) {}

    /** @brief The current arc.
     *  @return a pointer to the current arc.
     *  @throw std::overflow_error if there is no current arc.
     */
    [[nodiscard]] Item_Type get_current_arc() const { return this->get_curr(); }

    /// get_current_arc() without checking that there is a current arc.
    [[nodiscard]] Item_Type get_current_arc_ne() const noexcept
    {
      return this->get_curr_ne();
    }

    /** @brief The source node of the current arc.
     *  @return a pointer to the node given first when the arc was inserted.
     *  @throw std::overflow_error if there is no current arc.
     */
    [[nodiscard]] const typename GT::Node * get_src_node() const
    {
      return this->underlying().get_src_node();
    }

    /// get_src_node() without checking that there is a current arc.
    [[nodiscard]] const typename GT::Node * get_src_node_ne() const noexcept
    {
      return this->underlying().get_src_node_ne();
    }

    /** @brief The target node of the current arc.
     *  @return a pointer to the node given second when the arc was inserted.
     *  @throw std::overflow_error if there is no current arc.
     */
    [[nodiscard]] const typename GT::Node * get_tgt_node() const
    {
      return this->underlying().get_tgt_node();
    }

    /// get_tgt_node() without checking that there is a current arc.
    [[nodiscard]] const typename GT::Node * get_tgt_node_ne() const noexcept
    {
      return this->underlying().get_tgt_node_ne();
    }
  };

  /** @brief Read-only iterator on the arcs of a node.
   *
   *  Wraps `GT::Node_Arc_Iterator`: it visits what that iterator visits (the
   *  arcs adjacent to the node; on a directed graph class, its outgoing
   *  arcs) and hands out `const Arc *` and `const Node *`.
   *
   *  @tparam GT the graph type.
   *  @see Const_Graph_View Const_Node_Iterator Const_Arc_Iterator
   *  @ingroup Graphs
   */
  template <class GT>
  class Const_Node_Arc_Iterator
    : public Const_Iterator<typename GT::Node_Arc_Iterator>
  {
    using Base = Const_Iterator<typename GT::Node_Arc_Iterator>;

    static_assert(std::is_same_v<decltype(std::declval<const Base &>().get_curr()),
                                 const typename GT::Arc *>,
                  "GT::Node_Arc_Iterator::get_curr() must hand out an Arc * by value");

  public:
    /// The type of the items: a pointer to a constant arc.
    using Item_Type = const typename GT::Arc *;

    /// The type of what is iterated: a constant node.
    using Set_Type = const typename GT::Node *;

    /// An iterator with no current arc.
    Const_Node_Arc_Iterator() = default;

    /** @brief Iterator positioned on the first arc of the node `p`.
     *  @param[in] p a node of the graph; it is not modified through the
     *  iterator.
     */
    explicit Const_Node_Arc_Iterator(const typename GT::Node *p)
      : Base(typename GT::Node_Arc_Iterator(const_graph_view_detail::as_graph_item(p))) {}

    /** @brief The current arc.
     *  @return a pointer to the current arc.
     *  @throw std::overflow_error if there is no current arc.
     */
    [[nodiscard]] Item_Type get_current_arc() const { return this->get_curr(); }

    /// get_current_arc() without checking that there is a current arc.
    [[nodiscard]] Item_Type get_current_arc_ne() const noexcept
    {
      return this->get_curr_ne();
    }

    /** @brief The node at the other end of the current arc.
     *  @return a pointer to the node connected to the iterated node through
     *  the current arc.
     *  @throw std::overflow_error if there is no current arc.
     */
    [[nodiscard]] const typename GT::Node * get_tgt_node() const
    {
      return this->underlying().get_tgt_node();
    }

    /// get_tgt_node() without checking that there is a current arc.
    [[nodiscard]] const typename GT::Node * get_tgt_node_ne() const noexcept
    {
      return this->underlying().get_tgt_node_ne();
    }
  };

  /** @brief Strict read-only view of a graph.
   *
   *  Every member hands out `const Node *` and `const Arc *`, and every
   *  operation received is called with them: neither the information nor
   *  the bookkeeping of the nodes and of the arcs can be written through a
   *  view. See the description of tpl_const_graph_view.H for the meaning of a
   *  `const` graph, the cost, the lifetime and the limits of a view.
   *
   *  The members mirror the reading members of the graph classes, with the
   *  same names. `Node` and `Arc` are the *constant* node and arc types, so
   *  that `Node *` reads as in the graph classes and the view satisfies
   *  `AlephGraph`.
   *
   *  @tparam GT the graph type: `List_Graph`, `List_Digraph`, `List_SGraph`,
   *  `List_SDigraph`, `Array_Graph`, `Array_Digraph` or any class with their
   *  interface.
   *  @see const_view() Const_Node_Iterator Const_Arc_Iterator
   *  Const_Node_Arc_Iterator
   *  @ingroup Graphs
   */
  template <class GT>
  class Const_Graph_View
  {
    const GT *graph_ptr;

    friend class Const_Node_Iterator<GT>;
    friend class Const_Arc_Iterator<GT>;

    template <class T>
    static constexpr T * as_graph_item(const T *p) noexcept
    {
      return const_graph_view_detail::as_graph_item(p);
    }

  public:
    using Graph_Type = GT; ///< The type of the graph that is viewed.

    using Node = const typename GT::Node; ///< The constant node type.

    using Arc = const typename GT::Arc; ///< The constant arc type.

    /// The type of the information stored in a node.
    using Node_Type = typename GT::Node_Type;

    /// The type of the information stored in an arc.
    using Arc_Type = typename GT::Arc_Type;

    /// Read-only iterator on the nodes.
    using Node_Iterator = Const_Node_Iterator<GT>;

    /// Read-only iterator on the arcs.
    using Arc_Iterator = Const_Arc_Iterator<GT>;

    /// Read-only iterator on the arcs of a node.
    using Node_Arc_Iterator = Const_Node_Arc_Iterator<GT>;

    /** @brief View of the graph `g`.
     *  @param[in] g the graph; it must outlive the view.
     */
    explicit Const_Graph_View(const GT & g) noexcept : graph_ptr(&g) {}

    /// A view of a temporary graph would dangle.
    Const_Graph_View(const GT &&) = delete;

    /// Total number of nodes.
    [[nodiscard]] size_t get_num_nodes() const noexcept
    {
      return graph_ptr->get_num_nodes();
    }

    /// Synonym of get_num_nodes().
    [[nodiscard]] size_t vsize() const noexcept { return get_num_nodes(); }

    /// `true` if the graph has no nodes.
    [[nodiscard]] bool is_empty() const noexcept { return get_num_nodes() == 0; }

    /// Total number of arcs.
    [[nodiscard]] size_t get_num_arcs() const noexcept
    {
      return graph_ptr->get_num_arcs();
    }

    /// Synonym of get_num_arcs().
    [[nodiscard]] size_t esize() const noexcept { return get_num_arcs(); }

    /// `true` if the graph is directed.
    [[nodiscard]] bool is_digraph() const noexcept
    {
      return graph_ptr->is_digraph();
    }

    /** @brief Number of arcs of a node, as the graph counts them.
     *  @param[in] p a node of the graph.
     *  @return the number of arcs of `p`.
     */
    [[nodiscard]] size_t get_num_arcs(Node *p) const noexcept
    {
      return graph_ptr->get_num_arcs(as_graph_item(p));
    }

    /** @brief Degree of a node: the number of its arcs.
     *  @param[in] p a node of the graph.
     *  @return the number of arcs of `p`.
     */
    [[nodiscard]] size_t degree(Node *p) const noexcept
    {
      return get_num_arcs(p);
    }

    /** @brief The source node of an arc.
     *  @param[in] a an arc of the graph.
     *  @return the node given first when `a` was inserted.
     */
    [[nodiscard]] Node * get_src_node(Arc *a) const noexcept
    {
      return graph_ptr->get_src_node(as_graph_item(a));
    }

    /** @brief The target node of an arc.
     *  @param[in] a an arc of the graph.
     *  @return the node given second when `a` was inserted.
     */
    [[nodiscard]] Node * get_tgt_node(Arc *a) const noexcept
    {
      return graph_ptr->get_tgt_node(as_graph_item(a));
    }

    /** @brief The node at the other end of an arc.
     *  @param[in] a an arc of the graph.
     *  @param[in] p one of the end points of `a`.
     *  @return the other end point of `a`.
     */
    [[nodiscard]] Node * get_connected_node(Arc *a, Node *p) const noexcept
    {
      return graph_ptr->get_connected_node(as_graph_item(a), as_graph_item(p));
    }

    /** @brief Any node of the graph.
     *  @pre the graph has at least one node; an empty graph is handled as
     *  `GT::get_first_node()` handles it, and not every graph class checks.
     *  @return a pointer to a node.
     */
    [[nodiscard]] Node * get_first_node() const
    {
      return graph_ptr->get_first_node();
    }

    /** @brief Any arc of the graph.
     *  @pre the graph has at least one arc; a graph without arcs is handled
     *  as `GT::get_first_arc()` handles it, and not every graph class checks.
     *  @return a pointer to an arc.
     */
    [[nodiscard]] Arc * get_first_arc() const
    {
      return graph_ptr->get_first_arc();
    }

    /** @brief Any arc of a node.
     *  @param[in] p a node of the graph.
     *  @return a pointer to an arc of `p`.
     *  @throw std::range_error if `p` has no arcs.
     */
    [[nodiscard]] Arc * get_first_arc(Node *p) const
    {
      return graph_ptr->get_first_arc(as_graph_item(p));
    }

    /// Iterator positioned on the first node.
    [[nodiscard]] Node_Iterator get_node_it() const
    {
      return Node_Iterator(*this);
    }

    /// Iterator positioned on the first arc.
    [[nodiscard]] Arc_Iterator get_arc_it() const
    {
      return Arc_Iterator(*this);
    }

    /** @brief Iterator positioned on the first arc of a node.
     *  @param[in] p a node of the graph.
     *  @return an iterator on the arcs of `p`.
     */
    [[nodiscard]] Node_Arc_Iterator get_arc_it(Node *p) const
    {
      return Node_Arc_Iterator(p);
    }

    /** @brief Visit the nodes while an operation answers `true`.
     *  @param[in] op called as `op(p)` with `p` a `const Node *`; its result
     *  is used as a condition.
     *  @return `true` if every node was visited.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    bool traverse_nodes(Op && op) const
    {
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        if (not op(it.get_curr_ne()))
          return false;
      return true;
    }

    /** @brief Visit the arcs while an operation answers `true`.
     *  @param[in] op called as `op(a)` with `a` a `const Arc *`; its result
     *  is used as a condition.
     *  @return `true` if every arc was visited.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    bool traverse_arcs(Op && op) const
    {
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        if (not op(it.get_curr_ne()))
          return false;
      return true;
    }

    /** @brief Visit the arcs of a node while an operation answers `true`.
     *  @param[in] p a node of the graph.
     *  @param[in] op called as `op(a)` with `a` a `const Arc *`; its result
     *  is used as a condition.
     *  @return `true` if every arc of `p` was visited.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    bool traverse_arcs(Node *p, Op && op) const
    {
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        if (not op(it.get_curr_ne()))
          return false;
      return true;
    }

    /** @brief Visit every node.
     *  @param[in] op called as `op(p)` with `p` a `const Node *`.
     */
    template <class Op>
      requires CallableWith<Op &, Node *>
    void for_each_node(Op && op) const
    {
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        op(it.get_curr_ne());
    }

    /** @brief Visit every arc.
     *  @param[in] op called as `op(a)` with `a` a `const Arc *`.
     */
    template <class Op>
      requires CallableWith<Op &, Arc *>
    void for_each_arc(Op && op) const
    {
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        op(it.get_curr_ne());
    }

    /** @brief Visit every arc of a node.
     *  @param[in] p a node of the graph.
     *  @param[in] op called as `op(a)` with `a` a `const Arc *`.
     */
    template <class Op>
      requires CallableWith<Op &, Arc *>
    void for_each_arc(Node *p, Op && op) const
    {
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        op(it.get_curr_ne());
    }

    /** @brief `true` if every node satisfies a condition.
     *  @param[in] op the condition, on a `const Node *`.
     *  @return `true` if `op` holds for every node.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    [[nodiscard]] bool all_nodes(Op && op) const
    {
      return traverse_nodes(op);
    }

    /** @brief `true` if every arc satisfies a condition.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return `true` if `op` holds for every arc.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] bool all_arcs(Op && op) const
    {
      return traverse_arcs(op);
    }

    /** @brief `true` if every arc of a node satisfies a condition.
     *  @param[in] p a node of the graph.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return `true` if `op` holds for every arc of `p`.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] bool all_arcs(Node *p, Op && op) const
    {
      return traverse_arcs(p, op);
    }

    /** @brief Linear search of a node.
     *  @param[in] op the condition, on a `const Node *`.
     *  @return the first node satisfying `op`, or `nullptr`.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    [[nodiscard]] Node * search_node(Op && op) const
    {
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        if (Node *p = it.get_curr_ne(); op(p))
          return p;
      return nullptr;
    }

    /** @brief Linear search of an arc.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return the first arc satisfying `op`, or `nullptr`.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] Arc * search_arc(Op && op) const
    {
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        if (Arc *a = it.get_curr_ne(); op(a))
          return a;
      return nullptr;
    }

    /** @brief Linear search among the arcs of a node.
     *  @param[in] p a node of the graph.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return the first arc of `p` satisfying `op`, or `nullptr`.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] Arc * search_arc(Node *p, Op && op) const
    {
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        if (Arc *a = it.get_curr_ne(); op(a))
          return a;
      return nullptr;
    }

    /** @brief Search an arc connecting two nodes.
     *  @param[in] src the node whose arcs are searched.
     *  @param[in] tgt the node at the other end.
     *  @return an arc of `src` whose other end is `tgt`, or `nullptr`.
     */
    [[nodiscard]] Arc * search_arc(Node *src, Node *tgt) const noexcept
    {
      for (Node_Arc_Iterator it(src); it.has_curr(); it.next_ne())
        if (it.get_tgt_node_ne() == tgt)
          return it.get_curr_ne();
      return nullptr;
    }

    /** @brief Search a node by its information.
     *  @param[in] info the information, compared with `==`.
     *  @return a node whose information equals `info`, or `nullptr`.
     */
    [[nodiscard]] Node * find_node(const Node_Type & info) const
    {
      return search_node([&info] (Node *p) { return p->get_info() == info; });
    }

    /** @brief Search an arc by its information.
     *  @param[in] info the information, compared with `==`.
     *  @return an arc whose information equals `info`, or `nullptr`.
     */
    [[nodiscard]] Arc * find_arc(const Arc_Type & info) const
    {
      return search_arc([&info] (Arc *a) { return a->get_info() == info; });
    }

    /** @brief `true` if some node satisfies a condition.
     *  @param[in] op the condition, on a `const Node *`.
     *  @return `true` if `op` holds for at least one node.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    [[nodiscard]] bool exists_node(Op && op) const
    {
      return search_node(op) != nullptr;
    }

    /** @brief `true` if some arc satisfies a condition.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return `true` if `op` holds for at least one arc.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] bool exists_arc(Op && op) const
    {
      return search_arc(op) != nullptr;
    }

    /** @brief `true` if some arc of a node satisfies a condition.
     *  @param[in] p a node of the graph.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return `true` if `op` holds for at least one arc of `p`.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] bool exists_arc(Node *p, Op && op) const
    {
      return search_arc(p, op) != nullptr;
    }

    /** @brief Number of nodes satisfying a condition.
     *  @param[in] op the condition, on a `const Node *`.
     *  @return how many nodes satisfy `op`.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    [[nodiscard]] size_t count_nodes(Op && op) const
    {
      size_t count = 0;
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        if (op(it.get_curr_ne()))
          ++count;
      return count;
    }

    /** @brief Number of arcs satisfying a condition.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return how many arcs satisfy `op`.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] size_t count_arcs(Op && op) const
    {
      size_t count = 0;
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        if (op(it.get_curr_ne()))
          ++count;
      return count;
    }

    /** @brief The nodes of the graph.
     *  @return a list with a pointer to every node.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    [[nodiscard]] DynList<Node *> nodes() const
    {
      DynList<Node *> ret;
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(it.get_curr_ne());
      return ret;
    }

    /** @brief The arcs of the graph.
     *  @return a list with a pointer to every arc.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    [[nodiscard]] DynList<Arc *> arcs() const
    {
      DynList<Arc *> ret;
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(it.get_curr_ne());
      return ret;
    }

    /** @brief The arcs of a node.
     *  @param[in] p a node of the graph.
     *  @return a list with a pointer to every arc of `p`.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    [[nodiscard]] DynList<Arc *> arcs(Node *p) const
    {
      DynList<Arc *> ret;
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        ret.append(it.get_curr_ne());
      return ret;
    }

    /** @brief The nodes connected to a node.
     *  @param[in] p a node of the graph.
     *  @return a list with the node at the other end of every arc of `p`.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    [[nodiscard]] DynList<Node *> adjacent_nodes(Node *p) const
    {
      DynList<Node *> ret;
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        ret.append(it.get_tgt_node_ne());
      return ret;
    }

    /** @brief The nodes satisfying a condition.
     *  @param[in] op the condition, on a `const Node *`.
     *  @return a list with a pointer to every node satisfying `op`.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    template <class Op>
      requires PredicateWith<Op &, Node *>
    [[nodiscard]] DynList<Node *> filter_nodes(Op && op) const
    {
      DynList<Node *> ret;
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        if (Node *p = it.get_curr_ne(); op(p))
          ret.append(p);
      return ret;
    }

    /** @brief The arcs satisfying a condition.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return a list with a pointer to every arc satisfying `op`.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] DynList<Arc *> filter_arcs(Op && op) const
    {
      DynList<Arc *> ret;
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        if (Arc *a = it.get_curr_ne(); op(a))
          ret.append(a);
      return ret;
    }

    /** @brief The arcs of a node satisfying a condition.
     *  @param[in] p a node of the graph.
     *  @param[in] op the condition, on a `const Arc *`.
     *  @return a list with a pointer to every arc of `p` satisfying `op`.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    template <class Op>
      requires PredicateWith<Op &, Arc *>
    [[nodiscard]] DynList<Arc *> filter_arcs(Node *p, Op && op) const
    {
      DynList<Arc *> ret;
      for (Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
        if (Arc *a = it.get_curr_ne(); op(a))
          ret.append(a);
      return ret;
    }

    /** @brief Map the nodes to a list.
     *  @tparam T the type of the items of the list; by default, the type
     *  of the information of a node.
     *  @param[in] op called as `op(p)` with `p` a `const Node *`; its result
     *  converts to `T`.
     *  @return the list of the results, in the order of the visit.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    template <typename T = Node_Type, class Op>
      requires std::is_invocable_r_v<T, Op &, Node *>
    [[nodiscard]] DynList<T> nodes_map(Op op) const
    {
      DynList<T> ret;
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(concepts_detail::invoke_as<T (Node *)>::call(op, it.get_curr_ne()));
      return ret;
    }

    /** @brief Map the arcs to a list.
     *  @tparam T the type of the items of the list; by default, the type
     *  of the information of an arc.
     *  @param[in] op called as `op(a)` with `a` a `const Arc *`; its result
     *  converts to `T`.
     *  @return the list of the results, in the order of the visit.
     *  @throw std::bad_alloc if there is no memory for the list.
     */
    template <typename T = Arc_Type, class Op>
      requires std::is_invocable_r_v<T, Op &, Arc *>
    [[nodiscard]] DynList<T> arcs_map(Op op) const
    {
      DynList<T> ret;
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        ret.append(concepts_detail::invoke_as<T (Arc *)>::call(op, it.get_curr_ne()));
      return ret;
    }

    /** @brief Fold the nodes from the left.
     *  @tparam T the type of the accumulated value.
     *  @param[in] init the initial value.
     *  @param[in] op called as `op(acc, p)` with `p` a `const Node *`; its
     *  result converts to `T` and becomes the accumulated value.
     *  @return the accumulated value after the last node.
     */
    template <typename T = Node_Type, class Op>
      requires std::is_invocable_r_v<T, Op &, const T &, Node *>
    [[nodiscard]] T foldl_nodes(const T & init, Op op) const
    {
      T ret = init;
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        ret = concepts_detail::invoke_as<T (const T &, Node *)>::call(op, ret, it.get_curr_ne());
      return ret;
    }

    /** @brief Fold the arcs from the left.
     *  @tparam T the type of the accumulated value.
     *  @param[in] init the initial value.
     *  @param[in] op called as `op(acc, a)` with `a` a `const Arc *`; its
     *  result converts to `T` and becomes the accumulated value.
     *  @return the accumulated value after the last arc.
     */
    template <typename T = Arc_Type, class Op>
      requires std::is_invocable_r_v<T, Op &, const T &, Arc *>
    [[nodiscard]] T foldl_arcs(const T & init, Op op) const
    {
      T ret = init;
      for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
        ret = concepts_detail::invoke_as<T (const T &, Arc *)>::call(op, ret, it.get_curr_ne());
      return ret;
    }
  };

  /** @brief Strict read-only view of a graph.
   *
   *  @code
   *  auto view = const_view(g);
   *  for (auto it = view.get_arc_it(); it.has_curr(); it.next_ne())
   *    use(it.get_curr()->get_info());  // a reference to const
   *  @endcode
   *
   *  @param[in] g the graph; it must outlive the view.
   *  @return a `Const_Graph_View` of `g`.
   *  @ingroup Graphs
   */
  template <AlephGraph GT>
  [[nodiscard]] Const_Graph_View<GT> const_view(const GT & g) noexcept
  {
    return Const_Graph_View<GT>(g);
  }

  /// A view of a temporary graph would dangle.
  template <AlephGraph GT>
  void const_view(const GT &&) = delete;
} // namespace Aleph

# endif // TPL_CONST_GRAPH_VIEW_H
