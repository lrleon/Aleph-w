
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
#include <memory>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#endif

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

namespace
{
  // The weight of an arc, or its negation when `negated` is set: a
  // default-constructed instance orders the arcs the other way.
  struct SignedWeightDist
  {
    bool negated = false;

    int operator()(Arc *a) const noexcept
    {
      return negated ? -a->get_info() : a->get_info();
    }
  };

  // The weight of an arc times a factor, with no default constructor.
  struct FactorDist
  {
    int factor;

    explicit FactorDist(int f) noexcept : factor(f) { }

    int operator()(Arc *a) const noexcept
    {
      return factor * a->get_info();
    }
  };
} // namespace

// The heap ordered the arcs with a default-constructed distance; only the
// choice between two arcs with the same target used the given one.
TEST(ArcHeapDistance, OrdersByTheGivenDistance)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 5);
  Arc *a2 = g.insert_arc(nodes[0], nodes[2], 1);
  Arc *a3 = g.insert_arc(nodes[0], nodes[3], 3);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  ArcHeap<Graph, SignedWeightDist, MapAccess> heap(SignedWeightDist{true},
                                                    access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[2]);
  heap.put_arc(a3, nodes[3]);

  EXPECT_EQ(heap.get_min_arc(), a1);
  EXPECT_EQ(heap.get_min_arc(), a3);
  EXPECT_EQ(heap.get_min_arc(), a2);
}

// Building that default comparator required a default constructor.
TEST(ArcHeapDistance, TakesADistanceWithoutADefaultConstructor)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  Arc *a1 = g.insert_arc(nodes[0], nodes[1], 4);
  Arc *a2 = g.insert_arc(nodes[0], nodes[2], 2);

  std::unordered_map<Node *, HeapNode *> mapping;
  MapAccess access(mapping);
  ArcHeap<Graph, FactorDist, MapAccess> heap(FactorDist(3), access);

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[2]);

  EXPECT_EQ(heap.get_min_arc(), a2);
  EXPECT_EQ(heap.get_min_arc(), a1);
}

namespace
{
  // The weight of an arc; counts its calls, and the call number fail_at
  // throws (0 for none).
  struct FailingDist
  {
    long *calls = nullptr;
    long fail_at = 0;

    int operator()(Arc *a) const
    {
      if (calls != nullptr and ++*calls == fail_at)
        throw std::runtime_error("arc weight not available");
      return a->get_info();
    }
  };
} // namespace

// Comparing with the given distance, the heap called it in functions that
// were noexcept, so a distance that threw there ended the program.
TEST(ArcHeapDistance, WhatTheDistanceThrowsReachesTheCaller)
{
  Graph g;
  auto nodes = make_nodes(g, 6);
  std::vector<std::pair<Arc *, Node *>> puts;
  for (int i = 1; i < 6; ++i)
    puts.emplace_back(g.insert_arc(nodes[0], nodes[i], 10 * i), nodes[i]);
  for (int i = 1; i < 6; i += 2) // lighter arcs that replace others
    puts.emplace_back(g.insert_arc(nodes[0], nodes[i], 10 * i - 7), nodes[i]);

  // Puts the arcs in a heap and takes them out, with a distance that fails
  // at its call fail_at
  auto put_and_take = [&](long &calls, long fail_at)
  {
    std::unordered_map<Node *, HeapNode *> mapping;
    ArcHeap<Graph, FailingDist, MapAccess> heap(FailingDist{&calls, fail_at},
                                                 MapAccess(mapping));
    std::vector<int> weights;
    for (auto [arc, tgt] : puts)
      heap.put_arc(arc, tgt);
    while (not heap.is_empty())
      weights.push_back(heap.get_min_arc()->get_info());
    return weights;
  };

  long calls = 0;
  EXPECT_EQ(put_and_take(calls, 0), (std::vector<int>{3, 20, 23, 40, 43}));

  for (long k = 1; k <= calls; ++k)
    {
      long count = 0;
      EXPECT_THROW(put_and_take(count, k), std::runtime_error) << "call " << k;
    }
}

namespace
{
  // Keeps the handles of either heap as void *
  struct VoidMapAccess
  {
    std::unordered_map<Node *, void *> *map = nullptr;

