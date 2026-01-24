
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


/** @file tpl_spanning_tree.H
 *  @brief Spanning tree generation algorithms.
 *
 *  Constructs spanning trees using DFS or BFS traversal.
 *  For minimum spanning trees, see Prim.H and Kruskal.H.
 *
 *  ## Features
 *  - DFS spanning tree
 *  - BFS spanning tree (shortest paths in unweighted graphs)
 *  - Random spanning tree
 *
 *  ## Complexity: O(V + E)
 *
 *  @see Prim.H Minimum spanning tree
 *  @see Kruskal.H Minimum spanning tree
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_SPANNING_TREE_H
# define TPL_SPANNING_TREE_H

# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph {

/** @brief Compute a depth-first spanning tree of a graph.
 *
 * This class performs a depth-first traversal starting from a specified
 * node and constructs the spanning tree according to the DFS visit order.
 *
 * A spanning tree is a subgraph that includes all vertices of the original
 * graph connected by the minimum number of edges (V-1 edges for V vertices)
 * without forming any cycles.
 *
 * ## Algorithm:
 *
 * 1. Start from a source node and mark it as visited
 * 2. For each unvisited adjacent node, recursively build the tree
 * 3. Copy visited nodes and traversed arcs to the output tree
 * 4. Maintain node/arc mappings between original graph and tree
 *
 * ## Complexity:
 *
 * - **Time:** O(V + E) where V is vertices and E is edges
 * - **Space:** O(V) for recursion stack + O(V + E) for output tree
 *
 * ## Usage example:
 *
 * @code
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 * // ... populate graph ...
 * 
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> tree;
 * Find_Depth_First_Spanning_Tree<decltype(g)> dfs_tree;
 * auto root = dfs_tree(g, tree);
 * 
 * if (root != nullptr)
 *   std::cout << "Spanning tree has " << tree.get_num_nodes() << " nodes\n";
 * @endcode
 *
 * @note The algorithm uses the `Spanning_Tree` control bit on nodes and arcs.
 *       These bits are reset at the beginning of the algorithm.
 *
 * @note After execution, nodes and arcs in `tree` are mapped to their
 *       corresponding elements in `g` via cookies.
 *
 * @tparam GT Graph type (must satisfy Aleph graph concept).
 * @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 * @see Find_Breadth_First_Spanning_Tree For BFS-based spanning tree
 * @see graph_to_tree_node() To convert to Tree_Node representation
 *
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> 
class Find_Depth_First_Spanning_Tree
{
  SA sa;
  GT * gptr = nullptr;
  GT * tptr = nullptr;

  // Recursive DFS helper to build the spanning tree
  bool build_tree(typename GT::Node * gnode, typename GT::Arc * garc, 
                  typename GT::Node * tnode)
  {
    // Mark node and arc as part of spanning tree
    NODE_BITS(gnode).set_bit(Spanning_Tree, true);
    ARC_BITS(garc).set_bit(Spanning_Tree, true);

    // Create corresponding node in tree and establish mapping
    auto tree_tgt_node = tptr->insert_node(gnode->get_info());
    GT::map_nodes(gnode, tree_tgt_node);

    // Create corresponding arc in tree and establish mapping
    auto tarc = tptr->insert_arc(tnode, tree_tgt_node, garc->get_info()); 
    GT::map_arcs(garc, tarc);

    tnode = tree_tgt_node; 
    
    // Check if spanning tree is complete (all nodes covered)
    if (tptr->get_num_nodes() == gptr->get_num_nodes())
      return true;

    // Invariant: tree must be acyclic (nodes > arcs)
    assert(tptr->get_num_nodes() > tptr->get_num_arcs());

    // Explore adjacent nodes via DFS
    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
         i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes();
         i.next_ne())
      {
        auto arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        auto arc_tgt_node = i.get_tgt_node();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue;  // Target already visited via another arc

        if (build_tree(arc_tgt_node, arc, tnode))
          return true;
      }

    return false;
  }

  // Main algorithm entry point
  bool build_tree(GT & g, typename GT::Node * gnode, GT & tree)
  {
    gptr = &g;
    tptr = &tree;

    // Reset all control bits for fresh traversal
    gptr->reset_nodes();
    gptr->reset_arcs(); 

    // Ensure output tree is empty
    clear_graph(*tptr);
  
    // Mark starting node as visited
    NODE_BITS(gnode).set_bit(Spanning_Tree, true);
  
    // Create root node in tree and establish mapping
    auto tnode = tree.insert_node(gnode->get_info()); 
    GT::map_nodes(gnode, tnode);
  
    // Explore all adjacent arcs via DFS
    for (Node_Arc_Iterator<GT, SA> i(gnode, sa); 
         i.has_curr() and tptr->get_num_nodes() < gptr->get_num_nodes(); 
         i.next_ne())
      {
        auto arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        auto arc_tgt_node = i.get_tgt_node();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue;  // Target already visited via another arc

        if (build_tree(arc_tgt_node, arc, tnode))
          return true;
      }

    return true;
  }

public:

  /** @brief Construct a DFS spanning tree builder with optional arc filter.
   *
   * @param arc_filter Arc filter to control which arcs are traversed
   *        (default: show all arcs).
   */
  Find_Depth_First_Spanning_Tree(SA arc_filter = SA()) : sa(arc_filter) { /* empty */ }

  /** @brief Build a depth-first spanning tree starting from the first node.
   *
   * Constructs a spanning tree of the graph using depth-first traversal.
   * After execution, `tree` contains the spanning tree with nodes and arcs
   * mapped to their corresponding elements in `g`.
   *
   * @param[in] g Source graph to compute spanning tree from.
   * @param[out] tree Output graph that will contain the spanning tree.
   *             Will be cleared before construction.
   *
   * @return Pointer to the root node (first node of g) if the graph is
   *         connected and a spanning tree exists; `nullptr` otherwise.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::domain_error If the graph is empty.
   *
   * @note Uses the `Spanning_Tree` control bit on nodes and arcs.
   */
  typename GT::Node * operator () (GT & g, GT & tree)
  {
    ah_domain_error_if(g.get_num_nodes() == 0)
      << "Find_Depth_First_Spanning_Tree: graph is empty";

    auto start = g.get_first_node();
    if (not build_tree(g, start, tree)) 
      return nullptr;

    return start;
  }

  /** @brief Build a depth-first spanning tree starting from a specific node.
   *
   * Constructs a spanning tree of the graph using depth-first traversal
   * starting from the specified source node.
   *
   * @param[in] g Source graph to compute spanning tree from.
   * @param[in] gnode Starting node for the DFS traversal.
   * @param[out] tree Output graph that will contain the spanning tree.
   *             Will be cleared before construction.
   *
   * @return Pointer to the tree node corresponding to `gnode`.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `gnode` is `nullptr`.
   *
   * @note Uses the `Spanning_Tree` control bit on nodes and arcs.
   */
  typename GT::Node * operator () (GT & g, typename GT::Node * gnode, GT & tree)
  {
    ah_invalid_argument_if(gnode == nullptr)
      << "Find_Depth_First_Spanning_Tree: source node cannot be null";

    this->build_tree(g, gnode, tree);
    return static_cast<typename GT::Node *>(NODE_COOKIE(gnode));
  }
};


