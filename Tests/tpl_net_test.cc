
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
 * @file tpl_net_test.cc
 * @brief Tests for Tpl Net
 */
#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <vector>

#include <tpl_dynDlist.H>
#include <tpl_maxflow.H>
#include <tpl_net.H>
#include <tpl_net_sup_dem.H>
#include <tpl_netcapgraph.H>
#include <tpl_netcost.H>

using namespace Aleph;

namespace
{
  using Net = Net_Graph<Net_Node<int>, Net_Arc<int, int>>;

  struct SimpleNet
  {
    Net::Node *s = nullptr;
    Net::Node *a = nullptr;
    Net::Node *b = nullptr;
    Net::Node *t = nullptr;
    Net::Arc *sa = nullptr;
    Net::Arc *sb = nullptr;
    Net::Arc *ab = nullptr;
    Net::Arc *at = nullptr;
    Net::Arc *bt = nullptr;
  };

  SimpleNet build_simple_net(Net &net)
  {
    SimpleNet sn;
    sn.s = net.insert_node(0);
    sn.a = net.insert_node(1);
    sn.b = net.insert_node(2);
    sn.t = net.insert_node(3);

    sn.sa = net.insert_arc(sn.s, sn.a, 3, 0);
    sn.sb = net.insert_arc(sn.s, sn.b, 2, 0);
    sn.ab = net.insert_arc(sn.a, sn.b, 1, 0);
    sn.at = net.insert_arc(sn.a, sn.t, 2, 0);
    sn.bt = net.insert_arc(sn.b, sn.t, 3, 0);

    return sn;
  }

  struct SingleArcNet
  {
    Net::Node *s = nullptr;
    Net::Node *t = nullptr;
    Net::Arc *st = nullptr;
  };

  SingleArcNet build_single_arc(Net &net, int cap, int flow = 0)
  {
    SingleArcNet sn;
    sn.s = net.insert_node(0);
    sn.t = net.insert_node(1);
    sn.st = net.insert_arc(sn.s, sn.t, cap, flow);
    return sn;
  }
}

TEST(NetGraphBasics, SourcesSinksUpdateOnArcOperations)
{
  Net net;
  auto s = net.insert_node(1);
  auto t = net.insert_node(2);

  EXPECT_TRUE(net.is_source(s));
  EXPECT_TRUE(net.is_sink(s));
  EXPECT_TRUE(net.is_source(t));
  EXPECT_TRUE(net.is_sink(t));

  auto arc = net.insert_arc(s, t, 5, 0);

  EXPECT_TRUE(net.is_source(s));
  EXPECT_FALSE(net.is_sink(s));
  EXPECT_FALSE(net.is_source(t));
  EXPECT_TRUE(net.is_sink(t));

  net.remove_arc(arc);

  EXPECT_TRUE(net.is_source(s));
  EXPECT_TRUE(net.is_sink(s));
  EXPECT_TRUE(net.is_source(t));
  EXPECT_TRUE(net.is_sink(t));
}

TEST(NetGraphBasics, DisconnectConnectArcUpdatesSets)
{
  Net net;
  auto s = net.insert_node(1);
  auto t = net.insert_node(2);
  auto arc = net.insert_arc(s, t, 5, 0);

  net.disconnect_arc(arc);
  EXPECT_EQ(net.get_num_arcs(), 0U);
  EXPECT_TRUE(net.is_source(t));
  EXPECT_TRUE(net.is_sink(s));

  net.connect_arc(arc);
  EXPECT_EQ(net.get_num_arcs(), 1U);
  EXPECT_FALSE(net.is_source(t));
  EXPECT_FALSE(net.is_sink(s));
}

