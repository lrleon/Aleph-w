
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


/** @file Prim.H
 *  @brief Prim's algorithm for minimum spanning trees.
 *
 *  This file implements Prim's algorithm for finding the Minimum Spanning
 *  Tree (MST) of a connected, undirected, weighted graph. The algorithm
 *  grows the MST one vertex at a time, always adding the minimum-weight
 *  edge that connects a vertex in the tree to a vertex outside.
 *
 *  ## Algorithm Overview
 *
 *  1. Start with an arbitrary vertex
 *  2. Maintain a priority queue of edges crossing the cut
 *  3. Extract minimum-weight edge, add its endpoint to the tree
 *  4. Add new crossing edges to the queue
 *  5. Repeat until all vertices are in the tree
 *
 *  ## Key Features
 *
 *  - Finds minimum spanning tree for connected graphs
 *  - Uses binary heap for efficient edge selection
 *  - Supports custom distance/weight functions
 *  - Works with both List_Graph and Array_Graph
 *
 *  ## Complexity
 *
 *  | Implementation | Time | Space |
 *  |----------------|------|-------|
 *  | Binary Heap | O(E log V) | O(V) |
 *
 *  ## Comparison with Kruskal
 *
 *  | Aspect | Prim | Kruskal |
 *  |--------|------|---------|
 *  | Best for | Dense graphs | Sparse graphs |
 *  | Data structure | Priority queue | Union-Find |
 *  | Edge processing | One at a time | All sorted first |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build connected graph ...
 *
 *  List_Graph<Node, Arc> mst;
 *  Prim_Min_Spanning_Tree<decltype(g)> prim;
 *  prim(g, mst);
 *
 *  // mst now contains the minimum spanning tree
 *  ```
 *
 *  @see Kruskal.H Alternative MST algorithm (better for sparse graphs)
 *  @see tpl_graph_utils.H For graph traversal utilities
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef PRIM_H
# define PRIM_H

# include <ah-graph-concepts.H>

# include <tpl_agraph.H>
# include <tpl_graph_utils.H>
# include <archeap.H>
# include <ah-errors.H>
# include <cookie_guard.H>

namespace Aleph
{
  using namespace Aleph;

  template <AlephGraph GT>
  struct Prim_Info
  {
    typename GT::Node *tree_node = nullptr; // image in the spanning tree
    void *heap_node = nullptr; // pointer into the dedicated heap

    Prim_Info() : tree_node(nullptr), heap_node(nullptr)
    { /* empty */
    }
  };

