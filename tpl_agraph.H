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

/** @file tpl_agraph.H
 *  @brief Array-based graph implementation.
 *
 *  This file provides `Array_Graph` and `Array_Digraph`, graph implementations
 *  that use arrays for node storage. These offer better cache locality for
 *  dense graphs and efficient indexed access to nodes.
 *
 *  @ingroup Graphs
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

      @ingroup Grafos
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

    Graph_Anode(Node_Info && info) : Base(move(info))
    {
      init(Default_Cap);
    }

    Graph_Anode(const Graph_Anode & node) : Base(node.node_info)
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

    void allocate_more(size_t new_size)
    {
      if (new_size == 0)
        new_size = 1;

      const auto new_array = static_cast<void **>(realloc(arc_array, new_size * sizeof(void *)));
      ah_bad_alloc_if(new_array == nullptr);

      arc_array = new_array;
      arcs_dim = new_size;
      contract_threshold = arcs_dim / Contract_Factor;
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

    void remove_arc_ne(void *arc) noexcept
    {
      for (size_t i = 0; i < this->num_arcs; ++i)
        if (arc_array[i] == arc)
          {
            arc_array[i] = arc_array[--(this->num_arcs)];
            break;
          }

      if (this->num_arcs > contract_threshold)
        return;

      // contraction
      const size_t new_sz = arcs_dim >> 1; // num_arcs/2
      arc_array = static_cast<void **>(realloc(arc_array, new_sz * sizeof(void *)));
      arcs_dim = new_sz;
      contract_threshold = arcs_dim / Contract_Factor;
    }

    void remove_arc(void *arc)
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

      if (this->num_arcs > contract_threshold)
        return;

      // contraction
      const size_t new_sz = arcs_dim >> 1; // num_arcs/2
      arc_array = static_cast<void **>(realloc(arc_array, new_sz * sizeof(void *)));
      arcs_dim = new_sz;
      contract_threshold = arcs_dim / Contract_Factor;
    }

    bool compress() noexcept
    {
      const auto new_array = static_cast<void **>(realloc(arc_array,
                                                          this->num_arcs * sizeof(void *)));
      if (new_array == nullptr)
        return false;

      arc_array = new_array;
      arcs_dim = this->num_arcs;
      contract_threshold = this->num_arcs / Contract_Factor;

      return true;
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
      : Base(move(info))
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
      : Base(src, tgt, move(data))
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

    class Node_Arc_Iterator : public Array_Iterator<void *>
    {
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
        : Array_Iterator<void *>(no_exception_ctor,
                                 src->arc_array, src->arcs_dim, src->num_arcs),
          src_node(src)
      {
        // empty
      }

      Arc * get_curr() const
      {
        return static_cast<Arc *>(const_cast<Node_Arc_Iterator *>(this)->
          Array_Iterator<void *>::get_curr());
      }

      Arc * get_curr_ne() const noexcept
      {
        return static_cast<Arc *>(const_cast<Node_Arc_Iterator *>(this)->
          Array_Iterator<void *>::get_curr_ne());
      }

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

      Node * get_tgt_node() const
      {
        Arc *a = get_curr();
        return static_cast<Node *>(a->get_connected_node(src_node));
      }
    };

    virtual Node * insert_node(Node *p)
    {
      assert(p != nullptr);
      assert(p->num_arcs == 0);

      node_set.append(p);
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
          catch (std::bad_alloc)
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
      catch (std::bad_alloc)
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
      catch (bad_alloc)
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
      DynList<Arc *> arcs; // store arcs to delete
      if (this->digraph)
        // traverse all arcs of graph and store those whose source or target is p
        for (Arc_Iterator it(*this); it.has_curr(); it.next_ne())
          {
            if (Arc *arc = it.get_curr(); this->get_src_node(arc) == p or this->get_tgt_node(arc) == p)
              arcs.append(arc);
          }
      else
        // traverse arc of node
        for (size_t i = 0, n = p->num_arcs; i < n; ++i)
          {
            Arc *arc = static_cast<Arc *>(p->arc_array[i]);
            arcs.append(arc);
          }

      arcs.for_each([this](auto arc) { this->remove_arc(arc); });

      p->del(); // remove from node_set
      --this->num_nodes;

      delete p;
    }

    Node * get_first_node() const
    {
      return static_cast<Node *>(node_set.get_first());
    }

    Arc * get_first_arc() const
    {
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
      @ingroup Grafos
      @see Array_Graph
   */
  template <typename __Graph_Node = Graph_Anode<int>,
            typename __Graph_Arc = Graph_Aarc<int>>
  using Array_Digraph = Digraph<Array_Graph<__Graph_Node, __Graph_Arc>>;

} // namespace Aleph {

# endif //  TPL_AGRAPH_H