TEST(NetGraphSuperNodes, MakeAndUnmakeSuperSource)
{
  Net net;
  auto s1 = net.insert_node(1);
  auto s2 = net.insert_node(2);
  auto t1 = net.insert_node(3);
  auto t2 = net.insert_node(4);

  net.insert_arc(s1, t1, 5, 0);
  net.insert_arc(s2, t2, 7, 0);

  EXPECT_EQ(net.get_src_nodes().size(), 2U);
  EXPECT_EQ(net.get_sink_nodes().size(), 2U);

  const auto before_nodes = net.get_num_nodes();
  net.make_super_source();

  EXPECT_EQ(net.get_src_nodes().size(), 1U);
  EXPECT_EQ(net.get_num_nodes(), before_nodes + 1);

  auto super_source = net.get_source();
  EXPECT_EQ(net.get_out_degree(super_source), 2U);

  bool has_self = false;
  for (Node_Arc_Iterator<Net> it(super_source); it.has_curr(); it.next_ne())
    {
      if (it.get_tgt_node_ne() == super_source)
        has_self = true;
    }
  EXPECT_FALSE(has_self);

  net.unmake_super_source();
  EXPECT_EQ(net.get_num_nodes(), before_nodes);
  EXPECT_EQ(net.get_src_nodes().size(), 2U);
}

TEST(NetGraphSuperNodes, MakeAndUnmakeSuperSink)
{
  Net net;
  auto s1 = net.insert_node(1);
  auto s2 = net.insert_node(2);
  auto t1 = net.insert_node(3);
  auto t2 = net.insert_node(4);

  net.insert_arc(s1, t1, 5, 0);
  net.insert_arc(s2, t2, 7, 0);

  EXPECT_EQ(net.get_sink_nodes().size(), 2U);

  const auto before_nodes = net.get_num_nodes();
  net.make_super_sink();

  EXPECT_EQ(net.get_sink_nodes().size(), 1U);
  EXPECT_EQ(net.get_num_nodes(), before_nodes + 1);

  auto super_sink = net.get_sink();
  EXPECT_EQ(net.get_in_degree(super_sink), 2U);

  bool has_self = false;
  for (Node_Arc_Iterator<Net> it(super_sink); it.has_curr(); it.next_ne())
    {
      if (it.get_tgt_node_ne() == super_sink)
        has_self = true;
    }
  EXPECT_FALSE(has_self);

  net.unmake_super_sink();
  EXPECT_EQ(net.get_num_nodes(), before_nodes);
  EXPECT_EQ(net.get_sink_nodes().size(), 2U);
}

TEST(NetGraphFlowAlgorithms, FordFulkersonAndEdmondsKarp)
{
  Net net1;
  build_simple_net(net1);
  EXPECT_EQ(ford_fulkerson_maximum_flow(net1), 5);
  EXPECT_TRUE(net1.check_network());
  EXPECT_EQ(net1.flow_value(), 5);

  Net net2;
  build_simple_net(net2);
  EXPECT_EQ(edmonds_karp_maximum_flow(net2), 5);
  EXPECT_TRUE(net2.check_network());
  EXPECT_EQ(net2.flow_value(), 5);
}

TEST(NetGraphFlowAlgorithms, PreflowVariants)
{
  Net net1;
  build_simple_net(net1);
  EXPECT_EQ(fifo_preflow_maximum_flow(net1), 5);
  EXPECT_TRUE(net1.check_network());

  Net net2;
  build_simple_net(net2);
  EXPECT_EQ(heap_preflow_maximum_flow(net2), 5);
  EXPECT_TRUE(net2.check_network());

  Net net3;
  build_simple_net(net3);
  EXPECT_EQ(random_preflow_maximum_flow(net3), 5);
  EXPECT_TRUE(net3.check_network());
}

namespace
{
  // Four nodes after the source, each with an arc to each of four before
  // the sink, all of capacity 1: every perfect matching between them is a
  // maximum flow, so that the order of the pushes chooses one
  void build_matching_net(Net & net)
  {
    auto s = net.insert_node(0);
    auto t = net.insert_node(1);
    std::vector<Net::Node *> left, right;
    for (int i = 0; i < 4; ++i)
      left.push_back(net.insert_node(10 + i));
    for (int i = 0; i < 4; ++i)
      right.push_back(net.insert_node(20 + i));
    for (auto * l : left)
      net.insert_arc(s, l, 1, 0);
    for (auto * l : left)
      for (auto * r : right)
        net.insert_arc(l, r, 1, 0);
    for (auto * r : right)
      net.insert_arc(r, t, 1, 0);
  }

