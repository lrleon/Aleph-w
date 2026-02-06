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


/** @file tpl_maxflow.H
 *  @brief Advanced maximum flow algorithms.
 *
 *  This file provides advanced algorithms for the maximum flow problem,
 *  complementing the basic algorithms in tpl_net.H.
 *
 *  ## Algorithms Included
 *
 *  ### Dinic's Algorithm
 *  - **Complexity**: O(V²E) for general graphs, O(E√V) for unit capacity
 *  - **Best for**: Dense graphs, unit capacity networks
 *  - Uses level graphs and blocking flows for efficiency
 *
 *  ### Capacity Scaling
 *  - **Complexity**: O(E² log U) where U is max capacity
 *  - **Best for**: Networks with large capacity values
 *  - Uses scaling phases to reduce augmenting path searches
 *
 *  ### Flow Decomposition
 *  - Decomposes a flow into s-t paths and cycles
 *  - Useful for routing, analysis, and visualization
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <tpl_maxflow.H>
 *
 *  Net_Graph<> net;
 *  // ... build network ...
 *
 *  // Using Dinic's algorithm
 *  auto flow = dinic_maximum_flow(net);
 *
 *  // Using Capacity Scaling
 *  auto flow = capacity_scaling_maximum_flow(net);
 *
 *  // Decompose the flow
 *  auto [paths, cycles] = decompose_flow(net);
 *  for (const auto& path : paths)
 *    std::cout << "Path with flow " << path.flow << "\n";
 *  ```
 *
 *  @see tpl_net.H Basic network flow structures and algorithms
 *  @see tpl_netcost.H Minimum cost flow algorithms
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_MAXFLOW_H
#define TPL_MAXFLOW_H

#include <limits>
#include <sstream>
#include <iomanip>
#include <tpl_array.H>
#include <tpl_dynListQueue.H>
#include <tpl_net.H>
#include <tpl_dynDlist.H>
#include <ah-errors.H>
#include <cookie_guard.H>

namespace Aleph
{
  //==============================================================================
  // DINIC'S ALGORITHM
  //==============================================================================

  /** @brief Level information for Dinic's algorithm.
   *
   *  Stores the BFS level (distance from source) for each node.
   *  Used to construct level graphs for blocking flow computation.
   *
   *  @ingroup Networks
   */
  template <typename Flow_Type>
  struct Dinic_Node_Info
  {
    long level = -1; ///< BFS level from source (-1 = unreachable)
    bool blocked = false; ///< Whether node is blocked in current phase

    /** @brief Reset node information to initial state.
     *
     *  Sets level to -1 and blocked status to false.
     *
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Thread safety: Not thread-safe for concurrent writers.
     *  @note Exception safety: noexcept
     */
    void reset() noexcept
    {
      level = -1;
      blocked = false;
    }
  };

  /** @brief Access node level in Dinic's algorithm.
   *
   *  Retrieves a reference to the level field stored in the node's cookie.
   *
   *  @tparam Net Network type.
   *  @param p Pointer to network node.
   *  @return Reference to the level value.
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Thread safety: Not thread-safe if multiple threads access the same node's cookie.
   *  @note Exception safety: noexcept
   *  @ingroup Networks
   */
  template <class Net>
  inline long &dinic_level(typename Net::Node *p) noexcept
  {
    assert(NODE_COOKIE(p) != nullptr &&
           "NODE_COOKIE not initialized; call dinic_maximum_flow() instead");
    return static_cast<Dinic_Node_Info<typename Net::Flow_Type> *>
        (NODE_COOKIE(p))->level;
  }

  /** @brief Check if node is blocked in Dinic's algorithm.
   *
   *  Retrieves a reference to the blocked flag stored in the node's cookie.
   *
   *  @tparam Net Network type.
   *  @param p Pointer to network node.
   *  @return Reference to blocked flag.
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Thread safety: Not thread-safe if multiple threads access the same node's cookie.
   *  @note Exception safety: noexcept
   *  @ingroup Networks
   */
  template <class Net>
  inline bool &dinic_blocked(typename Net::Node *p) noexcept
  {
    assert(NODE_COOKIE(p) != nullptr &&
           "NODE_COOKIE not initialized; call dinic_maximum_flow() instead");
    return static_cast<Dinic_Node_Info<typename Net::Flow_Type> *>
        (NODE_COOKIE(p))->blocked;
  }

  /** @brief Check if a node's cookie is properly initialized for Dinic's algorithm.
   *
   *  Validates that NODE_COOKIE points to a valid Dinic_Node_Info structure.
   *
   *  @tparam Net Network type.
   *  @param p Pointer to network node.
   *  @return true if the cookie is valid, false otherwise.
   *  @throws none
   *  @note Complexity: O(1)
   *  @note Thread safety: Thread-safe for read-only access.
   *  @note Exception safety: noexcept
   *  @ingroup Networks
   */
  template <class Net>
  inline bool is_dinic_cookie_valid(typename Net::Node *p) noexcept
  {
    return NODE_COOKIE(p) != nullptr;
  }

  /** @brief Build level graph using BFS from source.
   *
   *  Assigns levels to nodes based on the shortest path distance from the source
   *  in the residual network. Only considers edges with positive residual
   *  capacity.
   *
   *  @tparam Net Network type.
   *  @param net Flow network.
   *  @param source Source node.
   *  @param sink Sink node.
   *  @return true if sink is reachable, false otherwise.
   *  @throws std::bad_alloc if memory allocation for the queue fails.
   *  @note Complexity: O(E) where E is the number of edges.
   *  @note Thread safety: Not thread-safe. Modifies node cookies.
   *  @note Exception safety: Basic guarantee.
   *
   *  @ingroup Networks
   */
  template <class Net>
  bool build_level_graph(Net & net, typename Net::Node *source,
                         typename Net::Node *sink)
  {
    using Node = typename Net::Node;
    using Flow_Type = typename Net::Flow_Type;

    // Runtime guard: verify NODE_COOKIE is initialized for Dinic's algorithm
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      ah_domain_error_if(not is_dinic_cookie_valid<Net>(it.get_curr()))
            << "NODE_COOKIE not initialized for Dinic's algorithm. "
            << "Call dinic_maximum_flow() instead of build_level_graph() directly, "
            << "or initialize Dinic_Node_Info for all nodes before calling.";

    // Reset all levels
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        dinic_level<Net>(p) = -1;
        dinic_blocked<Net>(p) = false;
      }

    // BFS from source
    DynListQueue<Node *> queue;
    dinic_level<Net>(source) = 0;
    queue.put(source);

    while (not queue.is_empty())
      {
        Node *curr = queue.front();
        queue.get();

        // Explore all adjacent arcs
        for (typename Net::Node_Arc_Iterator it(curr); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_curr();
            auto next = net.get_connected_node(arc, curr);

            // Skip if already visited
            if (dinic_level<Net>(next) >= 0)
              continue;

            // Check residual capacity
            Flow_Type residual;
            if (net.get_src_node(arc) == curr)
              residual = arc->cap - arc->flow; // Forward edge
            else
              residual = arc->flow; // Backward edge

            if (residual > Flow_Type{0})
              {
                dinic_level<Net>(next) = dinic_level<Net>(curr) + 1;
                queue.put(next);
              }
          }
      }

    return dinic_level<Net>(sink) >= 0;
  }

  /** @brief Find a blocking flow using DFS in the level graph.
   *
   *  Sends flow along paths that strictly increase in level until
   *  no more augmenting paths exist (blocking flow).
   *
   *  @tparam Net Network type.
   *  @param net Flow network.
   *  @param node Current node in DFS.
   *  @param sink Target sink node.
   *  @param pushed Flow pushed so far on the current path.
   *  @return Amount of flow pushed to sink.
   *  @throws none (assuming flow operations do not throw).
   *  @note Complexity: O(V*E) per phase in the worst case.
   *  @note Space (call stack): O(h) where h ≤ |V| (number of nodes on a root-to-leaf path).
   *  @note Thread safety: Not thread-safe. Modifies arc flows and node cookies.
   *  @note Exception safety: Basic guarantee.
   *
   *  @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type dinic_dfs(Net & net, typename Net::Node *node,
                                    typename Net::Node *sink,
                                    typename Net::Flow_Type pushed)
  {
    using Flow_Type = typename Net::Flow_Type;

    // Runtime guard: verify NODE_COOKIE is initialized
    if (not is_dinic_cookie_valid<Net>(node))
      {
        ah_domain_error()
          << "NODE_COOKIE not initialized for Dinic's algorithm. "
          << "Call dinic_maximum_flow() instead of dinic_dfs() directly, "
          << "or initialize Dinic_Node_Info for all nodes before calling.";
        return Flow_Type{0}; // Unreachable but satisfies return type
      }

    if (node == sink)
      return pushed;

    if (dinic_blocked<Net>(node))
      return Flow_Type{0};

    for (typename Net::Node_Arc_Iterator it(node); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto next = net.get_connected_node(arc, node);

        // Only follow edges to the next level
        if (dinic_level<Net>(next) != dinic_level<Net>(node) + 1)
          continue;

        // Check residual capacity and direction
        Flow_Type residual;
        const bool is_forward = (net.get_src_node(arc) == node);
        if (is_forward)
          residual = arc->cap - arc->flow;
        else
          residual = arc->flow;

        if (residual <= Flow_Type{0})
          continue;

        Flow_Type flow_to_push = std::min(pushed, residual);
        Flow_Type result = dinic_dfs(net, next, sink, flow_to_push);

        if (result > Flow_Type{0})
          {
            if (is_forward)
              arc->flow += result;
            else
              arc->flow -= result;
            return result;
          }
      }

    // No augmenting path through this node
    dinic_blocked<Net>(node) = true;
    return Flow_Type{0};
  }

  /** @brief Compute maximum flow using Dinic's algorithm.
   *
   *  Dinic's algorithm computes maximum flow by repeatedly:
   *  1. Building a level graph using BFS
   *  2. Finding blocking flows using DFS
   *
   *  This is more efficient than Ford-Fulkerson for dense graphs.
   *
   *  ## Complexity
   *  - General graphs: O(V²E)
   *  - Unit capacity graphs: O(E√V)
   *  - Bipartite matching: O(E√V)
   *
   *  ## Algorithm Steps
   *  1. Build level graph from source using BFS
   *  2. While sink is reachable in level graph:
   *     a. Find blocking flow using DFS
   *     b. Augment flow along blocking paths
   *     c. Rebuild level graph
   *
   *  @tparam Net Network type (must be Net_Graph or compatible).
   *  @param net Flow network with source and sink.
   *  @return Maximum flow value.
   *
   *  @throws domain_error If network is not single source/sink.
   *  @throws std::bad_alloc If memory allocation fails during algorithm execution.
   *  @note Complexity: O(V²E) for general graphs, O(E√V) for unit capacity graphs.
   *  @note Thread safety: Not thread-safe. Modifies network state and node cookies.
   *  @note Exception safety: Basic guarantee. Network may be partially modified.
   *
   *  @par Example
   *  ```cpp
   *  Net_Graph<> net;
   *  // ... build network ...
   *  auto max_flow = dinic_maximum_flow(net);
   *  ```
   *
   *  @see ford_fulkerson_maximum_flow
   *  @see edmonds_karp_maximum_flow
   *  @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type dinic_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    using Flow_Type = typename Net::Flow_Type;

    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network must have single source and single sink";

    Node *source = net.get_source();
    Node *sink = net.get_sink();

    // RAII guard to restore original cookies on exit (normal or exception)
    Cookie_Saver<Net> cookie_saver(net, true, false); // save nodes only

    // Allocate node info with default-constructed elements
    auto node_info = Array<Dinic_Node_Info<Flow_Type>>::create(net.vsize());
    size_t idx = 0;
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne(), ++idx)
      NODE_COOKIE(it.get_curr()) = &node_info[idx];

    Flow_Type max_flow{0};

    // Main loop: build level graph and find blocking flows
    while (build_level_graph(net, source, sink))
      {
        // Find all blocking flows in the current level graph
        Flow_Type blocked_flow;
        do
          {
            blocked_flow = dinic_dfs(net, source, sink,
                                     std::numeric_limits<Flow_Type>::max());
            max_flow += blocked_flow;
          }
        while (blocked_flow > Flow_Type{0});
      }

    // Cookie_Saver destructor will restore original NODE_COOKIE values
    return max_flow;
  }

  /** @brief Functor wrapper for Dinic's algorithm.
   *  @ingroup Networks
   */
  template <class Net>
  struct Dinic_Maximum_Flow
  {
    /** @brief Invoke Dinic's maximum flow algorithm on the given network.
     *
     *  This functor provides a convenient way to call dinic_maximum_flow
     *  as a function object, useful for passing to algorithms or storing
     *  in containers.
     *
     *  @param[in] net Flow network with a single source and sink.
     *  @return Maximum flow value of type typename Net::Flow_Type.
     *  @throws domain_error If network is not single source/sink.
     *  @throws std::bad_alloc If memory allocation fails during algorithm execution.
     *  @note Complexity: O(V²E) for general graphs, O(E√V) for unit capacity graphs.
     *  @note Thread safety: Not thread-safe. Modifies network state and node cookies.
     *  @note Exception safety: Basic guarantee. Network may be partially modified.
     */
    typename Net::Flow_Type operator()(Net & net) const
    {
      return dinic_maximum_flow(net);
    }
  };


  //==============================================================================
  // CAPACITY SCALING
  //==============================================================================

  /** @brief Compute maximum flow using capacity scaling.
   *
   *  Capacity scaling improves augmenting path algorithms by only considering
   *  edges with "significant" residual capacity in each phase. This reduces
   *  the number of augmenting paths found.
   *
   *  ## Algorithm
   *  1. Start with Δ = largest power of 2 ≤ max capacity
   *  2. While Δ ≥ 1:
   *     a. Find augmenting paths using only edges with residual ≥ Δ
   *     b. Augment flow along these paths
   *     c. Δ = Δ / 2
   *
   *  ## Complexity
   *  O(E² log U) where U is the maximum capacity.
   *
   *  @tparam Net Network type.
   *  @param net Flow network.
   *  @return Maximum flow value.
   *
   *  @throws domain_error If network is not single source/sink.
   *  @throws std::bad_alloc If memory allocation fails during algorithm execution.
   *  @note Complexity: O(E² log U) where U is the maximum capacity.
   *  @note Thread safety: Not thread-safe. Modifies network flow values.
   *  @note Exception safety: Basic guarantee. Network may be partially modified.
   *
   *  @par Example
   *  ```cpp
   *  Net_Graph<> net;
   *  // ... build network with large capacities ...
   *  auto max_flow = capacity_scaling_maximum_flow(net);
   *  ```
   *
   *  @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type capacity_scaling_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using Flow_Type = typename Net::Flow_Type;

    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network must have single source and single sink";

    Node *source = net.get_source();
    Node *sink = net.get_sink();

    // Find maximum capacity to determine starting Δ
    Flow_Type max_cap{0};
    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      max_cap = std::max(max_cap, it.get_curr()->cap);

    if (max_cap == Flow_Type{0})
      return Flow_Type{0};

    // Start with the largest power of 2 ≤ max_cap
    Flow_Type delta{1};
    while (delta <= max_cap / 2)
      delta *= 2;

    Flow_Type max_flow{0};

    // Scaling phases
    while (delta >= Flow_Type{1})
      {
        // Find augmenting paths with residual ≥ delta using BFS
        bool found_path = true;
        while (found_path)
          {
            found_path = false;

            // BFS to find an augmenting path
            DynMapTree<Node *, Node *> parent;
            DynMapTree<Node *, Arc *> parent_arc;
            DynMapTree<Node *, bool> is_forward;

            DynListQueue<Node *> queue;
            queue.put(source);
            parent[source] = nullptr;

            while (not queue.is_empty() and not parent.has(sink))
              {
                Node *curr = queue.front();
                queue.get();

                for (typename Net::Node_Arc_Iterator it(curr); it.has_curr();
                     it.next_ne())
                  {
                    Arc *arc = it.get_curr();
                    Node *next = net.get_connected_node(arc, curr);

                    if (parent.has(next))
                      continue;

                    Flow_Type residual;
                    bool forward = (net.get_src_node(arc) == curr);

                    if (forward)
                      residual = arc->cap - arc->flow;
                    else
                      residual = arc->flow;

                    if (residual >= delta)
                      {
                        parent[next] = curr;
                        parent_arc[next] = arc;
                        is_forward[next] = forward;
                        queue.put(next);
                      }
                  }
              }

            // If sink reached, augment flow
            if (parent.has(sink))
              {
                found_path = true;

                // Find minimum residual on a path
                Flow_Type path_flow = std::numeric_limits<Flow_Type>::max();
                for (Node *n = sink; n != source; n = parent[n])
                  {
                    Arc *arc = parent_arc[n];
                    Flow_Type residual = is_forward[n] ?
                                           (arc->cap - arc->flow) :
                                           arc->flow;
                    path_flow = std::min(path_flow, residual);
                  }

                // Augment flow
                for (Node *n = sink; n != source; n = parent[n])
                  {
                    Arc *arc = parent_arc[n];
                    if (is_forward[n])
                      arc->flow += path_flow;
                    else
                      arc->flow -= path_flow;
                  }

                max_flow += path_flow;
              }
          }

        delta /= 2;
      }

    return max_flow;
  }

  /** @brief Functor wrapper for capacity scaling.
   *  @ingroup Networks
   */
  template <class Net>
  struct Capacity_Scaling_Maximum_Flow
  {
    /** @brief Invoke capacity scaling maximum flow algorithm on the given network.
     *
     *  This functor provides a convenient way to call capacity_scaling_maximum_flow
     *  as a function object, useful for passing to algorithms or storing
     *  in containers.
     *
     *  @param[in] net Flow network with a single source and sink.
     *  @return Maximum flow value of type typename Net::Flow_Type.
     *  @throws domain_error If network is not single source/sink.
     *  @throws std::bad_alloc If memory allocation fails during algorithm execution.
     *  @note Complexity: O(E² log U) where U is the maximum capacity.
     *  @note Thread safety: Not thread-safe. Modifies network flow values.
     *  @note Exception safety: Basic guarantee. Network may be partially modified.
     */
    typename Net::Flow_Type operator()(Net & net) const
    {
      return capacity_scaling_maximum_flow(net);
    }
  };


  //==============================================================================
  // FLOW DECOMPOSITION
  //==============================================================================

  /** @brief Represents a flow path from source to sink.
   *
   *  A flow path consists of:
   *  - A sequence of arcs from source to sink
   *  - The amount of flow on this path
   *
   *  @tparam Net Network type.
   *  @ingroup Networks
   */
  template <class Net>
  struct FlowPath
  {
    using Arc = typename Net::Arc;
    using Node = typename Net::Node;
    using Flow_Type = typename Net::Flow_Type;

    DynList<Arc *> arcs; ///< Arcs in the path (source to sink order)
    DynList<Node *> nodes; ///< Nodes in the path
    Flow_Type flow{0}; ///< Flow on this path

    /** @brief Check if a path is empty.
     *  @return true if the path has no arcs, false otherwise.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] bool is_empty() const noexcept { return arcs.is_empty(); }

    /** @brief Get path length (number of arcs).
     *  @return Number of arcs in the path.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] size_t length() const noexcept { return arcs.size(); }
  };

  /** @brief Represents a flow cycle in the network.
   *
   *  A flow cycle is a closed path with a positive flow.
   *  These only occur with lower bounds or in residual networks.
   *
   *  @tparam Net Network type.
   *  @ingroup Networks
   */
  template <class Net>
  struct FlowCycle
  {
    using Arc = typename Net::Arc;
    using Node = typename Net::Node;
    using Flow_Type = typename Net::Flow_Type;

    DynList<Arc *> arcs; ///< Arcs in the cycle
    DynList<Node *> nodes; ///< Nodes in the cycle
    Flow_Type flow{0}; ///< Flow on this cycle
  };

  /** @brief Result of flow decomposition.
   *
   *  Contains all paths and cycles that compose the flow.
   *
   *  @tparam Net Network type.
   *  @ingroup Networks
   */
  template <class Net>
  struct FlowDecomposition
  {
    using Flow_Type = typename Net::Flow_Type;

    DynList<FlowPath<Net>> paths; ///< Source-to-sink paths
    DynList<FlowCycle<Net>> cycles; ///< Cycles (if any)

    /** @brief Total flow (sum of path flows).
     *  @return Sum of flow values of all paths.
     *  @throws none
     *  @note Complexity: O(N) where N is the number of paths.
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: noexcept
     */
    Flow_Type total_flow() const noexcept
    {
      Flow_Type sum{0};
      for (auto it = paths.get_it(); it.has_curr(); it.next_ne())
        sum += it.get_curr().flow;
      return sum;
    }

    /** @brief Number of paths.
     *  @return The count of decomposed paths.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] size_t num_paths() const noexcept { return paths.size(); }

    /** @brief Number of cycles.
     *  @return The count of decomposed cycles.
     *  @throws none
     *  @note Complexity: O(1)
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: noexcept
     */
    [[nodiscard]] size_t num_cycles() const noexcept { return cycles.size(); }
  };

  /** @brief Decompose network flow into paths and cycles.
   *
   *  Given a network with a feasible flow, this function decomposes the flow
   *  into a set of source-to-sink paths (and possibly cycles). Each unit of
   *  the flow is assigned to exactly one path.
   *
   *  ## Algorithm
   *  1. While there exists an arc from source with positive flow:
   *     a. Follow arcs with positive flow until reaching sink
   *     b. Find minimum flow on the path
   *     c. Subtract this flow from all arcs on the path
   *     d. Record the path
   *
   *  ## Properties
   *  - Number of paths ≤ number of arcs with positive flow
   *  - Sum of path flows = total flow value
   *  - Each arc's flow = sum of flows on paths using that arc
   *
   *  @tparam Net Network type.
   *  @param net Network with a feasible flow. Creates a working copy of flow
   *             values for decomposition; the original network flow values
   *             remain unchanged.
   *  @return FlowDecomposition containing all paths and cycles.
   *
   *  @throw domain_error If network is not single source/sink.
   *
   *  @par Example
   *  ```cpp
   *  Net_Graph<> net;
   *  // ... compute max flow ...
   *  auto decomp = decompose_flow(net);
   *
   *  std::cout << "Flow decomposed into " << decomp.num_paths() << " paths\n";
   *  for (const auto& path : decomp.paths)
   *    std::cout << "  Path with flow " << path.flow
   *              << " using " << path.length() << " arcs\n";
   *  ```
   *
   *  @see dinic_maximum_flow
   *  @ingroup Networks
   */
  template <class Net>
  FlowDecomposition<Net> decompose_flow(const Net & net)
  {
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using Flow_Type = typename Net::Flow_Type;

    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network must have single source and single sink";

    FlowDecomposition<Net> result;

    // Create a copy of flow values (we'll modify them during decomposition)
    DynMapTree<Arc *, Flow_Type> remaining_flow;
    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        Arc *arc = it.get_curr();
        remaining_flow[arc] = arc->flow;
      }

    Node *source = net.get_source();
    Node *sink = net.get_sink();

    // Find paths from source to sink
    while (true)
      {
        // Find an arc from source with positive remaining flow
        Arc *start_arc = nullptr;
        for (typename Net::Node_Arc_Iterator it(source); it.has_curr(); it.next_ne())
          {
            Arc *arc = it.get_curr();
            if (net.get_src_node(arc) == source and remaining_flow[arc] > Flow_Type{0})
              {
                start_arc = arc;
                break;
              }
          }

        if (start_arc == nullptr)
          break; // No more flow from source

        // Build path by following positive flow, tracking visited nodes to detect cycles
        FlowPath<Net> path;
        path.nodes.append(source);

        Node *curr = net.get_tgt_node(start_arc);
        path.arcs.append(start_arc);
        path.nodes.append(curr);
        path.flow = remaining_flow[start_arc];

        // Track visited nodes and their position in path for cycle detection
        DynMapTree<Node *, size_t> visited;
        visited[source] = 0;
        visited[curr] = 1;

        while (curr != sink)
          {
            // Find outgoing arc with positive flow
            Arc *next_arc = nullptr;
            for (typename Net::Node_Arc_Iterator it(curr); it.has_curr(); it.next_ne())
              {
                Arc *arc = it.get_curr();
                if (net.get_src_node(arc) == curr and remaining_flow[arc] > Flow_Type{0})
                  {
                    next_arc = arc;
                    break;
                  }
              }

            if (next_arc == nullptr)
              break; // No outgoing arc with a positive flow

            Node *next_node = net.get_tgt_node(next_arc);

            // Check if we've visited this node before (cycle detected)
            if (visited.contains(next_node))
              {
                // Extract the cycle starting from the repeated node
                const size_t cycle_start = visited[next_node];
                FlowCycle<Net> cycle;
                cycle.flow = remaining_flow[next_arc];

                // Build cycle from the position where we first saw next_node
                auto node_it = path.nodes.get_it();
                auto arc_it = path.arcs.get_it();

                // Skip to the cycle start position
                for (size_t i = 0; i < cycle_start; ++i)
                  {
                    node_it.next_ne();
                    arc_it.next_ne();
                  }

                // Collect cycle nodes and arcs, compute minimum flow
                while (arc_it.has_curr())
                  {
                    cycle.nodes.append(node_it.get_curr());
                    cycle.arcs.append(arc_it.get_curr());
                    cycle.flow = std::min(cycle.flow, remaining_flow[arc_it.get_curr()]);
                    node_it.next_ne();
                    arc_it.next_ne();
                  }
                // Add the closing arc and closing node to complete the cycle
                cycle.arcs.append(next_arc);
                cycle.flow = std::min(cycle.flow, remaining_flow[next_arc]);
                cycle.nodes.append(next_node);

                // Subtract cycle flow from cycle arcs
                for (auto it = cycle.arcs.get_it(); it.has_curr(); it.next_ne())
                  remaining_flow[it.get_curr()] -= cycle.flow;

                if (cycle.flow > Flow_Type{0})
                  result.cycles.append(std::move(cycle));

                // At this point we have removed all flow along the detected cycle
                // from 'remaining_flow'. We intentionally discard the current
                // partial path (the prefix up to 'cycle_start') and restart path
                // construction from the source in the outer loop. This does not
                // lose any flow because the prefix arcs were not modified and
                // remain available for future paths/cycles. The trade-off is that
                // we may re-traverse some nodes/edges, which is acceptable here
                // under the assumption that cycles in the residual flow are rare.
                break;
              }

            path.arcs.append(next_arc);
            path.flow = std::min(path.flow, remaining_flow[next_arc]);
            curr = next_node;
            path.nodes.append(curr);
            visited[curr] = path.nodes.size() - 1;
          }

        // Subtract path flow from all arcs (only if we reached sink)
        if (curr == sink and path.flow > Flow_Type{0})
          {
            for (auto it = path.arcs.get_it(); it.has_curr(); it.next_ne())
              remaining_flow[it.get_curr()] -= path.flow;
            result.paths.append(std::move(path));
          }
      }

    // Second phase: find cycles in the remaining flow (not connected to source)
    // These are cycles that exist independently of the s-t flow
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        Node *start = it.get_curr();

        // Try to find a cycle starting from this node
        while (true)
          {
            // Find an outgoing arc with positive remaining flow
            Arc *first_arc = nullptr;
            for (typename Net::Node_Arc_Iterator ait(start); ait.has_curr(); ait.next_ne())
              {
                Arc *arc = ait.get_curr();
                if (net.get_src_node(arc) == start and remaining_flow[arc] > Flow_Type{0})
                  {
                    first_arc = arc;
                    break;
                  }
              }

            if (first_arc == nullptr)
              break; // No outgoing flow from this node

            // Follow the flow to find a cycle
            FlowCycle<Net> cycle;
            DynMapTree<Node *, size_t> visited;
            DynList<Arc *> path_arcs;
            DynList<Node *> path_nodes;

            path_nodes.append(start);
            visited[start] = 0;

            Node *curr = net.get_tgt_node(first_arc);
            path_arcs.append(first_arc);
            path_nodes.append(curr);

            bool found_cycle = false;

            while (not found_cycle)
              {
                if (visited.contains(curr))
                  {
                    // Found a cycle starting at 'curr'
                    const size_t cycle_start = visited[curr];
                    cycle.flow = std::numeric_limits<Flow_Type>::max();

                    // Skip to cycle start in path
                    auto node_it = path_nodes.get_it();
                    auto arc_it = path_arcs.get_it();
                    for (size_t i = 0; i < cycle_start; ++i)
                      {
                        node_it.next_ne();
                        arc_it.next_ne();
                      }

                    // Collect cycle and find minimum flow
                    while (arc_it.has_curr())
                      {
                        cycle.nodes.append(node_it.get_curr());
                        cycle.arcs.append(arc_it.get_curr());
                        cycle.flow = std::min(cycle.flow, remaining_flow[arc_it.get_curr()]);
                        node_it.next_ne();
                        arc_it.next_ne();
                      }

                    found_cycle = true;
                    break;
                  }

                visited[curr] = path_nodes.size() - 1;

                // Find next arc with positive flow
                Arc *next_arc = nullptr;
                for (typename Net::Node_Arc_Iterator ait(curr); ait.has_curr(); ait.next_ne())
                  {
                    Arc *arc = ait.get_curr();
                    if (net.get_src_node(arc) == curr and remaining_flow[arc] > Flow_Type{0})
                      {
                        next_arc = arc;
                        break;
                      }
                  }

                if (next_arc == nullptr)
                  break; // Dead end, no cycle from this path

                path_arcs.append(next_arc);
                curr = net.get_tgt_node(next_arc);
                path_nodes.append(curr);
              }

            if (found_cycle and cycle.flow > Flow_Type{0})
              {
                // Subtract cycle flow from all cycle arcs
                for (auto cit = cycle.arcs.get_it(); cit.has_curr(); cit.next_ne())
                  remaining_flow[cit.get_curr()] -= cycle.flow;
                result.cycles.append(std::move(cycle));
              }
            else
              break; // No cycle found from this start node
          }
      }

    return result;
  }

  /** @brief Functor wrapper for flow decomposition.
   *  @ingroup Networks
   */
  template <class Net>
  struct Decompose_Flow
  {
    /** @brief Invoke flow decomposition on the given network.
     *
     *  This functor provides a convenient way to call decompose_flow
     *  as a function object, useful for passing to algorithms or storing
     *  in containers.
     *
     *  @param[in] net Network with a feasible flow (single source and sink).
     *  @return FlowDecomposition<Net> containing all paths and cycles.
     *  @throws domain_error If network is not single source/sink.
     *  @throws std::bad_alloc If memory allocation fails during decomposition.
     *  @note Complexity: O(P*E) where P is the number of paths in the decomposition.
     *  @note Thread safety: Thread-safe for read-only access to input network.
     *  @note Exception safety: Strong guarantee. No modifications to input network.
     */
    FlowDecomposition<Net> operator()(const Net & net) const
    {
      return decompose_flow(net);
    }
  };


  //==============================================================================
  // HIGHEST LABEL PREFLOW-PUSH (HLPP)
  //==============================================================================

  /** @brief Compute maximum flow using Highest-Label Preflow-Push.
   *
   *  This variant of preflow-push always processes the active node with the
   *  highest label (distance from sink). This leads to better practical
   *  performance than generic preflow-push.
   *
   *  ## Complexity
   *  O(V²√E) - often fastest in practice for dense graphs.
   *
   *  @tparam Net Network type.
   *  @param net Flow network.
   *  @return Maximum flow value.
   *
   *  @ingroup Networks
   */
  template <class Net>
  typename Net::Flow_Type hlpp_maximum_flow(Net & net)
  {
    using Node = typename Net::Node;
    using Arc = typename Net::Arc;
    using Flow_Type = typename Net::Flow_Type;

    ah_domain_error_if(not (net.is_single_source() and net.is_single_sink()))
      << "Network must have single source and single sink";

    Node *source = net.get_source();
    Node *sink = net.get_sink();
    const size_t n = net.vsize();

    // Node data: height, excess
    DynMapTree<Node *, long> height;
    DynMapTree<Node *, Flow_Type> excess;

    // Initialize
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        height[p] = 0;
        excess[p] = Flow_Type{0};
      }

    height[source] = n;

    // Saturate all arcs from the source
    for (typename Net::Node_Arc_Iterator it(source); it.has_curr(); it.next_ne())
      if (Arc *arc = it.get_curr(); net.get_src_node(arc) == source)
        {
          Flow_Type delta = arc->cap - arc->flow;
          arc->flow = arc->cap;
          excess[net.get_tgt_node(arc)] += delta;
          excess[source] -= delta;
        }

    // Active nodes organized by height (empty DynLists by default)
    auto buckets = Array<DynList<Node *>>::create(2 * n + 1);
    long max_height = 0;

    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        if (p != source and p != sink and excess[p] > Flow_Type{0})
          {
            buckets[height[p]].append(p);
            max_height = std::max(max_height, height[p]);
          }
      }

    // Main loop
    while (max_height >= 0)
      {
        if (buckets[max_height].is_empty())
          {
            --max_height;
            continue;
          }

        Node *u = buckets[max_height].remove_first();

        // Push/Relabel
        bool pushed = false;
        long min_height = 2 * n;

        for (typename Net::Node_Arc_Iterator it(u); it.has_curr() and excess[u] > Flow_Type{0};
             it.next_ne())
          {
            Arc *arc = it.get_curr();
            Node *v = net.get_connected_node(arc, u);

            Flow_Type residual;
            const bool is_forward = (net.get_src_node(arc) == u);
            if (is_forward)
              residual = arc->cap - arc->flow;
            else
              residual = arc->flow;

            if (residual > Flow_Type{0})
              {
                if (height[u] == height[v] + 1)
                  {
                    // Push
                    Flow_Type delta = std::min(excess[u], residual);
                    if (is_forward)
                      arc->flow += delta;
                    else
                      arc->flow -= delta;

                    excess[u] -= delta;
                    excess[v] += delta;

                    if (v != source and v != sink and
                        excess[v] == delta) // v became active
                      {
                        buckets[height[v]].append(v);
                        max_height = std::max(max_height, height[v]);
                      }

                    pushed = true;
                  }
                else
                  min_height = std::min(min_height, height[v]);
              }
          }

        if (excess[u] > Flow_Type{0})
          {
            if (not pushed)
              height[u] = min_height + 1; // Relabel
            buckets[height[u]].append(u);
            max_height = std::max(max_height, height[u]);
          }
      }

    return excess[sink];
  }

  /** @brief Functor wrapper for HLPP.
   *  @ingroup Networks
   */
  template <class Net>
  struct HLPP_Maximum_Flow
  {
    /** @brief Invoke the Highest-Label Preflow-Push algorithm on the given network.
     *
     *  This functor provides a convenient way to call hlpp_maximum_flow
     *  as a function object, useful for passing to algorithms or storing
     *  in containers.
     *
     *  @param[in] net Flow network with a single source and sink.
     *  @return Maximum flow value of type typename Net::Flow_Type.
     *  @throws domain_error If network is not single source/sink.
     *  @throws std::bad_alloc If memory allocation fails during algorithm execution.
     *  @note Complexity: O(V²√E) - often fastest in practice for dense graphs.
     *  @note Thread safety: Not thread-safe. Modifies network flow values.
     *  @note Exception safety: Basic guarantee. Network may be partially modified.
     */
    typename Net::Flow_Type operator()(Net & net) const
    {
      return hlpp_maximum_flow(net);
    }
  };


  //==============================================================================
  // FLOW STATISTICS
  //==============================================================================

  /** @brief Statistics about a network flow.
   *  @ingroup Networks
   */
  template <typename Flow_Type>
  struct FlowStatistics
  {
    Flow_Type total_flow{0}; ///< Total flow value
    Flow_Type total_capacity{0}; ///< Sum of all capacities
    size_t num_saturated_arcs{0}; ///< Arcs with flow = capacity
    size_t num_empty_arcs{0}; ///< Arcs with flow = 0
    size_t num_partial_arcs{0}; ///< Arcs with 0 < flow < capacity
    double utilization{0.0}; ///< flow / capacity ratio

    /** @brief Format statistics as a string.
     *
     *  Builds a formatted string representation of the statistics.
     *
     *  @return Formatted string containing all statistics.
     *  @note Complexity: O(1)
     *  @note Thread safety: Thread-safe for read-only access.
     *  @note Exception safety: Basic guarantee.
     */
    [[nodiscard]] std::string to_string() const
    {
      std::ostringstream oss;
      oss << "=== Flow Statistics ===\n"
          << "Total flow: " << total_flow << "\n"
          << "Total capacity: " << total_capacity << "\n"
          << "Saturated arcs: " << num_saturated_arcs << "\n"
          << "Empty arcs: " << num_empty_arcs << "\n"
          << "Partial arcs: " << num_partial_arcs << "\n"
          << "Utilization: " << std::fixed << std::setprecision(2)
          << (utilization * 100) << "%\n";
      return oss.str();
    }

    /** @brief Print statistics to standard output.
     *
     *  Formats the statistics and outputs to stdout.
     *
     *  @throws std::format_error if formatting fails.
     *  @note Complexity: O(1)
     *  @note Thread safety: Not thread-safe (writes to stdout).
     *  @note Exception safety: Basic guarantee.
     */
    void print() const
    {
      std::cout << to_string();
    }
  };

  /** @brief Compute statistics about the current flow in a network.
   *
   *  @tparam Net Network type.
   *  @param net Network with flow values.
   *  @return FlowStatistics structure.
   *  @throws none
   *  @note Complexity: O(E) where E is the number of arcs.
   *  @note Thread safety: Thread-safe for read-only access.
   *  @note Exception safety: Basic guarantee.
   *
   *  @ingroup Networks
   */
  template <class Net>
  FlowStatistics<typename Net::Flow_Type> compute_flow_statistics(const Net & net)
  {
    using Flow_Type = typename Net::Flow_Type;

    FlowStatistics<Flow_Type> stats;

    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        stats.total_capacity += arc->cap;

        if (arc->flow == arc->cap)
          ++stats.num_saturated_arcs;
        else if (arc->flow == Flow_Type{0})
          ++stats.num_empty_arcs;
        else
          ++stats.num_partial_arcs;
      }

    if (net.is_single_source())
      stats.total_flow = net.flow_value();

    if (stats.total_capacity > Flow_Type{0})
      stats.utilization = static_cast<double>(stats.total_flow) /
                          static_cast<double>(stats.total_capacity);

    return stats;
  }
} // namespace Aleph

#endif // TPL_MAXFLOW_H
