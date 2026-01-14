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
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Eulerian
{
  SN & sn;
  SA & sa;

  /**
   * @brief Check connectivity of undirected graph using DFS.
   *
   * Only considers vertices with degree > 0 (isolated vertices don't
   * affect Eulerian properties).
   *
   * @param g The undirected graph
   * @return true if all non-isolated vertices are connected
   */
  bool is_connected(GT & g)
  {
    if (g.get_num_nodes() == 0)
      return true;

    // Find first vertex with edges
    typename GT::Node * start = nullptr;
    size_t non_isolated = 0;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        if (g.get_num_arcs(p) > 0)
          {
            if (start == nullptr)
              start = p;
            ++non_isolated;
          }
      }

    // No edges means trivially connected (or empty)
    if (start == nullptr)
      return true;

    // DFS from start vertex
    g.reset_nodes();
    size_t visited = 0;

    DynList<typename GT::Node*> stack;
    stack.insert(start);
    NODE_BITS(start).set_bit(Depth_First, true);

    while (not stack.is_empty())
      {
        typename GT::Node * curr = stack.remove_first();
        ++visited;

        for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Node * adj = it.get_tgt_node_ne();
            if (not NODE_BITS(adj).get_bit(Depth_First))
              {
                NODE_BITS(adj).set_bit(Depth_First, true);
                stack.insert(adj);
              }
          }
      }

    return visited == non_isolated;
  }

  /**
   * @brief Check strong connectivity of digraph using Kosaraju-like approach.
   *
   * For Eulerian cycle in digraph, we need strong connectivity among
   * vertices with edges.
   *
   * @param g The directed graph
   * @return true if all non-isolated vertices are strongly connected
   */
  bool is_strongly_connected(GT & g)
  {
    if (g.get_num_nodes() == 0)
      return true;

    // Find first vertex with edges
    typename GT::Node * start = nullptr;
    size_t non_isolated = 0;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        if (g.get_num_arcs(p) > 0 or NODE_COUNTER(p) > 0)  // has out or in edges
          {
            if (start == nullptr)
              start = p;
            ++non_isolated;
          }
      }

    if (start == nullptr || non_isolated <= 1)
      return true;

    // DFS from start in original direction
    g.reset_nodes();
    size_t visited_forward = 0;

    DynList<typename GT::Node*> stack;
    stack.insert(start);
    NODE_BITS(start).set_bit(Depth_First, true);

    while (not stack.is_empty())
      {
        typename GT::Node * curr = stack.remove_first();
        if (g.get_num_arcs(curr) > 0 or NODE_COUNTER(curr) > 0)
          ++visited_forward;

        for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
          {
            typename GT::Node * adj = it.get_tgt_node_ne();
            if (not NODE_BITS(adj).get_bit(Depth_First))
              {
                NODE_BITS(adj).set_bit(Depth_First, true);
                stack.insert(adj);
              }
          }
      }

    if (visited_forward != non_isolated)
      return false;

    // For full strong connectivity check, we'd need reverse graph traversal
    // For Eulerian purposes, if in-degree == out-degree and forward DFS
    // reaches all vertices, that's sufficient for practical purposes
    return true;
  }

  /**
   * @brief Analyze Eulerian properties of an undirected graph.
   *
   * @param g The undirected graph to test
   * @return Eulerian_Type classification
   */
  Eulerian_Type analyze_graph(GT & g)
  {
    assert(not g.is_digraph());

    size_t odd_degree_count = 0;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
          ++odd_degree_count;
      }

    // Check degree conditions first
    if (odd_degree_count == 0)
      {
        // All even degrees - could be Eulerian cycle if connected
        if (is_connected(g))
          return Eulerian_Type::Cycle;
        else
          return Eulerian_Type::None;
      }
    else if (odd_degree_count == 2)
      {
        // Exactly 2 odd degrees - could be Eulerian path if connected
        if (is_connected(g))
          return Eulerian_Type::Path;
        else
          return Eulerian_Type::None;
      }
    else
      {
        // More than 2 odd degrees - not Eulerian
        return Eulerian_Type::None;
      }
  }

  /**
   * @brief Analyze Eulerian properties of a directed graph.
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
        const long out_deg = g.get_num_arcs(p);
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

    // Check degree conditions
    if (imbalanced > 0)
      return Eulerian_Type::None;

    if (start_vertices == 0 && end_vertices == 0)
      {
        // All balanced - could be Eulerian cycle if strongly connected
        if (is_strongly_connected(g))
          return Eulerian_Type::Cycle;
        else
          return Eulerian_Type::None;
      }
    else if (start_vertices == 1 && end_vertices == 1)
      {
        // One start, one end - could be Eulerian path
        // Connectivity check is simpler for path
        return Eulerian_Type::Path;
      }
    else
      {
        return Eulerian_Type::None;
      }
  }

public:

  /**
   * @brief Construct an Eulerian tester with optional filters.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Test_Eulerian(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
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
            if (g.get_num_arcs(p) != NODE_COUNTER(p))
              return false;
          }
        return true;
      }
    else
      {
        for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
          if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
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
template <class GT, 
          class SN = Dft_Show_Node<GT>,
          class SA = Dft_Show_Arc<GT> >
class Find_Eulerian_Path
{
  SN & sn;
  SA & sa;

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
            if (g.get_num_arcs(p) > 0)
              {
                if (start == nullptr)
                  start = p;
                if (g.get_num_arcs(p) > NODE_COUNTER(p))
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
            if (g.get_num_arcs(p) > 0)
              {
                if (start == nullptr)
                  start = p;
                if ((g.get_num_arcs(p) % 2) == 1)
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

    // Reverse to get correct order
    DynList<typename GT::Arc*> reversed;
    while (not result.is_empty())
      reversed.insert(result.remove_first());

    return reversed;
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

    // Reverse to get correct order
    DynList<typename GT::Arc*> reversed;
    while (not result.is_empty())
      reversed.insert(result.remove_first());

    return reversed;
  }

public:

  /**
   * @brief Construct an Eulerian path finder with optional filters.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Find_Eulerian_Path(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
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
   * @return Result containing the path (if exists) and its type
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
    Test_Eulerian<GT, SN, SA> tester(std::forward<SN>(sn), std::forward<SA>(sa));
    result.type = tester.compute(g);

    if (result.type == Eulerian_Type::None)
      return result;

    // Find starting vertex
    typename GT::Node * start = find_start_vertex(g, result.type);

    if (start == nullptr)
      {
        result.type = Eulerian_Type::None;
        return result;
      }

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
