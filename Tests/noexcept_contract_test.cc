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

/** @file noexcept_contract_test.cc
 *  @brief The exception contracts of the moves and the swaps of the
 *         classes that keep a type of the user.
 *
 *  A class that keeps a comparison, an operation, a policy or a filter of
 *  the user declares its move constructor, its move assignment and its
 *  swap() noexcept, or not, from what the type of the user does. A swap()
 *  whose specification asks std::is_nothrow_swappable_v, which evaluates
 *  the swap that the arguments find, but that calls std::swap(), which
 *  moves through a temporary, declares noexcept a move that may throw; a
 *  move that throws inside a noexcept function ends the program.
 *
 *  Each class is instantiated with adversarial types of the user, built
 *  on the type that it takes by default, of three kinds: a noexcept swap
 *  of its own and moves that throw, a move assignment that throws, and a
 *  copy that throws. The operations of those types that may throw count
 *  their calls, and throw while a test arms them. For each special member
 *  the test checks that it declares noexcept exactly when it runs none of
 *  them (a move constructor may also allocate), and then, armed, that it
 *  either completes or throws their exception and leaves both objects
 *  usable.
 *
 *  A class is one more line in Contract_Classes, with a case: the class
 *  with its types of the user made by A, an object of about n items, and
 *  what an ordinary user does with one.
 */

#include <atomic>
#include <cstddef>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <ah-errors.H>
#include <Floyd_Warshall.H>
#include <HLD.H>
#include <LCA.H>
#include <Tree_DP.H>
#include <Tree_Decomposition.H>
#include <thread_pool.H>
#include <tpl_b_tree.H>
#include <tpl_bplus_tree.H>
#include <tpl_ca_async_engine.H>
#include <tpl_ca_engine.H>
#include <tpl_ca_graph_automaton.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_multi_field_engine.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_multi_field_rule.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_parallel_engine.H>
#include <tpl_ca_rule.H>
#include <tpl_ca_storage.H>
#include <tpl_cartesian_tree.H>
#include <tpl_disjoint_sparse_table.H>
#include <tpl_dynArrayHeap.H>
#include <tpl_dynSkipList.H>
#include <tpl_fenwick_tree.H>
#include <tpl_graph.H>
#include <tpl_mo_on_trees.H>
#include <tpl_persistent_hash_map.H>
#include <tpl_rand_tree.H>
#include <tpl_segment_tree.H>
#include <tpl_sparse_table.H>
#include <tpl_tree_node.H>

using namespace Aleph;
using namespace Aleph::CA;

namespace
{
  // The operations of the adversarial types that may throw count their
  // calls, and throw while a test arms them. Atomic: the parallel engine
  // may copy its rule in its threads
  std::atomic<bool> armed{false};
  std::atomic<size_t> risky_calls{0};

  constexpr const char * adversarial_what = "an adversarial type of the user threw";

  void risky_operation()
  {
    ++risky_calls;
    ah_runtime_error_if(armed.load()) << adversarial_what;
  }

  bool is_adversarial(const std::exception & e)
  {
    return std::string(e.what()).find(adversarial_what) != std::string::npos;
  }

  // (a) A noexcept swap of its own, which the arguments find, and a move
  // constructor and a move assignment that throw
  template <class Base>
  struct Swap_Noexcept : Base
  {
    Swap_Noexcept() = default;
    explicit Swap_Noexcept(const Base & b) noexcept : Base(b) {}
    Swap_Noexcept(const Swap_Noexcept & o) noexcept : Base(o) {}
    Swap_Noexcept(Swap_Noexcept && o) : Base(std::move(o)) { risky_operation(); }

    Swap_Noexcept & operator = (const Swap_Noexcept & o) noexcept
    {
      Base::operator = (o);
      return *this;
    }

    Swap_Noexcept & operator = (Swap_Noexcept && o)
    {
      risky_operation();
      Base::operator = (std::move(o));
      return *this;
    }

    friend void swap(Swap_Noexcept & a, Swap_Noexcept & b) noexcept
    {
      std::swap(static_cast<Base &>(a), static_cast<Base &>(b));
    }
  };

  // (b) A move assignment that throws, and a move constructor that does not
  template <class Base>
  struct Assign_Throws : Base
  {
    Assign_Throws() = default;
    explicit Assign_Throws(const Base & b) noexcept : Base(b) {}
    Assign_Throws(const Assign_Throws & o) noexcept : Base(o) {}
    Assign_Throws(Assign_Throws && o) noexcept : Base(std::move(o)) {}