    void *& operator()(Node *p) { return (*map)[p]; }
  };

  struct BinaryArcHeap
  {
    using Heap = ArcHeap<Graph, ArcWeightDist, VoidMapAccess>;
  };

  struct FibonacciArcHeap
  {
    using Heap = ArcFibonacciHeap<Graph, ArcWeightDist, VoidMapAccess>;
  };

  template <class Kind>
  struct ArcHeapLifecycle : public ::testing::Test
  {
    using Heap = typename Kind::Heap;

    Graph g;
    std::vector<Node *> nodes = make_nodes(g, 6);
    std::vector<Arc *> arcs; // arcs[i - 1] goes from nodes[0] to nodes[i]

    ArcHeapLifecycle()
    {
      for (int i = 1; i < 6; ++i)
        arcs.push_back(g.insert_arc(nodes[0], nodes[i], 10 * i));
    }

    void put_all(Heap & heap)
    {
      for (int i = 1; i < 6; ++i)
        heap.put_arc(arcs[i - 1], nodes[i]);
    }

    static std::vector<int> take_all(Heap & heap)
    {
      std::vector<int> weights;
      while (not heap.is_empty())
        weights.push_back(heap.get_min_arc()->get_info());
      return weights;
    }

    static bool all_null(const std::unordered_map<Node *, void *> & handles)
    {
      return std::all_of(handles.begin(), handles.end(),
                         [](const auto & entry) { return entry.second == nullptr; });
    }
  };

  using ArcHeapKinds = ::testing::Types<BinaryArcHeap, FibonacciArcHeap>;
} // namespace

TYPED_TEST_SUITE(ArcHeapLifecycle, ArcHeapKinds);

// clear() deleted the heap nodes but left their handles, so putting an arc
// to the same target again used a freed node.
TYPED_TEST(ArcHeapLifecycle, ClearNullsTheHandles)
{
  std::unordered_map<Node *, void *> handles;
  typename TestFixture::Heap heap(ArcWeightDist{}, VoidMapAccess{&handles});

  this->put_all(heap);
  heap.clear();
  EXPECT_TRUE(heap.is_empty());
  EXPECT_TRUE(this->all_null(handles));

  heap.clear(); // twice
  EXPECT_TRUE(heap.is_empty());

  this->put_all(heap);
  EXPECT_EQ(this->take_all(heap), (std::vector<int>{10, 20, 30, 40, 50}));
  EXPECT_TRUE(this->all_null(handles));
}

// The destructor empties the heap in the same way, while the storage of the
// handles is still alive.
TYPED_TEST(ArcHeapLifecycle, DestroyingAFullHeapNullsTheHandles)
{
  std::unordered_map<Node *, void *> handles;
  {
    typename TestFixture::Heap heap(ArcWeightDist{}, VoidMapAccess{&handles});
    this->put_all(heap);
  }
  EXPECT_TRUE(this->all_null(handles));
}

// The move constructor of ArcHeap moved its base, which pointed to the
// header of the source, so the new heap broke once the source was gone.
TYPED_TEST(ArcHeapLifecycle, MoveConstructionOutlivesTheSource)
{
  using Heap = typename TestFixture::Heap;
  std::unordered_map<Node *, void *> handles;
  std::unique_ptr<Heap> moved;
  {
    Heap source(ArcWeightDist{}, VoidMapAccess{&handles});
    this->put_all(source);
    moved = std::make_unique<Heap>(std::move(source));
    EXPECT_TRUE(source.is_empty());
  }

  EXPECT_EQ(this->take_all(*moved), (std::vector<int>{10, 20, 30, 40, 50}));
  EXPECT_TRUE(this->all_null(handles));
}

