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
 *  - Eulerian cycle: All vertices have even degree
 *  - Eulerian path: Exactly 0 or 2 vertices have odd degree
 *
 *  ### Directed Graphs
 *  - Eulerian cycle: in-degree = out-degree for all vertices
 *  - Eulerian path: At most one vertex with out-in=1, at most one with in-out=1
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | Test Eulerian | O(V + E) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build graph ...
 *
 *  Test_Eulerian<decltype(g)> test;
 *  if (test(g))
 *    std::cout << "Graph is Eulerian\n";
 *  ```
 *
 *  @see tpl_graph.H Graph data structures
 *  @see hamiltonian.H Hamiltonian path/cycle algorithms
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef EULERIAN_H
# define EULERIAN_H

# include <tpl_graph.H>

namespace Aleph
{

/**
 * @brief Tests whether a graph or digraph is Eulerian.
 *
 * A graph is Eulerian if it contains an Eulerian cycle (a cycle that
 * visits every edge exactly once). This class tests the necessary and
 * sufficient conditions for Eulerian graphs.
 *
 * ## Conditions Tested
 *
 * **Undirected graphs**: All vertices must have even degree.
 *
 * **Directed graphs**: For every vertex, in-degree must equal out-degree.
 *
 * @tparam GT Graph type (List_Graph, Array_Graph, etc.)
 * @tparam SN Node filter for iteration (default: show all nodes)
 * @tparam SA Arc filter for iteration (default: show all arcs)
 *
 * ## Complexity
 *
 * - Time: O(V + E) for undirected, O(V + E) for directed
 * - Space: O(1) additional
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
 * @endcode
 *
 * @see hamiltonian.H For Hamiltonian path/cycle testing
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
   * @brief Test if an undirected graph is Eulerian.
   *
   * Checks that all vertices have even degree.
   *
   * @param g The undirected graph to test
   * @return true if all vertices have even degree
   */
  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if ((g.get_num_arcs(it.get_curr()) % 2) == 1)
	return false;

    return true;
  }

  /**
   * @brief Test if a directed graph is Eulerian.
   *
   * Checks that for every vertex, in-degree equals out-degree.
   * Uses node counters to track in-degrees.
   *
   * @param g The directed graph to test
   * @return true if in-degree == out-degree for all vertices
   */
  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

    // First pass: count in-degrees using node counters
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    // Second pass: verify in-degree == out-degree for each vertex
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
	typename GT::Node * p = it.get_curr();
	if (g.get_num_arcs(p) != NODE_COUNTER(p))
	  return false;
      }

    return true;
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
   * @brief Test if a graph is Eulerian.
   *
   * Automatically detects whether the graph is directed or undirected
   * and applies the appropriate test.
   *
   * @param g The graph to test
   * @return true if the graph contains an Eulerian cycle
   *
   * @note The graph must be connected for an Eulerian cycle to exist.
   *       This function only checks the degree conditions.
   */
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }
};


} // end namespace Aleph

# endif // EULERIAN_H
