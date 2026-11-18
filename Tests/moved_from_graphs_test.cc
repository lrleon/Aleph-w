
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

/** @file moved_from_graphs_test.cc
 *  @brief The moved-from tests (moved_from.H) of the tree and graph
 *         algorithms: LCA, heavy-light and centroid decompositions, tree
 *         DP, Mo on trees, Floyd-Warshall, the index of the arcs into the
 *         nodes, the Hungarian assignment and the simplex solvers.
 *
 *  The objects are built on small graphs that live to the end of the
 *  tests, so that no object outlives the graph whose nodes it keeps.
 */

#include "moved_from.H"

#include <Floyd_Warshall.H>
#include <HLD.H>
#include <Hungarian.H>
#include <LCA.H>
#include <Simplex.H>
#include <Tree_DP.H>
#include <Tree_Decomposition.H>
#include <tpl_array.H>
#include <tpl_dynMat.H>
#include <tpl_graph.H>
#include <tpl_mo_on_trees.H>
#include <tpl_tree_node.H>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Gnode = Graph::Node;
  using Dgraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Dnode = Dgraph::Node;
  using Tnode = Tree_Node<int>;

  constexpr size_t No_Id = std::numeric_limits<size_t>::max();

  // The tree of n nodes in which the node i is a child of the node
  // (i - 1) / 2 and has the info i % 7
  struct Test_Tree
  {
    Graph g;
    Array<Gnode *> nodes;

    explicit Test_Tree(const size_t n)
    {
      for (size_t i = 0; i < n; ++i)
        nodes.append(g.insert_node(static_cast<int>(i % 7)));
      for (size_t i = 1; i < n; ++i)
        g.insert_arc(nodes((i - 1) / 2), nodes(i), 1);
    }
  };

  // The tree of n nodes, built once: it outlives the objects built on it
  // and those that receive their state
  const Test_Tree & tree_of(const size_t n)
  {
    static DynList<Test_Tree> trees;
    for (auto it = trees.get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr().nodes.size() == n)
        return it.get_curr();
    return trees.append(Test_Tree(n));
  }

  // The same tree of Tree_Node, which frees its nodes
  struct Test_Node_Tree
  {
    Array<Tnode *> nodes;

    explicit Test_Node_Tree(const size_t n)
    {
      for (size_t i = 0; i < n; ++i)
        nodes.append(new Tnode(static_cast<int>(i % 7)));
      for (size_t i = 1; i < n; ++i)
        nodes((i - 1) / 2)->insert_rightmost_child(nodes(i));
    }

    // the source of a move is left without nodes, and frees none
    Test_Node_Tree(Test_Node_Tree &&) = default;

    ~Test_Node_Tree()
    {
      if (not nodes.is_empty())
        destroy_tree(nodes(0));
    }
  };

  const Test_Node_Tree & node_tree_of(const size_t n)
  {
    static DynList<Test_Node_Tree> trees;
    for (auto it = trees.get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr().nodes.size() == n)
        return it.get_curr();
    return trees.append(Test_Node_Tree(n));
  }

  // The cycle of n nodes and n arcs, i -> (i + 1) % n of weight 1 + i % 3
  struct Test_Digraph
  {
    Dgraph g;
    Array<Dnode *> nodes;

    explicit Test_Digraph(const size_t n)
    {
      for (size_t i = 0; i < n; ++i)
        nodes.append(g.insert_node(static_cast<int>(i)));
      for (size_t i = 0; i < n; ++i)
        g.insert_arc(nodes(i), nodes((i + 1) % n), static_cast<int>(1 + i % 3));
    }
  };

  const Test_Digraph & digraph_of(const size_t n)
  {
    static DynList<Test_Digraph> digraphs;
    for (auto it = digraphs.get_it(); it.has_curr(); it.next_ne())
      if (it.get_curr().nodes.size() == n)
        return it.get_curr();
    return digraphs.append(Test_Digraph(n));
  }

  // A node of a digraph that no index has
  Dnode * stranger()
  {
    static Dgraph g;
    static Dnode * const p = g.insert_node(0);
    return p;
  }
} // namespace

