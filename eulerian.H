
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


/** @file eulerian.H
 *  @brief Eulerian graph detection and path/cycle finding.
 *
 *  This file provides algorithms to test whether a graph or digraph is
 *  Eulerian (contains an Eulerian cycle) or semi-Eulerian (contains an
 *  Eulerian path).
 *
 *  ## Definitions
 *
 *  - **Eulerian Cycle**: A cycle that visits every edge exactly once
 *  - **Eulerian Path**: A path that visits every edge exactly once
 *  - **Eulerian Graph**: Contains an Eulerian cycle
 *  - **Semi-Eulerian Graph**: Contains an Eulerian path but not a cycle
 *
 *  ## Conditions for Eulerian Graphs
 *
 *  ### Undirected Graphs
 *  - Eulerian cycle: Connected + all vertices have even degree
 *  - Eulerian path: Connected + exactly 0 or 2 vertices have odd degree
 *
 *  ### Directed Graphs
 *  - Eulerian cycle: Strongly connected + in-degree = out-degree for all
 *  - Eulerian path: Weakly connected + at most one vertex with out-in=1,
 *                   at most one with in-out=1, rest balanced
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | Test Eulerian | O(V + E) |
 *  | With connectivity check | O(V + E) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build graph ...
 *
 *  Test_Eulerian<decltype(g)> test;
 *  
 *  // Simple check (backward compatible)
 *  if (test(g))
 *    std::cout << "Graph has Eulerian cycle\n";
 *
 *  // Detailed check
 *  auto result = test.compute(g);
 *  switch (result) {
 *    case Eulerian_Type::Cycle: cout << "Has Eulerian cycle\n"; break;
 *    case Eulerian_Type::Path:  cout << "Has Eulerian path\n"; break;
 *    case Eulerian_Type::None:  cout << "Not Eulerian\n"; break;
 *  }
 *  ```
 *
 *  @see tpl_graph.H Graph data structures
 *  @see hamiltonian.H Hamiltonian path/cycle algorithms
 *  @see tpl_test_connectivity.H Connectivity testing
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef EULERIAN_H
# define EULERIAN_H

# include <ah-graph-concepts.H>

# include <tpl_graph.H>
# include <tpl_graph_utils.H>

namespace Aleph
{

/**
 * @brief Enumeration for Eulerian graph classification.
 *
 * Represents the three possible Eulerian states of a graph:
 * - Cycle: Contains an Eulerian cycle (visits all edges exactly once, returns to start)
 * - Path: Contains an Eulerian path but not a cycle (visits all edges exactly once)
 * - None: Neither Eulerian cycle nor path exists
 */
enum class Eulerian_Type
{
  Cycle,  ///< Graph has an Eulerian cycle
  Path,   ///< Graph has an Eulerian path but not a cycle
  None    ///< Graph is not Eulerian
};

namespace eulerian_detail
{
  /** @brief Degree of a node as an Eulerian walk counts it.

      The degree of p by the arcs that sa shows: in a graph, a loop,
      which is once in the list of its node, adds 2; in a digraph, whose
      lists hold the arcs out, it is the degree out, where a loop adds 1.
   */
  template <class GT, class SA>
  size_t degree(const GT & g, typename GT::Node * p, SA & sa)
  {
    size_t d = 0;
    for (Node_Arc_Iterator<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
      d += (not g.is_digraph() and it.get_tgt_node_ne() == p) ? 2 : 1;
    return d;
  }

  /** @brief Whether the arcs that sa shows are all in one component.

      Their ends join the components, in either direction, which is what
      an Eulerian walk needs besides the degrees; the nodes without arcs
      do not count, and a graph without arcs has them all together. The
      cookies of the nodes hold a union-find, and are left null.
   */
  template <class GT, class SA>
  bool arcs_are_together(const GT & g, SA & sa)
  {
    using Node = typename GT::Node;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      NODE_COOKIE(it.get_curr()) = it.get_curr();

    auto root = [] (Node * x) noexcept
      {
        Node * r = x;
        while (NODE_COOKIE(r) != r)
          r = static_cast<Node *>(NODE_COOKIE(r));
        while (x != r) // path compression
          {
            Node * next = static_cast<Node *>(NODE_COOKIE(x));
            NODE_COOKIE(x) = r;
            x = next;
          }
        return r;
      };

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        Node * a = root(g.get_src_node(it.get_curr_ne()));
        Node * b = root(g.get_tgt_node(it.get_curr_ne()));
        if (a != b)
          NODE_COOKIE(a) = b;
      }

    bool together = true;
    Node * component = nullptr;
    for (Arc_Iterator<GT, SA> it(g, sa); together and it.has_curr(); it.next_ne())
      {
        Node * r = root(g.get_src_node(it.get_curr_ne()));
        if (component == nullptr)
          component = r;
        together = r == component;
      }

    g.reset_cookie_nodes();
    return together;
  }
} // namespace eulerian_detail

