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


/** @file tpl_cut_nodes.H
 *  @brief Articulation points (cut nodes), bridges, and biconnected components.
 *
 *  Provides two related algorithms for connectivity analysis of undirected
 *  graphs, both based on Tarjan's DFS low-link technique (O(V + E)):
 *
 *  ### Cut nodes (articulation points)
 *
 *  A **cut node** is a vertex whose removal disconnects the graph.
 *
 *  | Symbol | Meaning |
 *  |--------|---------|
 *  | `df[v]`  | DFS discovery time of v |
 *  | `low[v]` | minimum `df` reachable from v's subtree via ≤1 back-edge |
 *
 *  Condition (non-root): v is a cut node if ∃ child c with `low[c] >= df[v]`.
 *
 *  **API**
 *  - `Compute_Cut_Nodes<GT,SA>`: class — compute, then optionally paint
 *    biconnected blocks and extract mapped subgraphs.
 *  - `compute_cut_nodes(g)` / `compute_cut_nodes(g, start)`:
 *    free functions returning `DynList<Node*>`.
 *
 *  ### Bridges (cut edges / isthmuses)
 *
 *  A **bridge** is an edge whose removal disconnects the graph.
 *
 *  Condition: tree edge `(u → v)` is a bridge iff `low[v] > df[u]`
 *  (no back-edge from v's subtree escapes above u).
 *
 *  Parallel edges are handled correctly: only the exact parent arc (by
 *  pointer identity) is skipped; a second parallel arc acts as a back-edge.
 *
 *  **API**
 *  - `Compute_Bridges<GT,SA>`: class with `find_bridges(start)`,
 *    `find_bridges()`, `operator()(start)`, `operator()()`.
 *  - `find_bridges(g)` / `find_bridges(g, start)`:
 *    free functions returning `DynList<Arc*>`.
 *
 *  ### Biconnected components
 *
 *  After calling `Compute_Cut_Nodes::cut_nodes()`, call `paint_subgraphs()`
 *  to colour each biconnected block, then `map_subgraph()` or
 *  `compute_blocks()` to extract mapped copies.
 *
 *  @note Both algorithms require an **undirected**, **connected** graph.
 *        A `std::domain_error` is thrown for digraphs.
 *  @note Complexity: O(V + E) time and O(V) extra space.
 *
 *  @see tpl_components.H Connected components
 *  @see tpl_graph_utils.H Low-level graph utilities (df<GT>, low<GT>)
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_CUT_NODES_H
# define TPL_CUT_NODES_H

# include <ah-graph-concepts.H>

# include <algorithm>

# include <tpl_dfs_stack.H>
# include <tpl_graph_utils.H>

namespace Aleph
{
  /** Computation of cut nodes (articulation points) of a graph.

      The Compute_Cut_Nodes class computes everything related to the
      cut nodes of a connected graph. To do this, it performs a
      depth-first traversal starting from a given node and appends
      each cut node found in the graph to a dynamic list.

The dynamic list is provided by the user.

      A cut node is defined as a node such that when removed (together
      with its arcs), the graph becomes disconnected.

      The algorithm uses the Depth_First bit to mark nodes and arcs
      that have been visited. Additionally, the Build_Subtree bit is
      used to construct mapped copies of connected components around
      a cut node.

      This class also provides methods to "paint" the various blocks
      that would be separated by the cut nodes, as well as to obtain
      mapped copies of the connected components around the cut nodes,
      the cut graph, and the cross arcs.

      It is assumed that g is connected and no verification is
      performed in this regard.

The class template parameters are:
- GT: the graph type.
- SA: the arc filter for arc iterators.

\note If you need to compute the cut graph using the
map_cut_graph() method, then the arc filter class must
overload the operator () (GT&, GT::Arc*).

This class exports several operations with sequential
dependencies:
-# cut_nodes(): computes the cut nodes and inserts them into a
list. The operator () on the class is a synonym for this method.
-# paint_subgraphs(): "paints" the different components of the
graph around the previously computed cut nodes. The routine
returns the number of colors, one per component, that the graph
has. The colors are placed in the counters associated with each
node.
-# map_subgraph(): obtains a mapped copy of a connected component
associated with a given color.
-# map_cut_graph(): obtains a mapped copy of the "cut graph";
that is, the one composed only of the cut nodes of the graph.
This method also computes the cross arcs (those connecting a cut
node with a connected block).

If you want to execute everything at once, the compute_blocks()
method is provided, which computes the cut nodes, paints the
components, and calculates the mapped copies of the connected
components as well as the cross arcs.

      @ingroup Graphs
   */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Compute_Cut_Nodes
  {
    SA sa;
    GT *gptr = nullptr;
    DynDlist<typename GT::Node *> *list_ptr = nullptr;
    long curr_df = 0;
    long curr_color = 1;

    enum State { Init, Cut_Nodes_Computed, Painted } state;

    // Whether a copy made since the cut nodes were computed marked nodes
    // and arcs with Build_Subtree
    bool copies_marked = false;

    // A node that waits for the one that it visits, the arc through which
    // it was visited, the next arc of it to look at, its low value, and
    // whether it is a cut node of what was seen. The low value is needed
    // only while the node is in the stack, so it is kept here instead of
    // in the cookie of the node.
    struct Cut_Frame
    {
      typename GT::Node *p;
      typename GT::Arc *a;
      Node_Arc_Iterator<GT, SA> it;
      long low;
      bool p_is_cut_node = false;

      [[gnu::always_inline, gnu::flatten]]
      Cut_Frame(typename GT::Node *n, typename GT::Arc *arc, SA &sa,
                const long n_low)
        : p(n), a(arc), it(n, sa), low(n_low) {}

      [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }
    };

    // Visits p, reached through a, and pushes its frame
    [[gnu::always_inline]]
    Cut_Frame *visit(Dfs_Stack<Cut_Frame> &frames,
                     typename GT::Node *p, typename GT::Arc *a)
    {
      NODE_BITS(p).set_bit(Depth_First, true); // mark p as visited
      const long p_df = df<GT>(p) = curr_df++; // assign df number
      return &frames.push(p, a, sa, p_df); // its low value starts as it
    }

    // Moves the iterator of f to the next arc of f.p that goes to a node
    // not visited, which it marks, and gives that node, or nullptr when the
    // arcs of f.p are done. The arcs to nodes visited lower f.low, which is
    // kept in a local and stored once.
    [[gnu::always_inline, gnu::flatten]]
    typename GT::Node *next_child(Cut_Frame &f) noexcept
    {
      long p_low = f.low;
      typename GT::Node *child = nullptr;
      for (; f.it.has_curr(); f.it.next_ne())
        {
          auto arc = f.it.get_curr_ne();
          if (arc == f.a)
            continue; // a is the parent arc ==> ignore it

          auto tgt = f.it.get_tgt_node_ne();
          if (IS_NODE_VISITED(tgt, Depth_First))
            {
              if (not IS_ARC_VISITED(arc, Depth_First)) // non-tree arc?
                p_low = std::min(df<GT>(tgt), p_low);
              continue;
            }

          if (IS_ARC_VISITED(arc, Depth_First))
            continue;

          ARC_BITS(arc).set_bit(Depth_First, true); // mark arc
          child = tgt;
          break;
        }
      f.low = p_low;
      return child;
    }

    // Depth-first search from p, reached through a, that appends to the
    // list the cut nodes below the root. A path can have as many nodes as
    // the graph, too many for recursion, so that the search keeps its own
    // stack; the order of the visits is that of a recursion.
    void cut_nodes(Dfs_Stack<Cut_Frame> &frames,
                   typename GT::Node *p, typename GT::Arc *a)
    {
      Cut_Frame *f = visit(frames, p, a); // the top one
      while (true)
        {
          if (auto tgt = next_child(*f))
            { // the iterator of f stays on the arc to tgt, as in a recursion
              f = visit(frames, tgt, f->it.get_curr_ne());
              continue;
            }

          // at this point, f->p has already been explored
          auto child = f->p;
          const long child_low = f->low;
          if (f->p_is_cut_node)
            {
              NODE_BITS(child).set_bit(Cut, true);
              list_ptr->append(child);
            }
          frames.pop();
          if (frames.is_empty())
            return;

          f = &frames.top(); // the node from which child was visited
          f->low = std::min(child_low, f->low);
          if (child_low >= df<GT>(f->p) and df<GT>(child) != 0) // cut node?
            f->p_is_cut_node = true;
          f->next_arc(); // past the arc to child
        }
    }

  public:
    /** Computes the cut nodes.

  @param[in] start starting node from which to begin the computation.
        @param[out] list dynamic list where pointers to cut nodes of the
        graph are stored.
        @throw bad_alloc if there is no memory to insert into the dynamic
        list or if there is no memory for the internal computation data of
        the algorithm.

  The list must be provided by the user and is cleared at the beginning
  of the computation.
   */
    void cut_nodes(typename GT::Node *start,
                   DynDlist<typename GT::Node *> & list)
    {
      curr_df = 0; // global visit counter
      list_ptr = &list;

      list_ptr->empty();

      gptr->for_each_node([](auto p) // initialize nodes
                            {
                              NODE_COUNTER(p) = 0;
                              NODE_BITS(p).reset();
                              NODE_COOKIE(p) = nullptr;
                            });
      gptr->reset_arcs();
      copies_marked = false; // the bits were reset

      NODE_BITS(start).set_bit(Depth_First, true); // mark start
      df<GT>(start) = curr_df++;

      int call_counter = 0; // number of children of start
      Dfs_Stack<Cut_Frame> frames;

      // Traverse arcs from start while the graph is not fully spanned
      for (Node_Arc_Iterator<GT, SA> it(start, sa);
           it.has_curr() and curr_df < gptr->get_num_nodes(); it.next_ne())
        {
          auto tgt = it.get_tgt_node();
          if (IS_NODE_VISITED(tgt, Depth_First))
            continue;

          auto arc = it.get_curr();
          if (IS_ARC_VISITED(arc, Depth_First))
            continue;

          ARC_BITS(arc).set_bit(Depth_First, true);
          cut_nodes(frames, tgt, arc);
          ++call_counter;
        }

      if (call_counter > 1) // is the root an articulation point?
        { // yes ==> append it to the list
          NODE_BITS(start).set_bit(Cut, true);
          list_ptr->append(start);
        }

      state = Cut_Nodes_Computed;
    }

  private:
    // The next arc of a node to look at, in the searches that only need it
    struct Arc_Frame
    {
      Node_Arc_Iterator<GT, SA> it;

      [[gnu::always_inline, gnu::flatten]]
      Arc_Frame(typename GT::Node *p, SA &sa) : it(p, sa) {}

      [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }
    };

    // Moves the iterator of f to the next arc that goes to a node that is
    // not a cut node and was not painted, and paints both; gives that
    // node, or nullptr when the arcs are done
    [[gnu::always_inline, gnu::flatten]]
    typename GT::Node *next_to_paint(Arc_Frame &f) noexcept
    {
      for (; f.it.has_curr(); f.it.next_ne())
        {
          auto arc = f.it.get_curr_ne();
          if (is_arc_painted<GT>(arc))
            continue;

          auto tgt = f.it.get_tgt_node_ne();
          if (is_a_cut_node<GT>(tgt))
            continue;

          paint_arc<GT>(arc, curr_color);
          if (is_node_painted<GT>(tgt))
            continue;

          paint_node<GT>(tgt, curr_color);
          return tgt;
        }
      return nullptr;
    }

    // Paints with curr_color the block of p, which is not a cut node. A
    // path can have as many nodes as the graph, too many for recursion, so
    // that the search keeps its own stack; the order of the visits is that
    // of a recursion.
    void paint_subgraph(typename GT::Node *p)
    {
      assert(not is_a_cut_node <GT> (p));

      if (is_node_painted<GT>(p))
        return;

      paint_node<GT>(p, curr_color);

      Dfs_Stack<Arc_Frame> frames;
      Arc_Frame *f = &frames.push(p, sa); // the top one
      while (true)
        {
          if (auto tgt = next_to_paint(*f))
            { // the iterator of f stays on the arc to tgt, as in a recursion
              f = &frames.push(tgt, sa);
              continue;
            }

          frames.pop();
          if (frames.is_empty())
            return;

          f = &frames.top();
          f->next_arc();
        }
    }

    void paint_from_cut_node(typename GT::Node *p)
    {
      assert(is_a_cut_node <GT> (p));

      // Paint connected blocks adjacent to p with different colors
      for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();

          assert(not is_arc_painted <GT> (arc));

          auto tgt_node = it.get_tgt_node();
          if (is_a_cut_node<GT>(tgt_node)) // is it a cut arc?
            {
              ARC_BITS(arc).set_bit(Cut, true); // mark as cut arc
              continue; // move to next arc
            }
          paint_arc<GT>(arc, Cross_Arc); // mark as cross arc
          if (is_node_painted<GT>(tgt_node))
            continue;

          // paint the block reachable through this arc
          paint_subgraph(tgt_node);

          curr_color++; // next color (next arc belongs to a different block)

          assert(not is_arc_painted <GT> (arc));
        }
    }

    typename GT::Node * create_and_map_node(typename GT::Node *gp,
                                            const long & color,
                                            GT & sg)
    {
      (void) color;
      assert(get_color<GT>(gp) == color);
      assert(not IS_NODE_VISITED(gp, Build_Subtree));

      std::unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
      sg.insert_node(tp_auto.get());
      NODE_COOKIE(gp) = nullptr; // else it would compose with a former copy
      GT::map_nodes(gp, tp_auto.get());
      NODE_BITS(gp).set_bit(Build_Subtree, true);

      return tp_auto.release();
    }

    // A node of the block being copied that waits for the one that the
    // copy goes to, its image, and the next arc of it to look at
    struct Map_Frame
    {
      typename GT::Node *tsrc;
      Node_Arc_Iterator<GT, SA> it;

      [[gnu::always_inline, gnu::flatten]]
      Map_Frame(typename GT::Node *gsrc, SA &sa)
        : tsrc(mapped_node<GT>(gsrc)), it(gsrc, sa) {}

      [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }
    };

    // Moves the iterator of f to the next arc of the color that was not
    // copied, and gives it, or nullptr when the arcs are done
    [[gnu::always_inline, gnu::flatten]]
    typename GT::Arc *next_to_map(Map_Frame &f, const long & color) noexcept
    {
      for (; f.it.has_curr(); f.it.next_ne())
        {
          auto garc = f.it.get_curr_ne();
          if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
            continue; // arc has a different color or was already visited

          return garc;
        }
      return nullptr;
    }

    // Copies into sg, from gsrc, whose image is in sg, the arcs of the
    // given color not copied yet, with the nodes that they reach; the
    // search goes on from the target of each arc that it copies. A path
    // can have as many nodes as the graph, too many for recursion, so that
    // the search keeps its own stack; the order of the visits is that of a
    // recursion.
    void map_subgraph(GT & sg, typename GT::Node *gsrc, const long & color)
    {
      assert(get_color <GT> (gsrc) == color);

      // Copies garc, an arc of f.it, and gtgt, its target, if gtgt was not
      // copied; gives the frame of gtgt
      auto copy = [&](Map_Frame &f, typename GT::Arc *garc)
        {
          ARC_BITS(garc).set_bit(Build_Subtree, true);

          auto gtgt = f.it.get_tgt_node_ne();

          assert(get_color <GT> (gtgt) == color);

          typename GT::Node *ttgt = nullptr; // gtgt mapped into sg
          if (IS_NODE_VISITED(gtgt, Build_Subtree)) // already in sg?
            ttgt = mapped_node<GT>(gtgt);
          else
            ttgt = create_and_map_node(gtgt, color, sg);

          auto tarc = sg.insert_arc(f.tsrc, ttgt, garc->get_info());
          ARC_COOKIE(garc) = nullptr; // else it would compose with a former copy
          GT::map_arcs(garc, tarc);
          return Map_Frame(gtgt, sa);
        };

      // The node being visited keeps its iterator in here until the copy
      // goes on from a target of it; then it waits in frames, where it goes
      // on when the copy comes back
      Dfs_Stack<Map_Frame> frames;
      Map_Frame here(gsrc, sa);
      while (true)
        {
          if (auto garc = next_to_map(here, color))
            { // the iterator stays on garc, as in a recursion
              Map_Frame next = copy(here, garc);
              frames.push(std::move(here));
              here = std::move(next);
              continue;
            }

          while (true) // back to the nodes that wait
            {
              if (frames.is_empty())
                return;

              Map_Frame &f = frames.top();
              f.next_arc(); // past the arc that was copied
              if (auto garc = next_to_map(f, color))
                {
                  here = copy(f, garc);
                  break;
                }
              frames.pop();
            }
        }
    }

  public:
    /** Constructor for cut nodes calculator.

  @param[in] g the graph for which to compute the cut nodes.
  @param[in] __sa the arc filter for arc iterators.
     */
    Compute_Cut_Nodes(const GT & g, SA __sa = SA())
      : sa(__sa), gptr(&const_cast<GT &>(g)), state(Init)
    {
      /* empty */
    }

    /** \overload
     *
     *  Compute cut nodes starting from the first node of the graph.
     *
     *  @param[out] list Output list of cut nodes.
     */
    void operator ()(DynDlist<typename GT::Node *> & list)
    {
      cut_nodes(gptr->get_first_node(), list);
    }

    /** \overload
     *
     *  Compute cut nodes starting from a specific node.
     *
     *  @param[in] start Starting node for the computation.
     *  @param[out] list Output list of cut nodes.
     */
    void operator ()(typename GT::Node *start,
                     DynDlist<typename GT::Node *> & list)
    {
      cut_nodes(start, list);
    }

    /** Paints the connected components around the cut nodes.

    paint_subgraphs() paints the connected components around the cut
    nodes of a graph.

  The routine requires that the cut_nodes() method has been
  previously invoked.

  The "colors" of each component are placed in the NODE_COUNTER
  attribute of each node and arc. The first color starts at value 1.

  @note **Cut nodes keep color 0** (they don't belong to any block).
  @note **Cross arcs** (cut → non-cut) are stored with
        `ARC_COUNTER(arc) == Cross_Arc` (−1, which is NOT considered
        "painted" by `is_arc_painted`).
  @note **Cut arcs** (cut → cut) have the `Cut` bit set in `ARC_BITS`
        and counter = 0.

  Use this routine if it is not necessary or sufficient to obtain
  the components using the colors of the blocks. This saves the
  computation and memory of having to create new graphs.

  @return **The next unused color** (one past the highest color
          assigned).  If N biconnected blocks were found and painted
          with colors 1 … N, the return value is N + 1.
          Returns 1 without painting anything if there are no cut
          nodes (the graph is biconnected).
  @throw std::logic_error if the computation of cut nodes has not
  been previously invoked.
     */
    long paint_subgraphs()
    {
      ah_logic_error_if(state != Cut_Nodes_Computed)
        << "Cut nodes have not been computed or the class is in another phase";

      gptr->reset_counter_nodes();
      gptr->reset_counter_arcs();
      curr_color = 1;

      // Traverse each cut node and paint its adjacent blocks
      for (typename DynDlist<typename GT::Node *>::Iterator i(*list_ptr);
           i.has_curr(); i.next_ne())
        paint_from_cut_node(i.get_curr());

      state = Painted;

      return curr_color;
    }

    /** Obtains a mapped copy of the component with the given color.

  map_subgraph() extracts from the graph the component with the
  specified color and constructs a mapped graph in sg.

  \param[out] sg the graph where the mapped copy is to be obtained.
  \param[in] color the color to be extracted.
  \throw logic_error if the graph is not painted.
  \throw domain_error if there is no component with the given color.
    */
    void map_subgraph(GT & sg, const long & color)
    {
      ah_logic_error_if(state != Painted) << "Graph is not painted";

      clear_graph(sg);

      typename GT::Node *first = nullptr; // find the first node with the requested color

      // A former copy of the color marked its nodes and its arcs with
      // Build_Subtree, and some node of the color has the mark then
      bool copied_before = false;
      for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next_ne())
        if (get_color<GT>(it.get_curr()) == color)
          {
            first = it.get_curr();
            copied_before = copied_before or IS_NODE_VISITED(first, Build_Subtree);
          }

      if (first == nullptr) // did we find the color?
        {
          ah_domain_error_if(first == nullptr) << "Color does not exist in the graph";
        }

      if (copied_before)
        {
          gptr->reset_bit_nodes(Build_Subtree);
          gptr->reset_bit_arcs(Build_Subtree);
        }
      copies_marked = true;

      // create first, insert it into sg, and map it
      create_and_map_node(first, color, sg);
      try
        {
          map_subgraph(sg, first, color); // map the component
        }
      catch (...)
        {
          clear_graph(sg);
          throw;
        }
    }

    /** Computes the mapped cut graph of a graph.

 The cut graph of a graph is the one composed only of the cut nodes.

 The cross arcs are also computed; that is, those that connect cut
 nodes with connected components.

 The idea of this algorithm is that when used in combination with
 map_subgraph(), the original graph can be obtained from the mapped
 copies.

 @param[out] cut_graph the cut graph
 @param[out] cross_arc_list the list of cross arcs; that is, the
 arcs that go from a cut node to a connected component. These arcs
 do not belong to the cut graph.

 \throw logic_error if the graph has not been previously painted.
     */
    void map_cut_graph(GT & cut_graph,
                       DynDlist<typename GT::Arc *> & cross_arc_list)
    {
      ah_logic_error_if(state != Painted) << "Graph is not painted";

      clear_graph(cut_graph);

      // Traverse the cut-node list and insert them into cut_graph
      for (typename DynDlist<typename GT::Node *>::Iterator it(*list_ptr);
           it.has_curr(); it.next_ne())
        {
          auto gp = it.get_curr();

          assert(is_a_cut_node <GT> (gp));

          std::unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
          cut_graph.insert_node(tp_auto.get());
          NODE_COOKIE(gp) = nullptr; // else it would compose with a former copy
          GT::map_nodes(gp, tp_auto.release());
        }

      // Traverse arcs of g:
      // - cut_graph will contain cut arcs (between cut nodes)
      // - cross_arc_list will contain cross arcs (from cut nodes to blocks)
      for (Arc_Iterator<GT, SA> it(*gptr, sa); it.has_curr(); it.next_ne())
        {
          auto garc = it.get_curr();
          if (is_a_cross_arc<GT>(garc))
            {
              cross_arc_list.append(garc);
              continue;
            }

          if (not is_an_cut_arc<GT>(garc))
            continue;

          typename GT::Node *src = mapped_node<GT>(gptr->get_src_node(garc));
          typename GT::Node *tgt = mapped_node<GT>(gptr->get_tgt_node(garc));

          assert(src != nullptr and tgt != nullptr);

          typename GT::Arc *arc =
              cut_graph.insert_arc(src, tgt, garc->get_info());
          ARC_COOKIE(garc) = nullptr; // else it would compose with a former copy
          GT::map_arcs(garc, arc);
        }
    }

    /** Build mapped graphs around cut nodes.

        This routine takes a previously computed list of cut nodes and builds,
        in a single pass over nodes, mapped copies of the connected components
        around the cut nodes, as well as the cut graph. Then, it performs a
        pass over arcs to determine the cross arcs.

        If you need all mapped blocks and the cut graph, using this routine is
        more efficient than calling map_subgraph() and map_cut_graph() separately.

        The block of color c is the c-th of block_list. Without cut nodes,
        the graph is a single block, the only one of the list, and the cut
        graph is empty. The nodes that the computation of the cut nodes did
        not reach, which are in other components of the graph, are in no
        block.

        \param[out] block_list list of graphs where mapped copies of connected
        components will be stored.
        \param[out] cut_graph mapped cut graph.
        \param[out] cross_arc_list list of cross arcs.
        \throw bad_alloc if there is not enough memory.
        \throw logic_error if cut nodes have not been computed.
    */
    void compute_blocks(DynDlist<GT> & block_list,
                        GT & cut_graph,
                        DynDlist<typename GT::Arc *> & cross_arc_list)
    {
      ah_logic_error_if(state < Cut_Nodes_Computed) << "Cut nodes have not been computed";

      if (state == Cut_Nodes_Computed)
        paint_subgraphs();

      if (copies_marked) // by a former copy
        {
          gptr->reset_bit_nodes(Build_Subtree);
          gptr->reset_bit_arcs(Build_Subtree);
        }
      copies_marked = true;

      // curr_color is the next unused color, so the colors of the blocks
      // go from 1 to curr_color - 1, and the block of color c is the
      // (c - 1)-th. Without cut nodes nothing was painted: the graph is one
      // block, whose nodes and arcs have color 0.
      const long num_blocks = curr_color > 1 ? curr_color - 1 : 1;

      DynArray<GT *> blocks; // blocks in an array for fast access
      blocks.reserve(num_blocks);

      // Create an ordered list of empty components by color i
      for (long i = 0; i < num_blocks; ++i)
        blocks.access(i) = &block_list.append(GT());

      // Traverse nodes and copy/map according to color
      for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          if (IS_NODE_VISITED(p, Build_Subtree))
            continue;

          if (is_a_cut_node<GT>(p))
            continue;

          if (not IS_NODE_VISITED(p, Depth_First))
            continue; // in a component that cut_nodes() did not reach

          const long color = get_color<GT>(p);

          GT & sg = *blocks.access(color == 0 ? 0 : color - 1);

          create_and_map_node(p, color, sg);

          map_subgraph(sg, p, color);
        }

      map_cut_graph(cut_graph, cross_arc_list);
    }
  };

  // =========================================================================
  // Bridge finding (Tarjan low-link algorithm)
  // =========================================================================

  /** @brief Find bridge edges (isthmuses) of a connected undirected graph.
   *
   *  A **bridge** (also called *isthmus* or *cut edge*) is an arc whose
   *  removal disconnects the graph.  This class implements Tarjan's
   *  O(V + E) algorithm: a single DFS that tracks *discovery time* `df[v]`
   *  and *low-link* `low[v]` (earliest discovery time reachable from the
   *  subtree of `v` via back-edges).
   *
   *  **Bridge condition**: a DFS tree edge `(p → tgt)` is a bridge if and
   *  only if `low[tgt] > df[p]`, i.e., no back-edge in `tgt`'s subtree
   *  reaches an ancestor of `p`.
   *
   *  **Parallel edges** are handled correctly: only the exact parent arc
   *  (by pointer identity) is skipped; a second parallel edge acts as a
   *  back-edge and prevents either copy from being a bridge.
   *
   *  **Usage**:
   *  @code
   *  Compute_Bridges<Graph> cb(g);
   *  DynList<Graph::Arc *> bridges = cb.find_bridges();
   *  for (auto * a : bridges)
   *    std::cout << "bridge endpoint ids: "
   *              << g.get_src_node(a)->get_info() << " -- "
   *              << g.get_tgt_node(a)->get_info() << '\n';
   *  @endcode
   *
   *  @tparam GT  Graph type (must be undirected; see @ref tpl_graph.H).
   *  @tparam SA  Arc filter (default: `Dft_Show_Arc<GT>`).
   *
   *  @note Complexity: O(V + E) time and O(V) extra space.
   *  @note The graph must be undirected.  A domain_error is thrown for
   *        digraphs.
   *
   *  @ingroup Graphs
   *  @see Compute_Cut_Nodes, find_bridges(), is_an_cut_arc()
   */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Compute_Bridges
  {
    SA sa;
    GT *gptr;

    // A node that waits for the one that it visits, the arc through which
    // it was visited, and the next arc of it to look at
    struct Frame
    {
      typename GT::Node *p;
      typename GT::Arc *parent_arc;
      Node_Arc_Iterator<GT, SA> it;

      [[gnu::always_inline, gnu::flatten]]
      Frame(typename GT::Node *n, typename GT::Arc *a, SA &sa)
        : p(n), parent_arc(a), it(n, sa) {}

      [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }
    };

    // Moves the iterator of f to the next arc of f.p that goes to a node
    // not visited, which it marks, and gives that node, or nullptr when the
    // arcs of f.p are done. The back-edges lower low(f.p), which is stored
    // once: a store in the loop could be to the iterator, which would have
    // to be read again.
    [[gnu::always_inline, gnu::flatten]]
    typename GT::Node *next_child(Frame &f) noexcept
    {
      long p_low = low<GT>(f.p);
      typename GT::Node *child = nullptr;
      for (; f.it.has_curr(); f.it.next_ne())
        {
          auto arc = f.it.get_curr_ne();
          if (arc == f.parent_arc)
            continue; // skip the arc we came from

          auto tgt = f.it.get_tgt_node_ne();

          if (IS_NODE_VISITED(tgt, Depth_First))
            {
              // Back-edge: update low if this arc has not been traversed yet
              if (not IS_ARC_VISITED(arc, Depth_First))
                p_low = std::min(df<GT>(tgt), p_low);
              continue;
            }

          if (IS_ARC_VISITED(arc, Depth_First))
            continue;

          ARC_BITS(arc).set_bit(Depth_First, true); // mark tree arc
          child = tgt;
          break;
        }
      low<GT>(f.p) = p_low;
      return child;
    }

    // Depth-first search from p that appends to bridges those below p. A
    // path can have as many nodes as the graph, too many for recursion, so
    // that the search keeps its own stack; the order of the visits is that
    // of a recursion.
    void __dfs(Dfs_Stack<Frame> & frames, typename GT::Node *p,
               long & curr_df, DynList<typename GT::Arc *> & bridges)
    {
      auto visit = [&frames, &curr_df, this](typename GT::Node *n,
                                             typename GT::Arc *a)
        {
          NODE_BITS(n).set_bit(Depth_First, true);
          low<GT>(n) = df<GT>(n) = curr_df++;
          return &frames.push(n, a, sa);
        };

      Frame *f = visit(p, nullptr); // the top one
      while (true)
        {
          if (auto tgt = next_child(*f))
            { // the iterator of f stays on the arc to tgt, as in a recursion
              f = visit(tgt, f->it.get_curr_ne());
              continue;
            }

          auto child = f->p;
          auto arc = f->parent_arc;
          frames.pop();
          if (frames.is_empty())
            return;

          f = &frames.top(); // the node from which child was visited
          low<GT>(f->p) = std::min(low<GT>(child), low<GT>(f->p));

          if (low<GT>(child) > df<GT>(f->p)) // bridge condition
            bridges.append(arc);

          f->next_arc(); // past the arc to child
        }
    }

  public:
    /** @brief Constructor.
     *
     *  @param[in] g   Undirected connected graph.
     *  @param[in] sa  Arc filter.
     */
    Compute_Bridges(const GT & g, SA sa = SA())
      : sa(sa), gptr(&const_cast<GT &>(g))
    { /* empty */
    }

    /** @brief Find all bridge edges in the graph, starting from @p start.
     *
     *  Performs a DFS from @p start and then visits any nodes not
     *  reachable from @p start (disconnected components), returning
     *  every arc identified as a bridge.  The graph's `Depth_First`
     *  bits and `NODE_COUNTER` fields are used internally and reset
     *  before the search; `NODE_COOKIE` is used for low-link storage
     *  and cleared on return.
     *
     *  @param[in] start  Starting node for the first DFS.
     *  @return `DynList` of pointers to bridge arcs (order: DFS post-order).
     *  @throws std::domain_error     If the graph is a digraph.
     *  @throws std::invalid_argument If @p start is `nullptr` and the
     *                                graph is non-empty.
     */
    DynList<typename GT::Arc *> find_bridges(typename GT::Node *start)
    {
      ah_domain_error_if(gptr->is_digraph()) << "Compute_Bridges: does not work on digraphs";

      DynList<typename GT::Arc *> bridges;

      if (gptr->get_num_nodes() == 0)
        return bridges;

      ah_invalid_argument_if(start == nullptr) << "Compute_Bridges::find_bridges(): start must be non-null";

      gptr->for_each_node([](auto p)
                            {
                              NODE_COUNTER(p) = 0;
                              NODE_BITS(p).reset();
                            });
      gptr->reset_arcs();

      long curr_df = 0;
      Dfs_Stack<Frame> frames;
      __dfs(frames, start, curr_df, bridges);

      // Cover disconnected components: any node not reached from start.
      gptr->for_each_node([&](auto p)
                            {
                              if (not IS_NODE_VISITED(p, Depth_First))
                                __dfs(frames, p, curr_df, bridges);
                            });

      // NODE_COOKIE holds low-link values (stored as longs); clear them.
      gptr->for_each_node([](auto p) { NODE_COOKIE(p) = nullptr; });

      return bridges;
    }

    /** \overload
     *
     *  Starts the DFS from the first node returned by the graph.
     *  Returns an empty list immediately if the graph has no nodes.
     */
    DynList<typename GT::Arc *> find_bridges()
    {
      if (gptr->get_num_nodes() == 0)
        return DynList<typename GT::Arc *>{};
      return find_bridges(gptr->get_first_node());
    }

    /** \overload
     *
     *  Synonym for `find_bridges(start)`.
     */
    DynList<typename GT::Arc *> operator()(typename GT::Node *start)
    {
      return find_bridges(start);
    }

    /** \overload
     *
     *  Synonym for `find_bridges()` (from first node).
     */
    DynList<typename GT::Arc *> operator()()
    {
      return find_bridges();
    }
  };

  /** @brief Find all bridge edges in a connected undirected graph.
   *
   *  Convenience free function wrapping `Compute_Bridges<GT, SA>`.
   *  A **bridge** is an arc whose removal disconnects the graph.
   *
   *  @code
   *  auto bridges = find_bridges(g, start_node);
   *  for (auto * a : bridges)
   *    ...;
   *  @endcode
   *
   *  @tparam GT   Graph type.
   *  @tparam SA   Arc filter.
   *  @param[in] g      Undirected connected graph.
   *  @param[in] start  Starting node for the DFS.
   *  @param[in] sa     Arc filter.
   *  @return `DynList` of pointers to bridge arcs.
   *
   *  @ingroup Graphs
   *  @see Compute_Bridges
   */
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<typename GT::Arc *>
  find_bridges(const GT & g, typename GT::Node *start, SA sa = SA())
  {
    return Compute_Bridges<GT, SA>(g, sa).find_bridges(start);
  }

  /** \overload find_bridges(const GT &, typename GT::Node *, SA)
   *
   *  Starts from the first node of the graph.
   */
  template <AlephGraph GT>
  DynList<typename GT::Arc *> find_bridges(const GT & g)
  {
    return Compute_Bridges<GT>(g).find_bridges();
  }
} // end namespace Aleph

# endif // TPL_CUT_NODES_H