template <> struct Moved_From_Case<Binary_Lifting_LCA<Graph>>
{
  using L = Binary_Lifting_LCA<Graph>;

  static L make(const size_t n) { return L(tree_of(n).g); }

  static bool is_empty(const L & l)
  {
    return l.is_empty() and l.size() == 0 and l.num_levels() == 0 and l.root() == nullptr;
  }

  static void use(L & l)
  {
    for (size_t id = 0; id < l.size(); ++id)
      {
        EXPECT_EQ(l.id_of(l.node_of(id)), id);
        EXPECT_EQ(l.lca_id(l.root_id(), id), l.root_id());
        EXPECT_EQ(l.distance_id(l.root_id(), id), l.depth_of_id(id));
        EXPECT_EQ(l.kth_ancestor_id(id, l.depth_of_id(id)), l.root_id());
        EXPECT_TRUE(id == l.root_id() or l.is_ancestor_id(l.parent_id(id), id));
      }
    EXPECT_THROW((void) l.node_of(l.size()), std::out_of_range);
  }
};

template <> struct Moved_From_Case<Euler_RMQ_LCA<Graph>>
{
  using L = Euler_RMQ_LCA<Graph>;

  static L make(const size_t n) { return L(tree_of(n).g); }

  static bool is_empty(const L & l)
  {
    return l.is_empty() and l.size() == 0 and l.euler_tour_size() == 0 and l.root() == nullptr;
  }

  static void use(L & l)
  {
    for (size_t id = 0; id < l.size(); ++id)
      {
        EXPECT_EQ(l.id_of(l.node_of(id)), id);
        EXPECT_EQ(l.lca_id(l.root_id(), id), l.root_id());
        EXPECT_EQ(l.distance_id(l.root_id(), id), l.depth_of_id(id));
        EXPECT_TRUE(id == l.root_id() or l.lca_id(l.parent_id(id), id) == l.parent_id(id));
      }
    EXPECT_EQ(l.euler_tour_size(), l.is_empty() ? 0 : 2 * l.size() - 1);
    EXPECT_EQ(l.euler_tour().size(), l.euler_tour_size());
    EXPECT_THROW((void) l.node_of(l.size()), std::out_of_range);
  }
};

template <> struct Moved_From_Case<HLD_Sum<Graph, int>>
{
  using H = HLD_Sum<Graph, int>;

  static H make(const size_t n)
  {
    return H(tree_of(n).g, [](Gnode * p) { return p->get_info(); });
  }

  static bool is_empty(const H & h)
  {
    return h.is_empty() and h.size() == 0 and h.num_chains() == 0 and h.root() == nullptr;
  }

  static void use(H & h)
  {
    int total = 0;
    for (size_t id = 0; id < h.size(); ++id)
      {
        EXPECT_EQ(h.id_of(h.node_of(id)), id);
        EXPECT_EQ(h.lca_id(h.root_id(), id), h.root_id());
        EXPECT_EQ(h.path_query_id(id, id), h.get_value_at_id(id));
        h.point_update_id(id, h.get_value_at_id(id) + 1);
        total += h.get_value_at_id(id);
        (void) h.path_query_id(h.root_id(), id);
      }
    EXPECT_TRUE(h.is_empty() or h.subtree_query_id(h.root_id()) == total);
    EXPECT_THROW((void) h.node_of(h.size()), std::out_of_range);
  }
};

template <> struct Moved_From_Case<Heavy_Light_Decomposition<Graph>>
{
  using H = Heavy_Light_Decomposition<Graph>;

  static H make(const size_t n) { return H(tree_of(n).g); }

  static bool is_empty(const H & h)
  {
    return h.is_empty() and h.size() == 0 and h.root() == nullptr;
  }

  static void use(H & h)
  {
    for (size_t id = 0; id < h.size(); ++id)
      {
        EXPECT_EQ(h.id_of(h.node_of(id)), id);
        EXPECT_LT(h.position_of_id(id), h.size());
        const auto range = h.subtree_range_id(id);
        EXPECT_EQ(range.right - range.left + 1, h.subtree_size_of_id(id));
        EXPECT_EQ(h.lca_id(h.root_id(), id), h.root_id());
        EXPECT_EQ(h.distance_id(h.root_id(), id), h.depth_of_id(id));
        EXPECT_LT(h.head_id(id), h.size());
        size_t segments = 0;
        h.for_each_path_segment_id(h.root_id(), id,
                                   [&segments](size_t, size_t, bool) { ++segments; });
        EXPECT_GT(segments, 0u);
      }
    EXPECT_THROW((void) h.node_of(h.size()), std::out_of_range);
  }
};

template <> struct Moved_From_Case<HLD_Path_Query<Graph, int, Aleph::plus<int>>>
{
  using Q = HLD_Path_Query<Graph, int, Aleph::plus<int>>;

  static Q make(const size_t n) { return Q(tree_of(n).g, 0); }

  static bool is_empty(const Q & q)
  {
    return q.is_empty() and q.size() == 0 and q.decomposition().size() == 0;
  }

  static void use(Q & q)
  {
    int total = 0;
    for (size_t id = 0; id < q.size(); ++id)
      {
        q.update_node_id(id, 1);
        EXPECT_EQ(q.query_path_id(id, id), q.get_node_id(id));
        total += q.get_node_id(id);
      }
    EXPECT_TRUE(q.is_empty() or q.query_subtree_id(q.decomposition().root_id()) == total);
  }
};

