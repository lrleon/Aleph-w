#include <gtest/gtest.h>

#include <Dijkstra.H>
#include <tpl_graph.H>

#include <cstdlib>
#include <limits>
#include <vector>

using namespace Aleph;

namespace
{
using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using Node = Graph::Node;

struct BinHeapTag
{
  template <class G, class D, class A>
  using Heap = ArcHeap<G, D, A>;
};

using DijkstraInt =
  Dijkstra_Min_Paths<Graph, Dft_Dist<Graph>, Node_Arc_Iterator,
                     Dft_Show_Arc<Graph>, BinHeapTag::template Heap>;

static bool should_run_stress()
{
  return std::getenv("ALEPH_RUN_GRAPH_STRESS") != nullptr;
}

static size_t stress_nodes()
{
  const char *env = std::getenv("ALEPH_GRAPH_STRESS_NODES");
  if (env == nullptr)
    return 1000000;

  char *end = nullptr;
  const unsigned long long v = std::strtoull(env, &end, 10);
  if (end == env || *end != '\0')
    return 1000000;

  return static_cast<size_t>(v);
}
} // namespace

TEST(GraphStress, MillionNodeChain_DijkstraPaintOnly)
{
  if (!should_run_stress())
    GTEST_SKIP() << "Set ALEPH_RUN_GRAPH_STRESS=1 (and optionally ALEPH_GRAPH_STRESS_NODES=N) to enable.";

  const size_t N = stress_nodes();
  ASSERT_GE(N, 2u);

  Graph g;
  std::vector<Node *> nodes;
  nodes.reserve(N);

  for (size_t i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(static_cast<int>(i)));

  for (size_t i = 0; i + 1 < N; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  DijkstraInt dij;
  const bool found = dij.paint_partial_min_paths_tree(g, nodes.front(), nodes.back());
  ASSERT_TRUE(found);

  const int dist = dij.get_distance(nodes.back());
  ASSERT_EQ(dist, static_cast<int>(N - 1));
}
