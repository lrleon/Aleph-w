
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
 * @file graph_kept_filter_test.cc
 * @brief The filters that the algorithm objects keep.
 *
 * These classes took their filters by reference, also when the filter was
 * a temporary, such as the default argument: a named object referred to a
 * filter that was gone when it ran. A filter given as an lvalue is still
 * used where it is; one given as an rvalue is now moved into the object.
 */

#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <eulerian.H>
#include <hamiltonian.H>
#include <single_graph.H>
#include <topological_sort.H>
#include <tpl_find_path.H>
#include <tpl_graph.H>
#include <tpl_graph_indexes.H>
#include <tpl_test_acyclique.H>

using namespace Aleph;

namespace
{
  using G = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using D = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  constexpr int Live = 0x5eed;

  // A filter that shows every item, unless hide is set. Its destructor
  // wipes its mark, so that a call after it is gone is seen. The iterators
  // copy it.
  template <class Item>
  struct Marked_Filter
  {
    static inline long stale_calls = 0;
    volatile int mark = Live;
    bool hide = false;

    Marked_Filter() = default;
    Marked_Filter(const Marked_Filter & f) noexcept : mark(f.mark), hide(f.hide) {}
    Marked_Filter & operator = (const Marked_Filter & f) noexcept
    {
      mark = f.mark;
      hide = f.hide;
      return *this;
    }
    ~Marked_Filter() { mark = 0; }

    bool operator()(Item) noexcept
    {
      if (mark != Live)
        ++stale_calls;
      return not hide;
    }
  };

  using Arc_Filter = Marked_Filter<G::Arc *>;
  using Node_Filter = Marked_Filter<G::Node *>;

  // The complete graph on four nodes, which has a hamiltonian cycle; and
  // two dags of four nodes, each with an only topological order: that of
  // d, w[3] ... w[0], is not that of its nodes, which Q_Topological_Sort
  // would give without its arcs; that of e, x[0] ... x[3], is not the one
  // that Topological_Sort would give without them
  struct Graphs
  {
    G g;
    std::vector<G::Node *> v;
    D d, e;
    std::vector<D::Node *> w, x;

    Graphs()
    {
      for (int i = 0; i < 4; ++i)
        {
          v.push_back(g.insert_node(i));
          w.push_back(d.insert_node(i));
          x.push_back(e.insert_node(i));
        }
      for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
          g.insert_arc(v[i], v[j], i + j);
      for (int i = 3; i > 0; --i)
        d.insert_arc(w[i], w[i - 1], i);
      for (int i = 0; i + 1 < 4; ++i)
        e.insert_arc(x[i], x[i + 1], i);
    }

    std::vector<D::Node *> d_sorted() const { return {w[3], w[2], w[1], w[0]}; }
    std::vector<D::Node *> e_sorted() const { return {x[0], x[1], x[2], x[3]}; }
  };

  std::vector<D::Node *> as_vector(const DynDlist<D::Node *> & l)
  {
    std::vector<D::Node *> v;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      v.push_back(it.get_curr());
    return v;
  }

  struct KeptFilter : public ::testing::Test
  {
    void SetUp() override
    {
      Arc_Filter::stale_calls = 0;
      Node_Filter::stale_calls = 0;
    }

    void TearDown() override
    {
      EXPECT_EQ(Arc_Filter::stale_calls, 0);
      EXPECT_EQ(Node_Filter::stale_calls, 0);
    }
  };
}

// The objects are named, and their filters are temporaries: the default
// argument or one given
TEST_F(KeptFilter, AnObjectKeepsTheFilterGivenAsAnRvalue)
{
  Graphs t;

  Find_Path_Depth_First<G, Node_Arc_Iterator, Arc_Filter> dfs_path;
  Path<G> path(t.g);
  EXPECT_TRUE(dfs_path(t.g, t.v[0], t.v[3], path));

  Find_Path_Breadth_First<G, Node_Arc_Iterator, Arc_Filter> bfs_path{Arc_Filter()};
  Path<G> bfs(t.g);
  EXPECT_TRUE(bfs_path(t.g, t.v[0], t.v[3], bfs));

  Directed_Find_Path<D, Out_Iterator, Arc_Filter> directed(t.d);
  EXPECT_FALSE(directed.dfs(t.w[3], t.w[0]).is_empty());
  EXPECT_FALSE(directed.bfs(t.w[3], t.w[0]).is_empty());

  Is_Graph_Acyclique<G, Arc_Filter> acyclique{Arc_Filter()};
  EXPECT_FALSE(acyclique(t.g));
  Has_Cycle<G, Arc_Filter> has_cycle;
  EXPECT_TRUE(has_cycle(t.g));

  Topological_Sort<D, Out_Iterator, Arc_Filter> sort;
  DynDlist<D::Node *> order;
  sort(t.e, order);
  EXPECT_EQ(as_vector(order), t.e_sorted());
  Q_Topological_Sort<D, Out_Iterator, Arc_Filter> q_sort{Arc_Filter()};
  DynDlist<D::Node *> q_order;
  q_sort(t.d, q_order);
  EXPECT_EQ(as_vector(q_order), t.d_sorted());

  Test_Hamiltonian_Sufficiency<G, Node_Filter, Arc_Filter> ore;
  EXPECT_TRUE(ore(t.g));
  Test_Dirac_Condition<G, Node_Filter, Arc_Filter> dirac{Node_Filter(), Arc_Filter()};
  EXPECT_TRUE(dirac(t.g));

  Test_Single_Graph<G, Node_Filter, Arc_Filter> single;
  single(t.g);

  G triangle;
  auto a = triangle.insert_node(0);
  auto b = triangle.insert_node(1);
  auto c = triangle.insert_node(2);
  triangle.insert_arc(a, b, 0);
  triangle.insert_arc(b, c, 1);
  triangle.insert_arc(c, a, 2);
  Test_Eulerian<G, Node_Filter, Arc_Filter> eulerian;
  EXPECT_TRUE(eulerian(triangle));
  Find_Eulerian_Path<G, Node_Filter, Arc_Filter> euler_path;
  EXPECT_EQ(euler_path(triangle).path.size(), 3u);
}

