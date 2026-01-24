
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


/** @file tpl_graph_copy.H
 *  @brief Graph copy with explicit node mapping (not using cookies).
 *
 *  Provides a graph copy utility that maintains an explicit mapping from
 *  original nodes to copied nodes using a DynMapTree. This is useful when
 *  algorithms that use cookies need to operate on the copy without
 *  destroying the mapping relationship.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_GRAPH_COPY_H
#define TPL_GRAPH_COPY_H

#include <tpl_dynMapTree.H>
#include <tpl_graph.H>
#include <ah-errors.H>

namespace Aleph
{

/** Graph copy with explicit node mapping.

    This class creates a copy of a graph and maintains an explicit mapping
    from original nodes to their copies using a DynMapTree (not cookies).
    
    This is essential for algorithms like Johnson's that need to:
    1. Copy a graph
    2. Run algorithms that use cookies on the copy (Bellman-Ford, Dijkstra)
    3. Still be able to translate between original and copy nodes
    
    The mapping is unidirectional: original → copy.

    @tparam GT Graph type (List_Graph, List_Digraph, etc.)
    @tparam Tree Binary search tree type for the mapping (default Treap)

    Usage example:
    @code
    List_Digraph<Node, Arc> original;
    // ... build graph ...
    
    GraphCopyWithMapping<List_Digraph<Node, Arc>> copy(original);
    
    // Get copied node from original
    auto* orig_node = original.get_first_node();
    auto* copy_node = copy.get_copy(orig_node);
    
    // Access the copied graph
    auto& copied_graph = copy.get_graph();
    @endcode

    @ingroup Graphs
    @see copy_graph map_nodes Johnson
 */
template <class GT,
          template <class, class> class Tree = Treap>
class GraphCopyWithMapping
{
public:
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;
  using Node_Type = typename GT::Node_Type;
  using Arc_Type = typename GT::Arc_Type;

private:
  GT copied_graph;
  DynMapTree<Node*, Node*, Tree> node_map;  // original -> copy

  /// Build the copy and mapping
  void build_copy(const GT& src)
  {
    // Phase 1: Copy all nodes and build mapping
    for (typename GT::Node_Iterator it(src); it.has_curr(); it.next_ne())
      {
        Node* src_node = it.get_curr();
        Node* tgt_node = copied_graph.insert_node(src_node->get_info());
        node_map.insert(src_node, tgt_node);
      }

    // Phase 2: Copy all arcs using the node mapping
    for (typename GT::Arc_Iterator it(src); it.has_curr(); it.next_ne())
      {
        Arc* src_arc = it.get_curr();
        Node* src_src = const_cast<GT&>(src).get_src_node(src_arc);
        Node* src_tgt = const_cast<GT&>(src).get_tgt_node(src_arc);
        
        Node* tgt_src = node_map.find(src_src);
        Node* tgt_tgt = node_map.find(src_tgt);
        
        copied_graph.insert_arc(tgt_src, tgt_tgt, src_arc->get_info());
      }
  }

public:
  /** Construct a copy of the given graph with node mapping.

      @param[in] src The source graph to copy.
      @throw bad_alloc if there is not enough memory.
   */
  explicit GraphCopyWithMapping(const GT& src)
  {
    build_copy(src);
  }

  /// Default constructor (empty graph, empty mapping)
  GraphCopyWithMapping() = default;

  /// Move constructor
  GraphCopyWithMapping(GraphCopyWithMapping&&) = default;

  /// Move assignment
  GraphCopyWithMapping& operator=(GraphCopyWithMapping&&) = default;

  // Disable copy (graphs can be large)
  GraphCopyWithMapping(const GraphCopyWithMapping&) = delete;
  GraphCopyWithMapping& operator=(const GraphCopyWithMapping&) = delete;

  /** Get the copied graph.

      @return Reference to the copied graph.
   */
  GT& get_graph() noexcept { return copied_graph; }

  /** Get the copied graph (const version).

      @return Const reference to the copied graph.
   */
  const GT& get_graph() const noexcept { return copied_graph; }

  /** Get the copy of an original node.

      @param[in] orig Pointer to the original node.
      @return Pointer to the corresponding node in the copy.
      @throw std::domain_error if the node is not in the mapping.
   */
  Node* get_copy(Node* orig) const
  {
    auto* ptr = node_map.search(orig);
    ah_domain_error_if(ptr == nullptr)
      << "Node not found in mapping (not from original graph?)";
    return ptr->second;
  }

  /** Search for the copy of an original node (no exception).

      @param[in] orig Pointer to the original node.
      @return Pointer to the corresponding node in the copy, or nullptr if not found.
   */
  Node* search_copy(Node* orig) const noexcept
  {
    auto* ptr = node_map.search(orig);
    return ptr ? ptr->second : nullptr;
  }

  /** Check if an original node is in the mapping.

      @param[in] orig Pointer to the original node.
      @return true if the node is mapped, false otherwise.
   */
  bool has_copy(Node* orig) const noexcept
  {
    return node_map.search(orig) != nullptr;
  }

  /** Get the number of mapped nodes.

      @return Number of nodes in the mapping.
   */
  [[nodiscard]] size_t num_nodes() const noexcept { return node_map.size(); }

  /** Get the number of arcs in the copied graph.

      @return Number of arcs in the copy.
   */
  [[nodiscard]] size_t num_arcs() const noexcept { return copied_graph.get_num_arcs(); }

  /** Apply a function to each (original, copy) node pair.

      @param[in] op Function taking (Node* original, Node* copy).
   */
  template <typename Op>
  void for_each_mapping(Op op) const
  {
    node_map.for_each([&op](const auto& pair) {
      op(pair.first, pair.second);
    });
  }

  /** Insert a new node into the copied graph (not mapped).

      Useful for adding auxiliary nodes (like dummy nodes in Johnson's algorithm)
      that don't correspond to any original node.

      @param[in] info Node information.
      @return Pointer to the new node in the copied graph.
   */
  Node* insert_unmapped_node(const Node_Type& info = Node_Type())
  {
    return copied_graph.insert_node(info);
  }

  /** Insert a new node into the copied graph (not mapped, move version).

      @param[in] info Node information.
      @return Pointer to the new node in the copied graph.
   */
  Node* insert_unmapped_node(Node_Type&& info)
  {
    return copied_graph.insert_node(std::forward<Node_Type>(info));
  }

  /** Remove a node from the copied graph.

      If the node was mapped, the mapping entry is NOT removed (to keep the
      mapping consistent). Use with caution.

      @param[in] node Pointer to the node to remove.
   */
  void remove_node(Node* node)
  {
    copied_graph.remove_node(node);
  }

  /** Insert an arc into the copied graph.

      @param[in] src Source node (in the copy).
      @param[in] tgt Target node (in the copy).
      @param[in] info Arc information.
      @return Pointer to the new arc.
   */
  Arc* insert_arc(Node* src, Node* tgt, const Arc_Type& info = Arc_Type())
  {
    return copied_graph.insert_arc(src, tgt, info);
  }

  /** Remove an arc from the copied graph.

      @param[in] arc Pointer to the arc to remove.
   */
  void remove_arc(Arc* arc)
  {
    copied_graph.remove_arc(arc);
  }

  /** Clear the copied graph and mapping.
   */
  void clear()
  {
    node_map.empty();
    Aleph::clear_graph(copied_graph);
  }
};

} // namespace Aleph

#endif // TPL_GRAPH_COPY_H

