
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

# include <ah-graph-concepts.H>

# include <tpl_agraph.H>
# include <tpl_dfs_stack.H>
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
 * - **Space:** O(V) for the stack of the search + O(V + E) for output
 *   subgraph; the search does not recurse, so that a long path does not
 *   overflow the call stack
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
template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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

  // A node that waits for the one that it visits, its image in the
  // subgraph, and the next arc of it to look at
  struct Map_Frame
  {
    typename GT::Node * sg_src;
    Node_Arc_Iterator<GT, SA> it;

    [[gnu::always_inline, gnu::flatten]]
    Map_Frame(typename GT::Node * g_src, typename GT::Node * image, SA & sa)
      : sg_src(image), it(g_src, sa) {}

    [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }
  };

  // Marks p and counts it, and gives its frame, with its image in sg,
  // which it inserts if p has none
  [[gnu::always_inline]] Map_Frame visit(GT & sg, typename GT::Node * p)
  {
    NODE_BITS(p).set_bit(Build_Subtree, true);
    ++count;

    // Get or create mapped node in subgraph
    auto sg_p = mapped_node<GT>(p);
    if (sg_p == nullptr)
      {
        sg_p = sg.insert_node(p->get_info());
        GT::map_nodes(p, sg_p);
      }
    return Map_Frame(p, sg_p, sa);
  }

  // Copies into sg the arcs of f that were not, with their targets, up to
  // one whose target was not visited, which it gives, with the iterator
  // on that arc; nullptr when the arcs are done
  [[gnu::always_inline]]
  typename GT::Node * next_to_visit(GT & sg, Map_Frame & f)
  {
    for (; f.it.has_curr(); f.next_arc())
      {
        auto arc = f.it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Build_Subtree))
          continue;

        // Get the target before the mark, which the compiler takes for a
        // store that could change the iterator
        auto g_tgt = f.it.get_tgt_node_ne();

        // Mark arc as visited
        ARC_BITS(arc).set_bit(Build_Subtree, true);

        // Get or create mapped target node
        auto sg_tgt = mapped_node<GT>(g_tgt);
        if (sg_tgt == nullptr)
          {
            sg_tgt = sg.insert_node(g_tgt->get_info());
            GT::map_nodes(g_tgt, sg_tgt);
          }

        // Insert arc in subgraph and establish mapping
        auto sg_arc = sg.insert_arc(f.sg_src, sg_tgt, arc->get_info());
        GT::map_arcs(arc, sg_arc);

        if (not IS_NODE_VISITED(g_tgt, Build_Subtree))
          return g_tgt;
      }
    return nullptr;
  }

  // DFS that builds a mapped subgraph. A path can have as many nodes as
  // the graph, too many for recursion, so that the search keeps its own
  // stack; the order of the visits is that of a recursion.
  void build_subgraph(GT & sg, typename GT::Node * g_src)
  {
    if (IS_NODE_VISITED(g_src, Build_Subtree))
      return;

    // The node being visited keeps its iterator in here until it visits a
    // node; then it waits in frames, where it goes on when it comes back
    Dfs_Stack<Map_Frame> frames;
    Map_Frame here = visit(sg, g_src);
    while (true)
      {
        if (auto g_tgt = next_to_visit(sg, here))
          { // the iterator stays on the arc to g_tgt, as in a recursion
            frames.push(std::move(here));
            here = visit(sg, g_tgt);
            continue;
          }

        while (true) // back to the nodes that wait
          {
            if (frames.is_empty())
              return;

            Map_Frame & f = frames.top();
            f.next_arc(); // past the arc to the node that was visited
            if (auto g_tgt = next_to_visit(sg, f))
              {
                here = visit(sg, g_tgt);
                break;
              }
            frames.pop();
          }
      }
  }

  // The next arc of a node to look at
  struct Arc_Frame
  {
    Node_Arc_Iterator<GT, SA> it;

    [[gnu::always_inline, gnu::flatten]]
    Arc_Frame(typename GT::Node * p, SA & sa) : it(p, sa) {}

    [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }

    // Moves the iterator, while count < n, to the next arc not visited
    // whose target was not visited either, and gives that target; it marks
    // the arcs not visited that it passes. nullptr when the arcs are done
    // or count == n.
    [[gnu::always_inline, gnu::flatten]]
    typename GT::Node * next_target(const size_t & count, const size_t n) noexcept
    {
      for (; count < n and it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          if (IS_ARC_VISITED(arc, Build_Subtree))
            continue;

          // Mark arc as visited, after taking its target: the compiler
          // takes the mark for a store that could change the iterator
          auto tgt = it.get_tgt_node_ne();
          ARC_BITS(arc).set_bit(Build_Subtree, true);
          if (not IS_NODE_VISITED(tgt, Build_Subtree))
            return tgt;
        }
      return nullptr;
    }
  };

  // DFS that builds the list of the nodes reachable from p. A path can
  // have as many nodes as the graph, too many for recursion, so that the
  // search keeps its own stack; the order of the visits is that of a
  // recursion.
  template <template <class> class List>
  void build_subgraph(List<typename GT::Node*> & l, typename GT::Node * p)
  {
    if (IS_NODE_VISITED(p, Build_Subtree))
      return;

    const size_t n = gptr->get_num_nodes();

    // Marks q, appends it to l and gives its frame
    auto visit = [this, &l](typename GT::Node * q)
      {
        NODE_BITS(q).set_bit(Build_Subtree, true);
        ++count;
        l.append(q);
        return Arc_Frame(q, sa);
      };

    // The node being visited keeps its iterator in here until it visits a
    // node; then it waits in frames, where it goes on when it comes back
    Dfs_Stack<Arc_Frame> frames;
    Arc_Frame here = visit(p);
    while (true)
      {
        if (auto tgt = here.next_target(count, n))
          { // the iterator stays on the arc to tgt, as in a recursion
            frames.push(std::move(here));
            here = visit(tgt);
            continue;
          }

        while (true) // back to the nodes that wait
          {
            if (frames.is_empty())
              return;

            Arc_Frame & f = frames.top();
            f.next_arc(); // past the arc to the node that was visited
            if (auto tgt = f.next_target(count, n))
              {
                here = visit(tgt);
                break;
              }
            frames.pop();
          }
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
template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
