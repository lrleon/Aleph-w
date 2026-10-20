/**
 * @file generate_df_tree_test.cc
 * @brief Tests for generate_df_tree.H
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <generate_df_tree.H>
#include <tpl_cut_nodes.H>
#include <tpl_graph_utils.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;

  // Owner of a tree returned by build_df_low_tree()
  using Tree_Ptr = std::unique_ptr<Df_Low_Tree_Node, void (*)(Df_Low_Tree_Node *)>;

  Tree_Ptr own(Df_Low_Tree_Node * root)
  {
    return Tree_Ptr(root, [] (Df_Low_Tree_Node * p) { destroy_tree(p); });
  }

  // Two triangles, 0-1-2 and 1-3-4, and the arc 4-5: the cut nodes are 1
  // and 4, and the only bridge is 4-5. The info of an arc u-v is 10 u + v.
  struct Two_Triangles
  {
    Graph g;
    std::vector<Node *> n;

    Two_Triangles()
    {
      for (int i = 0; i < 6; ++i)
        n.push_back(g.insert_node(i));

      const int arcs[][2] = {{0, 1}, {1, 2}, {2, 0}, {1, 3}, {3, 4}, {4, 1}, {4, 5}};
      for (const auto & a : arcs)
        g.insert_arc(n[a[0]], n[a[1]], 10 * a[0] + a[1]);
    }
  };

  // The arcs are traversed in the order of insertion, so from 0 the tree is
  // 0 - 1 - {2, 3 - 4 - 5}, and the other arcs are 2-0 and 4-1
  const std::string two_triangles_low =
    "Root \"0,0,-\" \n"
    "Node 0 \"1,1,0\" \n"
    "Node 0.0 \"2,2,0\" \n"
    "Node 0.0 \"3,3,1\" \n"
    "Node 0.0.1 \"4,4,1\" \n"
    "Node 0.0.1.0 \"5,5,5\" \n"
    "Dashed-Connexion 0.0.0 0 L\n"
    "Dashed-Connexion 0.0.1.0 0.0 R\n";

  // Hides the arc whose info is info
  struct Hide_Arc
  {
    int info = -1;

    bool operator () (Arc * a) const noexcept
    {
      return a->get_info() != info;
    }
  };

  // Tree nodes of a tree in preorder
  std::vector<Df_Low_Tree_Node *> preorder(Df_Low_Tree_Node * root)
  {
    std::vector<Df_Low_Tree_Node *> nodes;
    std::function<void(Df_Low_Tree_Node *)> visit = [&] (Df_Low_Tree_Node * p)
    {
      nodes.push_back(p);
      for (auto c = p->get_left_child(); c != nullptr; c = c->get_right_sibling())
        visit(c);
    };
    visit(root);
    return nodes;
  }

  Df_Low_Tree_Node * find_label(Df_Low_Tree_Node * root, const std::string & label)
  {
    for (auto p : preorder(root))
      if (p->get_key().label == label)
        return p;
    return nullptr;
  }

  std::pair<std::string, std::string> labels(const Df_Low_Non_Tree_Arc & a)
  {
    return {a.first->get_key().label, a.second->get_key().label};
  }

  std::vector<std::pair<std::string, std::string>>
  labels(const DynList<Df_Low_Non_Tree_Arc> & arcs)
  {
    std::vector<std::pair<std::string, std::string>> ret;
    arcs.for_each([&ret] (const Df_Low_Non_Tree_Arc & a) { ret.push_back(labels(a)); });
    return ret;
  }

  using Label_Pairs = std::vector<std::pair<std::string, std::string>>;
}

TEST(Df_Low_Tree, writes_the_tree_with_df_low_and_the_other_arcs)
{
  Two_Triangles t;
  std::ostringstream out;
  write_df_low_tree(t.g, t.n[0], out);
  EXPECT_EQ(out.str(), two_triangles_low);
}

TEST(Df_Low_Tree, writes_the_labels_that_the_writer_chooses)
{
  Two_Triangles t;

  std::ostringstream df;
  write_df_low_tree<Write_Df_Tree_Df>(t.g, t.n[0], df);
  EXPECT_EQ(df.str(),
            "Root \"(0,0)\" \n"
            "Node 0 \"(1,1)\" \n"
            "Node 0.0 \"(2,2)\" \n"
            "Node 0.0 \"(3,3)\" \n"
            "Node 0.0.1 \"(4,4)\" \n"
            "Node 0.0.1.0 \"(5,5)\" \n"
            "Dashed-Connexion 0.0.0 0 L\n"
            "Dashed-Connexion 0.0.1.0 0.0 R\n");

  std::ostringstream label;
  write_df_low_tree<Write_Df_Tree_Label>
    (t.g, t.n[0], label,
     [] (Node * p) { return std::string(1, char('A' + p->get_info())); });
  EXPECT_EQ(label.str(),
            "Root \"A\" \n"
            "Node 0 \"B\" \n"
            "Node 0.0 \"C\" \n"
            "Node 0.0 \"D\" \n"
            "Node 0.0.1 \"E\" \n"
            "Node 0.0.1.0 \"F\" \n"
            "Dashed-Connexion 0.0.0 0 L\n"
            "Dashed-Connexion 0.0.1.0 0.0 R\n");
}

TEST(Df_Low_Tree, builds_the_tree_and_the_other_arcs)
{
  Two_Triangles t;
  DynList<Df_Low_Non_Tree_Arc> others;
  auto tree = own(build_df_low_tree(t.g, t.n[0], others));

  std::vector<std::string> keys;
  for (auto p : preorder(tree.get()))
    {
      const Df_Low_Key & k = p->get_key();
      keys.push_back(k.label + ":" + std::to_string(k.df) + ":" + std::to_string(k.low));
    }
  EXPECT_EQ(keys, (std::vector<std::string>{"0:0:-1", "1:1:0", "2:2:0", "3:3:1",
                                             "4:4:1", "5:5:5"}));
  EXPECT_EQ(labels(others), (Label_Pairs{{"2", "0"}, {"4", "1"}}));
}

TEST(Df_Low_Tree, the_tree_depends_on_the_start_node)
{
  Two_Triangles t;
  std::ostringstream out;
  write_df_low_tree(t.g, t.n[5], out);
  EXPECT_EQ(out.str(),
            "Root \"5,0,-\" \n"
            "Node 0 \"4,1,1\" \n"
            "Node 0.0 \"3,2,1\" \n"
            "Node 0.0.0 \"1,3,1\" \n"
            "Node 0.0.0.0 \"0,4,3\" \n"
            "Node 0.0.0.0.0 \"2,5,3\" \n"
            "Dashed-Connexion 0.0.0.0.0.0 0.0.0.0 L\n"
            "Dashed-Connexion 0.0.0.0 0.0 R\n");
}

TEST(Df_Low_Tree, a_single_node_is_only_a_root)
{
  Graph g;
  Node * p = g.insert_node(7);
  std::ostringstream out;
  write_df_low_tree(g, p, out);
  EXPECT_EQ(out.str(), "Root \"7,0,-\" \n");
}

TEST(Df_Low_Tree, the_default_label_writes_the_node_info)
{
  List_Graph<Graph_Node<std::string>, Graph_Arc<int>> g;
  auto a = g.insert_node("a");
  auto b = g.insert_node("b");
  g.insert_arc(a, b, 0);

  std::ostringstream out;
  write_df_low_tree(g, a, out);
  EXPECT_EQ(out.str(), "Root \"a,0,-\" \nNode 0 \"b,1,1\" \n");
}

TEST(Df_Low_Tree, a_parallel_arc_is_another_arc)
{
  Two_Triangles t;
  t.g.insert_arc(t.n[4], t.n[5], 45);

  DynList<Df_Low_Non_Tree_Arc> others;
  auto tree = own(build_df_low_tree(t.g, t.n[0], others));
  EXPECT_EQ(labels(others), (Label_Pairs{{"2", "0"}, {"4", "1"}, {"5", "4"}}));
  EXPECT_EQ(find_label(tree.get(), "5")->get_key().low, 4); // 4-5 is no bridge
}

TEST(Df_Low_Tree, an_arc_from_a_node_to_itself_is_left_out)
{
  Two_Triangles t;
  t.g.insert_arc(t.n[3], t.n[3], 33);

  std::ostringstream out;
  write_df_low_tree(t.g, t.n[0], out);
  EXPECT_EQ(out.str(), two_triangles_low);
}

TEST(Df_Low_Tree, only_the_arcs_that_the_filter_shows_count)
{
  Two_Triangles t;
  DynList<Df_Low_Non_Tree_Arc> others;
  auto tree = own(build_df_low_tree(t.g, t.n[0], others,
                                    Dft_Df_Tree_Label<Graph>(), Hide_Arc{20}));

  EXPECT_EQ(labels(others), (Label_Pairs{{"4", "1"}}));
  EXPECT_EQ(find_label(tree.get(), "2")->get_key().low, 2);
  EXPECT_EQ(find_label(tree.get(), "1")->get_key().low, 1);
}

TEST(Df_Low_Tree, only_the_nodes_that_the_start_node_reaches_are_in_the_tree)
{
  Two_Triangles t;
  Node * x = t.g.insert_node(10);
  Node * y = t.g.insert_node(11);
  t.g.insert_arc(x, y, 1011);
  t.g.insert_arc(y, x, 1110);

  DynList<Df_Low_Non_Tree_Arc> others;
  auto tree = own(build_df_low_tree(t.g, t.n[0], others));
  EXPECT_EQ(preorder(tree.get()).size(), 6U);
  EXPECT_EQ(labels(others), (Label_Pairs{{"2", "0"}, {"4", "1"}}));

  auto other = own(build_df_low_tree(t.g, x, others));
  EXPECT_EQ(preorder(other.get()).size(), 2U);
  EXPECT_EQ(labels(others), (Label_Pairs{{"11", "10"}}));
}

TEST(Df_Low_Tree, does_not_modify_the_graph)
{
  Two_Triangles t;
  for (auto p : t.n)
    {
      NODE_COOKIE(p) = p;
      NODE_COUNTER(p) = 100 + p->get_info();
    }
  for (auto it = t.g.get_arc_it(); it.has_curr(); it.next_ne())
    ARC_COOKIE(it.get_curr()) = it.get_curr();

  std::ostringstream out;
  write_df_low_tree(t.g, t.n[0], out);
  DynList<Df_Low_Non_Tree_Arc> others;
  auto tree = own(build_df_low_tree(t.g, t.n[0], others));

  for (auto p : t.n)
    {
      EXPECT_EQ(NODE_COOKIE(p), p);
      EXPECT_EQ(NODE_COUNTER(p), 100 + p->get_info());
    }
  for (auto it = t.g.get_arc_it(); it.has_curr(); it.next_ne())
    EXPECT_EQ(ARC_COOKIE(it.get_curr()), it.get_curr());
}

TEST(Df_Low_Tree, rejects_a_digraph_and_a_null_start_node)
{
  List_Digraph<Graph_Node<int>, Graph_Arc<int>> dg;
  auto a = dg.insert_node(0);
  dg.insert_arc(a, dg.insert_node(1), 0);

  DynList<Df_Low_Non_Tree_Arc> others;
  std::ostringstream out;
  EXPECT_THROW((void) build_df_low_tree(dg, a, others), std::domain_error);
  EXPECT_THROW(write_df_low_tree(dg, a, out), std::domain_error);

  Two_Triangles t;
  EXPECT_THROW((void) build_df_low_tree(t.g, nullptr, others), std::invalid_argument);
  EXPECT_THROW(write_df_low_tree(t.g, nullptr, out), std::invalid_argument);
  EXPECT_TRUE(out.str().empty());
}

TEST(Df_Low_Tree, an_exception_leaves_nothing_behind)
{
  Two_Triangles t;
  int calls = 0;
  auto label = [&calls] (Node * p)
  {
    if (++calls == 4) // node 3, after the arc 2-0 was found
      throw std::runtime_error("label");
    return std::to_string(p->get_info());
  };

  DynList<Df_Low_Non_Tree_Arc> others;
  EXPECT_THROW((void) build_df_low_tree(t.g, t.n[0], others, label), std::runtime_error);
  EXPECT_TRUE(others.is_empty());

  calls = 0;
  std::ostringstream out;
  EXPECT_THROW(write_df_low_tree(t.g, t.n[0], out, label), std::runtime_error);
  EXPECT_TRUE(out.str().empty());
}

TEST(Df_Low_Tree, a_tree_with_more_levels_than_generate_tree_writes_throws)
{
  for (const size_t n : {Max_Tree_Node_Depth, Max_Tree_Node_Depth + 1})
    {
      Graph g; // a path of n nodes, a tree of n levels
      Node * first = g.insert_node(0);
      Node * last = first;
      for (size_t i = 1; i < n; ++i)
        {
          Node * p = g.insert_node(int(i));
          g.insert_arc(last, p, 0);
          last = p;
        }

      std::ostringstream out;
      if (n <= Max_Tree_Node_Depth)
        EXPECT_NO_THROW(write_df_low_tree(g, first, out));
      else
        EXPECT_THROW(write_df_low_tree(g, first, out), std::overflow_error);
    }
}

// The df and low numbers give the same cut nodes and bridges as
// compute_cut_nodes() and find_bridges(), on random connected graphs with
// cycles and parallel arcs
TEST(Df_Low_Tree, agrees_with_cut_nodes_and_bridges)
{
  std::mt19937 rng(20261020);
  for (int round = 0; round < 200; ++round)
    {
      Graph g;
      const int num_nodes = 1 + int(rng() % 40);
      std::vector<Node *> n;
      for (int i = 0; i < num_nodes; ++i)
        n.push_back(g.insert_node(i));
      for (int i = 1; i < num_nodes; ++i) // a spanning tree
        g.insert_arc(n[i], n[rng() % i], 0);
      for (int k = int(rng() % (num_nodes + 1)); k > 0; --k)
        if (int u = int(rng() % num_nodes), v = int(rng() % num_nodes); u != v)
          g.insert_arc(n[u], n[v], 0);

      Node * src = n[rng() % num_nodes];
      DynList<Df_Low_Non_Tree_Arc> others;
      auto tree = own(build_df_low_tree(g, src, others));

      const auto nodes = preorder(tree.get());
      ASSERT_EQ(nodes.size(), size_t(num_nodes));
      ASSERT_EQ(others.size(), g.get_num_arcs() - (num_nodes - 1));

      std::set<std::string> cuts;
      std::set<std::pair<std::string, std::string>> bridges;
      for (size_t i = 0; i < nodes.size(); ++i)
        {
          Df_Low_Tree_Node * p = nodes[i];
          const Df_Low_Key & k = p->get_key();
          ASSERT_EQ(k.df, long(i)); // preorder
          ASSERT_EQ(k.low >= 0, p != tree.get());

          size_t children = 0;
          bool cut = false;
          for (auto c = p->get_left_child(); c != nullptr; c = c->get_right_sibling())
            {
              ++children;
              cut = cut or c->get_key().low >= k.df;
              if (c->get_key().low > k.df)
                bridges.insert(std::minmax(k.label, c->get_key().label));
            }
          if (p == tree.get() ? children > 1 : cut)
            cuts.insert(k.label);
        }

      others.for_each([] (const Df_Low_Non_Tree_Arc & a)
      { // from a descendant to its ancestor
        bool ancestor = false;
        for (auto q = a.first->get_parent(); q != nullptr and not ancestor; q = q->get_parent())
          ancestor = q == a.second;
        EXPECT_TRUE(ancestor) << a.first->get_key().label << " " << a.second->get_key().label;
      });

      std::set<std::string> expected_cuts;
      compute_cut_nodes(g, src).for_each([&] (Node * p)
      {
        expected_cuts.insert(std::to_string(p->get_info()));
      });
      std::set<std::pair<std::string, std::string>> expected_bridges;
      find_bridges(g, src).for_each([&] (Arc * a)
      {
        expected_bridges.insert(std::minmax(std::to_string(g.get_src_node(a)->get_info()),
                                            std::to_string(g.get_tgt_node(a)->get_info())));
      });

      EXPECT_EQ(cuts, expected_cuts) << "round " << round;
      EXPECT_EQ(bridges, expected_bridges) << "round " << round;
    }
}
