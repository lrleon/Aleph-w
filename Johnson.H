
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


/** @file Johnson.H
 *  @brief Johnson's algorithm for all-pairs shortest paths.
 *
 *  Implements Johnson's algorithm combining Bellman-Ford and Dijkstra
 *  for finding all-pairs shortest paths in sparse graphs, including
 *  graphs with negative edge weights. O(V²log V + VE) complexity.
 *
 *  ## Algorithm Overview
 *
 *  Johnson's algorithm works in the following steps:
 *  1. Add a dummy node q connected to all other nodes with zero-weight edges
 *  2. Run Bellman-Ford from q to compute node potentials h(v)
 *  3. Reweight all edges: w'(u,v) = w(u,v) + h(u) - h(v)
 *  4. For each source, run Dijkstra on the reweighted graph
 *  5. Adjust distances back: d(s,t) = d'(s,t) - h(s) + h(t)
 *
 *  ## Complexity
 *
 *  | Phase | Time |
 *  |-------|------|
 *  | Bellman-Ford | O(VE) |
 *  | V × Dijkstra | O(V(V log V + E)) |
 *  | **Total** | **O(V² log V + VE)** |
 *
 *  For sparse graphs (E = O(V)), this is O(V² log V), better than
 *  Floyd-Warshall's O(V³).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef JOHNSON_H
#define JOHNSON_H

#include <Dijkstra.H>
#include <Bellman_Ford.H>
#include <tpl_graph_copy.H>
#include <tpl_dynList.H>
#include <ah-errors.H>
#include <limits>
#include <type_traits>

namespace Aleph
{

/** Johnson's algorithm for all-pairs shortest paths.

    This class implements Johnson's algorithm for computing shortest paths
    between all pairs of vertices in a weighted directed graph. It handles
    graphs with negative edge weights (but not negative cycles).

    The algorithm is efficient for sparse graphs, with complexity
    O(V² log V + VE), compared to Floyd-Warshall's O(V³).

    @tparam GT Graph type (typically List_Digraph).
    @tparam Distance Arc weight accessor class. Must provide:
            - `Distance_Type`: The numeric type for weights
            - `operator()(Arc*)`: Returns the weight of an arc
    @tparam Ait Arc iterator template for traversing all arcs.
    @tparam NAit Node arc iterator template for outgoing arcs.
    @tparam SA Arc filter for internal iterators.

    Usage example:
    @code
    List_Digraph<Node, Arc> g;
    // ... build graph with possibly negative edges ...

    try {
      Johnson<List_Digraph<Node, Arc>> johnson(g);
      
      // Query shortest path between any pair
      auto path = johnson.find_path(src, tgt);
      auto dist = johnson.get_distance(src, tgt);
    }
    catch (const std::domain_error& e) {
      // Graph has a negative cycle
    }
    @endcode

    @warning The graph must be a **digraph**. For undirected graphs,
             convert to digraph first (each edge becomes two arcs).

    @see Bellman_Ford Dijkstra_Min_Paths Floyd_All_Shortest_Paths

    @ingroup Graphs
 */
template <class GT,
          class Distance = Dft_Dist<GT>,
          template <class, class> class Ait = Arc_Iterator,
          template <class, class> class NAit = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>>
class Johnson
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  using Distance_Type = typename Distance::Distance_Type;