// The move assignment of ArcHeap did not compile, and that of
// ArcFibonacciHeap left the handles of the arcs that the target held.
TYPED_TEST(ArcHeapLifecycle, MoveAssignmentEmptiesTheTarget)
{
  using Heap = typename TestFixture::Heap;
  std::unordered_map<Node *, void *> target_handles, source_handles;
  Heap target(ArcWeightDist{}, VoidMapAccess{&target_handles});
  target.put_arc(this->arcs[0], this->nodes[1]);
  target.put_arc(this->arcs[1], this->nodes[2]);
  {
    Heap source(ArcWeightDist{}, VoidMapAccess{&source_handles});
    this->put_all(source);
    target = std::move(source);
    EXPECT_TRUE(source.is_empty());
  }

  EXPECT_TRUE(this->all_null(target_handles));
  EXPECT_EQ(this->take_all(target), (std::vector<int>{10, 20, 30, 40, 50}));
  EXPECT_TRUE(this->all_null(source_handles));
}

namespace
{
  // Either heap of arcs, with the distance Dist
  struct BinaryArcHeapOf
  {
    template <class Dist>
    using Heap = ArcHeap<Graph, Dist, VoidMapAccess>;
  };

  struct FibonacciArcHeapOf
  {
    template <class Dist>
    using Heap = ArcFibonacciHeap<Graph, Dist, VoidMapAccess>;
  };

  template <class Kind>
  struct ArcHeapWithDistance : public ::testing::Test
  {
    template <class Dist>
    using Heap = typename Kind::template Heap<Dist>;

    Graph g;
    std::vector<Node *> nodes = make_nodes(g, 8);
  };

  using ArcHeapsOf = ::testing::Types<BinaryArcHeapOf, FibonacciArcHeapOf>;

  // The calls of a distance, and the one that throws (0 for none)
  struct Distance_Calls
  {
    long calls = 0;
    long fail_at = 0;
  };

  // The weight of an arc; counts its calls in calls, which the copies of
  // the distance share
  struct Shared_Failing_Dist
  {
    Distance_Calls *calls = nullptr;

    int operator()(Arc *a) const
    {
      if (calls != nullptr and ++calls->calls == calls->fail_at)
        throw std::runtime_error("arc weight not available");
      return a->get_info();
    }
  };
} // namespace

TYPED_TEST_SUITE(ArcHeapWithDistance, ArcHeapsOf);

// get_distance() gave a copy that the heap did not use: changing it changed
// neither the order of the arcs nor the choice between two arcs to the same
// node
TYPED_TEST(ArcHeapWithDistance, GetDistanceGivesTheDistanceThatOrders)
{
  using Heap = typename TestFixture::template Heap<SignedWeightDist>;
  auto & nodes = this->nodes;
  Arc *a1 = this->g.insert_arc(nodes[0], nodes[1], 1);
  Arc *a2 = this->g.insert_arc(nodes[0], nodes[2], 2);
  Arc *a3 = this->g.insert_arc(nodes[0], nodes[3], 3);
  Arc *a4 = this->g.insert_arc(nodes[0], nodes[3], 4); // parallel to a3

  std::unordered_map<Node *, void *> handles;
  Heap heap(SignedWeightDist{false}, VoidMapAccess{&handles});
  EXPECT_FALSE(heap.get_distance().negated);
  heap.get_distance().negated = true; // the heaviest arcs first

  heap.put_arc(a1, nodes[1]);
  heap.put_arc(a2, nodes[2]);
  heap.put_arc(a3, nodes[3]);
  heap.put_arc(a4, nodes[3]); // replaces a3, which is lighter

  Heap moved(std::move(heap)); // with its distance
  EXPECT_TRUE(moved.get_distance().negated);
  EXPECT_EQ(moved.get_min_arc(), a4);
  EXPECT_EQ(moved.get_min_arc(), a2);
  EXPECT_EQ(moved.get_min_arc(), a1);
  EXPECT_TRUE(moved.is_empty());

  Heap assigned(SignedWeightDist{false}, VoidMapAccess{&handles});
  assigned = std::move(moved);
  EXPECT_TRUE(assigned.get_distance().negated);
}