  std::vector<int> arc_flows(Net & net)
  {
    std::vector<int> flows;
    for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      flows.push_back(it.get_curr_ne()->flow);
    return flows;
  }
} // namespace

// The random preflow took its seed from the time, so that its runs could
// not be repeated: with a seed, two runs on equal networks push the same
// flows
TEST(NetGraphFlowAlgorithms, RandomPreflowWithASeedRepeatsItsPushes)
{
  std::set<std::vector<int>> seen;
  for (unsigned long seed : {7ul, 8ul, 9ul, 10ul, 11ul, 12ul, 13ul, 14ul})
    {
      Net net1, net2;
      build_matching_net(net1);
      build_matching_net(net2);
      EXPECT_EQ(random_preflow_maximum_flow(net1, seed), 4);
      EXPECT_EQ(random_preflow_maximum_flow(net2, seed), 4);
      EXPECT_TRUE(net1.check_network());
      const auto flows = arc_flows(net1);
      EXPECT_EQ(flows, arc_flows(net2)) << "seed " << seed;
      seen.insert(flows);
    }
  EXPECT_GT(seen.size(), 1u); // the seed chooses among the maximum flows
}

TEST(NetGraphAugmentingPaths, IncreaseFlowOnPath)
{
  Net net;
  auto nodes = build_single_arc(net, 4);

  auto path = find_aumenting_path_dfs(net, 0);
  auto slack = increase_flow(net, path);

  EXPECT_EQ(slack, 4);
  EXPECT_EQ(nodes.st->flow, 4);
}

TEST(NetGraphAugmentingPaths, SemiPathAndIncreaseFlow)
{
  Net net;
  auto nodes = build_single_arc(net, 4);

  Find_Aumenting_Path_DFS<Net> finder(net);
  DynList<Parc<Net>> semi_path;
  auto slack = finder.semi_path(nodes.s, nodes.t, semi_path, 0);

  EXPECT_EQ(slack, 4);
  EXPECT_FALSE(semi_path.is_empty());

  increase_flow(net, semi_path, slack);
  EXPECT_EQ(nodes.st->flow, 4);
}

TEST(NetGraphResidualHelpers, RemainingFlowAndFilter)
{
  Net net;
  auto nodes = build_single_arc(net, 10, 3);

  EXPECT_FALSE(is_residual<Net>(nodes.s, nodes.st));
  EXPECT_TRUE(is_residual<Net>(nodes.t, nodes.st));
  EXPECT_EQ(remaining_flow<Net>(nodes.s, nodes.st), 7);
  EXPECT_EQ(remaining_flow<Net>(nodes.t, nodes.st), 3);

  Net_Filt<Net> from_src(nodes.s);
  Net_Filt<Net> from_tgt(nodes.t);
  EXPECT_TRUE(from_src(nodes.st));
  EXPECT_TRUE(from_tgt(nodes.st));

  nodes.st->flow = nodes.st->cap;
  EXPECT_FALSE(from_src(nodes.st));

  nodes.st->flow = 0;
  EXPECT_FALSE(from_tgt(nodes.st));
}

TEST(NetGraphResidualNet, UpdateFlowFromResidual)
{
  Net net;
  build_single_arc(net, 10, 4);

  auto residual = preflow_create_residual_net(net);
  auto &rnet = std::get<0>(residual);
  using Rnet = PP_Res_Net<Net>;

  Net::Arc *original = nullptr;
  for (typename Rnet::Arc_Iterator it(rnet); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      if (arc->img != nullptr)
        {
          original = arc->img;
          arc->flow = 7;
          break;
        }
    }

  ASSERT_NE(original, nullptr);
  update_flow(rnet);
  EXPECT_EQ(original->flow, 7);
}

TEST(NetGraphMinCut, ComputesCutCapacity)
{
  Net net;
  auto nodes = build_simple_net(net);

  DynSetTree<Net::Node *> vs;
  DynSetTree<Net::Node *> vt;
  DynList<Net::Arc *> cuts;
  DynList<Net::Arc *> cutt;

  auto value = min_cut<Net, Edmonds_Karp_Maximum_Flow>(net, vs, vt, cuts, cutt);
  EXPECT_EQ(value, 5);
  EXPECT_TRUE(vs.contains(nodes.s));
  EXPECT_TRUE(vt.contains(nodes.t));

  int cut_cap = 0;
  for (typename DynList<Net::Arc *>::Iterator it(cuts); it.has_curr(); it.next_ne())
    cut_cap += it.get_curr()->cap;

  EXPECT_EQ(cut_cap, value);
}

