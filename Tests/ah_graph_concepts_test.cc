// Self-containment check: must stay the first include (aleph-concepts.md §4.6).
#include <ah-graph-concepts.H>

#include <gtest/gtest.h>

#include <Dijkstra.H>
#include <Johnson.H>
#include <K_Shortest_Paths.H>
#include <Kruskal.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_maxflow.H>
#include <tpl_net.H>
#include <tpl_sgraph.H>

// Positive/negative checks for the graph concepts of Phase 3 of
// aleph-concepts-plan.md, and for the algorithms they now constrain.

using namespace Aleph;

namespace
{
  using LG = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using LD = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using AG = Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>;
  using AD = Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>;
  using SG = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
  using SD = List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>;
  using NG = Net_Graph<Net_Node<int>, Net_Arc<int, double>>;

  struct NotAGraph
  {
    using Node = int;
  };

  // A distance without Distance_Type: fine for Kruskal (it only compares
  // dist(a) < dist(b)) but not an ArcDistance.
  struct PlainWeight
  {
    int operator()(LG::Arc * a) const { return a->get_info(); }
  };

  struct NonArithmeticCost
  {
    struct Cost
    {
      int v = 0;
    };
    using Distance_Type = Cost;
    Cost operator()(LG::Arc *) const { return {}; }
  };
} // namespace

// --- Concepts over the library graph types ---------------------------------

static_assert(AlephGraph<LG> and AlephGraph<LD> and AlephGraph<AG> and AlephGraph<AD>);
static_assert(AlephGraph<SG> and AlephGraph<SD> and AlephGraph<NG>);
static_assert(AlephGraph<const LG &>);
static_assert(not AlephGraph<NotAGraph> and not AlephGraph<DynList<int>> and not AlephGraph<int>);

static_assert(ArcFilter<Dft_Show_Arc<LG>, LG>);
static_assert(NodeFilter<Dft_Show_Node<LG>, LG>);
static_assert(not ArcFilter<Dft_Show_Node<LG>, LG>);
static_assert(not NodeFilter<Dft_Show_Arc<LG>, LG>);

static_assert(ArcDistance<Dft_Dist<LG>, LG>);
static_assert(not ArcDistance<PlainWeight, LG>);

static_assert(FlowNetwork<NG>);
static_assert(not FlowNetwork<LG>);

// --- Constrained algorithms --------------------------------------------------

template <class G>
concept can_dijkstra = requires { typename Dijkstra_Min_Paths<G>; };
template <class D>
concept can_dijkstra_with = requires { typename Dijkstra_Min_Paths<LG, D>; };
template <class D>
concept can_kruskal_with = requires { typename Kruskal_Min_Spanning_Tree<LG, D>; };
template <class N>
concept can_max_flow = requires(N & net) { dinic_maximum_flow(net); };
template <class D>
concept can_yen = requires(const LG & g, LG::Node * n, D d) {
  yen_k_shortest_paths<LG, D>(g, n, n, 1, d);
};

static_assert(can_dijkstra<LG> and can_dijkstra<NG>);
static_assert(not can_dijkstra<NotAGraph>);
static_assert(can_dijkstra_with<Dft_Dist<LG>> and not can_dijkstra_with<PlainWeight>);
// Kruskal keeps accepting distances without Distance_Type, as it always has.
static_assert(can_kruskal_with<PlainWeight>);
static_assert(can_max_flow<NG> and not can_max_flow<LG>);
// Former static_assert, now a constraint on the entry point.
static_assert(can_yen<Dft_Dist<LG>> and not can_yen<NonArithmeticCost>);

TEST(AhGraphConceptsTest, ConstrainedAlgorithmsStillRun)
{
  LG g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  auto * c = g.insert_node(3);
  g.insert_arc(a, b, 2);
  g.insert_arc(b, c, 3);
  g.insert_arc(a, c, 9);

  Path<LG> path(g);
  const auto d = Dijkstra_Min_Paths<LG>().find_min_path(g, a, c, path);
  EXPECT_EQ(d, 5);

  LG tree;
  Kruskal_Min_Spanning_Tree<LG, PlainWeight>()(g, tree);
  EXPECT_EQ(tree.get_num_arcs(), 2u);

  const auto paths = yen_k_shortest_paths<LG>(g, a, c, 2);
  EXPECT_EQ(paths.size(), 2u);
}