// If the distance throws, each arc in the heap has its handle, and each
// handle that is set is that of an arc in the heap. ArcFibonacciHeap lost
// the node that it was taking out and could leave the others out of the
// heap, and it kept the node of a new arc whose handle was not set.
TYPED_TEST(ArcHeapWithDistance, WhatTheDistanceThrowsLeavesEachArcWithItsHandle)
{
  using Heap = typename TestFixture::template Heap<Shared_Failing_Dist>;
  auto & nodes = this->nodes;
  std::vector<std::pair<Arc *, Node *>> puts;
  for (int i = 1; i < 8; ++i)
    puts.emplace_back(this->g.insert_arc(nodes[0], nodes[i], 10 * i), nodes[i]);
  for (int i = 1; i < 8; i += 2) // lighter arcs that replace others
    puts.emplace_back(this->g.insert_arc(nodes[0], nodes[i], 10 * i - 7),
                      nodes[i]);

  // Puts the arcs in a heap and takes them out, with a distance that
  // fails at the call calls.fail_at
  auto put_and_take = [&puts](Distance_Calls & calls)
  {
    std::unordered_map<Node *, void *> handles;
    Heap heap(Shared_Failing_Dist{&calls}, VoidMapAccess{&handles});
    try
      {
        std::vector<int> weights;
        for (auto [arc, tgt] : puts)
          heap.put_arc(arc, tgt);
        while (not heap.is_empty())
          weights.push_back(heap.get_min_arc()->get_info());
        return weights;
      }
    catch (...)
      {
        calls.fail_at = 0;
        const auto set = std::count_if(handles.begin(), handles.end(),
                                       [](const auto & entry)
                                       { return entry.second != nullptr; });
        long taken = 0;
        while (not heap.is_empty())
          {
            (void) heap.get_min_arc();
            ++taken;
          }
        EXPECT_EQ(taken, long(set));
        EXPECT_TRUE(std::all_of(handles.begin(), handles.end(),
                                [](const auto & entry)
                                { return entry.second == nullptr; }));
        throw;
      }
  };

  Distance_Calls calls;
  EXPECT_EQ(put_and_take(calls),
            (std::vector<int>{3, 20, 23, 40, 43, 60, 63}));

  for (long k = 1; k <= calls.calls; ++k)
    {
      Distance_Calls failing;
      failing.fail_at = k;
      EXPECT_THROW(put_and_take(failing), std::runtime_error) << "call " << k;
    }
}

namespace
{
  using FibonacciHeapOfArcs =
    ArcFibonacciHeap<Graph, ArcWeightDist, VoidMapAccess>;
  using FibonacciNode = FibonacciHeapOfArcs::handle_type;

  // Runs f, which must not throw, in a thread with a stack of 256 KiB, which
  // a recursion as deep as the trees of these tests are high overflows.
  // Without POSIX threads, it runs f in this thread.
  template <class F>
  void run_with_small_stack(F f)
  {
#if defined(__unix__) || defined(__APPLE__)
    pthread_attr_t attr;
    ASSERT_EQ(pthread_attr_init(&attr), 0);
    ASSERT_EQ(pthread_attr_setstacksize(&attr, 256 * 1024), 0);
    auto start = [](void *arg) -> void *
      {
        (*static_cast<F *>(arg))();
        return nullptr;
      };
    pthread_t thread;
    const int created = pthread_create(&thread, &attr, start, &f);
    pthread_attr_destroy(&attr);
    ASSERT_EQ(created, 0);
    ASSERT_EQ(pthread_join(thread, nullptr), 0);
#else
    f();
#endif
  }

  // A graph whose arcs go from a hub to other nodes, and the handles that a
  // Fibonacci heap of its arcs keeps for the nodes
  struct HubGraph
  {
    Graph g;
    Node *hub = g.insert_node(0);
    std::unordered_map<Node *, void *> handles;

    bool all_null() const
    {
      return std::all_of(handles.begin(), handles.end(),
                         [](const auto & entry) { return entry.second == nullptr; });
    }

    FibonacciNode handle(Node *p) { return static_cast<FibonacciNode>(handles[p]); }
  };