template <> struct Moved_From_Case<Centroid_Decomposition<Graph>>
{
  using C = Centroid_Decomposition<Graph>;

  static C make(const size_t n) { return C(tree_of(n).g); }

  static bool is_empty(const C & c)
  {
    return c.is_empty() and c.size() == 0 and c.centroid_root_id() == No_Id
      and c.centroid_root() == nullptr;
  }

  static void use(C & c)
  {
    EXPECT_EQ(c.centroid_root() == nullptr, c.is_empty());
    EXPECT_LE(c.max_centroid_level(), c.size());
    for (size_t id = 0; id < c.size(); ++id)
      {
        EXPECT_EQ(c.centroid_ancestors_of_id(id).size(), c.centroid_path_length_of_id(id));
        EXPECT_TRUE(c.is_centroid_ancestor_id(c.centroid_root_id(), id));
        EXPECT_NE(c.distance_to_centroid_id(id, c.centroid_root_id()), No_Id);
        size_t k = 0;
        c.for_each_centroid_ancestor_id(id, [&k](size_t, size_t, size_t) { ++k; });
        EXPECT_EQ(k, c.centroid_path_length_of_id(id));
      }
  }
};

// The size of the subtree of each node
template <> struct Moved_From_Case<Tree_DP<Graph, long>>
{
  using D = Tree_DP<Graph, long>;

  static D make(const size_t n)
  {
    const Test_Tree & t = tree_of(n);
    return D(t.g, n == 0 ? nullptr : t.nodes(0), [](Gnode *) { return 1L; },
             [](Gnode *, const long & acc, Gnode *, const long & child) { return acc + child; });
  }

  static bool is_empty(const D & d) { return d.size() == 0 and d.values().size() == 0; }

  static void use(D & d)
  {
    long largest = 0;
    for (size_t id = 0; id < d.size(); ++id)
      {
        Gnode * p = d.node_of(id);
        EXPECT_EQ(d.id_of(p), id);
        EXPECT_EQ(d.value(p), d.values()[id]);
        largest = std::max(largest, d.value(p));
      }
    EXPECT_EQ(largest, static_cast<long>(d.size())); // the subtree of the root
    EXPECT_EQ(d.values().size(), d.size());
  }
};

// The eccentricity of each node: its largest distance to another one
template <> struct Moved_From_Case<Reroot_DP<Graph, long>>
{
  using R = Reroot_DP<Graph, long>;

  static R make(const size_t n)
  {
    const Test_Tree & t = tree_of(n);
    return R(t.g, n == 0 ? nullptr : t.nodes(0), 0L, [](Gnode *) { return 0L; },
             [](const long & a, const long & b) { return std::max(a, b); },
             [](Gnode *, Gnode *, const long & v) { return v + 1; });
  }

  static bool is_empty(const R & r) { return r.size() == 0 and r.values().size() == 0; }

  static void use(R & r)
  {
    for (size_t id = 0; id < r.size(); ++id)
      {
        Gnode * p = r.node_of(id);
        EXPECT_EQ(r.id_of(p), id);
        EXPECT_EQ(r.value(p), r.values()[id]);
        EXPECT_LT(r.value(p), static_cast<long>(r.size()));
      }
    EXPECT_EQ(r.values().size(), r.size());
  }
};

// The engine gives no access to its nodes: they are those of the tree of
// its size, on which make() built it
template <> struct Moved_From_Case<Distinct_Count_Mo_On_Trees<Graph>>
{
  using M = Distinct_Count_Mo_On_Trees<Graph>;

  static M make(const size_t n)
  {
    const Test_Tree & t = tree_of(n);
    return M(t.g, n == 0 ? nullptr : t.nodes(0));
  }

  static bool is_empty(const M & m) { return m.is_empty() and m.size() == 0; }

  static void use(M & m)
  {
    const Test_Tree & t = tree_of(m.size());
    Array<Gnode *> roots;
    Array<std::pair<Gnode *, Gnode *>> pairs;
    for (size_t i = 0; i < m.size(); i += 1 + m.size() / 8)
      {
        roots.append(t.nodes(i));
        pairs.append(std::make_pair(t.nodes(0), t.nodes(i)));
      }
    EXPECT_EQ(m.subtree_solve(roots).size(), roots.size());
    EXPECT_EQ(m.path_solve(pairs).size(), pairs.size());
    // the values are i % 7
    EXPECT_TRUE(m.is_empty() or m.subtree_solve({t.nodes(0)})(0) == std::min<size_t>(m.size(), 7));
  }
};

