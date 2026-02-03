
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
# include <ah-errors.H>

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

  /** Node belonging to a graph implemented with double linked
        adjacency list.

        This class is used for defining a node of graph implemented with
        double linked adjacency lists.

        Basically, there are three ways for defining the information to
        be stored in a node:

        1. Specifying the data as template parameter of this class.
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
    Graph_Node(const Node_Info & info) noexcept : Base(info)
    { /* empty */
    }

    /** Move or rvalue constructor

          Construct a node whose data is moved from `info`. The control
          attributes are reset.

          @param[in] info data to move to the node

          @note The move constructor of `Node_Info` must be defined
      */
    Graph_Node(Node_Info && info = Node_Info()) noexcept
      : Base(std::move(info))
    {
      /* empty */
    }

    Graph_Node(const Graph_Node & node) noexcept
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

          The control bits and counter values are set to zero. The
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

  /** Arc of graph implemented with double linked adjacency lists.

        This class is used for defining an arc of graph implemented with
        double linked adjacency lists.

        Basically, there are three ways for defining the information to
        be stored in an arc:

        1. Specifying the data as template parameter of this class.
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
    Graph_Arc(const Arc_Info & info) noexcept
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
    Graph_Arc(Arc_Info && info = Arc_Info()) noexcept
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

  /** Graph implemented with double linked adjacency lists

        This class manages two template parameters
        - `_Graph_Node`: the type of node, which previously must have
        been defined through of Graph_Node class.
        - `_Graph_Arc`: the type of arc, which previously must have
        been defined through of `Graph_Arc` class.

        It is highly recommendable to use an alias for the final graph
        type. Some such as:

        using My_Graph = List_Graph<Graph_Node<My_Node_Type>,
        Graph_Arc<My_Arc_Type>>;

        Once instantiated a `List_Graph` object, its nodes and arcs must
        be accessed through the following subclasses:
        - `List_Graph<Node, Arc>::Node`
        - `List_Graph<Node, Arc>::Arc`

        Directed or non-directed? (how and when using them)
        ===================================================

        Using of alias is very "natural". For the previous shown alias
        example, you could type:

        My_Graph::Node * node_ptr = ....
        My_Graph::Arc * arc_ptr = ....

        This graph can be directed or non-directed. From a functional
        perspective, what gives the difference between a directed graph
        and a non-directed one, is the way for accessing to arcs given a
        node. Perhaps an example could help to understand
        better. Suppose that you have a graph `g` and two node pointers
        `p` and `q` respectively. Now, in order to specify an arc, you
        execute some such as:

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
        no direct way for knowing the incoming arcs, the node removal
        requires to traverse all the arcs for checking for those
        incoming to the removed one. So, the node removal is
        \f$O(E)\f$.

        Versatility (`List_Graph` vs `List_Digraph`)
        ---------------------------------------------

        So that, a guideline for choosing `List_Graph` vs `List_Digraph`
        is to ask if the node removal will be used. If yes, then how
        often? If you know that the node removal will be used several
        times, then prefer `List_Graph`; otherwise, that is, you know
        there is no node removals, then use `List_Digraph`, since it
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
    /// Returns reference to internal node Dlink for sorting operations
    Dlink & get_node_dlink() noexcept { return node_list; }

    /// Returns reference to internal arc Dlink for sorting operations
    Dlink & get_arc_dlink() noexcept { return arc_list; }
    /** Insertion of a node already allocated.

          This method takes a pointer to a `node` already allocated and
          inserts it in the graph.

          The node allocation and its live in memory is user's
          responsibility. At this regard, take in account that the graph
          destructor and `remove_method()` will assume that the nodes
          were allocated with `new` operator, and consequently it will call
          to `delete` for deallocating. So, it is very dangerous to use
          nodes whose memory does not come from `new`.

          \note Since that it is programmer responsibility to allocate
          memory for the node and in addition the graph destructor and the
          `remove_noe()` will call to `delete` operator, it is preferable
          and highly recommended to use any other overloaded version of
          `insert_node()` which automatically allocates node's memory. In
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
          node from graph and frees its memory. All the memory occupied
          by the arcs related to the node and the node itself is freed.

          The methods takes \f$O(1)\f$ on `List_Graph` objects and
          **\f$O(E)\f$ on `List_Digraph` objects**.

          @param[in] node to be removed and freed
      */
    virtual void remove_node(Node *node) noexcept
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
        this->remove_arcs_if([node, this](auto a) {
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

          This method serves as an arc of depart given a node. Some
          algorithms only need any arc adjacent to a node in order to start
          their computations. In these cases, this method is indicated.

          \note The idea of "first arc" does not obey any order; the method
          simply returns a arc. The majority of algorithms should be
          independent of processing order of arcs

          \return A pointer to an arc adjacent to `node`
      */
    Arc * get_first_arc(Node *node) const
    {
      ah_range_error_if(get_num_arcs (node) == 0) << "node has not arcs";

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
      if (not this->digraph) // if digraph ==> do not insert in other node
        { // insertion in target node
          if (src_node == tgt_node) // is it a loop?
            arc->tgt_arc_node = src_arc_node.get();
          else
            { // allocate arc node for tgt_node
              std::unique_ptr<Arc_Node> tgt_arc_node = std::make_unique<Arc_Node>(arc);

              // insertion in adjacency list of tgt_node
              arc->tgt_arc_node = tgt_arc_node.get();
              tgt_node->arc_list.append(tgt_arc_node.get());
              ++tgt_node->num_arcs;
              tgt_arc_node.release();
            }
        }

      // step 3 (remainder): insertion in adjacency list of src_node
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
      arc->del(); // unlink arc from graph arc list
      --this->num_arcs;
      delete arc;
    }

    /** Disconnect an arc from graph.

          This method removes an arc from the graph; that is, it does no
          longer belong to the graph. However, the memory is not freed.

          Although the use of this method is occasional, it is not
          exceptional. For example, some situations require temporarily
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

      /// Return the source node of current arc. Throw `overflow_error` if
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
        transportation. Node are the cities and arcs are the pathways. Now
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
  template <class GT, class SN = Dft_Show_Node<GT>>
  void for_each_node(const GT & g,
                     std::function<void (typename GT::Node *)> operation,
                     SN sn = SN())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      operation(it.get_curr());
  }

  /** Traverse all the arcs of graph filtering some ones according to a
        condition and executing an operation on them.

        @param[in] g the graph
        @param[in] operation to be executed on each seen arc
        @param[in] sa an arc filter

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  void for_each_arc(const GT & g,
                    std::function<void (typename GT::Arc *)> operation,
                    SA sa = SA())
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      operation(it.get_curr());
  }

  /** Traverse all the arcs adjacent to a node filtering some ones
        according to a condition and executing an operation on them.

        @param[in] g the graph
        @param[in] p a node pointer
        @param[in] operation to be executed on each seen arc
        @param[in] sa an arc filter

        @ingroup Graphs
    */
	  template <class GT, class SA = Dft_Show_Arc<GT>>
	  void for_each_arc(const GT & g,
	                    typename GT::Node *p,
	                    std::function<void (typename GT::Arc *)> operation,
	                    SA sa = SA())
	  {
	    (void) g;
	    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
	      operation(it.get_curr());
	  }

  /** Return `true` if condition `cond` is met on every filtered node of
        the graph.

        @param[in] g the graph
        @param[in] cond condition
        @param[in] sn node filter
        \result `true` if all the nodes satisfy the condition; `false` otherwise

        @ingroup Graphs
    */
  template <class GT, class SN = Dft_Show_Node<GT>>
  bool forall_node(const GT & g,
                   std::function<bool (typename GT::Node *)> cond,
                   SN sn = SN())
  {
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if (not cond(it.get_curr()))
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
  template <class GT, class SA = Dft_Show_Arc<GT>>
  bool forall_arc(const GT & g,
                  std::function<bool (typename GT::Arc *)> cond,
                  SA sa = SA())
  {
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      if (not cond(it.get_curr()))
        return false;
    return true;
  }

  /** Return `true` if condition `cond` is met on every filtered arc of a
        node.

        @param[in] p node pointer
        @param[in] cond condition
        @param[in] sa arc filter
        \result `true` if all the arcs satisfy the condition; `false` otherwise

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  bool forall_arc(typename GT::Node *p,
                  std::function<bool (typename GT::Arc *)> cond,
                  SA sa = SA())
  {
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      if (not cond(it.get_curr()))
        return false;
    return true;
  }

  /** Map the filtered nodes of a graph to a transformed type.

        @tparam GT Graph type
        @tparam T Result type
        @tparam Container Container type (default: DynList)
        @tparam SN Node filter type (default: Dft_Show_Node<GT>)
        @param g the graph
        @param transformation function `Node* -> T` that transforms each node
        @param sn node filter functor (default: Dft_Show_Node<GT>)
        @return a `Container<T>` object with all the nodes mapped through the
        transformation

        @ingroup Graphs
    */
  template <class GT, typename T,
            template<typename> class Container = DynList,
            class SN = Dft_Show_Node<GT>>
  Container<T> nodes_map(GT & g,
                         std::function<T (typename GT::Node *)> transformation,
                         SN sn = SN())
  {
    Container<T> ret_val;
    for_each_node<GT, SN>(g, [&ret_val, &transformation](typename GT::Node *p)
                            {
                              ret_val.append(transformation(p));
                            }, sn);
    return ret_val;
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
  template <class GT, typename T,
            template<typename> class Container = DynList,
            class SA = Dft_Show_Arc<GT>>
  Container<T> arcs_map(GT & g,
                        std::function<T (typename GT::Arc *)> transformation,
                        SA sa = SA())
  {
    Container<T> ret_val;
    for_each_arc<GT, SA>(g, [&ret_val, &transformation](typename GT::Arc *p)
                           {
                             ret_val.append(transformation(p));
                           }, sa);
    return ret_val;
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
  template <class GT, typename T,
            template<typename> class Container = DynList,
            class SA = Dft_Show_Arc<GT>>
  Container<T> arcs_map(GT & g,
                        typename GT::Node *p,
                        std::function<T (typename GT::Arc *)> transformation,
                        SA sa = SA())
  {
    Container<T> ret_val;
    for_each_arc<GT, SA>(g, p, [&ret_val, &transformation](typename GT::Arc *p)
                           {
                             ret_val.append(transformation(p));
                           }, sa);
    return ret_val;
  }

  /** Fold the filtered nodes of a graph.

        @param[in] g the graph
        @param[in] init initial value of folded result
        @param[in] operation folding operation on the node
        @param[in] sn node filter
        \result the final folded result

        @ingroup Graphs
    */
  template <class GT, typename T, class SN = Dft_Show_Node<GT>>
  T foldl_nodes(GT & g, const T & init,
                std::function<T (const T &, typename GT::Node *)> operation,
                SN sn = SN())
  {
    T ret_val = init;
    for_each_node<GT, SN>(g, [&ret_val, &operation](typename GT::Node *p)
                            {
                              ret_val = operation(ret_val, p);
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
  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  T foldl_arcs(GT & g, const T & init,
               std::function<T (const T &, typename GT::Arc *)> operation,
               SA sa = SA())
  {
    T ret_val = init;
    for_each_arc<GT, SA>(g, [&ret_val, &operation](typename GT::Arc *a)
                           {
                             ret_val = operation(ret_val, a);
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
  template <class GT, typename T, class SA = Dft_Show_Arc<GT>>
  T foldl_arcs(GT & g, typename GT::Node *p,
               const T & init,
               std::function<T (const T &, typename GT::Arc *)> operation,
               SA sa = SA())
  {
    T ret_val = init;
    for_each_arc<GT, SA>(g, p, [&ret_val, &operation](typename GT::Arc *a)
                           {
                             ret_val = operation(ret_val, a);
                           }, sa);
    return ret_val;
  }

  /** Directed graph (digraph) implemented with double-linked adjacency lists.

      This class models a directed graph. Functionally it is
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
    /// there is no current
    void prev() { it.prev(); }

    /// Return `true` the iterator has an current arc
    bool has_curr() const noexcept
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

    /// Reset the iterator to the last arc
    void reset_last() noexcept
    {
      it.reset_last();
    }

    /// Put the iterator in end state
    void end() noexcept
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

    void next_ne() noexcept
    {
      Base::next_ne();
      goto_first_valid();
    }
  };

  /** Filtered iterator for incoming arcs of a node.

      This iterator traverses the incoming arcs of a node, optionally
      filtered by the Show_Arc predicate.

      @tparam GT The graph type
      @tparam SA Arc filter predicate (default shows all arcs)
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

  public:
    using Item_Type = typename GT::Arc *;

    In_Iterator() = default;

    In_Iterator(typename GT::Node *p, SA sa = SA())
      : Base(p), show_arc(sa)
    {
      goto_first_valid();
    }

    void next()
    {
      Base::next();
      goto_first_valid();
    }

    void next_ne() noexcept
    {
      Base::next_ne();
      goto_first_valid();
    }
  };

  /** Return the nodes connected to the filtered outcoming arcs of `p`.

        @param[in] p node pointer
        @param[in] sa arc filter
        \result A `DynList<typename GT::Node*>` with the pointer to the
        outcoming nodes

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Node *> out_nodes(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Node *> ret;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_node_ne());
    return ret;
  }

  /** Return the nodes connected to the filtered incoming arcs to `p`.

        @param[in] p node pointer
        @param[in] sa arc filter
        \result A `DynList<typename GT::Node*>` with the pointer to the
        incoming nodes
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Node *> in_nodes(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Node *> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_node_ne());
    return ret;
  }

  /** Return the filtered incoming arcs of `p`.

        @param[in] p node pointer
        @param[in] sa arc filter

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> out_arcs(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return the filtered incoming arcs of `p`.

        @param[in] p node pointer
        @param[in] sa arc filter

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> in_arcs(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return all the filtered arcs related to `p`

        @param[in] p node pointer
        @param[in] sa arc filter
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *> arcs(typename GT::Node *p, SA sa = SA())
  {
    DynList<typename GT::Arc *> ret;
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ret.append(it.get_curr());
    return ret;
  }

  /** Return the filtered incoming pairs of (arc,node) related to node `p`

        @param[in] p node pointer
        @param[in] sa arc filter
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  DynList<ArcPair<GT>> in_pairs(typename GT::Node *p, SA sa = SA())
  {
    DynList<ArcPair<GT>> ret;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc *a = it.get_curr();
        ret.append(std::make_tuple(a, static_cast<typename GT::Node *>(a->get_connected_node(p))));
      }
    return ret;
  }

  /** Return the filtered outcoming pairs of (arc,node) related to node `p`

        @param[in] p node pointer
        @param[in] sa arc filter
        \result a `DynList<ArcPair<GT>>` containing the outcoming pairs
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
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

        @param[in] p node pointer
        @param[in] sa arc filter
        \result the incoming degree
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  size_t in_degree(typename GT::Node *p, SA sa = SA())
  {
    size_t count = 0;
    for (In_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  template <class GT>
  size_t in_degree(typename GT::Node *p)
  {
    size_t count = 0;
    for (_In_Iterator<GT> it(p); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  /** Compute the filtered out degree of node `p`

        \note This function computes the degree, it does not retrieve it.

        @param[in] p node pointer
        @param[in] sa arc filter
        \result the outcoming degree

        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  size_t out_degree(typename GT::Node *p, SA sa = SA())
  {
    size_t count = 0;
    for (Out_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      ++count;
    return count;
  }

  template <class GT>
  size_t out_degree(typename GT::Node *p)
  {
    size_t count = 0;
    for (_Out_Iterator<GT> it(p); it.has_curr(); it.next_ne())
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
  template <class GT, class Itor, class Operation>
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
  template <class GT, class Itor, class Operation>
  inline
  void for_each_arc(typename GT::Node *p, Operation op = Operation())
  {
    for (Itor it(p); it.has_curr(); it.next_ne())
      op(it.get_curr());
  }


  // Functional operations on input arcs

  /** Conditioned traversal of incoming arcs of a node.

        @param[in] p node pointer
        @param[in] op operation whose result must be `bool`. Si the result
        is `false`, then the traversal stops and the traverse returns
        `false`; otherwise, all the ars are traversed and it returns `true`
        \result `true` is all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <class GT, class Op>
  inline
  bool traverse_in_arcs(typename GT::Node *p, Op op = Op())
  {
    return traverse_arcs<GT, _In_Iterator<GT>, Op>(p, op);
  }

  /** Traverse the incoming arcs of a node and executes an operation

        @param[in] p node pointer
        @param[in] op operation to perform on each node
        @ingroup Graphs
    */
  template <class GT, class Op>
  inline
  void for_each_in_arc(typename GT::Node *p, Op op = Op())
  {
    for_each_arc<GT, _In_Iterator<GT>, Op>(p, op);
  }

  /** Test if the incoming arcs meet a condition.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op testing operation
        \result `true` if `op` returns `true` for all the incoming arcs
    */
  template <class GT, class Op>
  inline
  bool all_in_arc(typename GT::Node *p, Op op = Op())
  {
    return traverse_in_arcs(p, [&op](auto a)
                              {
                                return op(a);
                              });
  }

  /** Test if it exists an incoming arc satisfying an operation.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result `true` if there is at least an arc satisfying the operation
    */
  template <class GT, class Op>
  inline
  bool exists_in_arc(typename GT::Node *p, Op op = Op())
  {
    return not traverse_in_arcs<GT, Op>(p, [&op](auto a)
                                          {
                                            return not op(a);
                                          });
  }

  /** Search an incoming arc to a node satisfying a condition `op`.

        `op` must have the following signature:

        bool op(Arc * arc)

        @param[in] p node pointer
        @param[in] op operation for testing on each arc
        \result a pointer to a node satisfying the operation if this is
        found; `nullptr` otherwise
    */
  template <class GT, class Op>
  inline
  auto search_in_arc(typename GT::Node *p, Op op = Op())
  {
    typename GT::Arc *ret = nullptr;
    traverse_in_arcs(p, [&op, &ret](auto a)
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

        Where `T` is the transformed type from the arc.

        @param[in] p node pointer
        @param[in] op transformation
        \result a `DynList<T>` containing the mapping.
    */
  template <class GT, typename T>
  inline
  auto map_in_arcs(typename GT::Node *p, std::function<T (typename GT::Arc *)> op)
  {
    DynList<T> ret;
    for_each_in_arc(p, [&ret, &op](auto a)
                      {
                        ret.append(op(a));
                      });
    return ret;
  }

  /** Fold the incoming arcs of a node.

        This method folds the incoming arcs to a final folded type `T`. Each
        folding is done by an operation `op`, which must have the following
        signature:

        T op(const T & acc, Arc * arc)

        `acc` often is called an accumulator. `op` should take the
        accumulator and the arc and to produce a folded result.

        @param[in] p node pointer
        @param[in] init initial value of folding
        @param[in] op folding operation
        \result the final folded result after the last seen arc
    */
  template <class GT, typename T>
  inline
  T foldl_in_arcs(typename GT::Node *p, const T & init,
                  std::function<T (const T &, typename GT::Arc *)> op)
  {
    T ret = init;
    for_each_in_arc(p, [&ret, &op](auto a)
                      {
                        ret = op(ret, a);
                      });
    return ret;
  }

  /** Filter the incoming arcs meeting an condition.

        @param[in] p node pointer
        @param[in] cond filtering condition
        \result a `DynList<typename GT::Arc*>` containing the filtered arcs
    */
  template <class GT, class Op>
  inline
  DynList<typename GT::Arc *> filter_in_arcs(typename GT::Node *p, Op cond)
  {
    DynList<typename GT::Arc *> ret;
    for_each_in_arc(p, [&ret, &cond](auto a)
                      {
                        if (cond(a))
                          ret.append(a);
                      });
    return ret;
  }


  // Functional operation on output arcs

  /** Conditioned traversal of outcoming arcs of a node.

        @param[in] p node pointer
        @param[in] op operation whose result must be `bool`. Si the result
        is `false`, then the traversal stops and the traverse returns
        `false`; otherwise, all the ars are traversed and it returns `true`
        \result `true` is all the arcs were traversed: `false` otherwise
        @ingroup Graphs
    */
  template <class GT, class Op>
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
  template <class GT, class Op>
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
  template <class GT, class Op>
  inline
  bool all_out_arc(typename GT::Node *p, Op op = Op())
  {
    return traverse_out_arcs(p, [&op](auto a)
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
  template <class GT, class Op>
  inline
  bool exists_out_arc(typename GT::Node *p, Op op = Op())
  {
    return not traverse_out_arcs<GT, Op>(p, [&op](auto a)
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
  template <class GT, class Op>
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
  template <class GT, typename T>
  inline
  auto map_out_arcs(typename GT::Node *p, std::function<T (typename GT::Arc *)> op)
  {
    DynList<T> ret;
    for_each_out_arc(p, [&ret, &op](auto a)
                       {
                         ret.append(op(a));
                       });
    return ret;
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
  template <class GT, typename T>
  inline
  T foldl_out_arcs(typename GT::Node *p, const T & init,
                   std::function<T (const T &, typename GT::Arc *)> op)
  {
    T ret = init;
    for_each_out_arc(p, [&ret, &op](auto a)
                       {
                         ret = op(ret, a);
                       });
    return ret;
  }

  /** Filter the outcoming arcs meeting an condition.

        @param[in] p node pointer
        @param[in] cond filtering condition
        \result a `DynList<typename GT::Arc*>` containing the filtered arcs
    */
  template <class GT, class Op>
  inline
  DynList<typename GT::Arc *> filter_out_arcs(typename GT::Node *p, Op cond)
  {
    DynList<typename GT::Arc *> ret;
    for_each_out_arc(p, [&ret, &cond](auto a)
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
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  typename GT::Arc *
  search_arc(const GT & g,
             typename GT::Node *src, typename GT::Node *tgt,
             SA sa = SA()) noexcept
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
        @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
	  typename GT::Arc *
	  search_directed_arc(const GT & g,
	                      typename GT::Node *src, typename GT::Node *tgt,
	                      SA sa = SA()) noexcept
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
  template <class GT>
  typename GT::Node * mapped_node(typename GT::Node *p) noexcept
  {
    return static_cast<typename GT::Node *>(NODE_COOKIE(p));
  }

  /// Return the mapped arc through the cookie of `p`
  template <class GT>
  typename GT::Arc * mapped_arc(typename GT::Arc *a) noexcept
  {
    return static_cast<typename GT::Arc *>(ARC_COOKIE(a));
  }

  /// \overload mapped_node(typename GT::Node * p)
  template <class GTSRC, class GTTGT>
  typename GTTGT::Node * mapped_node(typename GTSRC::Node *p) noexcept
  {
    return static_cast<typename GTTGT::Node *>(NODE_COOKIE(p));
  }

  /// \overload  mapped_arc(typename GT::Arc * a)
  template <class GTSRC, class GTTGT>
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
        @throw bad_alloc if there is no enough memory
        @ingroup Graphs
    */
  template <class GT>
  inline
  void copy_graph(GT & gtgt, const GT & gsrc, bool cookie_map = false);

  /** Clean a graph: all its nodes and arcs are removed and freed.

        @param[in] g the graph
        @ingroup Graphs
    */
  template <class GT>
  inline void clear_graph(GT & g) noexcept;

  /** Functor that traverses the nodes of a graph and performs an
        operation.

        This functor has three template parameters:
        1. `GT`: the graph type
        2. `Operation`: operation functor to be performed on each node
        3. `SN`: node filter

        @ingroup Graphs
    */
  template <class GT, class Operation, class SN = Dft_Show_Node<GT>>
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
  template <class GT, class Operation, class SA = Dft_Show_Arc<GT>>
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
      for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
        op(g, it.get_current_arc_ne());
    }

    void operator()(GT & g, typename GT::Node *p,
                    Operation op = Operation()) const
    {
      for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
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
      ah_domain_error_if(g == nullptr)
      << "Path: Graph has not been specified";
    }

  public:
    /// Return `true` if the path is consistent
    bool check() const
    {
      auto l = list;
      while (not l.is_unitarian_or_empty())
        {
          auto d = l.remove_first();
          auto nxt = l.get_first().node;
          if (nxt == g->get_connected_node(d.arc, d.node))
            return false;
        }

      return true;
    }

    /// Return `true` if the directed path is consistent
    bool check_directed() const
    {
      return list.all([this](auto d)
                        {
                          return g->get_src_node(d.arc) == d.node;
                        })
             and list.get_last().arc == nullptr;
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

          @param start_node pointer to start node of path
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
          @param[in] start_node pointer to start node of path
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
    size_t size() const noexcept
    {
      return list.size();
    }

    /// Return `true` if the path is empty
    bool is_empty() const noexcept
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

    /// Copy constructor
    Path(const Path & path) : g(path.g), list(path.list) {}

    /// Move constructor
    Path(Path && path) : g(path.g), list(std::move(path.list)) {}

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
          of path.

          @param[in] node pointer to the node to append in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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
          of path.

          @param[in] p node pointer to the node to append in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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
          directed path.

          \note This method is exclusively for graphs non directed treated
          as directed.

          If the operation is successful, then `node` becomes the last node
          of path.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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

    /** Insert an arc as the first of path.

          This method inserts `arc` at the beginning of a path. For this,
          the adjacent arcs of the current first node of path are examined
          until find a arc linking `node`.

          If the operation is successful, then `node` becomes the first node
          of path.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the first
          node of path
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

          This method inserts `node` at the beginning of path. For this, the
          adjacent arcs of the last node are examined until finding an arc
          linking `node`.

          If the operation is successful, then `node` becomes the first node
          of path.

          @param[in] node pointer to the node to insert in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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
      ah_domain_error_if(arc == nullptr) << "There is no arc connecting node";

      Path_Desc item(node, arc);
      list.insert(item);
    }

    /** Append a node to a directed path

          This method inserts `node` at the end of a directed path. For
          this, the outcoming arcs of the last node of path are examined
          until finding an arc linking `node`.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          If the operation is successful, then `node` becomes the last node
          of path.

          @param[in] p node pointer to the node to insert in the path
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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
      ah_domain_error_if(arc == nullptr) << "There is no an arc connecting to the node";

      list.insert(Path_Desc(p, arc));
    }

    /** Append an arc to a directed path

          This method appends `arc` at the beginning of a directed path. For
          this, the target node of `arc` must be the current first node of
          directed path.

          \note This method is exclusively for graphs non-directed treated
          as directed.

          @param[in] arc pointer
          @throw bad_alloc if there is no enough memory
          @throw invalid_argument if `node` is not reachable from the last
          node of path
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

    /// Return the last arc of path; throws `overflow_error` if path
    /// is empty
    Arc * get_last_arc() const
    {
      ah_domain_error_if(list.is_unitarian())
        << "Path with only a node (without any arc)";

      typename DynDlist<Path_Desc>::Iterator it(list);
      it.reset_last();
      it.prev();
      return it.get_curr().arc;
    }

    /// Return `true` if `this` is a cycle; throws if path is empty
    bool is_cycle() const
    {
      return get_first_node() == get_last_node();
    }

    /** Remove the last node of path

          \return pointer to the extracted node
          \throw overflow_error if path is empty
      */
    Node * remove_last_node()
    {
      auto d = list.remove_last();
      list.get_last().arc = nullptr;
      return d.node;
    }

    /** Remove the first node of path

          \return pointer to the extracted node
          \throw overflow_error if path is empty
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
    class Iterator : public DynDlist<Path_Desc>::Iterator
    {
    public:
      /// Create an iterator on the first node of `path`
      Iterator(const Path & path) noexcept
        : DynDlist<Path_Desc>::Iterator(path.list)
      {}

    private:
      Path_Desc &get_curr_path_desc_ne() const noexcept
      {
        return this->DynDlist<Path_Desc>::Iterator::get_curr_ne();
      }

      Path_Desc &get_curr_path_desc() const
      {
        return this->DynDlist<Path_Desc>::Iterator::get_curr();
      }

    public:
      /// Return the current node of path. Throw `overflow_error` if path
      /// is empty of iterator has not current node
      Node * get_current_node() const
      {
        return this->get_curr_path_desc().node;
      }

      Node * get_current_node_ne() const noexcept
      {
        return this->get_curr_path_desc_ne().node;
      }

      /** Return the current arc of path.

            \note A path of `n` nodes has `n - 1` arcs. So if the iterator
            is positioned on the last node, the `get_current_arc()` has not
            current arc and will throw exception.

            \return pointer to the current arc
            \throw overflow_error if path is empty of iterator has not
            current node
        */
      Arc * get_current_arc() const
      {
        ah_overflow_error_if(this->is_in_last())
          << "Path iterator is in last node of path";

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
            second element its outcoming arc
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
            second element its outcoming arc
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
      bool has_current_arc() const noexcept
      {
        return this->has_curr() and not this->is_in_last();
      }

      /// Return `true` if the iterator has a current node
      bool has_current_node() const noexcept
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

    /// Return a list with the arcs of path (order according to the path)
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

  template <class GT>
  inline
  Path<GT> find_path_depth_first(const GT & g, typename GT::Node *start_node,
                                 typename GT::Node *end_node);

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

    for (auto it = g.get_arc_it(curr_node); it.has_curr(); it.next_ne())
      {
        auto next_arc = it.get_curr();
        if (IS_ARC_VISITED(next_arc, Find_Path))
          continue;

        ARC_BITS(next_arc).set_bit(Find_Path, true);
        if (auto next_node = it.get_tgt_node(); __find_path_depth_first<GT>(g, next_node, next_arc, end_node, path))
          {
            assert(path.get_last_node () == end_node);
            return true;
          }
      }

    path.remove_last_node();

    return false;
  }

  /** Depth first search of a path between two nodes.

        `find_path_depth_first()` recursively searches a path between
        `start_node`  y `end_node`

        @param[in] g the graph
        @param[in] start_node pointer to starting node of search
        @param[in] end_node pointer to ending node of search
        \return a `Path` object containing the path if this was found;
        an empty path otherwise
        @throw bad_alloc if there is no enough memory
        @see find_path_breadth_first()
        @ingroup Graphs
    */
  template <class GT>
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
  template <class GTS, class GTT>
  void map_nodes(typename GTS::Node *p, typename GTT::Node *q) noexcept
  {
    assert(p != nullptr and q != nullptr);

    // Use reinterpret_cast to preserve exact pointer value without any
    // implicit base class pointer adjustment from multiple inheritance
    if (NODE_COOKIE(p) == nullptr)
      {
        NODE_COOKIE(p) = reinterpret_cast<void*>(q);
        NODE_COOKIE(q) = reinterpret_cast<void*>(p);
        return;
      }

    NODE_COOKIE(q) = NODE_COOKIE(p);
    NODE_COOKIE(p) = reinterpret_cast<void*>(q);
  }

  /** Map two arcs of different types of graphs through their cookies

        \note It is intended that the mapping be done between at least
        homomorphic graphs

        @param[in] p pointer to the source arc
        @param[in] q pointer to the target arc

        @ingroup Graphs
    */
  template <class GTS, class GTT>
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

  template <class GT>
  void clear_graph(GT & g) noexcept
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

  template <class GT>
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
            gtgt.insert_node(tgt); // insert in target graph

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
      { // If exception occurs, clean this
        clear_graph(gtgt);
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
  template <class GTT, class GTS,
            class Copy_Node = Dft_Copy_Node<GTT, GTS>,
            class Copy_Arc = Dft_Copy_Arc<GTT, GTS>>
  void inter_copy_graph(GTT & gtgt, const GTS & gsrc,
                        const bool cookie_map = false)
  {
    try
      {
        clear_graph(gtgt); // clear this before copying
        DynMapAvlTree<typename GTS::Node *, typename GTT::Node *> map;
        // phase 1: traverse nodes of src_graph and insert copy in this
        for (typename GTS::Node_Iterator it(gsrc); it.has_curr(); it.next_ne())
          {
            typename GTS::Node *src_node = it.get_current_node_ne();
            std::unique_ptr<typename GTT::Node> tgt_node(new typename GTT::Node);
            Copy_Node()(tgt_node.get(), src_node);
            map.insert(src_node, tgt_node.get());

            typename GTT::Node *tgt = tgt_node.release();
            gtgt.insert_node(tgt); // insert in target graph

            if (cookie_map)
              map_nodes<GTS, GTT>(src_node, tgt);
          }

        assert(gtgt.get_num_nodes() == gsrc.get_num_nodes());

        // phase 2: for each arc of src_graph, create in this an
        // arc connecting the mapped nodes from map
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
      { // If exception occurs, clean this
        clear_graph(gtgt);
        throw;
      }
  }

  /** Filtered copy of graphs.

        This functor copies a graph according to filtering criterias
        for its nodes and arcs

        The template parameters are:
        - `GT`: the graph type
        - `SN`: node filter
        - `SA`: arc filter

        @ingroup Graphs
    */
  template <class GT, class SN = Dft_Show_Node<GT>, class SA = Dft_Show_Arc<GT>>
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

          // phase 1: traverse nodes of src_graph and insert copy in this
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

          // phase 2: for each arc of src_graph, create in this an
          // arc connecting the mapped nodes from map
          for (Arc_Iterator<GT, SA> it(gsrc, sa); it.has_curr(); it.next_ne())
            {
              typename GT::Arc *src_arc = it.get_curr();

              // get node images in target graph and create arc
              typename GT::Node *src_node = map[gsrc.get_src_node(src_arc)];
              typename GT::Node *tgt_node = map[gsrc.get_tgt_node(src_arc)];
              typename GT::Arc *tgt_arc =
                  gtgt.insert_arc(src_node, tgt_node, src_arc->get_info());

              if (cookie_map)
                GT::map_arcs(src_arc, tgt_arc);
            }
        }
      catch (...)
        { // If exception occurs, it is cleaned this
          clear_graph(gtgt);
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

        @ingroup Graphs
    */
  template <class GT, class Distance>
  struct Painted_Min_Spanning_Tree
  {
    /// Accumulative distance from the first seen arc until the last seen
    typename Distance::Distance_Type dist;

    Painted_Min_Spanning_Tree() noexcept : dist(0)
    { /* empty */
    }

    bool operator()(typename GT::Arc *a) noexcept
    {
      if (not IS_ARC_VISITED(a, Aleph::Spanning_Tree))
        return false;

      dist = dist + Distance()(a);

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
  template <class GT>
  inline
  bool are_equal(const GT & g1, const GT & g2);
} // end namespace Aleph

# endif /* TPL_GRAPH_H */
