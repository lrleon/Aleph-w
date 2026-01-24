
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


/** @file tpl_indexArc.H
 *  @brief Arc indexing for fast lookup by endpoint nodes.
 *
 *  @par Example: Fast arc search between nodes
 *  @code
 *  using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
 *  GT g;
 *  IndexArc<GT> arc_index(g);
 *  
 *  auto a = g.insert_node("A");
 *  auto b = g.insert_node("B");
 *  auto c = g.insert_node("C");
 *  
 *  // Insert and index arcs
 *  auto ab = arc_index.insert_in_graph(a, b, 10);
 *  auto bc = arc_index.insert_in_graph(b, c, 20);
 *  
 *  // O(log n) search for arc between two nodes
 *  auto found = arc_index.search(a, b);
 *  if (found)
 *    cout << "Arc weight: " << found->get_info() << endl;
 *  @endcode
 *
 *  @par Example: Road network queries
 *  @code
 *  // Quickly find road between two cities
 *  auto road = arc_index.search(city1, city2);
 *  if (road)
 *    cout << "Distance: " << road->get_info() << " km" << endl;
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_INDEXARC_H
# define TPL_INDEXARC_H

# include <cassert>
# include <functional>
# include <stdexcept>
# include <utility>

# include <tpl_dynSetTree.H>
# include <tpl_graph.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph
{


  /** Index for fast arc lookup by its endpoint nodes.

      `IndexArc` indexes the arcs of a graph by the pair of endpoint
      nodes the arc connects.

      - For directed graphs, the key is the ordered pair `(src, tgt)`.
      - For non-directed graphs, the key is the unordered pair
        `{src, tgt}` (so `(src, tgt)` and `(tgt, src)` refer to the same
        indexed arc).

      Main operations are:
      - `search(src, tgt)`: find an arc between two nodes.
      - `insert(arc)`: insert an already existing arc into the index.
      - `insert_in_graph(src, tgt, info)`: create an arc in the graph
        and index it.

      @tparam GT Graph type (typically `List_Graph` or `List_Digraph`).
      @tparam Tree Binary-search-tree type used internally by `DynSetTree`.
      @tparam SA Arc filter type used by iterators when building the index.

      @note `IndexArc` assumes a *simple graph* for correctness of the
      “one key -> one arc” mapping (no parallel arcs connecting the same
      endpoints). If the underlying graph contains parallel arcs, only one
      of them can be indexed.

      @see IndexNode Index_Graph
      @ingroup Graphs
   */
  template <
    class GT,
    template <class /* Key */, class /* Compare */> class Tree = Rand_Tree,
    class SA = Dft_Show_Arc<GT>
    >
  class IndexArc
  {
  private:

    typedef typename GT::Arc GT_Arc;
    typedef typename GT::Node GT_Node;
    typedef typename GT::Arc_Type GT_Arc_Type;
    typedef typename GT::Node_Type GT_Node_Type;

    struct Cmp_Arc
    {
      bool directed = false;

      static bool ptr_less(void * a, void * b) noexcept
      {
        return std::less<void*>{}(a, b);
      }

      Cmp_Arc() = default;

      explicit Cmp_Arc(bool __directed) noexcept : directed(__directed) {}

      static std::pair<void*, void*> endpoints(const GT_Arc * a, const bool directed) noexcept
      {
        if (directed)
          return { a->src_node, a->tgt_node };

        auto u = a->src_node;
        auto v = a->tgt_node;
        if (ptr_less(v, u))
          std::swap(u, v);
        return {u, v};
      }

      bool operator () (const GT_Arc * a1, const GT_Arc * a2) const
      {
        const auto e1 = endpoints(a1, directed);
        const auto e2 = endpoints(a2, directed);
        if (ptr_less(e1.first, e2.first))
          return true;
        if (ptr_less(e2.first, e1.first))
          return false;
        return ptr_less(e1.second, e2.second);
      }
    };

    GT & g;
    DynSetTree <GT_Arc *, Tree, Cmp_Arc> index;
    SA sa;

  public:

    /// Insert an arc pointer into the index.
    ///
    /// If an arc with the same endpoints already exists in the index,
    /// then no insertion is performed and the already indexed arc is
    /// returned.
    ///
    /// @param[in] e Arc pointer to index.
    /// @return The indexed arc pointer (either `e` or the already indexed one).
    GT_Arc * insert(GT_Arc * e)
    {
      auto p = index.contains_or_insert(e);
      return *p.first;
    }

    /** Search an arc that connects two nodes.

        For directed graphs, only the arc `(src, tgt)` is searched.
        For non-directed graphs, `(src, tgt)` and `(tgt, src)` refer to
        the same indexed arc.

        @param[in] src Source node pointer.
        @param[in] tgt Target node pointer.
        @return A pointer to the indexed arc if it exists; `nullptr` otherwise.
    */
    GT_Arc * search(void * src, void * tgt) const
    {
      GT_Arc arc;
      arc.src_node = src;
      arc.tgt_node = tgt;

      GT_Arc ** ret_val = index.search(&arc);

      return ret_val != nullptr ? *ret_val : nullptr;
    }

    /** Search an arc that connects two nodes as a directed pair.

        In directed graphs this is equivalent to `search(src, tgt)`.
        In non-directed graphs, direction is not meaningful and this
        method behaves like `search(src, tgt)`.

        @param[in] src Source node pointer.
        @param[in] tgt Target node pointer.
        @return A pointer to the indexed arc if it exists; `nullptr` otherwise.
    */
    GT_Arc * search_directed(void * src, void * tgt) const
    {
      GT_Arc arc;
      arc.src_node = src;
      arc.tgt_node = tgt;

      GT_Arc ** ret_val = index.search(&arc);

      return ret_val != nullptr ? *ret_val : nullptr;
    }

    /** Search an arc by using its endpoint pointers.

        @param[in] a Arc whose endpoints are used as search key.
        @return A pointer to the indexed arc if it exists; `nullptr` otherwise.
    */
    GT_Arc * search(GT_Arc * a) const
    {
      return search(a->src_node, a->tgt_node);
    }

    /** Create a new arc between two nodes, insert it into the graph and index it.

        This method enforces the *simple-graph* constraint: if an arc
        between these endpoints already exists (according to the index),
        a `std::domain_error` is thrown.

        @param[in] src Source node.
        @param[in] tgt Target node.
        @param[in] info Arc payload to copy/move into the new arc.
        @return Pointer to the created arc.
        @throw std::bad_alloc if there is not enough memory.
        @throw std::domain_error if there is already an arc between these nodes.
    */
    GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                             const GT_Arc_Type & info)
    {
      GT_Arc * a = search(src, tgt);

      ah_domain_error_if(a != nullptr)
        << "There is already an arc between these nodes";

      a = g.insert_arc(src, tgt, info);
      insert(a);

      return a;
    }

    /// \overload insert_in_graph(GT_Node * src, GT_Node * tgt, const GT_Arc_Type & info)
    GT_Arc * insert_in_graph(GT_Node * src, GT_Node * tgt,
                             GT_Arc_Type && info = GT_Arc_Type())
    {
      GT_Arc * a = search(src, tgt);

      ah_domain_error_if(a != nullptr)
        << "There is already an arc between these nodes";

      a = g.insert_arc(src, tgt, std::forward<GT_Arc_Type>(info));
      insert(a);

      return a;
    }

    /// Remove an arc from the index.
    void remove(GT_Arc * e)
    {
      index.remove(e);
    }

    /// Remove an arc from both index and graph.
    void remove_from_graph(GT_Arc * a)
    {
      remove(a);
      g.remove_arc(a);
    }

    /// Remove all arcs from the index.
    void clear_index()
    {
      index.empty();
    }

    /// Insert into the index all arcs currently present in the graph.
    void build_index()
    {
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
          if (GT_Arc * a = it.get_curr(); search(a) == nullptr)
            insert(a);
    }

  private:

    void init()
    {
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        insert(it.get_curr());
    }

  public:

    /** Construct an arc index for a graph.

        @param[in] __g Graph to be indexed.
        @param[in] with_init If `true`, all current arcs of `__g` are inserted
        into the index at construction time.
        @param[in] __sa Arc filter used when iterating arcs.
        @throw std::bad_alloc if there is not enough memory.
    */
    IndexArc(GT & __g, bool with_init = true, SA && __sa = SA())
      : g(__g), index(Cmp_Arc(__g.is_digraph())), sa(std::move(__sa))
    {
      if (with_init)
        init();
    }

    /** Construct an arc index for a graph (lvalue filter overload).

        @param[in] __g Graph to be indexed.
        @param[in] with_init If `true`, all current arcs of `__g` are inserted
        into the index at construction time.
        @param[in] __sa Arc filter used when iterating arcs.
        @throw std::bad_alloc if there is not enough memory.
    */
    IndexArc(GT & __g, bool with_init, SA & __sa)
      : g(__g), index(Cmp_Arc(__g.is_digraph())), sa(__sa)
    {
      if (with_init)
        init();
    }

    /// Return the number of arcs currently stored in the index.
    size_t size() const { return index.size(); }
  };

}

# endif

