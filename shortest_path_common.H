
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


/** @file shortest_path_common.H
 *  @brief Common utilities and base class for shortest path algorithms.
 *
 *  This file contains shared functionality used by Dijkstra's algorithm,
 *  A*, and other shortest path algorithms. It provides:
 *
 *  - Overflow-checked arithmetic operations
 *  - Common cookie structures for node/arc information
 *  - Initialize/Destroy functors for cookie management
 *  - Base class with common algorithm infrastructure
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef SHORTEST_PATH_COMMON_H
#define SHORTEST_PATH_COMMON_H

#include <limits>
#include <type_traits>
#include <ah-errors.H>
#include <tpl_graph.H>
#include <tpl_find_path.H>

namespace Aleph
{

// =============================================================================
// Cookie Access Macros (internal use)
// =============================================================================

// These macros provide unified access to node/arc cookies across algorithms

/// Convert node cookie to Node_Info pointer
#define SP_NODE_INFO(p) (static_cast<typename Shortest_Path_Base::Node_Info*>(NODE_COOKIE(p)))

/// Convert node cookie to Tree_Node_Info pointer
#define SP_TREE_NODE_INFO(p) (static_cast<typename Shortest_Path_Base::Tree_Node_Info*>(NODE_COOKIE(p)))

/// Access accumulated distance from node cookie
#define SP_ACC(p) (SP_NODE_INFO(p)->dist)

/// Access heap node handle from node cookie
#define SP_HEAPNODE(p) (SP_NODE_INFO(p)->heap_node)

/// Access parent pointer from node cookie
#define SP_PARENT(p) (SP_NODE_INFO(p)->ret_node)

/// Access tree node mapping from node cookie
#define SP_TREENODE(p) (SP_TREE_NODE_INFO(p)->tree_node)

/// Convert arc cookie to Arc_Info pointer
#define SP_ARC_INFO(p) (static_cast<typename Shortest_Path_Base::Arc_Info*>(ARC_COOKIE(p)))

/// Convert arc cookie to Tree_Arc_Info pointer
#define SP_TREE_ARC_INFO(p) (static_cast<typename Shortest_Path_Base::Tree_Arc_Info*>(ARC_COOKIE(p)))

/// Access arc potential from arc cookie
#define SP_POT(p) (SP_ARC_INFO(p)->pot)

/// Access tree arc mapping from arc cookie
#define SP_TREEARC(p) (SP_TREE_ARC_INFO(p)->tree_arc)

/// Get arc distance using Distance functor
#define SP_ARC_DIST(p) (Distance()(p))


// =============================================================================
// Overflow-Checked Arithmetic
// =============================================================================

/** @brief Namespace for shortest path algorithm implementation details. */
namespace shortest_path_detail
{

/** @brief Safely add two distance values with overflow checking.
 *
 *  For integral types, checks for overflow/underflow before performing
 *  the addition. For floating-point types, performs regular addition.
 *
 *  @tparam T Distance type (integral or floating-point).
 *  @param a First operand.
 *  @param b Second operand.
 *  @return Sum of a and b.
 *  @throw std::overflow_error If integer overflow/underflow would occur.
 */
template <typename T>
[[nodiscard]] inline T checked_add(const T& a, const T& b)
{
  if constexpr (std::is_integral_v<T>)
    {
      ah_overflow_error_if(b > 0 and a > std::numeric_limits<T>::max() - b)
        << "Integer overflow in distance addition: " << a << " + " << b;

      ah_overflow_error_if(b < 0 and a < std::numeric_limits<T>::min() - b)
        << "Integer underflow in distance addition: " << a << " + " << b;
    }

  return a + b;
}

} // namespace shortest_path_detail


// =============================================================================
// Base Class for Shortest Path Algorithms
// =============================================================================

/** @brief Base class providing common infrastructure for shortest path algorithms.
 *
 *  This template class encapsulates shared functionality between Dijkstra's
 *  algorithm, A*, and similar shortest path algorithms. It provides:
 *
 *  - Cookie structures for storing algorithm state in nodes/arcs
 *  - Initialize/Destroy functors for memory management
 *  - Common state management (init/uninit, painted flag, etc.)
 *  - Distance calculation from painted paths
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance accessor functor.
 *  @tparam Itor Arc iterator template.
 *  @tparam SA Arc filter for iterators.
 *  @tparam HeapT Priority queue template.
 *
 *  @ingroup Graphs
 */
template <class GT,
          class Distance,
          template <typename, class> class Itor,
          class SA,
          template <class, class, class> class HeapT>
