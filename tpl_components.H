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

/** @file tpl_components.H
 *  @brief Graph connectivity and connected components.
 *
 *  Algorithms for finding connected components in undirected graphs
 *  and testing connectivity. Uses DFS or Union-Find.
 *
 *  ## Features
 *  - Count connected components
 *  - Extract component subgraphs
 *  - Test if graph is connected
 *
 *  ## Complexity: O(V + E)
 *
 *  @see Tarjan.H Strongly connected components for digraphs
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_COMPONENTS_H
# define TPL_COMPONENTS_H

# include <tpl_agraph.H>
# include <ah-errors.H>

namespace Aleph {

/** @brief Build a mapped subgraph from a graph starting at a given node.
 *
 * This class performs a depth-first traversal of graph `g` starting from
 * a source node and constructs a mapped copy of all reachable nodes and arcs
 * (the entire graph if connected, or a connected component if not).
 *
 * The resulting subgraph maintains bidirectional mappings via cookies between
 * the original graph elements and their copies in the subgraph.
 *
 * ## Algorithm:
 *
 * 1. Start from source node and mark it as visited
 * 2. Create a copy of the node in the subgraph and establish mapping
 * 3. For each unvisited adjacent arc, copy it and recurse on target node
 * 4. Continue until all reachable nodes are visited
 *
 * ## Complexity:
 *
 * - **Time:** O(V + E) where V is vertices and E is edges in the component
 * - **Space:** O(V) for recursion stack + O(V + E) for output subgraph
 *
 * ## Usage example:
 *
 * @code
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 * // ... populate graph ...
 * 
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> subgraph;
 * Build_Subgraph<decltype(g)> builder;
 * builder(g, subgraph, g.get_first_node());
 * @endcode
 *
 * @note Uses the `Build_Subtree` control bit on nodes and arcs.
 *
 * @tparam GT Graph type (must satisfy Aleph graph concept).
 * @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 * @see Inconnected_Components For computing all connected components
 * @see copy_graph() For copying an entire graph
 *
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Build_Subgraph
{
  SA sa;
  const GT * gptr = nullptr;
  size_t count = 0;

public:

  /** @brief Construct a subgraph builder with optional arc filter.
   *
   * @param arc_filter Arc filter to control which arcs are traversed
   *        (default: show all arcs).
   */
  Build_Subgraph(SA arc_filter = SA())
    : sa(arc_filter) { /* empty */ }

  private:

  // Recursive DFS to build mapped subgraph
  void build_subgraph(GT & sg, typename GT::Node * g_src)
  {
    if (IS_NODE_VISITED(g_src, Build_Subtree))
      return;

    // Mark source node as visited
    NODE_BITS(g_src).set_bit(Build_Subtree, true);
    ++count;

    // Get or create mapped node in subgraph
    auto sg_src = mapped_node<GT>(g_src);
    if (sg_src == nullptr)
      {
        sg_src = sg.insert_node(g_src->get_info());
        GT::map_nodes(g_src, sg_src);
      }

    // Explore adjacent arcs
    for (Node_Arc_Iterator<GT, SA> i(g_src, sa); i.has_curr(); i.next_ne())
      {
        auto arc = i.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Build_Subtree))
          continue;

        // Mark arc as visited
        ARC_BITS(arc).set_bit(Build_Subtree, true);

        // Get or create mapped target node
        auto g_tgt = i.get_tgt_node();
        auto sg_tgt = mapped_node<GT>(g_tgt);
        if (sg_tgt == nullptr)
          {
            sg_tgt = sg.insert_node(g_tgt->get_info());
            GT::map_nodes(g_tgt, sg_tgt);
          }

        // Insert arc in subgraph and establish mapping
        auto sg_arc = sg.insert_arc(sg_src, sg_tgt, arc->get_info());
        GT::map_arcs(arc, sg_arc);

        // Recurse on target node
        build_subgraph(sg, g_tgt);
      }
  }

  // Recursive DFS to build list of reachable nodes
  template <template <class> class List>
  void build_subgraph(List<typename GT::Node*> & l, typename GT::Node * p)
  {
    if (IS_NODE_VISITED(p, Build_Subtree))
      return;

    // Mark node as visited and add to list
    NODE_BITS(p).set_bit(Build_Subtree, true);
    ++count;
    l.append(p);

    // Explore adjacent arcs
    for (Node_Arc_Iterator<GT, SA> it(p, sa);
         count < gptr->get_num_nodes() and it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Build_Subtree))
          continue;

        // Mark arc as visited and recurse
        ARC_BITS(arc).set_bit(Build_Subtree, true);
        build_subgraph(l, it.get_tgt_node());
      }
  }