/**
 * @brief Tests whether a graph or digraph is Eulerian.
 *
 * A graph is Eulerian if it contains an Eulerian cycle (a cycle that
 * visits every edge exactly once). This class tests the necessary and
 * sufficient conditions for Eulerian graphs.
 *
 * ## Conditions Tested
 *
 * **Undirected graphs**:
 * - Eulerian cycle: All vertices have even degree AND graph is connected
 * - Eulerian path: Exactly 2 vertices have odd degree AND graph is connected
 *
 * **Directed graphs**:
 * - Eulerian cycle: in-degree == out-degree for all vertices AND strongly connected
 * - Eulerian path: At most one vertex with out-in=1, at most one with in-out=1
 *
 * @tparam GT Graph type (List_Graph, Array_Graph, etc.)
 * @tparam SN Node filter for iteration (default: show all nodes)
 * @tparam SA Arc filter for iteration (default: show all arcs)
 *
 * ## Complexity
 *
 * - Time: O(V + E) including connectivity check
 * - Space: O(V) for connectivity check
 *
 * @par Example
 * @code
 * List_Graph<Node, Arc> g;
 * // Build a graph where all vertices have even degree
 * auto n1 = g.insert_node();
 * auto n2 = g.insert_node();
 * auto n3 = g.insert_node();
 * g.insert_arc(n1, n2);
 * g.insert_arc(n2, n3);
 * g.insert_arc(n3, n1);  // Triangle - all degrees are 2
 *
 * Test_Eulerian<decltype(g)> test;
 * assert(test(g) == true);  // Graph is Eulerian
 *
 * // Or use detailed result:
 * assert(test.compute(g) == Eulerian_Type::Cycle);
 * @endcode
 *
 * @see hamiltonian.H For Hamiltonian path/cycle testing
 * @see tpl_test_connectivity.H For connectivity testing
 * @ingroup Graphs
 * @author Leandro R. León
 */
template <AlephGraph GT, 
	  NodeFilter<GT> SN = Dft_Show_Node<GT>,
	  ArcFilter<GT> SA = Dft_Show_Arc<GT> >
class Test_Eulerian
{
  Kept_Filter<SN> sn;
  Kept_Filter<SA> sa;

  /**
   * @brief Analyze Eulerian properties of an undirected graph.
   *
   * The degrees count a loop twice; the arcs must be together.
   *
   * @param g The undirected graph to test
   * @return Eulerian_Type classification
   */
  Eulerian_Type analyze_graph(GT & g)
  {
    assert(not g.is_digraph());

    size_t odd_degree_count = 0;
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      odd_degree_count += eulerian_detail::degree(g, it.get_curr(), sa.get()) % 2;

    if (odd_degree_count != 0 and odd_degree_count != 2)
      return Eulerian_Type::None; // more than 2 odd degrees: not Eulerian

    if (not eulerian_detail::arcs_are_together(g, sa.get()))
      return Eulerian_Type::None;

    return odd_degree_count == 0 ? Eulerian_Type::Cycle : Eulerian_Type::Path;
  }

  /**
   * @brief Analyze Eulerian properties of a directed graph.
   *
   * With every node balanced, or all but a start and an end, the arcs
   * must be together; balanced, they are then strongly connected.
   *
   * @param g The directed graph to test
   * @return Eulerian_Type classification
   */
  Eulerian_Type analyze_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

    // First pass: count in-degrees using node counters
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    // Count imbalanced vertices
    size_t start_vertices = 0;   // out - in == 1
    size_t end_vertices = 0;     // in - out == 1
    size_t imbalanced = 0;       // |in - out| > 1

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        const long out_deg = long(eulerian_detail::degree(g, p, sa.get()));
        const long in_deg = NODE_COUNTER(p);
        const long diff = out_deg - in_deg;

