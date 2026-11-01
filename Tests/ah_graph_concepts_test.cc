// Self-containment check: must stay the first include (aleph-concepts.md §4.6).
#include <ah-graph-concepts.H>

#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>

#include <Dijkstra.H>
#include <Johnson.H>
#include <K_Shortest_Paths.H>
#include <io_graph.H>
#include <Kruskal.H>
#include <tpl_agraph.H>
#include <tpl_graph.H>
#include <tpl_indexGraph.H>
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

  // A filter that can be moved and not copied, which no algorithm can
  // take: each iterator, and each frame of a search, keeps a copy of it
  struct Move_Only_Filter
  {
    Move_Only_Filter() = default;
    Move_Only_Filter(Move_Only_Filter &&) = default;
    Move_Only_Filter(const Move_Only_Filter &) = delete;
    bool operator()(LG::Arc *) const { return true; }
  };

  // A filter that can be copied, and neither assigned nor built by default
  struct Reference_Filter
  {
    const int & limit;
    bool operator()(LG::Arc * a) const { return a->get_info() < limit; }
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
// An arc filter must be copied, and need not be assigned or built by default
static_assert(not ArcFilter<Move_Only_Filter, LG>);
static_assert(ArcFilter<Reference_Filter, LG>);

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

// --- Constrained utilities of tpl_graph.H ------------------------------------

template <class G>
concept can_copy_graph = requires(G & t, const G & s) { copy_graph(t, s); };
template <class G>
concept can_clear_graph = requires(G & g) { clear_graph(g); };
template <class G>
concept can_compare_graphs = requires(const G & g) { are_equal(g, g); };
template <class G>
concept can_for_each_node = requires(const G & g) { for_each_node(g, [](auto *) {}); };
template <class SA>
concept can_filter_arcs_with = requires(const LG & g) { for_each_arc(g, [](LG::Arc *) {}, SA()); };
template <class G>
concept can_out_nodes = requires(typename G::Node * p) { out_nodes<G>(p); };

static_assert(can_copy_graph<LG> and can_copy_graph<AD> and can_copy_graph<NG>);
static_assert(can_clear_graph<SG> and can_compare_graphs<LG> and can_for_each_node<LD>);
static_assert(not can_copy_graph<DynList<int>> and not can_clear_graph<int>);
static_assert(not can_compare_graphs<DynList<int>> and not can_for_each_node<DynList<int>>);
// Filters are checked at the call: a node filter is not an arc filter.
static_assert(can_filter_arcs_with<Dft_Show_Arc<LG>> and not can_filter_arcs_with<Dft_Show_Node<LG>>);
static_assert(can_out_nodes<LD> and not can_out_nodes<NotAGraph>);

// Graph I/O: the graph and the node/arc filters are checked when naming IO_Graph.
template <class G>
concept can_io_graph = requires { typename IO_Graph<G>; };
template <class NF>
concept can_io_graph_with_node_filter =
  requires { typename IO_Graph<LG, Dft_Load_Node<LG>, Dft_Store_Node<LG>,
                               Dft_Load_Arc<LG>, Dft_Store_Arc<LG>, NF>; };

static_assert(can_io_graph<LG> and can_io_graph<AD> and can_io_graph<SG>);
static_assert(not can_io_graph<NotAGraph> and not can_io_graph<DynList<int>>);
static_assert(can_io_graph_with_node_filter<Dft_Show_Node<LG>>);
static_assert(not can_io_graph_with_node_filter<Dft_Show_Arc<LG>>);

TEST(AhGraphConceptsTest, InOutArcHelpersCompileAndAgree)
{
  // These helpers never compiled: they called traverse_*_arcs and
  // for_each_*_arc without <GT>, which cannot be deduced from a Node *.
  LG g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  g.insert_arc(a, b, 4);
  g.insert_arc(a, b, 7);
  auto even = [](LG::Arc * arc) { return arc->get_info() % 2 == 0; };

  EXPECT_FALSE(all_out_arc<LG>(a, even));
  EXPECT_TRUE(exists_out_arc<LG>(a, even));
  EXPECT_EQ(search_in_arc<LG>(b, even)->get_info(), 4);
  EXPECT_EQ(filter_in_arcs<LG>(b, even).size(), 1u);
  EXPECT_EQ((map_out_arcs<LG, int>(a, [](LG::Arc * arc) { return arc->get_info(); }).size()), 2u);
  EXPECT_EQ((foldl_in_arcs<LG, int>(b, 0, [](const int & s, LG::Arc * arc)
                                     { return s + arc->get_info(); })), 11);

  // The one-argument form used to be ambiguous (two viable overloads).
  EXPECT_EQ(out_degree<LG>(a), 2u);
  EXPECT_EQ(in_degree<LG>(b), 2u);
  EXPECT_EQ(in_degree<LG>(a), 0u);
}

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

namespace
{
  // A filter whose call may throw
  template <class GT>
  struct May_Throw_Filter
  {
    bool operator()(typename GT::Arc *) const { return true; }
  };

  // A filter whose call throws while *armed
  template <class GT>
  struct Armed_Filter
  {
    const bool * armed = nullptr;

    bool operator()(typename GT::Arc *) const
    {
      if (*armed)
        throw std::runtime_error("Armed_Filter");
      return true;
    }
  };
} // namespace

// The searches of an arc, the moves of the iterators of digraphs and the
// steps of the depth-first searches were noexcept whatever their filter,
// and a filter that threw ended the program: they are noexcept as the
// filter, and what it throws comes out
TEST(AhGraphConceptsTest, WhatCallsTheFilterIsNoexceptAsTheFilter)
{
  static_assert(nothrow_arc_filter_v<LG, Dft_Show_Arc<LG>>);
  static_assert(not nothrow_arc_filter_v<LG, May_Throw_Filter<LG>>);
  static_assert(not nothrow_arc_filter_v<LG, Reference_Filter>);

  static_assert(noexcept(search_arc<LG>(std::declval<const LG &>(), nullptr, nullptr)));
  static_assert(not noexcept(search_arc<LG, May_Throw_Filter<LG>>(std::declval<const LG &>(),
                                                                  nullptr, nullptr)));
  static_assert(noexcept(search_directed_arc<LD>(std::declval<const LD &>(), nullptr, nullptr)));
  static_assert(not noexcept(search_directed_arc<LD, May_Throw_Filter<LD>>(
                  std::declval<const LD &>(), nullptr, nullptr)));
  static_assert(noexcept(search_spanning_tree_arc<LG>(std::declval<const LG &>(), nullptr,
                                                      nullptr)));
  static_assert(not noexcept(search_spanning_tree_arc<LG, May_Throw_Filter<LG>>(
                  std::declval<const LG &>(), nullptr, nullptr)));

  static_assert(noexcept(std::declval<Out_Iterator<LD> &>().next_ne()));
  static_assert(not noexcept(std::declval<Out_Iterator<LD, May_Throw_Filter<LD>> &>().next_ne()));
  static_assert(noexcept(std::declval<In_Iterator<LG> &>().next_ne()));
  static_assert(not noexcept(std::declval<In_Iterator<LG, May_Throw_Filter<LG>> &>().next_ne()));
  static_assert(noexcept(std::declval<In_Iterator<AD> &>().next_ne()));
  static_assert(not noexcept(std::declval<In_Iterator<AD, May_Throw_Filter<AD>> &>().next_ne()));

  using Frame = Dfs_Arc_Frame<Node_Arc_Iterator<LG>>;
  using May_Throw_Frame = Dfs_Arc_Frame<Node_Arc_Iterator<LG, May_Throw_Filter<LG>>>;
  static_assert(noexcept(std::declval<Frame &>().next_arc()));
  static_assert(noexcept(std::declval<Frame &>().template next_target<Aleph::Depth_First>()));
  static_assert(not noexcept(std::declval<May_Throw_Frame &>().next_arc()));
  static_assert(not noexcept(std::declval<May_Throw_Frame &>()
                             .template next_target<Aleph::Depth_First>()));

  LG g;
  auto * a = g.insert_node(1);
  auto * b = g.insert_node(2);
  auto * c = g.insert_node(3);
  auto * ab = g.insert_arc(a, b, 1);
  g.insert_arc(a, c, 2);

  bool armed = false;
  const Armed_Filter<LG> filter{&armed};
  EXPECT_EQ((search_arc<LG, Armed_Filter<LG>>(g, a, b, filter)), ab);
  armed = true;
  EXPECT_THROW((search_arc<LG, Armed_Filter<LG>>(g, a, b, filter)), std::runtime_error);
  EXPECT_THROW((search_spanning_tree_arc<LG, Armed_Filter<LG>>(g, a, b, filter)),
               std::runtime_error);

  // On the second arc of the node, after next_ne()
  armed = false;
  Node_Arc_Iterator<LG, Armed_Filter<LG>> it(a, filter);
  ASSERT_TRUE(it.has_curr());
  armed = true;
  EXPECT_THROW(it.next_ne(), std::runtime_error);
}
