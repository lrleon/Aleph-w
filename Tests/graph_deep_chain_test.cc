
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
 * @file graph_deep_chain_test.cc
 * @brief Depth-first searches over a path as long as the graph.
 *
 * A depth-first search goes as deep as the longest path that it follows,
 * which in a chain of n nodes is n. The searches that recursed kept a
 * frame of the call stack for each node, so that a chain of some
 * thousands of nodes overflowed a small stack; these tests run them in a
 * thread with a stack of 256 KiB, over chains of 100000 nodes.
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#  include <pthread.h>
#endif

#include <tpl_graph.H>
#include <Planarity_Test.H>
#include <Tarjan.H>
#include <generate_df_tree.H>
#include <graph_to_tree.H>
#include <kosaraju.H>
#include <topological_sort.H>
#include <tpl_components.H>
#include <tpl_cut_nodes.H>
#include <tpl_find_path.H>
#include <tpl_graph_utils.H>
#include <tpl_bipartite.H>
#include <tpl_spanning_tree.H>
#include <tpl_test_acyclique.H>
#include <tpl_test_cycle.H>
#include <tpl_test_path.H>

using namespace Aleph;

namespace
{
  constexpr size_t N = 100000;

  using Chain_Digraph = List_Digraph<Graph_Node<size_t>, Graph_Arc<size_t>>;
  using Chain_Graph = List_Graph<Graph_Node<size_t>, Graph_Arc<size_t>>;

  // Runs f in a thread with a stack of 256 KiB, in which a recursion as
  // deep as the chains of these tests overflows. Without POSIX threads,
  // it runs f in this thread.
  template <class F>
  void run_with_small_stack(F f)
  {
# if defined(__unix__) || defined(__APPLE__)
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
# else
    f();
# endif
  }