// insert_arc() checked the flow after the arc was in the network, with the
// sources and the sinks updated, so an arc with more flow than capacity was
// left in it
TEST(NetGraphBasics, AnArcWithMoreFlowThanCapacityIsNotInserted)
{
  Net net;
  auto s = net.insert_node(1);
  auto t = net.insert_node(2);

  EXPECT_THROW(net.insert_arc(s, t, 2, 3), std::overflow_error);
  EXPECT_THROW(net.insert_arc(s, t, 2, -1), std::overflow_error);
  EXPECT_EQ(net.get_num_arcs(), 0u);
  EXPECT_TRUE(net.is_sink(s));
  EXPECT_TRUE(net.is_source(t));

  auto arc = net.insert_arc(s, t, 2, 2);
  EXPECT_EQ(net.get_num_arcs(), 1u);
  EXPECT_EQ(arc->flow, 2);
  EXPECT_FALSE(net.is_sink(s));
  EXPECT_FALSE(net.is_source(t));
}

// The copy constructor inserts the arcs with two arguments, and the
// overload that took them was disabled for an arithmetic arc info, so a
// network with one did not compile its copy
TEST(NetGraphBasics, CopiesANetworkWithAnArithmeticArcInfo)
{
  using Info_Net = Net_Graph<Net_Node<int>, Net_Arc<int, double>>;
  Info_Net net;
  auto s = net.insert_node(1);
  auto a = net.insert_node(2);
  auto t = net.insert_node(3);
  net.insert_arc(s, a, 5.0, 2.0, 7);
  net.insert_arc(a, t, 4.0, 1.5, 8);

  Info_Net copy(net);
  ASSERT_EQ(copy.get_num_nodes(), 3u);
  ASSERT_EQ(copy.get_num_arcs(), 2u);
  EXPECT_TRUE(copy.is_single_source());
  EXPECT_TRUE(copy.is_single_sink());
  double cap = 0, flow = 0;
  int info = 0;
  for (Info_Net::Arc_Iterator it(copy); it.has_curr(); it.next_ne())
    {
      cap += it.get_curr()->cap;
      flow += it.get_curr()->flow;
      info += it.get_curr()->get_info();
    }
  EXPECT_EQ(cap, 9.0);
  EXPECT_EQ(flow, 3.5);
  EXPECT_EQ(info, 15);

  Info_Net assigned;
  assigned = net;
  EXPECT_EQ(assigned.get_num_arcs(), 2u);

  auto zero = copy.insert_arc(copy.get_first_node(), copy.get_first_node());
  EXPECT_EQ(zero->cap, 0.0);
  EXPECT_EQ(zero->get_info(), 0);
}

// The search kept in each node the node it came from, and the path took,
// of the arcs between both, the first one with some slack: with parallel
// arcs, one with less slack than asked for
TEST(NetGraphAugmentingPaths, ASemiPathHasTheSlackItWasAskedFor)
{
  for (bool light_first : {true, false})
    {
      SCOPED_TRACE(light_first ? "light arc first" : "heavy arc first");
      Net net;
      auto s = net.insert_node(0);
      auto a = net.insert_node(1);
      auto t = net.insert_node(2);
      if (light_first)
        net.insert_arc(s, a, 1, 0);
      auto heavy = net.insert_arc(s, a, 5, 0);
      if (not light_first)
        net.insert_arc(s, a, 1, 0);
      net.insert_arc(a, t, 4, 0);

      DynList<Parc<Net>> path;
      const auto slack =
        Find_Aumenting_Path_BFS<Net>(net).semi_path(s, t, path, 3);
      EXPECT_EQ(slack, 4);
      ASSERT_EQ(path.size(), 2u);
      EXPECT_EQ(get<0>(path.get_first()), heavy);

      Path<Net> nodes = Find_Aumenting_Path_DFS<Net>(net)(s, t, 3);
      EXPECT_EQ(nodes.size(), 3u);
    }
}

