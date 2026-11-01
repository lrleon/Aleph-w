
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

# include <ah-graph-concepts.H>

#include <limits>
#include <type_traits>
#include <ah-errors.H>
#include <tpl_graph.H>
#include <tpl_find_path.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynSetTree.H>
#include <cookie_guard.H>

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
template <AlephGraph GT,
          ArcDistance<GT> Distance,
          template <typename, class> class Itor,
          ArcFilter<GT> SA,
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

    /// Frees the cookie of `p` and leaves it null.
    static void release(typename GT::Node* p) noexcept
    {
      delete SP_NODE_INFO(p);
      NODE_COOKIE(p) = nullptr;
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

    /// Frees the cookie of `a` and leaves it null.
    static void release(typename GT::Arc* a) noexcept
    {
      delete SP_ARC_INFO(a);
      ARC_COOKIE(a) = nullptr;
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

    /// Frees the cookie of `p` and leaves it null.
    static void release(typename GT::Node* p) noexcept
    {
      delete SP_TREE_NODE_INFO(p);
      NODE_COOKIE(p) = nullptr;
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

    /// Frees the cookie of `a` and leaves it null.
    static void release(typename GT::Arc* a) noexcept
    {
      delete SP_TREE_ARC_INFO(a);
      ARC_COOKIE(a) = nullptr;
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
      ARC_COOKIE(ga) = nullptr;
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
      ARC_COOKIE(ga) = nullptr; // else map_arcs() would chain ta to aux
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
  Distance distance;               ///< Arc weights
  Get_Potential_Arc get_pot;       ///< Potential accessor
  Heap heap;                       ///< Priority queue
  bool painted = false;            ///< Whether graph has been painted
  GT* ptr_g = nullptr;             ///< Pointer to the graph
  typename GT::Node* s = nullptr;  ///< Start node
  bool beyond_max = false;         ///< A sum of the painting went beyond max()

  // =========================================================================
  // Protected Methods
  // =========================================================================

  /** @brief Initialize algorithm state.
   *
   *  Gives every node and every arc shown by the filter a new cookie. A
   *  previous painting is lost, since its bits and cookies are reset.
   *
   *  @tparam IN Node initializer functor type.
   *  @tparam IA Arc initializer functor type.
   *  @param g The graph.
   *  @param start The starting node.
   *  @throw std::bad_alloc If there is not enough memory. The cookies
   *         already allocated are freed and left null.
   */
  template <class IN, class IA>
  void init(const GT& g, typename GT::Node* start)
  {
    heap.empty();
    painted = false;
    beyond_max = false;

    ptr_g = &const_cast<GT&>(g);
    s = start;

    // the first num_nodes nodes and num_arcs arcs have a new cookie
    size_t num_nodes = 0, num_arcs = 0;
    try
      {
        for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne(), ++num_nodes)
          IN()(g, it.get_curr());
        // A hidden arc may have the bit of an earlier painting, which the
        // tree, unfiltered, would take; one pass, with the filter called
        // as an Arc_Iterator<GT, SA> calls it
        for (Arc_Iterator<GT> it(g); it.has_curr(); it.next_ne())
          if (auto a = it.get_curr_ne(); sa(a))
            {
              IA()(g, a);
              ++num_arcs;
            }
          else
            g.reset_bit(a, Aleph::Spanning_Tree);
      }
    catch (...)
      {
        Node_Iterator<GT> nit(g);
        for (size_t i = 0; i < num_nodes; ++i, nit.next_ne())
          IN::release(nit.get_curr());
        Arc_Iterator<GT, SA> ait(g, sa);
        for (size_t i = 0; i < num_arcs; ++i, ait.next_ne())
          IA::release(ait.get_curr());
        throw;
      }
  }

  /** @brief Returns a guard that calls uninit() when it goes out of scope.

      The algorithms take one right after init(), so that the cookies are
      freed however they end: by returning or by an exception.

      @tparam DN Node destroyer functor type.
      @tparam DA Arc destroyer functor type.
      @return The guard, which must live until the algorithm ends.
  */
  template <class DN, class DA>
  [[nodiscard]] auto uninit_on_exit()
  {
    return Scope_Guard(*ptr_g, [this](const GT &) { uninit<DN, DA>(); });
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

  /** @brief Adds a tentative distance that may go beyond max().
   *
   *  Sets `out` to `a + b` and returns true, but for an integral type
   *  whose sum goes beyond max(): then no distance that fits goes through
   *  it, and it returns false and remembers it in beyond_max, which
   *  check_beyond_max() looks at when the painting ends.
   *
   *  @param a First operand.
   *  @param b Second operand.
   *  @param[out] out The sum, if it fits.
   *  @return true if the sum fits; false if it goes beyond max().
   *  @throw std::overflow_error If the sum goes below lowest().
   */
  [[gnu::always_inline]] bool
  add_if_fits(const typename Distance::Distance_Type& a,
              const typename Distance::Distance_Type& b,
              typename Distance::Distance_Type& out)
  {
    using T = typename Distance::Distance_Type;
    if constexpr (std::is_integral_v<T>)
      {
        if (b > 0 and a > std::numeric_limits<T>::max() - b) [[unlikely]]
          {
            beyond_max = true;
            return false;
          }
        ah_overflow_error_if(b < 0 and a < std::numeric_limits<T>::min() - b)
          << "Integer underflow in distance addition: " << a << " + " << b;
      }
    out = a + b; // the checks of checked_add(), each once
    return true;
  }

  /** @brief Adds a priority, which only orders the heap: for an integral
   *  type, a sum beyond max() gives max().
   *
   *  @throw std::overflow_error If the sum goes below lowest().
   */
  [[gnu::always_inline]] typename Distance::Distance_Type
  saturated_add(const typename Distance::Distance_Type& a,
                const typename Distance::Distance_Type& b) const
  {
    using T = typename Distance::Distance_Type;
    if constexpr (std::is_integral_v<T>)
      {
        if (b > 0 and a > std::numeric_limits<T>::max() - b) [[unlikely]]
          return std::numeric_limits<T>::max();
        ah_overflow_error_if(b < 0 and a < std::numeric_limits<T>::min() - b)
          << "Integer underflow in distance addition: " << a << " + " << b;
      }
    return a + b; // the checks of checked_add(), each once
  }

  /** @brief After a painting in which a sum went beyond max(), throws if
   *  a node that `start` reaches was left out, or `end` if it is given.
   *
   *  Every path to such a node has a sum beyond max(), so that its
   *  distance does not fit. The walk goes through the arcs that the
   *  filter shows, without touching the painting.
   *
   *  @throw std::overflow_error If such a node exists.
   *  @throw std::bad_alloc If there is not enough memory.
   */
  void check_beyond_max(const GT& g, typename GT::Node* start,
                        typename GT::Node* end = nullptr) const
  {
    using Node = typename GT::Node;
    DynSetTree<Node*> seen;
    DynListQueue<Node*> queue;
    seen.insert(start);
    queue.put(start);
    while (not queue.is_empty())
      {
        Node* p = queue.get();
        ah_overflow_error_if(end == nullptr ? not IS_NODE_VISITED(p, Aleph::Spanning_Tree)
                                            : p == end)
          << "the distance from the start to a node does not fit in its type";
        for (Itor<GT, SA> it(p, sa); it.has_curr(); it.next_ne())
          if (Node* t = it.get_tgt_node(); seen.insert(t) != nullptr)
            queue.put(t);
      }
    (void) g;
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
    : sa(__sa), distance(dist), get_pot(dist), heap(get_pot, Heap_Info()),
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
   *
   *  Every computation resets the bits and cookies of the previous one,
   *  so only a painting that ended normally can be used.
   *
   *  @return true if the last computation was a painting and ended
   *          normally, false otherwise.
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
                total += distance(arc);
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

    return Aleph::get_min_path<GT, Distance>(s, end, path, distance);
  }

  // =========================================================================
  // Copy Painted Tree
  // =========================================================================

  /** @brief Distance totalizer class for path copying.
   *
   *  It keeps a copy of its distance accessor, which may have state, or no
   *  default constructor.
   */
  struct Total
  {
    typename Distance::Distance_Type dist = 0;

    /** @brief Distance accessor for the arc weights. */
    Distance distance{};

    /** @brief Builds the totalizer with a distance accessor built by
     *  `Distance()`.
     */
    Total() = default;

    /** @brief Builds the totalizer with a distance accessor.
     *
     *  @param[in] __distance Distance accessor for the arc weights.
     */
    explicit Total(Distance __distance) : distance(__distance) {}

    /** @brief Adds the weight of an arc to `dist`.
     *
     *  @param[in] a The arc.
     *  @return true.
     *  @throw anything that the distance throws; `dist` is then unchanged.
     */
    bool operator()(typename GT::Arc* a)
    {
      dist += distance(a);
      return true;
    }
  };

  /** @brief Arc filter that shows the arcs of the painted tree. */
  struct Painted_Arc
  {
    bool operator()(typename GT::Arc* a) const noexcept
    {
      return IS_ARC_VISITED(a, Aleph::Spanning_Tree);
    }
  };

  /** @brief Node filter that shows the nodes of the painted tree. */
  struct Painted_Node
  {
    bool operator()(typename GT::Node* p) const noexcept
    {
      return IS_NODE_VISITED(p, Aleph::Spanning_Tree);
    }
  };

  /** @brief Extracts the painted shortest paths tree and puts it in tree.
   *
   *  Only the nodes that the painting reaches are copied, as
   *  compute_min_paths_tree() does. The nodes and arcs of `g` and of
   *  `tree` are mapped through their cookies; the cookies of the nodes
   *  that are not reached are null. Since the cookies held the painting,
   *  the painting cannot be used afterwards and is_painted() becomes
   *  false.
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

    // Copy_Graph works on a copy of its filter, so the filter cannot be
    // the one that adds up the weights.
    typename Distance::Distance_Type total = 0;
    for (Arc_Iterator<GT, Painted_Arc> it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        total += distance(a);
        ARC_COOKIE(a) = nullptr;
      }

    // null cookies make Copy_Graph map both ways instead of chaining the
    // parents of the painting
    painted = false;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      NODE_COOKIE(it.get_curr()) = nullptr;

    Copy_Graph<GT, Painted_Node, Painted_Arc> copy_painted_arcs;
    copy_painted_arcs(tree, g);

    return total;
  }

  /** @brief Extracts path from tree to end node.
   *
   *  @param[in] tree The spanning tree built by compute_min_paths_tree.
   *  @param[in] end Destination node (in original graph).
   *  @param[out] path Path where result will be stored (in original graph),
   *              made of the arcs of `g` that those of the tree map to.
   *  @return The total cost of the path, by the distance of the arcs of `g`.
   *  @throw domain_error If the minimum paths tree has not been computed,
   *         or `end` is not in the tree, nor reached in it from the start.
   *  @throw bad_alloc If there is not enough memory.
   */
  typename Distance::Distance_Type
  get_min_path(const GT& tree, typename GT::Node* end, Path<GT>& path)
  {
    ah_domain_error_if(ptr_g == nullptr) << "Min path has not been computed";

    typename Distance::Distance_Type total_dist = 0;
    path.empty();
    if (end == s)
      {
        path.init(s);
        return total_dist;
      }

    auto ts = mapped_node<GT>(s);
    auto te = mapped_node<GT>(end);
    ah_domain_error_if(ts == nullptr or te == nullptr)
      << "get_min_path(): the end is not in the tree";

    // Every arc of the tree is one of the paths: the filter of the
    // painting has nothing to hide in it
    Path<GT> tree_path(tree);
    ah_domain_error_if(not (Find_Path_Depth_First<GT, Itor>()(tree, ts, te, tree_path)))
      << "get_min_path(): the end is not reached in the tree";

    // The arcs of g that those of the tree map to, which one of their
    // parallel arcs, searched by their ends, may not be
    path.init(s);
    for (typename Path<GT>::Iterator it(tree_path); it.has_current_arc(); it.next_ne())
      {
        auto ga = mapped_arc<GT>(it.get_current_arc_ne());
        path.append(ga);
        total_dist += distance(ga);
      }

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

} // namespace Aleph

#endif // SHORTEST_PATH_COMMON_H