    Assign_Throws & operator = (const Assign_Throws & o) noexcept
    {
      Base::operator = (o);
      return *this;
    }

    Assign_Throws & operator = (Assign_Throws && o)
    {
      risky_operation();
      Base::operator = (std::move(o));
      return *this;
    }
  };

  // (c) A copy that throws, and moves that do not
  template <class Base>
  struct Copy_Throws : Base
  {
    Copy_Throws() = default;
    explicit Copy_Throws(const Base & b) noexcept : Base(b) {}
    Copy_Throws(const Copy_Throws & o) : Base(o) { risky_operation(); }
    Copy_Throws(Copy_Throws && o) noexcept : Base(std::move(o)) {}

    Copy_Throws & operator = (const Copy_Throws & o)
    {
      risky_operation();
      Base::operator = (o);
      return *this;
    }

    Copy_Throws & operator = (Copy_Throws && o) noexcept
    {
      Base::operator = (std::move(o));
      return *this;
    }
  };

  template <template <class> class A> constexpr const char * kind_name = "";
  template <> constexpr const char * kind_name<Swap_Noexcept> = "Swap_Noexcept";
  template <> constexpr const char * kind_name<Assign_Throws> = "Assign_Throws";
  template <> constexpr const char * kind_name<Copy_Throws> = "Copy_Throws";

  // n values in [0, 100]: a fixed scramble, with repetitions
  Array<int> values_of(const size_t n)
  {
    auto a = Array<int>::create(n);
    for (size_t i = 0; i < n; ++i)
      a(i) = static_cast<int>((i * 37 + 11) % 101);
    return a;
  }

  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Gnode = Graph::Node;
  using Dgraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
  using Tnode = Tree_Node<int>;
  using Show_Arc = Dft_Show_Arc<Graph>;

  // The tree of n nodes in which the node i is a child of (i - 1) / 2,
  // with the info i % 7, built once: it outlives the objects built on it
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

  // The cycle of n nodes, i -> (i + 1) % n of weight 1 + i % 3
  struct Test_Digraph
  {
    Dgraph g;
    Array<Dgraph::Node *> nodes;

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

  // What an ordinary user does with a structure of ranges: read every
  // value, alone and in ranges
  template <class R>
  void use_ranges(R & r)
  {
    for (size_t i = 0; i < r.size(); ++i)
      {
        EXPECT_EQ(r.query(i, i), r.get(i));
        (void) r.query(0, i);
      }
    EXPECT_EQ(r.values().size(), r.size());
  }

  // ... and with an ordered tree
  template <class T>
  void use_ordered(T & t)
  {
    EXPECT_TRUE(t.verify());
    EXPECT_EQ(t.keys().size(), t.size());
    EXPECT_TRUE(t.insert(-1));
    EXPECT_TRUE(t.contains(-1));
    EXPECT_TRUE(t.remove(-1));
    EXPECT_TRUE(t.verify());
  }

  // ... and with a decomposition of a tree in ids, whose nodes it knows
  template <class D>
  void use_ids(D & d)
  {
    for (size_t id = 0; id < d.size(); ++id)
      EXPECT_EQ(d.id_of(d.node_of(id)), id);
    EXPECT_THROW((void) d.node_of(d.size()), std::out_of_range);
  }

  // ... and with an engine of rectangular lattices: step it and run it
  template <class Engine>
  void use_engine(Engine & e)
  {
    const size_t steps = e.steps_run();
    e.step();
    e.run(2);
    EXPECT_EQ(e.steps_run(), steps + 3);
  }

  // The cells and the steps of an engine
  template <class Engine>
  std::pair<size_t, size_t> engine_print(const Engine & e)
  {
    return {e.frame().size(), e.steps_run()};
  }

  using Extents = std::array<ca_size_t, 2>;
  using Grid = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
  using Two_Fields = Multi_Field_Lattice<Layout_AoS, OpenBoundary, 2, int, int>;

  // About n cells: one row for the smallest objects, ten rows otherwise
  Extents extents_of(const size_t n)
  {
    const ca_size_t rows = n < 10 ? 1 : 10;
    return {rows, n / rows};
  }

  // A lattice of about n cells, with a blinker where it fits
  Grid life_grid(const size_t n)
  {
    Grid g(extents_of(n), 0);
    if (g.size(1) >= 3)
      for (ca_index_t j = 0; j < 3; ++j)
        g.set({0, j}, 1);
    return g;
  }