namespace
{
  using Double_Net = Net_Graph<Net_Node<int>, Net_Arc<int, double>>;

  // A random network with a path from the node 0 to the last one, so that
  // it has one source and one sink, and capacities that are random reals,
  // or tenths, whose sums round
  Double_Net random_double_network(unsigned seed)
  {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> real(0.01, 10.0);
    Double_Net net;
    std::vector<Double_Net::Node *> v;
    const int n = 4 + int(gen() % 20);
    for (int i = 0; i < n; ++i)
      v.push_back(net.insert_node(i));
    auto cap = [&]() { return seed % 2 ? real(gen) : 0.1 * double(1 + gen() % 9); };
    for (int i = 0; i + 1 < n; ++i)
      net.insert_arc(v[i], v[i + 1], cap(), 0.0);
    for (int k = 0, m = int(gen() % 80); k < m; ++k)
      {
        const int a = int(gen() % (n - 1)), b = 1 + int(gen() % (n - 1));
        if (a < b)
          net.insert_arc(v[a], v[b], cap(), 0.0);
      }
    return net;
  }

  // Every flow is between zero and the capacity of its arc
  bool flows_within_capacities(Double_Net & net)
  {
    for (Double_Net::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      if (auto a = it.get_curr(); a->flow < 0 or a->flow > a->cap)
        return false;
    return true;
  }
}

// With a floating type of flow, an augmentation could leave a flow above
// the capacity of its arc, below zero, or next to zero on an arc of a node
// without other flow, by rounding: the checks of the network then failed,
// and so did its assertion in increase_flow(), in a build that has them
TEST(NetGraphAugmentingPaths, FloatingFlowsStayWithinTheirCapacities)
{
  for (unsigned seed = 0; seed < 300; ++seed)
    {
      SCOPED_TRACE(seed);
      Double_Net ff_net = random_double_network(seed);
      Double_Net ek_net = random_double_network(seed);
      Double_Net dinic_net = random_double_network(seed);
      ASSERT_TRUE(ff_net.is_single_source() and ff_net.is_single_sink());

      const double ff = ford_fulkerson_maximum_flow(ff_net);
      const double ek = edmonds_karp_maximum_flow(ek_net);
      const double dinic = dinic_maximum_flow(dinic_net);

      EXPECT_NEAR(ff, dinic, 1e-9 * std::max(1.0, dinic));
      EXPECT_NEAR(ek, dinic, 1e-9 * std::max(1.0, dinic));
      EXPECT_TRUE(flows_within_capacities(ff_net));
      EXPECT_TRUE(flows_within_capacities(ek_net));
      EXPECT_TRUE(ff_net.check_network());
      EXPECT_TRUE(ek_net.check_network());
    }
}

// A path of arcs of infinite capacity carries an infinite flow, which its
// nodes take in and send out: equal flows balance, also infinite ones,
// whose difference, inf - inf, is NaN, which check_node() and the
// comparison of the totals took as an imbalance, so that check_network()
// was false and flow_value() failed its assertion. An infinite flow into
// a node that sends 2.5 does not balance, which the tolerance of an
// infinite flow, infinite too, took as balanced
TEST(NetGraphInfiniteFlows, EqualInfiniteFlowsBalanceAndUnequalOnesDoNot)
{
  const double inf = std::numeric_limits<double>::infinity();
  Double_Net net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);
  auto sa = net.insert_arc(s, a, inf);
  auto at = net.insert_arc(a, t, inf);
  auto st = net.insert_arc(s, t, 2.5);
  sa->flow = inf;
  at->flow = inf;
  st->flow = 2.5;
  EXPECT_TRUE(net.check_node(a));
  EXPECT_TRUE(net.check_network());
  EXPECT_EQ(net.flow_value(), inf);

  at->flow = 2.5;  // a receives inf and sends 2.5
  EXPECT_FALSE(net.check_node(a));
  EXPECT_FALSE(net.check_network());
}

