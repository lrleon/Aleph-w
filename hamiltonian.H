
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 *  @ingroup Graphs
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
 * @ingroup Graphs
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
   * @brief Check if two nodes are adjacent in an undirected graph.
   *
   * @param[in] g The graph
   * @param[in] src First node
   * @param[in] tgt Second node
   * @return true if there is an edge between src and tgt
   */
	  bool are_adjacent(GT & g, typename GT::Node * src, typename GT::Node * tgt)
	  {
	    (void) g;

	    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
	      if (it.get_tgt_node_ne() == tgt)
	        return true;
	    return false;
	  }

  /**
   * @brief Test Ore's condition for undirected graphs.
   *
   * Checks that for all pairs of NON-ADJACENT vertices u, v:
   * deg(u) + deg(v) ≥ n
   *
   * @param g Undirected graph to test
   * @return true if Ore's condition is satisfied
   */
  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    const size_t n = g.get_num_nodes();

    // Ore's theorem requires n >= 3
    if (n < 3)
      return false;

    // Check all pairs of distinct vertices
    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
        typename GT::Node * src = i.get_curr();
        const size_t nsrc = g.get_num_arcs(src);

        // Start j from the node after i to avoid duplicate pairs
        Node_Iterator<GT, SN> j(i);
        j.next_ne();

        for (; j.has_curr(); j.next_ne())
          {
            typename GT::Node * tgt = j.get_curr();

            // Only check NON-ADJACENT pairs (Ore's theorem condition)
            if (are_adjacent(g, src, tgt))
              continue;

            // For non-adjacent pairs, check if deg(u) + deg(v) >= n
            if (nsrc + g.get_num_arcs(tgt) < n)
              return false;
          }
      }

    return true;
  }

  /**
   * @brief Check if arc src→tgt exists in a digraph.
   *
   * @param src Source node
   * @param tgt Target node
   * @return true if arc src→tgt exists
   */
  bool has_arc(typename GT::Node * src, typename GT::Node * tgt)
  {
    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
      if (it.get_tgt_node_ne() == tgt)
        return true;
    return false;
  }

  /**
   * @brief Test Ore-like condition for directed graphs.
   *
   * For digraphs, checks that for each pair (u,v) without arc u→v:
   * out-deg(u) + in-deg(v) ≥ n
   *
   * This is based on Ghouila-Houri's theorem extension for digraphs.
   *
   * @param g Directed graph to test
   * @return true if the digraph version of Ore's condition is satisfied
   */
  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    const size_t n = g.get_num_nodes();

    // Requires n >= 3
    if (n < 3)
      return false;

    g.reset_counter_nodes();

    // First pass: count in-degrees using node counters
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    // Check all ordered pairs (src, tgt) where src != tgt
    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
        typename GT::Node * src = i.get_curr();
        const size_t out_deg_src = g.get_num_arcs(src);

        for (Node_Iterator<GT, SN> j(g, sn); j.has_curr(); j.next_ne())
          {
            typename GT::Node * tgt = j.get_curr();

            if (src == tgt)
              continue;

            // If condition is satisfied, skip
            if (out_deg_src + NODE_COUNTER(tgt) >= n)
              continue;

            // Condition not satisfied; check if arc src→tgt exists
            // If arc exists, this pair doesn't need to satisfy the condition
            if (has_arc(src, tgt))
              continue;

            // Non-adjacent pair that doesn't satisfy the condition
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

/**
 * @brief Tests Dirac's sufficient condition for Hamiltonian graphs.
 *
 * Dirac's theorem provides a simpler sufficient condition than Ore's:
 * For an undirected simple graph G with n ≥ 3 vertices,
 * if deg(v) ≥ n/2 for every vertex v, then G is Hamiltonian.
 *
 * ## Comparison with Ore's Theorem
 *
 * | Property | Dirac | Ore |
 * |----------|-------|-----|
 * | Complexity | O(V) | O(V²) |
 * | Condition | deg(v) ≥ n/2 for all v | deg(u)+deg(v) ≥ n for non-adjacent |
 * | Strictness | More restrictive | Less restrictive |
 *
 * @note Dirac's condition implies Ore's condition, but not vice versa.
 *       A graph satisfying Dirac's condition also satisfies Ore's.
 *
 * @tparam GT Graph type (List_Graph, Array_Graph, etc.)
 * @tparam SN Node filter for iteration (default: show all nodes)
 * @tparam SA Arc filter for iteration (default: show all arcs)
 *
 * @par Example
 * @code
 * List_Graph<Node, Arc> g;
 * // Build complete graph K5 (each vertex has degree 4 ≥ 5/2)
 * // ...
 *
 * Test_Dirac_Condition<decltype(g)> test;
 * if (test(g))
 *   std::cout << "Graph satisfies Dirac's condition (is Hamiltonian)\n";
 * @endcode
 *
 * @warning This tests sufficiency, not necessity. A graph may be
 *          Hamiltonian without satisfying Dirac's condition.
 *
 * @ingroup Graphs
 * @author Leandro R. León
 */
template <class GT, 
          class SN = Dft_Show_Node<GT>,
          class SA = Dft_Show_Arc<GT> >
class Test_Dirac_Condition
{
  SN & sn;
  SA & sa;

  /**
   * @brief Test Dirac's condition for undirected graphs.
   *
   * Checks that deg(v) ≥ n/2 for all vertices v.
   *
   * @param g Undirected graph to test
   * @return true if Dirac's condition is satisfied
   */
  bool test_graph(GT & g)
  {
    assert(not g.is_digraph());

    const size_t n = g.get_num_nodes();

    // Dirac's theorem requires n >= 3
    if (n < 3)
      return false;

    const size_t min_degree = (n + 1) / 2;  // Ceiling of n/2

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        if (g.get_num_arcs(it.get_curr()) < min_degree)
          return false;
      }

    return true;
  }

  /**
   * @brief Test Dirac-like condition for directed graphs.
   *
   * For digraphs, checks that min(out-deg, in-deg) ≥ n/2 for all vertices.
   * This is a generalization of Dirac's theorem for directed graphs.
   *
   * @param g Directed graph to test
   * @return true if the digraph version of Dirac's condition is satisfied
   */
  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    const size_t n = g.get_num_nodes();

    if (n < 3)
      return false;

    const size_t min_degree = (n + 1) / 2;

    g.reset_counter_nodes();

    // First pass: count in-degrees
    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_tgt_node_ne())++;

    // Second pass: check both in-degree and out-degree
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        const size_t out_deg = g.get_num_arcs(p);
        const size_t in_deg = NODE_COUNTER(p);

        // Both must be at least n/2
        if (out_deg < min_degree || in_deg < min_degree)
          return false;
      }

    return true;
  }

