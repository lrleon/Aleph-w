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

/** @file tpl_graph_utils.H
 *  @brief Utility algorithms and operations for graphs.
 *
 *  This file provides a comprehensive collection of graph algorithms including
 *  traversals (BFS, DFS), path finding, connectivity tests, spanning trees,
 *  graph copying, and various utility functions for working with graphs.
 *
 *  @ingroup Graphs
 */

# ifndef TPL_GRAPH_UTILS_H
# define TPL_GRAPH_UTILS_H


# include <cassert>
# include <cstddef>
# include <limits>
# include <memory>
# include <tuple>
# include <utility>
# include <vector>
# include <tpl_agraph.H>
# include <tpl_dynListQueue.H>
# include <ah-errors.H>

using namespace Aleph;

namespace Aleph {

  /** @brief Internal recursive DFS used by depth_first_traversal().
   *
   * @tparam GT Graph type.
   * @param[in] g Graph being traversed.
   * @param[in] node Current node.
   * @param[in] arc Arc used to discover `node` (nullptr for the root call).
   * @param[in] visit Optional callback invoked when a node is discovered.
   * @param[in,out] count Number of discovered nodes so far.
   * @return `true` if the traversal must stop early; `false` otherwise.
   *
   * @note Uses the `Depth_First` control bit on both nodes and arcs.
   */
  template <class GT> inline static bool
  __depth_first_traversal(const GT & g, typename GT::Node * node,
                          typename GT::Arc *  arc,
                          bool (*visit)(const GT & g, typename GT::Node *,
                                        typename GT::Arc *),
                          size_t & count);

  /** @brief Depth-first traversal starting from a given node.
   *
   * Recursively traverses the connected component reachable from `start_node`.
   *
   * The callback must have the following signature:
   * `bool visit(const GT& g, typename GT::Node* curr, typename GT::Arc* from)`.
   *
   * `from` is the arc that discovered `curr` (nullptr for the start node).
   * If `visit` returns `true`, the traversal stops early. If `visit` is
   * `nullptr`, no callback is executed.
   *
   * @tparam GT Graph type.
   * @param[in] g Graph to traverse.
   * @param[in] start_node Starting node (must be non-null and belong to `g`).
   * @param[in] visit Optional visit callback (may be nullptr).
   * @return Number of discovered nodes.
   *
   * @note Resets the `Depth_First` control bit on all nodes and arcs.
   * @note Uses the `Depth_First` control bit on both nodes and arcs.
   *
   * @ingroup Grafos
   * @see Depth_First_Traversal, breadth_first_traversal, test_connectivity
   */
  template <class GT> inline size_t 
  depth_first_traversal(const GT & g, typename GT::Node * start_node,
                        bool (*visit)(const GT & g, typename GT::Node *, 
                                      typename GT::Arc *))
  {
    g.reset_bit_nodes(Depth_First);
    g.reset_bit_arcs(Depth_First);
    size_t counter = 0;

    __depth_first_traversal(g, start_node, nullptr, visit, counter);

    return counter;
  }

  /// \overload depth_first_traversal(const GT&, typename GT::Node*, bool (*)(...))
  template <class GT> inline 
  size_t depth_first_traversal(const GT & g, 
                               bool (*visit)(const GT &, typename GT::Node *, 
                                             typename GT::Arc *))
  {
    return depth_first_traversal(g, g.get_first_node(), visit);
  }

  /** @brief Default visit operation for traversals (never stops).
   *
   * @ingroup Grafos
   */
  template <class GT>
  struct Default_Visit_Op
  {
    /// Always continues the traversal (never stops early).
    bool operator () (const GT &, typename GT::Node *, typename GT::Arc *)
    {
      return false;
    }
  };

  /** @brief Stateful depth-first traversal functor.
   *
   * Traverses a graph in depth-first order, invoking an operation for each
   * discovered node.
   *
   * Template parameters:
   * - `GT`: graph type
   * - `Operation`: visit functor
   * - `SA`: arc filter used by the internal iterator
   *
   * `Operation` must provide:
   * `bool operator()(const GT& g, typename GT::Node* node, typename GT::Arc* from)`.
   *
   * If the operation returns `true`, traversal stops early.
   *
   * @note Uses the `Depth_First` control bit on both nodes and arcs.
   *
   * @ingroup Grafos
   * @see depth_first_traversal, Breadth_First_Traversal
   */
  template <class GT, 
            class Operation = Default_Visit_Op<GT>, 
            class SA        = Dft_Show_Arc<GT>> 
  class Depth_First_Traversal
  {
    Operation * op_ptr = nullptr;
    SA          sa;
    size_t      count = 0;
    const GT *  g_ptr = nullptr;

  private:

    bool __dft(typename GT::Node * node, typename GT::Arc * arc = nullptr)
    {
      if (IS_NODE_VISITED(node, Depth_First)) 
        return false; 

      NODE_BITS(node).set_bit(Depth_First, true);
      count++;

      if ((*op_ptr)(*g_ptr, node, arc)) 
        return true;

      if (count == g_ptr->get_num_nodes())
        return true;

      // Recursively traverse arcs incident to `node`.
      for (Node_Arc_Iterator<GT, SA> it(node, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          if (IS_ARC_VISITED(arc, Depth_First)) 
            continue;

          ARC_BITS(arc).set_bit(Depth_First, true);
          if (__dft (it.get_tgt_node_ne(), arc))
            return true;
        }

      return false;
    }

    size_t dft(const GT & g, typename GT::Node * start_node, Operation & __op)
    {
      op_ptr = &__op;
      g_ptr  = &g;
    
      g_ptr->reset_bit_nodes(Depth_First);
      g_ptr->reset_bit_arcs(Depth_First);

      count = 0;

      __dft(start_node);

      return count;
    }

  public:

    /// Construct a traversal functor using the arc filter `__sa`.
    Depth_First_Traversal(SA __sa = SA()) : sa(__sa) { /* empty */ }

    /** @brief Traverse starting from the first node of the graph.
     *
     * Traverses the connected component reachable from `g.get_first_node()`
     * in depth-first order.
     *
     * Side effects:
     * - Resets the `Depth_First` control bit on all nodes and arcs.
     * - Uses the `Depth_First` control bit on both nodes and arcs.
     *
     * @param[in] g Graph to traverse.
     * @param[in] op Visit operation.
     * @return Number of discovered nodes.
     * @throw bad_alloc If there is not enough memory.
     */
    size_t operator () (const GT & g, Operation op = Operation()) 
    {
      return dft(g, g.get_first_node(), op);
    }
 
    /** @brief Traverse starting from a given node.
     *
     * Traverses the connected component reachable from `sn` in depth-first
     * order.
     *
     * Side effects:
     * - Resets the `Depth_First` control bit on all nodes and arcs.
     * - Uses the `Depth_First` control bit on both nodes and arcs.
     *
     * @param[in] g Graph to traverse.
     * @param[in] sn Starting node (must be non-null and belong to `g`).
     * @param[in] op Visit operation.
     * @return Number of discovered nodes.
     * @throw bad_alloc If there is not enough memory.
     */
    size_t operator () (const GT & g, typename GT::Node * sn,
                        Operation op = Operation())
    {
      return dft(g, sn, op);
    }
  };


  template <class GT> inline static bool
  __depth_first_traversal(const GT & g, typename GT::Node * node, 
                          typename GT::Arc *  arc,
                          bool (*visit)(const GT & g, typename GT::Node *, 
                                        typename GT::Arc *),
                          size_t & count)
  {
    if (IS_NODE_VISITED(node, Depth_First)) 
      return false; 

    NODE_BITS(node).set_bit(Depth_First, true); // mark node visited
    count++;

    if (visit != nullptr) // invoke callback if provided
      if ((*visit)(g, node, arc)) 
        return true;

    if (count == g.get_num_nodes()) // all nodes discovered?
      return true;

    for (auto it = g.get_arc_it(node); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Depth_First)) 
          continue;

        ARC_BITS(arc).set_bit(Depth_First, true); // mark arc visited
        if (__depth_first_traversal(g, it.get_tgt_node_ne(), arc, visit, count))
          return true;
      }