// Ford-Fulkerson and Edmonds-Karp take a path of arcs of infinite capacity
// and return the infinite maximum flow, which check_network() takes; with
// assertions, they aborted, as check_network() was false
TEST(NetGraphInfiniteFlows, TheAugmentingPathsReturnAnInfiniteMaximumFlow)
{
  const double inf = std::numeric_limits<double>::infinity();
  for (int algo = 0; algo < 2; ++algo)
    {
      Double_Net net;
      auto s = net.insert_node(0);
      auto a = net.insert_node(1);
      auto t = net.insert_node(2);
      net.insert_arc(s, a, inf);
      net.insert_arc(a, t, inf);
      net.insert_arc(s, t, 2.5);
      const double value = algo == 0 ? ford_fulkerson_maximum_flow(net)
                                     : edmonds_karp_maximum_flow(net);
      EXPECT_EQ(value, inf) << (algo == 0 ? "ford_fulkerson" : "edmonds_karp");
      EXPECT_TRUE(net.check_network()) << (algo == 0 ? "ford_fulkerson" : "edmonds_karp");
    }
}

// =============================================================================
// What Net_Graph tells a network derived from it
// =============================================================================

namespace
{
  // A network that counts the calls to swapped() and the arcs that are
  // disconnected from it
  struct Told_Net : Net
  {
    int swaps = 0;
    int disconnections = 0;

    void swapped() noexcept override { ++swaps; }

    void disconnect_arc(Arc *arc) override
    {
      ++disconnections;
      Net::disconnect_arc(arc);
    }
  };
}

// swap(), the move and the assignments of Net_Graph, through a reference to
// it, tell once each network whose nodes and arcs they exchange
TEST(NetGraphDerived, ASwapTellsItsTwoNetworks)
{
  Told_Net one, two;
  build_single_arc(one, 5);
  Net &base = one;

  base.swap(two);
  EXPECT_EQ(one.swaps, 1);
  EXPECT_EQ(two.swaps, 1);
  EXPECT_EQ(two.get_num_arcs(), 1U);

  base = std::move(static_cast<Net &>(two));  // a swap
  EXPECT_EQ(one.swaps, 2);
  EXPECT_EQ(two.swaps, 2);
  EXPECT_EQ(one.get_num_arcs(), 1U);

  Net plain;
  base = plain;  // a copy of plain, and a swap with it
  EXPECT_EQ(one.swaps, 3);
  EXPECT_EQ(two.swaps, 2);

  Net taken(std::move(base));  // it tells the network that it empties
  EXPECT_EQ(one.swaps, 4);
}

// a network swapped with itself keeps its nodes and its arcs: nothing to
// tell
TEST(NetGraphDerived, ASwapWithItselfTellsNothing)
{
  Told_Net net;
  build_single_arc(net, 5);
  Net &base = net;

  base.swap(base);
  EXPECT_EQ(net.swaps, 0);
  EXPECT_EQ(net.get_num_nodes(), 2U);
  EXPECT_EQ(net.get_num_arcs(), 1U);
}

// disconnect_arc() is virtual, as remove_arc()
TEST(NetGraphDerived, ADisconnectionThroughTheBaseIsThatOfTheDerivedNetwork)
{
  Told_Net net;
  auto sn = build_single_arc(net, 5);
  Net &base = net;

  base.disconnect_arc(sn.st);
  EXPECT_EQ(net.disconnections, 1);
  EXPECT_EQ(net.get_num_arcs(), 0U);

  base.connect_arc(sn.st);
  EXPECT_EQ(net.get_num_arcs(), 1U);
}

// =============================================================================
// Every network derived from Net_Graph, once its nodes and arcs changed
// through a reference to the base
// =============================================================================
//
// A network may keep something for its nodes and its arcs: Net_Graph its
// super nodes and its sets of sources and of sinks, Net_Cap_Graph an
// auxiliary network that points to them, Net_Sup_Dem_Graph its two super
// nodes, and Net_Cost_Graph nothing, since the costs are in the arcs.
// Whatever changes the nodes and the arcs through a reference to Net_Graph
// must leave the network either with what it keeps or knowing that it lost
// it: the use of what it keeps says so, and touches no node nor arc that
// the network no longer has, which the sanitizers see. A network derived
// from Net_Graph that is not in Derived_Networks is not checked here: add
// it, with its Kept_By.

