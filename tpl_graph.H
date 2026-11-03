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


/** @file tpl_graph.H
 *  @brief Generic graph and digraph implementations.
 *
 *  This file provides the core graph data structures for Aleph-w, including
 *  `List_Graph` (undirected graphs) and `List_Digraph` (directed graphs).
 *  These templates support customizable node and arc information types,
 *  and provide efficient adjacency list representations with iterators.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_GRAPH_H
# define TPL_GRAPH_H

# include <memory>
# include <cassert>
# include <cstdint>
# include <optional>
# include <tuple>
# include <utility>
# include <functional>
# include <bitArray.H>
# include <tpl_dynArray.H>
# include <tpl_sort_utils.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynDlist.H>
# include <tpl_treapRk.H>
# include <filter_iterator.H>
# include <aleph-graph.H>
# include <graph-dry.H>
# include <ah-graph-concepts.H>
# include <ah-errors.H>
# include <ah-iterator.H>
# include <tpl_dfs_stack.H>

using namespace Aleph;

namespace Aleph
{
  template <typename Node_Info>
  struct Graph_Node;

  template <typename Arc_Info>
  struct Graph_Arc;

  class Arc_Node;

  template <class GT>
  class Path;

  template <typename __Graph_Node, typename __Graph_Arc>
  class List_Graph;

  // List_Digraph is defined as a type alias later in this file
  // using the generic Digraph<> template wrapper

  template <class GT>
  class Mat_Graph;

  template <typename MT, typename Entry_Info, typename Copy>
  class Ady_MaT;

  /** Node belonging to a graph implemented with a double linked
        adjacency list.

        This class is used for defining a node of the graph implemented with
        double-linked adjacency lists.

        Basically, there are three ways for defining the information to
        be stored in a node:

        1. Specifying the data as a template parameter of this class.
        2. By inheriting from this class and in the derived class to
        define the data
        3. Combination of two above

        @note The purpose of this class is only for specifying the node
        of a graph. It is not intended for declaration. Use the internal
        node subclass instead. Although you could access data and
        function members of this class, at the exception to `get_info()`
        it is not advisable to use them. They are accessible for
        implementation purposes.

        \note A node has three additional attributes: the control bits,
        the counter and the cookie.

        @ingroup Graphs
        @see List_Graph List_Digraph Graph_Arc Bit_Fields
    */
  template <typename __Node_Info = Empty_Class>
  struct Graph_Node
      : public Dlink,
        public GTNodeCommon<__Node_Info>
  {
    friend class GTNodeCommon<__Node_Info>;
    friend class Arc_Node;

    using Base = GTNodeCommon<__Node_Info>;
    using Node_Info = __Node_Info; /// The type of data stored in the node

    /** Copy constructor.

          Construct a node whose data is copied from `info`. The control
          attributes are reset.

          @param[in] info data to copy to the node

          @note The copy constructor of `Node_Info` must be defined
      */
    Graph_Node(const Node_Info & info)
      noexcept(std::is_nothrow_copy_constructible_v<Node_Info>)
      : Base(info)
    { /* empty */
    }

    /** Move or rvalue constructor

          Construct a node whose data is moved from `info`. The control
          attributes are reset.

          @param[in] info data to move to the node

          @note The move constructor of `Node_Info` must be defined
      */
    Graph_Node(Node_Info && info = Node_Info())
      noexcept(std::is_nothrow_move_constructible_v<Node_Info>)
      : Base(std::move(info))
    {
      /* empty */
    }

    Graph_Node(const Graph_Node & node)
      noexcept(std::is_nothrow_copy_constructible_v<Node_Info>)
      : Graph_Node(node.node_info)
    {
      // empty
    }

    Graph_Node &operator=(const Graph_Node & node)
    {
      if (&node == this)
        return *this;
      this->node_info = node.node_info;
      return *this;
    }

    /** Copy constructor from a node pointer.

          Creates a node and assigns `node->get_info()` as the value of
          the information contained in the node.

          The control bits and the counter values are set to zero. The
          `cookie` is set to `nullptr`.

          @param[in] node pointer to the node from which the information
          value to be assigned to the newly created node is copied.

          @note The copy constructor of the `Node_Info` class must be defined.
          @note The created instance is a different node from `node`.
      */
    Graph_Node(Graph_Node *node)
      : Base(node->get_info())
    {
      /* empty */
    }

    Dlink arc_list; /// Under no circumstances: do not touch it
  };

  /** Arc of graph implemented with double-linked adjacency lists.

        This class defines an arc of a graph implemented with double-linked adjacency lists.

        Basically, there are three ways for defining the information to
        be stored in an arc:

        1. Specifying the data as a template parameter of this class.
        2. By inheriting from this class and in the derived class to
        define the data
        3. Combination of two above

        @note The purpose of this class is only for specifying the arc
        of a graph. It is not intended for declaration. Use the internal
        node subclass instead. Although you could access data and
        function members of this class, at the exception to `get_info()`
        it is not advisable to use them. They are accessible for
        implementation purposes.

        \note An arc has three additional attributes: the control bits,
        the counter and the cookie.

        @ingroup Graphs
        @see List_Graph List_Digraph Graph_Node Bit_Fields
    */
  template <typename _Arc_Info = Empty_Class>
  struct Graph_Arc
      : public Dlink,
        public GTArcCommon<_Arc_Info>
  {
    friend class GTArcCommon<_Arc_Info>;

    using Base = GTArcCommon<_Arc_Info>;

    using Arc_Info = _Arc_Info; /// The type of data stored in the arc

    Arc_Node *src_arc_node = nullptr; // pointer to source node
    Arc_Node *tgt_arc_node = nullptr; // pointer to target node

    /** Copy constructor.

          Construct an arc whose data is copied from `info`. The control
          attributes are reset.

          @param[in] info data to copy to the arc

          @note The copy constructor of `Arc_Info` must be defined
      */
    Graph_Arc(const Arc_Info & info)
      noexcept(std::is_nothrow_copy_constructible_v<Arc_Info>)
      : Base(info)
    {
      /* empty */
    }

    /** Move or rvalue constructor

          Construct an arc whose data is moved from `info`. The control
          attributes are reset.

          @param[in] info data to move to the arc

          @note The move constructor of `Arc_Info` must be defined
      */
    Graph_Arc(Arc_Info && info = Arc_Info())
      noexcept(std::is_nothrow_move_constructible_v<Arc_Info>)
      : Base(std::move(info))
    {
      /* empty */
    }

    Graph_Arc(const Graph_Arc & arc)
      : Graph_Arc(arc.arc_info)
    {
      /* empty */
    }

    Graph_Arc &operator=(const Graph_Arc & arc)
    {
      if (&arc == this)
        return *this;
      this->arc_info = arc.arc_info;
      return *this;
    }
  };

  class Arc_Node : public Dlink
  {
  public:
    void *arc = nullptr;

    Arc_Node() noexcept : arc(nullptr) {}

    Arc_Node(void *__arc) noexcept : arc(__arc) {}
  };

  /** Graph implemented with double-linked adjacency lists

        This class manages two template parameters
        - `_Graph_Node`: the type of node, which previously must have
        been defined through of Graph_Node class.
        - `_Graph_Arc`: the type of arc, which previously must have
        been defined through of `Graph_Arc` class.

        It is highly recommendable to use an alias for the final graph
        type. Some such as

        using My_Graph = List_Graph<Graph_Node<My_Node_Type>,
        Graph_Arc<My_Arc_Type>>;

        Once instantiated a `List_Graph` object, its nodes and arcs must
        be accessed through the following subclasses:
        - `List_Graph<Node, Arc>::Node`
        - `List_Graph<Node, Arc>::Arc`

        Directed or non-directed? (how and when using them)
        ===================================================

        Using of alias is very "natural". For the previously shown alias
        example, you could type:

        My_Graph::Node * node_ptr = ....
        My_Graph::Arc * arc_ptr = ....

        This graph can be directed or non-directed. From a functional
        perspective, what gives the difference between a directed graph
        and a non-directed one is the way for accessing to arcs given a
        node. Perhaps an example could help to understand
        better. Suppose that you have a graph `g` and two node pointers
        `p` and `q` respectively. Now, to specify an arc, you
        execute some such as

        auto arc = g.insert_arc(p, q);

        If you execute

        auto my_node = g.get_connected_arc(arc, p);

        Then you will get `q`, which is the node connected to `p` through
        `arc`. Now, if you execute

        auto my_node = g.get_connected_arc(arc, q);

        Then you will get `p`, which is the node connected to `q`
        through `arc`. In this situation you are trying `g` as a
        non-directed graph.

        However, when you inserted the arc, you specified a source node
        and a target one. This knowledge, which is embedded in the
        internal data structure, can be profited by the user for
        treating `g` as a directed graph. In that case, instead of using
        the `get_connected_node()` method, you use other methods
        expressing the arc sense. For instance, in the previous example
        you could do:

        auto src = g.get_src_node(arc); // source node of arc
        auto tgt = g.get_tgt_node(arc); // target node of arc

        The class `List_Digraph`
        -----------------------

        `Aleph-w` (\f$\aleph_\omega\f$) exports a derived named
        `List_Digraph`. At the functional level, there is no almost any
        difference with `List_Graph`. The only functional difference is
        that `get_connected_node()` has no any sense on `List_Digraph`
        objects. Be very careful with the fact that `Aleph-w`
        (\f$\aleph_\omega\f$) does not perform any check of this. To use
        `get_connected_node()` for `List_Digraph` objects is an
        programming error.

        So that, if there is no practically much difference, then, why
        to use `List_Digraph`?

        Memory consumption
        ------------------

        Probably `List_Graph` is the more versatile graph class of
        `Aleph-w` (\f$\aleph_\omega\f$). However, its versatility is at
        the expense of highest memory consumption. First, the internal
        lists are double, not single. Second, for each arc, there is a
        double linked list of arc references. This implicates that the
        same arc information is duplicated in each involved node, the
        source and the target one.

        At the contrary, in `List_Digraph` the arc references are not
        stored in the target node. This fact could considerably save
        memory.

        Performance
        -----------

        For `List_Graph` all (yes, all!) operations are \f$O(1)\f$, but,
        as said, this performance is achieved at the expense of memory.

        For `List_Digraph` all operations are \f$O(1)\f$, except node
        removal. In this case, since with this representation there is
        no direct way to know the incoming arcs, the node removal
        requires traversing all the arcs for checking for those
        incoming to the removed one. So, the node removal is
        \f$O(E)\f$.

        Versatility (`List_Graph` vs `List_Digraph`)
        ---------------------------------------------

        So that a guideline for choosing `List_Graph` vs `List_Digraph`
        is to ask if the node removal will be used. If yes, then how
        often? If you know that the node removal will be used several
        times, then prefer `List_Graph`; otherwise, that is, you know
        there are no node removals, then use `List_Digraph`, since it
        uses less memory.

        Note on validations
        -------------------

        Almost all the methods of this class do not perform any
        validation. This is especially delicate for those methods
        involved topologic changes (insertions and removals of nodes or
        arcs).

        When a removal operation is called, this assumes that it is
        performed on a valid object. For example, `remove_node(p)`
        assumes that `p` is a valid node pointer. For valid we
        understand that the pointer was returned for another method of
        graph class and on the same graph. At this regard, is very
        important to be careful because no method validates if the
        involved object was or not created either this belongs to the
        graph.

        @see Graph_Node Graph_Arc Array_Graph List_SGraph
        @ingroup Graphs
        @see List_Digraph Path
    */
  template <typename _Graph_Node = Graph_Node<unsigned long>,
            typename _Graph_Arc = Graph_Arc<unsigned long>>
  class List_Graph
      : public GraphCommon<List_Graph<_Graph_Node, _Graph_Arc>,
                           _Graph_Node, _Graph_Arc>
  {
  public:
    using GT = List_Graph; /// The graph type

    using Node = _Graph_Node; /// The node class type
    using Arc = _Graph_Arc; /// The arc class type

    /// The type of data stored in the node
    using Node_Type = typename Node::Node_Type;

    /// The type of data stored in the arc
    using Arc_Type = typename Arc::Arc_Type;

    friend class GraphCommon<List_Graph<_Graph_Node, _Graph_Arc>,
                             _Graph_Node, _Graph_Arc>;

    using CommonBase = GraphCommon<List_Graph<_Graph_Node, _Graph_Arc>,
                                   _Graph_Node, _Graph_Arc>;

    using CommonBase::insert_node;
    using CommonBase::insert_arc;

  private:
    Dlink node_list;
    Dlink arc_list;

    static Node * dlink_to_node(Dlink *p) noexcept
    {
      return static_cast<Node *>(p);
    }

    static Arc * dlink_to_arc(Dlink *p) noexcept
    {
      return static_cast<Arc *>(p);
    }

    static Arc_Node * dlink_to_arc_node(Dlink *p) noexcept
    {
      return static_cast<Arc_Node *>(p);
    }

    static Arc * void_to_arc(Arc_Node *arc_node) noexcept
    {
      return static_cast<Arc *>(arc_node->arc);
    }

  public:
    /** @brief Return a reference to the internal node Dlink for sorting.
     *
     *  This accessor, `get_node_dlink()`, exposes the internal `node_list`
     *  used by sort operations (e.g., `sort_nodes()`).
     *
     *  @return Dlink& reference to `node_list`.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     *  @note Intended for sorting operations; mutating the list directly can
     *        violate graph invariants if used outside that context.
     */
    Dlink &get_node_dlink() noexcept { return node_list; }

    /** @brief Return a reference to the internal arc Dlink for sorting.
     *
     *  This accessor, `get_arc_dlink()`, exposes the internal `arc_list`
     *  used by sort operations (e.g., `sort_arcs()`).
     *
     *  @return Dlink& reference to `arc_list`.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     *  @note Intended for sorting operations; mutating the list directly can
     *        violate graph invariants if used outside that context.
     */
    Dlink &get_arc_dlink() noexcept { return arc_list; }
    /** Insertion of a node already allocated.

          This method takes a pointer to a `node` already allocated and
          inserts it in the graph.

          The node allocation and its life in memory are the user's
          responsibility. At this regard, take into account that the graph
          destructor and `remove_method()` will assume that the nodes
          were allocated with `new` operator, and consequently it will call
          to `delete` for deallocating. So, it is very dangerous to use
          nodes whose memory does not come from `new`.

          \note Since that it is programmer responsibility to allocate
          memory for the node, and in addition, the graph destructor and the
          `remove_noe()` will call `delete` operator, it is preferable
          and highly recommended to use any other overloaded version of
          `insert_node()` which automatically allocates the node's memory. In
          fact, those overloaded methods invoke this version.

          @param[in] node a pointer to an already allocated node.
          @return the node pointer
      */
    virtual Node * insert_node(Node *node) noexcept
    {
      ++this->num_nodes;
      node_list.append(node);
      return node;
    }

    /** Remove a node from the graph and free its memory.

          This method removes all the incoming and outcoming arcs
          referred to the node and frees the memory. Then removes the
          node from the graph and frees its memory. All the memory occupied
          by the arcs related to the node and the node itself is freed.

          The method takes \f$O(1)\f$ on `List_Graph` objects and
          **\f$O(E)\f$ on `List_Digraph` objects**.

          @param[in] node to be removed and freed
          @throw bad_alloc if, in a digraph, there is no memory for the list
          of the arcs of node, which is made before any is removed: the
          graph is then as it was.
      */
    virtual void remove_node(Node *node)
    {
      assert(node != nullptr);
      if (not this->digraph)
        while (not node->arc_list.is_empty()) // remove each arc related to node
          {
            Arc_Node *arc_node = dlink_to_arc_node(node->arc_list.get_next());
            Arc *arc = void_to_arc(arc_node);
            remove_arc(arc);
          }
      else // Scan all the arcs and remove those related to node
        this->remove_arcs_if([node, this](auto a)
                               {
                                 return this->get_src_node(a) == node or this->get_tgt_node(a) == node;
                               });

      // At this point the node has not more arcs
      node->del(); // unlink it from arc_list
      --this->num_nodes;
      delete node;
    }

    /** Return any node in the graph.

          This method serves as an entry point to the graph. Some algorithms
          only need an initial node which could be anyone. In these cases,
          this method is indicated.

          \note The idea of "first node" does not obey any order; the method
          simply returns a node.

          \return A pointer to a node in the graph
      */
    Node * get_first_node() const
    {
      ah_range_error_if(this->num_nodes == 0) << "Graph has not nodes";

      return dlink_to_node(const_cast<Dlink &>(node_list).get_next());
    }

    /** Return any arc adjacent to a node.

          This method serves as an arc of departure given a node. Some
          algorithms only need any arc adjacent to a node in order to start
          their computations. In these cases, this method is indicated.

          \note The idea of "first arc" does not obey any order; the method
          simply returns an arc. The majority of algorithms should be
          independent of the processing order of arcs

          \return A pointer to an arc adjacent to `node`
      */
    Arc * get_first_arc(Node *node) const
    {
      ah_range_error_if(this->get_num_arcs(node) == 0) << "node has not arcs";

      void *arc = dlink_to_arc_node(node->arc_list.get_next())->arc;
      return reinterpret_cast<Arc *>(arc);
    }

  private:
    Arc * insert_arc(Node *src_node, Node *tgt_node, void *a)
    {
      assert(src_node != nullptr and tgt_node != nullptr and a != nullptr);
      Arc *arc = static_cast<Arc *>(a);
      arc->src_node = src_node;
      arc->tgt_node = tgt_node;

      // step 3 (partial): allocate Arc_Node for src_node
      std::unique_ptr<Arc_Node> src_arc_node = std::make_unique<Arc_Node>(arc);

      // step 2: if graph ==> allocate Arc_Node for tgt_node
      if (not this->digraph) // if digraph ==> do not insert in another node
        { // insertion in the target node
          if (src_node == tgt_node) // is it a loop?
            arc->tgt_arc_node = src_arc_node.get();
          else
            { // allocate arc node for tgt_node
              std::unique_ptr<Arc_Node> tgt_arc_node = std::make_unique<Arc_Node>(arc);

              // insertion in an adjacency list of tgt_node
              arc->tgt_arc_node = tgt_arc_node.get();
              tgt_node->arc_list.append(tgt_arc_node.get());
              ++tgt_node->num_arcs;
              tgt_arc_node.release();
            }
        }

      // step 3 (remainder): insertion in the adjacency list of src_node
      arc->src_arc_node = src_arc_node.get();
      src_node->arc_list.append(src_arc_node.get());
      ++src_node->num_arcs;

      arc_list.append(arc); // step 4: insert in graph arc list
      ++this->num_arcs;
      src_arc_node.release();

      return arc;
    }

  public:
    /** Remove an arc from the graph and free it.

          @param[in] arc pointer to the arc to be removed from graph and free it
          \see disconnect_arc(Arc * arc)
      */
    virtual void remove_arc(Arc *arc) noexcept
    {
      assert(arc != nullptr);
      // step 1: remove Arc_node from src_node
      Node *src_node = this->get_src_node(arc);
      Arc_Node *src_arc_node = arc->src_arc_node;

      src_arc_node->del(); // unlink src_node from node list
      --src_node->num_arcs; // decrement arc counter of src_node
      delete src_arc_node; // free memory

      if (not this->digraph)
        { // arc removal in target node
          Node *tgt_node = this->get_tgt_node(arc);
          if (src_node != tgt_node) // check for loop removal
            { // step 2: remove Arc_node from tgt_node
              Arc_Node *tgt_arc_node = arc->tgt_arc_node;
              tgt_arc_node->del();
              --tgt_node->num_arcs;
              delete tgt_arc_node;
            }
        }

      // arc removal from graph
      arc->del(); // unlink arc from the graph arc list
      --this->num_arcs;
      delete arc;
    }

    /** Disconnect an arc from graph.

          This method removes an arc from the graph; that is, it does no
          longer belong to the graph. However, the memory is not freed.

          Although the use of this method is occasional, it is not
          exceptional. For example, some situations require to temporarily
          disconnect the arc and afterward connect it again.

          The disconnected arc is already for being reinserted in the graph
          with the `connect_arc()` method.

          \warning The arc remains out of graph. So, from the graph
          perspective, a disconnected arc does not exist longer, and it will
          not be freed by the graph destructor neither by `remove_arc()`. In
          fact, to call `remove_arc()` on a disconnected arc is an error. It
          is your responsibility to free a disconnected arc.

          @param[in] arc pointer to the arc to disconnect
          \see connect_arc(Arc * arc)
      */
    virtual void disconnect_arc(Arc *arc) noexcept
    {
      assert(arc != nullptr);
      Node *src_node = this->get_src_node(arc);
      Arc_Node *src_arc_node = arc->src_arc_node;
      src_arc_node->del(); // unlink src_node from node list
      --src_node->num_arcs; // decrement arc counter of src_node

      if (not this->digraph)
        { // arc removal in target node
          Node *tgt_node = this->get_tgt_node(arc);
          if (src_node != tgt_node) // check for loop removal
            { // step 2: remove Arc_node from target node tgt_node
              Arc_Node *tgt_arc_node = arc->tgt_arc_node;
              tgt_arc_node->del();
              --tgt_node->num_arcs;
            }
        }

      // arc removal from graph
      arc->del(); // unlink arc from graph arc list
      --this->num_arcs;
    }

    /** Connect a previously disconnected arc to the graph.

          This method serves for connecting an arc that has been
          disconnected with `disconnect_arc()` method. Once connected,
          the arc is again part of the graph and can be removed and
          freed by `remove_arc()` or by the destructor.

          @param[in] arc pointer to the arc to connect
          @return a pointer to the connected arc
      */
    virtual Arc * connect_arc(Arc *arc) noexcept
    {
      assert(arc != nullptr);
      Node *src_node = this->get_src_node(arc);
      Node *tgt_node = this->get_tgt_node(arc);
      Arc_Node *src_arc_node = arc->src_arc_node;
      Arc_Node *tgt_arc_node = arc->tgt_arc_node;

      if (not this->digraph) // if digraph ==> no need to insert in other node
        { // insertion in target node
          if (src_node != tgt_node) // check if it is a loop
            { // insertion in adjacency list of tgt_node
              tgt_node->arc_list.append(tgt_arc_node);
              ++tgt_node->num_arcs;
            }
        }

      src_node->arc_list.append(src_arc_node);
      ++src_node->num_arcs;
      arc_list.append(arc);
      ++this->num_arcs;

      return arc;
    }

    /** Return any arc in the graph.

          This method serves as an arc of depart in the graph. Some
          algorithms only need an initial arc which could be anyone. In
          these cases, this method is indicated.

          \note The idea of "first arc" does not obey any order; the method
          simply returns a arc.

          \return A pointer to an arc in the graph
      */
    Arc * get_first_arc() const
    {
      ah_range_error_if(this->get_num_arcs() == 0) << "Graph has not arcs";

      return dlink_to_arc(const_cast<Dlink &>(arc_list).get_next());
    }

    // sort_nodes() and sort_arcs() are inherited from GraphCommon via CRTP
    // They use the get_node_dlink() and get_arc_dlink() accessors defined above

    /// @see ALEPH_GRAPH_COPY_MOVE_CTORS
    ALEPH_GRAPH_COPY_MOVE_CTORS(List_Graph)

    /// Destructor: all the nodes and arcs and removed and freed
    virtual ~List_Graph()
    {
      clear_graph(*this);
    }

    /** Node iterator

          This is an iterator on all the nodes of graph.

          @ingroup Graphs
      */
    struct Node_Iterator : public GTNodeIterator<List_Graph>
    {
      using Base = GTNodeIterator<List_Graph>;

      /// Construct an iterator on the nodes of `g`
      Node_Iterator(const List_Graph & g)
        : Base(const_cast<Dlink &>(g.node_list))
      {
        // empty
      }
    };

    /** Iterator on the arcs of a graph.

          This iterator scans all the arcs related to a node.

          If the graph is not directed. then all the related nodes will
          be seen, independently of specified order when the arc was
          inserted. Use `get_connected_node()` for obtaining the target
          node given a source node and arc.

          If you are using `List_Graph` as a directed graph, then this
          iterator has not much sense. Use `Out_Iterator` or
          `In_Iterator` in order to iterate on the outcoming or incoming
          arcs of a node.

          If the graph is directed, then only the arcs specified as
          target (second parameter in the `insert_arc()` method) will be
          seen.

          \see In_Iterator Out_Iterator

          @ingroup Graphs
      */

    class Node_Arc_Iterator : public Dlink::Iterator
    {
      Node *src_node;

    public:
      using Item_Type = Arc *; /// The type of data of set

      using Set_Type = Node *; /// The container type (a node)

      Node_Arc_Iterator() = default;

      /** Constructs an iterator on the node src.

            @param[in] src the node over whom you wish to iterate.
        */
      Node_Arc_Iterator(Node *src) noexcept
        : Dlink::Iterator(&(src->arc_list)), src_node(src)
      {
        // empty
      }

      Arc_Node * get_current_arc_node() const
      {
        return dlink_to_arc_node(Dlink::Iterator::get_curr());
      }

      Arc_Node * get_current_arc_node_ne() const noexcept
      {
        return dlink_to_arc_node(Dlink::Iterator::get_curr_ne());
      }

      ///\overload get_current_arc()
      Arc * get_curr() const
      {
        return static_cast<Arc *>(get_current_arc_node()->arc);
      }

      ///\overload get_current_arc()
      Arc * get_curr_ne() const noexcept
      {
        return static_cast<Arc *>(get_current_arc_node_ne()->arc);
      }

      /// Return the current arc. Throw `overflow_error` is there is no
      /// current arc
      Arc * get_current_arc() const
      {
        return get_curr();
      }

      Arc * get_current_arc_ne() const noexcept
      {
        return get_curr_ne();
      }

      /// Return the connected node to source node (src passed in
      /// construction time) through the current arc. Throw
      /// `overflow_error` is there is no current
      Node * get_tgt_node() const
      {
        return static_cast<Node *>(get_current_arc()->get_connected_node(src_node));
      }

      Node * get_tgt_node_ne() const noexcept
      {
        return static_cast<Node *>(get_current_arc_ne()->get_connected_node(src_node));
      }

      Node * get_node() const
      {
        return get_tgt_node();
      }

      Node * get_node_ne() const noexcept
      {
        return get_tgt_node_ne();
      }
    };

    /** Iterator on all arcs of a graph.

          This iterator scans all the arcs of the graph. The apparition
          order is random, but this can be modified through of
          `sort_arcs()` method.

          @see sort_arcs()
      */
    struct Arc_Iterator : public Dlink::Iterator
    {
      using Item_Type = Arc *; /// The type of element over whom iterate

      using Set_Type = List_Graph; /// The type of set

      Arc_Iterator() = default;

      /// Initialize an iterator for all the arc of `g`
      Arc_Iterator(const List_Graph & g) noexcept
        : Dlink::Iterator(&const_cast<Dlink &>(g.arc_list))
      {
        // empty
      }

      /// Return the current arc. Throw `overflow_error` if there is no one
      Arc * get_current_arc_ne() const noexcept
      {
        return dlink_to_arc(const_cast<Dlink *>(Dlink::Iterator::get_curr_ne()));
      }

      Arc * get_current_arc() const
      {
        return dlink_to_arc(const_cast<Dlink *>(Dlink::Iterator::get_curr()));
      }

      Arc * get_curr() const
      {
        return get_current_arc();
      }

      Arc * get_curr_ne() const noexcept
      {
        return get_current_arc_ne();
      }

      /// Return the source node of the current arc. Throw `overflow_error` if
      /// there is no one. Of course, only has sense if one iterates on a
      /// directed graph
      Node * get_src_node() const
      {
        return static_cast<Node *>(get_current_arc()->src_node);
      }

      Node * get_src_node_ne() const
      {
        return static_cast<Node *>(get_current_arc_ne()->src_node);
      }

      /// Return the target node of current arc. Throw `overflow_error` if
      /// there is no one. Of course, only has sense if one iterates on a
      /// directed graph
      Node * get_tgt_node() const
      {
        return static_cast<Node *>(get_current_arc()->tgt_node);
      }

      Node * get_tgt_node_ne() const
      {
        return static_cast<Node *>(get_current_arc_ne()->tgt_node);
      }
    };

    /// Construct an empty graph
    List_Graph() = default;

    /// Swap in constant time `this` with `g`
    void swap(List_Graph & g) noexcept
    {
      this->common_swap(g);
      node_list.swap(&g.node_list);
      arc_list.swap(&g.arc_list);
    }
  };

  /** Default filter for filtered iterators on arcs.

        The default filtering action is none: that is, all the arcs would
        be shown.

        @ingroup Graphs
    */
  template <class GT>
  struct Dft_Show_Arc
  {
    bool operator()(typename GT::Arc * /* arc */) const noexcept
    {
      return true;
    }

    void set_cookie(void *) noexcept
    { /* empty */
    }
  };

  /** Whether a filter of type SA of the arcs of GT is copied, moved and
      called without exceptions: then an iterator that it filters, and a
      search through one, throw nothing. Else what the filter throws goes
      out of them.

        @ingroup Graphs
    */
  template <class GT, class SA>
  inline constexpr bool nothrow_arc_filter_v =
    std::is_nothrow_copy_constructible_v<SA> and std::is_nothrow_move_constructible_v<SA>
    and std::is_nothrow_invocable_r_v<bool, SA &, typename GT::Arc *>;

  /** \addtogroup Graphs
        @{
        Filtered iterators
        ==================

        The graph classes export several iterators: `Node_Iterator`,
        `Arc_Iterator`, `Node_Arc_Iterator`, `Out_Iterator` and
        `In_Iterator`. These iterators are exported as subclasses of the
        graph class. Suppose, for example, that you are using a class Map
        based on `Array_Graph` class (but it could have perfectly been base
        on any other graph class) and the graph is named `g`. Then, you
        could iterate on all the arcs through some such as:

        for (auto it = g.get_arc_it(); it.has_cur(); it.next())
        auto arc = it.get_curr(); // each arc of the graph is seen

        Now, there are several, perhaps many, situations where not all the
        arcs must be processed, but a subset of them satisfying a particular
        condition. In these cases, the filtered iterator come for us.

        As seen in the class `Filter_Iterator`, a filtered iterator is
        almost exactly the same thing as a normal iterator. The subtle
        difference is that on it operates a filter condition that filters
        all the items satisfying a special condition. Let's show a brief
        but constructive example.

        A brief example
        ----------------

        Suppose that your graph represents pathways of automotive
        transportation. Nodes are the cities and arcs are the pathways. Now
        suppose that your pathways are classified in `Highway`, `Road` and
        `Trail`. Now suppose that you wish to solve a problem that only
        involves highways; for example, you want to find the shortest path
        between two cities connected by only highways. Then you could
        specify a filter that catches highways:

        struct Only_Highway
        {
        bool operator () (GT::Arc * a)
        {
        return a->get_info().type == Highway;
        }
        };

        Now, if your shortest path algorithm receives this filter, this will
        only see highways and its results will only contain highways.

        Convention about filtered iterators
        -----------------------------------

        For each subclass iterator on a graph, it is exported a filtered
        iterator with the same name of subclassed one. Thus, for the
        subclass `GT::Arc_Iterator` exists a filtered version named
        `Arc_Iterator` and so on for the remainder iterator subclasses on
        the graphs.

        The filtered iterators have a great importance in `Aleph-w`
        (\f$\aleph_\omega\f$) algorithms on graphs, since almost every
        algorithm operates with filtered iterator.

        When to use then?
        ----------------

        The short answer is: almost always!

        Filtered iterators have of course a light but constant impact
        on performance. For each seen graph object a boolean predicate is
        tested. So, if you know that your algorithm does not require to
        filter, the use the subclass iterator. But if, in pursuit of
        generalization your algorithm could profit the filtered version,
        then design your algorithm on filtered iterator. In this way you
        will provide generality, which allows to reuse algorithms in a
        transparent way.

        A very good and concrete example of intensive use of filtered
        iterator is for solving the problem of maximizing a network flow at
        minimum cost. First, in order to manage the residual net, the arcs
        are filtered according to source node. Second, the residual cut arcs
        are also filtered according to flow value; if the flow is equal to
        the capacity then the arc is filtered. Third, in order to detect
        negative cycles the Bellman-Ford algorithm is used with a filter
        that only sees the arcs cost.
        @}
    */

  /** Filtered iterator of adjacent arcs of a node.

        `Node_Arc_Iterator` is a reflex class of `GT::Node_Arc_Iterator`
        except that the arcs are filtered according to filter criteria
        `Show_Arc`.

        `Show_Arc` must have the following signature:

        struct Show_Arc
        {
        bool operator () (GT::Arc * arc) { /// the filter condition }
        };

        So, this iterator will only show those arcs satisfying the filter.

        @ingroup Graphs
    */
  template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
  struct Node_Arc_Iterator :
      public Filter_Iterator<typename GT::Node *,
                             typename GT::Node_Arc_Iterator,
                             Show_Arc>
  {
    using Filter_Itor = Filter_Iterator<typename GT::Node *,
                                        typename GT::Node_Arc_Iterator,
                                        Show_Arc>;

    using Itor = Filter_Iterator<typename GT::Node *,
                                 typename GT::Node_Arc_Iterator,
                                 Show_Arc>;

    using Item_Type = typename Itor::Item_Type; /// type of element: Arc*
    using Set_Type = typename Itor::Set_Type; /// Type of set: `p`'s arcs

    Node_Arc_Iterator() = default;

    /** Construct and filtered iterator according to condition `sa`.

          @param[in] p node over whom you wish to iterate
          @param[in] sa the filter
      */
    Node_Arc_Iterator(typename GT::Node *p, Show_Arc sa = Show_Arc())
      : Itor(p, sa)
    {
      // empty
    }
  };

  /** Filtered iterator on all the arcs of a graph

        Arc_Iterator is an iterator similar to `GT::Arc_Iterator`,
        except that only are shown the arcs satisfying a filter
        condition.

        The condition must be specified through a functor whose
        signature must be as follows:

        struct Show_Arc
        {
        bool operator () (GT::Arc * arc) { /// the filter condition }
        };

        @ingroup Graphs
    */
  template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
  struct Arc_Iterator :
      public Filter_Iterator<GT, typename GT::Arc_Iterator, Show_Arc>
  {
    using Itor = Filter_Iterator<GT, typename GT::Arc_Iterator, Show_Arc>;

    using Item_Type = typename Itor::Item_Type; /// Type of element: Arc*
    using Set_Type = typename Itor::Set_Type; /// Type of set all the arcs

    Arc_Iterator() = default;

    /** Constructor,

          @param[in] g the graph
          @param[in] sa filter functor
      */
    Arc_Iterator(const GT & g, Show_Arc sa = Show_Arc())
      : Itor(g, sa)
    {
      // empty
    }
  };

  /** Default filter for the graph nodes.

        The condition is always `true`. So. all the node would be seen.

        @ingroup Graphs
    */
  template <class GT>
  struct Dft_Show_Node
  {
    bool operator()(typename GT::Node *) const noexcept
    {
      return true;
    }
  };

  /** A filter that an algorithm object keeps: one given as an lvalue is
      used where it is, so that the caller sees what the algorithm does to
      it; one given as an rvalue is moved into the keeper, so that it lives
      as long as the object.

      A keeper converts to a reference to its filter. The copy of a keeper
      of a moved filter has a copy of it; that of a keeper of an lvalue
      uses the same lvalue.

      @tparam F Type of the filter.
      @ingroup Graphs
  */
  template <class F>
  class Kept_Filter
  {
    std::optional<F> own; // the filter, if it was given as an rvalue
    F * filter;           // the filter that is used

  public:
    /// Use @p f where it is.
    Kept_Filter(F & f) noexcept : filter(&f) {}

    /// Keep @p f, moved.
    Kept_Filter(F && f) noexcept(std::is_nothrow_move_constructible_v<F>)
      : own(std::move(f)), filter(&*own) {}

    /// Copy a moved filter, or use the same lvalue.
    Kept_Filter(const Kept_Filter & k)
      noexcept(std::is_nothrow_copy_constructible_v<F>)
      : own(k.own), filter(own ? &*own : k.filter) {}

    /// Move a moved filter, or use the same lvalue.
    Kept_Filter(Kept_Filter && k)
      noexcept(std::is_nothrow_move_constructible_v<F>)
      : own(std::move(k.own)), filter(own ? &*own : k.filter) {}

    Kept_Filter & operator = (const Kept_Filter &) = delete;

    /// The filter.
    F & get() const noexcept { return *filter; }

    /// The filter.
    operator F & () const noexcept { return *filter; }
  };

  /** Filtered iterator on the nodes of a graph.

        @ingroup Graphs
    */
  template <class GT, class Show_Node = Dft_Show_Node<GT>>
  class Node_Iterator :
      public Filter_Iterator<GT, typename GT::Node_Iterator, Show_Node>
  {
  public:
    using Itor = Filter_Iterator<GT, typename GT::Node_Iterator, Show_Node>;

    using Item_Type = typename Itor::Item_Type; /// The element type: Node*

    using Set_Type = typename Itor::Set_Type; /// The set: the arcs of a graph

    Node_Iterator() = default;

    /** Construct a iterator with filter `sn`

          @param[in] g the graph
          @param[in] sn the node filter
      */
    Node_Iterator(const GT & g, Show_Node sn = Show_Node())
      : Itor(g, sn)
    {
      /* empty */
    }
  };

  /** Traverse all the nodes of graph filtering some ones according to a
        condition and executing an operation on them.

        @param[in] g the graph
        @param[in] operation to be executed on each seen node
        @param[in] sn a node filter

        @ingroup Graphs
    */
  template <AlephGraph GT, NodeFilter<GT> SN = Dft_Show_Node<GT>, class Op>
    requires std::is_invocable_r_v<void, Op &, typename GT::Node *>
  void for_each_node(const GT & g,
                     Op operation,
                     SN sn = SN())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      Aleph::concepts_detail::invoke_as<void (typename GT::Node *)>::call(operation, it.get_curr());
  }

  /** Traverse all the arcs of graph filtering some ones according to a
        condition and executing an operation on them.

        @param[in] g the graph
        @param[in] operation to be executed on each seen arc
        @param[in] sa an arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<void, Op &, typename GT::Arc *>
  void for_each_arc(const GT & g,
                    Op operation,
                    SA sa = SA())
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      Aleph::concepts_detail::invoke_as<void (typename GT::Arc *)>::call(operation, it.get_curr());
  }

  /** Traverse all the arcs adjacent to a node filtering some ones
        according to a condition and executing an operation on them.

        @param[in] g the graph
        @param[in] p a node pointer
        @param[in] operation to be executed on each seen arc
        @param[in] sa an arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<void, Op &, typename GT::Arc *>
  void for_each_arc(const GT & g,
                    typename GT::Node *p,
                    Op operation,
                    SA sa = SA())
  {
    (void) g;
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      Aleph::concepts_detail::invoke_as<void (typename GT::Arc *)>::call(operation, it.get_curr());
  }

  /** Return `true` if condition `cond` is met on every filtered node of
        the graph.

        @param[in] g the graph
        @param[in] cond condition
        @param[in] sn node filter
        \result `true` if all the nodes satisfy the condition; `false` otherwise

        @ingroup Graphs
    */
  template <AlephGraph GT, NodeFilter<GT> SN = Dft_Show_Node<GT>, class Op>
    requires std::is_invocable_r_v<bool, Op &, typename GT::Node *>
  bool forall_node(const GT & g,
                   Op cond,
                   SN sn = SN())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if (not Aleph::concepts_detail::invoke_as<bool (typename GT::Node *)>::call(cond, it.get_curr()))
        return false;
    return true;
  }

  /** Return `true` if condition `cond` is met on every filtered arc of
        the graph.

        @param[in] g the graph
        @param[in] cond condition
        @param[in] sa arc filter
        \result `true` if all the arcs satisfy the condition; `false` otherwise

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<bool, Op &, typename GT::Arc *>
  bool forall_arc(const GT & g,
                  Op cond,
                  SA sa = SA())
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      if (not Aleph::concepts_detail::invoke_as<bool (typename GT::Arc *)>::call(cond, it.get_curr()))
        return false;
    return true;
  }

  /** Return `true` if the condition `cond` is met on every filtered arc of a
        node.

        @param[in] p node pointer
        @param[in] cond condition
        @param[in] sa arc filter
        \result `true` if all the arcs satisfy the condition; `false` otherwise

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<bool, Op &, typename GT::Arc *>
  bool forall_arc(typename GT::Node *p,
                  Op cond,
                  SA sa = SA())
  {
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      if (not Aleph::concepts_detail::invoke_as<bool (typename GT::Arc *)>::call(cond, it.get_curr()))
        return false;
    return true;
  }

  /** Map the filtered nodes of a graph to a transformed type.

        @tparam GT Graph type
        @tparam T Result type
        @tparam Container Container type (default: DynList)
        @tparam SN Node filter type (default: Dft_Show_Node<GT>)
        @param[in] g the graph
        @param[in] transformation function `Node* -> T` that transforms each node
        @param[in] sn node filter functor (default: Dft_Show_Node<GT>)
        @return a `Container<T>` object with all the nodes mapped through the
        transformation

        @ingroup Graphs
    */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            NodeFilter<GT> SN = Dft_Show_Node<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, typename GT::Node *>
  Container<T> nodes_map(GT & g,
                         Op transformation,
                         SN sn = SN())
  {
    Container<T> ret_val;
    for_each_node<GT, SN>(g, [&ret_val, &transformation](typename GT::Node *p)
                            {
                              ret_val.append(Aleph::concepts_detail::invoke_as<T (typename GT::Node *)>::call(transformation, p));
                            }, sn);
    return ret_val;
  }

  /** \overload
   *
   *  Compatibility overload for callers that pass an actual
   *  `std::function<T (typename GT::Node *)>` (rather than a lambda or other
   *  callable) and rely on `T` being deduced from it, as the generic overload
   *  above cannot: `T` appears only in its return type and `requires`
   *  clause, never in a parameter, so it is not a deduced context there.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            NodeFilter<GT> SN = Dft_Show_Node<GT>>
  Container<T> nodes_map(GT & g,
                         std::function<T (typename GT::Node *)> transformation,
                         SN sn = SN())
  {
    // Forward through a lambda, not `transformation` itself: passing the
    // `std::function` as-is would make this call ambiguous between this
    // overload and the generic one (both exactly match a `std::function<T
    // (typename GT::Node *)>` argument once `Op` is deduced as that type).
    return nodes_map<GT, T, Container, SN>(
        g, [&transformation](typename GT::Node * p) { return transformation(p); }, sn);
  }

  /** Map the filtered arcs of a graph to a transformed type.

        @tparam GT Graph type
        @tparam T Result type
        @tparam Container Container type (default: DynList)
        @tparam SA Arc filter type (default: Dft_Show_Arc<GT>)
        @param g the graph
        @param transformation function `Arc* -> T` that transforms each arc
        @param sa arc filter functor (default: Dft_Show_Arc<GT>)
        @return a `Container<T>` object with all the arcs mapped through the
        transformation
        @ingroup Graphs
    */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, typename GT::Arc *>
  Container<T> arcs_map(GT & g,
                        Op transformation,
                        SA sa = SA())
  {
    Container<T> ret_val;
    for_each_arc<GT, SA>(g, [&ret_val, &transformation](typename GT::Arc *p)
                           {
                             ret_val.append(Aleph::concepts_detail::invoke_as<T (typename GT::Arc *)>::call(transformation, p));
                           }, sa);
    return ret_val;
  }

  /** \overload
   *
   *  Compatibility overload deducing `T` from an actual
   *  `std::function<T (typename GT::Arc *)>` argument; see the equivalent
   *  `nodes_map` overload for why the generic one above cannot deduce it.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Container<T> arcs_map(GT & g,
                        std::function<T (typename GT::Arc *)> transformation,
                        SA sa = SA())
  {
    // See the equivalent nodes_map overload for why this must go through a
    // lambda rather than forwarding `transformation` as-is.
    return arcs_map<GT, T, Container, SA>(
        g, [&transformation](typename GT::Arc * a) { return transformation(a); }, sa);
  }

  /** \overload
   *
   *  Map the filtered arcs of a specific node in a graph to a transformed type.
   *
   *  This overload filters arcs connected to a specific node `p`.
   *
   *  @tparam GT Graph type
   *  @tparam T Result type
   *  @tparam Container Container type (default: DynList)
   *  @tparam SA Arc filter type (default: Dft_Show_Arc<GT>)
   *  @param g the graph
   *  @param p node pointer (arcs connected to this node are filtered)
   *  @param transformation function `Arc* -> T` that transforms each arc
   *  @param sa arc filter functor (default: Dft_Show_Arc<GT>)
   *  @return a `Container<T>` object with all the arcs mapped through the
   *          transformation
   *  @see arcs_map(GT &, std::function<T (typename GT::Arc *)>, SA) For the
   *       version that maps all arcs in the graph.
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, typename GT::Arc *>
  Container<T> arcs_map(GT & g,
                        typename GT::Node *p,
                        Op transformation,
                        SA sa = SA())
  {
    Container<T> ret_val;
    for_each_arc<GT, SA>(g, p, [&ret_val, &transformation](typename GT::Arc *p)
                           {
                             ret_val.append(Aleph::concepts_detail::invoke_as<T (typename GT::Arc *)>::call(transformation, p));
                           }, sa);
    return ret_val;
  }

  /** \overload
   *
   *  Compatibility overload deducing `T` from an actual
   *  `std::function<T (typename GT::Arc *)>` argument; see the equivalent
   *  `nodes_map` overload for why the generic one above cannot deduce it.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            template<typename> class Container = DynList,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Container<T> arcs_map(GT & g,
                        typename GT::Node *p,
                        std::function<T (typename GT::Arc *)> transformation,
                        SA sa = SA())
  {
    // See the equivalent nodes_map overload for why this must go through a
    // lambda rather than forwarding `transformation` as-is.
    return arcs_map<GT, T, Container, SA>(
        g, p, [&transformation](typename GT::Arc * a) { return transformation(a); }, sa);
  }

  /** Fold the filtered nodes of a graph.

        @param[in] g the graph
        @param[in] init initial value of folded result
        @param[in] operation folding operation on the node
        @param[in] sn node filter
        \result the final folded result

        @ingroup Graphs
    */
  template <AlephGraph GT, typename T, NodeFilter<GT> SN = Dft_Show_Node<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Node *>
  T foldl_nodes(GT & g, const T & init,
                Op operation,
                SN sn = SN())
  {
    T ret_val = init;
    for_each_node<GT, SN>(g, [&ret_val, &operation](typename GT::Node *p)
                            {
                              ret_val = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Node *)>::call(operation, ret_val, p);
                            }, sn);
    return ret_val;
  }

  /** Fold the filtered arcs of a graph.

        @param[in] g the graph
        @param[in] init initial value of folded result
        @param[in] operation folding operation on the arc
        @param[in] sa arc filter
        \result the final folded result
        @ingroup Graphs
    */
  template <AlephGraph GT, typename T, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Arc *>
  T foldl_arcs(GT & g, const T & init,
               Op operation,
               SA sa = SA())
  {
    T ret_val = init;
    for_each_arc<GT, SA>(g, [&ret_val, &operation](typename GT::Arc *a)
                           {
                             ret_val = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Arc *)>::call(operation, ret_val, a);
                           }, sa);
    return ret_val;
  }

  /** Fold the filtered adjacent arcs of a node.

        @param[in] g the graph
        @param[in] p node pointer
        @param[in] init initial value of folded result
        @param[in] operation folding operation on the arc
        @param[in] sa arc filter
        \result the final folded result
        @ingroup Graphs
    */
  template <AlephGraph GT, typename T, ArcFilter<GT> SA = Dft_Show_Arc<GT>, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Arc *>
  T foldl_arcs(GT & g, typename GT::Node *p,
               const T & init,
               Op operation,
               SA sa = SA())
  {
    T ret_val = init;
    for_each_arc<GT, SA>(g, p, [&ret_val, &operation](typename GT::Arc *a)
                           {
                             ret_val = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Arc *)>::call(operation, ret_val, a);
                           }, sa);
    return ret_val;
  }

  /** Directed graph (digraph) implemented with double-linked adjacency lists.

      This class models a directed graph. Functionally, it is
      equivalent to the List_Graph class, except that this one
      handles directed graphs.

      This is a type alias for `Digraph<List_Graph<...>>`, which provides
      all the directed graph functionality through the generic Digraph
      template wrapper defined in graph-dry.H.

      @tparam __Graph_Node The node type. Must be defined from
      the Graph_Node class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @tparam __Graph_Arc The arc type. Must be defined from
      the Graph_Arc class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @see Graph_Node Graph_Arc Digraph
      @ingroup Graphs
      @see List_Graph
   */
  template <typename __Graph_Node = Graph_Node<int>,
            typename __Graph_Arc = Graph_Arc<int>>
  using List_Digraph = Digraph<List_Graph<__Graph_Node, __Graph_Arc>>;


  /** Alias used for encapsulating a pair of arc and node (related between
        them).

        @ingroup Graphs
    */
  template <class GT>
  using ArcPair = std::tuple<typename GT::Arc *, typename GT::Node *>;

  /** Filter the outcoming arcs.

        This filter is used by the directed graphs implemented with
        `List_Graph`.

        @ingroup Graphs
    */
  template <class GT>
  class __Out_Filt
  {
    typename GT::Node *src = nullptr;

  public:
    __Out_Filt(typename GT::Node *__src) noexcept : src(__src)
    { /* empty */
    }

    bool operator()(typename GT::Arc *a) const noexcept
    {
      assert(src);
      return a->src_node == src;
    }

    typename GT::Node * get_node(typename GT::Arc *a) const noexcept
    {
      assert(src);
      return static_cast<typename GT::Node *>(a->tgt_node);
    }
  };

  /** Filter the incoming arcs.

        This filter is used by the directed graphs implemented with
        `List_Graph`.

        @ingroup Graphs
    */
  template <class GT>
  class __In_Filt
  {
    typename GT::Node *tgt = nullptr;

  public:
    __In_Filt(typename GT::Node *__tgt) noexcept : tgt(__tgt)
    { /* empty */
    }

    bool operator()(typename GT::Arc *a) const noexcept
    {
      assert(tgt);
      return a->tgt_node == tgt;
    }

    typename GT::Node * get_node(typename GT::Arc *a) const noexcept
    {
      assert(tgt);
      return static_cast<typename GT::Node *>(a->src_node);
    }
  };

  /** Filtered iterator on directed graphs.

        @ingroup Graphs
    */
  template <class GT, class Filter>
  class Digraph_Iterator
  {
    using Itor = Filter_Iterator<typename GT::Node *,
                                 typename GT::Node_Arc_Iterator, Filter>;

    Filter filt;
    Itor it;

  public:
    using Item_Type = typename Itor::Item_Type; /// The type of element

    using Iterator_Type = Itor; /// Iterator type

    /// Whether the iterator moves back: the iterators of the lists of
    /// `List_SGraph` and `List_SDigraph` go only forward, and have no
    /// prev()
    static constexpr bool goes_back = Itor::goes_back;

    /// Whether the iterator goes to its last arc, which reset_last() and
    /// end() need
    static constexpr bool goes_to_last = Itor::goes_to_last;

    /// Initialize an iterator on adjacent arcs (incoming or outcoming) of `p`
    Digraph_Iterator(typename GT::Node *p)
      : filt(p), it(p, filt)
    {
      // empty
    }

    /// Move the iterator one position forward. Throws `overflow_error` it
    /// there is no current
    void next()
    {
      it.next();
    }

    void next_ne() noexcept
    {
      it.next_ne();
    }

    /// Move the iterator one position backward. Throws `overflow_error` if
    /// there is no current. Only where the iterator moves back (goes_back)
    void prev() requires goes_back { it.prev(); }

    /// Return `true` the iterator has an current arc
    [[nodiscard]] bool has_curr() const noexcept
    {
      return it.has_curr();
    }

    /// Return the current arc.  Throws `overflow_error` it there is no
    /// current
    typename GT::Arc * get_curr() const
    {
      return it.get_curr();
    }

    /// Return the current arc.  Throws `overflow_error` it there is no
    /// current
    typename GT::Arc * get_curr_ne() const noexcept
    {
      return it.get_curr_ne();
    }

    /// \overload
    /// Alias for get_curr(). Returns current arc.
    auto get_current_arc() const
    {
      return get_curr();
    }

    /// Return the connected node to `arc`. The result depends on iterator
    /// type
    typename GT::Node * get_node(typename GT::Arc *a) const noexcept
    {
      return filt.get_node(a);
    }

    /// Return the connected node to current arc.
    typename GT::Node * get_node() const
    {
      return this->get_node(this->get_curr());
    }

    /// \overload
    /// Alias for get_node(). Returns target node of current arc.
    auto get_tgt_node() const
    {
      return get_node();
    }

    /// Return the connected node to current arc.
    typename GT::Node * get_node_ne() const noexcept
    {
      return this->get_node(this->get_curr_ne());
    }

    /// \overload
    /// Alias for get_node_ne(). Returns target node of current arc (no exceptions).
    auto get_tgt_node_ne() const noexcept
    {
      return get_node_ne();
    }

    /// Reset the iterator to the first arc
    void reset_first() noexcept
    {
      it.reset_first();
    }

    /// Reset the iterator to the last arc. Only where the iterator goes to
    /// its last arc (goes_to_last)
    void reset_last() noexcept requires goes_to_last
    {
      it.reset_last();
    }

    /// Put the iterator in end state. Only where the iterator goes to its
    /// last arc (goes_to_last)
    void end() noexcept requires goes_to_last
    {
      put_itor_at_the_end(*this);
    }
  };


  /** Basic iterator for outcoming arcs of a node.

        @ingroup Graphs
    */
  template <class GT>
  using _Out_Iterator = Digraph_Iterator<GT, __Out_Filt<GT>>;


  /** Basic iterator for incoming arcs of a node.

        It walks the list of the node, which holds the arcs that enter it
        only in a graph that is not directed, as the networks are: on a
        digraph it sees only the loops of the node. Use `In_Iterator(g, p)`
        there.

        @ingroup Graphs
    */
  template <class GT>
  using _In_Iterator = Digraph_Iterator<GT, __In_Filt<GT>>;

  /** Filtered iterator for outcoming arcs of a node.

      This iterator traverses the outgoing arcs of a node, optionally
      filtered by the Show_Arc predicate.

      @tparam GT The graph type
      @tparam Show_Arc Arc filter predicate (default shows all arcs)
      @ingroup Graphs
    */
  template <class GT, class Show_Arc = Dft_Show_Arc<GT>>
  class Out_Iterator : public Digraph_Iterator<GT, __Out_Filt<GT>>
  {
    using Base = Digraph_Iterator<GT, __Out_Filt<GT>>;
    Show_Arc show_arc;

    void goto_first_valid()
    {
      while (Base::has_curr() and not show_arc(Base::get_curr_ne()))
        Base::next_ne();
    }

    void goto_last_valid()
    {
      while (Base::has_curr() and not show_arc(Base::get_curr_ne()))
        Base::prev();
    }

  public:
    using Item_Type = typename GT::Arc *;

    Out_Iterator() = default;

    Out_Iterator(typename GT::Node *p, Show_Arc sa = Show_Arc())
      : Base(p), show_arc(sa)
    {
      goto_first_valid();
    }

    void next()
    {
      Base::next();
      goto_first_valid();
    }

    /// next() without the check of the current arc; noexcept when the
    /// filter is, else what the filter throws goes out
    void next_ne() noexcept(std::is_nothrow_invocable_r_v<bool, Show_Arc &, typename GT::Arc *>)
    {
      Base::next_ne();
      goto_first_valid();
    }

    /// Move to the previous arc that the filter shows. Only where the
    /// iterator moves back: not on `List_SGraph` and `List_SDigraph`
    void prev() requires Base::goes_back
    {
      Base::prev();
      goto_last_valid();
    }

    /// Put the iterator on the first arc that the filter shows
    void reset_first()
    {
      Base::reset_first();
      goto_first_valid();
    }

    /// Put the iterator on the last arc that the filter shows. Only where
    /// the iterator goes to its last arc: not on `List_SGraph` and
    /// `List_SDigraph`
    void reset_last() requires Base::goes_to_last
    {
      Base::reset_last();
      goto_last_valid();
    }
  };

  /** Filtered iterator for incoming arcs of a node.

      This iterator traverses the incoming arcs of a node, those whose
      target is the node, optionally filtered by the Show_Arc predicate.

      A digraph keeps each arc only in the list of its source, while a
      graph that is not directed keeps it in the lists of both ends, so
      that the iterator has two forms:

      - `In_Iterator(g, p, sa)` takes the graph. On a digraph type
        (`DigraphType`) it walks the arcs of `g` and keeps those whose
        target is `p` and that `sa` shows, in the order of the arc
        iterator of `g`: to build it and go through all of them takes
        \f$O(E)\f$. On a graph that is not directed it walks the list of
        `p`, as the other form; it throws `domain_error` on a graph of an
        undirected type that says that it is directed (see
        `set_digraph()`), whose arcs into `p` are not in the list of `p`.
      - `In_Iterator(p, sa)` walks the list of `p`. It does not compile on
        a digraph type, whose list of `p` holds only the arcs that leave
        `p`. It cannot tell a graph of an undirected type that says that
        it is directed, on which it sees only the loops of `p`.

      The functions on the arcs into a node that take the graph, as
      `in_arcs(g, p)` and `in_degree(g, p)`, tell a digraph at run time,
      also of an undirected type. To go through the arcs into many nodes
      of a digraph, build an `In_Arcs_Index` once, in \f$O(V + E)\f$.

      prev() and reset_last() exist only where the iterators of the graph
      move back: not on `List_SGraph` and `List_SDigraph`.

      @tparam GT The graph type
      @tparam SA Arc filter predicate (default shows all arcs)
      @see In_Arcs_Index DigraphType
      @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class In_Iterator : public Digraph_Iterator<GT, __In_Filt<GT>>
  {
    using Base = Digraph_Iterator<GT, __In_Filt<GT>>;
    SA show_arc;

    void goto_first_valid()
    {
      while (Base::has_curr() and not show_arc(Base::get_curr_ne()))
        Base::next_ne();
    }

    void goto_last_valid()
    {
      while (Base::has_curr() and not show_arc(Base::get_curr_ne()))
        Base::prev();
    }

    // Throws the domain_error of In_Iterator(g, p) on a graph of an
    // undirected type that says that it is directed; out of line, so that
    // the iterator on a graph that is not directed costs only the test
    [[gnu::noinline, gnu::cold]] static void refuse_a_digraph()
    {
      ah_domain_error()
        << "In_Iterator(g, p): the graph says that it is directed, but its type does not, "
        << "so that its arcs into p are not in the list of p: use in_arcs(g, p), or "
        << "In_Arcs_Index";
    }

    // The node of a graph that is not directed, whose list holds the arcs
    // into it
    static typename GT::Node * of_lists(const GT & g, typename GT::Node *p)
    {
      if (g.is_digraph()) [[unlikely]]
        refuse_a_digraph();
      return p;
    }

  public:
    using Item_Type = typename GT::Arc *;

    In_Iterator() = default;

    /** Iterator on the arcs of the list of `p` whose target is `p` and that
        `sa` shows. The list holds the arcs into `p` only in a graph that
        is not directed: on a digraph type it does not compile.

        @param[in] p the node.
        @param[in] sa the arc filter.
    */
    In_Iterator(typename GT::Node *p, SA sa = SA())
      : Base(p), show_arc(sa)
    {
      goto_first_valid();
    }

    /** Iterator on the arcs of `g` into `p` that `sa` shows; on a graph of
        an undirected type, those of the list of `p`.

        It is inlined, as the form that takes only the node is, so that it
        costs on a graph that is not directed only the test of
        `g.is_digraph()`.

        @param[in] g the graph of `p`.
        @param[in] p the node.
        @param[in] sa the arc filter.
        @throw domain_error if `g` says that it is directed.
    */
    [[gnu::always_inline]] In_Iterator(const GT & g, typename GT::Node *p, SA sa = SA())
      : In_Iterator(of_lists(g, p), std::move(sa))
    {
      // empty
    }

    void next()
    {
      Base::next();
      goto_first_valid();
    }

    /// next() without the check of the current arc; noexcept when the
    /// filter is, else what the filter throws goes out
    void next_ne() noexcept(std::is_nothrow_invocable_r_v<bool, SA &, typename GT::Arc *>)
    {
      Base::next_ne();
      goto_first_valid();
    }

    /// Move to the previous arc that the filter shows. Only where the
    /// iterator moves back: not on `List_SGraph`
    void prev() requires Base::goes_back
    {
      Base::prev();
      goto_last_valid();
    }

    /// Put the iterator on the first arc that the filter shows
    void reset_first()
    {
      Base::reset_first();
      goto_first_valid();
    }

    /// Put the iterator on the last arc that the filter shows. Only where
    /// the iterator goes to its last arc: not on `List_SGraph`
    void reset_last() requires Base::goes_to_last
    {
      Base::reset_last();
      goto_last_valid();
    }
  };

  /** Filtered iterator for incoming arcs of a node of a digraph type.

      A digraph keeps each arc only in the list of its source: the
      iterator walks the arcs of the graph, in the order of its arc
      iterator, and keeps those whose target is the node and that the
      filter shows; to build it and go through all of them takes
      \f$O(E)\f$. `In_Iterator(g, p, sa)` builds it; `In_Iterator(p, sa)`,
      which would walk only the list of `p`, does not compile.

      @tparam GT The graph type, built with the `Digraph` template
      @tparam SA Arc filter predicate
      @see In_Arcs_Index
      @ingroup Graphs
    */
  template <class GT, class SA>
    requires DigraphType<GT>
  class In_Iterator<GT, SA> : public graph_dry_detail::In_Arcs_Walk<GT, SA>
  {
    using Base = graph_dry_detail::In_Arcs_Walk<GT, SA>;

  public:
    using Base::Base;
  };

  /** Index of the arcs that go into each node of a graph.

      A digraph keeps each arc only in the list of its source, so that
      `In_Iterator(g, p)` and `in_degree(g, p)` walk all the arcs of the
      graph for each node. `In_Arcs_Index` walks them once, in
      \f$O(V + E)\f$ expected, and keeps for each node the arcs whose
      target is the node and that the filter shows, in the order of the
      arc iterator of the graph; a loop is one arc into its node. Then
      `in_degree(p)` takes \f$O(1)\f$, and the iterator of `get_it(p)`
      \f$O(1)\f$ per arc, besides finding `p` in a table, \f$O(1)\f$
      expected.

      The index reads the target of each arc, so that it works on every
      kind of graph, directed or not, and on a graph of an undirected type
      that says that it is directed (see `set_digraph()`): on a graph that
      is not directed it gives the arcs that `In_Iterator(p)` gives. It
      does not use the cookies or the counters of the nodes. It is a
      picture of the graph when it is built: it does not see the arcs
      inserted or removed later, and it must not be used once the nodes
      or the arcs that it has are gone.

      Example:

          In_Arcs_Index<GT> in(g);
          for (auto it = in.get_it(p); it.has_curr(); it.next_ne())
            {
              auto a = it.get_curr_ne(); // an arc into p
              auto q = it.get_node_ne(); // its source
            }

      @tparam GT the graph type.
      @tparam SA the arc filter, which the index uses only while it is
      built.
      @see In_Iterator
      @ingroup Graphs
  */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class In_Arcs_Index
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    // The nodes in an open table, by their addresses, with linear probing,
    // and the number of each one; the table has 2^bits places, at least
    // twice the number of nodes
    unsigned bits = 1;
    Array<Node *> keys;    // nullptr: an empty place
    Array<size_t> number;

    // The arcs into the node numbered i are arcs(first(i)) to
    // arcs(first(i + 1) - 1)
    Array<size_t> first;
    Array<Arc *> arcs;

    static unsigned table_bits(const size_t n) noexcept
    {
      unsigned b = 1;
      while ((size_t(1) << b) < 2 * n)
        ++b;
      return b;
    }

    [[nodiscard]] size_t place_of(const Node *p) const noexcept
    {
      const auto key = std::uint64_t(reinterpret_cast<std::uintptr_t>(p));
      return size_t((key * 0x9E3779B97F4A7C15ULL) >> (64 - bits));
    }

    [[nodiscard]] size_t mask() const noexcept { return (size_t(1) << bits) - 1; }

    // The number of p, or the number of nodes if p is not of the graph
    [[nodiscard]] size_t number_of(const Node *p) const noexcept
    {
      for (size_t i = place_of(p); keys(i) != nullptr; i = (i + 1) & mask())
        if (keys(i) == p)
          return number(i);
      return first.size() - 1;
    }

    [[nodiscard]] size_t checked_number_of(const Node *p) const
    {
      const size_t i = number_of(p);
      ah_domain_error_if(i == first.size() - 1)
        << "In_Arcs_Index: the node is not of the indexed graph";
      return i;
    }

  public:
    /** Iterator on the arcs into a node, in the order of the arc iterator
        of the graph when the index was built. It is valid while its index
        lives.
    */
    class Iterator
    {
      Arc * const *arcs = nullptr;
      long num = 0;
      long pos = 0; // the position of the current arc

    public:
      /// The type of the items: arcs.
      using Item_Type = Arc *;

      /// An iterator without arcs.
      Iterator() noexcept = default;

      /// An iterator on the n arcs from a.
      Iterator(Arc * const *a, const long n) noexcept : arcs(a), num(n) {}

      /** An iterator on the arcs into `p` of `index`.

          @param[in] index the index.
          @param[in] p a node of the graph of the index.
          @throw domain_error if `p` was not a node of the graph.
      */
      Iterator(const In_Arcs_Index & index, const Node *p) : Iterator(index.get_it(p)) {}

      /// Return true if the iterator is on an arc.
      [[nodiscard]] bool has_curr() const noexcept { return pos >= 0 and pos < num; }

      /// Return the current arc, which must exist.
      [[nodiscard]] Arc * get_curr_ne() const noexcept { return arcs[pos]; }

      /** Return the current arc.

          @throw underflow_error if the iterator is before the first arc.
          @throw overflow_error if the iterator is past the last arc.
      */
      [[nodiscard]] Arc * get_curr() const
      {
        ah_underflow_error_if(pos < 0) << "In_Arcs_Index::Iterator: no current arc";
        ah_overflow_error_if(pos >= num) << "In_Arcs_Index::Iterator: no current arc";
        return get_curr_ne();
      }

      /// \overload get_curr_ne()
      [[nodiscard]] Arc * get_current_arc_ne() const noexcept { return get_curr_ne(); }

      /// \overload get_curr()
      [[nodiscard]] Arc * get_current_arc() const { return get_curr(); }

      /// Return the source of the current arc, which must exist.
      [[nodiscard]] Node * get_node_ne() const noexcept
      {
        return static_cast<Node *>(get_curr_ne()->src_node);
      }

      /// Return the source of the current arc. Throws as get_curr().
      [[nodiscard]] Node * get_node() const
      {
        return static_cast<Node *>(get_curr()->src_node);
      }

      /// Move to the next arc; the iterator must be on an arc.
      void next_ne() noexcept { ++pos; }

      /** Move to the next arc.

          @throw overflow_error if the iterator is not on an arc.
      */
      void next()
      {
        ah_overflow_error_if(not has_curr()) << "In_Arcs_Index::Iterator: no current arc";
        next_ne();
      }

      /// Move to the previous arc; the iterator must be on an arc.
      void prev_ne() noexcept { --pos; }

      /** Move to the previous arc.

          @throw underflow_error if the iterator is not on an arc.
      */
      void prev()
      {
        ah_underflow_error_if(not has_curr()) << "In_Arcs_Index::Iterator: no current arc";
        prev_ne();
      }

      /// Put the iterator on the first arc.
      void reset_first() noexcept { pos = 0; }

      /// Put the iterator on the last arc.
      void reset_last() noexcept { pos = num - 1; }

      /// Put the iterator past the last arc.
      void end() noexcept { pos = num; }

      /// Return the number of arcs of the iterator.
      [[nodiscard]] size_t size() const noexcept { return size_t(num); }
    };

    /** Index the arcs of `g` into each node that `sa` shows.

        @param[in] g the graph.
        @param[in] sa the arc filter, called once on each arc of `g`.
        @throw bad_alloc if there is no memory for the index.
        @throw anything that `sa` throws.
    */
    In_Arcs_Index(const GT & g, SA sa = SA())
      : bits(table_bits(g.get_num_nodes())),
        keys(size_t(1) << bits, nullptr),
        number(Array<size_t>::create(size_t(1) << bits)),
        first(g.get_num_nodes() + 1, 0),
        arcs(Array<Arc *>::create_reserved(g.get_num_arcs()))
    {
      size_t n = 0;
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne(), ++n)
        {
          Node *p = it.get_curr_ne();
          size_t i = place_of(p);
          while (keys(i) != nullptr)
            i = (i + 1) & mask();
          keys(i) = p;
          number(i) = n;
        }

      // The arcs that sa shows, in the order of the arcs of g, and the
      // number of the target of each; first(i) counts the arcs into i
      const size_t m = g.get_num_arcs();
      auto order = Array<Arc *>::create(m);
      auto target = Array<size_t>::create(m);
      size_t k = 0;
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        if (Arc *a = it.get_curr_ne(); sa(a))
          {
            const size_t t = number_of(static_cast<Node *>(a->tgt_node));
            assert(t < n);
            order(k) = a;
            target(k++) = t;
            ++first(t);
          }

      // first(i) is now past the arcs into i, and goes back to the first
      // of them as they are put, from the last one, in their order
      for (size_t i = 1; i < first.size(); ++i)
        first(i) += first(i - 1);
      arcs.putn(k);
      for (size_t j = k; j-- > 0; )
        arcs(--first(target(j))) = order(j);
    }

    /// Return the number of arcs of the index: those that the filter
    /// showed.
    [[nodiscard]] size_t size() const noexcept { return arcs.size(); }

    /** Return the number of arcs into `p` that the filter showed.

        @param[in] p a node of the graph.
        @throw domain_error if `p` was not a node of the graph.
    */
    [[nodiscard]] size_t in_degree(const Node *p) const
    {
      const size_t i = checked_number_of(p);
      return first(i + 1) - first(i);
    }

    /** Return an iterator on the arcs into `p` that the filter showed.

        @param[in] p a node of the graph.
        @throw domain_error if `p` was not a node of the graph.
    */
    [[nodiscard]] Iterator get_it(const Node *p) const
    {
      const size_t i = checked_number_of(p);
      const size_t b = first(i), e = first(i + 1);
      return Iterator(b == e ? nullptr : &arcs.base() + b, long(e - b));
    }
  };

  namespace in_arcs_detail
  {
    /** @brief Calls `op(a)` on each arc `a` of the digraph `g` into `p`
     *  that `sa` shows, while it returns true, and tells whether it went
     *  through them all.
     *
     *  A digraph keeps each arc only in the list of its source: this walks
     *  the arcs of the graph, in \f$O(E)\f$. Out of line, and with `op` by
     *  value, a lambda that refers to what it changes, so that the forms
     *  that take the graph cost on a graph that is not directed only the
     *  test of `g.is_digraph()`.
     */
    template <AlephGraph GT, class SA, class Op>
    [[gnu::noinline]] bool traverse_digraph_arcs_into(const GT & g, typename GT::Node *p,
                                                      SA & sa, Op op)
    {
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        if (typename GT::Arc *a = it.get_curr_ne(); a->tgt_node == p and sa(a) and not op(a))
          return false;
      return true;
    }

    /** @brief The number of arcs of the digraph `g` into `p` that `sa`
     *  shows, among the arcs of the graph, in \f$O(E)\f$; out of line, as
     *  traverse_digraph_arcs_into().
     */
    template <AlephGraph GT, class SA>
    [[gnu::noinline]] size_t digraph_in_degree(const GT & g, typename GT::Node *p, SA & sa)
    {
      size_t count = 0;
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        if (typename GT::Arc *a = it.get_curr_ne(); a->tgt_node == p and sa(a))
          ++count;
      return count;
    }

    /** @brief Calls `op(a)` on each arc `a` of `g` into `p` that `sa`
     *  shows, while it returns true, and tells whether it went through
     *  them all.
     *
     *  On a digraph, which it tells at run time, it walks the arcs of the
     *  graph, in \f$O(E)\f$; on a graph that is not directed, the list of
     *  `p`.
     */
    template <AlephGraph GT, class SA, class Op>
    bool traverse_arcs_into(const GT & g, typename GT::Node *p, SA & sa, Op op)
    {
      if constexpr (not DigraphType<GT>)
        if (not g.is_digraph())
          {
            for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
              if (not op(it.get_curr_ne()))
                return false;
            return true;
          }
      return traverse_digraph_arcs_into(g, p, sa, op);
    }
  } // namespace in_arcs_detail

  /** Return the nodes connected to the filtered outcoming arcs of `p`.

        @param[in] p node pointer
        @param[in] sa arc filter
        \result A `DynList<typename GT::Node*>` with the pointer to the
        outcoming nodes

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Node *> out_nodes(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Node *> ret;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_node_ne());
    return ret;
  }

  /** Return the nodes connected to the filtered incoming arcs to `p`.

        It reads the arcs into `p` from the list of `p`, as
        `In_Iterator(p)`, which holds them only in a graph that is not
        directed: it does not compile on a digraph type, and it cannot
        tell a graph of an undirected type that says that it is directed.
        Use `in_nodes(g, p, sa)` there.

        @param[in] p node pointer
        @param[in] sa arc filter
        \result A `DynList<typename GT::Node*>` with the pointer to the
        incoming nodes
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Node *> in_nodes(typename GT::Node *p, SA sa = SA())
  {
    static_assert(not DigraphType<GT>,
                  "in_nodes(p) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use in_nodes(g, p)");
    DynList<typename GT::Node *> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_node_ne());
    return ret;
  }

  /** Return the sources of the arcs of `g` into `p` that `sa` shows.

        On a digraph, which keeps each arc only in the list of its source
        and which the function tells at run time, it looks for them among
        the arcs of the graph, in \f$O(E)\f$; on a graph that is not
        directed, in the list of `p`.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] sa arc filter
        \result A `DynList<typename GT::Node*>` with the pointer to the
        incoming nodes
        @throw bad_alloc if there is no memory for the list
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Node *> in_nodes(const GT & g, typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Node *> ret;
    in_arcs_detail::traverse_arcs_into(g, p, sa, [&ret] (typename GT::Arc *a)
      {
        ret.append(static_cast<typename GT::Node *>(a->src_node));
        return true;
      });
    return ret;
  }

  /** Return the filtered incoming arcs of `p`.

        @param[in] p node pointer
        @param[in] sa arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> out_arcs(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return the filtered incoming arcs of `p`.

        It reads them from the list of `p`, as `In_Iterator(p)`: it does
        not compile on a digraph type; use `in_arcs(g, p, sa)` there.

        @param[in] p node pointer
        @param[in] sa arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> in_arcs(typename GT::Node *p, SA sa = SA())
  {
    static_assert(not DigraphType<GT>,
                  "in_arcs(p) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use in_arcs(g, p)");
    DynList<typename GT::Arc *> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return the arcs of `g` into `p` that `sa` shows.

        On a digraph, which the function tells at run time, it looks for
        them among the arcs of the graph, in \f$O(E)\f$, in their order;
        on a graph that is not directed, in the list of `p`.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] sa arc filter
        @throw bad_alloc if there is no memory for the list
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> in_arcs(const GT & g, typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    in_arcs_detail::traverse_arcs_into(g, p, sa, [&ret] (typename GT::Arc *a)
      {
        ret.append(a);
        return true;
      });
    return ret;
  }

  /** Return all the filtered arcs related to `p`

        @param[in] p node pointer
        @param[in] sa arc filter
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> arcs(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return the filtered incoming pairs of (arc,node) related to node `p`

        It reads the arcs from the list of `p`, as `In_Iterator(p)`: it
        does not compile on a digraph type; use `in_pairs(g, p, sa)` there.

        @param[in] p node pointer
        @param[in] sa arc filter
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<ArcPair<GT>> in_pairs(typename GT::Node *p, SA sa = SA())
  {
    static_assert(not DigraphType<GT>,
                  "in_pairs(p) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use in_pairs(g, p)");
    DynList<ArcPair<GT>> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        ret.append(std::make_tuple(a, static_cast<typename GT::Node *>(a->get_connected_node(p))));
      }
    return ret;
  }

  /** Return the pairs (arc, source) of the arcs of `g` into `p` that `sa`
        shows.

        On a digraph, which the function tells at run time, it looks for
        the arcs among those of the graph, in \f$O(E)\f$; on a graph that
        is not directed, in the list of `p`.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] sa arc filter
        @throw bad_alloc if there is no memory for the list
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<ArcPair<GT>> in_pairs(const GT & g, typename GT::Node *p, SA sa = SA())
  {
    DynList<ArcPair<GT>> ret;
    in_arcs_detail::traverse_arcs_into(g, p, sa, [&ret, p] (typename GT::Arc *a)
      {
        ret.append(std::make_tuple(a, static_cast<typename GT::Node *>(a->get_connected_node(p))));
        return true;
      });
    return ret;
  }

  /** Return the filtered outcoming pairs of (arc,node) related to node `p`

        @param[in] p node pointer
        @param[in] sa arc filter
        \result a `DynList<ArcPair<GT>>` containing the outcoming pairs
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<ArcPair<GT>> out_pairs(typename GT::Node *p, SA sa = SA())
  {
    DynList<ArcPair<GT>> ret;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        ret.append(std::make_tuple(a, static_cast<typename GT::Node *>(a->get_connected_node(p))));
      }
    return ret;
  }

  /** Compute the filtered in degree of node `p`.

        \note This function computes the degree, it does not retrieve it.

        It reads the arcs into `p` from the list of `p`, as
        `In_Iterator(p)`, which holds them only in a graph that is not
        directed: it does not compile on a digraph type, and it cannot
        tell a graph of an undirected type that says that it is directed.
        Use `in_degree(g, p, sa)` there.

        @param[in] p node pointer
        @param[in] sa arc filter (by default all the arcs are counted)
        \result the incoming degree
        @note `in_degree<GT>(p)` counts every incoming arc.
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  size_t in_degree(typename GT::Node *p, SA sa = SA())
  {
    static_assert(not DigraphType<GT>,
                  "in_degree(p) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use in_degree(g, p), or In_Arcs_Index "
                  "for many nodes");
    size_t count = 0;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  /** Compute the number of arcs of `g` into `p` that `sa` shows.

        On a digraph, which keeps each arc only in the list of its source
        and which the function tells at run time, it counts the arcs of the
        graph whose target is `p`, in \f$O(E)\f$; on a graph that is not
        directed, those of the list of `p`. To ask for many nodes of a
        digraph, build an `In_Arcs_Index`, whose `in_degree()` takes
        \f$O(1)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] sa arc filter (by default all the arcs are counted)
        \result the incoming degree
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  size_t in_degree(const GT & g, typename GT::Node *p, SA sa = SA())
  {
    if constexpr (not DigraphType<GT>)
      if (not g.is_digraph())
        {
          size_t count = 0;
          for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
            ++count;
          return count;
        }
    return in_arcs_detail::digraph_in_degree(g, p, sa);
  }

  /** Compute the filtered out degree of node `p`

        \note This function computes the degree, it does not retrieve it.

        @param[in] p node pointer
        @param[in] sa arc filter (by default all the arcs are counted)
        \result the outcoming degree
        @note `out_degree<GT>(p)` counts every outgoing arc.

        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  size_t out_degree(typename GT::Node *p, SA sa = SA())
  {
    size_t count = 0;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  /** Generic arcs traverse of a node.

        This template function receives threes template parameters:

        1. `GT`: the graph type.
        2. `Itor`: the iterator type; it is intended to be `_In_Iterator`
        or `_Out_Iterator`.
        3. `Operation`: an operation to be executed on each arc. This
        operation must have the following signature:

        bool operation(typename GT::Arc * arc)

        If `operation` returns `true` then the traversal continues to the
        next arc; otherwise the traversal stops and the result of
        `traverse()` is `false`. If all the arcs are traversed, then the
        result is `true`.

        @param[in] p node pointer
        @param[in] op operation to be performed on each arc
        \result `true` if all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <AlephGraph GT, class Itor, class Operation>
  inline
  bool traverse_arcs(typename GT::Node *p, Operation op = Operation())
  {
    for (Itor it(p); it.has_curr(); it.next_ne())
      if (not op(it.get_curr()))
        return false;
    return true;
  }

  /** Execute an operation on each arc of a node.

        This template function receives threes template parameters:

        1. `GT`: the graph type.
        2. `Itor`: the iterator type; it is intended to be `_In_Iterator`
        or `_Out_Iterator`.
        3. `Operation`: an operation to be executed on each arc. This
        operation must have the following signature:

        void operation(typename GT::Arc * arc)

        @param[in] p node pointer
        @param[in] op operation to be performed on each arc

        @ingroup Graphs
    */
  template <AlephGraph GT, class Itor, class Operation>
  inline
  void for_each_arc(typename GT::Node *p, Operation op = Operation())
  {
    for (Itor it(p); it.has_curr(); it.next_ne())
      op(it.get_curr());
  }


  // Functional operations on input arcs

  // The forms that take only the node read the arcs into it from its
  // list, which holds them only in a graph that is not directed: they do
  // not compile on a digraph type, and they cannot tell a graph of an
  // undirected type that says that it is directed. The forms that take the
  // graph tell a digraph at run time, and look for its arcs into the node
  // among the arcs of the graph, in O(E).

  /** Conditioned traversal of incoming arcs of a node.

        It does not compile on a digraph type: use traverse_in_arcs(g, p, op).

        @param[in] p node pointer
        @param[in] op operation whose result must be `bool`. If the result
        is `false`, then the traversal stops and the traverse returns
        `false`; otherwise, all the arcs are traversed and it returns `true`
        \result `true` if all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  bool traverse_in_arcs(typename GT::Node *p, Op op = Op())
  {
    static_assert(not DigraphType<GT>,
                  "traverse_in_arcs(p, op) does not see the arcs into p on a digraph, whose list "
                  "of p holds only the arcs that leave p: use traverse_in_arcs(g, p, op)");
    return traverse_arcs<GT, _In_Iterator<GT>, Op>(p, op);
  }

  /** Conditioned traversal of the arcs of `g` into `p`.

        On a digraph, which the function tells at run time, the arcs are
        looked for among those of the graph, in \f$O(E)\f$; on a graph that
        is not directed, in the list of `p`.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op operation whose result must be `bool`. If it is
        `false`, then the traversal stops and returns `false`
        \result `true` if all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  bool traverse_in_arcs(const GT & g, typename GT::Node *p, Op op = Op())
  {
    Dft_Show_Arc<GT> all;
    return in_arcs_detail::traverse_arcs_into(g, p, all, [&op] (typename GT::Arc *a) -> bool
      {
        return op(a);
      });
  }

  /** Traverse the incoming arcs of a node and executes an operation

        It does not compile on a digraph type: use for_each_in_arc(g, p, op).

        @param[in] p node pointer
        @param[in] op operation to perform on each node
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  void for_each_in_arc(typename GT::Node *p, Op op = Op())
  {
    static_assert(not DigraphType<GT>,
                  "for_each_in_arc(p, op) does not see the arcs into p on a digraph, whose list "
                  "of p holds only the arcs that leave p: use for_each_in_arc(g, p, op)");
    for_each_arc<GT, _In_Iterator<GT>, Op>(p, op);
  }

  /** Executes an operation on each arc of `g` into `p`: on a digraph,
        which the function tells at run time, among the arcs of the graph,
        in \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op operation to perform on each arc
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  void for_each_in_arc(const GT & g, typename GT::Node *p, Op op = Op())
  {
    Dft_Show_Arc<GT> all;
    in_arcs_detail::traverse_arcs_into(g, p, all, [&op] (typename GT::Arc *a)
      {
        op(a);
        return true;
      });
  }

  /** Test if the incoming arcs meet a condition.

        `op` must have the following signature:

        bool op(Arc * arc)

        It does not compile on a digraph type: use all_in_arc(g, p, op).

        @param[in] p node pointer
        @param[in] op testing operation
        \result `true` if `op` returns `true` for all the incoming arcs
    */
  template <AlephGraph GT, class Op>
  inline
  bool all_in_arc(typename GT::Node *p, Op op = Op())
  {
    static_assert(not DigraphType<GT>,
                  "all_in_arc(p, op) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use all_in_arc(g, p, op)");
    return traverse_in_arcs<GT>(p, [&op](auto a)
                              {
                                return op(a);
                              });
  }

  /** Test if the arcs of `g` into `p` meet a condition; on a digraph, in
        \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op testing operation
        \result `true` if `op` returns `true` for all the incoming arcs
    */
  template <AlephGraph GT, class Op>
  inline
  bool all_in_arc(const GT & g, typename GT::Node *p, Op op = Op())
  {
    return traverse_in_arcs(g, p, [&op](auto a)
                            {
                              return op(a);
                            });
  }

  /** Test if it exists an incoming arc satisfying an operation.

        `op` must have the following signature:

        bool op(Arc * arc)

        It does not compile on a digraph type: use exists_in_arc(g, p, op).

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result `true` if there is at least an arc satisfying the operation
    */
  template <AlephGraph GT, class Op>
  inline
  bool exists_in_arc(typename GT::Node *p, Op op = Op())
  {
    static_assert(not DigraphType<GT>,
                  "exists_in_arc(p, op) does not see the arcs into p on a digraph, whose list of "
                  "p holds only the arcs that leave p: use exists_in_arc(g, p, op)");
    return not traverse_in_arcs<GT>(p, [&op](auto a)
                                          {
                                            return not op(a);
                                          });
  }

  /** Test if an arc of `g` into `p` satisfies an operation; on a digraph,
        in \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result `true` if there is at least an arc satisfying the operation
    */
  template <AlephGraph GT, class Op>
  inline
  bool exists_in_arc(const GT & g, typename GT::Node *p, Op op = Op())
  {
    return not traverse_in_arcs(g, p, [&op](auto a)
                                {
                                  return not op(a);
                                });
  }

  /** Search an incoming arc to a node satisfying a condition `op`.

        `op` must have the following signature:

        bool op(Arc * arc)

        It does not compile on a digraph type: use search_in_arc(g, p, op).

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result a pointer to a node satisfying the operation if this is
        found; `nullptr` otherwise
    */
  template <AlephGraph GT, class Op>
  inline
  auto search_in_arc(typename GT::Node *p, Op op = Op())
  {
    static_assert(not DigraphType<GT>,
                  "search_in_arc(p, op) does not see the arcs into p on a digraph, whose list of "
                  "p holds only the arcs that leave p: use search_in_arc(g, p, op)");
    typename GT::Arc *ret = nullptr;
    traverse_in_arcs<GT>(p, [&op, &ret](auto a)
                       {
                         if (op(a))
                           {
                             ret = a;
                             return false;
                           }
                         return true;
                       });
    return ret;
  }

  /** Search an arc of `g` into `p` that satisfies `op`; on a digraph, in
        \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result the first arc that satisfies `op`, or `nullptr`
    */
  template <AlephGraph GT, class Op>
  inline
  auto search_in_arc(const GT & g, typename GT::Node *p, Op op = Op())
  {
    typename GT::Arc *ret = nullptr;
    traverse_in_arcs(g, p, [&op, &ret](auto a)
                     {
                       if (op(a))
                         {
                           ret = a;
                           return false;
                         }
                       return true;
                     });
    return ret;
  }

  /** Map the incoming arcs to a transformation,

        The transformation is called `op` and must have the following
        signature:

        T op(Arc * arc)

        Where `T` is the transformed type from the arc. It does not compile
        on a digraph type: use map_in_arcs<GT, T>(g, p, op).

        @param[in] p node pointer
        @param[in] op transformation
        \result a `DynList<T>` containing the mapping.
    */
  // An actual std::function<T (Arc *)> goes to the overload below: with T
  // given, the two took it, and the call was ambiguous, since GT, only in
  // the types of the node and of the arc, does not order them
  template <AlephGraph GT, typename T, class Op>
    requires (std::is_invocable_r_v<T, Op &, typename GT::Arc *>
              and not std::is_same_v<Op, std::function<T (typename GT::Arc *)>>)
  inline
  auto map_in_arcs(typename GT::Node *p, Op op)
  {
    static_assert(not DigraphType<GT>,
                  "map_in_arcs(p, op) does not see the arcs into p on a digraph, whose list of p "
                  "holds only the arcs that leave p: use map_in_arcs(g, p, op)");
    DynList<T> ret;
    for_each_in_arc<GT>(p, [&ret, &op](auto a)
                      {
                        ret.append(Aleph::concepts_detail::invoke_as<T (typename GT::Arc *)>::call(op, a));
                      });
    return ret;
  }

  /** \overload
   *
   *  Compatibility overload deducing `T` from an actual
   *  `std::function<T (typename GT::Arc *)>` argument; see the equivalent
   *  `nodes_map` overload for why the generic one above cannot deduce it.
   *  Its third template parameter, which only that type satisfies, keeps
   *  the call that names it, `map_in_arcs<GT, T, std::function<T (Arc *)>>`,
   *  which the generic one took before it left that type to this overload.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            class Op = std::function<T (typename GT::Arc *)>>
    requires std::is_same_v<Op, std::function<T (typename GT::Arc *)>>
  inline
  auto map_in_arcs(typename GT::Node *p, std::function<T (typename GT::Arc *)> op)
  {
    // See the equivalent nodes_map overload for why this must go through a
    // lambda rather than forwarding `op` as-is.
    return map_in_arcs<GT, T>(p, [&op](typename GT::Arc * a) { return op(a); });
  }

  /** Map the arcs of `g` into `p` to a transformation `op`, which must
        have the signature `T op(Arc * arc)`; on a digraph, in \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] op transformation
        \result a `DynList<T>` containing the mapping.
    */
  template <AlephGraph GT, typename T, class Op>
    requires std::is_invocable_r_v<T, Op &, typename GT::Arc *>
  inline
  auto map_in_arcs(const GT & g, typename GT::Node *p, Op op)
  {
    using Call = Aleph::concepts_detail::invoke_as<T (typename GT::Arc *)>;
    DynList<T> ret;
    for_each_in_arc(g, p, [&ret, &op](auto a)
                    {
                      ret.append(Call::call(op, a));
                    });
    return ret;
  }

  /** \overload
   *
   *  Compatibility overload deducing `T` from an actual
   *  `std::function<T (typename GT::Arc *)>` argument.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T>
  inline
  auto map_in_arcs(const GT & g, typename GT::Node *p, std::function<T (typename GT::Arc *)> op)
  {
    return map_in_arcs<GT, T>(g, p, [&op](typename GT::Arc * a) { return op(a); });
  }

  /** Fold the incoming arcs of a node.

        This method folds the incoming arcs to a final folded type `T`. Each
        folding is done by an operation `op`, which must have the following
        signature:

        T op(const T & acc, Arc * arc)

        `acc` often is called an accumulator. `op` should take the
        accumulator and the arc and to produce a folded result. It does
        not compile on a digraph type: use foldl_in_arcs<GT, T>(g, p, init,
        op).

        @param[in] p node pointer
        @param[in] init initial value of folding
        @param[in] op folding operation
        \result the final folded result after the last seen arc
    */
  template <AlephGraph GT, typename T, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Arc *>
  inline
  T foldl_in_arcs(typename GT::Node *p, const T & init,
                  Op op)
  {
    static_assert(not DigraphType<GT>,
                  "foldl_in_arcs(p, init, op) does not see the arcs into p on a digraph, whose "
                  "list of p holds only the arcs that leave p: use foldl_in_arcs(g, p, init, op)");
    T ret = init;
    for_each_in_arc<GT>(p, [&ret, &op](auto a)
                      {
                        ret = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Arc *)>::call(op, ret, a);
                      });
    return ret;
  }

  /** Fold the arcs of `g` into `p` with `op`, which must have the
        signature `T op(const T & acc, Arc * arc)`; on a digraph, in
        \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] init initial value of folding
        @param[in] op folding operation
        \result the final folded result after the last seen arc
    */
  template <AlephGraph GT, typename T, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Arc *>
  inline
  T foldl_in_arcs(const GT & g, typename GT::Node *p, const T & init, Op op)
  {
    using Call = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Arc *)>;
    T ret = init;
    for_each_in_arc(g, p, [&ret, &op](auto a)
                    {
                      ret = Call::call(op, ret, a);
                    });
    return ret;
  }

  /** Filter the incoming arcs meeting an condition.

        It does not compile on a digraph type: use filter_in_arcs(g, p,
        cond).

        @param[in] p node pointer
        @param[in] cond filtering condition
        \result a `DynList<typename GT::Arc*>` containing the filtered arcs
    */
  template <AlephGraph GT, class Op>
  inline
  DynList<typename GT::Arc *> filter_in_arcs(typename GT::Node *p, Op cond)
  {
    static_assert(not DigraphType<GT>,
                  "filter_in_arcs(p, cond) does not see the arcs into p on a digraph, whose list "
                  "of p holds only the arcs that leave p: use filter_in_arcs(g, p, cond)");
    DynList<typename GT::Arc *> ret;
    for_each_in_arc<GT>(p, [&ret, &cond](auto a)
                      {
                        if (cond(a))
                          ret.append(a);
                      });
    return ret;
  }

  /** Return the arcs of `g` into `p` that meet a condition; on a digraph,
        in \f$O(E)\f$.

        @param[in] g the graph of `p`
        @param[in] p node pointer
        @param[in] cond filtering condition
        \result a `DynList<typename GT::Arc*>` containing the filtered arcs
    */
  template <AlephGraph GT, class Op>
  inline
  DynList<typename GT::Arc *> filter_in_arcs(const GT & g, typename GT::Node *p, Op cond)
  {
    DynList<typename GT::Arc *> ret;
    for_each_in_arc(g, p, [&ret, &cond](auto a)
                    {
                      if (cond(a))
                        ret.append(a);
                    });
    return ret;
  }


  // Functional operation on output arcs

  /** Conditioned traversal of outcoming arcs of a node.

        @param[in] p node pointer
        @param[in] op operation whose result must be `bool`. If the result
        is `false`, then the traversal stops and the traverse returns
        `false`; otherwise, all the arcs are traversed and it returns `true`
        \result `true` if all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  bool traverse_out_arcs(typename GT::Node *p, Op op = Op())
  {
    return traverse_arcs<GT, _Out_Iterator<GT>, Op>(p, op);
  }

  /** Traverse the outcoming arcs of a node and executes an operation

        @param[in] p node pointer
        @param[in] op operation to perform on each node
        @ingroup Graphs
    */
  template <AlephGraph GT, class Op>
  inline
  void for_each_out_arc(typename GT::Node *p, Op op = Op())
  {
    for_each_arc<GT, _Out_Iterator<GT>, Op>(p, op);
  }

  /** Test if the outcoming arcs meet a condition.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op testing operation
        \result `true` if `op` returns `true` for all the incoming arcs
    */
  template <AlephGraph GT, class Op>
  inline
  bool all_out_arc(typename GT::Node *p, Op op = Op())
  {
    return traverse_out_arcs<GT>(p, [&op](auto a)
                               {
                                 return op(a);
                               });
  }

  /** Test if it exists an outcoming arc satisfying an operation.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result `true` if there is at least an arc satisfying the operation
    */
  template <AlephGraph GT, class Op>
  inline
  bool exists_out_arc(typename GT::Node *p, Op op = Op())
  {
    return not traverse_out_arcs<GT>(p, [&op](auto a)
                                           {
                                             return not op(a);
                                           });
  }

  /** Search an outcoming arc to a node satisfying a condition `op`.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result a pointer to a node satisfying the operation if this is
        found; `nullptr` otherwise
    */
  template <AlephGraph GT, class Op>
  inline
  auto search_out_arc(typename GT::Node *p, Op op = Op())
  {
    typename GT::Arc *ret = nullptr;
    traverse_out_arcs<GT>(p, [&op, &ret](auto a)
                            {
                              if (op(a))
                                {
                                  ret = a;
                                  return false;
                                }
                              return true;
                            });
    return ret;
  }

  /** Map the outcoming arcs to a transformation,

        The transformation is called `op` and must have the following
        signature:

        T op(Arc * arc)

        Where `T` is the transformed type from the arc.

        @param[in] p node pointer
        @param[in] op transformation
        \result a `DynList<T>` containing the mapping.
    */
  // An actual std::function<T (Arc *)> goes to the overload below, as in
  // map_in_arcs(p, op)
  template <AlephGraph GT, typename T, class Op>
    requires (std::is_invocable_r_v<T, Op &, typename GT::Arc *>
              and not std::is_same_v<Op, std::function<T (typename GT::Arc *)>>)
  inline
  auto map_out_arcs(typename GT::Node *p, Op op)
  {
    DynList<T> ret;
    for_each_out_arc<GT>(p, [&ret, &op](auto a)
                       {
                         ret.append(Aleph::concepts_detail::invoke_as<T (typename GT::Arc *)>::call(op, a));
                       });
    return ret;
  }

  /** \overload
   *
   *  Compatibility overload deducing `T` from an actual
   *  `std::function<T (typename GT::Arc *)>` argument; see the equivalent
   *  `nodes_map` overload for why the generic one above cannot deduce it.
   *  Its third template parameter keeps the call that names that type, as
   *  in `map_in_arcs(p, op)`.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename T,
            class Op = std::function<T (typename GT::Arc *)>>
    requires std::is_same_v<Op, std::function<T (typename GT::Arc *)>>
  inline
  auto map_out_arcs(typename GT::Node *p, std::function<T (typename GT::Arc *)> op)
  {
    // See the equivalent nodes_map overload for why this must go through a
    // lambda rather than forwarding `op` as-is.
    return map_out_arcs<GT, T>(p, [&op](typename GT::Arc * a) { return op(a); });
  }

  /** Fold the outcoming arcs of a node.

        This method folds the outcoming arcs to a final folded type
        `T`. Each folding is done by an operation `op`, which must have the
        following signature:

        T op(const T & acc, Arc * arc)

        `acc` often is called an accumulator. `op` should take the
        accumulator and the arc and to produce a folded result.

        @param[in] p node pointer
        @param[in] init initial value of folding
        @param[in] op folding operation
        \result the final folded result after the last seen arc
    */
  template <AlephGraph GT, typename T, class Op>
    requires std::is_invocable_r_v<T, Op &, const T &, typename GT::Arc *>
  inline
  T foldl_out_arcs(typename GT::Node *p, const T & init,
                   Op op)
  {
    T ret = init;
    for_each_out_arc<GT>(p, [&ret, &op](auto a)
                       {
                         ret = Aleph::concepts_detail::invoke_as<T (const T &, typename GT::Arc *)>::call(op, ret, a);
                       });
    return ret;
  }

  /** Filter the outcoming arcs meeting an condition.

        @param[in] p node pointer
        @param[in] cond filtering condition
        \result a `DynList<typename GT::Arc*>` containing the filtered arcs
    */
  template <AlephGraph GT, class Op>
  inline
  DynList<typename GT::Arc *> filter_out_arcs(typename GT::Node *p, Op cond)
  {
    DynList<typename GT::Arc *> ret;
    for_each_out_arc<GT>(p, [&ret, &cond](auto a)
                       {
                         if (cond(a))
                           ret.append(a);
                       });
    return ret;
  }

  /** Arc filtered searching given two nodes.

        This function receives two nodes and returns the first arc linking
        them. The arc sense is irrelevant for this function. Simply, it
        searches an arc linking the nodes

        @param[in] g the graph
        @param[in] src a node
        @param[in] tgt another node
        @param[in] sa filtering condition
        @return pointer to found node; \c nullptr otherwise
        @throw anything that sa throws
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  typename GT::Arc *
  search_arc(const GT & g,
             typename GT::Node *src, typename GT::Node *tgt,
             SA sa = SA()) noexcept(nothrow_arc_filter_v<GT, SA>)
  {
    assert(src != nullptr and tgt != nullptr);

    if (not g.is_digraph() and tgt->num_arcs < src->num_arcs)
      std::swap(tgt, src); // select the node with less arcs

    for (Node_Arc_Iterator<GT, SA> itor(src, sa); itor.has_curr(); itor.next_ne())
      if (itor.get_tgt_node_ne() == tgt)
        return itor.get_current_arc_ne();

    return nullptr;
  }

  /** Searching of directed arc linking two nodes.

        @param[in] g the graph
        @param[in] src source node pointer
        @param[in] tgt target node pointer
        @param[in] sa arc filter (default: Dft_Show_Arc)
        @return a pointer to a directed linking arc if this is found; \c
        nullptr otherwise.
        @throw anything that sa throws
        @ingroup Graphs
    */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  typename GT::Arc *
  search_directed_arc(const GT & g,
                      typename GT::Node *src, typename GT::Node *tgt,
                      SA sa = SA()) noexcept(nothrow_arc_filter_v<GT, SA>)
  {
    (void) g;
    assert(src != nullptr and tgt != nullptr);

    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
      if (typename GT::Arc *a = it.get_curr(); a->src_node == src and a->tgt_node == tgt)
        return a;

    return nullptr;
  }

  /** Return the mapped node through the cookie of `p`

        @ingroup Graphs
    */
  template <AlephGraph GT>
  typename GT::Node * mapped_node(typename GT::Node *p) noexcept
  {
    return static_cast<typename GT::Node *>(NODE_COOKIE(p));
  }

  /// Return the mapped arc through the cookie of `p`
  template <AlephGraph GT>
  typename GT::Arc * mapped_arc(typename GT::Arc *a) noexcept
  {
    return static_cast<typename GT::Arc *>(ARC_COOKIE(a));
  }

  /// \overload mapped_node(typename GT::Node * p)
  template <AlephGraph GTSRC, AlephGraph GTTGT>
  typename GTTGT::Node * mapped_node(typename GTSRC::Node *p) noexcept
  {
    return static_cast<typename GTTGT::Node *>(NODE_COOKIE(p));
  }

  /// \overload  mapped_arc(typename GT::Arc * a)
  template <AlephGraph GTSRC, AlephGraph GTTGT>
  typename GTTGT::Arc * mapped_arc(typename GTSRC::Arc *a) noexcept
  {
    return static_cast<typename GTTGT::Arc *>(ARC_COOKIE(a));
  }

  /** Explicit copy of graph.

        This function takes a source graph `gsrc` for copying to another
        target graph `gtgt`. First the function cleans `gtgt`; that is
        all its arc and nodes are removed and freed. Then `gsrc` is
        copied to `gtgt`- The resulting `gtgt` is isomorphic to `gsrc`.

        Eventually, the copy can be mapped through the cookies. For
        that, a forth bool parameter is specified-

        @param[in] gsrc source graph
        @param[in] gtgt target graph
        @param[in] cookie_map boolean indicating if the result must be
        mapped through the nodes and arcs cookies.
        @throw bad_alloc if there is no enough memory. Then gtgt is empty:
        the copy assignments of the graphs, which copy to a new graph and
        swap, keep their contents instead.
        @ingroup Graphs
    */
  template <AlephGraph GT>
  inline
  void copy_graph(GT & gtgt, const GT & gsrc, bool cookie_map = false);

  /** Clean a graph: all its nodes and arcs are removed and freed.

        The arcs are removed first, so that the removal of a node does not
        allocate.

        @param[in] g the graph
        @throw anything that the removal of an arc throws: that of
        Net_Graph puts the ends of the arc in its sets of sources and sinks,
        which may throw bad_alloc. Then the arcs removed are gone, and the
        graph keeps the others and all its nodes.
        @ingroup Graphs
    */
  template <AlephGraph GT>
  inline void clear_graph(GT & g);

  /** Functor that traverses the nodes of a graph and performs an
        operation.

        This functor has three template parameters:
        1. `GT`: the graph type
        2. `Operation`: operation functor to be performed on each node
        3. `SN`: node filter

        @ingroup Graphs
    */
  template <AlephGraph GT, class Operation, NodeFilter<GT> SN = Dft_Show_Node<GT>>
  class Operate_On_Nodes
  {
    SN sn;

  public:
    /// Initialize the functor with the filter `sa`
    Operate_On_Nodes(SN __sn = SN())
      : sn(__sn)
    { /* empty */
    }

    /** Call to `operation` on each node.

          @param[in] g the graph
          @param[in] op the operation
      */
    void operator()(const GT & g, Operation op = Operation())
    {
      for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
        op(g, it.get_curr());
    }

    /** Call to `operation` on each node.

          @param[in] g the graph
          @param[in] op the operation
      */
    void operator()(GT & g, Operation op = Operation())
    {
      for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
        op(g, it.get_curr());
    }
  };

  /** Functor that traverses the arcs of a graph and performs an
        operation.

        This functor has three template parameters:
        1. `GT`: the graph type
        2. `Operation`: operation functor to be performed on each arc
        3. `SA`: arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, class Operation, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Operate_On_Arcs
  {
    SA sa;

  public:
    /// Initialize the functor with the filter `sa`
    Operate_On_Arcs(SA __sa = SA())
      : sa(__sa)
    { /* empty */
    }

    /** Call to `op on each arc

          @param[in] g the graph
          @param[in] op the operation
      */
    void operator()(const GT & g, Operation op = Operation()) const
    {
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        op(g, it.get_curr());
    }

    void operator()(GT & g, Operation op = Operation()) const
    {
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        op(g, it.get_curr());
    }

    /** Call to `op on each arc of a node

          @param[in] g the graph
          @param[in] p node pointer
          @param[in] op the operation
      */
    void operator()(const GT & g, typename GT::Node *p,
                    Operation op = Operation()) const
    {
      for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
        op(g, it.get_current_arc_ne());
    }

    void operator()(GT & g, typename GT::Node *p,
                    Operation op = Operation()) const
    {
      for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
        op(g, it.get_current_arc_ne());
    }
  };

  /** Path on a graph.

        Many problems on graphs require dynamic building of partial or
        complete paths. This class models a path on a graph and it is
        designed for dynamically and sequentially building the path
        during graph's traversal.

        The paths are built by their ends; that is by inserting or
        appending. A path is a deque.

        Using on directed graphs
        ========================

        `Path<GT>` operates on both directed and non-directed
        graphs. However, it is necessary to take some previsions.

        Paths on explict directed graphs
        --------------------------------

        If the graph is directed (`List`Digraph`, `List_`SGraph` or
        `Array_Graph`), then the class operates transparently and
        equivalently than on non-directed graphs.

        Paths on implicit directed graphs
        ---------------------------------

        If for practice circumstances you  are treating a non-directed
        graph as a directed one, for example, because you need to know
        the incoming arcs, you must advertise to `Path` class about
        this fact.

        The way that we are designed consists in separating the path
        construction through specific methods; concretely, the methods
        `insert_directed()` and `append_directed()`. You could observe
        very extraneous behaviors if you forget this separation.

        @ingroup Graphs
        @see Path::Iterator
    */
  template <class GT>
  class Path
  {
  public:
    /// The type of data stored in the nodes
    using Node_Type = typename GT::Node_Type;

    /// The type of data stored in the arc
    using Arc_Type = typename GT::Arc_Type;

  private:
    const GT *g = nullptr;

    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    struct Path_Desc
    {
      Node *node; // source node
      Arc *arc; // adjacent arc

      Path_Desc(Node *_node = nullptr, Arc *_arc = nullptr) noexcept
        : node(_node), arc(_arc)
      {}

      bool operator==(const Path_Desc & r) const noexcept
      {
        if (not (node->get_info() == r.node->get_info()))
          return false;

        if (arc == nullptr)
          return r.arc == nullptr;

        if (r.arc == nullptr)
          return false;

        return arc->get_info() == r.arc->get_info();
      }
    };

    DynDlist<Path_Desc> list;

    void check_graph()
    {
      ah_domain_error_if(g == nullptr) << "Path: Graph has not been specified";
    }

  public:
    /// Return `true` if the path is consistent: each arc joins its node
    /// with the next one, either way, and the last node has no arc
    [[nodiscard]] bool check() const
    {
      const Path_Desc * prev = nullptr;
      for (typename DynDlist<Path_Desc>::Const_Iterator it(list); it.has_curr(); it.next_ne())
        {
          const Path_Desc & d = it.get_curr_ne();
          if (prev != nullptr)
            {
              if (prev->arc == nullptr)
                return false;
              auto src = g->get_src_node(prev->arc);
              auto tgt = g->get_tgt_node(prev->arc);
              if (not ((src == prev->node and tgt == d.node) or
                       (tgt == prev->node and src == d.node)))
                return false;
            }
          prev = &d;
        }

      return prev == nullptr or prev->arc == nullptr;
    }

    /// Return `true` if the directed path is consistent: each arc goes
    /// from its node to the next one, and the last node has no arc
    [[nodiscard]] bool check_directed() const
    {
      const Path_Desc * prev = nullptr;
      for (typename DynDlist<Path_Desc>::Const_Iterator it(list); it.has_curr(); it.next_ne())
        {
          const Path_Desc & d = it.get_curr_ne();
          if (prev != nullptr and
              (prev->arc == nullptr or g->get_src_node(prev->arc) != prev->node or
               g->get_tgt_node(prev->arc) != d.node))
            return false;
          prev = &d;
        }

      return prev == nullptr or prev->arc == nullptr;
    }

    /// Get a constant reference to the graph
    const GT &get_graph() const noexcept
    {
      return *g;
    }

    /// Return `true` if `this` is on graph `gr`
    bool inside_graph(const GT & gr) const noexcept
    {
      return g == &gr;
    }

    /// Construct a empty path on graph `__g`
    Path(const GT & __g) noexcept : g(&__g)
    {}

    Path() noexcept : g(nullptr)
    { /* empty */
    }

    /** Set the first node of a path

          @param start_node pointer to start node of a path
          @throw bad_alloc if there is no enough memory
          @note `start_node` (of course) must belong to the graph. This is
          not verified
      */
    void init(Node *start_node)
    {
      assert(start_node != nullptr);
      list.append(Path_Desc(start_node));
    }

    /** Construct a path starting from a given node

          @param[in] _g the graph
          @param[in] start_node pointer to start node of a path
          @throw bad_alloc if there is no enough memory
      */
    Path(const GT & _g, Node *start_node) : g(&_g)
    {
      init(start_node);
    }

    /** Set the graph of the path.

          Sometimes, by example, if you manage a path attribute inside a
          class, you do not use the graph, but you need to declare the
          path. This method is intended to set the graph of a path.

          Eventually, if a path is already built, then this one is erased
          before to set the graph.

          @param[in] __g the graph
          @param[in] start_node pointer to start node of path
          @throw bad_alloc if there is no enough memory
      */
    void set_graph(const GT & __g, Node *start_node = nullptr)
    {
      empty();
      g = &__g;
      if (start_node == nullptr)
        return;
      init(start_node);
    }

    /// Return the path length in nodes
    [[nodiscard]] size_t size() const noexcept
    {
      return list.size();
    }

    /// Return `true` if the path is empty
    [[nodiscard]] bool is_empty() const noexcept
    {
      return list.is_empty();
    }

    /// Clean the path: all the nodes and arc are removed
    void empty()
    {
      check_graph();
      while (not list.is_empty())
        list.remove_first();
    }

    /** @brief Empties the container. 
        
        @throws none
        @see empty()
    */
    void clear() { empty(); }

    /// Copy constructor
    Path(const Path & path) : g(path.g), list(path.list) {}

    /// Move constructor
    Path(Path && path)  noexcept : g(path.g), list(std::move(path.list)) {}

    /// Copy assignment
    Path &operator=(const Path & path)
    {
      if (this == &path)
        return *this;

      empty();
      g = path.g;
      list = path.list;
      return *this;
    }

    /// Move assignment
    Path &operator=(Path && path) noexcept
    {
      std::swap(g, path.g);
      list.swap(path.list);
      return *this;
    }

    /** Append an arc to the path.

          This method searches the last node of path and appends it an arc.

          The arc must be connected to the last node.

          @param[in] arc pointer to arc
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if arc is not linked to the last node of
          path
          @throw domain_error if the graph has not been specified
          @throw domain_error if the path is empty
      */
    void append(Arc *arc)
    {
      assert(arc != nullptr);
      check_graph();

      ah_domain_error_if(list.is_empty()) << "path is empty";

      auto & last_path_desc = list.get_last();
      auto last_node = last_path_desc.node;
      ah_invalid_argument_if(arc->src_node != last_node and arc->tgt_node != last_node)
      << "arc has not link to last node of path";

      last_path_desc.arc = arc;
      list.append(Path_Desc(g->get_connected_node(arc, last_node)));
    }

    /** Append a node to the path

          This method appends `node` at the end of path. For this, the
          adjacent arcs of the last node are examined until finding an arc
          linking `node`.

          If the path is empty, the `node` becomes its first node.

          If the operation is successful, then `node` becomes the last node
          of the path.

          @param[in] node pointer to the node to append in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of the path
          @throw domain_error if the graph has not been specified
      */
    void append(Node *node)
    {
      check_graph();

      if (list.is_empty())
        {
          init(node);
          return;
        }

      Node *last_node = get_last_node();
      Arc *arc = search_arc(*g, last_node, node);

      ah_invalid_argument_if(arc == nullptr)
        << "There is no an arc connecting to the node";

      append(arc);
    }

    /** Append a node to a directed path

          This method inserts `node` at the end of a directed path. For
          this, the outcoming arcs of the last node of path are examined
          until finding an arc linking `node`.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          If the path is empty, the `node` becomes its first node.

          If the operation is successful, then `node` becomes the last node
          of the path.

          @param[in] p node pointer to the node to append in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of the path
          @throw domain_error if the graph has not been specified
      */
    void append_directed(Node *p)
    {
      assert(p != nullptr);
      check_graph();
      if (list.is_empty())
        {
          init(p);
          return;
        }

      auto & last_path_desc = list.get_last();
      Node *last_node = last_path_desc.node;
      Arc *arc = search_directed_arc(*g, last_node, p);

      ah_invalid_argument_if(arc == nullptr) << "There is no an arc connecting to the node";

      assert(arc->src_node == last_path_desc.node);

      last_path_desc.arc = arc;
      list.append(Path_Desc(static_cast<Node *>(arc->tgt_node)));
    }

    /** Append an arc to a directed path

          This method appends `arc` at the end of a directed path. For this,
          the source node of `arc` must match the current last node of
          a directed path.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          If the operation is successful, then `node` becomes the last node
          of the path.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of the path
          @throw domain_error if the graph has not been specified
      */
    void append_directed(Arc *arc)
    {
      assert(arc != nullptr);
      check_graph();

      ah_domain_error_if(list.is_empty()) << "path is empty";

      auto & last_path_desc = list.get_last();
      Node *last_node = last_path_desc.node;
      ah_invalid_argument_if(arc->src_node != last_node)
        << "The arc does not connect the last node";

      last_path_desc.arc = arc;
      list.append(Path_Desc(static_cast<Node *>(arc->tgt_node)));
    }

    /** Insert an arc as the first of a path.

          This method inserts `arc` at the beginning of a path. For this,
          the adjacent arcs of the current first node of the path are examined
          until find an arc linking `node`.

          If the operation is successful, then `node` becomes the first node
          of the path.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the first
          node of the path
          @throw domain_error if the graph has not been specified
      */
    void insert(Arc *arc)
    {
      assert(arc != nullptr);
      check_graph();

      ah_domain_error_if(list.is_empty()) << "path is empty";

      auto & first_path_desc = list.get_first();
      auto first_node = first_path_desc.node;
      ah_invalid_argument_if(arc->src_node != first_node and arc->tgt_node != first_node)
        << "arc has not link to first node of path";

      Path_Desc item(g->get_connected_node(arc, first_node), arc);
      list.insert(item);
    }

    /** Insert a node to the path

          This method inserts `node` at the beginning of the path, through
          the arc that `search_arc()` finds between `node` and the first node
          of the path.

          If the path is empty, the `node` becomes its first node.

          If the operation is successful, then `node` becomes the first node
          of the path.

          @param[in] node pointer to the node to insert in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if no arc links `node` with the first node
          of the path; the path is then as it was
          @throw domain_error if the graph has not been specified
      */
    void insert(Node *node)
    {
      check_graph();

      if (list.is_empty())
        {
          init(node);
          return;
        }

      Node *first_node = get_first_node();
      Arc *arc = search_arc(*g, node, first_node); // search arc first_node-node
      ah_invalid_argument_if(arc == nullptr) << "There is no arc connecting node";

      Path_Desc item(node, arc);
      list.insert(item);
    }

    /** Insert a node at the beginning of a directed path

          This method inserts `p` at the beginning of a directed path. For
          this, the outcoming arcs of `p` are examined until finding an arc
          `p --> first`, where `first` is the first node of the path.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          If the path is empty, `p` becomes its first node.

          If the operation is successful, then `p` becomes the first node
          of the path.

          @param[in] p node pointer to the node to insert in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if there is no arc from `p` to the first
          node of the path; the path is then as it was
          @throw domain_error if the graph has not been specified
      */
    void insert_directed(Node *p)
    {
      assert(p != nullptr);
      check_graph();

      if (list.is_empty())
        {
          init(p);
          return;
        }

      Node *first_node = get_first_node();
      Arc *arc = search_directed_arc(*g, p, first_node);
      ah_invalid_argument_if(arc == nullptr) << "There is no an arc connecting to the node";

      list.insert(Path_Desc(p, arc));
    }

    /** Append an arc to a directed path

          This method appends `arc` at the beginning of a directed path. For
          this, the target node of `arc` must be the current first node of
          a directed path.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of the path
          @throw domain_error if the graph has not been specified
      */
    void insert_directed(Arc *arc)
    {
      assert(arc != nullptr);
      check_graph();

      ah_domain_error_if(list.is_empty()) << "path is empty";

      auto & first_path_desc = list.get_first();
      Node *first_node = first_path_desc.node;
      ah_invalid_argument_if(arc->tgt_node != first_node)
        << "The arc does not connect the first node";

      list.insert(Path_Desc(static_cast<Node *>(arc->src_node), arc));
    }

    /// Return the first node of path; throws `overflow_error` if path
    /// is empty
    Node * get_first_node() const
    {
      return list.get_first().node;
    }

    /// Return the last node of path; throws `overflow_error` if path
    /// is empty
    Node * get_last_node() const
    {
      auto & last_path_desc = list.get_last();
      assert(last_path_desc.arc == nullptr);
      return last_path_desc.node;
    }

    /// Return the first arc of path; throws `overflow_error` if path
    /// is empty
    Arc * get_first_arc() const
    {
      return list.get_first().arc;
    }

    /// Return the last arc of a path; throws `overflow_error` if the path
    /// is empty
    Arc * get_last_arc() const
    {
      ah_domain_error_if(list.is_unitarian())
        << "Path with only a node (without any arc)";

      typename DynDlist<Path_Desc>::Const_Iterator it(list);
      it.reset_last();
      it.prev();
      return it.get_curr().arc;
    }

    /// Return `true` if `this` is a cycle; throws if path is empty
    [[nodiscard]] bool is_cycle() const
    {
      return get_first_node() == get_last_node();
    }

    /** Remove the last node of path

          The path may be left empty.

          \return pointer to the extracted node
          \throw underflow_error if path is empty
      */
    Node * remove_last_node()
    {
      auto d = list.remove_last();
      if (not list.is_empty()) // the arc that went to the removed node
        list.get_last().arc = nullptr;
      return d.node;
    }

    /** Remove the first node of a path

          \return pointer to the extracted node
          \throw overflow_error if the path is empty
      */
    Node * remove_first_node()
    {
      auto d = list.remove_first();
      return d.node;
    }

    /// Fast swap between two paths (constant time)
    void swap(Path & path) noexcept
    {
      std::swap(g, path.g);
      list.swap(path.list);
    }

    /** Iterator on nodes and arcs of a path

          @ingroup Graphs
          @see Path
      */
    class Iterator : public Aleph::const_iterator_t<DynDlist<Path_Desc>>
    {
      // The path hands out its nodes and arcs, never its entries: a
      // read-only view of the list is all it needs.
      using Base = Aleph::const_iterator_t<DynDlist<Path_Desc>>;

    public:
      /// Create an iterator on the first node of `path`
      Iterator(const Path & path) noexcept
        : Base(path.list)
      {}

    private:
      const Path_Desc &get_curr_path_desc_ne() const noexcept
      {
        return this->Base::get_curr_ne();
      }

      const Path_Desc &get_curr_path_desc() const
      {
        return this->Base::get_curr();
      }

    public:
      /// Return the current node of a path. Throw `overflow_error` if path
      /// is empty of the iterator has not current node
      Node * get_current_node() const
      {
        return this->get_curr_path_desc().node;
      }

      Node * get_current_node_ne() const noexcept
      {
        return this->get_curr_path_desc_ne().node;
      }

      /** Return the current arc of a path.

            \note A path of `n` nodes has `n - 1` arcs. So if the iterator
            is positioned on the last node, the `get_current_arc()` has no
            current arc and will throw an exception.

            \return pointer to the current arc
            \throw overflow_error if path is empty of iterator has not
            current node
        */
      Arc * get_current_arc() const
      {
        ah_overflow_error_if(this->is_in_last()) << "Path iterator is in last node of path";

        return this->get_curr_path_desc().arc;
      }

      Arc * get_current_arc_ne() const noexcept
      {
        return this->get_curr_path_desc_ne().arc;
      }

      ///\overload get_current_node()
      Node * get_curr_ne() const noexcept
      {
        return get_current_node_ne();
      }

      Node * get_curr() const
      {
        return get_current_node();
      }

      /** Return a pair with the current node and arc

            \note If the iterator is positioned on the last node, then its
            arc is `nullptr`.

            \return a pair whose first element is the current node and
            the second element its outcoming arc
            \throw overflow_error if path is empty of iterator has not
            current node
        */
      std::pair<Node *, Arc *> get_pair() const
      {
        return std::make_pair(get_current_node(), get_current_arc());
      }

      /** Return a tuple with the current node and arc

            \note If the iterator is positioned on the last node, then its
            arc is `nullptr`.

            \return a tuple whose first element is the current node and
            the second element its outcoming arc
            \throw overflow_error if path is empty of iterator has not
            current node
        */
      std::tuple<Node *, Arc *> get_tuple() const
      {
        return std::make_tuple(get_current_node(), get_current_arc());
      }

      std::tuple<Node *, Arc *> get_tuple_ne() const noexcept
      {
        return std::make_tuple(get_current_node_ne(), get_current_arc_ne());
      }

      /** Return `true` if iterator has current arc.

            \note If the iterator is on the last node, then there is no
            current arc.

            @return a boolean indicating if the iterator is positioned on an
            arc
        */
      [[nodiscard]] bool has_current_arc() const noexcept
      {
        return this->has_curr() and not this->is_in_last();
      }

      /// Return `true` if the iterator has a current node
      [[nodiscard]] bool has_current_node() const noexcept
      {
        return this->has_curr();
      }
    };

    /// Returns an iterator on the path
    Iterator get_it() const
    {
      return Iterator(*this);
    }

    /** Execute an operation on each node of path

          \param[in] op operation
      */
    template <class Operation>
    void for_each_node(Operation op = Operation()) const
    {
      for (Iterator it(*this); it.has_current_node(); it.next_ne())
        op(it.get_current_node_ne());
    }

    /** Execute an operation on each arc of path

          \param[in] op operation
      */
    template <class Operation>
    void for_each_arc(Operation op = Operation()) const
    {
      for (Iterator it(*this); it.has_current_arc(); it.next_ne())
        op(it.get_current_arc_ne());
    }

    /// Return `true` if `node` belongs to the path
    bool contains_node(Node *node) const noexcept
    {
      for (Iterator it(*this); it.has_current_node(); it.next_ne())
        if (it.get_current_node_ne() == node)
          return true;
      return false;
    }

    /// Return `true` if `arc` belongs to the path
    bool contains_arc(Arc *arc) const noexcept
    {
      for (Iterator it(*this); it.has_current_arc(); it.next_ne())
        if (it.get_current_arc_ne() == arc)
          return true;
      return false;
    }

    /// Return a list with the nodes of path (order according to the path)
    DynList<Node *> nodes() const
    {
      DynList<Node *> ret_val;
      for_each_node([&ret_val](Node *p)
                      {
                        ret_val.append(p);
                      });
      return ret_val;
    }

    /// Return a list with the arcs of a path (order, according to the path)
    DynList<Arc *> arcs() const
    {
      DynList<Arc *> ret_val;
      for_each_arc([&ret_val](Arc *a)
                     {
                       ret_val.append(a);
                     });
      return ret_val;
    }

    /** Return `true` if `this` is equal to `p`,

          \note The comparison is done between node and arcs contents. This
          implicates that the operator `==` must be defined for the data
          types stored in the nodes and arcs respectively.

          @param[in] p path to compare with `this`
          \return `true` if node to node and arc to arc the paths are equal;
          `false` otherwise.
      */
    bool operator==(const Path & p) const noexcept
    {
      return eq(this->list, p.list);
    }

    ///  Return `true` if `this` is not equal to `p`,
    bool operator!=(const Path & p) const noexcept
    {
      return not eq(this->list, p.list);
    }
  };

  template <AlephGraph GT>
  inline
  Path<GT> find_path_depth_first(const GT & g, typename GT::Node *start_node,
                                 typename GT::Node *end_node);

  // Searches from curr_node, reached through curr_arc, a path to end_node,
  // which it puts in path. A path can have as many nodes as the graph, too
  // many for recursion, so that the search keeps its own stack; the order
  // of the visits is that of a recursion.
  template <class GT>
  static inline
  bool __find_path_depth_first(const GT & g, typename GT::Node *curr_node,
                               typename GT::Arc *curr_arc,
                               typename GT::Node *end_node, Path<GT> & path)
  {
    if (curr_node == end_node) // this test must be first in order to find cycles
      {
        path.append(curr_arc);
        return true;
      }

    if (IS_NODE_VISITED(curr_node, Find_Path))
      return false;

    path.append(curr_arc);
    NODE_BITS(curr_node).set_bit(Find_Path, true);

    // The node being visited keeps its iterator in here until it visits a
    // node; then it waits in frames, where it goes on when it comes back
    using Frame = Dfs_Arc_Frame<typename GT::Node_Arc_Iterator>;
    Dfs_Stack<Frame> frames;
    Frame here(g.get_arc_it(curr_node));
    while (true)
      {
        if (auto tgt = here.template next_target<Find_Path>(end_node))
          { // the iterator stays on the arc to tgt, as in a recursion
            path.append(here.it.get_current_arc_ne());
            if (tgt == end_node)
              return true;

            NODE_BITS(tgt).set_bit(Find_Path, true);
            frames.push(std::move(here));
            here = Frame(g.get_arc_it(tgt));
            continue;
          }

        path.remove_last_node(); // there is no path through it
        while (true) // back to the nodes that wait
          {
            if (frames.is_empty())
              return false;

            Frame & f = frames.top();
            f.next_arc(); // past the arc to the node that was visited
            if (auto tgt = f.template next_target<Find_Path>(end_node))
              {
                path.append(f.it.get_current_arc_ne());
                if (tgt == end_node)
                  return true;

                NODE_BITS(tgt).set_bit(Find_Path, true);
                here = Frame(g.get_arc_it(tgt));
                break;
              }
            path.remove_last_node();
            frames.pop();
          }
      }
  }

  /** Depth-first search of a path between two nodes.

        `find_path_depth_first()` searches depth-first a path between
        `start_node` and `end_node`, without recursion: the path can be as
        long as the graph.

        @param[in] g the graph
        @param[in] start_node pointer to the starting node of search
        @param[in] end_node pointer to ending node of search
        \return a `Path` object containing the path if this was found;
        an empty path otherwise
        @throw bad_alloc if there is no enough memory
        @see find_path_breadth_first()
        @ingroup Graphs
    */
  template <AlephGraph GT>
  inline
  Path<GT> find_path_depth_first(const GT & g, typename GT::Node *start_node,
                                 typename GT::Node *end_node)
  {
    Path<GT> path(g, start_node);

    g.reset_bit_nodes(Find_Path);
    g.reset_bit_arcs(Find_Path);
    NODE_BITS(start_node).set_bit(Find_Path, true);

    for (auto it = g.get_arc_it(start_node); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        ARC_BITS(arc).set_bit(Find_Path, true);
        auto next_node = it.get_tgt_node();
        if (IS_NODE_VISITED(next_node, Find_Path))
          continue;

        if (__find_path_depth_first<GT>(g, next_node, arc, end_node, path))
          return path;
      }

    path.empty();

    return path;
  }

  /** Map two nodes of different types of graphs through their cookies

        \note It is intended that the mapping be done between at least
        homomorphic graphs

        @param[in] p pointer to the source node
        @param[in] q pointer to the target node
        @ingroup Graphs
    */
  template <AlephGraph GTS, AlephGraph GTT>
  void map_nodes(typename GTS::Node *p, typename GTT::Node *q) noexcept
  {
    assert(p != nullptr and q != nullptr);

    // Use reinterpret_cast to preserve an exact pointer value without any
    // implicit base class pointer adjustment from multiple inheritance
    if (NODE_COOKIE(p) == nullptr)
      {
        NODE_COOKIE(p) = reinterpret_cast<void *>(q);
        NODE_COOKIE(q) = reinterpret_cast<void *>(p);
        return;
      }

    NODE_COOKIE(q) = NODE_COOKIE(p);
    NODE_COOKIE(p) = reinterpret_cast<void *>(q);
  }

  /** Map two arcs of different types of graphs through their cookies

        \note It is intended that the mapping be done between at least
        homomorphic graphs

        @param[in] p pointer to the source arc
        @param[in] q pointer to the target arc

        @ingroup Graphs
    */
  template <AlephGraph GTS, AlephGraph GTT>
  void map_arcs(typename GTS::Arc *p, typename GTT::Arc *q) noexcept
  {
    assert(p != nullptr and q != nullptr);

    if (ARC_COOKIE(p) == nullptr)
      {
        ARC_COOKIE(p) = q;
        ARC_COOKIE(q) = p;

        return;
      }

    ARC_COOKIE(q) = ARC_COOKIE(p);
    ARC_COOKIE(p) = q;
  }

  template <AlephGraph GT>
  void clear_graph(GT & g)
  {
    for (typename GT::Arc_Iterator it(g); it.has_curr();) // remove arcs
      {
        typename GT::Arc *arc = it.get_curr_ne();
        it.next_ne();
        g.remove_arc(arc);
      }

    for (typename GT::Node_Iterator it(g); it.has_curr();) // remove nodes
      {
        typename GT::Node *p = it.get_curr_ne();
        it.next_ne(); // advance before deletion (iterator consistency)
        g.remove_node(p); // remove it from the graph
      }
  }

  template <AlephGraph GT>
  void copy_graph(GT & gtgt, const GT & gsrc, const bool cookie_map)
  {
    try
      {
        clear_graph(gtgt); // clear this before copying
        DynMapAvlTree<typename GT::Node *, typename GT::Node *> map;

        // phase 1: traverse nodes of src_graph and insert copy in this
        for (typename GT::Node_Iterator it(gsrc); it.has_curr(); it.next_ne())
          {
            typename GT::Node *src_node = it.get_current_node_ne();
            std::unique_ptr<typename GT::Node>
                tgt_node(new typename GT::Node(src_node->get_info()));
            map.insert(src_node, tgt_node.get());

            typename GT::Node *tgt = tgt_node.release();
            assert(tgt->get_info () == src_node->get_info ());
            gtgt.insert_node(tgt); // insert in the target graph

            if (cookie_map)
              GT::map_nodes(src_node, tgt);
          }

        assert(gtgt.get_num_nodes() == gsrc.get_num_nodes());

        // phase 2: for each arc of src_graph, create in this an
        // arc connecting the mapped nodes from map
        for (typename GT::Arc_Iterator it(gsrc); it.has_curr(); it.next_ne())
          {
            typename GT::Arc *src_arc = it.get_current_arc_ne();

            // get node images in target graph and create arc
            typename GT::Node *src_node = map[gsrc.get_src_node(src_arc)];
            typename GT::Node *tgt_node = map[gsrc.get_tgt_node(src_arc)];
            typename GT::Arc *tgt_arc = gtgt.insert_arc(src_node, tgt_node);
            // tgt_arc->get_info() = src_arc->get_info();
            *tgt_arc = *src_arc;
            if (cookie_map)
              GT::map_arcs(src_arc, tgt_arc);
          }

        assert(gtgt.get_num_arcs() == gsrc.get_num_arcs());
      }
    catch (...)
      { // If an exception occurs, empty gtgt without allocating: a network
        // allocates when it removes an arc, and its destructor does not
        std::remove_cvref_t<decltype(gtgt)> empty;
        empty.get_cookie() = gtgt.get_cookie();
        gtgt.swap(empty);
        throw;
      }
  }

  /** Default copy node functor

        @ingroup Graphs
    */
  template <class GTT, class GTS>
  struct Dft_Copy_Node
  {
    void operator()(typename GTT::Node *tgt, typename GTS::Node *src) noexcept
    {
      tgt->get_info() = src->get_info();
    }
  };

  /** Default copy arc functor

        @ingroup Graphs
    */
  template <class GTT, class GTS>
  struct Dft_Copy_Arc
  {
    void operator()(typename GTT::Arc *tgt, typename GTS::Arc *src)
    {
      tgt->get_info() = src->get_info();
    }
  };

  /** Copy between different types of graphs.

        @param[in] gtgt target graph
        @param[in] gsrc source graph
        @param[in] cookie_map if `true` ==> a mapping will be done through
        nodes and arcs cookies
        @ingroup Graphs
    */
  template <AlephGraph GTT, AlephGraph GTS,
            class Copy_Node = Dft_Copy_Node<GTT, GTS>,
            class Copy_Arc = Dft_Copy_Arc<GTT, GTS>>
  void inter_copy_graph(GTT & gtgt, const GTS & gsrc,
                        const bool cookie_map = false)
  {
    try
      {
        clear_graph(gtgt); // clear this before copying
        DynMapAvlTree<typename GTS::Node *, typename GTT::Node *> map;
        // phase 1: traverse nodes of src_graph and insert a copy in this
        for (typename GTS::Node_Iterator it(gsrc); it.has_curr(); it.next_ne())
          {
            typename GTS::Node *src_node = it.get_current_node_ne();
            std::unique_ptr<typename GTT::Node> tgt_node(new typename GTT::Node);
            Copy_Node()(tgt_node.get(), src_node);
            map.insert(src_node, tgt_node.get());

            typename GTT::Node *tgt = tgt_node.release();
            gtgt.insert_node(tgt); // insert in the target graph

            if (cookie_map)
              map_nodes<GTS, GTT>(src_node, tgt);
          }

        assert(gtgt.get_num_nodes() == gsrc.get_num_nodes());

        // phase 2: for each arc of src_graph, create in this an
        // arc connecting the mapped nodes from the map
        for (typename GTS::Arc_Iterator it(gsrc); it.has_curr(); it.next_ne())
          {
            typename GTS::Arc *src_arc = it.get_current_arc_ne();

            // get node images in target graph and create arc
            typename GTT::Node *src_node = map[gsrc.get_src_node(src_arc)];
            typename GTT::Node *tgt_node = map[gsrc.get_tgt_node(src_arc)];
            typename GTT::Arc *tgt_arc = gtgt.insert_arc(src_node, tgt_node);
            Copy_Arc()(tgt_arc, src_arc);
            if (cookie_map)
              map_arcs<GTS, GTT>(src_arc, tgt_arc);
          }

        assert(gtgt.get_num_arcs() == gsrc.get_num_arcs());
      }
    catch (...)
      { // If an exception occurs, empty gtgt without allocating: a network
        // allocates when it removes an arc, and its destructor does not
        std::remove_cvref_t<decltype(gtgt)> empty;
        empty.get_cookie() = gtgt.get_cookie();
        gtgt.swap(empty);
        throw;
      }
  }

  /** Filtered copy of graphs.

        This functor copies a graph according to filtering criterias
        for its nodes and arcs: it copies the nodes that the node filter
        shows, and the arcs that the arc filter shows between two of
        them.

        The template parameters are:
        - `GT`: the graph type
        - `SN`: node filter
        - `SA`: arc filter

        @ingroup Graphs
    */
  template <AlephGraph GT, NodeFilter<GT> SN = Dft_Show_Node<GT>, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Copy_Graph
  {
    SN sn;
    SA sa;

  public:
    /** Constructor

          @param[in] __sa arc filter
          @param[in] __sn node filter
      */
    Copy_Graph(SA __sa = SA(), SN __sn = SN()) : sn(__sn), sa(__sa)
    {
      // empty
    }

  private:
    void copy(GT & gtgt, const GT & gsrc, const bool cookie_map)
    {
      try
        {
          clear_graph(gtgt); // clear this before copying
          DynMapAvlTree<typename GT::Node *, typename GT::Node *> map;

          // phase 1: traverse nodes of src_graph and insert a copy in this
          for (Node_Iterator<GT, SN> it(gsrc, sn); it.has_curr(); it.next_ne())
            {
              typename GT::Node *src_node = it.get_curr();
              std::unique_ptr<typename GT::Node>
                  tgt_node(new typename GT::Node(src_node));
              map.insert(src_node, tgt_node.get());

              typename GT::Node *tgt = tgt_node.release();
              gtgt.insert_node(tgt); // insert in target graph

              if (cookie_map)
                GT::map_nodes(src_node, tgt);
            }

          // phase 2: for each arc of src_graph between two copied nodes,
          // create in this an arc connecting their images
          for (Arc_Iterator<GT, SA> it(gsrc, sa); it.has_curr(); it.next_ne())
            {
              typename GT::Arc *src_arc = it.get_curr();

              // get node images in target graph and create arc
              auto *src_image = map.search(gsrc.get_src_node(src_arc));
              auto *tgt_image = map.search(gsrc.get_tgt_node(src_arc));
              if (src_image == nullptr or tgt_image == nullptr)
                continue; // a node that the node filter hides

              typename GT::Arc *tgt_arc =
                  gtgt.insert_arc(src_image->second, tgt_image->second,
                                  src_arc->get_info());

              if (cookie_map)
                GT::map_arcs(src_arc, tgt_arc);
            }
        }
      catch (...)
        { // If an exception occurs, gtgt is emptied, without allocating
          GT empty;
          empty.get_cookie() = gtgt.get_cookie();
          gtgt.swap(empty);
          throw;
        }
    }

  public:
    /** Perform the copy from `gsrc` to `gtgt`.

          @param[out] gtgt target graph
          @param[in] gsrc source graph
          @param[in] cookie_map if `true` ==> the nodes and arcs of both
          graphs are mapped through their cookies
      */
    void operator()(GT & gtgt, GT & gsrc, const bool cookie_map = true)
    {
      copy(gtgt, gsrc, cookie_map);
    }
  };

  /** Filter of painter arcs with that are set the Spanning_Tree control
        bit.

        Some `Aleph-w` (\f$\aleph_\omega\f$) algorithms set the
        Spanning_Tree control bit in order to mark their results. This
        simple filter filters those arc and addition computes the
        accumulative distance between all the seen arcs.

        The filter keeps a copy of its distance accessor, which may have
        state, or no default constructor.

        @ingroup Graphs
    */
  template <AlephGraph GT, class Distance>
  struct Painted_Min_Spanning_Tree
  {
    /// Accumulative distance from the first seen arc until the last seen
    typename Distance::Distance_Type dist;

    /// Distance accessor for the arc weights
    Distance distance;

    /// Build the filter with a distance accessor built by `Distance()`.
    Painted_Min_Spanning_Tree()
      noexcept(std::is_nothrow_default_constructible_v<Distance>)
      : dist(0), distance()
    { /* empty */
    }

    /** Build the filter with a distance accessor.

        @param[in] __distance distance accessor for the arc weights
    */
    explicit Painted_Min_Spanning_Tree(Distance __distance)
      : dist(0), distance(__distance)
    { /* empty */
    }

    /** Show the arc if it is painted, and then add its weight to `dist`.

        @param[in] a the arc
        @return true if `a` is painted
        @throw anything that the distance throws; `dist` is then unchanged
    */
    bool operator()(typename GT::Arc *a)
    {
      if (not IS_ARC_VISITED(a, Aleph::Spanning_Tree))
        return false;

      dist = dist + distance(a);

      return true;
    }
  };

  /** Fast graph comparison.

        If the data types stored in the nodes are sortable then this method
        can respond if two graphs are isomorphic in \f$O(V \lg V) + O(E)\f$.

        Both graphs must be of same type.

        \warning The cookies of nodes are used and set

        Since this function uses node indexes, this is defined in
        `tpl_indexGraph.H`.

        @param[in] g1 a graph
        @param[in] g2 a graph
        \return `true` if the graphs are equal; `false` otherwise
        \see find_depth_first_spanning_tree() find_breadth_first_spanning_tree()
        \see Kruskal_Min_Spanning_Tree Prim_Min_Spanning_Tree
        \see Dijkstra_Min_Paths Bellman_Ford
    */
  template <AlephGraph GT>
  inline
  bool are_equal(const GT & g1, const GT & g2);

  /** @brief Graph copy with explicit node mapping.
   *
   *  This class creates a copy of a graph and maintains an explicit mapping
   *  from original nodes to their copies using a DynMapTree (not cookies).
   *
   *  This is essential for algorithms like Johnson's that need to:
   *  1. Copy a graph
   *  2. Run algorithms that use cookies on the copy (Bellman-Ford, Dijkstra)
   *  3. Still be able to translate between original and copy nodes
   *
   *  The mapping is unidirectional: original → copy.
   *
   *  @tparam GT Graph type (List_Graph, List_Digraph, etc.)
   *  @tparam Tree Binary search tree type for the mapping (default Treap)
   *
   *  @par Example
   *  @code
   *  List_Digraph<Node, Arc> original;
   *  // ... build graph ...
   *
   *  GraphCopyWithMapping<List_Digraph<Node, Arc>> copy(original);
   *
   *  // Get copied node from original
   *  auto* orig_node = original.get_first_node();
   *  auto* copy_node = copy.get_copy(orig_node);
   *
   *  // Access the copied graph
   *  auto& copied_graph = copy.get_graph();
   *  @endcode
   *
   *  @ingroup Graphs
   *  @see copy_graph map_nodes Johnson
   */
  template <AlephGraph GT,
            template <class, class> class Tree = Treap>
  class GraphCopyWithMapping
  {
  public:
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Node_Type = typename GT::Node_Type;
    using Arc_Type = typename GT::Arc_Type;

  private:
    GT copied_graph;
    DynMapTree<Node *, Node *, Tree> node_map; // original -> copy

    /// Build the copy and mapping
    void build_copy(const GT & src)
    {
      // Phase 1: Copy all nodes and build mapping
      for (typename GT::Node_Iterator it(src); it.has_curr(); it.next_ne())
        {
          Node *src_node = it.get_curr();
          Node *tgt_node = copied_graph.insert_node(src_node->get_info());
          node_map.insert(src_node, tgt_node);
        }

      // Phase 2: Copy all arcs using the node mapping
      for (typename GT::Arc_Iterator it(src); it.has_curr(); it.next_ne())
        {
          Arc *src_arc = it.get_curr();
          Node *src_src = src.get_src_node(src_arc);
          Node *src_tgt = src.get_tgt_node(src_arc);

          Node *tgt_src = node_map.find(src_src);
          Node *tgt_tgt = node_map.find(src_tgt);

          copied_graph.insert_arc(tgt_src, tgt_tgt, src_arc->get_info());
        }
    }

  public:
    /** @brief Construct a copy of the given graph with node mapping.
     *
     *  Creates a deep copy of the source graph and builds a mapping from
     *  source nodes to their corresponding copies.
     *
     *  @param[in] src The source graph to copy.
     *  @throws std::bad_alloc if there is not enough memory.
     *  @note Complexity: O(V log V + E) due to map insertions.
     *  @note Exception safety: Strong guarantee.
     */
    explicit GraphCopyWithMapping(const GT & src)
    {
      build_copy(src);
    }

    /** @brief Default constructor.
     *
     *  Creates an empty graph and an empty mapping.
     *
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    GraphCopyWithMapping() = default;

    /** @brief Move constructor.
     *
     *  Transfers ownership of the graph and mapping from another instance.
     *
     *  @param[in,out] other The instance to move from.
     *  @throws none
     *  @post other is left in the default-constructed state (empty graph and mapping).
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    GraphCopyWithMapping(GraphCopyWithMapping && other) noexcept = default;

    /** @brief Move assignment operator.
     *
     *  Transfers ownership of the graph and mapping from another instance.
     *
     *  @param[in,out] other The instance to move from.
     *  @return Reference to this instance.
     *  @throws none
     *  @post other receives the previous contents of *this (swap semantics).
     *        If *this was empty, other becomes empty.
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    GraphCopyWithMapping &operator=(GraphCopyWithMapping && other) noexcept = default;

    // Disable copy (graphs can be large)
    GraphCopyWithMapping(const GraphCopyWithMapping &) = delete;

    GraphCopyWithMapping &operator=(const GraphCopyWithMapping &) = delete;

    /** @brief Get the copied graph.
     *
     *  @return Reference to the copied graph.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    GT &get_graph() noexcept { return copied_graph; }

    /** @brief Get the copied graph (const version).
     *
     *  @return Const reference to the copied graph.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    const GT &get_graph() const noexcept { return copied_graph; }

    /** @brief Get the copy of an original node.
     *
     *  Retrieves the node in the copied graph that corresponds to the given
     *  node in the original graph.
     *
     *  @param[in] orig Pointer to the original node.
     *  @return Pointer to the corresponding node in the copy.
     *  @throws std::domain_error if the node is not in the mapping.
     *  @note Complexity: O(log V)
     *  @note Exception safety: Strong guarantee.
     */
    Node * get_copy(Node *orig) const
    {
      auto *ptr = node_map.search(orig);
      ah_domain_error_if(ptr == nullptr)
      << "Node not found in mapping (not from original graph?)";
      return ptr->second;
    }

    /** @brief Search for the copy of an original node (no exception).
     *
     *  Retrieves the node in the copied graph that corresponds to the given
     *  node in the original graph, or nullptr if not found.
     *
     *  @param[in] orig Pointer to the original node.
     *  @return Pointer to the corresponding node in the copy, or nullptr if not found.
     *  @throws none
     *  @note Complexity: O(log V)
     *  @note Exception safety: noexcept
     */
    Node * search_copy(Node *orig) const noexcept
    {
      auto *ptr = node_map.search(orig);
      return ptr ? ptr->second : nullptr;
    }

    /** @brief Check if an original node is in the mapping.
     *
     *  @param[in] orig Pointer to the original node.
     *  @return true if the node is mapped, false otherwise.
     *  @throws none
     *  @note Complexity: O(log V)
     *  @note Exception safety: noexcept
     */
    bool has_copy(Node *orig) const noexcept
    {
      return node_map.search(orig) != nullptr;
    }

    /** @brief Get the number of mapped nodes.
     *
     *  @return Number of nodes in the mapping.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] size_t num_nodes() const noexcept { return node_map.size(); }

    /** @brief Get the number of arcs in the copied graph.
     *
     *  @return Number of arcs in the copy.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] size_t num_arcs() const noexcept { return copied_graph.get_num_arcs(); }

    /** @brief Apply a function to each (original, copy) node pair.
     *
     *  Iterates over all mappings and applies the given function.
     *
     *  @tparam Op Function type taking (Node* original, Node* copy).
     *  @param[in] op Function to apply.
     *  @throws whatever op throws.
     *  @note Complexity: O(V)
     *  @note Exception safety: Basic guarantee (depends on op).
     */
    template <typename Op>
    void for_each_mapping(Op op) const
    {
      node_map.for_each([&op](const auto & pair)
                          {
                            op(pair.first, pair.second);
                          });
    }

    /** @brief Insert a new node into the copied graph (not mapped).
     *
     *  Useful for adding auxiliary nodes (like dummy nodes in Johnson's algorithm)
     *  that don't correspond to any original node.
     *
     *  @param[in] info Node information.
     *  @return Pointer to the new node in the copied graph.
     *  @throws std::bad_alloc if memory allocation fails.
     *  @note Complexity: O(1)
     *  @note Exception safety: Strong guarantee.
     */
    Node * insert_unmapped_node(const Node_Type & info = Node_Type())
    {
      return copied_graph.insert_node(info);
    }

    /** @brief Insert a new node into the copied graph (not mapped, move version).
     *
     *  Useful for adding auxiliary nodes (like dummy nodes in Johnson's algorithm)
     *  that don't correspond to any original node.
     *
     *  @param[in] info Node information (moved).
     *  @return Pointer to the new node in the copied graph.
     *  @throws std::bad_alloc if memory allocation fails.
     *  @note Complexity: O(1)
     *  @note Exception safety: Strong guarantee.
     */
    Node * insert_unmapped_node(Node_Type && info)
    {
      return copied_graph.insert_node(std::forward<Node_Type>(info));
    }

    /** @brief Remove a node from the copied graph.
     *
     *  If the node was mapped, the mapping entry is removed to keep the
     *  mapping consistent.
     *
     *  @param[in] node Pointer to the node to remove.
     *  @throws Any exception thrown by the underlying mapping or graph removal operations.
     *  @note Complexity: O(degree + V) for List_Graph, O(E + V) for List_Digraph (due to reverse lookup in map).
     *  @note Exception safety: This function does not add new throwing points beyond those of the underlying containers.
     */
    void remove_node(Node *node)
    {
      // Maintain node_map consistency when a copied node is removed:
      // perform a reverse lookup (value -> key) and erase the corresponding entry.
      // This is O(N) because DynMapTree is keyed by original node, not by copy.
      Node *original_node = nullptr;
      node_map.for_each([&](const auto & pair)
                          {
                            if (pair.second == node)
                              {
                                original_node = pair.first;
                              }
                          });

      if (original_node)
          node_map.remove(original_node);

      copied_graph.remove_node(node);
    }

    /** @brief Insert an arc into the copied graph.
     *
     *  @param[in] src Source node (in the copy).
     *  @param[in] tgt Target node (in the copy).
     *  @param[in] info Arc information.
     *  @return Pointer to the new arc.
     *  @throws std::bad_alloc if memory allocation fails.
     *  @note Complexity: O(1)
     *  @note Exception safety: Strong guarantee.
     */
    Arc * insert_arc(Node *src, Node *tgt, const Arc_Type & info = Arc_Type())
    {
      return copied_graph.insert_arc(src, tgt, info);
    }

    /** @brief Remove an arc from the copied graph.
     *
     *  @param[in] arc Pointer to the arc to remove.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Exception safety: noexcept.
     */
    void remove_arc(Arc *arc)
    {
      copied_graph.remove_arc(arc);
    }

    /** @brief Clear the copied graph and mapping.
     *
     *  Removes all nodes and arcs from the copied graph and clears the mapping.
     *
     *  @throws none
     *  @note Complexity: O(V + E)
     *  @note Exception safety: noexcept.
     */
    void clear()
    {
      node_map.empty();
      Aleph::clear_graph(copied_graph);
    }
  };
} // end namespace Aleph

# endif /* TPL_GRAPH_H */