# define PRIMINFO(p) static_cast<Prim_Info<GT>*>(NODE_COOKIE(p))
# define TREENODE(p) (PRIMINFO(p)->tree_node)
# define HEAPNODE(p) (PRIMINFO(p)->heap_node)

  template <class GT, class Distance>
  struct Prim_Heap_Info
  {
    // Returns reference to the heap_node pointer stored in Prim_Info
    // The actual type is determined by ArcHeap, but stored as void*
    void *& operator ()(typename GT::Node *p)
    {
      return PRIMINFO(p)->heap_node;
    }
  };

  template <class GT, class Distance>
  struct Simple_Prim_Heap
  {
    // Returns reference to the cookie pointer used as heap node storage
    void *& operator ()(typename GT::Node *p)
    {
      return p->cookie;
    }
  };

  template <class GT>
  struct Init_Prim_Info
  {
    GT & tree;

    Init_Prim_Info(GT & __tree) : tree(__tree)
    { /* empty */
    }

    void operator ()(const GT & g, typename GT::Node *p)
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Prim_Info<GT>;
      TREENODE(p) = tree.insert_node(p->get_info());
    }
  };

  template <AlephGraph GT>
  struct Uninit_Prim_Info
  {
    void operator ()(const GT &, typename GT::Node *p)
    {
      const Prim_Info<GT> *aux = PRIMINFO(p);
      GT::map_nodes(p, TREENODE(p));
      delete aux;
    }
  };


  /** Computes the minimum spanning tree of a graph using
      Prim's algorithm.

      This class uses Prim's algorithm to compute the minimum
      spanning tree of a graph and stores it in another graph.

      The resulting minimum spanning tree is fully mapped to the graph.

      The algorithm uses an internal queue whose maximum length is
      proportional to the number of nodes of the graph.

      Prim's algorithm is recommended for dense graphs.

      The procedure is parameterized with the following
      specifications:
      -# GT: the graph type, based on List_Graph.
      -# Distance<GT>: the class that reads an arc's weight, which must
      export the following members:
        -# typedef Distance<GT>::Distance_Type: the data type that
        represents an arc's weight.
        -# Distance<GT>::Distance_Type operator()(typename GT::Arc *a):
        returns the weight value of arc a.
        -# Distance<GT>::Max_Distance: static constant
        corresponding to the maximum distance value an algorithm
        would consider as infinite.
        -# typename Distance<GT>::Zero_Distance: static constant
        corresponding to the neutral element of the sum. Traditionally,
        in the vast majority of cases, this will be zero.
        .
      -# Compare<GT>: class that compares two
         weights and whose prototype is:
         - bool operator () (const typename
           Distance<GT>::Distance_Type & op1,
           const typename Distance<GT>::Distance_Type & op2) const
         . By default, this class implements the less-than relational
         operator.
      -# SA: arc filter

      @see Kruskal_Min_Spanning_Tree
      @ingroup Graphs
   */
  template <AlephGraph GT,
            class Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Prim_Min_Spanning_Tree
  {
    typedef Prim_Heap_Info<GT, Distance> Acc_Heap;

    typedef Simple_Prim_Heap<GT, Distance> Acc_Simple_Heap;

    typedef ArcHeap<GT, Distance, Acc_Heap> Heap;

    typedef ArcHeap<GT, Distance, Acc_Simple_Heap> Simple_Heap;

    Distance dist;
    SA sa;

  public:
    /** Constructor.

        \param[in] __dist access to each arc's distance
        \param[in] __sa arc iterator filter
    */
    Prim_Min_Spanning_Tree(Distance __dist = Distance(), SA __sa = SA())
      : dist(__dist), sa(__sa)
    {
      // empty
    }

  private:
    void paint_min_spanning_tree(const GT & g, typename GT::Node *first)
    {
      ah_domain_error_if(g.is_digraph()) << "g is a digraph";

      g.reset_nodes();
      g.reset_arcs();

      NODE_BITS(first).set_bit(Aleph::Spanning_Tree, true); // visited

      Simple_Heap heap(dist, Acc_Simple_Heap());
      for (Node_Arc_Iterator<GT, SA> it(first, sa); it.has_curr(); it.next_ne())
        {
          typename GT::Arc *arc = it.get_current_arc_ne();
          heap.put_arc(arc, it.get_tgt_node_ne());
        }

      const size_t V1 = g.get_num_nodes() - 1;
      size_t count = 0;

      while (count < V1 and not heap.is_empty())
        { // get the next smallest arc
          typename GT::Arc *min_arc = heap.get_min_arc();
          if (IS_ARC_VISITED(min_arc, Aleph::Spanning_Tree))
            continue;

          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);

          typename GT::Node *src = g.get_src_node(min_arc);
          typename GT::Node *tgt = g.get_tgt_node(min_arc);
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // this arc would close a cycle in the tree

          typename GT::Node *tgt_node =
              IS_NODE_VISITED(src, Aleph::Spanning_Tree) ? tgt : src;

          NODE_BITS(tgt_node).set_bit(Aleph::Spanning_Tree, true);

          // insert tgt_node's unvisited arcs into the heap
          for (Node_Arc_Iterator<GT, SA> it(tgt_node, sa); it.has_curr();
               it.next_ne())
            {
              typename GT::Arc *arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              typename GT::Node *tgt = it.get_tgt_node_ne();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // node visited ==> would cause a cycle

              heap.put_arc(arc, tgt);
            }

          ++count;
          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);
        }
    }

    void min_spanning_tree(const GT & g, typename GT::Node *first, GT & tree)
    {
      ah_domain_error_if(g.is_digraph()) << "g is a digraph";

      clear_graph(tree);

      Init_Prim_Info<GT> init(tree);
      Operate_On_Nodes<GT, Init_Prim_Info<GT>>()(g, init);
      g.reset_arcs();

      // Scope guard for exception safety - ensures Uninit is always called
      Scope_Guard cleanup_guard(g, [](const GT & graph) {
        Operate_On_Nodes<GT, Uninit_Prim_Info<GT>>()(graph);
      });

      NODE_BITS(first).set_bit(Aleph::Spanning_Tree, true); // visited

      Heap heap(dist, Acc_Heap());
      // insert the first node's initial arcs into the heap
      for (Node_Arc_Iterator<GT, SA> it(first, sa); it.has_curr(); it.next_ne())
        {
          typename GT::Arc *arc = it.get_current_arc_ne();
          heap.put_arc(arc, it.get_tgt_node_ne());
        }

      const size_t V1 = g.get_num_nodes() - 1;

      while (tree.get_num_arcs() < V1 and not heap.is_empty())
        { // get the next smallest arc
          typename GT::Arc *min_arc = heap.get_min_arc();
          if (IS_ARC_VISITED(min_arc, Aleph::Spanning_Tree))
            continue;

          ARC_BITS(min_arc).set_bit(Aleph::Spanning_Tree, true);

          typename GT::Node *src = g.get_src_node(min_arc);
          typename GT::Node *tgt = g.get_tgt_node(min_arc);
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // this arc would close a cycle in the tree

          typename GT::Node *tgt_node =
              IS_NODE_VISITED(src, Aleph::Spanning_Tree) ? tgt : src;

          NODE_BITS(tgt_node).set_bit(Aleph::Spanning_Tree, true);

          // insert tgt_node's unvisited arcs into the heap
          for (Node_Arc_Iterator<GT, SA> it(tgt_node, sa); it.has_curr();
               it.next_ne())
            {
              typename GT::Arc *arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              typename GT::Node *tgt = it.get_tgt_node_ne();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // node visited ==> would cause a cycle

              heap.put_arc(arc, tgt);
            }

          // insert the new arc into tree and map it
          typename GT::Arc *tree_arc =
              tree.insert_arc(TREENODE(src), TREENODE(tgt), min_arc->get_info());
          GT::map_arcs(min_arc, tree_arc);
        }

      // cleanup_guard destructor will call Uninit_Prim_Info for all nodes
    }

  public:
    /** Invokes the computation of the minimum spanning tree using
        Prim's algorithm.

        @param[in] g the graph whose minimum spanning tree is to be
        computed.
        @param[out] tree the graph where the resulting minimum
        spanning tree is to be stored. This graph is cleared before
        the algorithm starts.
        @throw bad_alloc if there is not enough memory to build
        tree. In this case tree's value is indeterminate and it is
        not clean.
    */
    void operator ()(const GT & g, GT & tree)
    {
      min_spanning_tree(g, g.get_first_node(), tree);
    }

    /** Invokes the computation of the minimum spanning tree using
        Prim's algorithm.

        @param[in] g the graph whose minimum spanning tree is to be
        computed.
        @param[in] start the node from which the algorithm starts.
        @param[out] tree the graph where the resulting minimum
        spanning tree is to be stored. This graph is cleared before
        the algorithm starts.
        @throw bad_alloc if there is not enough memory to build
        tree. In this case tree's value is indeterminate and it is
        not clean.
    */
    void operator ()(const GT & g, typename GT::Node *start, GT & tree)
    {
      min_spanning_tree(g, start, tree);
    }

    /// overload ()
    void operator ()(const GT & g)
    {
      paint_min_spanning_tree(g, g.get_first_node());
    }

    void operator ()(const GT & g, typename GT::Node *start)
    {
      paint_min_spanning_tree(g, start);
    }
  };


# undef HEAPNODE
# undef TREENODE
# undef PRIMINFO
} // end namespace Aleph

# endif // PRIM_H
