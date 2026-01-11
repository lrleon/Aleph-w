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

/** @file shortest_path_common.H
 *  @brief Common utilities for shortest path algorithms.
 *
 *  This file contains shared functionality used by Dijkstra's algorithm,
 *  A*, and other shortest path algorithms. It provides:
 *
 *  - Overflow-checked arithmetic operations
 *  - Common cookie structures for node/arc information
 *  - Path extraction utilities
 *
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

#ifndef SHORTEST_PATH_COMMON_H
#define SHORTEST_PATH_COMMON_H

#include <limits>
#include <type_traits>
#include <ah-errors.H>
#include <tpl_graph.H>

namespace Aleph
{

/** @brief Namespace for shortest path algorithm implementation details.
 *
 *  Contains common utilities shared between Dijkstra, A*, Bellman-Ford,
 *  and other shortest path algorithms.
 */
namespace shortest_path_detail
{

// =============================================================================
// Overflow-Checked Arithmetic
// =============================================================================

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
 *
 *  @par Example
 *  @code
 *  int x = checked_add<int>(1000000000, 2000000000);  // throws overflow_error
 *  double y = checked_add<double>(1e308, 1e308);      // returns inf (no throw)
 *  @endcode
 */
template <typename T>
[[nodiscard]] inline T checked_add(const T& a, const T& b)
{
  if constexpr (std::is_integral_v<T>)
    {
      // Check for positive overflow
      ah_overflow_error_if(b > 0 and a > std::numeric_limits<T>::max() - b)
        << "Integer overflow in distance addition: " << a << " + " << b;

      // Check for negative overflow (underflow)
      ah_overflow_error_if(b < 0 and a < std::numeric_limits<T>::min() - b)
        << "Integer underflow in distance addition: " << a << " + " << b;
    }

  return a + b;
}

// =============================================================================
// Cookie Access Utilities
// =============================================================================

/** @brief Base structure for arc information in shortest path algorithms.
 *
 *  Stores the potential (priority) value used for heap ordering.
 *
 *  @tparam Distance_Type The numeric type for distances.
 */
template <typename Distance_Type>
struct Arc_Info_Base
{
  Distance_Type pot = 0;  ///< Arc potential (priority in heap)
};

/** @brief Extended arc info with tree arc mapping.
 *
 *  Used when building a separate spanning tree (not just painting).
 *
 *  @tparam GT Graph type.
 *  @tparam Distance_Type The numeric type for distances.
 */
template <typename GT, typename Distance_Type>
struct Tree_Arc_Info_Base : public Arc_Info_Base<Distance_Type>
{
  typename GT::Arc* tree_arc = nullptr;  ///< Corresponding arc in spanning tree
};

/** @brief Base structure for node information in shortest path algorithms.
 *
 *  Stores accumulated distance, heap node handle, and parent pointer.
 *
 *  @tparam Distance_Type The numeric type for distances.
 */
template <typename Distance_Type>
struct Node_Info_Base
{
  Distance_Type dist = 0;     ///< Accumulated distance from start
  void* heap_node = nullptr;  ///< Handle in priority queue
  void* ret_node = nullptr;   ///< Parent node in path (for backtracking)
};

/** @brief Extended node info with tree node mapping.
 *
 *  Used when building a separate spanning tree.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance_Type The numeric type for distances.
 */
template <typename GT, typename Distance_Type>
struct Tree_Node_Info_Base : public Node_Info_Base<Distance_Type>
{
  typename GT::Node* tree_node = nullptr;  ///< Corresponding node in spanning tree
};

// =============================================================================
// Path Computation Utilities
// =============================================================================

/** @brief Compute distance to a node by traversing the painted path.
 *
 *  After a graph has been painted with a shortest path tree, this function
 *  computes the total distance from the start node to a given destination
 *  by traversing the path backward using parent pointers stored in cookies.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance functor.
 *  @tparam Itor Arc iterator template.
 *  @tparam SA Arc filter.
 *  @tparam GetParent Functor to extract parent pointer from node cookie.
 *
 *  @param start The source node of the shortest path computation.
 *  @param node The destination node to compute distance to.
 *  @param sa Arc filter instance.
 *  @param get_parent Functor that extracts parent from node cookie.
 *  @return Total distance from start to node.
 *
 *  @note The graph must have been previously painted with a shortest path
 *        algorithm (spanning tree bits set and parent pointers stored).
 */
template <class GT, class Distance,
          template <typename, class> class Itor,
          class SA, class GetParent>
[[nodiscard]] typename Distance::Distance_Type
compute_path_distance(typename GT::Node* start,
                      typename GT::Node* node,
                      SA& sa,
                      GetParent get_parent)
{
  if (node == start)
    return 0;

  typename Distance::Distance_Type total = 0;

  for (auto curr = node; curr != start;)
    {
      auto parent = get_parent(curr);
      if (parent == nullptr)
        break;

      // Find the connecting arc from parent to curr
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

} // namespace shortest_path_detail

} // namespace Aleph

#endif // SHORTEST_PATH_COMMON_H
