
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


/** @file Stoer_Wagner.H
 *  @brief Stoer-Wagner deterministic min-cut algorithm.
 *
 *  Implements the Stoer-Wagner algorithm for finding minimum cuts in
 *  undirected weighted graphs. Unlike Karger's randomized approach,
 *  Stoer-Wagner is deterministic and always finds the exact minimum cut.
 *
 *  ## Algorithm Description
 *
 *  The Stoer-Wagner algorithm is based on a key observation: for any two
 *  vertices s and t, the minimum cut of the graph is either:
 *  1. The minimum s-t cut (separating s from t), OR
 *  2. The minimum cut of the graph with s and t merged
 *
 *  The algorithm exploits this by repeatedly finding a "minimum s-t cut"
 *  using a procedure called **Maximum Adjacency Search** (also known as
 *  Maximum Cardinality Search), then merging s and t.
 *
 *  ### Maximum Adjacency Search (MAS)
 *
 *  MAS grows a set A of vertices starting from an arbitrary vertex:
 *  1. Start with A = {arbitrary vertex}
 *  2. Repeatedly add the vertex v ∉ A that has maximum total edge weight
 *     to vertices in A
 *  3. Continue until all vertices are in A
 *  4. The last two vertices added (s and t) define a "minimum s-t cut"
 *
 *  **Key theorem**: The cut separating t from all other vertices has weight
 *  equal to the sum of edges incident to t. This is a minimum s-t cut!
 *
 *  ### Main Algorithm
 *
 *  1. Run MAS to find s, t, and the cut-of-the-phase (weight of edges to t)
 *  2. If this cut is smaller than best known, update best cut
 *  3. Merge s and t into a single vertex
 *  4. Repeat until only 2 vertices remain
 *  5. Return the minimum cut found
 *
 *  ## Time Complexity
 *
 *  - **With binary heap**: O(nm + n² log n)
 *  - **With Fibonacci heap**: O(nm + n² log n)
 *  - **Simple implementation**: O(n²m) or O(n³) for dense graphs
 *
 *  This implementation uses a binary heap, achieving O(nm + n² log n).
 *
 *  ## Comparison with Other Algorithms
 *
 *  | Algorithm      | Time Complexity    | Type          | Guarantee      |
 *  |----------------|-------------------|---------------|----------------|
 *  | Stoer-Wagner   | O(nm + n² log n)  | Deterministic | Exact          |
 *  | Karger         | O(n⁴ m)           | Randomized    | High prob.     |
 *  | Karger-Stein   | O(n² log³ n)      | Randomized    | High prob.     |
 *  | Max-Flow       | O(n³) or better   | Deterministic | Exact (s-t)    |
 *
 *  ## When to Use Stoer-Wagner
 *
 *  - **Exact answer required**: Deterministic, always finds true min-cut
 *  - **Weighted graphs**: Handles edge weights naturally
 *  - **Sparse graphs**: O(nm) term dominates, very efficient
 *  - **Small to medium graphs**: Practical for n < 10,000
 *
 *  ## Limitations
 *
 *  - Designed for undirected graphs only
 *  - For directed graphs, use max-flow based algorithms
 *  - Memory: O(n²) for the adjacency representation during merges
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef STOER_WAGNER_H
# define STOER_WAGNER_H

# include <limits>
# include <vector>
# include <queue>
# include <functional>
# include <htlist.H>
# include <tpl_sgraph.H>
# include <tpl_dynArray.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  /**
      Stoer-Wagner deterministic minimum cut algorithm.

      This class implements the Stoer-Wagner algorithm for finding the
      global minimum cut in an undirected weighted graph. Unlike randomized
      algorithms (Karger, Karger-Stein), Stoer-Wagner is deterministic and
      always finds the exact minimum cut.

      ## Algorithm Overview

      The algorithm is based on the observation that for any two vertices
      s and t, the global minimum cut is either:
      - The minimum s-t cut, OR
      - The minimum cut of the graph with s and t merged

      It uses Maximum Adjacency Search to efficiently find minimum s-t cuts.

      ## Template Parameters

      - **GT**: Graph type (must be based on List_Graph)
      - **Distance**: Functor to extract edge weight. Must provide
                      `Weight operator()(Arc*)`. Defaults to extracting
                      `arc->get_info()` as the weight.
      - **SA**: Arc filter functor. Defaults to showing all arcs.

      ## Weight Type

      The algorithm works with any numeric weight type that supports:
      - Addition (+)
      - Comparison (<)
      - Assignment (=)
      - Zero initialization

      ## Example Usage

      @code
      #include <Stoer_Wagner.H>
      #include <tpl_graph.H>

      using GT = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
      GT g;

      // Build weighted graph
      auto a = g.insert_node("A");
      auto b = g.insert_node("B");
      auto c = g.insert_node("C");
      auto d = g.insert_node("D");

      g.insert_arc(a, b, 2);  // Weight 2
      g.insert_arc(b, c, 3);
      g.insert_arc(c, d, 4);
      g.insert_arc(d, a, 1);
      g.insert_arc(a, c, 5);

      // Find minimum cut
      Stoer_Wagner_Min_Cut<GT> solver;
      DynList<GT::Node*> S, T;
      DynList<GT::Arc*> cut_arcs;

      int cut_weight = solver(g, S, T, cut_arcs);
      cout << "Min-cut weight: " << cut_weight << endl;
      @endcode

      @par Example: Unweighted graph (all edges have weight 1)
      @code
      // For unweighted graphs, use a constant distance functor
      struct UnitWeight {
        int operator()(GT::Arc*) const { return 1; }
      };

      Stoer_Wagner_Min_Cut<GT, UnitWeight> solver;
      int cut_size = solver(g, S, T, cut_arcs);
      @endcode

      @see Karger_Min_Cut Randomized algorithm (simpler, slower)
      @see Karger_Stein_Min_Cut Faster randomized algorithm
      @see https://en.wikipedia.org/wiki/Stoer%E2%80%93Wagner_algorithm

      @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Stoer_Wagner_Min_Cut
  {
  public:
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Weight = typename Distance::Distance_Type;

  private:
    Distance distance;
    SA sa;

    // Internal representation for contraction
    struct SWNode
    {
      DynList<Node*> members;     // Original nodes in this super-node
      size_t id;                  // Index for adjacency matrix
      bool merged;                // True if merged into another node
    };

    // Priority queue entry
    struct PQEntry
    {
      size_t node_id;
      Weight priority;

      bool operator<(const PQEntry& other) const
      {
        return priority < other.priority;  // Max-heap: higher priority first
      }
    };

    // Adjacency matrix for weights (dense representation for efficiency)
    std::vector<std::vector<Weight>> adj;
    std::vector<SWNode> nodes;
    size_t active_count;

    // Initialize internal structures from graph
    void init_from_graph(GT& g)
    {
      const size_t n = g.get_num_nodes();
      adj.assign(n, std::vector<Weight>(n, Weight(0)));
      nodes.resize(n);
      active_count = n;

      // Assign IDs to nodes
      size_t id = 0;
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        NODE_COUNTER(p) = id;
        nodes[id].members.append(p);
        nodes[id].id = id;
        nodes[id].merged = false;
        ++id;
      }

      // Build adjacency matrix
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        size_t src_id = NODE_COUNTER(g.get_src_node(a));
        size_t tgt_id = NODE_COUNTER(g.get_tgt_node(a));
        Weight w = distance(a);

        adj[src_id][tgt_id] += w;
        adj[tgt_id][src_id] += w;  // Undirected
      }
    }

    // Maximum Adjacency Search: returns (s, t, cut-of-phase weight)
    std::tuple<size_t, size_t, Weight> minimum_cut_phase()
    {
      const size_t n = nodes.size();

      // Find first active node to start
      size_t start = 0;
      while (start < n && nodes[start].merged)
        ++start;

      if (start >= n)
        return {0, 0, Weight(0)};

      // Priority (sum of weights to nodes in A)
      std::vector<Weight> key(n, Weight(0));
      std::vector<bool> in_A(n, false);

      // Use priority queue (max-heap)
      std::priority_queue<PQEntry> pq;

      // Initialize: add starting node to A
      in_A[start] = true;
      size_t last = start;
      size_t second_last = start;

      // Update priorities for neighbors of start
      for (size_t j = 0; j < n; ++j)
      {
        if (!nodes[j].merged && !in_A[j] && adj[start][j] > Weight(0))
        {
          key[j] = adj[start][j];
          pq.push({j, key[j]});
        }
      }

      // Add remaining nodes one by one
      for (size_t count = 1; count < active_count; ++count)
      {
        // Find node with maximum key not in A
        size_t next = n;
        while (!pq.empty())
        {
          auto top = pq.top();
          pq.pop();

          if (!in_A[top.node_id] && !nodes[top.node_id].merged &&
              top.priority == key[top.node_id])
          {
            next = top.node_id;
            break;
          }
        }

        if (next >= n)
        {
          // Graph is disconnected - find any unvisited node
          for (size_t j = 0; j < n; ++j)
          {
            if (!nodes[j].merged && !in_A[j])
            {
              next = j;
              break;
            }
          }
        }

        if (next >= n)
          break;

        // Add next to A
        second_last = last;
        last = next;
        in_A[next] = true;

        // Update keys for neighbors of next
        for (size_t j = 0; j < n; ++j)
        {
          if (!nodes[j].merged && !in_A[j] && adj[next][j] > Weight(0))
          {
            key[j] += adj[next][j];
            pq.push({j, key[j]});
          }
        }
      }

      // Cut-of-phase: weight of edges from last to A
      Weight cut_weight = key[last];

      return {second_last, last, cut_weight};
    }

    // Merge node t into node s
    void merge_nodes(size_t s, size_t t)
    {
      const size_t n = nodes.size();

      // Move members from t to s
      nodes[s].members.concat(nodes[t].members);
      nodes[t].merged = true;

      // Update adjacency: combine edges
      for (size_t j = 0; j < n; ++j)
      {
        if (j != s && j != t && !nodes[j].merged)
        {
          adj[s][j] += adj[t][j];
          adj[j][s] = adj[s][j];
        }
      }

      // Clear t's edges
      for (size_t j = 0; j < n; ++j)
      {
        adj[t][j] = Weight(0);
        adj[j][t] = Weight(0);
      }

      --active_count;
    }

  public:
    /** @brief Default constructor. */
    Stoer_Wagner_Min_Cut() = default;

    /** @brief Constructor with custom distance functor.
     *  @param _distance Functor to extract edge weights
     */
    explicit Stoer_Wagner_Min_Cut(Distance _distance)
      : distance(_distance)
    {}

    /** @brief Find minimum cut in graph.
     *
     *  Executes the Stoer-Wagner algorithm to find the exact minimum cut.
     *  For weighted graphs, returns the minimum total weight of cut edges.
     *  For unweighted graphs (weight=1), returns the minimum number of edges.
     *
     *  @param[in] g Input graph (must have at least 2 nodes)
     *  @param[out] vs First partition of vertices
     *  @param[out] vt Second partition of vertices
     *  @param[out] cut Arcs crossing the cut
     *  @return Weight of the minimum cut
     *
     *  @throw std::domain_error if graph has fewer than 2 nodes
     *
     *  @note Time complexity: O(nm + n² log n) with binary heap
     *  @note Space complexity: O(n²) for adjacency matrix
     */
    Weight operator()(GT& g,
                      DynList<Node*>& vs,
                      DynList<Node*>& vt,
                      DynList<Arc*>& cut)
    {
      const size_t n = g.get_num_nodes();
      ah_domain_error_if(n < 2) << "Graph must have at least 2 nodes";

      vs.empty();
      vt.empty();
      cut.empty();

      init_from_graph(g);

      Weight best_cut = std::numeric_limits<Weight>::max();

      // Main loop: n-1 phases
      while (active_count > 1)
      {
        auto [s, t, cut_weight] = minimum_cut_phase();
        (void)s;  // Suppress unused warning

        if (cut_weight < best_cut)
          best_cut = cut_weight;

        // Merge s and t
        merge_nodes(s, t);
      }

      // Reconstruct partitions
      // The best cut separates nodes[best_t].members from the rest
      // But nodes have been merged, so we need to track this differently

      // Rebuild: run algorithm again but stop at best cut
      init_from_graph(g);

      Weight target_cut = best_cut;
      DynList<Node*> cut_side;

      while (active_count > 1)
      {
        auto [s, t, cut_weight] = minimum_cut_phase();

        if (cut_weight == target_cut && cut_side.is_empty())
        {
          // This is our target cut - save t's side
          for (auto p : nodes[t].members)
            cut_side.append(p);
        }

        merge_nodes(s, t);
      }

      // Build partitions
      for (auto p : cut_side)
        vt.append(p);

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        if (!cut_side.exists([p](Node* q) { return p == q; }))
          vs.append(p);
      }

      // Find cut arcs
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        auto src = g.get_src_node(a);
        auto tgt = g.get_tgt_node(a);

        bool src_in_vs = vs.exists([src](Node* p) { return p == src; });
        bool tgt_in_vs = vs.exists([tgt](Node* p) { return p == tgt; });

        if (src_in_vs != tgt_in_vs)
          cut.append(a);
      }

      return best_cut;
    }

    /** @brief Find only the minimum cut weight (faster, no partition info).
     *
     *  If you only need the cut weight and not the actual partition,
     *  this method is slightly more efficient.
     *
     *  @param[in] g Input graph
     *  @return Weight of the minimum cut
     */
    Weight min_cut_weight(GT& g)
    {
      const size_t n = g.get_num_nodes();
      if (n < 2)
        return Weight(0);

      init_from_graph(g);

      Weight best_cut = std::numeric_limits<Weight>::max();

      while (active_count > 1)
      {
        auto [s, t, cut_weight] = minimum_cut_phase();
        best_cut = std::min(best_cut, cut_weight);
        merge_nodes(s, t);
      }

      return best_cut;
    }
  };

  /**
      Unit weight functor for unweighted graphs.

      Use this with Stoer_Wagner_Min_Cut when all edges should have
      weight 1 (i.e., minimize number of cut edges, not total weight).

      @code
      Stoer_Wagner_Min_Cut<GT, Unit_Weight<GT>> solver;
      size_t num_cut_edges = solver(g, S, T, cut_arcs);
      @endcode
   */
  template <class GT>
  struct Unit_Weight
  {
    using Distance_Type = size_t;
    size_t operator()(typename GT::Arc*) const { return 1; }
  };

} // namespace Aleph

# endif // STOER_WAGNER_H