        if (diff == 0)
          continue;  // Balanced vertex
        else if (diff == 1)
          ++start_vertices;
        else if (diff == -1)
          ++end_vertices;
        else
          ++imbalanced;
      }

    const bool cycle = start_vertices == 0 and end_vertices == 0;
    if (imbalanced > 0 or not (cycle or (start_vertices == 1 and end_vertices == 1)))
      return Eulerian_Type::None;

    if (not eulerian_detail::arcs_are_together(g, sa.get()))
      return Eulerian_Type::None;

    return cycle ? Eulerian_Type::Cycle : Eulerian_Type::Path;
  }

public:

  /**
   * @brief Construct an Eulerian tester with optional filters.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Test_Eulerian(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(std::move(__sn)), sa(std::move(__sa))
  {
    // empty 
  }

  /**
   * @brief Compute detailed Eulerian classification.
   *
   * Determines whether the graph has an Eulerian cycle, path, or neither.
   * This method checks both degree conditions AND connectivity.
   *
   * @param g The graph to test
   * @return Eulerian_Type classification (Cycle, Path, or None)
   *
   * @par Example
   * @code
   * Test_Eulerian<decltype(g)> test;
   * auto result = test.compute(g);
   * if (result == Eulerian_Type::Cycle)
   *   std::cout << "Has Eulerian cycle\n";
   * @endcode
   */
  Eulerian_Type compute(GT & g)
  {
    if (g.is_digraph())
      return analyze_digraph(g);
    else
      return analyze_graph(g);
  }

  /**
   * @brief Test if a graph has an Eulerian cycle.
   *
   * Automatically detects whether the graph is directed or undirected
   * and applies the appropriate test. This method checks both degree
   * conditions AND connectivity.
   *
   * @param g The graph to test
   * @return true if the graph contains an Eulerian cycle
   *
   * @note For detailed information (cycle vs path vs none), use compute().
   */
  bool operator () (GT & g)
  {
    return compute(g) == Eulerian_Type::Cycle;
  }

  /**
   * @brief Test if a graph has an Eulerian path.
   *
   * An Eulerian path visits every edge exactly once but may not
   * return to the starting vertex.
   *
   * @param g The graph to test
   * @return true if the graph contains an Eulerian path (cycle or path)
   *
   * @note Returns true for graphs with Eulerian cycles too.
   */
  bool has_eulerian_path(GT & g)
  {
    auto result = compute(g);
    return result == Eulerian_Type::Cycle || result == Eulerian_Type::Path;
  }

  /**
   * @brief Check only degree conditions (without connectivity check).
   *
   * This is faster but may return true for disconnected graphs that
   * don't actually have Eulerian cycles/paths.
   *
   * @param g The graph to test
   * @return true if degree conditions for Eulerian cycle are met
   *
   * @warning Does not verify connectivity. Use operator() or compute()
   *          for a complete test.
   */
  bool test_degree_only(GT & g)
  {
    if (g.is_digraph())
      {
        g.reset_counter_nodes();
        for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
          NODE_COUNTER(it.get_tgt_node_ne())++;

        for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
          {
            typename GT::Node * p = it.get_curr();
            if (long(eulerian_detail::degree(g, p, sa.get())) != NODE_COUNTER(p))
              return false;
          }
        return true;
      }
    else
      {
        for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
          if (eulerian_detail::degree(g, it.get_curr(), sa.get()) % 2 == 1)
            return false;
        return true;
      }
  }
};


/**
 * @brief Finds and constructs an Eulerian path or cycle using Hierholzer's algorithm.
 *
 * Hierholzer's algorithm efficiently constructs an Eulerian path or cycle
 * in O(E) time. It works by:
 * 1. Starting at an appropriate vertex (odd-degree for path, any for cycle)
 * 2. Following edges until returning to start or getting stuck
 * 3. Splicing in sub-tours from vertices with unused edges
 *
 * @tparam GT Graph type (List_Graph, Array_Graph, etc.)
 * @tparam SN Node filter for iteration (default: show all nodes)
 * @tparam SA Arc filter for iteration (default: show all arcs)
 *
 * ## Complexity
 *
 * - Time: O(E)
 * - Space: O(E) for storing the path
 *
 * @par Example
 * @code
 * List_Graph<Node, Arc> g;
 * // Build triangle
 * auto n1 = g.insert_node();
 * auto n2 = g.insert_node();
 * auto n3 = g.insert_node();
 * g.insert_arc(n1, n2);
 * g.insert_arc(n2, n3);
 * g.insert_arc(n3, n1);
 *
 * Find_Eulerian_Path<decltype(g)> finder;
 * auto [path, type] = finder(g);
 *
 * if (type == Eulerian_Type::Cycle)
 *   for (auto arc : path)
 *     // Process arc...
 * @endcode
 *
 * @ingroup Graphs
 * @author Leandro R. León
 */