public:

  /**
   * @brief Construct a Dirac condition tester.
   *
   * @param __sn Node filter (default: show all nodes)
   * @param __sa Arc filter (default: show all arcs)
   */
  Test_Dirac_Condition(SN && __sn = SN(), SA && __sa = SA()) 
    : sn(__sn), sa(__sa)
  {
    // empty 
  }

  /**
   * @brief Test if a graph satisfies Dirac's Hamiltonian condition.
   *
   * @param g Graph to test
   * @return true if deg(v) ≥ n/2 for all vertices
   *
   * @note This is faster than Ore's test: O(V) vs O(V²)
   */
  bool operator () (GT & g)
  {
    if (g.is_digraph())
      return test_digraph(g);
    else
      return test_graph(g);
  }

  /**
   * @brief Get the minimum degree required by Dirac's condition.
   *
   * @param g The graph
   * @return The minimum degree required (⌈n/2⌉)
   */
  size_t min_required_degree(GT & g) const
  {
    return (g.get_num_nodes() + 1) / 2;
  }

  /**
   * @brief Find the vertex with minimum degree.
   *
   * Useful for understanding why a graph fails Dirac's condition.
   *
   * @param g The graph
   * @return Pair of (minimum degree found, the vertex with that degree)
   */
  std::pair<size_t, typename GT::Node*> find_min_degree_vertex(GT & g)
  {
    size_t min_deg = std::numeric_limits<size_t>::max();
    typename GT::Node * min_node = nullptr;

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        size_t deg = g.get_num_arcs(p);
        if (deg < min_deg)
          {
            min_deg = deg;
            min_node = p;
          }
      }

    return {min_deg, min_node};
  }
};

} // end namespace Aleph

# endif // HAMILTONIAN_H