class Shortest_Path_Base
{
public:
  // =========================================================================
  // Cookie Structures
  // =========================================================================

  /** @brief Information stored in arc cookies for painting version.
   *
   *  Stores the potential (priority) value used for heap ordering.
   *  For Dijkstra: pot = accumulated distance to target.
   *  For A*: pot = g(n) + h(n) (f-cost to target).
   */
  struct Arc_Info
  {
    typename Distance::Distance_Type pot = 0;  ///< Arc potential (priority)
  };

  /** @brief Extended arc info with tree arc mapping.
   *
   *  Used when building a separate spanning tree (not just painting).
   */
  struct Tree_Arc_Info : public Arc_Info
  {
    typename GT::Arc* tree_arc = nullptr;  ///< Corresponding arc in spanning tree
  };

  /** @brief Information stored in node cookies for painting version.
   *
   *  Stores accumulated distance, heap handle, and parent pointer
   *  for path reconstruction.
   */
  struct Node_Info
  {
    typename Distance::Distance_Type dist = 0;  ///< Accumulated distance from start
    void* heap_node = nullptr;                   ///< Handle in priority queue
    void* ret_node = nullptr;                    ///< Parent node (for backtracking)
  };

  /** @brief Extended node info with tree node mapping.
   *
   *  Used when building a separate spanning tree.
   */
  struct Tree_Node_Info : public Node_Info
  {
    typename GT::Node* tree_node = nullptr;  ///< Corresponding node in spanning tree
  };

  // =========================================================================
  // Heap Access Functor
  // =========================================================================

  /** @brief Functor to access heap node handle from node cookie. */
  struct Heap_Info
  {
    void*& operator()(typename GT::Node* p) const noexcept
    {
      return SP_HEAPNODE(p);
    }
  };

  // =========================================================================
  // Get Potential Functor
  // =========================================================================

  /** @brief Functor to get arc potential for heap ordering. */
  struct Get_Potential_Arc : public Distance
  {
    Get_Potential_Arc() = default;

    Get_Potential_Arc(Distance& d) noexcept : Distance(d) { }

    typename Distance::Distance_Type
    operator()(typename GT::Arc* a) const noexcept
    {
      return SP_POT(a);
    }
  };

  // =========================================================================
  // Initialize Functors
  // =========================================================================

  /** @brief Node initialization for painting version. */
  struct Initialize_Node
  {
    void operator()(const GT& g, typename GT::Node* p) const
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Node_Info;
    }
  };

  /** @brief Arc initialization for painting version. */
  struct Initialize_Arc
  {
    void operator()(const GT& g, typename GT::Arc* a) const
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Arc_Info;
      SP_POT(a) = 0;
    }
  };

  /** @brief Node initialization for tree-building version. */
  struct Initialize_Tree_Node
  {
    void operator()(const GT& g, typename GT::Node* p) const
    {
      g.reset_bit(p, Aleph::Spanning_Tree);
      NODE_COOKIE(p) = new Tree_Node_Info;
    }
  };

  /** @brief Arc initialization for tree-building version. */
  struct Initialize_Tree_Arc
  {
    void operator()(const GT& g, typename GT::Arc* a) const
    {
      g.reset_bit(a, Aleph::Spanning_Tree);
      ARC_COOKIE(a) = new Tree_Arc_Info;
      SP_POT(a) = 0;
      SP_TREEARC(a) = nullptr;
    }
  };

  // =========================================================================
  // Destroy Functors
  // =========================================================================

  /** @brief Node cleanup for painting version. */
  struct Destroy_Node
  {
    void operator()(const GT&, typename GT::Node* p) const noexcept
    {
      void* tmp = SP_PARENT(p);
      delete SP_NODE_INFO(p);
      NODE_COOKIE(p) = tmp;
    }
  };

  /** @brief Arc cleanup for painting version. */
  struct Destroy_Arc
  {
    void operator()(const GT&, typename GT::Arc* ga) const noexcept
    {
      delete SP_ARC_INFO(ga);
    }
  };

  /** @brief Node cleanup and mapping for tree-building version. */
  struct Destroy_Tree_Node
  {
    void operator()(const GT&, typename GT::Node* p) const noexcept
    {
      auto aux = SP_TREE_NODE_INFO(p);
      auto tp = SP_TREENODE(p);
      if (tp != nullptr)
        {
          NODE_COOKIE(p) = NODE_COOKIE(tp) = nullptr;
          GT::map_nodes(p, tp);
        }
      else
        NODE_COOKIE(p) = nullptr;

      delete aux;
    }
  };

  /** @brief Arc cleanup and mapping for tree-building version. */
  struct Destroy_Tree_Arc
  {
    void operator()(const GT&, typename GT::Arc* ga) const noexcept
    {
      auto aux = SP_TREE_ARC_INFO(ga);
      typename GT::Arc* ta = SP_TREEARC(ga);
      if (ta != nullptr)
        {
          assert(IS_ARC_VISITED(ga, Aleph::Spanning_Tree));
          GT::map_arcs(ga, ta);
        }

      delete aux;
    }
  };

  // =========================================================================
  // Type Aliases
  // =========================================================================

  using Heap = HeapT<GT, Get_Potential_Arc, Heap_Info>;