  ThreadPool & small_pool()
  {
    static ThreadPool pool(2);
    return pool;
  }

  // The cases: Case<A> gives the class with its types of the user made
  // by A, make(n), an object of about n items, and use(x), what a user
  // does with one; print(x), what a move carries, when the class has no
  // size(); moves_allocate, when its move constructor allocates

  template <template <class> class A> struct Fenwick_Plus
  {
    static constexpr const char * name = "Gen_Fenwick_Tree_Plus";
    using type = Gen_Fenwick_Tree<int, A<Aleph::plus<int>>, Aleph::minus<int>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Fenwick_Minus
  {
    static constexpr const char * name = "Gen_Fenwick_Tree_Minus";
    using type = Gen_Fenwick_Tree<int, Aleph::plus<int>, A<Aleph::minus<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Segment
  {
    static constexpr const char * name = "Gen_Segment_Tree";
    using type = Gen_Segment_Tree<int, A<Aleph::plus<int>>>;
    static type make(const size_t n) { return type(values_of(n), 0); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Lazy_Segment
  {
    static constexpr const char * name = "Gen_Lazy_Segment_Tree";
    using type = Gen_Lazy_Segment_Tree<A<Add_Sum_Policy<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Sparse
  {
    static constexpr const char * name = "Gen_Sparse_Table";
    using type = Gen_Sparse_Table<int, A<Min_Op<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Disjoint_Sparse
  {
    static constexpr const char * name = "Gen_Disjoint_Sparse_Table";
    using type = Gen_Disjoint_Sparse_Table<int, A<Aleph::plus<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct Cartesian
  {
    static constexpr const char * name = "Gen_Cartesian_Tree";
    using type = Gen_Cartesian_Tree<int, A<Aleph::less<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t)
    {
      for (size_t i = 0; i < t.size(); ++i)
        if (const size_t p = t.parent_of(i); p != type::NONE)
          {
            EXPECT_LE(t.data_at(p), t.data_at(i));
          }
      EXPECT_EQ(t.inorder().size(), t.size());
    }
  };

  template <template <class> class A> struct Euler_Tour
  {
    static constexpr const char * name = "Gen_Euler_Tour_LCA";
    using type = Gen_Euler_Tour_LCA<int, A<Aleph::less<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & l)
    {
      EXPECT_EQ(l.euler_tour().size(), l.euler_tour_size());
      for (size_t i = 0; i < l.size(); ++i)
        EXPECT_LE(l.depth_of(l.lca(0, i)), l.depth_of(i));
      EXPECT_EQ(l.tree().size(), l.size());
    }
  };

  template <template <class> class A> struct Cartesian_RMQ
  {
    static constexpr const char * name = "Gen_Cartesian_Tree_RMQ";
    using type = Gen_Cartesian_Tree_RMQ<int, A<Aleph::less<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & q) { use_ranges(q); }
  };

  template <template <class> class A> struct Heap
  {
    static constexpr const char * name = "DynArrayHeap";
    using type = DynArrayHeap<int, A<Aleph::less<int>>>;
    static type make(const size_t n)
    {
      type h;
      for (const int v : values_of(n))
        (void) h.insert(v);
      return h;
    }
    static void use(type & h)
    {
      size_t count = 0;
      h.for_each([&count] (const int) { ++count; });
      EXPECT_EQ(count, h.size());
      (void) h.insert(-1);
      EXPECT_EQ(h.getMin(), -1);
    }
  };

  template <template <class> class A> struct B_Tree_Case
  {
    static constexpr const char * name = "Gen_B_Tree";
    using type = Gen_B_Tree<int, A<Aleph::less<int>>>;
    static type make(const size_t n)
    {
      type t;
      for (size_t i = 0; i < n; ++i)
        (void) t.insert(static_cast<int>(i));
      return t;
    }
    static void use(type & t) { use_ordered(t); }
  };

  template <template <class> class A> struct BPlus_Tree_Case
  {
    static constexpr const char * name = "Gen_BPlus_Tree";
    using type = Gen_BPlus_Tree<int, A<Aleph::less<int>>>;
    static type make(const size_t n)
    {
      type t;
      for (size_t i = 0; i < n; ++i)
        (void) t.insert(static_cast<int>(i));
      return t;
    }
    static void use(type & t) { use_ordered(t); }
  };

  template <template <class> class A> struct Hash_Map
  {
    static constexpr const char * name = "PersistentHashMap";
    using type = PersistentHashMap<int, int, A<Aleph::equal_to<int>>>;
    static type make(const size_t n)
    {
      type m;
      for (size_t i = 0; i < n; ++i)
        m = m.insert(static_cast<int>(i), static_cast<int>(i));
      return m;
    }
    static void use(type & m)
    {
      EXPECT_TRUE(m.verify());
      const Array<int> keys = m.keys();
      EXPECT_EQ(keys.size(), m.size());
      for (size_t i = 0; i < keys.size(); ++i)
        EXPECT_NE(m.find(keys(i)), nullptr);
      m = m.insert_or_assign(-1, 7);
      EXPECT_TRUE(m.contains(-1));
      EXPECT_TRUE(m.verify());
    }
  };

  template <template <class> class A> struct Skip_List
  {
    static constexpr const char * name = "DynSkipList";
    static constexpr bool moves_allocate = true;  // a header and a generator for the source
    using type = DynSkipList<int, A<std::less<int>>>;
    static type make(const size_t n)
    {
      type l(1UL);
      for (size_t i = 0; i < n; ++i)
        (void) l.insert(static_cast<int>(i));
      return l;
    }
    static void use(type & l)
    {
      size_t count = 0;
      l.for_each([&count] (const int) { ++count; });
      EXPECT_EQ(count, l.size());
      EXPECT_NE(l.insert(-1), nullptr);
      EXPECT_TRUE(l.has(-1));
      EXPECT_EQ(l.remove(-1), 1u);
    }
  };

  // A randomized tree does not own its nodes: use() frees them, as its user
  // would
  template <template <class> class A> struct Rand_Tree_Case
  {
    static constexpr const char * name = "Gen_Rand_Tree";
    static constexpr bool moves_allocate = true;  // a generator for the source
    using type = Rand_Tree<int, A<Aleph::less<int>>>;
    using Node = typename type::Node;
    static type make(const size_t n)
    {
      type t(1);
      for (size_t i = 0; i < n; ++i)
        (void) t.insert(new Node(static_cast<int>(i)));
      return t;
    }
    static void use(type & t)
    {
      EXPECT_TRUE(t.verify());
      Node * p = new Node(-1);
      EXPECT_EQ(t.insert(p), p);
      EXPECT_EQ(t.search(-1), p);
      EXPECT_TRUE(t.verify());
      destroyRec(t.getRoot());
    }
  };

  template <template <class> class A> struct Binary_Lifting
  {
    static constexpr const char * name = "Gen_Binary_Lifting_LCA";
    using type = Gen_Binary_Lifting_LCA<Graph, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g); }
    static void use(type & l)
    {
      use_ids(l);
      for (size_t id = 0; id < l.size(); ++id)
        EXPECT_EQ(l.lca_id(l.root_id(), id), l.root_id());
    }
  };

  template <template <class> class A> struct Euler_RMQ
  {
    static constexpr const char * name = "Gen_Euler_RMQ_LCA";
    using type = Gen_Euler_RMQ_LCA<Graph, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g); }
    static void use(type & l)
    {
      use_ids(l);
      for (size_t id = 0; id < l.size(); ++id)
        EXPECT_EQ(l.lca_id(l.root_id(), id), l.root_id());
      EXPECT_EQ(l.euler_tour().size(), l.euler_tour_size());
    }
  };

