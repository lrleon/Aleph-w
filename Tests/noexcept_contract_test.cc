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
 *  on the type that it takes by default, of five kinds: a noexcept swap
 *  of its own and moves that throw, a move assignment that throws, a copy
 *  that throws, a swap of its own that throws, and a move constructor
 *  that throws with no swap of its own. The operations of those types that
 *  may throw count their calls, and throw while a test arms them. For each
 *  special member the test checks that it declares noexcept exactly when
 *  it runs none of them (a move constructor may also allocate), and then,
 *  armed, that it either completes or throws their exception and leaves
 *  both objects usable; a swap or a move assignment that throws must leave
 *  both objects as they were.
 *
 *  A swap exchanges the members of the types of the user before the
 *  storage, and, if one of them throws, those before it back: the classes
 *  with two or more types of the user are instantiated with all of them of
 *  the fourth kind, and their swaps throw at each of them in turn. The
 *  engines are also instantiated with lattices whose swap throws, which go
 *  in the same sequence. When a swap that puts one back throws too, the
 *  objects may be neither as they were nor exchanged, but they can be
 *  destroyed and assigned: the swaps also throw at each pair of them.
 *
 *  A class is one more line in Contract_Classes, with a case: the class
 *  with its types of the user made by A, an object of about n items, and
 *  what an ordinary user does with one.
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <mutex>
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
#include <tpl_dynMapTree.H>
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

  // The swaps of the kind (d) are numbered from one while armed: those
  // whose numbers are throw_at and throw_also throw. Two of them make a swap
  // that puts back what another exchanged throw too
  std::atomic<long> throw_at{1};
  std::atomic<long> throw_also{0};
  std::atomic<long> swaps_armed{0};

  void risky_swap()
  {
    ++risky_calls;
    if (not armed.load())
      return;
    const long k = ++swaps_armed;
    ah_runtime_error_if(k == throw_at or k == throw_also) << adversarial_what;
  }

  // Arms the adversarial types, with the swaps numbered first and second
  // to throw (none second if it is 0)
  void arm_swaps(const long first, const long second = 0)
  {
    swaps_armed = 0;
    throw_at = first;
    throw_also = second;
    armed = true;
  }

  // The tag of each live object of the kind (d), by its address
  std::mutex tags_mutex;
  DynMapTree<std::uintptr_t, long> live_tags;
  std::atomic<long> last_tag{0};

  void set_tag(const void * p, const long tag)
  {
    const std::lock_guard<std::mutex> lock(tags_mutex);
    const auto key = reinterpret_cast<std::uintptr_t>(p);
    if (auto * pair = live_tags.search(key); pair != nullptr)
      pair->second = tag;
    else
      (void) live_tags.insert(key, tag);
  }

  void drop_tag(const void * p)
  {
    const std::lock_guard<std::mutex> lock(tags_mutex);
    const auto key = reinterpret_cast<std::uintptr_t>(p);
    if (live_tags.search(key) != nullptr)
      live_tags.remove_key(key);
  }

  // The tags of the objects of the kind (d) that x holds in its own bytes,
  // in the order of their addresses: whose types of the user x has
  template <class T>
  std::string tags_of(const T & x)
  {
    const std::lock_guard<std::mutex> lock(tags_mutex);
    const auto lo = reinterpret_cast<std::uintptr_t>(&x);
    std::string tags;
    live_tags.for_each([&tags, lo] (const auto & p)
                       {
                         if (p.first >= lo and p.first < lo + sizeof(T))
                           tags += std::to_string(p.second) + " ";
                       });
    return tags;
  }

  // (d) A swap of its own that throws, while armed, when throw_at reaches
  // it, before it changes anything, and copies and moves that do not. Each
  // object has a tag, which its copies and moves take and its swap
  // exchanges, so that tags_of() tells whose types of the user an object
  // holds. Built from its base without explicit, for the values that an
  // operation returns
  template <class Base>
  struct Swap_Throws : Base
  {
    long tag = ++last_tag;

    Swap_Throws() noexcept { set_tag(this, tag); }
    Swap_Throws(const Base & b) noexcept : Base(b) { set_tag(this, tag); }
    Swap_Throws(const Swap_Throws & o) noexcept : Base(o), tag(o.tag) { set_tag(this, tag); }
    Swap_Throws(Swap_Throws && o) noexcept : Base(std::move(o)), tag(o.tag) { set_tag(this, tag); }
    ~Swap_Throws() { drop_tag(this); }

    Swap_Throws & operator = (const Swap_Throws & o) noexcept
    {
      Base::operator = (o);
      set_tag(this, tag = o.tag);
      return *this;
    }

    Swap_Throws & operator = (Swap_Throws && o) noexcept
    {
      Base::operator = (std::move(o));
      set_tag(this, tag = o.tag);
      return *this;
    }

    friend void swap(Swap_Throws & a, Swap_Throws & b)
    {
      risky_swap();
      std::swap(static_cast<Base &>(a), static_cast<Base &>(b));
      std::swap(a.tag, b.tag);
      set_tag(&a, a.tag);
      set_tag(&b, b.tag);
    }
  };

  // (e) A move constructor that throws before it moves anything, a move
  // assignment that does not, and no swap of its own: std::swap() throws
  // when it builds its temporary
  template <class Base>
  struct Move_Ctor_Throws : Base
  {
    Move_Ctor_Throws() = default;
    explicit Move_Ctor_Throws(const Base & b) noexcept : Base(b) {}
    Move_Ctor_Throws(const Move_Ctor_Throws & o) noexcept : Base(o) {}
    Move_Ctor_Throws(Move_Ctor_Throws && o) : Base((risky_operation(), static_cast<Base &&>(o))) {}

    Move_Ctor_Throws & operator = (const Move_Ctor_Throws & o) noexcept
    {
      Base::operator = (o);
      return *this;
    }

    Move_Ctor_Throws & operator = (Move_Ctor_Throws && o) noexcept
    {
      Base::operator = (std::move(o));
      return *this;
    }
  };

  template <template <class> class A> constexpr const char * kind_name = "";
  template <> constexpr const char * kind_name<Swap_Noexcept> = "Swap_Noexcept";
  template <> constexpr const char * kind_name<Assign_Throws> = "Assign_Throws";
  template <> constexpr const char * kind_name<Copy_Throws> = "Copy_Throws";
  template <> constexpr const char * kind_name<Swap_Throws> = "Swap_Throws";
  template <> constexpr const char * kind_name<Move_Ctor_Throws> = "Move_Ctor_Throws";

  // A value of the user, for the classes that keep one of their own, as
  // an identity, in a member
  struct Int
  {
    int v = 0;

    Int() = default;
    Int(const int x) noexcept : v(x) {}

    friend bool operator == (const Int & a, const Int & b) noexcept { return a.v == b.v; }
  };

  struct Int_Plus
  {
    Int operator () (const Int & a, const Int & b) const noexcept { return a.v + b.v; }
  };

  struct Int_Less
  {
    bool operator () (const Int & a, const Int & b) const noexcept { return a.v < b.v; }
  };

  int value_of(const int x) noexcept { return x; }
  int value_of(const Int & x) noexcept { return x.v; }

  // The value x of the type V of a case: an int, or one made from an Int
  template <class V>
  V value_from(const int x)
  {
    if constexpr (std::is_same_v<V, int>)
      return x;
    else
      return V(Int(x));
  }

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

  // A lattice whose swap may throw, as the swaps of the kind (d), which
  // LatticeLike allows: the engines exchange it in the same sequence as
  // their types of the user
  template <class L>
  struct Swap_Throws_Lattice : L
  {
    using L::L;
    Swap_Throws_Lattice(const L & l) : L(l) {}
    Swap_Throws_Lattice(L && l) : L(std::move(l)) {}

    void swap(Swap_Throws_Lattice & o)
    {
      risky_swap();
      L::swap(o);
    }
  };

  using Throwing_Grid = Swap_Throws_Lattice<Grid>;

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

  // With tags pending below the root, which a policy swapped without its
  // arrays would apply as the other one does
  template <template <class> class A> struct Lazy_Segment
  {
    static constexpr const char * name = "Gen_Lazy_Segment_Tree";
    using type = Gen_Lazy_Segment_Tree<A<Add_Sum_Policy<int>>>;
    static type make(const size_t n)
    {
      type t(values_of(n));
      if (n > 2)
        t.update(1, n - 2, 3);
      return t;
    }
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

  // A dynamic array of values of the user, whose swap its own swap() runs
  template <template <class> class A> struct Dyn_Array
  {
    static constexpr const char * name = "DynArray";
    using V = A<Int>;
    using type = DynArray<V>;
    static type make(const size_t n)
    {
      type a;
      for (const int v : values_of(n))
        (void) a.append(value_from<V>(v));
      return a;
    }
    static void use(type & a)
    {
      for (size_t i = 0; i < a.size(); ++i)
        EXPECT_LE(value_of(a(i)), 100);
      (void) a.append(value_from<V>(-1));
      EXPECT_EQ(value_of(a(a.size() - 1)), -1);
    }
  };

  // A heap of values of the user, whose swap that of the heap runs
  template <template <class> class A> struct Heap_Value
  {
    static constexpr const char * name = "DynArrayHeap_Value";
    using V = A<Int>;
    using type = DynArrayHeap<V, Int_Less>;

    template <class H>
    static H make_with(const size_t n)
    {
      H h;
      for (const int v : values_of(n))
        (void) h.insert(value_from<V>(v));
      return h;
    }

    template <class H>
    static void use_with(H & h)
    {
      size_t count = 0;
      h.for_each([&count] (const V &) { ++count; });
      EXPECT_EQ(count, h.size());
      (void) h.insert(value_from<V>(-1));
      EXPECT_EQ(value_of(h.getMin()), -1);
    }

    static type make(const size_t n) { return make_with<type>(n); }
    static void use(type & h) { use_with(h); }
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
    static constexpr bool assignment_empties = true;  // the destination, before it swaps
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
        EXPECT_EQ(value_of(h.path_query_id(id, id)), value_of(h.get_value_at_id(id)));
        total += value_of(h.get_value_at_id(id));
      }
    EXPECT_TRUE(h.is_empty() or value_of(h.subtree_query_id(h.root_id())) == total);
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
    using V = std::decay_t<decltype(q.get_node_id(0))>;
    int total = 0;
    for (size_t id = 0; id < q.size(); ++id)
      {
        q.update_node_id(id, value_from<V>(1));
        EXPECT_EQ(value_of(q.query_path_id(id, id)), value_of(q.get_node_id(id)));
        total += value_of(q.get_node_id(id));
      }
    EXPECT_TRUE(q.is_empty()
                or value_of(q.query_subtree_id(q.decomposition().root_id())) == total);
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

  // The classes with two or more types of the user, with all of them of
  // the kind A, so that their swaps can throw at each of them in turn

  template <template <class> class A> struct Fenwick_Both
  {
    static constexpr const char * name = "Gen_Fenwick_Tree_Both";
    using type = Gen_Fenwick_Tree<int, A<Aleph::plus<int>>, A<Aleph::minus<int>>>;
    static type make(const size_t n) { return type(values_of(n)); }
    static void use(type & t) { use_ranges(t); }
  };

  template <class V>
  Array<V> values_of_type(const size_t n)
  {
    const Array<int> v = values_of(n);
    auto a = Array<V>::create(n);
    for (size_t i = 0; i < n; ++i)
      a(i) = value_from<V>(v(i));
    return a;
  }

  template <template <class> class A> struct Segment_Both
  {
    static constexpr const char * name = "Gen_Segment_Tree_Both";
    using V = A<Int>;
    using type = Gen_Segment_Tree<V, A<Int_Plus>>;
    static type make(const size_t n) { return type(values_of_type<V>(n), V(Int(0))); }
    static void use(type & t) { use_ranges(t); }
  };

  template <template <class> class A> struct HLD_All
  {
    static constexpr const char * name = "Gen_HLD_All";
    using V = A<Int>;
    using type = Gen_HLD<Graph, V, A<Int_Plus>, A<Show_Arc>>;
    static type make(const size_t n)
    {
      return type(tree_of(n).g, [] (Gnode * p) { return V(Int(p->get_info())); }, V(Int(0)));
    }
    static void use(type & h) { use_hld(h); }
  };

  template <template <class> class A> struct Path_Query_All
  {
    static constexpr const char * name = "Gen_HLD_Path_Query_All";
    using V = A<Int>;
    using type = Gen_HLD_Path_Query<Graph, V, A<Int_Plus>, A<Show_Arc>>;
    static type make(const size_t n) { return type(tree_of(n).g, V(Int(0))); }
    static void use(type & q) { use_path_query(q); }
  };

  template <template <class> class A> struct Life_All
  {
    static constexpr const char * name = "Synchronous_Engine_All";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using type = Synchronous_Engine<Grid, Rule, Nh>;
    static type make(const size_t n)
    {
      type e(life_grid(n), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Async_All
  {
    static constexpr const char * name = "Async_Engine_All";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using Scheme = A<Random_Asynchronous_Update<>>;
    using type = Async_Engine<Grid, Rule, Nh, Scheme>;
    static type make(const size_t n)
    {
      type e(life_grid(n), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}),
             Scheme(Random_Asynchronous_Update<>(7, 10)));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Parallel_All
  {
    static constexpr const char * name = "Parallel_Synchronous_Engine_All";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using type = Parallel_Synchronous_Engine<Grid, Rule, Nh>;
    static type make(const size_t n)
    {
      Parallel_Engine_Config cfg;
      cfg.pool = &small_pool();
      cfg.num_partitions = 2;
      cfg.min_parallel_cells = 0;
      type e(life_grid(n), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}), cfg);
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Field_All
  {
    static constexpr const char * name = "Multi_Field_Engine_All";
    using Rule = A<Field_Slice_Rule<0, Game_Of_Life_Rule>>;
    using Nh = A<Moore<2, 1>>;
    using type = Multi_Field_Engine<Two_Fields, Rule, Nh>;
    static type make(const size_t n)
    {
      Two_Fields lat(extents_of(n), std::tuple{0, 7});
      type e(std::move(lat), Rule(Field_Slice_Rule<0, Game_Of_Life_Rule>(make_game_of_life_rule())),
             Nh(Moore<2, 1>{}));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  // A heap whose comparators and values are both of the kind A

  template <template <class> class A> struct Heap_Both
  {
    static constexpr const char * name = "DynArrayHeap_Both";
    using V = A<Int>;
    using type = DynArrayHeap<V, A<Int_Less>>;
    static type make(const size_t n) { return Heap_Value<A>::template make_with<type>(n); }
    static void use(type & h) { Heap_Value<A>::template use_with<type>(h); }
  };

  // The engines with a lattice whose swap throws as the types of the user

  template <template <class> class A> struct Life_Lattice
  {
    static constexpr const char * name = "Synchronous_Engine_Lattice";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using type = Synchronous_Engine<Throwing_Grid, Rule, Nh>;
    static type make(const size_t n)
    {
      type e(Throwing_Grid(life_grid(n)), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Async_Lattice
  {
    static constexpr const char * name = "Async_Engine_Lattice";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using Scheme = A<Random_Asynchronous_Update<>>;
    using type = Async_Engine<Throwing_Grid, Rule, Nh, Scheme>;
    static type make(const size_t n)
    {
      type e(Throwing_Grid(life_grid(n)), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}),
             Scheme(Random_Asynchronous_Update<>(7, 10)));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Parallel_Lattice
  {
    static constexpr const char * name = "Parallel_Synchronous_Engine_Lattice";
    using Rule = A<Game_Of_Life_Rule>;
    using Nh = A<Moore<2, 1>>;
    using type = Parallel_Synchronous_Engine<Throwing_Grid, Rule, Nh>;
    static type make(const size_t n)
    {
      Parallel_Engine_Config cfg;
      cfg.pool = &small_pool();
      cfg.num_partitions = 2;
      cfg.min_parallel_cells = 0;
      type e(Throwing_Grid(life_grid(n)), Rule(make_game_of_life_rule()), Nh(Moore<2, 1>{}), cfg);
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Field_Lattice
  {
    static constexpr const char * name = "Multi_Field_Engine_Lattice";
    using Rule = A<Field_Slice_Rule<0, Game_Of_Life_Rule>>;
    using Nh = A<Moore<2, 1>>;
    using Fields = Swap_Throws_Lattice<Two_Fields>;
    using type = Multi_Field_Engine<Fields, Rule, Nh>;
    static type make(const size_t n)
    {
      type e(Fields(Two_Fields(extents_of(n), std::tuple{0, 7})),
             Rule(Field_Slice_Rule<0, Game_Of_Life_Rule>(make_game_of_life_rule())), Nh(Moore<2, 1>{}));
      e.step();
      return e;
    }
    static auto print(const type & e) { return engine_print(e); }
    static void use(type & e) { use_engine(e); }
  };

  template <template <class> class A> struct Graph_Lattice_Case
  {
    static constexpr const char * name = "Graph_Synchronous_Engine_Lattice";
    using Rule = A<Game_Of_Life_Rule>;
    using Nodes = Swap_Throws_Lattice<Graph_Lattice<int>>;
    using type = Graph_Synchronous_Engine<Nodes, Rule>;
    static type make(const size_t n)
    {
      Graph_Lattice<int> g(make_path_graph_adjacency(n, true), 0);
      for (size_t i = 0; i < n; i += 3)
        g.set_node(i, 1);
      type e(Nodes(std::move(g)), Rule(make_game_of_life_rule()));
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

  // Each class, with the five kinds of types of the user
  template <template <template <class> class> class... Cases>
  using Contract_Classes = ::testing::Types<Contract<Cases, Swap_Noexcept>...,
                                            Contract<Cases, Assign_Throws>...,
                                            Contract<Cases, Copy_Throws>...,
                                            Contract<Cases, Swap_Throws>...,
                                            Contract<Cases, Move_Ctor_Throws>...>;

  using Classes = Contract_Classes<
    Fenwick_Plus, Fenwick_Minus, Segment, Lazy_Segment, Sparse, Disjoint_Sparse,
    Cartesian, Euler_Tour, Cartesian_RMQ, Heap, Heap_Value, Dyn_Array, B_Tree_Case,
    BPlus_Tree_Case, Hash_Map,
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

  // Whether the move assignment empties the destination before it swaps:
  // then one that throws leaves it empty
  template <class Case>
  constexpr bool assignment_empties()
  {
    if constexpr (requires { Case::assignment_empties; })
      return Case::assignment_empties;
    else
      return false;
  }

  // What a swap or a move assignment that throws must leave of an object:
  // what it had, with its own types of the user
  template <class Case, class T, class P>
  void expect_as_it_was(const T & x, const P & print, const std::string & tags, const char * what,
                        const char * which)
  {
    EXPECT_EQ(print_of<Case>(x), print) << what << " threw and left " << which << " changed";
    EXPECT_EQ(tags_of(x), tags) << what << " threw and left " << which
                                << " with types of the user that are not its own";
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
  // build c; expect checks what it did, from the prints of a and b before,
  // and on_throw what it leaves when it throws, from their prints and their
  // tags before. Disarmed, it counts the operations of the types of the
  // user that may throw that op runs: if op is noexcept it must run none,
  // since armed it would end the program, and if it is not, it must run
  // some, unless allows_other says that something else may throw. Armed,
  // op throws if and only if it runs one, and then the exception of the
  // type of the user, and it leaves the objects usable
  template <class Case, class Op, class Expect, class On_Throw>
  void check_contract(const char * what, const bool is_noexcept, const bool allows_other,
                      Op op, Expect expect, On_Throw on_throw)
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
    const std::string tags_a = tags_of(a);
    const std::string tags_b = tags_of(b);
    std::optional<T> c;
    bool threw = false;
    arm_swaps(1);
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
    else
      on_throw(a, b, print_a, print_b, tags_a, tags_b);
    use_all<Case>(a, b, c);
  }
} // namespace

template <class T> class Noexcept_Contract : public ::testing::Test {};

TYPED_TEST_SUITE(Noexcept_Contract, Classes, Contract_Name);

TYPED_TEST(Noexcept_Contract, MoveConstructor)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  // A move constructor that throws may leave its source empty
  check_contract<Case>("the move constructor", std::is_nothrow_move_constructible_v<T>,
                       moves_allocate<Case>(),
                       [] (T & a, T &, std::optional<T> & c) { c.emplace(std::move(a)); },
                       [] (T &, T &, std::optional<T> & c, const auto & pa, const auto &)
                         {
                           ASSERT_TRUE(c.has_value());
                           EXPECT_EQ(print_of<Case>(*c), pa);
                         },
                       [] (T &, T &, const auto &, const auto &, const auto &, const auto &) {});
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
                           },
                         [] (T & a, T & b, const auto & pa, const auto & pb, const auto & ta,
                             const auto & tb)
                           {
                             expect_as_it_was<Case>(a, pa, ta, "the move assignment", "the source");
                             if constexpr (assignment_empties<Case>())
                               {
                                 EXPECT_EQ(b.size(), 0u) << "the destination is not empty";
                                 EXPECT_EQ(tags_of(b), tb);
                               }
                             else
                               expect_as_it_was<Case>(b, pb, tb, "the move assignment",
                                                      "the destination");
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
                           },
                         [] (T & a, T & b, const auto & pa, const auto & pb, const auto & ta,
                             const auto & tb)
                           {
                             expect_as_it_was<Case>(a, pa, ta, "swap()", "this object");
                             expect_as_it_was<Case>(b, pb, tb, "swap()", "the other");
                           });
  else
    GTEST_SKIP() << "the class has no swap()";
}

namespace
{
  // The classes with two or more types of the user, all of the kind (d)
  using Rollback_Classes = ::testing::Types<
    Contract<Fenwick_Both, Swap_Throws>, Contract<Segment_Both, Swap_Throws>,
    Contract<HLD_All, Swap_Throws>, Contract<Path_Query_All, Swap_Throws>,
    Contract<Life_All, Swap_Throws>, Contract<Async_All, Swap_Throws>,
    Contract<Parallel_All, Swap_Throws>, Contract<Field_All, Swap_Throws>,
    Contract<Heap_Both, Swap_Throws>, Contract<Life_Lattice, Swap_Throws>,
    Contract<Async_Lattice, Swap_Throws>, Contract<Parallel_Lattice, Swap_Throws>,
    Contract<Field_Lattice, Swap_Throws>, Contract<Graph_Lattice_Case, Swap_Throws>>;

  // op, a swap or a move assignment, throws at each swap of a type of the
  // user that it runs, in turn: each time it must leave both objects as
  // they were, with their types of the user, and usable
  template <class Case, class Op>
  void check_rollback(const char * what, Op op)
  {
    using T = typename Case::type;
    size_t swaps = 0;
    {
      T a = Case::make(20);
      T b = Case::make(5);
      const size_t before = risky_calls;
      op(a, b);
      swaps = risky_calls - before;
    }
    EXPECT_GE(swaps, 2u) << what << " exchanges fewer than two types of the user";
    for (size_t k = 1; k <= swaps; ++k)
      {
        T a = Case::make(20);
        T b = Case::make(5);
        const auto print_a = print_of<Case>(a);
        const auto print_b = print_of<Case>(b);
        const std::string tags_a = tags_of(a);
        const std::string tags_b = tags_of(b);
        bool threw = false;
        arm_swaps(static_cast<long>(k));
        try
          {
            op(a, b);
          }
        catch (const std::exception & e)
          {
            threw = true;
            EXPECT_TRUE(is_adversarial(e)) << what << " threw " << e.what();
          }
        armed = false;
        EXPECT_TRUE(threw) << what << " did not throw at its swap " << k;
        SCOPED_TRACE(std::string(what) + " threw at its swap " + std::to_string(k));
        expect_as_it_was<Case>(a, print_a, tags_a, what, "an object");
        expect_as_it_was<Case>(b, print_b, tags_b, what, "the other");
        Case::use(a);
        Case::use(b);
      }
  }

  // op throws at each swap i of a type of the user and again at a swap j
  // that puts back one before it: then the objects may be neither as they
  // were nor exchanged (the classes say so), but they can be destroyed as
  // they are, or assigned and then used as any other
  template <class Case, class Op>
  void check_double_failure(const char * what, Op op)
  {
    using T = typename Case::type;
    size_t swaps = 0;
    {
      T a = Case::make(20);
      T b = Case::make(5);
      const size_t before = risky_calls;
      op(a, b);
      swaps = risky_calls - before;
    }
    size_t twice = 0;
    for (long i = 2; i <= static_cast<long>(swaps); ++i)
      for (long j = i + 1; j < 2 * i; ++j)  // the i - 1 swaps that put back those before i
        for (const bool assign : {false, true})
          {
            SCOPED_TRACE(std::string(what) + " threw at its swaps " + std::to_string(i) + " and " +
                         std::to_string(j));
            T a = Case::make(20);
            T b = Case::make(5);
            arm_swaps(i, j);
            try
              {
                op(a, b);
                ADD_FAILURE() << what << " did not throw";
              }
            catch (const std::exception & e)
              {
                EXPECT_TRUE(is_adversarial(e)) << what << " threw " << e.what();
              }
            armed = false;
            if (swaps_armed >= j)
              ++twice;
            if (assign)
              {
                a = Case::make(20);
                b = Case::make(5);
                Case::use(a);
                Case::use(b);
              }
          }
    EXPECT_GT(twice, 0u) << what << " never threw while it put back what it exchanged";
  }
} // namespace

template <class T> class Noexcept_Contract_Rollback : public ::testing::Test {};

TYPED_TEST_SUITE(Noexcept_Contract_Rollback, Rollback_Classes, Contract_Name);

TYPED_TEST(Noexcept_Contract_Rollback, Swap)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  check_rollback<Case>("swap()", [] (T & a, T & b) { a.swap(b); });
}

TYPED_TEST(Noexcept_Contract_Rollback, MoveAssignment)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  check_rollback<Case>("the move assignment", [] (T & a, T & b) { b = std::move(a); });
}

TYPED_TEST(Noexcept_Contract_Rollback, SwapThatThrowsTwice)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  check_double_failure<Case>("swap()", [] (T & a, T & b) { a.swap(b); });
}

TYPED_TEST(Noexcept_Contract_Rollback, MoveAssignmentThatThrowsTwice)
{
  using Case = typename TypeParam::C;
  using T = typename Case::type;
  check_double_failure<Case>("the move assignment", [] (T & a, T & b) { b = std::move(a); });
}

namespace
{
  // A rule that owns a resource, an array that its move leaves to the
  // other rule. Its move assignment throws while armed, and it has no swap
  // of its own: std::swap() leaves it moved from when it throws, without
  // its resource. Its steps run with it or without it
  std::atomic<size_t> steps_without_resource{0};

  template <class Base>
  struct Resource_Rule : Base
  {
    Array<int> resource = Array<int>::create(1);

    explicit Resource_Rule(const Base & b) : Base(b) {}
    Resource_Rule(const Resource_Rule &) = default;
    Resource_Rule(Resource_Rule &&) noexcept = default;
    Resource_Rule & operator = (const Resource_Rule &) = default;

    Resource_Rule & operator = (Resource_Rule && o)
    {
      risky_operation();
      Base::operator = (std::move(o));
      resource = std::move(o.resource);
      return *this;
    }

    template <class... Args>
    auto operator () (const Args &... args) const
      -> decltype(std::declval<const Base &>()(args...))
    {
      if (resource.is_empty())
        ++steps_without_resource;
      return Base::operator () (args...);
    }
  };

  // A swap of engines that throws in that of their rules leaves each
  // engine with its own lattices, and both step
  template <class Case>
  void check_rule_with_a_resource()
  {
    using T = typename Case::type;
    T a = Case::make(20);
    T b = Case::make(5);
    const auto print_a = print_of<Case>(a);
    const auto print_b = print_of<Case>(b);
    bool threw = false;
    armed = true;
    try
      {
        a.swap(b);
      }
    catch (const std::exception & e)
      {
        threw = true;
        EXPECT_TRUE(is_adversarial(e)) << Case::name << " threw " << e.what();
      }
    armed = false;
    EXPECT_TRUE(threw) << Case::name;
    EXPECT_EQ(print_of<Case>(a), print_a) << Case::name << " exchanged its lattices";
    EXPECT_EQ(print_of<Case>(b), print_b) << Case::name << " exchanged its lattices";
    Case::use(a);
    Case::use(b);
  }
} // namespace

// The rule of an engine that owns a resource, and whose move assignment
// throws inside std::swap(): the lattices are not exchanged without the
// rules, and both engines go on stepping
TEST(Noexcept_Contract_Engines, RuleThatOwnsAResource)
{
  check_rule_with_a_resource<Life<Resource_Rule>>();
  check_rule_with_a_resource<Async_Life<Resource_Rule>>();
  check_rule_with_a_resource<Parallel_Life<Resource_Rule>>();
  check_rule_with_a_resource<Field_Life<Resource_Rule>>();
  check_rule_with_a_resource<Graph_Life<Resource_Rule>>();
}

// The move constructor of DynSkipList builds its comparison as a copy of
// that of the source: with a comparison whose move assignment throws, it
// runs none of the operations of the comparison that may throw, and ends
// without throwing
TEST(Noexcept_Contract_Skip_List, MoveConstructorWhenTheComparisonAssignmentThrows)
{
  using Case = Skip_List<Assign_Throws>;
  using L = typename Case::type;
  L src = Case::make(20);
  std::optional<L> dst;
  const size_t before = risky_calls;
  armed = true;
  EXPECT_NO_THROW(dst.emplace(std::move(src)));
  armed = false;
  EXPECT_EQ(risky_calls - before, 0u) << "the move constructor ran an operation of the "
                                      << "comparison that may throw";
  ASSERT_TRUE(dst.has_value());
  EXPECT_EQ(dst->size(), 20u);
  Case::use(*dst);
  Case::use(src);
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

namespace
{
  // The moves of a set over a tree whose swap runs that of its comparison
  // (Rand_Tree and Splay_Tree_Rk): with a comparison whose own swap throws
  // they throw, instead of ending the program, and leave the source as it
  // was; with Aleph::less they stay noexcept
  template <template <typename, class> class Tree>
  void check_set_moves(const char * tree)
  {
    using Set = DynSetTree<int, Tree, Swap_Throws<Aleph::less<int>>>;
    EXPECT_TRUE((std::is_nothrow_move_constructible_v<DynSetTree<int, Tree>>)) << tree;
    EXPECT_TRUE((std::is_nothrow_move_assignable_v<DynSetTree<int, Tree>>)) << tree;
    // armed, a noexcept move would end the program
    ASSERT_FALSE(std::is_nothrow_move_constructible_v<Set>) << tree;
    ASSERT_FALSE(std::is_nothrow_move_assignable_v<Set>) << tree;

    Set a;
    for (int i = 0; i < 20; ++i)
      (void) a.insert(i);
    arm_swaps(1);
    EXPECT_THROW({ Set b(std::move(a)); }, std::runtime_error) << tree;
    armed = false;
    EXPECT_EQ(a.size(), 20u) << tree;
    EXPECT_TRUE(a.verify()) << tree;

    Set c;
    (void) c.insert(-1);
    arm_swaps(1);
    EXPECT_THROW(c = std::move(a), std::runtime_error) << tree;
    armed = false;
    EXPECT_EQ(a.size(), 20u) << tree;
    EXPECT_EQ(c.size(), 0u) << tree << ": the assignment frees the elements of its destination first";
    (void) c.insert(5);
    EXPECT_TRUE(c.contains(5)) << tree;
  }
} // namespace

TEST(Noexcept_Contract_Dyn_Set, MovesOverARandTreeWhoseComparisonSwapThrows)
{
  check_set_moves<Rand_Tree>("Rand_Tree");
}

TEST(Noexcept_Contract_Dyn_Set, MovesOverARankedSplayTreeWhoseComparisonSwapThrows)
{
  check_set_moves<Splay_Tree_Rk>("Splay_Tree_Rk");
}