template <AlephGraph GT, 
          NodeFilter<GT> SN = Dft_Show_Node<GT>,
          ArcFilter<GT> SA = Dft_Show_Arc<GT> >
class Find_Eulerian_Path
{
  Kept_Filter<SN> sn;
  Kept_Filter<SA> sa;

  /**
   * @brief Find starting vertex for Eulerian path/cycle.
   *
   * For cycles: any vertex with edges
   * For paths: a vertex with odd degree (undirected) or out-in=1 (directed)
   *
   * @param g The graph
   * @param type The Eulerian type (Cycle or Path)
   * @return Starting vertex, or nullptr if graph has no edges
   */
  typename GT::Node * find_start_vertex(GT & g, Eulerian_Type type)
  {
    typename GT::Node * start = nullptr;
    typename GT::Node * odd_vertex = nullptr;

    if (g.is_digraph())
      {
        // For digraphs, find vertex with out-degree > in-degree (start of path)
        g.reset_counter_nodes();
        for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
          NODE_COUNTER(it.get_tgt_node_ne())++;

        for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
          {
            typename GT::Node * p = it.get_curr();
            const long out_deg = long(eulerian_detail::degree(g, p, sa.get()));
            if (out_deg > 0)
              {
                if (start == nullptr)
                  start = p;
                if (out_deg > NODE_COUNTER(p))
                  odd_vertex = p;  // out > in: this is start of path
              }
          }
      }
    else
      {
        // For undirected, find vertex with odd degree
        for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
          {
            typename GT::Node * p = it.get_curr();
            const size_t deg = eulerian_detail::degree(g, p, sa.get());
            if (deg > 0)
              {
                if (start == nullptr)
                  start = p;
                if (deg % 2 == 1)
                  odd_vertex = p;
              }
          }
      }

    // For paths, must start at odd-degree vertex
    if (type == Eulerian_Type::Path && odd_vertex != nullptr)
      return odd_vertex;

    return start;
  }

  /**
   * @brief Hierholzer's algorithm for undirected graphs.
   *
   * @param g The undirected graph
   * @param start Starting vertex
   * @return List of arcs forming the Eulerian path/cycle
   */
  DynList<typename GT::Arc*> hierholzer_undirected(GT & g, typename GT::Node * start)
  {
    // Mark all arcs as unused
    g.reset_arcs();

    DynList<typename GT::Arc*> result;
    DynList<typename GT::Arc*> current_path;
    DynList<typename GT::Node*> stack;

    stack.insert(start);

    while (not stack.is_empty())
      {
        typename GT::Node * curr = stack.get_first();

        // Find an unused edge from curr
        typename GT::Arc * unused_arc = nullptr;
        for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc * arc = it.get_curr();
            if (not IS_ARC_VISITED(arc, Depth_First))
              {
                unused_arc = arc;
                break;
              }
          }

        if (unused_arc != nullptr)
          {
            // Mark arc as used
            ARC_BITS(unused_arc).set_bit(Depth_First, true);

            // Get the other endpoint
            typename GT::Node * next = g.get_connected_node(unused_arc, curr);

            stack.insert(next);
            current_path.insert(unused_arc);
          }
        else
          {
            // No unused edges from curr - backtrack
            stack.remove_first();
            if (not current_path.is_empty())
              result.insert(current_path.remove_first());
          }
      }