  // Puts in heap, which must be empty, a single tree that is a chain of n
  // heap nodes (CLRS, exercise 19.4-1), and returns the node whose handle is
  // its root. Each round puts the arcs a < b < c below the root and takes a,
  // so that b takes c and the chain as children; then it puts a lighter arc
  // to the node of c, which cuts c, and takes it, so that b ends on top.
  Node * build_chain(HubGraph & hg, FibonacciHeapOfArcs & heap, int n)
  {
    int key = 4 * n;
    Node *top = hg.g.insert_node(1);
    heap.put_arc(hg.g.insert_arc(hg.hub, top, key), top);

    Node *node_a = hg.g.insert_node(2), *node_c = hg.g.insert_node(3);
    Arc *a = hg.g.insert_arc(hg.hub, node_a, 0);
    Arc *c = hg.g.insert_arc(hg.hub, node_c, 0);
    Arc *lighter_c = hg.g.insert_arc(hg.hub, node_c, 0);
    for (int i = 1; i < n; ++i)
      {
        Node *node_b = hg.g.insert_node(4);
        a->get_info() = key - 3;
        c->get_info() = key - 1;
        lighter_c->get_info() = key - 4;
        heap.put_arc(a, node_a);
        heap.put_arc(hg.g.insert_arc(hg.hub, node_b, key - 2), node_b);
        heap.put_arc(c, node_c);
        (void) heap.get_min_arc(); // a
        heap.put_arc(lighter_c, node_c);
        (void) heap.get_min_arc(); // lighter_c
        top = node_b;
        key -= 2;
      }
    return top;
  }

  // The number of heap nodes from the handle of top down by child
  size_t height_of_chain(HubGraph & hg, Node *top)
  {
    size_t height = 0;
    for (auto p = hg.handle(top); p != nullptr; p = p->child)
      ++height;
    return height;
  }
}

// ArcFibonacciHeap cleared the handles recursively, so a tree as high as the
// heap is large overflowed the stack.
TEST(ArcFibonacciHeapDepth, ClearNullsTheHandlesOfATreeAsHighAsTheHeap)
{
  constexpr int n = 50000;
  HubGraph hg;
  FibonacciHeapOfArcs heap(ArcWeightDist{}, VoidMapAccess{&hg.handles});
  Node *top = build_chain(hg, heap, n);
  ASSERT_EQ(height_of_chain(hg, top), size_t(n));

  run_with_small_stack([&heap] { heap.clear(); });
  EXPECT_TRUE(heap.is_empty());
  EXPECT_TRUE(hg.all_null());
}

TEST(ArcFibonacciHeapDepth, DestroyingATreeAsHighAsTheHeapNullsTheHandles)
{
  constexpr int n = 50000;
  HubGraph hg;
  auto heap = std::make_unique<FibonacciHeapOfArcs>(ArcWeightDist{},
                                                    VoidMapAccess{&hg.handles});
  Node *top = build_chain(hg, *heap, n);
  ASSERT_EQ(height_of_chain(hg, top), size_t(n));

  run_with_small_stack([&heap] { heap.reset(); });
  EXPECT_TRUE(hg.all_null());
}

// The handles of trees of several children and levels are cleared too.
TEST(ArcFibonacciHeapDepth, ClearNullsTheHandlesOfBushyTrees)
{
  HubGraph hg;
  const std::vector<Node *> targets = make_nodes(hg.g, 300);
  FibonacciHeapOfArcs heap(ArcWeightDist{}, VoidMapAccess{&hg.handles});
  std::mt19937 gen(141);
  auto put_arcs = [&](int count)
    {
      for (int i = 0; i < count; ++i)
        {
          Node *p = targets[gen() % targets.size()];
          heap.put_arc(hg.g.insert_arc(hg.hub, p, int(gen() % 100000)), p);
        }
    };
  put_arcs(1000);
  for (int i = 0; i < 60; ++i) // links the roots in trees
    (void) heap.get_min_arc();
  put_arcs(1000);              // lighter arcs cut heap nodes
  for (int i = 0; i < 20; ++i)
    (void) heap.get_min_arc();

  size_t degree = 0, depth = 0;
  for (Node *p : targets)
    if (auto q = hg.handle(p); q != nullptr)
      {
        degree = std::max(degree, q->degree);
        size_t d = 0;
        for (; q->parent != nullptr; q = q->parent)
          ++d;
        depth = std::max(depth, d);
      }
  ASSERT_GE(degree, 3u);
  ASSERT_GE(depth, 3u);

  heap.clear();
  EXPECT_TRUE(heap.is_empty());
  EXPECT_TRUE(hg.all_null());
}