public:

  /** @brief Build a mapped subgraph starting from a specific node.
   *
   * Performs a DFS traversal from `g_src` and constructs a mapped copy
   * of all reachable nodes and arcs in `sg`.
   *
   * @param[in] g Source graph to traverse.
   * @param[out] sg Output subgraph (should be empty; will contain mapped copy).
   * @param[in] g_src Starting node for the traversal.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `g_src` is `nullptr`.
   *
   * @note Uses the `Build_Subtree` control bit on nodes and arcs.
   * @note After execution, nodes/arcs in `sg` are mapped to `g` via cookies.
   */
  void operator () (const GT & g, GT & sg, typename GT::Node * g_src)
  {
    ah_invalid_argument_if(g_src == nullptr)
      << "Build_Subgraph: source node cannot be null";

    gptr = &g;
    count = 0;
    build_subgraph(sg, g_src);
  }

  /** @brief Build and return a subgraph containing all nodes reachable from src.
   *
   * @param[in] g Source graph to traverse.
   * @param[in] src Starting node for the traversal.
   *
   * @return A new graph containing mapped copies of all reachable nodes/arcs.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `src` is `nullptr`.
   */
  GT operator () (const GT & g, typename GT::Node * src)
  {
    ah_invalid_argument_if(src == nullptr)
      << "Build_Subgraph: source node cannot be null";

    GT sg;
    gptr = &g;
    count = 0;
    build_subgraph(sg, src);
    return sg;
  }

  /** @brief Build a list of all nodes reachable from a source node.
   *
   * Performs DFS from `src` and collects all reachable nodes into `list`.
   *
   * @param[in] g Source graph to traverse.
   * @param[out] list Output list to store reachable node pointers.
   * @param[in] src Starting node for the traversal.
   *
   * @throw std::bad_alloc If memory allocation fails.
   * @throw std::invalid_argument If `src` is `nullptr`.
   *
   * @note Uses the `Build_Subtree` control bit on nodes and arcs.
   */
  void operator () (const GT & g, DynList<typename GT::Node*> & list,
                    typename GT::Node * src)
  {
    ah_invalid_argument_if(src == nullptr)
      << "Build_Subgraph: source node cannot be null";

    gptr = &g;
    count = 0;
    build_subgraph<DynList>(list, src);
  }
};


