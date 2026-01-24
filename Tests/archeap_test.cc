
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
 * @file archeap_test.cc
 * @brief Tests for Archeap
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>

#include <tpl_graph.H>
#include <archeap.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node  = Graph::Node;
  using Arc   = Graph::Arc;

  std::vector<Node *> make_nodes(Graph &g, int n)
  {
    std::vector<Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  struct ArcWeightDist
  {
    int operator()(Arc *a) const noexcept
    {
      return a->get_info();
    }
  };

  using BinHeapType = BinHeap<Arc *, Distance_Compare<Graph, ArcWeightDist>>;
  using HeapNode    = typename BinHeapType::Node;

  struct MapAccess
  {
    std::unordered_map<Node *, HeapNode *> *map = nullptr;

    MapAccess() = default;

    explicit MapAccess(std::unordered_map<Node *, HeapNode *> &m) noexcept
      : map(&m)
    { }

    HeapNode *& operator()(Node *p)
    {
      return (*map)[p];
    }
  };

  using TestArcHeap = ArcHeap<Graph, ArcWeightDist, MapAccess>;
} // namespace

TEST(ArcHeapBasic, SingleArcInsertAndExtract)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  Arc *a = g.insert_arc(nodes[0], nodes[1], 5);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  TestArcHeap heap(ArcWeightDist{}, access);

  heap.put_arc(a, nodes[1]);

  ASSERT_EQ(mapping.size(), 1U);
  ASSERT_NE(mapping[nodes[1]], nullptr);

  Arc *min_arc = heap.get_min_arc();
  EXPECT_EQ(min_arc, a);
  EXPECT_EQ(mapping[nodes[1]], nullptr);
}

TEST(ArcHeapBasic, KeepsBestOfParallelArcs)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 10);
  Arc *a2 = g.insert_arc(nodes[0], nodes[1], 3);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  TestArcHeap heap(ArcWeightDist{}, access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[1]);

  Arc *min_arc = heap.get_min_arc();
  EXPECT_EQ(min_arc, a2);
  EXPECT_EQ(mapping[nodes[1]], nullptr);
}

TEST(ArcHeapBasic, IgnoresWorseArcForSameTarget)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 2);
  Arc *a2 = g.insert_arc(nodes[0], nodes[1], 7);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  TestArcHeap heap(ArcWeightDist{}, access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[1]);

  Arc *min_arc = heap.get_min_arc();
  EXPECT_EQ(min_arc, a1);
  EXPECT_EQ(mapping[nodes[1]], nullptr);
}

TEST(ArcHeapBasic, ExtractsArcsInIncreasingDistanceAcrossTargets)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 5);
  Arc *a2 = g.insert_arc(nodes[0], nodes[2], 1);
  Arc *a3 = g.insert_arc(nodes[0], nodes[3], 3);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  TestArcHeap heap(ArcWeightDist{}, access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[2]);
  heap.put_arc(a3, nodes[3]);

  std::vector<int> extracted;
  extracted.reserve(3);
  extracted.push_back(heap.get_min_arc()->get_info());
  extracted.push_back(heap.get_min_arc()->get_info());
  extracted.push_back(heap.get_min_arc()->get_info());

  EXPECT_EQ(extracted.size(), 3U);
  EXPECT_TRUE(std::is_sorted(extracted.begin(), extracted.end()));
}

TEST(ArcHeapBasic, ClearsMappingForAllTargets)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 4);
  Arc *a2 = g.insert_arc(nodes[0], nodes[2], 2);
  Arc *a3 = g.insert_arc(nodes[0], nodes[3], 6);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  TestArcHeap heap(ArcWeightDist{}, access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[2]);
  heap.put_arc(a3, nodes[3]);

  EXPECT_EQ(mapping.size(), 3U);
  EXPECT_NE(mapping[nodes[1]], nullptr);
  EXPECT_NE(mapping[nodes[2]], nullptr);
  EXPECT_NE(mapping[nodes[3]], nullptr);

  (void)heap.get_min_arc();
  (void)heap.get_min_arc();
  (void)heap.get_min_arc();

  EXPECT_EQ(mapping[nodes[1]], nullptr);
  EXPECT_EQ(mapping[nodes[2]], nullptr);
  EXPECT_EQ(mapping[nodes[3]], nullptr);
}

TEST(ArcHeapRandom, ExtractsNonDecreasingByDistance)
{
  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> node_count(2, 8);
  std::uniform_int_distribution<int> weight_dist(0, 100);

  for (int iter = 0; iter < 100; ++iter)
    {
      Graph g;
      const int n = node_count(rng);
      auto nodes = make_nodes(g, n);

      std::unordered_map<Node *, HeapNode *> mapping;
      MapAccess access(mapping);
      TestArcHeap heap(ArcWeightDist{}, access);

      std::vector<int> weights;
      for (int i = 1; i < n; ++i)
        {
          const int w = weight_dist(rng);
          Arc *a = g.insert_arc(nodes[0], nodes[i], w);
          heap.put_arc(a, nodes[i]);
          weights.push_back(w);
        }

      if (weights.empty())
        continue;

      std::vector<int> extracted;
      extracted.reserve(weights.size());
      for (size_t i = 0; i < weights.size(); ++i)
        extracted.push_back(heap.get_min_arc()->get_info());

      EXPECT_TRUE(std::is_sorted(extracted.begin(), extracted.end()));
    }
}