/** @brief Compute a breadth-first spanning tree of a graph.
 *
 * This class performs a breadth-first traversal starting from a specified
 * node and constructs the spanning tree according to the BFS visit order.
 *
 * Unlike DFS spanning trees, BFS spanning trees have the property that the
 * path from the root to any node in the tree is the shortest path (in terms
 * of number of edges) in the original graph.
 *
 * ## Algorithm:
 *
 * 1. Start from a source node and add its adjacent arcs to a queue
 * 2. Process arcs from the queue, adding unvisited nodes to the tree
 * 3. For each newly added node, enqueue its adjacent arcs
 * 4. Continue until all reachable nodes are visited
 *
 * ## Complexity:
 *
 * - **Time:** O(V + E) where V is vertices and E is edges
 * - **Space:** O(V) for the queue + O(V + E) for output tree
 *
 * ## Usage example:
 *
 * @code
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 * // ... populate graph ...
 * 
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> tree;
 * Find_Breadth_First_Spanning_Tree<decltype(g)> bfs_tree;
 * auto start = g.get_first_node();
 * bfs_tree(g, start, tree);
 * @endcode
 *
 * @note The algorithm uses the `Spanning_Tree` control bit on nodes and arcs.
 *
 * @tparam GT Graph type (must satisfy Aleph graph concept).
 * @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 * @throw std::bad_alloc If memory allocation fails for tree or internal queue.
 *
 * @see Find_Depth_First_Spanning_Tree For DFS-based spanning tree
 *
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> 
class Find_Breadth_First_Spanning_Tree
{
  SA sa;

  // Main BFS algorithm to build spanning tree
  void build_tree(GT & g, typename GT::Node * gp, GT & tree)
  {
    // Reset control bits for fresh traversal
    g.reset_bit_nodes(Spanning_Tree);
    g.reset_bit_arcs(Spanning_Tree);

    // Ensure output tree is empty
    clear_graph(tree); 

    // Create root node in tree and establish mapping
    auto tp = tree.insert_node(gp->get_info());
    GT::map_nodes(gp, tp);

    // Initialize queue with arcs adjacent to starting node
    DynListQueue<typename GT::Arc*> q;
    for (Node_Arc_Iterator<GT, SA> i(gp, sa); i.has_curr(); i.next_ne())
      q.put(i.get_current_arc_ne());

    // Mark starting node as visited
    NODE_BITS(gp).set_bit(Spanning_Tree, true); 

    // BFS loop
    while (not q.is_empty()) 
      {
        auto garc = q.get(); 
        ARC_BITS(garc).set_bit(Spanning_Tree, true); 
        auto gsrc = g.get_src_node(garc);  
        auto gtgt = g.get_tgt_node(garc);

        // Skip if both endpoints already visited
        if (IS_NODE_VISITED(gsrc, Spanning_Tree) and 
            IS_NODE_VISITED(gtgt, Spanning_Tree))
          continue;

        // Ensure gsrc is the visited node and gtgt is the new one
        if (IS_NODE_VISITED(gtgt, Spanning_Tree))
          std::swap(gsrc, gtgt);

        auto tsrc = mapped_node<GT>(gsrc);
        NODE_BITS(gtgt).set_bit(Spanning_Tree, true);

        // Create new node in tree and establish mapping
        auto ttgt = tree.insert_node(gtgt->get_info());
        GT::map_nodes(gtgt, ttgt);

        // Create new arc in tree and establish mapping
        auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
        GT::map_arcs(garc, tarc);
        
        // Check if spanning tree is complete
        if (tree.get_num_nodes() == g.get_num_nodes())
          break; 

        // Enqueue arcs adjacent to newly added node
        for (Node_Arc_Iterator<GT, SA> i(gtgt, sa); i.has_curr(); i.next_ne())
          {
            auto current_arc = i.get_current_arc_ne();
            if (IS_ARC_VISITED(current_arc, Spanning_Tree)) 
              continue;

            // Skip arcs where both endpoints are already visited
            if (IS_NODE_VISITED(g.get_src_node(current_arc), Spanning_Tree) and 
                IS_NODE_VISITED(g.get_tgt_node(current_arc), Spanning_Tree))
              continue;
            q.put(current_arc);
          }
      }
  }

public:

  /** @brief Construct a BFS spanning tree builder with optional arc filter.
   *
   * @param arc_filter Arc filter to control which arcs are traversed
   *        (default: show all arcs).
   */
  Find_Breadth_First_Spanning_Tree(SA arc_filter = SA()) : sa(arc_filter) { /* empty */ }

  /** @brief Build a breadth-first spanning tree starting from a specific node.
   *
   * Constructs a spanning tree of the graph using breadth-first traversal
   * starting from the specified source node.
   *
   * @param[in] g Source graph to compute spanning tree from.
   * @param[in] gnode Starting node for the BFS traversal.
   * @param[out] tree Output graph that will contain the spanning tree.
   *             Will be cleared before construction.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `gnode` is `nullptr`.
   *
   * @note Uses the `Spanning_Tree` control bit on nodes and arcs.
   * @note After execution, nodes and arcs in `tree` are mapped to their
   *       corresponding elements in `g` via cookies.
   */
  void operator () (GT & g, typename GT::Node * gnode, GT & tree) 
  {
    ah_invalid_argument_if(gnode == nullptr)
      << "Find_Breadth_First_Spanning_Tree: source node cannot be null";

    this->build_tree(g, gnode, tree);
  }

  /** @brief Build a breadth-first spanning tree starting from the first node.
   *
   * Constructs a spanning tree using the first node of the graph as root.
   *
   * @param[in] g Source graph to compute spanning tree from.
   * @param[out] tree Output graph that will contain the spanning tree.
   *
   * @return Pointer to the tree node corresponding to the root.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::domain_error If the graph is empty.
   */
  typename GT::Node * operator () (GT & g, GT & tree)
  {
    ah_domain_error_if(g.get_num_nodes() == 0)
      << "Find_Breadth_First_Spanning_Tree: graph is empty";

    auto start = g.get_first_node();
    this->build_tree(g, start, tree);
    return static_cast<typename GT::Node *>(NODE_COOKIE(start));
  }
};

