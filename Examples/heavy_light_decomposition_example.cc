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
 * @file heavy_light_decomposition_example.cc
 * @brief Story-driven demo of Heavy-Light Decomposition path/subtree queries.
 *
 * Scenario: "The Aurora Power Grid"
 * ----------------------------------
 * Each node is a substation with a current maintenance cost.
 * We need:
 * - sum on any transmission path (u, v)
 * - sum on complete service areas (subtrees)
 * - point updates when maintenance budgets change.
 */

# include <Tree_Decomposition.H>
# include <tpl_graph.H>

# include <cassert>
# include <cstdio>

using namespace Aleph;

namespace
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = G::Node;

  int brute_path_sum(const Array<int> & values,
                     const Heavy_Light_Decomposition<G> & hld,
                     size_t u,
                     size_t v)
  {
    int sum = 0;

    while (hld.depth_of_id(u) > hld.depth_of_id(v))
      {
        sum += values(u);
        u = hld.parent_id(u);
      }

    while (hld.depth_of_id(v) > hld.depth_of_id(u))
      {
        sum += values(v);
        v = hld.parent_id(v);
      }

    while (u != v)
      {
        sum += values(u);
        sum += values(v);
        u = hld.parent_id(u);
        v = hld.parent_id(v);
      }

    return sum + values(u);
  }

  int brute_subtree_sum(const Array<int> & values,
                        const Heavy_Light_Decomposition<G> & hld,
                        const size_t u)
  {
    int sum = 0;
    for (size_t v = 0; v < hld.size(); ++v)
      if (hld.is_ancestor_id(u, v))
        sum += values(v);

    return sum;
  }
}

int main()
{
  G g;

  //                     [Central]
  //                    /    |    \\ (edge)
  //               [North] [West] [South]
  //                /   \             |
  //          [N-A]   [N-B]        [S-A]
  //                                /   \\ (edge)
  //                             [S-A1] [S-A2]

  auto * central = g.insert_node(50);
  auto * north   = g.insert_node(30);
  auto * west    = g.insert_node(20);
  auto * south   = g.insert_node(25);
  auto * na      = g.insert_node(10);
  auto * nb      = g.insert_node(12);
  auto * sa      = g.insert_node(18);
  auto * sa1     = g.insert_node(7);
  auto * sa2     = g.insert_node(9);

  g.insert_arc(central, north, 1);
  g.insert_arc(central, west, 1);
  g.insert_arc(central, south, 1);
  g.insert_arc(north, na, 1);
  g.insert_arc(north, nb, 1);
  g.insert_arc(south, sa, 1);
  g.insert_arc(sa, sa1, 1);
  g.insert_arc(sa, sa2, 1);

  const char * labels[] = {
      "Central", "North", "West", "South", "N-A",
      "N-B", "S-A", "S-A1", "S-A2"};

  HLD_Path_Query<G, int, Aleph::plus<int>> grid(g, central, 0);
  const auto & hld = grid.decomposition();

  auto values = Array<int>::create(hld.size());
  for (size_t i = 0; i < hld.size(); ++i)
    values(i) = hld.node_of(i)->get_info();

  std::printf("=== Aurora Power Grid (HLD path queries) ===\n\n");
  std::printf("Node mapping (id -> label, base position):\n");
  for (size_t i = 0; i < hld.size(); ++i)
    std::printf("  id=%zu  %-8s  pos=%zu\n", i, labels[i], hld.position_of_id(i));

  std::printf("\nPath maintenance cost queries:\n");

  const struct
  {
    Node * u;
    Node * v;
    const char * name;
  } queries[] = {
      {na, nb, "N-A -> N-B"},
      {na, sa2, "N-A -> S-A2"},
      {west, sa1, "West -> S-A1"},
      {central, sa2, "Central -> S-A2"}
  };

  for (const auto & q : queries)
    {
      const size_t u = hld.id_of(q.u);
      const size_t v = hld.id_of(q.v);

      const int ans = grid.query_path_id(u, v);
      const int brute = brute_path_sum(values, hld, u, v);

      std::printf("  %-18s  HLD=%3d  brute=%3d\n", q.name, ans, brute);
      assert(ans == brute);
    }

  std::printf("\nSubtree (service area) totals:\n");
  for (auto * x : {central, north, south, sa})
    {
      const size_t id = hld.id_of(x);
      const int ans = grid.query_subtree_id(id);
      const int brute = brute_subtree_sum(values, hld, id);
      std::printf("  %-8s subtree total = %3d\n", labels[id], ans);
      assert(ans == brute);
    }

  std::printf("\nBudget changes (point updates):\n");
  std::printf("  +5 to S-A2, set West to 26\n");

  const size_t id_sa2 = hld.id_of(sa2);
  const size_t id_west = hld.id_of(west);

  grid.update_node_id(id_sa2, 5);
  values(id_sa2) += 5;

  grid.set_node_id(id_west, 26);
  values(id_west) = 26;

  const int after_path = grid.query_path(north, sa2);
  const int brute_after_path = brute_path_sum(values, hld, hld.id_of(north), hld.id_of(sa2));
  std::printf("  North -> S-A2 after update: HLD=%d brute=%d\n", after_path, brute_after_path);
  assert(after_path == brute_after_path);

  std::printf("\nHow HLD splits path N-A -> S-A2 into base-array segments:\n");
  hld.for_each_path_segment(na, sa2,
                            [&](const size_t l,
                                const size_t r,
                                const bool reversed)
                            {
                              std::printf("  segment [%zu, %zu] (%s)\n",
                                          l,
                                          r,
                                          reversed ? "reversed" : "forward");
                            });

  std::printf("\nAll assertions passed.\n");
  return 0;
}