protected:
  // =========================================================================
  // Member Variables
  // =========================================================================

  SA sa;                           ///< Arc filter
  Get_Potential_Arc get_pot;       ///< Potential accessor
  Heap heap;                       ///< Priority queue
  bool painted = false;            ///< Whether graph has been painted
  GT* ptr_g = nullptr;             ///< Pointer to the graph
  typename GT::Node* s = nullptr;  ///< Start node

  // =========================================================================
  // Protected Methods
  // =========================================================================

  /** @brief Initialize algorithm state.
   *
   *  @tparam IN Node initializer functor type.
   *  @tparam IA Arc initializer functor type.
   *  @param g The graph.
   *  @param start The starting node.
   */
  template <class IN, class IA>
  void init(const GT& g, typename GT::Node* start)
  {
    heap.empty();

    ptr_g = &const_cast<GT&>(g);
    s = start;

    Operate_On_Nodes<GT, IN>()(g);
    (Operate_On_Arcs<GT, IA>(sa))(g);
  }

  /** @brief Cleanup algorithm state.
   *
   *  @tparam DN Node destroyer functor type.
   *  @tparam DA Arc destroyer functor type.
   */
  template <class DN, class DA>
  void uninit()
  {
    // Clear heap first to avoid use-after-free when heap destructor
    // tries to compare arcs whose cookies have been freed
    heap.empty();

    Operate_On_Nodes<GT, DN>()(*ptr_g);
    (Operate_On_Arcs<GT, DA, SA>(sa))(*ptr_g);
  }

  /** @brief Checked addition to prevent integer overflow.
   *
   *  @param a First operand.
   *  @param b Second operand.
   *  @return Sum of a and b.
   *  @throw std::overflow_error If overflow would occur.
   */
  typename Distance::Distance_Type
  checked_add(const typename Distance::Distance_Type& a,
              const typename Distance::Distance_Type& b) const
  {
    return shortest_path_detail::checked_add(a, b);
  }