/** @brief Compute the connected components of a graph.
 *
 * This class takes a graph `g` (possibly disconnected) and computes all
 * its connected components, storing them as a list of mapped subgraphs
 * or as lists of node pointers.
 *
 * A connected component is a maximal subgraph in which any two vertices
 * are connected to each other by paths.
 *
 * ## Algorithm:
 *
 * 1. Iterate through all nodes of the graph
 * 2. For each unvisited node, perform DFS to find all reachable nodes
 * 3. Store each component as a separate subgraph or node list
 *
 * ## Complexity:
 *
 * - **Time:** O(V + E) where V is vertices and E is edges
 * - **Space:** O(V + E) for the output components
 *
 * ## Usage example:
 *
 * @code
 * List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 * // ... populate possibly disconnected graph ...
 * 
 * DynList<decltype(g)> components;
 * Inconnected_Components<decltype(g)> cc;
 * cc(g, components);
 * 
 * std::cout << "Graph has " << components.size() << " components\n";
 * @endcode
 *
 * @note The name "Inconnected" is maintained for backward compatibility.
 *       It refers to finding components in a potentially disconnected graph.
 *
 * @note Uses the `Build_Subtree` control bit on nodes and arcs.
 *
 * @tparam GT Graph type (must satisfy Aleph graph concept).
 * @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 * @see Build_Subgraph For building a single component
 * @see copy_graph() For copying an entire connected graph
 *
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>>
class Unconnected_Components
{
  SA sa;

public:

  /** @brief Construct a component finder with optional arc filter.
   *
   * @param arc_filter Arc filter to control which arcs are traversed
   *        (default: show all arcs).
   */
  Unconnected_Components(SA arc_filter = SA())
    : sa(arc_filter) { /* empty */ }

  /** @brief Compute connected components as mapped subgraphs.
   *
   * @tparam List Container template for the output list.
   * @param[in] g Source graph.
   * @param[out] list Output list of subgraphs, one per component.
   *
   * @throw std::bad_alloc If memory allocation fails.
   */
  template <template <class> class List>
  void compute_blocks(const GT & g, List<GT> & list)
  {
    g.reset_nodes();
    g.reset_arcs();
    size_t count = 0;  // Count of visited nodes

    for (typename GT::Node_Iterator it(g);
         count < g.get_num_nodes() and it.has_curr(); it.next_ne())
      {
        auto curr = it.get_current_node_ne();
        if (IS_NODE_VISITED(curr, Build_Subtree))
          continue;

        // Create new subgraph for this component
        GT & subgraph = list.append(GT());

        Build_Subgraph<GT, SA> build(sa);
        build(g, subgraph, curr);

        count += subgraph.get_num_nodes();
      }
  }

  /** @brief Compute connected components as lists of node pointers.
   *
   * @tparam List Container template for the output lists.
   * @param[in] g Source graph.
   * @param[out] list Output list of node lists, one per component.
   *
   * @throw std::bad_alloc If memory allocation fails.
   */
  template <template <class> class List>
  void compute_lists(const GT & g, List<List<typename GT::Node*>> & list)
  {
    g.reset_nodes();
    g.reset_arcs();
    size_t count = 0;  // Count of visited nodes

    for (typename GT::Node_Iterator i(g);
         count < g.get_num_nodes() and i.has_curr(); i.next_ne())
      {
        auto curr = i.get_current_node_ne();
        if (IS_NODE_VISITED(curr, Build_Subtree))
          continue;

        // Create new node list for this component
        auto & l = list.append(List<typename GT::Node*>());

        Build_Subgraph<GT, SA> build(sa);
        build(g, l, curr);

        count += l.size();
      }
  }

  /** @brief Compute connected components as mapped subgraphs.
   *
   * Finds all connected components in the graph and stores each as a
   * separate mapped subgraph in the output list.
   *
   * @param[in] g Source graph.
   * @param[out] list Output list of subgraphs, one per component.
   *             Each subgraph is mapped to `g` via cookies.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @note Uses the `Build_Subtree` control bit on nodes and arcs.
   */
  void operator () (const GT & g, DynList<GT> & list)
  {
    compute_blocks<DynList>(g, list);
  }

  /** @brief Compute connected components as lists of node pointers.
   *
   * Finds all connected components and stores each as a list of
   * pointers to the original nodes in `g`.
   *
   * @param[in] g Source graph.
   * @param[out] list Output list of node lists, one per component.
   *
   * @throw std::bad_alloc If memory allocation fails.
   *
   * @note Uses the `Build_Subtree` control bit on nodes and arcs.
   */
  void operator () (const GT & g, DynList<DynList<typename GT::Node*>> & list)
  {
    compute_lists<DynList>(g, list);
  }

  /** @brief Count the number of connected components in a graph.
   *
   * @param[in] g Source graph.
   *
   * @return Number of connected components.
   */
  size_t count_components(const GT & g)
  {
    DynList<DynList<typename GT::Node*>> components;
    compute_lists<DynList>(g, components);
    return components.size();
  }

  /** @brief Check if a graph is connected.
   *
   * @param[in] g Source graph.
   *
   * @return `true` if the graph has exactly one connected component
   *         (or is empty), `false` otherwise.
   */
  bool is_connected(const GT & g)
  {
    if (g.get_num_nodes() <= 1)
      return true;
    return count_components(g) == 1;
  }
};


} // end namespace Aleph

# endif //  TPL_COMPONENTS_H