namespace
{
  // For a network derived from Net_Graph: fill() gives it nodes, arcs and
  // what it keeps for them; use() uses all that it keeps and returns
  // whether the network had it; apart says whether it stays when the nodes
  // and the arcs go to another network
  template <class N>
  struct Kept_By;

  // its super source, over two sources
  template <>
  struct Kept_By<Net_Graph<>>
  {
    static constexpr bool apart = false;

    static void fill(Net_Graph<> &net)
    {
      auto a = net.insert_node();
      auto b = net.insert_node();
      auto c = net.insert_node();
      net.insert_arc(a, c, 1.0, 0.0);
      net.insert_arc(b, c, 1.0, 0.0);
      net.make_super_source();
    }

    static bool use(Net_Graph<> &net)
    {
      if (not net.with_super_source)
        return false;
      net.unmake_super_source();
      return true;
    }
  };

  // its auxiliary network, whose arcs point to its nodes and arcs
  template <>
  struct Kept_By<Net_Cap_Graph<>>
  {
    static constexpr bool apart = true;

    static void fill(Net_Cap_Graph<> &net)
    {
      auto a = net.insert_node(10.0);
      auto b = net.insert_node(5.0);
      auto c = net.insert_node(10.0);
      net.insert_arc(a, b, 8.0, 0.0);
      net.insert_arc(b, c, 8.0, 0.0);
      net.compute_aux_net();
    }

    static bool use(Net_Cap_Graph<> &net)
    {
      if (not net.has_aux_net())
        return false;
      bool used = true;
      try
        {
          net.update();
        }
      catch (const std::domain_error &)
        {
          used = false;
        }
      net.free_aux_net();
      return used;
    }
  };

  // its super source and its super sink, which are nodes of it
  template <>
  struct Kept_By<Net_Sup_Dem_Graph<>>
  {
    static constexpr bool apart = true;

    static void fill(Net_Sup_Dem_Graph<> &net)
    {
      auto s = net.insert_node(10.0);
      auto t = net.insert_node(-10.0);
      net.insert_arc(s, t, 20.0, 0.0);
      net.compute_aux_net();
    }

    static bool use(Net_Sup_Dem_Graph<> &net)
    {
      if (not net.exist_aux_net())
        return false;
      (void) net.is_feasible();
      DynDlist<Net_Sup_Dem_Graph<>::Node *> supplies, demands;
      net.non_feasible_nodes(supplies, demands);
      net.free_aux_net();
      return true;
    }
  };

  // nothing: the costs go with the arcs
  template <>
  struct Kept_By<Net_Cost_Graph<>>
  {
    static constexpr bool apart = false;

    static void fill(Net_Cost_Graph<> &net)
    {
      auto a = net.insert_node();
      auto b = net.insert_node();
      net.insert_arc(a, b, 5.0, 2.0);
    }

    static bool use(Net_Cost_Graph<> &net)
    {
      (void) net.flow_cost();
      return true;
    }
  };

  // Whether the numbers of nodes and of arcs of net are those that its
  // iterators see, and its sources and its sinks are nodes of it
  template <class B>
  bool sound(const B &net)
  {
    size_t nodes = 0, arcs = 0;
    for (typename B::Node_Iterator it(net); it.has_curr(); it.next_ne())
      ++nodes;
    for (typename B::Arc_Iterator it(net); it.has_curr(); it.next_ne())
      ++arcs;
    if (nodes != net.get_num_nodes() or arcs != net.get_num_arcs())
      return false;
    auto is_node = [&net] (typename B::Node *p)
      {
        for (typename B::Node_Iterator it(net); it.has_curr(); it.next_ne())
          if (it.get_curr() == p)
            return true;
        return false;
      };
    for (auto it = net.get_src_nodes().get_it(); it.has_curr(); it.next_ne())
      if (not is_node(it.get_curr()))
        return false;
    for (auto it = net.get_sink_nodes().get_it(); it.has_curr(); it.next_ne())
      if (not is_node(it.get_curr()))
        return false;
    return true;
  }