public:
  // =========================================================================
  // Constructors
  // =========================================================================

  /** @brief Default constructor.
   *
   *  @param dist Distance functor for arc weights.
   *  @param __sa Arc filter for iterators.
   */
  Shortest_Path_Base(Distance dist = Distance(), SA __sa = SA())
    : sa(__sa), get_pot(dist), heap(get_pot, Heap_Info()),
      painted(false), ptr_g(nullptr), s(nullptr)
  {
    // empty
  }

  // =========================================================================
  // State Getters
  // =========================================================================

  /** @brief Check if a computation has been performed.
   *  @return true if a graph has been processed, false otherwise.
   */
  [[nodiscard]] bool has_computation() const noexcept { return ptr_g != nullptr; }

  /** @brief Check if the graph has been painted.
   *  @return true if a painting method was called, false otherwise.
   */
  [[nodiscard]] bool is_painted() const noexcept { return painted; }

  /** @brief Get the start node of the last computation.
   *  @return Pointer to the start node, or nullptr if no computation done.
   */
  [[nodiscard]] typename GT::Node* get_start_node() const noexcept { return s; }

  /** @brief Get the graph of the last computation.
   *  @return Pointer to the graph, or nullptr if no computation done.
   */
  [[nodiscard]] GT* get_graph() const noexcept { return ptr_g; }

  // =========================================================================
  // Distance Calculation
  // =========================================================================

  /** @brief Gets the accumulated distance to a node after painting.
   *
   *  After painting the graph with a shortest path tree, this method
   *  traverses the path from the start node to the given node and
   *  computes the total distance.
   *
   *  @param[in] node The destination node.
   *  @return The accumulated distance from start to node.
   *  @throw domain_error If the graph has not been painted or if node
   *         is not reachable from the start node.
   *
   *  @note Time complexity: O(path_length)
   */
  typename Distance::Distance_Type
  get_distance(typename GT::Node* node)
  {
    ah_domain_error_if(not painted) << "Graph has not been painted";
    ah_domain_error_if(node == nullptr) << "node cannot be null";

    if (node == s)
      return 0;

    ah_domain_error_if(not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
      << "node is not reachable from start";

    typename Distance::Distance_Type total = 0;

    for (auto curr = node; curr != s;)
      {
        auto parent = static_cast<typename GT::Node*>(NODE_COOKIE(curr));
        if (parent == nullptr)
          break;

        for (Itor<GT, SA> it(parent, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            auto tgt = it.get_tgt_node();
            if (tgt == curr and IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              {
                total += Distance()(arc);
                break;
              }
          }
        curr = parent;
      }

    return total;
  }

  // =========================================================================
  // Path Extraction from Painted Graph
  // =========================================================================

  /** @brief Extracts a shortest path to end from a previously painted graph.
   *
   *  Takes a graph on which shortest paths have been painted and extracts
   *  the shortest path to end, storing it in path.
   *
   *  @param[in] end Destination node of the path.
   *  @param[out] path Path where the shortest path will be stored.
   *  @return The total cost of the path.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If the graph has not been previously painted.
   */
  typename Distance::Distance_Type
  get_min_path(typename GT::Node* end, Path<GT>& path)
  {
    ah_domain_error_if(ptr_g == nullptr) << "Min path has not been computed";
    ah_domain_error_if(not painted) << "Graph has not been painted";

    return Aleph::get_min_path<GT, Distance>(s, end, path);
  }

  // =========================================================================
  // Copy Painted Tree
  // =========================================================================

  /** @brief Distance totalizer class for path copying. */
  struct Total
  {
    typename Distance::Distance_Type dist = 0;

    Total() noexcept = default;

    bool operator()(typename GT::Arc* a) noexcept
    {
      dist += Distance()(a);
      return true;
    }
  };

  /** @brief Extracts the painted shortest paths tree and puts it in tree.
   *
   *  @param[in] g The previously painted graph.
   *  @param[out] tree The graph where the spanning tree will be copied.
   *  @return The total distance of the spanning tree.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If the graph has not been previously painted.
   */
  typename Distance::Distance_Type
  copy_painted_min_paths_tree(GT& g, GT& tree)
  {
    ah_domain_error_if(not painted) << "Graph has not been painted";

    using Paint_Filt = Painted_Min_Spanning_Tree<GT, Distance>;
    Paint_Filt paint_filter;
    (Copy_Graph<GT, Dft_Show_Node<GT>, Paint_Filt>(paint_filter))(tree, g);

    return paint_filter.dist;
  }

  /** @brief Extracts path from tree to end node.
   *
   *  @param[in] tree The spanning tree built by compute_min_paths_tree.
   *  @param[in] end Destination node in the original graph.
   *  @param[out] path Path where result will be stored (in original graph).
   *  @return The total cost of the path.
   */
  typename Distance::Distance_Type
  get_min_path(const GT& tree, typename GT::Node* end, Path<GT>& path)
  {
    ah_domain_error_if(ptr_g == nullptr) << "Min path has not been computed";

    auto ts = mapped_node<GT>(s);
    auto te = mapped_node<GT>(end);

    Path<GT> tree_path(tree);
    (Find_Path_Depth_First<GT, Itor>(sa))(tree, ts, te, tree_path);

    // Build path in original graph
    path.empty();
    path.init(s);

    for (auto it = tree_path.get_it(); it.has_curr(); it.next())
      {
        auto tree_node = it.get_curr();
        if (tree_node != ts)  // Skip start node (already added via init)
          path.append(mapped_node<GT>(tree_node));
      }

    // Sum arc weights from the tree path
    typename Distance::Distance_Type total_dist = 0;
    tree_path.for_each_arc([&total_dist](typename GT::Arc* a)
      {
        total_dist += Distance()(a);
      });

    return total_dist;
  }
};


// Cleanup macros (they are redefined in derived classes with appropriate types)
#undef SP_NODE_INFO
#undef SP_TREE_NODE_INFO
#undef SP_ACC
#undef SP_HEAPNODE
#undef SP_PARENT
#undef SP_TREENODE
#undef SP_ARC_INFO
#undef SP_TREE_ARC_INFO
#undef SP_POT
#undef SP_TREEARC
#undef SP_ARC_DIST

} // namespace Aleph

#endif // SHORTEST_PATH_COMMON_H