    // Each arc went to the front of result when its walk backtracked past
    // it, the last of the walk first, so that result is in order
    return result;
  }

  /**
   * @brief Hierholzer's algorithm for directed graphs.
   *
   * @param g The directed graph
   * @param start Starting vertex
   * @return List of arcs forming the Eulerian path/cycle
   */
  DynList<typename GT::Arc*> hierholzer_directed(GT & g, typename GT::Node * start)
  {
    // Mark all arcs as unused
    g.reset_arcs();

    DynList<typename GT::Arc*> result;
    DynList<typename GT::Arc*> current_path;
    DynList<typename GT::Node*> stack;

    stack.insert(start);

    while (not stack.is_empty())
      {
        typename GT::Node * curr = stack.get_first();

        // Find an unused outgoing edge from curr
        typename GT::Arc * unused_arc = nullptr;
        for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Arc * arc = it.get_curr();
            if (not IS_ARC_VISITED(arc, Depth_First))
              {
                unused_arc = arc;
                break;
              }
          }

        if (unused_arc != nullptr)
          {
            // Mark arc as used
            ARC_BITS(unused_arc).set_bit(Depth_First, true);

            // Get target node
            typename GT::Node * next = g.get_tgt_node(unused_arc);

            stack.insert(next);
            current_path.insert(unused_arc);
          }
        else
          {
            // No unused edges from curr - backtrack
            stack.remove_first();
            if (not current_path.is_empty())
              result.insert(current_path.remove_first());
          }
      }

    // Each arc went to the front of result when its walk backtracked past
    // it, the last of the walk first, so that result is in order
    return result;
  }

public:

  /**
   * @brief Construct an Eulerian path finder with optional filters.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Find_Eulerian_Path(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(std::move(__sn)), sa(std::move(__sa))
  {
    // empty 
  }

  /**
   * @brief Result type: path and its classification.
   */
  struct Result
  {
    DynList<typename GT::Arc*> path;  ///< The Eulerian path/cycle as arc list
    Eulerian_Type type;               ///< Classification (Cycle, Path, or None)
  };

  /**
   * @brief Find an Eulerian path or cycle in the graph.
   *
   * @param g The graph to search
   * @return Result containing the path (if exists) and its type. Without
   *         arcs, the type is Cycle and the path is empty.
   *
   * @par Example
   * @code
   * Find_Eulerian_Path<Graph> finder;
   * auto result = finder(g);
   *
   * if (result.type != Eulerian_Type::None)
   *   {
   *     cout << "Found " << (result.type == Eulerian_Type::Cycle ? "cycle" : "path")
   *          << " with " << result.path.size() << " edges\n";
   *   }
   * @endcode
   */
  Result operator () (GT & g)
  {
    Result result;

    // First check if Eulerian path/cycle exists
    Test_Eulerian<GT, SN, SA> tester(SN(sn.get()), SA(sa.get())); // copies
    result.type = tester.compute(g);

    if (result.type == Eulerian_Type::None)
      return result;

    // Find starting vertex. There is none only if no arc is shown: then
    // the empty walk is the Eulerian cycle, as Test_Eulerian says
    typename GT::Node * start = find_start_vertex(g, result.type);

    if (start == nullptr)
      return result;

    // Run Hierholzer's algorithm
    if (g.is_digraph())
      result.path = hierholzer_directed(g, start);
    else
      result.path = hierholzer_undirected(g, start);

    return result;
  }

  /**
   * @brief Get only the arc list (convenience method).
   *
   * @param g The graph to search
   * @return List of arcs in Eulerian order, empty if none exists
   */
  DynList<typename GT::Arc*> find_path(GT & g)
  {
    return (*this)(g).path;
  }

  /**
   * @brief Get the node sequence of the Eulerian path/cycle.
   *
   * @param g The graph
   * @return List of nodes in traversal order, empty if no path exists
   */
  DynList<typename GT::Node*> find_node_sequence(GT & g)
  {
    Result result = (*this)(g);
    DynList<typename GT::Node*> nodes;

    if (result.type == Eulerian_Type::None || result.path.is_empty())
      return nodes;

    // Get first node
    typename GT::Arc * first_arc = result.path.get_first();
    typename GT::Node * curr;

    if (g.is_digraph())
      curr = g.get_src_node(first_arc);
    else
      {
        // For undirected, need to find proper starting node
        typename GT::Node * start = find_start_vertex(g, result.type);
        curr = start;
      }

    nodes.append(curr);

    for (auto arc : result.path)
      {
        if (g.is_digraph())
          curr = g.get_tgt_node(arc);
        else
          curr = g.get_connected_node(arc, curr);
        nodes.append(curr);
      }

    return nodes;
  }
};

} // end namespace Aleph

# endif // EULERIAN_H
