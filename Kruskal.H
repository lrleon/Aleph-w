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

/** @file Kruskal.H
 *  @brief Kruskal's minimum spanning tree algorithm.
 *
 *  Implements Kruskal's algorithm for finding minimum spanning trees
 *  using union-find data structure. Sorts edges by weight and greedily
 *  adds edges that don't create cycles. O(E log E) complexity.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef KRUSKAL_H
# define KRUSKAL_H

# include <ahFunction.H>
# include <tpl_agraph.H>
# include <tpl_graph_utils.H>
# include <tpl_test_acyclique.H>
# include <tpl_union.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Computes the minimum spanning tree of a graph using
      Kruskal's algorithm.

      Kruskal's algorithm is recommended for sparse graphs. It works
      by sorting all arcs by weight and greedily adding arcs that
      don't form a cycle.

      The procedure is parameterized with the following specifications:
      -# GT: The graph type based on List_Graph.
      -# Distance<GT>: The arc weight accessor class that must export
         the following attributes:
        -# typedef Distance<GT>::Distance_Type: The data type that
           represents an arc weight.
        -# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
           Returns the weight value of arc a.
        -# Distance<GT>::Max_Distance: Static constant representing the
           maximum distance value that an algorithm would consider as
           infinity.
        -# typename Distance<GT>::Zero_Distance: Static constant
           corresponding to the neutral element of addition. Traditionally,
           in the vast majority of cases, this is zero.
        .
      -# SA: Arc filter for arc iterators.

      Time complexity: O(E log E) where E is the number of arcs.
      Space complexity: O(V) for the Union-Find structure.

      @see Prim_Min_Spanning_Tree
      @ingroup Graphs
  */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Kruskal_Min_Spanning_Tree
  {
    Distance dist_val;
    Distance & dist;
    SA sa;
    bool painted;

    /// @brief Helper struct for initializing node counters during DFS.
    struct Init_Node
    {
      long count;

      Init_Node() noexcept : count(0) { /* empty */ }

      void operator ()(const GT &, typename GT::Node *p) noexcept
      {
        NODE_COUNTER(p) = count++;
        NODE_BITS(p).set_bit(Aleph::Spanning_Tree, false);
      }
    };

    static bool arc_is_in_tree(Fixed_Relation & tree, long i, long j) noexcept
    {
      return tree.are_connected(i, j);
    }

  public:
    /** @brief Constructor.

        @param __dist Distance accessor for arc weights.
        @param __sa Arc filter for arc iterators.
    */
    Kruskal_Min_Spanning_Tree(Distance __dist = Distance(), SA __sa = SA())
      : dist_val(__dist), dist(dist_val), sa(__sa), painted(false)
    {
      /* empty */
    }

    /// @brief Constructor with reference to external distance accessor.
    Kruskal_Min_Spanning_Tree(Distance & __dist, SA __sa = SA())
      : dist_val(), dist(__dist), sa(__sa), painted(false)
    {
      /* empty */
    }

    /// @brief Returns true if the spanning tree has been painted on the graph.
    bool is_painted() const noexcept { return painted; }

    /// @brief Filter for arcs painted by Kruskal's algorithm.
    template <class G, class GT_SA>
    struct Paint_Filt
    {
      GT_SA & sa;

      Paint_Filt(GT_SA & __sa) : sa(__sa) { /* empty */ }

      bool operator ()(typename G::Arc *a) const noexcept
      {
        if (not sa(a))
          return false;

        return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
      }
    };

    /** @brief Paints the minimum spanning tree arcs on the graph.

        After calling this method, the arcs belonging to the MST are marked
        with the Spanning_Tree bit. Use is_painted() to check if the tree
        has been painted.

        @note The const_cast on g is safe because sort_arcs only changes
        the physical ordering of arcs, not the logical graph structure.

        @param[in] g The graph for which to compute the minimum spanning tree.
        @throw domain_error If g is a digraph.
    */
    void paint_min_spanning_tree(const GT & g)
    {
      ah_domain_error_if(g.is_digraph()) << "g is a digraph";

      g.reset_bit_arcs(Aleph::Spanning_Tree); // clear arc marking bits
      Operate_On_Nodes<GT, Init_Node>()(g, Init_Node());

      typedef Distance_Compare<GT, Distance> DCMP;
      DCMP comp(dist);
      // Safe const_cast: sort_arcs only changes physical arc order, not structure
      const_cast<GT &>(g).template sort_arcs<DCMP>(comp);
      const size_t V = g.get_num_nodes();

      Fixed_Relation tree(V);

      // Traverse sorted arcs of g until all nodes are in one component
      for (Arc_Iterator<GT, SA> it(g, sa); tree.get_num_blocks() > 1 and
                                           it.has_curr(); it.next_ne())
        { // next smallest arc
          auto arc = it.get_current_arc_ne();
          const long i = NODE_COUNTER(g.get_src_node(arc));
          const long j = NODE_COUNTER(g.get_tgt_node(arc));
          if (arc_is_in_tree(tree, i, j))
            continue;

          tree.join(i, j);
          ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
        }

      painted = true;
    }

    /** @brief Paints the MST on g and copies it to a separate tree graph.

        First paints the spanning tree arcs on g, then creates a copy of
        the MST in the tree parameter.

        @param[in] g The graph for which to compute the minimum spanning tree.
        @param[out] tree The graph where the resulting minimum spanning tree
               will be stored. This graph is cleared before the algorithm starts.
        @throw bad_alloc If there is not enough memory to construct the tree.
        @throw domain_error If g is a digraph.
    */
    void paint_min_spanning_tree(const GT & g, GT & tree)
    {
      paint_min_spanning_tree(g);
      clear_graph(tree); // clear destination graph

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto gp = it.get_curr();
          auto tp = tree.insert_node(gp->get_info());
          GT::map_nodes(gp, tp);
        }

      typedef Paint_Filt<GT, SA> F;
      for (Arc_Iterator<GT, F> it(g, F(sa)); it.has_curr(); it.next_ne())
        {
          auto ga = it.get_current_arc_ne();
          auto tsrc = mapped_node<GT>(g.get_src_node(ga));
          auto ttgt = mapped_node<GT>(g.get_tgt_node(ga));
          auto ta = tree.insert_arc(tsrc, ttgt, ga->get_info());
          GT::map_arcs(ga, ta);
        }
    }

    /** @brief Computes the minimum spanning tree using Kruskal's algorithm.

        @param[in] g The graph for which to compute the minimum spanning tree.
        @param[out] tree The graph where the resulting minimum spanning tree
               will be stored. This graph is cleared before the algorithm starts.
        @throw bad_alloc If there is not enough memory to construct the tree.
               In this case, the value of tree is indeterminate and not clean.
        @throw domain_error If g is a digraph.
     */
    void operator ()(const GT & g, GT & tree)
    {
      paint_min_spanning_tree(g, tree);
    }

    /** @brief Paints the minimum spanning tree on the graph using Kruskal's
        algorithm.

        After the algorithm completes, the arcs of g that belong to the
        spanning tree are marked with the Spanning_Tree bit.

        @param[in] g The graph for which to compute the minimum spanning tree.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If g is a digraph.
    */
    void operator ()(const GT & g)
    {
      paint_min_spanning_tree(g);
    }
  };
} // end namespace Aleph

# endif // KRUSKAL_H