private:
  const GT* orig_graph = nullptr;
  GraphCopyWithMapping<GT> graph_copy;  ///< Copy of graph with node mapping
  DynMapTree<Node*, Node*> copy_to_orig_node;
  DynMapTree<Arc*, Arc*> copy_to_orig_arc;
  DynMapTree<Node*, Distance_Type> h;   ///< Node potentials from Bellman-Ford
  Distance dist;
  SA sa;
  bool initialized = false;

  struct Reweighted_Dist
  {
    using Distance_Type = typename Johnson::Distance_Type;

    static_assert(std::is_convertible_v<typename GT::Arc_Type, Distance_Type>,
                  "Reweighted_Dist requires arc info convertible to Distance_Type");

    Distance_Type operator()(Arc* arc) const
    {
      return static_cast<Distance_Type>(arc->get_info());
    }
  };

  static Distance_Type checked_add(const Distance_Type& a, const Distance_Type& b)
  {
    return shortest_path_detail::checked_add(a, b);
  }

  static Distance_Type checked_sub(const Distance_Type& a, const Distance_Type& b)
  {
    if constexpr (std::is_integral_v<Distance_Type>)
      {
        ah_overflow_error_if(b > 0 and
                             a < std::numeric_limits<Distance_Type>::min() + b)
          << "Integer underflow in distance subtraction: " << a << " - " << b;

        ah_overflow_error_if(b < 0 and
                             a > std::numeric_limits<Distance_Type>::max() + b)
          << "Integer overflow in distance subtraction: " << a << " - " << b;
      }

    return a - b;
  }

  /** Reweight all arcs using node potentials.
   *  
   *  New weight: w'(u,v) = w(u,v) + h(u) - h(v)
   *  This guarantees all weights are non-negative.
   */
  void reweight_arcs()
  {
    GT& g = graph_copy.get_graph();
    
    for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next())
      {
        auto arc = it.get_curr();
        auto u = g.get_src_node(arc);
        auto v = g.get_tgt_node(arc);
        
        Distance_Type w = dist(arc);
        if (auto* orig = copy_to_orig_arc.search(arc))
          w = dist(orig->second);

        Distance_Type hu = h.find(u);
        Distance_Type hv = h.find(v);
        Distance_Type w_prime = checked_sub(checked_add(w, hu), hv);
        
        // Store reweighted value in the arc
        arc->get_info() = w_prime;
      }
  }

  void build_reverse_mappings(const GT& orig)
  {
    copy_to_orig_node.empty();
    copy_to_orig_arc.empty();

    graph_copy.for_each_mapping([this](Node* orig_node, Node* copy_node) {
      copy_to_orig_node.insert(copy_node, orig_node);
    });

    DynMapTree<std::pair<Node*, Node*>, DynList<Arc*>> buckets;
    GT& copy_g = graph_copy.get_graph();

    for (typename GT::Arc_Iterator it(copy_g); it.has_curr(); it.next_ne())
      {
        Arc* copy_arc = it.get_curr();
        Node* csrc = copy_g.get_src_node(copy_arc);
        Node* ctgt = copy_g.get_tgt_node(copy_arc);
        buckets[std::make_pair(csrc, ctgt)].append(copy_arc);
      }

    for (typename GT::Arc_Iterator it(orig); it.has_curr(); it.next_ne())
      {
        Arc* orig_arc = it.get_curr();
        Node* osrc = const_cast<GT&>(orig).get_src_node(orig_arc);
        Node* otgt = const_cast<GT&>(orig).get_tgt_node(orig_arc);

        Node* csrc = graph_copy.get_copy(osrc);
        Node* ctgt = graph_copy.get_copy(otgt);

        auto* bucket = buckets.search(std::make_pair(csrc, ctgt));
        ah_domain_error_if(bucket == nullptr or bucket->second.is_empty())
          << "Arc mapping mismatch between original and copied graph";

        Arc* copy_arc = bucket->second.remove_first();
        copy_to_orig_arc.insert(copy_arc, orig_arc);
      }

    ah_domain_error_if(copy_to_orig_arc.size() != copy_g.get_num_arcs())
      << "Arc mapping mismatch between original and copied graph";
  }

  Path<GT> map_copy_path_to_original(const Path<GT>& copy_path) const
  {
    ah_domain_error_if(orig_graph == nullptr)
      << "Original graph not available";

    Path<GT> orig_path(*orig_graph);

    if (copy_path.is_empty())
      return orig_path;

    auto it = copy_path.get_it();
    auto* node_pair = copy_to_orig_node.search(it.get_current_node());
    ah_domain_error_if(node_pair == nullptr)
      << "Node not found in mapping";

    orig_path.init(node_pair->second);

    for (; it.has_curr() and it.has_current_arc(); it.next())
      {
        auto* arc_pair = copy_to_orig_arc.search(it.get_current_arc());
        ah_domain_error_if(arc_pair == nullptr)
          << "Arc not found in mapping";

        orig_path.append(arc_pair->second);
      }

    return orig_path;
  }