  // three nodes in a chain, with nothing kept for them
  template <class B>
  void fill_chain(B &net)
  {
    auto x = net.insert_node();
    auto y = net.insert_node();
    auto z = net.insert_node();
    net.insert_arc(x, y, 1.0, 0.0);
    net.insert_arc(y, z, 1.0, 0.0);
  }

  template <class N>
  class NetDerivedChanged : public ::testing::Test
  {};

  using Derived_Networks = ::testing::Types<Net_Graph<>, Net_Cap_Graph<>, Net_Sup_Dem_Graph<>, Net_Cost_Graph<>>;
}

TYPED_TEST_SUITE(NetDerivedChanged, Derived_Networks);

// The ways that take all the nodes and the arcs of the network to another
// one, or free them: the assignments, the swap() and the move of Net_Graph
TYPED_TEST(NetDerivedChanged, ItKnowsThatItsNodesWentThroughTheBase)
{
  using N = TypeParam;
  using B = Net_Graph<typename N::Node, typename N::Arc>;
  using K = Kept_By<N>;
  const char *const ways[] = {"a copy assignment", "a move assignment", "a swap()", "a move construction",
                              "a swap() with another one that keeps the same"};
  // with the other network alive when what the network keeps is used, and
  // gone, which frees the nodes that it took
  for (const bool gone : {false, true})
    for (int way = 0; way < 5; ++way)
      {
        SCOPED_TRACE(std::string(ways[way]) + (gone ? ", and the other network gone" : ""));
        N net;
        K::fill(net);
        B &base = net;
        auto plain = std::make_unique<B>();
        fill_chain(*plain);
        auto other = std::make_unique<N>();
        K::fill(*other);
        if (way == 0)
          base = *plain;
        else if (way == 1)
          base = std::move(*plain);
        else if (way == 2)
          base.swap(*plain);
        else if (way == 3)
          *plain = B(std::move(base));
        else
          base.swap(static_cast<B &>(*other));
        if (gone)
          {
            plain.reset();
            other.reset();
          }

        const bool used = K::use(net);
        if (K::apart)
          {
            EXPECT_FALSE(used); // what it kept was for the nodes that went
          }
        EXPECT_TRUE(sound(base));
        if (not gone)
          {
            if (way == 4)
              {
                const bool other_used = K::use(*other);
                if (K::apart)
                  {
                    EXPECT_FALSE(other_used);
                  }
              }
            EXPECT_TRUE(sound(*plain));
            EXPECT_TRUE(sound(static_cast<B &>(*other)));
          }
      }
}

// The ways that take some of them, or none: what the network keeps is used
// then, or refused, and nothing that is gone is touched
TYPED_TEST(NetDerivedChanged, ItUsesNothingThatIsGoneAfterAChangeThroughTheBase)
{
  using N = TypeParam;
  using B = Net_Graph<typename N::Node, typename N::Arc>;
  using K = Kept_By<N>;
  const char *const ways[] = {"nothing", "a swap() with itself", "an assignment to itself", "a move to itself",
                              "remove_node() of its first node", "remove_arc() of its first arc",
                              "an arc disconnected and freed", "clear_graph()", "copy_graph() into it"};
  for (int way = 0; way < 9; ++way)
    {
      SCOPED_TRACE(ways[way]);
      N net;
      K::fill(net);
      B &base = net;
      B &same = net;
      B plain;
      fill_chain(plain);
      if (way == 1)
        base.swap(same);
      else if (way == 2)
        base = static_cast<const B &>(same);
      else if (way == 3)
        base = std::move(same);
      else if (way == 4)
        base.remove_node(base.get_first_node());
      else if (way == 5)
        base.remove_arc(base.get_first_arc());
      else if (way == 6)
        {
          auto a = base.get_first_arc();
          base.disconnect_arc(a);
          delete a;
        }
      else if (way == 7)
        clear_graph(base);
      else if (way == 8)
        copy_graph(base, plain);

      const bool used = K::use(net);
      if (way < 4)
        {
          EXPECT_TRUE(used); // the network is the one that it was
        }
      EXPECT_TRUE(sound(base));
      EXPECT_TRUE(sound(plain));
    }
}