// As the previous one, on a tree of Tree_Node, of one node at least: the
// constructor demands a root
template <> struct Moved_From_Case<Distinct_Count_Mo_On_Tree_Node<int>>
{
  using M = Distinct_Count_Mo_On_Tree_Node<int>;

  static M make(const size_t n) { return M(node_tree_of(std::max<size_t>(n, 1)).nodes(0)); }

  static bool is_empty(const M & m) { return m.is_empty() and m.size() == 0; }

  static void use(M & m)
  {
    const Test_Node_Tree & t = node_tree_of(m.size());
    Array<Tnode *> roots;
    Array<std::pair<Tnode *, Tnode *>> pairs;
    for (size_t i = 0; i < m.size(); i += 1 + m.size() / 8)
      {
        roots.append(t.nodes(i));
        pairs.append(std::make_pair(t.nodes(i), t.nodes(m.size() - 1)));
      }
    EXPECT_EQ(m.subtree_solve(roots).size(), roots.size());
    EXPECT_EQ(m.path_solve(pairs).size(), pairs.size());
    EXPECT_TRUE(m.is_empty() or m.subtree_solve({t.nodes(0)})(0) == std::min<size_t>(m.size(), 7));
  }
};

// n^2 distances: a cycle of at most 30 nodes
template <> struct Moved_From_Case<Floyd_All_Shortest_Paths<Dgraph>>
{
  using F = Floyd_All_Shortest_Paths<Dgraph>;

  static F make(const size_t n) { return F(digraph_of(std::clamp<size_t>(n, 1, 30)).g); }

  static bool is_empty(const F & f)
  {
    return f.get_nodes_ref().size() == 0 and f.get_dist_mat().rows() == 0
      and f.get_path_mat().rows() == 0 and not f.has_negative_cycle();
  }

  static void use(F & f)
  {
    const long n = static_cast<long>(f.get_nodes_ref().size());
    for (long i = 0; i < n; ++i)
      for (long j = 0; j < n; ++j)
        {
          const Path<Dgraph> path = f.get_min_path(i, j);
          EXPECT_EQ(path.get_first_node(), f.select_node(i));
          EXPECT_EQ(path.size() == 1, i == j); // every node reaches the others
        }
    // a matrix index past the nodes is out of range, whatever their number
    EXPECT_THROW((void) f.get_min_path(n, 0), std::out_of_range);
  }
};

// The cycle of n nodes has n arcs: the index of n arcs is that of the cycle
// of n nodes
template <> struct Moved_From_Case<In_Arcs_Index<Dgraph>>
{
  using I = In_Arcs_Index<Dgraph>;

  static I make(const size_t n) { return I(digraph_of(n).g); }

  static bool is_empty(const I & x) { return x.size() == 0; }

  static void use(I & x)
  {
    // whatever the index, it does not find a node of another digraph
    EXPECT_THROW((void) x.in_degree(stranger()), std::domain_error);

    const Test_Digraph & d = digraph_of(x.size());
    size_t total = 0;
    for (size_t i = 0; i < d.nodes.size(); ++i)
      {
        size_t k = 0;
        for (auto it = x.get_it(d.nodes(i)); it.has_curr(); it.next_ne(), ++k)
          EXPECT_EQ(d.g.get_tgt_node(it.get_curr_ne()), d.nodes(i));
        EXPECT_EQ(k, x.in_degree(d.nodes(i)));
        total += k;
      }
    EXPECT_EQ(total, x.size());
  }
};

// A square problem of at most 40 rows
template <> struct Moved_From_Case<Hungarian_Assignment<long>>
{
  using H = Hungarian_Assignment<long>;

  static long cost(const size_t i, const size_t j) { return static_cast<long>((i * 31 + j * 17) % 97); }

  static H make(const size_t n)
  {
    const size_t rows = std::clamp<size_t>(n, 1, 40);
    DynMatrix<long> c(rows, rows, 0);
    for (size_t i = 0; i < rows; ++i)
      for (size_t j = 0; j < rows; ++j)
        c.write(i, j, cost(i, j));
    return H(c);
  }

  static bool is_empty(const H & h)
  {
    return h.rows() == 0 and h.cols() == 0 and h.dimension() == 0 and h.get_total_cost() == 0
      and h.get_row_assignments().size() == 0 and h.get_col_assignments().size() == 0;
  }

  static void use(H & h)
  {
    long total = 0;
    for (size_t i = 0; i < h.rows(); ++i)
      {
        const long j = h.get_assignment(i);
        ASSERT_GE(j, 0);
        EXPECT_EQ(h.get_col_assignments()[static_cast<size_t>(j)], static_cast<long>(i));
        total += cost(i, static_cast<size_t>(j));
      }
    EXPECT_EQ(total, h.get_total_cost());
    EXPECT_EQ(h.get_assignments().size(), h.rows());
    EXPECT_THROW((void) h.get_assignment(h.rows()), std::out_of_range);
  }
};

