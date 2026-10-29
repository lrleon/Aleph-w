
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


/** @file tpl_agraph.H
 *  @brief Array-based graph implementation.
 *
 *  This file provides `Array_Graph` and `Array_Digraph`, graph implementations
 *  that use arrays for node storage. These offer better cache locality for
 *  dense graphs and efficient indexed access to nodes.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_AGRAPH_H
# define TPL_AGRAPH_H

# include <tpl_dynSetTree.H>
# include <array_it.H>
# include <tpl_sgraph.H>
# include <ah-errors.H>

namespace Aleph
{
  using namespace Aleph;

  /** Node of `Array_Graph`

      @ingroup Graphs
   */
  template <typename Node_Info = Empty_Class>
  class Graph_Anode
      : public Dlink,
        public GTNodeCommon<Node_Info>
  {
    using Base = GTNodeCommon<Node_Info>;
    friend class GTNodeCommon<Node_Info>;

    static constexpr size_t Contract_Factor = 4;
    static constexpr size_t Default_Cap = 4;

    void init(const size_t dim)
    {
      arcs_dim = dim;
      this->num_arcs = 0;
      contract_threshold = arcs_dim / Contract_Factor;
      arc_array = nullptr;
      if (arcs_dim == 0)
        return;

      arc_array = static_cast<void **>(malloc(arcs_dim * sizeof(void *)));
      ah_bad_alloc_if(arc_array == nullptr);
    }

  public:
    void **arc_array = nullptr;
    size_t arcs_dim = 0;
    size_t contract_threshold = 0;

    Graph_Anode()
    {
      init(0);
    }

    Graph_Anode(const Node_Info & info) : Base(info)
    {
      init(Default_Cap);
    }

    Graph_Anode(Node_Info && info) : Base(std::move(info))
    {
      init(Default_Cap);
    }

    Graph_Anode(const Graph_Anode & node) : Dlink(), Base(node.node_info)
    {
      init(0);
    }

    Graph_Anode &operator =(const Graph_Anode & node)
    {
      if (&node == this)
        return *this;
      this->node_info = node.node_info;
      return *this;
    }

    Graph_Anode(Graph_Anode *p) : Base(p->get_info())
    {
      init(0);
    }

    virtual ~Graph_Anode()
    {
      if (arc_array != nullptr)
        free(arc_array);
    }

  private:
    // Gives the array room for new_size arcs, or frees it if new_size is 0:
    // realloc() with a size of 0 may free the array and give back nullptr,
    // which would look like a failure. If realloc() fails, the array is as
    // it was, and the result is false.
    bool resize_arc_array(const size_t new_size) noexcept
    {
      if (new_size == 0)
        {
          free(arc_array);
          arc_array = nullptr;
          arcs_dim = 0;
          contract_threshold = 0;
          return true;
        }

      const auto new_array = static_cast<void **>(realloc(arc_array, new_size * sizeof(void *)));
      if (new_array == nullptr)
        return false;

      arc_array = new_array;
      arcs_dim = new_size;
      contract_threshold = arcs_dim / Contract_Factor;
      return true;
    }

    // After a removal, halves the array if it has room for four times the
    // arcs; if realloc() fails, the array keeps its room
    void contract_arc_array() noexcept
    {
      if (this->num_arcs > contract_threshold)
        return;

      (void) resize_arc_array(arcs_dim >> 1); // num_arcs/2
    }

  public:
    void allocate_more(size_t new_size)
    {
      if (new_size == 0)
        new_size = 1;

      ah_bad_alloc_if(not resize_arc_array(new_size));
    }

    void * insert_arc(void * arc) 
    {
      // Defensive: adjacency storage must be consistent before insertion.
      // If any invariant is broken, reset and reallocate using malloc.
      // NOTE: Do NOT free arc_array here - if we hit this condition, arc_array
      // likely contains garbage from uninitialized memory (not a valid allocation).
      if (arc_array == nullptr or arcs_dim == 0 or this->num_arcs > arcs_dim)
        {
          arc_array = static_cast<void **>(malloc(Default_Cap * sizeof(void *)));
          ah_bad_alloc_if(arc_array == nullptr);
          arcs_dim = Default_Cap;
          contract_threshold = arcs_dim / Contract_Factor;
          this->num_arcs = 0;
        }

      if (this->num_arcs == arcs_dim)
        allocate_more(arcs_dim << 1); // 2*arcs_dim
    
      arc_array[this->num_arcs++] = arc;

      return arc;
    }

    void remove_arc_ne(const void *arc) noexcept
    {
      for (size_t i = 0; i < this->num_arcs; ++i)
        if (arc_array[i] == arc)
          {
            arc_array[i] = arc_array[--(this->num_arcs)];
            break;
          }

      contract_arc_array();
    }

    void remove_arc(const void *arc)
    {
      bool removed = false;
      for (size_t i = 0; i < this->num_arcs; ++i)
        if (arc_array[i] == arc)
          {
            arc_array[i] = arc_array[--(this->num_arcs)];
            removed = true;
            break;
          }

      ah_domain_error_if(not removed) << "arc for deleting not found";

      contract_arc_array();
    }

    bool compress() noexcept
    {
      return resize_arc_array(this->num_arcs);
    }
  };


  template <typename Arc_Info = Empty_Class>
  class Graph_Aarc
      : public Dlink,
        public GTArcCommon<Arc_Info>
  {
    friend class GTArcCommon<Arc_Info>;
    using Base = GTArcCommon<Arc_Info>;

  public:
    Graph_Aarc(const Arc_Info & info)
      : Base(info)
    {
      /* empty */
    }

    Graph_Aarc(Arc_Info && info = Arc_Info())
      : Base(std::move(info))
    {
      /* empty */
    }

    Graph_Aarc(const Graph_Aarc & arc)
      : Graph_Aarc(arc.arc_info)
    { /* empty */
    }

    Graph_Aarc &operator =(const Graph_Aarc & arc)
    {
      if (&arc == this)
        return *this;
      this->arc_info = arc.arc_info;
      return *this;
    }

    Graph_Aarc(void *src, void *tgt, const Arc_Info & data)
      : Base(src, tgt, data)
    {
      // empty
    }

    Graph_Aarc(void *src, void *tgt, Arc_Info && data = Arc_Info())
      : Base(src, tgt, std::move(data))
    {
      // empty
    }
  };


  template <class __Graph_Node = Graph_Anode<unsigned long>,
            class __Graph_Arc = Graph_Aarc<unsigned long>>
  class Array_Graph
      : public GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
                           __Graph_Node, __Graph_Arc>
  {
  public:
    using Node = __Graph_Node;
    using Arc = __Graph_Arc;
    using Node_Type = typename Node::Node_Type;
    using Arc_Type = typename Arc::Arc_Type;

    friend class GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
                             __Graph_Node, __Graph_Arc>;

    using CommonBase = GraphCommon<Array_Graph<__Graph_Node, __Graph_Arc>,
                                   __Graph_Node, __Graph_Arc>;

    using CommonBase::insert_node;
    using CommonBase::insert_arc;

  private:
    // using DynSetNode = DynSetTree<Node*, Avl_Tree>;
    using DynSetArc = DynSetTree<Arc *, Rand_Tree>;

    Dlink node_set;
    Dlink arc_set;

  public:
    /// Returns reference to internal node Dlink for sorting operations
    Dlink & get_node_dlink() noexcept { return node_set; }

    /// Returns reference to internal arc Dlink for sorting operations
    Dlink & get_arc_dlink() noexcept { return arc_set; }

    struct Node_Iterator : public GTNodeIterator<Array_Graph>
    {
      using Base = GTNodeIterator<Array_Graph>;
      using Base::Base;

      Node_Iterator(const Array_Graph & g) noexcept
        : Base(const_cast<Dlink &>(g.node_set))
      {
        // empty
      }
    };

    /** Iterator over arcs of a graph.

           This iterator traverses each arc of the graph. The visit order
           can be altered if the arcs are previously sorted.

           @see sort_arcs()
        */
    struct Arc_Iterator : public GTArcIterator<Array_Graph>
    {
      using Base = GTArcIterator<Array_Graph>;
      using Base::Base;

      Arc_Iterator(const Array_Graph & g)
        : Base(const_cast<Dlink &>(g.arc_set))
      {
        // empty
      }
    };

    /** Iterator over the arcs of a node.

        It goes through the arcs that the node has when the iterator is
        built, in the order of its array of arcs, with the operations of an
        Array_Iterator. It keeps only the array, the number of arcs, the
        position and the node, so that a search that keeps an iterator for
        each node of a path, as the depth-first ones do, copies little.
    */
    class Node_Arc_Iterator
    {
      void **arcs = nullptr;
      long num_arcs = 0;
      long pos = 0; // the position of the current arc
      Node *src_node = nullptr;

    public:
      /// The data type returned by get_curr().
      using Item_Type = Arc *;

      /// The set type over which iteration is performed.
      using Set_Type = Node *;

      /// Instantiate an empty (invalid) iterator.
      Node_Arc_Iterator() noexcept
      { /* empty */
      }

      /// Instantiate an iterator over node src.
      Node_Arc_Iterator(Node *src) noexcept
        : arcs(src->arc_array), num_arcs(static_cast<long>(src->num_arcs)),
          src_node(src)
      {
        // empty
      }

      /// Return the array of the arcs of the node.
      void ** get_base() noexcept { return arcs; }

      /// Return the array of the arcs of the node.
      void * const * get_base() const noexcept { return arcs; }

      /// Return true if the iterator is on an arc.
      bool has_curr() const noexcept { return pos >= 0 and pos < num_arcs; }

      /// Return true if the iterator is on the last arc.
      bool is_last() const noexcept
      {
        return has_curr() and pos == num_arcs - 1;
      }

      /// Return the position of the current arc, counted from 0.
      long get_pos() const noexcept { return pos; }

      /** Return the current arc.

          @throw underflow_error if the iterator is before the first arc.
          @throw overflow_error if the iterator is past the last arc.
      */
      Arc * get_curr() const
      {
        ah_underflow_error_if(pos < 0)
          << "Array_Iterator::get_curr(): no current item";
        ah_overflow_error_if(pos >= num_arcs)
          << "Array_Iterator::get_curr(): no current item";
        return get_curr_ne();
      }

      /// Return the current arc, which must exist.
      Arc * get_curr_ne() const noexcept
      {
        return static_cast<Arc *>(arcs[pos]);
      }

      /// Move to the next arc; the iterator must be on an arc.
      void next_ne() noexcept { ++pos; }

      /** Move to the next arc.

          @throw overflow_error if the iterator is not on an arc, or is
          past the last one.
      */
      void next()
      {
        ah_overflow_error_if(num_arcs == 0 or pos >= num_arcs)
          << "Array_Iterator::next(): no current item";
        next_ne();
      }

      /// Move to the previous arc; the iterator must be on an arc.
      void prev_ne() noexcept { --pos; }

      /** Move to the previous arc.

          @throw underflow_error if the iterator is not on an arc, or is
          before the first one.
      */
      void prev()
      {
        ah_underflow_error_if(num_arcs == 0 or pos < 0)
          << "Array_Iterator::prev(): no current item";
        prev_ne();
      }

      /// Put the iterator on the first arc.
      void reset() noexcept { pos = 0; }

      /// Put the iterator on the first arc.
      void reset_first() noexcept { reset(); }

      /// Put the iterator on the last arc.
      void reset_last() noexcept { pos = num_arcs - 1; }

      /// Put the iterator on position new_pos; the number of arcs is the
      /// position past the last one.
      void set_pos(const long new_pos) noexcept { pos = new_pos; }

      /// Put the iterator past the last arc.
      void end() noexcept { pos = num_arcs; }

      /// Return the current arc.
      Arc * get_current_arc_ne() const noexcept { return get_curr_ne(); }

      /// Return the current arc.
      Arc * get_current_arc() const { return get_curr(); }

      /// Return the target node of the current arc.
      Node * get_tgt_node_ne() const
      {
        Arc *a = get_curr_ne();
        return static_cast<Node *>(a->get_connected_node(src_node));
      }

      /// Return the target node of the current arc; it throws as get_curr().
      Node * get_tgt_node() const
      {
        Arc *a = get_curr();
        return static_cast<Node *>(a->get_connected_node(src_node));
      }
    };

    /** Inserts the node p, made with new, which the graph takes.

        @param[in] p node without arcs that belongs to no graph.
        @return p.
        @throw bad_alloc if the set of nodes cannot take p, which is then
        deleted, as List_SGraph::insert_node() and Net_Graph::insert_node()
        do: the methods that allocate the node and call this one do not
        free it.
    */
    virtual Node * insert_node(Node *p)
    {
      assert(p != nullptr);
      assert(p->num_arcs == 0);

      try
        {
          node_set.append(p);
        }
      catch (...)
        {
          delete p;
          throw;
        }
      ++this->num_nodes;

      return p;
    }

    void compress()
    {
      for (Node_Iterator it(*this); it.has_curr(); it.next_ne())
        it.get_curr()->compress();
    }

  private:
    Arc * try_insert_arc(Node *src, Node *tgt, void *a)
    {
      Arc *aptr = static_cast<Arc *>(a);

      aptr->src_node = src;
      aptr->tgt_node = tgt;
      src->insert_arc(aptr);

      if (not this->digraph and src != tgt)
        {
          try
            {
              tgt->insert_arc(aptr);
            }
          catch (const std::bad_alloc &)
            {
              src->remove_arc(aptr);
              throw;
            }
        }

      try
        {
          arc_set.append(aptr);
          ++this->num_arcs;
        }
      catch (const std::bad_alloc &)
        {
          src->remove_arc(aptr);
          if (not this->digraph and src != tgt)
            tgt->remove_arc(aptr);
          throw;
        }

      return aptr;
    }

  public:
    Arc * connect_arc(Arc *arc)
    {
      return
          try_insert_arc(this->get_src_node(arc), this->get_tgt_node(arc), arc);
    }

  private:
    Arc * insert_arc(Node *src, Node *tgt, void *a)
    {
      bool compress_done = false;

    retry:
      try
        {
          return try_insert_arc(src, tgt, a);
        }
      catch (const std::bad_alloc &)
        {
          if (compress_done)
            throw;

          compress();
          compress_done = true;
          goto retry;
        }
    }

  public:
    Arc * disconnect_arc(Arc *arc)
    {
      Node *src = static_cast<Node *>(arc->src_node);
      Node *tgt = static_cast<Node *>(arc->tgt_node);

      src->remove_arc_ne(arc);
      if (not this->digraph and src != tgt)
        tgt->remove_arc_ne(arc);

      arc->del(); // delete it from arc_set
      --this->num_arcs;

      return arc;
    }

    virtual void remove_arc(Arc *a)
    {
      delete disconnect_arc(a);
    }

    virtual void remove_node(Node *p)
    {
      if (this->digraph)
        // traverse all arcs of graph and remove those whose source or target is p
        this->remove_arcs_if([p, this](auto arc) {
          return this->get_src_node(arc) == p or this->get_tgt_node(arc) == p;
        });
      else
        {
          // traverse arcs of node (collect then delete for safe iteration)
          DynList<Arc *> arcs;
          for (size_t i = 0, n = p->num_arcs; i < n; ++i)
            arcs.append(static_cast<Arc *>(p->arc_array[i]));
          arcs.for_each([this](auto arc) { this->remove_arc(arc); });
        }

      p->del(); // remove from node_set
      --this->num_nodes;

      delete p;
    }

    /// @throw range_error if the graph has no nodes
    Node * get_first_node() const
    {
      ah_range_error_if(this->num_nodes == 0) << "Graph has not nodes";
      return static_cast<Node *>(node_set.get_first());
    }

    /// @throw range_error if the graph has no arcs
    Arc * get_first_arc() const
    {
      ah_range_error_if(this->num_arcs == 0) << "Graph has not arcs";
      return static_cast<Arc *>(arc_set.get_first());
    }

    Arc * get_first_arc(Node *p) const
    {
      ah_range_error_if(this->get_num_arcs(p) == 0) << "Node has no arcs";
      return static_cast<Arc *>(p->arc_array[0]);
    }

    Array_Graph()
    {
      assert(this->num_nodes == 0 and this->num_arcs == 0 and
             node_set.is_empty() and arc_set.is_empty());
    }

  private:
    void clear() noexcept
    {
      while (not arc_set.is_empty())
        delete static_cast<Arc *>(arc_set.remove_first());

      while (not node_set.is_empty())
        delete static_cast<Node *>(node_set.remove_first());

      this->num_nodes = this->num_arcs = 0;
    }

  public:
    virtual ~Array_Graph() noexcept
    {
      clear();
    }

    void swap(Array_Graph & g) noexcept
    {
      this->common_swap(g);
      node_set.swap(g.node_set);
      arc_set.swap(g.arc_set);
    }

    /// @see ALEPH_GRAPH_COPY_MOVE_CTORS
    ALEPH_GRAPH_COPY_MOVE_CTORS(Array_Graph)

  private:
    template <class Cmp>
    struct Cmp_Arc
    {
      Cmp & cmp;

      Cmp_Arc(Cmp && __cmp = Cmp())
        : cmp(__cmp)
      { /* empty */
      }

      Cmp_Arc(Cmp & __cmp)
        : cmp(__cmp)
      { /* empty */
      }

      bool operator ()(Arc *a1, Arc *a2) const noexcept
      {
        return cmp(a1, a2);
      }
    };

    // sort_nodes() and sort_arcs() are inherited from GraphCommon via CRTP
    // They use the get_node_dlink() and get_arc_dlink() accessors defined above
  };

  /** Directed graph (digraph) implemented with array-based adjacency lists.

      This class models a directed graph. Functionally it is
      equivalent to the Array_Graph class, except that this one
      handles directed graphs.

      This is a type alias for `Digraph<Array_Graph<...>>`, which provides
      all the directed graph functionality through the generic Digraph
      template wrapper defined in graph-dry.H.

      @tparam __Graph_Node The node type. Must be defined from
      the Graph_Anode class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @tparam __Graph_Arc The arc type. Must be defined from
      the Graph_Aarc class, either by attribute inclusion,
      by derivation, or by a combination of both.
      @see Graph_Anode Graph_Aarc Digraph
      @ingroup Graphs
      @see Array_Graph
   */
  template <typename __Graph_Node = Graph_Anode<int>,
            typename __Graph_Arc = Graph_Aarc<int>>
  using Array_Digraph = Digraph<Array_Graph<__Graph_Node, __Graph_Arc>>;

} // namespace Aleph {

# endif //  TPL_AGRAPH_H
