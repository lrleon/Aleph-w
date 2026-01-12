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

/** @file tpl_cut_nodes.H
 *  @brief Articulation points (cut nodes) and bridges.
 *
 *  Finds cut vertices and bridge edges whose removal disconnects
 *  the graph. Uses DFS with low-link values.
 *
 *  ## Features
 *  - Find all cut vertices
 *  - Find all bridges
 *  - Compute biconnected components
 *
 *  ## Complexity: O(V + E)
 *
 *  @see tpl_components.H Connected components
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_CUT_NODES_H
# define TPL_CUT_NODES_H

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
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Compute_Cut_Nodes
  {
    SA sa;
    GT *gptr = nullptr;
    DynDlist<typename GT::Node *> *list_ptr = nullptr;
    long curr_df = 0;
    long curr_color = 1;

    enum State { Init, Cut_Nodes_Computed, Painted } state;

    void cut_nodes(typename GT::Node *p, typename GT::Arc *a)
    {
      NODE_BITS(p).set_bit(Depth_First, true); // mark p as visited
      low<GT>(p) = df<GT>(p) = curr_df++; // assign df number

      // traverse arcs of p
      bool p_is_cut_node = false;
      for (Node_Arc_Iterator<GT, SA> i(p, sa); i.has_curr(); i.next_ne())
        {
          auto arc = i.get_curr();
          if (arc == a)
            continue; // a is the parent arc ==> ignore it

          auto tgt = i.get_tgt_node();
          if (IS_NODE_VISITED(tgt, Depth_First))
            {
              if (not IS_ARC_VISITED(arc, Depth_First)) // non-tree arc?
                low<GT>(p) = std::min(df<GT>(tgt), low<GT>(p));
              continue;
            }

          if (IS_ARC_VISITED(arc, Depth_First))
            continue;

          ARC_BITS(arc).set_bit(Depth_First, true); // mark arc

          cut_nodes(tgt, arc);
          low<GT>(p) = std::min(low<GT>(tgt), low<GT>(p));
          if (low<GT>(tgt) >= df<GT>(p) and df<GT>(tgt) != 0) // cut node?
            p_is_cut_node = true;
        }

      // at this point, p has already been explored recursively
      if (p_is_cut_node)
        {
          NODE_BITS(p).set_bit(Cut, true);
          list_ptr->append(p);
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
                              low<GT>(p) = -1;
                            });
      gptr->reset_arcs();

      NODE_BITS(start).set_bit(Depth_First, true); // mark start
      df<GT>(start) = curr_df++;

      int call_counter = 0; // recursion call counter

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
          cut_nodes(tgt, arc);
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
    void paint_subgraph(typename GT::Node *p)
    {
      assert(not is_a_cut_node <GT> (p));

      if (is_node_painted<GT>(p))
        return;

      paint_node<GT>(p, curr_color);

      for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          if (is_arc_painted<GT>(arc))
            continue;

          auto tgt = it.get_tgt_node();
          if (is_a_cut_node<GT>(tgt))
            continue;

          paint_arc<GT>(arc, curr_color);
          paint_subgraph(tgt);
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
          else
            {
              paint_arc<GT>(arc, Cross_Arc); // mark as cross arc
              if (is_node_painted<GT>(tgt_node))
                continue;
            }

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
      assert(get_color<GT>(gp) == color);
      assert(not IS_NODE_VISITED(gp, Build_Subtree));

      std::unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
      sg.insert_node(tp_auto.get());
      GT::map_nodes(gp, tp_auto.get());
      NODE_BITS(gp).set_bit(Build_Subtree, true);

      return tp_auto.release();
    }

    void map_subgraph(GT & sg, typename GT::Node *gsrc, const long & color)
    {
      assert(get_color <GT> (gsrc) == color);

      auto tsrc = mapped_node<GT>(gsrc); // gsrc mapped into sg

      // Traverse arcs of gsrc and insert into sg those with the requested color
      for (Node_Arc_Iterator<GT, SA> i(gsrc, sa); i.has_curr(); i.next_ne())
        {
          auto garc = i.get_curr();
          if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
            continue; // arc has a different color or was already visited

          ARC_BITS(garc).set_bit(Build_Subtree, true);

          auto gtgt = i.get_tgt_node();

          assert(get_color <GT> (gtgt) == color);

          typename GT::Node *ttgt = nullptr; // gtgt mapped into sg
          if (IS_NODE_VISITED(gtgt, Build_Subtree)) // already in sg?
            ttgt = mapped_node<GT>(gtgt);
          else
            ttgt = create_and_map_node(gtgt, color, sg);

          auto tarc = sg.insert_arc(tsrc, ttgt, garc->get_info());
          GT::map_arcs(garc, tarc);

          map_subgraph(sg, gtgt, color);
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

    /// \overload cut_nodes()
    void operator ()(DynDlist<typename GT::Node *> & list)
    {
      cut_nodes(gptr->get_first_node(), list);
    }

    /// \overload cut_nodes()
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

  Use this routine if it is not necessary or sufficient to obtain
  the components using the colors of the blocks. This saves the
  computation and memory of having to create new graphs.

  @return the number of components, which is the number of colors.
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

      for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next_ne())
        if (get_color<GT>(it.get_curr()) == color)
          first = it.get_curr();

      if (first == nullptr) // did we find the color?
        ah_domain_error_if(first == nullptr) << "Color does not exist in the graph";

      // create first, insert it into sg, and map it
      create_and_map_node(first, color, sg);
      try
        {
          map_subgraph(sg, first, color); // map the component
        }
      catch (...)
        {
          clear_graph(sg);
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
      ah_logic_error_if(state < Cut_Nodes_Computed)
      << "Cut nodes have not been computed";

      if (state == Cut_Nodes_Computed)
        paint_subgraphs();

      const long & num_colors = curr_color;

      DynArray<GT *> blocks; // blocks in an array for fast access
      // Whether it is empty or not indicates if it has been processed.
      blocks.reserve(num_colors);

      // Create an ordered list of empty components by color i
      for (int i = 0; i < num_colors; ++i)
        blocks.access(i) = &block_list.append(GT());

      // Traverse nodes and copy/map according to color
      for (typename GT::Node_Iterator it(*gptr); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          if (IS_NODE_VISITED(p, Build_Subtree))
            continue;

          if (is_a_cut_node<GT>(p))
            continue;

          const long color = get_color<GT>(p);

          GT & sg = *blocks.access(color - 1);

          create_and_map_node(p, color, sg);

          map_subgraph(sg, p, color);
        }

      map_cut_graph(cut_graph, cross_arc_list);
    }
  };
} // end namespace Aleph

# endif // TPL_CUT_NODES_H