  // Inserts in g the chain 0 -> 1 -> ... -> n - 1 and, if closed, the arc
  // n - 1 -> 0; returns its nodes
  template <class GT>
  std::vector<typename GT::Node *> build_chain(GT & g, size_t n, bool closed)
  {
    std::vector<typename GT::Node *> nodes;
    nodes.reserve(n);
    for (size_t i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    for (size_t i = 0; i + 1 < n; ++i)
      g.insert_arc(nodes[i], nodes[i + 1], i);
    if (closed)
      g.insert_arc(nodes[n - 1], nodes[0], n - 1);
    return nodes;
  }
}

// Tarjan_Connected_Components: each node of an open chain is a component
TEST(DeepChain, TarjanFindsTheComponentsOfAnOpenChain)
{
  Chain_Digraph g;
  build_chain(g, N, false);
  size_t num = 0;
  bool has_cycle = true;
  bool is_dag = false;
  DynList<size_t> sizes;
  run_with_small_stack([&]
    {
      Tarjan_Connected_Components<Chain_Digraph> tarjan;
      num = tarjan.num_connected_components(g);
      has_cycle = tarjan.has_cycle(g);
      is_dag = tarjan.is_dag(g);
      tarjan.connected_components(g, sizes);
    });
  EXPECT_EQ(num, N);
  EXPECT_FALSE(has_cycle);
  EXPECT_TRUE(is_dag);
  EXPECT_EQ(sizes.size(), N);
  EXPECT_TRUE(sizes.all([](size_t s) { return s == 1; }));
}

// and a closed chain is one component, and a cycle
TEST(DeepChain, TarjanFindsTheComponentOfAClosedChain)
{
  Chain_Digraph g;
  build_chain(g, N, true);
  DynList<Chain_Digraph> blocks;
  DynList<Chain_Digraph::Arc *> between;
  DynList<DynList<Chain_Digraph::Node *>> lists;
  bool connected = false;
  bool has_cycle = false;
  bool found = false;
  Path<Chain_Digraph> cycle(g);
  run_with_small_stack([&]
    {
      Tarjan_Connected_Components<Chain_Digraph> tarjan;
      tarjan.connected_components(g, blocks, between);
      tarjan.connected_components(g, lists);
      connected = tarjan.test_connectivity(g);
      has_cycle = tarjan.has_cycle(g);
      found = tarjan.compute_cycle(g, cycle);
    });
  ASSERT_EQ(blocks.size(), 1u);
  EXPECT_EQ(blocks.get_first().get_num_nodes(), N);
  EXPECT_EQ(blocks.get_first().get_num_arcs(), N);
  EXPECT_TRUE(between.is_empty());
  ASSERT_EQ(lists.size(), 1u);
  EXPECT_EQ(lists.get_first().size(), N);
  EXPECT_TRUE(connected);
  EXPECT_TRUE(has_cycle);
  ASSERT_TRUE(found);
  EXPECT_EQ(cycle.size(), N + 1); // the first node closes it
  EXPECT_EQ(cycle.get_first_node(), cycle.get_last_node());
}

// Compute_Cut_Nodes and Compute_Bridges: the inner nodes of a path are cut
// nodes, and its arcs bridges; a cycle has none
TEST(DeepChain, CutNodesAndBridgesOfAPathAndOfACycle)
{
  Chain_Graph path;
  build_chain(path, N, false);
  Chain_Graph cycle;
  build_chain(cycle, N, true);
  DynDlist<Chain_Graph::Node *> path_cuts, cycle_cuts;
  size_t path_bridges = 0, cycle_bridges = 1;
  Chain_Graph cut_graph;
  DynDlist<Chain_Graph::Arc *> cross;
  run_with_small_stack([&]
    {
      Compute_Cut_Nodes<Chain_Graph> cc(path);
      cc(path.get_first_node(), path_cuts);
      cc.paint_subgraphs();
      cc.map_cut_graph(cut_graph, cross);
      Compute_Cut_Nodes<Chain_Graph> cycle_cc(cycle);
      cycle_cc(cycle.get_first_node(), cycle_cuts);
      path_bridges = find_bridges(path, path.get_first_node()).size();
      cycle_bridges = find_bridges(cycle, cycle.get_first_node()).size();
    });
  EXPECT_EQ(path_cuts.size(), N - 2);
  EXPECT_EQ(cut_graph.get_num_nodes(), N - 2);
  EXPECT_EQ(cut_graph.get_num_arcs(), N - 3);
  EXPECT_EQ(cross.size(), 2u);
  EXPECT_TRUE(cycle_cuts.is_empty());
  EXPECT_EQ(path_bridges, N - 1);
  EXPECT_EQ(cycle_bridges, 0u);
}

// The block of a cycle with a node hung from it is as deep as the cycle;
// painting it and copying it go through it
TEST(DeepChain, PaintsAndCopiesTheBlockOfACycle)
{
  for (bool utils : {false, true})
    {
      SCOPED_TRACE(utils ? "tpl_graph_utils.H" : "Compute_Cut_Nodes");
      Chain_Graph g;
      auto nodes = build_chain(g, N, true);
      auto hung = g.insert_node(N);
      g.insert_arc(nodes[0], hung, N);
      size_t cuts = 0, block_nodes = 0, block_arcs = 0, blocks = 0;
      run_with_small_stack([&]
        {
          if (utils)
            {
              auto list = compute_cut_nodes(g, nodes[1]);
              cuts = list.size();
              paint_subgraphs(g, list);
              Chain_Graph block = map_subgraph(g, NODE_COUNTER(nodes[1]));
              block_nodes = block.get_num_nodes();
              block_arcs = block.get_num_arcs();
              return;
            }
          Compute_Cut_Nodes<Chain_Graph> cc(g);
          DynDlist<Chain_Graph::Node *> list;
          cc(nodes[1], list);
          cuts = list.size();
          cc.paint_subgraphs();
          Chain_Graph block;
          cc.map_subgraph(block, NODE_COUNTER(nodes[1]));
          block_nodes = block.get_num_nodes();
          block_arcs = block.get_num_arcs();

          Compute_Cut_Nodes<Chain_Graph> all(g);
          DynDlist<Chain_Graph::Node *> again;
          all(nodes[1], again);
          DynDlist<Chain_Graph> block_list;
          Chain_Graph cut_graph;
          DynDlist<Chain_Graph::Arc *> cross;
          all.compute_blocks(block_list, cut_graph, cross);
          for (auto it = block_list.get_it(); it.has_curr(); it.next_ne())
            blocks += it.get_curr().get_num_nodes() > 0;
        });
      EXPECT_EQ(cuts, 1u);
      EXPECT_EQ(block_nodes, N - 1); // the cut node is not in the block
      EXPECT_EQ(block_arcs, N - 2);
      if (not utils)
        {
          EXPECT_EQ(blocks, 2u);
        }
    }
}

// Build_Subgraph, Unconnected_Components and inconnected_components()
TEST(DeepChain, CopiesTheComponentOfAPath)
{
  Chain_Graph g;
  auto nodes = build_chain(g, N, false);
  size_t sub_arcs = 0, list_nodes = 0, components = 0, utils_nodes = 0;
  run_with_small_stack([&]
    {
      g.reset_nodes();
      g.reset_arcs();
      sub_arcs = Build_Subgraph<Chain_Graph>()(g, nodes[N / 2]).get_num_arcs();
      g.reset_nodes();
      g.reset_arcs();
      DynList<Chain_Graph::Node *> list;
      Build_Subgraph<Chain_Graph>()(g, list, nodes[N / 2]);
      list_nodes = list.size();
      components = Unconnected_Components<Chain_Graph>().count_components(g);
      auto blocks = inconnected_components(g);
      utils_nodes = blocks.get_first().get_num_nodes();
    });
  EXPECT_EQ(sub_arcs, N - 1);
  EXPECT_EQ(list_nodes, N);
  EXPECT_EQ(components, 1u);
  EXPECT_EQ(utils_nodes, N);
}

// Find_Path_Depth_First and the tests of a path or a cycle from one node
TEST(DeepChain, FindsAndTestsThePathOfAChain)
{
  Chain_Graph path;
  auto p = build_chain(path, N, false);
  Chain_Graph cycle;
  auto c = build_chain(cycle, N, true);
  size_t found = 0, free_found = 0, dft = 0, dft_class = 0, tree_arcs = 0,
         class_tree_arcs = 0;
  bool to_end = false, class_to_end = false, cycle_found = false, class_cycle = false;
  bool path_acyclique = false, cycle_acyclique = true, class_acyclique = false,
       class_has_cycle = false;
  run_with_small_stack([&]
    {
      Path<Chain_Graph> found_path(path);
      Find_Path_Depth_First<Chain_Graph>()(path, p[0], p[N - 1], found_path);
      found = found_path.size();
      free_found = find_path_depth_first(path, p[0], p[N - 1]).size();
      dft = depth_first_traversal<Chain_Graph>(path, p[0], nullptr);
      dft_class = Depth_First_Traversal<Chain_Graph>()(path, p[0]);
      to_end = test_for_path(path, p[0], p[N - 1]);
      class_to_end = Test_For_Path<Chain_Graph>()(path, p[0], p[N - 1]);
      cycle_found = test_for_cycle(cycle, c[0]);
      class_cycle = Test_For_Cycle<Chain_Graph>()(cycle, c[0]);
      path_acyclique = is_graph_acyclique(path);
      cycle_acyclique = is_graph_acyclique(cycle, c[0]);
      class_acyclique = Is_Graph_Acyclique<Chain_Graph>()(path);
      class_has_cycle = Has_Cycle<Chain_Graph>()(cycle);
      tree_arcs = find_depth_first_spanning_tree(path, p[0]).get_num_arcs();
      Chain_Graph tree;
      Find_Depth_First_Spanning_Tree<Chain_Graph>()(path, p[0], tree);
      class_tree_arcs = tree.get_num_arcs();
    });
  EXPECT_EQ(found, N);
  EXPECT_EQ(free_found, N);
  EXPECT_EQ(dft, N);
  EXPECT_EQ(dft_class, N);
  EXPECT_TRUE(to_end);
  EXPECT_TRUE(class_to_end);
  EXPECT_TRUE(cycle_found);
  EXPECT_TRUE(class_cycle);
  EXPECT_TRUE(path_acyclique);
  EXPECT_FALSE(cycle_acyclique);
  EXPECT_TRUE(class_acyclique);
  EXPECT_TRUE(class_has_cycle);
  EXPECT_EQ(tree_arcs, N - 1);
  EXPECT_EQ(class_tree_arcs, N - 1);
}

// Kosaraju and the topological sort of a directed chain
TEST(DeepChain, KosarajuAndTopologicalSortOfAChain)
{
  Chain_Digraph open, closed;
  auto nodes = build_chain(open, N, false);
  build_chain(closed, N, true);
  size_t open_components = 0, closed_components = 0, blocks = 0, sorted = 0;
  Chain_Digraph::Node * first = nullptr;
  run_with_small_stack([&]
    {
      open_components = kosaraju_connected_components(open).size();
      closed_components = kosaraju_connected_components(closed).size();
      DynList<Chain_Digraph> blk_list;
      DynList<Chain_Digraph::Arc *> arcs;
      kosaraju_connected_components(closed, blk_list, arcs);
      blocks = blk_list.size();
      DynDlist<Chain_Digraph::Node *> order;
      Topological_Sort<Chain_Digraph>()(open, order);
      sorted = order.size();
      first = order.get_first();
    });
  EXPECT_EQ(open_components, N);
  EXPECT_EQ(closed_components, 1u);
  EXPECT_EQ(blocks, 1u);
  EXPECT_EQ(sorted, N);
  EXPECT_EQ(first, nodes[0]);
}

namespace
{
  struct Key_Of_Node
  {
    void operator()(Chain_Graph::Node * p, Tree_Node<size_t> * t) const
    {
      t->get_key() = p->get_info();
    }
  };