public:
  /** Construct a Johnson's all-pairs shortest paths executor.

      The constructor:
      1. Copies the input graph (preserving node mapping)
      2. Runs Bellman-Ford to compute node potentials
      3. Reweights all edges to be non-negative

      @param[in] g The graph to operate on.
      @param[in] d Arc-weight accessor (default constructed).
      @param[in] __sa Arc filter for internal iterators (default constructed).
      @throw std::bad_alloc if there is not enough memory.
      @throw std::domain_error if a negative cycle is detected.
   */
  explicit Johnson(const GT& g, Distance d = Distance(), SA __sa = SA())
    : orig_graph(&g), graph_copy(g), dist(d), sa(__sa)
  {
    GT& gc = graph_copy.get_graph();

    build_reverse_mappings(g);
    
    // Compute node potentials using Bellman-Ford
    // This will throw domain_error if negative cycle exists
    Bellman_Ford<GT, Distance, Ait, NAit, SA> bf(gc, dist, sa);
    h = bf.compute_nodes_weights();
    
    // Reweight all arcs
    reweight_arcs();
    
    initialized = true;
  }

  /// Default constructor (uninitialized state)
  Johnson() = default;

  /** Check if the algorithm has been initialized.
   *  @return true if ready for queries, false otherwise.
   */
  [[nodiscard]] bool is_initialized() const noexcept { return initialized; }

  /** Get the shortest distance from source to target.

      @param[in] src Source node (from original graph).
      @param[in] tgt Target node (from original graph).
      @return The shortest distance from src to tgt, or infinity if unreachable.
      @throw std::domain_error if not initialized or nodes not found.
   */
  Distance_Type get_distance(Node* src, Node* tgt)
  {
    ah_domain_error_if(!initialized) << "Johnson not initialized";
    ah_domain_error_if(src == nullptr or tgt == nullptr) << "Source or target node is null";

    // Same node - distance is 0
    if (src == tgt)
      return Distance_Type(0);

    // Get corresponding nodes in the copy
    Node* csrc = graph_copy.get_copy(src);
    Node* ctgt = graph_copy.get_copy(tgt);
    
    GT& gc = graph_copy.get_graph();
    
    // Run Dijkstra from csrc on reweighted graph
    Dijkstra_Min_Paths<GT, Reweighted_Dist, NAit, SA> dijkstra(Reweighted_Dist(), sa);
    dijkstra.paint_min_paths_tree(gc, csrc);
    
    if (not IS_NODE_VISITED(ctgt, Aleph::Spanning_Tree))
      return std::numeric_limits<Distance_Type>::infinity();

    // Get reweighted distance (may throw if unreachable)
    Path<GT> path(gc);
    Distance_Type d_prime = dijkstra.get_min_path(ctgt, path);
    
    // Adjust back: d(s,t) = d'(s,t) - h(s) + h(t)
    Distance_Type hs = h.find(csrc);
    Distance_Type ht = h.find(ctgt);
    
    return d_prime - hs + ht;
  }

  /** Find the shortest path from source to target.

      @param[in] src Source node (from original graph).
      @param[in] tgt Target node (from original graph).
      @return Path in the original graph from src to tgt, or an empty path if unreachable.
      @throw std::domain_error if not initialized or nodes not found.
   */
  Path<GT> find_path(Node* src, Node* tgt)
  {
    ah_domain_error_if(!initialized) << "Johnson not initialized";
    ah_domain_error_if(src == nullptr or tgt == nullptr) << "Source or target node is null";

    // Get corresponding nodes in the copy
    Node* csrc = graph_copy.get_copy(src);
    Node* ctgt = graph_copy.get_copy(tgt);
    
    GT& gc = graph_copy.get_graph();
    
    // Run Dijkstra from csrc on reweighted graph
    Dijkstra_Min_Paths<GT, Reweighted_Dist, NAit, SA> dijkstra(Reweighted_Dist(), sa);
    dijkstra.paint_min_paths_tree(gc, csrc);

    if (not IS_NODE_VISITED(ctgt, Aleph::Spanning_Tree))
      return Path<GT>(*orig_graph);
    
    // Get path in copy graph
    Path<GT> copy_path(gc);
    dijkstra.get_min_path(ctgt, copy_path);

    return map_copy_path_to_original(copy_path);
  }

  /** Compute the shortest distance for all pairs.

      @return A map where key is (src, tgt) pair and value is shortest distance.
      @throw std::domain_error if not initialized.
   */
  DynMapTree<std::pair<Node*, Node*>, Distance_Type> 
  compute_all_pairs_distances()
  {
    ah_domain_error_if(!initialized) << "Johnson not initialized";
    
    DynMapTree<std::pair<Node*, Node*>, Distance_Type> result;
    GT& gc = graph_copy.get_graph();
    
    // For each node as source
    graph_copy.for_each_mapping([&](Node* orig_src, Node* copy_src) {
      // Run Dijkstra from copy_src
      Dijkstra_Min_Paths<GT, Reweighted_Dist, NAit, SA> dijkstra(Reweighted_Dist(), sa);
      dijkstra.paint_min_paths_tree(gc, copy_src);
      
      Distance_Type hs = h.find(copy_src);
      
      // Get distance to all targets
      graph_copy.for_each_mapping([&](Node* orig_tgt, Node* copy_tgt) {
        if (not IS_NODE_VISITED(copy_tgt, Aleph::Spanning_Tree))
          {
            result.insert(std::make_pair(orig_src, orig_tgt),
                          std::numeric_limits<Distance_Type>::infinity());
            return;
          }

        Path<GT> path(gc);
        Distance_Type d_prime = dijkstra.get_min_path(copy_tgt, path);
        Distance_Type ht = h.find(copy_tgt);
        Distance_Type d = d_prime - hs + ht;
        result.insert(std::make_pair(orig_src, orig_tgt), d);
      });
    });
    
    return result;
  }

  /** Get the node potential (h value) for a node.

      The potential h(v) represents the shortest distance from the dummy
      source to v, computed by Bellman-Ford.

      @param[in] node Node from the original graph.
      @return The potential h(node).
      @throw std::domain_error if not initialized or node not found.
   */
  Distance_Type get_potential(Node* node) const
  {
    ah_domain_error_if(!initialized) << "Johnson not initialized";
    Node* copy_node = graph_copy.get_copy(node);
    return h.find(copy_node);
  }

  /** Get access to the internal reweighted graph.

      Useful for debugging or advanced operations.

      @return Reference to the internal graph copy.
   */
  GT& get_reweighted_graph() noexcept { return graph_copy.get_graph(); }

  /** Get const access to the internal reweighted graph.
   */
  const GT& get_reweighted_graph() const noexcept { return graph_copy.get_graph(); }

  /** Get the copy of a node in the reweighted graph.

      @param[in] orig Original node.
      @return Corresponding node in the reweighted graph.
   */
  Node* get_copy_node(Node* orig) const { return graph_copy.get_copy(orig); }
};

} // namespace Aleph

#endif // JOHNSON_H