// An object uses the filter given as an lvalue where it is: what changes in
// it after the object was built counts. Here it hides every arc.
TEST_F(KeptFilter, AnObjectUsesTheFilterGivenAsAnLvalue)
{
  Graphs t;

  Arc_Filter f1;
  Find_Path_Depth_First<G, Node_Arc_Iterator, Arc_Filter> dfs_path(f1);
  f1.hide = true;
  Path<G> path(t.g);
  EXPECT_FALSE(dfs_path(t.g, t.v[0], t.v[3], path));

  Arc_Filter f2;
  Find_Path_Breadth_First<G, Node_Arc_Iterator, Arc_Filter> bfs_path(f2);
  f2.hide = true;
  Path<G> bfs(t.g);
  EXPECT_FALSE(bfs_path(t.g, t.v[0], t.v[3], bfs));

  Arc_Filter f3;
  Directed_Find_Path<D, Out_Iterator, Arc_Filter> directed(t.d, f3);
  f3.hide = true;
  EXPECT_TRUE(directed.dfs(t.w[3], t.w[0]).is_empty());

  // a count of 0 arcs leaves out the test of the count: they search
  Arc_Filter f4;
  Is_Graph_Acyclique<G, Arc_Filter> acyclique(f4);
  f4.hide = true;
  EXPECT_TRUE(acyclique(t.g, 0));

  Arc_Filter f5;
  Has_Cycle<G, Arc_Filter> has_cycle(f5);
  f5.hide = true;
  EXPECT_FALSE(has_cycle(t.g, 0));

  Arc_Filter f6;
  Topological_Sort<D, Out_Iterator, Arc_Filter> sort(f6);
  f6.hide = true;
  DynDlist<D::Node *> order;
  sort(t.e, order);
  EXPECT_NE(as_vector(order), t.e_sorted());

  Arc_Filter f7;
  Q_Topological_Sort<D, Out_Iterator, Arc_Filter> q_sort(f7);
  f7.hide = true;
  DynDlist<D::Node *> q_order;
  q_sort(t.d, q_order);
  EXPECT_NE(as_vector(q_order), t.d_sorted());
}

// The copy of an object that keeps a moved filter has its own; that of the
// object was gone when the copy ran
TEST_F(KeptFilter, TheCopyOfAnObjectHasItsOwnFilter)
{
  Graphs t;
  std::optional<Topological_Sort<D, Out_Iterator, Arc_Filter>> sort;
  sort.emplace(Arc_Filter());
  auto copy = *sort;
  sort.reset();

  DynDlist<D::Node *> order;
  copy(t.e, order);
  EXPECT_EQ(as_vector(order), t.e_sorted());
}

// The indexes call their filters only while they are built
TEST_F(KeptFilter, TheIndexesCallTheirFiltersWhileTheyAreBuilt)
{
  Graphs t;
  Nodes_Index<G, Dft_Node_Cmp<G>, Treap, Node_Filter> nodes(t.g, Dft_Node_Cmp<G>(),
                                                           Node_Filter());
  EXPECT_EQ(nodes.size(), 4u);

  Node_Filter f;
  f.hide = true;
  Dft_Node_Cmp<G> cmp;
  Nodes_Index<G, Dft_Node_Cmp<G>, Treap, Node_Filter> by_lvalue(t.g, cmp, f);
  EXPECT_EQ(by_lvalue.size(), 0u);

  Arcs_Index<G, Dft_Arc_Cmp<G>, Treap, Arc_Filter> arcs(t.g, Dft_Arc_Cmp<G>(),
                                                       Arc_Filter());
  EXPECT_EQ(arcs.size(), 6u);
}
