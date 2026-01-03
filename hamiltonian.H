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

/** @file hamiltonian.H
 *  @brief Hamiltonian sufficiency testing for graphs.
 *
 *  This file provides algorithms to test sufficient conditions for a
 *  graph to contain a Hamiltonian cycle. A Hamiltonian cycle visits
 *  every vertex exactly once and returns to the starting vertex.
 *
 *  ## Definitions
 *
 *  - **Hamiltonian Cycle**: A cycle visiting every vertex exactly once
 *  - **Hamiltonian Path**: A path visiting every vertex exactly once
 *  - **Hamiltonian Graph**: Contains a Hamiltonian cycle
 *
 *  ## Ore's Theorem (Sufficiency Condition)
 *
 *  For an undirected simple graph G with n ≥ 3 vertices:
 *  If for every pair of non-adjacent vertices u and v,
 *  deg(u) + deg(v) ≥ n, then G is Hamiltonian.
 *
 *  @note This tests sufficiency, not necessity. A graph may be
 *        Hamiltonian without satisfying Ore's condition.
 *
 *  ## Complexity
 *
 *  | Operation | Time |
 *  |-----------|------|
 *  | Sufficiency test (undirected) | O(V²) |
 *  | Sufficiency test (directed) | O(V² + E) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Graph<Node, Arc> g;
 *  // ... build complete graph K5 ...
 *
 *  Test_Hamiltonian_Sufficiency<decltype(g)> test;
 *  if (test(g))
 *    std::cout << "Graph satisfies Ore's condition (is Hamiltonian)\n";
 *  ```
 *
 *  @see eulerian.H Eulerian path/cycle detection
 *  @see tpl_find_path.H Path finding algorithms
 *
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef HAMILTONIAN_H
# define HAMILTONIAN_H

# include <tpl_graph.H>

namespace Aleph 
{

/**
 * @brief Tests Ore's sufficient condition for Hamiltonian graphs.
 *
 * This class tests whether a graph satisfies Ore's theorem, which provides
 * a sufficient (but not necessary) condition for a graph to be Hamiltonian.
 *
 * ## Ore's Theorem
 *
 * For an undirected graph G with n ≥ 3 vertices: if for every pair of
 * non-adjacent vertices u and v, deg(u) + deg(v) ≥ n, then G has a
 * Hamiltonian cycle.
 *
 * For directed graphs, the condition is extended to in-degree + out-degree.
 *
 * @tparam GT Graph type (List_Graph, Array_Graph, etc.)
 * @tparam SN Node filter for iteration (default: show all nodes)
 * @tparam SA Arc filter for iteration (default: show all arcs)
 *
 * ## Complexity
 *
 * - Undirected: O(V²) - checks all pairs of non-adjacent vertices
 * - Directed: O(V² + E) - also counts in-degrees
 *
 * @par Example
 * @code
 * // Complete graph K5 satisfies Ore's condition
 * List_Graph<Node, Arc> k5;
 * // ... build K5 ...
 *
 * Test_Hamiltonian_Sufficiency<decltype(k5)> test;
 * assert(test(k5) == true);  // All deg sums ≥ n for non-adjacent
 * @endcode
 *
 * @warning A graph may be Hamiltonian without satisfying this condition.
 *          This is a sufficiency test only.
 *
 * @ingroup Grafos
 * @author Leandro R. León
 */
template <class GT, 
	  class SN = Dft_Show_Node<GT>,
	  class SA = Dft_Show_Arc<GT> >
class Test_Hamiltonian_Sufficiency
{
  SN & sn;
  SA & sa;

  /**
   * @brief Test Ore's condition for undirected graphs.
   *
   * Checks that for all pairs of vertices u, v:
   * deg(u) + deg(v) ≥ n
   *
   * @param g Undirected graph to test
   * @return true if Ore's condition is satisfied
   */
  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
	typename GT::Node * src = i.get_curr();
	const size_t & nsrc = g.get_num_arcs(src);

	i.next_ne();

	for (Node_Iterator<GT, SN> j(i); j.has_curr(); j.next_ne())
	  if (nsrc + g.get_num_arcs(j.get_curr()) < n)
	    return false;
      }

    return true;
  }

  /**
   * @brief Test Ore's condition for directed graphs.
   *
   * For digraphs, checks that for each pair (u,v) without arc u→v:
   * out-deg(u) + in-deg(v) ≥ n
   *
   * @param g Directed graph to test
   * @return true if the digraph version of Ore's condition is satisfied
   */
  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    g.reset_counter_nodes();

    // First pass: count in-degrees using node counters
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    const size_t & n = g.get_num_nodes();

    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
	typename GT::Node * src = i.get_curr();

	for (Node_Iterator<GT, SN> j(g, sn); j.has_curr(); j.next_ne())
	  {
	    typename GT::Node * tgt = j.get_curr();

	    if (src == tgt)
	      continue;

	    if (g.get_num_arcs(src) + NODE_COUNTER(tgt) >= n)
	      continue;

	    // Check if arc src→tgt exists
	    bool terminate = true;
	    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); 
		 it.next_ne())
	      if (src == it.get_tgt_node_ne())
		{
		  terminate = false;
		  break;
		}
	    
	    if (terminate)
	      return false;
	  }
      } 

    return true;
  }

public:

  /**
   * @brief Construct a Hamiltonian sufficiency tester.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Test_Hamiltonian_Sufficiency(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

  /**
   * @brief Test if a graph satisfies Ore's Hamiltonian condition.
   *
   * Automatically detects directed vs undirected and applies the
   * appropriate test.
   *
   * @param g Graph to test
   * @return true if Ore's sufficient condition is satisfied
   *
   * @note Returns true only guarantees the graph IS Hamiltonian.
   *       Returns false does NOT mean it isn't Hamiltonian.
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

# endif // HAMILTONIAN_H