// At most 50 variables, under a restriction on their sum; use() adds
// another one and solves the program, while it is not solved
template <> struct Moved_From_Case<Simplex<double>>
{
  using S = Simplex<double>;

  static void add_sum_restriction(S & s)
  {
    auto row = Array<double>::create(s.get_num_vars() + 1);
    for (size_t j = 0; j < s.get_num_vars(); ++j)
      row(j) = 1.0 + j % 2;
    row(s.get_num_vars()) = 10.0;
    (void) s.put_restriction(&row.base());
  }

  static S make(const size_t n)
  {
    S s(std::clamp<size_t>(n, 1, 50));
    for (size_t j = 0; j < s.get_num_vars(); ++j)
      s.put_objetive_function_coef(j, 1.0 + j % 3);
    add_sum_restriction(s);
    return s;
  }

  static bool is_empty(const S & s)
  {
    return s.get_num_vars() == 0 and s.get_num_restrictions() == 0
      and s.get_state() == S::Not_Solved;
  }

  static void use(S & s)
  {
    const size_t n = s.get_num_vars();
    for (size_t j = 0; j < n; ++j)
      {
        s.put_objetive_function_coef(j, 1.0 + j % 3);
        EXPECT_EQ(s.get_objetive_function_coef(j), 1.0 + j % 3);
      }
    for (size_t r = 0; r < s.get_num_restrictions(); ++r)
      for (size_t j = 0; j < n; ++j)
        EXPECT_GT(s.get_restriction_coef(r, j), 0.0);
    EXPECT_THROW(s.put_objetive_function_coef(n, 1.0), std::out_of_range);
    if (s.get_state() == S::Not_Solved)
      {
        add_sum_restriction(s);
        s.prepare_linear_program();
        EXPECT_EQ(s.solve(), S::Solved);
      }
    s.load_solution();
    EXPECT_TRUE(s.verify_solution());
  }
};

// At most 50 variables and two constraints
template <> struct Moved_From_Case<RevisedSimplex<double>>
{
  using S = RevisedSimplex<double>;

  static S make(const size_t n) { return S(std::clamp<size_t>(n, 1, 50), 2); }

  static bool is_empty(const S & s)
  {
    return s.get_num_vars() == 0 and s.get_num_constraints() == 0
      and s.get_state() == S::Not_Solved;
  }

  static void use(S & s)
  {
    const size_t n = s.get_num_vars();
    for (size_t j = 0; j < n; ++j)
      s.set_objective(j, 1.0 + j % 3);
    for (size_t i = 0; i < s.get_num_constraints(); ++i)
      {
        for (size_t j = 0; j < n; ++j)
          s.set_constraint(i, j, 1.0 + (i + j) % 2);
        s.set_rhs(i, 10.0);
      }
    EXPECT_THROW(s.set_objective(n, 1.0), std::out_of_range);
    // solve() runs once: a second use finds the program solved
    EXPECT_EQ(s.get_state() == S::Not_Solved ? s.solve() : s.get_state(), S::Solved);
    for (size_t j = 0; j < n; ++j)
      EXPECT_GE(s.get_solution(j), 0.0);
    EXPECT_TRUE(s.verify_solution());
  }
};

using Graphs = ::testing::Types<Binary_Lifting_LCA<Graph>, Euler_RMQ_LCA<Graph>,
                                HLD_Sum<Graph, int>, Heavy_Light_Decomposition<Graph>,
                                HLD_Path_Query<Graph, int, Aleph::plus<int>>,
                                Centroid_Decomposition<Graph>, Tree_DP<Graph, long>,
                                Reroot_DP<Graph, long>, Distinct_Count_Mo_On_Trees<Graph>,
                                Distinct_Count_Mo_On_Tree_Node<int>,
                                Floyd_All_Shortest_Paths<Dgraph>, In_Arcs_Index<Dgraph>,
                                Hungarian_Assignment<long>, Simplex<double>,
                                RevisedSimplex<double>>;
INSTANTIATE_TYPED_TEST_SUITE_P(Graphs, MovedFrom, Graphs);
