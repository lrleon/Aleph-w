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

/** @file tpl_sgraph.H
 *  @brief Simple graph implementation with adjacency lists.
 *
 *  This file provides `List_SGraph` and `List_SDigraph`, simplified graph
 *  implementations using singly linked adjacency lists. These are memory-
 *  efficient alternatives to the standard graph classes.
 *
 *  @ingroup Graphs
 */

# ifndef TPL_SGRAPH_H
# define TPL_SGRAPH_H

# include <memory>
# include <htlist.H>
# include <tpl_graph.H>
# include <tpl_dynSetTree.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Node for graphs implemented with simple adjacency lists.

      This class defines a node or vertex of a graph or digraph
      implemented using singly-linked adjacency lists.

      There are basically three ways to define the information that
      belongs to a graph node:
      - Through the type parameter inherent to the node called
        Node_Info.
      - Through inheritance from this base class. This is the way to
        maintain different node types in a graph.
      - A combination of the above two.

      @tparam Node_Info Data type stored in the node and accessible
      via the get_info() method.

      @note This class is only intended for defining graph nodes.
      It is not intended to be used directly by users. It is
      preferable to use it through the methods provided by
      List_SGraph. While it is possible to use some methods or access
      some of its attributes, it is highly recommended not to
      use them directly. Instead, use the List_SGraph::Node type
      and its associated methods.

      A graph node has three additional attributes present for
      each node, named as follows:
      - Control bits (Bit_Fields): set of bits used by
        algorithms to mark the node.
      - Counter (counter): counter generally used to mark
        a visit or processing order.
      - cookie: opaque pointer to any other temporary information
        to be associated with the node.

      @ingroup Grafos
      @see List_SGraph List_SDigraph Graph_Sarc Bit_Fields
   */
  template <typename Node_Info = Empty_Class>
  struct Graph_Snode : public GTNodeCommon<Node_Info>
  {
    using Base = GTNodeCommon<Node_Info>;
    friend class GTNodeCommon<Node_Info>;

    DynList<void *> arc_list; ///< Adjacency list of arcs incident to this node

    /** Constructor that assigns an attribute value.

        Creates a node and assigns the value info as the information
        contained in the node.

        The control bits and counter values are set to zero.
        The cookie is set to nullptr.

        @param[in] info The information value to assign to the
        newly created node.

        @note The copy constructor of the Node_Info class must be defined.
    */
    Graph_Snode(const Node_Info & info)
      : Base(info)
    {
      /* empty */
    }

    Graph_Snode(Node_Info && info = Node_Info())
      : Base(move(info))
    {
      /* empty */
    }

    Graph_Snode(const Graph_Snode & node)
      : Graph_Snode(node.node_info)
    {
      /* empty */
    }

    Graph_Snode &operator =(const Graph_Snode & node)
    {
      if (&node == this)
        return *this;
      this->node_info = node.node_info;
      return *this;
    }

    /** Copy constructor from a node pointer.

        Creates a node and assigns node->get_info() as the information
        value contained in the node.

        The control bits and counter values are set to zero.
        The cookie is set to nullptr.

        @param[in] node Pointer to the node from which to copy the
        information value to assign to the newly created node.

        @note The copy constructor of the Node_Info class must be defined.
        @note The created instance is a different node from node.
    */
    Graph_Snode(Graph_Snode *node)
      : Base(node->get_info())
    {
      /* empty */
    }
  };

  /** Arc for graphs implemented with simple adjacency lists.

      This class defines an arc or edge of a graph or digraph
      implemented using singly-linked adjacency lists.

      There are basically three ways to define the information that
      belongs to a graph arc:
      - Through the type parameter inherent to the arc called
        Arc_Info.
      - Through inheritance from this base class. This is the way to
        maintain different arc types in a graph.
      - A combination of the above two.

      @tparam Arc_Info Data type stored in the arc and accessible
      via the get_info() method.

      @note This class is only intended for defining graph arcs.
      It is not intended to be used directly by users. It is
      preferable to use it through the methods provided by
      List_SGraph. While it is possible to use some methods or access
      some of its attributes, it is highly recommended not to
      use them directly. Instead, use the List_SGraph::Arc type
      and its associated methods.

      A graph arc has three additional attributes present for
      each arc, named as follows:
      - Control bits (Bit_Fields): set of bits used by
        algorithms to mark the arc.
      - Counter (counter): counter generally used to mark
        a visit or processing order.
      - cookie: opaque pointer to any other temporary information
        to be associated with the arc.

      @ingroup Grafos
      @see List_SGraph List_SDigraph Graph_Snode Bit_Fields
   */
  template <typename Arc_Info = Empty_Class>
  class Graph_Sarc : public GTArcCommon<Arc_Info>
  {
    using Base = GTArcCommon<Arc_Info>;

  public:
    Graph_Sarc(const Arc_Info & info)
      : Base(info)
    {
      /* empty */
    }

    Graph_Sarc(Arc_Info && info = Arc_Info())
      : Base(move(info))
    {
      /* empty */
    }

    Graph_Sarc(const Graph_Sarc & arc)
      : Graph_Sarc(arc.arc_info)
    {
      /* empty */
    }

    Graph_Sarc &operator =(const Graph_Sarc & arc)
    {
      if (&arc == this)
        return *this;
      this->arc_info = arc.arc_info;
      return *this;
    }

    Graph_Sarc(void *src, void *tgt, const Arc_Info & data)
      : Base(src, tgt, data)
    {
      // empty
    }

    Graph_Sarc(void *src, void *tgt, Arc_Info && data)
      : Base(src, tgt, move(data))
    {
      // empty
    }
  };


  /** Graph class implemented with singly-linked adjacency lists.

      List_SGraph<Node, Arc> is a class that models graphs
      represented using adjacency lists.

      The class handles two fundamental type parameters:
      - __Graph_Node: the node type that must be defined
        using the Graph_Snode class.
      - __Graph_Arc: the arc type that must be defined
        using the Graph_Sarc class.

      These classes must have been previously defined.

      Once a List_SGraph<Node, Arc> is instantiated, nodes and
      arcs must be accessed through the internal types:
      - List_SGraph<Node, Arc>::Node
      - List_SGraph<Node, Arc>::Arc

      @tparam __Graph_Node The node type. Must be defined from
      the Graph_Snode class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @tparam __Graph_Arc The arc type. Must be defined from
      the Graph_Sarc class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @see Graph_Snode Graph_Sarc
      @ingroup Grafos
      @see List_SDigraph Path
   */
  template <typename __Graph_Node = Graph_Snode<unsigned long>,
            typename __Graph_Arc = Graph_Sarc<unsigned long>>
  class List_SGraph
      : public GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
                           __Graph_Node, __Graph_Arc>
  {
  public:
    using Node = __Graph_Node;
    using Arc = __Graph_Arc;
    using Node_Type = typename Node::Node_Type;
    using Arc_Type = typename Arc::Arc_Type;

    friend class GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
                             __Graph_Node, __Graph_Arc>;

    using CommonBase = GraphCommon<List_SGraph<__Graph_Node, __Graph_Arc>,
                                   __Graph_Node, __Graph_Arc>;

    using CommonBase::insert_node;
    using CommonBase::insert_arc;

  private:
    using DynSetNode = DynSetTree<Node *, Rand_Tree>;
    using DynSetArc = DynSetTree<Arc *, Rand_Tree>;

    DynSetNode node_list; ///< Set of nodes in the graph
    DynSetArc arc_list; ///< Set of arcs in the graph

  public:
    List_SGraph() = default;

    virtual ~List_SGraph()
    {
      clear();
    }

    void swap(List_SGraph & g) noexcept
    {
      this->common_swap(g);
      node_list.swap(g.node_list);
      arc_list.swap(g.arc_list);
    }

    /// @see ALEPH_GRAPH_COPY_MOVE_CTORS
    ALEPH_GRAPH_COPY_MOVE_CTORS(List_SGraph)

    /** Node iterator for a graph.

        This class iterates over all nodes of a graph.

        @ingroup Grafos
     */
    class Node_Iterator : public DynSetNode::Iterator
    {
    public:
      /// Data type returned by get_curr()
      using Item_Type = Node *;

      /// The set type over which iteration is performed
      using Set_Type = List_SGraph;

      Node_Iterator() = default;

      Node_Iterator(const List_SGraph & g) noexcept
        : DynSetNode::Iterator(g.node_list)
      {
        // empty
      }

      /// Returns the current node
      Node * get_current_node() const
      {
        return this->get_curr();
      }

      Node * get_current_node_ne() const noexcept
      {
        return this->get_curr_ne();
      }
    };

    /** Arc iterator for a graph node.

        This class iterates over all adjacent arcs of a node
        belonging to a graph or digraph.

        @ingroup Grafos
     */
    class Node_Arc_Iterator : public DynList<void *>::Iterator
    {
      Node *src_node;

    public:
      /// Data type returned by get_curr()
      using Item_Type = Arc *;

      /// The set type over which iteration is performed
      using Set_Type = Node *;

      /// Instantiate an empty (invalid) iterator
      Node_Arc_Iterator() = default;

      /// Instantiate an iterator over node src
      Node_Arc_Iterator(Node *src) noexcept
        : DynList<void *>::Iterator(src->arc_list), src_node(src)
      {
        // empty
      }

      /// Return get_current_arc() without exception
      Arc * get_curr_ne() const noexcept
      {
        return static_cast<Arc *>(this->Iterator::get_curr_ne());
      }

      /// Return get_current_arc()
      Arc * get_curr() const
      {
        return static_cast<Arc *>(this->Iterator::get_curr());
      }

      Arc * get_current_arc_ne() const noexcept
      {
        return get_curr_ne();
      }

      Arc * get_current_arc() const
      {
        return get_curr();
      }

      /// Returns the target node of the current arc
      Node * get_tgt_node_ne() const noexcept
      {
        Arc *a = get_curr_ne();
        return static_cast<Node *>(a->get_connected_node(src_node));
      }

      /// Returns the target node of the current arc
      Node * get_tgt_node() const
      {
        Arc *a = get_curr();
        return static_cast<Node *>(a->get_connected_node(src_node));
      }
    };

    /** Iterator over arcs of a graph.

        This iterator traverses each arc of the graph. The visit order
        can be altered if the arcs are previously sorted.

        @see sort_arcs()
     */
    class Arc_Iterator : public DynSetArc::Iterator
    {
    public:
      /// Data type returned by get_curr()
      using Item_Type = Arc *;

      /// The set type over which iteration is performed
      using Set_Type = List_SGraph;

      Arc_Iterator() = default;

      Arc_Iterator(const List_SGraph & _g) noexcept
        : DynSetArc::Iterator(_g.arc_list)
      {
        // empty
      }

      /// Returns a pointer to the current arc
      Arc * get_current_arc_ne() const noexcept
      {
        return this->get_curr_ne();
      }

      /// Returns the source node of the current arc (only meaningful for digraphs)
      Node * get_src_node_ne() const noexcept
      {
        return static_cast<Node *>(get_current_arc_ne()->src_node);
      }

      /// Returns the target node of the current arc (only meaningful for digraphs)
      Node * get_tgt_node_ne() const noexcept
      {
        return static_cast<Node *>(get_current_arc_ne()->tgt_node);
      }

      /// Returns a pointer to the current arc
      Arc * get_current_arc() const
      {
        return this->get_curr();
      }

      /// Returns the source node of the current arc (only meaningful for digraphs)
      Node * get_src_node() const { return static_cast<Node *>(get_current_arc()->src_node); }

      /// Returns the target node of the current arc (only meaningful for digraphs)
      Node * get_tgt_node() const { return static_cast<Node *>(get_current_arc()->tgt_node); }
    };


    /** Insertion of a node whose memory has already been allocated.

        This method assumes a node of type List_SGraph::Node pointed to
        by the parameter p and inserts it into the graph.

        @param[in] p Pointer to an already created node that does not belong
        to any graph.

        @return Pointer to the inserted node.

        @note Since when the node is removed or the graph is destroyed
        the delete operator will be invoked, p must have been
        imperatively allocated with new.
        @note In general, this routine should not be used. Instead, use
        the other insertion method (which allocates memory automatically).
     */
    virtual Node * insert_node(Node *p)
    {
      assert(p->arc_list.is_empty());

      ++this->num_nodes;
      node_list.append(p);
      return p;
    }

  private:
    Arc * insert_arc(Node *src, Node *tgt, void *a)
    {
      Arc *arc = static_cast<Arc *>(a);

      arc->src_node = src;
      arc->tgt_node = tgt;

      src->arc_list.append(a);
      ++src->num_arcs;
      if (not this->digraph and src != tgt)
        {
          tgt->arc_list.append(a);
          ++tgt->num_arcs;
        }

      arc_list.append(arc);
      ++this->num_arcs;

      return arc;
    }

    void disconnect_arc(Arc *arc)
    {
      Node *src = static_cast<Node *>(arc->src_node);
      Node *tgt = static_cast<Node *>(arc->tgt_node);

      src->arc_list.remove_ne([arc](auto a) { return a == arc; });
      --src->num_arcs;

      if (not this->digraph and src != tgt)
        {
          tgt->arc_list.remove_ne([arc](auto a) { return a == arc; });
          --tgt->num_arcs;
        }
    }

  public:
    /** Removes the arc.

        The operation removes the arc from the graph and then frees its
        memory.

        The arc must belong to the graph and no verification is performed
        in this regard.

        @param[in] arc Pointer to the arc to remove.
     */
    virtual void remove_arc(Arc *arc) noexcept
    {
      disconnect_arc(arc);
      arc_list.remove(arc);
      --this->num_arcs;
      delete arc;
    }

    virtual void remove_node(Node *p) noexcept
    {
      DynList<Arc *> arcs_to_remove;
      arc_list.for_each([this, &arcs_to_remove, p](auto arc)
                          {
                            if (this->get_src_node(arc) == p or this->get_tgt_node(arc) == p)
                              {
                                this->disconnect_arc(arc);
                                --this->num_arcs;
                                arcs_to_remove.append(arc);
                              }
                          });

      arcs_to_remove.for_each([this](auto a)
                                {
                                  arc_list.remove(a);
                                  delete a;
                                });
      node_list.remove(p);
      --this->num_nodes;

      delete p;
    }

    Node * get_first_node() const
    {
      return static_cast<Node *>(node_list.get_first());
    }

    Arc * get_first_arc() const
    {
      return static_cast<Arc *>(arc_list.get_first());
    }

    /// Returns the first arc of node p
    /// @throws range_error if node has no arcs
    Arc * get_first_arc(Node *p) const
    {
      ah_range_error_if(this->get_num_arcs(p) == 0) << "Node has no arcs";
      return static_cast<Arc *>(p->arc_list.get_first());
    }

  private:
    void clear() noexcept
    {
      arc_list.for_each([](Arc *p) { delete p; });
      node_list.for_each([](Node *p) { delete p; });
    }

  public:
    /** Sort all arcs of the graph according to a comparison criteria.

        @note This implementation uses a DynSetTree for arc storage,
        which maintains its own ordering. Sorting requires extracting
        arcs to a list, sorting, and rebuilding the set. This is O(E log E).

        @tparam Compare Comparison functor type with signature
        bool operator()(Arc*, Arc*).
        @param[in] cmp Comparison functor.
     */
    template <class Compare>
    void sort_arcs(Compare & cmp)
    {
      DynList<Arc *> temp_list;
      arc_list.for_each([&temp_list](Arc *a) { temp_list.append(a); });
      arc_list.empty();

      // Sort the temporary list
      mergesort(temp_list, cmp);

      // Rebuild the arc_list from sorted order
      temp_list.for_each([this](Arc *a) { arc_list.append(a); });
    }

    /// @overload sort_arcs(Compare &)
    template <class Compare>
    void sort_arcs(Compare && cmp = Compare())
    {
      sort_arcs(cmp);
    }
  };

  /** Directed graph (digraph) implemented with simple adjacency lists.

      This class models a directed graph. Functionally it is
      equivalent to the List_SGraph class, except that this one
      handles directed graphs.

      This is a type alias for `Digraph<List_SGraph<...>>`, which provides
      all the directed graph functionality through the generic Digraph
      template wrapper defined in graph-dry.H.

      @tparam __Graph_Node The node type. Must be defined from
      the Graph_Snode class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @tparam __Graph_Arc The arc type. Must be defined from
      the Graph_Sarc class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @see Graph_Snode Graph_Sarc Digraph
      @ingroup Grafos
      @see List_SGraph
   */
  template <typename __Graph_Node = Graph_Snode<int>,
            typename __Graph_Arc = Graph_Sarc<int>>
  using List_SDigraph = Digraph<List_SGraph<__Graph_Node, __Graph_Arc>>;
} // end namespace Aleph

# endif /* TPL_SGRAPH_H */