    return false; // keep exploring
  } 


  /** @brief Breadth-first traversal starting from a given node.
   *
   * Traverses the connected component reachable from `start` in
   * breadth-first order.
   *
   * The callback must have the following signature:
   * `bool visit(const GT& g, typename GT::Node* curr, typename GT::Arc* from)`.
   *
   * `from` is the arc that discovered `curr` (nullptr for the start node).
   * If `visit` returns `true`, the traversal stops early. If `visit` is
   * `nullptr`, no callback is executed.
   *
   * @tparam GT Graph type.
   * @param[in] g Graph to traverse.
   * @param[in] start Starting node (must be non-null and belong to `g`).
   * @param[in] visit Optional visit callback (may be nullptr).
   * @return Number of discovered nodes.
   *
   * @note Resets the `Breadth_First` control bit on all nodes and arcs.
   * @note Uses the `Breadth_First` control bit on both nodes and arcs.
   *
   * @ingroup Grafos
   * @see breadth_first_traversal(const GT&, bool (*)(...)),
   *      depth_first_traversal, Breadth_First_Traversal
   */
  template <class GT> inline size_t
  breadth_first_traversal(const GT & g, typename GT::Node * start,
                          bool (*visit)(const GT &, typename GT::Node *,
                                        typename GT::Arc *) )
  {
    g.reset_bit_nodes(Breadth_First);
    g.reset_bit_arcs(Breadth_First);
    DynListQueue<typename GT::Arc*> q;

    for (auto it = g.get_arc_it(start); it.has_curr(); it.next_ne())
      q.put(it.get_current_arc_ne());

    NODE_BITS(start).set_bit(Breadth_First, true);
    size_t node_counter = 1;

    if (visit != nullptr)
      if ((*visit)(g, start, nullptr))
        return 1;

    while (not q.is_empty() and node_counter < g.get_num_nodes())
      {
        auto arc = q.get();
        ARC_BITS(arc).set_bit(Breadth_First, true);

        auto src = g.get_src_node(arc);
        auto tgt = g.get_tgt_node(arc);
        if (IS_NODE_VISITED(src, Breadth_First) and
            IS_NODE_VISITED(tgt, Breadth_First))
          continue;

        auto visit_node = IS_NODE_VISITED(src, Breadth_First) ? tgt : src;
        if (visit != nullptr)
          if ((*visit)(g, visit_node, arc))
            break;

        NODE_BITS(visit_node).set_bit(Breadth_First, true); 
        node_counter++;

        for (auto it = g.get_arc_it(visit_node); it.has_curr(); it.next_ne())
          {
            auto curr_arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(curr_arc, Breadth_First)) 
              continue; 

            if (IS_NODE_VISITED(g.get_src_node(curr_arc), Breadth_First) and 
                IS_NODE_VISITED(g.get_tgt_node(curr_arc), Breadth_First))
              continue; // both endpoints already visited
	  
            q.put(curr_arc);
          }
      }

    return node_counter;
  }

  /// \overload breadth_first_traversal(const GT&, typename GT::Node*, bool (*)(...))
  template <class GT> inline size_t
  breadth_first_traversal(GT & g, 
                          bool (*visit)(const GT &, typename GT::Node *, 
                                        typename GT::Arc *))
  {
    return breadth_first_traversal(g, g.get_first_node(), visit);
  }

      
  /** @brief Stateful breadth-first traversal functor.
   *
   * Traverses a graph in breadth-first order, invoking an operation for each
   * discovered node.
   *
   * Template parameters:
   * - `GT`: graph type
   * - `Operation`: visit functor
   * - `SA`: arc filter used by the internal iterator
   *
   * `Operation` must provide:
   * `bool operator()(const GT& g, typename GT::Node* node, typename GT::Arc* from)`.
   *
   * If the operation returns `true`, traversal stops early.
   *
   * @note Resets the `Breadth_First` control bit on all nodes and arcs.
   * @note Uses the `Breadth_First` control bit on both nodes and arcs.
   *
   * @ingroup Grafos
   * @see breadth_first_traversal, Depth_First_Traversal
   */
  template <class GT, 
            class Operation = Default_Visit_Op<GT>, 
            class SA        = Dft_Show_Arc<GT>> 
  class Breadth_First_Traversal
  {
    SA     sa;
    size_t count = 0;

    size_t bft(const GT & g, typename GT::Node * start, Operation & op)
    {
      g.reset_bit_nodes(Breadth_First); 
      g.reset_bit_arcs(Breadth_First);  
      DynListQueue<typename GT::Arc*> q; // pending arcs queue

      for (Node_Arc_Iterator<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        q.put(it.get_current_arc_ne());

      NODE_BITS(start).set_bit(Breadth_First, true); 
      count = 1;

      if (op (g, start, nullptr))
        return 1;

      while (not q.is_empty() and count < g.get_num_nodes()) 
        {
          auto arc = q.get();
          ARC_BITS(arc).set_bit(Breadth_First, true); 

          auto src = g.get_src_node(arc); 
          auto tgt = g.get_tgt_node(arc);

          if (IS_NODE_VISITED(src, Breadth_First) and
              IS_NODE_VISITED(tgt, Breadth_First))
            continue;

          auto curr = IS_NODE_VISITED(src, Breadth_First) ? tgt : src;
          if (op (g, curr, arc))
            break;

          NODE_BITS(curr).set_bit(Breadth_First, true); 
          count++;

          for (Node_Arc_Iterator<GT, SA> it(curr, sa); it.has_curr(); it.next_ne())
            {
              auto curr_arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(curr_arc, Breadth_First)) 
                continue; 

              if (IS_NODE_VISITED(g.get_src_node(curr_arc), Breadth_First) and 
                  IS_NODE_VISITED(g.get_tgt_node(curr_arc), Breadth_First))
                continue;

              q.put(curr_arc);
            }
        }

      return count;
    }  

  public:

    /// Construct a traversal functor using the arc filter `__sa`.
    Breadth_First_Traversal(SA __sa = SA()) : sa(__sa) { /* empty */ }

    /** @brief Traverse starting from the first node of the graph.
     *
     * Traverses the connected component reachable from `g.get_first_node()`
     * in breadth-first order.
     *
     * Side effects:
     * - Resets the `Breadth_First` control bit on all nodes and arcs.
     * - Uses the `Breadth_First` control bit on both nodes and arcs.
     *
     * @param[in] g Graph to traverse.
     * @param[in] op Visit operation.
     * @return Number of discovered nodes.
     * @throw bad_alloc If there is not enough memory.
     */
    size_t operator () (const GT & g, Operation op)
    {
      return bft (g, g.get_first_node(), op);
    }

    /** @brief Traverse starting from a given node.
     *
     * Traverses the connected component reachable from `p` in breadth-first
     * order.
     *
     * Side effects:
     * - Resets the `Breadth_First` control bit on all nodes and arcs.
     * - Uses the `Breadth_First` control bit on both nodes and arcs.
     *
     * @param[in] g Graph to traverse.
     * @param[in] p Starting node (must be non-null and belong to `g`).
     * @param[in] op Visit operation.
     * @return Number of discovered nodes.
     * @throw bad_alloc If there is not enough memory.
     */
    size_t operator () (const GT & g, typename GT::Node * p, 
                        Operation && op = Operation())
    {
      return bft(g, p, op);
    }

    /** @brief Traverse starting from a given node (operation by reference).
     *
     * Equivalent to the rvalue overload, but takes the operation by reference.
     *
     * @param[in] g Graph to traverse.
     * @param[in] p Starting node (must be non-null and belong to `g`).
     * @param[in] op Visit operation.
     * @return Number of discovered nodes.
     * @throw bad_alloc If there is not enough memory.
     */
    size_t operator () (const GT & g, typename GT::Node * p, Operation & op)
    {
      return bft(g, p, op);
    }
  };

  /** @brief Breadth-first search of a (shortest-by-edges) path between two nodes.
   *
   * Searches a path from `start` to `end` using breadth-first search (BFS).
   * If a path exists, the returned path has the minimum number of arcs
   * (ties depend on iteration order).
   *
   * Side effects:
   * - Resets all node and arc control attributes via `g.reset_nodes()` and
   *   `g.reset_arcs()`.
   * - Uses the `Find_Path` control bit on nodes and arcs.
   * - Stores predecessor pointers in node cookies while searching.
   *
   * @tparam GT Graph type.
   * @param[in] g Graph to search.
   * @param[in] start Start node (must be non-null and belong to `g`).
   * @param[in] end End node (must be non-null and belong to `g`).
   * @return A `Path<GT>` describing the found path; an empty path if no path exists.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see find_path_depth_first(), test_for_path()
   */
  template <class GT> inline 
  Path<GT> find_path_breadth_first(const GT & g, typename GT::Node * start, 
                                   typename GT::Node * end)
  {
    ah_invalid_argument_if(start == nullptr or end == nullptr)
      << "find_path_breadth_first(): start and end must be non-null";

    if (start == end)
      return Path<GT>(g, start);

    g.reset_nodes();
    g.reset_arcs(); 
  
    DynListQueue<typename GT::Arc*> q; 

    for (auto it = g.get_arc_it(start); it.has_curr(); it.next_ne())
      q.put(it.get_current_arc_ne());
        
    NODE_BITS(start).set_bit(Find_Path, true);

    bool path_found = false;

    while (not q.is_empty())
      {
        auto arc = q.get(); 
        auto src = g.get_src_node(arc); 
        auto tgt = g.get_tgt_node(arc);

        if (IS_NODE_VISITED(src, Find_Path) and IS_NODE_VISITED(tgt, Find_Path))
          continue;
      
        if (IS_NODE_VISITED(tgt, Find_Path))
          std::swap(src, tgt);

        ARC_BITS(arc).set_bit(Find_Path, true); 
        NODE_BITS(tgt).set_bit(Find_Path, true);
        NODE_COOKIE(tgt) = src;
        
        if (tgt == end)
          {
            path_found = true;
            break;
          }

        for (auto it = g.get_arc_it(tgt); it.has_curr(); it.next_ne())
          {
            auto a = it.get_current_arc_ne();
            if (IS_ARC_VISITED(a, Find_Path))
              continue;

            if (IS_NODE_VISITED(g.get_src_node(a), Find_Path) and 
                IS_NODE_VISITED(g.get_tgt_node(a), Find_Path))
              continue;

            q.put(a);
          }
      }
  
    if (not path_found)
      return Path<GT>(g);

    q.empty(); // free queue memory for eventually saving the required for path
  
    Path<GT> path(g, end);
    auto p = end;
    while (p != start)
      {
        p = (typename GT::Node *) NODE_COOKIE(p);
        path.insert(p);
      }

    return path;
  }


  /** @brief Connectivity test for undirected graphs.
   *
   * @param[in] g Graph to test.
   * @return `true` if `g` is connected; `false` otherwise.
   *
   * @note Returns `false` for an empty graph.
   * @note This function is not intended for multigraphs.
   * @throw domain_error If `g` is a directed graph.
   *
   * @ingroup Grafos
   * @see depth_first_traversal()
   */
  template <class GT> inline 
  bool test_connectivity(const GT & g)
  {
    ah_domain_error_if(g.is_digraph())
      << "test_connectivity() does not work on digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return false;

    if (g.get_num_arcs() + 1 < num_nodes)
      return false; 

    return depth_first_traversal<GT>(g, nullptr) == num_nodes;
  }


  /** @brief Internal helper used by test_for_cycle(). */
  template <class GT> inline static
  bool __test_cycle(const GT & g, typename GT::Node *, typename GT::Node *);


  /** @brief Search for a cycle reachable from a given node.
   *
   * Performs a depth-first exploration starting at `src` and returns `true` if
   * a cycle that returns to `src` is found.
   *
   * @note This is a *single-source* cycle check. If you only need to know
   * whether the graph has any cycle at all, prefer has_cycle().
   * @note Uses the `Test_Cycle` control bit on both nodes and arcs.
   *
   * @param[in] g Graph to search.
   * @param[in] src Source node (must be non-null and belong to `g`).
   * @return `true` if a cycle returning to `src` is found; `false` otherwise.
   *
   * @ingroup Grafos
   * @see has_cycle(), is_graph_acyclique()
   */
  template <class GT> inline 
  bool test_for_cycle(const GT & g, typename GT::Node * src)
  {
    ah_invalid_argument_if(src == nullptr)
      << "test_for_cycle(): src must be non-null";

    g.reset_bit_nodes(Test_Cycle);
    g.reset_bit_arcs(Test_Cycle);
    for (auto it = g.get_arc_it(src); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true);
        if (__test_cycle(g, src, it.get_tgt_node_ne()))
          return true;
      }

    return false;
  }


  template <class GT> inline static bool
  __test_cycle(const GT & g, typename GT::Node * src, typename GT::Node * curr)
  {
    if (src == curr) 
      return true; // detected cycle

    if (IS_NODE_VISITED(curr, Test_Cycle)) 
      return false;

    NODE_BITS(curr).set_bit(Test_Cycle, true);

    for (auto it = g.get_arc_it(curr); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue;

        ARC_BITS(arc).set_bit(Test_Cycle, true);
        if (__test_cycle(g, src, it.get_tgt_node_ne()))
          return true;
      }

    return false; 
  }  

  /** @brief Internal DFS used by is_graph_acyclique(). */
  template <class GT> inline static
  bool __is_graph_acyclique(const GT & g, typename GT::Node * curr_node)
  {
    if (IS_NODE_VISITED(curr_node, Test_Cycle)) 
      return false;

    NODE_BITS(curr_node).set_bit(Test_Cycle, true);

    for (auto it = g.get_arc_it(curr_node); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Test_Cycle)) 
          continue; 

        ARC_BITS(arc).set_bit(Test_Cycle, true); 

        if (not __is_graph_acyclique(g, it.get_tgt_node_ne())) 
          return false;
      }
    // All outgoing arcs explored without detecting a back edge.
    return true; 
  }


  /** @brief Return `true` if an undirected graph is acyclic.
   *
   * This is a cycle-detection routine for undirected graphs. It performs a
   * DFS-like traversal while marking arcs to avoid immediately revisiting the
   * parent edge.
   *
   * @note Uses the `Test_Cycle` control bit on both nodes and arcs.
   * @note Returns `true` for empty graphs.
   *
   * @param[in] g Graph to test.
   * @param[in] start_node Starting node (must be non-null and belong to `g`).
   * @return `true` if the component reachable from `start_node` is acyclic;
   * `false` otherwise.
   * @throw domain_error If `g` is a directed graph.
   *
   * @ingroup Grafos
   * @see has_cycle(), is_graph_acyclique(const GT&)
   */
  template <class GT> inline 
  bool is_graph_acyclique(const GT & g, typename GT::Node * start_node)
  {
    ah_domain_error_if(g.is_digraph())
      << "is_graph_acyclique() does not work for digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return true;

    ah_invalid_argument_if(start_node == nullptr)
      << "is_graph_acyclique(): start_node must be non-null";

    if (num_nodes == 1)
      return true;
  
    if (g.get_num_arcs() >= num_nodes) 
      return false; 

    g.reset_bit_arcs(Test_Cycle);
    g.reset_bit_nodes(Test_Cycle);

    return __is_graph_acyclique(g, start_node);
  }

  /** @brief Return `true` if an undirected graph is acyclic.
   *
   * Equivalent to calling is_graph_acyclique(g, node) for every connected
   * component of `g`.
   *
   * @note Uses the `Test_Cycle` control bit on both nodes and arcs.
   * @note Returns `true` for empty graphs.
   *
   * @param[in] g Graph to test.
   * @return `true` if `g` is acyclic; `false` otherwise.
   * @throw domain_error If `g` is a directed graph.
   *
   * @ingroup Grafos
   * @see has_cycle(), is_graph_acyclique(const GT&, typename GT::Node*)
   */
  template <class GT> inline 
  bool is_graph_acyclique(const GT & g)
  {
    ah_domain_error_if(g.is_digraph())
      << "is_graph_acyclique() does not work for digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return true;

    if (num_nodes == 1)
      return true;

    if (g.get_num_arcs() >= num_nodes) 
      return false; 

    g.reset_bit_arcs(Test_Cycle);
    g.reset_bit_nodes(Test_Cycle);

    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne()) 
      {
        auto current_node = it.get_current_node_ne();
        if (IS_NODE_VISITED(current_node, Test_Cycle)) 
          continue; 

        if (not __is_graph_acyclique(g, current_node)) 
          return false;
      }

    return true;
  }

      
  /** @brief Return `true` if an undirected graph has at least one cycle.
   *
   * @param[in] g Graph to test.
   * @return `true` if `g` contains a cycle; `false` otherwise.
   *
   * @ingroup Grafos
   * @see is_graph_acyclique()
   */
  template <class GT> 
  inline bool has_cycle(const GT & g)
  {
    return not is_graph_acyclique(g);
  }

 
  /** @brief Return `true` if there is a path between two nodes.
   *
   * Performs a depth-first reachability test from `start_node` to
   * `end_node`.
   *
   * @note Uses the `Find_Path` control bit on both nodes and arcs.
   *
   * @param[in] g Graph to test.
   * @param[in] start_node Start node (must be non-null and belong to `g`).
   * @param[in] end_node End node (must be non-null and belong to `g`).
   * @return `true` if `end_node` is reachable from `start_node`; `false` otherwise.
   *
   * @ingroup Grafos
   * @see find_path_depth_first(), find_path_breadth_first()
   */
  template <class GT> inline 
  bool test_for_path(const GT & g, typename GT::Node * start_node, 
                     typename GT::Node * end_node)
  { 
    ah_invalid_argument_if(start_node == nullptr or end_node == nullptr)
      << "test_for_path(): start_node and end_node must be non-null";

    if (start_node == end_node)
      return true;

    g.reset_bit_nodes(Find_Path); 
    g.reset_bit_arcs(Find_Path);

    for (auto it = g.get_arc_it(start_node); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        ARC_BITS(arc).set_bit(Find_Path, true);
        if (__test_for_path(g, it.get_tgt_node_ne(), end_node)) 
          return true; 
      }

    return false;
  }


  /** @brief Internal recursive DFS used by test_for_path(). */
  template <class GT> inline static
  bool __test_for_path(const GT & g, typename GT::Node * curr_node, 
                       typename GT::Node * end_node)  
  {
    if (curr_node == end_node) 
      return true;

    if (IS_NODE_VISITED(curr_node, Find_Path))
      return false;

    NODE_BITS(curr_node).set_bit(Find_Path, true);

    for (auto it = g.get_arc_it(curr_node); it.has_curr(); it.next_ne())
      { 
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Find_Path)) 
          continue; 

        ARC_BITS(arc).set_bit(Find_Path, true);
        if (__test_for_path(g, it.get_tgt_node_ne(), end_node)) 
          return true;
      }

    return false;
  }

  /// @brief Forward declaration for inconnected_components().
  template <class GT> inline 
  DynList<GT> inconnected_components(const GT & g);

  /// @brief Forward declaration for build_subgraph().
  template <class GT> inline 
  void build_subgraph(const GT & g, GT & sg, 
                      typename GT::Node * g_src, size_t & node_count); 
    

  /** @brief Compute the connected components as mapped subgraphs.
   *
   * Builds a `DynList<GT>` where each element is a subgraph containing one
   * connected component of `g`.
   *
   * Side effects:
   * - Resets all node and arc control attributes via `g.reset_nodes()` and
   *   `g.reset_arcs()`.
   * - Uses the `Build_Subtree` control bit on both nodes and arcs.
   * - Maps nodes and arcs between each subgraph and `g` via cookies.
   *
   * @param[in] g Graph to split.
   * @return List of mapped subgraphs, one per connected component.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see build_subgraph(), copy_graph()
   */
  template <class GT> inline
  DynList<GT> inconnected_components(const GT & g) 
  {
    g.reset_nodes(); 
    g.reset_arcs(); 

    DynList<GT> list;
    size_t count = 0; // visited node counter
    for (auto it = g.get_node_it(); count < g.get_num_nodes() and it.has_curr();
         it.next_ne())
      {
        auto curr = it.get_current_node_ne();
        if (IS_NODE_VISITED(curr, Build_Subtree)) 
          continue;

        list.append(GT());
        GT & subgraph = list.get_last();
        build_subgraph(g, subgraph, curr, count); 
      }

    return list;
  }


  /** @brief Build a mapped subgraph from a starting node.
   *
   * Traverses `g` in depth-first order starting at `g_src`, copying every
   * reachable node and arc into `sg`.
   *
   * `node_count` accumulates the number of visited nodes across repeated calls
   * (useful when building all components).
   *
   * @note Uses the `Build_Subtree` control bit on both nodes and arcs.
   * @note Uses cookies to map nodes/arcs between `g` and `sg`.
   *
   * @param[in] g Original graph.
   * @param[out] sg Destination graph (expected to be initially empty).
   * @param[in] g_src Starting node in `g` (must be non-null and belong to `g`).
   * @param[in,out] node_count Visited node counter.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see inconnected_components(), copy_graph()
   */
  template <class GT> inline 
  void build_subgraph(const GT & g, GT & sg, 
                      typename GT::Node * g_src, size_t & node_count)
  {
    if (IS_NODE_VISITED(g_src, Build_Subtree)) 
      return;

    NODE_BITS(g_src).set_bit(Build_Subtree, true);
    ++node_count; 

    auto sg_src = mapped_node<GT>(g_src);
    if (sg_src == nullptr) // not mapped yet
      {
        sg_src = sg.insert_node(g_src->get_info());
        GT::map_nodes(g_src, sg_src);
      }

    for (auto it = g.get_arc_it(g_src); 
         node_count < g.get_num_nodes() and it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Build_Subtree)) 
          continue; 

        ARC_BITS(arc).set_bit(Build_Subtree, true);
        auto g_tgt  = it.get_tgt_node_ne();
        auto sg_tgt = mapped_node<GT>(g_tgt);
        if (sg_tgt == nullptr) // sg_tgt mapped in sg?
          {
            sg_tgt = sg.insert_node(g_tgt->get_info()); 
            GT::map_nodes(g_tgt, sg_tgt); 
          }

        auto sg_arc = sg.insert_arc(sg_src, sg_tgt, arc->get_info());
        GT::map_arcs(arc, sg_arc); 

        build_subgraph(g, sg, g_tgt, node_count);
      }
  }

  
  /** @brief Internal recursive DFS used by find_depth_first_spanning_tree(). */
  template <class GT> inline static
  bool __find_depth_first_spanning_tree(const GT &          g, 
                                        typename GT::Node * gnode, 
                                        typename GT::Arc *  garc, 
                                        GT &                tree,
                                        typename GT::Node * tnode);

  /** @brief Build a depth-first spanning tree (mapped to the original graph).
   *
   * Returns a spanning tree of the connected component reachable from `gnode`,
   * visiting nodes in depth-first order.
   *
   * Side effects:
   * - Resets all node and arc control attributes via `g.reset_nodes()` and
   *   `g.reset_arcs()`.
   * - Uses the `Spanning_Tree` control bit on both nodes and arcs.
   * - Maps nodes and arcs between `g` and the returned tree via cookies.
   *
   * @param[in] g Graph to traverse.
   * @param[in] gnode Start node (must be non-null and belong to `g`).
   * @return A mapped spanning tree of the reachable component.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see find_breadth_first_spanning_tree()
   */
  template <class GT> inline
  GT find_depth_first_spanning_tree(const GT & g, typename GT::Node * gnode)
  {
    g.reset_nodes();
    g.reset_arcs(); 

    GT tree;

    NODE_BITS(gnode).set_bit(Spanning_Tree, true);
  
    auto tnode = tree.insert_node(gnode->get_info()); 
    GT::map_nodes(gnode, tnode);
  
    for (auto it = g.get_arc_it(gnode); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        auto arc_tgt_node = it.get_tgt_node_ne();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue;

        if (__find_depth_first_spanning_tree(g, arc_tgt_node, arc, tree, tnode))
          return tree;
      }

    return tree;
  }

  /// \overload find_depth_first_spanning_tree()
  template <class GT> inline
  GT find_depth_first_spanning_tree(const GT & g)
  {
    return find_depth_first_spanning_tree(g, g.get_node());
  }

  
  template <class GT> inline static
  bool __find_depth_first_spanning_tree(const GT & g, typename GT::Node * gnode, 
                                        typename GT::Arc *  garc, 
                                        GT & tree, typename GT::Node * tnode)
  {
    NODE_BITS(gnode).set_bit(Spanning_Tree, true);
    ARC_BITS(garc).set_bit(Spanning_Tree, true);

    auto tree_tgt_node = tree.insert_node(gnode->get_info());
    GT::map_nodes(gnode, tree_tgt_node);

    auto tarc = tree.insert_arc(tnode, tree_tgt_node, garc->get_info()); 
    GT::map_arcs(garc, tarc);

    tnode = tree_tgt_node; 
    if (tree.get_num_nodes() == g.get_num_nodes())
      return true;

    assert(tree.get_num_nodes() > tree.get_num_arcs()); // tree invariant

    for (auto it = g.get_arc_it(gnode); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Spanning_Tree)) 
          continue;

        auto arc_tgt_node = it.get_tgt_node_ne();
        if (IS_NODE_VISITED(arc_tgt_node, Spanning_Tree))
          continue;

        if (__find_depth_first_spanning_tree(g, arc_tgt_node, arc, tree, tnode))
          return true; // spanning tree completed
      }

    return false;
  }

  /** @brief Build a breadth-first spanning tree (mapped to the original graph).
   *
   * Returns a spanning tree of the connected component reachable from `gp`,
   * visiting nodes in breadth-first order.
   *
   * Side effects:
   * - Resets the `Spanning_Tree` control bit on all nodes and arcs.
   * - Uses the `Spanning_Tree` control bit on both nodes and arcs.
   * - Maps nodes and arcs between `g` and the returned tree via cookies.
   *
   * @param[in,out] g Graph to traverse.
   * @param[in] gp Start node (must be non-null and belong to `g`).
   * @return A mapped spanning tree of the reachable component.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see find_depth_first_spanning_tree()
   */
  template <class GT> inline
  GT find_breadth_first_spanning_tree(GT & g, typename GT::Node * gp)
  {
    g.reset_bit_nodes(Spanning_Tree);
    g.reset_bit_arcs(Spanning_Tree);

    GT tree;

    std::unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
    tree.insert_node(tp_auto.get());
    GT::map_nodes(gp, tp_auto.release());
    NODE_BITS(gp).set_bit(Spanning_Tree, true);

    DynListQueue<typename GT::Arc*> q;
    for (auto it = g.get_arc_it(gp); it.has_curr(); it.next_ne())
      q.put(it.get_curr());

    while (not q.is_empty()) 
      {
        auto garc = q.get(); 
        ARC_BITS(garc).set_bit(Spanning_Tree, true);
        auto gsrc = g.get_src_node(garc);  
        auto gtgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(gsrc, Spanning_Tree) and 
            IS_NODE_VISITED(gtgt, Spanning_Tree))
          continue;

        if (IS_NODE_VISITED(gtgt, Spanning_Tree)) // gtgt visited?
          std::swap(gsrc, gtgt); // make gsrc the visited endpoint

        auto tsrc = mapped_node<GT>(gsrc);
        NODE_BITS(gtgt).set_bit(Spanning_Tree, true);

        // Create and map the new tree node.
        std::unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
        tree.insert_node(ttgt_auto.get());
        auto ttgt = ttgt_auto.release();
        GT::map_nodes(gtgt, ttgt);

        // Insert and map the new tree arc.
        auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
        GT::map_arcs(garc, tarc);
        if (tree.get_num_nodes() == g.get_num_nodes())
          break; 

        // Enqueue arcs incident to the newly visited node.
        for (auto it = g.get_arc_it(gtgt); it.has_curr(); it.next_ne())
          {
            auto current_arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(current_arc, Spanning_Tree)) 
              continue;

            if (IS_NODE_VISITED(g.get_src_node(current_arc),Spanning_Tree) and 
                IS_NODE_VISITED(g.get_tgt_node(current_arc),Spanning_Tree))
              continue;
            q.put(current_arc);
          }
      }

    return tree;
  }

  /** @brief Build a graph from a list of arcs (typically a spanning tree).
   *
   * Builds a new graph `ret` containing every arc in `arcs` (skipping null
   * entries). Nodes are created on demand.
   *
   * Mapping:
   * - Each node created in `ret` stores the original node pointer in its cookie.
   * - Each arc created in `ret` stores the original arc pointer in its cookie.
   *
   * @note This is a one-way mapping (`ret` → original). The original graph is
   * not modified.
   *
   * @param[in] arcs Dynamic array containing the arcs to insert.
   * @return A graph containing exactly the arcs provided in `arcs`.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   */
  template <class GT>
  GT build_spanning_tree(const DynArray<typename GT::Arc*> & arcs)
  {
    using Node = typename GT::Node;
    using Arc  = typename GT::Arc;

    GT ret;
    DynMapTree<Node*, Node*> table;
    arcs.for_each([&table, &ret] (Arc * ga)
                  {
                    if (ga == nullptr)
                      return;

                    Node * gsrc = (Node*) ga->src_node;
                    Node * gtgt = (Node*) ga->tgt_node;

                    Node * tsrc;
                    auto * pair_ptr = table.search(gsrc);
                    if (pair_ptr)
                      tsrc = pair_ptr->second;
                    else
                      {
                        tsrc = ret.insert_node(gsrc->get_info());
                        table.insert(gsrc, tsrc);
                        NODE_COOKIE(tsrc) = gsrc;
                      }
	
                    Node * ttgt;
                    pair_ptr = table.search(gtgt);
                    if (pair_ptr)
                      ttgt = pair_ptr->second;
                    else
                      {
                        ttgt = ret.insert_node(gtgt->get_info());
                        table.insert(gtgt, ttgt);
                        NODE_COOKIE(ttgt) = gtgt;
                      }

                    Arc * ta = ret.insert_arc(tsrc, ttgt);
                    *ta = *ga;
                    ARC_COOKIE(ta) = ga;
                  });

    return ret;
  }

  /// @brief Internal helper: DFS discovery time stored in `NODE_COUNTER(p)`.
  template <class GT> inline static 
  long & df(typename GT::Node * p)
  {
    return NODE_COUNTER(p);
  }

  /// @brief Internal helper: low-link value stored directly in `NODE_COOKIE(p)`.
  /// The cookie pointer itself is reinterpreted as a long value.
  template <class GT> inline static 
  long & low(typename GT::Node * p)
  {
    return reinterpret_cast<long&>(NODE_COOKIE(p));
  }

  /** @brief Internal DFS step used by compute_cut_nodes(). */
  template <class GT> inline static
  void __compute_cut_nodes(const GT & g, DynList<typename GT::Node *> & list, 
                           typename GT::Node * p, typename GT::Arc * a,
                           long & curr_df);

  /** @brief Compute articulation points (cut vertices) of an undirected graph.
   *
   * Uses a DFS-based algorithm (Tarjan-style) to compute the articulation
   * points reachable from `start`.
   *
   * Side effects:
   * - Resets arc control attributes via `g.reset_arcs()`.
   * - Resets node counters and control bits (all node bit fields are cleared).
   * - Uses the `Depth_First` and `Cut` control bits on nodes, and `Depth_First`
   *   on arcs.
   * - Temporarily stores low-link values in node cookies and clears all node
   *   cookies to `nullptr` before returning.
   *
   * @note For a disconnected graph, only the connected component reachable
   * from `start` is analyzed.
   *
   * @param[in] g Graph to analyze (must not be a digraph).
   * @param[in] start Starting node (must be non-null and belong to `g`).
   * @return List of articulation nodes (pointers into `g`).
   * @throw domain_error If `g` is a directed graph.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see paint_subgraphs(), map_subgraph(), map_cut_graph()
   */
  template <class GT>
  DynList<typename GT::Node*> 
  compute_cut_nodes(const GT & g, typename GT::Node * start)
  {
    DynList<typename GT::Node*> list;

    ah_domain_error_if(g.is_digraph())
      << "compute_cut_nodes() does not work on digraphs";

    if (g.get_num_nodes() == 0)
      return list;

    ah_invalid_argument_if(start == nullptr)
      << "compute_cut_nodes(): start must be non-null";

    using Node = typename GT::Node;

    std::vector<Node*> nodes;
    nodes.reserve(g.get_num_nodes());
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      nodes.push_back(it.get_curr());

    std::vector<long> low_values(nodes.size(), -1);

    for (size_t i = 0; i < nodes.size(); ++i)
      {
        auto p = nodes[i];
        NODE_COUNTER(p) = 0;
        NODE_BITS(p).reset();
        NODE_COOKIE(p) = &low_values[i];
      }

    struct Cookie_Guard
    {
      std::vector<Node*> & nodes;

      ~Cookie_Guard()
      {
        for (auto p : nodes)
          NODE_COOKIE(p) = nullptr;
      }
    } cookie_guard{nodes};

    g.reset_arcs();
    long current_df = 0;
    NODE_BITS(start).set_bit(Depth_First, true);
    low<GT>(start) = df<GT>(start) = current_df++;
    int call_counter = 0;
  
    // Traverse arcs incident to `start`.
    for (auto it = g.get_arc_it(start); 
         it.has_curr() and current_df < g.get_num_nodes(); it.next_ne())
      {
        auto tgt = it.get_tgt_node_ne();
        if (IS_NODE_VISITED(tgt, Depth_First)) 
          continue; 

        auto arc = it.get_current_arc_ne();
        if (IS_ARC_VISITED(arc, Depth_First)) 
          continue;

        ARC_BITS(arc).set_bit(Depth_First, true);
        __compute_cut_nodes(g, list, tgt, arc, current_df); 
        ++call_counter;
      }

    // Root is an articulation point iff it has more than one DFS child.
    if (call_counter > 1)
      {
        NODE_BITS(start).set_bit(Cut, true);
        list.append(start);
      }

    return list;
  }

  /// \overload compute_cut_nodes(const GT & g, typename GT::Node * start)
  template <class GT>
  DynList<typename GT::Node*> compute_cut_nodes(const GT & g)
  {
    return compute_cut_nodes(g, g.get_node());
  }
      
  /** @brief Internal recursive step for compute_cut_nodes(). */
  template <class GT> inline static
  void __compute_cut_nodes(const GT & g, DynList<typename GT::Node *> & list, 
                           typename GT::Node * p, typename GT::Arc * a, 
                           long & curr_df)
  {
    NODE_BITS(p).set_bit(Depth_First, true);
    low<GT>(p) = df<GT>(p) = curr_df++;

    bool p_is_cut_node = false;
    for (auto it = g.get_arc_it(p); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (arc == a) 
          continue; // parent arc

        auto tgt = it.get_tgt_node_ne();
        if (IS_NODE_VISITED(tgt, Depth_First)) 
          { 
            if (not IS_ARC_VISITED(arc, Depth_First))
              if (df<GT>(tgt) < low<GT>(p))
                low<GT>(p) = df<GT>(tgt);

            continue;
          }

        if (IS_ARC_VISITED(arc, Depth_First)) 
          continue;

        ARC_BITS(arc).set_bit(Depth_First, true);

        __compute_cut_nodes(g, list, tgt, arc, curr_df);

        if (low<GT>(tgt) < low<GT>(p)) 
          low<GT>(p) = low<GT>(tgt);

        if (low<GT>(tgt) >= df<GT>(p) and df<GT>(tgt) != 0)
          p_is_cut_node = true;
      }

    if (p_is_cut_node)
      {
        NODE_BITS(p).set_bit(Cut, true);
        list.append(p);
      }
  }

  /** @brief Special marker for arcs connecting a cut node to a non-cut block.
   *
   * Used by paint_subgraphs()/map_cut_graph().
   */
  const long Cross_Arc = -1;

  /// @brief Return `true` if the arc is marked as a cross-arc by paint_subgraphs().
  template <class GT> inline static 
  bool is_a_cross_arc(typename GT::Arc * a) 
  {
    return ARC_COUNTER(a) == Cross_Arc; 
  }

  /// @brief Return `true` if the node is marked as an articulation point.
  template <class GT> inline static 
  bool is_a_cut_node(typename GT::Node * p)
  {
    return NODE_BITS(p).get_bit(Cut);
  }

  /// @brief Return `true` if the arc is marked as a cut-arc (between cut nodes).
  template <class GT> inline static 
  bool is_an_cut_arc(typename GT::Arc * a)
  {
    return ARC_BITS(a).get_bit(Cut);
  }

  /// @brief Return `true` if the node has a positive color.
  template <class GT> inline static 
  bool is_node_painted(typename GT::Node * p)
  {
    return NODE_COUNTER(p) > 0;
  }

  /// @brief Return `true` if the arc has a positive color.
  template <class GT> inline static 
  bool is_arc_painted(typename GT::Arc * arc)
  {
    return ARC_COUNTER(arc) > 0;
  }

  /// @brief Set the node color (stored in `NODE_COUNTER`).
  template <class GT> inline static 
  void paint_node(typename GT::Node * p, const long & color)
  {
    NODE_COUNTER(p) = color;
  }

  /// @brief Set the arc color (stored in `ARC_COUNTER`).
  template <class GT> inline static 
  void paint_arc(typename GT::Arc * a, const long & color)
  {
    ARC_COUNTER(a) = color;
  }

  /// @brief Return the node color (stored in `NODE_COUNTER`).
  template <class GT> inline static 
  const long & get_color(typename GT::Node * p)
  {
    return NODE_COUNTER(p);
  }

  /// @brief Return the arc color (stored in `ARC_COUNTER`).
  template <class GT> inline static 
  const long & get_color(typename GT::Arc * a)
  {
    return ARC_COUNTER(a);
  }

  /** @brief Internal DFS that paints a non-cut block with `current_color`. */
  template <class GT> inline static
  void __paint_subgraph(const GT & g, typename GT::Node * p, long current_color)
  {
    assert(not is_a_cut_node <GT> (p));

    if (is_node_painted <GT> (p)) 
      return; 

    paint_node <GT> (p, current_color);

    for (auto it = g.get_arc_it(p); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (is_arc_painted <GT> (arc))
          continue;

        auto tgt = it.get_tgt_node_ne();
        if (is_a_cut_node <GT> (tgt))
          continue;

        paint_arc <GT> (arc, current_color); 

        __paint_subgraph(g, tgt, current_color); 
      }
  }

  /** @brief Internal step that paints all blocks adjacent to a cut node. */
  template <class GT> inline static
  void 
  __paint_from_cut_node(const GT & g, typename GT::Node * p, long & current_color)
  {
    assert(is_a_cut_node <GT> (p));

    // Paint each adjacent non-cut block with a new color.
    for (auto it = g.get_arc_it(p); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();

        assert(not is_arc_painted <GT> (arc));

        auto tgt_node = it.get_tgt_node_ne();
        if (is_a_cut_node <GT> (tgt_node)) // cut-to-cut arc
          {
            ARC_BITS(arc).set_bit(Cut, true);
            continue;
          }
        else 
          {
            paint_arc <GT> (arc, Cross_Arc);
            if (is_node_painted <GT> (tgt_node)) 
              continue; 
          }

        __paint_subgraph(g, tgt_node, current_color);

        ++current_color;

        assert(not is_arc_painted <GT> (arc));
      }
  }

  /** @brief Paint connected blocks around articulation points.
   *
   * Given a list of cut nodes (typically computed with compute_cut_nodes()),
   * paints the graph so that each biconnected block around the cut nodes gets
   * a distinct positive "color".
   *
   * Color storage:
   * - Node colors are stored in `NODE_COUNTER(node)`.
   * - Arc colors are stored in `ARC_COUNTER(arc)`.
   *
   * Special markers:
   * - Cut nodes keep color `0`.
   * - Cross arcs (cut → non-cut) are marked with `ARC_COUNTER(arc) == Cross_Arc`.
   * - Cut arcs (cut → cut) are marked by setting the `Cut` control bit on arcs.
   *
   * Return value:
   * - The first used color is `1`.
   * - The function returns the next unused color value (so valid colors are
   *   in the range `[1, return_value - 1]`).
   *
   * Protocol:
   * - The `Cut` bit on nodes must already mark cut nodes.
   * - Do not reset/alter node/arc control bits between computing cut nodes and
   *   calling this function.
   *
   * Side effects:
   * - Resets all node and arc counters (colors) to `0` before painting.
   *
   * @param[in] g Graph to paint.
   * @param[in] cut_node_list Cut nodes list (as returned by compute_cut_nodes()).
   * @return Next unused color id (one past the last used color).
   *
   * @ingroup Grafos
   * @see compute_cut_nodes(), map_subgraph(), map_cut_graph()
   */
  template <class GT> inline long 
  paint_subgraphs(const GT & g, const DynList<typename GT::Node*> & cut_node_list)
  {
    g.reset_counter_nodes();
    g.reset_counter_arcs();
    long current_color = 1;

    for (auto it = cut_node_list.get_it(); it.has_curr(); it.next_ne())
      __paint_from_cut_node(g, it.get_curr(), current_color);

    return current_color;
  }

  /** @brief Internal recursive step used by map_subgraph(). */
  template <class GT> inline static
  void __map_subgraph(const GT & g, GT & sg, typename GT::Node * gsrc, 
                      const long color)
  {
    assert(get_color <GT> (gsrc) == color);

    auto tsrc = mapped_node<GT>(gsrc); // image of gsrc in sg

    // Traverse arcs and copy those with the requested color.
    for (auto it = g.get_arc_it(gsrc); it.has_curr(); it.next_ne())
      {
        auto garc = it.get_current_arc_ne();
        if (get_color<GT>(garc) != color or IS_ARC_VISITED(garc, Build_Subtree))
          continue;

        ARC_BITS(garc).set_bit(Build_Subtree, true); 

        auto gtgt = it.get_tgt_node_ne(); 

        assert(get_color <GT> (gtgt) == color);

        typename GT::Node * ttgt = nullptr; // image of gtgt in sg
        if (IS_NODE_VISITED(gtgt, Build_Subtree))
          ttgt = mapped_node<GT> (gtgt);
        else
          {     // gtgt not in sg yet: copy & map it
            std::unique_ptr<typename GT::Node> ttgt_auto(new typename GT::Node(gtgt));
            sg.insert_node(ttgt_auto.get());
            GT::map_nodes(gtgt, ttgt_auto.get());
            NODE_BITS(gtgt).set_bit(Build_Subtree, true); 
            ttgt = ttgt_auto.release(); 
          }

        auto tarc = sg.insert_arc(tsrc, ttgt, garc->get_info());
        GT::map_arcs(garc, tarc);

        __map_subgraph(g, sg, gtgt, color);
      }
  }

  /** @brief Extract a mapped subgraph containing a given color.
   *
   * Assumes `g` has been painted (e.g., via paint_subgraphs()) and extracts the
   * connected block whose nodes/arcs have the given positive `color`.
   *
   * Side effects:
   * - Resets the `Build_Subtree` control bit on all nodes and arcs.
   * - Uses the `Build_Subtree` control bit during extraction.
   * - Maps nodes and arcs between `g` and the returned subgraph via cookies.
   *
   * @param[in] g Painted graph.
   * @param[in] color Color value to extract.
   * @return A mapped subgraph containing only nodes/arcs with `color`.
   * @throw domain_error If `color` does not exist in `g`.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see paint_subgraphs(), compute_cut_nodes(), map_cut_graph()
   */
  template <class GT>
  GT map_subgraph(const GT & g, const long color)
  {
    g.reset_bit_nodes(Build_Subtree);
    g.reset_bit_arcs(Build_Subtree);

    typename GT::Node * first = nullptr;
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      if (get_color <GT> (it.get_current_node_ne()) == color)
        {
          first = it.get_current_node_ne();
          break;
        }

    ah_domain_error_if(first == nullptr)
      << "Color does not exist in the graph";

    GT sg;
    std::unique_ptr<typename GT::Node> auto_tsrc(new typename GT::Node(first));
    sg.insert_node(auto_tsrc.get());
    GT::map_nodes(first, auto_tsrc.release());
    NODE_BITS(first).set_bit(Build_Subtree, true);

    __map_subgraph(g, sg, first, color);
  
    return sg;
  }

  /** @brief Extract the cut graph and cross-arc list.
   *
   * Requires a graph previously processed by:
   * - compute_cut_nodes(): marks cut nodes with the `Cut` bit
   * - paint_subgraphs(): paints blocks and marks cross/cut arcs
   *
   * The returned cut graph contains:
   * - all cut nodes
   * - all cut arcs (arcs between cut nodes; marked with the `Cut` bit on arcs)
   *
   * Cross arcs (cut → non-cut) are returned separately in `cross_arc_list`.
   *
   * @param[in] g Painted graph.
   * @param[in] cut_node_list Cut nodes list.
   * @return `{cut_graph, cross_arc_list}`.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   * @see map_subgraph(), compute_cut_nodes(), paint_subgraphs()
   */
  template <class GT>
  std::tuple<GT, DynList<typename GT::Arc*>>
  map_cut_graph(const GT & g, const DynList<typename GT::Node*> & cut_node_list)
  {
    GT cut_graph;
    DynList<typename GT::Arc*> cross_arc_list;

    for (auto it = cut_node_list.get_it(); it.has_curr(); it.next_ne())
      {
        auto gp = it.get_curr();

        assert(is_a_cut_node <GT> (gp));

        std::unique_ptr<typename GT::Node> tp_auto(new typename GT::Node(gp));
        cut_graph.insert_node(tp_auto.get());
        GT::map_nodes(gp, tp_auto.release());
      }

    // cut_graph contains cut-to-cut arcs; cross_arc_list collects cut-to-noncut arcs.
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto garc = it.get_current_arc_ne();
        if (is_a_cross_arc <GT> (garc))
          {
            cross_arc_list.append(garc); 
            continue;
          }

        if (not is_an_cut_arc <GT> (garc)) 
          continue;

        auto src = mapped_node<GT>(g.get_src_node(garc));
        auto tgt = mapped_node<GT>(g.get_tgt_node(garc));

        assert(src != nullptr and tgt != nullptr);

        auto arc = cut_graph.insert_arc(src, tgt, garc->get_info());
        GT::map_arcs(garc, arc);
      }
  
    return { cut_graph, cross_arc_list };
  }


  /** @brief Comparison functor for arc weights/distances.
   *
   * Commonly used by algorithms such as Dijkstra/Prim together with heaps.
   *
   * @tparam GT Graph type.
   * @tparam Distance Distance accessor functor.
   *
   * @ingroup Grafos
   */
  template <class GT, class Distance> 
  struct Distance_Compare
  {
    Distance dist;

    Distance_Compare(Distance __dist = Distance()) : dist(__dist) { /* empty */ }

    bool operator () (typename GT::Arc * a1, typename GT::Arc * a2) const
    {
      return dist(a1) < dist(a2);
    }
  };


  /** @brief Compute the transpose (arc-reversed) digraph.
   *
   * Builds a new digraph containing the same nodes as `g` and every arc
   * reversed.
   *
   * Side effects:
   * - Resets all node and arc control attributes via `g.reset_nodes()` and
   *   `g.reset_arcs()`.
   * - Maps nodes and arcs between `g` and the returned digraph via cookies.
   *
   * @param[in] g Input digraph.
   * @return The transposed digraph, mapped to `g`.
   * @throw domain_error If `g` is not a digraph.
   * @throw bad_alloc If there is not enough memory.
   *
   * @ingroup Grafos
   */
  template <class GT> 
  GT invert_digraph(const GT & g)
  {
    ah_domain_error_unless(g.is_digraph())
      << "invert_digraph() requires a digraph (or a graph temporarily treated as a digraph)";

    g.reset_nodes();
    g.reset_arcs();
    GT gi;

    // Copy all nodes first so isolated vertices are preserved.
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto gp = it.get_curr();
        auto ip = gi.insert_node(gp->get_info());
        GT::map_nodes(gp, ip);
      }

    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto arc   = it.get_curr();

        auto ssrc = g.get_src_node(arc);
        auto stgt = g.get_tgt_node(arc);

        auto rsrc = mapped_node<GT>(ssrc);
        auto rtgt = mapped_node<GT>(stgt);

        assert(rsrc != nullptr and rtgt != nullptr);

        typename GT::Arc * ai = gi.insert_arc(rtgt, rsrc, arc->get_info());
        GT::map_arcs(arc, ai);
      }

    assert(g.get_num_arcs() == gi.get_num_arcs() and 
           g.get_num_nodes() == gi.get_num_nodes());

    return gi;
  }


  /** @brief Functor for computing the transposed digraph, filtering arcs.
   *
   * Produces the transpose of `g`, including only arcs that satisfy the filter
   * `SA`.
   *
   * Nodes are always preserved (including isolated nodes).
   *
   * @ingroup Grafos
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Invert_Digraph
  {
    SA sa;

  public:

    /// Construct a functor using the arc filter `__sa`.
    Invert_Digraph(SA __sa) : sa(__sa) { /* empty */ }

    /** @brief Compute the transposed graph.
     *
     * @param[in] g Input digraph.
     * @return The transposed digraph, mapped to `g`.
     * @throw domain_error If `g` is not a digraph.
     * @throw bad_alloc If there is not enough memory.
     */
    GT operator () (const GT & g) const
    {
      ah_domain_error_unless(g.is_digraph())
        << "Invert_Digraph requires a digraph (or a graph temporarily treated as a digraph)";

      g.reset_nodes();
      g.reset_arcs();
      GT gi;

      // Copy all nodes first so isolated vertices are preserved.
      for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
        {
          auto gp = it.get_curr();
          auto ip = gi.insert_node(gp->get_info());
          GT::map_nodes(gp, ip);
        }

      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          auto arc  = it.get_curr();

          auto ssrc = g.get_src_node(arc);
          auto stgt = g.get_tgt_node(arc);

          auto rsrc = mapped_node<GT>(ssrc);
          auto rtgt = mapped_node<GT>(stgt);

          assert(rsrc != nullptr and rtgt != nullptr);

          typename GT::Arc * ai = gi.insert_arc(rtgt, rsrc, arc->get_info());
          GT::map_arcs(arc, ai);
        }

      assert(g.get_num_nodes() == gi.get_num_nodes());

      return gi;
    }
  };

  /** @brief Default distance accessor for arc weights.
   *
   * Reads the arc "distance" directly from `arc->get_info()`.
   *
   * @note Intended for numeric arc info types.
   *
   * @ingroup Grafos
   */
  template <class GT>
  class Dft_Dist
  {
  public:

    typedef typename GT::Arc_Type Distance_Type;

    static const Distance_Type Zero_Distance;

    static const Distance_Type Max_Distance;

    Distance_Type & operator () (typename GT::Arc * a) const
    {
      return a->get_info();
    }

    Distance_Type & operator () (typename GT::Arc * a, typename GT::Node*) const
    {
      return a->get_info();
    }

    static void set_zero(typename GT::Arc * a) { a->get_info() = 0; }
  };

  template <class GT>
  const typename Dft_Dist<GT>::Distance_Type Dft_Dist<GT>::Max_Distance =
    std::numeric_limits<typename Dft_Dist<GT>::Distance_Type>::max();

  template <class GT>
  const typename Dft_Dist<GT>::Distance_Type Dft_Dist<GT>::Zero_Distance =
    typename Dft_Dist<GT>::Distance_Type{};
  

  /** @brief Reconstruct a path encoded via node cookies and compute its cost.
   *
   * Many shortest-path algorithms store a predecessor pointer in each node
   * cookie. This function follows that predecessor chain from `end` back to `s`,
   * builds the corresponding `Path<GT>` (in forward order), and returns the
   * total cost according to the `Distance` accessor.
   *
   * Preconditions:
   * - For every node `p` on the path (except `s`), `NODE_COOKIE(p)` must point
   *   to its predecessor node.
   * - `path` must already be associated with the graph containing the nodes
   *   (i.e., `path.empty()` and `path.insert()` must be valid operations).
   *
   * @param[in] s Start node.
   * @param[in] end End node.
   * @param[in,out] path Output path (cleared and rebuilt).
   * @return Total distance along the reconstructed path.
   * @throw invalid_argument If `s` or `end` is nullptr.
   * @throw domain_error If the cookie chain is broken (nullptr encountered).
   * @throw bad_alloc If there is not enough memory.
   */
  /** @brief Helper to find the spanning tree arc between two nodes in a multigraph.
   *
   * When there are parallel arcs between two nodes, this function searches
   * for the arc that is marked as part of the spanning tree.
   *
   * @param g The graph
   * @param src Source node
   * @param tgt Target node
   * @return Pointer to the spanning tree arc, or nullptr if not found
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  typename GT::Arc *
  search_spanning_tree_arc(const GT & g,
                           typename GT::Node *src, typename GT::Node *tgt,
                           SA sa = SA()) noexcept
  {
    assert(src != nullptr and tgt != nullptr);

    // For efficiency, iterate over the node with fewer arcs
    if (not g.is_digraph() and tgt->num_arcs < src->num_arcs)
      std::swap(tgt, src);

    typename GT::Arc * first_match = nullptr;
    for (Node_Arc_Iterator<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
      {
        if (it.get_tgt_node_ne() != tgt)
          continue;

        auto arc = it.get_current_arc_ne();
        // Prefer arc marked as spanning tree
        if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
          return arc;

        // Keep first match as fallback
        if (first_match == nullptr)
          first_match = arc;
      }

    return first_match; // Return first match if no spanning tree arc found
  }

  template <class GT, class Distance = Dft_Dist<GT>>
  typename Distance::Distance_Type
  get_min_path(typename GT::Node * s, typename GT::Node * end, Path<GT> & path)
  {
    using Distance_Type = typename Distance::Distance_Type;

    ah_invalid_argument_if(s == nullptr or end == nullptr)
      << "get_min_path(): s and end must be non-null";

    Distance_Type dist{};
    path.empty();

    if (s == end)
      {
        path.init(end);
        return dist;
      }

    // Build path from end to start following cookies, collecting arcs
    Distance distance;
    DynList<std::pair<typename GT::Node*, typename GT::Arc*>> node_arc_list;

    auto curr = end;
    while (curr != s)
      {
        auto prev = static_cast<typename GT::Node *>(NODE_COOKIE(curr));
        ah_domain_error_if(prev == nullptr)
          << "get_min_path(): broken cookie chain (nullptr)";

        // Find the spanning tree arc between prev and curr
        auto arc = search_spanning_tree_arc<GT>(path.get_graph(), prev, curr);
        ah_domain_error_if(arc == nullptr)
          << "get_min_path(): no arc connecting nodes in path";

        node_arc_list.insert(std::make_pair(curr, arc));
        dist += distance(arc);
        curr = prev;
      }

    // Now build path from start to end
    path.init(s);
    for (auto it = node_arc_list.get_it(); it.has_curr(); it.next())
      {
        auto & [node, arc] = it.get_curr();
        (void)node; // suppress warning
        path.append(arc);
      }

    return dist;
  }

  /** @brief Compute the total cost (sum of arc weights) of a graph.
   *
   * Iterates over all arcs of a graph (filtered by `SA`) and sums their
   * distance/weight using the `Distance` accessor.
   *
   * Two usage patterns:
   * - Call `total_cost(g)` / `operator()(g)` to compute the sum for a whole graph.
   * - Use `operator()(arc)` as an accumulator with traversal utilities such as
   *   `traverse_arcs()`, then read the result via value().
   *
   * @tparam GT Graph type.
   * @tparam Distance Distance accessor (defaults to Dft_Dist).
   * @tparam SA Arc filter for Arc_Iterator.
   *
   * @ingroup Grafos
   */
  template <class GT, 
            class Distance = Dft_Dist<GT>, 
            class SA       = Dft_Show_Arc<GT>>
  class Total_Cost
  {
    Distance dist;
    SA       sa;
    typename Distance::Distance_Type sum{};

  public:

    Total_Cost(Distance __dist = Distance(), SA __sa = SA()) 
      : dist(__dist), sa(__sa)
    {
      // empty
    }

    /// Compute the total cost
    typename Distance::Distance_Type total_cost(GT & g)
    {
      sum = typename Distance::Distance_Type{};

      // Traverse all arcs and sum their weights
      for (Arc_Iterator <GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        sum += dist(it.get_current_arc_ne());

      return sum;
    }

    /// \overload total_cost
    typename Distance::Distance_Type operator () (GT & g)
    {
      return total_cost (g);
    }

    /// Reset the internal accumulator used by `operator()(Arc*)`.
    void reset() noexcept
    {
      sum = typename Distance::Distance_Type{};
    }

    /// Return the accumulated value (after using `operator()(Arc*)`).
    typename Distance::Distance_Type value() const noexcept
    {
      return sum;
    }

    bool operator () (typename GT::Arc * a)
    {
      if (not sa(a))
        return true; // skip but keep traversing

      sum += dist(a);
      return true; // keep traversing
    }
  };



} // end namespace Aleph

# endif // TPL_GRAPH_UTILS_H
