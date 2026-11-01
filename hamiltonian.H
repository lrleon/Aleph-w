
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

# include <algorithm>
# include <limits>
# include <utility>

# include <ah-graph-concepts.H>

# include <tpl_dynMapTree.H>
# include <tpl_dynSetTree.H>
# include <tpl_graph.H>

namespace Aleph 
{

namespace hamiltonian_detail
{
  /** @brief The simple graph that the arcs that a filter shows give.

      For each node, the nodes that its arcs go to, without itself and
      each once (either way in a graph, out in a digraph), and in a
      digraph how many nodes come to each one: the degrees of the
      theorems of Ore and Dirac, which are on simple graphs. Loops and
      parallel arcs leave no trace.
   */
  template <class GT, class SA>
  struct Simple_Graph
  {
    using Node = typename GT::Node;

    DynMapTree<Node *, DynSetTree<Node *>> out;
    DynMapTree<Node *, size_t> in;

    Simple_Graph(const GT & g, SA & sa)
    {
      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
        {
          out.insert(it.get_curr(), DynSetTree<Node *>());
          in.insert(it.get_curr(), 0);
        }
      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
        {
          Node * p = it.get_curr();
          auto & to = out.find(p);
          for (Node_Arc_Iterator<GT, SA> a(p, sa); a.has_curr(); a.next_ne())
            if (Node * q = a.get_tgt_node_ne(); q != p and to.insert(q) != nullptr)
              ++in.find(q);
        }
    }

    /// The neighbors of p, or the nodes that it goes to in a digraph
    size_t degree(Node * p) const { return out.find(p).size(); }

    /// The nodes that come to p, in a digraph
    size_t in_degree(Node * p) const { return in.find(p); }

    /// Whether p and q are joined, or p goes to q in a digraph
    bool joined(Node * p, Node * q) const { return out.find(p).contains(q); }
  };
} // namespace hamiltonian_detail

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
template <AlephGraph GT, 
	  NodeFilter<GT> SN = Dft_Show_Node<GT>,
	  ArcFilter<GT> SA = Dft_Show_Arc<GT> >
class Test_Hamiltonian_Sufficiency
{
  Kept_Filter<SN> sn;
  Kept_Filter<SA> sa;

  /**
   * @brief Test Ore's condition for undirected graphs.
   *
   * Checks that for all pairs of NON-ADJACENT vertices u, v:
   * deg(u) + deg(v) ≥ n, in the simple graph of the arcs that the
   * filter shows
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

    const hamiltonian_detail::Simple_Graph<GT, SA> simple(g, sa.get());

    // Check all pairs of distinct vertices
    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
        typename GT::Node * src = i.get_curr();

        // Start j from the node after i to avoid duplicate pairs
        Node_Iterator<GT, SN> j(i);
        j.next_ne();

        for (; j.has_curr(); j.next_ne())
          {
            typename GT::Node * tgt = j.get_curr();

            // Only check NON-ADJACENT pairs (Ore's theorem condition)
            if (simple.joined(src, tgt))
              continue;

            // For non-adjacent pairs, check if deg(u) + deg(v) >= n
            if (simple.degree(src) + simple.degree(tgt) < n)
              return false;
          }
      }

    return true;
  }

  /**
   * @brief Test Ore-like condition for directed graphs.
   *
   * For digraphs, checks that for each pair (u,v) without arc u→v:
   * out-deg(u) + in-deg(v) ≥ n, in the simple digraph of the arcs that
   * the filter shows
   *
   * This is Woodall's theorem, the extension of Ore's to digraphs.
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

    const hamiltonian_detail::Simple_Graph<GT, SA> simple(g, sa.get());

    // Check all ordered pairs (src, tgt) where src != tgt
    for (Node_Iterator<GT, SN> i(g, sn); i.has_curr(); i.next_ne())
      {
        typename GT::Node * src = i.get_curr();
        for (Node_Iterator<GT, SN> j(g, sn); j.has_curr(); j.next_ne())
          {
            typename GT::Node * tgt = j.get_curr();
            if (src == tgt or simple.joined(src, tgt))
              continue;

            // A pair without an arc that does not satisfy the condition
            if (simple.degree(src) + simple.in_degree(tgt) < n)
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
    : sn(std::move(__sn)), sa(std::move(__sa))
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
template <AlephGraph GT, 
          NodeFilter<GT> SN = Dft_Show_Node<GT>,
          ArcFilter<GT> SA = Dft_Show_Arc<GT> >
class Test_Dirac_Condition
{
  Kept_Filter<SN> sn;
  Kept_Filter<SA> sa;

  /**
   * @brief Test Dirac's condition for undirected graphs.
   *
   * Checks that deg(v) ≥ n/2 for all vertices v, in the simple graph of
   * the arcs that the filter shows.
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
    const hamiltonian_detail::Simple_Graph<GT, SA> simple(g, sa.get());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      if (simple.degree(it.get_curr()) < min_degree)
        return false;

    return true;
  }

  /**
   * @brief Test Dirac-like condition for directed graphs.
   *
   * Checks that in-deg(v) ≥ n/2 and out-deg(v) ≥ n/2 for all v, in the
   * simple digraph of the arcs that the filter shows (Ghouila-Houri).
   *
   * @param g Directed graph to test
   * @return true if the condition is satisfied
   */
  bool test_digraph(GT & g)
  {
    assert(g.is_digraph());

    const size_t n = g.get_num_nodes();
    if (n < 3)
      return false;

    const size_t min_degree = (n + 1) / 2;
    const hamiltonian_detail::Simple_Graph<GT, SA> simple(g, sa.get());

    // Both must be at least n/2
    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        if (simple.degree(p) < min_degree or simple.in_degree(p) < min_degree)
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
    : sn(std::move(__sn)), sa(std::move(__sa))
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
   * Useful for understanding why a graph fails Dirac's condition: the
   * degree is the one that the condition takes, in the simple graph of
   * the arcs that the filter shows; in a digraph, the least of the
   * degrees in and out.
   *
   * @param g The graph
   * @return Pair of (minimum degree found, the vertex with that degree)
   */
  std::pair<size_t, typename GT::Node*> find_min_degree_vertex(GT & g)
  {
    size_t min_deg = std::numeric_limits<size_t>::max();
    typename GT::Node * min_node = nullptr;
    const hamiltonian_detail::Simple_Graph<GT, SA> simple(g, sa.get());

    for (Node_Iterator<GT, SN> it(g, sn); it.has_curr(); it.next_ne())
      {
        typename GT::Node * p = it.get_curr();
        size_t deg = simple.degree(p);
        if (g.is_digraph())
          deg = std::min(deg, simple.in_degree(p));
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