/** @brief Build a spanning tree from an array of arcs.
 *
 * This class constructs a spanning tree graph from an array of arcs
 * that were previously computed by another algorithm (e.g., Kruskal, Prim).
 *
 * ## Usage example:
 *
 * @code
 * DynArray<Graph::Arc*> mst_arcs;
 * // ... populate mst_arcs from MST algorithm ...
 * 
 * Build_Spanning_Tree<Graph> builder;
 * Graph tree = builder(&mst_arcs);
 * @endcode
 *
 * @tparam GT Graph type.
 *
 * @ingroup Graphs
 */
template <class GT>
class Build_Spanning_Tree
{
public:

  /** @brief Construct spanning tree from array of arc pointers.
   *
   * @param[in] arcs Array of arc pointers that form the spanning tree.
   *
   * @return A new graph containing only the nodes and arcs from the
   *         spanning tree.
   *
   * @throw std::bad_alloc If memory allocation fails.
   */
  GT operator () (const DynArray<typename GT::Arc*> & arcs) const
  {
    return build_spanning_tree<GT>(arcs);
  }

  /** @brief Construct spanning tree from pointer to array of arc pointers.
   *
   * @param[in] arcs Pointer to array of arc pointers that form the spanning tree.
   *
   * @return A new graph containing only the nodes and arcs from the
   *         spanning tree.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `arcs` is `nullptr`.
   */
  GT operator () (const DynArray<typename GT::Arc*> * arcs) const
  {
    ah_invalid_argument_if(arcs == nullptr)
      << "Build_Spanning_Tree: arcs array cannot be null";
    return build_spanning_tree<GT>(*arcs);
  }
};

} // end namespace Aleph

# endif //  TPL_SPANNING_TREE_H
