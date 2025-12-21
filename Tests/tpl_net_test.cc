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

#include <gtest/gtest.h>

#include <set>

#include <tpl_net.H>

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