  // What a user does with an HLD of sums
  template <class H>
  void use_hld(H & h)
  {
    use_ids(h);
    int total = 0;
    for (size_t id = 0; id < h.size(); ++id)
      {
        EXPECT_EQ(h.path_query_id(id, id), h.get_value_at_id(id));
        total += h.get_value_at_id(id);
      }
    EXPECT_TRUE(h.is_empty() or h.subtree_query_id(h.root_id()) == total);
  }

  template <template <class> class A> struct HLD_Op
  {
    static constexpr const char * name = "Gen_HLD_Op";
    using type = Gen_HLD<Graph, int, A<Aleph::plus<int>>>;
    static type make(const size_t n)
    {
      return type(tree_of(n).g, [] (Gnode * p) { return p->get_info(); }, 0);
    }
    static void use(type & h) { use_hld(h); }
  };

  template <template <class> class A> struct HLD_Filter
  {
    static constexpr const char * name = "Gen_HLD_Filter";
    using type = Gen_HLD<Graph, int, Aleph::plus<int>, A<Show_Arc>>;
    static type make(const size_t n)
    {
      return type(tree_of(n).g, [] (Gnode * p) { return p->get_info(); }, 0);
    }
    static void use(type & h) { use_hld(h); }
  };

  template <template <class> class A> struct Heavy_Light
  {
    static constexpr const char * name = "Gen_Heavy_Light_Decomposition";
    using type = Gen_Heavy_Light_Decomposition<Graph, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g); }
    static void use(type & h)
    {
      use_ids(h);
      for (size_t id = 0; id < h.size(); ++id)
        EXPECT_LT(h.position_of_id(id), h.size());
    }
  };

  // What a user does with a path query of sums
  template <class Q>
  void use_path_query(Q & q)
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

  template <template <class> class A> struct Path_Query_Op
  {
    static constexpr const char * name = "Gen_HLD_Path_Query_Op";
    using type = Gen_HLD_Path_Query<Graph, int, A<Aleph::plus<int>>>;
    static type make(const size_t n) { return type(tree_of(n).g, 0); }
    static void use(type & q) { use_path_query(q); }
  };

  template <template <class> class A> struct Path_Query_Filter
  {
    static constexpr const char * name = "Gen_HLD_Path_Query_Filter";
    using type = Gen_HLD_Path_Query<Graph, int, Aleph::plus<int>, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g, 0); }
    static void use(type & q) { use_path_query(q); }
  };

  template <template <class> class A> struct Centroid
  {
    static constexpr const char * name = "Gen_Centroid_Decomposition";
    using type = Gen_Centroid_Decomposition<Graph, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g); }
    static void use(type & c)
    {
      for (size_t id = 0; id < c.size(); ++id)
        EXPECT_TRUE(c.is_centroid_ancestor_id(c.centroid_root_id(), id));
      EXPECT_EQ(c.centroid_root() == nullptr, c.is_empty());
    }
  };

  // The size of the subtree of each node
  template <template <class> class A> struct Tree_DP_Case
  {
    static constexpr const char * name = "Gen_Tree_DP";
    using type = Gen_Tree_DP<Graph, long, A<Show_Arc>>;
    static type make(const size_t n)
    {
      const Test_Tree & t = tree_of(n);
      return type(t.g, t.nodes(0), [] (Gnode *) { return 1L; },
                  [] (Gnode *, const long & acc, Gnode *, const long & child) { return acc + child; });
    }
    static void use(type & d)
    {
      use_ids(d);
      for (size_t id = 0; id < d.size(); ++id)
        EXPECT_EQ(d.value(d.node_of(id)), d.values()[id]);
      EXPECT_EQ(d.values().size(), d.size());
    }
  };

  // The eccentricity of each node
  template <template <class> class A> struct Reroot_DP_Case
  {
    static constexpr const char * name = "Gen_Reroot_DP";
    using type = Gen_Reroot_DP<Graph, long, A<Show_Arc>>;
    static type make(const size_t n)
    {
      const Test_Tree & t = tree_of(n);
      return type(t.g, t.nodes(0), 0L, [] (Gnode *) { return 0L; },
                  [] (const long & a, const long & b) { return std::max(a, b); },
                  [] (Gnode *, Gnode *, const long & v) { return v + 1; });
    }
    static void use(type & r)
    {
      use_ids(r);
      for (size_t id = 0; id < r.size(); ++id)
        EXPECT_LT(r.value(r.node_of(id)), static_cast<long>(r.size()));
      EXPECT_EQ(r.values().size(), r.size());
    }
  };

  // The engine gives no access to its nodes: they are those of the tree of
  // its size
  template <template <class> class A> struct Mo_On_Trees
  {
    static constexpr const char * name = "Gen_Mo_On_Trees";
    using type = Gen_Mo_On_Trees<Graph, A<Distinct_Count_Policy<int>>>;
    static type make(const size_t n)
    {
      const Test_Tree & t = tree_of(n);
      return type(t.g, t.nodes(0));
    }
    static void use(type & m)
    {
      if (m.is_empty())
        return;
      const Test_Tree & t = tree_of(m.size());
      EXPECT_EQ(m.subtree_solve({t.nodes(0)})(0), std::min<size_t>(m.size(), 7));
    }
  };

  template <template <class> class A> struct Mo_On_Tree_Node
  {
    static constexpr const char * name = "Gen_Mo_On_Tree_Node";
    using type = Gen_Mo_On_Tree_Node<int, A<Distinct_Count_Policy<int>>>;
    static type make(const size_t n) { return type(node_tree_of(n).nodes(0)); }
    static void use(type & m)
    {
      if (m.is_empty())
        return;
      const Test_Node_Tree & t = node_tree_of(m.size());
      EXPECT_EQ(m.subtree_solve({t.nodes(0)})(0), std::min<size_t>(m.size(), 7));
    }
  };

  // What a user does with the shortest paths of a cycle
  template <class F>
  void use_floyd(F & f)
  {
    const long n = static_cast<long>(f.get_nodes_ref().size());
    for (long i = 0; i < n; ++i)
      EXPECT_EQ(f.get_min_path(i, (i + 1) % n).get_first_node(), f.select_node(i));
    EXPECT_THROW((void) f.get_min_path(n, 0), std::out_of_range);
  }

  template <template <class> class A> struct Floyd_Distance
  {
    static constexpr const char * name = "Floyd_All_Shortest_Paths_Distance";
    using type = Floyd_All_Shortest_Paths<Dgraph, A<Dft_Dist<Dgraph>>>;
    static type make(const size_t n) { return type(digraph_of(std::min<size_t>(n, 10)).g); }
    static size_t print(const type & f) { return f.get_nodes_ref().size(); }
    static void use(type & f) { use_floyd(f); }
  };

  template <template <class> class A> struct Floyd_Filter
  {
    static constexpr const char * name = "Floyd_All_Shortest_Paths_Filter";
    using type = Floyd_All_Shortest_Paths<Dgraph, Dft_Dist<Dgraph>, A<Dft_Show_Arc<Dgraph>>>;
    static type make(const size_t n) { return type(digraph_of(std::min<size_t>(n, 10)).g); }
    static size_t print(const type & f) { return f.get_nodes_ref().size(); }
    static void use(type & f) { use_floyd(f); }
  };

  template <template <class> class A> struct Life
  {
    static constexpr const char * name = "Synchronous_Engine";
    using Rule = A<Game_Of_Life_Rule>;
    using type = Synchronous_Engine<Grid, Rule, Moore<2, 1>>;
    static type make(const size_t n)
    {
      type e(life_grid(n), Rule(make_game_of_life_rule()), Moore<2, 1>{});
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Async_Life
  {
    static constexpr const char * name = "Async_Engine_Rule";
    using Rule = A<Game_Of_Life_Rule>;
    using type = Async_Engine<Grid, Rule, Moore<2, 1>>;
    static type make(const size_t n)
    {
      type e(life_grid(n), Rule(make_game_of_life_rule()), Moore<2, 1>{});
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Random_Life
  {
    static constexpr const char * name = "Async_Engine_Scheme";
    using Scheme = A<Random_Asynchronous_Update<>>;
    using type = Async_Engine<Grid, Game_Of_Life_Rule, Moore<2, 1>, Scheme>;
    static type make(const size_t n)
    {
      type e(life_grid(n), make_game_of_life_rule(), Moore<2, 1>{},
             Scheme(Random_Asynchronous_Update<>(7, 10)));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Parallel_Life
  {
    static constexpr const char * name = "Parallel_Synchronous_Engine";
    using Rule = A<Game_Of_Life_Rule>;
    using type = Parallel_Synchronous_Engine<Grid, Rule, Moore<2, 1>>;
    static type make(const size_t n)
    {
      Parallel_Engine_Config cfg;
      cfg.pool = &small_pool();
      cfg.num_partitions = 2;
      cfg.min_parallel_cells = 0;
      type e(life_grid(n), Rule(make_game_of_life_rule()), Moore<2, 1>{}, cfg);
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Field_Life
  {
    static constexpr const char * name = "Multi_Field_Engine";
    using Rule = A<Field_Slice_Rule<0, Game_Of_Life_Rule>>;
    using type = Multi_Field_Engine<Two_Fields, Rule, Moore<2, 1>>;
    static type make(const size_t n)
    {
      Two_Fields lat(extents_of(n), std::tuple{0, 7});
      type e(std::move(lat), Rule(Field_Slice_Rule<0, Game_Of_Life_Rule>(make_game_of_life_rule())),
             Moore<2, 1>{});
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Graph_Life
  {
    static constexpr const char * name = "Graph_Synchronous_Engine";
    using Rule = A<Game_Of_Life_Rule>;
    using type = Graph_Synchronous_Engine<Graph_Lattice<int>, Rule>;
    static type make(const size_t n)
    {
      Graph_Lattice<int> g(make_path_graph_adjacency(n, true), 0);
      for (size_t i = 0; i < n; i += 3)
        g.set_node(i, 1);
      type e(std::move(g), Rule(make_game_of_life_rule()));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e)
    {
      const size_t steps = e.steps_run();
      e.step();
      EXPECT_EQ(e.steps_run(), steps + 1);
    }
  };

  // A class of the test with the types of the user of the kind A
  template <template <template <class> class> class Case, template <class> class A>
  struct Contract
  {
    using C = Case<A>;
    static constexpr const char * kind = kind_name<A>;
  };

  // Each class, with the three kinds of types of the user
  template <template <template <class> class> class... Cases>
  using Contract_Classes = ::testing::Types<Contract<Cases, Swap_Noexcept>...,
                                            Contract<Cases, Assign_Throws>...,
                                            Contract<Cases, Copy_Throws>...>;

  using Classes = Contract_Classes<
    Fenwick_Plus, Fenwick_Minus, Segment, Lazy_Segment, Sparse, Disjoint_Sparse,
    Cartesian, Euler_Tour, Cartesian_RMQ, Heap, B_Tree_Case, BPlus_Tree_Case, Hash_Map,
    Skip_List, Rand_Tree_Case,
    Binary_Lifting, Euler_RMQ, HLD_Op, HLD_Filter, Heavy_Light, Path_Query_Op,
    Path_Query_Filter, Centroid, Tree_DP_Case, Reroot_DP_Case, Mo_On_Trees,
    Mo_On_Tree_Node, Floyd_Distance, Floyd_Filter,
    Life, Async_Life, Random_Life, Parallel_Life, Field_Life, Graph_Life>;

  struct Contract_Name
  {
    template <class T>
    static std::string GetName(int)
    {
      return std::string(T::C::name) + "_" + T::kind;
    }
  };

  // What a move must carry to its destination: the print of the case, or
  // the size
  template <class Case, class T>
  auto print_of(const T & x)
  {
    if constexpr (requires { Case::print(x); })
      return Case::print(x);
    else
      return static_cast<size_t>(x.size());
  }

  template <class Case>
  constexpr bool moves_allocate()
  {
    if constexpr (requires { Case::moves_allocate; })
      return Case::moves_allocate;
    else
      return false;
  }

  template <class Case, class T>
  void use_all(T & a, T & b, std::optional<T> & c)
  {
    Case::use(a);
    Case::use(b);
    if (c.has_value())
      Case::use(*c);
  }

  // Checks the contract of an operation: op applies it to a and b, and may
  // build c; expect checks what it did, from the prints of a and b before.
  // Disarmed, it counts the operations of the types of the user that may
  // throw that op runs: if op is noexcept it must run none, since armed it
  // would end the program, and if it is not, it must run some, unless
  // allows_other says that something else may throw. Armed, op throws if
  // and only if it runs one, and then the exception of the type of the
  // user, and it leaves the objects usable
  template <class Case, class Op, class Expect>
  void check_contract(const char * what, const bool is_noexcept, const bool allows_other,
                      Op op, Expect expect)
  {
    using T = typename Case::type;
    size_t ran = 0;
    {
      T a = Case::make(20);
      T b = Case::make(5);
      const auto print_a = print_of<Case>(a);
      const auto print_b = print_of<Case>(b);
      std::optional<T> c;
      const size_t before = risky_calls;
      op(a, b, c);
      ran = risky_calls - before;
      expect(a, b, c, print_a, print_b);
      use_all<Case>(a, b, c);
    }
    if (is_noexcept)
      {
        EXPECT_EQ(ran, 0u) << what << " is noexcept, but runs " << ran
                           << " operations of the types of the user that may throw";
        if (ran > 0)
          return;  // armed, it would end the program
      }
    else if (not allows_other)
      {
        EXPECT_GT(ran, 0u) << what << " may throw, but runs no operation of the types of the "
                           << "user that may throw";
      }

    T a = Case::make(20);
    T b = Case::make(5);
    const auto print_a = print_of<Case>(a);
    const auto print_b = print_of<Case>(b);
    std::optional<T> c;
    bool threw = false;
    armed = true;
    try
      {
        op(a, b, c);
      }
    catch (const std::exception & e)
      {
        threw = true;
        EXPECT_TRUE(is_adversarial(e)) << what << " threw " << e.what();
      }
    armed = false;
    EXPECT_EQ(threw, ran > 0) << what;
    if (not threw)
      expect(a, b, c, print_a, print_b);
    use_all<Case>(a, b, c);
  }
} // namespace

template <class T> class Noexcept_Contract : public ::testing::Test {};

TYPED_TEST_SUITE(Noexcept_Contract, Classes, Contract_Name);

TYPED_TEST(Noexcept_Contract, MoveConstructor)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  check_contract<Case>("the move constructor", std::is_nothrow_move_constructible_v<T>,
                       moves_allocate<Case>(),
                       [] (T & a, T &, std::optional<T> & c) { c.emplace(std::move(a)); },
                       [] (T &, T &, std::optional<T> & c, const auto & pa, const auto &)
                         {
                           ASSERT_TRUE(c.has_value());
                           EXPECT_EQ(print_of<Case>(*c), pa);
                         });
}

TYPED_TEST(Noexcept_Contract, MoveAssignment)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  if constexpr (std::is_move_assignable_v<T>)
    check_contract<Case>("the move assignment", std::is_nothrow_move_assignable_v<T>, false,
                         [] (T & a, T & b, std::optional<T> &) { b = std::move(a); },
                         [] (T &, T & b, std::optional<T> &, const auto & pa, const auto &)
                           {
                             EXPECT_EQ(print_of<Case>(b), pa);
                           });
  else
    GTEST_SKIP() << "the class has no move assignment";
}

TYPED_TEST(Noexcept_Contract, Swap)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  if constexpr (requires (T & x) { x.swap(x); })
    check_contract<Case>("swap()", noexcept(std::declval<T &>().swap(std::declval<T &>())), false,
                         [] (T & a, T & b, std::optional<T> &) { a.swap(b); },
                         [] (T & a, T & b, std::optional<T> &, const auto & pa, const auto & pb)
                           {
                             EXPECT_EQ(print_of<Case>(a), pb);
                             EXPECT_EQ(print_of<Case>(b), pa);
                           });
  else
    GTEST_SKIP() << "the class has no swap()";
}

namespace
{
  // A distance whose copy throws while armed, for the infinity that the
  // move of Floyd_All_Shortest_Paths copies
  struct Number
  {
    long v = 0;

    Number() = default;
    Number(const long x) noexcept : v(x) {}
    Number(const Number & o) : v(o.v) { risky_operation(); }
    Number(Number && o) noexcept : v(o.v) {}
    Number & operator = (const Number & o) = default;
    Number & operator = (Number && o) noexcept = default;

    friend Number operator + (const Number & a, const Number & b) noexcept { return a.v + b.v; }
    friend Number operator - (const Number & a, const Number & b) noexcept { return a.v - b.v; }
    friend bool operator == (const Number & a, const Number & b) noexcept { return a.v == b.v; }
    friend auto operator <=> (const Number & a, const Number & b) noexcept { return a.v <=> b.v; }
  };

  struct Number_Distance
  {
    using Distance_Type = Number;
    Number operator () (Dgraph::Arc * a) const noexcept { return a->get_info(); }
  };
} // namespace

template <> struct std::numeric_limits<Number> : std::numeric_limits<long>
{
  static Number max() noexcept { return std::numeric_limits<long>::max(); }
  static Number lowest() noexcept { return std::numeric_limits<long>::lowest(); }
  static constexpr bool is_integer = false;
};

// The move of the solver copies its infinity, which is const: with a
// distance type whose copy throws it is not noexcept, and a copy that
// throws leaves the source without nodes, usable
TEST(Noexcept_Contract_Floyd, InfinityWhoseCopyThrows)
{
  using F = Floyd_All_Shortest_Paths<Dgraph, Number_Distance>;
  F full(digraph_of(5).g);
  const size_t before = risky_calls;
  F moved(std::move(full));
  EXPECT_GT(risky_calls - before, 0u);
  // The solver orders its nodes by their addresses: ask by the nodes
  const auto & t = digraph_of(5);
  EXPECT_EQ(moved.get_min_path(t.nodes(0), t.nodes(1)).size(), 2u);
  ASSERT_FALSE(std::is_nothrow_move_constructible_v<F>)
    << "the move copies the infinity, whose copy may throw";  // armed, it would end the program

  F src(digraph_of(5).g);
  bool threw = false;
  armed = true;
  try
    {
      F dst(std::move(src));
    }
  catch (const std::exception & e)
    {
      threw = true;
      EXPECT_TRUE(is_adversarial(e)) << e.what();
    }
  armed = false;
  EXPECT_TRUE(threw);
  EXPECT_EQ(src.get_nodes_ref().size(), 0u);
  EXPECT_THROW((void) src.get_min_path(0L, 0L), std::out_of_range);
}
