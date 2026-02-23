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


/**
 * @file centroid_decomposition_example.cc
 * @brief Story-driven demo of Centroid Decomposition dynamic nearest-center queries.
 *
 * Scenario: "Emergency Response Network"
 * ---------------------------------------
 * Villages form a tree road network.  We repeatedly:
 * - activate villages with emergency teams
 * - ask for the distance from a village to the nearest active team.
 *
 * Centroid decomposition answers each operation in O(log n).
 *
 * All per-node arrays are indexed by centroid/HLD integer IDs obtained via
 * cd.id_of(node), not by insertion order, so the example is correct for any
 * graph backend and insertion sequence.
 */

# include <Tree_Decomposition.H>
# include <tpl_graph.H>

# include <algorithm>
# include <array>
# include <cassert>
# include <format>
# include <iostream>
# include <limits>

using namespace Aleph;

/**
 * @brief Demonstrates centroid-decomposition-based dynamic nearest-center queries on a fixed tree.
 *
 * Builds a small tree of labeled "villages", constructs a Centroid_Decomposition and a
 * Heavy_Light_Decomposition (used as an oracle for exact distances), and exercises:
 * - activation of centers (propagating distances to centroid ancestors),
 * - centroid-based nearest-center queries,
 * - brute-force verification of results via the HLD oracle.
 *
 * The program prints diagnostic information (centroid root and ancestor chain), activates
 * sample villages, compares centroid-query results against the brute-force oracle with
 * assertions, and reports success when all checks pass.
 *
 * @return int 0 on successful completion.
 */
int main()
{
  using G    = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = G::Node;

  G g;

  //                  0
  //              /   |   | (edge)
  //             1    2   3
  //           /  \       |
  //          4    5      6
  //                    / |  | (edge)
  //                   7  8  9
  //                          |
  //                          10

  auto * n0  = g.insert_node(0);
  auto * n1  = g.insert_node(1);
  auto * n2  = g.insert_node(2);
  auto * n3  = g.insert_node(3);
  auto * n4  = g.insert_node(4);
  auto * n5  = g.insert_node(5);
  auto * n6  = g.insert_node(6);
  auto * n7  = g.insert_node(7);
  auto * n8  = g.insert_node(8);
  auto * n9  = g.insert_node(9);
  auto * n10 = g.insert_node(10);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n0, n3, 1);
  g.insert_arc(n1, n4, 1);
  g.insert_arc(n1, n5, 1);
  g.insert_arc(n3, n6, 1);
  g.insert_arc(n6, n7, 1);
  g.insert_arc(n6, n8, 1);
  g.insert_arc(n6, n9, 1);
  g.insert_arc(n9, n10, 1);

  Centroid_Decomposition<G>      cd(g, n0);
  Heavy_Light_Decomposition<G>   hld(g, n0); // oracle distance for assertions

  const size_t n   = cd.size();
  const size_t INF = std::numeric_limits<size_t>::max() / 4;

  // --- Build label_by_id and node_by_id keyed on centroid IDs ---
  // This avoids assuming cd.id_of(ni) == i (which would be fragile and
  // graph-backend-dependent). Both cd and hld share the same Rooted_Tree_Topology
  // ID space (same Node_Iterator order on the same graph object), so
  // hld.distance_id(u, v) is valid with centroid IDs.

  const std::array nodes_arr = { n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10 };
  const std::array village_label = {
      "Village-0",  "Village-1",  "Village-2",  "Village-3",  "Village-4",
      "Village-5",  "Village-6",  "Village-7",  "Village-8",  "Village-9",
      "Village-10"
  };

  auto label_by_id = Array<const char *>::create(n);
  auto node_by_id  = Array<Node *>::create(n);

  for (size_t i = 0; i < n; ++i)
    {
      label_by_id(i) = "";
      node_by_id(i)  = nullptr;
    }

  for (size_t i = 0; i < nodes_arr.size(); ++i)
    {
      const size_t id      = cd.id_of(nodes_arr[i]);
      label_by_id(id) = village_label[i];
      node_by_id(id)  = nodes_arr[i];
    }

  // --- Per-node bookkeeping arrays indexed by centroid ID ---
  auto best   = Array<size_t>::create(n);
  auto active = Array<char>::create(n);

  for (size_t i = 0; i < n; ++i)
    {
      best(i)   = INF;
      active(i) = 0;
    }

  // Activate a village by centroid ID: propagate its distance to every
  // centroid ancestor so query() can find it later in O(log n).
  auto activate = [&](const size_t u)
    {
      active(u) = 1;
      cd.for_each_centroid_ancestor_id(
          u,
          [&](const size_t c, const size_t d, const size_t)
          {
            if (d < best(c))
              best(c) = d;
          });
    };

  // Query the nearest active village from village u (centroid ID).
  auto query = [&](const size_t u)
    {
      size_t ans = INF;
      cd.for_each_centroid_ancestor_id(
          u,
          [&](const size_t c, const size_t d, const size_t)
          {
            if (best(c) != INF)
              ans = std::min(ans, best(c) + d);
          });
      return ans;
    };

  // Brute-force oracle: scan all active nodes and use HLD for exact distance.
  // Uses centroid IDs throughout (valid since cd and hld share the same topology).
  auto brute_query = [&](const size_t u)
    {
      size_t ans = INF;
      for (size_t v = 0; v < n; ++v)
        if (active(v))
          ans = std::min(ans, hld.distance_id(u, v));
      return ans;
    };

  // --- Demo ---

  std::cout << std::format("=== Emergency Response Network (Centroid Decomposition) ===\n\n");
  std::cout << std::format("Centroid root: {} (id={})\n\n",
                           label_by_id(cd.centroid_root_id()),
                           cd.centroid_root_id());

  std::cout << std::format("Centroid chain for Village-10:\n");
  cd.for_each_centroid_ancestor_id(
      cd.id_of(n10),
      [&](const size_t c, const size_t d, const size_t k)
      {
        std::cout << std::format("  k={}  centroid={}  dist={}\n", k, label_by_id(c), d);
      });

  std::cout << std::format("\nActivate team at Village-0 and Village-8\n");
  activate(cd.id_of(n0));
  activate(cd.id_of(n8));

  for (Node * node : { n4, n5, n7, n10 })
    {
      const size_t u     = cd.id_of(node);
      const size_t ans   = query(u);
      const size_t brute = brute_query(u);
      std::cout << std::format("  nearest({:<10}) = {}\n", label_by_id(u), ans);
      assert(ans == brute);
    }

  std::cout << std::format("\nActivate new team at Village-10\n");
  activate(cd.id_of(n10));

  for (Node * node : { n2, n6, n9, n10 })
    {
      const size_t u     = cd.id_of(node);
      const size_t ans   = query(u);
      const size_t brute = brute_query(u);
      std::cout << std::format("  nearest({:<10}) = {}\n", label_by_id(u), ans);
      assert(ans == brute);
    }

  std::cout << std::format("\nAll assertions passed.\n");
  return 0;
}