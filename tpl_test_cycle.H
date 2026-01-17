
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


/** @file tpl_test_cycle.H
 *  @brief Cycle detection in graphs.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TEST_CYCLE_H
# define TPL_TEST_CYCLE_H

# include <tpl_graph.H>
# include <ah-errors.H>

namespace Aleph {

/** @brief Test whether a cycle is reachable from a given node.
 *
 * The `Test_For_Cycle` class performs a depth-first search starting from a
 * specified source node to detect whether there exists any path that returns
 * to that source, forming a cycle.
 *
 * This is a *single-source* cycle detection algorithm. It only checks for
 * cycles reachable from the specified node. If the node is not part of any
 * cycle, the algorithm returns `false`, even if the graph contains cycles
 * elsewhere.
 *
 * ## Algorithm:
 *
 * The algorithm uses depth-first traversal with the following logic:
 * 1. Mark all outgoing arcs from the source node
 * 2. For each unmarked adjacent node, recursively explore its paths
 * 3. If any path returns to the source node, a cycle is detected
 * 4. Uses the `Test_Cycle` control bit to mark visited nodes and arcs
 *
 * ## Complexity:
 *
 * - **Time:** O(V + E) where V is the number of nodes and E is the number
 *   of arcs reachable from the source node.
 * - **Space:** O(V) for the recursion stack in the worst case.
 *
 * ## Usage example:
 *
 * @code
 * Graph g;
 * auto n1 = g.insert_node(1);
 * auto n2 = g.insert_node(2);
 * auto n3 = g.insert_node(3);
 * g.insert_arc(n1, n2);
 * g.insert_arc(n2, n3);
 *
 * Test_For_Cycle<Graph> cycle_test;
 * assert(!cycle_test(g, n1)); // No cycle yet
 *
 * g.insert_arc(n3, n1); // Create triangle
 * assert(cycle_test(g, n1));  // Cycle detected
 * @endcode
 *
 * @warning This algorithm modifies the `Test_Cycle` control bit on all
 *          visited nodes and arcs. If you need this bit for other purposes,
 *          save and restore it, or use a different algorithm.
 *
 * @note For undirected graphs, the algorithm correctly handles bidirectional
 *       edges by marking arcs as visited. This prevents false detection of
 *       trivial 2-cycles (e.g., going from A to B and back via the same edge).
 *
 * @tparam GT Graph type (must satisfy Aleph graph concept).
 * @tparam SA Arc filter type (default: `Dft_Show_Arc<GT>`).
 *
 * @see test_for_cycle() Free function alternative in tpl_graph_utils.H
 * @see has_cycle() To check if *any* cycle exists in the graph
 * @see is_graph_acyclique() To verify the entire graph is acyclic
 *
 * @ingroup Graphs
 */
template <class GT, class SA = Dft_Show_Arc<GT>> 
class Test_For_Cycle
{
  mutable typename GT::Node * src = nullptr;
  SA sa;

  // Recursive DFS helper to detect if a path from curr returns to src
  bool test_cycle(typename GT::Node * curr) const
  {
    // Base case: found a path back to source -> cycle detected
    if (src == curr) 
      return true;

    // Already visited this node in current DFS -> no cycle via this path
    if (IS_NODE_VISITED(curr, Test_Cycle)) 
      return false;

    // Mark current node as visited
    NODE_BITS(curr).set_bit(Test_Cycle, true);

    // Explore all outgoing arcs from current node
    for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_current_arc_ne();
        
        // Skip already-visited arcs to avoid infinite loops
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        // Mark arc as visited
        ARC_BITS(arc).set_bit(Test_Cycle, true);

        // Recursively check if target node leads to a cycle
        if (test_cycle(it.get_tgt_node()))
          return true;
      }

    // All paths from curr explored without finding src -> no cycle
    return false; 
  }  

  // Main algorithm entry point
  bool test_cycle(const GT & g, typename GT::Node * s) const
  {
    ah_invalid_argument_if(s == nullptr)
      << "Test_For_Cycle: source node cannot be null";

    src = s;

    // Reset control bits for a fresh traversal
    g.reset_bit_nodes(Test_Cycle);
    g.reset_bit_arcs(Test_Cycle);

    // Explore all arcs adjacent to source node
    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
      {
        typename GT::Arc * arc = it.get_current_arc_ne();
        
        // Skip already-visited arcs (shouldn't happen in first iteration)
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        // Mark arc as visited
        ARC_BITS(arc).set_bit(Test_Cycle, true);

        // Check if target node leads back to source
        if (test_cycle(it.get_tgt_node()))
          return true;
      }

    // All paths explored without finding a cycle
    return false;
  }

public:

  /** @brief Construct a cycle tester with optional arc filter.
   *
   * @param arc_filter Arc filter to control which arcs are traversed
   *        (default: show all arcs).
   */
  Test_For_Cycle(SA arc_filter = SA()) : sa(arc_filter) { /* empty */ }

  /** @brief Test whether a cycle is reachable from the given source node.
   *
   * Performs a depth-first search starting from `src_node` to detect whether
   * there exists any path that returns to `src_node`, forming a cycle.
   *
   * @param[in] g Graph to search (not modified, but control bits are used).
   * @param[in] src_node Source node to start the search from.
   *
   * @return `true` if a cycle reachable from `src_node` is found;
   *         `false` otherwise.
   *
   * @throw std::invalid_argument if `src_node` is `nullptr`.
   *
   * @note This algorithm only verifies the *existence* of a cycle reachable
   *       from `src_node`. It does not return the cycle itself or any
   *       information about its composition.
   *
   * @note The algorithm uses the `Test_Cycle` control bit on both nodes and
   *       arcs. These bits are reset at the beginning of the algorithm.
   *
   * @warning For large graphs, the recursion depth can be significant.
   *          Ensure adequate stack space is available.
   */
  bool operator () (const GT & g, typename GT::Node * src_node) const
  {
    return this->test_cycle(g, src_node);
  }
};

} // end namespace Aleph


# endif //  TPL_TEST_CYCLE_H