  // The height of a tree that is a chain
  template <class Key>
  size_t height_of_chain(Tree_Node<Key> * root)
  {
    size_t height = 0;
    for (auto t = root; t != nullptr; t = t->get_left_child())
      ++height;
    return height;
  }
}

// graph_to_tree_node() and Graph_To_Tree_Node build a tree as deep as the
// path from which they build it, which destroy_tree() frees
TEST(DeepChain, BuildsTheTreeOfAPath)
{
  Chain_Graph g;
  auto nodes = build_chain(g, N, false);
  size_t height = 0, class_height = 0;
  run_with_small_stack([&]
    {
      auto tree = graph_to_tree_node<Chain_Graph, size_t, Key_Of_Node>(g, nodes[0]);
      g.reset_arcs();
      auto class_tree = Graph_To_Tree_Node<Chain_Graph, size_t, Key_Of_Node>()(g, nodes[0]);
      height = height_of_chain(tree);
      class_height = height_of_chain(class_tree);
      destroy_tree(tree);
      destroy_tree(class_tree);
    });
  EXPECT_EQ(height, N);
  EXPECT_EQ(class_height, N);
}

// build_df_low_tree() builds the depth-first tree of a cycle, a path as
// long as the graph with an arc back to the start
TEST(DeepChain, BuildsTheDfLowTreeOfACycle)
{
  Chain_Graph g;
  auto nodes = build_chain(g, N, true);
  size_t height = 0, non_tree_arcs = 0;
  long low_of_last = -2;
  run_with_small_stack([&]
    {
      DynList<Df_Low_Non_Tree_Arc> arcs;
      auto root = build_df_low_tree(g, nodes[0], arcs);
      height = height_of_chain(root);
      non_tree_arcs = arcs.size();
      auto last = root;
      while (last->get_left_child() != nullptr)
        last = last->get_left_child();
      low_of_last = last->get_key().low;
      destroy_tree(root);
    });
  EXPECT_EQ(height, N);
  EXPECT_EQ(non_tree_arcs, 1u); // from the last node to the start
  EXPECT_EQ(low_of_last, 0);
}

// L1 ... LN are matched with R1 ... RN in the first phase of Hopcroft-Karp,
// and L0, whose only arc goes to R1, is left free: the augmenting path from
// it, L0 R1 L1 R2 ... LN RN+1, goes through all the nodes
TEST(DeepChain, HopcroftKarpFollowsALongAugmentingPath)
{
  Chain_Graph g;
  std::vector<Chain_Graph::Node *> left(N + 1), right(N + 2);
  for (size_t i = 1; i <= N; ++i) // L1 first: the coloring starts there
    left[i] = g.insert_node(i);
  left[0] = g.insert_node(size_t(0));
  for (size_t i = 1; i <= N + 1; ++i)
    right[i] = g.insert_node(N + i);
  g.insert_arc(left[0], right[1], size_t(0));
  for (size_t i = 1; i <= N; ++i)
    {
      g.insert_arc(left[i], right[i], 2 * i - 1); // the first phase takes it
      g.insert_arc(left[i], right[i + 1], 2 * i);
    }

  size_t matched = 0;
  run_with_small_stack([&]
    {
      DynDlist<Chain_Graph::Arc *> matching;
      hopcroft_karp_matching(g, matching);
      matched = matching.size();
    });
  EXPECT_EQ(matched, N + 1);
}

// The LR planarity test orients and tests the edges in two depth-first
// searches as deep as the path, and its embedding follows chains of
// references as long
TEST(DeepChain, TestsThePlanarityOfACycle)
{
  Chain_Graph g;
  build_chain(g, N, true);
  bool planar = false, embedded = false;
  size_t faces = 0;
  run_with_small_stack([&]
    {
      Planarity_Test_Options options;
      options.compute_embedding = true;
      options.embedding_allow_bruteforce_fallback = false;
      auto result = planarity_test(g, options);
      planar = result.is_planar;
      embedded = result.has_combinatorial_embedding;
      faces = result.embedding_num_faces;
    });
  EXPECT_TRUE(planar);
  EXPECT_TRUE(embedded);
  EXPECT_EQ(faces, 2u); // inside and outside
}
